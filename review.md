# Revisão do core da V2

Este documento acompanha a revisão do core da V2 por partes. Cada seção explica o código como ele está, a razão das decisões identificadas, os trade-offs envolvidos, os problemas encontrados, as correções aplicadas e o que foi deliberadamente deixado para uma etapa posterior.

Status das partes:

1. Fundações e arquitetura global — revisada.
2. Domínio — revisado.
3. Contratos da aplicação — revisados.
4. Casos de uso e regras de sincronização — revisados.
5. Infraestrutura local — revisada.
6. Integração AniList — revisada no limite das fixtures; conexão real adiada.
7. Composição, concorrência e ciclo de vida — revisados.
8. Fronteira consumida pelo frontend — revisada.
9. Testes e estabilidade — revisados.
10. Conclusão transversal e contratos entregues — revisados.

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

A ordem dos membros do contexto também é relevante. Como C++ destrói membros na ordem inversa da declaração, o coordenador é destruído antes dos repositories, o banco e o logger. Essa ordem favorece o encerramento, pois o trabalho que usa infraestrutura deixa de existir antes das dependências. No início desta revisão, o desligamento explícito do logger em `aboutToQuit` ainda podia ocorrer antes de uma sincronização ativa terminar. A parte 7 corrigiu a ordem operacional: primeiro o coordenador encerra e aguarda seu trabalho; somente depois o logger drena a fila e para.

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
- A sincronização inicial continua usando fixture, por decisão desta fase. A parte 9 eliminou a dependência do layout físico ao empacotá-la em `:/fixtures/media-library.json`, mas fixture ainda não é contrato de produção. A escolha entre fonte local e GraphQL deverá se tornar explícita na composição antes da integração real.
- `ApplicationContext` expõe detalhes concretos de infraestrutura porque também funciona como contêiner de ownership. É aceitável no composition root, mas seu crescimento deve ser controlado; ele não deve virar um service locator acessível ao restante da aplicação.
- O `CMakeLists.txt` ainda recompila fontes compartilhadas em vários executáveis de teste. A criação de bibliotecas internas será reavaliada depois que as fronteiras das próximas partes estiverem confirmadas.
- A ordem entre sincronização e logger foi corrigida na parte 7. Permanece a dívida distinta de cancelamento ponta a ponta e prazo máximo de shutdown: hoje a aplicação espera com segurança, mas uma operação que nunca retorna ainda pode impedir o encerramento.

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

## 2. Domínio

### O que significa domínio neste projeto

O domínio é o vocabulário das regras que continuaria relevante mesmo se SQLite, GraphQL e QML fossem substituídos. No estado atual, esse vocabulário contém a mídia, seu tipo e estado de publicação, os campos que podem participar da sincronização e as políticas e resultados de merge.

Uma forma prática de testar se algo pertence ao domínio é perguntar por qual motivo ele mudaria:

- se muda porque a regra de progresso ou conflito mudou, tende a ser domínio;
- se muda porque o formato da resposta GraphQL mudou, é infraestrutura;
- se muda porque o fluxo de retry ou outbox mudou, é aplicação;
- se muda porque o arquivo de configuração ganhou uma chave, é configuração da aplicação.

Antes da revisão, a pasta `domain` reunia todos esses motivos diferentes. A separação física foi corrigida para que o diretório represente uma fronteira real, não apenas uma categoria genérica de "modelos".

### `Media` como modelo central

`Media` reúne duas famílias de dados:

- catálogo externo: identificador, títulos, quantidade de capítulos, nota média, capa, sinopse, tipo e estado de publicação;
- estado do usuário: capítulos consumidos, próximo capítulo e nota pessoal.

A vantagem desse modelo único é simplicidade: mapper, repository e apresentação transportam um objeto sem joins ou composições adicionais. O custo é que dados com owners diferentes passam a ter o mesmo ciclo de vida. Uma atualização de catálogo não deveria sobrescrever progresso ou nota pessoal, e a infraestrutura precisa lembrar dessa regra em toda operação.

Esse custo já aparece no SQL, que atualiza somente os campos de catálogo. A solução é funcional no estágio atual, mas a regra de ownership está implícita na query. Conforme edição local e sincronização crescerem, separar catálogo, entrada da lista e progresso em modelos ou agregados distintos tornará a regra explícita. Essa mudança não foi feita agora porque exige revisar schema, repositories e contratos de leitura em conjunto.

### Estado indeterminado em `Media` — corrigido

Os inteiros e enums de `Media` não possuíam inicializadores. Em C++, `Media media;` inicializava `QString` e `QStringList`, mas deixava tipos primitivos com valores indeterminados. O mapper AniList preenchia dados de catálogo, porém não preenchia `ConsumedChapters`, `NextChapter` ou `PersonalScore`. Qualquer consumidor desses campos podia observar lixo de memória, comportamento indefinido e resultados diferentes entre builds.

Todos os escalares agora começam em zero, e tipo e status começam em `Unknown`. Zero aqui significa ausência do valor de consumo, enquanto `Unknown` significa que a classificação ainda não foi estabelecida. A distinção é melhor do que escolher `Anime` ou `NotReleased` como defaults, pois defaults de negócio falsos transformam ausência de informação em informação incorreta.

### Enums de mídia — corrigido

`MediaType` e `MediaStatus` eram enums não escopados. Isso colocava nomes como `Anime` e `Released` no namespace externo e permitia conversões implícitas mais fáceis entre enum e inteiro.

Eles agora são `enum class`, possuem valores persistidos explícitos e incluem `Unknown = -1`. Os valores existentes de banco foram preservados: `Anime` e `NotReleased` continuam sendo zero, por exemplo. Isso evita uma migração de dados causada apenas pela melhoria do tipo.

O mapper também deixou de interpretar qualquer texto desconhecido como anime ou como mídia não lançada. Valores externos conhecidos são mapeados normalmente; valores não reconhecidos permanecem `Unknown`. O trade-off é que consumidores precisam tratar esse estado. Esse trabalho é preferível a exibir silenciosamente uma classificação falsa e será revisado especialmente na fronteira com o frontend.

### Políticas de merge

`AniListField`, `AniListMergePolicy`, `AniListMergeResult`, `AniListFieldValue` e `AniListMergeDecision` formam a parte mais próxima de um domínio comportamental nesta versão.

`AniListField` identifica semanticamente o dado em conflito. `AniListMergePolicy` descreve a regra escolhida, como remoto vence ou maior valor vence. `AniListMergeResult` descreve o efeito da decisão, como aplicar, enfileirar ou exigir confirmação. Separar política de resultado é uma boa decisão: duas políticas diferentes podem produzir o mesmo resultado e uma mesma política pode ignorar ou enfileirar conforme os valores.

`AniListFieldValue` usa `std::variant<QString, int>`. O benefício é ter um conjunto fechado de tipos e evitar `QVariant`, que aceitaria combinações em tempo de execução sem ajuda do compilador. O custo é que a relação entre campo e tipo ainda não está codificada: é possível construir um título com `int` ou um progresso com `QString`. A aplicação faz algumas verificações, mas o estado inválido continua representável. Tipos específicos por campo ou comandos tipados seriam mais seguros quando os contratos de escrita estiverem estáveis.

### Tipos técnicos removidos do domínio

Foram reposicionados:

- `AniListMediaDto`, `AniListGraphQlResponse` e `AniListGraphQlError` para `infrastructure/anilist`, pois representam payload e transporte;
- `AniListSettings` e `HttpSettings` para `application/configuration`, pois configuram execução e integração;
- `AniListCredentials` para `application/anilist`, pois pertence ao contrato de autenticação usado pelo caso de uso;
- `AniListPendingChange`, seu status e a categoria de erro de sincronização para `application/anilist`, pois representam outbox, tentativas, retry e estado operacional;
- `MediaPage` e `MediaSyncFilter` para `application/media`, pois representam entrada e saída paginada de um caso de uso.

Os aliases `AniListPage` e `AniListSyncFilter` foram removidos porque não possuíam consumidores e apenas ofereciam dois nomes para o mesmo conceito genérico. `MediaEnumFormatter` também foi removido: ele não tinha consumidor de produção, e transformar enums em texto é responsabilidade da borda que precisa daquele texto, não uma regra do domínio.

### Qt dentro do domínio

O domínio continua usando `QString`, `QStringList` e, indiretamente nos modelos de aplicação, `QDateTime` e `QList`. Em uma arquitetura completamente independente de framework, esses tipos seriam substituídos por tipos da biblioteca padrão.

Neste projeto, o benefício de mantê-los é integração direta com Qt SQL, JSON, signals e QML, com menos conversões e ownership de strings bem conhecido. O custo é que os testes e a biblioteca de domínio precisam de Qt Core e não podem ser reutilizados fora do ecossistema Qt sem adaptação. Como a V2 é deliberadamente uma aplicação Qt, a dependência de tipos de valor de Qt é aceitável. O domínio não deve, entretanto, depender de `QObject`, QML, Qt SQL ou Qt Network.

### Por que `Media` não foi encapsulada completamente agora

Os campos de `Media` permanecem públicos. Simplesmente torná-los privados e gerar getters e setters não criaria invariantes; apenas aumentaria o código. Antes de encapsular, o projeto precisa decidir regras como:

- um ID zero representa rascunho válido ou mídia inválida;
- progresso pode superar o total quando o total remoto está desatualizado;
- notas aceitam qual faixa e qual precisão;
- próximo capítulo é derivado do progresso ou persistido independentemente;
- quais campos pertencem ao catálogo e quais pertencem ao usuário.

Depois dessas decisões, métodos com intenção, como avançar progresso ou aplicar atualização de catálogo, poderão impedir estados inválidos. Até lá, os defaults seguros eliminam comportamento indefinido, mas não substituem invariantes reais.

### Dívidas identificadas, mas não corrigidas nesta parte

- `Media` ainda combina catálogo externo e estado do usuário e permite mutação direta de todos os campos.
- A convenção de nomes públicos de `Media` usa PascalCase, diferente do restante da V2. Uma renomeação deve acompanhar a estabilização do contrato, evitando churn em mappers, repositories, testes e apresentação.
- `AniListFieldValue` não relaciona cada campo ao seu tipo permitido em tempo de compilação.
- `MediaSyncFilter` ainda usa strings livres para tipo, status e lista e não valida `startingPage` ou `perPage`; isso será examinado com os contratos da aplicação.
- O mapper SQLite converte inteiros do banco diretamente para enums. Dados corrompidos ou de uma versão futura precisam ser validados na fronteira de persistência.
- Os estados de publicação atuais são mínimos. Hiato, cancelamento e outros estados do AniList permanecem `Unknown` até que o comportamento esperado seja definido.

