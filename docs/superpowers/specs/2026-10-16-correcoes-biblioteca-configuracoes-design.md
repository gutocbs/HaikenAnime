# Correções de biblioteca, configurações e sincronização

## Objetivo

Documentar as correções solicitadas para a biblioteca, configurações, sincronização AniList e estados de erro. A implementação deve ocorrer em fatias pequenas, preservando a separação entre apresentação, controllers, serviços de aplicação e persistência.

## Correções

### 1. Signal handlers do QML

Corrigir os handlers de `SettingsScreen.qml` que usam parâmetros implícitos, especialmente `checked` em switches. Os handlers devem declarar formalmente seus parâmetros.

Aceite: alterar qualquer configuração não produz o warning `Parameter "checked" is not declared`.

### 2. Janela inicial maximizada

A janela principal deve iniciar sempre maximizada. Hoje ela abre com tamanho fixo e, muitas vezes, em uma posição que impede o usuário de alcançar o botão de maximizar.

### 3. Filtro inicial da biblioteca

A tela inicial da biblioteca deve selecionar `WATCHING` por padrão, em vez de `Todos`. A alteração afeta apenas a seleção inicial; o usuário continua podendo trocar o filtro.

### 4. Mídias removidas do AniList

Em uma sincronização completa, mídias presentes no banco mas ausentes no snapshot do AniList devem ser marcadas como removidas da fonte e deixar de ser tratadas como ativas.

O mecanismo preferencial é o campo existente `source_removed_at`, mantendo separados o estado da fonte externa, o estado pessoal do usuário e os dados locais. Não usar `user_list_status = 99`, pois isso mistura remoção da fonte com listas pessoais. A reconciliação não deve ocorrer em sincronizações filtradas ou parciais.

### 5. Total de capítulos desconhecido

Valores nulos de capítulos devem ser persistidos como `0` quando o contrato do banco exigir um valor não nulo. Na apresentação, `0` usado como marcador de desconhecido deve aparecer como `?` (ex.: hoje o card mostra `3/0`; deve mostrar `3/?`).

DTOs e mapeadores devem preservar a nulidade até a fronteira de persistência/apresentação quando essa distinção for necessária.

### 6. Isolamento de erros por tela

Mensagens de erro devem pertencer ao fluxo que as originou. Um erro de sincronização não deve aparecer automaticamente na Home, nas configurações e em outras telas.

Devem permanecer separados, no mínimo, os estados de sincronização AniList, varredura local, limpeza de cache, autenticação e operações da biblioteca. Cada tela deve consumir apenas o estado do seu fluxo.

### 7. Conta AniList

Quando a conta estiver conectada ou durante o OAuth, o botão de conexão deve ficar desabilitado.

Adicionar uma ação de remoção de conta que interrompa listener OAuth, apague tokens e segredos, limpe nome/identificador/estado de autenticação, impeça concorrência e preserve a biblioteca local. Tokens, callbacks e payloads não devem ser registrados em logs.

### 8. Progresso da sincronização

A sincronização deve publicar progresso entre início e fim, atravessando coordenador/serviço, controller e QML. O contrato deve suportar etapa atual, quantidade processada, total ou total desconhecido e estado em execução/concluído/falho.

A barra deve ser fina, usar as cores do serviço e ficar visível somente enquanto houver sincronização ativa ou mensagem final relevante.

### 9. Padrão visual dos botões

Botões equivalentes devem usar o componente visual da aplicação, em vez do estilo nativo do Windows/Qt Controls. Hover, pressionado, foco, desabilitado e acessibilidade devem seguir o padrão existente.

### 10. Remover mídia da lista

O menu de edição dos detalhes deve permitir remover a mídia da lista pessoal. O fluxo deve confirmar a ação, atualizar o banco, atualizar controller e biblioteca sem reinício, atualizar/fechar o painel e exibir erros apenas no contexto da operação.

## Ordem de implementação

1. Corrigir handlers implícitos do QML.
2. Garantir que a janela principal inicie maximizada.
3. Definir `WATCHING` como filtro inicial da biblioteca.
4. Corrigir a apresentação de capítulos desconhecidos.
5. Finalizar a reconciliação de mídias removidas do AniList.
6. Isolar estados de erro por fluxo e tela.
7. Corrigir conexão e adicionar remoção de conta.
8. Propagar progresso real e redesenhar a barra.
9. Uniformizar botões das configurações.
10. Adicionar remoção de mídia pelo painel de detalhes.

## Validação

Cada fatia deve ter testes focados antes da próxima etapa. A evidência deve distinguir testes unitários/integrados, build dos alvos, CTest, runtime e inspeção visual/manual.

Casos mínimos:

