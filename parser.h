/************************** parser.h ******************************************/
/* Subor:               parser.h - TODO                                       */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/
#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "ial.h"
#include "stack.h"

enum error_codes {
    E_OK = 0,
    E_LEX = 1,
    E_SYN = 2,
    E_SEM = 3,
    E_TYPE = 4,
    E_SEM_OTHER = 6,
    E_INPUT = 7,
    E_INIT = 8,
    E_ZERO_DIV = 9,
    E_OTHER = 10,
    E_INTERNAL = 99
};

enum context {
    C_GLOBAL = 0,
    C_CLASS = 1,
    C_FUNCTION = 2
};

enum phase {
    P_FIRST = 0,
    P_SECOND = 1
};

/*Structure for context */
//global table - store global table for classes and constants
//class key - key of actual class
//function key - key of actual function
//identifier_item - helper variable for storing definition of variables
//context type - CLASS/FUNCTION
typedef struct curr_context {
    h_table * global_table;
    string * class_key;
    string * function_key;
    t_item * identifier_item;
    enum context context_type;
} GContext;

/********************HELPER FUNCTIONS*****************************/

int check_semicolon();
void throw_error(int code);
void check_main_run();
void create_builtin_ts();
int check_builtin(t_item * item);
void generate_instruction(tInstrList* tList,int type, void * ptr1, void * ptr2, void * ptr3);

/******************************PARSING FUNCTIONS******************/
int parse(int phase);
void parse_class_list();
void parse_class();
void parse_def_list();
void parse_def_prefix();
void parse_def();
void parse_init();
void parse_param_list();
void parse_param_list2();
void parse_param();
void parse_call_param_list(t_item * fun_item);
void parse_call_param_list2(t_item * fun_item);
void parse_stat_list();
void parse_complex_stat();
void parse_stat();
int parse_type();
void check_lex_error();

#endif
