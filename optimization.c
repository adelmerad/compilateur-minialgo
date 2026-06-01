#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quadruplets.h"
#include "optimization.h"

/* ==========================================================
   IMPLEMENTATION DES OPTIMISATIONS
   ----------
   Strategie : on applique les 5 optimisations en boucle
   jusqu'a ce qu'aucune transformation ne soit possible.
   Les quads supprimes sont marques NOP, puis compactes a la fin.
   ========================================================== */

/* ---------- HELPERS ---------- */

/* Marque un quadruplet comme supprime (op = "NOP") */
static void mark_nop(int i) {
    strcpy(quads[i].op, "NOP");
    quads[i].arg1[0] = '\0';
    quads[i].arg2[0] = '\0';
    quads[i].result[0] = '\0';
}

/* Verifie si un quad a ete supprime */
static int is_nop(int i) {
    return strcmp(quads[i].op, "NOP") == 0;
}

/* Verifie si une chaine est un temporaire T0, T1, ... */
static int is_temp(const char *s) {
    if (!s || s[0] != 'T' || s[1] == '\0') return 0;
    for (int i = 1; s[i]; i++)
        if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

/* ==========================================================
   OPTIMISATION 1 : SIMPLIFICATION ALGEBRIQUE
   ----------
   x*0=0, x*1=x, x*2=x+x, x+0=x, x-0=x, x-x=0, x/1=x
   ========================================================== */
static int optimize_algebraic(void) {
    int nb_opt = 0;
    for (int i = 0; i < quad_count; i++) {
        if (is_nop(i)) continue;

        const char *op = quads[i].op;
        const char *a1 = quads[i].arg1;
        const char *a2 = quads[i].arg2;

        /* MULTIPLICATION */
        if (strcmp(op, "*") == 0) {
            /* x*0 = 0  OU  0*x = 0 */
            if (strcmp(a2, "0") == 0 || strcmp(a1, "0") == 0) {
                strcpy(quads[i].op, "=");
                strcpy(quads[i].arg1, "0");
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
            /* x*1 = x */
            if (strcmp(a2, "1") == 0) {
                strcpy(quads[i].op, "=");
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
            /* 1*x = x */
            if (strcmp(a1, "1") == 0) {
                strcpy(quads[i].op, "=");
                strcpy(quads[i].arg1, a2);
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
            /* x*2 = x+x */
            if (strcmp(a2, "2") == 0) {
                strcpy(quads[i].op, "+");
                strcpy(quads[i].arg2, a1);
                nb_opt++;
                continue;
            }
        }

        /* ADDITION : x+0 = x, 0+x = x */
        if (strcmp(op, "+") == 0) {
            if (strcmp(a2, "0") == 0) {
                strcpy(quads[i].op, "=");
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
            if (strcmp(a1, "0") == 0) {
                strcpy(quads[i].op, "=");
                strcpy(quads[i].arg1, a2);
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
        }

        /* SOUSTRACTION : x-0 = x, x-x = 0 */
        if (strcmp(op, "-") == 0) {
            if (strcmp(a2, "0") == 0) {
                strcpy(quads[i].op, "=");
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
            if (strcmp(a1, a2) == 0) {
                strcpy(quads[i].op, "=");
                strcpy(quads[i].arg1, "0");
                quads[i].arg2[0] = '\0';
                nb_opt++;
                continue;
            }
        }

        /* DIVISION : x/1 = x */
        if (strcmp(op, "/") == 0 && strcmp(a2, "1") == 0) {
            strcpy(quads[i].op, "=");
            quads[i].arg2[0] = '\0';
            nb_opt++;
        }
    }
    return nb_opt;
}

/* ==========================================================
   OPTIMISATION 2 : REPLI DES CONSTANTES (CONSTANT FOLDING)
   ----------
   Si arg1 et arg2 sont tous deux des nombres, on calcule
   le resultat directement : (+, 5, 3, T0) -> (=, 8, _, T0)
   ========================================================== */
static int optimize_constant_folding(void) {
    int nb_opt = 0;
    for (int i = 0; i < quad_count; i++) {
        if (is_nop(i)) continue;
        if (!is_number(quads[i].arg1) || !is_number(quads[i].arg2)) continue;

        const char *op = quads[i].op;
        double a = atof(quads[i].arg1);
        double b = atof(quads[i].arg2);
        double r;
        int valid = 1;

        if      (strcmp(op, "+") == 0) r = a + b;
        else if (strcmp(op, "-") == 0) r = a - b;
        else if (strcmp(op, "*") == 0) r = a * b;
        else if (strcmp(op, "/") == 0) {
            if (b == 0) valid = 0;
            else r = a / b;
        }
        else valid = 0;

        if (valid) {
            char buf[32];
            /* Si entier, afficher sans decimales */
            if (r == (long)r) snprintf(buf, sizeof(buf), "%ld", (long)r);
            else              snprintf(buf, sizeof(buf), "%g", r);
            strcpy(quads[i].op, "=");
            strcpy(quads[i].arg1, buf);
            quads[i].arg2[0] = '\0';
            nb_opt++;
        }
    }
    return nb_opt;
}
