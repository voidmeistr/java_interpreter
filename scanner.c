/************************** scanner.h *****************************************/
/* Subor:               scanner.h  					                          */
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
#include <ctype.h>
#include <string.h>
#include "scanner.h"
#include "garbage_collector.h"

//Source file
FILE *source;
//Line counter
int line = 1;
//List of keywords
const char* keyWords[] = {"boolean","break","class","continue","do","double",
                          "else","false","for","if","int","return","String",
                          "static","true","void","while"};
// Number of kew words
const int KW_COUNT = 17;
// Rollback Memory variables
tToken ** tokenMemory = NULL;
int memSize = 0;
int memAct = 0;

// Creates new token and returns his pointer
tToken * createToken(void){
    tToken * tok = gc_alloc_pointer(sizeof(struct Token));
    tok->data = NULL;
    tok->state = S_START;
    tok->line = 0;
    tok->size = 0;
    return tok;
}

// Init of rollback memory
int initMem(void){
    if(tokenMemory == NULL) tokenMemory = gc_alloc_pointer(sizeof(tToken*));
    if(tokenMemory == NULL) return 0;
    tokenMemory[0]=NULL;
    memSize=1;
    memAct=0;                           //index of current memory token
    return 1;
}

// Reset of rollback memory
void clearMem(void){
    int i;
    for(i=0;i<memSize;i++){
        tokenMemory[i]=NULL;
    }
    memAct=0;
}

// Init of token before first use or to reset him to defualt
int initToken(tToken * token){
    if(token != NULL){
        token->state = S_START;
//        if(token->data != NULL) free(token->data);
        token->data = gc_alloc_pointer(sizeof(char));
        if(token->data == NULL) return 0;
        token->line = 0;
        token->size = 1;
        token->data[0] = '\0';
        return 1;
    }
    else return 0;
}

// Removes addinitional zero characters from integer and double
int remZeros(tToken * token){
    int i=0;
    int sgn=0;
    int sgn1=0;
    int size = token->size;
    int j=size-2;
    for(i=0;i<size;i++){
        if(token->data[i]=='.'){
            sgn=1;
            break;
        }
        if(token->data[i]=='\0'){
            sgn=1;
            break;
        }
        if(token->data[i]!='0') break;
    }
    if(token->state == 6){                      // if S_DOUBLE
        for(j=size-2;j>0;j--){
            if(token->data[j]=='.'){
                sgn1=1;
                break;
            }
            if(token->data[j]!='0') break;
        }
    }
    if(sgn == 1) i--;
    if(sgn1 == 1) j++;
    char *buf = gc_alloc_pointer((j-i+2)*sizeof(char));
    if(buf == NULL) return 0;
    int k=0;
    int l=0;
    for(k=i;k<=j;k++){
        buf[l]=token->data[k];
        l++;
    }
    buf[l]='\0';
//    free(token->data);
    token->data = buf;
    token->size = j-i+2;
    return 1;
}
// Appends loaded character to token's data
int charAppend(tToken * token , char c){
    token->size++;
    token->data = gc_realloc_pointer(token->data,token->size*sizeof(char));
    if(token->data == NULL) return 0;
    token->data[token->size-2] = c;
    token->data[token->size-1] = '\0';
    return 1;
}

void shiftData(tToken * token, int from, int times){
    int i;
    for(i=from+times;i<token->size;i++){
        token->data[i-times]=token->data[i];
    }
    token->size=i-times;
}

// Checks correctnes of escape sequencies and replace them for one character
int checkAndReplaceESC(tToken* token){
    int i=0;
    int octa=0;
    char octaChar;
    char buf[3];
    while(token->data[i] != '\0'){
        // there is a escape sequence
        if(token->data[i] == '\\'){
            i++;
            if(token->data[i]=='n' || token->data[i]=='t' || token->data[i]=='"' || token->data[i]=='\\'){
                if(token->data[i]=='n'){
                    buf[0]='\n';
                }
                else if(token->data[i]=='t'){
                    buf[0]='\t';
                }
                else if(token->data[i]=='"'){
                    buf[0]='"';
                }
                else if(token->data[i]=='\\'){
                    buf[0]='\\';
                }
                token->data[i-1]=buf[0];        //replace char
                shiftData(token,i,1);
                continue;
            }
            else if(token->data[i]>= '0' && token->data[i]<='3'){               // octal escape sequence
                buf[0]=token->data[i];
                i++;
                if(token->data[i]>= '0' && token->data[i]<='7'){
                    buf[1]=token->data[i];
                    i++;
                    if(token->data[i]>= '0' && token->data[i]<='7'){
                        buf[2]=token->data[i];
                        octa = strtoul(buf,NULL,8);         // convert 
                        if(octa<1 || octa>255) return 0;    // 001 - 377 oct = 1 - 255 dec
                        octaChar=octa;
                        token->data[i-3]=octaChar;          // replace char
                        shiftData(token,i-2,3);
                    }
                    else return 0;
                }
                else return 0;
            }
            else return 0;
        }
        i++;
    }
    return 1;
}
//Function which controls whether idintifier isn't a key word
int checkKeyWord(tToken* token){
    int i=0;
    if(token->state == S_IDENTIF){
        for(i=0 ; i<KW_COUNT ; i++){
            if(strcmp(keyWords[i],token->data) == 0) return 1;
        }
    }
    if(token->state == S_FULL_IDENTIF){
        while(token->data[i]!='.'){
            i++;
        }
        char *part = gc_alloc_pointer((i+1)*sizeof(char));
        int j;
        for(j=0;j<i;j++){
            part[j]=token->data[j];
        }
        part[i]='\0';
        for(j=0 ; j<KW_COUNT ; j++){
            if(strcmp(keyWords[j],part) == 0){
                return 1;
            }
        }
        part = gc_alloc_pointer((token->size-i-1)*sizeof(char));
        for(j=i+1;j<token->size;j++){
            part[j-i-1] = token->data[j];
        }
        for(i=0 ; i<KW_COUNT ; i++){
            if(strcmp(keyWords[i],part) == 0){
                return 1;
            }
        }
    }
    return 0;
}

