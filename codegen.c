#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "quadruplets.h"
#include "symbol_table.h"

/* ==========================================================
   GENERATION DE CODE 8086 (compatible emu8086)
   ----------
   Toutes les valeurs sont sur 16 bits (DW = Define Word).
   Les FLOAT sont representes comme INTEGER (limitation 8086).
   ========================================================== */

static FILE *fout = NULL;

/* ---------- HELPERS ---------- */

/* Verifie si une chaine est un nombre litteral : "42", "-5", "+12" */
static int is_literal(const char *s) {
    if (!s || !*s) return 0;
    if (*s == '-' || *s == '+') s++;
    if (!*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

/* Detecte un acces tableau "T[expr]" et extrait nom et indice */
static int is_array_access(const char *s, char *array_name, char *index_name) {
    const char *lb = strchr(s, '[');
    const char *rb = strchr(s, ']');
    if (!lb || !rb || rb <= lb) return 0;

    int name_len = lb - s;
    if (name_len <= 0 || name_len >= 50) return 0;
    strncpy(array_name, s, name_len);
    array_name[name_len] = '\0';

    int idx_len = rb - lb - 1;
    if (idx_len <= 0 || idx_len >= 50) return 0;
    strncpy(index_name, lb + 1, idx_len);
    index_name[idx_len] = '\0';
    return 1;
}

/* Charge la valeur d'un operande dans un registre (AX ou BX) */
static void load_to_reg(const char *reg, const char *operand) {
    char arr[50], idx[50];
    if (is_literal(operand)) {
        fprintf(fout, "    MOV %s, %s\n", reg, operand);
    } else if (is_array_access(operand, arr, idx)) {
        fprintf(fout, "    MOV SI, %s\n", idx);
        fprintf(fout, "    ADD SI, SI         ; SI = indice * 2 (taille word)\n");
        fprintf(fout, "    MOV %s, %s[SI]\n", reg, arr);
    } else {
        fprintf(fout, "    MOV %s, %s\n", reg, operand);
    }
}

/* Stocke AX dans une variable ou un element de tableau */
static void store_from_ax(const char *operand) {
    char arr[50], idx[50];
    if (is_array_access(operand, arr, idx)) {
        fprintf(fout, "    PUSH AX\n");
        fprintf(fout, "    MOV SI, %s\n", idx);
        fprintf(fout, "    ADD SI, SI\n");
        fprintf(fout, "    POP AX\n");
        fprintf(fout, "    MOV %s[SI], AX\n", arr);
    } else {
        fprintf(fout, "    MOV %s, AX\n", operand);
    }
}

/* ==========================================================
   SEGMENT DATA : declaration des variables et temporaires
   ========================================================== */
static void gen_data_segment(void) {
    fprintf(fout, "DATA SEGMENT\n");

    /* Variables, constantes et tableaux declares par l'utilisateur */
    for (int i = 0; i < TABLE_SIZE; i++) {
        SymbolEntry *e = table[i];
        while (e) {
            if (e->is_declared) {
                if (e->nature == NAT_CONSTANT) {
                    fprintf(fout, "    %-10s DW %d         ; CONSTANTE\n",
                            e->name, (int)e->const_value);
                } else if (e->nature == NAT_ARRAY) {
                    fprintf(fout, "    %-10s DW %d DUP(0) ; TABLEAU\n",
                            e->name, e->array_size);
                } else {
                    fprintf(fout, "    %-10s DW 0          ; VARIABLE\n", e->name);
                }
            }
            e = e->next;
        }
    }

    /* Temporaires utilises dans les quadruplets */
    char declared[1000][16];
    int nb_declared = 0;
    for (int i = 0; i < quad_count; i++) {
        const char *fields[3] = {quads[i].arg1, quads[i].arg2, quads[i].result};
        for (int k = 0; k < 3; k++) {
            const char *s = fields[k];
            if (s[0] != 'T' || s[1] == '\0') continue;
            /* Verifier que c'est bien Txxx (chiffres apres T) */
            int ok = 1;
            for (int p = 1; s[p]; p++) {
                if (s[p] < '0' || s[p] > '9') { ok = 0; break; }
            }
            if (!ok) continue;
            /* Eviter les doublons */
            int found = 0;
            for (int d = 0; d < nb_declared; d++) {
                if (strcmp(declared[d], s) == 0) { found = 1; break; }
            }
            if (!found && nb_declared < 1000) {
                strcpy(declared[nb_declared++], s);
                fprintf(fout, "    %-10s DW 0          ; TEMPORAIRE\n", s);
            }
        }
    }

    /* Chaine pour le saut de ligne (utilisee par WRITE_INT) */
    fprintf(fout, "    NEWLINE    DB 13,10,'$'\n");
    fprintf(fout, "DATA ENDS\n\n");
}

/* ==========================================================
   PROCEDURE WRITE_INT : affiche AX en decimal signe
   ========================================================== */
static void gen_write_proc(void) {
    fprintf(fout,
        "; ----- WRITE_INT : affiche AX en decimal -----\n"
        "WRITE_INT PROC\n"
        "    PUSH AX\n"
        "    PUSH BX\n"
        "    PUSH CX\n"
        "    PUSH DX\n"
        "    MOV CX, 0          ; CX = nombre de chiffres a afficher\n"
        "    MOV BX, 10         ; diviseur\n"
        "    ; Gestion du signe negatif\n"
        "    CMP AX, 0\n"
        "    JGE WRITE_POS\n"
        "    PUSH AX\n"
        "    MOV DL, '-'\n"
        "    MOV AH, 02h\n"
        "    INT 21h\n"
        "    POP AX\n"
        "    NEG AX\n"
        "WRITE_POS:\n"
        "    ; Extraire les chiffres en divisant par 10 (a l'envers)\n"
        "WRITE_LOOP1:\n"
        "    XOR DX, DX\n"
        "    DIV BX             ; AX / 10, reste dans DX\n"
        "    ADD DL, '0'\n"
        "    PUSH DX            ; empiler le chiffre\n"
        "    INC CX\n"
        "    CMP AX, 0\n"
        "    JNE WRITE_LOOP1\n"
        "    ; Afficher les chiffres dans le bon ordre (depiler)\n"
        "WRITE_LOOP2:\n"
        "    POP DX\n"
        "    MOV AH, 02h\n"
        "    INT 21h\n"
        "    LOOP WRITE_LOOP2\n"
        "    ; Saut de ligne\n"
        "    MOV DX, OFFSET NEWLINE\n"
        "    MOV AH, 09h\n"
        "    INT 21h\n"
        "    POP DX\n"
        "    POP CX\n"
        "    POP BX\n"
        "    POP AX\n"
        "    RET\n"
        "WRITE_INT ENDP\n\n");
}

/* ==========================================================
   PROCEDURE READ_INT : lit un entier au clavier dans AX
   ========================================================== */
static void gen_read_proc(void) {
    fprintf(fout,
        "; ----- READ_INT : lit un entier dans AX -----\n"
        "READ_INT PROC\n"
        "    PUSH BX\n"
        "    PUSH CX\n"
        "    PUSH DX\n"
        "    XOR BX, BX         ; BX = resultat accumule\n"
        "    XOR CX, CX         ; CX = flag negatif\n"
        "    MOV AH, 01h\n"
        "    INT 21h            ; lire le 1er caractere\n"
        "    CMP AL, '-'\n"
        "    JNE READ_DIGIT_CHECK\n"
        "    MOV CX, 1          ; signe negatif detecte\n"
        "    MOV AH, 01h\n"
        "    INT 21h            ; lire le prochain caractere\n"
        "READ_DIGIT_CHECK:\n"
        "    CMP AL, 13         ; Entree = fin de lecture\n"
        "    JE READ_END\n"
        "    CMP AL, '0'\n"
        "    JB READ_NEXT\n"
        "    CMP AL, '9'\n"
        "    JA READ_NEXT\n"
        "    ; BX = BX * 10 + (AL - '0')\n"
        "    PUSH AX\n"
        "    MOV AX, BX\n"
        "    MOV DX, 10\n"
        "    MUL DX\n"
        "    MOV BX, AX\n"
        "    POP AX\n"
        "    SUB AL, '0'\n"
        "    XOR AH, AH\n"
        "    ADD BX, AX\n"
        "READ_NEXT:\n"
        "    MOV AH, 01h\n"
        "    INT 21h\n"
        "    JMP READ_DIGIT_CHECK\n"
        "READ_END:\n"
        "    MOV AX, BX\n"
        "    CMP CX, 1\n"
        "    JNE READ_DONE\n"
        "    NEG AX\n"
        "READ_DONE:\n"
        "    POP DX\n"
        "    POP CX\n"
        "    POP BX\n"
        "    RET\n"
        "READ_INT ENDP\n\n");
}

/* ==========================================================
   TRADUCTION D'UN QUADRUPLET EN 8086
   ========================================================== */
static void gen_quad_asm(int i) {
    const char *op  = quads[i].op;
    const char *a1  = quads[i].arg1;
    const char *a2  = quads[i].arg2;
    const char *res = quads[i].result;

    /* Etiquette pour chaque quad (cible potentielle d'un saut) */
    fprintf(fout, "LBL_%d:\n", i);

    /* AFFECTATION : (=, X, _, Y) */
    if (strcmp(op, "=") == 0) {
        load_to_reg("AX", a1);
        store_from_ax(res);
    }
    /* OPERATIONS ARITHMETIQUES */
    else if (strcmp(op, "+") == 0) {
        load_to_reg("AX", a1);
        load_to_reg("BX", a2);
        fprintf(fout, "    ADD AX, BX\n");
        store_from_ax(res);
    }
    else if (strcmp(op, "-") == 0) {
        load_to_reg("AX", a1);
        load_to_reg("BX", a2);
        fprintf(fout, "    SUB AX, BX\n");
        store_from_ax(res);
    }
    else if (strcmp(op, "*") == 0) {
        load_to_reg("AX", a1);
        load_to_reg("BX", a2);
        fprintf(fout, "    IMUL BX\n");
        store_from_ax(res);
    }
    else if (strcmp(op, "/") == 0) {
        load_to_reg("AX", a1);
        fprintf(fout, "    CWD                ; extension de signe AX -> DX:AX\n");
        load_to_reg("BX", a2);
        fprintf(fout, "    IDIV BX\n");
        store_from_ax(res);
    }
    /* NEGATION UNAIRE : (NEG, X, _, T) */
    else if (strcmp(op, "NEG") == 0) {
        load_to_reg("AX", a1);
        fprintf(fout, "    NEG AX\n");
        store_from_ax(res);
    }
    /* COMPARAISONS : retournent 1 (vrai) ou 0 (faux) */
    else if (strcmp(op, ">")  == 0 || strcmp(op, "<")  == 0 ||
             strcmp(op, ">=") == 0 || strcmp(op, "<=") == 0 ||
             strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        const char *jmp_true =
            (strcmp(op, ">")  == 0) ? "JG"  :
            (strcmp(op, "<")  == 0) ? "JL"  :
            (strcmp(op, ">=") == 0) ? "JGE" :
            (strcmp(op, "<=") == 0) ? "JLE" :
            (strcmp(op, "==") == 0) ? "JE"  : "JNE";
        load_to_reg("AX", a1);
        load_to_reg("BX", a2);
        fprintf(fout, "    CMP AX, BX\n");
        fprintf(fout, "    %s CMP_TRUE_%d\n", jmp_true, i);
        fprintf(fout, "    MOV AX, 0\n");
        fprintf(fout, "    JMP CMP_END_%d\n", i);
        fprintf(fout, "CMP_TRUE_%d:\n", i);
        fprintf(fout, "    MOV AX, 1\n");
        fprintf(fout, "CMP_END_%d:\n", i);
        store_from_ax(res);
    }
    /* OPERATEURS LOGIQUES */
    else if (strcmp(op, "&&") == 0) {
        load_to_reg("AX", a1);
        load_to_reg("BX", a2);
        fprintf(fout, "    AND AX, BX\n");
        store_from_ax(res);
    }
    else if (strcmp(op, "||") == 0) {
        load_to_reg("AX", a1);
        load_to_reg("BX", a2);
        fprintf(fout, "    OR AX, BX\n");
        store_from_ax(res);
    }
    else if (strcmp(op, "!") == 0) {
        load_to_reg("AX", a1);
        fprintf(fout, "    CMP AX, 0\n");
        fprintf(fout, "    JE NOT_TRUE_%d\n", i);
        fprintf(fout, "    MOV AX, 0\n");
        fprintf(fout, "    JMP NOT_END_%d\n", i);
        fprintf(fout, "NOT_TRUE_%d:\n", i);
        fprintf(fout, "    MOV AX, 1\n");
        fprintf(fout, "NOT_END_%d:\n", i);
        store_from_ax(res);
    }
    /* SAUT CONDITIONNEL : (BZ, cond, _, label) */
    else if (strcmp(op, "BZ") == 0) {
        load_to_reg("AX", a1);
        fprintf(fout, "    CMP AX, 0\n");
        fprintf(fout, "    JE LBL_%s\n", res);
    }
    /* SAUT INCONDITIONNEL : (BR, _, _, label) */
    else if (strcmp(op, "BR") == 0) {
        fprintf(fout, "    JMP LBL_%s\n", res);
    }
    /* ECRITURE : (WRITE, X, _, _) */
    else if (strcmp(op, "WRITE") == 0) {
        load_to_reg("AX", a1);
        fprintf(fout, "    CALL WRITE_INT\n");
    }
    /* LECTURE : (READ, _, _, Y) */
    else if (strcmp(op, "READ") == 0) {
        fprintf(fout, "    CALL READ_INT\n");
        store_from_ax(res);
    }
    fprintf(fout, "\n");
}

/* ==========================================================
   GENERATION COMPLETE DU FICHIER .ASM
   ========================================================== */
void generate_assembly(const char *filename) {
    fout = fopen(filename, "w");
    if (!fout) {
        printf("Erreur : impossible de creer %s\n", filename);
        return;
    }

    /* En-tete */
    fprintf(fout, "; ===========================================\n");
    fprintf(fout, "; Code 8086 genere par le compilateur MiniAlgo\n");
    fprintf(fout, "; Compatible emu8086\n");
    fprintf(fout, "; ===========================================\n\n");

    /* Segment de pile */
    fprintf(fout, "PILE SEGMENT STACK\n");
    fprintf(fout, "    DW 100 DUP(?)\n");
    fprintf(fout, "PILE ENDS\n\n");

    /* Segment de donnees */
    gen_data_segment();

    /* Segment de code */
    fprintf(fout, "CODE SEGMENT\n");
    fprintf(fout, "    ASSUME CS:CODE, DS:DATA, SS:PILE\n\n");

    fprintf(fout, "MAIN PROC\n");
    fprintf(fout, "    MOV AX, DATA\n");
    fprintf(fout, "    MOV DS, AX\n\n");

    /* Traduire chaque quadruplet */
    for (int i = 0; i < quad_count; i++) {
        gen_quad_asm(i);
    }

    /* Etiquette de fin (pour les sauts au-dela du dernier quad) */
    fprintf(fout, "LBL_%d:\n", quad_count);
    fprintf(fout, "    MOV AH, 4Ch\n");
    fprintf(fout, "    INT 21h\n");
    fprintf(fout, "MAIN ENDP\n\n");

    /* Procedures auxiliaires */
    gen_write_proc();
    gen_read_proc();

    fprintf(fout, "CODE ENDS\n");
    fprintf(fout, "END MAIN\n");

    fclose(fout);
    printf("Code assembleur 8086 genere : %s\n", filename);
}
