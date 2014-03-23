//
//  scanner.c
//  Lab3
//
//  Created by Bryce Holton.
//  Copyright (c) 2014 Bryce Holton. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "ctype.h"

/*******************
 Static functions needed for the scanner
 You need to design the proper parameter list and 
 return types for functions with ???.
 ******************/

static char get_char(char *ptr);
static void skip_comment(char *ptr);
static void skip_blanks(char *ptr);
static Token get_word(char *ptr);
static Token get_number(char *ptr);
static Token get_string(char *ptr);
static Token get_special(char *ptr);
static void downshift_word(char word[], int length);
static BOOLEAN is_reserved_word(char word[], int length, TokenCode *resCode);

typedef enum
{
    LETTER, DIGIT, QUOTE, SPECIAL, EOF_CODE,
}
CharCode;

/*********************
 Static Variables for Scanner
 Must be initialized in the init_scanner function.
 *********************/
static FILE *src_file;
static char src_name[MAX_FILE_NAME_LENGTH];
static char todays_date[DATE_STRING_LENGTH];
static CharCode char_table[256];  // The character table

typedef struct
{
    char *string;
    TokenCode token_code;
}
RwStruct;

const RwStruct rw_table[9][10] = {
    {{"do",DO},{"if",IF},{"in",IN},{"of",OF},{"or",OR},{"to",TO},{NULL,0}}, //Reserved words of size 2
    {{"and",AND},{"div",DIV},{"end",END},{"for",FOR},{"mod",MOD},{"nil",NIL},{"not",NOT},{"set",SET},{"var",VAR},{NULL,0}}, 
	//Reserved words of size 3
    {{"case",CASE},{"else",ELSE},{"file",FFILE},{"goto",GOTO},{"then",THEN},{"type",TYPE},{"with",WITH},{NULL,0}}, //Reserved words of size 4
    {{"array",ARRAY},{"begin",BEGIN},{"const",CONST},{"label",LABEL},{"until",UNTIL},{"while",WHILE},{NULL,0}},  //Reserved words of size 5
    {{"downto",DOWNTO}, {"packed",PACKED},{"record",RECORD}, {"repeat",REPEAT},{NULL,0}},  // Reserved words of size 6
    {{"program", PROGRAM},{NULL,0}}, // Reserved words of size 7
    {{"function", FUNCTION},{NULL,0}}, // Reserved words of size 8
    {{"procedure", PROCEDURE},{NULL,0}}  // Reserved words of size 9
};

