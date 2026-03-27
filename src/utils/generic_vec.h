#ifndef func_lang_utils_generic_vec_h
#define func_lang_utils_generic_vec_h

#include "../prelude.h"

#define DEFINE_Vec(T)\
struct Vec_ ## T {\
	T *ptr;\
	usize len;\
	usize cap;\
};

#define DEFINE_ENUM_Vec(T)\
struct Vec_ ## T {\
	enum T *ptr;\
	usize len;\
	usize cap;\
};

#define DEFINE_STRUCT_Vec(T)\
struct Vec_ ## T {\
	struct T *ptr;\
	usize len;\
	usize cap;\
};

/// Vec_push(*Vec T, T)
#define Vec_push(vec, val) do {\
	typeof(vec) v = (vec);\
    if (v->len == v->cap) {\
        usize new_cap = v->cap == 0 ? (sizeof(v->ptr[0]) * 4) : (v->cap * 2);\
		void *new_ptr = reallocate(v->ptr, new_cap);\
		v->ptr = new_ptr;\
		v->cap = new_cap;\
    }\
    v->ptr[v->len++] = (val);\
} while (0)

/// Vec_get(ref Vec T, usize, out var T)
#define Vec_get(vec, index, out) do {\
	typeof(vec) v = (vec);\
	if (v.len >= (index)) {\
		(out) = null;\
	}\
	(out) = v.ptr + (index);\
} while (0)

#endif
