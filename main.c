/* ************************ main.c ********************************************/
/* Subor:               main.c - TODO                                         */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#include <stdio.h>
#include "scanner.h"
#include "parser.h"
#include "frame.h"
#include "interpret.h"
#include "ial.h"

extern GContext current_context;
extern tInstrList * staticVariablesList;

extern FILE * source;

int main(int argc, char** argv) {

    char * filename;

    if (argc > 1) {
        filename = argv[1];

        gc_list_init();

        FILE * f = fopen(filename, "r");
        setSourceFile(f);

        parse(P_FIRST);
        fclose(f);
        f = fopen(filename, "r");
        setSourceFile(f);
        parse(P_SECOND);

        check_main_run();


        /*find run intruction list*/
        string * main_key = gc_alloc_pointer(sizeof(string));
        init_value_str(main_key, "Main");

        string * run_key = gc_alloc_pointer(sizeof(string));
        init_value_str(run_key, "run");

        t_item * class_item = NULL;
        hash_table_search(current_context.global_table, main_key, &class_item);

        t_item * fun_item = NULL;
        hash_table_search(class_item->data.value.class.loc_table, run_key, &fun_item);


        f_table * new_frame = NULL;
        create_frame_from_table(&new_frame, fun_item->data.value.function.loc_table);

        int static_code = StaticInterpret(staticVariablesList);

        print_elements_of_list(*staticVariablesList);

        if(static_code != 0) {
            throw_error(static_code);
        }

        print_elements_of_list(*fun_item->data.value.function.instrList);

        int code = Interpret(fun_item->data.value.function.instrList, new_frame);

        if(code != 0) {
            throw_error(code);
        }

        gc_list_free();
        fclose(f);
    } else {
        fprintf(stderr, "Too few arguments, filename argument needed\n");
    }

    return E_OK;
}
