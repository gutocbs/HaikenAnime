# Logging

## Objetivo

Registrar o fluxo de inicialização, carregamento de configurações, SQLite, queries e sincronização sem bloquear a aplicação.

## Destino e retenção

- Diretório: `%APPDATA%/HaikenAnime/logs/`.
- Arquivo diário: `haikenanime-YYYY-MM-DD.log`.
- Cada execução adiciona um separador de início no arquivo do dia.
- A retenção é configurada por `logging.retentionDays` em `Settings.json`.
- O valor padrão é 7 dias; arquivos antigos são removidos na inicialização do logger.

## Formato

```text
2026-09-23T16:00:00.123 [ERROR] [QueryStore] Could not open SQL query file.
```

Os níveis e categorias são definidos pelos enums `LogLevel` e `LogCategory`. A conversão para `INFO`, `WARN`, `ERROR` e os nomes das categorias ocorre exclusivamente em `toLogString`, evitando valores textuais arbitrários nas chamadas.

## Operação assíncrona

O logger usa uma fila protegida e uma thread dedicada para gravar os eventos. A fila tem capacidade limitada; quando cheia, o evento mais antigo é descartado para que o fluxo principal não fique bloqueado. O encerramento tenta processar os eventos pendentes por até dois segundos.

Falhas do logger não interrompem a aplicação e não geram recursão de logging.

## Eventos permitidos

- Início e fim da composição da aplicação.
- Abertura e migração do banco.
- Leitura e validação da configuração de queries.
- Falhas de leitura dos arquivos SQL/GraphQL.
- Início e falha da sincronização.

## Dados proibidos

Não registrar credenciais, tokens, headers, payloads completos, conteúdo integral de queries, dados pessoais ou conteúdo completo de mídia.

## Responsabilidades

Os logs devem ser adicionados somente em componentes que conhecem o contexto da operação: composição da aplicação, coordenador de sincronização, banco, configuração/query stores e clientes de integração. Entidades, mappers, DTOs e classes de apresentação não devem conhecer o logger.
