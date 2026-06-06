# MiniAlgo compiler — build with `make` (Linux/macOS) or `mingw32-make` (Windows)

CC      = gcc
CFLAGS  = -Wall -O2
BISON   = bison
FLEX    = flex
TARGET  = minialgo

SRC     = quadruplets.c symbol_table.c optimization.c codegen.c
GEN     = parser.tab.c parser.tab.h lex.yy.c

.PHONY: all run errors clean

all: $(TARGET)

parser.tab.c parser.tab.h: parser.y
	$(BISON) -d parser.y

lex.yy.c: lexer.l parser.tab.h
	$(FLEX) lexer.l

$(TARGET): parser.tab.c lex.yy.c $(SRC)
	$(CC) $(CFLAGS) -o $@ lex.yy.c parser.tab.c $(SRC)

# Compile the sample program and generate output.asm
run: $(TARGET)
	./$(TARGET) test.txt

# Compile the program full of intentional errors (expected to fail)
errors: $(TARGET)
	-./$(TARGET) test_sujet.txt

clean:
	rm -f $(TARGET) $(TARGET).exe $(GEN) parser.output output.asm *.o
