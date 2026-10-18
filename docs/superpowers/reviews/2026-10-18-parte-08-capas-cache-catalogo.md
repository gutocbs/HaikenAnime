# Report — Parte 8: Capas, cache e catálogo sazonal

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `71c1a9b` (`feature/revisao-geral`)
- **Status:** aprovado (com a recomendação) e aplicado — aguardando build do revisor

---

## 1. O que esta parte faz

- **Capas** (`CoverDownloadCoordinator`): a Home envia a janela de capas visíveis e a de *prefetch*. O
  coordinator descarta da fila o que saiu da janela e responde na hora com a capa já em cache (mesma mídia,
  URL e qualidade, arquivo presente). O resto é baixado com no máximo `maxConcurrentDownloads` em paralelo,
  visíveis primeiro. Falhas temporárias (rede, 408/429/5xx) são refeitas até `maxRetries`, respeitando
  `Retry-After`; falhas definitivas entram em *cooldown* (5 min).
  - `QtCoverDownloader` baixa para um temporário `cover-XXXXXX.tmp` com limite de bytes e *timeout*.
  - `CoverFileStore` valida o conteúdo (formato real, MIME, dimensões 32–4096) e publica atomicamente como
    `<mediaId>-<hash da URL>.<ext>` em `AppLocalDataLocation/covers`.
  - `SqliteCoverCacheRepository` registra a entrada em `cover_cache`; o arquivo antigo só sai depois da
    gravação. Uma capa que a Home não consegue carregar volta a "ausente" (`ReportMissingFile`).
  - `ResolveCoverSource` escolhe a URL pela qualidade pedida, com *fallback* para as outras.
- **Temporários** (`CoverTemporaryStore`): na inicialização e na limpeza, apaga os `cover-*.tmp` abandonados.
  No Windows faz isso relativo a um *handle* da pasta (`NtCreateFile`), sem seguir links nem junções. É
  complexo de propósito (troca da pasta durante a limpeza não redireciona a exclusão) e **fica como está**.
- **Limpeza do cache** (`ClearLocalCacheUseCase`, botão em Configurações): invalida downloads em andamento
  (nova geração), apaga entradas e arquivos de capa e os temporários, e devolve contagens e falhas parciais.
- **Catálogo sazonal** (`SeasonalCatalogCoordinator` + `GraphQlSeasonalCatalogDataSource`): com ano e estação
  válidos, busca páginas de 50 da query `seasonal-catalog.graphql` (filtro adulto opcional), junta páginas sem
  repetir mídia e guarda até 24 páginas por 5 min em memória. Espaça as requisições em pelo menos 1 s. Pedidos
  feitos durante uma requisição viram um único pendente, e uma resposta atrasada de outra seleção é
  descartada.

---

## 2. Componentes e achados

| Componente | Linhas | Achado |
| --- | --- | --- |
| `ClearLocalCacheUseCase` + `ICacheCleanupParticipant` | 147 | a composição passa **lista vazia** de participantes, então `removedDerivedItems`, a interface e o laço são mortos. `Cancel()` **nunca é chamado**: `Start` roda síncrono na thread da GUI, e a máquina de estados atômica (`Idle/Running/CancellationRequested`) e `result.cancelled` só existem para participantes |
| `CoverDownloadCoordinator` (application) | 202 + 64 | **única classe de `application` que inclui `infrastructure/logging/AsyncLogger.h`**; `Active::attempt` é sempre 0 e nunca é lido (as tentativas ficam em `attempts_`) |
| `ICoverFileStore::RemoveOrphans` + `CoverFileStore::RemoveOrphans` | ~25 | **sem uso em produção**; obriga 4 *fakes* de teste a implementá-lo |
| `CoverDownloadTypes.h` | 34 | `CoverFailureCategory::Persistence` nunca é produzido |
| `SqliteCoverCacheRepository` | 112 + 26 | 4 caminhos de query no construtor (padrão já simplificado nas Partes 4, 6 e 7) |
| `SeasonalCatalogCoordinator` | 228 + 97 | o mesmo `SeasonalCatalogRequest{year_, seasonKey_, página, perPage_, includeAdultContent_}` é montado à mão em 4 lugares; `cachePage` repete o laço de `removeCachedPage` |
| `QtCoverDownloader`, `CoverFileStore`, `CoverSourceResolver`, `CoverQuality`, `GraphQlSeasonalCatalogDataSource`, `SeasonalCatalogTypes` | — | ok. `etag`/`lastModified` são gravados mas nunca usados para revalidar; ficam, porque são colunas do banco |

---

## 3. Testes

