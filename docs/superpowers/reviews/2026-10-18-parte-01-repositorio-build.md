# Report — Parte 1: Repositório e build

- **Plano:** [`2026-10-18-revisao-geral.md`](../plans/2026-10-18-revisao-geral.md)
- **Commit analisado:** `775bcec` (`feature/reconhecimento-episodios`)
- **Status:** aprovado e aplicado em `feature/revisao-geral` — validado

---

## 0. Baseline

Build limpo em `cmake-build-baseline` (MSVC 2022, Ninja, Debug, Qt 6.8.3 `msvc2022_64`, `BUILD_TESTING=ON`):

| Medida | Resultado |
| --- | --- |
| Configure + build | OK, **1.194 passos** de compilação/link |
| Testes registrados (`ctest`) | **85** |
| `ctest -j 8` | **80 passaram, 5 falharam**, 21,6 s |
| Falhas | `AlternativeNamesTests`, `AniListOAuthCallbackReceiverTests`, `WindowsUrlProtocolRegistrarTests`, `PersonalListChangeServiceTests`, `AniListAccountOwnershipTests`, todas com `0xc0000135` (DLL não encontrada) |
| Causa | as 5 suítes **não estão na lista `HAIKENANIME_TESTS`**, que é quem põe as DLLs do Qt no `PATH` do teste |
| Rodadas manualmente com o Qt no `PATH` | **todas passam**: 38 casos, 0 falhas |

**Conclusão:** o código está verde. As 5 falhas são de registro no CMake e são a referência de "falha
pré-existente". Elas desaparecem com o P1-02 (quando o `PATH`/`TIMEOUT` passa a ser aplicado
automaticamente a toda suíte). Depois do P1-02 o critério de aceite passa a ser **85/85**.

O smoke manual do app (checklist 2.4 do plano) fica para você executar antes de aprovarmos a aplicação
desta parte, já que precisa de interação com a janela.

---

## 1. O que esta parte faz

Esta parte não é uma funcionalidade do app. É a **estrutura que produz o app e os testes**:

- **`CMakeLists.txt`** (2.090 linhas) define:
  1. o executável `HaikenAnime` (fontes de `src/`, Anitomy, módulo QML `HaikenAnime`, recursos Qt com SQL,
     GraphQL, `Settings.json`, ícone e traduções `pt_BR`/`en`);
  2. ~85 executáveis de teste Qt Test (um por suíte), cada um com **sua própria lista de fontes** e recursos;
  3. o *deploy* de desenvolvimento no Windows (`POST_BUILD`): plugins `platforms`/`sqldrivers`, runtime
     MinGW, DLLs do DXC e `windeployqt`;
  4. ajustes de ambiente (Qt MinGW por padrão, descoberta do Vulkan SDK).
- **`scripts/build-release.ps1`** gera o pacote de release (MSVC + Ninja) em `dist/`.
- **Recursos** (`resources/`, `Settings.json`, `tests/fixtures/`) são empacotados por `qt_add_resources`.
- **`.gitignore`**, **`README.md`** e **`codex.md`** descrevem e protegem o repositório.

Junto com isso convivem no repositório **arquivos da V1 que não entram no build**.

---

## 2. Componentes

| Item | Responsabilidade | Situação |
| --- | --- | --- |
| `CMakeLists.txt` — alvo `HaikenAnime` | executável, QML, recursos, traduções | ativo |
| `CMakeLists.txt` — alvos de teste | ~85 executáveis, fontes listadas à mão | ativo, muito duplicado |
| `CMakeLists.txt` — deploy Windows | cópia de DLLs + `windeployqt` | ativo |
| `scripts/build-release.ps1` | pacote de release | ativo |
| `Settings.json` | configuração embutida | ativo (revisado na Parte 3) |
| `resources/sqlite`, `resources/anilist`, `resources/qml`, `resources/icons`, `resources/images` | recursos do app | ativo, com itens mortos (ver P1-05) |
| `tests/fixtures/` | dados gravados para testes | ativo, com itens mortos (ver P1-04) |
| `QML/`, `mainclass.*`, `src/base/`, `src/utilities/`, `src/clients/`, `lib/robotlib/`, `qrc/`, `qml.qrc`, `queries.qrc`, `HaikenAnime.pro`, `HaikenAnime.pro.user` | V1 (qmake) | **não compilado**, 157 arquivos |
| `.gitignore` | ignora builds, binários, IDE, `/docs`, `*superpowers*/`, `/dist` | ativo, com regra contraditória |
| `README.md` / `codex.md` | visão do projeto / regras para IA | parcialmente desatualizados |

**Números relevantes**

- O build limpo executa **1.194 passos** de compilação/link. O projeto tem ~130 `.cpp` em `src/`, 9 do
  Anitomy e ~85 de testes: as mesmas fontes são recompiladas dezenas de vezes (ex.: `AsyncLogger.cpp`,
  `SqliteDatabase.cpp`, Anitomy compilado em 5 alvos; `LocalLibraryScanCompositionTests` e
  `LocalLibraryRecognitionCompositionTests` recompilam **todas** as fontes do app).
