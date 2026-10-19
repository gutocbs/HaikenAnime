# Report — Parte 12: Composição e entrada

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `00b33c6` (`feature/revisao-geral`)
- **Status:** aguardando aprovação — nenhuma alteração aplicada

---

## 1. O que esta parte faz

- **`createApplicationContext`** (`src/app/ApplicationComposition`) monta tudo o que não é tela, nesta ordem:
  1. lê o `Settings.json` embutido;
  2. inicia o log assíncrono;
  3. abre e migra o SQLite — se falhar, devolve o contexto com `initializationError` e a janela abre assim
     mesmo;
  4. cria repositórios, outbox e o serviço de edição;
  5. cria biblioteca local (scan e reconhecimento), próximo episódio e reconhecimento de players;
  6. lê as preferências (semeando-as a partir do `Settings.json` na primeira vez);
  7. cria capas e limpeza de cache;
  8. cria a conta AniList (Credential Manager, OAuth se configurado), o runtime de sync e o catálogo
     sazonal.

  Os membros do `ApplicationContext` são destruídos em ordem inversa, e as *threads* (scan, reconhecimento)
  são declaradas por último para serem encerradas primeiro.
- **`main.cpp`**:
  1. trata o *callback* OAuth recebido por linha de comando, repassando-o à instância aberta e saindo;
  2. registra o protocolo `haikenanime://`;
  3. instala a tradução;
  4. cria os 8 controllers e liga ~40 sinais entre eles e os serviços (reload da Home após sync ou edição,
     pedido de reconhecimento local, persistência da ordenação e da última conta AniList, remoção da conta,
     sync ao conectar, preferências aplicadas em tempo real);
  5. expõe os controllers ao QML;
  6. no encerramento, para a sync com limite de 5 s, depois scan, reconhecimento e log, registrando os
     tempos.

---

## 2. Componentes e achados

| Componente | Achado |
| --- | --- |
| `main.cpp` (435 linhas) | quase tudo é **fiação**, o oposto do que o próprio `codex.md` pede ("`main.cpp` focado em Qt/QML"): criação dos controllers, ~40 `connect`, lambdas de regra (dono das mudanças, conta a lembrar, pedido de sync) e o encerramento em duas etapas |
| `persistHomeSortPreference` / `persistLastAniListUserId` (`main.cpp`) | a mesma função duas vezes (copia as preferências, muda um campo, grava, troca) |
| Lambdas de log | `[logger](msg){ if (logger) logger->info(categoria, msg); }` aparece **9×** (6 na composição, 3 no `main.cpp`) |
| `ApplicationContext::coverQuality` | o item do plano já não existe (saiu em uma parte anterior); nada a fazer |
| `ApplicationComposition.cpp` | *includes* sem uso (`SqlQueryStore.h`, `SqliteQueries.h`, duplicados de `AniListViewerClient.h`/`WindowsAniListOAuthLauncher.h`) |
| `codex.md` | **desatualizado**: fala de V1/V2, `F:\Haiken V1`, `infrastructure/persistence/sqlite/`, `LegacyDataImporter`, "sincronização inicial", que não existem mais, e não cita `SERVICE.md`, `validate.ps1`, a regra do build, `SqliteQueries`, a paleta nem os padrões de nome desta revisão |
| `TranslationLoader`, `MpvAwareLocalFileOpener`, `AniListOAuthCallbackReceiver`, `scheduleStartupLibraryScan` | ok |

---

## 3. Testes

| Suíte | Avaliação |
| --- | --- |
| `LocalLibraryScanCompositionTests` (11), `LocalLibraryRecognitionCompositionTests` (4), `RecognitionCompositionTests` (11), `TranslationLoaderTests` | **manter**. Elas cobrem a composição de serviços e o ciclo de vida dos *workers*. A fiação do `main.cpp` não tem teste hoje; com o P12-01 ela passa a ser testável, e eu proponho **um** teste novo de caracterização (`PresentationCompositionTests`): monta a apresentação sobre um banco temporário e verifica as ligações principais (sync concluída → Home recarrega; ordenação alterada → preferência gravada) |
| `LocalLibraryScanCompositionTests` — o nome | vários casos testam preferências, ordenação e capas, não a biblioteca. Renomear a suíte para `ApplicationCompositionTests` (sem mudar os casos) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P12-01** | `[Simplificar]` | `src/app/PresentationComposition.{h,cpp}`: `PresentationContext` é dono dos controllers e serviços de tela, e `CreatePresentationContext(ApplicationContext&, AniListOAuthCallbackReceiver&, QObject &lifetime)` faz toda a fiação; `ExposeToQml(...)` registra as *context properties*. O encerramento vira `ShutdownApplication(ApplicationContext&)` em `src/app/ApplicationShutdown`. O `main.cpp` fica com Qt/QML: app, repasse do *callback* OAuth, protocolo, tradução, *engine* e saída (~80 linhas). Mesma ordem de criação e destruição de hoje | `main.cpp` −~350 linhas; fiação testável | **médio** | `PresentationCompositionTests` (novo, antes da mudança), suítes de composição, smoke completo + log "Application shutdown completed" |
| **P12-02** | `[Unificar]` | `persistHomeSortPreference`/`persistLastAniListUserId` → um `UpdateUserPreferences(context, mudança, error)` | −~15 linhas | baixo | smoke (ordenar a Home, conectar conta) |
| **P12-03** | `[Unificar]` | `LogSink(AsyncLogger *, LogCategory, LogLevel)` em `AsyncLogger.h` devolve o `std::function<void(const QString &)>` que as 9 lambdas montam à mão | −~25 linhas | baixo | build |
| **P12-04** | `[Padronizar]` | Limpar os *includes* sem uso de `ApplicationComposition.cpp`; renomear `LocalLibraryScanCompositionTests` → `ApplicationCompositionTests` | limpeza | baixo | build |
| **P12-05** | `[Doc]` | **`codex.md` → `AGENTS.md`** (`git mv` + reescrita, ~80 linhas). Conteúdo: o que é o app e onde está a bíblia (`SERVICE.md`); camadas e dependências permitidas; regras de trabalho (o assistente não roda build/ctest e pede ao revisor; `scripts/validate.ps1` é local e não versionado; um item por commit; refatoração sem mudar comportamento; correção começa por teste que falha); convenções (inglês, PascalCase, queries em arquivo via `SqliteQueries`, repositórios carregam as próprias queries, `Theme.qml`, QML só apresenta, só expor ao QML o que ele usa); testes (Qt Test/CTest, banco temporário, *fakes*, nada de testar texto de QML); o que não fazer (tocar o banco real, segredos em log). **`README.md`** e **`SERVICE.md`**: revisão final do estado do programa | documentação fiel | baixo | leitura |

### Ordem sugerida
P12-04 → P12-03 → P12-02 → teste de caracterização → P12-01 → P12-05.

---

## 5. Fora de escopo (vai para a Parte 13)

| Item |
| --- |
| `createApplicationContext` descarta o erro de `AniListAuthManager::load` (`static_cast<void>`): uma falha ao ler a conta do Credential Manager não deixa rastro no log |

---

## 6. Decisões para o revisor

1. **P12-01 com o teste de caracterização novo** (recomendo). A alternativa é mover a fiação sem teste,
   confiando só no smoke.
