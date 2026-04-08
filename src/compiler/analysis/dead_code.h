#ifndef func_lang_compiler_analysis_dead_code_h
#define func_lang_compiler_analysis_dead_code_h

#include "../../parsing/ast.h"
#include "../../parsing/nodes.h"

void remove_dead_code(struct DeclarationNode *root);

#endif
