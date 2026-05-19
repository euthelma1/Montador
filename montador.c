#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 1000
#define MAX_LEN   1000
#define MAX_EQU   100

void runSimulator(const char *objFile);
void generateObj(const char *preFile);

//auxiliar
int endsWith(const char *str, const char *ext) {
    int ls = strlen(str), le = strlen(ext);
    if (le > ls) return 0;
    return strcmp(str + ls - le, ext) == 0;
}

//util

void upper(char *s) {
    for (int i = 0; s[i]; i++) s[i] = toupper((unsigned char)s[i]);
}

void removeComment(char *s) {
    char *p = strchr(s, ';');
    if (p) *p = '\0';
}

int isEmpty(const char *s) {
    for (int i = 0; s[i]; i++)
        if (!isspace((unsigned char)s[i])) return 0;
    return 1;
}

//extrai próximo token a partir de src[*pos]
// retorna 1 se achou token, 0 se chegou ao fim
int nextToken(const char *src, int *pos, char *dest) {
    while (src[*pos] && isspace((unsigned char)src[*pos])) (*pos)++;
    if (!src[*pos]) return 0;
    int i = 0;
    while (src[*pos] && !isspace((unsigned char)src[*pos]))
        dest[i++] = src[(*pos)++];
    dest[i] = '\0';
    return 1;
}


// tabela EQU

typedef struct { char name[50]; char value[50]; } Equ;
Equ  equTable[MAX_EQU];
int  equCount = 0;

void addEqu(const char *name, const char *value) {
    strcpy(equTable[equCount].name,  name);
    strcpy(equTable[equCount].value, value);
    equCount++;
}

const char *findEqu(const char *name) {
    for (int i = 0; i < equCount; i++)
        if (strcmp(equTable[i].name, name) == 0)
            return equTable[i].value;
    return NULL;
}

// substitui tokens EQU dentro de uma linha 
// trata vírgulas coladas sem quebrar o COPY
void applyEqu(char *line) {
    char result[MAX_LEN] = "";
    int i = 0;
    while (line[i]) {
        if (isspace((unsigned char)line[i])) {
            int start = i;
            while (line[i] && isspace((unsigned char)line[i])) i++;
            strncat(result, line + start, i - start);
        } else {
            char tok[MAX_LEN] = "";
            int j = 0;
            while (line[i] && !isspace((unsigned char)line[i])) tok[j++] = line[i++];
            tok[j] = '\0';

            // divide por virgula para substituir cada sub-token separadamente
            char tmp[MAX_LEN];
            strcpy(tmp, tok);
            char *p = tmp;
            int first = 1;
            while (*p) {
                char sub[MAX_LEN] = "";
                int k = 0;
                while (*p && *p != ',') sub[k++] = *p++;
                sub[k] = '\0';
                if (*p == ',') p++;

                const char *eq = findEqu(sub);
                if (!first) strcat(result, ",");
                strcat(result, eq ? eq : sub);
                first = 0;
            }
        }
    }
    strcpy(line, result);
}

//tabela de instrucoes 

typedef struct { char *name; int opcode; int size; } Instr;

Instr inst[] = {
    {"ADD",1,2},{"SUB",2,2},{"MULT",3,2},{"DIV",4,2},
    {"JMP",5,2},{"JMPN",6,2},{"JMPP",7,2},{"JMPZ",8,2},
    {"COPY",9,3},{"LOAD",10,2},{"STORE",11,2},
    {"INPUT",12,2},{"OUTPUT",13,2},{"STOP",14,1}
};
int instCount = sizeof(inst) / sizeof(inst[0]);

int findInst(const char *n) {
    for (int i = 0; i < instCount; i++)
        if (strcmp(n, inst[i].name) == 0) return i;
    return -1;
}


//  TABELA DE SÍMBOLOS


typedef struct { char name[50]; int value; int defined; int list_head; } Symbol;
Symbol symTable[500];
int    symCount = 0;

int findSym(const char *n) {
    for (int i = 0; i < symCount; i++)
        if (strcmp(symTable[i].name, n) == 0) return i;
    return -1;
}

