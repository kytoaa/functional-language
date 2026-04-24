#include "lexer.h"

struct Lexer {
    const char *src;
    const char *token_start;
    const char *current;
    u32 line;
};

static struct Lexer lexer = {};

static u32 current_len()
{
    return lexer.current - lexer.token_start;
}

static bool at_end()
{
    return *lexer.current == EOF_CHAR;
}

static char peek()
{
    return *lexer.current;
}
static char peek_next()
{
    if (at_end())
        return EOF_CHAR;
    return lexer.current[1];
}
static char consume()
{
    lexer.current++;
    return lexer.current[-1];
}

static bool match(char expected)
{
    if (at_end())
        return false;
    if (*lexer.current != expected)
        return false;

    lexer.current++;
    return true;
}

static struct Token make_token(enum TokenType type)
{
    return (struct Token){
        .start = lexer.token_start,
        .len = current_len(),
        .line = lexer.line,
        .type = type,
    };
}
static struct Token make_error(const char *msg)
{
    u32 len = 0;
    while (msg[len] != '\0')
        len++;

    return (struct Token){
        .start = msg,
        .len = len,
        .line = lexer.line,
        .type = TOKEN_ERROR,
    };
}

static void skip_whitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                consume();
                break;
            case '\n':
                lexer.line++;
                consume();
                break;
            case '-':
                if (peek_next() == '-') {
                    while (peek() != '\n' && !at_end())
                        consume();
                    break;
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

static bool check_keyword(const char *keyword, u32 len)
{
    if (len != current_len())
        return false;

    for (u32 i = 0; i < len; i++) {
        if (keyword[i] != lexer.token_start[i])
            return false;
    }
    return true;
}

static enum TokenType identifier_type() 
{
    switch (*lexer.token_start) {
        case 'i':
            return check_keyword("if", 2) ? TOKEN_IF
                 : check_keyword("in", 2) ? TOKEN_IN
                 : TOKEN_IDENT;
        case 't':
            return check_keyword("then", 4) ? TOKEN_THEN
                 : check_keyword("true", 4) ? TOKEN_TRUE
                 : TOKEN_IDENT;
        case 'e':
            return check_keyword("else", 4) ? TOKEN_ELSE : TOKEN_IDENT;
        case 'c':
            return check_keyword("case", 4) ? TOKEN_CASE : TOKEN_IDENT;
        case 'l':
            return check_keyword("let", 3) ? TOKEN_LET : TOKEN_IDENT;
        case 'f':
            return check_keyword("fun", 3) ? TOKEN_FUN
                 : check_keyword("false", 5) ? TOKEN_FALSE
                 : TOKEN_IDENT;
        case 'a':
            return check_keyword("and", 3) ? TOKEN_AND : TOKEN_IDENT;
        case 'o':
            return check_keyword("or", 2) ? TOKEN_OR
                 : check_keyword("of", 2) ? TOKEN_OF
                 : TOKEN_IDENT;
        case 'n':
            return check_keyword("not", 3) ? TOKEN_NOT : TOKEN_IDENT;

        case 'm':
            return check_keyword("mod", 3) ? TOKEN_MOD : TOKEN_IDENT;

        case 'u':
            return check_keyword("use", 3) ? TOKEN_USE : TOKEN_IDENT;

        case '_':
            return check_keyword("_", 1) ? TOKEN_UNDERSCORE : TOKEN_IDENT;

        default:
            return TOKEN_IDENT;
    }
}

static struct Token ident()
{
    while (is_alpha(peek()) || is_digit(peek()))
        consume();
    return make_token(identifier_type());
}
static struct Token number()
{
    while (is_digit(peek()) || peek() == '_')
        consume();
    return make_token(TOKEN_NUM);
}

static struct Token char_token()
{
    if (peek() == '\\')
        consume();
    consume();
    if (!match('\''))
        return make_error("invalid character literal");
    return make_token(TOKEN_CHAR);
}

void init_lexer(const char *src)
{
    lexer = (struct Lexer){
        .src = src,
        .token_start = src,
        .current = src,
        .line = 1,
    };
}

struct Token next_token()
{
    skip_whitespace();
    lexer.token_start = lexer.current;

    if (at_end())
        return make_token(TOKEN_EOF);

    char c = consume();

    if (is_alpha(c))
        return ident();
    if (is_digit(c))
        return number();
    
    switch (c) {
        case '(':
            return make_token(match(')') ? TOKEN_UNIT : TOKEN_L_PAREN);
        case ')':
            return make_token(TOKEN_R_PAREN);
        case '[':
            return make_token(TOKEN_L_BRACKET);
        case ']':
            return make_token(TOKEN_R_BRACKET);
        case '{':
            return make_token(TOKEN_L_BRACE);
        case '}':
            return make_token(TOKEN_R_BRACE);

        case ';':
            return make_token(TOKEN_SEMICOLON);

        case '|':
            return make_token(TOKEN_PIPE);

        case '=':
            return make_token(match('=') ? TOKEN_EQUAL
                             : match('>') ? TOKEN_WIDE_ARROW
                             : TOKEN_EQ);
        case '>':
            return make_token(match('=') ? TOKEN_GREATER_EQ : TOKEN_GREATER);
        case '<':
            return make_token(match('=') ? TOKEN_LESS_EQ : TOKEN_LESS);

        case '+':
            return make_token(TOKEN_ADD);
        case '-':
            return make_token(match('>') ? TOKEN_ARROW : TOKEN_SUB);
        case '*':
            return make_token(TOKEN_MUL);
        case '/':
            return make_token(TOKEN_DIV);

        case ':':
            return make_token(match(':') ? TOKEN_DOUBLE_COLON : TOKEN_COLON);

        case '\'':
            return char_token();
    }

    return make_token(TOKEN_ERROR);
}

