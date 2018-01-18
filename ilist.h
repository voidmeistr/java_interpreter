/************************** ilist.h *******************************************/
/* Subor:               ilist.h                                               */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

/* This is the last stage of the whole project. Purpose of the interpret part
 * is to take queue, containing whole program and execute it. Queue is made as
 * double-linked list (so interpret can jump forth and back when dealing with
 * cycles, conditions and other stream-altering situations) and interpret is
 * implemented as infinite cycle, which is taking one element of list at a time.
 */

/* First and most important part is instruction tape. IT is long list
 * containing all possible commands in three-adress code*.
 *
 * *Three-adress code is code, where every command can have at most 3 adresses,
 * for example command for adding two numbers together have two sources (one
 * for each number) and one destination (for storing result).
 */

#ifndef Ilist
#define Ilist
#include <stdio.h>
#include <stdlib.h>

enum instruction {
    //TODO IS_INIT EVERYWHERE
    //TODO ADD GLOBAL CONVERSIONS
    //numerical instructions, supports int and double variables
            In_ADD,     //Addition, usage: In_ADD dest/src1/src2
    //result: dest = src1 + src2
            In_SUB,     //Substraction, usage: In_SUB dest/src1/src2
    //result: dest = src1 - src2
            In_MUL,     //Multiplication, usage: In_MUL dest/src1/src2
    //result: dest = src1 * src2
            In_DIV,     //Division, usage: In_DIV dest/src1/src2
    //result: dest = src1 / src2

    //TODO CONVERSION TO STRINGS
            In_CON,     //Concatenation of strings, usage: In_CON dest/src/src

    In_ASS,     //Assign
    IN_SET_LAST, //Set value of the last result usage: In_SET_LAST dest/null/null
    //TODO COMPARSIONS:
    //Comparsion instructions, don't support string variables
            In_GT,      //Greater than, usage: In_GT src1/src2 (src1 > src2?)
    In_GTE,      //Greater than or equal, usage: In_GT src1/src2 (src1 > src2?)
    In_LT,      //Lower than, usage: In_LT src1/src2 (src1 < src2?)
    In_LTE,      //Lower than or equal, usage: In_LT src1/src2 (src1 < src2?)
    In_EQ,      //Equal, usage: In_EQ src1/src2 (src1 == src2?)
    In_NEQ,     //Not Equal, usage: In_NEQ src1/src2 (src1 != src2?)

    //Loading instructions
            In_LI,      //Load Integer, usage: In_LI dest/src
    In_LD,      //Load Double, usage: In_LD dest/src
    In_LS,      //Load String, usage:In_LS dest/src

    //TODO JUMPS
    //Stream altering instructions:
            In_JMP,     //Jump, usage: JMP dest
    In_JMPT,    //Jump if True, usage: JMP dest (takes last comparsion result)
    In_JMPF,    //Jump if False, usage: JMP dest (takes last comparsion result)

    //TODO PRINT STRINGS
    //Print instructions
            In_PRT,     //Print, usage: In_PRT src

    //TODO STRING GLOBALS
    //String instructions
            In_STRL,    //String Length, usage: In_STRL dest/src, return value is int
    /*
     * Instruction for substring should be here:

    */
            In_SSTR,   //Substring, hard to choose usage (4 variables in function call)
    In_STRC,    //String compare, usage: In_STRC dest/src/src
    In_STRF,    //String find, usage: In_STRF dest/src1/src2 (src1 is base
    //string, src2 is string searched for)
            In_STRS,     //String sort, usage: In_STRS dest/src

    /*INSTRUCTIONS FOR WORKING WITH FUNCTIONS*/
            In_FUN_CALL, // call function, usage: In_FCL pointer_to_fun_in_table/null/null
    In_SET_PARAM, //set value of parameter, it will be in order, usage: In_SET_PARAM pointer_to_value/null/null
    In_START_FUN, //usage: In_START_FUN pointer_to_f_list/null/null
    In_SET_RESULT, //usage: In_SET_RESULT pointer_to_value/null/null
    In_DESTROY_FRAME, // usage: In_DESTROY_FRAME null/null/null
    In_NONE
};

/* Now we need functions and structures for working with instructions and
 * instruction tape. We need structure for instruction, structure for list item
 * and structure for DL list.
 */

/* Instruction: */
typedef struct {
    int instType;   //instruction code
    void *addr1;    //first address
    void *addr2;    //second address
    void *addr3;    //third address
} tInstr;

/* List item: */
typedef struct tListItem{
    tInstr Instruction;             //instruction
    struct tListItem *nextItem;      //next list item
    struct tListItem *previousItem;  //previous list item
} tListItem;

/* Double-linked List: */
typedef struct{
    tListItem *first;     //pointer to first item in list
    tListItem *last;      //pointer to last item in list
    tListItem *active;    //pointer to active item in list
} tInstrList;

/*********************FUNCTIONS*****************************/

/* Initialization of list: */
void listInit(tInstrList *L);

/* adding instruction to list: */
void listInsertLast(tInstrList *L, tInstr I);

/* Setting active item: */
void listFirst(tInstrList *L);

/* moving active pointer to right: */
void listNext(tInstrList *L);

/* changing active item explicitly: */
void listGoto(tInstrList *L, void *gotoInstr);

/* returning pointer to last instruction: */
void *listGetPointerLast(tInstrList *L);

/* returning pointer to active instruction:
 * in case of nonactive list, function returns NULL
 */
tInstr* listGetData(tInstrList *L);

/* Removing instruction: */
void listRemoveFirst(tInstrList *L);

/* Removing all instructions from list: */
void listFree(tInstrList *L);

void print_elements_of_list(tInstrList TL);
#endif
