/************************** interpret.c ***************************************/
/* Subor:               interpret.c - TODO                                    */
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

/* First and most important part is instruction tape. And IT is long list
 * containing all possible commands in three-adress code*.
 *
 * *Three-adress code is code, where every command can have at most 3 adresses,
 * for example command for adding two numbers together have two sources (one
 * for each number) and one destination (for storing result).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "ilist.h"
#include "ial.h"
#include "string.h"
#include "str.h"
#include "frame.h"
#include "garbage_collector.h"
#include "parser.h"

/***********MACROS*****************/
/*for readability of code */
#define dn double_number
#define in integer
#define var data.value.variable
#define con data.value.constant
#define CHUNK 400 //for loading input strings.
#define DEBUG

/*removing "magic numbers" from code: */
#define Integer 1
#define Double_number 2

#define division 0
#define multiplication 1
#define substraction 2
#define addition 3

/* Function for implicit conversion of variables: */
int conversions(f_item *src1, f_item *src2){
    if(src1->type == T_INT && src2->type == T_INT){
        return Integer;
    }
    if(src1->type == T_DOUBLE && src2->type == T_DOUBLE){
        return Double_number;
    }else if(src1->type == T_INT){
        src1->value.dn = src1->value.in;
        src1->type = T_DOUBLE;
    } else{
        src2->value.dn = src2->value.dn;
        src2->type = T_DOUBLE;
    }
    return Double_number;
}

int StatConversions(t_item *src1, t_item *src2){
    if(src1->var.type == T_INT && src2->var.type == T_INT){
        return Integer;
    }
    if(src1->var.type == T_DOUBLE && src2->var.type == T_DOUBLE){
        return Double_number;
    }else if(src1->var.type == T_INT){
        src1->var.value.dn = src1->var.value.in;
        src1->var.type = T_DOUBLE;
    } else{
        src2->var.value.dn = src2->var.value.dn;
        src2->var.type = T_DOUBLE;
    }
    return Double_number;
}

/* Searching correct item in frame: */
f_item *frame_search(t_item *orig, f_table *table){
    int ret = 0;
    f_item *Ptr = NULL;
    ret = frame_table_search(table, &(orig->key),&Ptr);

    if(ret == HASH_SEARCH_FAIL){
        return NULL;
    }
    return Ptr;
}