//Function saves current token to memory on last free place.
void addToMem(tToken * tok){
    tToken *tmp = createToken();        // make duplicate
    int i = 0;
    tmp->line = tok->line;
    tmp->size = tok->size;
    tmp->state = tok->state;
    tmp->data = gc_realloc_pointer(tmp->data, tok->size*sizeof(char));
    strcpy(tmp->data, tok->data);
    // integer or double
    if(tok->state == 5 || tok->state == 6){
        remZeros(tmp);
    }
    i = memAct;

    while(tokenMemory[i] != NULL && i<memSize) {
        i++;
    }

    if(i == memSize-1){         // memory full
        tokenMemory = gc_realloc_pointer(tokenMemory, (memSize+1)*sizeof(tToken*));
        tokenMemory[i+1] = NULL;
        memSize++;
        tokenMemory[i]=tmp;
    }
    else{                       // position available
        tokenMemory[i]=tmp;
    }
}

// Function returns token from memory and move actual pointer in memory
tToken * getPrevToken(){
    tToken *tmp=tokenMemory[memAct];
    if(memAct == memSize-1) {
        tokenMemory = gc_realloc_pointer(tokenMemory,(memSize+1)*sizeof(tToken*));
        memSize++;
        tokenMemory[memSize-1]=NULL;
    }
    memAct++;
    return tmp;
}
//Function for filling token with addinitional data, used before token is returned, calls addToMem if saving is enabled
void setToken(tToken* token,int line,tState state){
    token->line = line;
    token->state = state;
    if(saving!=0){
        addToMem(token);
    }
}

// set source file
void setSourceFile(FILE *f)
{
    source = f;
}

