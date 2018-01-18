/************************** precedence.c **************************************/
/* Subor:               precedence.c                                          */
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
#include <string.h>
#include "precedence.h"
#include "ial.h"
#include "str.h"
#include "ilist.h"

#define PO_MORE 0       // defines ">"
#define PO_LESS 1       // defines "<"
#define PO_EQUAL 2      // defines "="
#define PO_NULL -1      // precedence is not defined

#define PTABLE_SIZE 20

#define DEBUG

// precedence table, static sets everything to PO_MORE -> 0
static short precedence_table[PTABLE_SIZE][PTABLE_SIZE];

// rules which determine action for making an expression
enum rules {
    RULE1,          // 1st rule :   E -> id
    RULE2,          // 2nd rule :   E -> ( E )
    RULE3,          // 3rd rule :   E -> E + E
    RULE4,          // 4th rule :   E -> E - E
    RULE5,          // 5th rule :   E -> E * E
    RULE6,          // 6th rule :   E -> E / E
    RULE7,          // 7th rule :   E -> E < E
    RULE8,          // 8th rule :   E -> E > E
    RULE9,          // 9th rule :   E -> E <= E
    RULE10,         // 10th rule :  E -> E >= E
    RULE11,         // 11th rule :  E -> E == E
    RULE12          // 12th rule :  E -> E != E
};

#define SIZE_OF_RULE_ARRAY 3

//based on this rules, we will be able to detect operations
unsigned int rule2[SIZE_OF_RULE_ARRAY] = {TI_RBRACKET, TI_EXPR, TI_LBRACKET};
unsigned int rule3[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_PLUS, TI_EXPR};
unsigned int rule4[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_MINUS, TI_EXPR};
unsigned int rule5[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_MUL, TI_EXPR};
unsigned int rule6[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_DIV, TI_EXPR};
unsigned int rule7[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_LESS, TI_EXPR};
unsigned int rule8[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_MORE, TI_EXPR};
unsigned int rule9[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_LESS_EQUAL, TI_EXPR};
unsigned int rule10[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_MORE_EQUAL, TI_EXPR};
unsigned int rule11[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_EQUAL, TI_EXPR};
unsigned int rule12[SIZE_OF_RULE_ARRAY] = {TI_EXPR, TI_NOT_EQUAL, TI_EXPR};

extern tToken *token;                           // taking token from parser
extern struct curr_context current_context;     // context which define where we are(in which class, function etc.)


tInstrList *instrList;                         // table with instructions

static int instrCnt;
static int cntForUniqueNames;
static int expectedRBrackets;
static int currentPhase;
t_item *result;                                 // global variable with pointer to a result of precedence analysis
t_item *class, *function;                       /* pointers for class or class and function, so we know where is
                                                   precedence analysis called */

