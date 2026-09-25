# Planejamento de melhorias futuras

## Frontend e integração com o backend

- Implementar inicialmente uma única tela QML, mantendo a organização e o comportamento geral da V1.
- Manter a primeira etapa sem conexão, autenticação, persistência ou ações funcionais no frontend.
- Representar os estados de carregamento, vazio e erro desde a primeira versão visual.
- Criar uma camada de integração entre a apresentação e os casos de uso do backend.
- Criar controllers ou view models específicos para cada tela, evitando um novo objeto central monolítico.
- Fazer a camada de integração determinar quais dados serão exibidos, incluindo ordenação, filtragem, separação e agrupamento.
- Manter o QML responsável apenas pela renderização dos dados e estados recebidos.
- Integrar inicialmente somente dados de leitura já salvos no banco local.
- Validar a integração com testes controlados antes de adicionar chamadas externas ou operações de escrita.
- Evoluir o frontend de forma intercalada com o backend após o primeiro fluxo de leitura funcionar.
- Avaliar melhorias visuais adicionais somente depois que a tela tiver o funcionamento básico validado.

Este arquivo registra melhorias, endurecimentos e integrações que não fazem parte do fluxo mínimo do teste inicial, mas devem ser considerados antes de tratar a integração com o AniList como pronta para produção.

## Integração com o AniList

- Manter fixtures como provider ativo durante as próximas etapas. A conexão real com o AniList será uma das últimas entregas; parsers e mappers devem ser exercitados com respostas controladas até lá, sem apresentar esses testes como validação do endpoint oficial.
- Substituir o provider baseado em fixture pelo provider GraphQL real.
- Consolidar o transporte de update em uma única mutation `SaveMediaListEntry` por mídia. O contrato `IAniListUpdateClient` já recebe os campos agrupados por mídia, mas a implementação inicial ainda utiliza internamente as mutations específicas existentes. O agrupamento atual prepara o contrato, mas não garante atomicidade: mutations separadas podem aplicar apenas parte dos campos. A consolidação deverá ser feita quando o contrato remoto for validado, junto com idempotência e reconciliação da outbox.
- Normalizar a nomenclatura das funções de leitura do `AniListMediaMapper`. Fixture e GraphQL possuem formatos diferentes e justificam funções separadas, enquanto o mapeamento comum para domínio evita duplicação; a mudança de estilo não deve ser apresentada como validação da integração real.
- Utilizar o endpoint oficial configurável do AniList.
- Confirmar e implementar as queries GraphQL definitivas para anime, manga e novels.
- Usar `POST` com `query` e `variables`, seguindo o contrato oficial da API.
- Validar o formato real das respostas, incluindo `data`, `errors`, `Page`, `media` e `pageInfo`.
- Confirmar as mutations contra o schema e a API real antes de ativar updates. A query de catálogo já teve seus campos de leitura conferidos no schema oficial; `synonyms` foi corrigido para o nível de `Media`, e `type`/`format` permanecem separados.
- Implementar autenticação OAuth completa quando o fluxo de login for definido.
- Implementar renovação, invalidação e atualização segura de tokens.
- Adicionar tratamento de rate limit, backoff e retry controlado.
- Definir timeouts, limites de payload e política para indisponibilidade do serviço.
- Introduzir cancelamento ponta a ponta no caso de uso, em `IMediaDataSource`, em `IAniListUpdateClient` e no transporte HTTP, chegando a `QNetworkReply::abort()`. Um `bool cancelled` apenas no serviço daria uma falsa garantia, pois não interromperia `fetchPage` ou `updateMedia` bloqueados.
- Versionar ou identificar as queries GraphQL usadas pela aplicação.
- Substituir o transporte síncrono atual em conjunto com a revisão de concorrência, scheduler, rate limit e cancelamento ponta a ponta. `QEventLoop` aninhado permite reentrância, `QThread::msleep` bloqueia a thread chamadora e retry recursivo mistura resiliência com transporte; os testes por fixture não exercitam esses custos.

## Secrets e segurança

- Substituir o `FileSecretStore` por armazenamento no SQLite. **Contrato `ISecretStore` já preparado; implementação pendente.**
- Manter explícito que `QSaveFile` oferece atomicidade de gravação, não proteção do conteúdo. O `FileSecretStore` evita arquivos parcialmente escritos, mas username e token continuam em texto puro e não devem ser tratados como armazenamento seguro de produção.
- Adicionar ao armazenamento de secrets uma operação explícita para apagar ou invalidar credenciais, necessária para logout e revogação segura.
- Criptografar os secrets em repouso.
- Definir como a chave de criptografia será protegida no Windows.
- Evitar que tokens apareçam em logs, dumps, mensagens de erro ou arquivos temporários.
- Definir permissões restritas para o arquivo ou banco que armazenar credenciais.
- Criar migração dos secrets do arquivo texto para o armazenamento seguro.
- Remover ou invalidar o arquivo texto após a migração, mediante uma estratégia segura.
- Avaliar proteção adicional usando recursos nativos do sistema operacional.

