
; definições EQU
VERDADE  EQU 1
FALSO    EQU 0
LIMITE   EQU 5
NEG      EQU -3
HEXA     EQU 0xFF

; DATA vem antes de TEXT — deve ser invertido no .pre
SECTION DATA
N1:     SPACE
N2:     SPACE
N3:     SPACE
RESULT: SPACE
CONT:   CONST LIMITE    ; deve virar CONST 5
NEG_V:  CONST NEG       ; deve virar CONST -3
HEX_V:  CONST HEXA      ; deve virar CONST 0XFF

SECTION TEXT

; teste de tabs e espaços extras
	INPUT		N1   	; lê primeiro número
    INPUT   N2          ; lê segundo número

; rótulo sozinho na linha
INICIO:
    LOAD    N1
    ADD     N2
    STORE   RESULT

; IF verdadeiro — bloco deve aparecer no .pre
IF VERDADE
    OUTPUT RESULT
ENDIF

; IF falso — bloco deve sumir
IF FALSO
    OUTPUT N1
    OUTPUT N2
ENDIF

; COPY com vírgula sem espaço
    COPY N1,N2

; loop simples com forward reference para .pen
LOOP:
    LOAD    CONT
    JMPZ    FIM
    SUB     N1
    STORE   CONT
    JMP     LOOP

FIM:
    OUTPUT  RESULT
    STOP