void initPTable() {
    precedence_table[TI_LBRACKET][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_RBRACKET] = PO_EQUAL;
    precedence_table[TI_LBRACKET][TI_PLUS] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_MINUS] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_MUL] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_DIV] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_LESS] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_MORE] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_LESS_EQUAL] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_MORE_EQUAL] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_EQUAL] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_NOT_EQUAL] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_BOEOPA] = PO_NULL;
    precedence_table[TI_LBRACKET][TI_ID] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_INT] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_LBRACKET][TI_STR] = PO_LESS;

    precedence_table[TI_RBRACKET][TI_LBRACKET] = PO_NULL;
    precedence_table[TI_RBRACKET][TI_ID] = PO_NULL;
    precedence_table[TI_RBRACKET][TI_INT] = PO_NULL;
    precedence_table[TI_RBRACKET][TI_DOUBLE] = PO_NULL;
    precedence_table[TI_RBRACKET][TI_STR] = PO_NULL;

    precedence_table[TI_PLUS][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_PLUS][TI_MUL] = PO_LESS;
    precedence_table[TI_PLUS][TI_DIV] = PO_LESS;
    precedence_table[TI_PLUS][TI_ID] = PO_LESS;
    precedence_table[TI_PLUS][TI_INT] = PO_LESS;
    precedence_table[TI_PLUS][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_PLUS][TI_STR] = PO_LESS;

    precedence_table[TI_MINUS][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_MINUS][TI_MUL] = PO_LESS;
    precedence_table[TI_MINUS][TI_DIV] = PO_LESS;
    precedence_table[TI_MINUS][TI_ID] = PO_LESS;
    precedence_table[TI_MINUS][TI_INT] = PO_LESS;
    precedence_table[TI_MINUS][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_MINUS][TI_STR] = PO_LESS;

    precedence_table[TI_MUL][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_MUL][TI_ID] = PO_LESS;
    precedence_table[TI_MUL][TI_INT] = PO_LESS;
    precedence_table[TI_MUL][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_MUL][TI_STR] = PO_LESS;

    precedence_table[TI_DIV][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_DIV][TI_ID] = PO_LESS;
    precedence_table[TI_DIV][TI_INT] = PO_LESS;
    precedence_table[TI_DIV][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_DIV][TI_STR] = PO_LESS;

    precedence_table[TI_LESS][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_LESS][TI_PLUS] = PO_LESS;
    precedence_table[TI_LESS][TI_MINUS] = PO_LESS;
    precedence_table[TI_LESS][TI_MUL] = PO_LESS;
    precedence_table[TI_LESS][TI_DIV] = PO_LESS;
    precedence_table[TI_LESS][TI_INT] = PO_LESS;
    precedence_table[TI_LESS][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_LESS][TI_STR] = PO_LESS;
    precedence_table[TI_LESS][TI_ID] = PO_LESS;

    precedence_table[TI_MORE][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_MORE][TI_PLUS] = PO_LESS;
    precedence_table[TI_MORE][TI_MINUS] = PO_LESS;
    precedence_table[TI_MORE][TI_MUL] = PO_LESS;
    precedence_table[TI_MORE][TI_DIV] = PO_LESS;
    precedence_table[TI_MORE][TI_INT] = PO_LESS;
    precedence_table[TI_MORE][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_MORE][TI_STR] = PO_LESS;
    precedence_table[TI_MORE][TI_ID] = PO_LESS;

    precedence_table[TI_LESS_EQUAL][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_PLUS] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_MINUS] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_MUL] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_DIV] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_INT] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_STR] = PO_LESS;
    precedence_table[TI_LESS_EQUAL][TI_ID] = PO_LESS;

    precedence_table[TI_MORE_EQUAL][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_PLUS] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_MINUS] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_MUL] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_DIV] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_INT] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_STR] = PO_LESS;
    precedence_table[TI_MORE_EQUAL][TI_ID] = PO_LESS;

    precedence_table[TI_EQUAL][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_EQUAL][TI_PLUS] = PO_LESS;
    precedence_table[TI_EQUAL][TI_MINUS] = PO_LESS;
    precedence_table[TI_EQUAL][TI_MUL] = PO_LESS;
    precedence_table[TI_EQUAL][TI_DIV] = PO_LESS;
    precedence_table[TI_EQUAL][TI_LESS] = PO_LESS;
    precedence_table[TI_EQUAL][TI_MORE] = PO_LESS;
    precedence_table[TI_EQUAL][TI_LESS_EQUAL] = PO_LESS;
    precedence_table[TI_EQUAL][TI_MORE_EQUAL] = PO_LESS;
    precedence_table[TI_EQUAL][TI_INT] = PO_LESS;
    precedence_table[TI_EQUAL][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_EQUAL][TI_STR] = PO_LESS;
    precedence_table[TI_EQUAL][TI_ID] = PO_LESS;

    precedence_table[TI_NOT_EQUAL][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_PLUS] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_MINUS] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_MUL] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_DIV] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_LESS] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_MORE] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_LESS_EQUAL] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_MORE_EQUAL] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_INT] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_STR] = PO_LESS;
    precedence_table[TI_NOT_EQUAL][TI_ID] = PO_LESS;

    precedence_table[TI_BOEOPA][TI_LBRACKET] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_RBRACKET] = PO_NULL;
    precedence_table[TI_BOEOPA][TI_PLUS] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_MINUS] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_MUL] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_DIV] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_LESS] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_MORE] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_LESS_EQUAL] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_MORE_EQUAL] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_EQUAL] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_NOT_EQUAL] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_BOEOPA] = PO_NULL;
    precedence_table[TI_BOEOPA][TI_ID] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_INT] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_DOUBLE] = PO_LESS;
    precedence_table[TI_BOEOPA][TI_STR] = PO_LESS;

    precedence_table[TI_ID][TI_LBRACKET] = PO_NULL;
    precedence_table[TI_ID][TI_ID] = PO_NULL;
    precedence_table[TI_ID][TI_INT] = PO_NULL;
    precedence_table[TI_ID][TI_DOUBLE] = PO_NULL;
    precedence_table[TI_ID][TI_STR] = PO_NULL;

    precedence_table[TI_DOUBLE][TI_LBRACKET] = PO_NULL;
    precedence_table[TI_DOUBLE][TI_ID] = PO_NULL;
    precedence_table[TI_DOUBLE][TI_INT] = PO_NULL;
    precedence_table[TI_DOUBLE][TI_DOUBLE] = PO_NULL;
    precedence_table[TI_DOUBLE][TI_STR] = PO_NULL;

    precedence_table[TI_INT][TI_LBRACKET] = PO_NULL;
    precedence_table[TI_INT][TI_ID] = PO_NULL;
    precedence_table[TI_INT][TI_INT] = PO_NULL;
    precedence_table[TI_INT][TI_DOUBLE] = PO_NULL;
    precedence_table[TI_INT][TI_STR] = PO_NULL;

    precedence_table[TI_STR][TI_LBRACKET] = PO_NULL;
    precedence_table[TI_STR][TI_PLUS] = PO_MORE;
    precedence_table[TI_STR][TI_MINUS] = PO_MORE;
    precedence_table[TI_STR][TI_MUL] = PO_MORE;
    precedence_table[TI_STR][TI_DIV] = PO_MORE;
    precedence_table[TI_STR][TI_LESS] = PO_MORE;
    precedence_table[TI_STR][TI_MORE] = PO_MORE;
    precedence_table[TI_STR][TI_LESS_EQUAL] = PO_MORE;
    precedence_table[TI_STR][TI_MORE_EQUAL] = PO_MORE;
    precedence_table[TI_STR][TI_EQUAL] = PO_MORE;
    precedence_table[TI_STR][TI_NOT_EQUAL] = PO_MORE;
    precedence_table[TI_STR][TI_ID] = PO_NULL;
    precedence_table[TI_STR][TI_DOUBLE] = PO_NULL;
    precedence_table[TI_STR][TI_INT] = PO_NULL;
    precedence_table[TI_STR][TI_STR] = PO_NULL;
}