void remap_param_list(tInstrList * list, f_table * frame) {
    tListItem* tmp = list->first;
    while (tmp != NULL) {
        if(tmp->Instruction.instType == In_SET_PARAM) {

            f_item * op1 = (f_item *) frame_search(tmp->Instruction.addr1,frame);
            if(op1 != NULL) {
                tmp->Instruction.addr1 = op1;
            } else {
                f_item * item = NULL;
                item = gc_alloc_pointer(sizeof(f_item));
                t_item * titem = tmp->Instruction.addr1;
                switch(titem->data.type) {
                    case CONSTANT:
                        item->type = titem->data.value.constant.type;
                        item->is_init = true;
                        item->key = titem->key;
                        switch(titem->data.value.constant.type) {
                            case T_INT:
                                item->value.integer = titem->data.value.constant.value.integer;
                                break;
                            case T_DOUBLE:
                                item->value.double_number = titem->data.value.constant.value.double_number;
                                break;
                            case T_STR:
                                item->value.str = titem->data.value.constant.value.str;
                                break;
                            default:
                                break;
                        }
                        break;
                    case VARIABLE:
                        item->type = titem->data.value.constant.type;
                        item->is_init = titem->data.value.variable.is_init;
                        item->key = titem->key;
                        switch(titem->data.value.variable.type) {
                            case T_INT:
                                item->value.integer = titem->data.value.variable.value.integer;
                                break;
                            case T_DOUBLE:
                                item->value.double_number = titem->data.value.variable.value.double_number;
                                break;
                            case T_STR:
                                item->value.str = titem->data.value.variable.value.str;
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }

                tmp->Instruction.addr1 = item;
            }
        }
        tmp = tmp->nextItem;
    }
}

/* Function for creating frame (frames are used in called functions) */
void create_frame_from_table(f_table ** frame, h_table * symbol) {
    *frame = frame_table_init();
    for(int i = 0; i < HASH_TABLE_SIZE; i++) {
//        frame->items[i] = gc_alloc_pointer(sizeof(f_item));
        if (symbol->items[i] != NULL) {
            f_item * newItem;
            newItem = gc_alloc_pointer(sizeof(f_item));

            fprintf(stderr, "TYPE of item: %d\n", symbol->items[i]->data.type);
            switch (symbol->items[i]->data.type) {
                case VARIABLE:
                    newItem->key = symbol->items[i]->key;
                    frame_table_insert(*frame, &newItem->key, &newItem);
                    newItem->key = symbol->items[i]->key;
                    newItem->value = symbol->items[i]->var.value;
                    newItem->is_init = symbol->items[i]->var.is_init;
                    newItem->type = symbol->items[i]->var.type;
                    newItem->next = NULL;

                    break;
                case CONSTANT:
                    newItem->key = symbol->items[i]->key;
                    frame_table_insert(*frame, &newItem->key, &newItem);
                    newItem->value = symbol->items[i]->con.value;
                    newItem->type = symbol->items[i]->con.type;
                    newItem->is_init = true;
                    newItem->next = NULL;
                    break;
                default:
                    break;
            }

            t_item * tmp_t = symbol->items[i]->next;

            while(tmp_t != NULL) {
                f_item * newItem;
                newItem = gc_alloc_pointer(sizeof(f_item));

                switch (tmp_t->data.type) {
                    case VARIABLE:
                        newItem->key = tmp_t->key;
                        frame_table_insert(*frame, &newItem->key, &newItem);
                        newItem->value = tmp_t->var.value;
                        newItem->is_init = tmp_t->var.is_init;
                        newItem->type = tmp_t->var.type;
                        newItem->next = NULL;
                        break;

                    case CONSTANT:
                        newItem->key = tmp_t->key;
                        frame_table_insert(*frame, &newItem->key, &newItem);
                        newItem->value = tmp_t->con.value;
                        newItem->type = tmp_t->con.type;
                        newItem->is_init = true;
                        newItem->next = NULL;
                        break;
                    default:
                        break;
                }
                tmp_t = tmp_t->next;
            }
        }
    }
}

/*********************SUBSTRING***************************************/
int substring(string *s, int begin_index, int n, string *temp_dest) {
    if ( (begin_index < 0) || ((begin_index + n) > (int) strlen(s->str)) || n < 0) {
        return E_OTHER;
    }
    else {
        temp_dest->str = gc_alloc_pointer(n+1);
        for (int i = begin_index; i < (begin_index + n); i++) {
            temp_dest->str[i-begin_index] = s->str[i];
        }
        temp_dest->str[n] = '\0';
        temp_dest->length = strlen(temp_dest->str);
        temp_dest->alloc_size = temp_dest->length + 1;
        return 0;
    }
}

/* Function for setting static variables, constants and so on: */
int StaticInterpret(tInstrList *code){
    /*******************VARIABLES:******************************************/

    tInstr *Instr;
    //int ret;
    int tmplen = 0;
    char cPom[CHUNK];
    t_item *op1;
    t_item *op2;
    t_item *op3;
    //t_item *last;
    int mathOperation = 0;
    
    int iOp1 = 0;
    int iOp2 = 0;
    int iOp3 = 0;
    double dOp1 = 0;
    double dOp2 = 0;
    double dOp3 = 0;
    int tOp1 = 0;
    int tOp2 = 0;
    int tOp3 = 0;
    string sOp1;
    string sOp2;
    string sOp3;
    bool initOp1 = false;
    bool initOp2 = false;
    bool initOp3 = false;

    int last_int = 0;
    double last_double = 0;
    string last_string;
    /*********************************CODE:********************************/

    /*First thing is to set active item in instruction tape:*/
    listFirst(code);

    /* Next is while cycle, cycling until "active" pointer points to NULL: */
    while(code->active != NULL){

        /* reseting internal variables: */
        mathOperation = 0;
        iOp1 = 0;
        iOp2 = 0;
        iOp3 = 0;
        dOp1 = 0;
        dOp2 = 0;
        dOp3 = 0;
        tOp1 = 0;
        tOp2 = 0;
        tOp3 = 0;
        sOp1.str = NULL;
        sOp1.length = 0;
        sOp1.alloc_size = 0;
        sOp2.str = NULL;
        sOp2.length = 0;
        sOp2.alloc_size = 0;
        sOp3.str = NULL;
        sOp3.length = 0;
        sOp3.alloc_size = 0;
        initOp1 = false;
        initOp2 = false;
        initOp3 = false;

        /* Copying active instruction for easier work: */
        Instr = listGetData(code);

        /* And copying addresses of operands: */
        op1 = (t_item *) Instr->addr1;
        op2 = (t_item *) Instr->addr2;
        op3 = (t_item *) Instr->addr3;

        /* Copying first operand: */
        if(op1 != NULL){
            if(op1->data.type == CONSTANT){
                if(op1->con.type == T_INT){
                    iOp1 = op1->con.value.in;
                    tOp1 = T_INT;
                }else if(op1->con.type == T_DOUBLE){
                    dOp1 = op1->con.value.dn;
                    tOp1 = T_DOUBLE;
                }else{
                    sOp1 = op1->con.value.str;
                    tOp1 = T_STR;
                }
                initOp1 = true;
            }else{
                if(op1->var.is_init){
                    initOp1 = true;
                }
                if(op1->var.type == T_INT){
                    iOp1 = op1->var.value.in;
                    tOp1 = T_INT;
                }else if(op1->var.type == T_DOUBLE){
                    dOp1 = op1->var.value.dn;
                    tOp1 = T_DOUBLE;
                }else{
                    sOp1 = op1->var.value.str;
                    tOp1 = T_STR;
                }
            }
        }
        
        /* Second operand: */
        if(op2 != NULL){
            if(op2->data.type == CONSTANT){
                if(op2->con.type == T_INT){
                    iOp2 = op2->con.value.in;
                    tOp2 = T_INT;
                }else if(op2->con.type == T_DOUBLE){
                    dOp2 = op2->con.value.dn;
                    tOp2 = T_DOUBLE;
                }else{
                    sOp2 = op2->con.value.str;
                    tOp2 = T_STR;
                }
                initOp2 = true;
            }else{
                if(op2->var.is_init){
                    initOp2 = true;
                }
                if(op2->var.type == T_INT){
                    iOp2 = op2->var.value.in;
                    tOp2 = T_INT;
                }else if(op2->var.type == T_DOUBLE){
                    dOp2 = op2->var.value.dn;
                    tOp2 = T_DOUBLE;
                }else{
                    sOp2 = op2->var.value.str;
                    tOp2 = T_STR;
                }
            }
        }

        /*Third operand: */
        if(op3 != NULL){
            if(op3->data.type == CONSTANT){
                if(op3->con.type == T_INT){
                    iOp3 = op3->con.value.in;
                    tOp3 = T_INT;
                }else if(op3->con.type == T_DOUBLE){
                    dOp3 = op3->con.value.dn;
                    tOp3 = T_DOUBLE;
                }else{
                    sOp3 = op3->con.value.str;
                    tOp3 = T_STR;
                }
                initOp3 = true;
            }else{
                if(op3->var.is_init){
                    initOp3 = true;
                }
                if(op3->var.type == T_INT){
                    iOp3 = op3->var.value.in;
                    tOp3 = T_INT;
                }else if(op3->var.type == T_DOUBLE){
                    dOp3 = op3->var.value.dn;
                    tOp3 = T_DOUBLE;
                }else{
                    sOp3 = op3->var.value.str;
                    tOp3 = T_STR;
                }
            }
        }

        /* Every item is some instruction which needs to be executed. Switch
         * is best for that (lot of instructions, if/elseif/else would be just
         * ugly).
         */
        switch(Instr->instType){

            /* Adding two numbers together: */
            case In_ADD:
                mathOperation++; //incrementing of variable, look into In_DIV

                /* Subtracting one number from the other: */
            case In_SUB:
                mathOperation++; //incrementing of variable, look into In_DIV

                /* Multiplication of two numbers: */
            case In_MUL:
                mathOperation++; //incrementing of variable, look into In_DIV

                /* Dividing one number by the second: */
            case In_DIV:
                /* mathOperation can be 0,1,2 or 3. 0 equals to Division, 1
                 * equals to Multiplication, 2 to Substraction and 3 to
                 * Addition. This si result of function of switch block and
                 * break options. Reason for this kind of construction is
                 * removing duplicite code and making easier debugging (in case
                 * of a bug is there only 1 place where is repair needed, not 4).
                 */

                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp3 == T_DOUBLE && mathOperation == division){
                    if(dOp3 == 0) {return E_ZERO_DIV;}
                }else if(tOp3 == T_INT && mathOperation == division){
                    if(iOp3 == 0) {return E_ZERO_DIV;}
                }
                /* Making mathematic operation: */
                if(tOp2 == T_DOUBLE && tOp3 == T_DOUBLE){
                    switch(mathOperation){
                        case division:
                            last_double = dOp2 / dOp3;
                            break;
                        case multiplication:
                            last_double = dOp2 * dOp3;
                            break;
                        case substraction:
                            last_double = dOp2 - dOp3;
                            break;
                        case addition:
                            last_double = dOp2 + dOp3;
                            break;
                        default:
                            return E_INTERNAL;
                            break;
                    }
                }else if (tOp2 == T_DOUBLE && tOp3 == T_INT){
                    switch(mathOperation){
                        case division:
                            last_double = dOp2 / iOp3;
                            break;
                        case multiplication:
                            last_double = dOp2 * iOp3;
                            break;
                        case substraction:
                            last_double = dOp2 - iOp3;
                            break;
                        case addition:
                            last_double = dOp2 + iOp3;
                            break;
                        default:
                            return E_INTERNAL;
                            break;
                    }
                }else if (tOp2 == T_INT && tOp3 == T_DOUBLE){
                    switch(mathOperation){
                        case division:
                            last_double = iOp2 / dOp3;
                            break;
                        case multiplication:
                            last_double = iOp2 * dOp3;
                            break;
                        case substraction:
                            last_double = iOp2 - dOp3;
                            break;
                        case addition:
                            last_double = iOp2 + dOp3;
                            break;
                        default:
                            return E_INTERNAL;
                            break;
                    }
                }else {
                    switch(mathOperation){
                        case division:
                            last_int = iOp2 / iOp3;
                            break;
                        case multiplication:
                            last_int = iOp2 * iOp3;
                            break;
                        case substraction:
                            last_int = iOp2 - iOp3;
                            break;
                        case addition:
                            last_int = iOp2 + iOp3;
                            break;
                        default:
                            return E_INTERNAL;
                            break;
                    }
                }

                /* Saving result into destination: */
                if(op1->data.type == CONSTANT){
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        op1->con.value.dn = last_double;
                        op1->con.type = T_DOUBLE;
                    }else{
                        op1->con.value.in = last_int;
                        op1->con.type = T_INT;
                    }
                }else{
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        op1->var.value.dn = last_double;
                        op1->var.type = T_DOUBLE;
                    }else{
                        op1->var.value.in = last_int;
                        op1->var.type = T_INT;
                    }
                }
                break;

            case In_ASS:
                if(op1->data.type == CONSTANT){
                    if(tOp1 == T_INT){
                        op1->con.value.in = last_int;
                        op1->con.type = T_INT;
                    }else if(tOp1 == T_DOUBLE){
                        op1->con.value.dn = last_double;
                        op1->con.type = T_DOUBLE;
                    }else{
                        op1->con.value.str = last_string;
                        op1->con.type = T_STR;
                    }
                }else{
                    if(tOp1 == T_INT){
                        op1->var.value.in = last_int;
                        op1->var.type = T_INT;
                        op1->var.is_init = true;
                    }else if(tOp1 == T_DOUBLE){
                        op1->var.value.dn = last_double;
                        op1->var.type = T_DOUBLE;
                        op1->var.is_init = true;
                    }else{
                        op1->var.value.str = last_string;
                        op1->var.type = T_STR;
                        op1->var.is_init = true;
                    }
                }
                break;

                /* Concatenation of two strings together: */
            case In_CON:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_CON\n");
#endif
                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp2 != T_STR){
                    if(tOp2 == T_INT){
                        //INTEGER
                        snprintf(cPom,CHUNK,"%d",iOp2);
                        sOp2.str = cPom;
                        sOp2.length = strlen(cPom);
                    }else if(tOp2 == T_DOUBLE){
                        //DOUBLE
                        snprintf(cPom,CHUNK,"%g",dOp2);
                        sOp2.str = cPom;
                        sOp2.length = strlen(cPom);
                    }else {return E_INTERNAL;}
                }else if(tOp3 != T_STR){
                    if(tOp3 == T_INT){
                        //INTEGER
                        snprintf(cPom,CHUNK,"%d",iOp3);
                        sOp3.str = cPom;
                        sOp3.length = strlen(cPom);
                    }else if(tOp3 == T_DOUBLE){
                        //DOUBLE
                        snprintf(cPom,CHUNK,"%g",dOp3);
                        sOp3.str = cPom;
                        sOp3.length = strlen(cPom);
                    }else {return E_INTERNAL;}
                }
                tmplen = sOp2.length + sOp3.length;
                if(op1->data.type == CONSTANT){
                    op1->con.value.str.str =
                        gc_realloc_pointer(op1->con.value.str.str,
                                sizeof(char) * tmplen+1);
                    strcat(op1->con.value.str.str, sOp2.str);
                    strcat(op1->con.value.str.str, sOp3.str);
                    op1->con.value.str.length = tmplen;
                    op1->con.value.str.alloc_size = tmplen +1;
                    op1->con.type = T_STR;
                    last_string = op1->con.value.str;
                }else{
                    op1->var.value.str.str =
                        gc_realloc_pointer(op1->var.value.str.str,
                                sizeof(char) * tmplen+1);
                    strcat(op1->var.value.str.str, sOp2.str);
                    strcat(op1->var.value.str.str, sOp3.str);
                    op1->var.value.str.length = tmplen;
                    op1->var.value.str.alloc_size = tmplen +1;
                    op1->var.type = T_STR;
                    last_string = op1->var.value.str;
                }
                break;

            case IN_SET_LAST:
                if(initOp1 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT){
                    last_int = iOp1;
                }else if(tOp1 == T_DOUBLE){
                    last_double = dOp1;
                }else{
                    last_string = sOp1;
                }
                break;

            default:
                return E_INTERNAL;
                break;
        }
        listNext(code);
    }
    //listFree(code);
    return 0; //Should be E_OK, but... not defined
}

