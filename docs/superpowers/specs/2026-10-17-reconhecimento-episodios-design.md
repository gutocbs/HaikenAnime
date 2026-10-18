# Reconhecimento automático de episódios por plugins

## Status

Especificação arquitetural para revisão. Esta etapa não implementa código.

## Objetivo

Adicionar ao HaikenAnime V2 o reconhecimento automático de episódios em reprodução,
inicialmente com mpv no Windows e Plex Media Server. Quando uma reprodução válida
permanecer aberta pelo tempo configurado, o sistema deve atualizar automaticamente
apenas episódios ainda não assistidos, registrar o histórico e sincronizar a alteração
com o AniList.

O usuário não confirma cada episódio. A interface apresenta reconhecimentos e permite
corrigir ou ignorar casos incorretos.

## Escopo da V1 do recurso

- Player local mpv no Windows.
- Plex Media Server acessível pela API HTTP, incluindo sessões de outros usuários
  selecionados pelo usuário.
- Autenticação Plex iniciada no navegador por fluxo oficial de login/PIN.
- Configuração dinâmica dos plugins por uma tela genérica.
- Tempo mínimo configurável, com padrão de três minutos.
- Atualização somente quando o episódio ainda não estiver marcado como assistido.
- Persistência de histórico, estado de reconhecimento e erros relevantes.
- Atualização local do progresso e sincronização posterior com AniList.

Ficam fora desta etapa: Crunchyroll, Plex Web como player local, plugins externos
carregados dinamicamente, detecção genérica de processos mpv iniciados fora do
HaikenAnime e confirmação obrigatória pelo usuário.

## Decisões de arquitetura

### Plugins internos com contrato comum

Os plugins serão implementações internas da aplicação, compiladas junto com a V2.
O contrato permite adicionar novos players sem acoplar a regra de progresso a uma
API específica.

Cada plugin deve expor identidade, capacidades, esquema de configurações, teste de
conexão e observação de sessões. O plugin produz dados normalizados de reprodução;
ele não decide se um episódio deve ser marcado como assistido.

Capacidades possíveis incluem:

- configurações dinâmicas;
- autenticação via navegador;
- teste de conexão;
- seleção de usuários;
- múltiplas sessões;
- encerramento explícito de sessão.

### mpv

O HaikenAnime iniciará o mpv com `--input-ipc-server` apontando para um named pipe
do Windows. O `MpvPlugin` consultará o JSON IPC para obter caminho/título, posição,
estado de pausa, duração e eventos de encerramento.

A descoberta de instâncias iniciadas externamente não faz parte do caminho principal.
Se o IPC não estiver disponível, a sessão não será considerada confiável para
reconhecimento automático.

### Plex

O `PlexPlugin` usará o fluxo de autenticação no navegador, armazenará o token de
forma protegida e descobrirá os servidores disponíveis para a conta. O usuário
selecionará o servidor e quais usuários serão monitorados.

As sessões serão consultadas pela API do Plex Media Server. O plugin deve preservar
a identidade do usuário, dispositivo e sessão para evitar atribuir a reprodução de
outra pessoa ao perfil local incorreto.

### Configurações dinâmicas

Cada plugin fornece um `SettingsSchema` declarativo com campos tipados, rótulos,
descrições, valores padrão, validações e indicação de segredo. A apresentação
renderiza os tipos genéricos; validação, autenticação e persistência específica
permanecem no plugin/serviço de aplicação.

Segredos, especialmente tokens Plex, não devem aparecer em logs nem ser devolvidos
à QML após o armazenamento. A tela deve oferecer ações específicas como conectar,
desconectar, testar conexão e selecionar usuários.

## Componentes propostos

### Domínio

- `PlaybackSession`: identidade do player, item, usuário, estado e timestamps.
- `EpisodeCandidate`: série, temporada, episódio e confiança do reconhecimento.
- `RecognitionRecord`: resultado, origem, estado, correção e timestamps.
- regra configurável de duração mínima para conclusão.

### Aplicação

- `EpisodeRecognitionService`: coordena plugins e normaliza candidatos.
- `PlaybackObservationService`: mantém temporizadores e invalida sessões encerradas
  antes do limite.
