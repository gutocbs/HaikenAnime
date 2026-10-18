# Validação: reconhecimento de episódios e tela "Assistindo agora"

Branch `feature/reconhecimento-episodios`.

- Spec do reconhecimento: `docs/superpowers/specs/2026-10-17-reconhecimento-episodios-design.md`;
  plano: `docs/superpowers/plans/2026-10-17-reconhecimento-episodios.md` (Task 9).
- Spec da tela: `docs/superpowers/specs/2026-10-17-assistindo-agora-design.md`;
  plano: `docs/superpowers/plans/2026-10-17-assistindo-agora.md` (Task 7).
- Contratos externos: `docs/superpowers/references/2026-10-17-mpv-plex-contratos.md`.

Legenda: **Automatizado** = teste nesta execução. **Ao vivo (usuário)** = verificado pelo usuário no
app real, em 2026-10-17. **Evidência** = conferido pelo agente em dados reais, sem alterá-los
(banco aberto em modo somente leitura e log do app). **Não verificado** = sem evidência; não
declarado como aprovado.

## Execução automatizada (`cmake-build-validation`, MSVC 2022 x64, Qt 6.8.3)

- Build completo: exit 0.
- Testes focados (`ctest -R "Recognition|Plugin|Mpv|Plex|NowWatching|ActiveSession|PlaybackObservation|ProgressDecision|QmlRecognition|SqliteDatabase|LocalMediaResolver"`): **26/26**.
- Suíte completa, exceto 5 testes conhecidos: **78/79**.
- Falhas anteriores à branch e alheias à feature (corrigidas à parte na branch
  `hotfix/testes-preexistentes`, ainda não integrada):
  - `AlternativeNamesTests`, `AniListOAuthCallbackReceiverTests`, `WindowsUrlProtocolRegistrarTests`,
    `PersonalListChangeServiceTests` e `AniListAccountOwnershipTests` saem com `0xc0000135` (DLL do Qt
    não encontrada), por estarem fora de `HAIKENANIME_TESTS`. Sem timeout, o diálogo do Windows pode
    travar a suíte (`AlternativeNamesTests` ficou 1435 s esperando).
  - `LocalLibraryScanCompositionTests::workerProductsReleaseConnectionsAndShutdownBeforeDatabase`
    falha de forma determinística.
- `git diff --check master..HEAD`: limpo. Nenhum executável, banco, log ou artefato de build
  entre os 140 arquivos da branch, e nenhum segredo fixo no código.

## Reconhecimento de episódios (spec, seção "Testes e critérios de aceitação")

| Critério | Verificação | Resultado |
| :--- | :--- | :--- |
| HaikenAnime inicia o mpv com IPC e reconhece um episódio | `MpvPluginTests`, `MpvIpcClientTests` (named pipe real) | Automatizado OK. **Ao vivo (usuário)**: episódio assistido 3+ min pelo mpv foi marcado como assistido. |
| Fechar antes de três minutos não altera o progresso | `PlaybackObservationServiceTests::closingAtTwoMinutesFiftyNineIsIncomplete`, `MpvPluginTests::closingBeforeThreeMinutesDoesNotComplete`, `EpisodeRecognitionServiceTests::closingBeforeThresholdIsRecordedIncomplete` | Automatizado OK. **Ao vivo (usuário)**: OK. |
| Tempo configurado muda o limite sem recompilar | `PlaybackObservationServiceTests::policyChangeAppliesToActiveSessions`, `EpisodeRecognitionServiceTests::configuredPolicyChangesThreshold` | Automatizado OK. Ao vivo: **não verificado**. |
| Sessão Plex selecionada é reconhecida | `PlexPluginTests`, `EpisodeRecognitionServiceTests::unseenEpisodeAfterThresholdIsMarkedWatchedOnce` | Automatizado OK. **Evidência**: registro 12 do histórico, Plex, "Seitokai ni mo Ana wa Aru!" T1 E1, `marked-watched`, sincronização `synced` (chegou ao AniList). |
| Sessões de usuários não selecionados são ignoradas | `PlexPluginTests::reportsOnlySelectedUsers`, `deselectingUserStopsTheirSession` | Automatizado OK. Ao vivo: **não verificado** (só um usuário Plex foi usado). |
| Episódios inéditos atualizados uma única vez | `RecognitionHistoryServiceTests::duplicateSessionEpisodeNeverAppliesProgressTwice`, `EpisodeRecognitionServiceTests::duplicateSessionNeverAppliesProgressTwice`, `PlexPluginTests::repeatedPollsDoNotStartTheSameSessionTwice` | Automatizado OK. |
| Histórico e falhas de sincronização persistidos | `SqliteRecognitionHistoryRepositoryTests`, `RecognitionSyncFeedbackTests`, `AniListPendingChangeProcessorTests` | Automatizado OK. **Evidência**: estado de sincronização `synced` gravado no registro 12. |
| Autenticação Plex e configurações pela tela genérica | `PluginSettingsControllerTests`, `PlexAuthenticationTests`, `QmlRecognitionStructureTests` | Automatizado OK. **Evidência**: `plugin_setting` com `serverId`, `selectedUsers` e `mpv.enabled`; consultas ao servidor ativas no log. |
| App abre com Plex ou mpv indisponíveis | `RecognitionCompositionTests` (sem servidor, sem mpv, configurações inválidas, sem banco) | Automatizado OK. Servidor Plex desligado ao vivo: **não verificado**. |
| Token nunca exposto | `PlexAuthenticationTests::tokenNeverAppearsInMessagesOrLogs`, `PlexPluginTests::serverTokenNeverAppearsInLogs`, `PluginSettingsControllerTests::messagesNeverContainTheToken` | Automatizado OK. **Evidência**: 0 ocorrências de `X-Plex-Token`, `authToken` ou `accessToken` nos logs de 2026-10-17 e 2026-10-18; `plugin_setting` não guarda token (fica no Windows Credential Manager). |