// create some kind of terminal we will need in Stack
terminal *createTerm(pt_items typeOfTerm) {
    terminal *tmp_term = (terminal *) gc_alloc_pointer(sizeof(terminal));

    if (tmp_term == NULL) {
        fprintf(stderr, "Error allocating memory.");
        throw_error(E_INTERNAL);
    }

    tmp_term->typeOfItem = typeOfTerm;
    tmp_term->term_data = NULL;

    return tmp_term;
}

pt_items getType(tToken *term) {
    switch (term->state) {
        case S_LBRACKET:
            expectedRBrackets++;
            return TI_LBRACKET;

        case S_RBRACKET:
            expectedRBrackets--;
            if (expectedRBrackets < 0)
                return TI_BOEOPA;
            else
                return TI_RBRACKET;

        case S_PLUS:
            return TI_PLUS;

        case S_MINUS:
            return TI_MINUS;

        case S_DIV:
            return TI_DIV;

        case S_MULTIPLY:
            return TI_MUL;

        case S_LESS:
            return TI_LESS;

        case S_GREATER:
            return TI_MORE;

        case S_LESSOREQ:
            return TI_LESS_EQUAL;

        case S_GREATEROREQ:
            return TI_MORE_EQUAL;

        case S_EQUAL:
            return TI_EQUAL;

        case S_NOTEQUAL:
            return TI_NOT_EQUAL;

        case S_IDENTIF:
            return TI_ID;

        case S_FULL_IDENTIF:
            return TI_FULL_QUALIF_ID;

        case S_INT:
            return TI_INT;

        case S_DOUBLE:
            return TI_DOUBLE;

        case S_STR:
            return TI_STR;

            // every other type of Token
        default:
            return TI_BOEOPA;
    }
}

// function returns constant only from global table of symbols
t_item *getConstFromTable(string *key, pt_items type_of_const) {
    t_item *const_item = NULL;

    // if we find constant in global table of symbols, we just return it
    if (!hash_table_search(current_context.global_table, key, &const_item)) {
#ifdef DEBUG
        if (const_item->data.value.constant.type == T_INT)
            fprintf(stderr, "We found constant: '%d' (key: '%s')\n", const_item->data.value.constant.value.integer,
                    key->str);
        else if (const_item->data.value.constant.type == T_DOUBLE)
            fprintf(stderr, "We found constant: '%g' (key: '%s')\n",
                    const_item->data.value.constant.value.double_number, key->str);
        else
            fprintf(stderr, "We found constant: '%s' (key: '%s')\n", const_item->data.value.constant.value.str.str,
                    key->str);
#endif
        return const_item;
    }
        // if we din't find constant in table, we have to create one
    else {
        const_item = (t_item *) gc_alloc_pointer(sizeof(t_item));
        if (const_item == NULL) {
            fprintf(stderr, "Error allocating memory.");
            throw_error(E_INTERNAL);
        }

        const_item->key = *key;

        hash_table_insert(current_context.global_table, &const_item->key, &const_item);
        const_item->data.type = CONSTANT;

        if (type_of_const == TI_INT) {
            const_item->data.value.constant.type = T_INT;
            const_item->data.value.constant.value.integer = atoi(key->str);
        } else if (type_of_const == TI_DOUBLE) {
            const_item->data.value.constant.type = T_DOUBLE;
            const_item->data.value.constant.value.double_number = strtod(key->str, NULL);
        } else {
            const_item->data.value.constant.type = T_STR;
            init_value_str(&const_item->data.value.constant.value.str, key->str);
        }

#ifdef DEBUG
        if (const_item->data.value.constant.type == T_INT)
            fprintf(stderr, "We found constant: '%d' (key: '%s')\n", const_item->data.value.constant.value.integer,
                    key->str);
        else if (const_item->data.value.constant.type == T_DOUBLE)
            fprintf(stderr, "We found constant: '%g' (key: '%s')\n",
                    const_item->data.value.constant.value.double_number, key->str);
        else
            fprintf(stderr, "We found constant: '%s' (key: '%s')\n", const_item->data.value.constant.value.str.str,
                    key->str);
#endif
        return const_item;
    }
}

