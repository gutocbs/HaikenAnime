# SERVICE.md — HaikenAnime V2

> Documento de referência do programa: **o que ele faz, como está organizado, como os fluxos funcionam
> e quais padrões seguir** ao adicionar ou alterar funcionalidades. Mantenha-o atualizado sempre que um
> contrato, fluxo, tabela ou padrão mudar.
>
> - Visão de usuário, build e execução: [`README.md`](README.md)
> - Regras para assistentes de IA: [`AGENTS.md`](AGENTS.md)
> - Specs, planos e validações de features: `docs/superpowers/` (pasta ignorada pelo `.gitignore`; arquivos
>   versionados com `git add -f`)

---

## 1. O que o programa faz

HaikenAnime é um aplicativo desktop (Qt 6 / QML, Windows) para organizar uma biblioteca de anime e mangá:

| Funcionalidade | Resumo |
| --- | --- |
| **Biblioteca (Home)** | Lista as mídias da lista pessoal do AniList guardadas localmente, com filtros por tipo (anime/mangá), lista (assistindo, planejado, …), busca e ordenação. Painel de detalhes, edição de progresso/nota/status e remoção da lista. |
| **Sincronização AniList** | Baixa a lista pessoal do usuário (anime e mangá) e envia as edições locais (outbox de *pending changes*). Roda em background com agendamento adaptativo, retry e cancelamento. |
| **Conta AniList** | Login OAuth (implicit grant) via protocolo `haikenanime://`; credenciais no Windows Credential Manager; remoção de conta sem perder edições pendentes (cada edição pertence a uma conta). |
| **Catálogo sazonal** | Navega pelo catálogo de uma temporada no AniList (sem login) e permite adicionar/editar mídias na lista pessoal. |
| **Capas** | Baixa, valida e mantém em cache (disco + SQLite) as capas em três qualidades. |
| **Biblioteca local** | Varre pastas configuradas, registra os arquivos de vídeo, reconhece título/temporada/episódio com Anitomy e associa à mídia do catálogo. Permite "assistir próximo episódio". |
| **Reconhecimento de episódios** | Plugins de player (**mpv** via IPC e **Plex** via API) observam a reprodução; ao completar um episódio o progresso é atualizado automaticamente e entra no outbox. Telas de histórico de reconhecimento e "Assistindo agora". |
| **Configurações** | Preferências do usuário (pastas, extensões, escala de nota, qualidade de capa, idioma, título preferido, listas sincronizadas, conteúdo adulto, …), configurações dinâmicas de plugins, limpeza de cache. |
| **Idiomas** | Interface em pt-BR e en (Qt Linguist). |

Princípio central: **o banco local é sempre a fonte exibida**. A janela abre com os dados persistidos e a
sincronização atualiza em background; falha de rede ou de infraestrutura nunca impede a abertura da janela.

---

## 2. Stack, build e testes

- **C++20**, **Qt 6.8** (Core, Gui, Network, Qml, Quick, QuickControls2, Sql, Test, LinguistTools), **CMake ≥ 3.21**
- **SQLite** (driver `qsqlite` do Qt), **AniList GraphQL**, **Anitomy** (`lib/anitomy`, compilado junto)
- Windows: `Advapi32` (Credential Manager e registro do protocolo), ícone via `resources/icons/HaikenAnime.rc`

```powershell
# Build + testes (MinGW; para MSVC troque o CMAKE_PREFIX_PATH e use um prompt do VS)
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure

# Um teste específico
ctest --test-dir build -C Debug -R HomeScreenControllerTests --output-on-failure

# Pacote de release (MSVC + Ninja) → dist/
powershell -ExecutionPolicy Bypass -File scripts/build-release.ps1
```

Estrutura do build:

- `anitomy` (biblioteca estática com `lib/anitomy`) e `haiken_core` (biblioteca estática com **todas** as
  fontes de `src/`, coletadas por `GLOB_RECURSE`) são compiladas uma vez e ligadas pelo app e pelos testes.
- `HaikenAnime` = `main.cpp` + `haiken_core` + módulo QML + recursos + traduções.
- Queries SQL, GraphQL e migrações entram por `GLOB` e são expostas pelo nome do arquivo
  (`:/sqlite/queries/<arquivo>.sql`); criar um arquivo novo não exige mexer no CMake (basta reconfigurar).
