#ifndef QUADRUPLETS_H
#define QUADRUPLETS_H

/* ==========================================================
   Structure et API pour le code intermediaire sous forme
   de QUADRUPLETS : (OPR, OPD1, OPD2, RES)

   VERSION PRESENTATION : jusqu'aux quadruplets uniquement
   (pas d'optimisation, pas de generation 8086 dans ce cours)
   ========================================================== */

#define MAX_QUADS 1000
#define MAX_STR   50

/* ----- Structure d'un quadruplet ----- */
typedef struct {
    char op[MAX_STR];      /* operateur : +, -, *, /, =, BZ, BR, WRITE, READ, ... */
    char arg1[MAX_STR];    /* premier operande */
    char arg2[MAX_STR];    /* second operande (ou vide) */
    char result[MAX_STR];  /* destination (ou cible de saut) */
} Quadruplet;

/* ----- Variables globales ----- */
extern Quadruplet quads[MAX_QUADS];
extern int quad_count;
extern int temp_count;

/* ----- Fonctions ----- */

/* Initialise les compteurs */
void init_quads(void);

/* Genere un quadruplet et retourne son index */
int  gen_quad(const char *op, const char *arg1, const char *arg2, const char *result);

/* Cree un nom de temporaire "T0", "T1", ... */
char* new_temp(void);

/* Met a jour le champ 'result' d'un quadruplet (backpatching) */
void update_quad_result(int index, const char *result);

/* Affiche les quadruplets sous forme de tableau */
void display_quads(void);

/* Teste si une chaine represente un nombre (entier ou reel) */
int  is_number(const char *s);

#endif