t_item *getIdFromTable(string *identifierKey, string *classKey) {

    t_item *identifier = NULL;
    t_item *someClass = NULL;

    if (currentPhase == P_FIRST) {
        /* variables in static context must be declared earlier in "source code" than accessing them somewhere in current or another class */
        if (current_context.context_type == C_CLASS) {
            /* if class key is not NULL, variable is from another class -> access a.k.a full qualified access */
            if (classKey != NULL) {
                hash_table_search(current_context.global_table, classKey, &someClass);
                hash_table_search(someClass->data.value.class.loc_table, identifierKey, &identifier);
            } else
                hash_table_search(class->data.value.class.loc_table, identifierKey, &identifier);

            if (identifier == NULL) {
                fprintf(stderr, "Variable not find in static context\n");
                throw_error(E_SEM_OTHER);
            }
        }
        return identifier;
    }
    else if (currentPhase == P_SECOND) {
        // if classKey IS NOT NULL, it means, we have to find full qualified identifier
        if (classKey == NULL) {
            switch (current_context.context_type) {
                /* we must finding from 3 different types of table of symbols:
                 * 1st function table of symbols
                 * 2nd class table of symbols
                 * 3rd global table of symbols
                */
                case C_FUNCTION:
                    // finding in function table of symbols
                    hash_table_search(function->data.value.function.loc_table, identifierKey, &identifier);
#ifdef DEBUG
                    fprintf(stderr, "We found identifier in local function table of symbols\n");
#endif
                    if (identifier != NULL)
                        return identifier;

                case C_CLASS:

                    // finding in class table of symbols
                    hash_table_search(class->data.value.class.loc_table, identifierKey, &identifier);
                    if (identifier != NULL)
                        return identifier;

                case C_GLOBAL:
#ifdef DEBUG
                    fprintf(stderr, "Warning: We are finding identifier from global table of symbols!\n");
#endif
                    // finding in global table of symbols
                    hash_table_search(current_context.global_table, identifierKey, &identifier);
                    if (identifier != NULL)
                        return identifier;
            }
        } else {
            hash_table_search(current_context.global_table, classKey, &someClass);
            hash_table_search(someClass->data.value.class.loc_table, identifierKey, &identifier);
            if (identifier != NULL)
                return identifier;
        }

        // if we got here, it means we didn't find identifier in tables of symbols
        fprintf(stderr, "Error while parsing expression: Undefined identifier.\n");
        throw_error(E_SEM);
    }

    return NULL;
}

int compareExpressions(tStack S, unsigned int *rule_array) {
    int resultOfComparison = 0;
    for (int i = 0; i < SIZE_OF_RULE_ARRAY; ++i) {
        if (((terminal *) S->stack_data[i])->typeOfItem != rule_array[i])
            return -1;
    }
    return resultOfComparison;
}

int findRule(tStack S) {
    int rule = -1;

    if (compareExpressions(S, rule2) == 0) return RULE2;
    else if (compareExpressions(S, rule3) == 0) return RULE3;
    else if (compareExpressions(S, rule4) == 0) return RULE4;
    else if (compareExpressions(S, rule5) == 0) return RULE5;
    else if (compareExpressions(S, rule6) == 0) return RULE6;
    else if (compareExpressions(S, rule7) == 0) return RULE7;
    else if (compareExpressions(S, rule8) == 0) return RULE8;
    else if (compareExpressions(S, rule9) == 0) return RULE9;
    else if (compareExpressions(S, rule10) == 0) return RULE10;
    else if (compareExpressions(S, rule11) == 0) return RULE11;
    else if (compareExpressions(S, rule12) == 0) return RULE12;

    return rule;
}

enum types operandIs(terminal *operand) {

    if (operand->term_data->data.type == CONSTANT) {
        if (operand->term_data->data.value.constant.type == T_STR)
            return T_STR;
        else if (operand->term_data->data.value.constant.type == T_DOUBLE)
            return T_DOUBLE;
        else
            return T_INT;
    } else if (operand->term_data->data.type == VARIABLE) {
        if (operand->term_data->data.value.variable.type == T_STR)
            return T_STR;
        else if (operand->term_data->data.value.variable.type == T_DOUBLE)
            return T_DOUBLE;
        else
            return T_INT;
    }
    // Function will never get here, but ... who knows
    return T_INT;
}

char *generateUniqueName(char *string) {
    cntForUniqueNames++;

    // predicting that we will not generate more than 9999 unique names for same string
    int digits;
    if (cntForUniqueNames > 999)
        digits = 4;
    else if (cntForUniqueNames > 99)
        digits = 3;
    else if (cntForUniqueNames > 9)
        digits = 2;
    else
        digits = 1;

    char *cDigits = gc_alloc_pointer(digits + 1);
    sprintf(cDigits, "%d", cntForUniqueNames);

    char *uniqueName = gc_alloc_pointer(strlen(string) + 1 + digits);
    uniqueName[0] = '\0';

    strcat(uniqueName, string);
    strcat(uniqueName, cDigits);

    return uniqueName;
}

