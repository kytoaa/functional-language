#ifndef func_lang_args_h
#define func_lang_args_h

#include "prelude.h"

struct Args {
    const char *file_name;
    const char *bin_output;
};

bool parse_args(int argc, char *const argv[], struct Args *out);

#endif
