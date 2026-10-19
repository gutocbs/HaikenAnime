# Plugin mpv

`src/infrastructure/recognition/mpv/` — `MpvPlugin`, `MpvIpcClient`, `QtLocalSocketMpvTransport`.
Contrato geral: [`README.md`](README.md).

## Comportamento

- Só reconhece o mpv **aberto pelo HaikenAnime** ("Assistir" na Home, com o plugin habilitado). Um mpv aberto
  por fora, sem IPC, nunca é reportado.
- `launchEpisode` inicia `mpv --input-ipc-server=\\.\pipe\haikenanime-mpv-<uuid> -- <arquivo>` sem esperar o
  processo. O episódio já vem da biblioteca, com `mediaId` e confiança 1.
- Conecta ao *named pipe* com até **60 tentativas a cada 250 ms** (~15 s, porque o mpv demora no primeiro
  uso). Se não conectar, a sessão é descartada sem erro para o usuário, só com uma linha no log.
- A sessão começa (e o tempo passa a contar) quando o IPC conecta. Observa `pause`, `time-pos`, `path`,
  `media-title` e `duration`. `time-pos` vira *snapshot* no máximo 1×/s; `media-title`/`duration` viram
  `onSessionDetailsChanged`.
- Fim: `end-file` com `eof` → `Ended`; `error` → `Unavailable`; `redirect` ou troca de `path` → `Replaced`;
  `stop`/`quit`, evento `shutdown` ou desconexão → `Closed`. Abrir outro episódio pelo app encerra a sessão
  anterior como `Replaced`.

## Configurações

| Campo | Tipo | Padrão | Uso |
| --- | --- | --- | --- |
| `enabled` | Boolean | `false` | se desligado, "Assistir" abre no programa padrão, sem reconhecimento |
| `executablePath` | Text (obrigatório) | `mpv` | programa usado; "Testar conexão" verifica se ele existe |

## Protocolo (mpv JSON IPC)

Fontes: [ipc.rst](https://github.com/mpv-player/mpv/blob/master/DOCS/man/ipc.rst),
[input.rst](https://github.com/mpv-player/mpv/blob/master/DOCS/man/input.rst),
[manual](https://mpv.io/manual/stable/#json-ipc).

- Uma mensagem JSON em UTF-8 **por linha**. O mpv pode mandar UTF-8 inválido, e o cliente filtra antes de
  interpretar. Eventos podem chegar antes das respostas; as respostas seguem a ordem dos comandos
  (`request_id` volta igual).
- `{"command": ["observe_property", <id>, "time-pos"]}` → eventos
  `{"event": "property-change", "id": <id>, "name": "time-pos", "data": 52.0}` até a conexão fechar.
- Eventos de ciclo de vida: `start-file`, `file-loaded`, `end-file` (`reason`: `eof`, `stop`, `quit`, `error`,
  `redirect`, `unknown`) e `shutdown`. Campos desconhecidos são ignorados.