void generateMathOperation(tStack S, char *name_for_instruction, enum instruction I) {
    string resultKey;
    terminal *operand1, *operand2, *operator, *destination = NULL;

    // 1st operand
    operand1 = (terminal *) popItem(S);
    // operator
    operator = (terminal *) popItem(S);
    // 2nd operand
    operand2 = (terminal *) popItem(S);

    destination = createTerm(TI_EXPR);

    if (currentPhase == P_SECOND) {
        instrCnt++;

        // changing "Instruction add" to a instruction for concatenation between 2 strings
        if (I == In_ADD && (operandIs(operand1) == T_STR || operandIs(operand2) == T_STR )) {
            name_for_instruction = generateUniqueName("In_CON");
            I = In_CON;
        }

        init_value_str(&resultKey, name_for_instruction);
        resultKey.str = generateUniqueName(resultKey.str);

        // destination/result of operation must be stored in function table of symbols, if precedence analysis
        // is called in function, otherwise in class
        if (function != NULL)
            hash_table_insert(function->data.value.function.loc_table, &resultKey, &destination->term_data);
        else if (class != NULL)
            hash_table_insert(class->data.value.class.loc_table, &resultKey, &destination->term_data);
#ifdef DEBUG
        else {
            fprintf(stderr, "Warning: result of math operation is stored in global table of symbols!\n");
            hash_table_insert(current_context.global_table, &resultKey, &destination->term_data);
        }
#endif

        destination->term_data->key = resultKey;
        destination->term_data->data.type = CONSTANT;

        switch (operator->typeOfItem) {
            case TI_PLUS:
                if (operandIs(operand1) == T_STR) {
                    destination->term_data->data.value.constant.type = T_STR;
                } else if (operandIs(operand2) == T_STR) {
                    destination->term_data->data.value.constant.type = T_STR;
                } else if (operandIs(operand1) == T_DOUBLE || operandIs(operand2) == T_DOUBLE)
                    destination->term_data->data.value.constant.type = T_DOUBLE;

                else
                    destination->term_data->data.value.constant.type = T_INT;
                break;

            case TI_MINUS:
            case TI_MUL:
            case TI_DIV:
                if (operandIs(operand1) == T_STR || operandIs(operand2) == T_STR) {
                    fprintf(stderr, "Error while parsing expression: String cannot be used in math operations.\n");
                    throw_error(E_TYPE);
                } else if (operandIs(operand1) == T_DOUBLE || operandIs(operand2) == T_DOUBLE)
                    destination->term_data->data.value.constant.type = T_DOUBLE;
                else
                    destination->term_data->data.value.constant.type = T_INT;
                break;

            default:
                ;
        }

#ifdef DEBUG
        fprintf(stderr, "name_for_instr: %s\n", name_for_instruction);
#endif

        result = destination->term_data;
        generate_instruction(instrList, I,
                             result,
                             operand1->term_data,
                             operand2->term_data);
    }

    // we add result of operation back to Stack for further operations
    addItem(&S, destination);
}

void generateRelationOperation(tStack S, char *name_for_instruction, enum instruction I) {
    string resultValKey;
    terminal *operand1, *operand2;

    // 1st operand
    operand1 = (terminal *) popItem(S);
    // getting rid of operator
    popItem(S);
    // 2nd operand
    operand2 = (terminal *) popItem(S);

    // based on phase in LL grammar
    if (currentPhase == P_SECOND) {
        instrCnt++;
        
        if (operandIs(operand1) == T_STR || operandIs(operand2) == T_STR) {
            fprintf(stderr, "Error while parsing expression: String cannot be used in relational operations.\n");
            throw_error(E_TYPE);
        }

        init_value_str(&resultValKey, name_for_instruction);
        // result must be stored in function table of symbols, if precedence analysis
        // is called in function, otherwise in class
        if (function != NULL)
            hash_table_insert(function->data.value.function.loc_table, &resultValKey, &result);
        else if (class != NULL)
            hash_table_insert(class->data.value.function.loc_table, &resultValKey, &result);
#ifdef DEBUG
        else {
            fprintf(stderr, "Warning: result of relation operation is stored in global table of symbols!\n");
            hash_table_insert(current_context.global_table, &resultValKey, &result);
        }
#endif

        result->key = resultValKey;
        result->data.type = CONSTANT;
        result->data.value.constant.type = T_BOOL;

        generate_instruction(instrList, I,
                             operand1->term_data,
                             operand2->term_data,
                             NULL);
    }

    // now we shall end Precedence analysis, so as result we put special symbol ( '$' )
    addItem(&S, createTerm(TI_BOEOPA));
}

