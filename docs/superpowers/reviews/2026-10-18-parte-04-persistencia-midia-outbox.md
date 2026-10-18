# Report — Parte 4: Persistência de mídia e outbox

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `8daf391` (`feature/revisao-geral`)
- **Status:** aguardando aprovação — nenhuma alteração aplicada

---

## 1. O que esta parte faz

É onde a biblioteca e as edições do usuário são gravadas no SQLite:

- **`SqliteMediaRepository`** (implementa `IMediaRepository` = leitura + *upsert* + atualização da lista
  pessoal + conciliação de snapshot):
  - `readAll` lê todas as mídias (usado pela Home, catálogo sazonal, reconhecimento);
  - `upsert` grava as mídias vindas da sync numa transação. Os **nomes alternativos** existentes são
    mesclados com os da fonte (nunca apagados), e os campos do usuário são preservados pela própria query;
  - `reconcileAuthoritativeSnapshot` marca como "removidas da fonte" as mídias ativas que não vieram num
    snapshot completo da lista do AniList, exceto as que têm mudança pendente.
- **`SqlitePendingChangeRepository`** é o **outbox** do AniList (`anilist_pending_changes`). Ele enfileira,
  lê por mídia e por dono (`ownerUserId`), adota mudanças sem dono e atualiza status/tentativas. Os valores
  (`int` ou texto) são gravados como JSON `{"type", "value"}`.
- **`SqlitePersonalListChangeWriter`** grava **numa transação** a edição local da mídia (atualiza, ou insere
  se ainda não existe), cancela uma remoção pendente (re-adição) e enfileira as mudanças no outbox. A remoção
  marca `personal_list_removed_at` e enfileira a deleção.
- **`PersonalListChangeService`** (application) transforma "mídia antes / mídia depois" em mudanças do
  outbox (progresso, nota, status em formato AniList), carimba o dono e avisa o runtime de sync.
- **`SeasonalPersonalListService`** é usado pelo catálogo sazonal: localiza a mídia na biblioteca e salva
  pela mesma via (cria a entrada se ela ainda não estiver na lista).
- **`SqliteUserPreferencesRepository`** lê e grava a linha única de `user_preferences`, validando,
  normalizando extensões, convertendo a pasta antiga (`library_root`) para a lista (`library_roots`) e
  avisando quando o título preferido gravado não é suportado.

Tudo roda na thread da GUI (conexão principal), exceto o `upsert`/conciliação/outbox usados pela sync, que
rodam no job de sync com conexão própria (Parte 6).

---

## 2. Componentes

| Arquivo | Linhas | Responsabilidade | Observação |
| --- | --- | --- | --- |
| `SqliteMediaRepository.cpp` | 262 | mídia: ler, *upsert*, editar lista, conciliar | tem 1 SQL inline; `updatePersonalListMedia` só é usado no caminho sem outbox |
| `SqliteMediaMapper.cpp` | 48 | linha SQL → `Media` | parse de nomes/links |
| `SqlitePendingChangeRepository.cpp` | 222 | outbox | 2 construtores; 2 queries como membro com default |
| `SqlitePersonalListChangeWriter.cpp` | 254 | edição + outbox numa transação | **repete** o *bind* do *upsert* (24 campos), da atualização da lista, a codificação do outbox e a serialização de links/nomes |
| `SqliteUserPreferencesRepository.cpp` | 168 | preferências | 3 cópias do parse "array JSON de strings" |
| `PersonalListChangeService.cpp` | 82 | edição → mudanças do outbox | ok |
| `SeasonalPersonalListService.cpp` | 96 | edição a partir do catálogo | caminho alternativo sem outbox |
| ports `IMediaReader/Writer/SnapshotReconciler/PersonalListMediaWriter`, `IMediaRepository`, `IPersonalListChangeWriter`, `IPendingChangeRepository`, `IUserPreferencesRepository` | — | contratos | `IPersonalListMediaWriter` só serve ao caminho sem outbox |

**O caminho "sem outbox".** `HomeScreenController::SaveSelectedMediaFromEditor` e
`SeasonalPersonalListService::save` têm dois caminhos: com `PersonalListChangeService` (grava e enfileira para
o AniList) e, se ele for nulo, um alternativo que grava direto (`updatePersonalListMedia`/`upsert`) **sem
enfileirar nada**. Na composição, o serviço nunca é nulo quando o repositório existe. O alternativo só é
exercitado pelos testes (8 na Home, 9 no catálogo sazonal).

---

## 3. Testes

