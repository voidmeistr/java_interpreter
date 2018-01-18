/************************** str.h  *******************************************/
/* Subor:               str.h                                                 */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/
#ifndef __STRING_H__
#define __STRING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_ALLOC_ERROR 2
#define STR_ERROR       1
#define STR_OK          0
#define STR_LEN_INIT sizeof(char)


typedef struct
{
    char* str;
    int length;
    int alloc_size;
} string;

void free_str(string *str);

int duplicate_str(string *duplicate, string *source);

int init_value_str(string *str, const char *value_str);

#endif
