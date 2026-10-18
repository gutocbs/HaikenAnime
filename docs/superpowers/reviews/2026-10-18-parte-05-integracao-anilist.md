# Report — Parte 5: Integração AniList

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `22182cd` (`feature/revisao-geral`)
- **Status:** aprovado (com as correções e as duas recomendações) e aplicado — validado

---

## 1. O que esta parte faz

Tudo o que conversa com o AniList, exceto o agendamento (Parte 6):

- **Cliente e parsers** (`infrastructure/anilist`):
  - `AniListGraphQlClient` faz `POST` GraphQL com timeout, retry limitado, *probe* de cancelamento e log
    seguro (sem tokens nem *payloads*);
  - `AniListGraphQlResponseParser` valida o envelope `data`/`errors`;
  - `AniListGraphQlUserListParser` lê a `MediaListCollection` (filtro de status e tipo, ids observados) e
    `AniListGraphQlPageParser` lê um `Page` de catálogo (usado pelo catálogo sazonal);
  - `AniListMediaMapper` converte JSON → DTO → `Media`.
- **Fontes de dados:** `GraphQlAniListDataSource` (implementa `IAniListDataSource`) baixa a lista do
  usuário em *chunks*, e ainda suporta partições de catálogo. `GraphQlSeasonalCatalogDataSource` atende o
  catálogo sazonal.
- **Sincronização:** `AniListSyncService` pagina a fonte, concilia mudanças pendentes com o valor remoto
  (`AniListPendingChangeReconciler`), grava as mídias e o checkpoint de página e respeita cancelamento e
  timeout. O executor da Parte 6 o chama para `ANIME` e depois `MANGA`; a conciliação do snapshot completo
  é feita fora dele, por `ReconcileCompleteAniListSnapshot`.
- **Outbox** (envio das edições): `AniListPendingChangeDrain` percorre as mídias com pendências e
  `AniListPendingChangeProcessor`, para cada mídia, compacta (a última edição de cada campo vence), envia a
  deleção confirmada ou um `SaveMediaListEntry` com progresso, nota e status (`AniListUpdateClient`),
  atualiza os status (Processing → Succeeded/Failed) e avisa o observador (histórico de reconhecimento).
  `AniListPendingChangeFactory` cria as mudanças e `AniListSyncErrorClassifier` classifica erros pelo texto.
- **Conta:** `AniListAuthManager` guarda as credenciais (Credential Manager), monta a URL OAuth, valida o
  token com a query `viewer` e pode ser suspenso durante a remoção. `AniListOAuthCallbackReceiver`
  encaminha o callback `haikenanime://` para a instância em execução, e `RemoveAniListAccountUseCase`
  orquestra a remoção. `AniListAccountOwnership` decide o dono das edições e quando pular a sync.

---

## 2. Componentes e achados

