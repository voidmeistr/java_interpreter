/************************** stack.c *******************************************/
/* Subor:               stack.c                                               */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include <stdlib.h>
#include "stack.h"
#include "garbage_collector.h"

tStack createStack() {
    tStack new = (tStack) gc_alloc_pointer(sizeof(struct Stack) + sizeof(void *)*STACK_SIZE);
    if (new != NULL) {
        new->top = -1;
        // stack_data size is just offset for maximal value of index in Stack
        new->size = new->top + STACK_SIZE;
        return new;
    } else
        return NULL;
}

tStack reallocStack(tStack S) {
    return (tStack) gc_realloc_pointer((void *)S, sizeof(struct Stack) + sizeof(void *)*(S->size+STACK_SIZE+1));
}

// AddsItem to function to the top if position is negative number, otherwise it will add item
// item on the position and move item on this position and remaining items to te right
int addItem(tStack *S, void *item) {
    if (S == NULL || item == NULL){
//        printf("Trying to add item but %p(stack_data) or %p(token) is NULL\n", (void*)S, item);
        return 0;
    }

    // Reallocating Stack, if is full
    if ((*S)->top == (*S)->size) {
        tStack tmp = reallocStack((*S));
        if (tmp == NULL) {
            puts("Error reallocating memory");
            return 0;
        } else {
            *S = tmp;
            // if we have new reallocated Stack, we have to increase Stack size
            (*S)->size += STACK_SIZE;
        }
    }


    // adding terminal into stack_data
    (*S)->top++;
    (*S)->stack_data[(*S)->top] = item;


    return 1;
}

void *popItem(tStack S) {
    S->top--;
    return S->stack_data[S->top+1];
}


