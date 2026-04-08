#ifndef func_lang_object_h
#define func_lang_object_h

#include "value.h"

enum ObjType {
    OBJ_BOX,
    OBJ_CONS,
    OBJ_CLOSURE,
    OBJ_THUNK,
    OBJ_APPLICATION,
};

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)
#define TO_OBJ(val)     (&val->obj)

struct Obj {
    enum ObjType type;
    struct Obj *next;
};

struct Box {
    struct Obj obj;
    struct Value val;
};

struct Cons {
    struct Obj obj;
    struct Value l;
    struct Value r;
};

struct ClosureInfo {
    u32 arity;
    u32 address;
};

/// dynamically sized
struct Closure {
    struct Obj obj;
    struct ClosureInfo *info;
    /// struct Box *captures[]
};

/// dynamically sized
struct Thunk {
    struct Obj obj;
    /// struct Box *fvs[]
};

/// dynamically sized
struct Application {
    struct Obj obj;
    // always points to a `struct Closure`, never a `struct Application`
    struct Obj *closure;
    u32 arity;
    u32 arg_count;
    /// struct Box *arguments[]
};

static inline bool is_obj_type(struct Value val, enum ObjType type)
{
    return IS_OBJ(val) && OBJ_TYPE(val) == type;
}

struct Application *obj_create_application(u8 arg_count);
struct Box **obj_dyn_fields(struct Obj *obj);

#endif
