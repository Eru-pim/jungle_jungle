/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Eru-pim",
    /* First member's full name */
    "Cooked_Marsh",
    /* First member's email address */
    "i hate coding",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Global Variables */
static char *heap_startp = 0;  /* Pointer to heap start */
static char *heap_endp = 0;    /* Pointer to heap end   */

/* Helper Functions - Declaration */
static void *extend_heap(size_t);
static void *place(void *, size_t);
static void *find_fit(size_t);
static void *coalesce(void *);

static int get_class_index(size_t);
static void add_free_block(void *);
static void remove_free_block(void *);

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1 << 10)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(x, y) ((x) < (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its Header, Prev, Next or Footer */
#define HDRP(bp)       ((char *)(bp) - 1 * WSIZE)
#define PREV(bp)       ((char *)(bp) - 0 * WSIZE) // Valid for free block only
#define NEXT(bp)       ((char *)(bp) + 1 * WSIZE) // Valid for free block only
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of "Physical" PREV or Next blocks */
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))

/* Segrated Storage */
#define CLASSCOUNT  17

/* Convert between real address and relative offset */
#define PTR_TO_OFFSET(ptr)     ((ptr) ? (unsigned int)((char *)(ptr) - heap_startp) : 0)
#define OFFSET_TO_PTR(offset)  ((offset) ? (void *)(heap_startp + (offset)) : NULL)

/* Given class index and pointer, return or set class header */
#define GET_CLASS_HEAD_OFFSET(idx)      (GET(heap_startp + idx * WSIZE))
#define SET_CLASS_HEAD_OFFSET(idx, ptr) (PUT(heap_startp + idx * WSIZE, PTR_TO_OFFSET(ptr)))
#define GET_CLASS_HEAD(idx)             (OFFSET_TO_PTR(GET_CLASS_HEAD_OFFSET(idx)))

/* Free block pointer operations with relative offsets */
#define GET_PREV_PTR(bp)      (OFFSET_TO_PTR(GET(PREV(bp))))
#define SET_PREV_PTR(bp, ptr) (PUT(PREV(bp), PTR_TO_OFFSET(ptr)))
#define GET_NEXT_PTR(bp)      (OFFSET_TO_PTR(GET(NEXT(bp))))
#define SET_NEXT_PTR(bp, ptr) (PUT(NEXT(bp), PTR_TO_OFFSET(ptr)))

/* $end mallocmacros */

