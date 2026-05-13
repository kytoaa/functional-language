#include "slice_functions.h"
#include "../../object.h"
#include "../utils.h"

void extern_func_slice_drop()
{
    Val index_val = pop_val();
    if (index_val->type != OBJ_BOX) {
        return runtime_error("not an integer");
    }
    struct Box *index = (struct Box*)index_val;
    if (index->val.type != VALUE_INT) {
        return runtime_error("not an integer");
    }
    if (index->val.as.integer == 0) {
        return;
    }

    Val val = pop_val();
    switch (val->type) {
        case OBJ_SLICE:{
            struct SliceObj *slice = (struct SliceObj*)val;
            struct SliceObj *new_slice = obj_create_slice(slice->array, slice->start, 0);
            if (slice->len <= index->val.as.integer) {
                new_slice->len = 0;
                new_slice->start += index->val.as.integer;
            } else {
                new_slice->len = slice->len - index->val.as.integer;
            }
            push_val(as_val(new_slice));
            break;
        }
        case OBJ_ARRAY:{
            struct ArrayObj *array = (struct ArrayObj*)val;
            struct SliceObj *new_slice = obj_create_slice(array, 0, 0);
            if (array->len <= index->val.as.integer) {
                new_slice->len = 0;
                new_slice->start = index->val.as.integer;
            } else {
                new_slice->len = array->len - index->val.as.integer;
                new_slice->start = index->val.as.integer;
            }
            printf("pushed a %d with len %d, start %d\n", new_slice->obj.type, new_slice->len, new_slice->start);
            push_val(as_val(new_slice));
            break;
        }
        default:
            return runtime_error("not an array");
    }
}

void extern_func_slice_take()
{
    Val index_val = pop_val();
    if (index_val->type != OBJ_BOX) {
        return runtime_error("not an integer");
    }
    struct Box *index = (struct Box*)index_val;
    if (index->val.type != VALUE_INT) {
        return runtime_error("not an integer");
    }
    if (index->val.as.integer == 0) {
        return;
    }

    Val val = pop_val();
    switch (val->type) {
        case OBJ_SLICE:{
            struct SliceObj *slice = (struct SliceObj*)val;
            struct SliceObj *new_slice = obj_create_slice(slice->array, slice->start, index->val.as.integer);
            if (slice->len <= index->val.as.integer) {
                new_slice->len = slice->len;
            }
            push_val(as_val(new_slice));
            break;
        }
        case OBJ_ARRAY:{
            struct ArrayObj *array = (struct ArrayObj*)val;
            struct SliceObj *new_slice = obj_create_slice(array, 0, index->val.as.integer);
            if (array->len <= index->val.as.integer) {
                new_slice->len = array->len;
            }
            push_val(as_val(new_slice));
            break;
        }
        default:
            return runtime_error("not an array");
    }
}

void extern_func_slice_index()
{
    Val val = pop_val();
    if (val->type != OBJ_SLICE) {
        return runtime_error("not an array");
    }
    struct ArrayObj *array = (struct ArrayObj*)val;
    Val index_val = pop_val();
    if (index_val->type != OBJ_BOX) {
        return runtime_error("not an integer");
    }
    struct Box *index = (struct Box*)index_val;
    if (index->val.type != VALUE_INT) {
        return runtime_error("not an integer");
    }

    if (array->len <= index->val.as.integer)
        return runtime_error("index out of bounds");

    switch (array->val_type) {
        case VALUE_INT:{
            i32 value = ((i32*)array->ptr)[index->val.as.integer];
            struct Box *result = obj_create_box();
            result->val = INT_VAL(value);
            push_val(as_val(result));
            break;
        }
        case VALUE_CHAR:{
            char value = ((char*)array->ptr)[index->val.as.integer];
            struct Box *result = obj_create_box();
            result->val = CHAR_VAL(value);
            push_val(as_val(result));
            break;
        }
        case VALUE_BOOL:{
            bool value = ((bool*)array->ptr)[index->val.as.integer];
            struct Box *result = obj_create_box();
            result->val = BOOL_VAL(value);
            push_val(as_val(result));
            break;
        }
        default:
            panic("unreachable: not a valid array");
    }
}

void extern_func_slice_len()
{
    Val val = pop_val();
    i32 len = 0;
    switch (val->type) {
        case OBJ_SLICE:
            len = ((struct SliceObj*)val)->len;
            break;
        case OBJ_ARRAY:
            len = ((struct ArrayObj*)val)->len;
            break;
        default:
            printf("got a %d\n", val->type);
            return runtime_error("len: not an array");
    }
    struct Box *result = obj_create_box();
    result->val = INT_VAL(len);
    push_val(as_val(result));
}