- `ProgressDecisionService`: verifica se o episódio é inédito e produz a alteração.
- `RecognitionHistoryService`: persiste reconhecimentos, correções, ignorados e erros.
- `PluginSettingsService`: administra schemas, valores, segredos e testes.
- `AniListSyncService`: recebe alterações locais e sincroniza de forma desacoplada.

### Infraestrutura

- adapter de named pipe/JSON IPC do mpv;
- cliente HTTP/autenticação/discovery do Plex;
- armazenamento seguro de credenciais;
- repositórios SQLite para histórico e configurações;
- implementações dos plugins mpv e Plex.

### Apresentação

- `PluginSettingsController` para a tela dinâmica de plugins;
- `EpisodeRecognitionController` para estado atual e histórico;
- modelos QML para campos de configuração e registros de reconhecimento.

QML não deve interpretar dados de players, aplicar o temporizador, decidir progresso
ou chamar AniList diretamente.

## Fluxo de reconhecimento

1. Um plugin inicia ou observa uma sessão.
2. O serviço normaliza título, usuário, episódio e fonte.
3. O sistema cria uma observação pendente.
4. O temporizador acumula apenas enquanto a sessão permanece válida e o processo/API
   confirma que a reprodução continua disponível.
5. Ao atingir o tempo configurado, o serviço consulta o progresso local.
6. Se o episódio já foi assistido, registra o reconhecimento sem alterar progresso.
7. Se ainda não foi assistido, atualiza o progresso local e registra o histórico.
8. A sincronização com AniList é enfileirada ou executada conforme o mecanismo de
   sincronização existente.
9. Se a sessão desaparecer antes do limite, o candidato é descartado ou registrado
   como incompleto, sem marcar o episódio.

## Conflitos e correções

- Baixa confiança não altera progresso automaticamente.
- Sessões de usuários não selecionados pelo usuário são ignoradas.
- O mesmo episódio/sessão não deve gerar múltiplas alterações idênticas.
- ~~Reconhecimentos incorretos permanecem corrigíveis no histórico.~~
- ~~A correção deve permitir escolher o anime/episódio correto ou ignorar o registro.~~
- **Decisão do usuário (2026-10-17, após a validação):** o histórico não oferece Corrigir nem
  Ignorar. A única ação é **Excluir**: remove o registro do histórico local e descarta da fila do
  AniList a alteração de progresso que ele enfileirou e que ainda não foi enviada. O progresso local
  não é alterado. Um reconhecimento errado é desfeito editando o progresso do anime manualmente.
- Falhas no AniList não desfazem a alteração local; ficam pendentes para retry e
  devem ser visíveis como erro de sincronização.

## Testes e critérios de aceitação

Testes unitários devem cobrir normalização, temporizador, encerramento antecipado,
episódio já assistido, filtragem por usuário, deduplicação, configuração dinâmica e
falhas de autenticação/conexão. Testes de integração devem usar doubles para Plex,
IPC e AniList, sem depender de contas ou players reais.

O recurso estará pronto quando for possível demonstrar que:

- o HaikenAnime inicia mpv com IPC e reconhece um episódio;
- o fechamento antes de três minutos não altera progresso;
- o tempo configurado altera o limite sem recompilação;
- uma sessão Plex selecionada é reconhecida;
- sessões de usuários não selecionados são ignoradas;
- episódios inéditos são atualizados uma única vez;
- histórico e falhas de sincronização são persistidos;
- autenticação Plex e configurações podem ser administradas pela tela genérica;
- a aplicação continua abrindo mesmo quando Plex ou mpv estão indisponíveis.

## Riscos e limites conhecidos

- A execução real do fluxo Plex depende de um servidor e conta disponíveis para
  validação manual posterior.
- O mpv só é reconhecível de modo confiável quando iniciado pelo HaikenAnime com IPC.
- Tokens Plex exigem armazenamento seguro e cuidado para não vazarem em logs.
- O reconhecimento de títulos vindos de arquivos/servidores pode exigir regras de
  normalização e associação com a biblioteca local.
- A API Plex pode expor sessões conforme permissões da conta; a seleção de usuários
  deve tratar usuários ausentes ou não autorizados como estado normal.