- O `POST_BUILD` só copia plugins (`platforms`, `sqldrivers`), runtime MinGW e DXC. O `windeployqt` roda
  apenas no `scripts/build-release.ps1`; builds de desenvolvimento carregam o Qt do kit que está no `PATH`.

---

## 3. Mapa do repositório

```text
main.cpp                  # Entrada: cria o contexto, os controllers, faz a fiação de sinais e carrega o QML
src/
├── domain/               # Entidades e regras puras (sem Qt SQL/QML/rede)
│   ├── media/            #   Media, MediaType, MediaStatus, UserListStatus, MediaLink
│   ├── anilist/          #   AniListField/FieldValue (campo e valor de uma mudança pendente)
│   ├── library/          #   LocalFileRecord
│   └── recognition/      #   PlaybackSession, EpisodeCandidate, RecognitionPolicy, RecognitionRecord
├── application/          # Casos de uso, serviços e interfaces (ports I*)
│   ├── anilist/          #   sync, outbox (pending changes), auth/OAuth, ownership, remoção de conta
│   ├── cache/            #   ClearLocalCacheUseCase
│   ├── catalog/          #   tipos e port do catálogo sazonal
│   ├── configuration/    #   Settings (arquivo), UserPreferences (banco) e validador
│   ├── covers/           #   coordenação de download/cache de capas
│   ├── library/          #   tipos de scan, resolução local → mídia, leitura do próximo episódio
│   ├── media/            #   repositório de mídia (ports), títulos, progresso, edição da lista pessoal
│   ├── recognition/      #   pipeline de reconhecimento, plugins, histórico, settings de plugin
│   └── scheduling/       #   tipos/política do agendador de sync
├── infrastructure/       # Adapters concretos
│   ├── anilist/          #   cliente GraphQL, parsers, mapper, data sources, update client, OAuth Windows
│   ├── configuration/    #   JsonSettingsReader (Settings.json)
│   ├── covers/           #   arquivo final, temporário e downloader Qt
│   ├── database/         #   SqliteDatabase (+migrações), repositórios Sqlite*, SqlQueryStore
│   ├── library/          #   scanner, enumerador de diretórios, Anitomy, abridor de arquivo
│   ├── logging/          #   AsyncLogger
│   ├── recognition/      #   mpv (IPC), plex (HTTP/auth), launcher de processo
│   └── secrets/          #   Windows Credential Manager (AniList e plugins)
├── presentation/         # Controllers QObject e QAbstractListModel expostos ao QML
│   ├── home/  seasonal/  settings/  sync/  recognition/
│   └── media/            #   textos de mídia compartilhados (progresso, rótulos, detalhes)
└── app/                  # Composição e coordenação de workflows (threads, timers, runtime de sync)
resources/
├── qml/                  # Telas e componentes (módulo QML "HaikenAnime")
├── sqlite/               # migrations/*.sql, queries/*.sql
├── anilist/              # queries/*.graphql, mutations/*.graphql
├── icons/  images/
tests/
├── unit/                 # Uma suíte Qt Test por arquivo, um executável CTest por suíte
└── fixtures/             # Respostas GraphQL/mpv/Plex gravadas, snapshot do schema SQLite
translations/             # HaikenAnime_pt_BR.ts, HaikenAnime_en.ts
Settings.json             # Configuração da aplicação (embutida como recurso :/config/Settings.json)
scripts/build-release.ps1 # Empacotamento de release
lib/anitomy/              # Parser de nomes de arquivo (terceiro)
```

---

## 4. Arquitetura em camadas

```text
 QML (resources/qml) ──► presentation (controllers/models) ──► application (casos de uso + ports I*)
                                                                    ▲            │
                                                    app (composição) │            ▼
                                                                    └── infrastructure (implementa ports)
                                                           domain ◄── usado por todas as camadas
```

Regras de dependência (obrigatórias):

- `domain` não depende de QML, SQL, rede, AniList ou filesystem.
- `application` depende só de `domain` e de **interfaces** (`I*`). Nunca inclui `infrastructure`.
- `infrastructure` implementa as interfaces de `application` (SQLite, AniList, disco, players, segredos, log).
- `presentation` adapta casos de uso ao QML (propriedades, sinais, `Q_INVOKABLE`, modelos). Não acessa SQL nem rede.
- `app` é o único lugar que conhece implementações concretas e as liga (factories, contexto, threads).
- **QML só apresenta**: filtros, ordenação, regras de progresso, decisões de sync e de player ficam em C++.
- Configuração é lida apenas por `JsonSettingsReader` → `Settings`; preferências do usuário apenas por
  `IUserPreferencesRepository`.