int last_int = 0;
double last_double = 0;
string last_string;

/* Main function: */
int Interpret(tInstrList *code, f_table *frame){
#ifdef DEBUG
    fprintf(stderr, "Start interpreting\n");
#endif
    /*******************VARIABLES:******************************************/

    tInstr *Instr;
    int ret = -1;
    int cmp = -1;
    char cPom[CHUNK];
    f_item *op1 = NULL;
    f_item *op2 = NULL;
    f_item *op3 = NULL;

    /* Hromada proměnnejch, abych se nemusel srát s globálníma věcma uvnitř funkci:
     * vždycky písmeno druh (i - integer, d - double, s - string), pak "Op" jako
     * operátor a číslo operátoru. Pak jsou tOp1/2/3 - to je "typ Operátoru 1/2/3"
     * může být T_INT, T_DOUBLE, T_STR. initOp1/2/3 je prostě bool value, jestli je
     * proměnná identifikovaná. Všechny tyhe proměnný jsou v každém kroku cyklu
     * resetovaný na výchozí hodnoty a nastavují se při loadingu operátorů.
     */
    int iOp1 = 0;
    int iOp2 = 0;
    int iOp3 = 0;
    double dOp1 = 0;
    double dOp2 = 0;
    double dOp3 = 0;
    string sOp1;
    sOp1.str = NULL;
    sOp1.length = 0;
    sOp1.alloc_size = 0;
    string sOp2;
    sOp2.str = NULL;
    sOp2.length = 0;
    sOp2.alloc_size = 0;
    string sOp3;
    sOp3.str = NULL;
    sOp3.length = 0;
    sOp3.alloc_size = 0;
    int tOp1 = 0;
    int tOp2 = 0;
    int tOp3 = 0;
    bool initOp1 = false;
    bool initOp2 = false;
    bool initOp3 = false;

    t_item *op1g = NULL;
    t_item *op2g = NULL;
    t_item *op3g = NULL;
    int op1_g = false;
    size_t  inputlen = 0;
    size_t tmplen = 0;

    last_int = 0;
    last_double = 0;

    /*********************************CODE:********************************/

    /*First thing is to set active item in instruction tape:*/
    listFirst(code);

    /* Next is while cycle, cycling until "active" pointer points to NULL: */
    while(code->active != NULL){
        /* Copying active instruction for easier work: */
        Instr = listGetData(code);

#ifdef DEBUG
        fprintf(stderr, "Prva adresa instrukcie\n");
#endif

        /* Reseting variables: */
        op1 = NULL;
        op2 = NULL;
        op3 = NULL;
        iOp1 = 0;
        iOp2 = 0;
        iOp3 = 0;
        dOp1 = 0;
        dOp2 = 0;
        dOp3 = 0;
        sOp1.str = NULL;
        sOp1.length = 0;
        sOp1.alloc_size = 0;
        sOp2.str = NULL;
        sOp2.length = 0;
        sOp2.alloc_size = 0;
        sOp3.str = NULL;
        sOp3.length = 0;
        sOp3.alloc_size = 0;
        tOp1 = 0;
        tOp2 = 0;
        tOp3 = 0;
        op2g = NULL;
        op3g = NULL;
        op1_g = false;
        initOp1 = false;
        initOp2 = false;
        initOp3 = false;
        inputlen = 0;
        tmplen = 0;
#ifdef DEBUG
        fprintf(stderr, "Prva adresa1\n");
#endif

        /* And copying addresses of operands: */
        if(!(Instr->instType == In_JMP || Instr->instType == In_JMPF || Instr->instType == In_JMPT)) {
#ifdef DEBUG
            fprintf(stderr, "Som tu\n");
#endif
            if(Instr->addr1 != NULL) {
                op1 = (f_item *) frame_search(Instr->addr1,frame);
                if (op1 == NULL) {
                    op1g = (t_item *) Instr->addr1;
                    op1_g = true;
                    if (op1g->data.type == CONSTANT){
                        if (op1g->data.value.constant.type == T_INT){
                            iOp1 = op1g->data.value.constant.value.in;
                            initOp1 = true;
                            tOp1 = T_INT;
                        }else if(op1g->data.value.constant.type == T_DOUBLE){
                            dOp1 = op1g->data.value.constant.value.dn;
                            initOp1 = true;
                            tOp1 = T_DOUBLE;
                        }else if(op1g->data.value.constant.type == T_STR){
                            sOp1 = op1g->data.value.constant.value.str;
                            initOp1 = true;
                            tOp1 = T_STR;
                        }
                    }else if(op1g->data.type == VARIABLE){
                        if(op1g->data.value.variable.type == T_INT){
                            iOp1 = op1g->data.value.variable.value.in;
                            initOp1 = op1g->data.value.variable.is_init;
                            tOp1 = T_INT;
                        }else if(op1g->data.value.variable.type == T_DOUBLE){
                            dOp1 = op1g->data.value.variable.value.dn;
                            initOp1 = op1g->data.value.variable.is_init;
                            tOp1 = T_DOUBLE;
                        }else if(op1g->data.value.variable.type == T_STR){
                            sOp1 = op1g->data.value.variable.value.str;
                            initOp1 = op1g->data.value.variable.is_init;
                            tOp1 = T_STR;
                        }
                    }
                }else{
                    if (op1->type == T_INT){
                        iOp1 = op1->value.in;
                        initOp1 = op1->is_init;
                        tOp1 = T_INT;
                    }else if (op1->type == T_DOUBLE){
                        dOp1 = op1->value.dn;
                        initOp1 = op1->is_init;
                        tOp1 = T_DOUBLE;
                    }else if(op1->type == T_STR){
                        sOp1 = op1->value.str;
                        initOp1 = op1->is_init;
                        tOp1 = T_STR;
                    }
                }
            }
        }


#ifdef DEBUG
        fprintf(stderr, "Prva adresa\n");
#endif

        //In function call is second adress instruction tape for parameters
        //it will crash if we try to find it in frame
        if(Instr->instType != In_FUN_CALL) {
            if(Instr->addr2 != NULL) {
                op2 = (f_item *) frame_search(Instr->addr2,frame);
                if (op2 == NULL) {
                    op2g = (t_item *) Instr->addr2;
                    if (op2g->data.type == CONSTANT){
                        if (op2g->data.value.constant.type == T_INT){
                            iOp2 = op2g->data.value.constant.value.in;
                            initOp2 = true;
                            tOp2 = T_INT;
                        }else if(op2g->data.value.constant.type == T_DOUBLE){
                            dOp2 = op2g->data.value.constant.value.dn;
                            initOp2 = true;
                            tOp2 = T_DOUBLE;
                        }else if(op2g->data.value.constant.type == T_STR){
                            sOp2 = op2g->data.value.constant.value.str;
                            initOp2 = true;
                            tOp2 = T_STR;
                        }
                    }else if(op2g->data.type == VARIABLE){
                        if(op2g->data.value.variable.type == T_INT){
                            iOp2 = op2g->data.value.variable.value.in;
                            initOp2 = op2g->data.value.variable.is_init;
                            tOp2 = T_INT;
                        }else if(op2g->data.value.variable.type == T_DOUBLE){
                            dOp2 = op2g->data.value.variable.value.dn;
                            initOp2 = op2g->data.value.variable.is_init;
                            tOp2 = T_DOUBLE;
                        }else if(op2g->data.value.variable.type == T_STR){
                            sOp2 = op2g->data.value.variable.value.str;
                            initOp2 = op2g->data.value.variable.is_init;
                            tOp2 = T_STR;
                        }
                    }
                }else{
                    if (op2->type == T_INT){
                        iOp2 = op2->value.in;
                        initOp2 = op2->is_init;
                        tOp2 = T_INT;
                    }else if (op2->type == T_DOUBLE){
                        dOp2 = op2->value.dn;
                        initOp2 = op2->is_init;
                        tOp2 = T_DOUBLE;
                    }else if(op2->type == T_STR){
                        sOp2 = op2->value.str;
                        initOp2 = op2->is_init;
                        tOp2 = T_STR;
                    }
                }
            }
        }



        if(Instr->addr3 != NULL) {
            op3 = (f_item *) frame_search(Instr->addr3,frame);
            if (op3 == NULL) {
                op3g = (t_item *) Instr->addr3;
                if (op3g->data.type == CONSTANT){
                    if (op3g->data.value.constant.type == T_INT){
                        iOp3 = op3g->data.value.constant.value.in;
                        initOp3 = true;
                        tOp3 = T_INT;
                    }else if(op3g->data.value.constant.type == T_DOUBLE){
                        dOp3 = op3g->data.value.constant.value.dn;
                        initOp3 = true;
                        tOp3 = T_DOUBLE;
                    }else if(op3g->data.value.constant.type == T_STR){
                        sOp3 = op3g->data.value.constant.value.str;
                        initOp3 = true;
                        tOp3 = T_STR;
                    }
                }else if(op3g->data.type == VARIABLE){
                    if(op3g->data.value.variable.type == T_INT){
                        iOp3 = op3g->data.value.variable.value.in;
                        initOp3 = op3g->data.value.variable.is_init;
                        tOp3 = T_INT;
                    }else if(op3g->data.value.variable.type == T_DOUBLE){
                        dOp3 = op3g->data.value.variable.value.dn;
                        initOp3 = op3g->data.value.variable.is_init;
                        tOp3 = T_DOUBLE;
                    }else if(op3g->data.value.variable.type == T_STR){
                        sOp3 = op3g->data.value.variable.value.str;
                        initOp3 = op3g->data.value.variable.is_init;
                        tOp3 = T_STR;
                    }
                }
            }else{
                if (op3->type == T_INT){
                    iOp3 = op3->value.in;
                    initOp3 = op3->is_init;
                    tOp3 = T_INT;
                }else if (op3->type == T_DOUBLE){
                    dOp3 = op3->value.dn;
                    initOp3 = op3->is_init;
                    tOp3 = T_DOUBLE;
                }else if(op3->type == T_STR){
                    sOp3 = op3->value.str;
                    initOp3 = op3->is_init;
                    tOp3 = T_STR;
                }
            }
        }

        /* Every item is some instruction which needs to be executed. Switch
         * is best for that (lot of instructions, if/elseif/else would be just
         * ugly).
         */
        switch(Instr->instType){

            /* Adding two numbers together: */
            case In_ADD:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_ADD\n");
#endif
                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp2 == T_DOUBLE && tOp3 == T_DOUBLE){
                    last_double = dOp2 + dOp3;
                }else if(tOp2 == T_DOUBLE && tOp3 == T_INT){
                    last_double = dOp2 + iOp3;
                }else if(tOp2 == T_INT && tOp3 == T_DOUBLE){
                    last_double = iOp2 + dOp3;
                }else{
                    last_int = iOp2 + iOp3;
                }

                if(op1_g == true){
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.dn = last_double;
                            op1g->data.value.constant.type = T_DOUBLE;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.dn = last_double;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }else{
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.in = last_int;
                            op1g->data.value.constant.type = T_INT;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.in = last_int;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }
                }else{
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        op1->value.dn = last_double;
                        op1->type = T_DOUBLE;
                    }else{
                        op1->value.in = last_int;
                        op1->type = T_INT;
                    }
                }
                break;

                /* Subtracting one number from the other: */
            case In_SUB:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_SUB\n");
