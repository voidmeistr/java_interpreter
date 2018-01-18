/************************** frame.c ******************************************/
/* Subor:               frame.c -                                             */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include "frame.h"
#include "garbage_collector.h"

/* START OF HASH TABLE */

//Hash function
//return hash_code, the place where the item will be stored
//int hash_code ( string key ) {
//    int ret_val = 1;
//    for (int i = 0; i < key.length; i++)
//        ret_val += key.str[i];
//    return (ret_val % HASH_TABLE_SIZE);
//}


//initialize hash table
//function return pointer on initialized table
f_table * frame_table_init()
{
    f_table * frame_table = (f_table *) gc_alloc_pointer(sizeof(f_table) + sizeof(f_item *)*HASH_TABLE_SIZE );
    if ( frame_table != NULL)
    {
        for (int i = 0; i < HASH_TABLE_SIZE; i++)
        {
            frame_table->items[i] = NULL; //every element pointer is set on NULL
        }
    }
    else
    {
        return NULL;
    }
    frame_table->size = HASH_TABLE_SIZE; //set size on maxsize of hash_table
    return frame_table;
}

//finding item in hash_table
//when you find searched item, pointer where the pointer to found item will be stored,
//if item is not found, pointer has undefined value
int frame_table_search(f_table *frame_table,string *key, f_item **item)
{
    if (frame_table != NULL)
    {
        f_item *searched = frame_table->items[hash_code(*key)];
        while (searched != NULL)
        {
            if(strcmp(searched->key.str,key->str) == 0) //item is found
            {
                if (item != NULL)
                {
                    *item = searched;   //set pointer
                }
                return HASH_SEARCH_OK;
            }
            searched = searched->next;  //if not found go found to the next pointer
        }
        return HASH_SEARCH_FAIL;
    }
    return HASH_SEARCH_FAIL;
}

//allocate place for new item and insert new item to hash_table
int frame_table_insert(f_table *frame_table, string *key, f_item **item)
{
    f_item * inserted;
    if (frame_table_search(frame_table,key,item) == HASH_SEARCH_OK)
    {
        return HASH_INSERT_FOUND;   //on key position exist another item
    }
    inserted = gc_alloc_pointer(sizeof(f_item));   //allocate memory for new item
    if (inserted != NULL)
    {
        inserted->next = frame_table->items[hash_code(*key)];    //next item will be the actual item
        if ((duplicate_str(&inserted->key,key)) != STR_OK )
        {
            return HASH_INSERT_ALLOC_ERR;
        }
        if (item != NULL)
        {
            *item = inserted;  //pointer where pointer to inserted item will be stored (only if exists)
        }
        frame_table->items[hash_code(*key)] = inserted;  //actual item is inserted item
        return HASH_INSERT_OK;
    }
    else
    {
        return HASH_INSERT_ALLOC_ERR;
    }
}
