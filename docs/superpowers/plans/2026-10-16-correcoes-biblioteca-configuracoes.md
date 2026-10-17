# Correções de biblioteca, configurações e sincronização — Plano de implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Entregar as 10 correções aprovadas (handlers QML, janela maximizada, filtro WATCHING, `?` para total desconhecido, reconciliação de mídias removidas da AniList, estado de sincronização isolado com progresso real, conta AniList conectável/removível com fila por dono, botões no padrão visual e remoção de mídia propagada para a AniList) em fatias pequenas e validadas.

**Architecture:** Mantém a separação existente — QML só apresenta; controllers em `src/presentation`; regras em `src/application`; SQLite/GraphQL em `src/infrastructure` com SQL/GraphQL externos em `resources/`. O estado de sincronização ganha um dono único (`SynchronizationStatusController`) alimentado por um contrato `SyncProgress` que atravessa serviço → executor → coordinator → runtime. Remoções usam colunas próprias (`source_removed_at`, `personal_list_removed_at`) e a fila de pendências ganha `owner_user_id`.

**Tech Stack:** C++20, Qt 6.8.3 (Quick/QML, Network, Sql), SQLite, CMake/Ninja (MSVC x64), QtTest/CTest.

**Spec:** `docs/superpowers/specs/2026-10-16-correcoes-biblioteca-configuracoes-design.md`

## Global Constraints

- Implementar as tarefas na ordem; parar ao fim de cada tarefa para revisão/aceite antes da próxima.
- Build de validação: `cmake -S . -B cmake-build-validation -G Ninja -DBUILD_TESTING=ON` (MSVC x64 + Qt 6.8.3 MSVC; o gerador Visual Studio falha por `PATH`/`Path` duplicados — use Ninja).
- Toda tarefa termina com: testes focados, `cmake --build cmake-build-validation`, `ctest --test-dir cmake-build-validation --output-on-failure` completo e, se houver UI, inspeção em runtime. O relatório da tarefa distingue cada evidência.
- QML não define regra de negócio, filtros, defaults de persistência nem formatação de progresso.
- SQL e GraphQL ficam em arquivos externos registrados em `CMakeLists.txt` (alias de recurso) e `resources/sqlite/sqlite-queries.json` quando aplicável.
- Migrations seguem o padrão existente: arquivo de alteração + arquivo `NNN-mark-...-applied.sql`, registrados em `SqliteDatabase` e `CMakeLists.txt`; preservam mídias, pendências, cache e arquivos locais.
- `source_removed_at` = removida pela fonte; `personal_list_removed_at` = removida pelo usuário; `user_list_status` = só o estado da lista. Nunca usar `user_list_status = 99`.
- Nunca registrar token, callback OAuth, payload GraphQL ou tenant em log.
- Textos de UI via `qsTr`/`tr` e atualizados em `translations/HaikenAnime_pt_BR.ts` e `HaikenAnime_en.ts`.
- Mensagens de commit sem linhas de atribuição.

## Review Focus

- Sincronização cancelada no meio do MANGA (ex.: remoção de conta ou encerramento do app): nenhuma mídia pode ser marcada como removida — teste na Task 6.
- Mídia adicionada pelo catálogo sazonal e ainda não enviada (pendência `Pending`/`Failed`) ausente do snapshot: não pode sumir da biblioteca — teste na Task 6.
- Troca de conta com edições feitas enquanto desconectado: devem pertencer à última conta, nunca à nova — teste na Task 10.
- Remover mídia sem conexão/AniList fora do ar: a mídia continua oculta e a deleção é reenviada depois, sem a sync ressuscitá-la — teste na Task 13.
- Sincronizações sobrepostas (UserList termina e PendingChanges começa) fazendo a barra piscar ou sumir cedo: o timer de 4 s só conta depois que nenhuma partição está em execução — teste na Task 8.

---

### Task 1: Handlers QML com parâmetros declarados

**Files:**
- Modify: `resources/qml/SettingsScreen.qml` (componente `SettingSwitch` ~l.413-428; usos ~l.533-536, 610-617, 662, 695-701, 706-712)
- Modify: `tests/unit/QmlStructureTests.cpp`

**Interfaces:**
- Produces: nenhum contrato novo; `SettingSwitch.toggled(bool checked)` mantém a assinatura.

- [ ] **Step 1: Teste estrutural `implicitSignalParametersAreDeclared`** em `QmlStructureTests`: para cada arquivo em `resources/qml/*.qml`, reprova linhas que casem `on(Toggled|Activated|Accepted)\s*:\s*[^f{(]*\b(checked|index)\b` sem `function(` ou `=>` no handler. Assert: lista de violações vazia.
- [ ] **Step 2: Rodar** `cmake --build cmake-build-validation --target QmlStructureTests && ctest --test-dir cmake-build-validation -R QmlStructureTests --output-on-failure`. Esperado: FAIL citando `SettingsScreen.qml`.
- [ ] **Step 3: Converter os handlers** que usam `checked`/`index` para a forma `function`: sinais com parâmetro (`SettingSwitch.toggled(bool checked)`, `ComboBox.activated(int index)`) usam `function(checked) { ... }`/`function(index) { ... }`; sinais sem parâmetro (`Switch`/`CheckBox.toggled()`) usam `function() { ... }` lendo a propriedade pelo `id` do próprio controle (ex.: `onToggled: function() { settingSwitch.toggled(innerSwitch.checked) }`).
- [ ] **Step 4: Rodar o teste** (PASS) e abrir o app, alternar cada switch/checkbox/combo das Configurações e confirmar ausência de `Parameter "checked" is not declared` no console.
- [ ] **Step 5: Build completo + CTest e commit** `fix: declare QML signal handler parameters`.

