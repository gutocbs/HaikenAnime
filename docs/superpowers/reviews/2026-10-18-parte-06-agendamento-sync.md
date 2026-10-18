# Report — Parte 6: Agendamento e execução da sync

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `2675f00` (`feature/revisao-geral`)
- **Status:** aprovado (com as duas recomendações) e aplicado — validado (a migração 027 não foi necessária; ver seção 7)
- *(O `InitialSyncCoordinator`, previsto aqui, já saiu na Parte 5.)*

---

## 1. O que esta parte faz

Decide **quando** cada sincronização roda e a executa fora da thread da GUI:

- **`SyncTaskTypes` / `SyncTaskPolicy`** (application): cada partição tem um estado persistido
  (`SyncTaskState`: status, última tentativa/sucesso, próxima execução, falhas, retentativas imediatas,
  checkpoint de página, geração, prioridade, validade do cache) e uma política (`SyncSchedulePolicy`:
  intervalo normal, TTL de dado obsoleto, backoff exponencial com teto, *cooldown* para erro não
  recuperável, *jitter*, retentativas imediatas). `SyncTaskPolicy::Decide` calcula a próxima execução.
- **`AdaptiveSyncCoordinator`** (app, Qt): carrega os estados (`sync_task_state`), cria os que faltam, roda
  **uma tarefa por vez** por prioridade, reexecuta a partição que recebeu um pedido enquanto rodava,
  controla gerações (uma conclusão atrasada de tarefa cancelada é ignorada), segura `PendingChanges` até a
  primeira `UserList` terminar bem e cancela/para com confirmação do executor.
- **`AdaptiveSyncRuntime`**: hospeda coordinator, repositório e executor numa thread própria e expõe
  `requestNow`/`cancel`/`shutdown` e os sinais `BackgroundTask*` para a GUI.
- **`ApplicationSyncTaskExecutor`**: cada tarefa roda num `QThread` próprio, com conexão SQLite e
  `QNetworkAccessManager` próprios. `UserList` baixa anime e depois mangá, concilia pendências e o snapshot;
  `PendingChanges` drena o outbox e alimenta o histórico de reconhecimento. Ambos respeitam cancelamento.
- **`SqliteSyncTaskStateRepository`** lê e grava `sync_task_state`, convertendo enums para texto.

---

## 2. Componentes e achados

| Componente | Linhas | Achado |
| --- | --- | --- |
| `SyncTaskTypes.h` | 161 | `SyncTaskKind` e `SyncPartition` têm **7 valores cada** (1:1); só `UserList` e `PendingChange(s)` existem. Catálogo ativo/inativo/concluído, capas e metadados derivados são planos que não chegaram a existir |
| `SyncTaskPolicy` | 178 + 35 | gatilho `CompletedToActive` e o enum `SyncTaskPromotion` só servem a uma promoção de catálogo que nada aciona; `Evaluate()` só é usado nos testes |
| `AdaptiveSyncCoordinator` | 381 | `NotifyLocalChange()` não é chamado por ninguém; o ramo de `Request` que **troca a partição** (promoção) e o `Remove` do repositório só existem para isso |
| `ISyncTaskExecutor` / `SyncTaskExecutionResult` | — | `retryAfter` e `confirmedCursor` **nunca são preenchidos** pelo executor; o coordinator e a política os tratam assim mesmo |
| `ApplicationSyncTaskExecutor` | 401 + 74 | `Execute` é uma única lambda de ~250 linhas com os dois fluxos; `if (auditLogger) auditLogger(...)` repetido ~15×; `AuthenticatedUserListConfiguration::credentials` é um *fallback* nunca usado (o `credentialsProvider` é sempre passado); recebe 3 caminhos de query do estado de tarefa |
| `AdaptiveSyncRuntime` | 243 + 79 | ok; repete 5× o mesmo *relay* de sinal do coordinator para a GUI |
| `SqliteSyncTaskStateRepository` | 206 + 23 | caminhos de query no construtor (mesmo padrão já simplificado na Parte 4); `Remove` só para a promoção |

**Dado real relevante:** o banco do revisor tem linhas antigas em `sync_task_state` para `active-catalog`,
`inactive-catalog` e `completed-catalog`. Hoje o coordinator as ignora (não estão nas políticas
configuradas). ~~Se os valores saírem dos enums, o `ReadAll` rejeita o texto desconhecido e a sync não
inicializa; por isso o P6-02 inclui uma migração que apaga essas linhas.~~ **Corrigido na aplicação:** o
`ReadAll` **pula** linhas com tipo ou partição desconhecidos (não falha), então essas linhas continuam
ignoradas sem migração. Ver seção 7.