| Suíte | O que cobre | Avaliação |
| --- | --- | --- |
| `SqliteRepositoryTests` (27) | *round-trip* de mídia, preservação de campos do usuário e de nomes alternativos, outbox (dono, adoção, valores inválidos, status), conciliação de snapshot, remoção/re-adição | **manter**; construtores ficam mais curtos com P4-04 |
| └ `personalListUpdatePersistsEditorFieldsWithoutOverwritingCatalogMetadata`, `editorUpdateStillReplacesAndRemovesAlternativeNames` | `updatePersonalListMedia` do repositório | **ajustar** com P4-01: as mesmas asserções passam a usar `SqlitePersonalListChangeWriter::save` (mesma query e mesmo *bind*) |
| `SqliteUserPreferencesRepositoryTests` (14) | *round-trip*, valores inválidos, migração de pasta única, fallbacks | **manter** |
| `PersonalListChangeServiceTests` (7) | mudanças geradas, dono, aviso após gravar, remoção | **manter** |
| `HomeScreenControllerTests` — 8 casos com `SetPersonalListMediaWriter` | edição pela Home | **ajustar** com P4-01: usar `PersonalListChangeService` sobre um `IPersonalListChangeWriter` falso (as asserções sobre a mídia salva continuam iguais e passam a verificar também o outbox) |
| `SeasonalCatalogControllerTests` — 9 casos com `SeasonalPersonalListService(&repo, &repo, &repo)` | edição pelo catálogo | **ajustar** com P4-01, da mesma forma |

Nenhum teste desta parte cai nos critérios de remoção.

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P4-01** | `[Remover]` | Remover o caminho "sem outbox": `HomeScreenController::SetPersonalListMediaWriter` e o ramo alternativo; em `SeasonalPersonalListService`, os parâmetros `IMediaWriter`/`IPersonalListMediaWriter` e seus ramos; `IPersonalListMediaWriter` e `SqliteMediaRepository::updatePersonalListMedia`. O `PersonalListChangeService` passa a ser obrigatório (referência) nos dois. Testes ajustados conforme a seção 3. | um único caminho de gravação; −~60 linhas de produção | médio (muitos testes mudam) | `HomeScreenControllerTests`, `SeasonalCatalogControllerTests`, `SqliteRepositoryTests` + smoke editando pela Home e pelo catálogo |
| **P4-02** | `[Unificar]` | Concentrar no `SqliteMediaMapper` o *bind* do *upsert* de mídia, o *bind* da atualização da lista pessoal e a (de)serialização de nomes e links. Hoje estão escritos 2× (repositório e writer). A diferença intencional continua: o repositório mescla os nomes alternativos com os existentes; o writer grava os do editor. | −~70 linhas duplicadas; um campo novo de mídia é adicionado num lugar só | baixo | `SqliteRepositoryTests` |
| **P4-03** | `[Unificar]` | Codificação dos valores do outbox (`EncodeValue`/`DecodeValue`) e o *bind* de uma mudança pendente, hoje duplicados entre `SqlitePendingChangeRepository::enqueue` e o writer, num único lugar (helper interno de `infrastructure/database`). | −~40 linhas | baixo | `SqliteRepositoryTests` |
| **P4-04** | `[Simplificar]` | Cada repositório desta parte usa as próprias constantes de `SqliteQueries.h` em vez de receber caminhos no construtor: `SqliteMediaRepository(db)`, `SqlitePendingChangeRepository(db)` (um construtor só), `SqlitePersonalListChangeWriter(db)`, `SqliteUserPreferencesRepository(db)`. Todos os chamadores (composição, executor de sync, testes) passam hoje exatamente esses caminhos. | construtores de 3 a 6 caminhos → 1 parâmetro | baixo | build + `ctest` |
| **P4-05** | `[Padronizar]` | O SQL inline `SELECT alternative_names FROM media WHERE id = :id` do *upsert* vai para `read-media-alternative-names.sql` (regra do projeto: SQL fora do C++). | consistência | baixo | `SqliteRepositoryTests` |
| **P4-06** | `[Simplificar]` | `SqliteUserPreferencesRepository::read`: as três leituras de "array JSON de strings" (pastas, extensões, listas) viram um helper, mantendo as mensagens de erro de cada uma. | −~25 linhas | baixo | `SqliteUserPreferencesRepositoryTests` |

### Anotado para outras partes
- `PersonalListChangeService` converte `UserListStatus` → status do AniList (`CURRENT`, `PAUSED`, …). O mesmo
  mapeamento existe em `MediaSyncFilter` e, provavelmente, no mapper do AniList (Parte 5).
- `persistHomeSortPreference`/`persistLastAniListUserId` (`main.cpp`) repetem "copiar preferências → alterar →
  `replace`" (Parte 12).

### Ordem sugerida
P4-05 → P4-04 → P4-02 → P4-03 → P4-06 → P4-01 (o de maior impacto nos testes por último).

---

## 5. Fora de escopo (vai para a Parte 13)

- **Transação aberta no `upsert` de mídia:** se o `prepare` da query de *upsert* falhar, o método retorna
  sem `rollback`, deixando a transação aberta na conexão.
- **Erro perdido ao gravar preferências:** em `SqliteUserPreferencesRepository::replace`, se o arquivo `.sql`
  não puder ser lido, a mensagem do `SqlQueryStore` é sobrescrita por `query.lastError()` (vazia) e o método
  falha sem dizer por quê.

---

## 6. Decisões para o revisor

1. **P4-01:** remover o caminho de gravação sem outbox (recomendo **sim**). Toda edição do usuário passa a
   ir, obrigatoriamente, para o outbox, como já acontece em produção hoje.