### Task 2: Janela principal maximizada

**Files:**
- Modify: `resources/qml/Main.qml`
- Modify: `tests/unit/QmlStructureTests.cpp`

- [ ] **Step 1: Teste `mainWindowStartsMaximized`**: `Main.qml` contém `visibility: Window.Maximized` e importa `QtQuick.Window` (ou usa `ApplicationWindow` com o enum qualificado).
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Adicionar `visibility: Window.Maximized`** ao `ApplicationWindow`; manter `width: 1440`/`height: 900` como tamanho de restauração.
- [ ] **Step 4: Rodar teste (PASS) e validar em runtime**: o app abre maximizado; "restaurar" volta a 1440×900 com o botão de maximizar alcançável.
- [ ] **Step 5: Build + CTest e commit** `fix: start main window maximized`.

### Task 3: Filtro inicial WATCHING

**Files:**
- Modify: `src/presentation/home/HomeScreenController.h` (l.222), `src/presentation/home/HomeScreenController.cpp` (`browseCriteriaActive` ~l.363, `ConfigureBrowseOptions` ~l.490-511, `ClearBrowseCriteria` ~l.579)
- Test: `tests/unit/HomeScreenControllerTests.cpp`

**Interfaces:**
- Produces: `inline constexpr auto DefaultListFilterKey = "current";` (no `.cpp`, namespace anônimo) e `QString HomeScreenController::defaultListFilter() const` (privado) que devolve `current` se existir nas opções, senão a primeira opção.

- [ ] **Step 1: Testes**: `Should_StartWithCurrentFilter_When_Created` (`activeListFilter() == "current"`, `browseCriteriaActive() == false`); `Should_FallbackToFirstOption_When_CurrentIsNotConfigured`; `Should_ResetToCurrent_When_BrowseCriteriaCleared` (após `SetListFilter("planning")` + `ClearBrowseCriteria()`); `Should_ShowOnlyCurrentMedia_When_Started` (modelo com 1 Current + 1 Completed → `filteredMediaCount() == 1`).
- [ ] **Step 2: Rodar** `HomeScreenControllerTests` e ver FAIL.
- [ ] **Step 3: Implementar**: `activeListFilter_` inicia em `DefaultListFilterKey`; `ConfigureBrowseOptions`, `ClearBrowseCriteria` e `browseCriteriaActive` usam `defaultListFilter()` no lugar de `firstOptionKey(listOptions_)`. Ajustar testes existentes que assumiam `"all"` inicial.
- [ ] **Step 4: Rodar** (PASS) e verificar no app que a biblioteca abre em WATCHING e a troca para "Todos" funciona.
- [ ] **Step 5: Build + CTest e commit** `fix: open library on watching list`.

### Task 4: Total desconhecido como `?`

**Files:**
- Create: `src/application/media/ProgressFormatter.h`, `src/application/media/ProgressFormatter.cpp`
- Modify: `src/presentation/home/HomeScreenController.cpp` (`mediaProgress` l.56, `cardProgress` l.60, `PreviewCardMetadata` l.686, `PreviewCompactDetailMetadata` l.705), `src/presentation/seasonal/SeasonalCatalogController.cpp` (l.173), `CMakeLists.txt`
- Test: `tests/unit/MediaDomainTests.cpp`, `tests/unit/HomeScreenControllerTests.cpp`, `tests/unit/SeasonalCatalogControllerTests.cpp`

**Interfaces:**
- Produces: `QString FormatProgress(int consumed, int total);` → `"3/12"` ou `"3/?"` quando `total <= 0`; `consumed < 0` vira `0`.

- [ ] **Step 1: Testes**: `FormatProgress(3, 12) == "3/12"`, `FormatProgress(3, 0) == "3/?"`, `FormatProgress(0, -1) == "0/?"`, `FormatProgress(-2, 5) == "0/5"`; no controller, mídia com `TotalChapters = 0` e `ConsumedChapters = 3` → `selectedProgress() == "3/?"`, card `"Progresso 3/?"`, prévia compacta `"3/?"`; no sazonal, label de progresso com `?`.
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Implementar `FormatProgress`** e trocar as quatro formatações do Home e a do Sazonal; `cardProgress` deixa de usar `"Progresso —"` e passa a `"Progresso %1".arg(FormatProgress(...))` (atualizar `.ts`). Persistência e `AniListMediaMapper` (`value_or(0)`) não mudam.
- [ ] **Step 4: Rodar** (PASS) e conferir no app um card sem total.
- [ ] **Step 5: Build + CTest e commit** `fix: show unknown media totals as question mark`.