| Componente | Situação |
| --- | --- |
| `AniListSyncService` (173 + 76 linhas) | tem **dois caminhos de fonte** (`IMediaDataSource` legado e `IAniListDataSource`) e dois colaboradores opcionais, `snapshotReconciler` e `pendingProcessor`, que a produção **sempre passa nulos** |
| `IMediaDataSource`, `FileAniListDataSource`, `RecordedGraphQlAniListDataSource` (143 linhas, *header-only*), `AniListMediaMapper::fromFixtureJson` | só servem ao `InitialSyncCoordinator` (morto: `ApplicationContext::initialSync` nunca é atribuído) e a testes |
| `AniListSyncWorker`, `AniListUpdateWorker` | compilados, **sem nenhum uso** (nem em testes) |
| `GraphQlAniListDataSource` — ramo de catálogo; `AniListDataSourceRequest::ForPartition` (variáveis de catálogo); campos `variables`/`queryIdentity`/`refresh` | a produção só pede `UserList`; o `fetchPage` **descarta** as `variables` montadas em `ForPartition`, e `queryIdentity`/`refresh` só existem para o cache da fonte gravada |
| `AniListGraphQlClient::execute(query, AniListDataSourceRequest)` | sobrecarga que só lê `request.variables` (e faz a infraestrutura depender de um tipo de `application`) |
| `media-page.graphql`, `AniListSettings::mediaQueryFile` | só usados por testes; a chave é obrigatória no `Settings.json` sem servir para nada |
| Mapeamento `UserListStatus` ↔ status AniList (`CURRENT`, `PAUSED`…) | escrito em **4 lugares**: `PersonalListChangeService`, `AniListPendingChangeReconciler` (ida e volta), `MediaSyncFilter`, `AniListMediaMapper` |
| `AniListPendingChangeGrouper` | o processor trata **uma mídia por vez** (`getPending(mediaId)`), então o agrupamento por mídia sempre produz no máximo um grupo |
| `AniListMergePolicyResolver` + `AniListMergeService` + `domain/anilist/AniListMerge{Policy,Result,Decision}` | o único uso é o reconciler, para 3 campos: progresso = maior valor (conflito se não for inteiro), nota e status = valor local. As políticas `RemoteWins`/`LocalWins`/`RequiresConfirmation` e os campos de catálogo de `AniListField` (`ExternalId`, `Title`, `AlternativeTitles`, `CoverUrl`, `Synopsis`, `LocalFiles`) nunca são alcançados |
| `HttpFactory` | 7 linhas que embrulham `new QNetworkAccessManager(parent)` |
| Auth/OAuth/remoção de conta, `AniListUpdateClient`, `AniListViewerClient`, `GraphQlQueryStore`, `AniListAccountOwnership` | ok |

---

## 3. Testes