---

## 5. Ciclo de vida da aplicação

### 5.1 Inicialização (`main.cpp`)

1. `QGuiApplication`; se o processo foi aberto por um callback OAuth (`haikenanime://oauth/callback#…`),
   `AniListOAuthCallbackReceiver` encaminha a URL para a instância em execução (via `QLocalServer`) e sai.
2. `createApplicationContext()` (`src/app/ApplicationComposition.cpp`) monta tudo e devolve um
   `ApplicationContext` (dono de todas as dependências):
   1. lê `Settings.json` (`:/config/Settings.json`) e inicia o `AsyncLogger`;
   2. abre o SQLite e aplica as migrações;
   3. cria repositórios de mídia/outbox, `PersonalListChangeService`;
   4. cria os coordenadores de **scan** e **reconhecimento local** e o leitor de episódios locais;
   5. cria o contexto de **reconhecimento de episódios** (`createRecognitionContext`, plugins mpv/Plex);
   6. lê `UserPreferences` do banco (ou grava os defaults de `Settings.json` na primeira execução);
   7. monta capas (cache, temporário, downloader, coordenador) e limpeza de cache;
   8. carrega credenciais AniList, cliente GraphQL, cliente *viewer* e launcher OAuth;
   9. cria o `AdaptiveSyncRuntime` (thread própria) e o coordenador do catálogo sazonal.
   Falha ao abrir/migrar o banco ou ao carregar queries → `initializationError` preenchido e repositórios
   nulos; a janela abre mesmo assim e mostra o erro.
3. Registra o protocolo `haikenanime` no registro do Windows e instala a tradução do idioma salvo.
4. Cria os controllers (`HomeScreenController`, `SeasonalCatalogController`, `SettingsController`,
   `SynchronizationStatusController`, `PluginSettingsController`, `EpisodeRecognitionController`,
   `NowWatchingController`) e conecta sinais entre eles e o runtime de sync.
5. Expõe os controllers como **context properties** e carrega `HaikenAnime/Main.qml`.
6. Se há conta conectada e a sync está habilitada, agenda `UserList` + `PendingChanges`; se o
   reconhecimento automático está ligado, agenda o scan inicial das pastas (`scheduleStartupLibraryScan`).

Context properties disponíveis no QML: `homeController`, `settingsController`, `synchronizationStatus`,
`seasonalCatalogController`, `pluginSettingsController`, `recognitionController`, `nowWatchingController`
(os três últimos podem ser `null` se o reconhecimento não pôde ser composto).

### 5.2 Desligamento

`QCoreApplication::aboutToQuit` é o caminho único de saída: para o runtime de sync (espera até 5 s), depois
scan, reconhecimento local, reconhecimento de episódios, capas e por fim o logger. A ordem dos membros do
`ApplicationContext` garante que *workers* sejam destruídos (e suas threads unidas) antes das dependências.

### 5.3 Threads e conexões SQLite

| Thread | O que roda | Conexão SQLite |
| --- | --- | --- |
| GUI | controllers, QML, leituras da Home, edições, reconhecimento de episódios, capas | conexão principal do `ApplicationContext` |
| `AdaptiveSyncRuntime` | `AdaptiveSyncCoordinator` (agenda) | própria (repositório de estado de tarefas) |
| Job de sync (`QThread::create` por tarefa) | `ApplicationSyncTaskExecutor` (rede + persistência) | própria, aberta no job |
| Scan local / reconhecimento local | `LocalLibraryScanCoordinator` / `LocalLibraryRecognitionCoordinator` | própria por worker |
| Logger | escrita em arquivo | — |

Regra: **uma `QSqlDatabase` nunca é usada fora da thread que a criou**. Workers recebem *factories* que
abrem sua própria conexão (`SqliteDatabase(databasePath)`). Resultados voltam à GUI por sinais
(conexões *queued*).

---

## 6. Fluxos funcionais

### 6.1 Biblioteca (Home)

- `HomeScreenController` lê todas as mídias (`IMediaReader::readAll`) e mantém dois `HomeMediaModel`:
  `mediaModel` (filtrado/ordenado) e `fullMediaModel` (biblioteca completa).
- Filtros, busca e ordenação são aplicados no controller; a ordenação escolhida é salva em
  `UserPreferences.homeSortKey` (sinal `sortPreferenceChanged` → `main.cpp`).
