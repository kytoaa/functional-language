#include "gc.h"
#include "../object.h"
#include "../prelude.h"
#include "vm.h"

#ifdef LOG_GC
#include <stdio.h>
#endif


struct Worklist {
    u32 len;
    u32 cap;
    struct Obj **objects;
};

static struct Worklist worklist = {};

static void worklist_push(struct Obj *obj)
{
    if (worklist.len == worklist.cap) {
        u32 new_cap = (worklist.cap == 0) ? 16 : worklist.cap * 2;
        struct Obj **new_ptr = realloc_mem(worklist.objects, new_cap * sizeof(struct Obj*));
        worklist.cap = new_cap;
        worklist.objects = new_ptr;
    }
    worklist.objects[worklist.len++] = obj;
}
static struct Obj *worklist_pop()
{
    if (worklist.len > 0) {
        return worklist.objects[--worklist.len];
    }
    return null;
}

static void mark_obj(struct Obj *obj)
{
    if (obj == null)
        return;
    if (obj->flags.gc_marked)
        return;

    obj->flags.gc_marked = true;

    worklist_push(obj);
}

static void process_box(struct Box *box)
{
    return;
}
static void process_file_handle(struct FileHandleObj *file_handle)
{
    return;
}
static void process_runtime_type(struct RuntimeType *runtime_type)
{
    return;
}
static void process_object(struct Object *object)
{
    struct Box **dyn_fields = obj_dyn_fields(TO_OBJ(object));

    for (u32 i = 0; i < object->arg_count; i++) {
        mark_obj(TO_OBJ(dyn_fields[i]));
    }
    return;
}
static void process_slice_obj(struct SliceObj *slice)
{
    mark_obj(TO_OBJ(slice->array));
    return;
}
static void process_array_obj(struct ArrayObj *array)
{
    return;
}
static void process_cons(struct Cons *cons)
{
    mark_obj(cons->l);
    mark_obj(cons->r);
}
static void process_closure(struct Closure *closure)
{
    struct Box **dyn_fields = obj_dyn_fields(TO_OBJ(closure));

    for (u32 i = 0; i < closure->info->capture_count; i++) {
        mark_obj(TO_OBJ(dyn_fields[i]));
    }
}
static void process_thunk(struct Thunk *thunk)
{
    struct Box **dyn_fields = obj_dyn_fields(TO_OBJ(thunk));
    mark_obj(thunk->evaluated);

    for (u32 i = 0; i < thunk->info->capture_count; i++) {
        mark_obj(TO_OBJ(dyn_fields[i]));
    }
}
static void process_application(struct Application *application)
{
    mark_obj(application->closure);

    struct Box **args = obj_dyn_fields(TO_OBJ(application));
    
    for (u32 i = 0; i < application->arg_count; i++) {
        mark_obj(TO_OBJ(args[i]));
    }
}

static void process_obj(struct Obj *obj)
{
    if (obj == null)
        return;

    switch (obj->type) {
        case OBJ_BOX:
            process_box((struct Box*)obj);
            break;
        case OBJ_CONS:
            process_cons((struct Cons*)obj);
            break;
        case OBJ_OBJECT:
            process_object((struct Object*)obj);
            break;
        case OBJ_CLOSURE:
            process_closure((struct Closure*)obj);
            break;
        case OBJ_THUNK:
            process_thunk((struct Thunk*)obj);
            break;
        case OBJ_APPLICATION:
            process_application((struct Application*)obj);
            break;
        case OBJ_FILE_HANDLE:
            process_file_handle((struct FileHandleObj*)obj);
            break;
        case OBJ_SLICE:
            process_slice_obj((struct SliceObj*)obj);
            break;
        case OBJ_ARRAY:
            process_array_obj((struct ArrayObj*)obj);
            break;
        case OBJ_RUNTIME_TYPE:
            process_runtime_type((struct RuntimeType*)obj);
            break;

        default:
            return panic("not an object");
    }
}

static inline void mark_vm_val(u64 val)
{
    // is val a ptr?
    if ((val & 0x8000000000000000) == 0)
        return;

    mark_obj(val_ptr(val));
}

static void mark_roots(struct VM *vm)
{
    for (u32 i = 0; i < vm->registers[STACK_PTR]; i++) {
        mark_vm_val(vm->stack[i]);
    }
    for (u32 i = 0; i < vm->registers[BINDING_PTR]; i++) {
        mark_vm_val(vm->bindings[i]);
    }
    for (u32 i = 0; i < vm->static_thunks.len; i++) {
        mark_obj(vm->static_thunks.ptr[i]->evaluated);
    }
    mark_vm_val(vm->registers[REG_1]);
}

static void trace_objects()
{
    while (worklist.len > 0) {
        struct Obj *obj = worklist_pop();
        process_obj(obj);
    }
}

static void free_obj(struct Obj *object)
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

static void sweep()
{
    struct Obj *prev = null;
    struct Obj *current = *get_most_recent_alloc();

    #ifdef LOG_GC
    u32 processed_objects = 0;
    u32 freed_objects = 0;
    #endif

    while (current != null) {
        if (current->flags.gc_marked) {
            current->flags.gc_marked = false;
            prev = current;
            current = current->next;
        } else {
            struct Obj *unreferenced = current;
            current = unreferenced->next;

            if (prev != null) {
                prev->next = current;
            } else {
                *get_most_recent_alloc() = current;
            }

            free_obj(unreferenced);
            #ifdef LOG_GC
                freed_objects += 1;
            #endif
        }
        #ifdef LOG_GC
            processed_objects += 1;
        #endif
    }
    #ifdef LOG_GC
        printf("gc done, processed %d, freed %d\n", processed_objects, freed_objects);
    #endif
}

void run_gc()
{
    mark_roots(&vm);
    trace_objects();
    sweep();
}

void end_gc()
{
    free_mem(worklist.objects);
    worklist = (struct Worklist){};
}

