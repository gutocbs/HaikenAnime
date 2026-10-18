# Plano de revisão geral — simplificação e limpeza do HaikenAnime V2

- **Data:** 2026-10-18
- **Branch de origem:** `feature/reconhecimento-episodios` (`775bcec`)
- **Objetivo:** deixar o código e os testes mais simples e limpos **sem alterar comportamento**.
  Não é uma rodada de correção de bugs: problemas encontrados são **anotados**, não corrigidos.
- **Referência do programa:** [`SERVICE.md`](../../../SERVICE.md)

---

## 1. Como a revisão funciona

1. O programa foi dividido em **12 partes** (seção 3). Cada parte é revisada inteira — código **e** testes.
   A **Parte 13** reúne os problemas "fora de escopo" anotados nos reports e é a única que corrige
   comportamento.
2. Ao fim de cada parte é entregue um **report** (modelo na seção 4) explicando o que o código faz, sua
   função no programa, a avaliação dos testes e a lista de melhorias propostas.
3. **Nenhuma alteração é feita antes da sua aprovação do report.** Você pode aprovar, recusar ou ajustar
   item a item.
4. Itens aprovados são aplicados (seção 2) e só então seguimos para a próxima parte.
5. Ao final, o `SERVICE.md` é revisado de novo para refletir o estado limpo.

---

## 2. Garantia de "não alterar comportamento"

### 2.1 Baseline (antes da Parte 1)
- Build limpo (`BUILD_TESTING=ON`) + `ctest` completo; registrar quantidade de testes, falhas pré-existentes
  e tempo. Falhas pré-existentes são documentadas e não contam como regressão.
- Smoke manual do app (checklist 2.4) para termos a referência visual/funcional.

### 2.2 Aplicação de cada item
- **Um item = um commit pequeno** (`refactor:`, `chore:`, `test:`, `build:`, `docs:`). Nunca misturar
  refatoração com mudança de comportamento.
- Refatorações de lógica são feitas **sob testes existentes**; se a área não tem cobertura suficiente para
  provar equivalência, primeiro escrevemos o teste de caracterização (commit separado) e depois refatoramos.
- Mudança de contrato (sinal, `Q_PROPERTY`, `Q_INVOKABLE`, interface `I*`, query, chave JSON) atualiza todos
  os consumidores e testes **no mesmo commit**.
- Persistência: nenhuma mudança de schema ou de valores gravados. Migrações existentes continuam aplicáveis
  a bancos antigos (testado com banco na versão anterior).
- Após cada item: build + suítes afetadas.

### 2.3 Fechamento de cada parte
- Build limpo + `ctest` completo, comparado com o baseline (mesmos testes passando, exceto os removidos com
  aprovação).
- Smoke manual quando a parte tocar QML, composição ou fluxos de usuário.

### 2.4 Checklist de smoke manual
Abrir o app (banco existente) · Home carrega e filtra/ordena/busca · selecionar mídia e editar
progresso/nota/status · catálogo sazonal carrega e pagina · Configurações abre/salva · sync manual roda e a
barra some · próximo episódio local abre · telas de histórico e "Assistindo agora" abrem · fechar o app sem
travar (log "Application shutdown completed").

### 2.5 Critérios para apagar um teste
Um teste só é removido se cair em pelo menos um caso abaixo — e o report diz qual e o que o substitui:
- **(a)** testa código que está sendo removido (código morto);
- **(b)** é duplicado por outro teste igual ou mais forte;
- **(c)** verifica **texto do código-fonte** (busca de string) em vez de comportamento, e existe teste
  comportamental equivalente — ou a regra não precisa de teste;
- **(d)** testa algo trivial (default de struct, getter/setter, constante) sem regra de negócio.

### 2.6 Categorias de item
`[Remover]` código morto · `[Simplificar]` mesma lógica com menos código · `[Unificar]` duplicação ·
`[Padronizar]` naming/estilo · `[Teste-remover]` · `[Teste-ajustar]` · `[Doc]`.
Cada item recebe risco **baixo / médio / alto** e a forma de validação.

