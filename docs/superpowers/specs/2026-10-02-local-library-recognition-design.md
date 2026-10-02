# Reconhecimento de biblioteca local — Especificação arquitetural

## Objetivo

Associar arquivos de vídeo encontrados na biblioteca local às mídias de anime já cadastradas no banco de dados, usando a Anitomy V1 para extrair título e episódio do nome do arquivo. A associação persistida permitirá que o detalhe de um anime abra o próximo episódio disponível em disco pelo player padrão do sistema.

Esta etapa amplia a fundação de inventário local existente. O escaneamento de diretórios continua sendo responsável por descobrir e atualizar arquivos; o reconhecimento passa a consumir esse inventário sem ler o conteúdo dos vídeos.

## Escopo

### Incluído

- Suporte inicial somente para anime.
- Uso da Anitomy V1 já presente em `lib/anitomy`.
- Extração de título, temporada quando disponível e número do episódio.
- Resolução estrita contra as mídias já persistidas localmente, usando título principal, título em inglês, título original, títulos alternativos e sinônimos disponíveis no catálogo.
- Persistência do resultado de reconhecimento e da associação arquivo–mídia–episódio.
- Idempotência por identidade lógica do arquivo: diretório raiz persistido mais caminho relativo normalizado.
- Reprocessamento somente quando o registro ainda não tiver resultado definitivo ou quando a identidade/metadados relevantes do arquivo mudarem conforme o contrato definido na implementação.
- Consulta do próximo episódio disponível para uma mídia de anime.
- Ação `Assistir` no detalhe do anime, delegada ao controller/backend.
- Abertura do caminho local pelo player padrão do sistema, sem lógica de player embutida no QML.
- Testes unitários, de repositório, de integração de composição e de estrutura QML.

### Fora do escopo

- Reconhecimento de mangas ou novels nesta etapa.
- Consulta de AniList, busca na internet ou criação automática de novas mídias.
- Leitura do conteúdo, duração, codecs, hashes ou metadados internos dos vídeos.
- Heurísticas amplas de fuzzy matching além da normalização necessária para comparar os títulos.
- Seleção manual de uma associação ambígua na interface.
- Controle de reprodução, pausa, progresso do player ou escolha de player configurável.
- Alteração da arquitetura visual da tela de detalhes.

## Decisões arquiteturais

### Separação das etapas

O fluxo será separado em quatro responsabilidades:

1. `LocalLibraryScanner` enumera arquivos aceitos e mantém o inventário físico.
2. Um reconhecedor de nomes converte `fileName` em um resultado tipado da Anitomy V1.
3. Um resolvedor compara o título extraído com o catálogo local e decide entre associado, não reconhecido ou ambíguo.
4. Um repositório persiste o estado e fornece consultas para o próximo episódio e para a abertura do arquivo.

O controller de apresentação apenas solicita a ação, expõe estado e encaminha o caminho retornado. Ordenação, filtros, regras de associação e acesso ao banco permanecem fora do QML.

### Suporte inicial e extensibilidade

O reconhecimento deve receber um tipo de mídia ou uma política de reconhecimento, mas a primeira política habilitada será `Anime`. O modelo persistido deve permitir distinguir o tipo de mídia sem presumir que todo arquivo futuro seja anime. Mangas e novels serão avaliados em uma etapa posterior, com seus próprios padrões de capítulo/volume e extensões, sem ampliar o escopo desta implementação.

### Contrato de reconhecimento

O resultado intermediário deve representar, no mínimo:

- nome e caminho do arquivo de origem;
- título extraído;
- número do episódio, quando existente;
- temporada, quando existente;
- tipo de mídia reconhecido;
- estado do reconhecimento;
- diagnóstico não sensível em caso de falha.

Arquivos sem título ou sem episódio numérico válido não devem ser associados automaticamente. Episódios especiais, intervalos de episódios e múltiplos números devem ser classificados explicitamente como não suportados ou ambíguos na primeira versão, sem escolher silenciosamente um episódio incorreto.

### Resolução estrita

O título será normalizado de forma determinística para comparação — diferenças de maiúsculas/minúsculas, espaços e separadores equivalentes não devem criar divergências — preservando o texto original para diagnóstico. A resolução consultará apenas as variantes de título já persistidas para a mídia.

Resultados possíveis:

- `associated`: exatamente uma mídia local corresponde e há episódio válido;
- `unrecognized`: nenhum título local corresponde ou os dados mínimos não foram extraídos;
- `ambiguous`: mais de uma mídia corresponde ou o episódio não pode ser escolhido sem risco;
- `unsupported`: o tipo ou formato não pertence ao suporte inicial.

O estado reconhecido não deve ser confundido com associação: um título pode ser reconhecido pela Anitomy, mas não corresponder a uma mídia cadastrada.

### Idempotência e mudança de arquivo

O banco deve possuir uma restrição única por raiz configurada e caminho relativo normalizado, aproveitando o inventário existente. Uma nova execução deve consultar esse registro antes de chamar novamente a associação. Registros com resultado definitivo não serão relacionados uma segunda vez.