- Títulos exibidos seguem `preferredTitleKey` (`MediaTitleResolver`); progresso com total desconhecido é
  exibido como `?` (`FormatProgress`). Rótulos de status/tipo e o mapa do painel de detalhes vêm de
  `presentation/media/MediaPresentation` (um único lugar para todas as telas).
- Recarrega quando: sync de `UserList` termina, catálogo sazonal salva uma edição, reconhecimento aplica
  progresso, reconhecimento local persiste um lote.

### 6.2 Edição da lista pessoal e outbox

- Toda edição (progresso, nota, status, remoção) passa por `PersonalListChangeService`, que usa
  `IPersonalListChangeWriter` (`SqlitePersonalListChangeWriter`) para **numa transação** atualizar a mídia e
  enfileirar `AniListPendingChange` em `anilist_pending_changes`.
- Cada mudança recebe `ownerUserId` = conta conectada, ou a última conta conhecida
  (`AniListAccountOwnership::ResolveOwnerUserId`). Mudanças só são enviadas à conta dona.
- Após salvar, o serviço notifica o runtime (`requestNow(PendingChanges)`) se houver token.
- Remoção: a mídia fica oculta (`personal_list_removed_at`) e uma deleção é enfileirada; sincronizações
  posteriores não a trazem de volta; editar de novo pelo catálogo/editor a re-adiciona.

### 6.3 Sincronização AniList

**Agendamento** (`src/app/AdaptiveSync*`, `src/application/scheduling`):

- Só existem duas partições: `UserList` (intervalo padrão 1 h, prioridade maior) e `PendingChanges` (1 min).
  O estado de cada uma (`SyncTaskState`: status, próxima execução, falhas, checkpoint de página, geração) é
  persistido em `sync_task_state`; linhas de outros tipos (restos de versões antigas) são ignoradas na leitura.
- Uma tarefa por vez. `PendingChanges` só roda depois da primeira `UserList` bem-sucedida. Um pedido feito
  durante a execução da partição a reexecuta logo depois (gatilho `LocalChange`).
- `SyncTaskPolicy` decide quando rodar, backoff exponencial com teto, cooldown e TTL de dados obsoletos.
- `AdaptiveSyncRuntime` hospeda o coordenador numa thread, expõe `requestNow`, `cancel` (com
  confirmação na thread da GUI) e `shutdown`, e emite `BackgroundTask*` consumidos por
  `SynchronizationStatusController` (barra de progresso) e pela Home.

**Execução** (`ApplicationSyncTaskExecutor`, uma thread por job):

- Sem conta (ou conta suspensa durante remoção) → tarefa concluída como "pulada", sem erro.
- `UserList`: para `ANIME` e depois `MANGA`, baixa as páginas da lista do usuário filtrando pelos status
  habilitados (`UserPreferences.enabledUserLists`), faz *upsert* das mídias, grava o checkpoint de página,
  concilia mudanças pendentes com o estado remoto (`AniListPendingChangeReconciler`: vence o maior
  progresso; nota e status locais aguardam envio) e, se o snapshot for
  completo, marca como removidas da fonte as mídias que sumiram (`ReconcileCompleteAniListSnapshot`).
- `PendingChanges`: adota mudanças sem dono, e `AniListPendingChangeDrain` → `AniListPendingChangeProcessor`
  envia os grupos via `AniListUpdateClient` (mutations GraphQL). Uma mudança só sai da fila após
  confirmação da API; cancelamento devolve a mudança à fila sem contar tentativa. O resultado de cada
  envio de progresso é refletido no histórico de reconhecimento (`RecognitionSyncFeedback`).
- Erros são classificados (`AniListSyncErrorClassifier` → `AniListSyncErrorCategory`) para decidir retry.

**Conta** (`AniListAuthManager`, `RemoveAniListAccountUseCase`): login abre o navegador com a URL OAuth, o
callback chega pelo `AniListOAuthCallbackReceiver`, o token é validado com a query `viewer` e salvo no
Credential Manager. Remover a conta: cancela a sync em andamento, suspende credenciais, apaga o segredo,
lembra o `lastAniListUserId` (edições offline continuam pertencendo a ela).

### 6.4 Catálogo sazonal

