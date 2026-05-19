Trabalho Prático 1 - Software Básico
====================================

Alunos:
- Letícia ---
- Thelma ---

Sistema Operacional Utilizado:
- Linux

Como Compilar:
Abra o terminal na pasta onde está o arquivo fonte e execute o seguinte comando utilizando o compilador GCC:
gcc montador.c -o ASSEMBLER

Como Rodar:
O programa possui 3 modos de execução distintos, que devem ser rodados em sequência:

1. Pré-processamento:
   ./ASSEMBLER teste.asm
   (Este comando irá gerar o arquivo 'teste.pre')

2. Montagem e Geração de Código de Máquina:
   ./ASSEMBLER teste.pre
   (Este comando irá gerar os arquivos 'teste.pen' e 'teste.obj')

3. Simulador:
   ./ASSEMBLER teste.obj
   (Este comando irá carregar o código de máquina na memória e iniciar a simulação, aguardando os inputs do usuário).