# Integração com o AniList

## Objetivo

Preparar a integração da V2 com a API do AniList, que utiliza GraphQL, mantendo a implementação separada por responsabilidades e permitindo testar o fluxo inicialmente sem depender da API externa.

A primeira versão deverá ler dados de um fixture local, aplicar filtros opcionais, simular o mesmo contrato que será usado pelo cliente GraphQL, persistir os dados no SQLite e executar a sincronização fora da thread principal.

## Estado atual

- Passo 1 concluído: estrutura-base da V2 confirmada.
- Passo 2 concluído: contratos iniciais da aplicação criados.
- Passo 3 concluído: modelos iniciais de filtro, paginação, GraphQL, erros e DTO de mídia criados.
- Passo 4 concluído inicialmente: armazenamento de secrets em arquivo texto criado.
- Passo 5 concluído inicialmente: gerenciador de autorização desacoplado do armazenamento criado.
- Passo 6 concluído inicialmente: fábrica HTTP e cliente GraphQL com POST criados.
- Configurações e queries devem permanecer fora das classes: `Settings.json` fornece valores e arquivos `.graphql` fornecem operações.
- Passo 7 concluído inicialmente: providers local e GraphQL criados sob o mesmo contrato.
- O mapeamento de JSON para `AniListMediaDto` é centralizado em `AniListMediaMapper`, com métodos distintos para fixture e GraphQL.
- O mapeamento de `AniListMediaDto` para `Media`, incluindo tipo e status, também pertence ao `AniListMediaMapper`.
- Passo 9 iniciado: repositório SQLite com upsert transacional e preservação de campos locais criado.
- Passo 10 iniciado: worker de sincronização em `QThread` conectado temporariamente ao `main.cpp` para teste local.
- Passo 11 iniciado: fixtures GraphQL e testes do fluxo local de filtro, paginação e sincronização adicionados.
- Passo 12 concluído inicialmente: CMake, dependências Qt, build e CTest integrados.
- Passo 13 concluído como preparação: `ISecretStore` foi mantido como contrato estável e `FileSecretStore` foi explicitamente marcado como implementação temporária.
- Base do merge implementada: `AniListField`, `AniListMergePolicy`, `AniListMergeResult` e `AniListMergePolicyResolver` centralizam as regras por campo.
- Passo 2 do update iniciado: modelo `AniListPendingChange` e factory de validação criados para representar alterações locais pendentes.
- Passo 3 do update iniciado: tabela `anilist_pending_changes` e contrato `IPendingChangeRepository` criados para a outbox.
- Passo 4 do update iniciado: mutations GraphQL externas para progresso, nota, status/lista e exclusão criadas.
- Passo 5 do update iniciado: contrato e cliente de mutations criados, separados do cliente GraphQL genérico.
- Passo 6 do update iniciado: `AniListMergeService` aplica políticas por campo e retorna decisões explícitas.
- Passo 7 do update iniciado: alterações pendentes agora possuem timestamps local/remoto observado e versão remota.
- Passo 8 do update iniciado: retries HTTP configuráveis por `Settings.json` e limitados a timeout/falhas de transporte.
- Passo 9 do update iniciado: processador da outbox criado, confirmando alterações somente após sucesso da mutation.
- Passo 10 do update iniciado: worker dedicado para processar mutations em thread separada criado.
- Passo 11 do update iniciado: testes de sucesso, falha, retry lógico e exclusão sem confirmação adicionados.
- Passo 13 do update concluído inicialmente: contrato do fluxo documentado e primeira suíte completa validada.

## Contrato do fluxo de update

O fluxo de update deverá seguir esta sequência:

```text
Alteração local
    -> AniListPendingChangeFactory
    -> IPendingChangeRepository.Enqueue
    -> AniListMergeService
    -> AniListPendingChangeProcessor
    -> IAniListUpdateClient
    -> mutation GraphQL
    -> confirmação da API
    -> status Succeeded ou Failed
```

Garantias do fluxo:

