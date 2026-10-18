# Report — Parte 7: Biblioteca local

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `8a25b93` (`feature/revisao-geral`)
- **Status:** aprovado (com as duas recomendações) e aplicado — aguardando build do revisor
- *(`MpvAwareLocalFileOpener` fica para a Parte 9, junto com o reconhecimento de reprodução.)*

---

## 1. O que esta parte faz

Mantém o inventário dos arquivos de vídeo das pastas da biblioteca e associa cada arquivo a uma mídia do
catálogo, para a Home saber qual é o próximo episódio local e abri-lo.

- **Varredura** (`LocalLibraryScanCoordinator` → `LocalLibraryScanner` → `QtDirectoryEnumerator`): numa thread
  de baixa prioridade, percorre cada pasta configurada (recursivo, sem seguir links/junções, ignorando pastas
  ocultas e arquivos temporários). Filtra pelas extensões permitidas e entrega lotes de até 200 entradas, com
  pausa de 25 ms entre lotes. No Windows a listagem usa `FindFirstFileW`/`FindNextFileW` para distinguir fim de
  listagem de erro. Cada pasta vira um `library_scans` (`running` → `succeeded`/`failed`/`interrupted`); os lotes
  são gravados em `local_files` e, **só se a varredura terminar bem**, os arquivos não vistos dessa raiz ficam
  indisponíveis. Uma pasta inacessível (disco desconectado) não impede as outras.
- **Reconhecimento** (`LocalLibraryRecognitionCoordinator`): ao fim da varredura de cada raiz, lê os arquivos
  pendentes (não processados, não reconhecidos ou ambíguos). Para cada um, o `AnitomyLocalFileRecognizer`
  extrai título, temporada e episódio do nome (Anitomy + *fallback* por regex para `S01E02`, `- 05`, `Season 2`,
  `Part 2`; rejeita intervalos como `01-12`). O `LocalMediaResolver` então associa: a pasta da mídia que contém
  o arquivo vence; senão, o título normalizado com marcador de temporada (`2nd season`, `season 2`, `s2`,
  `part 2`, romanos, número), ignorando mangá/novel. Grava em lotes de 200. Editar uma mídia (pasta ou nomes
  alternativos) dispara `startForFolder`/`startForPending` (`LocalRecognitionRequest`, ligado em `main.cpp`).
  Pedidos durante uma execução viram **uma** execução seguinte, sem duplicatas.
- **Próximo episódio** (`LocalEpisodeReader`): a Home lê o menor episódio associado e disponível acima do
  progresso, e a contagem de episódios disponíveis; `QtLocalFileOpener` abre o arquivo no programa padrão.

---

## 2. Componentes e achados

| Componente | Linhas | Achado |
| --- | --- | --- |
| `LocalEpisodeReader` (application) | 54 + 31 | **SQL na camada `application`**: inclui `SqlQueryStore`, usa `QSqlQuery` (vindo da Parte 3). Recebe os 2 caminhos de query no construtor |
| `ILocalFileRepository` | 37 | junta dois papéis: os 4 métodos de **varredura** (usados só pelo scan coordinator) e os 4 de **reconhecimento** (só pelo recognition coordinator), estes com implementação padrão "not supported" para os *fakes* não precisarem implementá-los |
| `SqliteLocalFileRepository` | 302 + 40 | construtor com **10 caminhos de query** (os 5 últimos com default vazio); `ComparablePath` é cópia de `LocalMediaCatalogIndex::NormalizeFolder` |
| `OwnedLocalFileRepository`, `OwnedLibraryScanner`, `OwnedSyncTaskStateRepository` (composição) | ~70 | só existem para manter a conexão/enumerador vivos junto do produto; repetem à mão cada método da interface (8 no de arquivos locais) |
| `AnitomyLocalFileRecognizer` / `ILocalFileRecognizer` | 184 + 25 | parâmetro `requestedKind` **nunca é passado** (sempre `Anime`), então o ramo "tipo não suportado" é morto; `#include <iostream>` sem uso |
| `LocalMediaResolver` | 126 + 30 | 3 sobrecargas de `resolve` (catálogo; índice; índice + caminho), a do meio só repassa caminho vazio; o ramo `mediaKind != Anime` é morto pelo mesmo motivo acima |
| `LocalMediaCatalogIndex` | 82 + 41 | `titleCount()` sem uso |
| `LocalLibraryRecognitionCoordinator` | 167 + 55 | `start()` é apelido de `startForPending()` ("kept for the scan-completed wiring"), usado só pela composição |
| `LocalLibraryScanTypes.h` | 45 | comentário obsoleto "(Task 1 policy)" |
| `LocalLibraryScanner`, `QtDirectoryEnumerator`, `QtLocalFileOpener`, `LocalLibraryScanCoordinator`, `LocalRecognitionRequest` | — | ok. `batchSize`/`batchPauseMs` e a `ListingFactory` só variam nos testes, mas são as costuras que permitem testar lotes e erros de listagem do Windows |

