
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "tables.h"

/*******************************
 * Helper Functions
 *******************************/

void allocation_failed() {
    write_to_log("Error: allocation failed\n");
    exit(1);
}

void addr_alignment_incorrect() {
    write_to_log("Error: address is not a multiple of 4.\n");
}

void name_already_exists(const char* name) {
    write_to_log("Error: name '%s' already exists in table.\n", name);
}

void write_symbol(FILE* output, uint32_t addr, const char* name) {
    fprintf(output, "%u\t%s\n", addr, name);
}

/*******************************
 * Symbol Table Functions
 *******************************/

/* Creates a new SymbolTable containg 0 elements and returns a pointer to that
   table. Multiple SymbolTables may exist at the same time. 
   If memory allocation fails, you should call allocation_failed(). 
 */
SymbolTable* create_table() {
    SymbolTable *t = malloc(sizeof(SymbolTable));
    if (t == NULL) {
      allocation_failed();
    }
    (*t).tbl = malloc(sizeof(Symbol) * 10);
    (*t).len = 0;
    (*t).cap = 10;
    return t;
}

/* Frees the given SymbolTable and all associated memory. */
void free_table(SymbolTable* table) {
    Symbol *t = table->tbl;
    while (t < table->len) {
      free(t->name);
      free(t);
      t++;
    }
    free(table);
}

/* Adds a new symbol and its address to the SymbolTable pointed to by TABLE. 
   ADDR is given as the byte offset from the first instruction. The SymbolTable
   must be able to resize itself as more elements are added. 

   Note that NAME may point to a temporary array, so it is not safe to simply
   store the NAME pointer. You must store a copy of the given string.

   If ADDR is not word-aligned, you should call addr_alignment_incorrect() and
   return -1. If NAME already exists in the table, you should call 
   name_already_exists() and return -1. If memory allocation fails, you should
   call allocation_failed(). 

   Otherwise, you should store the symbol name and address and return 0.
 */
int add_to_table(SymbolTable* table, const char* name, uint32_t addr) {
    if ((addr % 4) != 0) {
        addr_alignment_incorrect();
        return -1;
    }
    if ((*table).len == (*table).cap) {
        int r = (4 * (*table).cap) * (sizeof(Symbol));
        table->tbl = realloc(table->tbl, r);
        if (table->tbl == NULL) {
            allocation_failed();
            return -1;
        }
        table->cap = table->cap * 4;
    }
    int i;
    Symbol* t = table->tbl;
    for (i = 0; i < table->len; i++) {
      if (strcmp(t[i].name, name) == 0) {
          name_already_exists(name);
          return -1;
      }
    }
    Symbol *temp = malloc(sizeof(Symbol));
    temp->name = malloc(sizeof(name) + 1);
    strcpy(temp->name, name);
    temp->addr = addr;
    (*table).len += 1;
    t[i] = *temp;
    return 0;
  }

/* Returns the address (byte offset) of the given symbol. If a symbol with name
   NAME is not present in TABLE, return -1.
 */
int64_t get_addr_for_symbol(SymbolTable* table, const char* name) {
    int i;
    Symbol* t = (*table).tbl;
    for (i = 0; i < (*table).len; i++) {
      if (strcmp(t[i].name, name) == 0) {
          return t[i].addr;
      }
    }
    // printf("%s%s :\n", "Failed", name);
    // for (i = 0; i < (*table).len; i++) {
    //   printf("%s\n", t[i].name);
    // }
    return -1;   
  }

/* Writes the SymbolTable TABLE to OUTPUT. You should use write_symbol() to
   perform the write. Do not print any additional whitespace or characters.
 */
void write_table(SymbolTable* table, FILE* output) {
    /* YOUR CODE HERE */
}