- uma alteração local deve ser persistida antes do envio remoto;
- uma mutation só será considerada concluída após resposta GraphQL válida sem erros;
- falhas devem permanecer disponíveis para retry;
- erros GraphQL não devem ser tratados como sucesso apenas por possuírem HTTP 200;
- exclusões exigem confirmação explícita;
- políticas de merge devem ser aplicadas por campo;
- o cliente GraphQL não deve conhecer a outbox nem as regras de merge;
- o repositório não deve conhecer detalhes de HTTP ou GraphQL;
- workers não devem executar operações de rede ou banco na thread principal.

Estados previstos para uma alteração pendente:

```text
Pending -> Processing -> Succeeded
                    \-> Failed -> Pending
Pending -> RequiresConfirmation
```

O estado `RequiresConfirmation` não poderá ser enviado automaticamente pelo processador.
- Queries SQL de persistência também são mantidas em arquivos externos e fornecidas aos repositórios pelo construtor.
- Passo 8 iniciado: `AniListSyncService` implementa filtros, paginação, mapeamento para `Media` e persistência por página.
- A V1 é somente referência e não deve ser modificada.
- Todo o desenvolvimento desta funcionalidade deve ocorrer em `V2/`.

## Decisões arquiteturais

### Organização por responsabilidade

A integração não será concentrada em uma única classe. As responsabilidades serão distribuídas entre `domain`, `application` e `infrastructure`:

```text
V2/src/
├── domain/
│   ├── media/
│   └── anilist/
├── application/
│   └── anilist/
└── infrastructure/
    ├── anilist/
    ├── secrets/
    └── database/
```

- `domain`: modelos e regras independentes de Qt Network, SQLite e filesystem.
- `application`: contratos, filtros e caso de uso de sincronização.
- `infrastructure`: HTTP, GraphQL, secrets, fixture local e persistência.
- `presentation`: integração futura com QML, fora do escopo inicial.

### Comunicação com a API

- O AniList será acessado através de GraphQL.
- As operações serão enviadas por `POST`, seguindo o padrão da API.
- O endpoint será configurável, com o padrão esperado do AniList definido na infraestrutura.
- A requisição terá `Content-Type: application/json` e `Accept: application/json`.
- O payload deverá separar `query` e `variables`.
- A autenticação usará `Authorization: Bearer <token>` quando houver token disponível.
- O cliente GraphQL não será responsável por paginação de negócio, persistência ou controle de UI.

### Secrets e autorização

Será criada uma abstração para armazenamento de secrets, permitindo trocar a implementação sem alterar o cliente AniList:

```text
ISecretStore
    └── FileSecretStore       # implementação inicial
    └── SqliteSecretStore      # etapa futura
```

Na primeira etapa:

- os secrets serão lidos e gravados em um arquivo de texto local;
- não haverá criptografia real;
- tokens não serão incluídos no código, fixtures ou logs;
- a interface já deverá representar a futura leitura e descriptografia.

Uma classe separada, como `AniListAuthManager`, recuperará os dados por meio do `ISecretStore` e fornecerá as credenciais necessárias ao cliente GraphQL. Autorização e transporte HTTP permanecerão separados.

### Fonte de dados local e fonte GraphQL

O fluxo de aplicação dependerá de uma abstração de fonte de dados. Haverá duas implementações compatíveis:

```text
IAniListDataSource
    ├── FileAniListDataSource       # primeira etapa
    └── GraphQlAniListDataSource    # preparada para uso real
```

O fixture atual está em:

```text
V2/tests/fixtures/media-library.json
```

Quando necessário, serão adicionados fixtures que representem o envelope GraphQL real, incluindo paginação, erros e campos ausentes.

### Filtros

A sincronização receberá um filtro opcional, representado por um objeto próprio, como `AniListSyncFilter`.

O filtro poderá contemplar, conforme o contrato final:

- usuário;
- tipo de mídia;
- lista/status;
- página inicial;
- quantidade por página;
- demais critérios suportados pelo query GraphQL.

Quando o filtro não for informado, o fluxo deverá permitir uma sincronização geral, especialmente útil na primeira execução do programa.

### Paginação

- As páginas começarão em `1`.
- `perPage` será configurável e terá um valor padrão definido pelo serviço.
- A execução continuará enquanto `hasNextPage` for verdadeiro.
- Cada página poderá ser persistida em uma transação independente.
- A sincronização deverá poder ser interrompida entre páginas.
- A resposta GraphQL não será interpretada por busca textual; será desserializada como JSON estruturado.