---

## 3. Testes

| Suíte | Avaliação |
| --- | --- |
| `LocalEpisodeReaderTests` (3) | **manter**, apontando para a implementação movida (P7-01) |
| `SqliteLocalFileRepositoryTests` (18) | **manter**; `reportsStatementAndTransactionErrors` perde a metade que injeta `missing.sql` (critério **a**, o caminho deixa de ser parâmetro) e mantém a do erro de transação |
| `AnitomyLocalFileRecognizerTests` (15) | **apagar** `rejectsUnsupportedKinds` (critério **a**); o resto **fica** |
| `LocalMediaResolverTests` (40) | **apagar** `rejectsUnsupportedKind` (critério **a**); `indexedResolveMatchesCatalogResolveForEveryFixtureTitle` **fica** (compara as duas sobrecargas que sobram); o resto **fica** |
| `LocalLibraryRecognitionCoordinatorTests` (5), `LocalLibraryScanCoordinatorTests` (18), `LocalLibraryScannerTests` (21), `QtLocalFileOpenerTests` (2) | **manter** |
| `LocalLibraryRecognitionCompositionTests` (4) | **manter** |
| `LocalLibraryScanCompositionTests` (11) | **manter**. `workerProductsReleaseConnectionsAndShutdownBeforeDatabase` é a garantia do P7-04 (ordem de destruição). Vários casos testam preferências, ordenação e capas, não a biblioteca; o nome da suíte é revisto na Parte 12 (composição) |

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P7-01** | `[Padronizar]` | Mover `LocalEpisodeReader` para `infrastructure/database/SqliteLocalEpisodeReader` (usando `SqliteQueries` internamente, só a conexão no construtor); `ILocalEpisodeReader` fica em `application/library/ILocalEpisodeReader.h`. Home e composição passam a incluir a interface. A suíte vira `SqliteLocalEpisodeReaderTests` | camadas corretas | baixo | `LocalEpisodeReaderTests`, `HomeScreenControllerTests` |
| **P7-02** | `[Simplificar]` | `SqliteLocalFileRepository` usa as próprias constantes de `SqliteQueries`: construtor só com a conexão (mesmo padrão das Partes 4 e 6) | −10 parâmetros | baixo | `SqliteLocalFileRepositoryTests` |
| **P7-03** | `[Simplificar]` | Separar `ILocalFileRepository` em `ILocalFileScanRepository` (varredura) e `ILocalFileRecognitionRepository` (reconhecimento), sem implementações "not supported"; cada coordinator recebe a *factory* da sua interface; o SQLite implementa as duas | contrato honesto | baixo | coordinators + `SettingsControllerTests` (*fakes* de varredura) |
| **P7-04** | `[Simplificar]` | Trocar os 3 *wrappers* de ownership por herança "base-from-member": uma base privada guarda a conexão (ou o enumerador) e a classe herda o repositório/scanner, sem repetir métodos. Exige tirar o `final` de `SqliteLocalFileRepository`, `SqliteSyncTaskStateRepository` e `LocalLibraryScanner`. A ordem de destruição continua: produto antes da conexão | −~55 linhas | médio | `LocalLibraryScanCompositionTests::workerProductsReleaseConnectionsAndShutdownBeforeDatabase`, smoke |
| **P7-05** | `[Remover]` | `requestedKind` do `ILocalFileRecognizer`/`AnitomyLocalFileRecognizer` e os ramos "tipo não suportado" do recognizer e do resolver | −~20 linhas | baixo | suítes do recognizer e do resolver |
| **P7-06** | `[Simplificar]` | `LocalMediaResolver::resolve`: 3 → 2 sobrecargas (`catalog`; `index` + caminho opcional); remover `LocalMediaCatalogIndex::titleCount` | API menor | baixo | `LocalMediaResolverTests` |
| **P7-07** | `[Unificar]` | `ComparablePath` do repositório passa a chamar `LocalMediaCatalogIndex::NormalizeFolder` (mesma regra, linha a linha) | −15 linhas, uma regra só | baixo | `SqliteLocalFileRepositoryTests` (casos de pasta, inclusive Unicode) |
| **P7-08** | `[Padronizar]` | Remover o apelido `LocalLibraryRecognitionCoordinator::start` (composição chama `startForPending`), o `#include <iostream>` e o comentário "(Task 1 policy)" | limpeza | baixo | build |