## Persistência

- Tornar explícitas as regras de ownership entre dados de catálogo, entrada da lista e dados do usuário. Atualizações remotas não devem depender apenas de uma lista implícita de colunas preservadas pela query SQL.
- Avaliar a separação de `Media` em modelos ou agregados com ciclos de vida próprios para catálogo, progresso e estado da lista antes de ampliar os fluxos de escrita.
- Finalizar o schema que separa catálogo externo e dados controlados pelo usuário.
- Adicionar identificador da fonte e identificador externo da mídia.
- Registrar data da última sincronização por mídia e por execução.
- Criar tabela para execuções de sincronização, status, falhas e páginas processadas.
- Substituir o schema hardcoded em um único método por migrações versionadas em arquivos SQL, com sequência explícita (`1 -> 2 -> 3`), checksum e rejeição de versões futuras. A validação atual garante apenas que a versão declarada foi realmente registrada; ela ainda não constitui um mecanismo completo de evolução do schema.
- Garantir foreign keys, `WAL`, `busy_timeout` e transações conforme o uso real.
- Definir regras completas para não sobrescrever progresso, nota e arquivos locais do usuário.
- Implementar limpeza ou reconciliação de mídias removidas da lista remota.
- Criar índices para os filtros e buscas mais frequentes.

## Sincronização

- Completar o `AniListSyncService` para o fluxo real, preservando o caminho determinístico por fixture já implementado.
- A sincronização inicial durante a inicialização já existe no fluxo por fixture; reavaliá-la ao ativar autenticação, cancelamento e rede real.
- Verificar alterações pendentes durante a sincronização inicial antes de considerar o estado sincronizado.
- Reprocessar alterações pendentes na próxima execução caso não seja possível concluí-las durante a inicialização.
- Definir, junto com o scheduler e a política de retry, uma leitura global ou em lotes da outbox. O contrato atual `getPending(mediaId)` serve ao fluxo por mídia, mas não permite que um worker descubra sozinho todas as pendências prontas para nova tentativa.
- O scheduler recorrente e o intervalo por `Settings.json` já existem para o fluxo local; evoluí-los com persistência, retry, rate limit e cancelamento antes do uso real.
- Aplicar filtros por usuário, tipo, lista e status na query GraphQL.
- Permitir sincronização geral na primeira execução.
- Persistir cada página de forma idempotente.
- Retomar uma sincronização interrompida a partir da última página confirmada.
- Definir se a sincronização será incremental ou sempre completa.
- Adicionar modo de pré-visualização antes de persistir alterações.
- Definir conflitos entre dados remotos e dados locais.
- Adicionar cancelamento cooperativo entre páginas e requisições.
- Emitir progresso detalhado sem expor informações sensíveis.

## Regras de merge do update

As regras de merge serão definidas por campo ou grupo de campos, e não por uma política única para toda a mídia.

- Avaliar a substituição de `AniListField + AniListFieldValue` por comandos de update tipados. A validação antecipada atual impede persistir combinações impossíveis, mas duplica na factory parte do conhecimento mantido pelo update client; tipos próprios podem tornar estados inválidos irrepresentáveis.
- Adicionar um teste de integração do caso de uso que prove o estado final conjunto de dados remotos, valores locais, outbox e persistência. Testes unitários do resolver comprovam a tabela de políticas, mas não comprovam que o fluxo real a aplica a todos os campos.

### Dados pertencentes ao AniList

- Título, títulos alternativos, capa, descrição e demais informações de catálogo serão atualizados pelo valor recebido do AniList.
- Alterações locais nesses campos não terão prioridade sobre o estado remoto.

### Dados pertencentes ao usuário

- Progresso, nota pessoal e outros dados de consumo serão tratados como dados locais.
- Alterações locais serão colocadas em uma fila de sincronização.
- A fila será processada após a sincronização do estado remoto.
- O valor local será descartado ou enviado ao AniList conforme a regra específica do campo.
- Para progresso, o maior valor deverá ser utilizado por padrão.

### Alterações de lista/status

- Alterações locais terão prioridade sobre alterações remotas.
- Exemplo: se o AniList indicar `PLAN TO WATCH` e o estado local indicar `WATCHING`, o estado local `WATCHING` será preservado e enviado ao AniList quando aplicável.
- A regra deverá considerar a alteração pendente local antes de removê-la da fila.

### Exclusões