- Queries SQL aparecem 2× no CMake (alias + bundle), algumas 3–4× (bundles dos testes).

---

## 3. Testes

Esta parte não tem suítes próprias; ela **define como todas são construídas**. Avaliação da estrutura:

| Aspecto | Situação | Avaliação |
| --- | --- | --- |
| Um executável por suíte | permite rodar/falhar isoladamente | **manter** |
| Lista manual de fontes por suíte | 1.500+ linhas, fácil esquecer arquivo, rebuild lento | **simplificar** (P1-02) |
| Lista `HAIKENANIME_TESTS` para `PATH`/`TIMEOUT` | precisa ser lembrada a cada suíte nova; **5 suítes já foram esquecidas e falham no `ctest`** (ver baseline); `QmlRecognitionStructureTests` está fora dela e tem `set_tests_properties` próprio | **simplificar** (P1-02) |
| `foreach` que injeta migrações em suítes específicas | idem | **simplificar** (P1-02) |
| Fixtures | 3 arquivos sem nenhum uso | **apagar** (P1-04) |

Os testes em si são avaliados nas partes de cada área.

---

## 4. Melhorias propostas

| ID | Categoria | Descrição | Ganho | Risco | Validação |
| --- | --- | --- | --- | --- | --- |
| **P1-01** | `[Remover]` | Apagar o legado V1 não compilado: `QML/`, `mainclass.cpp/.h`, `src/base/`, `src/utilities/`, `src/clients/`, `lib/robotlib/`, `qrc/`, `qml.qrc`, `queries.qrc`, `HaikenAnime.pro`, `HaikenAnime.pro.user`. Nenhum arquivo da V2/CMake/scripts os referencia (só se referenciam entre si). A V1 de consulta continua em `F:\Haiken V1` e no histórico do git. | −157 arquivos (~33,8k linhas); some a confusão "qual `Home.qml`?" | baixo | build + `ctest` idênticos ao baseline |
| **P1-02** | `[Simplificar]` | Reestruturar o `CMakeLists.txt`: (a) biblioteca estática interna `haiken_core` com todas as fontes de `src/` (menos `main.cpp`); (b) `anitomy` como biblioteca própria; (c) função `haiken_add_test(Nome [RESOURCES …] [DEFINITIONS …])` que cria o executável, liga em `haiken_core` + `Qt6::Test`, registra o `add_test` e aplica `PATH`/`TIMEOUT` (eliminando a lista `HAIKENANIME_TESTS`); (d) as duas suítes de composição deixam de recompilar o app inteiro. Exceção mantida: `CoverTemporaryStoreTests` continua compilando `CoverTemporaryStore.cpp` com o `#define HAIKENANIME_COVER_TEMPORARY_STORE_TESTING`. Recursos (SQL/migrações/`Settings.json`) continuam por alvo, para não depender de inicialização de recursos em biblioteca estática. | CMake de ~2.100 para ~400–500 linhas; build limpo com uma fração dos 1.194 passos; suíte nova = 1 linha | médio | mesmo conjunto de testes registrados (`ctest -N` igual ao baseline) e todos passando; app abre e funciona (smoke) |
| **P1-03** | `[Simplificar]` | Registrar queries SQL e GraphQL por `file(GLOB … CONFIGURE_DEPENDS)` + alias pelo nome do arquivo, como já é feito com as migrações, em vez de listar cada arquivo duas vezes. | −~200 linhas; query nova não exige mexer no CMake | baixo | os mesmos caminhos `:/sqlite/queries/*`, `:/anilist/*` disponíveis; `ctest` |
| **P1-04** | `[Teste-remover]` | Apagar fixtures sem uso: `tests/fixtures/graphql-error.json`, `tests/fixtures/graphql-page.json`, `tests/fixtures/graphql/empty-response.json` (critério **2.5-a/d**: não são lidos por nenhum teste). | menos ruído | baixo | `ctest` |
| **P1-05** | `[Remover]` | Remover do executável o `fixture_resource_bundle` (`:/fixtures/media-library.json`, `graphql/page-response.json` e `graphql/userlist-response.json`, ~83k linhas de JSON). Nenhum código de produção lê `:/fixtures`; o único teste que lê (`AniListFlowTests`) já embute a sua cópia. | executável menor; dado de teste fora do binário | baixo | `ctest`; app abre |
| **P1-06** | `[Remover]` | Remover do executável as mutations `update-progress.graphql`, `update-score.graphql` e `update-list-status.graphql`: nenhum código nem teste as carrega (o envio usa só `update-media-list-entry.graphql`). Apagar os três arquivos e suas entradas no CMake. (`media-page.graphql` só é usado por teste e é tratado na Parte 5, junto com `mediaQueryFile`.) | menos recursos mortos | baixo | `ctest` |
| **P1-07** | `[Padronizar]` | `.gitignore`: hoje ignora `/docs` e `*superpowers*/`, mas os arquivos de `docs/superpowers/` são versionados com `git add -f` (os novos — plano e reports — ficam invisíveis no `git status`). Proposta: parar de ignorar `docs/` (mantendo `/.superpowers`, a pasta de trabalho local). | docs aparecem no `git status` e no PR | baixo | `git status` |
| **P1-09** | `[Simplificar]` | Remover o bloco que força `CMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64` em builds MinGW. O `PATH` do usuário já tem `C:\Qt\6.8.3\mingw_64\bin`, e o `find_package(Qt6)` do CMake já procura nas entradas `…\bin` do `PATH`, então o kit MinGW continua sendo encontrado sem o bloco. Builds MSVC seguem passando `-DCMAKE_PREFIX_PATH` (como faz o `build-release.ps1`). | menos configuração pessoal no CMake | baixo | configure MinGW sem `-DCMAKE_PREFIX_PATH` encontra o Qt `mingw_64`; configure MSVC com prefixo explícito continua igual |
| **P1-10** | `[Simplificar]` | Tirar o `windeployqt` do `POST_BUILD`: ele roda a cada build (inclusive Debug) e o `build-release.ps1` já executa o próprio `windeployqt --release` no pacote. As cópias baratas (`platforms`, `sqldrivers`, runtime MinGW, DXC) continuam. | build incremental mais rápido | médio | build Debug MinGW roda pelo diretório de build com o Qt no `PATH`; `build-release.ps1` gera o `dist/` igual ao atual |
| **P1-08** | `[Doc]` | `README.md`: a árvore de "Architecture" lista `QML/` como interface (a real é `resources/qml/`) e falta `src/presentation/`. Corrigir a árvore e apontar para o `SERVICE.md`. (`codex.md` fica para a reescrita como `AGENTS.md` no fim da revisão, como combinado.) | README fiel ao código | baixo | leitura |