#endif
                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp2 == T_DOUBLE && tOp3 == T_DOUBLE){
                    last_double = dOp2 - dOp3;
                }else if(tOp2 == T_DOUBLE && tOp3 == T_INT){
                    last_double = dOp2 - iOp3;
                }else if(tOp2 == T_INT && tOp3 == T_DOUBLE){
                    last_double = iOp2 - dOp3;
                }else{
                    last_int = iOp2 - iOp3;
                }

                if(op1_g == true){
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.dn = last_double;
                            op1g->data.value.constant.type = T_DOUBLE;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.dn = last_double;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }else{
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.in = last_int;
                            op1g->data.value.constant.type = T_INT;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.in = last_int;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }
                }else{
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        op1->value.dn = last_double;
                        op1->type = T_DOUBLE;
                    }else{
                        op1->value.in = last_int;
                        op1->type = T_INT;
                    }
                }
                break;

                /* Multiplication of two numbers: */
            case In_MUL:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_MUL\n");
#endif
                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp2 == T_DOUBLE && tOp3 == T_DOUBLE){
                    last_double = dOp2 * dOp3;
                }else if(tOp2 == T_DOUBLE && tOp3 == T_INT){
                    last_double = dOp2 * iOp3;
                }else if(tOp2 == T_INT && tOp3 == T_DOUBLE){
                    last_double = iOp2 * dOp3;
                }else{
                    last_int = iOp2 * iOp3;
                }

                if(op1_g == true){
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.dn = last_double;
                            op1g->data.value.constant.type = T_DOUBLE;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.dn = last_double;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }else{
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.in = last_int;
                            op1g->data.value.constant.type = T_INT;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.in = last_int;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }
                }else{
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        op1->value.dn = last_double;
                        op1->type = T_DOUBLE;
                    }else{
                        op1->value.in = last_int;
                        op1->type = T_INT;
                    }
                }
                break;

                /* Dividing one number by the second: */
            case In_DIV:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_DIV\n");
