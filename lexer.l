%{
/* ==========================================================
   Analyseur Lexical pour le langage MiniAlgo
   Projet de compilation - USTHB 2025/2026
   Format des erreurs (selon sujet) :
     "Erreur <Type> : ligne X , colonne Y , element Z"
   ========================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "parser.tab.h"

int column = 1;
int nb_err_lex = 0;

#define YY_USER_ACTION { column += yyleng; }
%}

%option yylineno
%option noyywrap 
%option noinput
%option nounput

%x COMMENT

LETTRE      [a-zA-Z]
CHIFFRE     [0-9]
IDENT       {LETTRE}({LETTRE}|{CHIFFRE}|_)*
ENTIER      {CHIFFRE}+
REEL        {CHIFFRE}+\.{CHIFFRE}+