- nenhum warning de parâmetros implícitos ao editar configurações;
- janela principal inicia maximizada;
- biblioteca inicia em `WATCHING`;
- mídia ausente no snapshot completo é marcada como removida;
- sincronização filtrada não marca mídias não observadas como removidas;
- capítulos desconhecidos aparecem como `?`;
- erros de sincronização não vazam para outras telas;
- conta conectada não permite nova conexão;
- remoção de conta elimina credenciais sem apagar biblioteca local;
- barra reflete progresso intermediário;
- remoção de mídia atualiza banco e interface;
- remoção de mídia é propagada para a AniList e a sincronização não a restaura;
- pendências de outra conta não são enviadas após troca de conta;
- barra de progresso some alguns segundos após sucesso ou falha.

## Decisões e limites

- `source_removed_at` representa remoção pela fonte externa.
- `user_list_status` permanece reservado ao estado da lista do usuário.
- Remoção de conta não apaga mídia local por padrão.
- Reconciliação de remoção só vale para sincronização completa.
- QML permanece responsável pela apresentação; persistência, filtros, ordenação, sincronização e reconhecimento ficam nos controllers e serviços apropriados.
- `personal_list_removed_at` representa remoção pela ação do usuário; é separado de `source_removed_at` e de `user_list_status`.
- Remoção de mídia da lista é propagada para a AniList.
- Pendências pertencem a uma conta (`owner_user_id`) e só são enviadas para essa conta; nunca são descartadas por troca de conta.
- O estado da sincronização AniList tem um único dono (`SynchronizationStatusController`); Home e Configurações não guardam cópia dele.

## Design detalhado

### Diagnóstico do estado atual

| Item | Situação encontrada | Local |
| :--- | :--- | :--- |
| 1 | `SettingSwitch` declara `signal toggled(bool checked)` e os handlers usam o parâmetro injetado | `resources/qml/SettingsScreen.qml` |
| 2 | `Main.qml` abre com 1440×900 fixos, sem `visibility` | `resources/qml/Main.qml` |
| 3 | `activeListFilter_` inicia em `"all"` | `src/presentation/home/HomeScreenController.h` |
| 4 | A reconciliação existe, mas nunca roda: o executor sempre envia `type` e `acceptedListStatuses`, e `isCompleteAuthoritativeSnapshot` exige ambos vazios | `src/app/ApplicationSyncTaskExecutor.cpp`, `src/infrastructure/anilist/GraphQlAniListDataSource.cpp` |
| 5 | Persistência já grava `0` (`value_or(0)`); apresentação formata `%1/%2` sem tratar `0` | `HomeScreenController.cpp`, `SeasonalCatalogController.cpp` |
| 6 | Falha de qualquer partição muda `state` da Home para `error` e preenche `errorMessage` geral das Configurações | `main.cpp` |
| 7 | Botão desabilitado só durante OAuth; `AniListAuthManager::clear()` existe sem ação de UI | `SettingsController.cpp` |
| 8 | `HomeScreenController::notifySynchronizationProgress` existe, mas nada o chama | — |
| 9 | 24 `Button` nativos em 6 arquivos QML; o padrão visual é o `SettingsButton` inline | `resources/qml/*.qml` |
| 10 | `AniListField::Deletion` existe, mas fica em `RequiresConfirmation` e `AniListUpdateClient` recusa a operação | `AniListPendingChangeProcessor.cpp`, `AniListUpdateClient.cpp` |

### 1. Handlers explícitos

Todos os handlers com parâmetro passam à forma `onToggled: function(checked) { ... }` (`SettingSwitch`, `Switch` e `CheckBox` das configurações). `QmlStructureTests` ganha verificação que reprova handlers que usem parâmetros de sinal sem declará-los.

### 2. Janela maximizada

`Main.qml` define `visibility: Window.Maximized`; `width`/`height` permanecem como tamanho de restauração. Validação: teste estrutural + inspeção em runtime.

### 3. Filtro inicial

`activeListFilter_` inicia em `current` (WATCHING). O filtro padrão usado por `ClearBrowseCriteria` e `browseCriteriaActive` passa a ser a constante `DefaultListFilterKey = "current"`, com fallback para a primeira opção quando `current` não existir nas opções configuradas.

### 4. Reconciliação de mídias removidas da fonte

