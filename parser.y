%{
/* ==========================================================
   Analyseur Syntaxique + Semantique pour MiniAlgo
   VERSION PRESENTATION : jusqu'aux quadruplets uniquement.
   Format des erreurs (selon sujet) :
     "Erreur <Type> : ligne X , colonne Y , element Z"
   ========================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "quadruplets.h"

extern int yylex();
extern int yylineno;
extern int column;
extern char *yytext;
extern int nb_err_lex;
extern FILE *yyin;

void yyerror(const char *s);

int nb_err_syn = 0;
int nb_err_sem = 0;
DataType current_type;   /* type courant pour declarations */

/* Helper : detecte un identificateur invalide (marque par le lexer) */
int is_invalid_id(const char *name) {
    return (name && name[0] == '!' && name[1] == '!');
}

/* Pile pour le backpatching */
int label_stack[200];
int label_top = 0;
void push_lbl(int v) { label_stack[label_top++] = v; }
int  pop_lbl(void)   { return label_stack[--label_top]; }
%}

%union {
    int    int_val;
    double float_val;
    char   str[50];
    struct {
        char name[50];
        int  type;
    } expr;
}

%token PROGRAM DECL ENDDECL BEGIN_T END
%token INTEGER FLOAT CONST
%token IF ELSE FOR WHILE WRITE READ
%token AND OR NOT
%token EQ NEQ GEQ LEQ

%token <str>       IDF
%token <int_val>   INTEGER_CONST
%token <float_val> FLOAT_CONST

%type <expr> expression

%left OR
%left AND
%left EQ NEQ
%left '<' '>' GEQ LEQ
%left '+' '-'
%left '*' '/'
%right NOT UMINUS UPLUS

%start program

%%

/* ==========================================================
   REGLE RACINE
   ========================================================== */
program
    : PROGRAM IDF DECL declarations ENDDECL
      BEGIN_T instructions END
      {
          printf("\n========== Compilation terminee ==========\n");
          printf("Erreurs lexicales   : %d\n", nb_err_lex);
          printf("Erreurs syntaxiques : %d\n", nb_err_syn);
          printf("Erreurs semantiques : %d\n", nb_err_sem);

          /* AFFICHAGE TOUJOURS : la TS et les quadruplets sont affiches
             meme en cas d'erreur, pour montrer ce qui a ete analyse. */
          printf("\n===== Table des symboles =====\n");
          display_symbol_table();

          printf("\n===== Code intermediaire (quadruplets) =====\n");
          display_quads();

          if (nb_err_lex == 0 && nb_err_syn == 0 && nb_err_sem == 0) {
              printf("\n>>> Programme VALIDE <<<\n");
          } else {
              printf("\n>>> Compilation echouee : %d erreur(s) au total <<<\n",
                     nb_err_lex + nb_err_syn + nb_err_sem);
          }
      }
    ;

/* ==========================================================
   DECLARATIONS
   ========================================================== */
declarations
    : /* vide */
    | declarations declaration
    ;

declaration
    : type ':' id_list ';'

    | type ':' IDF '[' INTEGER_CONST ']' ';'
      {
          /* Tableau : INTEGER : T[10] ; */
          if (!is_invalid_id($3)) {
              int decl_ok = 1;
              if ($5 <= 0) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (taille de tableau invalide : %d)\n",
                         yylineno, column, $3, $5);
                  nb_err_sem++;
                  decl_ok = 0;
              }
              if (decl_ok) {
                  if (!declare_symbol($3, current_type, NAT_ARRAY, yylineno)) {
                      printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (double declaration)\n",
                             yylineno, column, $3);
                      nb_err_sem++;
                  } else {
                      set_array_size($3, $5);
                  }
              }
          }
      }

    | CONST IDF '=' INTEGER_CONST ';'
      {
          if (!is_invalid_id($2)) {
              if (!declare_symbol($2, TYPE_INTEGER, NAT_CONSTANT, yylineno)) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (double declaration)\n",
                         yylineno, column, $2);
                  nb_err_sem++;
              } else {
                  set_const_value($2, (double)$4);
              }
          }
      }

    | CONST IDF '=' FLOAT_CONST ';'
      {
          if (!is_invalid_id($2)) {
              if (!declare_symbol($2, TYPE_FLOAT, NAT_CONSTANT, yylineno)) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (double declaration)\n",
                         yylineno, column, $2);
                  nb_err_sem++;
              } else {
                  set_const_value($2, $4);
              }
          }
      }

    /* RECUPERATION D'ERREUR */
    | error ';'   { yyerrok; }
    ;

