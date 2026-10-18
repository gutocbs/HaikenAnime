# Report — Parte 3: Infraestrutura base

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `39bd9ec` (`feature/revisao-geral`)
- **Status:** aguardando aprovação — nenhuma alteração aplicada

---

## 1. O que esta parte faz

São os serviços de base de que todo o resto depende:

- **`SqliteDatabase`** abre (ou cria) o arquivo `haikenanime.sqlite` em `AppDataLocation`, liga
  `foreign_keys` e `busy_timeout`, e cada thread abre a sua própria conexão com um nome único.
  `migrate()` (só a conexão principal chama, na inicialização) liga o WAL e, **numa única transação**, refaz
  a cadeia inteira de 26 versões de schema a cada abertura do app. Cada passo é idempotente: inspeciona as
  colunas (`PRAGMA table_info`), cria só o que falta e grava a versão (`INSERT OR IGNORE INTO
  schema_version`). No fim confere que as versões 1 a 26 estão registradas; se algo falhar, faz rollback
  e a janela abre com o erro.
- **`SqlQueryStore`** lê o texto de uma query de um recurso `:/sqlite/queries/…`. Os repositórios recebem o
  **caminho** e releem o arquivo a cada operação.
- **`SqliteQueryConfiguration` + `sqlite-queries.json`** mapeiam nomes lógicos (`readMedia`) para esses
  caminhos. A composição carrega o JSON e repassa os caminhos para os construtores dos repositórios.
- **`JsonSettingsReader`** lê e valida o `Settings.json` embutido (AniList, HTTP, capas, catálogo sazonal,
  políticas de sync, retenção de log, defaults de preferências). Em caso de falha, a composição segue com
  os defaults.
- **`AsyncLogger`** grava log em arquivo diário numa thread própria, com fila limitada a 4.096 entradas e
  retenção em dias.
- **Segredos:** `WindowsCredentialStore` (token AniList) e `WindowsPluginSecretStore` (segredos de plugin,
  como o token do Plex), ambos no Windows Credential Manager. `FileSecretStore` é uma implementação em
  arquivo.

---

## 2. Componentes

| Arquivo | Responsabilidade | Situação |
| --- | --- | --- |
| `SqliteDatabase.cpp` (464 linhas) | conexão + migrações | `migrate()` com ~350 linhas, encadeamento manual |
| `resources/sqlite/migrations/*.sql` (75 arquivos) | DDL + 26 `mark-*-applied` de 1 linha + 5 `inspect-*` + 1 checagem de versões | muitos arquivos de 1 linha |
| `SqlQueryStore` | lê query de recurso | ativo; relê o arquivo a cada uso |
| `SqliteQueryConfiguration` (159 linhas) + `sqlite-queries.json` | nome → caminho | cobre 31 queries; outras 13 são caminhos fixos no C++ (executor, recognition, `read-pending-media-ids`) |
| `JsonSettingsReader.cpp` (342 linhas) | parse + validação do `Settings.json` | `read()` com ~230 linhas |
| `AsyncLogger` | log em arquivo | ok |
| `WindowsCredentialStore`, `WindowsPluginSecretStore` | Credential Manager | mesmo código de `CredReadW`/`CredWriteW`/`CredDeleteW` nos dois |
| `FileSecretStore` | segredos em arquivo | **sem uso em produção** |

---

## 3. Testes

