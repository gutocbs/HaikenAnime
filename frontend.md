# Frontend da V2

## Objetivo

Transpor o frontend principal da V1 para a V2, preservando inicialmente a mesma organização e o comportamento geral. A primeira entrega deve estabelecer uma base visual funcional, sem conexão com backend, autenticação ou ações que alterem dados.

## Estratégia por etapas

1. Consultar a V1 como referência visual e comportamental.
2. Recriar a tela inicial na V2 usando Qt Quick/QML.
3. Manter inicialmente uma única tela.
4. Criar estados visuais de carregamento, vazio e erro como componentes estáticos.
5. Usar dados temporários apenas para validar a composição visual e o comportamento básico.
6. Integrar posteriormente a tela aos dados salvos no banco.
7. Após a integração inicial funcionar, pausar a evolução do frontend até que novas partes do backend estejam disponíveis.
8. Retomar a evolução de forma intercalada com o desenvolvimento do backend.

## Escopo inicial

Incluído:

- Tela inicial.
- Organização geral inspirada na V1.
- Componentes de conteúdo, carregamento, estado vazio e erro.
- Layout adaptável ao tamanho da janela.
- Dados estáticos ou controlados para validação inicial.

Não incluído inicialmente:

- Conexões com o backend.
- Autenticação.
- Botões com efeitos persistentes ou chamadas externas.
- Sincronização, downloads, busca avançada ou mutations.

## Responsabilidades do frontend

O QML deve renderizar os dados e estados recebidos pelo modelo/controller da tela. Ele não deve decidir:

- quais dados devem ser exibidos;
- como os dados devem ser ordenados;
- como os dados devem ser separados ou agrupados;
- quais regras de negócio devem ser aplicadas.

Melhorias visuais poderão ser avaliadas depois que a tela estiver funcional e integrada. Essas melhorias devem preservar a organização e o comportamento definidos, salvo nova decisão explícita.

## Organização esperada

- `src/presentation/`: controllers, modelos e adaptadores específicos da apresentação.
- `resources/`: arquivos QML, ícones e demais recursos visuais.
- Uma classe de apresentação/integração específica para cada tela ou fluxo relevante.
- Componentes compartilhados somente quando houver comportamento ou apresentação realmente reutilizável.

A organização deve evitar tanto regras de negócio nos arquivos QML quanto uma classe central monolítica equivalente ao antigo `MainClass`.

## Critérios da primeira entrega visual

- A aplicação inicia diretamente na tela inicial.
- A tela pode ser visualizada sem backend disponível.
- Os estados carregando, vazio e erro podem ser apresentados de forma determinística.
- A tela mantém a organização geral da V1.
- O build da V2 continua reproduzível pelo CMake.
- A validação visual é feita em mais de um tamanho de janela.