`SeasonalCatalogController` → `SeasonalCatalogCoordinator` → `GraphQlSeasonalCatalogDataSource`
(query `seasonal-catalog.graphql`, sem autenticação). O coordenador tem cache em memória com TTL e limite
de entradas, intervalo mínimo entre requisições e paginação. Adição/edição na lista pessoal passa por
`SeasonalPersonalListService` → `PersonalListChangeService` (mesmo outbox da Home).

### 6.5 Capas

`CoverDownloadCoordinator` recebe janelas de capas visíveis (`RequestCoverWindow` da Home), escolhe a URL
pela qualidade (`CoverSourceResolver`), baixa com concorrência limitada (`QtCoverDownloader`) num arquivo
temporário (`CoverTemporaryStore`), valida tamanho/dimensões, move para o cache final (`CoverFileStore`,
em `AppLocalDataLocation/covers`) e registra em `cover_cache`. Falhas entram em *cooldown*.
`ClearLocalCacheUseCase` limpa cache de capas e temporários.

### 6.6 Biblioteca local

1. **Scan** (`LocalLibraryScanCoordinator` + `LocalLibraryScanner` + `QtDirectoryEnumerator`): para cada
   pasta de `libraryRoots`, enumera arquivos com as extensões configuradas, grava em lotes em `local_files`
   (registro do scan em `library_scans`) e marca como indisponíveis os arquivos que sumiram — só quando o
   scan da pasta termina bem; uma pasta inacessível não impede as outras. Grava pelo contrato
   `ILocalFileScanRepository`.
2. **Reconhecimento** (`LocalLibraryRecognitionCoordinator`): ao fim do scan de uma pasta (ou ao salvar
   pasta/nomes alternativos de uma mídia), lê arquivos pendentes, extrai título/temporada/episódio com
   `AnitomyLocalFileRecognizer` e associa à mídia via `LocalMediaResolver` + `LocalMediaCatalogIndex`
   (a pasta da mídia que contém o arquivo vence; senão, títulos e nomes alternativos normalizados com
   marcador de temporada; mangá e novel ignorados). Arquivos locais são sempre lidos como anime. Lê e
   grava pelo contrato `ILocalFileRecognitionRepository`, em lotes, e emite sinais que atualizam a Home.
   Pedidos durante uma execução viram uma única execução seguinte.
3. **Assistir** (`ILocalEpisodeReader`, implementado por `SqliteLocalEpisodeReader`, + `ILocalFileOpener`): a Home mostra o próximo episódio disponível e
   abre o arquivo. `MpvAwareLocalFileOpener` abre pelo mpv (com IPC) quando o plugin mpv está habilitado,
   senão usa o handler padrão do sistema.

### 6.7 Reconhecimento de episódios (players)

Pipeline (`EpisodeRecognitionService`):
`IPlaybackPlugin` (mpv/Plex) → `IPlaybackObserver` → `CatalogEpisodeCandidateResolver` (associa título à
mídia) → `PlaybackObservationService` + `RecognitionPolicy` (reprodução contínua / limiar de conclusão) →
`ProgressDecisionService` (aplicar, ignorar, já assistido…) → `RecognitionHistoryService`
(`recognition_record`) → `PersonalListProgressWriter` → `PersonalListChangeService` (outbox).

- **mpv** (`infrastructure/recognition/mpv`): lança o mpv com `--input-ipc-server` e lê propriedades pelo
  named pipe (`MpvIpcClient`, `QtLocalSocketMpvTransport`).
- **Plex** (`infrastructure/recognition/plex`): autenticação por PIN no navegador, descoberta de servidores,
  *polling* de sessões filtrado por usuários selecionados.
- Configurações de plugin são **dinâmicas**: cada plugin declara um `PluginSettingsSchema`;
  `PluginSettingsService` valida e persiste em `plugin_setting` (segredos no Credential Manager via
  `WindowsPluginSecretStore`); `applyRecognitionSettings` empurra os valores para o runtime.
- `ActiveSessionRegistry` mantém as sessões em andamento, exibidas por `NowWatchingController`.
- `EpisodeRecognitionController` + `RecognitionListModel` exibem e apagam o histórico.

### 6.8 Configurações e preferências

- **`Settings.json`** (somente leitura, embutido): endpoint/IDs AniList, HTTP (timeout/retries), capas,
  catálogo sazonal, sync, retenção de logs e **defaults** de `UserPreferences`.
- **`UserPreferences`** (tabela `user_preferences`, uma linha): tudo que o usuário altera em
  Configurações. `UserPreferencesValidator` valida antes de salvar.