### Ordem sugerida
P7-08 → P7-05 → P7-06 → P7-07 → P7-01 → P7-02 → P7-03 → P7-04.

---

## 5. Fora de escopo (vai para a Parte 13)

| Item |
| --- |
| `SqliteLocalFileRepository::upsertBatch`/`completeScan`/`failScan`: quando o `.sql` não pode ser lido, o erro do `SqlQueryStore` é sobrescrito por `query.lastError()` (vazio) — mesmo defeito do `SqliteUserPreferencesRepository` (Parte 4) |
| `LocalEpisodeReader::readAvailableEpisodeCount` devolve `false` **sem mensagem** quando o `mediaId` é inválido ou a consulta não traz linha |

---

## 6. Decisões para o revisor

1. **P7-04 com herança** (recomendo). A alternativa é manter os *wrappers* como estão; o ganho é só de
   tamanho, e a herança exige tirar o `final` de 3 classes.
2. **`LocalMediaKind`** depois do P7-05: só `Anime` passa a ser produzido. Recomendo **manter o enum** como
   está: ele é o tipo do campo `mediaKind`, que vai para a coluna de texto `local_files.media_kind`, e
   reduzi-lo não tira lógica nenhuma. A alternativa é reduzi-lo a `Anime`/`Unsupported`.

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| P7-08 apelido `start`, `<iostream>` e comentário obsoleto | `8065ae7` |
| P7-05 `requestedKind` e ramos de tipo não suportado | `66d9088` |
| P7-06 sobrecargas do resolver e `titleCount` | `da71ce3` |
| P7-07 `ComparablePath` → `NormalizeFolder` | `b12e490` |
| P7-01 `SqliteLocalEpisodeReader` na infraestrutura | `69e2ade` |
| P7-02 repositório de arquivos locais com queries próprias | `5604a0c` |
| P7-03 contratos de varredura e de reconhecimento | `cb5c7dc` |
| P7-04 ownership por herança | `0a31907` |

Notas:
- P7-05: o default de `LocalFileRecognition::mediaKind` passou de `Unsupported` para `Anime`, o único valor
  que o recognizer e o `EpisodeRecognitionService` produzem. Sem isso, um reconhecimento montado com os valores
  padrão deixaria de ser rejeitado pelo resolver.
- P7-04 também trocou o `OwnedSyncTaskStateRepository` da Parte 6. Os 3 *wrappers* viraram
  `OwnedLibraryScanner` (herda o enumerador e o `LocalLibraryScanner`) e o *template*
  `ConnectionOwningRepository<Repository>` (herda a conexão e o repositório). O `final` saiu de
  `SqliteLocalFileRepository`, `SqliteSyncTaskStateRepository` e `LocalLibraryScanner`, com um comentário
  explicando o porquê.
- Testes: `LocalEpisodeReaderTests` virou `SqliteLocalEpisodeReaderTests`;
  `SqliteLocalFileRepositoryTests::reportsStatementAndTransactionErrors` virou `reportsTransactionErrors`
  (sem a metade do `missing.sql`), e o *helper* `statement()` saiu; os *fakes* dos coordinators e do
  `SettingsControllerTests` implementam só o contrato que usam.
