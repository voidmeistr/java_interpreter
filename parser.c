/************************** parser.c ******************************************/
/* Subor:               parser.c                                              */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include "parser.h"
#include "scanner.h"
#include "ial.h"
#include "str.h"
#include "garbage_collector.h"
#include "ilist.h"
#include <stdlib.h>
#include <stdio.h>
#include "precedence.h"
#include "interpret.h"

#define DEBUG

/***************************GLOBAL VARIABLES***************************/
//lexical analysis will put token in this variable when getToken() function is called.
tToken * token = NULL;

//in result is stored result from precedence analysis
extern t_item * result;

//in current context we store actual context of recursive descent
//struct is better explained in parser.h file
GContext current_context;

//varible store actual phase of recursive descent
//we have 2 phases in our project because of static variables
int global_phase;

//garbage collector list is linked list which save pointers to allocated memory
g_list * list = NULL;

//saving is true when we want to store some tokens for rollback
int saving = 0;

//helper variable for counting the parameters in function call
int number_of_parameters_in_call = 0;
t_item * actual_parameter = NULL;
bool is_return = false;

//3 helper adresses for storing the adresses of opernads in builtin functions
t_item * op1 = NULL;
t_item * op2 = NULL;
t_item * op3 = NULL;

//stack which remebers if we are in complex statment
tStack complex_stat_stack;

//instruction lists

//instuction tape for instructions which initialize static varibles of whole program
tInstrList * staticVariablesList = NULL;
//pointer to actual list in current context
tInstrList * actualList = NULL;
//list for parameters of function
tInstrList * tmpCallParamList = NULL;

extern FILE * source;

void generate_instruction(tInstrList* tList,int type, void * ptr1, void * ptr2, void * ptr3) {
    tInstr new_inst;
    new_inst.instType = type;
    new_inst.addr1 = ptr1;
    new_inst.addr2 = ptr2;
    new_inst.addr3 = ptr3;

#ifdef DEBUG
    fprintf(stderr, "token je %s, on line: %d, generate instruction of type: %d\n", token->data, token->line, type );
#endif

    listInsertLast(tList, new_inst);
}

