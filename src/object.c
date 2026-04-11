#include "object.h"
#include "prelude.h"

static struct Obj *most_recent_alloc = null;

static struct Obj *alloc_obj(u32 size)
{
    struct Obj *new_obj = alloc_mem(size);
    new_obj->next = most_recent_alloc;
    most_recent_alloc = new_obj;

    return new_obj;
}

struct Application *obj_create_application(u8 arg_count)
{
    const u32 size = sizeof(struct Application) + sizeof(void*) * arg_count;
    struct Application *appl = (struct Application*)alloc_obj(size);

    appl->obj.type = OBJ_APPLICATION;
    appl->arg_count = arg_count;

    return appl;
}
struct Box *obj_create_box()
{
    const u32 size = sizeof(struct Box);
    struct Box *box = (struct Box*)alloc_obj(size);

    box->obj.type = OBJ_BOX;
    box->obj.flags.is_whnf = true;

    return box;
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