### Resultado da parte 2

O domínio agora contém regras e vocabulário, em vez de formatos GraphQL, configuração e estado operacional. `Media` deixou de produzir valores indeterminados, enums ficaram escopados e dados externos desconhecidos deixaram de ser classificados silenciosamente como valores válidos.

As mudanças preservam os valores persistidos conhecidos e tornam a ausência de informação explícita. A evolução mais importante ainda pendente é definir invariantes reais e separar o ownership de catálogo e dados do usuário antes de encapsular a entidade.

### Validação executada

- Build Debug completo: concluído.
- Nova suíte `MediaDomainTests`: defaults neutros e enums desconhecidos validados.
- `AniListFlowTests`: mapeamento de tipos externos desconhecidos e defaults de campos locais validados.
- CTest: 11 de 11 testes passaram.

## 3. Contratos da aplicação

### O que é um contrato neste contexto

Um contrato é mais do que uma classe com métodos virtuais. Ele define o que um consumidor pode pedir, quem fornece a implementação, quem controla a vida dos objetos, como sucesso e falha são distinguidos e quais garantias existem sobre os parâmetros de saída.

Na arquitetura da V2, as interfaces da camada `application` são portas. Os casos de uso dependem dessas portas; SQLite, filesystem e GraphQL são adapters que as implementam. Isso permite testar sincronização, apresentação e autenticação com doubles pequenos, sem banco ou rede reais.

### Contratos de mídia segregados — corrigido

O contrato genérico de mídia ficava em `application/anilist/IMediaRepository`. A localização sugeria que toda persistência de mídia pertencia ao AniList, embora leitura local, apresentação e futuras fontes devam ser independentes dessa integração. O contrato foi movido para `application/media`.

Ele também foi separado em três interfaces:

- `IMediaReader` oferece apenas leitura e é consumido pelo controller da Home;
- `IMediaWriter` oferece apenas upsert e é consumido pela sincronização;
- `IMediaRepository` combina ambas para implementações, como SQLite, que oferecem as duas capacidades.

Essa segregação reduz o acoplamento e melhora os testes. O double da Home não precisa fingir que sabe persistir; o double da sincronização não precisa implementar uma leitura que nunca será chamada. O custo é a existência de mais dois tipos, mas cada tipo representa uma capacidade efetivamente diferente.

### Fonte de dados genérica — corrigido

`IAniListDataSource` era apenas um alias de `IMediaDataSource`. Ele não adicionava método, semântica ou garantia. Dois nomes para o mesmo contrato faziam o serviço declarar uma dependência específica enquanto armazenava uma referência ao tipo genérico.

O alias foi removido. `FileAniListDataSource` e `GraphQlAniListDataSource` implementam diretamente `IMediaDataSource`, e `AniListSyncService` depende dessa porta genérica. Assim, o nome do caso de uso pode continuar específico do AniList enquanto a fonte permanece substituível.

Um alias seria justificável se oferecesse uma migração temporária ou um vocabulário público estável. Como não havia consumidores externos nem diferença comportamental, ele apenas escondia a abstração real.

### Convenção de sucesso, saída e erro — corrigida

Os contratos usam atualmente o padrão:

```text
bool operação(entrada, saída, error)
```

Depois da revisão, a convenção é:

- `true` é a única indicação de sucesso;
- em sucesso, `error` deve estar vazio;
- resultados de saída são substituídos, não anexados ao conteúdo anterior;
- em falha, `error` deve conter uma descrição útil;
- o consumidor não deve deduzir sucesso apenas observando se a coleção está vazia.

Antes, `IMediaRepository::ReadAll` retornava diretamente uma lista e usava `error` paralelamente. Uma lista vazia poderia ser uma biblioteca vazia ou uma falha, e um erro antigo poderia sobreviver quando a implementação não o limpasse. `readAll` agora retorna `bool` e recebe a coleção por saída, tornando vazio com sucesso diferente de vazio com falha.

Repositories, data sources, secret store, settings reader, cliente de update e serviços limpam os resultados relevantes no início. Testes com uma mensagem de erro preexistente verificam que operações bem-sucedidas não devolvem estado residual.

### Por que não foi criado um `Result<T>` agora

Um tipo como `Result<T>` poderia transportar sucesso ou erro sem parâmetros de saída e impedir combinações incoerentes. Essa alternativa tende a produzir assinaturas mais expressivas, especialmente quando erros ganham códigos estruturados.

O trade-off é introduzir neste momento uma abstração transversal ainda sem política definida para categorias de erro, warnings, falhas parciais ou valores opcionais. Um `Result<T>` mal desenhado apenas moveria a ambiguidade para dentro de outro tipo. O padrão `bool + saída + error` foi mantido porque é simples, uniforme e já compatível com Qt; sua convenção agora está explícita e testada. A migração poderá ser reconsiderada quando os erros deixarem de ser strings livres.

### Nomenclatura pública — corrigida parcialmente

Os contratos não relacionados a sinais Qt misturavam PascalCase (`ReadAll`, `Upsert`, `Process`) com lower camel case (`fetchPage`, `read`). Os métodos revisados agora seguem lower camel case: `readAll`, `upsert`, `enqueue`, `getPending`, `updateStatus`, `updateMedia`, `process` e `synchronize`.

Slots e sinais antigos dos workers ainda usam nomes iniciados por maiúscula. Eles serão avaliados junto com threading e ciclo de vida, pois renomeá-los isoladamente não resolve ownership, cancelamento ou encerramento daqueles objetos.

### Ownership e tempo de vida

Serviços como `AniListSyncService`, `AniListPendingChangeProcessor` e `AniListAuthManager` recebem dependências por referência e armazenam referências não proprietárias. Isso comunica que eles usam, mas não destroem, repositories, data sources, stores e clients.

O benefício é não pagar por alocação dinâmica ou `shared_ptr` quando a composição já conhece a ordem de vida dos objetos. O custo é uma pré-condição: as dependências devem viver mais que o serviço. Essa pré-condição é satisfeita na composição atual e nos testes locais, mas precisa continuar visível ao refatorar workers ou mover objetos entre threads.

O processador de pendências aceita ponteiro opcional dentro do sincronizador. O `nullptr` representa sincronização somente de leitura. Essa escolha evita uma implementação vazia, porém cria dois modos de funcionamento na mesma classe; a revisão dos casos de uso avaliará se o processamento da outbox deve continuar acoplado ao final da sincronização.

### Autenticação e secrets

`ISecretStore` cuida apenas da persistência das credenciais. `IAniListAuthProvider` fornece uma fotografia das credenciais para o transporte. `AniListAuthManager` liga os dois contratos e mantém o último valor carregado com sucesso.

Essa separação é saudável porque trocar arquivo texto por SQLite criptografado não obriga o cliente GraphQL a mudar. Retornar `AniListCredentials` por valor também evita expor uma referência ao cache interno. O custo é copiar usuário e token a cada solicitação; para o volume atual isso é irrelevante, mas um container seguro com limpeza de memória poderá ser necessário quando a estratégia definitiva de secrets for implementada.

Ainda não existe operação explícita para apagar ou invalidar credenciais. Esse contrato será necessário para logout e revogação segura.

### Outbox e update

`IPendingChangeRepository` representa armazenamento durável da intenção local; `IAniListUpdateClient` representa somente o envio remoto. Essa divisão impede que o repository conheça GraphQL e que o client remoto decida quando uma intenção pode ser descartada.

Os contratos continuam granulares o suficiente para doubles simples. Entretanto, `getPending(mediaId)` exige que o chamador já conheça um ID. Não existe uma operação para recuperar toda a fila pendente ou um lote pronto para retry, o que limitará um worker global de recuperação. A necessidade exata deve ser definida com scheduler e regras de retry antes de ampliar a interface.

### Configuração

`ISettingsReader` recebe o destino de configuração por referência. A implementação JSON agora reinicializa `Settings` e limpa `error` antes da leitura, evitando misturar defaults ou erros de uma tentativa anterior com uma nova tentativa.

O contrato é fácil de substituir nos testes, mas ainda lê toda a configuração como uma unidade. Se configurações editáveis e defaults empacotados forem combinados, será preciso decidir se merge e precedência pertencem ao reader ou a um serviço de configuração separado.

### Cancelamento

Nenhum contrato de `IMediaDataSource` ou `IAniListUpdateClient` recebe hoje um token de cancelamento. `AniListSyncService` verifica timeout entre páginas e etapas, mas não consegue interromper uma implementação bloqueada dentro de `fetchPage` ou `updateMedia`.

Adicionar apenas um `bool cancelled` ao serviço criaria uma falsa garantia. Cancelamento precisa atravessar o caso de uso, o data source e o transporte HTTP, chegando à capacidade de abortar o `QNetworkReply`. Por isso, a assinatura não foi ampliada parcialmente nesta etapa; o contrato completo deve ser introduzido junto com a revisão de concorrência.

### Erros estruturados

As portas retornam descrições em `QString`, e `AniListSyncErrorClassifier` infere categorias analisando texto. Isso é simples para a fase atual, mas frágil: mudar uma mensagem pode mudar retry, apresentação ou diagnóstico sem erro de compilação.

Uma evolução melhor é retornar erro estruturado com categoria, mensagem segura para o usuário e detalhe técnico opcional. Essa mudança deve ser coordenada entre infraestrutura, casos de uso, logging e apresentação; não foi simulada apenas dentro de uma interface.

### Dívidas identificadas, mas não corrigidas nesta parte

- Falta um contrato de cancelamento cooperativo que alcance requisições de rede bloqueadas.
- Falhas continuam sendo strings livres e a classificação depende do texto.
- `ISecretStore` não possui operação de remoção ou invalidação de credenciais.
- `IPendingChangeRepository` não oferece leitura global ou em lotes da outbox.
- O ponteiro opcional para `AniListPendingChangeProcessor` cria dois modos no sincronizador e pode indicar casos de uso que deveriam ser separados.
- Os filtros ainda não têm validação contratual nem tipos fechados para tipo, status e lista.
- Referências não proprietárias dependem da composição manter rigorosamente a ordem de vida, especialmente ao introduzir threads.

### Resultado da parte 3

Os contratos agora estão organizados pela capacidade que oferecem, e não pela integração que primeiro os utilizou. Leitura, escrita, fonte externa, secrets, outbox e update possuem fronteiras menores e doubles mais simples.

