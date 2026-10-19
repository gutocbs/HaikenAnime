# Plugin Plex

`src/infrastructure/recognition/plex/` — `PlexPlugin`, `PlexApiClient`, `PlexAuthentication`, `QtPlexHttp`.
Contrato geral: [`README.md`](README.md).

## Comportamento

- Consulta `/status/sessions` do servidor escolhido a cada **15 s** (configurável de 5 a 60 s; mais que 60 s
  perderia tempo de reprodução) e reporta só episódios (`type="episode"`) dos **usuários selecionados**.
- O episódio vai como título da série + temporada + episódio, com confiança **0,9**: os dados do Plex são
  estruturados, mas a série ainda precisa ser associada à biblioteca.
- Identidade da sessão: `servidor:sessionKey`. O Plex mantém uma sessão fechada como "pausada" por um tempo
  e abre outra ao reabrir o mesmo episódio; para o mesmo usuário + aparelho + episódio, **só vale a chave
  mais nova** (a antiga para como `Replaced`).
- Só `playing` conta tempo; `paused`/`buffering` não. Uma sessão que some da lista para como `Closed`.
- Falhas: **3** consultas seguidas sem resposta encerram as sessões como `Unavailable` (uma falha isolada
  não zera o tempo contado). *Token* recusado (401): encerra as sessões, esquece o servidor e tenta de novo
  após **5 min**.
- Cada mudança no que o servidor reporta gera uma linha de log (sem *tokens*).

## Configurações

| Campo | Tipo | Padrão | Uso |
| --- | --- | --- | --- |
| `serverId` | Text | — | servidor observado (escolhido na lista da tela) |
| `selectedUsers` | Text | — | ids dos usuários do Plex, separados por vírgula |
| `pollSeconds` | Number (5–60) | 15 | intervalo de consulta |

O `clientIdentifier` (único por instalação) e o *token* ficam no Credential Manager, nunca no banco.

## API

Fonte oficial: [developer.plex.tv/pms](https://developer.plex.tv/pms/). Complementos da comunidade onde falta
o schema: [Plexopedia — Get Active Sessions](https://www.plexopedia.com/plex-media-server/api/server/sessions/),
[python-plexapi #638](https://github.com/pkkid/python-plexapi/issues/638).

- Cabeçalhos: `X-Plex-Client-Identifier`, `X-Plex-Product`, `X-Plex-Token` e `Accept: application/json`
  (sem ele o servidor responde XML).
- **Login (PIN)**: `POST https://plex.tv/api/v2/pins?strong=true` → `id`, `code`; abrir
  `https://app.plex.tv/auth#?clientID=<id>&code=<code>&context%5Bdevice%5D%5Bproduct%5D=HaikenAnime`;
  consultar `GET https://plex.tv/api/v2/pins/<id>` (~1×/s) até `authToken` vir preenchido (o PIN expira em
  ~30 min); validar com `GET https://plex.tv/api/v2/user` (200 ok, 401 inválido). O fluxo JWT recomendado pela
  documentação oficial fica como evolução.
- **Servidores**: `GET https://clients.plex.tv/api/v2/resources?includeHttps=1&includeRelay=1&includeIPv6=1`.
  Cada recurso traz `clientIdentifier`, `provides` (contém `server`), `accessToken` e `connections[]`; a
  preferência é conexão local, depois remota direta e por último *relay*.
- **Sessões**: `GET <servidor>/status/sessions` com o `accessToken` do recurso → `MediaContainer.Metadata[]`:
  `grandparentTitle` (série), `parentIndex` (temporada), `index` (episódio), `title`, `duration`,
  `viewOffset` (ms, atualizado a cada ~10 s), `sessionKey`, `User` (`id`, `title`) e `Player`
  (`machineIdentifier`, `title`, `state`).
- **Usuários**: `GET <servidor>/accounts` → `MediaContainer.Account[]` (`id`, `name`), documentado pela
  comunidade. A seleção também aceita os `User.id` vistos nas sessões.
