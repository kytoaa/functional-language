#include "slice_functions.h"
#include "../../object.h"
#include "../utils.h"
#include <string.h>

static usize val_type_size(enum ValueType type)
{
    switch (type) {
        case VALUE_CHAR:
            return sizeof(char);
        case VALUE_BOOL:
            return sizeof(bool);
        case VALUE_INT:
            return sizeof(i32);
        case VALUE_UNIT:
            return 0;
    }
}

void extern_func_slice_empty()
{
    struct ArrayObj *array = obj_create_array(0, VALUE_INT, null);
    push_val(as_val(array));
}

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
    Val index_val = pop_val();
    if (index_val->type != OBJ_BOX) {
        return runtime_error("not an integer");
    }
    struct Box *index = (struct Box*)index_val;
    if (index->val.type != VALUE_INT) {
        return runtime_error("not an integer");
    }

    Val val = pop_val();

    struct ArrayObj *array = null;
    u32 start_index = 0;
    switch (val->type) {
        case OBJ_SLICE:{
            struct SliceObj *slice = (struct SliceObj*)val;
            if (slice->len <= index->val.as.integer) {
                return runtime_error("index out of bounds");
            }
            array = slice->array;
            start_index = slice->start;
            break;
        }
        case OBJ_ARRAY:{
            array = (struct ArrayObj*)val;
            break;
        }
        default:
            return runtime_error("not an array");
    }

    const u32 read_index = start_index + index->val.as.integer;
    if (read_index >= array->len)
        return runtime_error("index out of bounds");

    struct Box *result = obj_create_box();

    switch (array->val_type) {
        case VALUE_INT:{
            i32 value = ((i32*)array->ptr)[read_index];
            result->val = INT_VAL(value);
            break;
        }
        case VALUE_CHAR:{
            char value = ((char*)array->ptr)[read_index];
            result->val = CHAR_VAL(value);
            break;
        }
        case VALUE_BOOL:{
            bool value = ((bool*)array->ptr)[read_index];
            result->val = BOOL_VAL(value);
            break;
        }
        default:
            panic("unreachable: not a valid array");
    }

    push_val(as_val(result));
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

struct SliceInfo {
    struct ArrayObj *array;
    u32 start;
    u32 len;
};
static struct SliceInfo get_slice_info(Val val)
{
    struct SliceInfo slice = {};
    switch (val->type) {
        case OBJ_ARRAY:{
            struct ArrayObj *array = (struct ArrayObj*)val;
            return (struct SliceInfo){
                .array = array,
                .len = array->len,
                .start = 0,
            };
        }
        case OBJ_SLICE:{
            struct SliceObj *slice_obj = (struct SliceObj*)val;
            return (struct SliceInfo){
                .array = slice_obj->array,
                .len = slice_obj->len,
                .start = 0,
            };
        }
        default:
            return (struct SliceInfo){ .array = null };
    }
}

void extern_func_slice_join()
{
    Val l_val = pop_val();
    struct SliceInfo l_slice = get_slice_info(l_val);
    if (l_slice.array == null)
        return runtime_error("slice join l is not a slice");

    Val r_val = pop_val();
    struct SliceInfo r_slice = get_slice_info(r_val);
    if (r_slice.array == null)
        return runtime_error("slice join r is not a slice");

    if (l_slice.len == 0)
        return push_val(as_val(r_val));
    if (r_slice.len == 0)
        return push_val(as_val(l_val));

    if (l_slice.array->val_type != r_slice.array->val_type)
        return runtime_error("incompatible slice types");

    struct ArrayObj *new_array = obj_create_array(l_slice.len + r_slice.len, l_slice.array->val_type, null);
    u32 size = val_type_size(l_slice.array->val_type);

    if (size == 0)
        return runtime_error("not a valid slice");

    memcpy(new_array->ptr, l_slice.array->ptr + l_slice.start * size, l_slice.len * size);
    memcpy(new_array->ptr + l_slice.len * size, r_slice.array->ptr + r_slice.start * size, r_slice.len * size);

    push_val(as_val(new_array));
}

void extern_func_slice_cons()
{
    Val push_value = pop_val();
    if (push_value->type != OBJ_BOX) {
        return runtime_error("cannot push to slice");
    }
    struct Box *push = (struct Box*)push_value;

    Val slice_val = pop_val();

    struct SliceInfo slice = get_slice_info(slice_val);
    if (slice.array == null)
        return runtime_error("slice cons is not a slice");

    if (slice.len > 0 && slice.array->val_type != push->val.type)
        return runtime_error("incompatible types");

    struct ArrayObj *new_array = obj_create_array(slice.len + 1, push->val.type, null);

    switch (push->val.type) {
        case VALUE_INT:
            *(i32*)new_array->ptr = push->val.as.integer;
            break;
        case VALUE_BOOL:
            *(bool*)new_array->ptr = push->val.as.boolean;
            break;
        case VALUE_CHAR:
            *(char*)new_array->ptr = push->val.as.character;
            break;
        default:
            return runtime_error("not a valid slice");
    }

    const u32 val_size = val_type_size(push->val.type);
    memcpy(new_array->ptr + 1, slice.array->ptr + slice.start * val_size, slice.len * val_size);

    push_val(as_val(new_array));
}

void extern_func_slice_push()
{
    Val slice_val = pop_val();

    Val push_value = pop_val();
    if (push_value->type != OBJ_BOX) {
        return runtime_error("cannot push to slice");
    }
    struct Box *push = (struct Box*)push_value;

    struct SliceInfo slice = get_slice_info(slice_val);
    if (slice.array == null)
        return runtime_error("slice cons is not a slice");

    if (slice.len > 0 && slice.array->val_type != push->val.type)
        return runtime_error("incompatible types");

    struct ArrayObj *new_array = obj_create_array(slice.len + 1, push->val.type, null);

    switch (push->val.type) {
        case VALUE_INT:
            ((i32*)new_array->ptr)[slice.len] = push->val.as.integer;
            break;
        case VALUE_BOOL:
            ((bool*)new_array->ptr)[slice.len] = push->val.as.boolean;
            break;
        case VALUE_CHAR:
            ((char*)new_array->ptr)[slice.len] = push->val.as.character;
            break;
        default:
            return runtime_error("not a valid slice");
    }

    const u32 val_size = val_type_size(push->val.type);
    memcpy(new_array->ptr, slice.array->ptr + slice.start * val_size, slice.len * val_size);

    push_val(as_val(new_array));
}

