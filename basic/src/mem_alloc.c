#include "mem_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "mem_alloc_types.h"
#include "my_mmap.h"

/* pointer to the beginning of the memory region to manage */
void *heap_start;

void * heap_end;

/* Pointer to the first free block in the heap */
mem_free_block_t *first_free;

int memory_size;


#define ULONG(x)((long unsigned int)(x))

#if defined(FIRST_FIT)

void *ff_alloc(size_t size)
{
    mem_free_block_t *i = first_free;

    while((void*)i >= heap_start && (void*)i <= heap_end)
    {
      if(i->size >= (size + sizeof(mem_used_block_t)))
        break;
      i = i->next;
    }
        
    return i;
}

#elif defined(BEST_FIT)

void *bf_alloc(size_t size)
{
    mem_free_block_t *i = first_free;
    mem_free_block_t *min = NULL;
    
    while((void*)i >= heap_start && (void*)i <= heap_end)
    {
      if(i->size >= (size + sizeof(mem_used_block_t)))
        {
          if(min == NULL)
          {
            min = i;
          }
          if(i->size < min->size)
          {
            min = i;
          }
        }
      i = i->next;
    }

    return (void *)min;
}

#elif defined(NEXT_FIT)

mem_free_block_t *prev_i = NULL;

void *get_next()
{
    mem_free_block_t *i = prev_i;
    mem_used_block_t *tmp;

    if((void*)i > heap_end || i == NULL)
      i = first_free;

    while(i != heap_end)
    {
      if(i->isFree)
      {
        break;
      }
      else
      {
        tmp = (void *)i;
        i = (void *)tmp + sizeof(mem_used_block_t) + tmp->size;
      }
    }
    return i;
}

void *nf_alloc(size_t size)
{
    mem_free_block_t *i = get_next();
    
    while((void*)i >= heap_start && (void*)i <= heap_end)
    {      
      if(i->size >= (size + sizeof(mem_used_block_t)))
        break;      
      i = i->next;
    }

    prev_i = i;

    return i;
}

#endif


void run_at_exit(void)
{
    fprintf(stderr,"YEAH B-)\n");
    
    /* TODO: insert your code here */
}

void update_free_list()
{
    mem_free_block_t *i = heap_start;
    mem_free_block_t *prev_i = NULL;
    mem_used_block_t *tmp;

    while(i != heap_end)
    {
      if(i->isFree)
      {
        if(prev_i != NULL)
          prev_i->next = i; 
        else
          first_free = i;
        
        prev_i = i;
        i = (void *)i + i->size;
      }
      else
      {
        tmp = (void *)i;
        i = (void *)tmp + sizeof(mem_used_block_t) + tmp->size;
      }
    }
}

void memory_init(void)
{
    /* register the function that will be called when the programs exits */
    atexit(run_at_exit);

    memory_size = MEMORY_SIZE;

    heap_start = my_mmap(memory_size);
    heap_end = heap_start + memory_size;
    first_free = heap_start;
    first_free->size = memory_size;
    first_free->isFree = 1;
}

void *memory_alloc(size_t size)
{
    if(size <= 0)
    {
      printf("Size should be greater than 0\n");
      exit(0);
    }

    mem_free_block_t *free;
    mem_used_block_t *tmp;

    #if defined(FIRST_FIT)
    free = ff_alloc(size);
    #elif defined(BEST_FIT)
    free = bf_alloc(size);
    #elif defined(NEXT_FIT)
    free = nf_alloc(size);
    #endif

    if(free->size >= (size + sizeof(mem_used_block_t)))
    {
      int tmp2 = free->size;
      tmp = (void*)free; 
      if(free->size < (size + sizeof(mem_used_block_t) + sizeof(mem_free_block_t)))
      {
        // When no space to put free block after splitting
        memset(free, 0, free->size);
        tmp->size = tmp2 - sizeof(mem_free_block_t);
      }
      else
      {
        memset(free, 0, free->size);
        tmp->size = size;

        free = (void *)free + sizeof(mem_used_block_t) + tmp->size;
        free->size = tmp2 - sizeof(mem_used_block_t) - tmp->size;
        free->isFree = 1;
      }
      print_alloc_info((void*)tmp + sizeof(mem_used_block_t), size);
      update_free_list();
    }
    else
    {
      print_alloc_error(size);
    }
    
    return ((void*)tmp + sizeof(mem_used_block_t));
}

