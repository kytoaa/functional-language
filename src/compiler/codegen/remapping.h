#ifndef func_lang_compiler_codegen_remapping_h
#define func_lang_compiler_codegen_remapping_h

#include "../../bytecode.h"
#include "../../parsing/ast.h"

struct RemappingWork {
    union {
        struct IdentifierNode *identifier;
        struct NamespaceAccessNode *namespace_access;
    };
    struct Location loc;
    u16 arg_count;
    u32 bytecode_index;
    u16 searching_from_module;
    bool is_namespace;
    bool is_pattern_constructor;
};
struct RemappingQueue {
    struct RemappingWork *ptr;
    struct RemappingWork *start;
    u32 cap;
    u32 len;
};

struct CompilingChunk {
    struct Chunk *chunk;
    struct {
        struct RemappingQueue *queue;
        u32 count;
    } remapping;
};

void enqueue_remapping_work(struct RemappingQueue *queue, struct RemappingWork work);
struct RemappingWork dequeue_remapping_work(struct RemappingQueue *queue);
void free_remapping_queue(struct RemappingQueue *queue);

#endif