### Task 5: IDs observados e snapshot completo no parser/data source

**Files:**
- Modify: `src/infrastructure/anilist/AniListGraphQlUserListParser.h/.cpp`, `src/application/anilist/IAniListDataSource.h` (`AniListDataSourceResult`), `src/infrastructure/anilist/GraphQlAniListDataSource.cpp` (l.100-108), `src/infrastructure/anilist/RecordedGraphQlAniListDataSource.h` (l.95-138), `src/application/anilist/AniListSyncService.h/.cpp`
- Test: `tests/unit/AniListGraphQlUserListParserTests.cpp`, `tests/unit/AniListFlowTests.cpp`

**Interfaces:**
- Produces:
  - `AniListGraphQlUserListParser::parse(const QJsonObject &data, QList<Media> &media, QString &error, const QStringList &acceptedListStatuses = {}, const QList<MediaType> &acceptedMediaTypes = {}, QSet<int> *observedMediaIds = nullptr)` — preenche `observedMediaIds` com todo `media.id > 0` de toda entrada, antes de qualquer filtro.
  - `AniListDataSourceResult::observedMediaIds` (`QSet<int>`).
  - Regra de `isCompleteAuthoritativeSnapshot` (GraphQL e Recorded): `partition == UserList && startingPage == 1 && status.isEmpty() && list.isEmpty()` — `type` e `acceptedListStatuses` deixam de invalidar.
  - `struct AniListSyncOutcome { QSet<int> observedMediaIds; bool completeSnapshot = false; };` e `const AniListSyncOutcome &AniListSyncService::lastOutcome() const` — `completeSnapshot` só é `true` se o primeiro chunk foi completo e o laço terminou com `hasNextPage == false` sem erro.
  - `AniListSyncService` passa a usar `observedMediaIds` (não só os IDs persistidos) quando reconcilia internamente (fluxo de fixture/`InitialSyncCoordinator`).

- [ ] **Step 1: Testes do parser**: com `acceptedListStatuses = {"CURRENT"}`, uma entrada `DROPPED` não entra em `media` mas entra em `observedMediaIds`; entrada sem `media` objeto mas com `mediaId` não é contada; IDs duplicados aparecem uma vez.
- [ ] **Step 2: Testes de fluxo** (`AniListFlowTests`): request com `type = "ANIME"` e `acceptedListStatuses` preenchidos em 2 chunks → `lastOutcome().completeSnapshot == true` e `observedMediaIds` contém as entradas filtradas; falha no 2º chunk → `completeSnapshot == false`; `status` preenchido → `false`.
- [ ] **Step 3: Rodar** e ver FAIL.
- [ ] **Step 4: Implementar** conforme Interfaces (acumular `observedMediaIds` de cada página no serviço; zerar `lastOutcome_` no início de `synchronize`).
- [ ] **Step 5: Rodar** (PASS), build + CTest e commit `fix: track observed anilist entries for snapshot completeness`.

### Task 6: Reconciliação no executor, com exclusão de pendências

**Files:**
- Modify: `src/application/media/IMediaSnapshotReconciler.h`, `src/infrastructure/database/SqliteMediaRepository.h/.cpp` (l.173-229), `src/application/anilist/IPendingChangeRepository.h`, `src/infrastructure/database/SqlitePendingChangeRepository.h/.cpp`, `src/app/ApplicationSyncTaskExecutor.cpp` (l.120-170), `src/application/anilist/AniListSyncService.cpp` (l.146-154), `CMakeLists.txt`, `resources/sqlite/sqlite-queries.json`
- Create: `resources/sqlite/queries/read-unsent-media-ids.sql`
- Test: `tests/unit/SqliteRepositoryTests.cpp`, `tests/unit/AniListFlowTests.cpp`, `tests/unit/InitialSyncCoordinatorTests.cpp`

**Interfaces:**
- Consumes: `AniListSyncService::lastOutcome()` (Task 5).
- Produces:
  - `virtual bool reconcileAuthoritativeSnapshot(const QSet<int> &observedMediaIds, const QSet<int> &excludedMediaIds, int &removedCount, QString &error) = 0;`
  - `virtual bool getMediaIdsWithUnsentChanges(QSet<int> &mediaIds, QString &error) = 0;` em `IPendingChangeRepository` — SQL: `SELECT DISTINCT media_id FROM anilist_pending_changes WHERE status IN (0, 1, 3, 4)` (Pending, Processing, Failed, RequiresConfirmation).
  - No executor: o serviço é construído com `snapshotReconciler = nullptr`; após o laço ANIME/MANGA, **somente** se ambos tiverem `completeSnapshot == true`, `result.succeeded` e não cancelado, une os `observedMediaIds`, lê as exclusões e chama o reconciliador. Log: `"AniList snapshot reconciliation marked %1 media as removed from source."`.

