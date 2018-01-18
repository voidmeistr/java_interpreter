/************************** scanner.h *****************************************/
/* Subor:               scanner.h  					      */
/* Projekt:             Implementacia interpretu ifj16                        */
/* Varianta zadania:    a/2/II                                                */
/* Meno, login:         Tomáš Strych            xstryc05                      */
/*                      Radoslav Pitoňák        xpiton00                      */
/*                      Andrej Dzilský          xdzils00                      */
/*                      Milan Procházka         xproch91                      */
/*                      Ondřej Břínek           xbrine03                      */
/******************************************************************************/

#ifndef SCANNER_H
#define SCANNER_H
 
#include <stdio.h>


// States of DFA
typedef enum State
{
	S_START,			// 0 - Starting state
	//list of terminals
	S_LEX_ERR,			// 1 - Blind branch of DFA - lexical error
	S_KEYWORD,			// 2 - Key word
	S_IDENTIF,			// 3 - Identifier
	S_INT,				// 4 - Integer
	S_DOUBLE,			// 5 - Double
	S_STR,				// 6 - String
	S_ASSIGMENT,		// 7 - =
	// math
	S_PLUS,				// 8 - +
	S_MINUS,			// 9 - -
	S_DIV,				// 10 - /
	S_MULTIPLY,			// 11 - *
	// logic
	S_EQUAL,			// 12 - ==
	S_NOTEQUAL,			// 13 - !=
	S_LESS,				// 14 - <
	S_LESSOREQ,			// 15 - <=
	S_GREATER,			// 16 - >
	S_GREATEROREQ,		// 17 - >=

	S_COMMA,			// 18 - ,
	S_SEMICOLON,		// 19 - ;
	S_LBRACKET,			// 20 - (
	S_RBRACKET,			// 21 - )
	S_L_BRACKET,		// 22 - {
	S_R_BRACKET,		// 23 - }
	S_FULL_IDENTIF,		// 24 - Full identifier class and her variable or function separated by dot
	S_EOF,				// 25 - End of file
	// Other states of DFA
	S_REAL,				// 26 - encountered dot after number
	S_EXP,				// 27 - encoutered 'e' or 'E' after number
	S_EXPO,				// 28 - exponent after 'e'
	S_EXCL,				// 29 - exlamation mark possible to be !=
	S_LINE_COM,			// 30 - line comment
	S_BL_COM,			// 31 - block comment
	S_INTER_ERR			// 32 - other error like malloc
} tState;

// Token structure
typedef struct Token
{
    tState state;		// State/type of token
    char *data;			// data
    int size;			// how many characters are stored
    int line;			// line where token was loaded
} tToken;

// Global variable for token
extern tToken *token;
//variable for rollback in tokens
extern tToken **tokenMemory;
//variable for rollback recording
extern int saving;
//Creates an empty token
tToken * createToken(void);
//Function frees tokens and sets values as NULL in memory
void clearMem(void);
//Initialization of token before first usage or initialize for next loading of token
int initToken(tToken * token);
//Append single character into token's data , alwayes terminated by '\0'
int charAppend(tToken * token , char c);
//Sets input file 
void setSourceFile(FILE *f);
//Function which returns token with its properities
//Intitializes token and tokenMemory if they weren't already.
tToken * getToken(void);

#endif