| Suíte / caso | O que cobre | Avaliação |
| --- | --- | --- |
| `SqliteDatabaseTests` (28 casos) | abertura, pragmas, schema, idempotência, upgrades de versões antigas sem perda de dados, rollback | **manter**; ver abaixo |
| `…RequiresVersionSevenAndRollsBack`, `…EightAndRollsBack`, `…NineAndRollsBack` | trigger que ignora o `INSERT` da versão N → migração falha e faz rollback | **ajustar**: 3 cópias do mesmo teste → 1 teste *data-driven* que cobre **todas** as 26 versões (critério **b**, e fica mais forte) |
| *(faltando)* snapshot do schema final | — | **criar** antes de refatorar o `migrate()` (teste de caracterização: `sqlite_master` + `schema_version` de um banco novo comparados a uma fixture) |
| `SqlQueryStoreTests` (4) | leitura, erro, arquivo vazio, rejeita SQL inline | **manter** |
| `SqliteQueryConfigurationTests` (4) | parse do JSON, chave faltando | **apagar** se P3-04 for aprovado (critério **a**) |
| `LocalLibraryScanCompositionTests::scannerQueryFailureDoesNotDisableMedia(+_data)` e `…IsLoggedNonfatally`; `LocalLibraryRecognitionCompositionTests::recognitionQueryConfigurationFailureDoesNotDisableTheApplication` | app abre mesmo com o JSON de queries incompleto/errado | **apagar** se P3-04 for aprovado (critério **a**: o cenário deixa de existir; o JSON é embutido no binário). Falhas reais de banco continuam cobertas nos testes dos coordenadores |
| `JsonSettingsReaderTests` (20) | parse, defaults, tipos errados, políticas, OAuth | **manter**; ajustar os casos de `covers.quality` se P3-08 for aprovado |
| `FileSecretStoreTests` (6) | store em arquivo | **apagar** com P3-05 (critério **a**) |
| `WindowsCredentialStoreTests` (2), `WindowsPluginSecretStoreTests` (4) | Credential Manager real | **manter** |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P3-01** | `[Teste-ajustar]` | **Antes de tudo:** criar o teste de caracterização do schema final e juntar os 3 testes de rollback num *data-driven* que cobre as versões 1 a 26. Commit separado, rodado contra o código **atual**. | rede de segurança para o P3-02 | baixo | o teste novo passa no código atual |
| **P3-02** | `[Simplificar]` | Reescrever `migrate()` como uma **lista declarativa de passos** executada em ordem: `script(arquivo)`, `addColumn(tabela, coluna, arquivo)`, `markVersion(n)` e um passo especial para a reconstrução de `local_files` (v17). Mantém a transação única, a mesma ordem, a idempotência, o rollback e a checagem final das 26 versões. Os 7 `ALTER TABLE sync_task_state …` hoje escritos no C++ viram arquivos `019-add-*.sql`, como os demais. | ~350 → ~120 linhas; nova migração = 1–2 linhas na lista | **médio** | P3-01 + todos os `SqliteDatabaseTests` + smoke com o banco real do revisor |
| **P3-03** | `[Simplificar]` | *(depende da decisão 1)* O runner gera o SQL genérico (`PRAGMA table_info(<tabela>)` e `INSERT OR IGNORE INTO schema_version …`), e saem **32 arquivos** de 1 linha (26 `mark-*`, 5 `inspect-*`, 1 checagem de versões). | menos arquivos sem conteúdo | baixo | igual ao P3-02 |
| **P3-04** | `[Simplificar]` | *(depende da decisão 2)* Trocar `sqlite-queries.json` + `SqliteQueryConfiguration` + `hasExistingQueries` + `ApplicationCompositionOptions::queryConfigurationPath` por um header de constantes (`SqliteQueries.h`, com um caminho por query). Ele passa a ser usado também onde hoje há caminho fixo (executor de sync, reconhecimento). Os construtores dos repositórios continuam iguais nesta parte; cada um pode internalizar as próprias queries quando a sua parte for revisada. | −~200 linhas; um único modelo de registro de queries | baixo | build + `ctest` (menos os testes apagados) |
| **P3-05** | `[Remover]` | `FileSecretStore` (+ `FileSecretStoreTests`): produção usa só o Credential Manager. | −~255 linhas | baixo | build |
| **P3-06** | `[Unificar]` | Helper interno `WindowsCredentialManager` (`readGeneric`/`writeGeneric`/`deleteGeneric` + a mensagem de erro) usado pelos dois stores. Cada store mantém o seu formato: JSON das credenciais AniList e texto puro, com a cópia zerada da memória, para os plugins. | −~40 linhas duplicadas | baixo | `Windows*StoreTests` |
| **P3-07** | `[Simplificar]` | `JsonSettingsReader::read()`: separar em uma função por seção (`readAniList`, `readHttp`, `readCovers`, `readSync`, `readSeasonalCatalog`, `readUserPreferences`), **mantendo a ordem das validações e as mensagens de erro**. As duas sobrecargas de `readPositivePolicyInteger` viram um template. | leitura mais fácil | baixo | `JsonSettingsReaderTests` |
| **P3-08** | `[Remover]` | `covers.quality` do `Settings.json`: lido, validado e guardado em `CoverSettings::quality` → `ApplicationContext::coverQuality`, mas **nunca usado**. A qualidade efetiva vem de `userPreferences.covers.quality`. Remover a chave, o campo e o parse. | menos configuração enganosa | baixo | `JsonSettingsReaderTests` ajustados + smoke (capas na qualidade escolhida) |

### Ordem sugerida
P3-05 → P3-06 → P3-08 → P3-07 → P3-04 → **P3-01 → P3-02 → P3-03**. A migração fica por último e com o
teste de caracterização antes, porque é o item mais sensível da parte.

---

## 5. Fora de escopo (vai para a Parte 13)

- **Mensagem de erro da migração:** quando um passo falha, o `lastError_` específico é sobrescrito no
  fim por `query.lastError()`. Se a falha não veio de uma query (ex.: recurso que não abre), a mensagem
  vira o genérico "SQLite migration versions **1 through 15** were not recorded.", que, além de tudo, cita
  a faixa errada (hoje são 26 versões). O P3-02 preserva esse comportamento.
- **Queries relidas a cada operação:** os repositórios releem o arquivo `.sql` em toda chamada, e o
  `SqlQueryStore` grava um `INFO "Loaded SQL query file"` **a cada leitura** (log ruidoso + I/O
  desnecessário). Corrigir = carregar uma vez na construção, o que muda o conteúdo do log.
- **Camada violada (vai para a Parte 7):** `application/library/LocalEpisodeReader` inclui
  `infrastructure/database/SqlQueryStore` e usa `QSqlQuery`, ou seja, acessa SQL a partir da camada
  `application`.

---

## 6. Decisões para o revisor

1. **SQL genérico das migrações (P3-03):** gerar no runner o `PRAGMA table_info(...)` e o
   `INSERT OR IGNORE INTO schema_version` e apagar 32 arquivos de 1 linha? Recomendo **sim**. É SQL de
   infraestrutura, não de regra; os DDLs reais continuam em arquivo. Alternativa: manter os arquivos (o
   runner só os referencia).
2. **Registro de queries (P3-04):** trocar o JSON por constantes em C++? Recomendo **sim**. O JSON é
   embutido no binário e não pode ser alterado sem recompilar, então a indireção não traz flexibilidade e
   obriga 4 testes a simular um JSON quebrado. Alternativa: manter o JSON e só incluir nele as 13 queries
   que hoje estão fixas no C++.