type
    : INTEGER  { current_type = TYPE_INTEGER; }
    | FLOAT    { current_type = TYPE_FLOAT;   }
    ;

id_list
    : IDF
      {
          if (!is_invalid_id($1)) {
              if (!declare_symbol($1, current_type, NAT_VARIABLE, yylineno)) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (double declaration)\n",
                         yylineno, column, $1);
                  nb_err_sem++;
              }
          }
      }
    | id_list ',' IDF
      {
          if (!is_invalid_id($3)) {
              if (!declare_symbol($3, current_type, NAT_VARIABLE, yylineno)) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (double declaration)\n",
                         yylineno, column, $3);
                  nb_err_sem++;
              }
          }
      }
    ;

/* ==========================================================
   INSTRUCTIONS
   ========================================================== */
instructions
    : /* vide */
    | instructions instruction
    ;

instruction
    : assignment
    | if_stmt
    | for_stmt
    | while_stmt
    | write_stmt
    | read_stmt
    | error ';'   { yyerrok; }
    ;

/* ---------- AFFECTATION ---------- */
assignment
    : IDF '=' expression ';'
      {
          SymbolEntry *s = search_symbol($1);
          if (s == NULL || !s->is_declared) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (variable non declaree)\n",
                     yylineno, column, $1);
              nb_err_sem++;
          } else if (s->nature == NAT_CONSTANT) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (modification d'une constante)\n",
                     yylineno, column, $1);
              nb_err_sem++;
          } else {
              /* Verification compatibilite de type */
              if (s->type != $3.type && $3.type != TYPE_UNKNOWN && s->type != TYPE_UNKNOWN) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (incompatibilite de type)\n",
                         yylineno, column, $1);
                  nb_err_sem++;
              }
              gen_quad("=", $3.name, "", $1);
              set_initialized($1);
          }
      }
    | IDF '[' expression ']' '=' expression ';'
      {
          SymbolEntry *s = search_symbol($1);
          if (s == NULL || !s->is_declared) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (variable non declaree)\n",
                     yylineno, column, $1);
              nb_err_sem++;
          } else if (s->nature != NAT_ARRAY) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (n'est pas un tableau)\n",
                     yylineno, column, $1);
              nb_err_sem++;
          } else {
              if ($3.type != TYPE_INTEGER && $3.type != TYPE_UNKNOWN) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (indice de tableau non entier)\n",
                         yylineno, column, $1);
                  nb_err_sem++;
              }
              if (is_number($3.name)) {
                  int idx = atoi($3.name);
                  if (idx < 0 || idx >= s->array_size) {
                      printf("Erreur Semantique : ligne %d , colonne %d , element '%s[%d]' (indice hors bornes, taille=%d)\n",
                             yylineno, column, $1, idx, s->array_size);
                      nb_err_sem++;
                  }
              }
              if (s->type != $6.type && $6.type != TYPE_UNKNOWN && s->type != TYPE_UNKNOWN) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (incompatibilite de type)\n",
                         yylineno, column, $1);
                  nb_err_sem++;
              }
              char dest[50];
              snprintf(dest, sizeof(dest), "%s[%s]", $1, $3.name);
              gen_quad("=", $6.name, "", dest);
          }
      }
    ;

/* ---------- IF / ELSE ---------- */
if_stmt
    : if_header instructions '}' else_clause
    ;

if_header
    : IF '(' expression ')' '{'
      { push_lbl(gen_quad("BZ", $3.name, "", "")); }
    ;