---

## 3. Partes da revisão

Ordem pensada para remover ruído primeiro (legado e build), depois ir das camadas mais internas
(domínio, infraestrutura) às externas (apresentação, QML, composição). Os achados preliminares abaixo são
**hipóteses** levantadas no mapeamento; cada uma será confirmada (ou descartada) no report da parte.

| # | Parte | Escopo principal | Tamanho aprox. |
| --- | --- | --- | --- |
| 1 | Repositório e build | legado V1, `CMakeLists.txt`, recursos, fixtures, `.gitignore`, `README`/`codex.md` | CMake 2.1k + legado ~168k |
| 2 | Domínio e regras de mídia | `src/domain`, `application/media`, `application/configuration` | ~1,7k |
| 3 | Infraestrutura base | `SqliteDatabase` + migrações, `SqlQueryStore`, `SqliteQueryConfiguration`, `JsonSettingsReader`, `AsyncLogger`, `secrets` | ~2,5k + SQL |
| 4 | Persistência de mídia e outbox | `SqliteMediaRepository`/`Mapper`, `SqlitePendingChangeRepository`, `SqlitePersonalListChangeWriter`, `SqliteUserPreferencesRepository`, `PersonalListChangeService`, `SeasonalPersonalListService` | ~1,5k |
| 5 | Integração AniList | `application/anilist`, `infrastructure/anilist`, `AniListOAuthCallbackReceiver` | ~3,8k |
| 6 | Agendamento e execução da sync | `application/scheduling`, `AdaptiveSync*`, `ApplicationSyncTaskExecutor`, `InitialSyncCoordinator`, `SqliteSyncTaskStateRepository` | ~2,2k |
| 7 | Biblioteca local | `application/library`, `infrastructure/library`, `SqliteLocalFileRepository`, `LocalLibrary*Coordinator` | ~1,9k |
| 8 | Capas, cache e catálogo sazonal | `covers`, `cache`, `catalog`, `SeasonalCatalogCoordinator`, `SqliteCoverCacheRepository` | ~1,7k |
| 9 | Reconhecimento de episódios | `domain/recognition`, `application/recognition`, `infrastructure/recognition`, repositórios de plugin/histórico, `RecognitionComposition`, `MpvAwareLocalFileOpener` | ~4,3k |
| 10 | Controllers de apresentação | `src/presentation/*` | ~3,5k |
| 11 | QML e traduções | `resources/qml`, `translations` | ~4,6k |
| 12 | Composição e entrada | `main.cpp`, `ApplicationComposition`, `TranslationLoader`, fechamento do `SERVICE.md` | ~1,1k |
| 13 | Correções | itens "fora de escopo" coletados nas Partes 1–12 | — |

Os testes de cada área (≈25,8k linhas em `tests/unit`) são revisados **na parte da área**.

### Parte 1 — Repositório e build
- `[Remover]` Legado V1 não compilado: `QML/`, `mainclass.*`, `src/base/`, `src/utilities/`, `src/clients/`,
  `lib/robotlib/`, `qrc/`, `qml.qrc`, `queries.qrc`, `HaikenAnime.pro`, `HaikenAnime.pro.user` (a V1 de
  referência está em `F:\Haiken V1`).
- `[Simplificar]` ~85 executáveis de teste recompilam as mesmas fontes, listadas à mão: criar uma
  biblioteca estática interna (ex.: `haiken_core`) + Anitomy como biblioteca + função `haiken_add_test()`.
  Expectativa: `CMakeLists.txt` de ~2.100 para poucas centenas de linhas e build mais rápido.
- `[Simplificar]` Queries SQL/GraphQL listadas duas vezes (alias + bundle) → GLOB, como já é feito nas migrações.
- `[Remover]` `fixture_resource_bundle` embute fixtures de teste (~83k linhas de JSON) no executável de
  produção — confirmar dependência do `InitialSyncCoordinator` (Parte 6).