Se o arquivo for alterado de maneira observável pelo inventário — por exemplo, nome/caminho normalizado, tamanho ou data de modificação — a política de invalidação deverá marcar o reconhecimento como pendente e permitir uma nova tentativa. Arquivos apenas ausentes continuam sujeitos à reconciliação do scanner e não devem ser apagados automaticamente junto com o vínculo histórico nesta etapa.

### Persistência

A evolução do esquema deve manter separadas as informações físicas e semânticas. O registro local deve conservar o caminho, disponibilidade e metadados do arquivo, além de campos de reconhecimento e associação, incluindo no mínimo:

- estado do reconhecimento;
- título extraído;
- tipo de mídia;
- temporada opcional;
- episódio opcional;
- `media_id` opcional;
- diagnóstico e timestamps de reconhecimento.

As alterações devem usar a migração SQLite existente, SQL externo em `resources/sqlite/queries/` e configuração em `resources/sqlite/sqlite-queries.json`. A gravação de uma associação deve ser transacional e não pode deixar `media_id` preenchido quando o estado não for `associated`.

### Próximo episódio

Para uma mídia de anime, o backend consultará somente arquivos associados, disponíveis e com episódio numérico válido. O próximo episódio será o menor número maior que o progresso consumido da mídia. Se não houver progresso consumido, o primeiro episódio disponível será retornado. Empates do mesmo episódio devem ser resolvidos por uma política determinística documentada — preferencialmente caminho normalizado — e nunca por ordem incidental do sistema de arquivos.

Se nenhum episódio elegível existir, o controller deve expor `Assistir` desabilitado e uma mensagem de estado apropriada. A consulta não deve abrir o arquivo nem atualizar progresso automaticamente.

### Abertura do arquivo

Uma porta de aplicação deverá receber o caminho validado pelo repositório e delegar a abertura ao mecanismo padrão do sistema (`QDesktopServices` ou adaptador equivalente). O caminho deve ser convertido para uma URL de arquivo de forma segura. Falhas de abertura retornam diagnóstico ao controller; não há fallback silencioso para outro arquivo.

## Integração com a apresentação

O detalhe existente do anime deve receber apenas as propriedades e ações necessárias, preservando a composição atual:

- `canWatch` ou equivalente;
- episódio que será aberto, quando conhecido;
- estado/mensagem de biblioteca local;
- invocável `WatchNext()` ou nome equivalente segundo o padrão atual.

O texto do botão será `Assistir`. O QML não deve listar diretórios, ordenar episódios, comparar títulos, montar SQL ou construir caminhos. A ação deve ser segura quando a mídia não estiver selecionada, for de tipo diferente de anime, estiver sem associação ou o arquivo tiver ficado indisponível desde a consulta.

## Ciclo de execução

O reconhecimento deve executar fora da thread da interface e usar conexão SQLite pertencente à thread de trabalho, seguindo o padrão já adotado pelo scanner. O fluxo recomendado é:

1. concluir ou obter um inventário válido;
2. selecionar registros pendentes ou alterados;
3. reconhecer nomes com Anitomy V1;
4. resolver títulos contra um snapshot local do catálogo;
5. persistir resultados em lotes transacionais;
6. publicar contagens de processados, associados, não reconhecidos, ambíguos e falhos;
7. atualizar o detalhe da mídia sem bloquear a abertura da janela.

Falhas de reconhecimento não devem impedir a aplicação ou a biblioteca catalogada de abrir. O logging deve ser agregado por execução, sem registrar um evento de sucesso para cada arquivo e sem expor caminhos desnecessariamente.

## Critérios de aceitação

- Um nome de anime com título e episódio válidos é associado à única mídia correspondente no banco.
- Títulos alternativos e sinônimos persistidos resolvem sem exigir consulta externa.
- Uma segunda execução não repete a associação de um arquivo já definitivo.
- Um arquivo alterado pode ser reprocessado segundo a política de invalidação, sem duplicar sua linha lógica.
- Títulos desconhecidos, ambiguidades, especiais e episódios inválidos não são associados silenciosamente.
- A associação permanece disponível após reiniciar a aplicação.
- `Assistir` abre o menor episódio disponível acima do progresso consumido.
- `Assistir` permanece desabilitado quando não existe episódio elegível ou quando o arquivo está indisponível.
- Abertura, reconhecimento e persistência não são executados pelo QML.
- A extensão futura para manga/novel permanece possível sem fingir suporte nesta etapa.

## Riscos e decisões adiadas

- Títulos de fansub podem não coincidir com as variantes do catálogo; ampliar heurísticas exige uma etapa própria com fixtures e métricas de falsos positivos.
- O tratamento de episódios especiais e intervalos será apenas classificação segura nesta versão; uma política de reprodução para esses casos será decidida posteriormente.
- A definição exata de “arquivo alterado” — tamanho, data, caminho e eventual conteúdo — deve ser fechada no plano de implementação com base no esquema já existente.
- O player padrão pode não aceitar todos os formatos; a primeira versão reportará a falha do sistema sem criar um player interno.

