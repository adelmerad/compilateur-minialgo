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
