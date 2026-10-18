# Report — Parte 2: Domínio e regras de mídia

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `cc91cf6` (`feature/revisao-geral`)
- **Status:** aguardando aprovação — nenhuma alteração aplicada

---

## 1. O que esta parte faz

É o **vocabulário do programa**: os tipos que todas as outras camadas trocam entre si e as regras puras
sobre mídia e preferências.

- **`src/domain/media`** — a entidade `Media` (uma obra da lista: títulos, nomes alternativos, progresso,
  notas, capas, temporada, próximo episódio, links, caminho local, status) e seus enums `MediaType`
  (Anime/Manga/Novel), `MediaStatus` (lançamento da obra) e `UserListStatus` (lista pessoal: assistindo,
  planejando, em pausa, abandonado, concluído).
- **`src/domain/anilist`** — campos (`AniListField`), valores (`AniListFieldValue`) e o resultado/política de
  merge entre o valor local e o remoto. É usado pelo outbox e pela conciliação com o AniList (Parte 5).
- **`src/domain/library`** — registros de arquivo local observados no scan e reconhecidos (Parte 7).
- **`src/application/media`** (parte genérica):
  - `MediaTitleResolver`: escolhe o título exibido (romaji/english/native) com fallback determinístico;
  - `AlternativeNames`: junta nomes alternativos da fonte aos já salvos, sem duplicar (usado no *upsert*);
  - `ProgressFormatter` e `MediaDetailsPresentation`: textos prontos para a tela (progresso `3/?`,
    temporada, próximo episódio, links, mapa `selected*` do painel de detalhes);
  - ports do repositório de mídia (`IMediaReader`/`Writer`/…), revisados na Parte 4, e
    `MediaSyncFilter`/`MediaPage`/`IMediaDataSource`, revisados na Parte 5.
- **`src/application/configuration`** — `Settings` (o que vem do `Settings.json`), `UserPreferences` (o que o
  usuário altera, salvo no banco), chaves de idioma e de apresentação do card, e `UserPreferencesValidator`
  (escala de nota, intervalo de sync, pastas da biblioteca sem duplicar ou aninhar, extensões normalizadas,
  listas habilitadas).

Ninguém aqui roda em thread própria, acessa banco ou rede: são tipos e funções puras. Exceção: os
helpers de apresentação, que usam `QCoreApplication::translate` e `QVariantMap`.

---

## 2. Componentes

| Arquivo | Responsabilidade | Usado por |
| --- | --- | --- |
| `domain/media/Media.h` (+ `MediaLink`, `MediaType`, `MediaStatus`, `UserListStatus`) | entidade e enums | todas as camadas |
| `domain/anilist/*` | campos/valores/políticas de merge | outbox e conciliação (Parte 5) |
| `domain/library/LocalFileRecord.h` | registros de arquivos locais | biblioteca local (Parte 7) |
| `application/media/MediaTitleResolver` | título preferido + validação da chave | Home, catálogo, Assistindo agora, Settings, validador, repositório de preferências |
| `application/media/AlternativeNames` | merge de nomes alternativos | `SqliteMediaRepository` |
| `application/media/ProgressFormatter` | `"consumido/total"` com `?` | só controllers de apresentação |
| `application/media/MediaDetailsPresentation` | rótulos e mapa do painel de detalhes | só controllers de apresentação |
| `application/configuration/Settings.h` (+ `AniListSettings`, `HttpSettings`) | configuração do arquivo | composição e leitor JSON |
| `application/configuration/ISettingsReader.h` | interface do leitor de configuração | **só** `JsonSettingsReader` a implementa; ninguém depende dela |
| `application/configuration/UserPreferences.h` | preferências do usuário | Settings, composição, repositório |
| `application/configuration/UserPreferencesValidator` | validação das preferências | `SettingsController`, repositório |
| `CardStatusPresentation.h`, `LanguagePreference.h` | chaves ↔ enum/idioma | Settings, Home, tradução |

---

## 3. Testes

