# Tela "Assistindo agora"

## Status

Especificação para revisão. Complementa
`docs/superpowers/specs/2026-10-17-reconhecimento-episodios-design.md`, cujas Tasks 1–8 já
estão implementadas na branch `feature/reconhecimento-episodios`.

## Objetivo

Mostrar ao usuário, enquanto um episódio está em reconhecimento automático, qual anime e qual
episódio estão sendo assistidos. Os dados do anime vêm do banco local. Os dados do episódio vêm
do plugin que observou a reprodução, quando ele os tiver: o HaikenAnime **exibe** esses dados,
mas **não os busca** por conta própria.

## Decisões do brainstorming

- O botão **"Assistindo agora"** fica no cabeçalho da Home, **à direita** de "Configurações", e só
  aparece enquanto existe ao menos uma sessão em reconhecimento. Botões novos sempre entram à
  direita dos existentes.
- Há **um único botão**, sem contagem. Com mais de uma sessão ativa, a escolha é feita **dentro
  da tela**, por um seletor de sessão. A tela mostra uma sessão por vez.
- A tela é **informativa**: não mostra barra de andamento até o tempo mínimo, estado de
  reprodução, desfecho previsto nem ações. Corrigir/ignorar continuam no Histórico.
- **Sem imagem de episódio.** A thumb do Plex exige o token na URL; a imagem exibida é a capa do
  anime vinda do banco (ou o placeholder atual).
- Arquitetura: **abordagem A**. O pipeline mantém um registro de sessões ativas e o
  `PlaybackSession` ganha um `EpisodeDetails` opcional preenchido pelo plugin.

## Componentes

### Domínio

`EpisodeDetails` (novo valor, opcional dentro de `PlaybackSession`):

- `sourceTitle`: o nome cru que o plugin viu (nome do arquivo no mpv, `grandparentTitle` no Plex);
- `title`: título do episódio;
- `summary`: sinopse;
- `durationSeconds`: duração;
- `airDate`: data de exibição.

Todos os campos, exceto `sourceTitle`, são opcionais. Nenhum campo carrega segredo.

### Plugins (só o que já recebem, sem chamadas novas)

- **Plex:** a partir do mesmo `GET /status/sessions` já consultado, preenche `title`, `summary`
  (`summary`), `durationSeconds` (`duration`) e `airDate` (`originallyAvailableAt`). O
  `PlexApiClient` passa a ler `summary` e `originallyAvailableAt`. Se os detalhes mudarem entre
  duas consultas da mesma sessão e do mesmo episódio, o plugin reporta a atualização.
- **mpv:** observa também `media-title` e `duration` pelo JSON IPC e preenche `title` e
  `durationSeconds`. Não há sinopse nem data.

### Aplicação

`ActiveSessionRegistry` (classe nova e isolada):

- recebe os eventos de início, atualização de detalhes e fim de sessão já tratados pelo pipeline;
- guarda, por sessão: `PlaybackSession` (com `EpisodeDetails`), o `EpisodeCandidate` já associado
  à biblioteca (com `mediaId` quando houver) e o horário de início;
- expõe `sessions()`, em ordem de início, e um callback `changed`;
- remove a sessão quando ela é encerrada, por qualquer `StopReason`;
- ignora eventos depois de `clear()`, chamado pelo `stop()` do pipeline.

O `EpisodeRecognitionService` alimenta o registro e expõe uma referência somente leitura. A
política de tempo mínimo, a decisão de progresso e o histórico **não mudam**.

O `IPlaybackObserver` ganha `onSessionDetailsChanged(sessionId, EpisodeDetails)`, com
implementação padrão vazia, para que plugins atualizem os detalhes sem reiniciar a observação.

### Apresentação

`NowWatchingController` (QObject):

- `active` (bool): existe ao menos uma sessão;
- `sessions` (lista para o seletor): `sessionKey`, rótulo curto (anime ou título cru, usuário e
  origem);
- `selectedSessionKey` e `SelectSession(key)`. A seleção padrão é a sessão mais recente; se a
  sessão selecionada termina, a seleção passa para a próxima ativa;
- `current` (mapa da sessão selecionada):
  - anime, do banco quando associado: capa, título preferido, progresso X/Y, status da lista;
  - episódio, do plugin: temporada e episódio, título, sinopse, duração, data de exibição;
  - contexto: origem (mpv/Plex), usuário e dispositivo;
  - `associated` (bool) e, quando falso, o rótulo "não associado à biblioteca";
- emite `changed` a partir do callback do registro, enfileirado na thread da GUI.

QML:

