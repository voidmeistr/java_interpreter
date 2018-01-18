/************************** ial.h *********************************************/
/* Subor:               ial.h -                                               */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/
#ifndef __IAL_H__
#define __IAL_H__

#include <stdio.h>
#include <stdbool.h>

#include "str.h"
#include "scanner.h"
#include "ilist.h"

//TODO komentare, doxygen

#define HEAP_SORT_FAIL 1
#define HEAP_SORT_OK 0

#define HASH_SEARCH_FAIL 1
#define HASH_SEARCH_OK 0

#define HASH_INSERT_FOUND 2
#define HASH_INSERT_ALLOC_ERR 1
#define HASH_INSERT_OK 0

#define MAX_HASH_TABLE_SIZE 71 //Max size of hash table

enum hash_table_data_types {
    VARIABLE,
    FUNCT,
    FUNCT_RINT,
    FUNCT_RDOUBLE,
    FUNCT_RSTR,
    FUNCT_FIND,
    FUNCT_PRINT,
    FUNCT_SRT,
    FUNCT_SUBSTR,
    FUNCT_LEN,
    FUNCT_COMP,
    FUNCT_ITEM,
    CONSTANT,
    CLASS,
};

enum types {
    T_INT,          // 0
    T_DOUBLE,       // 1
    T_STR,          // 2
    T_BOOL,         // 3
    T_VOID,         // 4
};

typedef union {
    string str;
    int integer;
    double double_number;
}t_data_value;

typedef struct tab_data t_data;

typedef struct tab_item t_item;

typedef struct table h_table;

typedef union {
    struct {
        int type;
        t_data_value value;
        bool is_init;
    } variable;
    struct {
        int ret_type;
        int param_count;
        tInstrList *instrList;
        h_table *loc_table;
        t_item *first_param;
    } function;
    struct {
        int type;
        t_data_value value;
    } constant;
    struct {
        int type;
        t_item *next;
    } fun_item;
    struct {
        h_table *loc_table;
    }class;
} tab_data_value;

struct tab_data{
    int type;
    tab_data_value value;
};

struct tab_item{
    string key;
    t_data data;
    t_item *next;
};

struct table {
    int size;
    t_item * items[];
};

extern int HASH_TABLE_SIZE;


void sift_down(string *character, int left, int right);

void heap_sort_algorithm(string *character);

int heap_sort(string *character, string *duplicate);

int find(string *str, string *substring);

int hash_code ( string key );

h_table * hash_table_init();

void hash_table_clear_all(h_table *hash_table);

int hash_table_search(h_table *hash_table,string *key, t_item **item);

int hash_table_insert(h_table *hash_table, string *key, t_item **item);

#endif