- [ ] **Step 1: Testes de repositório**: mídia ausente e não excluída recebe `source_removed_at`; mídia ausente mas em `excludedMediaIds` não recebe; `read-media` não a retorna; upsert posterior zera `source_removed_at`.
- [ ] **Step 2: Testes de executor/fluxo** (fakes de data source por tipo): ANIME e MANGA completos → ausente marcada; MANGA falha → nada marcado; cancelamento entre ANIME e MANGA → nada marcado; lista `dropped` desabilitada e mídia `DROPPED` no response → não marcada; mídia com pendência `Failed` ausente → não marcada.
- [ ] **Step 3: Rodar** e ver FAIL.
- [ ] **Step 4: Implementar** a nova assinatura (ajustar `RecordingSnapshotReconciler` em `AniListFlowTests` e chamadas do serviço, que passam `{}` como exclusões), a query e a orquestração no executor.
- [ ] **Step 5: Rodar** (PASS), build + CTest e commit `fix: reconcile media removed from anilist after full sync`.

### Task 7: Contrato de progresso da sincronização

**Files:**
- Create: `src/application/scheduling/SyncProgress.h`
- Modify: `src/application/scheduling/ISyncTaskExecutor.h`, `src/application/anilist/AniListSyncService.h/.cpp`, `src/application/anilist/AniListPendingChangeDrain.h/.cpp`, `src/app/ApplicationSyncTaskExecutor.h/.cpp`, `src/app/AdaptiveSyncCoordinator.h/.cpp`, `src/app/AdaptiveSyncRuntime.h/.cpp`, `CMakeLists.txt`
- Test: `tests/unit/AniListFlowTests.cpp`, `tests/unit/AdaptiveSyncCoordinatorTests.cpp`, `tests/unit/AdaptiveSyncRuntimeTests.cpp`

**Interfaces:**
- Produces:
  - `struct SyncProgress final { SyncPartition partition = SyncPartition::UserList; QString stageKey; int stageIndex = 0; int stageCount = 0; int processedItems = 0; int totalItems = -1; };` + `Q_DECLARE_METATYPE(SyncProgress)`. Chaves de etapa: `"anime"`, `"manga"`, `"reconciliation"` (UserList, `stageCount = 3`); `"pending_changes"` (PendingChanges, `stageCount = 1`, `totalItems` = quantidade de mídias pendentes).
  - `using ProgressReporter = std::function<void(const SyncProgress &)>;` e `virtual void SetProgressReporter(ProgressReporter reporter) {}` (não puro) em `ISyncTaskExecutor`.
  - `void AniListSyncService::setProgressReporter(std::function<void(int processedItems)> reporter)` — chamado após cada página persistida com o total acumulado de mídias da execução.
  - `void AniListPendingChangeDrain::setProgressReporter(std::function<void(int processed, int total)> reporter)`.
  - `AdaptiveSyncCoordinator` registra o reporter no executor no construtor e reemite `void TaskProgress(SyncProgress progress)` na própria thread via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.
  - `AdaptiveSyncRuntime` reemite `void BackgroundTaskProgress(SyncProgress progress)` e emite também `void BackgroundTaskStarted(SyncPartition partition)` a partir de `TaskStarted`.

- [ ] **Step 1: Testes**: serviço com 2 páginas de 3 e 2 mídias reporta `3` e depois `5`; coordinator com executor fake que reporta 2 progressos emite `TaskProgress` 2× na ordem, com partição correta; runtime encaminha `BackgroundTaskStarted`/`BackgroundTaskProgress`.
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Implementar** os contratos; no executor, cada tipo/etapa monta o `SyncProgress` (anime=1, manga=2, reconciliation=3 com `totalItems = -1` nas etapas de lista).
- [ ] **Step 4: Rodar** (PASS) com `--repeat until-fail:10` nos testes de coordinator/runtime.
- [ ] **Step 5: Build + CTest e commit** `feat: publish synchronization progress`.

### Task 8: `SynchronizationStatusController` e isolamento de erros

**Files:**
- Create: `src/presentation/sync/SynchronizationStatusController.h/.cpp`, `tests/unit/SynchronizationStatusControllerTests.cpp`
- Modify: `src/presentation/home/HomeScreenController.h/.cpp` (remover `notifySynchronization*`, `synchronizationProgress*`), `src/presentation/settings/SettingsController.h/.cpp` (remover `notifySynchronization*`; `synchronizationRunning` passa a ser lido do novo controller no QML), `main.cpp` (l.153-196, 224-236), `resources/qml/Home.qml` (l.506-622), `resources/qml/SettingsScreen.qml` (botão "Sincronizar agora"), `CMakeLists.txt`
- Test: `tests/unit/HomeScreenControllerTests.cpp`, `tests/unit/SettingsControllerTests.cpp`