void memory_free(void *p)
{
    mem_free_block_t *free = (void *)p - sizeof(mem_used_block_t);

    mem_used_block_t *used = (void *)p - sizeof(mem_used_block_t);
    
    // Placing Free Block
    free = (void *)used;
    free->size = memory_get_allocated_block_size(p);
    free->isFree = 1;
    print_free_info((void*)free + sizeof(mem_used_block_t));

    mem_free_block_t *tmp, *i;
    tmp = (void *)free + free->size;

    if(tmp->isFree)
    {
      // Contiguous free blocks
      free->size += tmp->size;
      memset(tmp, 0, tmp->size);
    }

    i = first_free;
    
    while((void*)i >= heap_start && (void*)i <= heap_end)
    {      
      tmp = i;
      i = i->next;
      if(i >= free)
        break;
    }

    if(((void*)tmp + tmp->size) == free)
    {
      // Contiguous free blocks
      tmp->size += free->size;
      memset(free, 0, free->size);
    }

    update_free_list();
}

size_t memory_get_allocated_block_size(void *addr)
{
    mem_used_block_t *used = addr - sizeof(mem_used_block_t);

    if(used->size != 0)
    {
      return used->size + sizeof(mem_used_block_t);
    }
    else
      return 0;
}


void print_mem_state(void)
{
    mem_free_block_t *i = heap_start;
    mem_used_block_t *tmp;

    printf("[");
    while((void*)i != heap_end)
    {
      
      if(i->isFree)
      {
        for (int j = 0; j < i->size; j++)
        {
          printf("_");
        }
        i = (void*)i + i->size;
      }
      else
      {
        tmp = (void *)i;
        for (int j = 0; j < tmp->size; j++)
        {
          printf("X");
        }
        i = (void *)tmp + sizeof(mem_used_block_t) + tmp->size;
      }
    }
    printf("]\n");
}


void print_info(void) {
  fprintf(stderr, "Memory : [%lu %lu] (%lu bytes)\n", (long unsigned int) heap_start, (long unsigned int) (heap_start+MEMORY_SIZE), (long unsigned int) (MEMORY_SIZE));
}

void print_free_info(void *addr){
    if(addr){
        fprintf(stderr, "FREE  at : %lu \n", ULONG(addr - heap_start));
    }
    else{
        fprintf(stderr, "FREE  at : %lu \n", ULONG(0));
    }
    
}

void print_alloc_info(void *addr, int size){
  if(addr){
    fprintf(stderr, "ALLOC at : %lu (%d byte(s))\n", 
	    ULONG(addr - heap_start), size);
  }
  else{
    fprintf(stderr, "Warning, system is out of memory\n"); 
  }
}

void print_alloc_error(int size) 
{
    fprintf(stderr, "ALLOC error : can't allocate %d bytes\n", size);
}


#ifdef MAIN
int main(int argc, char **argv){

  /* The main can be changed, it is *not* involved in tests */
  memory_init();
  print_info();
  int i ; 
  for( i = 0; i < 10; i++){
    char *b = memory_alloc(rand()%8);
    memory_free(b);
  }

  char * a = memory_alloc(15);
  memory_free(a);


  a = memory_alloc(10);
  memory_free(a);

  fprintf(stderr,"%lu\n",(long unsigned int) (memory_alloc(9)));
  return EXIT_SUCCESS;
}
#endif 