void makeInstruction(tStack S, int rule) {

    char *instrName;
    switch (rule) {

        // 1st rule : E -> identifier
        case RULE1:
            // we just make Expression from identifier
            ((terminal *) S->stack_data[S->top])->typeOfItem = TI_EXPR;
            result = ((terminal *)S->stack_data[S->top])->term_data;
            if (currentPhase == P_SECOND)
                instrCnt++;
            break;

            // 2nd rule : E -> ( E )
        case RULE2:

            // pop out R_Bracket
            popItem(S);
            // saving expression
            terminal *expression = (terminal *) popItem(S);
            // pop out L_Bracket
            popItem(S);
            // and adding Expression back to Stack
            addItem(&S, expression);
            break;

            // 3rd rule : E -> E + E
        case RULE3:
            instrName = generateUniqueName("In_ADD");
            generateMathOperation(S, instrName, In_ADD);
            break;

            // 4th rule : E -> E - E
        case RULE4:
            instrName = generateUniqueName("In_SUB");
            generateMathOperation(S, instrName, In_SUB);
            break;

            // 5th rule : E -> E * E
        case RULE5:
            instrName = generateUniqueName("In_MUL");
            generateMathOperation(S, instrName, In_MUL);
            break;

            // 6th rule : E -> E / E
        case RULE6:
            instrName = generateUniqueName("In_DIV");
            generateMathOperation(S, instrName, In_DIV);
            instrCnt++;
            break;

            // 7th rule : E -> E < E
        case RULE7:
            instrName = generateUniqueName("In_LT");
            generateRelationOperation(S, instrName, In_LT);
            break;

            // 8th rule : E -> E > E
        case RULE8:
            instrName = generateUniqueName("In_GT");
            generateRelationOperation(S, instrName, In_GT);
            break;

            // 9th rule : E -> E <= E
        case RULE9:
            instrName = generateUniqueName("In_LTE");
            generateRelationOperation(S, instrName, In_LTE);
            break;

            // 10th rule : E -> E >= E
        case RULE10:
            instrName = generateUniqueName("In_GTE");
            generateRelationOperation(S, instrName, In_GTE);
            break;

            // 11th rule : E -> E == E
        case RULE11:
            instrName = generateUniqueName("In_EQ");
            generateRelationOperation(S, instrName, In_EQ);
            break;

            // 12th rule : E -> E != E
        case RULE12:
            instrName = generateUniqueName("In_NEQ");
            generateRelationOperation(S, instrName, In_NEQ);
            break;

        default:;
    }
}

void makeExpression(tStack S) {
    tStack tmp = createStack();         // temporary Stack

    // moving terminals from Main Stack to Temporary Stack until we find "Begin of Instruction" terminal
    while (((terminal *) S->stack_data[S->top])->typeOfItem != TI_INST_B) {
        addItem(&tmp, popItem(S));
        if (tmp->top > 2) {
            fprintf(stderr, "Internal error while parsing expression.\n");
            throw_error(E_INTERNAL);
        }
    }

    // clearing temporary begin_of_instruction terminal, which is on top of the Stack
    if (((terminal *) S->stack_data[S->top])->typeOfItem == TI_INST_B)
        popItem(S);
    else {
        fprintf(stderr, "Internal error while parsing expression.\n");
        throw_error(E_INTERNAL);
    }

    switch (((terminal *) tmp->stack_data[tmp->top])->typeOfItem) {
        // if we have on top only one of these tokens, then we are applying only 1st rule
        case TI_INT:
        case TI_DOUBLE:
        case TI_STR:
        case TI_ID:
            // just for sure, that we have only 1 item in Stack
            if (tmp->top < 1) {
                // making Expression from item we have in temporary Stack
                makeInstruction(tmp, RULE1);
                // adding this Expression back Main Stack
                addItem(&S, popItem(tmp));
            } else {
                fprintf(stderr, "We don't have rule for that.\n");
                throw_error(E_SYN);
            }

            break;

            // case when we must have 3 items in stack in order to make some expression -> instruction
        case TI_EXPR:
        case TI_LBRACKET:
            // making sure, that we have 3 items in Stack
            if (tmp->top == 2) {
                int rule;
                // if we find rule
                if ((rule = findRule(tmp)) == -1) {
                    fprintf(stderr, "We don't have rule for that.\n");
                    throw_error(E_SYN);
                }
#ifdef DEBUG
                fprintf(stderr, "We found rule: %d\n", rule);
#endif
                // we apply rule and make Instruction
                makeInstruction(tmp, rule);
                // adding result of expression into Main Stack
                addItem(&S, popItem(tmp));
                break;
            } else {
                fprintf(stderr, "We don't have rule for that.\n");
                throw_error(E_SYN);
            }

        default:
            fprintf(stderr, "We don't have rule for that.\n");
            throw_error(E_SYN);
    }
}