**Interfaces:**
- Consumes: `SyncProgress`, `AdaptiveSyncRuntime::BackgroundTaskStarted/Progress/Completed/Failed` (Task 7).
- Produces (`SynchronizationStatusController : QObject`, exposto ao QML como `synchronizationStatus`):
  - Propriedades (`NOTIFY changed`): `bool running`, `QString outcome` (`"idle" | "running" | "succeeded" | "failed"`), `QString stageKey`, `QString stageLabel`, `int stageIndex`, `int stageCount`, `int processedItems`, `int totalItems`, `bool progressKnown` (`totalItems > 0`), `int percent`, `QString message`, `bool visible`.
  - Construtor `explicit SynchronizationStatusController(int hideDelayMs = 4000, QObject *parent = nullptr);`
  - Slots: `void HandleStarted(SyncPartition)`, `void HandleProgress(const SyncProgress &)`, `void HandleCompleted(SyncPartition)`, `void HandleFailed(SyncPartition, const QString &safeError)`.
  - Regra: conta partições ativas; `visible = true` enquanto houver alguma; ao zerar, `outcome` = `failed` se alguma falhou na rodada, senão `succeeded`; inicia `QTimer` single-shot de `hideDelayMs`; ao disparar, `visible = false` e `outcome = "idle"`. Novo início cancela o timer.
  - `main.cpp`: `BackgroundTaskCompleted(UserList)` continua chamando `homeController.reload()`; nenhuma falha de sync chama mais Home/Settings. `InitializationFailed` e o erro de agendamento vão para `HandleFailed`.

- [ ] **Step 1: Testes do controller** (`hideDelayMs = 20`, `QTRY_COMPARE`): started → `visible && running`; progresso → `stageLabel`/`processedItems`; completed → `outcome == "succeeded"`, ainda visível, depois `visible == false`; failed → `outcome == "failed"` e `message` com o erro; UserList conclui enquanto PendingChanges ainda roda → continua visível e `running`; novo started durante o timer cancela a ocultação.
- [ ] **Step 2: Testes de isolamento**: `HomeScreenController` não expõe mais estado de sync (remover/ajustar testes antigos de `notifySynchronizationFailed`); `state()` só vira `"error"` por falha de `readAll`; `SettingsController::errorMessage()` não muda por falha de sync.
- [ ] **Step 3: Rodar** e ver FAIL.
- [ ] **Step 4: Implementar** o controller, religar `main.cpp`, registrar a context property `synchronizationStatus`, e trocar no `Home.qml`/`SettingsScreen.qml` as leituras de `controller.state === "loading"`/`synchronizationProgress*`/`synchronizationRunning` por `synchronizationStatus.*` (a barra visual definitiva vem na Task 9; aqui manter a `ProgressBar` existente ligada a `synchronizationStatus.visible`/`percent`).
- [ ] **Step 5: Rodar** (PASS); em runtime, forçar falha de sync (rede desligada) e confirmar que Home e Configurações não mostram o erro fora da área de sincronização.
- [ ] **Step 6: Build + CTest e commit** `fix: isolate synchronization state from screens`.

### Task 9: `AppButton`, `SyncProgressBar` e substituição dos botões nativos

> Executar com a skill `frontend-design`, respeitando a paleta existente (`ink #172033`, `muted #68758a`, `line #d7dee8`, `surface #ffffff`, `surfaceSoft #f6f8fb`, `accent #315d91`, `danger #b42318`).

**Files:**
- Create: `resources/qml/AppButton.qml`, `resources/qml/SyncProgressBar.qml`
- Modify: `resources/qml/SettingsScreen.qml` (remover `component SettingsButton`, l.430-450), `Home.qml`, `EditMediaPanel.qml`, `MediaDetailsPanel.qml`, `SeasonalCatalogScreen.qml`, `CoverPreview.qml`, `CMakeLists.txt` (`QML_FILES`)
- Test: `tests/unit/QmlStructureTests.cpp`

**Interfaces:**
- Produces:
  - `AppButton` (`Basic.Button`): `property string variant: "primary"` (`primary | secondary | flat | danger`); estados hover/down/activeFocus/disabled conforme o `SettingsButton` atual (primary: `accent`, hover `#3c6eaa`, down `#24466d`); `secondary`: fundo `surface` borda `line` texto `ink`; `flat`: sem borda, texto `accent`; `danger`: fundo `danger`. `Accessible.name` = `text` por padrão. Altura implícita 34.
  - `SyncProgressBar` (`Item`): `required property var status` (o `synchronizationStatus`); altura 3 px, trilho `line`, preenchimento `#02A9FF` (`danger` quando `status.outcome === "failed"`), raio 1.5, animação indeterminada quando `!status.progressKnown && status.running`; rótulo curto à direita: `"%1 · %2 itens".arg(status.stageLabel).arg(status.processedItems)` ou `status.message` ao final; `visible: status.visible`.

