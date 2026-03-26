#ifndef func_lang_object_h
#define func_lang_object_h

#include "value.h"

enum ObjType {
    OBJ_CONS,
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

static inline bool is_obj_type(struct Value val, enum ObjType type)
{
    return IS_OBJ(val) && OBJ_TYPE(val) == type;
}

#endif
