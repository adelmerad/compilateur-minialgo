#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

/* ==========================================================
   Implementation de la Table des Symboles
   ========================================================== */

SymbolEntry *table[TABLE_SIZE];

/* ----------------------------------------------------------
   init_symbol_table
   Initialise tous les seaux (buckets) a NULL.
   A appeler UNE SEULE FOIS au debut du programme.
   ---------------------------------------------------------- */
void init_symbol_table(void) {
    for (int i = 0; i < TABLE_SIZE; i++)
        table[i] = NULL;
}

/* ----------------------------------------------------------
   free_symbol_table
   Libere toute la memoire allouee (anti fuite memoire).
   A appeler a la fin du programme.
   ---------------------------------------------------------- */
void free_symbol_table(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        SymbolEntry *e = table[i];
        while (e) {
            SymbolEntry *tmp = e;
            e = e->next;
            free(tmp);
        }
        table[i] = NULL;
    }
}

/* ----------------------------------------------------------
   hash_func
   Fonction de hachage (methode de Horner, tres classique) :
   h = h * 31 + caractere
   Retourne un indice dans [0, TABLE_SIZE[
   ---------------------------------------------------------- */
unsigned int hash_func(const char *name) {
    unsigned int h = 0;
    while (*name) {
        h = h * 31 + (unsigned char)(*name);
        name++;
    }
    return h % TABLE_SIZE;
}

/* ----------------------------------------------------------
   search_symbol
   Parcourt la liste chainee du seau correspondant.
   Retourne un pointeur sur l'entree ou NULL.
   Complexite : O(1) en moyenne, O(n) au pire.
   ---------------------------------------------------------- */
SymbolEntry* search_symbol(const char *name) {
    if (!name) return NULL;
    unsigned int idx = hash_func(name);
    SymbolEntry *e = table[idx];
    while (e) {
        if (strcmp(e->name, name) == 0)
            return e;
        e = e->next;
    }
    return NULL;
}

/* ----------------------------------------------------------
   insert_symbol
   Appelee par le LEXER lors de la premiere rencontre.
   - Retourne 1 si inseree
   - Retourne 0 si deja presente (le lexer l'ignore)
   ---------------------------------------------------------- */
int insert_symbol(const char *name, DataType type, Nature nature, int line) {
    if (!name) return 0;
    if (search_symbol(name) != NULL)
        return 0;   /* deja present : ne rien faire */

    unsigned int idx = hash_func(name);
    SymbolEntry *e = (SymbolEntry *)malloc(sizeof(SymbolEntry));
    if (!e) return 0;

    strncpy(e->name, name, MAX_NAME - 1);
    e->name[MAX_NAME - 1] = '\0';
    e->type           = type;
    e->nature         = nature;
    e->is_initialized = 0;
    e->is_declared    = 0;      /* insere par le lexer, pas encore declare */
    e->const_value    = 0.0;
    e->array_size     = 0;
    e->line_declared  = line;
    e->next           = table[idx];  /* chainage en tete */
    table[idx]        = e;
    return 1;
}

/* ----------------------------------------------------------
   declare_symbol
   Appelee par le PARSER lors d'une declaration.
   - Si le symbole existe (insere par le lexer) et n'est PAS encore
     declare : on le complete et on marque is_declared = 1.
   - Si le symbole est deja declare : DOUBLE DECLARATION => retourne 0.
   - Si le symbole n'existe pas du tout (cas rare) : on le cree.
   Retourne 1 si OK, 0 si erreur (double declaration).
   ---------------------------------------------------------- */
int declare_symbol(const char *name, DataType type, Nature nature, int line) {
    SymbolEntry *e = search_symbol(name);

    if (e) {
        if (e->is_declared) {
            /* DOUBLE DECLARATION : erreur semantique */
            return 0;
        }
        /* Entree deja inseree par le lexer : on la complete */
        e->type          = type;
        e->nature        = nature;
        e->line_declared = line;
        e->is_declared   = 1;
        return 1;
    }

    /* Cas rare : identifiant non vu par le lexer (securite) */
    insert_symbol(name, type, nature, line);
    e = search_symbol(name);
    if (e) {
        e->is_declared = 1;
        return 1;
    }
    return 0;
}