- [ ] **Step 1: Testes estruturais**: `nativeButtonsAreNotUsed` — nenhum `^\s*(Basic\.)?Button\s*\{` nos 6 arquivos; `appButtonVariantsExist` — `AppButton.qml` declara `variant` e trata os 4 valores; `syncProgressBarUsesAniListColor` — `SyncProgressBar.qml` contém `#02A9FF` e liga `visible` a `status.visible`.
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Criar os componentes** e substituir os 24 botões (ações principais → `primary`; Fechar/Cancelar/navegação → `flat`/`secondary`; destrutivos → `danger`); trocar a `ProgressBar` do rodapé da Home e adicionar `SyncProgressBar` no card "Conta e sincronização".
- [ ] **Step 4: Rodar** (PASS) e inspecionar em runtime: hover, pressionado, foco por Tab, desabilitado e leitor de tela (nome acessível) em Configurações, Home, painéis e Sazonal; barra durante sync, ao concluir e ao falhar (some após ~4 s).
- [ ] **Step 5: Build + CTest e commit** `feat: apply shared button and progress bar styles`.

### Task 10: Fila de pendências por conta

**Files:**
- Create: `resources/sqlite/migrations/021-add-owner-user-id-to-pending-changes.sql`, `021-mark-pending-change-owner-applied.sql`, `022-add-last-anilist-user-id-preference.sql`, `022-mark-last-anilist-user-id-applied.sql`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`, `src/application/anilist/AniListPendingChange.h` (`qint64 ownerUserId = 0;`), `src/infrastructure/database/SqlitePendingChangeRepository.h/.cpp`, `resources/sqlite/queries/enqueue-pending-change.sql`, `read-pending-changes.sql`, `read-pending-media-ids.sql`, `src/application/configuration/UserPreferences.h` (`qint64 lastAniListUserId = 0;`), `src/infrastructure/database/SqliteUserPreferencesRepository.cpp`, `resources/sqlite/queries/read-user-preferences.sql`, `upsert-user-preferences.sql`, `src/infrastructure/database/SqlitePersonalListChangeWriter.cpp`, `src/application/media/PersonalListChangeService.h/.cpp`, `src/app/ApplicationSyncTaskExecutor.cpp`, `main.cpp`, `CMakeLists.txt`
- Test: `tests/unit/SqliteDatabaseTests.cpp`, `tests/unit/SqliteUserPreferencesRepositoryTests.cpp`, `tests/unit/AniListPendingChangeProcessorTests.cpp`, `tests/unit/PersonalListChangeServiceTests.cpp`

**Interfaces:**
- Produces:
  - Coluna `anilist_pending_changes.owner_user_id INTEGER NOT NULL DEFAULT 0`; coluna `user_preferences.last_anilist_user_id INTEGER NOT NULL DEFAULT 0`.
  - `SqlitePendingChangeRepository::setOwnerUserId(qint64 ownerUserId)` — leituras filtram `owner_user_id = :owner OR owner_user_id = 0`; `bool adoptOrphanChanges(qint64 ownerUserId, int &adoptedCount, QString &error)` faz `UPDATE ... SET owner_user_id = :owner WHERE owner_user_id = 0`.
  - `PersonalListChangeService::setOwnerProvider(std::function<qint64()> provider)` — devolve `credentials().userId` se conectado, senão `preferences.lastAniListUserId`; cada `AniListPendingChange` criado recebe esse dono.
  - Executor (PendingChanges e UserList): antes de processar, `setOwnerUserId(credentials.userId)` e `adoptOrphanChanges`, logando `"Adopted %1 pending AniList changes without owner."` quando `> 0`.
  - `main.cpp`: após `aniListAuthenticationSucceeded`, persiste `lastAniListUserId = credentials.userId`.

- [ ] **Step 1: Testes**: migrations 021/022 preservam linhas existentes com dono `0`/preferência `0`; enqueue grava o dono; com dono atual 2, pendência do dono 1 não é lida nem enviada (processor com update client fake não recebe chamada); órfã é adotada e passa a ter dono 2; serviço sem conta usa `lastAniListUserId`.
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Implementar** migrations, queries, repositórios, provider e ligações.
- [ ] **Step 4: Rodar** (PASS), build + CTest e commit `feat: scope pending anilist changes to account owner`.

### Task 11: Conectar desabilitado e remoção de conta

**Files:**
- Create: `src/application/anilist/RemoveAniListAccountUseCase.h/.cpp`, `tests/unit/RemoveAniListAccountUseCaseTests.cpp`
- Modify: `src/app/AdaptiveSyncCoordinator.h/.cpp`, `src/app/AdaptiveSyncRuntime.h/.cpp`, `src/presentation/settings/SettingsController.h/.cpp`, `main.cpp`, `resources/qml/SettingsScreen.qml` (card "Serviço conectado", l.501-523), `CMakeLists.txt`
- Test: `tests/unit/SettingsControllerTests.cpp`, `tests/unit/AdaptiveSyncCoordinatorTests.cpp`

**Interfaces:**
- Consumes: `AniListAuthManager::clear(QString &)`, `AppButton` variante `danger` (Task 9), `UserPreferences::lastAniListUserId` (Task 10).
- Produces:
  - `void AdaptiveSyncCoordinator::Cancel(SyncPartition partition, std::function<void()> acknowledged);` e `void AdaptiveSyncRuntime::cancel(SyncPartition partition, std::function<void()> acknowledged);` (executa na thread do worker; `acknowledged` chamado na thread da UI via `QMetaObject::invokeMethod`). Partição sem tarefa ativa confirma imediatamente.
  - `class RemoveAniListAccountUseCase` com dependências injetadas: `std::function<void()> stopOAuthListener`, `std::function<void(SyncPartition, std::function<void()>)> cancelPartition`, `AniListAuthManager &`, `std::function<bool(qint64 userId, QString &error)> rememberLastAccount`, `AuditLogger`. Método `bool Start(std::function<void(bool succeeded, QString error)> completion);` — devolve `false` se já houver remoção em andamento; ordem: parar listener → cancelar UserList e PendingChanges (aguarda os dois acks) → `clear()` → `rememberLastAccount(userIdRemovido)` → `completion`. Logs: `"AniList account removal started."`, `"... completed."`, `"... failed."` sem detalhes de credencial.
  - `SettingsController`: `Q_PROPERTY(bool aniListConnected ...)`, `Q_PROPERTY(bool aniListRemovalInProgress ...)`, `Q_INVOKABLE void RemoveAniListAccount();`, `void SetAniListAccountRemoval(RemoveAniListAccountUseCase *useCase);` — estado `"removing"` durante a remoção; ao concluir: limpa `aniListUsername_`, estado `"disconnected"`, mensagem própria em `aniListAuthenticationMessage` (não em `errorMessage`).
  - QML: "Conectar conta" `enabled: controller.aniListConnectionAvailable && !controller.aniListConnected && !controller.aniListAuthenticationInProgress && !controller.aniListRemovalInProgress`; "Remover conta" (`danger`, visível se `aniListConnected`) abre `Dialog` de confirmação ("A biblioteca local e as alterações não sincronizadas serão mantidas.").

- [ ] **Step 1: Testes do caso de uso** (fakes registrando a ordem): ordem exata `stopListener, cancel(UserList), cancel(PendingChanges), clear, remember`; `clear` só ocorre após os dois acks (ack atrasado manualmente); segunda chamada durante a execução devolve `false`; falha de `clear` → `completion(false, ...)` e nada é lembrado; biblioteca (fake repository de mídia) não recebe chamada.
- [ ] **Step 2: Testes do controller e coordinator**: conectado → `aniListConnected == true` e `ConnectAniList()` não chama o launcher; `RemoveAniListAccount()` coloca `aniListRemovalInProgress` e, ao concluir, `aniListUsername() == ""`; `Cancel` de partição ativa confirma após o executor confirmar.
- [ ] **Step 3: Rodar** e ver FAIL.
- [ ] **Step 4: Implementar** conforme Interfaces e ligar em `main.cpp`.
- [ ] **Step 5: Rodar** (PASS); em runtime: conectar, iniciar sync, remover conta, confirmar que a sync é cancelada, o usuário some, a biblioteca continua e o log não contém token.
- [ ] **Step 6: Build + CTest e commit** `feat: allow removing the anilist account`.

### Task 12: Envio da deleção para a AniList

**Files:**
- Create: `resources/anilist/queries/media-list-entry-id.graphql`
- Modify: `resources/anilist/mutations/delete-list-entry.graphql`, `src/infrastructure/anilist/AniListUpdateClient.h/.cpp` (l.64-65), `src/application/anilist/IAniListUpdateClient.h`, `src/application/anilist/AniListPendingChangeProcessor.cpp` (l.30-39), `src/app/ApplicationSyncTaskExecutor.cpp` (l.199-203), `CMakeLists.txt`
- Test: `tests/unit/AniListUpdateClientTests.cpp`, `tests/unit/AniListPendingChangeProcessorTests.cpp`

**Interfaces:**
- Produces:
  - `virtual bool deleteMediaListEntry(int mediaId, qint64 userId, QString &error) = 0;` em `IAniListUpdateClient`.
  - Query `query MediaListEntryId($userId: Int!, $mediaId: Int!) { MediaList(userId: $userId, mediaId: $mediaId) { id } }`; mutation `mutation DeleteListEntry($id: Int!) { DeleteMediaListEntry(id: $id) { deleted } }`.
  - `AniListUpdateClient::deleteMediaListEntry`: resolve o `id`; erro GraphQL "Not Found"/`MediaList: null` → sucesso (já removida); `deleted == false` → erro.
  - Processor: `Deletion` com status `Pending`/`Failed` é enviado via `deleteMediaListEntry` (substitui qualquer outra pendência da mesma mídia, marcando-as `Superseded`); `RequiresConfirmation` continua sem envio.

- [ ] **Step 0 (DoR):** confirmar no schema oficial (https://docs.anilist.co/ → `Mutation.DeleteMediaListEntry` e `Query.MediaList`) os argumentos `id` e `userId/mediaId`; se divergirem, parar e reportar antes de codar.
- [ ] **Step 1: Testes** (cliente com transporte fake): sequência query → mutation com o `id` resolvido; `MediaList` nulo → `true` sem mutation; `deleted: false` → `false` com erro; processor envia `Deletion Pending`, marca `Succeeded`, e marca `Superseded` as outras pendências da mídia; falha incrementa `attempts` e mantém `Failed`.
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Implementar** conforme Interfaces; o executor passa `credentials.userId`.
- [ ] **Step 4: Rodar** (PASS), build + CTest e commit `feat: send confirmed list deletions to anilist`.

### Task 13: Remover mídia da lista pelo painel

**Files:**
- Create: `resources/sqlite/migrations/023-add-personal-list-removed-at-to-media.sql`, `023-mark-personal-list-removed-at-applied.sql`, `resources/sqlite/queries/mark-media-personal-list-removed.sql`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`, `resources/sqlite/queries/read-media.sql`, `update-personal-list-media.sql`, `read-catalog-media-for-recognition.sql`, `src/application/media/IPersonalListChangeWriter.h`, `src/infrastructure/database/SqlitePersonalListChangeWriter.h/.cpp`, `src/application/media/PersonalListChangeService.h/.cpp`, `src/presentation/home/HomeScreenController.h/.cpp`, `resources/qml/EditMediaPanel.qml`, `resources/qml/Home.qml`, `resources/qml/MediaDetailsPanel.qml`, `CMakeLists.txt`, `resources/sqlite/sqlite-queries.json`
- Test: `tests/unit/SqliteDatabaseTests.cpp`, `tests/unit/SqliteRepositoryTests.cpp`, `tests/unit/PersonalListChangeServiceTests.cpp`, `tests/unit/HomeScreenControllerTests.cpp`