void init_scanner(FILE *source_file, char source_name[], char date[])
{
    src_file = source_file;
    strcpy(src_name, source_name);
    strcpy(todays_date, date);
    /*******************
     initialize character table, this table is useful for identifying what type of character 
     we are looking at by setting our array up to be a copy the ascii table.  Since C thinks of 
     a char as like an int you can use ch in get_token as an index into the table.
     *******************/
    for (int i = 0; i <= 255; i++)
        {    
            if (isalpha((char) i))
                char_table[i] = LETTER;
            else if (isnumeric((char) i))
                char_table[i] = DIGIT;
            else if (i == 39 || i == 34)
                char_table[i] = QUOTE;
            else if (i == 46)
                char_table[i] = EOF_CODE;
            else
                char_table[i] = SPECIAL;
        }
    
}
BOOLEAN get_source_line(char source_buffer[])
{
    char print_buffer[MAX_SOURCE_LINE_LENGTH + 9];
    static int line_number = 0;
    
    if (fgets(source_buffer, MAX_SOURCE_LINE_LENGTH, src_file) != NULL)
    {
        ++line_number;
        sprintf(print_buffer, "%4d: %s", line_number, source_buffer);
        print_line(print_buffer, src_name, todays_date);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
Token* get_token()
{
    char ch; //This can be the current character you are examining during scanning.
    char token_string[MAX_TOKEN_STRING_LENGTH]; //Store your token here as you build it.
    char *token_ptr = &token_string[0]; //write some code to point this to the beginning of token_string
    Token toRtrn;
    int char_code;
    Token *tkn_PTR;
    tkn_PTR = (Token *)malloc(sizeof(*tkn_PTR)); // dynamically allocate memory for a new token
    
    skip_blanks(token_ptr);
        //1.  Skip past all of the blanks
    ch = get_char(token_ptr);
    char_code = char_table[(int) ch];
    if (char_code == QUOTE)
            toRtrn = get_string(token_ptr);
    else if (char_code == LETTER)
            toRtrn = get_word(token_ptr);
    else if (char_code == DIGIT)
            toRtrn = get_number(token_ptr); 
    else if (char_code == SPECIAL)
            toRtrn = get_special(token_ptr); 
    else if (char_code == EOF_CODE)
            {
                toRtrn.tCode = END_OF_FILE;
            }
    tkn_PTR = &toRtrn;
    return tkn_PTR;
}

static char get_char(char *ptr)
{
    /*
     If at the end of the current line (how do you check for that?),
     we should call get source line.  If at the EOF (end of file) we should
     set the character ch to EOF and leave the function.
     */
     char ch;
     static char current_line_buffer[MAX_SOURCE_LINE_LENGTH];

     if (ptr == &current_line_buffer[MAX_SOURCE_LINE_LENGTH] || current_line_buffer == NULL)
     {
        if (!get_source_line(current_line_buffer))
            {
                ch = (char) 46; // Assume failure to get next line means there isn't one
                return ch; 
            }
        ptr = &current_line_buffer[0]; // Set pointer to beginning of new line buffer
    }
    ch = *ptr;
    return ch;

}
static void skip_blanks(char *ptr)
{
    /*
     Write some code to skip past the blanks in the program and return a pointer
     to the first non blank character
     */
     while (get_char(ptr) == ' ')
        ptr++;
}

static void skip_comment(char *ptr)
{
    /*
     Write some code to skip past the comments in the program and return a pointer
     to the first non blank character.  Watch out for the EOF character.
     */
    char ch;
    do
        {
            ptr++;
            ch = get_char(ptr);
        } while (ch != '}' || ch != EOF_CODE);
}


static Token get_word(char *ptr)
{
    char token_string[MAX_TOKEN_STRING_LENGTH];
    char curChar = get_char(ptr);
    Token returnWord;
    TokenCode tCode = NO_TOKEN;
    int length;
    
    for (int i = 0; isalpha(curChar) || isnumeric((curChar)); i++) // Continue until a nonalphanumeric character is found
    {
        token_string[i]  = curChar;
        ptr++;
        curChar = get_char(ptr);
        length = i;
    }

    downshift_word(token_string, length);
    
    if (!is_reserved_word(token_string, length, &tCode))
      returnWord.tCode = IDENTIFIER;
    else
      returnWord.tCode = tCode;
    returnWord.stringValue = &token_string[0];
    return returnWord;
}



static Token get_number(char *ptr)
{
  char tokenString[MAX_TOKEN_STRING_LENGTH];
  Token numberToken;
  numberToken -> tCode = NUMBER;
  //input num holds current char puts in output num, rest is data checking
  char inputNum = getChar(*ptr);
  int outputNum = inputNum;
  float floatNum = (float) outputNum;

  BOOLEAN eStart;
  BOOLEAN eNeg;

  int i = 0;
  do{
    inputNum = get_char(ptr); //start here, input
    if(inputNum == '-'){
      tokenString[i] = '-'; //at whatever point put in string
      eNeg = TRUE;
    }
    else if(inputNum == 'e'){
      tokenString[i] = 'e'; //at whatever point
      eStart = TRUE;
    }
    else if(isnumeric(inputNum)){
      tokenString[i] = inputNum; //whatever point
    }
    else
      break;
    char* nextPtr = ptr++;
  }while(isnumeric(get_char(nextPtr)) || get_char(nextPtr) == 'e' || get_char(nextPtr) == '-');

  if (eStart = True){
    numberToken.stringValue = tokenString;
    return numberToken;
  }
  else if (eStart = True && eNeg = True){
    numberToken.stringValue = tokenString;
    return numberToken;
  }
  else{
    numberToken.intValue = inputNum;
    numberToken.lType = INTEGER_LIT;
    return numberToken;
  }
}
static Token get_string(char *ptr)
{
  char newString[DECENT_LINE_LENGTH];
  Token stringToken;
  int i = 0;
  //since first char passed is ' or ''
  ptr++;
  //skip to next char
  nextChar = get_char(ptr);
  while(nextChar != '\'' || nextChar != '\"'){
    newString[i] = nextChar;
    i++;
  }
  
  newString[i+1] = '\0';//creates char*
  stringToken.stringValue = newString[];
  stringToken.lType = STRING_LIT;
  stringToken.tCode = STRING;
  return stringToken;
}
static Token get_special(char *ptr)
{
    char ch = get_char(ptr);
    Token tokie;
    next_ch = get_char(ptr+1);
    switch(ch){
	case '^':
	    tokie.tCode = UPARROW;
	    break;
	case '*':
	    tokie.tCode = STAR;
	    break;
	case '(':
	    tokie.tCode = LPAREN;
	    break;
	case ')':
	    tokie.tCode = RPAREN;
	    break;
	case '-':
	    tokie.tCode = MINUS;
	    break;
	case '+':
	    tokie.tCode = PLUS;
	    break;
	case '=':
	    tokie.tCode = EQUAL;
	    break;
	case '[':
	    tokie.tCode = LBRACKET;
	    break;
	case ']':
	    tokie.tCode = RBRACKET;
	    break;
	case ':':
	    if(get_char(ptr++) == '='){
		tokie.tCode = COLONEQUAL;}
	    else{
		tokie.tCode = COLON;
		ptr--;}
	    break;
	case ';':
	    tokie.tCode = SEMICOLON;
	    break;
	case '<':
	    ch = get_char(ptr++);
	    if(ch  == '>'){
		tokie.tCode = NE;}
	    else if(ch == '='){
		tokie.tCode = LE;}
	    else{
		tokie.tCode = LT;
		ptr--;}
	    break;
	case '>':
	    if(get_char(ptr++) == '='){
		tokie.tCode = GE;}
	    else{
		tokie.tCode = GT;
		ptr--;}		
	    break;
	case ',':
	    tokie.tCode = COMMA;
	    break;
	case '.':
	    if(get_char(ptr++) == '.'){
		tokie.tCode = DOTDOT;}
	    else{
		tokie.tCode = PERIOD;
		ptr--;}
	    break;
	case '/':
	    tokie.tCode = SLASH;
	    break;
	}
	tokie.stringValue = SYMBOL_STRINGS[tokie.tCode];
	return tokie;
}

static void downshift_word(char word[], int length)
{
   
	for (int i = 0; i < length; i++)
		word[i] = tolower(word[i]);
}

static BOOLEAN is_reserved_word(char word[], int length, TokenCode *resCode)
{
  char *curRW;
  BOOLEAN disc;
  for (int i = 0; rw_table[length-2][i].token_code != 0; i++)
  {
    curRW = ( (char*) rw_table[length-2][i].string);
    disc = FALSE;
    for (int j = 0; j < length; j++)
      if ((* (curRW + j)) != word[j])
          disc = TRUE;
    if (!disc)
    {
      *resCode = rw_table[length-2][i].token_code;
      return TRUE;
    }
  }
  return FALSE;
    
}