### Ordem sugerida de aplicação
P1-01 → P1-04 → P1-05 → P1-06 → P1-03 → P1-02 → P1-07 → P1-08 → **P1-09 → P1-10** (remoções primeiro,
para o CMake novo já nascer sem o que é morto; os ajustes de ambiente ficam por último, a pedido). Cada
item é um commit.

---

## 5. Fora de escopo (anotado, não corrigido)

- `Settings.json` usa `Q:\` como pasta padrão da biblioteca — é um default de comportamento; não mexo.
- `QmlStructureTests`/`QmlRecognitionStructureTests`: o `ENVIRONMENT` coloca `QT_QUICK_BACKEND=software`
  **dentro do `PATH`** (separador `\;`), então essa variável nunca é definida para os testes. O
  comportamento foi preservado no CMake novo; corrigir é decisão da Parte 11 (QML).

---

## 6. Decisões para o revisor

1. **P1-01 (legado V1):** apagar tudo de uma vez num commit único? (Recomendo sim; o histórico guarda tudo.)
2. **P1-02 (CMake):** de acordo com a biblioteca estática interna `haiken_core`? Alternativa mais
   conservadora: só criar a função `haiken_add_test()` e manter as listas de fontes por teste (ganha menos).
3. **P1-07 (`.gitignore`):** versionar `docs/` normalmente daqui pra frente?

---

## 7. Aplicação

| Item | Commit |
| --- | --- |
| SERVICE.md | `9ffc707` |
| P1-01 legado V1 | `7187c03` |
| P1-04 fixtures sem uso | `423eee1` |
| P1-05 fixtures fora do executável | `f8e3adc` |
| P1-06 mutations sem uso | `4f33774` |
| P1-03 GLOB de queries | `2fbed70` (validado: build incremental + `ctest` 85/85) |
| P1-02 `haiken_core` + `haiken_add_test` | `4cf485b` (CMake 2.090 → ~420 linhas) |
| P1-07 `.gitignore` / `docs/` versionado | `24639bd` |
| P1-08 README | `fa5c437` |
| P1-09 sem `CMAKE_PREFIX_PATH` forçado | `625ae8c` |
| P1-10 `windeployqt` só no release | `b239e1c` |
| SERVICE.md (build novo) | `f114acf` |

Notas de equivalência do P1-02:
- cada teste mantém exatamente os mesmos módulos Qt (e, portanto, o mesmo tipo de aplicação do `QTEST_MAIN`),
  os mesmos recursos, defines e includes;
- `CoverTemporaryStoreTests` continua compilando o próprio `CoverTemporaryStore.cpp` (`NO_CORE`);
- `SqliteSyncTaskStateRepositoryTests` embutia as migrações duas vezes (mesmos arquivos, mesmo prefixo);
  agora uma vez;
- as 5 suítes que estavam fora da lista e `QmlRecognitionStructureTests` passam a ter `PATH` do Qt e
  `TIMEOUT 30` (a mais lenta do baseline leva 18 s).

### Validação final (build do revisor, `feature/revisao-geral`)
- Primeira rodada: `TranslationLoaderTests` falhou — a reescrita do P1-02 perdeu o `qt_add_translations`
  desse teste (único comando perdido, conferido contra o CMake anterior). Corrigido em `d6be608`.
- Ferramenta: `scripts/validate.ps1` (`b1c51aa`), que carrega o ambiente do MSVC antes do build.
- Rodada final: build MSVC + `ctest` **85/85**, configure MinGW sem prefixo, `dist/` gerado, smoke OK.
