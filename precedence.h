/************************** precendence.h *************************************/
/* Subor:               precendence.h                                         */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "stack.h"
#include "ial.h"
#include "scanner.h"
#include "parser.h"
#include "str.h"
#include "ilist.h"
#include "garbage_collector.h"

// items which can contain precedence_table
typedef enum {
    TI_LBRACKET,    // (        (0)
    TI_RBRACKET,    // )        (1)
    TI_PLUS,        // +        (2)
    TI_MINUS,       // -        (3)
    TI_MUL,         // *        (4)
    TI_DIV,         // /        (5)
    TI_LESS,        // <        (6)
    TI_MORE,        // >        (7)
    TI_LESS_EQUAL,  // <=       (8)
    TI_MORE_EQUAL,  // >=       (9)
    TI_EQUAL,       // ==       (10)
    TI_NOT_EQUAL,   // !=       (11)
    TI_BOEOPA,      // $        (12)    Begin or end of precedence analysis
    TI_ID,          // Id       (13)    Identifier
    TI_INT,         // Integer  (14)
    TI_DOUBLE,      // Double   (15)
    TI_STR,         // String   (16)

    TI_FULL_QUALIF_ID,  // Full qualified Identifier
    TI_INST_B,          // starting point of some potential rule
    TI_EXPR,            // expression, which determine some rule
} pt_items;

// item with all information we need in precedence analyse
typedef struct term {
    pt_items typeOfItem;    // item from precedence table
    t_item *term_data;      // item from table of symbols
} terminal;

/*
 * brackets = 0 -> expression doesn't end with bracket
 * brackets = 1 -> expression ends with bracket
 * phase = P_FIRST -> first phase, we don't make instructions
 * phase = P_SECOND -> second phase, we are making instructions
 */
void parseExpression(int phase, tInstrList *IList);

#endif //PRECEDENCE_H
