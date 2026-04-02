#ifndef func_lang_parsing_ident_table_h
#define func_lang_parsing_ident_table_h

#include "../prelude.h"

struct IdentifierTable {
    // owned memory
    struct {
        // ast memory
        char *ptr;
        u32 len;
    } *items;
    u32 count;
    u32 cap;
};

char *ident_table_get(struct IdentifierTable *table, const char *str, u32 str_len);
void init_ident_table(struct IdentifierTable *table);
void free_ident_table(struct IdentifierTable *table);

#endif