Sucesso e falha deixaram de depender do conteúdo residual de `QString` ou de uma coleção vazia. O contrato atual não é o desenho final para erros e cancelamento, mas é coerente, testável e suficientemente explícito para sustentar a próxima revisão dos casos de uso.

### Validação executada

- Build Debug completo: concluído.
- Testes adicionados para garantir limpeza de erros preexistentes em sincronização, processamento da outbox e secrets.
- Doubles da Home e da sincronização passaram a implementar somente leitura ou somente escrita.
- CTest: 11 de 11 testes passaram.

## 4. Casos de uso e regras de sincronização

### Escopo desta parte

Esta parte acompanha o caminho que transforma uma sincronização solicitada em leitura paginada, persistência local e processamento da outbox. Foram revisados `AniListSyncService`, as regras de merge, a criação, compactação e agrupamento de pendências e `AniListPendingChangeProcessor`.

O ponto didático central é separar três perguntas que parecem iguais, mas não são:

1. **Qual valor deve prevalecer?** Essa é uma regra de merge.
2. **Qual intenção precisa sobreviver até uma tentativa futura?** Essa é uma regra da outbox.
3. **Quando é seguro afirmar que a operação terminou?** Essa é uma regra de consistência entre banco local e serviço remoto.

Misturar essas perguntas em um único método produziria muitos `if`s e tornaria difícil testar por que uma decisão foi tomada. O projeto as distribui entre resolver, merge service, factory, compactor, grouper e processor. A separação é boa, mas a orquestração ainda é parcial.

### Fluxo de leitura e persistência

`AniListSyncService::synchronize` executa este fluxo:

```text
validar paginação
    -> buscar uma página
    -> conferir se a página recebida é a solicitada
    -> persistir a página
    -> repetir enquanto houver próxima página
    -> processar a outbox das mídias vistas, quando configurada
```

Persistir uma página antes de buscar a seguinte limita a quantidade de dados em memória e permite que trabalho válido sobreviva a uma falha posterior. O trade-off é que a sincronização completa não é atômica: se a terceira página falhar, as duas primeiras já foram gravadas. Isso é aceitável somente com `upsert` idempotente e, futuramente, checkpoint de execução.

Antes, valores de paginação inválidos eram silenciosamente convertidos para `1`. Isso escondia erros do chamador: pedir página zero parecia funcionar, mas executava outra solicitação. Agora `startingPage` e `perPage` precisam ser positivos, e a falha é classificada como `InvalidData`.

Também foi adicionada uma invariância de progresso: a página devolvida deve ser exatamente a página pedida. Sem essa verificação, um data source defeituoso poderia responder sempre com a página 1 e manter o `while` ativo até o timeout — ou para sempre quando o timeout estivesse desabilitado. IDs sincronizados agora são mantidos em `QSet`, evitando reprocessar a mesma outbox quando uma fonte repete uma mídia em mais de uma página.

### Regras de merge

`AniListMergePolicyResolver` concentra a tabela de ownership por campo:

- catálogo externo usa `RemoteWins`;
- progresso usa `MaxValue`;
- nota pessoal gera uma intenção local quando difere do remoto;
- status da lista preserva a alteração local mais recente;
- arquivos locais usam `LocalWins`;
- exclusão exige confirmação.

Centralizar essa tabela evita que cada caller invente uma regra. O custo é que adicionar um campo exige revisar o `switch`; neste caso isso é desejável, porque o compilador e os testes tornam a decisão visível.

`AniListMergeService` é puro: recebe campo, valor local e valor remoto e devolve uma decisão, sem banco ou rede. Essa pureza torna os cenários baratos de testar. Foi corrigido o caso de `MaxValue` com tipos incompatíveis: antes ele preservava silenciosamente o valor local e dizia `Applied`; agora retorna `Conflict`, porque uma string usada como progresso é uma violação de contrato, não uma escolha válida de merge.

O nome `LocalWinsLatest` precisa ser entendido com cuidado. O merge service escolhe o valor local; quem decide qual das várias intenções locais é a mais recente é o compactor, usando `localUpdatedAt`, depois `createdAt` e `id`. Ele não compara atualmente a data da alteração local com uma versão remota.

### Criação de intenções locais

`AniListPendingChangeFactory` é a porta de entrada para uma intenção que poderá chegar à rede. Por isso, sua responsabilidade não pode ser apenas preencher timestamps.

A factory agora:

- reinicializa a saída e limpa erro residual;
- exige identificador positivo;
- aceita apenas campos atualmente graváveis pelo fluxo de update;
- exige inteiro para progresso e nota, e texto para status e exclusão;
- rejeita transições sem mudança, exceto a intenção especial de exclusão;
- cria exclusões em `RequiresConfirmation`.

Validar cedo impede que um valor impossível seja persistido, sobreviva por várias execuções e falhe apenas no transporte. O trade-off é duplicar parte do conhecimento sobre campos suportados pelo update client. Uma evolução possível é representar cada comando de update por um tipo próprio, eliminando combinações inválidas em tempo de compilação.

### Compactação e agrupamento

O compactor não apaga histórico: ordena as alterações e marca como `Superseded` os estados de lista substituídos por uma intenção local mais recente. Manter o registro permite auditoria e diagnóstico; apagar seria mais simples, mas perderia a explicação de como o estado final foi alcançado.

O grouper reúne alterações por mídia. A fronteira remota recebe uma operação conceitual por mídia, preparando o caminho para uma única mutation `SaveMediaListEntry`. A implementação de infraestrutura ainda executa mutations específicas por campo, portanto o agrupamento atual reduz a fragmentação do contrato, mas ainda não oferece atomicidade remota.

### Máquina de estados da outbox

O processador executa, em ordem, `Pending/Failed/Processing -> Processing -> Succeeded` ou `Failed`. Tentativas remotas que falham incrementam `attempts` e preservam `lastError`, permitindo retry posterior.

Uma correção importante foi separar **armazenado na fila** de **autorizado para envio**. Uma exclusão sem confirmação antes chegava ao client, era recusada e virava `Failed`. Isso modelava uma decisão aguardando o usuário como falha técnica. Agora exclusões antigas ainda marcadas como `Pending` são migradas para `RequiresConfirmation`, e entradas nesse estado não são enviadas nem contam como erro. Ainda falta o comando explícito de confirmação que as tornará enviáveis.

Marcar `Processing` antes da chamada remota torna visível que a intenção saiu do estado de espera. Por outro lado, as mudanças de status são persistidas uma a uma. Se a segunda gravação falhar, a primeira pode ficar em `Processing`; se a mutation funcionar e a confirmação local falhar, o retry poderá reenviar uma operação que já ocorreu. Resolver isso exige transação ou operação em lote, recuperação de leases abandonados e uma política explícita de idempotência. Não seria correto esconder o problema com mais `if`s dentro do processador.

### O que ainda não forma um fluxo completo

As classes de policy, merge e factory estão testadas isoladamente, mas ainda não são chamadas pelo fluxo de sincronização para reconciliar cada campo recebido. Hoje `AniListSyncService` entrega a página ao writer e depende da persistência para preservar colunas locais; depois, opcionalmente, processa pendências somente das mídias presentes nas páginas recebidas.

Isso significa que a tabela de merge descreve a política desejada, mas não prova que todos os campos seguem essa política no fluxo real. Um teste unitário do resolver prova a regra; um teste de integração do caso de uso precisa provar que remoto, local, outbox e persistência terminam no estado esperado.

O ponteiro opcional do processor também faz `synchronize` representar dois casos de uso: download somente de leitura e download seguido de publicação. Manter juntos simplifica a inicialização atual, mas mistura dois resultados independentes: todas as páginas podem ter sido persistidas e a sincronização ainda retornar falha porque uma mutation posterior falhou. A separação deverá ser decidida junto com scheduler e apresentação de estado.

### Dívidas identificadas, mas não corrigidas nesta parte

- O merge ainda não está orquestrado no fluxo real de ingestão; a proteção de campos locais depende hoje da query de persistência.
- Não existe comando nem estado de confirmação concluída que libere uma exclusão para envio.
- Transições de status da outbox não são atômicas por grupo, e entradas abandonadas em `Processing` não possuem lease ou regra formal de recuperação.
- Depois de sucesso remoto e falha no acknowledgement local, o sistema oferece semântica de pelo menos uma vez; idempotência e reconciliação ainda precisam ser definidas.
- A ordem usa timestamps e `id`, mas o desempate de objetos ainda não persistidos, com datas iguais e `id == 0`, depende da ordem de entrada.
- O processamento para na primeira mídia com falha e não produz um relatório de sucesso parcial.
- A outbox é processada apenas para IDs presentes na sincronização corrente; o worker global dependerá da leitura em lote registrada no planejamento.
- Timeout e cancelamento continuam sendo observados somente entre operações; uma chamada bloqueada não pode ser abortada pelos contratos atuais.
- Categorias de erro ainda são inferidas de texto livre.
- Limites semânticos de progresso e nota, e o vocabulário fechado de status da lista, ainda precisam ser definidos antes de adicionar validação de faixa.

### Resultado da parte 4

Os casos de uso agora recusam paginação inconsistente, não entram em ciclo por uma fonte que devolve a página errada, não reprocessam IDs duplicados e distinguem conflito de tipo de um merge aplicado. A entrada da outbox ganhou validação e exclusões aguardando confirmação deixaram de ser tratadas como falha de rede.

A arquitetura está testável porque policy, merge, criação, compactação, agrupamento e processamento podem ser exercitados sem rede real. Ela ainda não deve ser chamada de sincronização bidirecional completa: faltam a orquestração do merge, a confirmação de exclusão, retry global e garantias de consistência da outbox.

### Validação executada

- Build Debug completo: concluído.
- Testes adicionados para paginação inválida, fonte que devolve página divergente, valores de merge incompatíveis, factory tipada, transição sem mudança e exclusão aguardando confirmação.
- CTest: 11 de 11 testes passaram.



## 5. Infraestrutura local

### Escopo desta parte

Esta parte revisa os adaptadores que ligam a aplicação ao ambiente local: abertura e schema do SQLite, repositories, mappers, SQL externo, configuração JSON, armazenamento temporário de credenciais e logging em arquivo.

Uma infraestrutura saudável não deve inventar regras de negócio. Sua responsabilidade é traduzir formatos e efeitos externos sem perder informação e sem declarar sucesso quando o efeito esperado não ocorreu. Em outras palavras, ela protege o core contra detalhes de SQLite, JSON, arquivos e Qt.

