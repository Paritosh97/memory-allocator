#ifndef   	_MEM_ALLOC_TYPES_H_
#define   	_MEM_ALLOC_TYPES_H_



/* Structure declaration for a free block */
typedef struct mem_free_block{
    unsigned int size;
    char isFree;
    struct mem_free_block *next;
    /* TODO: DEFINE */
} mem_free_block_t; 


/* Specific metadata for used blocks */
typedef struct mem_used_block{
    unsigned long size;
    /* TODO: DEFINE */
} mem_used_block_t;



#endif