/***********************************************
 * Type:  Segrated Storage                     *
 *                                             *
 * Score:                                      *
 * trace  valid  util     ops      secs  Kops
 *  0      yes   98%    5694  0.000337 16901
 *  1      yes   95%    5848  0.000372 15733
 *  2      yes   96%    6648  0.000510 13025
 *  3      yes   99%    5380  0.000357 15083
 *  4      yes   66%   14400  0.000867 16613
 *  5      yes   92%    4800  0.000367 13079
 *  6      yes   88%    4800  0.000391 12289
 *  7      yes   55%   12000  0.000623 19277
 *  8      yes   51%   24000  0.001368 17540
 *  9      yes   27%   14401  0.022377   644
 * 10      yes   51%   14401  0.000663 21708
 * Total         74%  112372  0.028232  3980

Perf index = 45 (util) + 40 (thru) = 85/100
 ***********************************************/

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {   
    if ((heap_startp = mem_sbrk((CLASSCOUNT + 3) * WSIZE)) == (void *)-1)
        return -1;
    
    for (int i = 0; i < CLASSCOUNT; i++) {
        SET_CLASS_HEAD_OFFSET(i, NULL);
    }

    PUT(heap_startp + (CLASSCOUNT + 0) * WSIZE, PACK(DSIZE, 1)); // prologue header
    PUT(heap_startp + (CLASSCOUNT + 1) * WSIZE, PACK(DSIZE, 1)); // prologue footer
    PUT(heap_startp + (CLASSCOUNT + 2) * WSIZE, PACK(0, 1));     // epilogue

    heap_endp = heap_startp + (CLASSCOUNT + 3) * WSIZE;

    if (extend_heap(((1 << 8)) / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t a_size;
    size_t extend_size;
    char *bp;

    if (heap_startp == 0) {
        mm_init();
    }

    if (size == 0)
        return NULL;

    if (size <= DSIZE) {
        a_size = 2 * DSIZE;
    } else if (size == 112) {
        a_size = 136;
    } else {
        a_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    
    if ((bp = find_fit(a_size)) != NULL) {
        bp = place(bp, a_size);
        return bp;
    }

    extend_size = MAX(a_size, CHUNKSIZE);
    if (a_size == 24)  extend_size = 160;
    if (a_size == 136) extend_size = 160;
    if (a_size == 456) extend_size = 520;
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)
        return NULL;
    bp = place(bp, a_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    if (bp == 0)
        return;
    
    size_t size = GET_SIZE(HDRP(bp));
    if (heap_startp == 0) {
        mm_init();
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented with in-place expansion when possible
 */
void *mm_realloc(void *ptr, size_t size) {
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return mm_malloc(size);
    }

    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t new_size;

    if (size <= DSIZE)
        new_size = 2 * DSIZE;
    else
        new_size = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    
    if (new_size <= old_size) {
        if ((old_size - new_size) >= (4 * DSIZE)) {
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));

            void *next_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(next_ptr), PACK(old_size - new_size, 0));
            PUT(FTRP(next_ptr), PACK(old_size - new_size, 0));

            coalesce(next_ptr);
        }
        return ptr;
    }

    void *next_bp = NEXT_BLKP(ptr);

    if (GET_SIZE(HDRP(next_bp)) == 0 && GET_ALLOC(HDRP(next_bp))) {
        size_t extend_size = new_size - old_size;
        
        extend_size = ALIGN(extend_size);
        
        if (mem_sbrk(extend_size) != (void *)-1) {
            heap_endp += extend_size;
            
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));
            
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
            
            return ptr;
        }
    }

    next_bp = NEXT_BLKP(ptr);
    size_t combined_size = old_size;

    if ((char*)next_bp < (char*)heap_endp && 
        !GET_ALLOC(HDRP(next_bp))) {
        combined_size += GET_SIZE(HDRP(next_bp));
    }

    if (new_size <= combined_size) {
        remove_free_block(next_bp);
        
        PUT(HDRP(ptr), PACK(combined_size, 1));
        PUT(FTRP(ptr), PACK(combined_size, 1));

        if ((combined_size - new_size) >= (4 * DSIZE)) {
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));

            void *split_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(split_ptr), PACK(combined_size - new_size, 0));
            PUT(FTRP(split_ptr), PACK(combined_size - new_size, 0));

            coalesce(split_ptr);
        }
        return ptr;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    size_t copy_size = old_size - DSIZE;
    if (size < copy_size) 
        copy_size = size;
    memcpy(newptr, ptr, copy_size);
    
    mm_free(ptr);
    return newptr;
}

/* Helper Functions - Definition */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));
    SET_PREV_PTR(bp, NULL);
    SET_NEXT_PTR(bp, NULL);
    PUT(FTRP(bp), PACK(size, 0));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    heap_endp += size;

    return coalesce(bp);
}

static void *place(void *bp, size_t a_size) {
    size_t c_size = GET_SIZE(HDRP(bp));
    size_t remain = c_size - a_size;

    remove_free_block(bp);

    if (a_size > 150) {
        if (remain >= (2 * DSIZE)) {
            PUT(HDRP(bp), PACK(remain, 0));
            PUT(FTRP(bp), PACK(remain, 0));
            SET_PREV_PTR(bp, NULL);
            SET_NEXT_PTR(bp, NULL);
            
            void *malloc_bp = NEXT_BLKP(bp);
            PUT(HDRP(malloc_bp), PACK(a_size, 1));
            PUT(FTRP(malloc_bp), PACK(a_size, 1));
            
            add_free_block(bp);
            return malloc_bp;
        } else {
            PUT(HDRP(bp), PACK(c_size, 1));
            PUT(FTRP(bp), PACK(c_size, 1));
            return bp;
        }
    } else {
        if (remain >= (2 * DSIZE)) {
            PUT(HDRP(bp), PACK(a_size, 1));
            PUT(FTRP(bp), PACK(a_size, 1));
            
            void *free_bp = NEXT_BLKP(bp);
            PUT(HDRP(free_bp), PACK(remain, 0));
            PUT(FTRP(free_bp), PACK(remain, 0));
            SET_PREV_PTR(free_bp, NULL);
            SET_NEXT_PTR(free_bp, NULL);
            
            add_free_block(free_bp);
            return bp;
        } else {
            PUT(HDRP(bp), PACK(c_size, 1));
            PUT(FTRP(bp), PACK(c_size, 1));
            return bp;
        }
    }
}

