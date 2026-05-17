#include "extern_functions.h"
#include "../utils.h"
#include "slice_functions.h"
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
            if (val->type != OBJ_FILE_HANDLE) {
                return runtime_error("not a file handle");
            }
            struct FileHandleObj *file = (struct FileHandleObj*)val;
            Val print_arg = pop_val();
            print_val(file->file, print_arg);
            break;
        }
        case VM_EXTERN_FUNC_WRITE_C_STRING:{
            Val val = pop_val();
            if (val->type != OBJ_FILE_HANDLE) {
                return runtime_error("not a file handle");
            }
            struct FileHandleObj *file = (struct FileHandleObj*)val;
            Val print_arg = pop_val();
            fprintf(file->file, "%s", (char*)pop_stack());
            break;
        }
        case VM_EXTERN_FUNC_READ_CONTENTS:{
            Val val = pop_val();
            if (val->type != OBJ_FILE_HANDLE) {
                return runtime_error("not a file handle");
            }
            struct FileHandleObj *file = (struct FileHandleObj*)val;
            fseek(file->file, 0, SEEK_END);
            usize file_size = ftell(file->file);
            rewind(file->file);

            struct ArrayObj *array = obj_create_array(file_size, VALUE_CHAR, null);
            fread(array->ptr, sizeof(char), file_size, file->file);

            push_val(as_val(array));
            break;
        }
        case VM_EXTERN_FUNC_READ_LINE:{
            Val val = pop_val();
            if (val->type != OBJ_FILE_HANDLE) {
                return runtime_error("not a file handle");
            }
            struct FileHandleObj *file = (struct FileHandleObj*)val;

            struct ArrayObj *array = obj_create_array(0, VALUE_CHAR, null);
            usize _cap = 0;
            array->len = getline((char**)&array->ptr, &_cap, file->file);

            push_val(as_val(array));
            break;
        }
        case VM_EXTERN_FUNC_STDIN:
        case VM_EXTERN_FUNC_STDOUT:
        case VM_EXTERN_FUNC_STDERR:{
            struct FileHandleObj *file_handle = get_std_stream(function);
            push_val(as_val(file_handle));
            break;
        }
        case VM_EXTERN_FUNC_SLICE_EMPTY:{
            extern_func_slice_empty();
            break;
        }
        case VM_EXTERN_FUNC_SLICE_LEN:{
            extern_func_slice_len();
            break;
        }
        case VM_EXTERN_FUNC_READ_SLICE_INDEX:{
            extern_func_slice_index();
            break;
        }
        case VM_EXTERN_FUNC_SLICE_DROP:{
            extern_func_slice_drop();
            break;
        }
        case VM_EXTERN_FUNC_SLICE_TAKE:{
            extern_func_slice_take();
            break;
        }
        case VM_EXTERN_FUNC_SLICE_JOIN:{
            extern_func_slice_join();
            break;
        }
        case VM_EXTERN_FUNC_SLICE_CONS:{
            extern_func_slice_cons();
            break;
        }
        case VM_EXTERN_FUNC_SLICE_PUSH:{
            extern_func_slice_push();
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
static void print_array_elements(FILE *out, struct ArrayObj *val, u32 start, u32 count)
{
    if (val->val_type == VALUE_CHAR) {
        fprintf(out, "%.*s", count, val->ptr + start);
    } else {
        for (u32 i = start; i < start + count; i++) {
            fprintf(out, "%d", (val->val_type == VALUE_BOOL) ? ((u32*)val->ptr)[i] : val->ptr[i]);
        }
    }
}
static void print_array(FILE *out, struct ArrayObj *val)
{
    print_array_elements(out, val, 0, val->len);
}
static void print_slice(FILE *out, struct SliceObj *slice)
{
    print_array_elements(out, slice->array, slice->start, slice->len);
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
        case OBJ_ARRAY:
            print_array(out, (struct ArrayObj*)val);
            break;
        case OBJ_SLICE:
            print_slice(out, (struct SliceObj*)val);
            break;
        default:
            fprintf(out, "%d\n", val->type);
            panic("not printable");
            break;
    }
}