| Suíte | Avaliação |
| --- | --- |
| `InitialSyncCoordinatorTests` (inteira) | **apagar** com P5-01 (critério **a**) |
| `AniListFlowTests` (29) | **dividir**: os casos da fonte em arquivo e do caminho legado (`fixtureAppliesFilterAndPagination`, `embeddedProductionFixtureCanBeRead`, `invalidFixtureReturnsError`, `legacyDataSourceDoesNotReconcileUnseenMedia`, `replayAfterCheckpointFailureIsIdempotentForLegacySource`…) e os de conciliação **dentro** do serviço (P5-04) são **apagados** (critério **a**); os de comportamento do serviço (paginação inválida, página errada, checkpoint, cancelamento, timeout, progresso, conciliação de pendências antes de gravar, `ReconcileCompleteAniListSnapshot`) **são mantidos**, passando a usar uma fonte falsa de `IAniListDataSource` |
| `AniListGraphQlParsingTests` (29) | 7 casos `recorded*` e `graphQlAdapterPostsPartitionVariablesAndParsesCatalogPage` são **apagados** com P5-01/P5-03 (critério **a**); `recordedLibraryFixtureIsComplete` **fica** (renomeado), porque testa o `AniListGraphQlPageParser`, ainda usado pelo catálogo sazonal, com uma resposta real de 50 mídias; os casos de cliente que usam `media-page.graphql` só como texto de query passam a usar outra query; o restante (envelope, capas, metadados, links, adaptador de lista do usuário, telemetria segura, cancelamento) **fica** |
| `AniListMergeServiceTests` (6), `AniListMergePolicyResolverTests` (2) | **apagar** com P5-07 (critério **a**). As regras que de fato importam (progresso maior vence, nota e status locais, conflito) já são cobertas por `AniListPendingChangeReconcilerTests` (6 casos) |
| `AniListPendingChangeProcessorTests::groupsChangesByMedia` | **ajustar** com P5-06: o *fake* devolve mudanças de várias mídias para um único `mediaId`, o que não acontece com o repositório real. Passa a verificar que as mudanças da mídia vão num único envio |
| demais (`AniListGraphQlUserListParserTests`, `AniListUpdateClientTests`, `AniListAuthManagerTests`, `AniListOAuth*Tests`, `RemoveAniListAccountUseCaseTests`, `AniListAccountOwnershipTests`, `AniListPendingChange{Drain,Factory,Reconciler}Tests`, `AniListSyncErrorClassifierTests`, `GraphQlQueryStoreTests`, `Windows*`) | **manter** |
| `LocalLibraryScanCompositionTests::productionCompositionDoesNotCreateLegacyStartupSynchronization` e `composesAdaptiveSynchronizationWithTheLegacyStartupSeam` | **ajustar** com P5-01: deixam de citar `initialSync`; o segundo continua verificando a sync adaptativa |
| fixtures `media-library.json` e `invalid-media-library.json` | **apagar** com P5-01: estão no formato da fonte em arquivo, que nenhum código de produção lê. `graphql/page-response.json` e `graphql/userlist-response.json` **continuam** (parsers de `Page` e de lista do usuário) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P5-01** | `[Remover]` | Remover a sync de fixture: `InitialSyncCoordinator` (+ `ApplicationContext::initialSync`), `RecordedGraphQlAniListDataSource`, `FileAniListDataSource`, `IMediaDataSource` e o caminho legado do `AniListSyncService` (construtor e ramo `dataSource_`), `AniListMediaMapper::fromFixtureJson` e os apelidos de status que só o formato de fixture usava ("Finished Airing", "RELEASED", "PLAN TO WATCH", "PLAN TO READ", "ON_HOLD"), `Settings::syncIntervalMs`/`sync.intervalMs` (só o coordenador usava) e as fixtures da seção 3. *(Antecipa o item previsto na Parte 6, porque a fonte gravada que ele usa sai aqui.)* | −~1.000 linhas de produção e ~380 de fixture | médio | build + `ctest` + smoke de sync |
| **P5-02** | `[Remover]` | `AniListSyncWorker` e `AniListUpdateWorker`. | −~90 linhas | baixo | build |
| **P5-03** | `[Remover]` | Catálogo fora da fonte da lista do usuário: ramo de catálogo do `GraphQlAniListDataSource`, `ForPartition` e os campos `variables`/`queryIdentity`/`refresh` de `AniListDataSourceRequest` (o pedido vira só o `MediaSyncFilter`), a sobrecarga do cliente GraphQL, `media-page.graphql` e `AniListSettings::mediaQueryFile` (+ chave no `Settings.json` e validação). Os valores de catálogo dos enums de partição saem na Parte 6. | fonte da lista do usuário só com o que é usado | baixo | `AniListGraphQlParsingTests` + smoke de sync |
| **P5-04** | `[Simplificar]` | `AniListSyncService`: remover os parâmetros `snapshotReconciler` e `pendingProcessor` (nulos em produção; a conciliação do snapshot é feita por `ReconcileCompleteAniListSnapshot`). | um construtor; −~30 linhas | baixo | `AniListFlowTests` mantidos |
| **P5-05** | `[Unificar]` | `AniListListStatus.h` (em `application/anilist`) com `AniListListStatusName(UserListStatus)`, `ParseAniListListStatus(QString)` e o mapeamento das listas habilitadas (que hoje está em `MediaSyncFilter`). Os 4 lugares passam a usá-lo. | uma fonte da verdade | baixo | `PersonalListChangeServiceTests`, `AniListPendingChangeReconcilerTests`, `AniListGraphQl*Tests` |
| **P5-06** | `[Simplificar]` | Remover `AniListPendingChangeGrouper`: o processor ordena as mudanças ativas da mídia e envia um único grupo (a mesma ordenação de hoje). | −2 arquivos | baixo | `AniListPendingChangeProcessorTests` |
| **P5-07** | `[Simplificar]` | Trocar `AniListMergePolicyResolver` + `AniListMergeService` + os 3 tipos de merge do domínio por uma função do reconciler: progresso = maior valor (conflito se não for inteiro), nota e status = valor local. Remover de `AniListField` os valores nunca usados (`ExternalId`, `Title`, `AlternativeTitles`, `CoverUrl`, `Synopsis`, `LocalFiles`), **fixando os números dos que ficam** (`Progress = 5`, `PersonalScore = 6`, `ListStatus = 7`, `Deletion = 9`), porque o campo é gravado como inteiro em `anilist_pending_changes`. `AniListField` identifica o campo de uma *mudança pendente a enviar* ao AniList, não os dados exibidos (título, sinopse e capa vêm de `Media`, preenchida pelo `AniListMediaMapper`); a `AniListPendingChangeFactory` já recusa qualquer campo além desses 4. O default de `AniListPendingChange::field` passa de `ExternalId` para `Progress` (sempre sobrescrito pela factory e pelo repositório). | −7 arquivos | médio | `AniListPendingChangeReconcilerTests`, `SqliteRepositoryTests` (*round-trip* do outbox) |
| **P5-08** | `[Remover]` | `HttpFactory` → `std::make_unique<QNetworkAccessManager>()` nos 3 pontos de uso. | −2 arquivos | baixo | build |