- `NowWatchingScreen.qml` (nova): cabeçalho com "Voltar" e, quando há mais de uma sessão, um
  seletor de sessão; cartão da sessão selecionada. Sem sessões, mostra
  "Nada sendo reproduzido no momento". Não volta sozinha para a Home.
- `Home.qml`: botão "Assistindo agora" à direita de "Configurações", visível só com
  `nowWatchingController !== null && nowWatchingController.active`.
- `Main.qml`: página `"nowWatching"`, no mesmo padrão de `settings` e `seasonal`.
- Visual refinado com `/frontend-design` dentro do padrão atual: paleta `ink`, `muted`, `line`,
  `surfaceSoft` e `accent`, `AppButton`, cards com raio de 7 px e tipografia das telas de
  Configurações e do Catálogo da temporada. Capa à esquerda, dados do anime à direita e bloco do
  episódio abaixo, com a sinopse limitada a algumas linhas.

A QML não tem temporizadores, chamadas de rede ou AniList, parsing de dados de player nem
qualquer exibição de token.

## Erros e degradação

- Sem pipeline de reconhecimento, `nowWatchingController` é nulo e o botão nunca aparece.
- Campos de episódio ausentes: a linha correspondente é omitida (sem "undefined" nem rótulo
  vazio).
- Falha ao ler o anime no banco: o item aparece como não associado e um WARNING estruturado vai
  para o log. A tela não falha.
- Uma sessão sem `mediaId` (título do Plex que não casou com a biblioteca) aparece com os dados
  do episódio e o rótulo "não associado à biblioteca".

## Testes

- `ActiveSessionRegistryTests`: início, atualização de detalhes, fim por cada `StopReason`,
  várias sessões ao mesmo tempo, troca de episódio na mesma sessão, eventos depois de `clear()`.
- `PlexApiClientTests`/`PlexPluginTests`: `summary`, `duration` e `originallyAvailableAt` chegam a
  `EpisodeDetails`; campos ausentes ficam vazios; mudança de detalhes gera uma atualização.
- `MpvPluginTests`: `media-title` e `duration` são observados e chegam a `EpisodeDetails`.
- `EpisodeRecognitionServiceTests`: o pipeline alimenta o registro e `stop()` o limpa, sem mudar
  as decisões de progresso existentes.
- `NowWatchingControllerTests`: associado vs. não associado, `active` muda e emite sinal,
  seleção padrão e troca automática quando a sessão selecionada termina, falha de leitura do
  banco.
- `QmlRecognitionStructureTests` ampliado: `NowWatchingScreen.qml` carrega com controller falso,
  o botão da Home fica atrás de `nowWatchingController !== null`, e as regras de proibição
  (sem Timer, rede, parsing de player ou token) valem para os arquivos novos.

## Critérios de aceite (BDD)

- **Dado que** um episódio da biblioteca é reproduzido no mpv pelo HaikenAnime, **quando** o IPC
  conecta, **então** o botão "Assistindo agora" aparece à direita de "Configurações" e a tela
  mostra o anime do banco e o título/duração do episódio vindos do mpv.
- **Dado que** dois usuários selecionados assistem no Plex ao mesmo tempo, **quando** o
  HaikenAnime consulta as sessões, **então** há um único botão e a tela oferece um seletor com as
  duas sessões, cada uma com seu usuário e dispositivo.
- **Dado que** o título do Plex não casa com a biblioteca, **quando** a sessão aparece, **então** a
  tela mostra os dados do episódio e o rótulo "não associado à biblioteca".
- **Dado que** a sessão selecionada termina e outra continua ativa, **quando** o fim chega,
  **então** a seleção passa para a sessão restante.
- **Dado que** o player é fechado e não resta sessão, **quando** o fim chega, **então** a tela
  mostra "Nada sendo reproduzido no momento" e o botão deixa de aparecer na Home.

## Fora do escopo

Imagem de episódio, barra de andamento até o tempo mínimo, estado de reprodução, desfecho
previsto, ações na tela, busca de metadados pelo HaikenAnime e detecção de players iniciados
fora do HaikenAnime.

## Riscos

- O mpv só oferece metadados mínimos (título do arquivo e duração); a riqueza da tela depende do
  Plex.
- A sinopse do Plex pode ser longa; a tela limita linhas e não rola dentro do cartão.
- Arquivos tocados também pela sessão paralela de feedback do AniList (`CMakeLists.txt`,
  `EpisodeRecognitionService`): a implementação deve começar depois que aquele trabalho for
  commitado nesta branch, para evitar conflito na mesma cópia de trabalho.