// Function returns Token filled with information about his state, size, data and line where he was loaded
// Inits tokenMemory and token if not initialized before its called
tToken * getToken(void){
    char c;
    if(tokenMemory == NULL) initMem();
    // rollback activated, if memory is not empty return token from memory
    if(saving == 0 && tokenMemory[memAct]!=NULL){
        token = getPrevToken();
        return token;
    }
    if(token == NULL) token = createToken();
    initToken(token);
    tState state = S_START;         // Starting state



    while(1){
        c = fgetc(source);	// character loaded
        switch (state){
            // Starting state
            case S_START: {
                // first level nods
                if(isalpha(c) || c == '$' || c=='_') state = S_IDENTIF;
                else if (isdigit(c)) state = S_INT;
                else if (c == '/') state = S_DIV;
                else if (c == '*') state = S_MULTIPLY;
                else if (c == '+') state = S_PLUS;
                else if (c == '-') state = S_MINUS;
                else if (c == '{') state = S_L_BRACKET;
                else if (c == '}') state = S_R_BRACKET;
                else if (c == ';') state = S_SEMICOLON;
                else if (c == ',') state = S_COMMA;
                else if (c == '(') state = S_LBRACKET;
                else if (c == ')') state = S_RBRACKET;
                else if (c == '=') state = S_ASSIGMENT;
                else if (c == '<') state = S_LESS;
                else if (c == '>') state = S_GREATER;
                else if (c == '!') state = S_EXCL;
                else if (c == EOF) state = S_EOF;
                else if (c == '"') state = S_STR;
                else if (isspace(c)){
                    if (c == '\n') line++;
                    break;
                }
                    // None of the 1st level nodes can be reached -> ERR
                else{
                    state = S_LEX_ERR;
                    setToken(token,line,state);
                    return(token);
                }
                if(state != S_STR){         // We want actual string without '"' inside data part
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                        setToken(token,line,state);
                        return(token);
                    }
                }

                break;
            }
                // 2nd level dots
                // these are final terminals
            case S_PLUS:
            case S_MULTIPLY:
            case S_MINUS:
            case S_L_BRACKET:
            case S_R_BRACKET:
            case S_SEMICOLON:
            case S_COMMA:
            case S_LBRACKET:
            case S_RBRACKET:
            case S_EOF:
            case S_LEX_ERR:
            case S_INTER_ERR: {
                ungetc(c, source);          // additional character loaded is returned to stream
                setToken(token,line,state);
                return (token);
            }
            case S_IDENTIF:{
                if(isalnum(c) || c == '_' || c == '$'){
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if(c == '.'){
                    state = S_FULL_IDENTIF;
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else{
                    ungetc(c,source);
                    setToken(token,line,state);
                    if(checkKeyWord(token) == 1) token->state = S_KEYWORD;      // Key word idintification
                    return (token);
                }
            }
            case S_INT:{
                if(isdigit(c)){
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if (c == '.'){
                    state = S_REAL;
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if (c == 'e' || c == 'E'){
                    state = S_EXP;
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else{
                    ungetc(c,source);
                    setToken(token,line,state);
                    remZeros(token);
                    return (token);
                }
            }
            case S_ASSIGMENT:{
                if(c == '='){
                    state = S_EQUAL;
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                        break;
                    }
                    setToken(token,line,state);
                    return (token);
                }
                else{
                    ungetc(c,source);
                    setToken(token,line,state);
                    return (token);
                }
            }
            case S_DIV:{
                if(c == '/'){
                    state = S_LINE_COM;
                    break;
                }
                else if(c == '*'){
                    state = S_BL_COM;
                    break;
                }
                else{
                    ungetc(c,source);
                    setToken(token,line,state);
                    return (token);
                }
            }
            case S_EXCL:{
                if (c == '='){
                    state = S_NOTEQUAL;
                    setToken(token,line,state);
                    return(token);
                }
                else{
                    state = S_LEX_ERR;
                    break;
                }
            }
            case S_LESS:{
                if(c == '=') state = S_LESSOREQ;
                else{
                    ungetc(c,source);
                }
                setToken(token,line,state);
                return(token);
            }
            case S_GREATER:{
                if(c == '=') state = S_GREATEROREQ;
                else{
                    ungetc(c,source);
                }
                setToken(token,line,state);
                return(token);
            }
            case S_STR:{
                if(c == EOF){
                    state = S_LEX_ERR;
                    ungetc(c,source);
                    break;
                }
                if(c == '"' && token->data[token->size-2]!='\\'){               // continues loading character if sequence \" was loaded                    if(!checkAndReplaceESC(token)) state = S_LEX_ERR;
                    if(!checkAndReplaceESC(token)) state = S_LEX_ERR;
                    setToken(token,line,state);
                    return (token);
                }
                else if(c > 31){                                                // smaller characters must be in escape sequence form
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else{
                state = S_LEX_ERR;
                }
                break;
            }
            case S_LINE_COM:{
                while(c != '\n'){
                    c = fgetc(source);
                    if(c == EOF){
                        state = S_EOF;
                        break;
                    }
                }
                line++;
                initToken(token);   // reset token
                state = S_START;    // begin from start
                break;
            }
            case S_BL_COM:{
                while(c != '*'){
                    if(c == '\n') line++;
                    if(c == EOF){
                        state = S_LEX_ERR;
                        ungetc(c,source);
                        break;
                    }
                    c = fgetc(source);
                }
                c = fgetc(source);
                if(c == EOF){                           // in case  /*   *'EOF'
                    state = S_LEX_ERR;
                    ungetc(c,source);
                    break;
                }
                if(c != '/') break;                     // still comment
                else {                                  // block comment ended ,resets token and begin from start
                    initToken(token);                   
                    state = S_START;
                    break;
                }
            }
                // 3rd level nods
            case S_FULL_IDENTIF:{
                if(isalnum(c) || c == '_' || c == '$'){
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if(token->data[token->size-2] !='.') {         
                    ungetc(c,source);
                    setToken(token,line,state);
                    if(checkKeyWord(token) == 1) token->state = S_LEX_ERR;
                    return (token);
                }
                else{
                    state=S_LEX_ERR;                    // nothing after dot
                    break;
                }
            }
            case S_REAL:{
                if(isdigit(c)){
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if((c == 'e' || c == 'E') && isdigit(token->data[token->size-2])) {        // previous character wasnt dot
                    state = S_EXP;
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if(isdigit(token->data[token->size-2])) {
                    ungetc(c,source);
                    state = S_DOUBLE;
                    setToken(token,line,state);
                    remZeros(token);
                    return (token);
                }
                else{                           // not a number after dot
                    state = S_LEX_ERR;
                    break;
                }
            }
            case S_EXP:{
                if (c == '+' || c == '-' || isdigit(c)) {
                    state = S_EXPO;
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else{
                    state = S_LEX_ERR;
                    break;
                }
            }
            case S_EXPO:{
                if(isdigit(c)){
                    if(!charAppend(token,c)){
                        state = S_INTER_ERR;
                    }
                    break;
                }
                else if (isdigit(token->data[token->size-2])) {
                    ungetc(c,source);
                    state = S_DOUBLE;
                    setToken(token,line,state);
                    return(token);
                }
                else{
                    state = S_LEX_ERR;
                    break;
                }
            }
            default:{
                break;
            }
        }
    }
}