- Exclusões remotas não serão aplicadas automaticamente.
- O sistema deverá registrar a exclusão como pendência que exige confirmação do usuário.
- A confirmação deverá ser persistida antes de remover a mídia ou seus dados locais.
- Uma configuração futura poderá permitir aceitar exclusões automaticamente; essa opção permanece fora do escopo inicial.

## Scheduler

- O scheduler deverá ser iniciado junto com o programa após a sincronização inicial.
- O intervalo será lido do `Settings.json`.
- O scheduler não deverá iniciar uma nova sincronização enquanto outra estiver em execução.
- Falhas não deverão bloquear as próximas execuções programadas.
- O estado da última execução e da próxima tentativa deverá estar disponível para diagnóstico.
- Decidir se download/persistência remota e publicação da outbox serão casos de uso separados. O ponteiro opcional do processor faz `synchronize` representar os dois fluxos e mistura seus resultados: o download pode terminar com sucesso e o serviço ainda reportar falha por causa de uma mutation posterior.

## Política de logs

- Definir uma abstração de logging para que application, infrastructure e workers não dependam diretamente de uma implementação concreta.
- Definir quais camadas podem gerar logs e quais devem retornar erros por contrato.
- Separar níveis de log, como diagnóstico, informação, aviso e erro.
- Definir destino, formato e ciclo de vida dos arquivos de log.
- Evitar que tokens, secrets, payloads sensíveis ou dados pessoais sejam registrados.
- Permitir que a configuração de logging seja controlada sem espalhar dependências pela aplicação.
- Definir como logs de rede, sincronização, mutations, retry e scheduler serão correlacionados.
- Substituir erros descritos apenas por `QString` por um resultado estruturado com categoria, mensagem segura para apresentação e detalhe técnico opcional. A mudança deve ser coordenada entre infraestrutura, casos de uso, logging e apresentação para eliminar classificação baseada no texto da mensagem.
- Adicionar rotação, limite de tamanho e política de retenção dos arquivos.
- Testar logging sem acoplar os testes de domínio a filesystem ou Qt Network.

## Scheduler para listas grandes

- Revisar o scheduler atual antes de definir uma frequência fixa para a sincronização completa.
- Considerar que uma lista grande pode tornar uma sincronização integral frequente custosa ou desnecessária.
- Avaliar particionamento por lista/status e tipo de mídia, como anime, manga e novel.
- Definir se cada partição terá seu próprio intervalo, prioridade e estado de última execução.
- Permitir que partições falhas sejam retomadas sem repetir toda a sincronização.
- Avaliar sincronização incremental, paginação por partição e limites por execução.
- Evitar execuções simultâneas da mesma partição e controlar concorrência entre partições.
- Considerar uma sincronização completa inicial e ciclos posteriores menores e direcionados.
- Permitir configuração futura de prioridade, frequência e ativação por lista/tipo no `Settings.json`.
- Registrar métricas de duração, páginas processadas, itens atualizados e falhas por partição.
- Definir como alterações pendentes do usuário serão priorizadas em relação às tarefas agendadas.

## Threading e ciclo de vida

- Aplicar as evoluções pendentes da política de encerramento, concorrência e ciclo de vida registrada em `review.md`; a ordem entre espera da sincronização, destruição das dependências e parada do logger já foi corrigida.
- Definir uma política para operações que não retornam durante shutdown. O coordenador agora impede novos trabalhos e aguarda a execução ativa, mas espera bloqueante não substitui cancelamento ponta a ponta nem oferece prazo máximo seguro para encerramento.
- Manter `AniListSyncWorker` e `AniListUpdateWorker` fora da composição até que scheduler, cancelamento e update real definam o ciclo completo. Integrá-los agora apenas daria aparência de uma arquitetura de workers persistentes sem resolver ownership. Nessa etapa deverá ser escolhida explicitamente uma thread curta por execução ou workers persistentes com event loop, ownership de worker, serviço, banco e transporte, e destruição na thread correta.
- Implementar o worker assíncrono definitivo.
- Garantir que `QNetworkAccessManager` seja criado e usado na thread de execução.
- Preservar a regra já aplicada de uma conexão SQLite própria por thread ao introduzir novos workers.
- Evoluir o encerramento seguro já implementado com cancelamento ponta a ponta e prazo máximo.
- Evitar chamadas bloqueantes na thread principal.
- Ampliar a exclusão mútua já existente por instância para múltiplos processos e recuperação após crash.
- Cobrir cancelamento, falha e destruição de objetos com testes.

## Qualidade e testes

