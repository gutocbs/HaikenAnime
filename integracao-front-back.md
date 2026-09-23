# Integração entre Frontend e Backend

## Objetivo

Criar uma camada explícita entre a apresentação e os serviços da V2. Essa camada determina quais dados devem ser exibidos pela tela e entrega ao frontend um modelo já preparado para apresentação.

## Princípio central

O frontend apenas recebe e renderiza dados. A QML não deve consultar diretamente o banco, executar regras de negócio, ordenar coleções, filtrar registros ou decidir quais itens pertencem a cada seção da tela.

O fluxo esperado é:

```text
SQLite / fonte de dados
        ↓
Repository
        ↓
Caso de uso ou serviço de aplicação
        ↓
Integração/controller específico da tela
        ↓
Modelo exposto ao QML
        ↓
Tela QML
```

## Responsabilidades por camada

### Repository

- Ler e persistir dados.
- Encapsular SQLite e detalhes de infraestrutura.
- Não conhecer QML ou detalhes de layout.

### Caso de uso ou serviço de aplicação

- Definir quais dados são necessários para o fluxo.
- Aplicar regras de negócio.
- Ordenar, filtrar, separar e agrupar dados.
- Definir contratos de sucesso, vazio e erro.

### Integração/controller da tela

- Orquestrar o carregamento dos dados da tela.
- Expor propriedades, modelos e sinais para o QML.
- Traduzir resultados da aplicação para estados consumíveis pela apresentação.
- Não acessar diretamente implementações concretas de banco quando houver uma interface de aplicação disponível.

### QML

- Renderizar os dados recebidos.
- Exibir os estados visuais definidos pelo controller.
- Encaminhar eventos de apresentação quando existirem.
- Não conter regras de negócio ou decisões sobre o conjunto de dados.

## Organização por tela

Cada tela deverá possuir uma classe de integração/controller própria, por exemplo `HomeScreenController` ou `HomeViewModel`. Essa classe não deve acumular responsabilidades de outras telas.

Comportamentos compartilhados devem ser extraídos para serviços, casos de uso ou componentes reutilizáveis somente quando houver uma necessidade real. Não deve ser recriado um objeto central com todas as responsabilidades da aplicação.

## Primeiro fluxo de integração

O primeiro conjunto de dados deverá representar informações salvas no banco local. A integração inicial será somente de leitura:

1. O controller solicita os dados necessários para a tela inicial.
2. A camada de aplicação consulta o repository apropriado.
3. A camada de aplicação ordena e separa os dados conforme o contrato da tela.
4. O controller publica o estado e os modelos resultantes.
5. O QML renderiza o resultado.

Não haverá criação, edição ou exclusão de dados nessa etapa.

## Estados mínimos

O contrato da integração deve permitir representar, no mínimo:

- `idle`: nenhuma operação iniciada;
- `loading`: carregamento em andamento;
- `ready`: dados disponíveis;
- `empty`: carregamento concluído sem itens para exibir;
- `error`: carregamento concluído com falha.

O erro deve ser apresentado ao frontend sem expor detalhes sensíveis de infraestrutura. Informações técnicas devem permanecer disponíveis para diagnóstico por meio da política de logs.

## Testes e evolução

- Testar a ordenação, separação e agrupamento na camada de aplicação, não no QML.
- Testar o controller com repositories ou casos de uso controlados.
- Validar respostas vazias, falhas e dados incompletos.
- Fazer o primeiro teste de integração com dados reais do banco local somente após a tela estática estar funcional.
- Manter a integração somente leitura até que os contratos de atualização do backend estejam definidos.
