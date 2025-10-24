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

/***********************************
 * Macros from code/vm/malloc/mm.c *
 ***********************************/
/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* $end mallocmacros */

/* Global Variables */
static char *heap_listp = 0;  /* Pointer to first block */
static char *rover;

/* Helper Functions - Declaration */
static void *extend_heap(size_t);
static void place(void *, size_t);
static void *find_fit(size_t);
static void *coalesce(void *);

/***********************************************
 * Type:  Next fit +                           *
 *        Inplace Realloc                      *
 * Score:                                      *
 * trace  valid  util     ops      secs  Kops  *
 *  0       yes   91%    5694  0.002253  2527  *
 *  1       yes   92%    5848  0.001524  3836  *
 *  2       yes   95%    6648  0.003871  1717  *
 *  3       yes   97%    5380  0.003822  1408  *
 *  4       yes   66%   14400  0.000391 36866  *
 *  5       yes   91%    4800  0.004349  1104  *
 *  6       yes   89%    4800  0.003728  1288  *
 *  7       yes   55%   12000  0.016367   733  *
 *  8       yes   51%   24000  0.008880  2703  *
 *  9       yes   27%   14401  0.052687   273  *
 * 10       yes   53%   14401  0.000456 31588  *
 * Total          73%  112372  0.098328  1143  *
 *                                             *
 * Perf index = 44 (util) + 40 (thru) = 84/100 *
 ***********************************************/

/* Current Work
 * 
 */

/* TODO
 * 
 */

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {   
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 1st prologue
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 2nd prologue
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     //     epilogue
    heap_listp += 2 * WSIZE;
    
    rover = heap_listp;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
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

    if (heap_listp == 0) {
        mm_init();
    }

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        a_size = 2 * DSIZE;
    else
        a_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
    if ((bp = find_fit(a_size)) != NULL) {
        place(bp, a_size);
        return bp;
    }

    extend_size = MAX(a_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)
        return NULL;
    place(bp, a_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    if (bp == 0)
        return;
    
    size_t size = GET_SIZE(HDRP(bp));
    if (heap_listp == 0) {
        mm_init();
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
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
    size_t new_size, inplace_size;

    if (size <= DSIZE)
        new_size = 2 * DSIZE;
    else
        new_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    inplace_size = old_size;
    if (GET_SIZE(HDRP(NEXT_BLKP(ptr))) > 0 && !GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
        inplace_size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));

    if (new_size <= inplace_size) {
        if (inplace_size > old_size) {
            PUT(HDRP(ptr), PACK(inplace_size, 1));
            PUT(FTRP(ptr), PACK(inplace_size, 1));
        }

        if ((inplace_size - new_size) >= (2 * DSIZE)) {
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));

            void *next_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(next_ptr), PACK(inplace_size - new_size, 0));
            PUT(FTRP(next_ptr), PACK(inplace_size - new_size, 0));

            coalesce(next_ptr);
        }
        return ptr;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    
    if (size < old_size) old_size = size;
    memcpy(newptr, ptr, old_size);
    
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
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void place(void *bp, size_t a_size) {
    size_t c_size = GET_SIZE(HDRP(bp));

    if ((c_size - a_size) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(a_size, 1));
        PUT(FTRP(bp), PACK(a_size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(c_size - a_size, 0));
        PUT(FTRP(bp), PACK(c_size - a_size, 0));
    } else {
        PUT(HDRP(bp), PACK(c_size, 1));
        PUT(FTRP(bp), PACK(c_size, 1));
    }
}

static void *find_fit(size_t a_size) {
    char *old_rover = rover;

    for (; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover))
        if (!GET_ALLOC(HDRP(rover)) && (a_size <= GET_SIZE(HDRP(rover))))
            return rover;

    for (rover = heap_listp; rover < old_rover; rover = NEXT_BLKP(rover))
        if (!GET_ALLOC(HDRP(rover)) && (a_size <= GET_SIZE(HDRP(rover))))
            return rover;
    
    return NULL;
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {         // Case: A - Block - A
        return bp;
    } else if (prev_alloc && !next_alloc) { // Case: A - Block - F
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { // Case: F - Block - A
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                // Case: F - Block - F
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)))
                + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // rover back
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp)))
        rover = bp;

    return bp;
}