**Interfaces:**
- Consumes: `AniListField::Deletion` enviado na Task 12; dono da Task 10.
- Produces:
  - Coluna `media.personal_list_removed_at TEXT` (nula por padrão). `read-media.sql` e `read-catalog-media-for-recognition.sql` passam a exigir `personal_list_removed_at IS NULL`; `upsert-media.sql` **não** toca a coluna; `update-personal-list-media.sql` a zera.
  - `virtual bool remove(const Media &media, const AniListPendingChange &deletion, QString &error) = 0;` em `IPersonalListChangeWriter` — transação: marca `personal_list_removed_at = now UTC ISO` e enfileira a deleção.
  - `bool PersonalListChangeService::remove(const Media &media, QString &error) const;` — cria `Deletion` via `AniListPendingChangeFactory` e força `status = Pending` (a confirmação foi feita na UI); dispara o notifier de pendências.
  - `HomeScreenController`: `Q_PROPERTY(QString mediaOperationErrorMessage READ ... NOTIFY selectionChanged)`, `Q_INVOKABLE bool RemoveSelectedMedia();` — remove de `allMedia_`, reconstrói modelos, limpa seleção, emite `mediaCountChanged`; falha preenche só `mediaOperationErrorMessage`. `SaveSelectedMediaFromEditor` também passa a usar `mediaOperationErrorMessage` no lugar de `errorMessage_`.
  - QML: `EditMediaPanel` ganha `AppButton { variant: "danger"; text: qsTr("Remover da lista") }` que abre `Dialog` de confirmação com o título; ao confirmar emite `signal removeRequested(int mediaId)`; `Home.qml` chama `controller.RemoveSelectedMedia()` e, em sucesso, fecha o editor e o `MediaDetailsPanel`; o erro aparece apenas no painel.