- `AniListGraphQlUserListParser` devolve `observedMediaIds`: todos os IDs de entradas recebidas, antes do filtro de listas habilitadas e de tipo.
- `AniListDataSourceResult` carrega esse conjunto. Snapshot completo = iniciou no chunk 1 e chegou ao fim (`hasNextChunk = false`); o filtro local de status não invalida o snapshot, pois a query já retorna a coleção inteira.
- `AniListSyncService` deixa de reconciliar; devolve os IDs observados e se o snapshot ficou completo.
- `ApplicationSyncTaskExecutor` agrega ANIME e MANGA e, somente se ambos terminarem completos e sem erro/cancelamento, chama `reconcileAuthoritativeSnapshot(observed, excluded, ...)`, onde `excluded` são os IDs com pendência local (`read-pending-media-ids.sql`).
- Sincronização filtrada, parcial, cancelada ou com falha não reconcilia.
- Log: quantidade de mídias marcadas, sem payload.
- Upsert posterior da mídia limpa `source_removed_at` (comportamento existente).

### 5. Total desconhecido

Função única `FormatProgress(consumed, total)` em `src/application/media` devolve `3/?` quando `total <= 0`. Usada em cards, detalhes, prévia compacta e catálogo sazonal. DTO preserva `std::optional` até o mapeamento para domínio (`value_or(0)`), que permanece a fronteira de persistência.

### 6 e 8. Estado de sincronização isolado e progresso

- Novo `SynchronizationStatusController` (presentation, exposto ao QML) é o dono do estado da sincronização AniList: `running`, `stageKey`, `stageLabel`, `stageIndex`, `stageCount`, `processedItems`, `totalItems` (`-1` = desconhecido), `outcome` (`idle` | `running` | `succeeded` | `failed`), `message`, `visible`.
- `visible` fica verdadeiro durante a sincronização e por 4 s após o término (sucesso ou falha), via `QTimer` com relógio/intervalo injetável para testes.
- `HomeScreenController` e `SettingsController` perdem `notifySynchronization*`. `state`/`errorMessage` da Home refletem apenas o carregamento da biblioteca; `errorMessage` das Configurações reflete apenas salvar/validar. Autenticação, varredura e limpeza de cache mantêm estados próprios. Sincronização concluída continua disparando `homeController.reload()`.
- Contrato: `struct SyncProgress { SyncPartition partition; int stageIndex; int stageCount; QString stageKey; int processed; int total; }`.
- Fluxo: `AniListSyncService` (callback `ProgressReporter` por chunk) → `ApplicationSyncTaskExecutor` → `AdaptiveSyncCoordinator::TaskProgress` (`Qt::QueuedConnection`) → `AdaptiveSyncRuntime::BackgroundTaskProgress` → `SynchronizationStatusController` (ligação em `main.cpp`).
- Etapas da partição UserList: Anime → Manga → Reconciliação. A partição PendingChanges reporta sua própria etapa.
- `SyncProgressBar.qml`: 3 px, trilho na cor `line`, preenchimento `#02A9FF` (cor da AniList), `danger` em falha, cantos arredondados, texto curto ao lado (ex.: "Manga · 120 itens"); indeterminado quando o total é desconhecido. Usado no rodapé da Home e no card "Conta e sincronização", lendo o mesmo controller.

### 7. Conta AniList

- Botão Conectar desabilitado nos estados `authenticated`, `authorizing`, `validating` e `removing`.
- Botão "Remover conta" (variante `danger`), visível com conta conectada, com diálogo de confirmação.
- `RemoveAniListAccountUseCase` (application), em ordem: (1) recusa se já houver remoção em andamento; (2) para o listener OAuth; (3) cancela as partições UserList e PendingChanges e aguarda a confirmação do cancelamento; (4) `AniListAuthManager::clear()` apaga token e segredos; (5) persiste o `userId` removido como última conta conhecida nas preferências; (6) limpa nome, identificador e estado.
- Biblioteca local e fila de pendências são preservadas.
- Migration `021-add-owner-user-id-to-pending-changes`: coluna `owner_user_id INTEGER NOT NULL DEFAULT 0`. Enqueue grava o `userId` da conta conectada ou, sem conta, o da última conta conhecida. Leituras de pendências filtram por `owner_user_id = :current OR owner_user_id = 0`; pendências órfãs (`0`, anteriores à migration) são adotadas pela primeira conta autenticada, com contagem registrada em log. Pendências de outra conta permanecem guardadas e voltam a valer se essa conta for reconectada.
- Ao conectar uma conta diferente, a primeira sincronização completa marca como removidas da fonte as mídias que não pertencem à nova conta (consequência esperada do item 4).
- Logs apenas de eventos (`account removal started/completed/failed`); nunca token, callback ou payload.

### 9. Botões no padrão da aplicação

