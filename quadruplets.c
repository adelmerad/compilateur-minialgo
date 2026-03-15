#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quadruplets.h"
#include "symbol_table.h"

/* ==========================================================
   Implementation des quadruplets (code intermediaire)
   VERSION PRESENTATION : pas d'optimisation, pas de 8086
   ========================================================== */

/* ----- Variables globales ----- */
Quadruplet quads[MAX_QUADS];
int quad_count = 0;
int temp_count = 0;

/* Pool de noms de temporaires */
static char temp_pool[MAX_QUADS][16];

/* ----------------------------------------------------------
   init_quads : remet a zero les compteurs.
   ---------------------------------------------------------- */
void init_quads(void) {
    quad_count = 0;
    temp_count = 0;
}

/* ----------------------------------------------------------
   gen_quad : genere un nouveau quadruplet.
   Les NULL sont traites comme des chaines vides.
   Retourne l'index du quadruplet genere.
   ---------------------------------------------------------- */
int gen_quad(const char *op, const char *arg1, const char *arg2, const char *result) {
    if (quad_count >= MAX_QUADS) {
        printf("Erreur : nombre maximal de quadruplets atteint (%d)\n", MAX_QUADS);
        exit(1);
    }
    strncpy(quads[quad_count].op,     op     ? op     : "", MAX_STR - 1);
    strncpy(quads[quad_count].arg1,   arg1   ? arg1   : "", MAX_STR - 1);
    strncpy(quads[quad_count].arg2,   arg2   ? arg2   : "", MAX_STR - 1);
    strncpy(quads[quad_count].result, result ? result : "", MAX_STR - 1);
    quads[quad_count].op[MAX_STR - 1]     = '\0';
    quads[quad_count].arg1[MAX_STR - 1]   = '\0';
    quads[quad_count].arg2[MAX_STR - 1]   = '\0';
    quads[quad_count].result[MAX_STR - 1] = '\0';
    return quad_count++;
}

/* ----------------------------------------------------------
   new_temp : cree un nouveau nom de temporaire "T0", "T1", ...
   Le nom est stocke dans temp_pool (memoire persistante).
   ---------------------------------------------------------- */
char* new_temp(void) {
    if (temp_count >= MAX_QUADS) {
        printf("Erreur : nombre maximal de temporaires atteint\n");
        exit(1);
    }
    snprintf(temp_pool[temp_count], sizeof(temp_pool[temp_count]), "T%d", temp_count);
    return temp_pool[temp_count++];
}

/* ----------------------------------------------------------
   update_quad_result : met a jour le champ 'result'
   (utilise pour le BACKPATCHING des sauts IF/WHILE/FOR).
   ---------------------------------------------------------- */
void update_quad_result(int index, const char *result) {
    if (index >= 0 && index < quad_count && result) {
        strncpy(quads[index].result, result, MAX_STR - 1);
        quads[index].result[MAX_STR - 1] = '\0';
    }
}

/* ----------------------------------------------------------
   display_quads : affiche les quadruplets sous forme de tableau.
   ---------------------------------------------------------- */
void display_quads(void) {
    if (quad_count == 0) {
        printf("(aucun quadruplet genere)\n");
        return;
    }
    printf("+---------+-----------+-----------+-----------+-----------+\n");
    printf("|   N     |    OPR    |   OPD1    |   OPD2    |    RES    |\n");
    printf("+---------+-----------+-----------+-----------+-----------+\n");
    for (int i = 0; i < quad_count; i++) {
        printf("|  %-6d | %-9s | %-9s | %-9s | %-9s |\n",
               i, quads[i].op, quads[i].arg1, quads[i].arg2, quads[i].result);
    }
    printf("+---------+-----------+-----------+-----------+-----------+\n");
    printf("Total : %d quadruplet(s)\n", quad_count);
}

/* ----------------------------------------------------------
   is_number : retourne 1 si 's' est un nombre (entier/reel signe).
   Utilise pour detecter la division par zero.
   ---------------------------------------------------------- */
int is_number(const char *s) {
    if (!s || !*s) return 0;
    if (*s == '-' || *s == '+') s++;
    if (!*s) return 0;
    int has_dot = 0;
    while (*s) {
        if (*s == '.') {
            if (has_dot) return 0;
            has_dot = 1;
        } else if (*s < '0' || *s > '9') {
            return 0;
        }
        s++;
    }
    return 1;
}