#endif
                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp3 == T_DOUBLE){
                    if(dOp3 == 0){return E_ZERO_DIV;}
                }else if(tOp3 == T_INT){
                    if(iOp3 == 0){return E_ZERO_DIV;}
                }
                if(tOp2 == T_DOUBLE && tOp3 == T_DOUBLE){
                    last_double = dOp2 / dOp3;
                }else if(tOp2 == T_DOUBLE && tOp3 == T_INT){
                    last_double = dOp2 / iOp3;
                }else if(tOp2 == T_INT && tOp3 == T_DOUBLE){
                    last_double = iOp2 / dOp3;
                }else{
                    last_int = iOp2 / iOp3;
                }

                if(op1_g == true){
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.dn = last_double;
                            op1g->data.value.constant.type = T_DOUBLE;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.dn = last_double;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }else{
                        if(op1g->data.type == CONSTANT){
                            op1g->data.value.constant.value.in = last_int;
                            op1g->data.value.constant.type = T_INT;
                        }else if(op1g->data.type == VARIABLE){
                            op1g->data.value.variable.value.in = last_int;
                            op1g->data.value.variable.type = T_INT;
                        }
                    }
                }else{
                    if(tOp2 == T_DOUBLE || tOp3 == T_DOUBLE){
                        op1->value.dn = last_double;
                        op1->type = T_DOUBLE;
                    }else{
                        op1->value.in = last_int;
                        op1->type = T_INT;
                    }
                }
                break;

            case In_ASS:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_ASS\n");