terminal *getTerminal() {
    token = getToken();     // getting token from scanner
    terminal *term;         // definition of terminal for identifier or constant
    string key, classKey;   // structure for key in identifiers or constants
    int error;
    char *class, *item_tmp;

    // converting State of token, so we can compare types of terminals with precedence table
    pt_items typeOfTerminal = getType(token);
#ifdef DEBUG
    fprintf(stderr, "GetToken(): %s\n", token->data);
#endif

    // we must get Data from some table of symbols
    switch (typeOfTerminal) {
        // now we know we must handle Constant in expression
        case TI_INT:
        case TI_DOUBLE:
        case TI_STR:
            // creating Terminal for particular type of Constant
            term = createTerm(typeOfTerminal);

            // converting token->data to string structure
            if ((error = init_value_str(&key, token->data)) == STR_ALLOC_ERROR || error == STR_ERROR) {
                fprintf(stderr, "Error allocating memory.\n");
                throw_error(E_INTERNAL);
            }

            term->term_data = getConstFromTable(&key, typeOfTerminal);
            return term;

        // here we know we must handle (full qualified) identifier, so we will find him in table
        case TI_ID:
            // creating terminal for particular identifier
            term = createTerm(TI_ID);

            // converting token->data to string structure
            if ((error = init_value_str(&key, token->data)) == STR_ALLOC_ERROR || error == STR_ERROR) {
                fprintf(stderr, "Error creating String.\n");
                throw_error(E_INTERNAL);
            }

            term->term_data = getIdFromTable(&key, NULL);
            return term;

        case TI_FULL_QUALIF_ID:
            // creating terminal for particular identifier
            term = createTerm(TI_ID);

            // converting FULL_QUALIFIED_ID into 2 strings
            int x = 0;
            // class
            class = gc_alloc_pointer(token->size);
            while (token->data[x] != '.') {
                class[x] = token->data[x];
                x++;
            }
            class[x] = '\0';
            x++;
            // identifier
            item_tmp = gc_alloc_pointer(token->size);
            int y = 0;
            while (token->data[x] != '\0') {
                item_tmp[y] = token->data[x];
                x++;
                y++;
            }
            item_tmp[y] = '\0';

            if ((error = init_value_str(&classKey, class)) == STR_ALLOC_ERROR || error == STR_ERROR) {
                fprintf(stderr, "Error creating String.\n");
                throw_error(E_INTERNAL);
            }

            if ((error = init_value_str(&key, item_tmp)) == STR_ALLOC_ERROR || error == STR_ERROR) {
                fprintf(stderr, "Error creating String.\n");
                throw_error(E_INTERNAL);
            }

            term->term_data = getIdFromTable(&key, &classKey);
            return term;

        case TI_LBRACKET:
            return createTerm(TI_LBRACKET);

        case TI_RBRACKET:
            return createTerm(TI_RBRACKET);

        case TI_PLUS:
            return createTerm(TI_PLUS);

        case TI_MINUS:
            return createTerm(TI_MINUS);

        case TI_MUL:
            return createTerm(TI_MUL);

        case TI_DIV:
            return createTerm(TI_DIV);

        case TI_LESS:
            return createTerm(TI_LESS);

        case TI_MORE:
            return createTerm(TI_MORE);

        case TI_LESS_EQUAL:
            return createTerm(TI_LESS_EQUAL);

        case TI_MORE_EQUAL:
            return createTerm(TI_MORE_EQUAL);

        case TI_EQUAL:
            return createTerm(TI_EQUAL);

        case TI_NOT_EQUAL:
            return createTerm(TI_NOT_EQUAL);

        default:
            return createTerm(TI_BOEOPA);
    }
}

#ifdef DEBUG

void printStack(tStack S) {
    fprintf(stderr, "Stack: [");
    for (int i = 0; i <= S->top; ++i) {
        switch (((terminal *) S->stack_data[i])->typeOfItem) {
            case TI_INT:
                fprintf(stderr, " int ");
                break;
            case TI_DOUBLE:
                fprintf(stderr, " double ");
                break;
            case TI_STR:
                fprintf(stderr, " string ");
                break;
            case TI_ID:
                fprintf(stderr, " Id ");
                break;
            case TI_EXPR:
                fprintf(stderr, " E ");
                break;
            case TI_BOEOPA:
                fprintf(stderr, " $ ");
                break;
            case TI_LBRACKET:
                fprintf(stderr, " ( ");
                break;
            case TI_RBRACKET:
                fprintf(stderr, " ) ");
                break;
            case TI_PLUS:
                fprintf(stderr, " + ");
                break;
            case TI_MINUS:
                fprintf(stderr, " - ");
                break;
            case TI_MUL:
                fprintf(stderr, " * ");
                break;
            case TI_DIV:
                fprintf(stderr, " / ");
                break;
            case TI_LESS:
                fprintf(stderr, " < ");
                break;
            case TI_MORE:
                fprintf(stderr, " > ");
                break;
            case TI_LESS_EQUAL:
                fprintf(stderr, " <= ");
                break;
            case TI_MORE_EQUAL:
                fprintf(stderr, " >= ");
                break;
            case TI_EQUAL:
                fprintf(stderr, " == ");
                break;
            case TI_NOT_EQUAL:
                fprintf(stderr, " != ");
                break;
            case TI_INST_B:
                fprintf(stderr, " '<' ");
                break;

            default:
                fprintf(stderr, " NOT IMPLEMENTED ");
        }
    }
    fprintf(stderr, "]\n");
}

