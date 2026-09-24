# Revisão do core da V2

Este documento acompanha a revisão do core da V2 por partes. Cada seção explica o código como ele está, a razão das decisões identificadas, os trade-offs envolvidos, os problemas encontrados, as correções aplicadas e o que foi deliberadamente deixado para uma etapa posterior.

Status das partes:

1. Fundações e arquitetura global — revisada.
2. Domínio — pendente.
3. Contratos da aplicação — pendente.
4. Casos de uso e regras de sincronização — pendente.
5. Infraestrutura local — pendente.
6. Integração AniList — pendente.
7. Composição, concorrência e ciclo de vida — pendente.
8. Fronteira consumida pelo frontend — pendente.
9. Testes e estabilidade — pendente.

## 1. Fundações e arquitetura global

### Escopo desta parte

Esta parte examina a estrutura de diretórios, a direção das dependências, o ponto de entrada, a composição principal e a definição dos alvos no CMake. As regras internas do domínio, o comportamento detalhado da sincronização e o contrato do controller serão aprofundados nas seções seguintes.

### Visão geral da arquitetura

O projeto adota uma arquitetura em camadas com composição explícita:

```text
main.cpp
    -> app
        -> presentation
        -> application (interfaces e casos de uso)
        -> infrastructure (implementações)
application
    -> domain
infrastructure
    -> application + domain
presentation
    -> application + domain
```

A regra mais importante não é apenas a existência das pastas, mas a direção das dependências. O domínio representa conceitos do negócio. A aplicação coordena esses conceitos e declara as portas de que precisa. A infraestrutura implementa essas portas usando SQLite, filesystem, HTTP ou GraphQL. A apresentação converte resultados da aplicação em um formato adequado ao QML. A pasta `app` é a raiz de composição e, por isso, é o único lugar em que conhecer simultaneamente interfaces e implementações concretas é esperado.

Essa direção permite substituir SQLite, AniList ou QML sem reescrever as regras centrais. O custo é uma quantidade maior de interfaces, factories e objetos pequenos. Para este projeto, o custo é justificável porque há integrações externas, persistência local, execução em threads e uma interface gráfica que evoluirão de forma independente.

### Caminho de inicialização

O `main.cpp` executa quatro tarefas:

1. cria o `QGuiApplication`;
2. solicita a construção do `ApplicationContext`;
3. conecta o coordenador de sincronização ao controller da Home;
4. registra o controller no contexto QML e carrega o módulo visual.

Essa implementação mantém o ponto de entrada relativamente pequeno. O `main.cpp` sabe que existe uma tela inicial e que seus estados acompanham a sincronização, mas não sabe abrir SQLite, carregar SQL ou construir uma requisição GraphQL. Isso evita repetir o padrão monolítico da V1.

O principal trade-off é que o ponto de entrada ainda realiza manualmente as conexões entre o coordenador e o controller. Essa ligação é legível enquanto existe apenas uma tela e um fluxo. Se várias telas passarem a reagir aos mesmos eventos, a composição deverá assumir também essas conexões ou fornecer um serviço de estado compartilhado; transformar o `main.cpp` em um catálogo crescente de `QObject::connect` recriaria um composition root informal e difícil de testar.

O `HomeScreenController` é criado na stack e registrado no QML como ponteiro. Isso é seguro no fluxo atual porque ele vive até o fim de `main`, ultrapassando a vida do `QQmlApplicationEngine`. O benefício é ownership simples, sem alocação dinâmica desnecessária. O cuidado obrigatório é nunca permitir que o engine sobreviva ao controller ou retenha esse ponteiro fora desse ciclo de vida.

### Composição e estado parcialmente disponível

`createApplicationContext()` constrói logger, banco, queries, repositories e sincronização. Em caso de falha, retorna um contexto com `initializationError`, em vez de encerrar o processo. Essa decisão permite que a janela abra e apresente um erro controlado, o que é coerente com uma aplicação desktop: uma falha no banco não deve parecer um crash silencioso.

O trade-off é que `ApplicationContext` pode existir em estado parcial. Todo consumidor precisa respeitar a nulabilidade das dependências. O método `isReady()` torna a intenção visível, mas hoje o `main.cpp` não o usa diretamente; ele repassa repository e erro ao controller. Essa abordagem é válida, desde que cada dependência opcional possua uma condição clara de disponibilidade. Estados parcialmente construídos sem uma regra explícita se tornam perigosos à medida que o contexto cresce.

A ordem dos membros do contexto também é relevante. Como C++ destrói membros na ordem inversa da declaração, o coordenador é destruído antes dos repositories, o banco e o logger. Essa ordem favorece o encerramento, pois o trabalho que usa infraestrutura deixa de existir antes das dependências. O desligamento explícito do logger em `aboutToQuit`, entretanto, pode ocorrer antes de uma sincronização ainda ativa terminar; a política completa de encerramento será tratada na parte de concorrência e ciclo de vida.

### CMake e fronteiras de build

O CMake define um executável Qt Quick, empacota QML, configuração e queries SQL como recursos, cria executáveis de teste independentes e executa `windeployqt` após o build da aplicação no Windows.

Empacotar configuração e queries essenciais como recursos torna a inicialização independente do diretório de trabalho. O trade-off é que esses arquivos passam a ser imutáveis no executável: isso é adequado para queries internas e valores padrão, mas não para configurações que o usuário deva editar em tempo de execução. A distinção entre configuração empacotada e configuração do usuário precisará permanecer explícita.