| Suíte | Avaliação |
| --- | --- |
| `ClearLocalCacheUseCaseTests` (6) | **apagar** `cancellationStopsBeforeTheNextParticipantAndReturnsToIdle` e `acceptedExternalCancellationIsReportedByCompletion` (critério **a**). **Ajustar** `clearsOnlyCoversAndRegisteredDerivedParticipants` (vira `clearsCoversAndTemporaryFiles`), `reportsPartialFailuresWithoutClaimingSuccess` e `repeatedCleanup…` (sem participantes). **Ajustar** `rejectsAConcurrentStart`: a chamada aninhada passa a vir do *fake* de temporários, não de um participante |
| `CoverFileStoreTests` (3) | **apagar** `removesOnlyBoundedOrphans` (critério **a**) |
| `SeasonalCatalogTests` (21) | **apagar** `graphQlDataSourceUsesExternalQueryAndExplicitSeasonVariables` (critério **b**): `graphQlDataSourcePostsExplicitVariablesAndParsesCatalogResponse` verifica as mesmas variáveis e a query no *payload* realmente enviado; o resto **fica** |
| `CoverDownloadCoordinatorTests` (6), `CoverTemporaryStoreTests` (7), `QtCoverDownloaderTests` (2), `CoverSourceResolverTests` (1), `CoverSettingsTests` (3), `SqliteCoverCacheRepositoryTests` (5), `SeasonalCatalogControllerTests` | **manter** (os *fakes* perdem `RemoveOrphans`) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P8-01** | `[Simplificar]` | `ClearLocalCacheUseCase` sem participantes e sem cancelamento: sai `ICacheCleanupParticipant`, `removedDerivedItems`, `Cancel()`, `State()`, `ClearLocalCacheState` e `result.cancelled`; um `bool` impede o início aninhado. O `SettingsController` perde o ramo "foi cancelada" (inalcançável) | −~70 linhas | baixo | `ClearLocalCacheUseCaseTests`, `SettingsControllerTests`, smoke "limpar cache" |
| **P8-02** | `[Padronizar]` | Mover `CoverDownloadCoordinator` e `ClearLocalCacheUseCase` para `src/app`, junto dos outros coordinators Qt que logam pelo `AsyncLogger`. `application/covers` fica com tipos e interfaces, e `application/cache` deixa de existir | camadas corretas | baixo | build + suítes de capas |
| **P8-03** | `[Remover]` | `RemoveOrphans` da interface, da implementação e dos 4 *fakes* | −~30 linhas | baixo | `CoverFileStoreTests` |
| **P8-04** | `[Remover]` | `CoverFailureCategory::Persistence` e `Active::attempt` | limpeza | baixo | `CoverDownloadCoordinatorTests` |
| **P8-05** | `[Simplificar]` | `SqliteCoverCacheRepository` com as próprias constantes de `SqliteQueries` (só a conexão no construtor) | −4 parâmetros | baixo | `SqliteCoverCacheRepositoryTests` |
| **P8-06** | `[Simplificar]` | `SeasonalCatalogCoordinator`: um `requestForPage(page)` substitui as 4 montagens; `cachePage` chama `removeCachedPage` | −~10 linhas | baixo | `SeasonalCatalogTests` |

### Ordem sugerida
P8-04 → P8-03 → P8-05 → P8-06 → P8-01 → P8-02.

---

## 5. Fora de escopo (vai para a Parte 13)

| Item |
| --- |
| `CoverDownloadCoordinator` ignora sem log a falha de leitura do cache no construtor e a falha de remoção em `ReportMissingFile` |

*(As estações em português fixas em `DefaultSeasonalCatalogSeasonOptions` já estão na Parte 13, vindas da
Parte 2.)*

---

## 6. Decisões para o revisor

1. **P8-02: mover para `src/app`** (recomendo). A camada de apresentação já inclui coordinators de `src/app`,
   então a mudança só alinha estes dois. A alternativa é mantê-los em `application` e trocar o `AsyncLogger*`
   por funções de log injetadas, o que acrescenta código para três níveis (info/warning/error).

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| P8-04 `CoverFailureCategory::Persistence` e `Active::attempt` | `883fff7` |
| P8-03 `RemoveOrphans` | `12f7ba0` |
| P8-05 repositório de capas com queries próprias | `c9ca9d4` |
| P8-06 `requestForPage` no catálogo sazonal | `066a875` |
| P8-01 limpeza de cache sem participantes e sem cancelamento | `56d1a65` |
| P8-02 `CoverDownloadCoordinator` e `ClearLocalCacheUseCase` em `src/app` | `0b27021` |
| Teste duplicado do catálogo sazonal (critério **b**) | `2abaa2d` |

Notas:
- P8-01: `CacheCleanupFailure::participant` virou `component` (o mesmo nome de `CoverCacheCleanupFailure`). O
  texto "A limpeza do cache local foi cancelada." saiu do `SettingsController`; a entrada correspondente nas
  traduções fica para a Parte 11.
- P8-01, testes: os 2 de cancelamento saíram. Os outros 4 de `ClearLocalCacheUseCaseTests` foram ajustados (o
  início aninhado agora vem do *fake* de temporários, e o teste confirma que um novo início funciona depois).
  Em `SettingsControllerTests`, o participante saiu: a contagem esperada passou de 14 para 9 itens, e o pedido
  concorrente vem do *fake* de temporários.
- P8-02: com a mudança, nenhum arquivo de `src/application` inclui mais algo de `src/infrastructure`.
