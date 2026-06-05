#include "object.h"
#include "prelude.h"
#include "vm/gc.h"
#include <string.h>

static struct Obj *most_recent_alloc = null;

struct Obj **get_most_recent_alloc()
{
    return &most_recent_alloc;
}

static u64 allocated_memory = 0;
static u64 heap_size = 1024;

void try_gc()
{
    if (allocated_memory > heap_size) {
        run_gc();
        heap_size *= 2;
        allocated_memory = 0;
    }
}
void free_objects()
{
    while (most_recent_alloc != null) {
        struct Obj *object = most_recent_alloc;
        most_recent_alloc = object->next;
        free_obj(object);
    }
    end_gc();
}

void free_obj(struct Obj *object)
{
    switch (object->type) {
        case OBJ_ARRAY:{
            struct ArrayObj *array = (struct ArrayObj*)object;
            free_mem(array->ptr);
            break;
        }
        default:
            break;
    }
    free_mem(object);
}


static struct Obj *alloc_obj(u32 size)
{
    struct Obj *new_obj = alloc_mem(size);
    for (u32 i = 0; i < size; i++) {
        ((u8*)new_obj)[i] = 0;
    }

    new_obj->next = most_recent_alloc;
    most_recent_alloc = new_obj;

    new_obj->flags.is_whnf = false;
    new_obj->flags.is_static = false;

    allocated_memory += size;

    return new_obj;
}

struct Box *obj_create_box()
{
    const u32 size = sizeof(struct Box);
    struct Box *box = (struct Box*)alloc_obj(size);

    obj_init_box(box);

    return box;
}
void obj_init_box(struct Box *box)
{
    box->obj.type = OBJ_BOX;
    box->obj.flags.is_whnf = true;
}

struct ArrayObj *obj_create_array(u32 len, enum ValueType type, u8 *ptr)
{
    const u32 size = sizeof(struct ArrayObj);
    struct ArrayObj *array = (struct ArrayObj*)alloc_obj(size);

    obj_init_array(array, len, type, ptr);

    return array;
}
void obj_init_array(struct ArrayObj *array, u32 len, enum ValueType type, u8 *ptr)
{
    array->obj.type = OBJ_ARRAY;
    array->obj.flags.is_whnf = true;

    u32 val_size = 0;
    switch (type) {
        case VALUE_INT:
            val_size = sizeof(i32);
            break;
        case VALUE_CHAR:
            val_size = sizeof(char);
            break;
        case VALUE_BOOL:
            val_size = sizeof(bool);
            break;
        default:
            panic("not valid for a byte array");
            break;
    }

    u32 cap = val_size * len;
    u8 *memory = null;
    if (cap > 0)
        memory = alloc_mem(cap);

    if (ptr != null)
        memcpy(memory, ptr, cap);

    array->ptr = memory;
    array->len = len;
    array->val_type = type;
}

struct SliceObj *obj_create_slice(struct ArrayObj *array, u32 start, u32 len)
{
    const u32 size = sizeof(struct SliceObj);
    struct SliceObj *slice = (struct SliceObj*)alloc_obj(size);

    obj_init_slice(slice, array, start, len);

    return slice;
}
void obj_init_slice(struct SliceObj *slice, struct ArrayObj *array, u32 start, u32 len)
{
    slice->obj.type = OBJ_SLICE;
    slice->obj.flags.is_whnf = true;

    slice->array = array;
    slice->start = start;
    slice->len = len;
}

struct Cons *obj_create_cons()
{
    const u32 size = sizeof(struct Cons);
    struct Cons *cons = (struct Cons*)alloc_obj(size);

    obj_init_cons(cons);

    return cons;
}
void obj_init_cons(struct Cons *cons)
{
    cons->obj.type = OBJ_CONS;
    cons->obj.flags.is_whnf = true;
    cons->l = null;
    cons->r = null;
}

struct Application *obj_create_application(u8 arg_count)
{
    const u32 size = sizeof(struct Application) + sizeof(void*) * arg_count;
    struct Application *appl = (struct Application*)alloc_obj(size);

    for (u32 i = 0; i < arg_count; i++) {
        ((u64*)(appl + 1))[i] = 0;
    }

    appl->obj.type = OBJ_APPLICATION;
    appl->obj.flags.is_whnf = false;
    appl->arg_count = arg_count;
    appl->arity = 0;

    return appl;
}

struct Closure *obj_create_closure(struct ClosureInfo *info)
{
    const u32 size = sizeof(struct Closure) + (info->capture_count * sizeof(struct Obj*));
    struct Closure *closure = (struct Closure*)alloc_obj(size);

    for (u32 i = 0; i < info->capture_count; i++) {
        ((u64*)(closure + 1))[i] = 0;
    }

    closure->obj.type = OBJ_CLOSURE;
    closure->obj.flags.is_whnf = true;
    closure->info = info;

    return closure;
}
struct Thunk *obj_create_thunk(struct ClosureInfo *info)
{
    const u32 size = sizeof(struct Thunk) + (info->capture_count * sizeof(struct Obj*));
    struct Thunk *thunk = (struct Thunk*)alloc_obj(size);

    for (u32 i = 0; i < info->capture_count; i++) {
        ((u64*)(thunk + 1))[i] = 0;
    }

    thunk->obj.type = OBJ_THUNK;
    thunk->obj.flags.is_whnf = false;
    thunk->info = info;
    thunk->evaluated = null;

    return thunk;
}

struct Object *obj_create_object(u16 type_info, u16 variant, u16 arg_count)
{
    const u32 size = sizeof(struct Object) + arg_count * sizeof(struct Obj*);
    struct Object *object = (struct Object*)alloc_obj(size);

    obj_init_object(object, type_info, variant, arg_count);

    return object;
}
void obj_init_object(struct Object *object, u16 type_info, u16 variant, u16 arg_count)
{
    object->obj.type = OBJ_OBJECT;
    object->obj.flags.is_whnf = true;
    object->type_info = type_info;
    object->variant = variant;
    object->arg_count = arg_count;
}

struct RuntimeType *obj_create_runtime_type(u16 type_info)
{
    const u32 size = sizeof(struct RuntimeType);
    struct RuntimeType *type = (struct RuntimeType*)alloc_obj(size);

    type->obj.type = OBJ_RUNTIME_TYPE;
    type->obj.flags.is_whnf = true;
    type->type_index = type_info;

    return type;
}

struct Box **obj_dyn_fields(struct Obj *obj)
{
    switch (obj->type) {
        case OBJ_CLOSURE:
            return (struct Box**)(((struct Closure*)obj) + 1);
        case OBJ_THUNK:
            return (struct Box**)(((struct Thunk*)obj) + 1);
        case OBJ_APPLICATION:
            return (struct Box**)(((struct Application*)obj) + 1);
        case OBJ_OBJECT:
            return (struct Box**)(((struct Object*)obj) + 1);
        default:
#ifdef DEBUG_CHECKS
            panic("object does not have dynamic fields");
#endif
            return null;
    }
}