- `[Remover]` Fixtures aparentemente sem uso: `graphql-error.json`, `graphql-page.json`,
  `graphql/empty-response.json`, `graphql/error-response.json`.
- `[Doc]` `README` (seção Architecture cita `QML/`) e `codex.md` (estrutura `persistence/sqlite`,
  `third_party/`, `CMakePresets.json`, `haikenanime.db`) descrevem uma estrutura que não existe.
- `[Padronizar]` `.gitignore` ignora `/docs` e `*superpowers*/`, mas esses arquivos são versionados à força.

### Parte 2 — Domínio e regras de mídia
- `[Simplificar]` `operator==` manual de `UserPreferences` → comparação *defaulted* do C++20.
- `[Padronizar]` `Media` é uma classe com campos públicos `PascalCase` (o `codex.md` desaconselha entidade
  sem invariantes) — avaliar custo/benefício; possivelmente só documentar.
- Cabeçalhos "Created by …" gerados pela IDE, helpers minúsculos (`ProgressFormatter`), tipos
  `Settings`/`HttpSettings`/`AniListSettings`.

### Parte 3 — Infraestrutura base
- `[Simplificar]` `SqliteDatabase::migrate()` é uma cadeia manual de ~400 linhas (inspect → alter → mark)
  → executor de migrações orientado a tabela, com a mesma ordem e a mesma idempotência.
- `[Unificar]` Registro de queries inconsistente: `sqlite-queries.json` cobre parte das queries; outras
  (plugin, histórico, `read-pending-media-ids`) são caminhos fixos no C++. Escolher **um** modelo
  (sugestão: constantes de caminho, eliminando o JSON e o `hasExistingQueries`).
- `[Simplificar]` Repositórios recebem 5–10 caminhos de query posicionais (`queries[0..9]`) → struct com
  campos nomeados.
- `[Remover]` `FileSecretStore` só é usado em testes (produção usa Windows Credential Manager).

### Parte 4 — Persistência de mídia e outbox
- Revisar a agregação `IMediaRepository` (4 interfaces) e onde cada uma é de fato necessária.
- `[Unificar]` `persistHomeSortPreference` / `persistLastAniListUserId` (em `main.cpp`) duplicam o padrão
  "copiar preferências → alterar campo → `replace`".

### Parte 5 — Integração AniList
- `[Remover]` `AniListSyncWorker` e `AniListUpdateWorker` são compilados mas não têm uso.
- `[Simplificar]` `AniListSyncService` tem dois construtores e dois tipos de data source
  (`IMediaDataSource` legado e `IAniListDataSource`); produção só usa o segundo.
- `[Remover]` `FileAniListDataSource` / `RecordedGraphQlAniListDataSource` (header-only + `.cpp` de 1 linha)
  só servem ao `InitialSyncCoordinator` e a testes.
- `[Remover]` `Settings.anilist.mediaQueryFile` (aponta para `media-page.graphql`, que não é recurso) parece
  morto, mas é obrigatório no `JsonSettingsReader`.
- `HttpFactory` com 7 linhas; pipeline de pending changes (Factory, Compactor, Grouper, Reconciler,
  Processor, Drain, Merge*) — confirmar responsabilidades e sobreposições.

### Parte 6 — Agendamento e execução da sync
- ~~`[Remover]` `InitialSyncCoordinator`~~ — feito na Parte 5 (P5-01).
- `[Simplificar]` `SyncTaskKind`/`SyncPartition` têm 7 valores cada (1:1), mas só `UserList` e
  `PendingChanges` existem. Avaliar reduzir/unificar sem quebrar linhas já gravadas em `sync_task_state`.
- `[Simplificar]` `ApplicationSyncTaskExecutor::Execute` é uma lambda de ~250 linhas com os dois fluxos →
  extrair uma função por partição; construtor com 10 `QString` posicionais; caminhos de query fixos.
- `[Simplificar]` `if (auditLogger) auditLogger(...)` repetido dezenas de vezes.