//main function for recursive descent
int parse(int phase) {
    token = getToken();

    //set phase to global variable
    global_phase = phase;

    if (global_phase == P_FIRST) {

        //init static variables list
        staticVariablesList = gc_alloc_pointer(sizeof(tInstrList));
        listInit(staticVariablesList);

        //global symbol table init
        h_table * global_symbol_table;
        global_symbol_table = hash_table_init();
        current_context.global_table = global_symbol_table;

        create_builtin_ts();
        complex_stat_stack = createStack();
    }

    //set context
    current_context.context_type = C_GLOBAL;
    current_context.function_key = NULL;
    current_context.identifier_item = NULL;
    current_context.class_key = NULL;

    parse_class_list();

    if (token->state == S_EOF) {
        //syntax OK
#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX OK EOF, on line: %d\n", token->data, token->line);
#endif
    } else {
        //syntax ERROR
        
        check_lex_error();

#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX Error no EOF, on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }

    return 0;
}

void parse_class_list() {

    if (token->state != S_EOF) {
        parse_class();
        parse_class_list();
    } else {
        return;
    }

}

void parse_class() {
    if (token->state == S_KEYWORD) {
        if ( !strcmp(token->data,"class") ) {
#ifdef DEBUG
            fprintf(stderr, "token je %s, on line: %d\n", token->data, token->line);
#endif
            //NEXT TOKEN
            token = getToken();
            
            if (token->state == S_IDENTIF) {
#ifdef DEBUG
                fprintf(stderr, "token je %s, on line: %d\n", token->data, token->line);
#endif
                //create key for the class in global symbol table
                string key;
                init_value_str(&key, token->data);

                if(global_phase == P_FIRST) {
                    //init of item
                    t_item * item;
                    item = gc_alloc_pointer(sizeof(t_item));

                    if(item == NULL) {
                        throw_error(E_INTERNAL);
                    }

                    item->key = key;

                    //insert class to table
                    int ret = hash_table_insert(current_context.global_table, &item->key, &item);
                    if(ret != HASH_INSERT_OK) {
                        if(ret == HASH_INSERT_FOUND) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error class redefinition, on line: %d\n", token->line);
#endif
                            throw_error(E_SEM);
                        } else if (ret == HASH_INSERT_ALLOC_ERR) {
                            throw_error(E_INTERNAL);
                        }
                    }

                    //set class data
                    item->data.type = CLASS;
                    item->data.value.class.loc_table = hash_table_init();

                    //set current class table to this
                    current_context.class_key = &item->key;
                    current_context.context_type = C_CLASS;
                    
                } else if (global_phase == P_SECOND) {
                    //set current class table to this
                    current_context.class_key = &key;
                    current_context.context_type = C_CLASS;
                }

                //NEXT TOKEN
                token = getToken();
                if (token->state == S_L_BRACKET) {
#ifdef DEBUG
                    fprintf(stderr, "token je %s , on line: %d\n", token->data, token->line);
#endif
                    //NEXT TOKEN
                    token = getToken();

                    parse_def_list();

                    if (token->state == S_R_BRACKET) {
                        //SYNTAX OK - definition of class

                        //NEXT TOKEN
                        token = getToken();
#ifdef DEBUG
                        fprintf(stderr, "SYNTAX OK - definition of class\n");
#endif
                    } else {
                        //SYNTAX ERROR - no '}' at the end of class definition
                        check_lex_error();
#ifdef DEBUG
                        fprintf(stderr, "SYNTAX ERROR - no '}' at the end of class definition, on line: %d\n",token->line);
#endif
                        throw_error(E_SYN);
                    }
                } else {
                    //SYNTAX ERROR - no '{' after name of class definition
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR - no '{' after name of class definition, on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            } else {
                //SYNTAX ERROR - missing name of class

                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "token je %s, SYNTAX ERROR - missing name of class, on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }
        } else {
            //SYNTAX ERROR - no class declaration as first token

            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - no class declaration as first token, on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    } else {
        //SYNTAX ERROR - no keyword as first token ('class' keyword is required)
        check_lex_error();
#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX ERROR - no keyword as first token ('class' keyword is required), on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }
}

void parse_def_list() {

    parse_def_prefix();
    parse_def();
    if (token->state == S_R_BRACKET) {
        //return - no definitions follows
#ifdef DEBUG
        fprintf(stderr, "token je %s, no definitions follows, on line: %d\n", token->data, token->line);
#endif

    } else {
        parse_def_list();
    }
}

void parse_def_prefix() {
    if (token->state == S_KEYWORD) {
        if (!strcmp(token->data, "static")) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //NEXT TOKEN
            token = getToken();
            
            int type = parse_type();

            if (token->state == S_IDENTIF) {
                //SYNTAX OK
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                //save identifier to symbol table
                //create key for the class in GTS
                string * key = gc_alloc_pointer(sizeof(string));
                init_value_str(key, token->data);
                
                t_item * item;
                if (global_phase == P_FIRST) {

                    item = gc_alloc_pointer(sizeof(t_item));
                    item->key = (*key);

                }

                //NEXT TOKEN
                token = getToken();

                if(token-> state == S_LBRACKET) {
                    //function definition

                    if (global_phase == P_FIRST) {
                        t_item * class_item;

                        hash_table_search(current_context.global_table, current_context.class_key, &class_item);


                        if(hash_table_insert(class_item->data.value.class.loc_table, &item->key, &item) == HASH_INSERT_FOUND) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, function redefinition, on line: %d\n", token->line);
#endif
                            throw_error(E_SEM);
                        }

                        item->data.type = FUNCT;
                        item->data.value.function.loc_table = hash_table_init();
                        item->data.value.function.instrList = NULL;
                        item->data.value.function.instrList = gc_alloc_pointer(sizeof(tInstrList));
                        listInit(item->data.value.function.instrList);
                        item->data.value.function.first_param = NULL;
                        item->data.value.function.ret_type = type;
                        item->data.value.function.param_count = 0;
                        
                        current_context.function_key = &item->key;
                        current_context.context_type = C_FUNCTION;
                    } else if(global_phase == P_SECOND) {

                        current_context.function_key = key;
                        current_context.context_type = C_FUNCTION;
                    }

                } else {
                    //variable semantic
                    if (global_phase == P_FIRST) {
                        if(type == T_VOID) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, void static variable, on line: %d\n", token->line);
#endif
                            throw_error(E_SYN);
                        }

                        current_context.identifier_item = item;
                        current_context.identifier_item->data.type = type; //save the data type to this variable, correct type is set later in parse_init()

#ifdef DEBUG
                        fprintf(stderr, "Saving identifier with key: %s\n", current_context.identifier_item->key.str);
#endif
                    } else if (global_phase == P_SECOND) {
                        t_item * item;

                        t_item * class_item;
                        hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                        hash_table_search(class_item->data.value.class.loc_table, key, &item);

                        current_context.identifier_item = item;
                        current_context.identifier_item->data.type = type;
                    }

                }

            } else {
                //SYNTAX ERROR - missing name of identifier or function
                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "token je %s, SYNTAX ERROR - missing name of identifier or function, on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }
        } else {
            //SYNTAX ERROR - missing keyword 'static near function or variable declaration'
            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - missing keyword 'static near function or variable declaration', on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    } else {
        //SYNTAX ERROR - no keyword as first token ('static' keyword is required)
        check_lex_error();
        if(token->state == S_R_BRACKET) {
#ifdef DEBUG
            fprintf(stderr, "SYNTAX OK, empty class\n");
#endif
        } else {
            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - no keyword as first token ('static' keyword is required), on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }

    }
}

void parse_def() {
    if(token->state == S_LBRACKET) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //NEXT TOKEN
        token = getToken();
        parse_param_list();
        if(token->state == S_RBRACKET) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //NEXT TOKEN
            token = getToken();
            if(token->state == S_L_BRACKET) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                //NEXT TOKEN
                token = getToken();

                //save pointer to actual instruction list
                t_item *class_item = NULL;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                t_item *fun_item = NULL;
                hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                actualList = fun_item->data.value.function.instrList;

                parse_stat_list();
                if(token->state == S_R_BRACKET) {
                    //SYNTAX OK
#ifdef DEBUG
                    fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                    //NEXT TOKEN
                    token = getToken();
                } else {
                    //SYNTAX ERROR - missing closing bracket '}'
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR - missing closing bracket '}', on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            } else {
                //SYNTAX ERROR - missing opening bracket '{'
                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "token je %s, SYNTAX ERROR - missing opening bracket '{', on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }
        } else {
            //SYNTAX ERROR - missing closing bracket ')'
            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - missing closing bracket ')', on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    } else {
        parse_init();
        if (check_semicolon()) {
            //SYNTAX OK
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        } else {
            //SYNTAX ERROR expected ';'
            check_lex_error();

            if(token->state == S_R_BRACKET) {
#ifdef DEBUG
                fprintf(stderr, "SYNTAX OK empty class");
#endif
            } else {
                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "SYNTAX ERROR expected ';', on line: %d\n",token->line);
#endif
                throw_error(E_SYN);
            }
        }
    }
}

void parse_init() {
    if(token->state == S_ASSIGMENT) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        if (global_phase == P_FIRST) {
            if(current_context.context_type == C_CLASS) {
                t_item * class_item = NULL;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                int type_tmp = current_context.identifier_item->data.type;

                if(hash_table_insert(class_item->data.value.class.loc_table, &(current_context.identifier_item->key), &current_context.identifier_item) == HASH_INSERT_FOUND) {
#ifdef DEBUG
                    fprintf(stderr, "semantic error, variable redefinition, on line: %d\n", token->line);
#endif
                    throw_error(E_SEM);
                } else {
                    current_context.identifier_item->data.value.variable.is_init = false;
                    current_context.identifier_item->data.value.variable.type = type_tmp;
                    current_context.identifier_item->data.type = VARIABLE;

#ifdef DEBUG
                    fprintf(stderr, "Variable inserted: %s\n", current_context.identifier_item->key.str);
#endif
                }
            }
        } else if (global_phase == P_SECOND) {
            if (current_context.context_type == C_FUNCTION) {
                t_item * class_item;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                t_item * function_item;
                hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &function_item);

                int type_tmp = current_context.identifier_item->data.type;

                if(hash_table_insert(function_item->data.value.function.loc_table, &current_context.identifier_item->key, &current_context.identifier_item) == HASH_INSERT_FOUND) {
#ifdef DEBUG
                    fprintf(stderr, "semantic error, variable redefinition, on line: %d\n", token->line);
#endif
                    throw_error(E_SEM);
                } else {

                    t_item * tmp_item = NULL;
                    hash_table_search(class_item->data.value.class.loc_table, &current_context.identifier_item->key, &tmp_item);

                    if(tmp_item != NULL) {
                        if(tmp_item->data.type == FUNCT) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, variable redefinition, on line: %d\n", token->line);
#endif
                            throw_error(E_SEM);
                        }
                    }

                    current_context.identifier_item->data.value.variable.type = type_tmp;
                    current_context.identifier_item->data.value.variable.is_init = false;
                    current_context.identifier_item->data.type = VARIABLE;

#ifdef DEBUG
                    fprintf(stderr, "Variable inserted: %d\n", current_context.identifier_item->data.value.variable.type);
#endif
                }
            } else {
                current_context.identifier_item->data.type = VARIABLE;
            }
        }

        if(global_phase == P_SECOND) {
            if (current_context.identifier_item->data.type != VARIABLE) {
#ifdef DEBUG
                fprintf(stderr, "Cannot assign to function: %d\n", current_context.identifier_item->data.type);
#endif
                throw_error(E_SEM);
            }
        }
        saving = true;

        //NEXT TOKEN
        token = getToken();

        if (token->state == S_IDENTIF || token->state == S_FULL_IDENTIF) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            t_item * item = NULL;

            if (token->state == S_IDENTIF) {
                if (global_phase == P_SECOND) {
                    string key;
                    init_value_str(&key, token->data);

                    t_item *class_item = NULL;
                    hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                    t_item *fun_item = NULL;
                    hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                    t_item *id_item = NULL;
                    hash_table_search(fun_item->data.value.function.loc_table, &key, &id_item);


                    if (id_item == NULL) {

                        t_item * tmp_item = NULL;
                        hash_table_search(class_item->data.value.class.loc_table, &key, &tmp_item);

                        if(tmp_item == NULL) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, variable: | %s | %s | %s | doesn't exist: %d\n",current_context.class_key->str,current_context.function_key->str, key.str, token->line);
#endif
                        } else {
                            item = tmp_item;
                        }
                    } else {
                        item = id_item;
                    }
                }

            } else if (token->state == S_FULL_IDENTIF) {

                if (global_phase == P_SECOND) {

                    int x = 0;
                    char *class = gc_alloc_pointer(token->size);
                    while(token->data[x] != '.') {
                        class[x] = token->data[x];
                        x++;
                    }
                    class[x] = '\0';
                    x++;
                    char *item_tmp = gc_alloc_pointer(token->size);
                    int y = 0;
                    while(token->data[x] != '\0'){
                        item_tmp[y] = token->data[x];
                        x++;
                        y++;
                    }
                    item_tmp[y]= '\0';

                    string class_key;
                    init_value_str(&class_key, class);

#ifdef DEBUG
                    fprintf(stderr, "class key: %s\n",class_key.str);
#endif

                    t_item *class_item = NULL;
                    hash_table_search(current_context.global_table, &class_key, &class_item);

                    if (class_item == NULL) {
#ifdef DEBUG
                        fprintf(stderr, "semantic error, class: %s doesn't exist: %d\n", class_key.str, token->line);
#endif
                        throw_error(E_SEM);
                    } else {
                        string key;
                        init_value_str(&key, item_tmp);

                        t_item *id_item = NULL;
                        hash_table_search(class_item->data.value.class.loc_table, &key, &id_item);


                        if (id_item == NULL) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, variable5: %s doesn't exist: %d\n", key.str, token->line);
#endif
                            throw_error(E_SEM);
                        }

#ifdef DEBUG
                        fprintf(stderr, "class_item key %s id item key: %s\n", class_key.str, key.str);
#endif

                        item = id_item;
                    }
                }
            }

            //NEXT TOKEN
            token = getToken();
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->state);
#endif

            if (token->state == S_LBRACKET) { //assignment of function
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                saving = false;
                clearMem();
#ifdef DEBUG
                fprintf(stderr, "som tu \n");
#endif


                if(global_phase == P_SECOND) {
                    if (item->data.type == VARIABLE) {
#ifdef DEBUG
                        fprintf(stderr, "Cannot call variable: %d\n", item->data.type);
#endif
                        throw_error(E_SEM);
                    }

                    if(item->data.value.function.ret_type == T_VOID) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s, assignment of void function, on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_INIT);
                    }
                }



                //parameter is pointer to actual function
                parse_call_param_list(item);

                if (token->state == S_RBRACKET) {
#ifdef DEBUG
                    fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                    //NEXT TOKEN
                    token = getToken();

                    //type control of assignment function
                    if(global_phase == P_SECOND) {
                        if (current_context.identifier_item->data.value.variable.type == item->data.value.function.ret_type) {
#ifdef DEBUG
                            fprintf(stderr, "Types in assignment are equals\n");

                            //GENERATE INSTRUCTION
                            //add instruction to actual tape
                            current_context.identifier_item->data.value.variable.is_init = true;
                            generate_instruction(actualList, In_ASS, current_context.identifier_item, NULL, NULL);
#endif
                        } else {
#ifdef DEBUG
                            fprintf(stderr, "Error, types in assignment are not equals\n");
#endif
                            throw_error(E_TYPE);
                        }
                    }

                }

            } else {
                //PRECEDENCE ANALYSIS
                saving = false;

                if(current_context.function_key != NULL) {
                    t_item *class_item = NULL;
                    hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                    t_item *fun_item = NULL;
                    hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                    if(fun_item == NULL) {
                        parseExpression(global_phase, staticVariablesList);
                    } else {
                        parseExpression(global_phase, fun_item->data.value.function.instrList);
                    }
                } else {
                    parseExpression(global_phase, staticVariablesList);
                }

                if(global_phase == P_SECOND) {
                    current_context.identifier_item->data.value.variable.is_init = true;

                    if(current_context.function_key != NULL) {
                        t_item *class_item = NULL;
                        hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                        t_item *fun_item = NULL;
                        hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                        //GENERATE INSTRUCTION
                        //add instruction to actual tape
                        current_context.identifier_item->data.value.variable.is_init = true;
                        generate_instruction(fun_item->data.value.function.instrList, In_ASS, current_context.identifier_item, NULL, NULL);
                    } else {
                        //GENERATE INSTRUCTION
                        //add instruction to static variables tape
                        current_context.identifier_item->data.value.variable.is_init = true;
                        generate_instruction(staticVariablesList, In_ASS, current_context.identifier_item, NULL, NULL);
                    }
                }
            }

        } else {
            //PRECEDENCE ANALYSIS
            saving = false;

            if (current_context.context_type == C_FUNCTION) {
                t_item *class_item = NULL;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                t_item *fun_item = NULL;
                hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                parseExpression(global_phase, fun_item->data.value.function.instrList);

            } else {
                parseExpression(global_phase, staticVariablesList);
            }

            if (global_phase == P_SECOND) {
#ifdef DEBUG
                fprintf(stderr, "type of result: %d\n", result->data.value.constant.type);
#endif
                if (current_context.identifier_item->data.value.variable.type != result->data.value.constant.type) {

#ifdef DEBUG
                    fprintf(stderr, "id type: %d, result type: %d\n", current_context.identifier_item->data.value.variable.type, result->data.value.constant.type);
#endif
                    if((current_context.identifier_item->data.value.variable.type  == T_DOUBLE) && (result->data.value.constant.type == T_INT)){
#ifdef DEBUG
                        fprintf(stderr, "assignment int to double\n");
#endif
                    } else {
#ifdef DEBUG
                        fprintf(stderr, "cannot assign to variable, invalid types\n");
#endif
                        throw_error(E_TYPE);
                    }
                }
            }


            if (global_phase == P_SECOND) {
                current_context.identifier_item->data.value.variable.is_init = true;

                if (current_context.context_type == C_FUNCTION) {
                    t_item *class_item = NULL;
                    hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                    t_item *fun_item = NULL;
                    hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                    //GENERATE INSTRUCTION
                    //add instruction to actual tape
                    current_context.identifier_item->data.value.variable.is_init = true;
                    generate_instruction(fun_item->data.value.function.instrList, In_ASS,
                                         current_context.identifier_item, NULL, NULL);
                } else {
                    //GENERATE INSTRUCTION
                    //add instruction to static variables tape
                    current_context.identifier_item->data.value.variable.is_init = true;
                    generate_instruction(staticVariablesList, In_ASS, current_context.identifier_item, NULL, NULL);
                }

            }
        }

    } else if(token->state == S_SEMICOLON){
        //declaration of uninitialized variable

        if(global_phase == P_FIRST) {
            if(current_context.context_type == C_CLASS) {
                t_item * class_item = NULL;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                int type_tmp = current_context.identifier_item->data.type;

                if(hash_table_insert(class_item->data.value.class.loc_table, &(current_context.identifier_item->key), &current_context.identifier_item) == HASH_INSERT_FOUND) {
#ifdef DEBUG
                    fprintf(stderr, "semantic error, variable redefinition, on line: %d\n", token->line);
#endif
                    throw_error(E_SEM);
                } else {
                    current_context.identifier_item->data.value.variable.is_init = false;
                    current_context.identifier_item->data.value.variable.type = type_tmp;
                    current_context.identifier_item->data.type = VARIABLE;

#ifdef DEBUG
                    fprintf(stderr, "Variable inserted: %d\n", token->line);
#endif
                }
            }
        } else if (global_phase == P_SECOND) {
            if (current_context.context_type == C_FUNCTION) {
                t_item * class_item;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);
                t_item * function_item;
                hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &function_item);

                int type_tmp = current_context.identifier_item->data.type;

                if(hash_table_insert(function_item->data.value.function.loc_table, &current_context.identifier_item->key, &current_context.identifier_item) == HASH_INSERT_FOUND) {
#ifdef DEBUG
                    fprintf(stderr, "semantic error, variable redefinition, on line: %d\n", token->line);
#endif
                    throw_error(E_SEM);
                } else {

                    t_item * tmp_item = NULL;
                    hash_table_search(class_item->data.value.class.loc_table, &current_context.identifier_item->key, &tmp_item);

                    if(tmp_item != NULL) {
                        if(tmp_item->data.type == FUNCT) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, variable redefinition, on line: %d\n", token->line);
#endif
                            throw_error(E_SEM);
                        }
                    }

                    current_context.identifier_item->data.value.variable.is_init = false;
                    current_context.identifier_item->data.value.variable.type = type_tmp;
                    current_context.identifier_item->data.type = VARIABLE;
#ifdef DEBUG
                    fprintf(stderr, "Variable inserted, type: %d\n", current_context.identifier_item->data.value.variable.type);
#endif
                }
            } else {
                current_context.identifier_item->data.type = VARIABLE;
            }
        }
    }
}