#endif
                if (op1_g == true){
                    if(op1g->data.type == CONSTANT){
                        if(tOp1 == T_INT){
                            op1g->con.value.in = last_int;
                        }else if(tOp1 == T_DOUBLE){
                            op1g->con.value.dn = last_double;
                        }else if(tOp1 == T_STR){
                            op1g->con.value.str = last_string;
                        }
                    }
                    else{
                        op1g->var.is_init = true;
                        if(tOp1 == T_INT){
                            op1g->var.value.in = last_int;
                        }else if(tOp1 == T_DOUBLE){
                            op1g->var.value.dn = last_double;
                        }else if(tOp1 == T_STR){
                            op1g->var.value.str = last_string;
                        }
                    }
                    break;
                }
                if (tOp1 == T_INT){
//#ifdef DEBUG
//                    fprintf(stderr, "In_ASS, assignment to INT variable key: %s with value: %d, last: %d\n", op1->key.str,op1->value.integer, last_int);
//#endif
                    op1->value.in = last_int;
//#ifdef DEBUG
//                    fprintf(stderr, "In_ASS, after assignment to INT variable key: %s with value: %d\n", op1->key.str,op1->value.integer);
//#endif
                }else if(tOp1 == T_DOUBLE){
//#ifdef DEBUG
//                    fprintf(stderr, "In_ASS, assignment to DOUBLE variable key: %s with value: %g\n", op1->key.str,op1->value.double_number);
//#endif
                    op1->value.dn = last_double;
//#ifdef DEBUG
//                    fprintf(stderr, "In_ASS, after assignment to DOUBLE variable key: %s with value: %g\n", op1->key.str,op1->value.double_number);
//#endif
                }else if(tOp1 == T_STR){
//#ifdef DEBUG
//                    fprintf(stderr, "In_ASS, assignment to STRING variable key: %s with value: %s\n", op1->key.str,op1->value.str.str);
//#endif
                    op1->value.str = last_string;
//#ifdef DEBUG
//                    fprintf(stderr, "In_ASS, after assignment to STRING variable key: %s with value: %s\n", op1->key.str,op1->value.str.str);
//#endif
                }else{
                    return E_INTERNAL;
                }
                break;
            case IN_SET_LAST:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_SET_LAST\n");
#endif
                if(op1_g == true) {
                    if(!initOp1) {
                        return E_INIT;
                    }
                    if(op1g->data.type == CONSTANT) {
                        switch(op1g->data.value.constant.type) {
                            case T_INT:
                                last_int = op1g->data.value.constant.value.integer;
                                break;
                            case T_DOUBLE:
                                last_double = op1g->data.value.constant.value.double_number;
                                break;
                            case T_STR:
                                last_string = op1g->data.value.constant.value.str;
                                break;
                            default:
                                break;
                        }
                    } else if (op1g->data.type == VARIABLE) {
                        switch(op1g->data.value.constant.type) {
                            case T_INT:
                                last_int = op1g->data.value.variable.value.integer;
                                break;
                            case T_DOUBLE:
                                last_double = op1g->data.value.variable.value.double_number;
                                break;
                            case T_STR:
                                last_string = op1g->data.value.variable.value.str;
                                break;
                            default:
                                break;
                        }
                    }
                } else {
                    if(!initOp1) {
                        return E_INIT;
                    }
                    switch(op1->type) {
                        case T_INT:
                            last_int = op1->value.integer;
                            break;
                        case T_DOUBLE:
                            last_double = op1->value.double_number;
                            break;
                        case T_STR:
                            last_string = op1->value.str;
                            break;
                        default:
                            break;
                    }
                }
                break;
                /* Concatenation of two strings together: */
            case In_CON:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_CON\n");
