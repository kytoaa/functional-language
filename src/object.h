#ifndef func_lang_object_h
#define func_lang_object_h

#include "value.h"

enum ObjType {
    OBJ_CONS,
    OBJ_FUNCTION,
    OBJ_THUNK,
};

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)

struct Obj {
    enum ObjType type;
    struct Obj *next;
};

struct Cons {
    struct Obj obj;
    struct Value l;
    struct Value r;
};

struct Function {
    struct Obj obj;

    // owned
    struct Capture *captures;
};

struct Capture {
    struct Value value;
    struct Capture *next;
};

static inline bool is_obj_type(struct Value val, enum ObjType type)
{
    return IS_OBJ(val) && OBJ_TYPE(val) == type;
}

#endif