int getOrCreateSym(const char *n) {
    int idx = findSym(n);
    if (idx != -1) return idx;
    
    strcpy(symTable[symCount].name, n);
    symTable[symCount].value = -1;
    symTable[symCount].defined = 0;
    symTable[symCount].list_head = -1; // -1 indica fim da lista
    symCount++;
    return symCount - 1;
}

void addSym(const char *n, int addr) {
    strcpy(symTable[symCount].name, n);
    symTable[symCount].value = addr;
    symCount++;
}

// PARSE de uma linha .pre
// extrai label sem ':', op, arg1, arg2

void parseLine(const char *line, char *label, char *op, char *arg1, char *arg2) {
    label[0] = op[0] = arg1[0] = arg2[0] = '\0';

    char tmp[MAX_LEN];
    strcpy(tmp, line);
    int pos = 0;

    char tok[MAX_LEN] = "";
    nextToken(tmp, &pos, tok);
    if (!tok[0]) return;

    if (tok[strlen(tok) - 1] == ':') {
        strncpy(label, tok, strlen(tok) - 1);
        label[strlen(tok) - 1] = '\0';
        nextToken(tmp, &pos, tok);
        if (!tok[0]) return;
    }

    strcpy(op, tok);

    char argTok[MAX_LEN] = "";
    nextToken(tmp, &pos, argTok);
    if (!argTok[0]) return;

    char *comma = strchr(argTok, ',');
    if (comma) {
        *comma = '\0';
        strcpy(arg1, argTok);
        strcpy(arg2, comma + 1);
    } else {
        strcpy(arg1, argTok);
        nextToken(tmp, &pos, argTok);
        if (argTok[0]) strcpy(arg2, argTok);
    }
}

// Normalizacao

void normalizeLines(char raw[][MAX_LEN], int rawCount,
                    char out[][MAX_LEN], int *outCount) {

    //remover comentarios + maiusculo + linha vazia
    char clean[MAX_LINES][MAX_LEN];
    int  cleanCount = 0;
    for (int i = 0; i < rawCount; i++) {
        char tmp[MAX_LEN];
        strcpy(tmp, raw[i]);
        removeComment(tmp);
        upper(tmp);
        if (!isEmpty(tmp))
            strcpy(clean[cleanCount++], tmp);
    }

    //coletar EQUs e remove-los do fluxo
    char afterEqu[MAX_LINES][MAX_LEN];
    int  afterEquCount = 0;
    for (int i = 0; i < cleanCount; i++) {
        char tmp[MAX_LEN];
        strcpy(tmp, clean[i]);
        char t1[50] = "", t2[50] = "", t3[50] = "";
        int pos = 0;
        nextToken(tmp, &pos, t1);
        nextToken(tmp, &pos, t2);
        nextToken(tmp, &pos, t3);

        if (strcmp(t2, "EQU") == 0 && t1[0] && t3[0]) {
            int len = strlen(t1);
            if (t1[len - 1] == ':') t1[len - 1] = '\0';
            addEqu(t1, t3);
            continue;
        }
        strcpy(afterEqu[afterEquCount++], clean[i]);
    }

    //  IF/ENDIF
    char afterIf[MAX_LINES][MAX_LEN];
    int  afterIfCount = 0;
    int  skipping = 0;
    for (int i = 0; i < afterEquCount; i++) {
        char tmp[MAX_LEN];
        strcpy(tmp, afterEqu[i]);
        char t1[50] = "", t2[50] = "";
        int pos = 0;
        nextToken(tmp, &pos, t1);
        nextToken(tmp, &pos, t2);

        if (strcmp(t1, "IF") == 0) {
            const char *val = findEqu(t2);
            skipping = (val == NULL || strcmp(val, "0") == 0) ? 1 : 0;
            continue;
        }
        
        // Se a flag estiver ativada (1)
        if (skipping) {
            skipping = 0; // "Desliga" o pulo para que a próxima linha seja salva normalmente
            continue;
        }

        // Se chegou aqui e não estava pulando, salva a linha
        strcpy(afterIf[afterIfCount++], afterEqu[i]);
    }

    //substituir aliases EQU
    for (int i = 0; i < afterIfCount; i++)
        applyEqu(afterIf[i]);

    //reconstroi cada linha com 1 espaço entre tokens
    char norm[MAX_LINES][MAX_LEN];
    int  normCount = 0;
    for (int i = 0; i < afterIfCount; i++) {
        char tmp[MAX_LEN];
        strcpy(tmp, afterIf[i]);
        char tokens[10][MAX_LEN];
        int  tc = 0, pos = 0;
        char tok[MAX_LEN];
        while (nextToken(tmp, &pos, tok) && tc < 10)
            strcpy(tokens[tc++], tok);
        if (tc == 0) continue;

        char rebuilt[MAX_LEN] = "";
        for (int t = 0; t < tc; t++) {
            if (t > 0) strcat(rebuilt, " ");
            strcat(rebuilt, tokens[t]);
        }
        strcpy(norm[normCount++], rebuilt);
    }

    //juntar rotulo sozinho com linha seguinte
    *outCount = 0;
    for (int i = 0; i < normCount; i++) {
        char tmp[MAX_LEN];
        strcpy(tmp, norm[i]);
        char tok[MAX_LEN] = "";
        int  pos = 0;
        nextToken(tmp, &pos, tok);
        while (tmp[pos] && isspace((unsigned char)tmp[pos])) pos++;
        int soloLabel = (tok[0] && tok[strlen(tok) - 1] == ':' && tmp[pos] == '\0');

        if (soloLabel && i + 1 < normCount) {
            char combined[MAX_LEN];
            snprintf(combined, MAX_LEN, "%s %s", tok, norm[i + 1]);
            strcpy(norm[i + 1], combined);
        } else {
            strcpy(out[(*outCount)++], tmp);
        }
    }
}