- Avaliar o encapsulamento de `Media` depois de definir invariantes para identificador, progresso, total, próxima mídia, notas e ownership dos campos. Evitar adicionar getters e setters que não protejam nenhuma regra.
- Avaliar a criação de bibliotecas internas por camada no CMake depois que as fronteiras arquiteturais estiverem consolidadas. O objetivo é evitar recompilar as mesmas fontes em vários executáveis de teste e fazer aplicação e testes consumirem os mesmos módulos.
- Adicionar fixtures no formato real de respostas GraphQL.
- Testar respostas com `data: null` e `errors`.
- Testar campos opcionais, valores nulos e mudanças de schema.
- Reavaliar `season`, `seasonYear`, datas, volumes, URL pública, próximo episódio e streaming somente quando alguma tela ou regra de negócio passar a consumi-los.
- Adicionar testes do provider GraphQL com um transporte controlado.
- Adicionar testes de paginação com múltiplas páginas.
- Adicionar testes de retry e rate limit.
- Adicionar testes de migração e recuperação de transação.
- Adicionar testes de concorrência e cancelamento.
- Executar CTest com o ambiente Qt corretamente configurado.
- Adicionar validação estática e sanitizadores quando o ambiente permitir.

## Interface e operação

- A integração inicial com `HomeScreenController` já existe; manter novos fluxos fora do QML e expô-los por controllers específicos.
- A Home já exibe estado, sucesso e erro. O progresso exposto ainda não recebe atualizações incrementais do coordenador; ligar esse fluxo sem misturar estado local e estado de sincronização.
- Substituir o estado textual da tela por um enum Qt registrado no meta-object quando o contrato estiver estabilizado. `QString` mantém o consumo QML simples e legível, mas não impede estados inválidos ou erros de digitação como `"loading"`.
- Separar o estado do conteúdo local do estado da sincronização. Hoje a mesma propriedade `state` faz uma falha de sincronização transformar a tela inteira em erro mesmo quando a biblioteca local continua disponível, e `loading` pode significar duas operações diferentes.
- Não usar textos traduzidos, como `statusLabel`, como identificadores para filtro ou agrupamento. Se uma tela futura precisar dessa semântica, o controller deverá preparar os grupos ou expor uma chave estável separada do texto apresentado.
- Evoluir `errorMessage` junto com o contrato estruturado de erros para separar mensagem pública e detalhe técnico reservado ao log. Enquanto readers e coordenadores retornarem apenas `QString`, o controller não consegue sanitizar erros com segurança sem voltar a classificar texto.
- Permitir configuração do tamanho da página e dos filtros.
- Informar quando o modo offline/fixture estiver sendo usado.
- Criar tela ou fluxo para configurar autorização do AniList.
- Adicionar opção para repetir uma sincronização que falhou.
- Registrar diagnóstico técnico sem registrar credenciais.

## Distribuição

- Incluir o plugin SQLite correto no pacote Windows.
- Incluir dependências do Qt Network quando a integração real for ativada.
- Verificar certificados e validação TLS no ambiente distribuído.
- Testar instalação limpa sem ferramentas de desenvolvimento disponíveis.
- Documentar configuração, autorização, backup e recuperação do banco.

## Critério para considerar a integração pronta

A integração somente deverá ser considerada pronta quando o provider GraphQL real, a autenticação segura, a persistência definitiva, o tratamento de falhas e os testes de rede/paginação estiverem implementados e validados separadamente do fixture local.

## Dívidas identificadas durante a revisão do core

As dívidas, seus motivos, trade-offs e a parte responsável por revisá-las são mantidos nas seções "Dívidas identificadas, mas não corrigidas nesta parte" de `review.md`, incluindo arquitetura, domínio, contratos, casos de uso, infraestrutura local, integração AniList, composição, concorrência, ciclo de vida, fronteira consumida pelo frontend, testes e estabilidade. Esta seção central mantém apenas a referência para evitar uma segunda lista exaustiva; as seções temáticas acima continuam registrando prioridades resumidas do produto. Para o item 7, a referência inclui shutdown potencialmente bloqueante, exclusão limitada à instância, scheduler sem persistência, workers ainda não compostos, transporte GraphQL síncrono, falhas silenciosas do logger, composição da operação dentro do coordenador e ausência de teste de fechamento real durante I/O externo.

## Pontos abertos do fluxo de sincronização

- Definir como tratar falhas parciais em mutations agrupadas por mídia. Inicialmente, a mídia inteira será considerada como falha.
- Avaliar se será necessário registrar o resultado individual de cada campo após os testes com mocks e a API real.
- Tornar atômicas as transições de status de cada grupo da outbox, definir recuperação de leases abandonados em `Processing` e estabelecer idempotência para o caso em que a mutation remota funciona, mas o acknowledgement local falha. Atualizações individuais de status não conseguem oferecer essas garantias.

## Contrato do update

O update deverá persistir a intenção local antes de enviar uma mutation, aplicar a política de merge específica do campo e somente finalizar a alteração após confirmação GraphQL sem erros. Falhas permanecem na outbox para retry; exclusões permanecem bloqueadas até confirmação do usuário.