`SettingsButton` é promovido a `resources/qml/AppButton.qml` com variantes `primary`, `secondary`, `flat` e `danger`, cobrindo hover, pressionado, foco, desabilitado e `Accessible.name`. Substitui os `Button` nativos em `SettingsScreen`, `Home`, `EditMediaPanel`, `MediaDetailsPanel`, `SeasonalCatalogScreen` e `CoverPreview`. A execução desta fatia usa a skill `frontend-design`. `QmlStructureTests` passa a reprovar `Button {` nativo nesses arquivos.

### 10. Remover mídia da lista

- Ação "Remover da lista" (variante `danger`) no menu de edição dos detalhes, com diálogo de confirmação exibindo o título.
- `PersonalListChangeService::remove(media)` em transação única: grava `personal_list_removed_at` (migration `023`) e enfileira `Deletion` já com status `Pending` — a confirmação do usuário é a confirmação exigida.
- `read-media.sql` exclui `personal_list_removed_at IS NOT NULL`. O upsert da sincronização não altera essa coluna; ela só é limpa quando o usuário adiciona a mídia novamente (`update-personal-list-media.sql`).
- `AniListUpdateClient` executa a deleção. **Contrato a verificar antes da fatia (bloqueio de DoR):** a mutation `DeleteMediaListEntry` da AniList recebe o `id` da entrada da lista, não o `mediaId`; o arquivo atual `delete-list-entry.graphql` usa `mediaId`. Proposta: resolver o `id` da entrada no momento do envio via `MediaList(userId, mediaId) { id }`; entrada inexistente é tratada como sucesso. Referência: https://docs.anilist.co/ (schema GraphQL `Mutation.DeleteMediaListEntry`).
- `HomeScreenController::RemoveSelectedMedia()` atualiza `allMedia_`, reconstrói os modelos e limpa a seleção sem reinício; o painel de detalhes fecha. Erros ficam em `mediaOperationErrorMessage`, exibido somente no painel.

### Critérios de aceitação (BDD)

- **Dado que** a sincronização completa de ANIME e MANGA terminou sem erro, **quando** uma mídia local não aparece em nenhuma entrada recebida e não tem pendência local, **então** ela recebe `source_removed_at` e deixa de ser exibida.
- **Dado que** a sincronização de MANGA falhou, **quando** a execução termina, **então** nenhuma mídia é marcada como removida.
- **Dado que** a lista "Dropped" está desabilitada, **quando** a sincronização completa termina, **então** mídias em "Dropped" na AniList não são marcadas como removidas.
- **Dado que** a sincronização AniList falhou, **quando** o usuário está na Home ou nas Configurações, **então** a falha aparece apenas na barra/estado de sincronização e não altera o estado da biblioteca nem a mensagem de salvamento.
- **Dado que** há sincronização em andamento, **quando** um chunk é processado, **então** a barra exibe etapa e itens processados; **e quando** a sincronização termina, **então** a barra some após 4 s.
- **Dado que** a conta está conectada, **quando** o usuário abre "Conta e sincronização", **então** "Conectar conta" está desabilitado e "Remover conta" está disponível.
- **Dado que** o usuário confirmou a remoção da conta durante uma sincronização, **quando** a remoção executa, **então** a sincronização é cancelada antes de apagar o token e a biblioteca local permanece intacta.
- **Dado que** há pendências da conta A, **quando** a conta B é conectada, **então** essas pendências não são enviadas.
- **Dado que** o usuário confirmou "Remover da lista", **quando** a operação termina, **então** a mídia some da biblioteca sem reinício, uma pendência `Deletion` é enviada à AniList e a sincronização seguinte não a restaura.
- **Dado que** uma mídia tem total de episódios/capítulos desconhecido, **quando** é exibida, **então** o progresso aparece como `3/?`.

### Observabilidade e testes

- Logs estruturados por evento (reconciliação: quantidade; remoção de conta: início/fim/falha; deleção: mediaId e resultado; adoção de órfãs: quantidade). Nunca registrar token, callback, payload ou tenant.
- Cada fatia registra: testes unitários/integrados executados, build do alvo, CTest e, quando houver UI, inspeção em runtime.
- Testes previstos: `QmlStructureTests` (handlers, `visibility`, botões nativos), `HomeScreenControllerTests` (filtro, `3/?`, remoção, isolamento de erro), `SynchronizationStatusControllerTests` (novo), `AniListGraphQlUserListParserTests` (IDs observados), `AniListSyncServiceTests`/`AniListFlowTests` (snapshot e progresso), `SqliteRepositoryTests` (reconciliação com exclusões, `personal_list_removed_at`), `SqliteDatabaseTests` (migrations 021, 022 e 023), `AniListPendingChangeProcessorTests` (deleção, dono), `PersonalListChangeServiceTests` (remoção), `SettingsControllerTests` (conectar/remover conta), testes do novo `RemoveAniListAccountUseCase`.
