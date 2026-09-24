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

- Substituir o provider baseado em fixture pelo provider GraphQL real.
- Consolidar o transporte de update em uma única mutation `SaveMediaListEntry` por mídia. O contrato `IAniListUpdateClient` já recebe os campos agrupados por mídia, mas a implementação inicial ainda utiliza internamente as mutations específicas existentes.
- Utilizar o endpoint oficial configurável do AniList.
- Confirmar e implementar as queries GraphQL definitivas para anime, manga e novels.
- Usar `POST` com `query` e `variables`, seguindo o contrato oficial da API.
- Validar o formato real das respostas, incluindo `data`, `errors`, `Page`, `media` e `pageInfo`.
- Implementar autenticação OAuth completa quando o fluxo de login for definido.
- Implementar renovação, invalidação e atualização segura de tokens.
- Adicionar tratamento de rate limit, backoff e retry controlado.
- Definir timeouts, limites de payload e política para indisponibilidade do serviço.
- Introduzir cancelamento ponta a ponta no caso de uso, em `IMediaDataSource`, em `IAniListUpdateClient` e no transporte HTTP, chegando a `QNetworkReply::abort()`. Um `bool cancelled` apenas no serviço daria uma falsa garantia, pois não interromperia `fetchPage` ou `updateMedia` bloqueados.
- Versionar ou identificar as queries GraphQL usadas pela aplicação.

## Secrets e segurança

- Substituir o `FileSecretStore` por armazenamento no SQLite. **Contrato `ISecretStore` já preparado; implementação pendente.**
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
- Implementar migrações versionadas em arquivos SQL.
- Garantir foreign keys, `WAL`, `busy_timeout` e transações conforme o uso real.
- Definir regras completas para não sobrescrever progresso, nota e arquivos locais do usuário.
- Implementar limpeza ou reconciliação de mídias removidas da lista remota.
- Criar índices para os filtros e buscas mais frequentes.

## Sincronização

- Implementar o `AniListSyncService` completo.
- Executar uma sincronização inicial durante a inicialização do programa.
- Verificar alterações pendentes durante a sincronização inicial antes de considerar o estado sincronizado.
- Reprocessar alterações pendentes na próxima execução caso não seja possível concluí-las durante a inicialização.
- Definir, junto com o scheduler e a política de retry, uma leitura global ou em lotes da outbox. O contrato atual `getPending(mediaId)` serve ao fluxo por mídia, mas não permite que um worker descubra sozinho todas as pendências prontas para nova tentativa.
- Adicionar schedulers para sincronizações recorrentes.
- Tornar o intervalo do scheduler configurável pelo `Settings.json`.
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

- Revisar a política completa de encerramento, concorrência e ciclo de vida registrada em `review.md`, incluindo a ordem entre cancelamento dos workers, espera das threads, destruição das dependências e parada do logger.
- Implementar o worker assíncrono definitivo.
- Garantir que `QNetworkAccessManager` seja criado e usado na thread de execução.
- Garantir uma conexão SQLite própria para cada thread que acessar o banco.
- Implementar encerramento seguro durante fechamento da aplicação.
- Evitar chamadas bloqueantes na thread principal.
- Definir política para executar apenas uma sincronização por vez.
- Cobrir cancelamento, falha e destruição de objetos com testes.

## Qualidade e testes

- Avaliar o encapsulamento de `Media` depois de definir invariantes para identificador, progresso, total, próxima mídia, notas e ownership dos campos. Evitar adicionar getters e setters que não protejam nenhuma regra.
- Avaliar a criação de bibliotecas internas por camada no CMake depois que as fronteiras arquiteturais estiverem consolidadas. O objetivo é evitar recompilar as mesmas fontes em vários executáveis de teste e fazer aplicação e testes consumirem os mesmos módulos.
- Adicionar fixtures no formato real de respostas GraphQL.
- Testar respostas com `data: null` e `errors`.
- Testar campos opcionais, valores nulos e mudanças de schema.
- Adicionar testes do provider GraphQL com um transporte controlado.
- Adicionar testes de paginação com múltiplas páginas.
- Adicionar testes de retry e rate limit.
- Adicionar testes de migração e recuperação de transação.
- Adicionar testes de concorrência e cancelamento.
- Executar CTest com o ambiente Qt corretamente configurado.
- Adicionar validação estática e sanitizadores quando o ambiente permitir.

## Interface e operação

- Integrar o serviço com controllers da camada de apresentação.
- Exibir estado, progresso, sucesso e erro da sincronização no QML.
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

As dívidas, seus motivos, trade-offs e a parte responsável por revisá-las são mantidos nas seções "Dívidas identificadas, mas não corrigidas nesta parte" de `review.md`, incluindo arquitetura, domínio, contratos e casos de uso. Este planejamento mantém apenas essa referência para evitar duplicação e divergência entre duas listas.

## Pontos abertos do fluxo de sincronização

- Definir como tratar falhas parciais em mutations agrupadas por mídia. Inicialmente, a mídia inteira será considerada como falha.
- Avaliar se será necessário registrar o resultado individual de cada campo após os testes com mocks e a API real.

## Contrato do update

O update deverá persistir a intenção local antes de enviar uma mutation, aplicar a política de merge específica do campo e somente finalizar a alteração após confirmação GraphQL sem erros. Falhas permanecem na outbox para retry; exclusões permanecem bloqueadas até confirmação do usuário.
