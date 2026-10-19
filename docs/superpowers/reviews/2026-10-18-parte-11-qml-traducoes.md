# Report — Parte 11: QML e traduções

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `921c631` (`feature/revisao-geral`)
- **Status:** aprovado (com as duas recomendações) e aplicado — validado (P11-05 movido para a Parte 13)

---

## 1. O que esta parte faz

- **QML** (`resources/qml`, módulo `HaikenAnime`, 19 arquivos, ~4,6k linhas):
  - `Main.qml` alterna Home, catálogo sazonal, Configurações e "Assistindo agora";
  - as telas (`Home`, `SeasonalCatalogScreen`, `SettingsScreen`, `NowWatchingScreen`,
    `EpisodeRecognitionScreen`, `PluginSettingsScreen`) só ligam propriedades e ações dos controllers da
    Parte 10;
  - os componentes compartilhados são `MediaCard`, `MediaDetailsPanel`, `EditMediaPanel`, `CoverPreview`,
    `BrowseControls`, `StatePanel` e `SyncProgressBar`;
  - os controles estilizados `App*` (botão, campo, *spin box*, *combo*) substituem os controles nativos do
    Windows.
- **Traduções**: os textos-fonte estão em português (`sourcelanguage="pt_BR"`). `HaikenAnime_en.ts` traz o
  inglês e `HaikenAnime_pt_BR.ts` a identidade. Os `.ts` são gerados pelo alvo `update_translations`
  (`lupdate -no-obsolete`), e o `TranslationLoader` carrega o catálogo do idioma escolhido antes do QML.

---

## 2. Componentes e achados

| Componente | Achado |
| --- | --- |
| Paleta de cores | as mesmas 8 cores (`ink`, `muted`, `line`, `surface`, `surfaceSoft`, `accent`, `accentSoft`, `danger`) estão **copiadas em 17 arquivos** (~90 declarações). Além disso, Home, catálogo e "Assistindo agora" as repassam aos filhos linha a linha (`ink: home.ink`, `muted: home.muted`… — 23 linhas). Nenhum chamador passa um valor diferente |
| `AppButton.qml` | `isPrimary` declarado e nunca lido |
| `EpisodeRecognitionScreen.qml` | o *delegate* declara `required property` `userId` e `diagnostic` e não usa nenhuma das duas |
| `Home.qml` (805), `SettingsScreen.qml` (963) | depois da paleta, não sobra repetição relevante: os blocos iguais restantes são o rótulo "HAIKEN ANIME" (3×) e o cabeçalho de *popup* (2–3×), poucas linhas cada. A extração sugerida no plano não compensa |
| `ids` sem referência | ~40 (`homeHeaderTitle`, `settingsSaveButton`…). Vários são buscados pelos testes de texto; são inofensivos e ficam |
| Traduções | `HaikenAnime_en.ts` tem **86 entradas `unfinished`** (o item "traduções em inglês faltando" da Parte 13 é maior do que o listado). Os `.ts` ainda guardam textos que saíram do código nas Partes 1–10 (por exemplo, "A limpeza do cache local foi cancelada.") porque não foram regenerados |

---

## 3. Testes

`QmlStructureTests` (35 casos) e `QmlRecognitionStructureTests` (13) somam 48 casos. **Só 5 carregam o QML**;
os outros 43 leem o arquivo `.qml` como texto e verificam se ele contém trechos exatos
(`controller.cacheCleanupRunning`, `"#02A9FF"`, `width: 1440`…). Dos 43, 7 verificam **regras gerais do
projeto** sobre todos os arquivos; os outros 36 são retratos do texto de uma tela.