#endif
                if(initOp2 == false || initOp3 == false){
                    return E_INIT;
                }
                if(tOp2 != T_STR){
                    if(tOp2 == T_INT){
                        //INTEGER
                        snprintf(cPom,CHUNK,"%d",iOp2);
                        sOp2.str = cPom;
                        sOp2.length = strlen(cPom);
                    }else if(tOp2 == T_DOUBLE){
                        //DOUBLE
                        snprintf(cPom,CHUNK,"%g",dOp2);
                        sOp2.str = cPom;
                        sOp2.length = strlen(cPom);
                    }else {return E_INTERNAL;}
                }else if(tOp3 != T_STR){
                    if(tOp3 == T_INT){
                        //INTEGER
                        snprintf(cPom,CHUNK,"%d",iOp3);
                        sOp3.str = cPom;
                        sOp3.length = strlen(cPom);
                    }else if(tOp3 == T_DOUBLE){
                        //DOUBLE
                        snprintf(cPom,CHUNK,"%g",dOp3);
                        sOp3.str = cPom;
                        sOp3.length = strlen(cPom);
                    }else {return E_INTERNAL;}
                }
                tmplen = sOp2.length + sOp3.length;
                if(op1_g == true){
                    op1g->var.value.str.str = NULL;
                    op1g->var.value.str.str = gc_realloc_pointer(op1g->var.value.str.str,
                            sizeof(char) * tmplen+1);
                    snprintf(op1g->var.value.str.str, tmplen+1, "%s%s", sOp2.str, sOp3.str);
                    op1g->var.value.str.length = tmplen;
                    op1g->var.value.str.alloc_size = tmplen +1;
                    op1g->var.type = T_STR;
                    last_string = op1g->var.value.str;
                }else{
                    op1->value.str.str = NULL;
                    op1->value.str.str = gc_realloc_pointer(op1->value.str.str,
                            sizeof(char) * tmplen+1);
                    snprintf(op1->value.str.str, tmplen+1, "%s%s", sOp2.str, sOp3.str);
                    op1->value.str.length = tmplen;
                    op1->value.str.alloc_size = tmplen +1;
                    op1->type = T_STR;
                    last_string = op1->value.str;
                }
                break;

                /* Testing if is first number greater then the other: */
            case In_GT:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_GT\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT && tOp2 == T_INT){
                    cmp = iOp1>iOp2?true:false;
                }else if(tOp1 == T_DOUBLE && tOp2 == T_INT){
                    cmp = dOp1>iOp2?true:false;
                }else if(tOp1 == T_INT && tOp2 == T_DOUBLE){
                    cmp = iOp1>dOp2?true:false;
                }else{
                    cmp = dOp1>dOp2?true:false;
                }
                break;

                /* Testing if is first number lower then the other: */
            case In_LT:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_LT\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT && tOp2 == T_INT){
                    fprintf(stderr,"Porovnanvam %d < %d \n",iOp1,iOp2);
                    cmp = iOp1<iOp2?true:false;
                }else if(tOp1 == T_DOUBLE && tOp2 == T_INT){
                    cmp = dOp1<iOp2?true:false;
                }else if(tOp1 == T_INT && tOp2 == T_DOUBLE){
                    cmp = iOp1<dOp2?true:false;
                }else{
                    cmp = dOp1<dOp2?true:false;
                }
                break;

                /* Testing if is first number greater or equal than the other: */
            case In_GTE:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_GTE\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT && tOp2 == T_INT){
                    cmp = iOp1>=iOp2?true:false;
                }else if(tOp1 == T_DOUBLE && tOp2 == T_INT){
                    cmp = dOp1>=iOp2?true:false;
                }else if(tOp1 == T_INT && tOp2 == T_DOUBLE){
                    cmp = iOp1>=dOp2?true:false;
                }else{
                    cmp = dOp1>=dOp2?true:false;
                }
                break;

                /* Testing if is first number lower or equal than the other: */
            case In_LTE:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_LTE\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT && tOp2 == T_INT){
                    cmp = iOp1<=iOp2?true:false;
                }else if(tOp1 == T_DOUBLE && tOp2 == T_INT){
                    cmp = dOp1<=iOp2?true:false;
                }else if(tOp1 == T_INT && tOp2 == T_DOUBLE){
                    cmp = iOp1<=dOp2?true:false;
                }else{
                    cmp = dOp1<=dOp2?true:false;
                }
                break;

                /* Testing for non-equal operands: */
            case In_NEQ:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_NEQ\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT && tOp2 == T_INT){
                    cmp = iOp1!=iOp2?true:false;
                }else if(tOp1 == T_DOUBLE && tOp2 == T_INT){
                    cmp = dOp1!=iOp2?true:false;
                }else if(tOp1 == T_INT && tOp2 == T_DOUBLE){
                    cmp = iOp1!=dOp2?true:false;
                }else{
                    cmp = dOp1!=dOp2?true:false;
                }
                break;

                /* Testing for equal operands: */
            case In_EQ:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_EQ\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }
                if(tOp1 == T_INT && tOp2 == T_INT){
                    cmp = iOp1==iOp2?true:false;
                }else if(tOp1 == T_DOUBLE && tOp2 == T_INT){
                    cmp = dOp1==iOp2?true:false;
                }else if(tOp1 == T_INT && tOp2 == T_DOUBLE){
                    cmp = iOp1==dOp2?true:false;
                }else{
                    cmp = dOp1==dOp2?true:false;
                }
#ifdef DEBUG
                fprintf(stderr, "End of In_EQ\n");
#endif
                break;

                /* Loading integer: */
            case In_LI:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_LI\n");
#endif
                ret = getchar();
                if((ret < '0' || ret > '9') && ret != '-'){
                    return E_INPUT;
                }else {
                    ungetc(ret, stdin);
                }
                ret = scanf("%d",&last_int);
                if(ret == EOF){
                    return E_INPUT;
                }
                ret = getchar();
                if(ret == '\n' || ret == '\r' || ret == '\0'){
                    break;
                }else {
                    return E_INPUT;
                }
                break;

                /* Loading double: */
            case In_LD:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_LD\n");
#endif
                ret = getchar();
                if((ret < '0' || ret > '9') && ret != '-'){
                    return E_INPUT;
                }else {
                    ungetc(ret, stdin);
                }
                ret = scanf("%lf",&last_double);
                if(ret == EOF){
                    return E_INPUT;
                }
                ret = getchar();
                if(ret == '\n' || ret == '\r' || ret == '\0' || ret == EOF){
                    break;
                }else {
                    return E_INPUT;
                }
                break;

                /* Loading string: */
            case In_LS:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_LS\n");
