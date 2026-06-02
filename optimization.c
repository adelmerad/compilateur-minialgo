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

/* ==========================================================
   OPTIMISATION 3 : PROPAGATION DE COPIE
   ----------
   Si (=, X, _, T) puis (op, T, ...), on remplace T par X
   dans les utilisations suivantes (tant que T et X ne sont
   pas modifiees, et tant qu'on ne croise pas un saut).
   ========================================================== */
static int optimize_copy_propagation(void) {
    int nb_opt = 0;
    for (int i = 0; i < quad_count; i++) {
        if (is_nop(i)) continue;
        if (strcmp(quads[i].op, "=") != 0) continue;
        if (quads[i].arg2[0] != '\0') continue;  /* pas une affectation simple */

        const char *src  = quads[i].arg1;
        const char *dest = quads[i].result;

        /* On propage seulement les temporaires */
        if (!is_temp(dest)) continue;

        for (int j = i + 1; j < quad_count; j++) {
            if (is_nop(j)) continue;
            /* Arret si dest ou src est reaffectee */
            if (strcmp(quads[j].result, dest) == 0) break;
            if (strcmp(quads[j].result, src) == 0) break;
            /* Arret sur un saut */
            if (quads[j].op[0] == 'B') break;

            if (strcmp(quads[j].arg1, dest) == 0) {
                strcpy(quads[j].arg1, src);
                nb_opt++;
            }
            if (strcmp(quads[j].arg2, dest) == 0) {
                strcpy(quads[j].arg2, src);
                nb_opt++;
            }
        }
    }
    return nb_opt;
}

/* ==========================================================
   OPTIMISATION 4 : ELIMINATION D'EXPRESSIONS REDONDANTES
   ----------
   Si on a (op, a, b, T1) et plus loin (op, a, b, T2),
   on remplace le second par (=, T1, _, T2).
   ========================================================== */
static int optimize_common_subexpr(void) {
    int nb_opt = 0;
    for (int i = 0; i < quad_count; i++) {
        if (is_nop(i)) continue;
        if (quads[i].arg2[0] == '\0') continue;
        if (strchr("+-*/", quads[i].op[0]) == NULL) continue;

        const char *op = quads[i].op;
        const char *a1 = quads[i].arg1;
        const char *a2 = quads[i].arg2;
        const char *r1 = quads[i].result;

        for (int j = i + 1; j < quad_count; j++) {
            if (is_nop(j)) continue;
            /* Arret si a1 ou a2 sont reaffectes */
            if (strcmp(quads[j].result, a1) == 0) break;
            if (strcmp(quads[j].result, a2) == 0) break;
            /* Arret sur un saut */
            if (quads[j].op[0] == 'B') break;

            /* Expression identique trouvee */
            if (strcmp(quads[j].op, op) == 0 &&
                strcmp(quads[j].arg1, a1) == 0 &&
                strcmp(quads[j].arg2, a2) == 0) {
                strcpy(quads[j].op, "=");
                strcpy(quads[j].arg1, r1);
                quads[j].arg2[0] = '\0';
                nb_opt++;
            }
        }
    }
    return nb_opt;
}

/* ==========================================================
   OPTIMISATION 5 : ELIMINATION DU CODE MORT
   ----------
   Si un temporaire T est affecte mais jamais utilise,
   on supprime le quadruplet qui l'a defini.
   ========================================================== */
static int optimize_dead_code(void) {
    int nb_opt = 0;
    for (int i = 0; i < quad_count; i++) {
        if (is_nop(i)) continue;
        const char *res = quads[i].result;

        /* Uniquement pour les temporaires */
        if (!is_temp(res)) continue;
        /* Ne pas supprimer les sauts ni WRITE/READ */
        if (quads[i].op[0] == 'B') continue;
        if (strcmp(quads[i].op, "WRITE") == 0) continue;
        if (strcmp(quads[i].op, "READ") == 0) continue;

        /* Verifier si res est utilise plus loin */
        int used = 0;
        for (int j = i + 1; j < quad_count; j++) {
            if (is_nop(j)) continue;
            if (strcmp(quads[j].arg1, res) == 0 ||
                strcmp(quads[j].arg2, res) == 0) {
                used = 1;
                break;
            }
            /* Si reaffecte, la valeur actuelle est definitivement morte */
            if (strcmp(quads[j].result, res) == 0) break;
        }

        if (!used) {
            mark_nop(i);
            nb_opt++;
        }
    }
    return nb_opt;
}

/* ==========================================================
   COMPACTAGE : supprime les NOP et renumerote les sauts
   ----------
   Les quads NOP sont retires du tableau. Les cibles des sauts
   (BZ, BR) sont mises a jour pour pointer vers les nouveaux
   indices.
   ========================================================== */
static void compact_quads(void) {
    int map[MAX_QUADS];
    int new_count = 0;

    /* 1. Construire la table de remapping : ancien_idx -> nouveau_idx */
    for (int i = 0; i < quad_count; i++) {
        if (is_nop(i)) {
            map[i] = -1;
        } else {
            map[i] = new_count;
            if (new_count != i) quads[new_count] = quads[i];
            new_count++;
        }
    }

    /* 2. Mettre a jour les cibles des sauts */
    for (int i = 0; i < new_count; i++) {
        if (quads[i].op[0] == 'B' && is_number(quads[i].result)) {
            int old_target = atoi(quads[i].result);
            int new_target;
            if (old_target >= quad_count) {
                /* Saut vers la fin du programme */
                new_target = new_count;
            } else {
                new_target = map[old_target];
                /* Si la cible a ete supprimee, on passe au quad suivant valide */
                while (new_target == -1 && old_target < quad_count) {
                    old_target++;
                    new_target = (old_target < quad_count) ? map[old_target] : new_count;
                }
            }
            snprintf(quads[i].result, MAX_STR, "%d", new_target);
        }
    }

    quad_count = new_count;
}

/* ==========================================================
   FONCTION PRINCIPALE : applique toutes les optimisations
   ========================================================== */
void optimize_quads(void) {
    int total = 0;
    int changes;
    int pass = 0;

    /* Boucle jusqu'a stabilisation (max 10 passes pour eviter boucle infinie) */
    do {
        changes = 0;
        changes += optimize_constant_folding();
        changes += optimize_algebraic();
        changes += optimize_copy_propagation();
        changes += optimize_common_subexpr();
        changes += optimize_dead_code();
        total += changes;
        pass++;
    } while (changes > 0 && pass < 10);

    /* Compacter : supprimer les NOP et renumeroter les sauts */
    int before = quad_count;
    compact_quads();

    printf("Optimisation : %d transformation(s) en %d passe(s)\n", total, pass);
    printf("Quadruplets : %d avant -> %d apres (- %d)\n",
           before, quad_count, before - quad_count);
}

/* Affichage des quadruplets optimises */
void display_optimized_quads(void) {
    if (quad_count == 0) {
        printf("(aucun quadruplet)\n");
        return;
    }
    for (int i = 0; i < quad_count; i++) {
        const char *op   = (quads[i].op[0]   == '\0') ? "vide" : quads[i].op;
        const char *arg1 = (quads[i].arg1[0] == '\0') ? "vide" : quads[i].arg1;
        const char *arg2 = (quads[i].arg2[0] == '\0') ? "vide" : quads[i].arg2;
        const char *res  = (quads[i].result[0] == '\0') ? "vide" : quads[i].result;
        printf("%d - ( %s , %s , %s , %s )\n", i, op, arg1, arg2, res);
        printf("--------------------------------------------\n");
    }
    printf("\nTotal apres optimisation : %d quadruplet(s)\n", quad_count);
}
