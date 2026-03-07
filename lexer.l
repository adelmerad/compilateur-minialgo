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

%%

 /* ========== 1. COMMENTAIRES ========== */

"//"[^\n]*                  { /* commentaire ligne : ignore */ }

"/*"                        { BEGIN(COMMENT); }
<COMMENT>"*/"               { BEGIN(INITIAL); }
<COMMENT>\n                 { column = 1; }
<COMMENT>.                  { /* ignorer contenu */ }
<COMMENT><<EOF>>            {
    printf("Erreur Lexicale : ligne %d , colonne %d , element 'commentaire non ferme'\n",
           yylineno, column);
    nb_err_lex++;
    yyterminate();
}

 /* ========== 2. MOTS-CLES ========== */

"PROGRAM"   { return PROGRAM; }
"DECL"      { return DECL; }
"ENDDECL"   { return ENDDECL; }
"BEGIN"     { return BEGIN_T; }
"END"       { return END; }
"INTEGER"   { return INTEGER; }
"FLOAT"     { return FLOAT; }
"CONST"     { return CONST; }
"IF"        { return IF; }
"ELSE"      { return ELSE; }
"FOR"       { return FOR; }
"WHILE"     { return WHILE; }
"READ"      { return READ; }
"WRITE"     { return WRITE; }

 /* ========== 3. OPERATEURS COMPOSES ========== */

"&&"        { return AND; }
"||"        { return OR; }
"!="        { return NEQ; }
"=="        { return EQ; }
">="        { return GEQ; }
"<="        { return LEQ; }
"!"         { return NOT; }

 /* ========== 4. CONSTANTES SIGNEES ========== */

"("[ \t]*[+-][ \t]*{REEL}[ \t]*")" {
    char buf[64];
    int j = 0;
    for (int i = 0; i < yyleng && j < 63; i++) {
        char c = yytext[i];
        if (c == '+' || c == '-' || (c >= '0' && c <= '9') || c == '.')
            buf[j++] = c;
    }
    buf[j] = '\0';
    yylval.float_val = atof(buf);
    return FLOAT_CONST;
}

"("[ \t]*[+-][ \t]*{ENTIER}[ \t]*")" {
    char buf[64];
    int j = 0;
    for (int i = 0; i < yyleng && j < 63; i++) {
        char c = yytext[i];
        if (c == '+' || c == '-' || (c >= '0' && c <= '9'))
            buf[j++] = c;
    }
    buf[j] = '\0';
    long v = atol(buf);
    if (v > 32767 || v < -32768) {
        printf("Erreur Lexicale : ligne %d , colonne %d , element '%s' (entier hors intervalle [-32768, 32767])\n",
               yylineno, column - yyleng, yytext);
        nb_err_lex++;
        yylval.int_val = 0;
    } else {
        yylval.int_val = (int)v;
    }
    return INTEGER_CONST;
}

 /* ========== 5. IDENTIFICATEURS ========== */

{IDENT} {
    if (yyleng > 8) {
        printf("Erreur Lexicale : ligne %d , colonne %d , element '%s' (identificateur > 8 caracteres)\n",
               yylineno, column - yyleng, yytext);
        nb_err_lex++;
        /* On marque yylval.str avec un prefixe special "!!" pour que le
           parser SACHE que l'identificateur est INVALIDE et ne doit pas
           etre insere dans la TS. */
        strcpy(yylval.str, "!!INVALID");
        return IDF;
    }

    /* Identificateur VALIDE : insertion dans la TS */
    strncpy(yylval.str, yytext, 8);
    yylval.str[8] = '\0';

    if (search_symbol(yylval.str) == NULL) {
        insert_symbol(yylval.str, TYPE_UNKNOWN, NAT_VARIABLE, yylineno);
    }
    return IDF;
}

 /* ========== 6. CONSTANTES NUMERIQUES ========== */

 /* Reel mal forme : 3.14.5 */
{ENTIER}\.{ENTIER}\.{ENTIER} {
    printf("Erreur Lexicale : ligne %d , colonne %d , element '%s' (reel mal forme)\n",
           yylineno, column - yyleng, yytext);
    nb_err_lex++;
    yylval.float_val = 0.0;
    return FLOAT_CONST;
}

{REEL} {
    yylval.float_val = atof(yytext);
    return FLOAT_CONST;
}

{ENTIER} {
    long v = atol(yytext);
    if (v > 32767) {
        printf("Erreur Lexicale : ligne %d , colonne %d , element '%s' (entier hors intervalle [-32768, 32767])\n",
               yylineno, column - yyleng, yytext);
        nb_err_lex++;
        yylval.int_val = 0;
    } else {
        yylval.int_val = (int)v;
    }
    return INTEGER_CONST;
}

 /* ========== 7. DELIMITEURS ========== */

"+"   { return '+'; }
"-"   { return '-'; }
"*"   { return '*'; }
"/"   { return '/'; }
">"   { return '>'; }
"<"   { return '<'; }
"="   { return '='; }
"("   { return '('; }
")"   { return ')'; }
"{"   { return '{'; }
"}"   { return '}'; }
"["   { return '['; }
"]"   { return ']'; }
";"   { return ';'; }
","   { return ','; }
":"   { return ':'; }

 /* ========== 8. BLANCS ========== */

\n            { column = 1; }
[ \t\r]+      { /* ignores */ }

 /* ========== 9. CARACTERES INVALIDES ========== */

. {
    printf("Erreur Lexicale : ligne %d , colonne %d , element '%s' (caractere invalide)\n",
           yylineno, column - yyleng, yytext);
    nb_err_lex++;
}

%%