#endif
                init_value_str(&sOp1, "\0");
                do{
                    fgets(cPom, CHUNK, stdin);
                    tmplen = strlen(cPom);
                    inputlen += tmplen;
                    ret = init_value_str(&sOp1, cPom);
                    if(ret != STR_OK){
                        return E_INPUT;
                    }
                } while (tmplen == CHUNK - 1 && cPom[CHUNK-2]!='\n');
                sOp1.str[strcspn(sOp1.str, "\r\n")] = 0;
                sOp1.length = inputlen -1;
                sOp1.alloc_size = inputlen;
                last_string = sOp1;
                break;

                /* Jump: */
            case In_JMP:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_JMP\n");
#endif
                code->active = Instr->addr1;
                break;

                /* Jump if last comparsion was false: */
            case In_JMPF:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_JMPF\n");
#endif
                if (cmp == false){
                    code->active = Instr->addr1;
                }
                break;

                /* Jump if last comparsion was true: */
            case In_JMPT:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_JMPT\n");
#endif
                if (cmp == true){
                    code->active = Instr->addr1;
                }
                break;

                /* Printing operand: */
            case In_PRT:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_PRT\n");
#endif
                //TODO - PARSING STRINGS AND ESCAPE SEQUENCES.
                if (op1_g == true){
                    if (op1g->data.type == CONSTANT){
                        if (op1g->data.value.constant.type == T_INT){
                            printf("%d",op1g->data.value.constant.value.in);
                        } else if(op1g->data.value.constant.type == T_DOUBLE){
                            printf("%g",op1g->data.value.constant.value.dn);
                        } else {
                            printf("%s",op1g->data.value.constant.value.str.str);
                        }
                        break;
                    }
                    if (op1g->data.value.variable.type == T_INT){
                        printf("%d",op1g->data.value.variable.value.in);
                    } else if(op1g->data.value.variable.type == T_DOUBLE){
                        printf("%g",op1g->data.value.variable.value.dn);
                    } else{
                        printf("%s",op1g->data.value.variable.value.str.str);

                    }
                    break;
                }
                if (op1->type == T_INT){
                    printf("%d",op1->value.in);
                } else if(op1->type == T_DOUBLE){
                    fprintf(stderr, "som tu2 \n");
                    printf("%g",op1->value.dn);
                } else{
                    printf("%s",op1->value.str.str);
                }
                break;

                /* Getting length of string: */
            case In_STRL:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_STRL\n");
#endif

                if(!initOp1) {
                    return E_INIT;
                }

                last_int = sOp1.length;
                break;

                /* Comparing two strings: */
            case In_STRC:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_STRC\n");
#endif

                if(!initOp1 || ! initOp2) {
                    return E_INIT;
                }

                int res1 = strcmp(sOp1.str, sOp2.str);

                if(res1 > 0) {
                    last_int = 1;
                } else if (res1 < 0) {
                    last_int = -1;
                } else {
                    last_int = 0;
                }
                break;

                /* Finding substring in string: */
            case In_STRF:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_STRF\n");
#endif
                if(initOp1 == false || initOp2 == false){
                    return E_INIT;
                }

                int res2 = find(&sOp1, &sOp2);

                last_int = res2;
                break;

                /* Sorting string: */
            case In_STRS:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_STRS\n");
#endif
                if (!initOp1) {
                    return E_INIT;
                }
                string duplicate;

                heap_sort(&sOp1, &duplicate);
                last_string = duplicate;
                break;

            case In_SSTR:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_SSTR\n");
#endif
                if(!initOp1 || !initOp2 || !initOp3) {
                    return E_INIT;
                }

                string res3;

                int tmp123 = substring(&sOp1, iOp2, iOp3, &res3);

                if(tmp123 == E_OTHER) {
                    return E_OTHER;
                }

                last_string = res3;
                break;

                /* Calling function: */
            case In_FUN_CALL:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_FUN_CALL\n");
#endif

                t_item * function_item = Instr->addr1;

                tInstrList function_list = *(function_item->data.value.function.instrList);

                f_table * new_frame = NULL;
                create_frame_from_table(&new_frame, function_item->data.value.function.loc_table);

                if(Instr->addr2 != NULL) {
#ifdef DEBUG
                    fprintf(stderr, "Function have parameters\n");
#endif
                    tInstrList param_list = *((tInstrList *)(Instr->addr2));
                    remap_param_list(&param_list, frame);
                    if(function_list.first != NULL) {
                        if (param_list.first == NULL) {
                            param_list.first = function_list.first;
                            param_list.last = function_list.last;
                        } else {
                            param_list.last->nextItem = function_list.first;
                            param_list.last = function_list.last;
                        }

#ifdef DEBUG
                        fprintf(stderr, "List of function:\n");
#endif
                        print_elements_of_list(param_list);
                    } else {
#ifdef DEBUG
                        fprintf(stderr, "Function have no instructions\n");
#endif
                    }

                    int ret = Interpret(&param_list, new_frame);

                    if (ret != E_OK) {
                        return ret;
                    }


                } else {
#ifdef DEBUG
                    fprintf(stderr, "No parameters tape\n");
#endif
                }

                break;

                /* Setting parameters for function: */
            case In_SET_PARAM:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_SET_PARAM\n");
#endif
                string * key = Instr->addr2;
                f_item * param = NULL;
                frame_table_search(frame, key, &param);

                if(param != NULL) {
#ifdef DEBUG
                    fprintf(stderr, "Parameter is in frame %s\n", key->str);
#endif

                    f_item * item = Instr->addr1;

                    *param = *item;
                    param->key = *key;


                } else {
#ifdef DEBUG
                    fprintf(stderr, "Parameter is not in frame %s\n", key->str);
#endif
                }

                break;
            case In_SET_RESULT:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_SET_RESULT\n");
#endif
                    switch(tOp1) {
                        case T_INT:
                            last_int = iOp1;
                            break;
                        case T_DOUBLE:
                            last_double = dOp1;
                            break;
                        case T_STR:
                            last_string = sOp1;
                            break;
                        default:
                            break;
                    }

                    code->active = NULL;

                break;

            case In_NONE:
#ifdef DEBUG
                fprintf(stderr, "Doing instruction In_NONE\n");
#endif
                break;

            default:
                //listFree(code);
                return E_INTERNAL; //should be INTERNAL ERROR
                break;
        }
        listNext(code);
    }
#ifdef DEBUG
    fprintf(stderr, "End of Interpret function\n");
#endif
    return E_OK; //Should be E_OK, but... not defined
}