/* ----------------------------------------------------------
   set_const_value
   Fixe la valeur d'une constante ET force nature = NAT_CONSTANT.
   ---------------------------------------------------------- */
void set_const_value(const char *name, double value) {
    SymbolEntry *e = search_symbol(name);
    if (e) {
        e->const_value    = value;
        e->is_initialized = 1;
        e->nature         = NAT_CONSTANT;
    }
}

/* ----------------------------------------------------------
   set_array_size
   Fixe la taille d'un tableau ET force nature = NAT_ARRAY.
   ---------------------------------------------------------- */
void set_array_size(const char *name, int size) {
    SymbolEntry *e = search_symbol(name);
    if (e) {
        e->array_size = size;
        e->nature     = NAT_ARRAY;
    }
}

/* ----------------------------------------------------------
   set_initialized
   Marque un symbole comme ayant recu une valeur (apres une
   affectation). Utile pour detecter l'usage d'une variable
   non initialisee.
   ---------------------------------------------------------- */
void set_initialized(const char *name) {
    SymbolEntry *e = search_symbol(name);
    if (e) e->is_initialized = 1;
}

/* ----------------------------------------------------------
   is_declared
   Retourne 1 si le symbole a ete declare par le programmeur,
   0 sinon (soit inexistant, soit juste vu par le lexer).
   Utilisee par le parser pour detecter "variable non declaree".
   ---------------------------------------------------------- */
int is_declared(const char *name) {
    SymbolEntry *e = search_symbol(name);
    return (e && e->is_declared) ? 1 : 0;
}

/* ----------------------------------------------------------
   get_type
   Retourne le type d'un symbole (pour verif de compatibilite).
   Retourne TYPE_UNKNOWN si symbole inexistant.
   ---------------------------------------------------------- */
DataType get_type(const char *name) {
    SymbolEntry *e = search_symbol(name);
    return e ? e->type : TYPE_UNKNOWN;
}

/* ----------------------------------------------------------
   display_symbol_table
   Affiche la TS sous forme de tableau formate.
   N'affiche que les symboles DECLARES par le programmeur
   (is_declared == 1), pas les artefacts du lexer.
   ---------------------------------------------------------- */
void display_symbol_table(void) {
    printf("\n");
    printf("+-----------+---------+-----------+-----------+--------+-------+\n");
    printf("| Nom       | Type    | Nature    | Valeur    | Taille | Ligne |\n");
    printf("+-----------+---------+-----------+-----------+--------+-------+\n");

    int nb = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        SymbolEntry *e = table[i];
        while (e) {
            if (!e->is_declared) { e = e->next; continue; }

            const char *t = (e->type == TYPE_INTEGER) ? "INTEGER" :
                            (e->type == TYPE_FLOAT)   ? "FLOAT"   : "UNKNOWN";
            const char *n = (e->nature == NAT_VARIABLE) ? "Variable"  :
                            (e->nature == NAT_CONSTANT) ? "Constante" :
                            (e->nature == NAT_ARRAY)    ? "Tableau"   : "?";

            printf("| %-9s | %-7s | %-9s |", e->name, t, n);

            if (e->nature == NAT_CONSTANT)
                printf(" %-9g |", e->const_value);
            else
                printf(" %-9s |", "-");

            if (e->nature == NAT_ARRAY)
                printf(" %-6d |", e->array_size);
            else
                printf(" %-6s |", "-");

            printf(" %-5d |\n", e->line_declared);
            nb++;
            e = e->next;
        }
    }
    if (nb == 0) {
        printf("|                       (table vide)                          |\n");
    }
    printf("+-----------+---------+-----------+-----------+--------+-------+\n");
    printf("Total : %d symbole(s) declare(s)\n\n", nb);
}