### Conexão SQLite e ownership

`SqliteDatabase` cria uma conexão com nome único, escolhe por padrão o diretório de dados da aplicação e remove a conexão do registry do Qt no encerramento. O nome único evita colisões entre testes e futuras conexões de workers.

`QSqlDatabase` é um handle implicitamente compartilhado. Quando `connection()` retorna por valor para um repository, não cria outro banco nem outra conexão: cria outro handle para a mesma conexão registrada. Isso é barato e funciona enquanto database e repository permanecem na mesma thread. O trade-off é importante: copiar o handle não torna a conexão segura para outra thread. Cada worker que acessar SQLite precisará abrir sua própria conexão na thread proprietária.

SQLite não ativa foreign keys automaticamente para cada conexão. O schema declarava `FOREIGN KEY(media_id)`, mas sem `PRAGMA foreign_keys = ON` a restrição era apenas aparente e uma pendência podia apontar para uma mídia inexistente. A abertura agora ativa a opção e falha se não conseguir configurá-la. O teste prova tanto o valor do pragma quanto a rejeição de uma referência órfã.

### Migração atual

`migrate()` executa a criação das tabelas dentro de uma transação. Se uma instrução falhar, ocorre rollback; se todas funcionarem, a versão 1 é registrada e a transação é confirmada. Repetir a operação é seguro para o schema atual por causa de `IF NOT EXISTS` e `INSERT OR IGNORE`.

Dois problemas foram corrigidos. Primeiro, o objeto guardava apenas `database_.lastError()`, embora a causa real de uma migração normalmente pertença ao `QSqlQuery`; assim, a inicialização podia falhar com diagnóstico vazio. Agora a classe preserva a falha da operação que realmente falhou. Segundo, executar `INSERT OR IGNORE` não prova que a versão existe: o `IGNORE` também pode esconder uma violação de constraint. A migração agora consulta explicitamente a versão antes do commit.

Isso ainda não é um mecanismo completo de migrações. Todo o schema continua hardcoded em um único método e não existe sequência de scripts `1 -> 2 -> 3`, checksum ou rejeição de versão futura. A correção atual torna a versão declarada verdadeira; não finge que o sistema de evolução já existe.

### Repository de mídia e mapper

`SqliteMediaRepository::upsert` usa uma transação por lote. Esse desenho evita uma biblioteca parcialmente atualizada quando um item intermediário falha e reduz o custo de commits individuais. A query atualiza somente os campos pertencentes ao catálogo e omite progresso, próximo capítulo e nota pessoal. Isso preserva dados locais, mas ainda deixa ownership implícito no texto SQL, dívida já registrada.

`AlternativeNames` era serializado como JSON durante o upsert, mas `SqliteMediaMapper` ignorava completamente a coluna na leitura. Esse é um exemplo clássico de assimetria: salvar funcionava, o banco continha o valor correto, mas o modelo reconstruído perdia informação. O mapper agora reconstrói as strings do array, e um teste de round trip protege o par gravação/leitura.

Usar JSON em uma coluna permite guardar uma lista sem criar imediatamente outra tabela. O benefício é simplicidade; os custos são perder consultas relacionais sobre aliases e precisar validar o documento na fronteira. Por enquanto um JSON inválido de aliases resulta em lista vazia, porque o mapper não possui contrato de falha. Tornar essa corrupção explícita exigirá mudar o mapper ou fazer o repository validar a linha.

### Repository da outbox e codec de valores

A outbox armazena `AniListFieldValue` como JSON marcado com `type` e `value`. A marca preserva a diferença entre o inteiro `12` e o texto `"12"`, algo que uma coluna textual sozinha não conseguiria reconstruir com segurança.

Antes, qualquer JSON inválido ou tipo desconhecido era convertido silenciosamente em string vazia. Isso transformava corrupção persistida em uma alteração aparentemente válida. O decoder agora valida documento, tag e tipo; valores inteiros também precisam ser finitos, sem parte fracionária e caber em `int`. Em falha, a coleção parcial é limpa e a operação retorna erro. `remoteObservedAt`, que era gravado mas nunca lido, também passou a participar do round trip.

Outra particularidade do Qt apareceu nos testes: uma `QString` default pode ser nula, e o driver a vincula como SQL `NULL`. As colunas `remote_version` e `last_error` são `NOT NULL`; confiar apenas no default do schema não ajuda quando o `INSERT` envia explicitamente `NULL`. O repository agora normaliza essas strings para texto vazio ao persistir.

`QSqlQuery::exec()` retornar `true` significa que o comando foi aceito, não que uma linha foi encontrada. `updateStatus` antes reportava sucesso para um ID inexistente. Agora exige exatamente uma linha afetada. Essa distinção é essencial para a outbox: sem ela, o processador poderia acreditar que confirmou uma mutation enquanto nenhuma pendência foi atualizada.

### SQL externo e configuração das queries

Separar SQL do C++ torna as queries legíveis, revisáveis e alteráveis sem recompô-las como concatenações de literais. `SqliteQueryConfiguration` resolve os nomes lógicos para recursos empacotados, e `SqlQueryStore` carrega o conteúdo completo.

O uso de `:/...` é deliberado: recursos Qt não dependem do diretório de trabalho escolhido pelo terminal ou IDE. O trade-off é que alterar um SQL empacotado ainda exige gerar novamente o executável; a separação melhora organização e testes, não cria configuração dinâmica em runtime.

Ambos os loaders agora substituem suas saídas e limpam erros antigos antes de trabalhar. Sem isso, uma falha poderia deixar uma query anterior no parâmetro de saída, ou um sucesso poderia continuar acompanhado por uma mensagem antiga. Os testes usam arquivo temporário real e resource Qt real, sem doubles do filesystem.

### Configuração da aplicação

`JsonSettingsReader` começa de um `Settings` limpo e oferece defaults para propriedades opcionais. Defaults são úteis para manter o arquivo pequeno, mas só devem cobrir ausência. Antes, um texto como `"five seconds"` em `timeoutMs` também virava o default e escondia configuração malformada.

Os campos numéricos agora aceitam default quando ausentes, mas rejeitam tipo incorreto, fração e valor fora do intervalo de `int`. Depois da leitura estrutural, as regras existentes continuam validando positividade e limites permitidos. Essa separação distingue duas perguntas: "o JSON possui o tipo correto?" e "o valor faz sentido para a aplicação?".

### Armazenamento temporário de credenciais

`FileSecretStore` usa `QSaveFile`, que grava em arquivo temporário e substitui o destino somente no commit. Isso evita um arquivo parcialmente escrito se o processo falhar durante a gravação. Atomicidade de arquivo, entretanto, não é segurança: username e token continuam em texto puro.

O store agora rejeita username ou token vazios e valores com quebra de linha. Sem essa validação, uma credencial vazia parecia carregada com sucesso, e uma quebra de linha poderia alterar a estrutura `chave=valor` do arquivo. Em falha de leitura, a saída é novamente limpa para não deixar uma credencial parcial acessível ao chamador.

Criptografia, permissões, remoção segura e revogação pertencem ao armazenamento definitivo e permanecem no planejamento. Endurecer o formato temporário reduz estados inválidos, mas não torna adequado guardar tokens de produção nesse arquivo.

### Logging local

`AsyncLogger` mantém uma fila protegida por mutex e uma thread dedicada que drena as entradas para um arquivo diário. Isso tira I/O de log do caminho principal. Quando a fila chega a 4096 entradas, a mais antiga é descartada para limitar memória; quando o logger não iniciou ou já está parando, novas mensagens também são descartadas.

Essas escolhas favorecem disponibilidade: logging não bloqueia indefinidamente a aplicação. O custo é perda silenciosa de diagnóstico, inclusive quando diretório ou arquivo não podem ser abertos. Como start/stop, thread e ordem de destruição pertencem diretamente ao item 7, esta parte apenas registra o comportamento; não altera parcialmente o ciclo de vida.

### Dívidas identificadas, mas não corrigidas nesta parte

- O schema ainda está hardcoded e a tabela de versão ainda não conduz uma sequência real de migrations.
- `WAL`, `busy_timeout` e a política de concorrência do SQLite ainda não foram definidos.
- Cada thread precisará de uma conexão SQLite própria; copiar `QSqlDatabase` não atende essa regra.
- O mapper de mídia ainda não reporta JSON de aliases corrompido, e inteiros persistidos são convertidos para enums sem validar faixa.
- A outbox ainda precisa validar datas, enums e coerência entre campo e tipo ao reconstruir dados antigos ou alterados fora da aplicação.
- A política de ownership continua codificada por omissão de colunas na query de upsert.
- `FileSecretStore` permanece em texto puro e não possui remoção/invalidação.
- O logger pode descartar mensagens silenciosamente e não expõe falha de inicialização ou escrita.
- Repositories e loaders ainda dependem de `QString` livre para diagnóstico, aguardando o contrato de erro estruturado.

### Resultado da parte 5

A infraestrutura local agora preserva os dados que grava, rejeita representações corrompidas da outbox, diferencia query executada de estado realmente atualizado e aplica a integridade referencial declarada pelo schema. Configurações mal tipadas e credenciais estruturalmente inválidas deixam de ser aceitas silenciosamente.

Os novos testes são majoritariamente round trips com SQLite e arquivos reais. Essa escolha custa mais que um teste puramente em memória, mas verifica exatamente a fronteira onde ocorreram os defeitos: binding do driver, JSON persistido, linhas afetadas, resources Qt e semântica do SQLite.

### Validação executada

- Os testes novos foram observados falhando antes das correções correspondentes.
- Foram adicionadas suítes para repositories SQLite, query store, configuração de queries e configuração JSON.
- `SqliteDatabaseTests` e `FileSecretStoreTests` foram ampliados com os novos contratos.
- Build Debug completo: concluído.
- CTest: 15 de 15 testes passaram.

## 6. Integração AniList

### Escopo desta parte

Esta parte revisa a fronteira entre o core e o AniList: carregamento de queries, montagem do payload, leitura do envelope GraphQL, validação de páginas, mapeamento de mídia, timeout/retry e mutations.

Por decisão do projeto, **não foi feita conexão real com o AniList**. Isso é uma restrição útil, não uma lacuna escondida: nas próximas etapas o provider ativo continuará baseado em fixtures, e a integração externa ficará entre as últimas entregas. Portanto, esta revisão corrige problemas estruturais demonstráveis com dados controlados, mas não afirma que query, autenticação, rate limit ou schema estejam homologados contra a API oficial.

