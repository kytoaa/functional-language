#ifndef func_lang_value_h
#define func_lang_value_h

#include "prelude.h"

enum ValueType {
    VALUE_UNIT,
    VALUE_INT,
    VALUE_BOOL,
    VALUE_CHAR,
    VALUE_OBJ,
};

struct Obj;

typedef struct Obj *Val;

/// values are typically boxed
struct Value {
    enum ValueType type;
    union {
        i32 integer;
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

static inline bool value_equal(struct Value l, struct Value r)
{
    if (l.type != r.type)
        return false;
    switch (l.type) {
        case VALUE_UNIT:
            return true;
        case VALUE_INT:
            return l.as.integer == r.as.integer;
        case VALUE_BOOL:
            return l.as.boolean == r.as.boolean;
        case VALUE_CHAR:
            return l.as.character == r.as.character;
        case VALUE_OBJ:
            panic("cannot compare objects");
    }
    return false;
}

#define INT_VAL(val)    ((struct Value){ VALUE_INT, { .integer = (val) } })
#define BOOL_VAL(val)   ((struct Value){ VALUE_BOOL, { .boolean = (val) } })
#define CHAR_VAL(val)   ((struct Value){ VALUE_CHAR, { .character = (val) } })
#define UNIT_VAL()      ((struct Value){ VALUE_UNIT, { .object = null } })
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