| Grupo | Casos | Avaliação |
| --- | --- | --- |
| Carregam o QML | `scanExtensionGridReachesAllBreakpointsAndPreservesInteractions`, `languageSelectionUsesBackendOptionsAndStartupInstallsBeforeQml`, `pluginSettingsScreenLoadsWithController`, `recognitionScreenLoadsWithController`, `nowWatchingScreenLoadsWithController` | **manter** |
| Regras gerais (lint) | `implicitSignalParametersAreDeclared`, `nativeButtonsAreNotUsed`, `nativeInputsAreNotUsed`, `qmlHasNoTimersNetworkOrPlayerParsing`, `qmlNeverDisplaysSecretsOrUsesNativeButtons`, `secretInputIsPasswordAndStartsEmpty`, `recognitionTextsDoNotNamePlugins` | **manter** |
| Duplicado | `scanExtensionGridUsesItsAvailableWidth` (o caso que carrega a grade verifica as mesmas colunas de verdade) | **apagar** (critério **b**) |
| Retratos de texto | os outros 35: 29 de `QmlStructureTests` e 6 de `QmlRecognitionStructureTests` (lista na seção 7 ao aplicar) | ver decisão 1 |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P11-01** | `[Remover]` | `AppButton.isPrimary` e as `required property` `userId`/`diagnostic` sem uso | limpeza | baixo | build + `QmlRecognitionStructureTests` |
| **P11-02** | `[Unificar]` | `Theme.qml` como *singleton* QML (`pragma Singleton`, registrado com `QT_QML_SINGLETON_TYPE`) com as 8 cores. Os 17 arquivos passam a usar `Theme.ink` etc., e somem as cópias e o repasse entre pai e filho. As cores próprias de um componente (por exemplo, o azul do AniList na barra de sync) ficam onde estão | −~110 linhas, uma paleta só | médio (visual) | build + smoke visual de todas as telas |
| **P11-03** | `[Teste-adicionar]` | **Teste de caracterização, antes dos itens acima:** `QmlScreensLoadTests` carrega Home, catálogo e Configurações com os controllers reais sobre *fakes* e usa `QTest::failOnWarning`, de modo que qualquer *binding* quebrado ou `ReferenceError` falhe. Cobre o que os retratos de texto tentavam proteger | rede de segurança real | baixo | o próprio teste |
| **P11-04** | `[Teste-remover]` | Conforme a decisão 1 | — | — | — |
| **P11-05** | `[Doc]` | Regenerar os `.ts` (alvo `update_translations`, rodado pelo revisor) no fim da parte, para tirar os textos que saíram do código | `.ts` fiéis ao código | baixo | `TranslationLoaderTests` + smoke em inglês |

### Ordem sugerida
P11-03 (teste novo, commit próprio) → P11-01 → P11-04 → P11-02 → P11-05.

---

## 5. Fora de escopo (vai para a Parte 13)

| Item |
| --- |
| O item já registrado de traduções em inglês passa a citar o número real: 86 entradas `unfinished` em `HaikenAnime_en.ts` |

---

## 6. Decisões para o revisor

1. **Retratos de texto do QML**: recomendo **apagar os 35** (critério **c**: verificam o texto do código, e o
   comportamento passa a ser coberto pelo P11-03, que carrega as telas e falha em qualquer *binding*
   quebrado, e pelo smoke). Eles quebram a cada refatoração legítima: o P11-02 sozinho invalidaria
   `appButtonVariantsExist`, `syncProgressBarUsesAniListColor` e vários outros. A alternativa é mantê-los e
   ajustá-los aos novos textos a cada mudança.
2. **P11-02 (Theme singleton)**: recomendo fazer. A alternativa é deixar a paleta copiada.

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| P11-03 `QmlScreensLoadTests` (antes das mudanças) | `9f5097e` |
| P11-01 propriedades QML sem uso | `ee7ddb0` |
| P11-04 retratos de texto e o duplicado | `64cffc5` |
| P11-02 paleta única (`Theme.qml`) | `d552bbf` |
| P11-05 `.ts` regenerados | movido para a Parte 13, junto com o preenchimento do inglês |

