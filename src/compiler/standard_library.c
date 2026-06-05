#include "standard_library.h"

const char STD_LIB_SRC[] = "\n\
mod = {\n\
    mod Option = type {\n\
        with Some a;\n\
        with None;\n\
\n\
        map f x = case x of\n\
            | Some x -> Some (f x)\n\
            | None -> None;\n\
\n\
        join x = case x of\n\
            | Some (Some x) -> Some x\n\
            | Some None     -> None\n\
            | None          -> None;\n\
\n\
        bind f x = join (map f x);\n\
\n\
        unwrap_or a x = case x of\n\
            | Some x -> x\n\
            | None   -> a;\n\
    };\n\
    with Some = Option..Some;\n\
    with None = Option..None;\n\
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
    type_of x = @std_builtin(type_of);\n\
\n\
    run = _io..run;\n\
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
        empty = @std_builtin(slice_empty);\n\
\n\
        index n s = @std_builtin(slice_index);\n\
        len s = @std_builtin(slice_len);\n\
        drop n s = @std_builtin(slice_drop);\n\
        take n s = @std_builtin(slice_take);\n\
        join a b = @std_builtin(slice_join);\n\
\n\
        cons c s = @std_builtin(slice_cons);\n\
        push s c = @std_builtin(slice_push);\n\
        unpack s = if len s == 0\n\
                      then ()\n\
                      else (index 0 s) :: unpack (drop 1 s);\n\
    };\n\
\n\
    mod io = _io..io;\n\
};\n\
\n\
mod _io = {\n\
    mod IO = type {\n\
        with New a;\n\
    };\n\
\n\
    stdin = @std_builtin(stdin);\n\
    stdout = @std_builtin(stdout);\n\
    stderr = @std_builtin(stderr);\n\
\n\
    seq a b = case $ a of | _ -> b;\n\
\n\
    exit = @std_builtin(exit);\n\
\n\
    run x = case x of\n\
        | IO..New x -> run x\n\
        | x -> seq x exit;\n\
\n\
    read_file_contents file = @std_builtin(read_file_contents);\n\
    read_line file = @std_builtin(read_file_line);\n\
\n\
    write file val = @std_builtin(write);\n\
    writeln file val = seq (write file val) (write file '\n');\n\
    print = write stdout;\n\
    println = writeln stdout;\n\
\n\
    mod io = {\n\
        stdin = super..stdin;\n\
        stdout = super..stdout;\n\
        stderr = super..stderr;\n\
\n\
        seq = super..seq;\n\
        read_file_contents file = let\n\
            result = super..read_file_contents file;\n\
            in super..IO..New result;\n\
        read_line file = let\n\
            result = super..read_line file;\n\
            in super..IO..New result;\n\
\n\
        write file val = let\n\
            result = super..write file val;\n\
            in super..IO..New result;\n\
        writeln file val = let\n\
            result = super..writeln file val;\n\
            in super..IO..New result;\n\
        print val = let\n\
            result = super..print val;\n\
            in super..IO..New result;\n\
        println val = let\n\
            result = super..println val;\n\
            in super..IO..New result;\n\
\n\
        map f a = case a of\n\
            | super..IO..New a -> super..IO..New (f a);\n\
        join a = case a of\n\
            | super..IO..New (super..IO..New a) -> super..IO..New a;\n\
        return = super..IO..New;\n\
    };\n\
};\n\
\n\
";

const char *std_lib_src()
{
    return STD_LIB_SRC;
}
u32 std_lib_src_len()
{
    return sizeof(STD_LIB_SRC);
}

