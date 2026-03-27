#include "parser.h"
#include "lexer.h"

struct Parser {
    struct Token prev;
    struct Token current;
};

static struct Parser parser = {};

enum Precedence {
    PREC_NONE,

    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // not -
    PREC_APPLICATION,
    PREC_PRIMARY,
};

static void advance()
{
    parser.prev = parser.current;
    parser.current = next_token();
}

bool compile(const char *src, struct Chunk *chunk)
{
    init_lexer(src);

    return false;
}
