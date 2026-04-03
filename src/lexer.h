#ifndef func_lang_lexer_h
#define func_lang_lexer_h

#include "prelude.h"

enum TokenType {
    TOKEN_EQ,
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,
    TOKEN_L_BRACKET,
    TOKEN_R_BRACKET,
    TOKEN_SEMICOLON,
    TOKEN_PIPE,

    TOKEN_IDENT,

    TOKEN_ARROW,
    TOKEN_WIDE_ARROW,
    TOKEN_FUN,
    TOKEN_LET,
    TOKEN_IN,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELSE,
    TOKEN_CASE,
    TOKEN_OF,

    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQ,
    TOKEN_LESS,
    TOKEN_LESS_EQ,
    TOKEN_COLON,
    TOKEN_DOUBLE_COLON,

    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    TOKEN_UNIT,
    TOKEN_NUM,
    TOKEN_CHAR,

    TOKEN_TRUE,
    TOKEN_FALSE,

    TOKEN_EOF,
    TOKEN_ERROR,
};

struct Token {
    const char *start;
    u32 len;
    u32 line;
    enum TokenType type;
};

void init_lexer(const char *src);

struct Token next_token();

static const char *token_type_name(enum TokenType type)
{
    switch (type) {
        case TOKEN_EQ:
            return "TOKEN_EQ";
        case TOKEN_L_PAREN:
            return "TOKEN_L_PAREN";
        case TOKEN_R_PAREN:
            return "TOKEN_R_PAREN";
        case TOKEN_L_BRACE:
            return "TOKEN_L_BRACE";
        case TOKEN_R_BRACE:
            return "TOKEN_R_BRACE";
        case TOKEN_L_BRACKET:
            return "TOKEN_L_BRACKET";
        case TOKEN_R_BRACKET:
            return "TOKEN_R_BRACKET";
        case TOKEN_SEMICOLON:
            return "TOKEN_SEMICOLON";
        case TOKEN_PIPE:
            return "TOKEN_PIPE";
        case TOKEN_IDENT:
            return "TOKEN_IDENT";
        case TOKEN_ARROW:
            return "TOKEN_ARROW";
        case TOKEN_WIDE_ARROW:
            return "TOKEN_WIDE_ARROW";
        case TOKEN_FUN:
            return "TOKEN_FUN";
        case TOKEN_LET:
            return "TOKEN_LET";
        case TOKEN_IN:
            return "TOKEN_IN";
        case TOKEN_IF:
            return "TOKEN_IF";
        case TOKEN_THEN:
            return "TOKEN_THEN";
        case TOKEN_ELSE:
            return "TOKEN_ELSE";
        case TOKEN_CASE:
            return "TOKEN_CASE";
        case TOKEN_OF:
            return "TOKEN_OF";
        case TOKEN_ADD:
            return "TOKEN_ADD";
        case TOKEN_SUB:
            return "TOKEN_SUB";
        case TOKEN_MUL:
            return "TOKEN_MUL";
        case TOKEN_DIV:
            return "TOKEN_DIV";
        case TOKEN_EQUAL:
            return "TOKEN_EQUAL";
        case TOKEN_GREATER:
            return "TOKEN_GREATER";
        case TOKEN_GREATER_EQ:
            return "TOKEN_GREATER_EQ";
        case TOKEN_LESS:
            return "TOKEN_LESS";
        case TOKEN_LESS_EQ:
            return "TOKEN_LESS_EQ";
        case TOKEN_COLON:
            return "TOKEN_COLON";
        case TOKEN_DOUBLE_COLON:
            return "TOKEN_DOUBLE_COLON";
        case TOKEN_AND:
            return "TOKEN_AND";
        case TOKEN_OR:
            return "TOKEN_OR";
        case TOKEN_NOT:
            return "TOKEN_NOT";
        case TOKEN_UNIT:
            return "TOKEN_UNIT";
        case TOKEN_NUM:
            return "TOKEN_NUM";
        case TOKEN_CHAR:
            return "TOKEN_CHAR";
        case TOKEN_TRUE:
            return "TOKEN_TRUE";
        case TOKEN_FALSE:
            return "TOKEN_FALSE";
        case TOKEN_EOF:
            return "TOKEN_EOF";
        case TOKEN_ERROR:
            return "TOKEN_ERROR";
        default:
            return "ERROR";
    }
}

#endif