- [ ] **Step 1: Testes**: migration 023 preserva mídias; `remove` grava a coluna e uma pendência `Deletion Pending` na mesma transação (falha no enqueue → rollback, coluna nula); `read-media` não retorna a mídia removida; upsert da sync da mesma mídia **não** a traz de volta; `update-personal-list-media` (re-adição pelo sazonal) a traz de volta; controller: `RemoveSelectedMedia()` → `hasSelection() == false`, `mediaCount()` decrementa, `errorMessage()` inalterado; falha do writer → `mediaOperationErrorMessage` preenchido e mídia mantida.
- [ ] **Step 2: Rodar** e ver FAIL.
- [ ] **Step 3: Implementar** conforme Interfaces.
- [ ] **Step 4: Rodar** (PASS); em runtime: remover uma mídia, ver sumir sem reinício, painel fechar, pendência enviada (log de PendingChanges concluído) e, após nova sync, a mídia não voltar; repetir com rede desligada e confirmar reenvio posterior.
- [ ] **Step 5: Build + CTest e commit** `feat: remove media from personal list`.

### Task 14: Verificação final da branch

- [ ] **Step 1: Configurar do zero e buildar**: `cmake -S . -B cmake-build-validation -G Ninja -DBUILD_TESTING=ON` e `cmake --build cmake-build-validation`; exit code 0.
- [ ] **Step 2: CTest completo**: `ctest --test-dir cmake-build-validation --output-on-failure`; 100% dos testes passando.
- [ ] **Step 3: Roteiro manual** cobrindo os 14 casos mínimos da spec (seção "Validação"), registrando resultado por caso em `.superpowers/sdd/2026-10-16-correcoes-biblioteca-configuracoes/progress.md`.
- [ ] **Step 4: Atualizar documentação**: `README.md` (remoção de conta, remoção de mídia, fila por conta) e traduções `.ts` revisadas; commit `docs: document account and media removal`.