### Parte 7 — Biblioteca local
- ~~`[Unificar]` `composeLibraryScanner` e `composeLibraryRecognition` repetem a montagem da lista de queries
  e da factory de repositório.~~ Feito na Parte 3 (P3-04).
- `OwnedLocalFileRepository`/`OwnedLibraryScanner` (wrappers de ownership) — avaliar alternativa mais simples.
- `[Padronizar]` (vindo da Parte 3) `application/library/LocalEpisodeReader` inclui `SqlQueryStore` e usa
  `QSqlQuery`: SQL dentro da camada `application`. Mover a implementação para `infrastructure/database`.

### Parte 8 — Capas, cache e catálogo sazonal
- `[Simplificar]` `ClearLocalCacheUseCase` recebe lista de participantes sempre vazia.
- Revisão geral do coordenador de capas e do cache do catálogo sazonal.

### Parte 9 — Reconhecimento de episódios
- `[Remover]` `MediaReaderProgressReader` não é usado (produção usa `SqliteMediaProgressReader`).
- Revisar `RecognitionContext` (membros e flags), plugins mpv/Plex e serviço de settings dinâmicas.

### Parte 10 — Controllers de apresentação
- `[Simplificar]` `HomeScreenController` (921 linhas) expõe ~45 propriedades `selected*` → objeto de
  seleção/detalhes reaproveitando `MediaDetailsPresentation`.
- `[Padronizar]` Métodos públicos misturam `PascalCase` (`ConfigureScoreScale`, `SetSort`) e `camelCase`
  (`reload`, `requestNow`). Definir um padrão e aplicar.
- `[Simplificar]` Configuração pós-construção por vários `Set*`/`Configure*`.

### Parte 11 — QML e traduções
- `[Teste-ajustar]`/`[Teste-remover]` `QmlStructureTests` (~1.000 linhas, ~277 asserções) verifica em grande
  parte se o **texto** do QML contém certas strings — frágil e acoplado à implementação.
- `[Simplificar]` `Home.qml` (805 linhas) e `SettingsScreen.qml` (963 linhas): extrair componentes repetidos.

### Parte 12 — Composição e entrada
- `[Simplificar]` `main.cpp` tem ~440 linhas de fiação (contraria o `codex.md`) → mover para uma composição
  de apresentação em `src/app/`, deixando `main.cpp` só com Qt/QML.
- `[Remover]` `ApplicationContext::coverQuality` é escrito e nunca lido.
- `[Simplificar]` Lambdas `[logger](msg){ if (logger) logger->info(...) }` repetidas → helper.
- `[Doc]` Fechamento: atualizar `SERVICE.md` e `README` com o estado final, e reescrever o `codex.md` como
  `AGENTS.md` (renomeado) refletindo o programa após a limpeza.

### Parte 13 — Correções (itens fora de escopo coletados)
Única parte que **altera comportamento**. Regras próprias:
- cada correção começa por um teste que reproduz o problema (falha antes, passa depois);
- cada item tem o comportamento esperado descrito e aprovado no report antes de mudar;
- um commit `fix:` por item.

Itens coletados até agora (a lista cresce a cada parte):