- `SettingsController` mantém um *draft*, valida, salva e emite `preferencesApplied`; `main.cpp` repassa as
  novas preferências aos demais controllers.

### 6.9 Logging

`AsyncLogger` grava em `AppDataLocation/logs/haikenanime-AAAA-MM-DD.log` numa thread própria, com
retenção em dias (`logging.retentionDays`). Níveis `Info`/`Warning`/`Error` e categorias `LogCategory`
(Application, Configuration, Database, Migration, QueryStore, Sync, Covers,
LocalLibrary). Camadas internas recebem *callbacks* (`std::function<void(const QString&)>`: `auditLogger`,
`diagnostics`, `info`) em vez do logger concreto.

**Nunca registrar** tokens, códigos/fragmentos de callback OAuth, tokens Plex ou segredos de plugin.

---

## 7. Persistência

- Arquivo: `AppDataLocation/haikenanime.sqlite` (`%APPDATA%/HaikenAnime/…` no Windows).
- `PRAGMA foreign_keys = ON`, `busy_timeout = 10000`, `journal_mode = WAL`.
- Tabelas: `schema_version`, `media`, `anilist_pending_changes`, `cover_cache`, `user_preferences`,
  `library_scans`, `local_files`, `sync_task_state`, `plugin_setting`, `recognition_record`.

### 7.1 Migrações

Os DDLs ficam em `resources/sqlite/migrations/NNN-descricao.sql`, embutidos em `:/sqlite/migrations/` (GLOB
no CMake). `SqliteDatabase::migrate()` percorre a lista declarativa `MigrationSteps()` **a cada
inicialização**, numa única transação. A cadeia é idempotente:
- `Script(arquivo)` roda um DDL (`CREATE TABLE IF NOT EXISTS …`);
- `AddColumn(tabela, coluna, arquivo)` só roda o `ALTER` se a coluna ainda não existe (`PRAGMA table_info`);
- `MarkVersion(n)` grava `INSERT OR IGNORE INTO schema_version`;
- no fim, todas as versões de 1 a `LatestSchemaVersion` precisam estar gravadas; senão, rollback.

Para adicionar uma migração:
1. criar o(s) arquivo(s) `NNN-…sql` com o DDL (um `ALTER TABLE … ADD COLUMN` por arquivo);
2. acrescentar ao fim de `MigrationSteps()` os passos e um `MarkVersion(NNN)`, e atualizar
   `LatestSchemaVersion`;
3. regenerar `tests/fixtures/sqlite/migrated-schema.txt` (snapshot do schema final verificado por
   `SqliteDatabaseTests::migrationProducesTheExpectedSchema`) e cobrir o upgrade de um banco na versão
   anterior sem perda de dados.

### 7.2 Queries

- SQL nunca fica hardcoded em C++: cada query é um arquivo em `resources/sqlite/queries/`, embutido (por
  GLOB) como `:/sqlite/queries/<arquivo>.sql` e carregado por `SqlQueryStore`.
- O caminho de cada query é uma constante em `src/infrastructure/database/SqliteQueries.h`
  (`SqliteQueries::ReadMedia`, …). Query nova = arquivo novo + constante nova. O CMake já pega o arquivo; os
  alvos de teste que leem a query a embutem com `qt_add_resources`.
- Repositórios recebem só a conexão (`SqliteMediaRepository(db)`) e usam as constantes internamente. O
  mapeamento de colunas de mídia (ler, *bind* do *upsert* e da edição da lista) fica em `SqliteMediaMapper`;
  o formato dos valores do outbox fica em `SqlitePendingChangeCodec`.
- O mesmo vale para GraphQL: `resources/anilist/{queries,mutations}/*.graphql`, carregados por
  `GraphQlQueryStore` de `:/anilist/queries/…` e `:/anilist/mutations/…`.

### 7.3 Recursos Qt

| Prefixo | Conteúdo |
| --- | --- |
| `:/config/Settings.json` | configuração |
| `:/sqlite/queries/`, `:/sqlite/migrations/` | SQL |
| `:/anilist/queries/`, `:/anilist/mutations/` | GraphQL |
| `:/icons/`, `qrc:/qt/qml/HaikenAnime/…` | ícone e módulo QML |
| `:/i18n/` | traduções compiladas |

---

## 8. Padrões de desenvolvimento

### 8.1 Código C++

- **Inglês** em nomes de classes, métodos, variáveis, chaves JSON e APIs internas. Textos de interface em
  `qsTr()` (QML) / `tr()`.