### Ordem sugerida
P5-02 → P5-08 → P5-01 → P5-04 → P5-03 → P5-05 → P5-06 → P5-07.

---

## 5. Fora de escopo (vai para a Parte 13)

Nada novo nesta parte.

---

## 6. Decisões para o revisor

1. **P5-01 inclui o `InitialSyncCoordinator`**, previsto originalmente na Parte 6. Fazer aqui (recomendo,
   porque a fonte gravada que ele usa sai aqui) ou deixar para a Parte 6?
2. **P5-07:** remover os valores não usados de `AniListField` fixando os números dos que ficam (recomendo),
   ou manter o enum como está e só simplificar o merge?

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| P5-02 workers sem uso | `0f00104` |
| P5-08 `HttpFactory` | `5588360` |
| P5-01 sync de fixture (inclui `InitialSyncCoordinator`) | `6d57893` |
| P5-04 colaboradores nulos do `AniListSyncService` | `0ec0b9b` |
| P5-03 só o caminho da lista do usuário na fonte AniList | `81a4c7a` |
| P5-05 `AniListListStatus.h` | `9e02a92` |
| P5-06 `AniListPendingChangeGrouper` removido | `242a39c` |
| P5-07 merge simplificado e `AniListField` numerado | `19f70f3` |

Notas sobre os testes:
- `AniListFlowTests`: de 29 para 21 casos. Saíram os da fonte em arquivo, os que só verificavam que o caminho
  legado não concilia, o de variáveis de catálogo e o de partição de catálogo. Os de comportamento do serviço
  passaram a usar fontes falsas de `IAniListDataSource` (`ConfigureTwoPages`). Os que verificavam a
  conciliação **dentro** do serviço agora verificam `lastOutcome()` (snapshot completo e ids observados),
  que é o que o executor usa.
- `AniListGraphQlParsingTests`: saíram os 7 casos da fonte gravada e o do adaptador de catálogo;
  `pageParserReadsTheRecordedLibraryResponse` (ex-`recordedLibraryFixtureIsComplete`) continua com
  `graphql/page-response.json`.
- `AdaptiveSyncCoordinatorTests::turnsPersistedCheckpointIntoTheNextPartitionRequestPage` usa a partição
  `UserList` (antes usava uma de catálogo só para ler as variáveis de catálogo, que deixaram de existir).
- `AniListPendingChangeProcessorTests::groupsChangesByMedia` virou `sendsEveryChangeOfTheMediaInOneUpdate`.
- `JsonSettingsReaderTests`: a chave `mediaQueryFile` saiu dos JSONs de teste (o leitor não a lê mais).

### Correção após o primeiro build
- `AniListGraphQlParsingTests` falhou: o P5-01 tirou do `AniListMediaMapper` o apelido `ON_HOLD`,
  apresentado no report como exclusivo do formato de fixture em arquivo. Na verdade, ele também chega pelo
  formato GraphQL (`graphql/page-response.json` tem 2 entradas assim), e as entradas viravam `Unknown`.
  O apelido foi restaurado em `1cff615`. Os outros apelidos removidos ("PLAN TO WATCH", "PLAN TO READ",
  "Finished Airing", "RELEASED") não aparecem em nenhuma entrada restante.

### Validação final (build do revisor)
- `scripts/validate.ps1`: build MSVC + `ctest` todo verde (após `1cff615`).
- Smoke: a sync baixou a lista do usuário e uma edição chegou ao AniList.