### As três fronteiras que estavam misturadas

Uma chamada GraphQL possui pelo menos três níveis diferentes:

1. o transporte HTTP pode funcionar ou falhar;
2. o corpo pode ou não ser um envelope GraphQL válido, com `data` e/ou `errors`;
3. o campo `data` pode ou não conter a operação `Page` com a estrutura esperada.

Antes, `AniListGraphQlClient` fazia os três trabalhos parcialmente: enviava HTTP, esperava a resposta e interpretava o envelope. Depois, `GraphQlAniListDataSource` convertia campos ausentes com `toInt()`, `toBool()` e `toArray()`. No Qt, essas conversões devolvem defaults; assim, uma resposta malformada podia virar uma página vazia com zeros em vez de uma falha na fronteira.

Foram extraídos dois componentes puros:

- `AniListGraphQlResponseParser` transforma bytes em `AniListGraphQlResponse` e valida o envelope;
- `AniListGraphQlPageParser` transforma o `data` de uma operação em `MediaPage` e valida `Page`, `pageInfo`, tipos de paginação e entradas de mídia.

O client continua responsável pelo transporte e apenas delega a interpretação dos bytes. O data source continua responsável por executar a operação e apenas delega a interpretação da página. Essa separação torna as regras testáveis com fixtures, sem fabricar uma rede falsa ou abrir o escopo da integração real.

### Envelope GraphQL e sucesso parcial

GraphQL permite que uma resposta contenha `data` e `errors` ao mesmo tempo. Por isso, o parser preserva ambas as partes; ele não trata automaticamente a presença de dados como sucesso da operação. O consumidor atual rejeita qualquer resposta com erros e usa a primeira mensagem para diagnóstico.

Também foi corrigida a semântica de `hasData()`: um objeto `data: {}` existe, mesmo estando vazio. Confundir presença com conteúdo fazia o tipo representar incorretamente o envelope. A validação específica — por exemplo, exigir `Page` — pertence ao parser da operação, que conhece essa obrigação.

O trade-off da validação estrita é rejeitar respostas que antes seriam toleradas. Neste ponto isso é desejável: defaults silenciosos escondem quebra de contrato. Ainda assim, o tratamento futuro de respostas parciais precisa ser uma decisão do caso de uso, não uma regra implícita do parser genérico.

O parser constrói a resposta em um valor temporário e só a publica quando todo o envelope é válido. Isso garante saída atômica: se `data` for válido, mas `errors` estiver malformado, o método falha com uma resposta vazia, em vez de deixar dados parciais que um caller poderia consumir por engano.

### Página e mapeamento de mídia

`AniListGraphQlPageParser` exige paginação inteira e coerente: página atual positiva, última página não anterior à atual e `hasNextPage` booleano. Exige também que `media` seja array e que cada entrada seja objeto. A saída é reinicializada antes do trabalho, portanto uma falha não deixa uma página antiga parcialmente acessível.

Depois da validação estrutural, `AniListMediaMapper` traduz o DTO externo para `Media`. Essa camada converte vocabulários externos, como `ANIME` e `FINISHED`, nos enums internos e usa `Unknown` para valores não reconhecidos. O mapper ainda não valida todos os campos internos de cada mídia; por exemplo, um `id` ausente vira zero. A escolha atual mantém a correção limitada ao defeito comprovado da página, mas indica uma próxima fronteira de validação antes da conexão real.

Há dois formatos de entrada: fixture local e GraphQL. Mantê-los no mesmo mapper reduz duplicação do mapeamento para domínio, mas preserva duas funções de leitura porque os formatos são diferentes. A nomenclatura dessas funções ainda é inconsistente e deverá ser normalizada sem confundir uma mudança de estilo com uma garantia de integração.

### Queries externas

`GraphQlQueryStore` mantém texto GraphQL fora do C++, permitindo revisar queries e mutations como documentos próprios. Assim como ocorreu com o SQL, o loader agora limpa `query` e `error` antes de tentar ler. Isso impede que uma falha deixe uma query anterior disponível ou que um sucesso carregue um erro residual.

Externalizar uma query melhora organização e testabilidade, mas não prova que ela corresponde ao schema remoto. A query e a fixture atuais concordam entre si; essa é apenas uma garantia interna. Campos como `synonyms`, filtros de usuário/lista e as mutations definitivas ainda precisam ser confirmados quando a integração real entrar no plano.

### Transporte, timeout e retry

`AniListGraphQlClient` monta um `POST` JSON com `query` e `variables`, acrescenta bearer token quando disponível e aplica timeout. Se o timeout dispara, chama `abort()` no `QNetworkReply`. Isso protege uma tentativa individual contra espera infinita dentro do próprio client.

O desenho atual, porém, é síncrono: usa um `QEventLoop` aninhado e dorme com `QThread::msleep` entre tentativas. O loop aninhado permite que outros eventos sejam processados de forma reentrante, enquanto o `msleep` bloqueia a thread chamadora. Recursão para retry também mistura política de resiliência com transporte. Esses custos ainda não aparecem nos testes de fixture e não foram mascarados por uma abstração incompleta; a substituição deve acontecer junto com concorrência, scheduler, rate limit e cancelamento ponta a ponta.

### Updates e atomicidade remota

`IAniListUpdateClient` recebe alterações agrupadas por mídia, mas `AniListUpdateClient` ainda percorre os campos e executa uma mutation por alteração. Isso cria uma diferença entre o contrato conceitual e o efeito remoto: progresso pode ser aceito e nota falhar, deixando um grupo parcialmente aplicado.

O agrupamento atual é uma preparação para uma mutation única `SaveMediaListEntry`, não uma garantia de atomicidade. Implementá-la agora exigiria afirmar um contrato remoto que deliberadamente ainda não será conectado. A dívida permanece registrada para a etapa real, junto com idempotência e reconciliação da outbox.

A exclusão também expõe um desalinhamento: o client aceita envio apenas quando o estado é `RequiresConfirmation`, enquanto o processador deliberadamente não envia itens nesse estado e ainda não existe um estado de confirmação concluída. O problema pertence ao fluxo de confirmação, não ao parser ou transporte, e continua pendente.

### Dívidas identificadas, mas não corrigidas nesta parte

- Queries, mutations, filtros e posição dos campos no schema ainda não foram validados contra o AniList real.
- O provider de fixture continua sendo o caminho ativo; autenticação OAuth, rate limit e comportamento de produção não foram exercitados.
- O client usa `QEventLoop` aninhado, retry recursivo e `QThread::msleep`, com riscos de reentrância e bloqueio da thread chamadora.
- O cancelamento não atravessa os contratos até `QNetworkReply::abort()`; apenas o timeout interno consegue abortar a tentativa.
- O parser de página valida o contêiner, mas o mapper ainda aceita campos de mídia ausentes ou com tipo incorreto usando defaults do Qt.
- O consumidor transforma erros GraphQL em `QString` e perde categoria, extensões e contexto além da primeira mensagem.
- As mutations agrupadas continuam sendo executadas separadamente e podem produzir sucesso remoto parcial.
- O fluxo de exclusão não possui um estado explícito de confirmação concluída que seja enviável.
- Nomes públicos dos helpers de infraestrutura ainda misturam lower camel case e PascalCase.

### Resultado da parte 6

A parte testável sem rede ficou mais estável: bytes, envelope GraphQL e conteúdo de `Page` agora são fronteiras distintas; respostas estruturalmente inválidas falham cedo; outputs antigos não sobrevivem a uma nova leitura; e o client/data source reutilizam os parsers exercitados por testes.

O resultado não é uma integração AniList pronta. É uma base que permite continuar desenvolvendo com fixtures e que reduz o trabalho arriscado quando a conexão real for finalmente habilitada, sem confundir compatibilidade interna com homologação externa.

### Validação executada

- Os novos testes foram observados falhando antes da criação dos parsers.
- `AniListGraphQlParsingTests` cobre envelope com dados, envelope com erros, envelope vazio, falha sem saída parcial, página válida, mapeamento e metadados malformados.
- `GraphQlQueryStoreTests` cobre limpeza de saída e erro entre falha e sucesso.
- Build completo: concluído.
- CTest: 17 de 17 testes passaram.
- `git diff --check`: concluído sem erros.




## 7. Composição, concorrência e ciclo de vida

### Escopo desta parte

Esta parte revisa quem cria os objetos da aplicação, quem possui cada dependência, em qual thread o trabalho acontece e como a aplicação encerra sem destruir recursos ainda em uso. O foco não é apenas “usar uma thread”: é estabelecer uma sequência de vida completa, desde a composição até o shutdown.

O provider ativo continua sendo `FileAniListDataSource`. A revisão não conecta o AniList real nem adiciona um cancelamento parcial. Ela endurece garantias que já podem ser provadas com fixtures e registra claramente o que depende de contratos futuros.

### Composition root e ownership

`createApplicationContext()` funciona como composition root: lê configuração, inicia o logger, abre o banco principal, carrega queries, cria repositories e, por fim, cria o coordenador de sincronização. Centralizar essas decisões evita que controllers ou QML conheçam classes de infraestrutura.

`ApplicationContext` usa `std::unique_ptr`, portanto o ownership é exclusivo e explícito. C++ destrói membros na ordem inversa da declaração. Como `initialSync` é declarado depois dos repositories, banco e logger, ele é destruído antes dessas dependências. Essa ordem é importante porque o coordenador mantém um ponteiro não-owning para o logger e sua execução cria adaptadores que usam os mesmos conceitos de infraestrutura.

RAII oferece uma rede de segurança, mas não substitui uma política operacional. Esperar apenas pela destruição do contexto tornava difícil expressar que a sincronização precisa parar antes do logger. Por isso o encerramento normal passou a ser explícito em `main.cpp`, enquanto os destructors continuam protegendo caminhos excepcionais.

### Afinidade de thread e SQLite

Uma conexão Qt SQL pertence à thread em que foi criada. Copiar `QSqlDatabase` copia um handle implicitamente compartilhado; não transforma a conexão em um recurso transportável entre threads.

O fluxo de background respeita essa regra: a lambda executada pela `QThread` cria dentro dela `SqliteDatabase`, query stores e `SqliteMediaRepository`. Esses objetos também são destruídos antes de a lambda terminar, ainda na mesma thread. O banco usado pela apresentação permanece separado na thread principal.