### Mudança na aplicação do P11-02: componente em vez de *singleton*
Um *singleton* QML só é reconhecido com uma entrada `singleton` no `qmldir`, e esse arquivo só existe no
módulo gerado pelo build. Os testes de QML carregam os arquivos direto de `resources/qml`, onde o
`Theme.ink` de um *singleton* não resolveria. `Theme.qml` virou um `QtObject` comum, e cada arquivo cria a
própria instância (`readonly property Theme theme: Theme {}`, uma linha). O objetivo se mantém: a paleta
existe em um lugar só e o repasse pai → filho sumiu (−~120 linhas em 15 arquivos). Os valores são os mesmos
de antes. A troca foi feita por script, só em código QML (strings e comentários intactos).

### Testes apagados (P11-04)
- Duplicado (critério **b**): `scanExtensionGridUsesItsAvailableWidth`.
- Retratos de texto (critério **c**), em `QmlStructureTests`: `settingsModeUsesNarrowMinimumWidth`,
  `browseControlsShowConfiguredLabelsAfterInitialization`, `homeRestoresListFilterSelectionAfterReturningFromSettings`,
  `homeErrorDetailStaysWithSynchronizationStatus`, `screensReadSynchronizationFromDedicatedStatusController`,
  `mediaCardAndSettingsUseControllerPreparedCardPresentation`, `previewCardsUseControllerPreparedMetadataInBothGrids`,
  `compactDetailsUseSeparatePreviewMetadata`, `compactDetailsAreReadOnlyAndSelectable`,
  `browseControlsClearFocusWithoutResettingCriteria`, `completeLibraryStaysOpenOutsideExplicitCloseAction`,
  `externalLinksUseVisibleInteractiveButtonStates`, `completeLibraryAllowsInteractionWithDetailsActions`,
  `coverPreviewReusesSelectedCoverSourceWithoutRequestingDownloads`, `preferredTitleSelectionUsesBackendOptions`,
  `seasonalCatalogRequiresExplicitFiltersAndHasResponsiveContent`, `seasonalCatalogPublishesCardsOnlyAfterResultsAreReady`,
  `secondaryScreenNavigationMatchesLibraryHeaderPlacement`, `settingsHeaderReservesMessageSpaceAfterSaveAction`,
  `settingsAccountSectionUsesAniListAuthenticationControllerState`,
  `seasonalDetailsKeepAddAndEditFlowsExplicitAndStatusGated`, `homeEditorOffersConfirmedRemovalFromPersonalList`,
  `mainKeepsSeasonalCatalogAsSeparateNavigation`, `localCacheCleanupStaysInLibrarySettingsAndUsesControllerState`,
  `settingsListsEveryLibraryFolder`, `settingsHasNoDownloadsSection`, `appButtonVariantsExist`,
  `syncProgressBarUsesAniListColor`, `mainWindowStartsMaximized`; em `QmlRecognitionStructureTests`:
  `recognitionScreenIsNotAConfirmationQueue`, `settingsScreenHostsPluginsAndHistory`,
  `nowWatchingScreenShowsEmptyMessageAndUnassociatedLabel`, `homeButtonIsRightOfSettingsAndGuarded`,
  `mainHasNowWatchingPage`, `nowWatchingUsesHomeLayoutAndAnimeDetailsButton`.
- Ficaram 12: os 5 que carregam QML e as 7 regras gerais (lista na seção 3).

### Nota sobre o `QmlScreensLoadTests`
O teste foi escrito antes das mudanças, mas o primeiro build é o deste conjunto. Se ele falhar, o aviso de
QML apontado (arquivo e linha na mensagem) pode já existir hoje; nesse caso o corrijo antes de seguir, sem
afrouxar o teste.

### Correção após o primeiro build
- `QmlScreensLoadTests` falhou só porque a imagem `cover-placeholder.svg` existe no módulo QML do app, mas
  não no executável de teste (`Cannot open: qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg`).
  A imagem foi adicionada ao teste no mesmo caminho (`24f6e36`). Nenhum *binding* estava quebrado.

### Validação final (build do revisor)
- `scripts/validate.ps1`: build MSVC + `ctest` 81/81 (após `24f6e36`); smoke visual OK.
- O revisor observou que só parte da UI aparece em inglês. A causa são as entradas não traduzidas no `.ts`,
  não o código. A regeneração (P11-05) e o preenchimento ficaram juntos na Parte 13.