void parse_param_list() {
    if(token->state == S_RBRACKET) {
        //return - no parameters function
#ifdef DEBUG
        fprintf(stderr, "token je %s, no parameters function on line: %d\n", token->data, token->line);
#endif
    } else {
        parse_param();
        parse_param_list2();
    }
}

void parse_param_list2() {
    if (token->state == S_RBRACKET) {
        //return - no parameters follows
#ifdef DEBUG
        fprintf(stderr, "token je %s, no parameters follows on line: %d\n", token->data, token->line);
#endif
    } else {
        if (token->state == S_COMMA) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //NEXT TOKEN
            token = getToken();

            parse_param();
            parse_param_list2();
        } else {
            //SYNTAX ERROR - comma ',' expected
            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - comma ',' expected, on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    }
}

void parse_param() {
    int type = parse_type();

    if(type == T_VOID) {
        //SYNTAX ERROR - void parameter
        check_lex_error();
#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX ERROR - void parameter, on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }

    if (token->state == S_IDENTIF) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //SYNTAX OK

        if(global_phase == P_FIRST) {
            t_item * class_item;
            hash_table_search(current_context.global_table, current_context.class_key, &class_item);

            t_item * function_item;
            hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &function_item);

            if (function_item->data.value.function.first_param == NULL) {
                //first parameter of function

                //save identifier to symbol table
                //create key for param
                string key;
                init_value_str(&key, token->data);

                function_item->data.value.function.param_count++;

                //item init
                t_item * item;
                if (global_phase == P_FIRST) {
                    item = gc_alloc_pointer(sizeof(t_item));
                    item->key = key;
                }

                //add parameter to local table of function
                hash_table_insert(function_item->data.value.function.loc_table, &key, &item);
                if(global_phase == P_FIRST) {
                    item->data.type = VARIABLE;
                    item->data.value.variable.type = type;
                }

                string param_key;
                init_value_str(&param_key, token->data);

                //item init
                t_item * param_item = NULL;
                if (global_phase == P_FIRST) {
                    param_item = gc_alloc_pointer(sizeof(t_item));
                    param_item->key = key;
                    param_item->data.type = FUNCT_ITEM;
                    param_item->data.value.fun_item.type = type;
                    param_item->data.value.fun_item.next = NULL;
                }

                function_item->data.value.function.first_param = param_item;
                function_item->data.value.function.first_param->next = NULL;

            } else {
                //save identifier to symbol table
                //create key for param
                string key;
                init_value_str(&key, token->data);

                function_item->data.value.function.param_count++;

                //item init
                t_item * item;
                if (global_phase == P_FIRST) {
                    item = gc_alloc_pointer(sizeof(t_item));
                    item->key = key;
                }

                //add parameter to local table of function
                hash_table_insert(function_item->data.value.function.loc_table, &key, &item);
                item->data.type = VARIABLE;
                item->data.value.variable.type = type;

                //helper variable
                t_item * tmp;
                tmp = function_item->data.value.function.first_param;

                while(tmp->next != NULL) {
                    tmp = tmp->next;
                }

                //save identifier to symbol table
                //create key for param
                string param_key;
                init_value_str(&param_key, token->data);

                tmp->next = gc_alloc_pointer(sizeof(t_item));
                tmp->next->key = param_key;
                tmp->next->data.type = FUNCT_ITEM;
                tmp->next->data.value.fun_item.type = type;
                tmp->next->next = NULL;
            }
        }

        //NEXT TOKEN
        token = getToken();
    } else {
        //SYNTAX ERROR - identifier expected
        check_lex_error();
#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX ERROR - identifier expected, on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }
}

