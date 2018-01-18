/************************** frame.h ******************************************/
/* Subor:               frame.h                                               */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/
#ifndef FRAME_H
#define FRAME_H

#include "ial.h"

typedef struct frame_item {
    string key;
    int type;
    t_data_value value;
    bool is_init;
    struct frame_item * next;
}f_item;

typedef struct frame {
    tInstr * nextInstr;
    t_data_value return_value;
    int size;
    f_item * items[];
}f_table;

f_table * frame_table_init();
int frame_table_search(f_table *frame_table,string *key, f_item **item);
int frame_table_insert(f_table *frame_table, string *key, f_item **item);

#endif
