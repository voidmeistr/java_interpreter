/************************** stack.h *******************************************/
/* Subor:               stack.h                                               */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#ifndef STACK_H
#define STACK_H

#include "precedence.h"

// expected size of items in Stack
#define STACK_SIZE 5

typedef struct Stack{
    int top;
    int size;
    void * stack_data[];
} *tStack;

// creates Stack with predefined size
tStack createStack();

// adds item into Stack
int addItem(tStack *S, void *item);

// returns token from top of the stack_data
void *popItem(tStack S);

/* ONLY FOR TESTING PURPOSE */

void printItems(tStack S);

//tToken *getTestToken(char *c, tState state);

#endif //STACK_H
