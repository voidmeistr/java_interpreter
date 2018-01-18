/************************** ial.c *********************************************/
/* Subor:               ial.c                                                 */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include "ial.h"
#include "garbage_collector.h"
#include "str.h"

#define DEBUG

int HASH_TABLE_SIZE = MAX_HASH_TABLE_SIZE;  //predefined maximum size of table

//sift_down, sift parent node down
//we compare out parent with greater children (right or left) and if parent is lesser than the chosen children
//they are switched and we can do that until our node is parent is lowermost node in binary tree
void sift_down(string *character, int left, int right) {
    int i = left;
    int j = 2 * i;
    bool cont;
    int temp = character->str[i];
    cont = j <= right;
    while (cont) {
        if (j < right)  //have a right brother
        {
            if (character->str[j] < character->str[j + 1]) //which one of brothers is greater
            {
                j = j + 1;
            }
        }
        if (temp >= character->str[j])   //end function when no one is greater or equal than actual character
        {
            cont = false;
        } else {
            character->str[i] = character->str[j]; //set new as old
            i = j;  //son becomes a father
            j = 2 * i;    //next left son
            cont = j <= right;
        }
    }
    character->str[i] = temp;   //switch is completely
}

//algorithm of heap sort, first step - build a heap, second step - sort the heap until it is lined up
void heap_sort_algorithm(string *character) {
#ifdef DEBUG
    fprintf(stderr, "Inside the function heap_sort_algorithm\n");
#endif
    int n = character->length - 1;
    char temp;
    int left = n / 2;
    int right = n;
    for (int i = left; i >= 0; i--)   // build a heap
    {
        sift_down(character, i, right);
    }
    for (right = n; right >= 1; right--)   //sort a heap
    {
        temp = character->str[0];   //switch the root with last active element
        character->str[0] = character->str[right];
        character->str[right] = temp;

        sift_down(character, 0, right - 1); //repair heap, switch can damage a heap
    }
}

//function which execute the heap sort, return sorted string
//character is input string and duplicate is created to be sure if something goes wrong and sort will fail
int heap_sort(string *character, string *duplicate) {
    if (duplicate_str(duplicate, character) == STR_ERROR)   //create a duplicate of input string
        return HEAP_SORT_FAIL;
#ifdef DEBUG
    fprintf(stderr, "Inside the function heap_sort\n");
#endif
    heap_sort_algorithm(duplicate); //heap sort with duplicated string
    return HEAP_SORT_OK;
}

//function which find substring in string and returns position when the string has started to be identical,
// we count numbers from zero
int find(string *str, string *substring) {
    int fail[substring->length];
    int k, r;

    //we create helpful vector which will tell us where is the next place where i can search in substring
    fail[0] = -1;   //necessary to move string
    for (k = 1; k < substring->length; k++) {
        r = fail[k - 1];
        while (r > -1 && substring->str[r] != substring->str[k - 1]) {
            r = fail[r];
        }
        fail[k] = r + 1;
    }
    //Knuth–Morris–Pratt algorithm
    int str_index = 0;
    int sub_str_index = 0;

    while (str_index < str->length && sub_str_index < substring->length) {
        if ((sub_str_index == -1) || (str->str[str_index] ==
                                      substring->str[sub_str_index])) {                   //if need to move with str_index or find same character
            str_index++;
            sub_str_index++;
        } else {
            sub_str_index = fail[sub_str_index];
            /*if index of fail vector on sub_str_index is 0 the value is -1 and this increase str_index
            in next instance of the cycle*/
        }
    }
    if (sub_str_index == substring->length) {
        return str_index - substring->length; //return the start position of substring in string
    } else {
        return -1;  //substring not found
    }
}

/* START OF HASH TABLE */

//Hash function
//return hash_code, the place where the item will be stored
int hash_code(string key) {
    int ret_val = 1;
    for (int i = 0; i < key.length; i++)
        ret_val += key.str[i];
    return (ret_val % HASH_TABLE_SIZE);
}


//initialize hash table
//function return pointer on initialized table
h_table *hash_table_init() {
    h_table *hash_table = (h_table *) gc_alloc_pointer(sizeof(h_table) + sizeof(t_item *) * HASH_TABLE_SIZE);
    if (hash_table != NULL) {
        for (int i = 0; i < HASH_TABLE_SIZE; i++) {
            hash_table->items[i] = NULL; //every element pointer is set on NULL
        }
    } else {
        return NULL;
    }
    hash_table->size = HASH_TABLE_SIZE; //set size on maxsize of hash_table
    return hash_table;
}

//function deallocate all alloc' ed memory in hash table and deallocate itself too
//firstly i need deallocate all items
void hash_table_clear_all(h_table *hash_table) {
    if (hash_table != NULL) {
        t_item *temp;   //item which is deallocated
        t_item *temp_next;
        for (int i = 0; i < hash_table->size; i++)  //iterate through table
        {
            temp = hash_table->items[i];
            while (temp != NULL) {
                temp_next = temp->next; //i need to remember which item is next
                free_str(&temp->key);   //deallocate key (key is type of string)
                switch (temp->data.type)    //based on type of data deallocate the right string
                {
                    case VARIABLE: {
                        if (temp->data.value.variable.type == T_STR) {
                            free_str(&temp->data.value.variable.value.str);
                        }
                        break;
                    }
                        //the function deallocation is executed recursively
                    case FUNCT:
                    case FUNCT_COMP:
                    case FUNCT_FIND:
                    case FUNCT_LEN:
                    case FUNCT_SRT:
                    case FUNCT_SUBSTR:
                    case CLASS: {
                        hash_table_clear_all(temp->data.value.function.loc_table);
                        break;
                    }
                    case CONSTANT: {
                        if (temp->data.value.constant.type == T_STR) {
                            free_str(&temp->data.value.constant.value.str);
                        }
                        break;
                    }
                }
                free(temp);         //deallocate item of hash_table
                temp = temp_next;   //set item as memorized next item
            }
        }
        free(hash_table);   //each item was deallocate at the end deallocate table
    }
}


//finding item in hash_table
//when you find searched item, pointer where the pointer to found item will be stored,
//if item is not found, pointer has undefined value
int hash_table_search(h_table *hash_table, string *key, t_item **item) {
    if (hash_table != NULL) {
        t_item *searched = hash_table->items[hash_code(*key)];

        while (searched != NULL) {
            if (strcmp(searched->key.str, key->str) == 0) //item is found
            {
                if (item != NULL) {
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
int hash_table_insert(h_table *hash_table, string *key, t_item **item) {
    t_item *inserted;
    if (hash_table_search(hash_table, key, item) == HASH_SEARCH_OK) {
        return HASH_INSERT_FOUND;   //on key position exist another item
    }
    inserted = gc_alloc_pointer(sizeof(t_item));   //allocate memory for new item
    if (inserted != NULL) {
        inserted->next = hash_table->items[hash_code(*key)];    //next item will be the actual item
        if ((duplicate_str(&inserted->key, key)) != STR_OK) {
            //free(inserted);
            return HASH_INSERT_ALLOC_ERR;
        }
        if (item != NULL) {
            *item = inserted;  //pointer where pointer to inserted item will be stored (only if exists)
        }
        hash_table->items[hash_code(*key)] = inserted;  //actual item is inserted item
        return HASH_INSERT_OK;
    } else {
        return HASH_INSERT_ALLOC_ERR;
    }
}
