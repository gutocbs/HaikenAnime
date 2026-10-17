# Contratos externos — mpv JSON IPC e Plex

Consultado em 2026-10-17. Base para as Tasks 5 (mpv) e 6 (Plex) do plano
`docs/superpowers/plans/2026-10-17-reconhecimento-episodios.md`.

## mpv JSON IPC

Fontes oficiais:
- [DOCS/man/ipc.rst](https://github.com/mpv-player/mpv/blob/master/DOCS/man/ipc.rst)
- [DOCS/man/input.rst](https://github.com/mpv-player/mpv/blob/master/DOCS/man/input.rst) (eventos e propriedades)
- [Manual estável](https://mpv.io/manual/stable/#json-ipc)

Transporte:
- Windows: `--input-ipc-server=\\.\pipe\<nome>` (named pipe).
- Mensagens JSON em UTF-8, **uma por linha** (terminadas em `\n`, sem `\n` interno).
  mpv pode enviar UTF-8 inválido: o cliente filtra antes de interpretar.
- Respostas e eventos podem chegar intercalados; eventos podem vir antes da resposta.
  As respostas chegam na ordem dos comandos.

Comandos e respostas:
```json
{ "command": ["get_property", "time-pos"], "request_id": 100 }
{ "error": "success", "data": 1.468135, "request_id": 100 }
```
- `request_id` é inteiro e volta igual na resposta (sem ele, a resposta traz 0).
- `error` diferente de `"success"` indica falha (ex.: propriedade indisponível).

Observação de propriedades:
```json
{ "command": ["observe_property", 1, "time-pos"] }
{ "error": "success" }
{ "event": "property-change", "id": 1, "data": 52.0, "name": "time-pos" }
```
- A observação termina quando a conexão fecha.

Eventos de ciclo de vida:
- `start-file` (`playlist_entry_id`), `file-loaded`, `playback-restart`, `seek`.
- `end-file`: `reason` ∈ `eof`, `stop`, `quit`, `error`, `redirect`, `unknown`;
  `playlist_entry_id`; `file_error` opcional.
- `shutdown`: o player está encerrando.
- Eventos podem trazer campos extras: ignorar o que não for conhecido.

Propriedades usadas:
- `path` (caminho absoluto do arquivo), `filename`, `media-title`.
- `time-pos` (segundos; `playback-time` é alias), `duration` (indisponível se desconhecida).
- `pause` (flag), `core-idle` (`yes` também enquanto bufferiza ou nada toca), `idle-active`.

## Plex

Fonte oficial: [Plex Media Server API — developer.plex.tv/pms](https://developer.plex.tv/pms/)
(seção "Authenticating with Plex" e referência de endpoints).
Complementos da comunidade, usados onde a página oficial não traz o schema:
[Plexopedia — Get Active Sessions](https://www.plexopedia.com/plex-media-server/api/server/sessions/),
[python-plexapi #638 (viewOffset)](https://github.com/pkkid/python-plexapi/issues/638).

Cabeçalhos comuns: `X-Plex-Client-Identifier` (persistente e único por instalação),
`X-Plex-Product`, `X-Plex-Token` (após autenticação), `Accept: application/json`
(sem ele o PMS responde XML).

Login no navegador (fluxo PIN com token legado, ainda suportado):
1. `POST https://plex.tv/api/v2/pins?strong=true` → JSON com `id` e `code`.
2. Abrir `https://app.plex.tv/auth#?clientID=<clientIdentifier>&code=<code>&context%5Bdevice%5D%5Bproduct%5D=HaikenAnime`.
3. Consultar `GET https://plex.tv/api/v2/pins/<id>` (~1 vez por segundo) até
   `authToken` deixar de ser `null`; o PIN expira (`expiresIn`, ~30 min).
4. Validar o token: `GET https://plex.tv/api/v2/user` → 200 válido, 401 inválido.

A documentação oficial recomenda o fluxo JWT (Ed25519, `clients.plex.tv`, renovação
a cada 7 dias). Na V1 usamos o fluxo de token legado e deixamos o JWT como evolução
(ver ledger).

Descoberta de servidores:
- `GET https://clients.plex.tv/api/v2/resources?includeHttps=1&includeRelay=1&includeIPv6=1`
  com `X-Plex-Token`.
- Cada recurso traz `name`, `clientIdentifier`, `provides` (contém `server`),
  `accessToken` e `connections[]` (`uri`, `local`, `relay`, `protocol`, `address`, `port`).
  Preferência: `local`, depois remota direta, e `relay` como último recurso.

Sessões ativas no servidor:
- `GET <servidor>/status/sessions` com `X-Plex-Token` (o `accessToken` do recurso) e
  `Accept: application/json`.
- `MediaContainer.size` e `MediaContainer.Metadata[]`. Episódio: `type="episode"`,
  `grandparentTitle` (série), `parentIndex` (temporada), `index` (episódio), `title`,
  `duration` e `viewOffset` (ms, atualizado a cada ~10 s), `sessionKey`, `ratingKey`, `guid`.
- Elementos filhos: `User` (`id`, `title`, `thumb`), `Player` (`machineIdentifier`,
  `title`, `product`, `platform`, `state` = `playing`/`paused`/`buffering`, `local`),
  `Session` (`id`, `bandwidth`, `location`).

Usuários do servidor: `GET <servidor>/accounts` → `MediaContainer.Account[]`
(`id`, `name`). Esse endpoint é documentado pela comunidade (python-plexapi
`systemAccounts`), não pela página oficial. A seleção de usuários também aceita os
`User.id` vistos nas sessões.
