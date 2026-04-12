#ifndef func_lang_object_h
#define func_lang_object_h

#include "value.h"

enum ObjType {
    OBJ_BOX,
    OBJ_CONS,
    OBJ_CLOSURE,
    OBJ_THUNK,
    OBJ_APPLICATION,

    OBJ_TYPE_COUNT,
};

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)
#define TO_OBJ(val)     (&val->obj)
#define IS_WHNF(val)    (val->flags.is_whnf)

typedef struct {
    bool is_whnf: 1;
    bool is_static: 1;
} ObjFlags;

struct Obj {
    enum ObjType type;
    ObjFlags flags;
    struct Obj *next;
};

struct Box {
    struct Obj obj;
    struct Value val;
};

struct Cons {
    struct Obj obj;
    Val l;
    Val r;
};

struct ClosureInfo {
    u32 arity;
    u32 address;
    u32 capture_count;
};

/// dynamically sized
struct Closure {
    struct Obj obj;
    struct ClosureInfo *info;
    /// struct Box *captures[]
};

/// dynamically sized
///
/// ## calling convention
///
/// if a thunk is a lazy computation, the first dynamic parameter
/// will be a `u64` containing the address to jump to
///
/// the thunk will expect the address of its free variables in `r1`
struct Thunk {
    struct Obj obj;
    /// updated once thunk has been evaluated
    struct Box *evaluated;
    /// struct Box *fvs[]
};

/// dynamically sized
///
/// arguments are stored in order of call, first argument is first applied value
struct Application {
    struct Obj obj;
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
struct Box *obj_create_box();
struct Closure *obj_create_closure(struct ClosureInfo *info);

struct Box **obj_dyn_fields(struct Obj *obj);

#endif
