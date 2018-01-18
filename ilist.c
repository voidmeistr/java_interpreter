/************************** ilist.c *********************************************/
/* Subor:               ilist.c                                               */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "ilist.h"
#include "garbage_collector.h"

/* Initialization of the list, nothing complicated. Just setting pointers to
 * NULL.
 */
void listInit(tInstrList *L) {
    L->first  = NULL;
    L->last   = NULL;
    L->active = NULL;
}

/* Removing all instructions from list. While cycle is going through whole list
 * and freeing one item at a time. Only things left are pointers to active and
 * last item, which need to be set to NULL.
 */
void listFree(tInstrList *L) {
    tListItem *ptr;
    while (L->first != NULL) {
        ptr = L->first;
        L->first = L->first->nextItem;
        free(ptr);
    }
    L->last = NULL;
    L->active = NULL;
}

/* Adding new instruction at the end of the list. First is allocation of memory
 * for instruction, then is instruction moved into the new allocated place.
 * After that it's only setting pointers. New added item points right to the
 * void (NULL) and points left to previous last item. Then if is list empty,
 * list pointer "first" points to new item. Otherwise, previous last item
 * points to new item. And finally, list pointer "last" points to newly added
 * item.
 */
void listInsertLast(tInstrList *L, tInstr I) {
    print_elements_of_list(*L);

    tListItem *newItem;
    newItem = gc_alloc_pointer(sizeof (tListItem));
    newItem->Instruction = I;
    newItem->nextItem = NULL;
    newItem->previousItem = L->last;
    if (L->first == NULL){
        L->first = newItem;
        L->last = newItem;
    }
    else{
        L->last->nextItem=newItem;
    }
    L->last=newItem;
}

//activate first instruction
void listFirst(tInstrList *L) {
    L->active = L->first;
}

// next instruction will be active
void listNext(tInstrList *L) {
    if (L->active != NULL)
        L->active = L->active->nextItem;
}

// instruction goto
void listGoto(tInstrList *L, void *gotoInstr) {
    L->active = (tListItem*) gotoInstr;
}

// return pointer to last instruction
void *listGetPointerLast(tInstrList *L) {
    return (void*) L->last;
}

// return active instruction
tInstr* listGetData(tInstrList *L) {
    if (L->active == NULL) {
        return NULL;
    }
    else
        return &L->active->Instruction;
}

void listRemoveFirst(tInstrList *L){
    tListItem *ptr;
    if (L->first == NULL){
        /* function got empty list, so it can't do anything */
    } else {
        ptr = L->first;
        L->first = ptr->nextItem;
        /* In case that first and last item is the same, we need to change last
         * pointer:
         */
        if (ptr == L->last){
            L->last = L->first;
        }
        /* In case of removing active item, we need to deactivate the whole
         * list:
         */
        if (L->active == ptr){
            L->active = NULL;
        }
        L->first->previousItem = NULL;
        free(ptr);
    }
}

//print instruction tape
void print_elements_of_list(tInstrList TL)	{

    tInstrList TempList=TL;
    int CurrListLength = 0;
    fprintf(stderr,"-----------------");
    while ((TempList.first!=NULL))	{
        fprintf(stderr,"\n \t%d",TempList.first->Instruction.instType);
        if ((TempList.first==TL.active) && (TL.active!=NULL))
            fprintf(stderr,"\t <= toto je aktivní prvek ");
        TempList.first=TempList.first->nextItem;
        CurrListLength++;
    }
    fprintf(stderr, "\n-----------------\n");
}