else_clause
    : /* pas de ELSE */
      {
          int bz = pop_lbl();
          char lbl[20];
          sprintf(lbl, "%d", quad_count);
          update_quad_result(bz, lbl);
      }
    | ELSE '{'
      {
          int bz = pop_lbl();
          push_lbl(gen_quad("BR", "", "", ""));
          char lbl[20];
          sprintf(lbl, "%d", quad_count);
          update_quad_result(bz, lbl);
      }
      instructions '}'
      {
          int br = pop_lbl();
          char lbl[20];
          sprintf(lbl, "%d", quad_count);
          update_quad_result(br, lbl);
      }
    ;

/* ---------- FOR ---------- */
for_stmt
    : FOR '(' IDF ':' expression ':' expression ':' expression ')'
      {
          SymbolEntry *s = search_symbol($3);
          if (s == NULL || !s->is_declared) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (variable non declaree)\n",
                     yylineno, column, $3);
              nb_err_sem++;
          } else if (s->nature == NAT_CONSTANT) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (modification d'une constante)\n",
                     yylineno, column, $3);
              nb_err_sem++;
          }
          gen_quad("=", $5.name, "", $3);
          set_initialized($3);
          push_lbl(quad_count);
          char *t = new_temp();
          gen_quad("<=", $3, $9.name, t);
          push_lbl(gen_quad("BZ", t, "", ""));
      }
      '{' instructions '}'
      {
          char *t = new_temp();
          gen_quad("+", $3, $7.name, t);
          gen_quad("=", t, "", $3);
          int bz_idx     = pop_lbl();
          int loop_start = pop_lbl();
          char lbl[20];
          sprintf(lbl, "%d", loop_start);
          gen_quad("BR", "", "", lbl);
          sprintf(lbl, "%d", quad_count);
          update_quad_result(bz_idx, lbl);
      }
    ;

/* ---------- WHILE ---------- */
while_stmt
    : while_mark '(' expression ')' '{'
      { $<int_val>$ = gen_quad("BZ", $3.name, "", ""); }
      instructions '}'
      {
          char lbl[20];
          sprintf(lbl, "%d", $<int_val>1);
          gen_quad("BR", "", "", lbl);
          sprintf(lbl, "%d", quad_count);
          update_quad_result($<int_val>6, lbl);
      }
    ;

while_mark
    : WHILE  { $<int_val>$ = quad_count; }
    ;

/* ---------- WRITE ---------- */
write_stmt
    : WRITE '(' expression ')' ';'
      { gen_quad("WRITE", $3.name, "", ""); }
    ;

/* ---------- READ ---------- */
read_stmt
    : READ '(' IDF ')' ';'
      {
          SymbolEntry *s = search_symbol($3);
          if (s == NULL || !s->is_declared) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (variable non declaree)\n",
                     yylineno, column, $3);
              nb_err_sem++;
          } else if (s->nature == NAT_CONSTANT) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (lecture dans une constante)\n",
                     yylineno, column, $3);
              nb_err_sem++;
          } else {
              gen_quad("READ", "", "", $3);
              set_initialized($3);
          }
      }
    ;

/* ==========================================================
   EXPRESSIONS
   ========================================================== */
