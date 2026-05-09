#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 1000
#define MAX_LEN   1000
#define MAX_EQU   100

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


typedef struct { char name[50]; int addr; } Symbol;
Symbol symTable[500];
int    symCount = 0;

int findSym(const char *n) {
    for (int i = 0; i < symCount; i++)
        if (strcmp(symTable[i].name, n) == 0) return i;
    return -1;
}

void addSym(const char *n, int addr) {
    strcpy(symTable[symCount].name, n);
    symTable[symCount].addr = addr;
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
        if (strcmp(t1, "ENDIF") == 0) { skipping = 0; continue; }
        if (!skipping)
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

// passagem unica


void onePass(const char *preFile) {
    FILE *f = fopen(preFile, "r");
    if (!f) { printf("Erro ao abrir .pre\n"); exit(1); }

    char line[MAX_LEN];
    int  LC = 0;
    while (fgets(line, MAX_LEN, f)) {
        line[strcspn(line, "\n")] = '\0';
        if (isEmpty(line)) continue;
        char label[50], op[50], arg1[50], arg2[50];
        parseLine(line, label, op, arg1, arg2);

        if (label[0]) {
            if (findSym(label) == -1) addSym(label, LC);
        }
        int idx = findInst(op);
        if (idx != -1)               LC += inst[idx].size;
        else if (strcmp(op, "SPACE") == 0) {
            int n = arg1[0] ? atoi(arg1) : 1;
            LC += (n < 1) ? 1 : n;
        }
        else if (strcmp(op, "CONST") == 0) LC += 1;
    }
    fclose(f);

    char penName[MAX_LEN];
    strcpy(penName, preFile);
    char *dot = strrchr(penName, '.');
    if (dot) *dot = '\0';
    strcat(penName, ".pen");

    f = fopen(preFile, "r");
    FILE *o = fopen(penName, "w");
    if (!f || !o) { printf("Erro ao criar .pen\n"); exit(1); }

    LC = 0;
    while (fgets(line, MAX_LEN, f)) {
        line[strcspn(line, "\n")] = '\0';
        if (isEmpty(line)) continue;
        char label[50], op[50], arg1[50], arg2[50];
        parseLine(line, label, op, arg1, arg2);

        int idx = findInst(op);
        if (idx != -1) {
            fprintf(o, "%d ", inst[idx].opcode);

            if (inst[idx].size >= 2) {
                int s = findSym(arg1);
                if (s == -1 || symTable[s].addr > LC)
                    fprintf(o, "-1 ");
                else
                    fprintf(o, "%d ", symTable[s].addr);
            }
            if (inst[idx].size == 3) {
                int s = findSym(arg2);
                if (s == -1 || symTable[s].addr > LC)
                    fprintf(o, "-1 ");
                else
                    fprintf(o, "%d ", symTable[s].addr);
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso:\n");
        printf("  ./montador arquivo.asm   -> gera arquivo.pre\n");
        printf("  ./montador arquivo.pre   -> gera arquivo.pen\n");
        return 1;
    }

    if (endsWith(argv[1], ".asm")) {
        preprocess(argv[1]);

    } else if (endsWith(argv[1], ".pre")) {
        onePass(argv[1]);

    } else {
        printf("Extensão inválida. Use .asm ou .pre\n");
        return 1;
    }

    return 0;
}
