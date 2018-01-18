/************************** garbage_collector.h  ******************************/
/* Subor:               garbage_collector.h                                   */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#ifndef __GARBAGE_COLLECTOR__
#define __GARBAGE_COLLECTOR__

#include <stdio.h>
#include <stdlib.h>

typedef struct gc_item {
    struct gc_item *next;
    void *data;
} *gc_item_ptr;

typedef struct {
    gc_item_ptr first;
} g_list;

void gc_list_init(); //init list
void gc_list_free(); //free list
void *gc_alloc_pointer(int size); //our malloc
void *gc_realloc_pointer(void *ptr, int size); //our realloc

//helpful function
gc_item_ptr find_pointer(void *ptr); //free pointer
#endif