// pre processamento 


void preprocess(const char *file) {
    FILE *in = fopen(file, "r");
    if (!in) { printf("Erro ao abrir arquivo\n"); exit(1); }

    char raw[MAX_LINES][MAX_LEN];
    int  rawCount = 0;
    char line[MAX_LEN];
    while (fgets(line, MAX_LEN, in) && rawCount < MAX_LINES) {
        line[strcspn(line, "\n")] = '\0';
        strcpy(raw[rawCount++], line);
    }
    fclose(in);

    char norm[MAX_LINES][MAX_LEN];
    int  normCount = 0;
    normalizeLines(raw, rawCount, norm, &normCount);

    //separar TEXT e DATA 
    char text[MAX_LINES][MAX_LEN]; int tc = 0;
    char data[MAX_LINES][MAX_LEN]; int dc = 0;
    int section = 0;

    for (int i = 0; i < normCount; i++) {
        char tmp[MAX_LEN];
        strcpy(tmp, norm[i]);
        char t1[50] = "", t2[50] = "";
        int pos = 0;
        nextToken(tmp, &pos, t1);
        nextToken(tmp, &pos, t2);

        if (strcmp(t1, "SECTION") == 0) {
            if (strcmp(t2, "TEXT") == 0) { section = 1; continue; }
            if (strcmp(t2, "DATA") == 0) { section = 2; continue; }
        }
        if (section == 1) strcpy(text[tc++], norm[i]);
        else if (section == 2) strcpy(data[dc++], norm[i]);
    }

    // monta nome do .pre 
    char outName[MAX_LEN];
    strcpy(outName, file);
    char *dot = strrchr(outName, '.');
    if (dot) *dot = '\0';
    strcat(outName, ".pre");

    FILE *o = fopen(outName, "w");
    if (!o) { printf("Erro ao criar .pre\n"); exit(1); }
    for (int i = 0; i < tc; i++) fprintf(o, "%s\n", text[i]);
    for (int i = 0; i < dc; i++) fprintf(o, "%s\n", data[i]);
    fclose(o);

    printf("Pré-processamento concluído: %s\n", outName);
}