void parse_call_param_list(t_item * fun_item) {
    if (token->state == S_RBRACKET) {
        //return - no parameters function
        if(global_phase == P_SECOND) {
            if (fun_item->data.value.function.param_count != number_of_parameters_in_call) {
#ifdef DEBUG
                fprintf(stderr, "Less parameters in call\n");
#endif
                throw_error(E_TYPE);
            }
        }
#ifdef DEBUG
        fprintf(stderr, "token je %s, no parameters function1 on line: %d\n", token->data, token->line);
#endif

        if(global_phase == P_SECOND) {

            int builtin = check_builtin(fun_item);
            if(builtin != -1) {
                switch (builtin) {
                    case FUNCT_RINT:
                        generate_instruction(actualList, In_LI,op1, op2, op3);
                        break;
                    case FUNCT_RDOUBLE:
                        generate_instruction(actualList, In_LD,op1, op2, op3);
                        break;
                    case FUNCT_RSTR:
                        generate_instruction(actualList, In_LS,op1, op2, op3);
                        break;
                    case FUNCT_PRINT:
                        generate_instruction(actualList, In_PRT,op1, op2, op3);
                        break;
                    case FUNCT_COMP:
                        generate_instruction(actualList, In_STRC,op1, op2, op3);
                        break;
                    case FUNCT_FIND:
                        generate_instruction(actualList, In_STRF,op1, op2, op3);
                        break;
                    case FUNCT_SRT:
                        generate_instruction(actualList, In_STRS,op1, op2, op3);
                        break;
                    case FUNCT_SUBSTR:
                        generate_instruction(actualList, In_SSTR,op1, op2, op3);
                        break;
                    case FUNCT_LEN:
                        generate_instruction(actualList, In_STRL,op1, op2, op3);
                        break;
                    default:
                        break;
                }

                //reset global helper variables for builtin
                op1 = NULL;
                op2 = NULL;
                op3 = NULL;

            } else {
                //create instruction for calling function with pointer to function in TS
                generate_instruction(actualList, In_FUN_CALL, fun_item, NULL, NULL);
            }
        }

        //reset
        number_of_parameters_in_call = 0;

    } else {

        tmpCallParamList = gc_alloc_pointer(sizeof(tInstrList));
        listInit(tmpCallParamList);

        int builtin = -1;

        if(global_phase == P_SECOND) {
            builtin = check_builtin(fun_item);
        }

        if(builtin != -1) {
            //PRECEDENCE ANALYSIS
            parseExpression(global_phase, actualList);
        } else {
            //PRECEDENCE ANALYSIS
            parseExpression(global_phase, tmpCallParamList);
        }

        if(global_phase == P_SECOND) {
            actual_parameter = fun_item->data.value.function.first_param;
        }

            if (actual_parameter != NULL) {
                if (result != NULL) {
                    if (builtin == FUNCT_PRINT) {
                        if(result->data.value.constant.type != T_BOOL || result->data.value.variable.type != T_BOOL) {
                            number_of_parameters_in_call++;
#ifdef DEBUG
                            fprintf(stderr, "token je %s, type control of parameter ok on line: %d\n", token->data, token->line);
#endif

                            if(builtin != -1) {
                                if(number_of_parameters_in_call == 1) {
                                    op1 = result;
                                } else if(number_of_parameters_in_call == 2) {
                                    op2 = result;
                                } else if(number_of_parameters_in_call == 3) {
                                    op3 = result;
                                }
                            } else {
                                generate_instruction(tmpCallParamList, In_SET_PARAM,result,&actual_parameter->key, NULL);
                            }


                            actual_parameter = actual_parameter->next;
                        } else {
#ifdef DEBUG
                            fprintf(stderr, "token je %s, type error on line: %d\n", token->data, token->line);
#endif
                            throw_error(E_TYPE);
                        }
                    } else {
                        if(actual_parameter->data.value.fun_item.type == result->data.value.constant.type) {
                            number_of_parameters_in_call++;
#ifdef DEBUG
                            fprintf(stderr, "token je %s, type control of parameter ok on line: %d\n", token->data, token->line);
#endif

                            if(builtin != -1) {
                                if(number_of_parameters_in_call == 1) {
                                    op1 = result;
                                } else if(number_of_parameters_in_call == 2) {
                                    op2 = result;
                                } else if(number_of_parameters_in_call == 3) {
                                    op3 = result;
                                }
                            } else {
                                generate_instruction(tmpCallParamList, In_SET_PARAM, result, &actual_parameter->key, NULL);
                            }


                            actual_parameter = actual_parameter->next;
                        } else {

                            if(actual_parameter->data.value.fun_item.type == T_DOUBLE) {
                                if(result->data.type == CONSTANT) {
                                    if(result->data.value.constant.type != T_INT) {
#ifdef DEBUG
                                        fprintf(stderr, "token je %s, type error on line: %d\n", token->data, token->line);
#endif
                                        throw_error(E_TYPE);
                                    } else {
                                        if(global_phase == P_SECOND) {
                                            number_of_parameters_in_call++;
                                        }
                                    }
                                } else {
                                    if(result->data.value.variable.type != T_INT) {
#ifdef DEBUG
                                        fprintf(stderr, "token je %s, type error on line: %d\n", token->data, token->line);
#endif
                                        throw_error(E_TYPE);
                                    } else {
                                        if(global_phase == P_SECOND) {
                                            number_of_parameters_in_call++;
                                        }
                                    }
                                }

                            } else {
#ifdef DEBUG
                                fprintf(stderr, "token je %s, type error on line: %d\n", token->data, token->line);
#endif
                                throw_error(E_TYPE);
                            }

                        }
                    }

                } else {
#ifdef DEBUG
                    fprintf(stderr, "token je %s, wrong number of parameters on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_TYPE);
                }
            } else if (result == NULL) {
#ifdef DEBUG
                fprintf(stderr, "token je %s, no parameters function2 on line: %d\n", token->data, token->line);
#endif
            } else {
                number_of_parameters_in_call++;
            }
        }
    parse_call_param_list2(fun_item);
}

void parse_call_param_list2(t_item * fun_item) {
    if (token->state == S_RBRACKET) {

        if(global_phase == P_SECOND) {
            if(fun_item->data.value.function.param_count != number_of_parameters_in_call) {
#ifdef DEBUG
                fprintf(stderr, "Less parameters in call\n");
#endif
                throw_error(E_TYPE);
            }

            int builtin = check_builtin(fun_item);
            if(builtin != -1) {
                switch (builtin) {
                    case FUNCT_RINT:
                        generate_instruction(actualList, In_LI,op1, op2, op3);
                        break;
                    case FUNCT_RDOUBLE:
                        generate_instruction(actualList, In_LD,op1, op2, op3);
                        break;
                    case FUNCT_RSTR:
                        generate_instruction(actualList, In_LS,op1, op2, op3);
                        break;
                    case FUNCT_PRINT:
                        generate_instruction(actualList, In_PRT,op1, op2, op3);
                        break;
                    case FUNCT_COMP:
                        generate_instruction(actualList, In_STRC,op1, op2, op3);
                        break;
                    case FUNCT_FIND:
                        generate_instruction(actualList, In_STRF,op1, op2, op3);
                        break;
                    case FUNCT_SRT:
                        generate_instruction(actualList, In_STRS,op1, op2, op3);
                        break;
                    case FUNCT_SUBSTR:
                        generate_instruction(actualList, In_SSTR,op1, op2, op3);
                        break;
                    case FUNCT_LEN:
                        generate_instruction(actualList, In_STRL,op1, op2, op3);
                        break;
                    default:
                        break;
                }

                //reset global helper variables for builtin
                op1 = NULL;
                op2 = NULL;
                op3 = NULL;

            } else {
                //create instruction for calling function with pointer to function in TS
                generate_instruction(actualList, In_FUN_CALL, fun_item, tmpCallParamList, NULL);
            }
        }

        //return - no parameters follows
#ifdef DEBUG
        fprintf(stderr, "token je %s, no parameters follows on line: %d\n", token->data, token->line);
#endif
        number_of_parameters_in_call = 0;
    } else {
        if (token->state == S_COMMA) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            clearMem();
            saving = true;

            //NEXT TOKEN
            token = getToken();

            if(token->state == S_RBRACKET || token->state == S_COMMA) {
#ifdef DEBUG
                fprintf(stderr, "Empty parameter in function call\n");
#endif
                throw_error(E_SYN);
            }

            saving = false;

            //PRECEDENCE ANALYSIS
            parseExpression(global_phase, tmpCallParamList);
            if(result != NULL) {
                if (global_phase == P_SECOND) {
                    if (actual_parameter != NULL) {
                        if(actual_parameter->data.value.fun_item.type == result->data.value.constant.type) {
                            number_of_parameters_in_call++;
#ifdef DEBUG
                            fprintf(stderr, "token je %s, type control of parameter ok on line: %d\n", token->data, token->line);
#endif
                            int builtin = check_builtin(fun_item);

                            if(builtin != -1) {
                                if(number_of_parameters_in_call == 1) {
                                    op1 = result;
                                } else if(number_of_parameters_in_call == 2) {
                                    op2 = result;
                                } else if(number_of_parameters_in_call == 3) {
                                    op3 = result;
                                }
                            } else {
                                generate_instruction(tmpCallParamList, In_SET_PARAM, result, &actual_parameter->key, NULL);
                            }

                            actual_parameter = actual_parameter->next;
                        } else {
#ifdef DEBUG
                            fprintf(stderr, "token je %s, type error on line: %d\n", token->data, token->line);
#endif
                            throw_error(E_TYPE);
                        }
                    } else {
#ifdef DEBUG
                        fprintf(stderr, "token je %s, wrong number of parameters on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_TYPE);
                    }
                }
            }

            parse_call_param_list2(fun_item);
        } else {
            //SYNTAX ERROR - comma ',' expected

            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - identifier expected, on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    }
}

void parse_stat_list() {

    if (token->state == S_R_BRACKET) {
        //no statements follows
#ifdef DEBUG
        fprintf(stderr, "token je %s, no statments follows on line: %d\n", token->data, token->line);
#endif
        if (current_context.context_type == C_FUNCTION) {

            if(complex_stat_stack->top == -1) {
                t_item * class_item;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                t_item * function_item;
                hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &function_item);

                if(global_phase == P_SECOND) {
                    if(function_item->data.value.function.ret_type != T_VOID) {
                        if(!is_return) {
#ifdef DEBUG
                            fprintf(stderr, "token je %s, Missing return in non-void function, on line: %d\n", token->data, token->line);
#endif
                            throw_error(E_INIT);
                        }
                    }

                    is_return = false;
                }

                current_context.context_type = C_CLASS;
            }
        }

        if(global_phase == P_SECOND) {
#ifdef DEBUG
            fprintf(stderr, "----Instruction tape of function------\n");
#endif
            print_elements_of_list(*actualList);
#ifdef DEBUG
            fprintf(stderr, "----END of Instruction tape of function------\n");
#endif
        }

    } else {
        parse_stat();
        parse_stat_list();
    }
}

void parse_complex_stat() {
    if (token->state == S_L_BRACKET) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif

        //set context to complex stat
        addItem(&complex_stat_stack, (void *) token );

        //NEXT TOKEN
        token = getToken();

        parse_stat_list();

        if (token ->state == S_R_BRACKET) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //SYNTAX OK

            //unset context to complex stat
            if(complex_stat_stack->top != -1) {
                popItem(complex_stat_stack);
            }


            //NEXT TOKEN
            token = getToken();
        } else {
            //SYNTAX ERROR - missing closing bracket '}'
            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, SYNTAX ERROR - missing closing bracket '}', on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    } else {
        //SYNTAX ERROR - missing opening bracket '{'
        check_lex_error();
#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX ERROR - missing opening bracket '{', on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }
}

void parse_stat() {
    if (token->state == S_IDENTIF || token->state == S_FULL_IDENTIF) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        t_item * first_item = NULL;

        if (token->state == S_IDENTIF) {
            if (global_phase == P_SECOND) {
                string key;
                init_value_str(&key, token->data);

                t_item *class_item = NULL;
                hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                t_item * fun_item = NULL;
                hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                t_item *id_item = NULL;
                hash_table_search(fun_item->data.value.function.loc_table, &key, &id_item);

                if (id_item == NULL) {
                    hash_table_search(class_item->data.value.class.loc_table, &key, &id_item);

                    if(id_item == NULL) {
#ifdef DEBUG
                        fprintf(stderr, "semantic error, global variable1: %s doesn't exist: %d\n", key.str, token->line);
#endif
                        throw_error(E_SEM);
                    }
                }

                //save type to next if (cannot assign to function)
                first_item = id_item;

#ifdef DEBUG
                fprintf(stderr, "id_item:  %d\n",first_item->data.type);
#endif
            }

        } else if (token->state == S_FULL_IDENTIF) {

            if (global_phase == P_SECOND) {

                int x = 0;
                char *class = gc_alloc_pointer(token->size);
                while(token->data[x] != '.') {
                    class[x] = token->data[x];
                    x++;

                }
                class[x] = '\0';
                x++;
                char *item_tmp = gc_alloc_pointer(token->size);
                int y = 0;
                while(token->data[x] != '\0'){
                    item_tmp[y] = token->data[x];
                    x++;
                    y++;
                }
                item_tmp[y]= '\0';

                string class_key;
                init_value_str(&class_key, class);

                t_item *class_item = NULL;
                hash_table_search(current_context.global_table, &class_key, &class_item);

                if (class_item == NULL) {
#ifdef DEBUG
                    fprintf(stderr, "semantic error, class: %s doesn't exist: %d\n", class_key.str, token->line);
#endif
                    throw_error(E_SEM);
                } else {
                    string key;
                    init_value_str(&key, item_tmp);

                    t_item *id_item = NULL;
                    hash_table_search(class_item->data.value.class.loc_table, &key, &id_item);

                    if (id_item == NULL) {
#ifdef DEBUG
                        fprintf(stderr, "semantic error, variable2: %s doesn't exist: %d\n", key.str, token->line);
#endif
                        throw_error(E_SEM);
                    }

                    //save type to next if (cannot assign to function)
                    first_item = id_item;

#ifdef DEBUG
                    fprintf(stderr, "id_item:  %d\n",first_item->data.type);
#endif
                }
            }
        }

        //NEXT TOKEN
        token = getToken();

        if (token->state == S_ASSIGMENT) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            if(global_phase == P_SECOND) {
                if (first_item->data.type != VARIABLE) {
#ifdef DEBUG
                    fprintf(stderr, "Cannot assign to function: %d\n", first_item->data.type);
#endif
                    throw_error(E_SEM);
                }
            }
            saving = true;

            //Simulation of rule stat2

            //NEXT TOKEN
            token = getToken();

            if (token->state == S_IDENTIF || token->state == S_FULL_IDENTIF) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                t_item * item = NULL;

                if (token->state == S_IDENTIF) {
                    if (global_phase == P_SECOND) {
                        string key;
                        init_value_str(&key, token->data);

                        t_item *class_item = NULL;
                        hash_table_search(current_context.global_table, current_context.class_key, &class_item);

                        t_item *fun_item = NULL;
                        hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);

                        t_item *id_item = NULL;
                        hash_table_search(fun_item->data.value.function.loc_table, &key, &id_item);

                        t_item * global_item = NULL;
                        hash_table_search(class_item->data.value.class.loc_table, &key, &global_item);

                        if (id_item == NULL) {

                            if(global_item == NULL) {
#ifdef DEBUG
                                fprintf(stderr, "semantic error, variable4: %s %s %s doesn't exist: %d\n",current_context.class_key->str,current_context.function_key->str, key.str, token->line);
#endif
                                throw_error(E_SEM);
                            } else {
                                item = global_item;
                            }

                        } else {
                            item = id_item;
                        }


                    }

                } else if (token->state == S_FULL_IDENTIF) {

                    if (global_phase == P_SECOND) {
                        int x = 0;
                        char *class = gc_alloc_pointer(token->size);
                        while(token->data[x] != '.') {
                            class[x] = token->data[x];
                            x++;
                        }
                        class[x] = '\0';
                        x++;

                        char *item_tmp = gc_alloc_pointer(token->size);
                        int y = 0;
                        while(token->data[x] != '\0'){
                            item_tmp[y] = token->data[x];
                            x++;
                            y++;
                        }
                        item_tmp[y]= '\0';

                        string class_key;
                        init_value_str(&class_key, class);

#ifdef DEBUG
                        fprintf(stderr, "class key: %s\n",class_key.str);
#endif

                        t_item *class_item = NULL;
                        hash_table_search(current_context.global_table, &class_key, &class_item);

                        if (class_item == NULL) {
#ifdef DEBUG
                            fprintf(stderr, "semantic error, class: %s doesn't exist: %d\n", class_key.str, token->line);
#endif
                            throw_error(E_SEM);
                        } else {
                            string key;
                            init_value_str(&key, item_tmp);

                            t_item *id_item = NULL;
                            hash_table_search(class_item->data.value.class.loc_table, &key, &id_item);


                            if (id_item == NULL) {
#ifdef DEBUG
                                fprintf(stderr, "semantic error, variable5: %s doesn't exist: %d\n", key.str, token->line);
#endif
                                throw_error(E_SEM);
                            }

#ifdef DEBUG
                            fprintf(stderr, "class_item key %s id item key: %s\n", class_key.str, key.str);
#endif

                            item = id_item;
                        }
                    }
                }

                //NEXT TOKEN
                token = getToken();

                if (token->state == S_LBRACKET) { //assignment of function
#ifdef DEBUG
                    fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                    saving = false;
                    clearMem();
#ifdef DEBUG
                    fprintf(stderr, "som tu \n");
#endif


                    if(global_phase == P_SECOND) {
                        if (item->data.type == VARIABLE) {
#ifdef DEBUG
                            fprintf(stderr, "Cannot call variable: %d\n", item->data.type);
#endif
                            throw_error(E_SEM);
                        }

                        if(item->data.value.function.ret_type == T_VOID) {
#ifdef DEBUG
                            fprintf(stderr, "token je %s, assignment of void function, on line: %d\n", token->data, token->line);
#endif
                            throw_error(E_INIT);
                        }
                    }

                    //parameter is pointer to actual function
                    parse_call_param_list(item);

                    if (token->state == S_RBRACKET) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                        //NEXT TOKEN
                        token = getToken();

                        if (check_semicolon()) {
                            //SYNTAX OK
                        } else {
                            //SYNTAX ERROR expected ';'
                            check_lex_error();
#ifdef DEBUG
                            fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                            throw_error(E_SYN);
                        }

                        //type control of assignment function
                        if(global_phase == P_SECOND) {
                            if (first_item->data.value.variable.type == item->data.value.function.ret_type) {
#ifdef DEBUG
                                fprintf(stderr, "Types in assignemnt are equal\n");
#endif
                                //GENERATE INSTRUCTION
                                //add instruction to actual function
                                first_item->data.value.variable.is_init = true;
                                generate_instruction(actualList, In_ASS, first_item, NULL, NULL);
                            } else {
#ifdef DEBUG
                                fprintf(stderr, "Error, types in assignemnt are not equal\n");
#endif
                                throw_error(E_TYPE);
                            }
                        }
                    }
                } else {
                    //PRECEDENCE ANALYSIS
                    saving = false;

                    print_elements_of_list(*actualList);

                    parseExpression(global_phase, actualList);

                    if(global_phase == P_SECOND) {
                        //GENERATE INSTRUCTION
                        current_context.identifier_item->data.value.variable.is_init = true;
                        generate_instruction(actualList, In_ASS, first_item, NULL, NULL);
                    }


                    if (check_semicolon()) {
                    //SYNTAX OK
                    } else {
                    //SYNTAX ERROR expected ';'
                        check_lex_error();
#ifdef DEBUG
                        fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_SYN);
                    }
                }

            } else {
                //PRECEDENCE ANALYSIS
                saving = false;
                parseExpression(global_phase, actualList);

                if(global_phase == P_SECOND) {
#ifdef DEBUG
                    fprintf(stderr, "type of result: %d\n", result->data.value.constant.type);
#endif

                    if(first_item->data.value.variable.type != result->data.value.constant.type) {
#ifdef DEBUG
                        fprintf(stderr, "id type: %d, result type: %d\n", current_context.identifier_item->data.value.variable.type, result->data.value.constant.type);
#endif

                        if((current_context.identifier_item->data.value.variable.type  == T_DOUBLE) && (result->data.value.constant.type == T_INT)) {
#ifdef DEBUG
                            fprintf(stderr, "assignment int to double\n");
#endif
                        } else {
#ifdef DEBUG
                            fprintf(stderr, "cannot assign to variable, invalid types\n");
#endif
                            throw_error(E_TYPE);
                        }
                    }

                    //GENERATE INSTRUCTION
                    current_context.identifier_item->data.value.variable.is_init = true;
                    generate_instruction(actualList, In_ASS, current_context.identifier_item, NULL, NULL);
                }

                if (check_semicolon()) {
                    //SYNTAX OK
                } else {
                    //SYNTAX ERROR expected ';'
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            }
        } else if (token->state == S_LBRACKET) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //Simulation of rule stat2

            //FUNCTION CALL IS IN parse call param list
            parse_call_param_list(first_item);

            if (token->state == S_RBRACKET) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                //NEXT TOKEN
                token = getToken();

                if(check_semicolon()) {
                    //SYNTAX OK
                } else {
                    //SYNTAX ERROR expected ';'
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            }
        }

    } else if (token ->state == S_KEYWORD) {
        if (!strcmp(token->data, "int")) { // Integer identifier declaration parsing
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif

            //cannot declare variables in complex statment!
            if (complex_stat_stack->top != -1) {
#ifdef DEBUG
                fprintf(stderr, "token je %s, semantic error, definition of variable on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }

            int type = T_INT;

            //NEXT TOKEN
            token = getToken();

            if (token->state == S_IDENTIF) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif

                //save identifier to symbol table
                //create key for variable
                string key;
                init_value_str(&key, token->data);

                //item init
                t_item * item;
                item = gc_alloc_pointer(sizeof(t_item));
                item->key = key;
                item->data.type = type; //saving type for parse_init

                current_context.identifier_item = item;

                //NEXT TOKEN
                token = getToken();

                parse_init();

                if (check_semicolon()) {
                    //SYNTAX OK
                } else {
                    //SYNTAX ERROR expected ';'
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            }

        } else if (!strcmp(token->data, "double")) { // Double identifier declaration parsing
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //cannot declare variables in complex statment!
            if (complex_stat_stack->top != -1) {
#ifdef DEBUG
                fprintf(stderr, "token je %s, semantic error, definition of variable on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }

            int type = T_DOUBLE;

            //NEXT TOKEN
            token = getToken();

            if (token->state == S_IDENTIF) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif

                //save identifier to symbol table
                //create key for variable
                string key;
                init_value_str(&key, token->data);

                //item init
                t_item * item;
                item = gc_alloc_pointer(sizeof(t_item));
                item->key = key;
                item->data.type = type; //saving type to parse_init

                current_context.identifier_item = item;

                //NEXT TOKEN
                token = getToken();

                parse_init();

                if (check_semicolon()) {
                    //SYNTAX OK
                } else {
                    //SYNTAX ERROR expected ';'
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            }

        } else if (!strcmp(token->data, "String")) { // String identifier declaration parsing
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //cannot declare variables in complex statment!
            if (complex_stat_stack->top != -1) {
#ifdef DEBUG
                fprintf(stderr, "token je %s, semantic error, definition of variable on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }

            int type = T_STR;

            //NEXT TOKEN
            token = getToken();

            if (token->state == S_IDENTIF) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                //save identifier to symbol table
                //create key for variable
                string key;
                init_value_str(&key, token->data);

                //item init
                t_item * item;
                item = gc_alloc_pointer(sizeof(t_item));
                item->key = key;
                item->data.type = type; //saving type for parse_init

                current_context.identifier_item = item;

#ifdef DEBUG
                fprintf(stderr, "current context string type: %d\n", current_context.identifier_item->data.type);
#endif

                //NEXT TOKEN
                token = getToken();

                parse_init();

                if (check_semicolon()) {
                    //SYNTAX OK
                } else {
                    //SYNTAX ERROR expected ';'
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            }
        } else if (!strcmp(token->data, "return")) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //PRECEDENCE ANALYSIS
            parseExpression(global_phase, actualList);

            t_item *class_item = NULL;
            hash_table_search(current_context.global_table, current_context.class_key, &class_item);

            t_item *fun_item = NULL;
            hash_table_search(class_item->data.value.class.loc_table, current_context.function_key, &fun_item);


            if(global_phase == P_SECOND) {
                if (result == NULL) {
                    if (fun_item->data.value.function.ret_type != T_VOID) {
#ifdef DEBUG
                        fprintf(stderr, "empty return in non-void function\n");
#endif
                        throw_error(E_TYPE);
                    }
                } else {
                    if (fun_item->data.value.function.ret_type != result->data.value.constant.type) {
#ifdef DEBUG
                        fprintf(stderr, "return type doesn't match\n");
#endif
                        throw_error(E_TYPE);
                    }
                }

                //GENERATE INSTRUCTION
                generate_instruction(actualList, In_SET_RESULT, result, NULL, NULL);
                is_return = true;
            }

            if (check_semicolon()) {
                //SYNTAX OK
            } else {
                //SYNTAX ERROR expected ';'
                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "token je %s, SYNTAX ERROR expected ';', on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }
        } else if (!strcmp(token->data, "if")) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif

            //NEXT TOKEN
            token = getToken();

            if (token->state == S_LBRACKET) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                //PRECEDENCE ANALYSIS
                parseExpression(global_phase, actualList);

                if(global_phase == P_SECOND) {
                    if(result == NULL) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s, Empty expression in if, on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_TYPE);
                    }
                    if(result->data.value.constant.type != T_BOOL || result->data.value.variable.type != T_BOOL) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s, TYPE error not bool expression in if, on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_TYPE);
                    }
                }


                print_elements_of_list(*actualList);

                if (token->state == S_RBRACKET) {
#ifdef DEBUG
                    fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                    //generate instruction for jump, adress is set in start of else!
                    tListItem *  tmp = NULL;
                    if(global_phase == P_SECOND) {
                        generate_instruction(actualList, In_JMPF,NULL,NULL,NULL);
                        tmp = (tListItem *) listGetPointerLast(actualList);
                    }

                    //NEXT TOKEN
                    token = getToken();

                    parse_complex_stat();

                    tListItem * else_skip = NULL;
                    if(global_phase == P_SECOND) {
                        //add last instruction after else later in else
                        generate_instruction(actualList, In_JMP,NULL,NULL,NULL);
                        else_skip = (tListItem *) listGetPointerLast(actualList);
                    }

                    if (!strcmp(token->data, "else")) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                        //add adress of if jump
                        if(global_phase == P_SECOND) {
                            //label for JMPF from if
                            generate_instruction(actualList, In_NONE, NULL, NULL, NULL);

                            tListItem * tmp1 = (tListItem *) listGetPointerLast(actualList);
                            tmp->Instruction.addr1 = tmp1;
                        }

                        //NEXT TOKEN
                        token = getToken();

                        parse_complex_stat();

                        if(global_phase == P_SECOND) {
                            //label for JMP from if
                            generate_instruction(actualList, In_NONE, NULL, NULL, NULL);

                            tListItem * tmp1 = (tListItem *) listGetPointerLast(actualList);
                            else_skip->Instruction.addr1 = tmp1;
                        }

                    } else {
                        //SYNTAX ERROR - 'else' keyword expected
                        check_lex_error();
#ifdef DEBUG
                        fprintf(stderr, "token je %s, SYNTAX ERROR - 'else' keyword expected, on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_SYN);
                    }

                } else {
                    //SYNTAX ERROR - ')' expected
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR - ')' expected, on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            } else {
                //SYNTAX ERROR - '(' expected
                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "token je %s, SYNTAX ERROR - '(' expected, on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }
        } else if (!strcmp(token->data, "while")) {
#ifdef DEBUG
            fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
            //NEXT TOKEN
            token = getToken();

            if (token->state == S_LBRACKET) {
#ifdef DEBUG
                fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                tListItem * while_label = NULL;
                if(global_phase == P_SECOND) {
                    generate_instruction(actualList, In_NONE, NULL, NULL, NULL);
                    while_label = (tListItem *) listGetPointerLast(actualList);
                }

                //PRECEDENCE ANALYSIS
                parseExpression(global_phase, actualList);

                if(global_phase == P_SECOND) {
                    if(result == NULL) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s, Empty expression in while, on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_TYPE);
                    }
                    if(result->data.value.constant.type != T_BOOL) {
#ifdef DEBUG
                        fprintf(stderr, "token je %s, TYPE error not bool expression in while, on line: %d\n", token->data, token->line);
#endif
                        throw_error(E_TYPE);
                    }
                }

                if (token->state == S_RBRACKET) {
#ifdef DEBUG
                    fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
                    //generate instruction for jump, adress is set after complex stat is parsed

                    tListItem * tmp = NULL;
                    if(global_phase == P_SECOND) {
                        generate_instruction(actualList, In_JMPF,NULL,NULL,NULL);
                        tmp = (tListItem *) listGetPointerLast(actualList);
                    }

                    //NEXT TOKEN
                    token = getToken();

                    parse_complex_stat();

                    //add instruction of jump to instruction which compute bool expression
                    if(global_phase == P_SECOND) {
                        generate_instruction(actualList, In_JMP, while_label, NULL, NULL);

                        generate_instruction(actualList, In_NONE, NULL, NULL, NULL);

                        //adress if not cycling
                        tListItem * tmp1 = (tListItem *) listGetPointerLast(actualList);
                        tmp->Instruction.addr1 =  tmp1;
                    }
                } else {
                    //SYNTAX ERROR - ')' expected
                    check_lex_error();
#ifdef DEBUG
                    fprintf(stderr, "token je %s, SYNTAX ERROR - ')' expected, on line: %d\n", token->data, token->line);
#endif
                    throw_error(E_SYN);
                }
            } else {
                //SYNTAX ERROR - '(' expected
                check_lex_error();
#ifdef DEBUG
                fprintf(stderr, "token je %s, SYNTAX ERROR - '(' expected, on line: %d\n", token->data, token->line);
#endif
                throw_error(E_SYN);
            }
        }  else {
            //SYNTAX ERROR
            check_lex_error();
#ifdef DEBUG
            fprintf(stderr, "token je %s, on line: %d\n", token->data, token->line);
#endif
            throw_error(E_SYN);
        }
    } else if (token->state == S_L_BRACKET) { //complex stat
#ifdef DEBUG
        fprintf(stderr, "token je %s, on line: %d\n", token->data, token->line);
#endif
        parse_complex_stat();
    } else {
        //SYNTAX ERROR
        check_lex_error();
#ifdef DEBUG
        fprintf(stderr, "token je %s, on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }
}

int parse_type() {
    if(!strcmp(token->data, "int")) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //NEXT TOKEN
        token = getToken();

        return T_INT;
    }else if(!strcmp(token->data, "double")) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //NEXT TOKEN
        token = getToken();
        return T_DOUBLE;
    } else if(!strcmp(token->data, "String")) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //NEXT TOKEN
        token = getToken();

        return T_STR;
    } else if(!strcmp(token->data, "void")) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //NEXT TOKEN
        token = getToken();

        return T_VOID;
    } else {
        //SYNTAX ERROR
        check_lex_error();
#ifdef DEBUG
        fprintf(stderr, "token je %s, SYNTAX ERROR, on line: %d\n", token->data, token->line);
#endif
        throw_error(E_SYN);
    }

    return -1;
}