- Uma classe por par `.h`/`.cpp` no módulo a que pertence; *include guards* `HAIKENANIME_<NOME>_H`.
- Entidades de domínio são structs/classes de dados com campos públicos em `PascalCase` (`Media::Name`,
  `MediaLink::Url`); conversões enum ↔ chave ficam no header do enum (`UserListStatusKey`,
  `CardStatusPresentationKey`) e não são reescritas nos controllers.
- **Interfaces** com prefixo `I` e destrutor virtual, definidas em `application/` (ports). Implementações
  em `infrastructure/`, nomeadas pela tecnologia (`Sqlite*`, `Qt*`, `Windows*`, `GraphQl*`).
- **Erros**: métodos falíveis retornam `bool` com `[[nodiscard]]` e preenchem `QString &error`
  (mensagem segura, sem segredos). Exceções não são usadas para fluxo.
- **Ownership**: `std::unique_ptr` no dono (contexto/composição); dependências injetadas por referência ou
  ponteiro não-dono no construtor. Sem singletons globais.
- **Callbacks** como `std::function` (notificadores, provedores, loggers) para manter camadas desacopladas.
- **Opcionais**: `std::optional` para valores ausentes; coleções nunca nulas.
- Status de lista ↔ AniList (`CURRENT`, `PAUSED`, …) só por `AniListListStatus.h`; `AniListField` é gravado
  como inteiro no outbox, então um valor novo ganha número próprio e os existentes nunca mudam.
- Comentários `/** */` ou `///` em APIs públicas cujo nome não é autoexplicativo (o quê, argumentos,
  retorno, condições de erro e thread de execução quando relevante).
- Toda chamada de rede tem timeout e retry limitado (`HttpSettings`); operações longas aceitam um
  *probe* de cancelamento (`std::function<bool()> isCancelled`).
- Estado entre threads só por sinais *queued* ou estruturas protegidas por mutex; nada de `QSqlDatabase`
  compartilhada.

### 8.2 Apresentação e QML

- Controllers expõem estado por `Q_PROPERTY` + sinais `…Changed`; ações por `Q_INVOKABLE`.
- Listas grandes usam `QAbstractListModel` (ex.: `HomeMediaModel`, `RecognitionListModel`).
- O QML não decide regras: recebe dados prontos (rótulos, *flags* `can*`, opções de combo).
- Componentes próprios (`AppButton`, `AppTextField`, `AppSpinBox`, `AppComboBox`) em vez dos controles
  nativos; parâmetros de sinal sempre declarados (`onToggled: function(checked) { … }`).
- Novos botões de cabeçalho/barra entram **à direita**, com um único ponto de entrada por feature.
- Todo texto visível em `qsTr()`; após mudar textos, atualizar os `.ts` (alvo `update_translations`).

### 8.3 Configuração

- Novo parâmetro técnico → `Settings.json` + struct em `application/configuration` + `JsonSettingsReader`
  (com validação e default) + `JsonSettingsReaderTests`.
- Nova preferência do usuário → migração (coluna em `user_preferences`) + `UserPreferences` (campo e
  `operator==`) + queries `read/upsert-user-preferences.sql` + `SqliteUserPreferencesRepository` +
  `UserPreferencesValidator` + default em `Settings.json`/`JsonSettingsReader` + `SettingsController` e QML.

### 8.4 Segurança

- Tokens AniList e segredos de plugin apenas no Windows Credential Manager.
- Nada de segredos em log, mensagens de erro exibidas, fixtures ou commits.
- Mensagens de erro de rede/API são reduzidas a texto seguro (`safeErrorDetail`, `safeMessage`).

---

## 9. Testes

- Framework **Qt Test** (`QTEST_MAIN`, `private slots`, `QCOMPARE`/`QVERIFY`), sem framework de mock:
  *doubles* são classes escritas à mão no próprio arquivo de teste (ex.: `Fake*Repository`).
- Cada suíte em `tests/unit/<Nome>Tests.cpp` é registrada com uma linha
  `haiken_add_test(<Nome> [QT <módulos>] [MIGRATIONS] [SOURCE_DIR] [DEFINITIONS …] [INCLUDES …])`: vira um
  executável próprio, ligado em `haiken_core`, com `PATH` do Qt e `TIMEOUT 30` no CTest.
  - `QT` lista os módulos além de Core/Test. Declare só o que o teste usa: com `Gui` o `QTEST_MAIN` cria
    uma `QGuiApplication`; sem ele, uma `QCoreApplication`.
  - `MIGRATIONS` embute as migrações (suítes que abrem banco); `SOURCE_DIR` define
    `HAIKENANIME_TEST_SOURCE_DIR` com a raiz do repositório.