// passagem unica com lista encadeada
void onePass(const char *preFile) {
    FILE *f = fopen(preFile, "r");
    if (!f) { printf("Erro ao abrir .pre\n"); exit(1); }

    char penName[MAX_LEN];
    strcpy(penName, preFile);
    char *dot = strrchr(penName, '.');
    if (dot) *dot = '\0';
    strcat(penName, ".pen");

    FILE *o = fopen(penName, "w");
    if (!o) { printf("Erro ao criar .pen\n"); exit(1); }

    int LC = 0;
    char line[MAX_LEN];
    
    symCount = 0;

    while (fgets(line, MAX_LEN, f)) {
        line[strcspn(line, "\n")] = '\0';
        if (isEmpty(line)) continue;

        char label[50], op[50], arg1[50], arg2[50];
        parseLine(line, label, op, arg1, arg2);

        // Se tem rótulo, define o símbolo
        if (label[0]) {
            int s = getOrCreateSym(label);
            symTable[s].value = LC;
            symTable[s].defined = 1;
        }

        int idx = findInst(op);
        if (idx != -1) {
            fprintf(o, "%d ", inst[idx].opcode);

            if (inst[idx].size >= 2) {
                int s = getOrCreateSym(arg1);
                if (symTable[s].defined) {
                    fprintf(o, "%d ", symTable[s].value);
                } else {
                    fprintf(o, "%d ", symTable[s].list_head);
                    symTable[s].list_head = LC + 1; // Guarda a posição da pendência
                }
            }
            if (inst[idx].size == 3) {
                int s = getOrCreateSym(arg2);
                if (symTable[s].defined) {
                    fprintf(o, "%d ", symTable[s].value);
                } else {
                    fprintf(o, "%d ", symTable[s].list_head);
                    symTable[s].list_head = LC + 2; // Guarda a posição da pendência
                }
            }
            LC += inst[idx].size;
            
        } else if (strcmp(op, "CONST") == 0) {
            int v = 0;
            if (strncmp(arg1, "0X", 2) == 0 || strncmp(arg1, "-0X", 3) == 0)
                v = (int)strtol(arg1, NULL, 16);
            else
                v = atoi(arg1);
            fprintf(o, "%d ", v);
            LC += 1;

        } else if (strcmp(op, "SPACE") == 0) {
            int n = arg1[0] ? atoi(arg1) : 1;
            if (n < 1) n = 1;
            for (int i = 0; i < n; i++) fprintf(o, "0 ");
            LC += n;
        }
    }

    fclose(f);
    fclose(o);
    printf(".pen gerado: %s\n", penName);
}

// Generate .obj
void generateObj(const char *preFile) {
    char penName[MAX_LEN];
    strcpy(penName, preFile);
    char *dot = strrchr(penName, '.');
    if (dot) *dot = '\0';
    strcat(penName, ".pen");

    FILE *f = fopen(penName, "r");
    if (!f) {
        printf("Erro ao abrir %s para geracao do .obj\n", penName);
        exit(1);
    }

   
    int code[5000];
    int codeSize = 0;
    while (fscanf(f, "%d", &code[codeSize]) == 1) {
        codeSize++;
    }
    fclose(f);

    // Resolve as pendências
    for (int i = 0; i < symCount; i++) {
        if (!symTable[i].defined) {
            printf("ERRO: Símbolo '%s' foi usado mas nunca definido!\n", symTable[i].name);
            exit(1);
        }
        
        int curr = symTable[i].list_head;
        while (curr != -1) {
            int next = code[curr];         
            code[curr] = symTable[i].value; 
            curr = next;                    
        }
    }

    // .obj 
    char objName[MAX_LEN];
    strcpy(objName, preFile);
    dot = strrchr(objName, '.');
    if (dot) *dot = '\0';
    strcat(objName, ".obj");

    FILE *o = fopen(objName, "w");
    if (!o) {
        printf("Erro ao criar .obj\n");
        exit(1);
    }

    for (int i = 0; i < codeSize; i++) {
        if (i > 0) fprintf(o, " ");
        fprintf(o, "%d", code[i]);
    }

    fclose(o);
    printf(".obj gerado com sucesso: %s\n", objName);
}

