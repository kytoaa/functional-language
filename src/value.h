#ifndef func_lang_value_h
#define func_lang_value_h

#include "prelude.h"

enum ValueType {
    VALUE_INT,
    VALUE_BOOL,
    VALUE_CHAR,
    VALUE_OBJ,
};

struct Obj;

struct Value {
    enum ValueType type;
    union {
        u32 integer;
        bool boolean;
        u32 character;
        struct Obj *object;
    } as;
};

struct ValueList {
    struct Value *ptr;
    u32 len;
    u32 cap;
};

#define INT_VAL(val)    ((struct Value){ VALUE_INT, { .integer = (val) } })
#define BOOL_VAL(val)   ((struct Value){ VALUE_BOOL, { .boolean = (val) } })
#define CHAR_VAL(val)   ((struct Value){ VALUE_CHAR, { .character = (val) } })
#define OBJ_VAL(val)    ((struct Value){ VALUE_OBJ, { .object = (val) } })

#define IS_INT(val)     ((val).type == VALUE_INT)
#define IS_BOOL(val)    ((val).type == VALUE_BOOL)
#define IS_CHAR(val)    ((val).type == VALUE_CHAR)
#define IS_OBJ(val)     ((val).type == VALUE_OBJ)

#define AS_INT(val)     ((val).as.integer)
#define AS_BOOL(val)    ((val).as.boolean)
#define AS_CHAR(val)    ((val).as.character)
#define AS_OBJ(val)     ((val).as.object)

#endif