### Erros GraphQL

O fluxo deverá diferenciar:

- erro de transporte ou rede;
- erro HTTP;
- JSON inválido;
- erro GraphQL dentro de uma resposta HTTP válida;
- dados incompletos ou incompatíveis;
- falha de persistência;
- cancelamento da operação.

Respostas GraphQL contendo `errors` devem ser tratadas mesmo quando o status HTTP for bem-sucedido. O caso `data: null` não pode ser convertido em uma falha genérica de desserialização.

### Persistência

A persistência será feita pelo repositório SQLite da infraestrutura, por meio de uma interface consumida pela aplicação.

Regras iniciais:

- usar o ID externo do AniList como identificador da mídia;
- realizar insert/update idempotente;
- não criar duplicatas em sincronizações repetidas;
- usar transações;
- usar banco temporário nos testes;
- preservar dados controlados pelo usuário, como progresso e nota pessoal, quando apropriado;
- separar dados externos do catálogo de dados locais do usuário.

### Execução assíncrona

A sincronização será executada em uma thread separada da thread principal.

O worker deverá:

- ser iniciado e encerrado por um controlador próprio;
- emitir sinais de início, progresso, conclusão e erro;
- permitir cancelamento;
- respeitar interrupções entre requisições e páginas;
- criar e usar objetos de rede na thread de execução;
- criar e usar a conexão SQLite na thread correta;
- aguardar o encerramento antes de destruir seus objetos.

A thread principal não deverá executar chamadas bloqueantes de rede ou persistência durante o fluxo normal.

## Componentes planejados

```text
application/anilist/
├── AniListSyncFilter
├── AniListSyncService
├── IAniListDataSource
├── IMediaRepository
└── ISecretStore

infrastructure/anilist/
├── AniListGraphQlClient
├── GraphQlAniListDataSource
├── FileAniListDataSource
├── AniListAuthManager
└── HttpFactory

infrastructure/secrets/
└── FileSecretStore

infrastructure/database/
└── SqliteMediaRepository
```

Os nomes poderão ser ajustados durante o passo de definição dos contratos, desde que as responsabilidades permaneçam separadas.

## Plano de execução

1. Confirmar a estrutura-base da V2. **Concluído.**
2. Definir os contratos da aplicação para secrets, autenticação, fonte de dados, repositório e sincronização. **Concluído inicialmente.**
3. Criar os modelos de filtro, paginação, envelope GraphQL, erros e DTOs de mídia. **Concluído inicialmente.**
4. Implementar o armazenamento inicial de secrets em arquivo texto. **Concluído inicialmente.**
5. Implementar o gerenciador de autorização do AniList. **Concluído inicialmente.**
6. Implementar `HttpFactory` e o cliente GraphQL com `POST`. **Concluído inicialmente.**
7. Implementar os providers local e GraphQL sob o mesmo contrato.
8. Implementar filtros opcionais e sincronização paginada.
9. Implementar persistência idempotente no SQLite.
10. Implementar o worker em thread separada, com progresso, erro e cancelamento.
11. Adicionar fixtures e testes unitários/de integração local.
12. Integrar fontes e testes ao CMake e validar com build, CTest e `git diff --check`.
13. Substituir futuramente o armazenamento em arquivo por secrets criptografados no SQLite. **Contrato preparado; implementação futura.**

## Critérios de validação

- O fluxo local deve funcionar sem acesso à internet.
- Uma sincronização repetida não deve duplicar mídias.
- Um filtro informado deve restringir os dados processados.
- Um filtro ausente deve permitir busca geral.
- Erros GraphQL devem ser expostos de forma distinta de erros de rede.
- O cancelamento não deve deixar a thread ou a conexão SQLite em estado inválido.
- Nenhum token deve aparecer em logs, fixtures ou mensagens de erro.
- Os testes devem utilizar fixtures e bancos temporários.

## Mapeamento inicial de merge do update

As regras de merge serão definidas por campo. A sincronização deverá identificar a origem da informação e aplicar a política correspondente antes de persistir ou enfileirar uma alteração.

