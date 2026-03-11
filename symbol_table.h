#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

/* ==========================================================
   Table des Symboles pour MiniAlgo
   Implementee comme une TABLE DE HACHAGE avec chainage
   (exigence du sujet, partie 3).
   ========================================================== */

#define TABLE_SIZE 100   /* taille de la table de hachage */
#define MAX_NAME   10    /* 8 caracteres + '\0' + marge de securite */

/* ----- Types de donnees supportes par MiniAlgo ----- */
typedef enum {
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_UNKNOWN   /* type non encore determine (insertion par le lexer) */
} DataType;

/* ----- Nature du symbole ----- */
typedef enum {
    NAT_VARIABLE,
    NAT_CONSTANT,
    NAT_ARRAY
} Nature;

/* ----- Une entree de la table des symboles ----- */
typedef struct SymbolEntry {
    char   name[MAX_NAME];     /* nom du symbole (max 8 car.) */
    DataType type;             /* INTEGER / FLOAT / UNKNOWN */
    Nature   nature;           /* VARIABLE / CONSTANT / ARRAY */
    int      is_initialized;   /* 1 si une valeur a ete affectee */
    int      is_declared;      /* 0 = vu par le lexer, 1 = declare par le programmeur */
    double   const_value;      /* valeur (pour les constantes) */
    int      array_size;       /* taille (pour les tableaux) */
    int      line_declared;    /* numero de ligne de declaration */
    struct SymbolEntry *next;  /* chainage en cas de collision */
} SymbolEntry;

/* ----- Variable globale : la table des symboles ----- */
extern SymbolEntry *table[TABLE_SIZE];

/* ----- Fonctions de gestion ----- */
void init_symbol_table(void);
void free_symbol_table(void);

unsigned int hash_func(const char *name);

/* Recherche : retourne un pointeur sur l'entree, ou NULL */
SymbolEntry* search_symbol(const char *name);

/* Insertion (utilisee par le lexer) :
   - retourne 1 si inseree, 0 si deja presente */
int insert_symbol(const char *name, DataType type, Nature nature, int line);

/* Declaration par le programmeur (utilisee par le parser) :
   - retourne 1 si OK
   - retourne 0 si DOUBLE DECLARATION (erreur semantique) */
int declare_symbol(const char *name, DataType type, Nature nature, int line);

/* Fixe la valeur d'une constante + marque comme initialisee et NAT_CONSTANT */
void set_const_value(const char *name, double value);

/* Fixe la taille d'un tableau */
void set_array_size(const char *name, int size);

/* Marque un symbole comme initialise (apres affectation) */
void set_initialized(const char *name);

/* Verifie si un symbole a ete declare par le programmeur */
int is_declared(const char *name);

/* Retourne le type d'un symbole (TYPE_UNKNOWN si inexistant) */
DataType get_type(const char *name);

/* Affiche la table des symboles sous forme de tableau formate */
void display_symbol_table(void);

#endif
