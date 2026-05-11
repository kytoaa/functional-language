#include "extern_functions.h"
#include "utils.h"
#include <stdio.h>

#define DEBUG_CHECKS

static void print_val(FILE *out, Val val);
static void print_cons(FILE *out, struct Cons *cons);

static struct FileHandleObj *get_std_stream(enum VmExternFunction function)
{
    static bool initialised;
    static struct FileHandleObj stdin_obj, stdout_obj, stderr_obj;

    if (!initialised) {
        stdin_obj = (struct FileHandleObj){
            .obj = {
                .type = OBJ_FILE_HANDLE,
                .flags = { .is_whnf = true, .is_static = true, .gc_marked = false },
            },
            .file = stdin,
        };
        stdout_obj = (struct FileHandleObj){
            .obj = {
                .type = OBJ_FILE_HANDLE,
                .flags = { .is_whnf = true, .is_static = true, .gc_marked = false },
            },
            .file = stdout,
        };
        stderr_obj = (struct FileHandleObj){
            .obj = {
                .type = OBJ_FILE_HANDLE,
                .flags = { .is_whnf = true, .is_static = true, .gc_marked = false },
            },
            .file = stderr,
        };
        initialised = true;
    }
    switch (function) {
        case VM_EXTERN_FUNC_STDIN:
            return &stdin_obj;
        case VM_EXTERN_FUNC_STDOUT:
            return &stdout_obj;
        case VM_EXTERN_FUNC_STDERR:
            return &stderr_obj;
        default:
            return null;
    }
}

void call_extern_function(enum VmExternFunction function)
{
    switch (function) {
        case VM_EXTERN_FUNC_WRITE:{
            Val val = pop_val();
            switch (val->type) {
                case OBJ_FILE_HANDLE:
                    break;
                case OBJ_THUNK:{
                    struct Thunk *thunk = (struct Thunk*)val;
                    val = thunk->evaluated;
                    if (val != null && val->type == OBJ_FILE_HANDLE)
                        break;
                }
                default:
                    printf("got a %d\n", ((struct Box*)val)->val.type);
                    return runtime_error("not a file handle");
            }
            struct FileHandleObj *file = (struct FileHandleObj*)val;
            Val print_arg = pop_val();
            print_val(file->file, print_arg);
            break;
        }
        case VM_EXTERN_FUNC_WRITE_C_STRING:{
            Val val = pop_val();
            switch (val->type) {
                case OBJ_FILE_HANDLE:
                    break;
                case OBJ_THUNK:{
                    struct Thunk *thunk = (struct Thunk*)val;
                    val = thunk->evaluated;
                    if (val != null && val->type == OBJ_FILE_HANDLE)
                        break;
                }
                default:
                    return runtime_error("not a file handle");
            }
            struct FileHandleObj *file = (struct FileHandleObj*)val;
            Val print_arg = pop_val();
            fprintf(file->file, "%s", (char*)pop_stack());
            break;
        }
        case VM_EXTERN_FUNC_STDIN:
        case VM_EXTERN_FUNC_STDOUT:
        case VM_EXTERN_FUNC_STDERR:{
            struct FileHandleObj *file_handle = get_std_stream(function);
            push_val(as_val(file_handle));
            break;
        }
        default:
            panic("not an extern function");
    }
}

static void print_value(FILE *out, struct Value val)
{
    switch (val.type) {
        case VALUE_UNIT:
            fprintf(out, "()");
            break;
        case VALUE_INT:
            fprintf(out, "%d", val.as.integer);
            break;
        case VALUE_BOOL:
            fprintf(out, val.as.boolean ? "true" : "false");
            break;
        case VALUE_CHAR:
            fprintf(out, "%c", val.as.character);
            break;
    }
}

static void print_cons(FILE *out, struct Cons *cons)
{
    fprintf(out, "(");
    print_val(out, cons->l);
    fprintf(out, " :: ");
    print_val(out, cons->r);
    fprintf(out, ")");
}

static void print_val(FILE *out, Val val)
{
    if (val == null)
        panic("null");

    switch (val->type) {
        case OBJ_BOX:
            print_value(out, ((struct Box*)val)->val);
            break;
        case OBJ_CONS:
            print_cons(out, (struct Cons*)val);
            break;
        case OBJ_APPLICATION:
            fprintf(out, "<app: %d>", ((struct Application*)val)->arg_count);
            break;
        case OBJ_THUNK:
            fprintf(out, "<thunk>");
            break;
        case OBJ_CLOSURE:
            fprintf(out, "<closure>");
            break;
        default:
            fprintf(out, "%d\n", val->type);
            panic("not printable");
            break;
    }
}
