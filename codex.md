# Contexto do projeto

## Organização de versões

- A V2 é o projeto ativo deste repositório e fica na raiz de `F:\HaikenAnime`.
- Todo desenvolvimento, alteração de código e criação de arquivos da V2 deve ser feito na raiz do repositório, respeitando sua organização por responsabilidade.
- A V1 foi movida para `F:\Haiken V1` e deve ser tratada somente como referência para consultar regras de negócio, comportamentos existentes, contratos, telas e decisões da versão anterior.
- Não modificar, mover, renomear ou excluir arquivos de `F:\Haiken V1` durante o desenvolvimento da V2, salvo solicitação explícita.
- A separação entre os diretórios é intencional: a V2 evolui na raiz e a V1 permanece disponível para consulta.
- Ao consultar a V1, adaptar o comportamento para a arquitetura da V2; não copiar automaticamente limitações, organização de arquivos ou decisões técnicas da V1.

## Instruções para assistentes

- Antes de implementar uma regra de negócio na V2, consultar a V1 quando o comportamento anterior for relevante.
- Tratar a V1 como fonte de referência, não como área de trabalho.
- Priorizar a arquitetura, contratos e padrões definidos na V2.
- Manter alterações da V2 isoladas na raiz de `F:\HaikenAnime`.
- Usar sempre nomes em inglês para variáveis, classes e métodos na V2, incluindo propriedades de entidades, chaves de JSON e APIs internas.
- Documentar o código público criado na V2. Interfaces, classes públicas e seus métodos devem ter comentários curtos explicando sua responsabilidade, parâmetros relevantes, resultado esperado e condições de erro quando aplicável.
- Quando possível, não manter queries SQL ou GraphQL hardcoded em classes C++; armazená-las em arquivos separados e carregá-las pela infraestrutura de configuração/composição.

## Testes

- Toda mudança concluída na V2 deve ter testes unitários correspondentes. Testes são uma prioridade do projeto e devem cobrir o comportamento esperado, regras de negócio e casos de erro relevantes.
- Não é necessário criar os testes imediatamente quando uma implementação ainda estiver incompleta ou em desenvolvimento. Porém, antes de considerar a mudança concluída, os testes devem ser adicionados ou atualizados.
- Os testes devem ficar em `V2/tests/unit/` e ser executáveis pelo CMake/CTest.
- Testes de persistência devem usar bancos temporários, fixtures ou doubles; nunca devem usar diretamente o banco real do usuário.
- Ao finalizar uma mudança, executar os testes afetados e informar claramente quando a execução não for possível.

## Diretrizes para a reescrita da V2

A nova implementação deve usar uma organização por responsabilidade, e não apenas por tipo de arquivo. A estrutura de referência é:

```text
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── app/              # entrada da aplicação e composição principal
│   ├── domain/           # entidades e regras de negócio
│   ├── application/     # casos de uso, serviços e interfaces (ports)
│   ├── infrastructure/  # SQLite, AniList, filesystem, downloads e logging
│   └── presentation/    # controllers, modelos QML e interface
├── resources/            # QML, queries GraphQL, ícones e demais recursos
├── third_party/          # bibliotecas externas, como Anitomy
├── tests/
│   ├── unit/
│   ├── integration/
│   └── fixtures/
└── docs/
```

Os arquivos `.h` e `.cpp` devem permanecer juntos dentro do módulo ao qual pertencem. A separação entre `domain`, `application`, `infrastructure` e `presentation` deve ser preservada na raiz do projeto:

- `domain` não deve depender de QML, SQLite, AniList ou filesystem.
- `application` deve conter casos de uso e depender de interfaces, não de implementações concretas.
- `infrastructure` deve implementar persistência, integrações externas, varredura de arquivos, downloads e logging.
- `presentation` deve adaptar os casos de uso para Qt/QML, usando controllers e `QAbstractListModel` quando apropriado.

O antigo `MainClass` não deve ser recriado como uma classe central monolítica. Suas responsabilidades devem ser distribuídas entre serviços e controllers menores, por exemplo `LibraryService`, `SyncService`, `LocalMediaService`, `SettingsService` e `SyncController`.

A primeira versão da V2 deve priorizar o núcleo da biblioteca: modelo de domínio, persistência local, listas, busca, progresso, configurações básicas, migração dos dados antigos e uma interface QML mínima. AniList, indexação de arquivos locais, downloads, torrents, reconhecimento de players e automação devem ser adicionados gradualmente.

Preferir CMake com builds fora da árvore. O diretório de build não deve ser versionado. A camada de persistência deve ser projetada para SQLite e migrações, enquanto as regras atuais de normalização de títulos e a biblioteca Anitomy devem ser preservadas atrás de interfaces próprias e cobertas por testes.

Não transportar diretamente para a V2 os seguintes padrões da V1: `MainClass` como centro da aplicação, `Database` monolítico, arrays paralelos na QML, sinais numerados, uso indiscriminado de `QVariant`, singletons globais e entidades com campos públicos sem invariantes.

## Persistência local

A V2 deve utilizar SQLite como banco de dados local. Não deve existir um servidor de banco separado. O banco será um arquivo local criado no diretório de dados do usuário, por exemplo:

```text
%APPDATA%/HaikenAnime/haikenanime.db
```

A persistência deve ficar isolada em `infrastructure/persistence/sqlite/`:

```text
src/infrastructure/persistence/
├── sqlite/
│   ├── SqliteDatabase.h
│   ├── SqliteDatabase.cpp
│   ├── SqliteMediaRepository.h
│   ├── SqliteMediaRepository.cpp
│   └── migrations/
│       ├── 001_initial.sql
│       └── 002_add_local_files.sql
└── legacy/
    ├── LegacyDataImporter.h
    └── LegacyDataImporter.cpp
```

A camada `application` deve depender de interfaces como `IMediaRepository`, e não diretamente de `QtSql` ou SQLite. O acesso SQL, as migrações, backups e a configuração da conexão devem permanecer nos repositórios e componentes da infraestrutura.

O esquema inicial deve separar o catálogo externo dos dados do usuário, contemplando pelo menos mídia, aliases, entradas nas listas, progresso, arquivos locais, operações de sincronização, configurações e versão das migrações.

Usar transações, foreign keys habilitadas, `WAL` quando adequado e `busy_timeout`. O banco real do usuário não deve ser usado diretamente nos testes; usar fixtures e bancos temporários.

Para distribuição Qt no Windows, incluir o módulo `QtSql` e o plugin de driver SQLite (`sqldrivers/qsqlite.dll`) junto ao executável. O `sqlite3.exe` não é necessário para a aplicação e deve ser tratado apenas como ferramenta opcional de desenvolvimento. Usar `windeployqt` como base do empacotamento e verificar manualmente se o diretório `sqldrivers` foi incluído.
