# Planejamento de melhorias futuras

Este arquivo registra melhorias, endurecimentos e integrações que não fazem parte do fluxo mínimo do teste inicial, mas devem ser considerados antes de tratar a integração com o AniList como pronta para produção.

## Integração com o AniList

- Substituir o provider baseado em fixture pelo provider GraphQL real.
- Utilizar o endpoint oficial configurável do AniList.
- Confirmar e implementar as queries GraphQL definitivas para anime, manga e novels.
- Usar `POST` com `query` e `variables`, seguindo o contrato oficial da API.
- Validar o formato real das respostas, incluindo `data`, `errors`, `Page`, `media` e `pageInfo`.
- Implementar autenticação OAuth completa quando o fluxo de login for definido.
- Implementar renovação, invalidação e atualização segura de tokens.
- Adicionar tratamento de rate limit, backoff e retry controlado.
- Definir timeouts, limites de payload e política para indisponibilidade do serviço.
- Versionar ou identificar as queries GraphQL usadas pela aplicação.

## Secrets e segurança

- Substituir o `FileSecretStore` por armazenamento no SQLite.
- Criptografar os secrets em repouso.
- Definir como a chave de criptografia será protegida no Windows.
- Evitar que tokens apareçam em logs, dumps, mensagens de erro ou arquivos temporários.
- Definir permissões restritas para o arquivo ou banco que armazenar credenciais.
- Criar migração dos secrets do arquivo texto para o armazenamento seguro.
- Remover ou invalidar o arquivo texto após a migração, mediante uma estratégia segura.
- Avaliar proteção adicional usando recursos nativos do sistema operacional.

## Persistência

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
- Aplicar filtros por usuário, tipo, lista e status na query GraphQL.
- Permitir sincronização geral na primeira execução.
- Persistir cada página de forma idempotente.
- Retomar uma sincronização interrompida a partir da última página confirmada.
- Definir se a sincronização será incremental ou sempre completa.
- Adicionar modo de pré-visualização antes de persistir alterações.
- Definir conflitos entre dados remotos e dados locais.
- Adicionar cancelamento cooperativo entre páginas e requisições.
- Emitir progresso detalhado sem expor informações sensíveis.

## Threading e ciclo de vida

- Implementar o worker assíncrono definitivo.
- Garantir que `QNetworkAccessManager` seja criado e usado na thread de execução.
- Garantir uma conexão SQLite própria para cada thread que acessar o banco.
- Implementar encerramento seguro durante fechamento da aplicação.
- Evitar chamadas bloqueantes na thread principal.
- Definir política para executar apenas uma sincronização por vez.
- Cobrir cancelamento, falha e destruição de objetos com testes.

## Qualidade e testes

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
