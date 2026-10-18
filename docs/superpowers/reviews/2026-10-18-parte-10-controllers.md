# Report — Parte 10: Controllers de apresentação

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `06992a8` (`feature/revisao-geral`)
- **Status:** aprovado (com a recomendação) e aplicado — aguardando build do revisor

---

## 1. O que esta parte faz

São os `QObject`/`QAbstractListModel` que o QML usa. Eles não guardam regra de negócio: leem dos serviços,
formatam com `MediaPresentation`/`MediaTitleResolver` e repassam as ações.

- **`HomeScreenController`** + `HomeMediaModel`:
  - carrega a biblioteca e mantém dois modelos, a prévia de 9 itens e a lista completa filtrada;
  - aplica tipo de mídia, lista, ordenação e busca;
  - mantém a seleção e seus detalhes (`selected*`);
  - salva pelo editor via `PersonalListChangeService` e pede o reconhecimento local quando pasta ou nomes
    mudam;
  - mostra o próximo episódio local e abre o arquivo;
  - pede janelas de capas ao `CoverDownloadCoordinator` e reage a capa disponível, ausente ou limpa.
- **`SeasonalCatalogController`** + `SeasonalCatalogMediaModel`: filtros de ano/estação, paginação, seleção e
  inclusão/edição na lista pessoal (`SeasonalPersonalListService`).
- **`SettingsController`**: rascunho e gravação das preferências; pastas e extensões da biblioteca com scan
  manual; limpeza de cache; conexão, *callback* OAuth e remoção da conta AniList.
- **`PluginSettingsController`**: formulário genérico pelo `PluginSettingsSchema`, com conexão Plex
  (servidores e usuários) e teste de conexão.
- **`SynchronizationStatusController`**: barra de progresso da sync. Junta as tarefas de uma "rodada", só
  mostra o resultado quando nada mais roda e esconde a barra depois de 4 s.
- **`NowWatchingController`** e **`EpisodeRecognitionController`** + `RecognitionListModel`: telas "Assistindo
  agora" e histórico de reconhecimentos (paginação e remoção).

---

## 2. Componentes e achados

| Componente | Achado |
| --- | --- |
| `HomeScreenController` | `PreviewCardMetadata` e `PreviewCompactDetailMetadata` (`Q_INVOKABLE`) **não são chamados pelo QML** (só por testes); a sobrecarga de 3 argumentos de `cardStatusLabel` existe só para eles. `ClearCoverCache()` não tem nenhum chamador (a limpeza vem do `SettingsController`) |
| `Q_PROPERTY` que o QML não lê | Home: `mediaCount`, `nextLocalEpisode`; NowWatching: `selectedSessionKey`; Settings: `aniListAuthenticationState`; SyncStatus: `stageKey`, `stageIndex`, `stageCount`, `totalItems` (são insumos de `stageLabel`/`percent`). Os testes leem os *getters* para verificar estado |
| `Q_INVOKABLE` que o QML não chama | `SeasonalCatalogController::SaveSelectedToPersonalList` (o QML usa a versão `…FromEditor`, que a chama); `SettingsController::HandleAniListOAuthCallback` (só o `main.cpp` chama) |
| Nomes de método | o `company.md` pede **PascalCase** para métodos. Hoje os *getters* de propriedade e os sinais seguem o padrão Qt (camelCase), e quase todo o resto já é PascalCase. Exceções: `HomeScreenController::reload`, `HomeMediaModel::setMedia` (o mesmo modelo tem `UpdateCover`/`ClearCovers`), `SeasonalCatalogMediaModel::setMedia`/`configurePresentation`, `NowWatchingController::setPreferredTitleKey`/`setCoverSource` (a classe tem `Refresh`/`InvalidateLibrary`), `RecognitionListModel::setRecords`/`appendRecords` |
| `selected*` (Home ~25, catálogo ~22) | o plano sugeria um objeto de seleção. Cada *getter* já é uma linha que delega a `MediaPresentation`, então o objeto só **moveria** essas linhas e reescreveria ~45 *bindings* no QML. **Não recomendo** (ver decisões) |
| `Configure*`/`Set*` após a construção | o plano sugeria movê-los para o construtor, mas o `main.cpp` os **reaplica em tempo de execução** quando as preferências mudam (`preferencesSaved`). Ficam como estão |
| `SettingsController`, `PluginSettingsController`, `SynchronizationStatusController`, `EpisodeRecognitionController`, `MediaPresentation` | ok |

