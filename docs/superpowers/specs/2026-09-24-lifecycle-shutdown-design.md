# Item 7 — composição, concorrência e ciclo de vida

## Objetivo

Tornar explícito e testável o ciclo de vida da sincronização em background sem implementar cancelamento parcial nem substituir o provider baseado em fixtures. O encerramento deve impedir novas execuções, aguardar com segurança o trabalho já iniciado e preservar o logger até que a sincronização deixe de utilizá-lo.

## Escopo

Esta etapa cobre:

- `ApplicationContext` e sua ordem de ownership;
- `main.cpp` e a sequência de encerramento;
- `InitialSyncCoordinator`, seu scheduler e sua `QThread`;
- a interação do coordenador com `AsyncLogger`;
- workers de sincronização e update atualmente não usados pela composição;
- testes unitários do contrato de ciclo de vida;
- documentação pedagógica em `review.md` e registro das dívidas em `planning.md`.

Esta etapa não cobre:

- conexão real com o AniList;
- cancelamento de uma operação já bloqueada dentro do data source;
- mudança do transporte GraphQL síncrono para assíncrono;
- scheduler persistente ou política definitiva de retry;
- execução real das mutations de update.

## Estado atual e problema

`InitialSyncCoordinator` cria uma `QThread` por execução. A lambda da thread captura o coordenador por ponteiro, constrói banco, repositories, fixture e serviço dentro da própria thread, e agenda o resultado de volta na thread principal.

O destructor aguarda a thread terminar. Essa espera evita destruir o coordenador enquanto a lambda ainda o utiliza, mas não constitui uma política completa de shutdown:

- o scheduler não é parado explicitamente antes da espera;
- não existe estado `stopping` que impeça novos `start()`;
- o callback de conclusão pode tentar reativar o scheduler durante o encerramento;
- `main.cpp` para o logger em `aboutToQuit` antes de o `ApplicationContext` destruir o coordenador e aguardar a sincronização;
- esperar uma thread não cancela o trabalho que ela executa.

O último ponto é deliberadamente preservado. Os contratos atuais não transportam cancelamento até o filesystem, SQLite ou HTTP. Adicionar apenas uma flag ao coordenador criaria uma garantia falsa.

## Desenho escolhido

### Estado e API do coordenador

`InitialSyncCoordinator` receberá uma operação de sincronização injetável para testes e manterá sua operação de produção como default. A operação deve ser criada/executada dentro da thread de trabalho, preservando a regra de ownership do SQLite.

O coordenador terá uma operação pública de `shutdown()` e estados observáveis internamente:

- ocioso: pode iniciar;
- executando: ignora solicitações concorrentes de início;
- encerrando: não aceita novas execuções nem reinicia o scheduler.

`shutdown()` será idempotente. Ele deverá:

1. marcar o coordenador como encerrando;
2. parar o scheduler;
3. aguardar a thread ativa terminar;
4. liberar o objeto `QThread` concluído;
5. deixar o coordenador permanentemente incapaz de iniciar novo trabalho.

O destructor chamará `shutdown()` como rede de segurança. O encerramento normal continuará sendo explícito em `main.cpp`, permitindo controlar a ordem relativa ao logger.

### Sequência da aplicação

Ao receber `aboutToQuit`, `main.cpp` deverá:

1. chamar `initialSync->shutdown()` quando o coordenador existir;
2. somente depois chamar `logger->stop()`.

Assim, qualquer mensagem emitida durante a finalização da sincronização ainda pode entrar na fila do logger. Depois disso, `AsyncLogger::stop()` drena a fila e aguarda sua própria thread.

Os membros de `ApplicationContext` continuarão em uma ordem cuja destruição inversa elimina primeiro o coordenador e depois suas dependências. O shutdown explícito não substitui a ordem RAII; os dois mecanismos se complementam.

### Callback de conclusão

O callback enfileirado na thread principal emitirá `completed` ou `failed`. Ele só reiniciará o scheduler quando o coordenador não estiver encerrando. Eventos destinados a um `QObject` destruído são removidos pelo Qt, mas o desenho não dependerá disso para controlar o scheduler.

### Logger

`AsyncLogger` continuará com sua thread própria, fila limitada e encerramento bloqueante que drena as entradas. A revisão explicará que `thread_.quit()` não interrompe `run()`: quem encerra o loop é `stopping_` acompanhado de `QWaitCondition`.

Esta etapa não redesenhará o logger como `QObject` movido para outra thread. O padrão atual possui sincronização interna própria e pode ser endurecido separadamente quando forem definidos diagnóstico de falhas de escrita e política de descarte.

### Workers não compostos

`AniListSyncWorker` e `AniListUpdateWorker` serão documentados como abstrações atualmente não conectadas ao fluxo real. Não serão integrados artificialmente nesta etapa: seus serviços são referências não-owning, e usá-los corretamente exigiria definir quem cria e destrói serviços, banco, client e worker em cada thread.

## Testes

Os testes usarão uma operação controlada e real, sem mockar `QThread`. Eles deverão provar:

- duas chamadas de `start()` durante uma execução produzem somente uma operação;
- `shutdown()` aguarda a operação ativa;
- depois de `shutdown()`, `start()` não cria novo trabalho;
- chamar `shutdown()` mais de uma vez é seguro;
- concluir durante o shutdown não reativa o scheduler.

Cada correção de comportamento será precedida por um teste observado em falha. A suíte completa continuará sendo executada ao final.

## Trade-offs

Injetar uma operação pequena torna o coordenador testável sem abrir SQLite real nem depender de tempo arbitrário. O custo é adicionar uma seam de composição que existe principalmente para separar política de execução de trabalho concreto. Ela permanece legítima em produção porque também esclarece que o coordenador agenda trabalho, mas não precisa conhecer sua implementação.

O shutdown ainda pode bloquear indefinidamente se a operação interna nunca retornar. Isso é mais seguro do que destruir uma `QThread` ativa, mas não é cancelamento. A solução definitiva depende do token de cancelamento ponta a ponta já registrado no planejamento.

Criar uma nova `QThread` por execução é simples e compatível com a fixture. Reutilizar uma thread com event loop reduziria criação de threads, mas introduziria ownership de workers e objetos thread-affine antes de os contratos de cancelamento e scheduler estarem definidos.

## Documentação resultante

`review.md` receberá uma seção pedagógica para o item 7, explicando composição, RAII, afinidade de thread, SQLite por thread, shutdown, logger, scheduler e workers não utilizados.

`planning.md` receberá:

- os pontos solicitados da revisão do item 6;
- referência às dívidas não corrigidas do item 7;
- cancelamento ponta a ponta e limite da espera bloqueante;
- evolução futura do scheduler e do transporte;
- decisão futura sobre workers persistentes e ownership de serviços.

## Critérios de conclusão

- A fixture continua sendo o provider ativo.
- A aplicação encerra a sincronização antes do logger.
- O coordenador não inicia trabalho após shutdown.
- O scheduler não é reativado durante shutdown.
- A espera por trabalho ativo é explicitamente diferenciada de cancelamento.
- Os testes de ciclo de vida e a suíte completa passam.
- `review.md` e `planning.md` refletem limites, trade-offs e dívidas sem afirmar capacidades ainda inexistentes.