- Queries e configuração que o teste lê por recurso são adicionadas logo abaixo, com `qt_add_resources`
  no alvo do teste (cada suíte embute só as queries de que precisa).
- Persistência sempre em **banco temporário** (`QTemporaryDir`); nunca o banco real do usuário.
- Rede e players nunca reais: fixtures em `tests/fixtures/` e transportes/HTTP falsos
  (ex.: `PlexTestSupport.h`, transporte mpv falso).
- Testes de QML (`QmlStructureTests`, `QmlRecognitionStructureTests`) carregam os arquivos de
  `resources/qml` com controllers falsos.
- Nome do teste descreve comportamento e resultado esperado (`deletionRequiresConfirmation`); um
  comportamento por teste; cobrir erros e cancelamentos, não só o caminho feliz.
- Toda mudança concluída tem teste correspondente; rode a suíte afetada e depois o `ctest` completo.

Mapa rápido de suítes por área:

| Área | Suítes principais |
| --- | --- |
| Domínio | `RecognitionDomainTests`, `AlternativeNamesTests`, `MediaTitleResolverTests` |
| SQLite | `SqliteDatabaseTests`, `SqliteRepositoryTests`, `Sqlite*RepositoryTests`, `SqlQueryStoreTests` |
| AniList | `AniListGraphQl*Tests`, `AniListFlowTests`, `AniListPendingChange*Tests`, `AniListUpdateClientTests`, `AniListAuthManagerTests`, `AniListOAuth*Tests`, `RemoveAniListAccountUseCaseTests` |
| Sync | `SyncTaskPolicyTests`, `AdaptiveSyncCoordinatorTests`, `AdaptiveSyncRuntimeTests` |
| Biblioteca local | `LocalLibraryScanner*Tests`, `LocalLibraryScan*Tests`, `LocalLibraryRecognition*Tests`, `AnitomyLocalFileRecognizerTests`, `LocalMediaResolverTests`, `SqliteLocalEpisodeReaderTests` |
| Capas/cache | `Cover*Tests`, `QtCoverDownloaderTests`, `ClearLocalCacheUseCaseTests` |
| Reconhecimento | `Mpv*Tests`, `Plex*Tests`, `Plugin*Tests`, `PlaybackObservationServiceTests`, `ProgressDecisionServiceTests`, `RecognitionHistory*Tests`, `RecognitionSyncFeedbackTests`, `EpisodeRecognition*Tests`, `RecognitionCompositionTests`, `ActiveSessionRegistryTests`, `NowWatchingControllerTests` |
| Apresentação | `MediaPresentationTests`, `HomeScreenControllerTests`, `SettingsControllerTests`, `SeasonalCatalog*Tests`, `SynchronizationStatusControllerTests`, `QmlStructureTests`, `QmlRecognitionStructureTests` |
| Configuração/plataforma | `JsonSettingsReaderTests`, `UserPreferencesValidatorTests`, `TranslationLoaderTests`, `Windows*Tests` |

---

## 10. Receitas

**Nova tela**
1. Controller em `src/presentation/<area>/` (propriedades + `Q_INVOKABLE`), testado em `<Nome>ControllerTests`.
2. QML em `resources/qml/`, registrado em `qt_add_qml_module`.
3. Instanciar/conectar em `main.cpp`, expor como context property e adicionar a navegação em `Main.qml`.

**Novo plugin de player**
1. Implementar `IPlaybackPlugin` em `src/infrastructure/recognition/<player>/` (declare capacidades e
   `PluginSettingsSchema`; reporte eventos ao `IPlaybackObserver`, nunca decida progresso).
2. Registrar em `createRecognitionContext` e aplicar as settings em `applyRecognitionSettings`.
3. Testes com transporte/HTTP falso e fixtures.

**Nova chamada AniList**
1. Arquivo `.graphql` em `resources/anilist/…` + recurso no CMake.
2. Parser/mapper em `infrastructure/anilist`, consumido via interface de `application/anilist`.
3. Testes com fixture JSON gravada.

**Nova tabela/coluna** → ver §7.1 e §7.2.
