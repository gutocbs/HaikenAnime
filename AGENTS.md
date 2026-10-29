# AGENTS.md — regras para assistentes de IA

HaikenAnime é um app desktop Qt 6 / QML (Windows, C++20, CMake, SQLite) que organiza uma biblioteca de anime e
mangá, sincroniza a lista pessoal com o AniList, associa arquivos locais e reconhece episódios assistidos no
mpv e no Plex. **A referência do programa é o [`SERVICE.md`](SERVICE.md)**: fluxos, persistência, threads,
padrões de desenvolvimento e de teste. Leia a seção da área antes de mudá-la e atualize-a no mesmo trabalho.

## Como trabalhar

- **Toolchain Windows obrigatória:** use o Qt MinGW 6.8.3 com o MinGW distribuído pelo Qt
  (`C:/Qt/6.8.3/mingw_64` + `C:/Qt/Tools/mingw1310_64`). Não misture `mingw_64` com o gerador
  Visual Studio/MSVC, nem use `msvc2022_64` neste checkout. A configuração recomendada usa Ninja,
  `gcc.exe` e `g++.exe` explicitamente:
  `cmake -S . -B build-mingw -G Ninja -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe`
  `-DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64`
  `-DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug`.
- **Não rode build nem `ctest`.** O build é lento: quando precisar compilar ou testar, pare e peça ao revisor.
  Ele roda `scripts/validate.ps1` (`-Tests <regex>` filtra suítes) e
  informa o resultado. Os logs ficam em
  `cmake-build-validation/Testing/Temporary/LastTest.log` e podem ser lidos.
- **Um item = um commit pequeno** (`refactor:`, `fix:`, `test:`, `docs:`, `build:`), adicionando os arquivos
  explicitamente (nunca `git add -a`).
- **Refatoração não muda comportamento.** Quando a área não tem teste que prove a equivalência, primeiro um
  teste de caracterização (commit próprio).
- **Correção começa por um teste que falha**, e cada correção é um commit `fix:` próprio.
- Mudou um contrato (sinal, `Q_PROPERTY`, `Q_INVOKABLE`, interface `I*`, query, chave JSON)? Atualize todos os
  consumidores e testes no mesmo commit.
- Persistência: nenhuma mudança de schema sem migração nova. As migrações existentes não mudam, e o snapshot
  `tests/fixtures/sqlite/migrated-schema.txt` acompanha o schema.
- Nunca use o banco real do usuário, nem registre *tokens*, credenciais ou *payloads* de *callback* no log.

## Arquitetura

```text
main.cpp            Qt/QML: app, repasse do callback OAuth, protocolo, tradução, engine, saída
src/app/            composição (ApplicationComposition, PresentationComposition, RecognitionComposition),
                    encerramento (ApplicationShutdown) e coordenadores Qt com threads/timers
src/presentation/   controllers e modelos expostos ao QML
src/application/    casos de uso, serviços e interfaces (ports I*)
src/domain/         entidades e regras puras
src/infrastructure/ SQLite, AniList, arquivos, players, Windows, log
resources/          QML (+ Theme.qml), SQL (queries e migrações), GraphQL, imagens
tests/unit/         Qt Test + CTest; tests/fixtures/ com respostas gravadas
```

- `domain` não depende de nada externo; `application` só de interfaces; `infrastructure` implementa as
  interfaces; `presentation` não acessa SQL nem rede; o **QML só apresenta**.
- `main.cpp` não conhece repositórios, queries nem SQLite: a montagem fica em `src/app/`.
- Falha de infraestrutura não impede a janela: o contexto devolve `initializationError` e dependências nulas.

## Convenções

- **Inglês** em código, chaves JSON e APIs; textos de interface em `qsTr()`/`tr()` (fonte em português) e
  `.ts` atualizados pelo alvo `update_translations`.
- **Métodos em PascalCase**; *getters* de `Q_PROPERTY`, sinais e *overrides* do Qt em camelCase.
- Comentário `///` em API pública cujo nome não basta; nada de comentário redundante.
- SQL e GraphQL ficam em arquivos (`resources/sqlite/queries`, `resources/anilist`); os repositórios carregam
  as próprias queries pelas constantes de `SqliteQueries.h`, e o construtor recebe só a conexão.
- Erros: métodos falíveis devolvem `bool` e preenchem `QString &error` com uma mensagem segura.
- Controllers só expõem ao QML o que o QML usa. Cores só pelo `Theme.qml`
  (`readonly property Theme theme: Theme {}`).
- Logs pelo `AsyncLogger`; para componentes que recebem uma função de log, use `LogSink(logger, categoria)`.
- Novos botões de cabeçalho/barra entram à direita, com um único ponto de entrada por feature.

## Testes

- Toda mudança concluída tem teste em `tests/unit/`, registrado com `haiken_add_test` (ver `SERVICE.md`).
- Banco sempre temporário; rede e players sempre falsos (fixtures, transportes e HTTP falsos).
- QML se testa carregando a tela (`QmlScreensLoadTests` falha em qualquer aviso de QML), nunca procurando
  texto no arquivo `.qml`.
- Nome do teste descreve o comportamento e o resultado; um comportamento por teste; cobrir erros.
