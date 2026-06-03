#ifndef CODEGEN_H
#define CODEGEN_H

/* ==========================================================
   Generation de code Assembleur 8086 (compatible emu8086)
   ----------
   Traduit les quadruplets en code assembleur 8086 :
     - Segment PILE (pile d'execution)
     - Segment DATA (variables, constantes, tableaux, temporaires)
     - Segment CODE (instructions traduites + procedures WRITE/READ)
   ========================================================== */

void generate_assembly(const char *filename);

#endif