int check_semicolon() {
    if (token->state == S_SEMICOLON) {
#ifdef DEBUG
        fprintf(stderr, "token je %s on line: %d\n", token->data, token->line);
#endif
        //read next token
        token = getToken();

        return 1;
    } else {
        return 0;
    }
}

void check_main_run() {
    string class_key;
    init_value_str(&class_key, "Main");

    string function_key;
    init_value_str(&function_key, "run");

    t_item * class_item = NULL;
    hash_table_search(current_context.global_table, &class_key, &class_item);

    if(class_item != NULL) {

#ifdef DEBUG
        fprintf(stderr, "Main is defined\n");
#endif
        t_item * function_item = NULL;
        hash_table_search(class_item->data.value.class.loc_table, &function_key, &function_item);

        if(function_item != NULL) {
#ifdef DEBUG
            fprintf(stderr, "run is defined\n");
#endif

            //check if run is function
            if (function_item->data.type == FUNCT) {
#ifdef DEBUG
                fprintf(stderr, "run is function\n");
#endif
                if(function_item->data.value.function.ret_type == T_VOID) {
#ifdef DEBUG
                    fprintf(stderr, "run is void\n");
#endif

                    if(function_item->data.value.function.param_count == 0) {
#ifdef DEBUG
                        fprintf(stderr, "run has no parameters\n");
#endif
                    } else {
#ifdef DEBUG
                        fprintf(stderr, "run shouldnt have parameters\n");
#endif
                        throw_error(E_SEM);
                    }
                } else {
#ifdef DEBUG
                    fprintf(stderr, "run with bad retrun type\n");
#endif
                    throw_error(E_SEM);
                }
            } else {
#ifdef DEBUG
                fprintf(stderr, "function run is not defined\n");
#endif
                throw_error(E_SEM);
            }
        } else {
#ifdef DEBUG
            fprintf(stderr, "function run is not defined\n");
#endif
            throw_error(E_SEM);
        }
    } else {
#ifdef DEBUG
        fprintf(stderr, "class Main is not defined\n");
#endif
        throw_error(E_SEM);
    }


}