---

## 3. Testes

| Suíte | Avaliação |
| --- | --- |
| `HomeScreenControllerTests` (56) | **apagar** `preparesPreviewCardMetadataForBothStatusPresentations`, `preparesCompactDetailPreviewMetadataSeparatelyFromCards` e `preparesCompactDetailPreviewProgressWithZeroTotal` (critério **a**); **ajustar** o caso de total desconhecido (`3/?`), que perde as 2 verificações de `Preview*` e mantém as de `selectedProgress` e do modelo; o resto **fica** (só renomeia `reload` → `Reload`) |
| `SeasonalCatalogControllerTests` (19), `SettingsControllerTests` (31), `PluginSettingsControllerTests` (16), `SynchronizationStatusControllerTests` (31), `NowWatchingControllerTests` (22), `EpisodeRecognitionControllerTests` (6), `MediaPresentationTests` (3) | **manter** (ajustes mecânicos de nome do P10-03) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P10-01** | `[Remover]` | `HomeScreenController::PreviewCardMetadata`, `PreviewCompactDetailMetadata` (com a sobrecarga de `cardStatusLabel`) e `ClearCoverCache` | −~45 linhas | baixo | `HomeScreenControllerTests` |
| **P10-02** | `[Simplificar]` | Tirar do QML o que ele não usa: as 8 `Q_PROPERTY` acima viram *getters* C++ comuns, e os 2 `Q_INVOKABLE` viram métodos comuns. Os *getters* ficam porque os testes verificam estado por eles | API QML menor | baixo | build + smoke das telas (o QML não referencia nenhum) |
| **P10-03** | `[Padronizar]` | Regra: *getters* de propriedade e sinais em camelCase (convenção Qt/QML); todo o resto em PascalCase. Renomear as 9 exceções acima (`Reload`, `SetMedia` ×2, `ConfigurePresentation`, `ConfigurePreferredTitle`, `SetCoverSource`, `SetRecords`, `AppendRecords`), atualizando `main.cpp` e testes. A regra entra no `SERVICE.md` | padrão único | baixo | build |

### Ordem sugerida
P10-01 → P10-02 → P10-03.

---

## 5. Fora de escopo (vai para a Parte 13)

Nada novo.

---

## 6. Decisões para o revisor

1. **Não fazer o objeto de seleção** nem mover os `Configure*` para o construtor (recomendo), pelos motivos da
   seção 2. A alternativa é um `MediaSelection` exposto como `QObject` com as propriedades de detalhe,
   compartilhado por Home e catálogo, reescrevendo os *bindings* do QML na Parte 11.

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| P10-01 `Preview*Metadata` e `ClearCoverCache` | `dddbbff` |
| P10-02 8 `Q_PROPERTY` e 2 `Q_INVOKABLE` fora do QML | `269b460` |
| P10-03 métodos em PascalCase | `7a7e21e` |

Notas:
- P10-01: as duas sobrecargas de `cardStatusLabel` viraram uma só, que usa `media.ListStatus`.
- P10-03: `NowWatchingController::setPreferredTitleKey` virou `ConfigurePreferredTitle`, o mesmo nome usado na
  Home e no catálogo. A regra de nomes entrou no `SERVICE.md` (§ 8.1).
- O objeto de seleção e a mudança dos `Configure*` não foram feitos (decisão 1).
