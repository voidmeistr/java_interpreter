/************************** interpret.h ***************************************/
/* Subor:               interpret.h - TODO                                    */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#ifndef Inter
#define Inter

#include "ilist.h"
#include "frame.h"
#include "ial.h"

/* Supporting functions: */
int conversions(f_item *src1, f_item *src2);
int StatConversions(t_item *src1, t_item *src2);
f_item *frame_search(t_item *orig, f_table *table);
void create_frame_from_table(f_table **frame, h_table *symbol);
int StaticInterpret(tInstrList *code);

/*Main interpretation function: */
int Interpret(tInstrList *code, f_table *frame);
#endif