expression
    : expression '+' expression
      {
          char *t = new_temp();
          $$.type = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT :
                    ($1.type == TYPE_UNKNOWN || $3.type == TYPE_UNKNOWN) ? TYPE_UNKNOWN : TYPE_INTEGER;
          gen_quad("+", $1.name, $3.name, t);
          strcpy($$.name, t);
      }
    | expression '-' expression
      {
          char *t = new_temp();
          $$.type = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT :
                    ($1.type == TYPE_UNKNOWN || $3.type == TYPE_UNKNOWN) ? TYPE_UNKNOWN : TYPE_INTEGER;
          gen_quad("-", $1.name, $3.name, t);
          strcpy($$.name, t);
      }
    | expression '*' expression
      {
          char *t = new_temp();
          $$.type = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT :
                    ($1.type == TYPE_UNKNOWN || $3.type == TYPE_UNKNOWN) ? TYPE_UNKNOWN : TYPE_INTEGER;
          gen_quad("*", $1.name, $3.name, t);
          strcpy($$.name, t);
      }
    | expression '/' expression
      {
          if (is_number($3.name) && atof($3.name) == 0) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '/' (division par zero)\n",
                     yylineno, column);
              nb_err_sem++;
          }
          char *t = new_temp();
          $$.type = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT :
                    ($1.type == TYPE_UNKNOWN || $3.type == TYPE_UNKNOWN) ? TYPE_UNKNOWN : TYPE_INTEGER;
          gen_quad("/", $1.name, $3.name, t);
          strcpy($$.name, t);
      }
    | expression '>' expression
      { char *t = new_temp(); gen_quad(">",  $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression '<' expression
      { char *t = new_temp(); gen_quad("<",  $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression GEQ expression
      { char *t = new_temp(); gen_quad(">=", $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression LEQ expression
      { char *t = new_temp(); gen_quad("<=", $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression EQ expression
      { char *t = new_temp(); gen_quad("==", $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression NEQ expression
      { char *t = new_temp(); gen_quad("!=", $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression AND expression
      { char *t = new_temp(); gen_quad("&&", $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | expression OR expression
      { char *t = new_temp(); gen_quad("||", $1.name, $3.name, t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | NOT expression
      { char *t = new_temp(); gen_quad("!", $2.name, "", t);
        strcpy($$.name, t); $$.type = TYPE_INTEGER; }
    | '-' expression  %prec UMINUS
      { char *t = new_temp(); gen_quad("NEG", $2.name, "", t);
        strcpy($$.name, t); $$.type = $2.type; }
    | '+' expression  %prec UPLUS
      { $$ = $2; }
    | '(' expression ')'
      { $$ = $2; }

    | IDF
      {
          SymbolEntry *s = search_symbol($1);
          if (s == NULL || !s->is_declared) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (variable non declaree)\n",
                     yylineno, column, $1);
              nb_err_sem++;
              $$.type = TYPE_UNKNOWN;
          } else {
              $$.type = s->type;
          }
          strcpy($$.name, $1);
      }

    | IDF '[' expression ']'
      {
          SymbolEntry *s = search_symbol($1);
          if (s == NULL || !s->is_declared) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (variable non declaree)\n",
                     yylineno, column, $1);
              nb_err_sem++;
              $$.type = TYPE_UNKNOWN;
          } else if (s->nature != NAT_ARRAY) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (n'est pas un tableau)\n",
                     yylineno, column, $1);
              nb_err_sem++;
              $$.type = s->type;
          } else {
              if ($3.type != TYPE_INTEGER && $3.type != TYPE_UNKNOWN) {
                  printf("Erreur Semantique : ligne %d , colonne %d , element '%s' (indice de tableau non entier)\n",
                         yylineno, column, $1);
                  nb_err_sem++;
              }
              if (is_number($3.name)) {
                  int idx = atoi($3.name);
                  if (idx < 0 || idx >= s->array_size) {
                      printf("Erreur Semantique : ligne %d , colonne %d , element '%s[%d]' (indice hors bornes, taille=%d)\n",
                             yylineno, column, $1, idx, s->array_size);
                      nb_err_sem++;
                  }
              }
              $$.type = s->type;
          }
          char tmp[50];
          snprintf(tmp, sizeof(tmp), "%s[%s]", $1, $3.name);
          strcpy($$.name, tmp);
      }

    | INTEGER_CONST
      {
          if ($1 < -32768 || $1 > 32767) {
              printf("Erreur Semantique : ligne %d , colonne %d , element '%d' (entier hors intervalle [-32768, 32767])\n",
                     yylineno, column, $1);
              nb_err_sem++;
          }
          sprintf($$.name, "%d", $1);
          $$.type = TYPE_INTEGER;
      }
    | FLOAT_CONST
      {
          sprintf($$.name, "%g", $1);
          $$.type = TYPE_FLOAT;
      }
    ;

%%