void create_builtin_ts() {

    //initialization of class ifj16 in symbol table
    string class_key;
    init_value_str(&class_key, "ifj16");

    //init of item
    t_item * item = NULL;
    item = gc_alloc_pointer(sizeof(t_item));
    if(item == NULL) {
        throw_error(E_INTERNAL);
    }

    item->key = class_key;

    if(hash_table_insert(current_context.global_table, &item->key, &item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    item->data.type = CLASS;
    item->data.value.class.loc_table = hash_table_init();

    //get reference to ifj16 class
    t_item * class_item;
    hash_table_search(current_context.global_table, &class_key, &class_item);

    //function: int readInt()
    string readInt_key;
    init_value_str(&readInt_key, "readInt");

    //init of item
    t_item * readInt_item = NULL;
    readInt_item = gc_alloc_pointer(sizeof(t_item));
    if(readInt_item == NULL) {
        throw_error(E_INTERNAL);
    }

    readInt_item->key = readInt_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &readInt_item->key, &readInt_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    readInt_item->data.type = FUNCT_RINT;
    readInt_item->data.value.function.loc_table = hash_table_init();
    readInt_item->data.value.function.instrList = NULL;
    readInt_item->data.value.function.first_param = NULL;
    readInt_item->data.value.function.ret_type = T_INT;
    readInt_item->data.value.function.param_count = 0;

    //function: int readDouble()
    string readDouble_key;
    init_value_str(&readDouble_key, "readDouble");

    //init of item
    t_item * readDouble_item = NULL;
    readDouble_item = gc_alloc_pointer(sizeof(t_item));
    if(readDouble_item == NULL) {
        throw_error(E_INTERNAL);
    }

    readDouble_item->key = readDouble_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &readDouble_item->key, &readDouble_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    readDouble_item->data.type = FUNCT_RDOUBLE;
    readDouble_item->data.value.function.loc_table = hash_table_init();
    readDouble_item->data.value.function.instrList = NULL;
    readDouble_item->data.value.function.first_param = NULL;
    readDouble_item->data.value.function.ret_type = T_DOUBLE;
    readDouble_item->data.value.function.param_count = 0;

    //function: int readString()
    string readString_key;
    init_value_str(&readString_key, "readString");

    //init of item
    t_item * readString_item = NULL;
    readString_item = gc_alloc_pointer(sizeof(t_item));
    if(readString_item == NULL) {
        throw_error(E_INTERNAL);
    }

    readString_item->key = readString_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &readString_item->key, &readString_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    readString_item->data.type = FUNCT_RSTR;
    readString_item->data.value.function.loc_table = hash_table_init();
    readString_item->data.value.function.instrList = NULL;
    readString_item->data.value.function.first_param = NULL;
    readString_item->data.value.function.ret_type = T_STR;
    readString_item->data.value.function.param_count = 0;

    //function: void print( <term> | <concatenation> )
    string print_key;
    init_value_str(&print_key, "print");

    //init of item
    t_item * print_item = NULL;
    print_item = gc_alloc_pointer(sizeof(t_item));
    if(print_item == NULL) {
        throw_error(E_INTERNAL);
    }

    print_item->key = print_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &print_item->key, &print_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    print_item->data.type = FUNCT_PRINT;
    print_item->data.value.function.loc_table = hash_table_init();
    print_item->data.value.function.instrList = NULL;
    print_item->data.value.function.first_param = NULL;
    print_item->data.value.function.ret_type = T_VOID;
    print_item->data.value.function.param_count = 1;

    //create parameters
    //create key for param
    string param1_key;
    init_value_str(&param1_key, "param1");

    //item init
    t_item * param1;
    param1 = gc_alloc_pointer(sizeof(t_item));
    param1->key = param1_key;
    param1->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &param1_key, &param1);
    param1->data.value.variable.type = T_STR; //T_CONCAT, dont know what type comes to print function

    string param_key;
    init_value_str(&param_key, "param1");

    //item init
    t_item * param_item;
    param_item = gc_alloc_pointer(sizeof(t_item));
    param_item->key = param_key;
    param_item->data.type = FUNCT_ITEM;
    param_item->data.value.fun_item.type = T_STR;
    param_item->data.value.fun_item.next = NULL;


    print_item->data.value.function.first_param = param_item;


    //function: int length(String s)
    string length_key;
    init_value_str(&length_key, "length");

    //init of item
    t_item * length_item = NULL;
    length_item = gc_alloc_pointer(sizeof(t_item));
    if(length_item == NULL) {
        throw_error(E_INTERNAL);
    }

    length_item->key = length_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &length_item->key, &length_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    length_item->data.type = FUNCT_LEN;
    length_item->data.value.function.loc_table = hash_table_init();
    length_item->data.value.function.instrList = NULL;
    length_item->data.value.function.first_param = NULL;
    length_item->data.value.function.ret_type = T_INT;
    length_item->data.value.function.param_count = 1;

    //create parameters
    //create key for param
    string length_param1_key;
    init_value_str(&length_param1_key, "length_param1_key");

    //item init
    t_item * length_param1;
    length_param1 = gc_alloc_pointer(sizeof(t_item));
    length_param1->key = param1_key;
    length_param1->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &length_param1_key, &length_param1);
    length_param1->data.value.variable.type = T_STR; //T_CONCAT, dont know what type comes to print function

    string length_param_key;
    init_value_str(&length_param_key, "length_param1_key");

    //item init
    t_item * length_param_item;
    length_param_item = gc_alloc_pointer(sizeof(t_item));
    length_param_item->key = param_key;
    length_param_item->data.type = FUNCT_ITEM;
    length_param_item->data.value.fun_item.type = T_STR;
    length_param_item->data.value.fun_item.next = NULL;


    length_item->data.value.function.first_param = length_param_item;


    //function: String compare(String s, int i, int n)
    string substr_key;
    init_value_str(&substr_key, "substr");

    //init of item
    t_item * substr_item = NULL;
    substr_item = gc_alloc_pointer(sizeof(t_item));
    if(substr_item == NULL) {
        throw_error(E_INTERNAL);
    }

    substr_item->key = substr_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &substr_item->key, &substr_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    substr_item->data.type = FUNCT_SUBSTR;
    substr_item->data.value.function.loc_table = hash_table_init();
    substr_item->data.value.function.instrList = NULL;
    substr_item->data.value.function.first_param = NULL;
    substr_item->data.value.function.ret_type = T_STR;
    substr_item->data.value.function.param_count = 3;

    //create parameters
    //create key for param
    string substr_param1_key;
    init_value_str(&substr_param1_key, "substr_param1_key");

    //item init
    t_item * substr_param1;
    substr_param1 = gc_alloc_pointer(sizeof(t_item));
    substr_param1->key = param1_key;
    substr_param1->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &substr_param1_key, &substr_param1);
    substr_param1->data.value.variable.type = T_INT;

    string substr_param2_key;
    init_value_str(&substr_param2_key, "substr_param2_key");

    //item init
    t_item * substr_param2;
    substr_param2 = gc_alloc_pointer(sizeof(t_item));
    substr_param2->key = param1_key;
    substr_param2->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &substr_param2_key, &substr_param2);
    substr_param2->data.value.variable.type = T_INT;

    string substr_param3_key;
    init_value_str(&substr_param3_key, "substr_param3_key");

    //item init
    t_item * substr_param3;
    substr_param3 = gc_alloc_pointer(sizeof(t_item));
    substr_param3->key = param1_key;
    substr_param3->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &substr_param3_key, &substr_param3);
    substr_param3->data.value.variable.type = T_STR;

    string substr_param_1_key;
    init_value_str(&substr_param_1_key, "substr_param_1_key");

    //item init
    t_item * substr_param1_item;
    substr_param1_item = gc_alloc_pointer(sizeof(t_item));
    substr_param1_item->key = substr_param_1_key;
    substr_param1_item->data.type = FUNCT_ITEM;
    substr_param1_item->data.value.fun_item.type = T_STR;
    substr_param1_item->data.value.fun_item.next = NULL;

    string substr_param_2_key;
    init_value_str(&substr_param_2_key, "substr_param_2_key");

    //item init
    t_item * substr_param2_item;
    substr_param2_item = gc_alloc_pointer(sizeof(t_item));
    substr_param2_item->key = substr_param_2_key;
    substr_param2_item->data.type = FUNCT_ITEM;
    substr_param2_item->data.value.fun_item.type = T_INT;
    substr_param2_item->data.value.fun_item.next = NULL;

    string substr_param_3_key;
    init_value_str(&substr_param_3_key, "substr_param_3_key");

    //item init
    t_item * substr_param3_item;
    substr_param3_item = gc_alloc_pointer(sizeof(t_item));
    substr_param3_item->key = substr_param_3_key;
    substr_param3_item->data.type = FUNCT_ITEM;
    substr_param3_item->data.value.fun_item.type = T_INT;
    substr_param3_item->data.value.fun_item.next = NULL;



    substr_item->data.value.function.first_param = substr_param1_item;
    substr_item->data.value.function.first_param->next = substr_param2_item;
    substr_item->data.value.function.first_param->next->next = substr_param3_item;

    //function: int compare(String s1, String s2)
    string compare_key;
    init_value_str(&compare_key, "compare");

    //init of item
    t_item * compare_item = NULL;
    compare_item = gc_alloc_pointer(sizeof(t_item));
    if(compare_item == NULL) {
        throw_error(E_INTERNAL);
    }

    compare_item->key = compare_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &compare_item->key, &compare_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    compare_item->data.type = FUNCT_COMP;
    compare_item->data.value.function.loc_table = hash_table_init();
    compare_item->data.value.function.instrList = NULL;
    compare_item->data.value.function.first_param = NULL;
    compare_item->data.value.function.ret_type = T_INT;
    compare_item->data.value.function.param_count = 2;

    //create parameters
    //create key for param
    string compare_param1_key;
    init_value_str(&compare_param1_key, "compare_param1_key");

    //item init
    t_item * compare_param1;
    compare_param1 = gc_alloc_pointer(sizeof(t_item));
    compare_param1->key = param1_key;
    compare_param1->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &compare_param1_key, &compare_param1);
    compare_param1->data.value.variable.type = T_STR;

    string compare_param2_key;
    init_value_str(&compare_param2_key, "compare_param2_key");

    //item init
    t_item * compare_param2;
    compare_param2 = gc_alloc_pointer(sizeof(t_item));
    compare_param2->key = param1_key;
    compare_param2->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &compare_param2_key, &compare_param2);
    compare_param2->data.value.variable.type = T_STR;

    string compare_param_1_key;
    init_value_str(&compare_param_1_key, "compare_param_1_key");

    //item init
    t_item * compare_param1_item;
    compare_param1_item = gc_alloc_pointer(sizeof(t_item));
    compare_param1_item->key = compare_param_1_key;
    compare_param1_item->data.type = FUNCT_ITEM;
    compare_param1_item->data.value.fun_item.type = T_STR;
    compare_param1_item->data.value.fun_item.next = NULL;

    string compare_param_2_key;
    init_value_str(&compare_param_2_key, "compare_param_2_key");

    //item init
    t_item * compare_param2_item;
    compare_param2_item = gc_alloc_pointer(sizeof(t_item));
    compare_param2_item->key = compare_param_2_key;
    compare_param2_item->data.type = FUNCT_ITEM;
    compare_param2_item->data.value.fun_item.type = T_STR;
    compare_param2_item->data.value.fun_item.next = NULL;

    compare_item->data.value.function.first_param = compare_param1_item;
    compare_item->data.value.function.first_param->next = compare_param2_item;

    //function: int find(String s, String search)
    string find_key;
    init_value_str(&find_key, "find");

    //init of item
    t_item * find_item = NULL;
    find_item = gc_alloc_pointer(sizeof(t_item));
    if(find_item == NULL) {
        throw_error(E_INTERNAL);
    }

    find_item->key = find_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &find_item->key, &find_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    find_item->data.type = FUNCT_FIND;
    find_item->data.value.function.loc_table = hash_table_init();
    find_item->data.value.function.instrList = NULL;
    find_item->data.value.function.first_param = NULL;
    find_item->data.value.function.ret_type = T_INT;
    find_item->data.value.function.param_count = 2;

    //create parameters
    //create key for param
    string find_param1_key;
    init_value_str(&find_param1_key, "find_param1_key");

    //item init
    t_item * find_param1;
    find_param1 = gc_alloc_pointer(sizeof(t_item));
    find_param1->key = param1_key;
    find_param1->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &find_param1_key, &find_param1);
    find_param1->data.value.variable.type = T_STR;

    string find_param2_key;
    init_value_str(&find_param2_key, "find_param2_key");

    //item init
    t_item * find_param2;
    find_param2 = gc_alloc_pointer(sizeof(t_item));
    find_param2->key = param1_key;
    find_param2->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &find_param2_key, &find_param2);
    find_param2->data.value.variable.type = T_STR;

    string find_param_1_key;
    init_value_str(&find_param_1_key, "find_param_1_key");

    //item init
    t_item * find_param1_item;
    find_param1_item = gc_alloc_pointer(sizeof(t_item));
    find_param1_item->key = find_param_1_key;
    find_param1_item->data.type = FUNCT_ITEM;
    find_param1_item->data.value.fun_item.type = T_STR;
    find_param1_item->data.value.fun_item.next = NULL;

    string find_param_2_key;
    init_value_str(&find_param_2_key, "find_param_2_key");

    //item init
    t_item * find_param2_item;
    find_param2_item = gc_alloc_pointer(sizeof(t_item));
    find_param2_item->key = find_param_2_key;
    find_param2_item->data.type = FUNCT_ITEM;
    find_param2_item->data.value.fun_item.type = T_STR;
    find_param2_item->data.value.fun_item.next = NULL;

    find_item->data.value.function.first_param = find_param1_item;
    find_item->data.value.function.first_param->next = find_param2_item;

    //function: String sort(String s)
    string sort_key;
    init_value_str(&sort_key, "sort");

    //init of item
    t_item * sort_item = NULL;
    sort_item = gc_alloc_pointer(sizeof(t_item));
    if(sort_item == NULL) {
        throw_error(E_INTERNAL);
    }

    sort_item->key = sort_key;

    if(hash_table_insert(class_item->data.value.class.loc_table, &sort_item->key, &sort_item) != HASH_INSERT_OK) {
        throw_error(E_INTERNAL);
    }

    sort_item->data.type = FUNCT_SRT;
    sort_item->data.value.function.loc_table = hash_table_init();
    sort_item->data.value.function.instrList = NULL;
    sort_item->data.value.function.first_param = NULL;
    sort_item->data.value.function.ret_type = T_STR;
    sort_item->data.value.function.param_count = 1;

    //create parameters
    //create key for param
    string sort_param1_key;
    init_value_str(&sort_param1_key, "sort_param1_key");

    //item init
    t_item * sort_param1;
    sort_param1 = gc_alloc_pointer(sizeof(t_item));
    sort_param1->key = param1_key;
    sort_param1->data.type = VARIABLE;

    //add parameter to local table of function
    hash_table_insert(print_item->data.value.function.loc_table, &sort_param1_key, &sort_param1);
    sort_param1->data.value.variable.type = T_STR;

    //item init
    t_item * sort_param1_item;
    sort_param1_item = gc_alloc_pointer(sizeof(t_item));
    sort_param1_item->key = param_key;
    sort_param1_item->data.type = FUNCT_ITEM;
    sort_param1_item->data.value.fun_item.type = T_STR;
    sort_param1_item->data.value.fun_item.next = NULL;

    sort_item->data.value.function.first_param = sort_param1_item;
}