O benefício é evitar acesso cruzado a uma conexão. O custo é repetir parte da composição dentro da operação de sincronização. Extrair essa operação para um componente próprio poderá melhorar organização, mas não deve mover a criação do banco de volta para a thread principal.

### Coordenador, scheduler e exclusão mútua

`InitialSyncCoordinator` possui o `QTimer` e o objeto `QThread` de cada execução. `start()` registra a execução como ativa antes de emitir `started()` e recusa uma nova solicitação enquanto esse estado estiver marcado. Isso estabelece exclusão mútua dentro desta instância: dois ticks do timer, duas chamadas próximas ou até uma chamada reentrante disparada pelo próprio sinal não iniciam duas sincronizações simultâneas.

Marcar o estado antes do sinal é importante porque sinais Qt com conexão direta executam o slot imediatamente, dentro da pilha de `start()`. Se o estado fosse registrado apenas depois, um slot conectado a `started()` poderia chamar `start()` outra vez e entrar em recursão, ou chamar `shutdown()` e ainda assim deixar a chamada original criar uma thread. Após emitir o sinal, o coordenador reavalia `stopping_`: um shutdown reentrante torna-se terminal e impede que o trabalho chegue a ser criado.

Essa proteção não é um lock persistente nem distribuído. Duas instâncias do processo, dois coordenadores ou uma retomada após crash não compartilham o estado. O scheduler definitivo ainda precisará registrar execução, próxima tentativa e recuperação conforme o planejamento.

Para tornar essa política testável, o coordenador aceita uma `SyncOperation` opcional. Nos testes, ela controla quando o trabalho entra e sai; a `QThread` continua real. Em produção, a ausência dessa operação usa exatamente o fluxo atual de fixture, SQLite e `AniListSyncService`. A seam separa “quando executar” de “qual trabalho executar”; ela não cria um segundo provider de produção.

### Shutdown explícito

`shutdown()` agora marca o coordenador como encerrando, para o scheduler, aguarda a thread ativa e libera o objeto `QThread` concluído. O estado é terminal: depois do shutdown, `start()` não aceita novo trabalho. Repetir `shutdown()` é seguro.

O callback de conclusão é enfileirado para a thread proprietária do coordenador. Se ele for processado depois de `shutdown()`, ainda poderá emitir o resultado, mas não reiniciará o scheduler porque consulta o estado de encerramento. Isso elimina a corrida lógica em que uma conclusão tardia rearmaria novas execuções durante a saída.

Em `main.cpp`, `aboutToQuit` chama primeiro `initialSync->shutdown()` e somente depois `logger->stop()`. A ordem segue a dependência: enquanto a sincronização ainda pode produzir mensagens, o logger precisa aceitar entradas; depois, o logger drena sua fila e termina.

### Esperar não é cancelar

`QThread::wait()` impede que o coordenador e suas dependências sejam destruídos enquanto a operação ainda usa `this` ou o logger. Isso resolve segurança de vida útil, mas não interrompe a operação.

Se filesystem, SQLite ou uma futura chamada HTTP nunca retornarem, o shutdown também poderá esperar indefinidamente. Adicionar apenas `stopping_` ao coordenador não mudaria esse fato. Cancelamento verdadeiro precisa atravessar caso de uso, data source, update client e transporte até a operação bloqueada — no caso de HTTP, chegando a `QNetworkReply::abort()`.

O trade-off atual é consciente: espera potencialmente longa é preferível a destruir uma thread ainda ativa e provocar use-after-free. O planejamento mantém a evolução de cancelamento e prazo de encerramento como uma mudança transversal futura.

### Ciclo de vida do logger

`AsyncLogger` possui sua própria `QThread`, mutex, fila e `QWaitCondition`. O slot conectado a `QThread::started` usa conexão direta e entra em `run()`, que ocupa a thread drenando a fila. Por isso `thread_.quit()` sozinho não encerra esse loop: quem efetivamente permite a saída é `stopping_`, acordado por `condition_.wakeOne()`.

`stop()` sinaliza essa condição, espera a fila esvaziar e aguarda a thread. O destructor chama `stop()` novamente com segurança. O desenho favorece não perder as mensagens já enfileiradas no encerramento, mas ainda pode descartar entradas quando a fila lota ou quando não consegue abrir o arquivo, sem expor esse diagnóstico ao chamador.

### Workers existentes, mas não compostos

`AniListSyncWorker` e `AniListUpdateWorker` encapsulam chamadas a serviços e emitem sinais de ciclo de vida, porém não participam da composição atual. Ambos armazenam referências não-owning; mover apenas esses QObjects para uma thread não define onde serviços, repositories, conexão SQLite e transporte devem nascer ou morrer.

Integrá-los agora daria aparência de uma arquitetura de workers persistentes sem resolver ownership. A decisão foi mantê-los como peças ainda não compostas até que scheduler, cancelamento e update real definam o ciclo completo. Nessa etapa será possível escolher entre uma thread curta por execução e workers persistentes com event loop e destruição na thread correta.

### Dívidas identificadas, mas não corrigidas nesta parte

- Uma operação que não retorna pode bloquear o shutdown indefinidamente; não há deadline nem cancelamento ponta a ponta.
- A exclusão mútua existe apenas dentro de uma instância do coordenador e não sobrevive a processo encerrado ou múltiplas instâncias.
- O scheduler não persiste execução, checkpoint, próxima tentativa ou lease.
- `AniListSyncWorker` e `AniListUpdateWorker` não possuem composição nem política definitiva de ownership.
- O transporte GraphQL continua síncrono, com event loop aninhado e espera bloqueante entre retries.
- `AsyncLogger` não expõe falha de criação de diretório, abertura ou escrita, e pode descartar mensagens silenciosamente.
- A operação de produção ainda é montada dentro do coordenador; uma extração futura deve preservar a criação de recursos thread-affine dentro da thread executora.
- O shutdown explícito é chamado pela aplicação, mas ainda não existe teste de janela real fechando durante I/O externo bloqueado.

### Resultado da parte 7

O ciclo de vida agora possui um contrato observável: não há execuções concorrentes dentro do coordenador, shutdown impede novos trabalhos, aguarda o trabalho ativo, não rearma o scheduler e pode ser chamado repetidamente. A aplicação preserva o logger até a sincronização terminar.

Os testes exercitam o coordenador com `QThread` real e operações controladas por semáforos. Isso evita transformar o teste em uma afirmação sobre um mock e permite provar a ordem de retorno do shutdown. A solução permanece honesta sobre seu limite: aguardar com segurança não equivale a cancelar.

### Validação executada

- O primeiro teste falhou porque a operação injetável e `shutdown()` ainda não existiam.
- Uma mutação removendo `QThread::wait()` fez o teste de espera falhar; restaurar a espera tornou a suíte verde.
- Foram cobertos início concorrente, espera por trabalho ativo, shutdown antes do primeiro início, repetição do shutdown e conclusão durante encerramento.
- Também foram cobertos os dois casos reentrantes de `started()`: novo `start()` não duplica a execução e `shutdown()` impede a criação tardia do trabalho.
- O provider de produção continua baseado na fixture local.
- Build completo: concluído.
- CTest: 18 de 18 testes passaram.
- `git diff --check`: concluído sem erros.

## 8. Fronteira consumida pelo frontend

### Escopo desta parte

Esta parte revisa o contrato que cruza a última fronteira do core: o que o `HomeScreenController` e o `HomeMediaModel` entregam ao QML. O objetivo é garantir que a tela receba dados e estados já preparados, sem consultar infraestrutura, interpretar enums de domínio ou decidir quando recarregar a biblioteca.

O layout visual não foi redesenhado. `Main.qml`, `Home.qml`, `MediaCard.qml` e `StatePanel.qml` continuam responsáveis por composição, responsividade e renderização. A revisão se concentra na direção do fluxo e no significado das propriedades expostas.

### O controller como adaptador de apresentação

`HomeScreenController` depende de `IMediaReader`, não de `SqliteMediaRepository`. Essa escolha mantém SQLite fora da apresentação e permite testar a tela com um reader controlado. O ponteiro é não-owning porque o composition root controla a vida do repository; o controller apenas usa essa porta durante seu próprio ciclo de vida.

O controller converte o resultado de `readAll()` nos estados `loading`, `ready`, `empty` e `error`, além de publicar mensagens e a quantidade de mídias. Isso é trabalho de apresentação: o repository informa sucesso, dados ou erro, enquanto o controller decide como esse resultado será descrito para a tela.

O estado continua exposto como `QString`. Strings são simples de consumir no QML e mantêm o contrato legível, mas não oferecem validação em tempo de compilação. Um erro de digitação em `"loading"` criaria um estado que a tela não reconhece. Um enum Qt registrado no meta-object seria mais seguro quando esse contrato se estabilizar.

### Modelo consumido pelo QML

`HomeMediaModel` adapta `QList<Media>` para `QAbstractListModel`. Os roles públicos são o contrato de cada card: `mediaId`, `title`, `progress`, `score`, `statusLabel` e `coverUrl`. O QML não recebe a entidade inteira nem conhece sua disposição em memória.

Formatações simples pertencem a esse adaptador. Progresso é exposto como `consumido/total`, nota ausente vira um travessão e o status vira texto de apresentação. Antes da revisão, o modelo entregava o valor numérico de `MediaStatus` e `Home.qml` tratava apenas o valor `1` como “Em lançamento”; qualquer outro valor aparecia como “Concluído”. Isso fazia `Unknown` e `NotReleased` produzirem informação falsa.

Agora todos os valores são tratados explicitamente: desconhecido, ainda não lançado, em lançamento e concluído. O texto usa o mecanismo de tradução do Qt no C++, preservando a possibilidade de catálogo de traduções sem devolver ao QML a interpretação do enum de domínio.

O trade-off de entregar strings prontas é que uma futura tela que precise agrupar ou filtrar por status não deve tentar inferir semântica comparando o texto traduzido. Nesse cenário, o controller deverá preparar grupos próprios ou expor também uma chave semântica estável; o texto apresentado nunca deve virar identificador de negócio.

### Quem inicia e repete o carregamento

Antes da correção, `Home.qml` chamava `controller.reload()` em `Component.onCompleted` e novamente ao receber `mediaUpdated`. Embora o acesso ao banco continuasse encapsulado, a tela decidia quando executar o fluxo de leitura. Isso invertia parcialmente a fronteira definida pelo projeto: o QML deveria renderizar o resultado, não coordenar a atualização do modelo.

