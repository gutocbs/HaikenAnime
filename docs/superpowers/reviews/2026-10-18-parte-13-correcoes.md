# Report — Parte 13: Correções

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `06c3c71` (`feature/revisao-geral`)
- **Status:** aguardando aprovação — nenhuma alteração aplicada

Esta é a única parte que **muda comportamento**. Cada correção começa por um teste que falha e vai num commit
`fix:` próprio (teste + correção). Onde não há como escrever teste automatizado (script de build,
variável de ambiente do CTest, texto de documentação), a validação é dita explicitamente.

---

## 1. Itens

### A. Build e ambiente

| ID | Origem | Problema | Correção proposta | Validação |
| --- | --- | --- | --- | --- |
| **C13-01** | Parte 1 | `QT_QUICK_BACKEND=software` está **dentro** do `PATH` no `ENVIRONMENT` dos testes de QML, então nunca é definido | `ENVIRONMENT` como lista de duas variáveis (`PATH=…;QT_QUICK_BACKEND=software`, com o separador de lista do CMake) | `ctest -V` mostra a variável; testes de QML verdes |
| **C13-02** | Parte 1 | `build-release.ps1` só carrega o ambiente do MSVC quando não acha `cl.exe`; com `cl.exe` no `PATH` sem `INCLUDE`/`LIB`, o build falha | Mesmo critério do `validate.ps1`: carregar quando `VCToolsInstallDir` ou `INCLUDE` estiverem vazios | o revisor roda o script num terminal comum e num Developer PowerShell |
| **C13-03** | Parte 1 | `Settings.json` usa `Q:\` como pasta padrão da biblioteca | Default **vazio** (o usuário escolhe a pasta em Configurações); ver decisão 1 | `JsonSettingsReaderTests` / `ApplicationCompositionTests` |

### B. Erros e logs que somem ou enganam

| ID | Origem | Problema | Correção proposta |
| --- | --- | --- | --- |
| **C13-04** | Parte 3 | Falha de migração troca o erro específico por uma mensagem genérica que cita "versions 1 through 15" | Manter a mensagem da etapa que falhou (versão + erro do SQLite) |
| **C13-05** | Parte 3 | Os repositórios releem o `.sql` a cada operação, e o `SqlQueryStore` loga `INFO` a cada leitura (log poluído, I/O à toa) | `SqlQueryStore` guarda em memória o texto já lido (os recursos `qrc` não mudam) e loga só a primeira leitura de cada query |
| **C13-06** | Parte 4 | `SqliteMediaRepository::upsert` sai sem `rollback` se o `prepare` falhar (transação aberta) | Mesmo guarda RAII de transação do repositório de arquivos locais |
| **C13-07** | Partes 4 e 7 | `SqliteUserPreferencesRepository::replace` e `SqliteLocalFileRepository::upsertBatch`/`completeScan`/`failScan` sobrescrevem o erro do `SqlQueryStore` com `query.lastError()` (vazio) | Separar "carregar query" de "preparar", preservando o erro do primeiro |
| **C13-08** | Parte 7 | `SqliteLocalEpisodeReader::readAvailableEpisodeCount` devolve `false` sem mensagem (`mediaId` inválido ou sem linha) | Mensagens explícitas, como já faz `readNextEpisode` |
| **C13-09** | Parte 8 | `CoverDownloadCoordinator` ignora sem log a falha ao ler o cache (construtor) e ao remover a entrada (`ReportMissingFile`) | Logar como *warning* |
| **C13-10** | Parte 12 | `createApplicationContext` descarta o erro de `AniListAuthManager::load` | Logar como *warning* (sem dados da credencial) |
| **C13-11** | Parte 9 | Avisos do reconhecimento adicionados **depois** da inicialização nunca aparecem (falha ao abrir o mpv, falha assíncrona ao selecionar o servidor Plex) | `RecognitionContext` recebe uma função de aviso (`LogSink` de *warning*) e loga na hora; a lista `warnings` sai |

### C. Comportamento visível

| ID | Origem | Problema | Correção proposta |
| --- | --- | --- | --- |
| **C13-12** | Parte 2 | O intervalo de sincronização das Configurações (`synchronizationIntervalMs`) é salvo, mas o agendador não usa | O valor salvo vira o `normalInterval` da política `UserList`, na inicialização e ao salvar as Configurações; `PendingChanges` continua com 1 min (ver decisão 2) |
| **C13-13** | Revisor (P10) | Cards de mangá e light novel mostram "Assistindo" | **Causa:** em `HaikenAnime_pt_BR.ts`, o texto do card "Em andamento" (`HomeMediaModel`) foi traduzido como "Assistindo" para qualquer tipo. **Correção:** o card passa a usar "Assistindo" para anime e "Lendo" para mangá/novel já no código, e a tradução pt-BR volta a ser a identidade |
| **C13-14** | Revisor (P11) | Ao abrir os detalhes de um anime durante a sync, os dados somem quando ela termina | **Causa:** `HomeScreenController::Reload()`, chamado ao fim da sync, sempre chama `clearSelection()`. **Correção:** depois de recarregar, a seleção é reaplicada com os dados novos da mesma mídia; só é limpa se a mídia saiu da biblioteca ou do tipo ativo |
| **C13-15** | Revisor (P11) | Não há como abrir a pasta de logs pela interface | `SettingsController::OpenLogsFolder()` abre `AppData/HaikenAnime/logs` no explorador (abridor injetável para teste), com o botão "Abrir pasta de logs" em Configurações, à direita, no mesmo padrão dos outros botões (ver decisão 3) |
| **C13-16** | Revisor (P9) | Sessões encerradas antes do tempo mínimo viram registros `Incomplete` no histórico | Deixar de gravá-las: o *callback* `incomplete` só encerra a observação. Registros `Incomplete` já gravados ficam (ver decisão 4) |
| **C13-17** | Revisor (P9) | O histórico só mostra progresso reconhecido pelos players; as edições manuais de progresso não aparecem | Toda edição manual que **aumenta o progresso** (Home e catálogo) grava um registro com origem "manual" (status `MarkedWatched`, episódio = novo progresso), seguindo a sincronização com o AniList como os outros. É uma funcionalidade nova, então segue a decisão 5 antes de implementar |
| **C13-18** | Parte 9 | `RecognitionSyncFeedback::SafeError` grava no histórico textos fixos em português | Gravar uma **chave** estável (`retryable`, `unauthorized`, `rejected`, `persistence`, `unknown`) e traduzir na exibição (`RecognitionListModel`). Registros antigos com texto livre continuam sendo mostrados como estão |

### D. Traduções

| ID | Origem | Problema | Correção proposta | Validação |
| --- | --- | --- | --- | --- |
| **C13-19** | Partes 2 e 11 | Inglês incompleto: 86 entradas `unfinished` em `HaikenAnime_en.ts`; os `.ts` ainda têm textos que saíram do código | 1) o revisor roda `update_translations`; 2) eu preencho as entradas inglesas (estações, "%1 de %2", "Episódio %1", telas novas…); 3) a pt-BR fica como identidade. Vem **depois** de C13-13/15/17/18, que mudam textos | smoke da UI em inglês; `TranslationLoaderTests` |

### E. Documentação

| ID | Origem | Problema | Correção proposta |
| --- | --- | --- | --- |
| **C13-20** | Revisor (P13) | Tudo fica em `docs/superpowers/`, versionado com `git add -f` | `docs/superpowers/` sai do Git (`git rm --cached`, os arquivos continuam na sua máquina) e entra no `.gitignore`. Fica versionado só o que é referência, numa pasta nova `docs/references/`: o contrato do mpv/Plex (`2026-10-17-mpv-plex-contratos.md`) — ver decisão 6. `SERVICE.md`, `README.md` e `AGENTS.md` deixam de apontar para `docs/superpowers/`, e este plano e os reports da revisão ficam só locais |

---

## 2. Ordem sugerida

1. Documentação e ambiente: C13-20, C13-01, C13-02.
2. Erros e logs (baixo risco, testes de unidade diretos): C13-04 → C13-11.
3. Comportamento: C13-14, C13-13, C13-15, C13-16, C13-12, C13-18, C13-03.
4. C13-17 (funcionalidade nova, depois da decisão 5).
5. C13-19 (traduções) por último, porque vários itens acima mudam textos.

Faço tudo em blocos (1, 2, 3…) e peço um build ao fim de cada bloco.

---

## 3. Decisões para o revisor

1. **C13-03 — pasta padrão da biblioteca**: vazia (recomendo; a tela já pede para adicionar pastas) ou outra,
   como `%USERPROFILE%\Videos`?
2. **C13-12 — intervalo**: aplicar só à lista do usuário (recomendo) ou também às mudanças pendentes?
3. **C13-15 — lugar do botão de logs**: na seção que tem "Limpar cache local" (recomendo), ou numa seção nova
   "Diagnóstico"?
4. **C13-16 — registros `Incomplete` antigos**: manter (recomendo; a correção só para de criar novos) ou
   apagar com uma migração?
5. **C13-17 — histórico de edições manuais**: só aumentos de progresso (recomendo) ou toda edição (nota,
   status, remoção)? A edição manual deve aparecer com a mesma etiqueta "Marcado como assistido", ou com
   "Manual"?
6. **C13-20 — doc do mpv/Plex**: sua mensagem terminou em "podemos transformar…". Eu proponho movê-la para
   `docs/references/mpv-plex.md` (sem a referência às *tasks* do plano antigo). A alternativa é
   transformá-la numa seção do `SERVICE.md` (§ Reconhecimento). O que você tinha em mente?