Os testes são executáveis separados. Isso mantém cada suíte pequena e simples, mas recompila fontes compartilhadas diversas vezes e torna o `CMakeLists.txt` extenso. Uma biblioteca interna por camada reduziria repetição e faria o linker validar as mesmas fronteiras usadas pela aplicação. Ainda não foi criada porque essa mudança altera substancialmente a topologia do build e deve acompanhar a revisão completa dos módulos, não uma limpeza inicial.

### Problemas encontrados e correções

#### 1. Implementação concreta na camada de aplicação — corrigido

`AniListUpdateClient` implementa `IAniListUpdateClient` usando `AniListGraphQlClient` e `GraphQlQueryStore`. Como essas duas dependências são detalhes de GraphQL e filesystem, a classe é um adapter de infraestrutura, não um caso de uso.

Manter essa classe em `application/anilist` invertia a direção desejada: a aplicação precisava incluir a infraestrutura. A classe foi movida para `infrastructure/anilist`, enquanto a interface permaneceu na aplicação. O comportamento não mudou; mudou apenas o lugar que expressa corretamente sua responsabilidade.

#### 2. Fontes repetidas no alvo principal — corrigido

O alvo `HaikenAnime` listava repetidamente implementações de merge, compactação, agrupamento e processamento de pendências. O CMake tolerava a repetição, por isso o build passava, mas o manifesto deixava de ser uma descrição confiável do alvo e aumentava o risco de conflitos em futuras edições.

As duplicatas foram removidas. A escolha de continuar listando fontes explicitamente, em vez de usar `file(GLOB)`, é intencional: a configuração só muda quando uma fonte é deliberadamente adicionada. O trade-off é manutenção manual, compensada por builds mais previsíveis.

#### 3. Caminho do Qt imposto à máquina — corrigido

O CMake aplicava `C:/Qt/6.8.3/mingw_64` com `FORCE`. Isso ajudava a máquina atual, mas ignorava qualquer `CMAKE_PREFIX_PATH` fornecido por outro desenvolvedor ou pela integração contínua.

O caminho conhecido agora é apenas um fallback quando nenhum prefixo foi informado. Para o deploy, a raiz do Qt é derivada do executável associado ao target importado `Qt6::qmake`, em vez de repetir o caminho absoluto. O projeto mantém a conveniência local sem transformar a instalação desta máquina em contrato global.

#### 4. Dependência de testes no build da aplicação — corrigido

`Qt6::Test` era exigido mesmo para configurar somente o executável final. Os testes agora são condicionados por `BUILD_TESTING`, usando o suporte padrão de `CTest`. Com `BUILD_TESTING=OFF`, a aplicação não precisa localizar o módulo Qt Test.

O padrão continua sendo testar, porque `BUILD_TESTING` nasce habilitado. A opção serve para empacotamento e ambientes mínimos; não é uma justificativa para omitir testes durante desenvolvimento.

#### 5. Downcast após apagar o tipo concreto — corrigido

A composição criava um `SqliteMediaRepository`, convertia imediatamente o ownership para `unique_ptr<IMediaRepository>` e depois usava `static_cast` para configurar o logger. Embora o cast fosse correto naquele ponto, ele dependia de conhecimento que o tipo da variável já havia apagado.

Agora a implementação concreta é configurada primeiro e só depois transferida para o ponteiro da interface. Isso elimina o cast e deixa a sequência didática: construir, configurar e publicar pela abstração.

### Dívidas identificadas, mas não corrigidas nesta parte

- `InitialSyncCoordinator` ainda constrói diretamente banco, query stores, repository, data source e serviço dentro da thread. Isso concentra composição e execução na mesma classe e dificulta testes isolados. A correção depende da revisão do ciclo de vida e das fronteiras de thread.
- A sincronização inicial usa um fixture localizado a partir de `tests/fixtures`. Isso é coerente com a fase de integração local registrada no projeto, mas não é um contrato de produção. A escolha da fonte precisa se tornar explícita na composição antes da distribuição.
- `ApplicationContext` expõe detalhes concretos de infraestrutura porque também funciona como contêiner de ownership. É aceitável no composition root, mas seu crescimento deve ser controlado; ele não deve virar um service locator acessível ao restante da aplicação.
- O `CMakeLists.txt` ainda recompila fontes compartilhadas em vários executáveis de teste. A criação de bibliotecas internas será reavaliada depois que as fronteiras das próximas partes estiverem confirmadas.
- A política de encerramento pode parar o logger antes do término de um worker. Esse ponto exige uma decisão coordenada sobre cancelamento, espera máxima e destruição, portanto pertence à revisão de concorrência.

### Resultado da parte 1

A separação macro do projeto é adequada e já evita o objeto central monolítico da V1. O principal risco encontrado foi uma violação real da direção das dependências, agora corrigida. O build ficou menos dependente da máquina atual e seu manifesto passou a representar melhor as fontes e os testes.

As correções desta parte são estruturais e não pretendem alterar regras de negócio, payloads, persistência ou apresentação.

### Validação executada

- Configuração CMake Debug com `BUILD_TESTING=ON`: concluída.
- Build completo do executável e das suítes: concluído.
- CTest: 10 de 10 testes passaram.
- Configuração CMake Release com `BUILD_TESTING=OFF`: concluída; foram gerados apenas o executável e os alvos auxiliares do módulo QML, sem executáveis de teste.
- `git diff --check`: concluído sem erro de whitespace.

O CMake informou que os headers de Vulkan não estão instalados e o `windeployqt` informou a ausência de `dxcompiler.dll` e `dxil.dll`. Esses avisos não impediram configuração, compilação, deploy das dependências Qt ou testes, e não foram causados pelas alterações desta revisão. A interface não foi aberta para validação visual nesta parte, pois nenhuma mudança de apresentação foi realizada.