int check_builtin(t_item * item) {
    switch (item->data.type) {
        case FUNCT_RINT:
            return FUNCT_RINT;
            break;
        case FUNCT_RDOUBLE:
            return FUNCT_RDOUBLE;
            break;
        case FUNCT_RSTR:
            return FUNCT_RSTR;
            break;
        case FUNCT_PRINT:
            return FUNCT_PRINT;
            break;
        case FUNCT_COMP:
            return FUNCT_COMP;
            break;
        case FUNCT_FIND:
            return FUNCT_FIND;
            break;
        case FUNCT_SRT:
            return FUNCT_SRT;
            break;
        case FUNCT_SUBSTR:
            return FUNCT_SUBSTR;
            break;
        case FUNCT_LEN:
            return FUNCT_LEN;
            break;
        default:
            return -1;
    }
}

void throw_error(int code) {
    gc_list_free();
    fclose(source);
    exit(code);
}

void check_lex_error() {
    if(token->state == S_LEX_ERR) {
#ifdef DEBUG
        fprintf(stderr, "LEX ERROR, TOKEN: state: %d data: %s , line %d\n", token->state, token->data, token->line);
#endif
        throw_error(E_LEX);
    } else if (token->state == S_INTER_ERR) {
        throw_error(E_INTERNAL);
    }
}