void runSimulator(const char *objFile) {
    FILE *f = fopen(objFile, "r");
    if (!f) {
        printf("Erro ao abrir ficheiro .obj\n");
        exit(1);
    }

    int memoria[5000];
    int memSize = 0;
    
    while (fscanf(f, "%d", &memoria[memSize]) == 1) {
        memSize++;
    }
    fclose(f);

    int ACC = 0;
    int PC = 0;
    int rodando = 1;

    while (rodando && PC < memSize) {
        int opcode = memoria[PC];

        switch (opcode) {
            case 1: // ADD: ACC = ACC + memory[memory[PC+1]]
                ACC += memoria[memoria[PC + 1]];
                PC += 2;
                break;
            case 2: // SUB: ACC = ACC - memory[memory[PC+1]]
                ACC -= memoria[memoria[PC + 1]];
                PC += 2;
                break;
            case 3: // MULT: ACC = ACC * memory[memory[PC+1]]
                ACC *= memoria[memoria[PC + 1]];
                PC += 2;
                break;
            case 4: // DIV: ACC = ACC / memory[memory[PC+1]]
                if (memoria[memoria[PC + 1]] == 0) {
                    printf("Erro: Divisao por zero!\n");
                    rodando = 0;
                } else {
                    ACC /= memoria[memoria[PC + 1]];
                    PC += 2;
                }
                break;
            case 5: // JMP: PC = memory[PC+1] 
                PC = memoria[PC + 1];
                break;
            case 6: // JMPN: pula se ACC < 0
                if (ACC < 0) PC = memoria[PC + 1];
                else PC += 2;
                break;
            case 7: // JMPP: pula se ACC > 0
                if (ACC > 0) PC = memoria[PC + 1];
                else PC += 2;
                break;
            case 8: // JMPZ: pula se ACC == 0
                if (ACC == 0) PC = memoria[PC + 1];
                else PC += 2;
                break;
            case 9: // COPY: memory[memory[PC+2]] = memory[memory[PC+1]]
                memoria[memoria[PC + 2]] = memoria[memoria[PC + 1]];
                PC += 3;
                break;
            case 10: // LOAD: ACC = memory[memory[PC+1]]
                ACC = memoria[memoria[PC + 1]];
                PC += 2;
                break;
            case 11: // STORE: memory[memory[PC+1]] = ACC
                memoria[memoria[PC + 1]] = ACC;
                PC += 2;
                break;
            case 12: // INPUT: lê para memory[memory[PC+1]]
                printf("INPUT: ");
                fflush(stdout);
                if (scanf("%d", &memoria[memoria[PC + 1]]) != 1) {
                    printf("Erro na leitura de dados.\n");
                }
                PC += 2;
                break;
            case 13: // OUTPUT: imprime memory[memory[PC+1]]
                printf("OUTPUT: %d\n", memoria[memoria[PC + 1]]);
                PC += 2;
                break;
            case 14: // STOP
                rodando = 0;
                break;
            default:
                printf("Erro: Opcode desconhecido %d no endereco %d\n", opcode, PC);
                rodando = 0;
                break;
        }
    }
    printf("Simulacao terminada com sucesso.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso:\n");
        printf("  ./ASSEMBLER arquivo.asm   -> gera arquivo.pre\n");
        printf("  ./ASSEMBLER arquivo.pre   -> gera arquivo.pen\n");
        printf("  ./ASSEMBLER arquivo.obj   -> roda o simulador\n");
        return 1;
    }

    if (endsWith(argv[1], ".asm")) {
        preprocess(argv[1]);

    } else if (endsWith(argv[1], ".pre")) {
        printf("Executando Montagem Final para: %s\n", argv[1]);
        onePass(argv[1]);
        generateObj(argv[1]);
    } else if (endsWith(argv[1], ".obj")) {
        printf("Iniciando o simulador para: %s\n", argv[1]);
        runSimulator(argv[1]);
    } else {
        printf("Extensão inválida. Use .asm ou .pre\n");
        return 1;
    }

    return 0;
}
