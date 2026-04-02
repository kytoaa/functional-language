#ifndef func_lang_parsing_parser_h
#define func_lang_parsing_parser_h

#include "ast.h"
#include "../lexer.h"

struct ParseError {
    const char *msg;
    struct Token token;
};

bool build_ast(const char *src, struct AstNode **out, struct ParseError *err);

#endif
