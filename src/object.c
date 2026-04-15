#include "object.h"
#include "prelude.h"

static struct Obj *most_recent_alloc = null;

static struct Obj *alloc_obj(u32 size)
{
    struct Obj *new_obj = alloc_mem(size);
    new_obj->next = most_recent_alloc;
    most_recent_alloc = new_obj;

    new_obj->flags.is_whnf = false;
    new_obj->flags.is_static = false;

    return new_obj;
}

struct Application *obj_create_application(u8 arg_count)
{
    const u32 size = sizeof(struct Application) + sizeof(void*) * arg_count;
    struct Application *appl = (struct Application*)alloc_obj(size);

    appl->obj.type = OBJ_APPLICATION;
    appl->obj.flags.is_whnf = false;
    appl->arg_count = arg_count;
    appl->arity = 0;

    return appl;
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

struct Closure *obj_create_closure(struct ClosureInfo *info)
{
    const u32 size = sizeof(struct Closure) + (info->capture_count * sizeof(struct Object*));
    struct Closure *closure = (struct Closure*)alloc_obj(size);

    closure->obj.type = OBJ_CLOSURE;
    closure->obj.flags.is_whnf = true;
    closure->info = info;

    return closure;
}
struct Thunk *obj_create_thunk(struct ClosureInfo *info)
{
    const u32 size = sizeof(struct Thunk) + (info->capture_count * sizeof(struct Object*));
    struct Thunk *thunk = (struct Thunk*)alloc_obj(size);

    thunk->obj.type = OBJ_THUNK;
    thunk->obj.flags.is_whnf = false;
    thunk->info = info;
    thunk->evaluated = null;

    return thunk;
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
        default:
#ifdef DEBUG_CHECKS
            panic("object does not have dynamic fields");
#endif
            return null;
    }
}