---

## 3. Testes

| Suíte | Avaliação |
| --- | --- |
| `SyncTaskPolicyTests` (22) | **apagar** `promotesCompletedCatalogToActiveCatalog`, `honorsRetryAfterWithoutApplyingJitter`, `honorsRetryAfterBeforeEveryRunNowTrigger` (critério **a**); **ajustar** os que percorrem os 7 tipos (`providesStablePersistedNamesForEveryTaskKind`, `selectsDistinctDefaultPoliciesForEveryTaskKind`, `appliesDistinctDefaultCadencesAtDecisionLevel`, `prioritizesWatchingAndReadingOverInactiveAndCompletedCatalogs`, `derivesScheduledPriorityFromPartition`) para os 2 que existem; `promotesLocalChangesForImmediateSynchronization` mantém só "roda já"; os que usam `Evaluate` passam a usar `Decide`; os de backoff, *cooldown*, *jitter*, relógio e validade de cache **ficam** |
| `AdaptiveSyncCoordinatorTests` (26) | **apagar** `retainsExistingActivePartitionDuringCompletedPromotion` e `forwardsRetryAfterToTheSchedulingPolicy` (critério **a**); **ajustar** `promotesManualAndLocalChangeRequests` (sem `NotifyLocalChange`) e os que montam estados de catálogo; o resto **fica** (prioridade, uma por vez, *rerun*, gerações, cancelamento, *shutdown*, `PendingChanges` depois de `UserList`, progresso) |
| `AdaptiveSyncRuntimeTests` (14) | **manter** |
| `SqliteSyncTaskStateRepositoryTests` (7) | **apagar** `removesTaskPartitionState` (critério **a**); os demais ficam (o de texto desconhecido continua valendo) |
| `JsonSettingsReaderTests` — políticas por tipo | **ajustar** para os 2 tipos |
| ~~`SqliteDatabaseTests`~~ | ~~ganha um caso para a migração nova~~ — sem migração, nada muda (ver seção 7) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P6-01** | `[Remover]` | Promoção de partição: `NotifyLocalChange`, `SyncTaskTrigger::CompletedToActive`, `SyncTaskPromotion`, o ramo de troca de partição em `Request`, `ISyncTaskStateRepository::Remove` (+ implementação e `delete-sync-task-state.sql`). O gatilho `LocalChange` fica, porque é o que reexecuta uma partição pedida durante a execução. | −~80 linhas | baixo | `SyncTaskPolicyTests`, `AdaptiveSyncCoordinatorTests` |
| **P6-02** | `[Simplificar]` | Reduzir `SyncTaskKind`/`SyncPartition` a `UserList` e `PendingChange(s)` (enums, nomes, políticas default, prioridades, `PartitionFor`, leitura de políticas do `Settings.json`, repositório). Mesmos textos persistidos para os 2. **Migração 027** apaga de `sync_task_state` as linhas dos tipos removidos (hoje ignoradas). | −~120 linhas; fim de 5 partições fantasma | **médio** (toca o banco) | `SqliteDatabaseTests` (caso novo + snapshot), `SqliteSyncTaskStateRepositoryTests`, smoke com o banco real |
| **P6-03** | `[Remover]` | `confirmedCursor` (nunca produzido): sai do estado, do resultado, do coordinator e das queries de leitura/gravação. A coluna fica no banco. | menos estado morto | baixo | `SqliteSyncTaskStateRepositoryTests` |
| **P6-04** | `[Remover]` | `retryAfter` (nunca produzido): sai do resultado do executor e do pedido da política. | política mais simples | baixo | `SyncTaskPolicyTests` |
| **P6-05** | `[Remover]` | `SyncTaskPolicy::Evaluate` (só os testes chamam; é `Decide(...).schedule`). | −1 API | baixo | `SyncTaskPolicyTests` |
| **P6-06** | `[Simplificar]` | `ApplicationSyncTaskExecutor`: extrair `RunUserListTask` e `RunPendingChangesTask` da lambda; um helper `audit()` para o logger opcional; remover `AuthenticatedUserListConfiguration::credentials` (*fallback* nunca usado). O repositório de estado passa a usar as próprias constantes de query, e saem os 3 caminhos do executor, da composição e do `OwnedSyncTaskStateRepository`. | `Execute` de ~250 para ~40 linhas | médio | `ctest` + smoke de sync (lista e pendências) |
| **P6-07** | `[Simplificar]` | `AdaptiveSyncRuntime`: um helper para os 5 *relays* de sinal coordinator → GUI. | −~25 linhas | baixo | `AdaptiveSyncRuntimeTests` |

