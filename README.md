# MiniAlgo Compiler

![C](https://img.shields.io/badge/language-C-00599C?logo=c&logoColor=white)
![Flex](https://img.shields.io/badge/Flex-lexer-4CAF50)
![Bison](https://img.shields.io/badge/Bison-parser-FF9800)
![8086](https://img.shields.io/badge/target-8086_assembly-6E40C9)

A complete compiler for **MiniAlgo**, a small algorithmic language, written in C with Flex and Bison. It goes from source code to **8086 assembly** runnable in emu8086, through every classic compilation phase.

Compilation course project — USTHB, Master's in Software Engineering, 2025/2026.

## ✨ Features

- **Lexical analysis** with Flex — keywords, identifiers, integer and float literals, comments, line/column tracking
- **Syntax analysis** with Bison — declarations, constants, arrays, `IF/ELSE`, `WHILE`, `FOR`, `READ`, `WRITE`, arithmetic and logical expressions
- **Semantic analysis** — undeclared or duplicate variables, type mismatches, constant modification, division by zero, array bounds and index checks
- **Symbol table** — hash table with chaining
- **Intermediate code** — quadruplets with backpatching for jumps
- **5 optimization passes** on the quadruplets
- **8086 code generation** — stack, data and code segments, compatible with emu8086

## 🏗️ Pipeline

```
source.txt ──▶ Lexer (Flex) ──▶ Parser + semantic checks (Bison) ──▶ Quadruplets
                                         │                                 │
                                         ▼                                 ▼
                                   Symbol table                    5 optimization passes
                                                                           │
                                                                           ▼
                                                                output.asm (8086)
```

## 📝 The MiniAlgo Language

```
PROGRAM Example

DECL
    INTEGER : A, B, I, Res ;
    FLOAT   : F ;
    INTEGER : T[10] ;
    CONST PI = 3.14 ;
ENDDECL

BEGIN
    A = 5 ;
    B = 3 ;

    IF (A > B) {
        Res = A ;
    }
    ELSE {
        Res = B ;
    }

    WHILE (A < 10 && A >= 0) {
        A = A + 1 ;
    }

    // FOR (variable : start : step : end)
    FOR (I : 0 : 1 : 9) {
        T[I] = I * 2 ;
    }

    WRITE(Res) ;
    READ(B) ;
END
```

## ⚙️ Optimizations

| # | Pass | Example |
|---|------|---------|
| 1 | Algebraic simplification | `x*1 → x`, `x+0 → x`, `x*2 → x+x`, `x-x → 0` |
| 2 | Constant folding | `(+, 5, 3, T0) → (=, 8, _, T0)` |
| 3 | Copy propagation | `(=, X, _, T)` then `(op, T, …)` → uses `X` directly |
| 4 | Common subexpression elimination | a repeated `(op, a, b, T2)` reuses `T1` |
| 5 | Dead code elimination | removes temporaries that are never read |

Passes run until nothing changes, then jump targets are remapped. On the sample program: **463 → 459 quadruplets, 20 transformations in 3 passes**.

## 🚀 Build and Run

Requirements: `gcc`, `flex`, `bison` and `make`.

```bash
make            # generate the lexer/parser and build ./minialgo
make run        # compile test.txt → prints the symbol table and quadruplets, writes output.asm
make errors     # compile test_sujet.txt, a program full of intentional errors
make clean
```

Or by hand:

```bash
bison -d parser.y
flex lexer.l
gcc -o minialgo lex.yy.c parser.tab.c quadruplets.c symbol_table.c optimization.c codegen.c
./minialgo test.txt
```

On Windows, use `mingw32-make`. With the GnuWin32 build of Bison installed under `C:\Program Files (x86)`, set `BISON_PKGDATADIR=C:/PROGRA~2/GnuWin32/share/bison` and `M4=C:/PROGRA~2/GnuWin32/bin/m4.exe` first: its `m4` cannot handle spaces in the install path.

Open `output.asm` in **emu8086** to run the generated program.

## 🔎 Sample Output

Errors follow the course format, `Erreur <Type> : ligne X , colonne Y , element Z`:

```
Erreur Semantique : ligne 20 , colonne 18 , element 'A' (double declaration)
Erreur Semantique : ligne 48 , colonne 15 , element '/' (division par zero)
Erreur Semantique : ligne 51 , colonne 16 , element 'T[15]' (indice hors bornes, taille=10)
```

Quadruplets after optimization, and the generated 8086 code:

```
0 - ( = , 5 , vide , A )              CODE SEGMENT
1 - ( = , 3 , vide , B )                  ASSUME CS:CODE, DS:DATA, SS:PILE
2 - ( = , 0 , vide , C )              MAIN PROC
                                          MOV AX, DATA
                                          MOV DS, AX
                                      LBL_0:
                                          MOV AX, 5
                                          MOV A, AX
```

## 📂 Project Structure

| File | Role |
|------|------|
| `lexer.l` | Flex lexical analyzer |
| `parser.y` | Bison grammar, semantic checks, quadruplet generation, `main` |
| `symbol_table.c/.h` | Hash table with chaining |
| `quadruplets.c/.h` | Intermediate code and backpatching |
| `optimization.c/.h` | The 5 optimization passes |
| `codegen.c/.h` | 8086 assembly generation |
| `test.txt` | Sample program covering the whole language |
| `test_sujet.txt` | Program with intentional lexical, syntax and semantic errors |

## 🗓️ Versions

- **v1.0** — front end and intermediate code: lexer, parser, semantic analysis, symbol table, quadruplets
- **v2.0** — the 5 optimization passes and 8086 code generation

## 👤 Author

**Adel Merad** — [GitHub](https://github.com/adelmerad)
