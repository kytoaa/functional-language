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
\n\
        ap a b = case a :: b of\n\
            | (Some f) :: (Some x) -> Some (f x)\n\
            | otherwise            -> None;\n\
\n\
        mod functor = {\n\
            fmap = super..map;\n\
            `<$>` = fmap;\n\
            `<&>` x f = fmap f x;\n\
        };\n\
        mod applicative = {\n\
            mod functor = super..functor;\n\
            `<*>` a b = super..ap a b;\n\
            pure = super..Some;\n\
        };\n\
        mod monad = {\n\
            mod applicative = super..applicative;\n\
            `>>=` m f = super..bind f m;\n\
            return = super..Some;\n\
        };\n\
\n\
    };\n\
    with Some = Option..Some;\n\
    with None = Option..None;\n\
\n\
    mod Result = type {\n\
        with Err e;\n\
        with Ok a;\n\
\n\
        map f a = case a of\n\
            | Err e -> Err e\n\
            | Ok a  -> Ok (f a);\n\
\n\
        mod functor = {\n\
            fmap = super..map;\n\
        };\n\
        mod applicative = {\n\
            mod functor = super..functor;\n\
            pure = super..monad..return;\n\
            `<*>` a b = a >>= fun f ->\n\
                        b >>= fun x -> super..Ok (f x);\n\
        };\n\
        mod monad = {\n\
            mod applicative = super..applicative;\n\
            return = super..Ok;\n\
            `>>=` m f = case m of\n\
                | super..Err e -> super..Err e\n\
                | super..Ok a  -> f a;\n\
            `>>` a b = a >>= fun x -> b;\n\
        };\n\
    };\n\
\n\
    mod list = {\n\
        map f l = case l of\n\
            | ()      -> ()\n\
            | x :: xs -> (f x) :: (map f xs);\n\
\n\
        append a b = case a of\n\
            | ()      -> b\n\
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
\n\
        mod monad = {\n\
            `>>=` m f = super..bind f m;\n\
            return x = x :: ();\n\
        };\n\
    };\n\
\n\
    type_of x = @std_builtin(type_of);\n\
\n\
    compose f g = fun x -> f (g x);\n\
    flip f x y = f y x;\n\
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
		mod State = type {\n\
			with State;\n\
		};\n\
\n\
		with IO runState;\n\
\n\
		-- IO a -> State -> (a, State)\n\
		runState s = case s of\n\
			| IO runState -> runState;\n\
\n\
		return x = IO (fun s -> x :: s);\n\
\n\
		map f = bind (return . f);\n\
		bind f x = let\n\
			run s = case super..strict_pair (runState x s) of\n\
				| a :: s1 -> runState (f a) s1;\n\
			in IO run;\n\
\n\
		mod functor = {\n\
			fmap = super..map;\n\
		};\n\
		mod applicative = {\n\
			mod functor = super..functor;\n\
\n\
			pure = super..monad..return;\n\
			`<*>` a1 a2 = use super..monad { `>>=`; return }\n\
				in a1 >>= (fun f ->\n\
				   a2 >>= (fun x -> return (f x)));\n\
		};\n\
		mod monad = {\n\
			mod applicative = super..applicative;\n\
\n\
			return = super..return;\n\
			`>>=` m f = super..bind f m;\n\
			`>>` a b = a >>= fun x -> b;\n\
		};\n\
	};\n\
\n\
	stdin = @std_builtin(stdin);\n\
	stdout = @std_builtin(stdout);\n\
	stderr = @std_builtin(stderr);\n\
	stream_err stream = @std_builtin(stream_err);\n\
\n\
	seq a b = case $ a of | _ -> b;\n\
    strict_pair p = case p of\n\
        | a :: b -> case $ a of\n\
            | a1 -> case $ b of\n\
            | b1 -> a1 :: b1;\n\
\n\
	exit = @std_builtin(exit);\n\
\n\
	run x = case x of\n\
		| IO..IO runState -> strict_pair (runState IO..State..State)\n\
		| x 			  -> seq x exit;\n\
\n\
    read_file_contents file = @std_builtin(read_file_contents);\n\
    read_line file = @std_builtin(read_file_line);\n\
\n\
    write file val = @std_builtin(write);\n\
\n\
	mod io = {\n\
		mod functor = super..IO..functor;\n\
		mod applicative = super..IO..applicative;\n\
		mod monad = super..IO..monad;\n\
\n\
		stdin = super..stdin;\n\
		stdout = super..stdout;\n\
		stderr = super..stderr;\n\
\n\
		write file val = monad..return (super..write file val);\n\
		writeln file val = use monad { `>>` } in write file val >> write file '\n';\n\
		print = write stdout;\n\
		println = writeln stdout;\n\
\n\
		read_file_contents = monad..return . super..read_file_contents;\n\
		read_line = monad..return . super..read_line;\n\
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