| Origem | Item |
| --- | --- |
| Parte 1 | `QmlStructureTests`/`QmlRecognitionStructureTests`: `QT_QUICK_BACKEND=software` vai dentro do `PATH` e nunca é definido |
| Parte 1 | `scripts/build-release.ps1` só carrega o ambiente do MSVC quando não acha `cl.exe`; com `cl.exe` no `PATH` sem `INCLUDE`/`LIB`, o build falha (o `validate.ps1` já checa `VCToolsInstallDir`/`INCLUDE`) |
| Parte 1 | `Settings.json` usa `Q:\` como pasta padrão da biblioteca — avaliar um default neutro |
| Parte 2 | O intervalo de sincronização escolhido em Configurações (`synchronizationIntervalMs`) é salvo, mas o agendador não o usa |
| Parte 2 | Traduções em inglês faltando: estações do ano, "%1 de %2", "Episódio %1" |
| Parte 3 | Falha de migração sobrescreve o erro específico por uma mensagem genérica que cita "versions 1 through 15" |
| Parte 3 | Repositórios releem o `.sql` a cada operação e o `SqlQueryStore` loga `INFO` a cada leitura |
| Parte 4 | `SqliteMediaRepository::upsert` retorna sem `rollback` se o `prepare` falhar (transação fica aberta) |
| Parte 4 | `SqliteUserPreferencesRepository::replace` perde a mensagem de erro quando o `.sql` não pode ser lido |
| Parte 7 | `SqliteLocalFileRepository::upsertBatch`/`completeScan`/`failScan`: quando o `.sql` não pode ser lido, o erro do `SqlQueryStore` é sobrescrito por `query.lastError()` (vazio) |
| Parte 7 | `SqliteLocalEpisodeReader::readAvailableEpisodeCount` devolve `false` sem mensagem quando o `mediaId` é inválido ou a consulta não traz linha |
| Parte 8 | `CoverDownloadCoordinator` ignora sem log a falha de leitura do cache no construtor e a falha de remoção em `ReportMissingFile` |

---

## 4. Modelo do report de cada parte

```markdown
# Report — Parte N: <nome>

## 1. O que esta parte faz
Função no programa, onde entra no fluxo (quem chama / quem ela chama), threads envolvidas.

## 2. Componentes
| Arquivo | Responsabilidade | Usado por |

## 3. Testes
| Suíte | O que cobre | Avaliação (manter / ajustar / apagar — critério 2.5) |

## 4. Melhorias propostas
| ID | Categoria | Descrição | Ganho | Risco | Validação |

## 5. Fora de escopo (anotado, não corrigido)
Bugs ou riscos encontrados durante a leitura.

## 6. Decisões para o revisor
Perguntas em que há mais de um caminho razoável.
```

---

## 5. Status

| Parte | Report | Aprovação | Aplicada |
| --- | --- | --- | --- |
| Baseline | — | — | ☑ 80/85 (5 falhas de `PATH`, código verde) |
| 1 | ☑ [report](../reviews/2026-10-18-parte-01-repositorio-build.md) | ☑ | ☑ validada: `ctest` 85/85, MinGW sem prefixo, release `dist/`, smoke OK |
| 2 | ☑ [report](../reviews/2026-10-18-parte-02-dominio-midia.md) | ☑ | ☑ validada: `ctest` 85/85, smoke OK (inclui troca de idioma) |
| 3 | ☑ [report](../reviews/2026-10-18-parte-03-infraestrutura-base.md) | ☑ | ☑ validada: `ctest` verde, smoke com o banco real (migrou e funcionou) |
| 4 | ☑ [report](../reviews/2026-10-18-parte-04-persistencia-midia-outbox.md) | ☑ | ☑ validada: `ctest` verde, smoke de edição (Home e catálogo, inclusive adição) |
| 5 | ☑ [report](../reviews/2026-10-18-parte-05-integracao-anilist.md) | ☑ | ☑ validada: `ctest` verde, smoke de sync (lista baixada, edição enviada) |
| 6 | ☑ [report](../reviews/2026-10-18-parte-06-agendamento-sync.md) | ☑ | ☑ validada: `ctest` verde (após `ac42360`) |
| 7 | ☑ [report](../reviews/2026-10-18-parte-07-biblioteca-local.md) | ☑ | ☑ validada: `ctest` verde |
| 8 | ☑ [report](../reviews/2026-10-18-parte-08-capas-cache-catalogo.md) | ☑ | ☐ aplicada, aguardando build |
| 9 | ☐ | ☐ | ☐ |
| 10 | ☐ | ☐ | ☐ |
| 11 | ☐ | ☐ | ☐ |
| 12 | ☐ | ☐ | ☐ |
| 13 | ☐ | ☐ | ☐ |