### Ordem sugerida
P6-05 → P6-04 → P6-03 → P6-01 → P6-02 → P6-06 → P6-07.

---

## 5. Fora de escopo (vai para a Parte 13)

Nada novo. O intervalo de sincronização das Configurações, que o agendador ignora, já está lá desde a
Parte 2.

---

## 6. Decisões para o revisor

1. **P6-02 com migração 027** apagando as linhas antigas de catálogo de `sync_task_state` (recomendo). A
   alternativa é o repositório **ignorar** linhas de tipos desconhecidos em vez de falhar, o que muda o
   comportamento para dados corrompidos e o teste `rejectsUnknownPersistedEnumTextAndMalformedTimestamp`.
2. **Manter dois enums** (`SyncTaskKind` e `SyncPartition`, 1:1) com 2 valores cada (recomendo: as duas
   colunas existem no banco e os textos são diferentes, `pending-change` × `pending-changes`), ou
   unificá-los num só?

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| P6-05 `SyncTaskPolicy::Evaluate` | `70bffaf` |
| P6-04 `retryAfter` | `546e265` |
| P6-03 `confirmedCursor` | `02df6b8` |
| P6-01 promoção de partição e `Remove` | `ebeeadb` |
| P6-02 dois tipos e duas partições (sem migração) | `70fc863` |
| P6-06 executor com um método por tarefa | `a94cb47` |
| P6-07 *relay* de sinais do runtime | `e9f4fea` |

### Correção do report: migração 027 não aplicada
A decisão 1 foi aprovada com base numa premissa errada. O `SqliteSyncTaskStateRepository::ReadAll` não
falha com texto desconhecido: ele **pula a linha** (`continue`), e o teste
`rejectsUnknownPersistedEnumTextAndMalformedTimestamp` já verificava isso. Antes, as linhas
`active-catalog`/`inactive-catalog`/`completed-catalog` do banco real eram lidas e depois descartadas pelo
coordinator (sem política); agora são descartadas já na leitura. O resultado é o mesmo, sem tocar no
banco. As linhas continuam gravadas; apagá-las fica como opção, se o revisor quiser.

### Notas sobre os testes
- `SyncTaskPolicyTests`: saíram `promotesCompletedCatalogToActiveCatalog` e os 2 de `retryAfter`;
  `promotesLocalChangesForImmediateSynchronization` virou `runsLocalChangesImmediately`;
  `prioritizesWatchingAndReadingOverInactiveAndCompletedCatalogs` virou
  `prioritizesUserListOverPendingChanges`; os que percorriam 7 tipos percorrem os 2.
- `AdaptiveSyncCoordinatorTests`: saíram `retainsExistingActivePartitionDuringCompletedPromotion` e
  `forwardsRetryAfterToTheSchedulingPolicy`; `promotesManualAndLocalChangeRequests` virou
  `runsManualRequestsForFreshPartitionsImmediately`; os que usavam partições de catálogo como "segunda
  partição" usam `PendingChanges` (em `recordsIndependentProgressWhenAnotherPartitionFails` a ordem
  inverteu: a `UserList` conclui e a `PendingChanges` falha).
- `SqliteSyncTaskStateRepositoryTests`: saíram `removesTaskPartitionState` e
  `preservesQueryLoaderDiagnostic` (o caminho da query deixou de ser parâmetro, critério **a**); a linha
  desconhecida do teste de enum agora é uma linha `active-catalog`, igual às do banco real.
- `SynchronizationStatusControllerTests::ignoresUntrackedPartitions` saiu: com 2 partições, as duas são
  acompanhadas (`isTracked` removido).
- `JsonSettingsReaderTests`: o caso de duração negativa usa `pending-change` em vez de `cover`. Uma
  política `cover` no `Settings.json` passa a ser ignorada, como qualquer chave desconhecida.
- `AniListAccountOwnershipTests`: saiu a linha que verificava uma partição de capas.

### Correção após o primeiro build
- `SqliteSyncTaskStateRepositoryTests::roundTripsUtcTimestampsAndNullableValues` quebrou: a leitura ordena
  por `task_kind`, e com os tipos novos o estado sem datas (`pending-change`) vinha primeiro. O teste passou a
  gravar o estado completo como `PendingChange` e o vazio como `UserList` (`ac42360`).
- `AniListOAuthCallbackReceiverTests::forwardsCallbackToExistingServer` falhou uma vez sem relação com a
  Parte 6 (nenhum arquivo tocado); rodado de novo, passou.

### Validação final (build do revisor)
- `scripts/validate.ps1`: build MSVC + `ctest` todo verde (após `ac42360`).