## Tela "Assistindo agora" (spec, seção "Critérios de aceite (BDD)")

| Critério | Verificação | Resultado |
| :--- | :--- | :--- |
| Botão à direita de "Configurações", visível só com sessão ativa | `QmlRecognitionStructureTests::homeButtonIsRightOfSettingsAndGuarded`, `NowWatchingControllerTests::inactiveWithoutSessions` | Automatizado OK. **Ao vivo (usuário)**: OK. |
| Anime do banco + dados do episódio do plugin | `NowWatchingControllerTests::associatedSessionShowsAnimeFromDatabase`, `PlexPluginTests::startedSessionCarriesEpisodeDetails`, `MpvPluginTests::mediaTitleAndDurationBecomeDetails` | Automatizado OK. **Ao vivo (usuário)**: OK. |
| Várias sessões: um card por sessão, seleção dentro da tela | `NowWatchingControllerTests::defaultsToMostRecentSession`, `selectSessionSwitchesCurrent`, `selectionMovesWhenSelectedSessionEnds` | Automatizado OK. Duas sessões ao vivo: **não verificado**. |
| "não associado à biblioteca" só quando o título realmente não casa | `NowWatchingControllerTests::unassociatedSessionShowsEpisodeOnly`, `LocalMediaResolverTests::animeFileIgnoresMangaAndNovelWithSameTitle` | Automatizado OK. **Ao vivo (usuário)**: após a correção do mangá homônimo, "Seitokai ni mo Ana wa Aru!" passou a aparecer associado. |
| Player fechado: card some; sem sessões, o botão some | `ActiveSessionRegistryTests`, `PlexPluginTests::vanishedSessionStopsAsClosed`, `newerSessionOfSameEpisodeReplacesOlderOne` | Automatizado OK. **Evidência**: log com `Plex session stopped: … reason=no-longer-reported` logo depois de o servidor deixar de listar a sessão. |
| Pausa indicada sem encerrar a sessão | `NowWatchingControllerTests::exposesPausedStateFromThePlayer`, `ActiveSessionRegistryTests::startsAsPlayingAndTracksStateChangesOnce` | Automatizado OK. **Evidência**: transições `state=paused`/`state=playing` da sessão `…:13` no log. |
| Nomes de usuário e aparelho no lugar dos ids | `PlexPluginTests::startedSessionCarriesDisplayNames`, `NowWatchingControllerTests::fallsBackToIdsWithoutDisplayNames` | Automatizado OK. **Ao vivo (usuário)**: OK. |
| Excluir do histórico (sem alterar o AniList; descarta o progresso não enviado) | `RecognitionHistoryServiceTests::remove*`, `SqliteRecognitionHistoryRepositoryTests::removeDeletesRecordAndDiscardsOnlyMatchingQueuedProgress` | Automatizado OK. **Evidência**: os registros 1 a 11 foram removidos; resta só o 12. |

## Comportamentos do Plex observados na validação

- O Plex continua listando uma sessão fechada como `paused` por algum tempo, que depende do
  cliente. O HaikenAnime mostra a sessão como pausada e a encerra quando o servidor deixa de
  listá-la.
- Ao reabrir o mesmo episódio, o Plex cria uma nova `sessionKey`; a sessão anterior do mesmo
  usuário, aparelho e episódio passa a ser tratada como substituída.

## Pendências conhecidas

- Não verificado ao vivo: tempo mínimo alterado, servidor Plex indisponível, filtro de outro
  usuário do Plex e duas sessões simultâneas.
- Desempenho: a cada mudança de sessão, a GUI relê as 2400 mídias (~0,35 s por leitura). Há uma
  sessão separada proposta para resolver isso.
- Falhas de teste pré-existentes: dependem da integração da branch `hotfix/testes-preexistentes`.
