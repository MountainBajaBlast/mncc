#ifndef LEXER_H
#define LEXER_H



typedef enum {
     TK_EOF,
     TK_RETURN,
     TK_NUMBER,
     TK_SEMICOLON,
     TK_START,
     TK_END,
     TK_FUNCNAME,
     TK_INTEGER,
     TK_VARIABLE,
     TK_ADD,
     TK_SUBTRACTION,
     TK_ASSIGN,
     TK_MULTYPLY,
     TK_DIVISION,
     TK_LARPEN,
     TK_RARPEN,
     TK_CONST,
     TK_IF,
     TK_ELSE,
     TK_CMP
} TokenNames;


typedef struct {
     TokenNames type;
     char text[60];
} Token;


typedef struct {
     char *source;
     int pos;
} Lexer;

Token next_token(Lexer *lexer);

#endif

