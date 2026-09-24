# Revisão do core da V2

Este documento acompanha a revisão do core da V2 por partes. Cada seção explica o código como ele está, a razão das decisões identificadas, os trade-offs envolvidos, os problemas encontrados, as correções aplicadas e o que foi deliberadamente deixado para uma etapa posterior.

Status das partes:

1. Fundações e arquitetura global — revisada.
2. Domínio — revisado.
3. Contratos da aplicação — revisados.
4. Casos de uso e regras de sincronização — revisados.
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
