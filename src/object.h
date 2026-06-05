#ifndef func_lang_object_h
#define func_lang_object_h

#include "value.h"
#include <stdio.h>

/// the minumum number of u64s required to store the obj
#define OBJ_U64_SIZE(obj) ((sizeof(obj) + sizeof(u64) - 1) / sizeof(u64))

enum ObjType {
    OBJ_BOX,
    OBJ_CONS,
    OBJ_CLOSURE,
    OBJ_THUNK,
    OBJ_APPLICATION,
    OBJ_FILE_HANDLE,
    OBJ_ARRAY,
    OBJ_SLICE,
    OBJ_RUNTIME_TYPE,
    OBJ_OBJECT,

    OBJ_TYPE_COUNT,
};

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)
#define TO_OBJ(val)     (&val->obj)
#define IS_WHNF(val)    (val->flags.is_whnf)

typedef struct {
    bool is_whnf : 1;
    bool is_static : 1;
    bool gc_marked : 1;
} ObjFlags;

struct Obj {
    enum ObjType type;
    ObjFlags flags;
    struct Obj *next;
};

typedef struct Obj *Val;

#define val_ptr(val) ((Val)(((u64)val) & 0x7fffffffffffffff))
#define as_val(val) ((Val)((u64)(val) | 0x8000000000000000))

struct Box {
    struct Obj obj;
    struct Value val;
};

struct Cons {
    struct Obj obj;
    Val l;
    Val r;
};

struct FileHandleObj {
    struct Obj obj;
    FILE *file;
};

struct ArrayObj {
    struct Obj obj;
    u8 *ptr;
    u32 len;
    enum ValueType val_type;
};
struct SliceObj {
    struct Obj obj;
    struct ArrayObj *array;
    u32 len;
    u32 start;
};

struct RuntimeType {
    struct Obj obj;
    u16 type_index;
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
/// the thunk will expect the address of its free variables in `r1`
struct Thunk {
    struct Obj obj;
    struct ClosureInfo *info;
    /// updated once thunk has been evaluated
    Val evaluated;
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

struct TypeInfo {
    char *name;
    u16 name_len;
};

struct Object {
    struct Obj obj;
    u16 type_info;
    u16 variant;
    u16 arg_count;
    /// struct Obj *arguments[]
};

struct Box *obj_create_box();
struct FileHandleObj *obj_create_file_handle();
struct ArrayObj *obj_create_array(u32 len, enum ValueType type, u8 *ptr);
struct SliceObj *obj_create_slice(struct ArrayObj *array, u32 start, u32 len);
struct Cons *obj_create_cons();
struct Application *obj_create_application(u8 arg_count);
struct Closure *obj_create_closure(struct ClosureInfo *info);
struct Thunk *obj_create_thunk(struct ClosureInfo *info);
struct Object *obj_create_object(u16 type_info, u16 variant, u16 arg_count);
struct RuntimeType *obj_create_runtime_type(u16 type_info);

void obj_init_box(struct Box *box);
void obj_init_cons(struct Cons *cons);
void obj_init_array(struct ArrayObj *slice, u32 len, enum ValueType type, u8 *ptr);
void obj_init_slice(struct SliceObj *slice, struct ArrayObj *array, u32 start, u32 len);
void obj_init_object(struct Object *object, u16 type_info, u16 variant, u16 arg_count);

struct Box **obj_dyn_fields(struct Obj *obj);

struct Obj **get_most_recent_alloc();
void try_gc();
void free_objects();
void free_obj(struct Obj *obj);

#endif