#endif

void parseExpression(int phase, tInstrList *IList) {
#ifdef DEBUG
    fprintf(stderr, "\nSTART OF PRECEDENCE\n");
#endif

    tStack Stack = createStack();       // stack_data for all operands and operators we must handle

    terminal *current_term;             // current terminal which helps us better handle analysis in Stack
    terminal *tmp_term;                 // temporary terminal (like: $ or '<' )
    int result_of_comparison;           // comparison between 2 items from precedence table

    // initialising result of Expression and pointers to a particular tables
    result = NULL;
    class = NULL;
    function = NULL;
    instrCnt = 0;
    expectedRBrackets = 0;

    // some information we need from LL grammar
    currentPhase = phase;
    instrList = IList;

    // searching for tables for class or class + function, based on where is precedence analysis called
    if (current_context.context_type == C_FUNCTION) {
        hash_table_search(current_context.global_table, current_context.class_key, &class);
        hash_table_search(class->data.value.class.loc_table, current_context.function_key, &function);
    } else {
        hash_table_search(current_context.global_table, current_context.class_key, &class);
    }

    if (currentPhase == P_SECOND && current_context.context_type == C_CLASS) {
        fprintf(stderr, "Class key: %s\n", class->key.str);
    }

    //fprintf(stderr, "Function key: %s\n",function->key.str);

    // creating Begin_Of_Stack terminal
    tmp_term = createTerm(TI_BOEOPA);

    // adding BoS_terminal at the beginning of Stack
    addItem(&Stack, tmp_term);

    // getting first terminal
    current_term = getTerminal();

    // if we get empty Expression
    if (current_term->typeOfItem == TI_RBRACKET || current_term->typeOfItem == TI_BOEOPA)
        return;

#ifdef DEBUG
    fprintf(stderr, "First terminal has token: '%s'\n", token->data);
#endif

    // fill in the remaining data about precedence of operators
    initPTable();

    while (((terminal *) Stack->stack_data[Stack->top])->typeOfItem != TI_BOEOPA ||
           current_term->typeOfItem != TI_BOEOPA) {

        // Getting rid of Expression for a while
        tStack tmpStack = createStack();
        while (((terminal *) Stack->stack_data[Stack->top])->typeOfItem == TI_EXPR)
            addItem(&tmpStack, popItem(Stack));

        if (((terminal *) Stack->stack_data[Stack->top])->typeOfItem != TI_BOEOPA ||
            current_term->typeOfItem != TI_BOEOPA)
            // comparing top of the Stack with latest loaded terminal via precedence table
            result_of_comparison = precedence_table[((terminal *) Stack->stack_data[Stack->top])->typeOfItem][current_term->typeOfItem];
        else {
#ifdef DEBUG
            fprintf(stderr, "\nEND OF PRECEDENCE\n");
#endif
            if (currentPhase == P_SECOND && instrCnt == 1)
                generate_instruction(instrList, IN_SET_LAST, result, NULL, NULL);
            return;
        }

#ifdef DEBUG
        fprintf(stderr, "Comparing types %d:%d\n", ((terminal *) Stack->stack_data[Stack->top])->typeOfItem,
                current_term->typeOfItem);
#endif

        // Putting Expression back to Main Stack
        while (tmpStack->top >= 0)
            addItem(&Stack, popItem(tmpStack));

        switch (result_of_comparison) {
            case PO_EQUAL:
                // adding terminal into Stack
                addItem(&Stack, current_term);

                // and loading another terminal
                current_term = getTerminal();

                break;

            case PO_LESS:
                // creating potential Begin_of_Instruction terminal to Stack
                tmp_term = createTerm(TI_INST_B);

                // Getting rid of Expression for a while
                tStack tmppStack = createStack();
                while (((terminal *) Stack->stack_data[Stack->top])->typeOfItem == TI_EXPR)
                    addItem(&tmppStack, popItem(Stack));

                // adding Begin_of_instruction to Stack
                addItem(&Stack, tmp_term);

                // Putting Expression back to Stack
                while (tmppStack->top >= 0)
                    addItem(&Stack, popItem(tmppStack));

                // adding terminal into Stack
                addItem(&Stack, current_term);

                // and loading another terminal
                current_term = getTerminal();

                break;

            case PO_MORE:
                // we make expression based on rule and items in Stack
                makeExpression(Stack);

                break;

            case PO_NULL:
                fprintf(stderr, "Error while parsing expression.\n");
                throw_error(E_SYN);
        }
#ifdef DEBUG
        printStack(Stack);
#endif
    }

#ifdef DEBUG
    fprintf(stderr, "\nEND OF PRECEDENCE\n");
#endif
}