O carregamento inicial agora é iniciado pelo composition root antes de o controller ser exposto ao engine QML. Quando a sincronização termina, `notifySynchronizationCompleted()` solicita diretamente a nova leitura. O QML perdeu tanto a chamada inicial quanto a conexão de recarga e passou a apenas observar propriedades e modelo.

Essa decisão também remove sinais que não representavam eventos consumidos pela apresentação. `mediaUpdated`, `synchronizationCompleted` e `synchronizationError` apenas duplicavam mudanças já observáveis em `mediaModel`, `state`, `statusMessage` e `errorMessage`. Menos caminhos equivalentes tornam o contrato menor e reduzem o risco de um consumidor esquecer uma das notificações.

### Estados visuais e sincronização

O QML decide como desenhar cada estado: cor, texto secundário, visibilidade do progresso, número de colunas e adaptação ao espaço. Essas são decisões visuais legítimas. Ele não ordena, filtra, agrupa ou consulta mídias.

O contrato atual, porém, usa a mesma propriedade `state` para representar carregamento do conteúdo local e execução da sincronização. Se a biblioteca já estiver carregada e uma sincronização posterior falhar, o controller muda toda a tela para `error`, embora os dados locais continuem disponíveis no modelo. Da mesma forma, `loading` pode significar leitura local ou sincronização remota.

Separar esses eixos agora exigiria decidir a experiência desejada para dados antigos durante atualização, falha recuperável e progresso parcial. A revisão não inventou esse comportamento. A evolução adequada é manter um estado de conteúdo — carregando, pronto, vazio ou indisponível — e um estado de sincronização independente — ociosa, executando, concluída ou falha.

### Erros apresentados

Uma falha ao abrir o repository agora produz a mensagem de status “Não foi possível carregar os dados locais”, em vez de afirmar incorretamente que “A sincronização falhou”. O QML apresenta essa mensagem preparada pelo controller, sem substituir todo erro por um título fixo de sincronização. Essa distinção é importante: inicialização local e sincronização são operações diferentes e pedem diagnósticos diferentes.

Ainda assim, `errorMessage` recebe diretamente a descrição devolvida por `IMediaReader` ou pelo coordenador. Enquanto erros forem apenas `QString`, o controller não consegue separar com segurança uma mensagem adequada ao usuário de detalhes técnicos. Sanitizar por comparação textual repetiria a fragilidade já identificada no classificador de erros. A solução continua sendo um erro estruturado com mensagem pública e detalhe reservado ao log.

### Dívidas identificadas, mas não corrigidas nesta parte

- `state` é uma string e não um contrato tipado; valores inválidos só seriam percebidos em execução.
- Estado do conteúdo e estado da sincronização ainda estão combinados em uma única propriedade.
- `errorMessage` ainda pode expor diretamente detalhes técnicos porque as portas retornam apenas `QString`.
- `HomeMediaModel::setMedia()` reinicia o modelo inteiro. É simples e correto para o volume atual, mas perde seleção e gera mais trabalho visual que atualizações incrementais.
- `progress` e `score` são strings prontas. Isso mantém o QML passivo, mas uma futura edição ou localização numérica poderá exigir valor semântico e texto formatado como contratos distintos.
- O contrato de capas foi separado em `remoteCoverUrl`, `coverSource` e `coverState`. O card usa `Image` assíncrona, preserva a última capa válida durante falhas de download e volta ao placeholder quando o arquivo local desaparece.
- O cache usa metadados SQLite e arquivos em `AppLocalDataLocation/covers`. A publicação é validada e atômica; a capa anterior só é removida depois que o novo arquivo e seu metadado foram persistidos.
- A fila prioriza itens visíveis, deduplica solicitações, limita concorrência, aplica retry apenas a falhas temporárias e descarta conclusões de gerações anteriores após limpeza.
- A validação atual cobre build, testes de unidade/rede local e permanência do processo no startup. A inspeção visual interativa larga/estreita permanece necessária.
- O registro por `setContextProperty` depende de um nome global conhecido pelo QML. Registro de tipos ou injeção explícita por componente poderá tornar dependências de telas futuras mais locais.
- Não há teste automatizado de binding QML nem validação visual multi-tamanho nesta parte; os testes cobrem o contrato C++ e a compilação valida os recursos QML.

### Resultado da parte 8

A fronteira ficou unidirecional: composição e controller iniciam operações; o modelo entrega valores preparados; o QML observa e renderiza. A tela não conhece repository, SQLite, enum de domínio nem política de recarga.

Os testes protegem a recarga após sincronização e o texto apresentado para todos os valores de `MediaStatus`. A revisão também diferencia corretamente falha de carregamento local de falha de sincronização. Permanecem explícitas as decisões maiores que dependem da experiência futura da interface, principalmente a separação dos dois estados e o contrato estruturado de erros.

### Validação executada

- O teste de recarga após sincronização falhou antes de o controller assumir a operação e passou depois da correção.
- O teste dos status falhou na compilação porque `StatusLabelRole` ainda não existia e passou após o modelo fornecer o texto preparado.
- O teste da mensagem de inicialização falhou com a descrição incorreta de sincronização e passou após distinguir o carregamento local.
- A revisão independente encontrou textos QML que ainda classificavam todo erro como sincronização; os bindings passaram a apresentar o `statusMessage` preparado pelo controller e `reload()` deixou de ser invocável pelo QML.
- O provider de produção continua sendo a fixture local; nenhuma integração real com AniList foi ativada.
- Build completo, incluindo compilação do cache QML: concluído.
- CTest: 18 de 18 testes passaram.
- `git diff --check`: concluído sem erros.

## 9. Testes e estabilidade

### Escopo desta parte

Esta parte revisa como o core demonstra seu comportamento: organização dos testes, isolamento de recursos externos, fidelidade entre código testado e código de produção, execução pelo CTest e limites das evidências atuais. Estabilidade aqui não significa apenas “os testes passaram uma vez”; significa que a suíte é determinística, reproduzível em layouts válidos de build e honesta sobre o que ainda não exercita.

### Estrutura atual da suíte

O CMake registra 18 executáveis de teste, que juntos contêm 78 funções de teste Qt. Um executável CTest não equivale a um único cenário: cada classe usa slots privados para representar casos independentes, enquanto o CTest agrega o resultado do processo inteiro.

Os testes formam três grupos práticos, embora todos estejam hoje em `tests/unit`:

- regras e transformações em memória, como merge, classificação, factory, compactação e controller;
- testes de componente com filesystem ou SQLite reais em diretórios temporários;
- testes de integração delimitada, como sincronização por fixture e coordenação com `QThread` real.

Essa combinação é saudável, porque doubles pequenos isolam regras e recursos reais verificam adapters importantes. O nome único `unit`, entretanto, esconde a diferença de custo e finalidade. Quando a suíte crescer, separar ou rotular unitários, componentes e integrações permitirá executar feedback rápido sem perder os cenários mais completos.

### Doubles, fixtures e recursos reais

Os casos de uso usam fakes escritos para as portas da aplicação. Eles armazenam resultados observáveis — lotes persistidos, status atualizados ou erros retornados — e os testes verificam o comportamento do objeto real sob teste, não a existência do fake. Isso mantém os testes legíveis e evita dependência de um framework de mocks.

Filesystem e SQLite são exercitados com `QTemporaryDir`. Cada teste cria dados descartáveis e não toca o banco ou os secrets reais do usuário. Essa é uma propriedade importante de estabilidade: repetir a suíte não depende do estado deixado por uma execução anterior.

As fixtures AniList continuam controladas e versionadas no repositório. Elas provam paginação local, parsing, defaults e persistência por página. Não provam compatibilidade atual com o endpoint oficial, autenticação, rate limit, TLS ou comportamento real de rede.

### Caminho das fixtures — corrigido

`AniListFlowTests` construía o caminho das fixtures a partir de `applicationDirPath()/../tests`. Isso funcionava somente quando o diretório de build era filho direto da raiz do projeto, como `cmake-build-validation`. Um build igualmente válido em `out/item9-validation` procurava `out/tests/fixtures` e falhava.

A falha foi reproduzida em um build aninhado antes da correção. Agora o CMake injeta em `AniListFlowTests` o diretório absoluto de fixtures derivado de `CMAKE_CURRENT_SOURCE_DIR`. Os cenários que exercitam arquivo físico não dependem mais da posição do executável ou do diretório escolhido pelo gerador.

A composição da aplicação possuía o mesmo problema: apontava para `../tests/fixtures` ao lado do executável, o que falharia em um build aninhado ou no pacote distribuído. Como a fixture é deliberadamente o provider de produção nesta fase, `media-library.json` agora é empacotado como recurso Qt e a composição usa `:/fixtures/media-library.json`. Um teste separado prova que `FileAniListDataSource` lê esse recurso.

Os dois caminhos cumprem objetivos diferentes. O caminho absoluto incorporado somente no executável de teste exercita o adapter contra um arquivo físico do checkout; o recurso Qt torna o provider temporário da aplicação independente do layout e do deploy. Quando o provider real substituir a fixture, o bundle poderá ser removido sem alterar o contrato de leitura do adapter.

### CTest e dependências Qt

Os testes são executáveis separados e não passam pelo `windeployqt` da aplicação. O CMake fornece o diretório dos binários Qt no `PATH` de cada teste, permitindo que CTest encontre as DLLs sem depender do terminal que o iniciou.

Foi também definido um timeout de 30 segundos para cada executável. A maioria termina em frações de segundo; portanto, ultrapassar esse limite indica deadlock, espera não liberada ou problema ambiental que precisa aparecer como falha. O timeout não corrige a causa de um hang, mas impede que uma execução automatizada fique bloqueada indefinidamente e perca o diagnóstico dos demais testes.

### Fidelidade entre aplicação e testes

Hoje cada executável lista e recompila diretamente os arquivos `.cpp` de produção de que precisa. Isso facilita começar uma suíte sem criar várias bibliotecas, mas produz dois riscos:

- aplicação e teste podem receber listas de fontes ou definições de compilação diferentes;
- a mesma implementação é recompilada em vários alvos, aumentando tempo de build e manutenção do manifesto.

A dívida de criar bibliotecas internas por camada já está no planejamento. O ganho principal não é apenas velocidade: aplicação e testes passariam a vincular exatamente o mesmo artefato compilado, reduzindo a possibilidade de um teste validar uma composição diferente da entregue.

Os testes de repository usam SQLite real, mas algumas queries são versões mínimas declaradas no próprio teste. Eles provam binding, mapeamento e transações do adapter, não garantem sozinhos que todos os arquivos SQL empacotados mantenham o mesmo contrato. O build valida que os recursos existem e o teste de configuração valida os aliases declarados; ainda falta um teste de integração que carregue a configuração e execute o conjunto real de queries como a aplicação faz.

