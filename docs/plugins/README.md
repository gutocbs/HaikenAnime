# Plugins de player

Um plugin conecta um player (mpv, Plex…) ao reconhecimento de episódios. Ele **só observa**: diz quais
sessões existem, o que está tocando e quando cada sessão para. Quem decide se o episódio conta como assistido,
grava o histórico e atualiza o progresso é o app (`EpisodeRecognitionService`, ver `SERVICE.md`). Este guia
basta para escrever um plugin novo; [`mpv.md`](mpv.md) e [`plex.md`](plex.md) descrevem só os dois plugins
existentes.

## Contrato (`src/application/recognition/IPlaybackPlugin.h`)

```cpp
class IPlaybackPlugin {
    QString id() const;                       // estável e único ("mpv", "plex"); vai para o histórico e as settings
    QString displayName() const;              // nome mostrado nas Configurações
    PluginCapabilities capabilities() const;  // ações extras que a tela de Configurações oferece
    PluginSettingsSchema settingsSchema() const;
    void observe(IPlaybackObserver &observer); // começa a reportar sessões
    void testConnection(std::function<void(const ConnectionTestResult &)> completion);
    void stop();                              // para timers/IPC/rede; pode ser chamado mais de uma vez
};
```

Eventos que o plugin envia ao `IPlaybackObserver`:

| Evento | Quando |
| --- | --- |
| `onSessionStarted(session, candidate)` | uma sessão começou, ou a mesma sessão passou a outro episódio |
| `onSessionUpdated(sessionId, snapshot)` | a cada observação: posição, `Playing`/`Paused`, `observedAt` |
| `onSessionDetailsChanged(sessionId, details)` | só os metadados de exibição mudaram (título do episódio, duração…) |
| `onSessionStopped(sessionId, reason)` | a sessão acabou: `Closed`, `Ended`, `Unavailable` ou `Replaced` |

Regras do contrato:

- **Identidade**: `PlaybackSessionId::create(pluginId, chave)`. A chave deve ser única dentro do plugin
  (o Plex usa `servidor:sessionKey`; o mpv, o nome do *pipe*).
- **Episódio**: `EpisodeCandidate::create(título, mediaId, temporada, episódio, confiança)`. Mande o título
  como o player o vê; o app associa à biblioteca. Se o plugin já conhece a mídia (o mpv abre arquivos da
  biblioteca), mande o `mediaId`. A confiança vai de 0 a 1 e é comparada com o mínimo de 0,8.
- **Tempo**: o app soma o tempo contínuo pelos `observedAt` dos *snapshots*. Pausa não conta, e um
  intervalo acima de **60 s** entre dois *snapshots* conta como 60 s. Então reporte pelo menos a cada 60 s
  enquanto toca, e não mais que ~1 vez por segundo.
- **Nada de decisões**: o plugin nunca mexe em progresso, histórico ou AniList, e nunca registra *tokens* no
  log (use mensagens seguras em `ConnectionTestResult::safeMessage`).
- **Assíncrono sem bloquear**: espera e repetição usam a função `schedule(delayMs, callback)` recebida nas
  dependências (`QTimer` em produção, imediata nos testes), nunca `sleep`.

## Configurações

`settingsSchema()` declara os campos (`Text`, `Number`, `Boolean`, `Selection`, `Secret`), com rótulo,
descrição, *default*, mínimo/máximo e opções. A tela de Configurações renderiza o formulário sozinha, e o
`PluginSettingsService` valida, grava em `plugin_setting` e guarda os `Secret` no Credential Manager
(`IPluginSecretStore`, nunca expostos à tela). Os valores salvos chegam ao plugin por
`applyRecognitionSettings` (`src/app/RecognitionComposition.cpp`), na inicialização e a cada "Salvar".

Capacidades (`PluginCapability`) que a tela entende: `BrowserAuthentication` (botão de login no navegador),
`ConnectionTest` (botão "Testar conexão") e `UserSelection` (lista de usuários do serviço).

## Como adicionar um plugin

1. Implementar `IPlaybackPlugin` em `src/infrastructure/recognition/<player>/`, recebendo as dependências
   (transporte/HTTP, `schedule`, `clock`, `log`) por uma struct, para poder testar com versões falsas.
2. Registrar em `createRecognitionContext` (criar, `RegisterSchema`, `registry.registerPlugin`) e ler as
   settings em `applyRecognitionSettings`.
3. Testes em `tests/unit/` com transporte/HTTP falso e respostas gravadas em `tests/fixtures/`. Cubra pelo
   menos: início, atualização e fim de sessão; troca de episódio na mesma sessão; player indisponível; e que
   nenhum segredo aparece em mensagens.
4. Atualizar `SERVICE.md` (§ Reconhecimento de episódios) e criar `docs/plugins/<player>.md` com o que for
   específico do serviço (endpoints, eventos, limites).