| Campo ou grupo | Origem principal | Regra inicial |
|---|---|---|
| ID externo | AniList | Identifica a mídia e não deve ser alterado localmente. |
| Título e títulos alternativos | AniList | O valor recebido do AniList sobrescreve o valor local. |
| Capa, descrição e dados de catálogo | AniList | O valor remoto sobrescreve o valor local. |
| Tipo e status de publicação | AniList | O valor remoto atualiza o catálogo local. |
| Progresso | Usuário | O maior valor entre local e remoto deve prevalecer. |
| Nota pessoal | Usuário | A alteração local deve ser enfileirada e enviada conforme a regra do campo. |
| Lista/status do usuário | Usuário, com prioridade local | A alteração local prevalece sobre o valor remoto e deve ser enviada ao AniList. |
| Arquivos locais e dados de consumo | Usuário | O estado local não deve ser sobrescrito pelo catálogo remoto. |
| Exclusões | Requer confirmação | Uma exclusão remota não deve ser aplicada automaticamente. |

Essas regras poderão evoluir para políticas individuais por campo, mas não devem ser codificadas diretamente no cliente GraphQL ou no repositório SQLite.

## Possível representação no código

Uma alternativa simples é representar a política como um enum e centralizar a decisão em um componente de merge:

```text
AniListFieldPolicy
├── RemoteWins
├── LocalWins
├── MaxValue
├── QueueLocalChange
└── RequiresConfirmation

AniListMergePolicy
└── policyFor(field)

AniListMergeService
└── merge(local, remote, pendingChanges)
```

O mapeamento pode ser uma tabela explícita entre um identificador de campo e sua política. Isso evita espalhar `if` pelo sincronizador e permite testar cada regra isoladamente. Como o projeto ainda não possui reflexão de propriedades para `Media`, a primeira implementação deverá usar identificadores explícitos ou funções de merge por campo, em vez de tentar descobrir campos automaticamente.

### Decisão para a representação das políticas

Será usado um enum para identificar os campos sincronizáveis e outro enum para identificar a política aplicada:

```text
AniListField
├── ExternalId
├── Title
├── AlternativeTitles
├── CoverUrl
├── Synopsis
├── Progress
├── PersonalScore
├── ListStatus
└── LocalFiles

AniListMergePolicy
├── RemoteWins
├── LocalWins
├── MaxValue
├── QueueLocalChange
└── RequiresConfirmation
```

Uma função central, como `PolicyFor(field)`, associará cada campo à sua política. A aplicação da política ficará em funções separadas, como `MergeRemoteWins`, `MergeLocalWins`, `MergeMaxValue`, `QueueLocalChange` e `RequiresConfirmation`.

O merge não deverá depender de nomes de propriedades ou reflexão. Cada campo será processado explicitamente, permitindo associar políticas diferentes a campos que hoje pertencem ao mesmo grupo e tornando os conflitos individualmente testáveis.

## Decisões do fluxo de sincronização

Para `LocalWinsLatest`, a alteração mais recente será identificada por `localUpdatedAt`. Não usaremos uma sequência adicional no teste inicial; ela só será considerada se timestamps empatados exigirem uma ordenação total.

O timeout global será independente do timeout HTTP individual e será configurado em `sync.timeoutMs` no `Settings.json`. Ao expirar, a execução será cancelada, as alterações não concluídas permanecerão na outbox e a falha será considerada temporária.

As alterações serão agrupadas por mídia e enviadas em um único request por mídia. Inicialmente, qualquer falha da mutation marcará a mídia inteira como falha. O tratamento de falhas parciais ficará aberto para revisão após os testes com mocks e a validação da API real.

A sincronização inicial será executada imediatamente. O scheduler somente começará após sua conclusão, e nenhuma nova sincronização poderá iniciar enquanto outra estiver em andamento.

## Fora do escopo inicial

- OAuth completo e fluxo de login interativo.
- Criptografia real dos secrets.
- Armazenamento criptografado no SQLite.
- Mutations para alterar listas, notas ou progresso no AniList.
- Sincronização automática periódica.
- Integração final com QML.