### Concorrência e determinismo

`InitialSyncCoordinatorTests` usa uma `QThread` real e semáforos para controlar entrada e liberação do trabalho. Isso é mais forte que substituir a thread por um mock: os testes verificam espera no shutdown, exclusão de execuções, reentrância de sinais e não reativação do scheduler.

Os cenários com trabalho bloqueado usam semáforos para controlar as regiões críticas, mas alguns cenários negativos ainda dependem apenas de pequenas esperas de relógio para provar que uma chamada não ocorreu. Esses asserts temporais tendem a ficar frágeis em máquinas muito lentas. Novos testes concorrentes devem preferir eventos, barreiras e condições observáveis; tempo deve ser somente um limite de segurança.

### O que a suíte ainda não prova

- Não existe teste de composição completa para `createApplicationContext()` usando todos os recursos reais empacotados.
- Não existe teste automatizado de bindings QML, criação da janela ou estados renderizados.
- Não há validação visual em múltiplos tamanhos nesta revisão.
- `AsyncLogger` não possui suíte própria para fila, retenção, falha de escrita e shutdown concorrente.
- O provider GraphQL, o update client e o transporte HTTP não são exercitados com um transporte controlado.
- Não há teste ponta a ponta de remoto, merge, outbox e persistência final no mesmo cenário.
- Cancelamento, rate limit, retry assíncrono, recuperação após crash e múltiplas instâncias continuam fora da cobertura.
- Não existe pipeline de integração contínua no repositório para repetir build e CTest em ambiente limpo.
- Não há relatório de cobertura, análise estática ou sanitizadores configurados.
- Nenhum teste contra a API real do AniList foi executado, por decisão de escopo.

### Dívidas identificadas, mas não corrigidas nesta parte

- Separar ou rotular testes unitários, de componente e de integração.
- Criar bibliotecas internas para que aplicação e testes vinculem os mesmos artefatos compilados.
- Testar a composição completa e as queries SQL reais configuradas.
- Adicionar testes do logger e dos adapters de rede com transportes controlados.
- Adicionar teste de binding QML e smoke test de inicialização da janela sem tornar falha de infraestrutura fatal.
- Substituir esperas temporais restantes por sincronização baseada em eventos quando novos cenários concorrentes forem adicionados.
- Criar CI com configuração limpa, build, CTest e preservação de logs em falhas.
- Introduzir cobertura, análise estática e sanitizadores de forma compatível com Qt e MinGW ou em um job adicional suportado.
- Manter validação visual e teste contra serviços externos separados da suíte determinística por fixture.

### Resultado da parte 9

O core possui uma base de testes útil e rápida, com bons limites para filesystem, SQLite, casos de uso e concorrência local. A revisão tornou o acesso às fixtures independente do layout do build e impediu hangs ilimitados no CTest.

A principal conclusão pedagógica é que estabilidade é composta por camadas de evidência. Os testes atuais dão confiança no core local; o build comprova integração de compilação e recursos; nenhum dos dois, isoladamente, comprova UI renderizada, deployment limpo ou integração real com AniList.

### Validação executada

- O build aninhado reproduziu a falha de `AniListFlowTests` antes da correção do caminho das fixtures.
- O mesmo teste passou no mesmo build aninhado depois de o caminho ser fornecido pelo CMake.
- A configuração e o build completos no diretório principal de validação foram concluídos depois das alterações.
- CTest: 18 de 18 executáveis passaram, totalizando 78 funções de teste Qt no código-fonte.
- A configuração gerada confirmou timeout de 30 segundos para os 18 testes.
- `git diff --check`: concluído sem erros.
- O teste do recurso de produção falhou antes de a fixture ser empacotada e passou depois da correção.
- O provider ativo permaneceu baseado em fixture local, agora distribuída como recurso Qt.

## 10. Conclusão transversal e contratos entregues

### Por que existe uma conclusão transversal

Revisar cada camada separadamente ajuda a enxergar responsabilidades, mas uma aplicação não falha respeitando a divisão de pastas. Um contrato de erro definido na aplicação afeta a infraestrutura que o produz e a tela que o apresenta; uma decisão de thread afeta SQLite, rede, logging e shutdown. Por isso, esta última parte verifica se as conclusões anteriores formam um sistema coerente.

O critério adotado não é “todo o produto está pronto”. O critério é mais preciso: identificar o que o core já garante, quais limites o frontend pode consumir sem conhecer infraestrutura e quais promessas ainda seriam falsas nesta fase baseada em fixtures.

### Leitura final da arquitetura

| Parte | Responsabilidade confirmada | Limite atual |
| --- | --- | --- |
| Domínio | Representar mídia, status e mudanças locais sem depender de Qt SQL, HTTP ou QML | `Media` ainda é amplo e permite combinações que tipos mais específicos poderiam impedir |
| Aplicação | Orquestrar leitura, sincronização e outbox por portas substituíveis | Cancelamento e erros ainda não são contratos estruturados ponta a ponta |
| Infraestrutura | Implementar SQLite, arquivos, parsing e transporte sem vazar detalhes para QML | Migrações, secrets seguros e transporte assíncrono ainda não estão completos |
| Apresentação | Preparar modelo, mensagens e estados consumíveis pela Home | Estado local e estado de sincronização ainda compartilham a mesma propriedade textual |
| Composição | Escolher implementações, possuir dependências e controlar o ciclo de vida | O coordenador ainda compõe parte de sua infraestrutura dentro da thread |

Essa tabela também explica por que “ter interfaces” não basta para afirmar que existe desacoplamento. A arquitetura só funciona quando a direção das dependências é respeitada, o composition root escolhe as implementações e cada contrato descreve uma garantia que sua implementação consegue cumprir.

### Contrato que o frontend pode consumir agora

`HomeScreenController` é a fronteira oficial da Home. O QML pode observar:

- `mediaModel`, com os roles `mediaId`, `title`, `progress`, `score`, `statusLabel` e `coverUrl`;
- `state`, atualmente limitado por convenção a `idle`, `loading`, `ready`, `empty` e `error`;
- `statusMessage` e `errorMessage`;
- `synchronizationProgress` e `synchronizationProgressKnown`;
- `mediaCount`.

As propriedades de progresso fazem parte da fronteira, mas a composição atual ainda não publica avanço incremental: ela notifica início, conclusão e falha. Na prática, o progresso fica indeterminado durante a execução e chega a 100% ao concluir. Expor o valor não equivale a já possuir telemetria detalhada do caso de uso.

O frontend deve renderizar esses dados e reagir às notificações. Ele não deve abrir banco, interpretar resposta GraphQL, ordenar ou agrupar regras de negócio, nem reconstruir significado a partir de texto traduzido. `statusLabel`, por exemplo, é texto de apresentação; não é uma chave estável para filtros.

Esse contrato é suficiente para evoluir a Home com dados locais e fixtures, mas possui limites explícitos. `state` ainda é `QString`, `errorMessage` ainda pode receber detalhe técnico e o mesmo estado representa carregamento local e sincronização. Essas limitações estão documentadas para que simplicidade temporária não seja confundida com decisão definitiva.

### Contrato de execução e encerramento

A composição cria as dependências concretas; o trabalho de sincronização cria sua conexão SQLite na thread em que ela será usada; o coordenador impede sobreposição dentro da própria instância; e o shutdown impede novos agendamentos, aguarda a execução ativa e só então permite parar o logger.

Essa garantia é de vida útil, não de cancelamento. `wait()` evita destruir objetos ainda em uso, mas não interrompe filesystem, SQLite ou HTTP bloqueado. Dizer apenas que existe um timeout seria enganoso: enquanto o token de cancelamento não atravessar todas as portas até o transporte, o core somente consegue detectar expiração entre operações cooperativas.

### O que “estável” significa ao fim desta revisão

O core está estável para continuar o desenvolvimento determinístico baseado em fixtures porque:

- as responsabilidades e a direção das dependências estão identificadas;
- os contratos usados pelo caso de uso e pela apresentação possuem doubles simples;
- SQLite e recursos são criados ou resolvidos de forma compatível com o contexto de execução;
- a Home recebe dados preparados por um controller, sem acessar infraestrutura;
- o ciclo de vida possui ordem explícita e cobertura concorrente;
- build e testes exercitam o caminho local sem depender da API real.

“Estável” não significa pronto para produção. O review não validou autenticação real, schema remoto, rate limit, cancelamento de rede, idempotência após falha parcial, secrets protegidos, migrações incrementais, janela QML renderizada ou deployment em máquina limpa. Esses pontos não são pequenos detalhes: são as fronteiras que separam um core confiável para evolução local de uma integração externa confiável.

### Ordem recomendada para as próximas decisões

1. Estruturar erros e cancelamento ponta a ponta, junto com transporte assíncrono e política de scheduler/shutdown.
2. Fechar atomicidade, idempotência, confirmação e recuperação da outbox antes de publicar mutations reais.
3. Evoluir migrations, concorrência SQLite, ownership do modelo e armazenamento de credenciais.
4. Separar estado do conteúdo e estado da sincronização no contrato da apresentação e adicionar testes QML e validação visual.
5. Somente então ativar e validar a conexão real com AniList, mantendo fixtures como suíte determinística.

A ordem importa. Conectar a API antes de definir cancelamento e reconciliação faria o sistema ganhar efeitos externos antes de saber interrompê-los ou recuperá-los. Do mesmo modo, refinar a tela antes de separar seus estados consolidaria um contrato que já sabemos ser temporário.

### Dívidas e fonte de verdade

As dívidas específicas permanecem nas seções de cada parte e são referenciadas por `planning.md`. Esta conclusão não replica a lista completa, porque duas listas independentes inevitavelmente divergem. A ordem acima organiza dependências entre os problemas; as seções anteriores preservam contexto, justificativa e trade-offs.

### Resultado final do review

O core está apto a sustentar a próxima etapa de desenvolvimento local e da apresentação com fixtures. Seus contratos atuais são pequenos, substituíveis e testáveis, e as principais garantias de ownership, persistência local e ciclo de vida estão explícitas.

Ele ainda não deve ser chamado de integração AniList pronta nem de aplicação pronta para produção. Essa distinção é o principal resultado do review: sabemos não apenas o que o código faz, mas também onde suas garantias terminam e qual sequência reduz o risco das próximas mudanças.