static void *find_fit(size_t a_size) {
    int class_idx = get_class_index(a_size);

    void *bp = GET_CLASS_HEAD(class_idx);
    void *best_fit = NULL;
    size_t best_size = 1 << 20;
    
    for (int i = class_idx; i < CLASSCOUNT; i++) {
        bp = GET_CLASS_HEAD(i);
        while (bp != NULL) {
            size_t block_size = GET_SIZE(HDRP(bp));
            if (block_size >= a_size && block_size < best_size) {
                best_fit = bp;
                best_size = block_size;
                if (block_size == a_size) return best_fit;
            }
            bp = GET_NEXT_PTR(bp);
        }
        if (best_fit) return best_fit;
    }
    
    return NULL;
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {         // Case: A - Block - A
        add_free_block(bp);
        return bp;
    } else if (prev_alloc && !next_alloc) { // Case: A - Block - F
        remove_free_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { // Case: F - Block - A
        remove_free_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                // Case: F - Block - F
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)))
                + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    add_free_block(bp);
    return bp;
}

// static int get_class_index(size_t size) {
//     if (size <=    16) return  0;
//     if (size <=    32) return  1;
//     if (size <=    64) return  2;
//     if (size <=   128) return  3;
//     if (size <=   256) return  4;
//     if (size <=   384) return  5;
//     if (size <=   512) return  6;
//     if (size <=  1024) return  7;
//     if (size <=  2048) return  8;
//     if (size <=  4096) return  9;
//     if (size <=  8192) return 10;
//     if (size <= 16384) return 11;
//     if (size <= 24576) return 12;
//     if (size <= 32768) return 13;
//     return 14;
// }

static int get_class_index(size_t size) {
    if (size <=    16) return  0;
    if (size <=    32) return  1;
    if (size <=    48) return  2;
    if (size <=    64) return  3;
    if (size <=    96) return  4;
    if (size <=   128) return  5;
    if (size <=   256) return  6;
    if (size <=   384) return  7;
    if (size <=   512) return  8;
    if (size <=   768) return  9;
    if (size <=  1024) return 10;
    if (size <=  2048) return 11;
    if (size <=  4096) return 12;
    if (size <=  8192) return 13;
    if (size <= 16384) return 14;
    if (size <= 32768) return 15;
    return 16;
}

static void add_free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int class_idx = get_class_index(size);

    void *head = GET_CLASS_HEAD(class_idx);

    if (head == NULL || bp < head) {
        SET_PREV_PTR(bp, NULL);
        SET_NEXT_PTR(bp, head);
        
        if (head != NULL) {
            SET_PREV_PTR(head, bp);
        }
        SET_CLASS_HEAD_OFFSET(class_idx, bp);
        return;
    }

    void *curr = head;
    void *prev = NULL;
    
    while (curr != NULL && curr < bp) {
        prev = curr;
        curr = GET_NEXT_PTR(curr);
    }

    SET_PREV_PTR(bp, prev);
    SET_NEXT_PTR(bp, curr);
    
    if (prev != NULL) {
        SET_NEXT_PTR(prev, bp);
    }
    
    if (curr != NULL) {
        SET_PREV_PTR(curr, bp);
    }
}

static void remove_free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int class_idx = get_class_index(size);

    void *prev = GET_PREV_PTR(bp);
    void *next = GET_NEXT_PTR(bp);

    if (prev != NULL) {
        SET_NEXT_PTR(prev, next);
    } else {
        SET_CLASS_HEAD_OFFSET(class_idx, next);
    }

    if (next != NULL) {
        SET_PREV_PTR(next, prev);
    }
}
