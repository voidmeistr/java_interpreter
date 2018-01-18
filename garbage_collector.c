/********************* garbage_collector.c ************************************/
/* Subor:               garbage_collector.c                                   */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include "garbage_collector.h"
#include "parser.h"

g_list list;

void gc_list_init()
{
    list.first = NULL;
}

void gc_list_free()
{
    gc_item_ptr temp = NULL;
    while (list.first != NULL)  //go through whole list
    {
        //free items
        temp = list.first;
        list.first = temp->next;
        free(temp->data);
        free(temp);
    }
}

void *gc_alloc_pointer(int size)
{
    gc_item_ptr temp = malloc(sizeof(struct gc_item));
    if (temp == NULL)
    {
        throw_error(E_INTERNAL);
    }
    else
    {
        temp->data = NULL;
        temp->next = NULL;
        if (list.first == NULL)
        {
            list.first = temp;  //insert first item
        }
        else
        {
            temp->next = list.first; //insert item to beginning
            list.first = temp;
        }//i dont have allocated data

        void *data = malloc(size);
        if (data == NULL)
        {
            throw_error(E_INTERNAL);
        }
        temp->data = data;
        return data;
    }

    return NULL;
}

void *gc_realloc_pointer(void *ptr, int size)
{
    void *new;

    gc_item_ptr gc_item = find_pointer(ptr);

    if (gc_item == NULL) {
        ptr = gc_alloc_pointer(size);
        return ptr;
    } else {
        new = realloc(ptr ,size);
        if (new != NULL) {
            gc_item->data = new;
            return new;
        }
        else
            throw_error(E_INTERNAL);
    }

    return NULL;
}

gc_item_ptr find_pointer(void *ptr)
{
    gc_item_ptr temp = list.first;

    while (temp != NULL) //found item
    {
        if (temp->data == ptr)
            return temp;

        temp = temp->next;  //move in list
    }
    return NULL;
}
