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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    ""
    /* Second member's email address (leave blank if none) */
    ""
};
/* 기본 정수 선언 */
#define WSIZE   4 // word 단위
#define DSIZE   8 // byte 단위
#define CHUNKSIZE   (1<<12) // 힙늘릴때, 4kb만큼 늘릴 것이다.


/* 최대 값 */
#define MAX(x, y) ((x) > (y)? (x) : (y))

/* 사이즈와 할당정보를 통합해서, 헤더와 풋터에 저장할 수 있는 값을 리턴*/
#define PACK(size, alloc)   ((size)|(alloc))    

/* 주소 p가 참조하는 위드를 읽어 리턴/ 여기서 캐스팅이 매우 중요*/
#define GET(p) (*(unsigned int *)(p))

/* 주소 p가 참조하는 워드에 val을 저장 */
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* 주소 p의 Header 또는 Footer의 사이즈와 할당 비트를 리턴 */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 블럭 포인터를 bp라고 했을때, 그것의 Header와 Footer를 가리키는 포인터를 리턴 */
#define HDRP(bp) ((int *)(bp) - WSIZE)
#define FTRP(bp) ((int *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 블럭 포인터를 bp라고 했을때, 다음과 이전 블럭 포인터를 각각 리턴 */
#define NEXT_BLKP(bp) ((int *)(bp) + GET_SIZE(((int *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((int *)(bp) - GET_SIZE(((int *)(bp) - DSIZE)))
 
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init : 최초 가용 블록으로 힙 생성하기
 * mm_malloc이나 mm_free를 호출하기 전에 응용은 mm_init 함수를 호출해서 힙을 초기화해야 함                                               
 */
static char * heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(int *bp);

int mm_init(void)
{
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) - 1)
        return -1;
    PUT(heap_listp, 0);     // 정렬을 위한 padding, heap_listp가 참조하는 워드에 0을 저장, 미사용 패딩워드
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue 헤더, DSIZE와 할당되었다는 1 저장
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue 풋터, DSIZE와 할당되었다는 1 저장
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));      // epiloque 헤더, 크기가 0 + 할당되었다는 1 저장 
    heap_listp += (2  * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

static void *extend_heap(size_t words)
{
    int *bp;
    size_t size;

    /* 짝수 갯수의 워드들을 할당한다. 정렬 제한을 유지하기 위해 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* 초기와한다 free 블럭의 Header와 Footer를, 그리고 Epilogue 헤더를 */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /* 만약 이전 블럭이 free하다면 연결한다.*/
    return coalesce(bp);
    }

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 * 가용 리스트에서 블록 할당하기
 */
void *mm_malloc(size_t size)
{
    size_t asize;   // 블럭 사이즈를 조정
    size_t extendsize;  // 맞지 않으면, 힙을 확장하는 amount
    int *bp;
    /* 비논리적인 요청은 무시한다. */
    if (size == 0) return NULL;
    /* 헤더와 푸터 포함해 블록사이즈 다시 조정 */
    if (size <= DSIZE) asize = 2*DSIZE;
    /* size보다 클 때, 블록이 가질 수 있는 크기 중 최적화 크기를 가짐*/
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* 맞게(fit) 하기 위해, free list를 찾는다. */
    if (bp = find_fit(asize, bp) != NULL) {
        place(bp, asize);
        return bp; // place를 마친 블록의 위치를 리턴
    }

    /* No fit found. GET more memory place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize); // 확장된 상태에서 asize를 넣어라.
    return bp;

    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    int *bp;       // 포인터형 변수가 (char *)형인 이유?
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

find_fit(asize, bp) {
    /* first fit 방법으로 먼저 구현*/
    int *bp;
    char *start = HDRP(bp);
    char *end = FTRP(bp);

    bp = start;
    /* p가 end보다 작고, *p가 홀수이거나 asize보다 작을 동안에 while 루프
    11111111111111111111111111111110와의 보수연산자, *p의 가장 오른쪽 비트를 0으로 없앨수 있음. */
    while ((bp < end) && ((*bp & 1) || (*bp <= asize)))
        bp = bp + (*bp & -2); 
}

place(bp, asize) {
    /* 원래 강의자료에는 bp대신, *bp로 나와있는데, int랑 자료형이 안맞아서, 우선 bp로 바꿔놓겠음!*/
    int newsize = ((asize + 1) >> 1) << 1;
    int oldsize = *bp & -2;
    *bp = newsize | 1;
    if (newsize < oldsize)
        *(bp + newsize) = oldsize - newsize;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        /* case 01 */
        return bp;
    }
    else if (prev_alloc && !next_alloc) {
        /* case 02 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {
        /* case 03 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        /* case 04 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














