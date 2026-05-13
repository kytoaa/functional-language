#include "standard_library.h"

const char STD_LIB_SRC[] = "\n\
mod = {\n\
    mod option = {\n\
        Some x = true :: x;\n\
        None x = false :: ();\n\
\n\
        map f x = case x of\n\
            | true :: x -> Some (f x)\n\
            | false :: _ -> None;\n\
\n\
        join x = case x of\n\
            | true :: (true :: x) -> Some x\n\
            | false :: ()           -> None\n\
            | false :: (false::_) -> None;\n\
\n\
        bind f x = join (map f x);\n\
\n\
        unwrap_or a x = case x of\n\
            | true :: x   -> x\n\
            | false :: () -> a;\n\
    };\n\
\n\
    mod list = {\n\
        map f l = case l of\n\
            | ()       -> ()\n\
            | x :: xs -> (f x) :: (map f xs);\n\
\n\
        append a b = case a of\n\
            | () -> b\n\
            | x :: xs -> x :: (append xs b);\n\
\n\
        filter p l = case l of\n\
            | ()      -> ()\n\
            | x :: xs -> if p x then x :: filter p xs\n\
                                else filter p xs;\n\
\n\
        foldr f a l = case l of\n\
            | x :: xs   -> f x (foldr f a xs)\n\
            | otherwise -> a;\n\
\n\
        join = foldr append ();\n\
\n\
        bind f l = join (map f l);\n\
    };\n\
\n\
    fst p = case p of\n\
        | a :: _ -> a;\n\
\n\
    snd p = case p of\n\
        | _ :: a -> a;\n\
    \n\
    const a = fun b -> a;\n\
\n\
    curry f a b = f (a :: b);\n\
    uncurry f pair = case pair of\n\
        | a :: b -> f a b;\n\
\n\
    mod slice = {\n\
        index n s = @std_builtin(slice_index);\n\
        len s = @std_builtin(slice_len);\n\
        drop n s = @std_builtin(slice_drop);\n\
        take n s = @std_builtin(slice_take);\n\
    };\n\
    mod io = {\n\
        stdin = @std_builtin(stdin);\n\
        stdout = @std_builtin(stdout);\n\
        stderr = @std_builtin(stderr);\n\
\n\
        seq a b = case $ a of | _ -> b;\n\
\n\
        read_file_contents file = @std_builtin(read_file_contents);\n\
        read_line file = @std_builtin(read_file_line);\n\
\n\
        write file val = @std_builtin(write);\n\
        writeln file val = seq (write file val) (write file '\n');\n\
        print = write stdout;\n\
        println = writeln stdout;\n\
    };\n\
};";

const char *std_lib_src()
{
    return STD_LIB_SRC;
}
u32 std_lib_src_len()
{
    return sizeof(STD_LIB_SRC);
}