| Suíte / caso | O que cobre | Avaliação |
| --- | --- | --- |
| `MediaDomainTests::formatProgress*` (3 casos) | `FormatProgress` (total conhecido, `?`, negativo) | **manter**, movendo para a suíte do novo helper de apresentação (P2-03) |
| `MediaDomainTests::defaultMediaHasSafeNeutralValues` | valores default dos campos de `Media` | **apagar** (critério **d**: default de struct, sem regra) |
| `AlternativeNamesTests` (7) | ordem, duplicados por caixa/espaço, vazios, nomes não latinos | **manter** |
| `MediaTitleResolverTests` (3) | preferência + fallback, não altera a fonte | **manter** |
| `UserPreferencesValidatorTests` — casos de nota, intervalo, pastas, extensões, listas, idioma, título | regras de validação | **manter** |
| `UserPreferencesValidatorTests::comparesScannerPreferences` | o `operator==` manual compara pastas e extensões | **apagar** junto com o P2-04 (critério **d**: depois disso a comparação é gerada pelo compilador) |
| `UserPreferencesValidatorTests::usesExactScannerDefaults` | repete a lista literal de defaults e verifica que os defaults são válidos | **ajustar**: manter só "os defaults são válidos" (a lista literal é o critério **d**) |
| `rejectsUnsupportedCardStatusPresentation`, linha "unsupported quality" | enum fora do intervalo via `static_cast` | manter (cobrem o ramo defensivo do validador) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P2-01** | `[Unificar]` | A conversão `UserListStatus` ↔ chave (`current`, `planning`, `on_hold`, `dropped`, `completed`) e a lista de chaves suportadas estão escritas **7 vezes em 5 arquivos**: `HomeScreenController` (nos dois sentidos), `SeasonalCatalogController` (nos dois sentidos), `SettingsController::SetUserListEnabled`, `UserPreferencesValidator` e os defaults de `UserPreferences`. Proposta: `UserListStatusKey()`, `UserListStatusFromKey()` e `UserListKeys()` em `domain/media/UserListStatus.h`, no mesmo padrão de `CardStatusPresentation.h`, e todos passam a usá-las. Os **rótulos** de cada tela continuam diferentes (são textos de UX distintos: "Assistindo"/"Em andamento"…). | uma fonte da verdade para as chaves | baixo | `HomeScreenControllerTests`, `SeasonalCatalogControllerTests`, `SettingsControllerTests`, `UserPreferencesValidatorTests` |
| **P2-02** | `[Unificar]` | Os rótulos de `MediaStatus` ("Ainda não lançado", "Em lançamento", "Concluído", "Desconhecido") e de `MediaType` estão em 3 lugares: `MediaDetailsPresentation`, `HomeScreenController` e `SeasonalCatalogController`. Proposta: `MediaStatusLabel()`/`MediaTypeLabel()` num único helper, com o contexto de tradução `HomeMediaModel`. Conferido no `.ts`: o inglês é idêntico nos dois contextos, então o texto exibido não muda em nenhum idioma. As entradas do contexto `SeasonalCatalogController` ficam obsoletas e saem na próxima atualização do `.ts`. | −~40 linhas duplicadas | baixo | testes dos controllers + troca de idioma no smoke |
| **P2-03** | `[Simplificar]` | Mover `ProgressFormatter` e `MediaDetailsPresentation` de `application/media` para `src/presentation/media/MediaPresentation.{h,cpp}` (junto com os helpers do P2-02). Eles produzem `QVariantMap` e textos traduzidos para a tela e só são usados por controllers, então pertencem à apresentação. Os testes de `FormatProgress` vão para `MediaPresentationTests`, e `MediaDomainTests` deixa de existir. | camadas mais honestas; menos arquivos minúsculos | baixo | build + mesmas asserções de `FormatProgress` |
| **P2-04** | `[Simplificar]` | `operator==` de `UserPreferences` escrito à mão (16 campos) → `bool operator==(const UserPreferences &) const = default;`. Mesma semântica: compara todos os membros em ordem, e quem adicionar um campo não precisa mais lembrar de atualizar a comparação. | −17 linhas; elimina uma armadilha | baixo | `SettingsControllerTests` (usa a comparação para *dirty*) |
| **P2-05** | `[Remover]` | Cabeçalhos "Created by gutocbs" gerados pela IDE em `Media.h`, `MediaStatus.h`, `MediaType.h`. | ruído | baixo | build |
| **P2-06** | `[Remover]` | `ISettingsReader`: interface sem nenhum consumidor. A composição usa `JsonSettingsReader` diretamente, e não há *double* nos testes. | menos indireção | baixo | build |
| **P2-07** | `[Teste-remover]`/`[Teste-ajustar]` | Os ajustes de teste da seção 3: apagar `defaultMediaHasSafeNeutralValues` e `comparesScannerPreferences`; reduzir `usesExactScannerDefaults` a "defaults são válidos". | testes focados em regra | baixo | `ctest` |

### Deixado para outras partes (anotado aqui porque o arquivo é desta camada)
- `Settings::syncIntervalMs`: só o `InitialSyncCoordinator` usa → sai junto com ele (Parte 6).
- `AniListSettings::mediaQueryFile`: sai junto com `media-page.graphql` (Parte 5).
- `MediaSyncFilter` traz o mapeamento das chaves de lista para os status do AniList (`on_hold` → `PAUSED`)
  dentro de `application`, contrariando a regra de manter nomes do AniList nos adapters (Parte 5).
- `AniListField` tem valores (`Title`, `CoverUrl`, `Synopsis`, `AlternativeTitles`, `ExternalId`,
  `LocalFiles`) que só aparecem no resolver de políticas e nunca entram no outbox (Parte 5).

### Ordem sugerida
P2-05 → P2-06 → P2-04 (+ teste) → P2-01 → P2-02 + P2-03 (juntos, porque criam o mesmo arquivo) → P2-07.

---

## 5. Fora de escopo (anotado, não corrigido)

- **Intervalo de sincronização ignorado:** Configurações permite escolher o intervalo
  (`UserPreferences::synchronizationIntervalMs`), que é validado e salvo, mas o agendador não o lê. Os
  intervalos vêm das políticas do `Settings.json`/defaults (`UserList` = 1 h). Hoje a opção não tem efeito.
- **Traduções faltando em inglês:** "Inverno", "Primavera", "Verão", "Outono", "%1 de %2" e "Episódio %1"
  estão sem tradução no `HaikenAnime_en.ts` (*unfinished*), então aparecem em português com o app em inglês.

---

## 6. Decisões para o revisor

1. **`Media` com campos públicos `PascalCase`** (o `codex.md` desaconselha entidade sem invariantes):
   recomendo **manter**. Renomear ou encapsular toca praticamente todos os arquivos do projeto, sem ganho
   de comportamento. A convenção seria registrada no `SERVICE.md`. Alternativa: encapsular aos poucos,
   numa iniciativa própria fora desta revisão.
2. **P2-06:** remover `ISettingsReader` (recomendo) ou mantê-la como contrato formal da camada?
3. **P2-03:** de acordo com criar `src/presentation/media/` para os helpers de texto de mídia?
