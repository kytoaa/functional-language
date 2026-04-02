#include "ident_table.h"
#include "ast.h"
#include "string.h"

char *ident_table_get(struct IdentifierTable *table, const char *str, u32 str_len)
{
    for (u32 i = 0; i < table->count; i++) {
        if (table->items[i].len != str_len)
            continue;

        if (memcmp(str, table->items[i].ptr, str_len) == 0) {
            return table->items[i].ptr;
        }
    }
    if (table->cap == table->count) {
        u32 new_cap = (table->cap == 0) ? 8 : table->cap * 2;
        table->items = realloc_mem(table->items, sizeof(*table->items) * new_cap);
        table->cap = new_cap;
    }
    char *str_mem = ast_alloc(str_len);
    memcpy(str_mem, str, str_len);

    table->items[table->count].ptr = str_mem;
    table->items[table->count].len = str_len;
    table->count += 1;

    return str_mem;
}

void init_ident_table(struct IdentifierTable *table)
{
    *table = (struct IdentifierTable){
        .items = null,
        .count = 0,
        .cap = 0,
    };
}
void free_ident_table(struct IdentifierTable *table)
{
    free_mem(table->items);
    init_ident_table(table);
}
