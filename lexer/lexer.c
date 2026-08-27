#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void lex_number(Lexer *lexer, Token *token)
{
	int i = 0;
	while (isdigit((unsigned char)lexer->source[lexer->pos])) {
		token->text[i++] = lexer->source[lexer->pos++];
		if (i >= (int)sizeof(token->text) - 1)
			break;
	}
	token->text[i] = '\0';
	token->type = TK_NUMBER;
}

static struct {
	char *word;
	TokenNames type;
} keywords[] = {{"int", TK_INTEGER},   {"return", TK_RETURN},
                {"const", TK_CONST},	{"if", TK_IF},	 
                {"else", TK_ELSE}};

static void lex_ident_or_keyword(Lexer *lexer, Token *token)
{
	int i = 0;

	while (isalnum((unsigned char)lexer->source[lexer->pos]) ||
	       lexer->source[lexer->pos] == '_') {
		if (i < (int)sizeof(token->text) - 1) {
			token->text[i++] = lexer->source[lexer->pos];
		}
		lexer->pos++;
	}
	token->text[i] = '\0';

        
	token->type = TK_VARIABLE;
	for (size_t k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++) {
		if (strcmp(token->text, keywords[k].word) == 0) {
			token->type = keywords[k].type;
			break;
		}
	}
        if (token->type == TK_VARIABLE &&lexer->source[lexer->pos] == '(') {
            token->type = TK_FUNCNAME;
        }

}

Token next_token(Lexer *lexer)
{
	Token token;

	while (lexer->source[lexer->pos] == ' ' ||
	       lexer->source[lexer->pos] == '\t' ||
	       lexer->source[lexer->pos] == '\n' ||
	       lexer->source[lexer->pos] == '\r') {
		lexer->pos++;
	}

	char current = lexer->source[lexer->pos];

	switch (current) {
	case '\0':
		token.type = TK_EOF;
		strcpy(token.text, "EOF");
		return token;
	case ';':
		token.type = TK_SEMICOLON;
		strcpy(token.text, ";");
		lexer->pos++;
		return token;
	case '+':
		token.type = TK_ADD;
		strcpy(token.text, "+");
		lexer->pos++;
		return token;
	case '-':
		token.type = TK_SUBTRACTION;
		strcpy(token.text, "-");
		lexer->pos++;
		return token;
	case '*':
		token.type = TK_MULTYPLY;
		strcpy(token.text, "*");
		lexer->pos++;
		return token;
	case '/':
		token.type = TK_DIVISION;
		strcpy(token.text, "/");
		lexer->pos++;
		return token;
	case '{':
		token.type = TK_START;
		strcpy(token.text, "{");
		lexer->pos++;
		return token;
	case '=':
		if (lexer->source[lexer->pos + 1] == '=') {
			token.type = TK_CMP;
			strcpy(token.text, "==");
			lexer->pos += 2;
		} else {
			token.type = TK_ASSIGN;
			strcpy(token.text, "=");
			lexer->pos++;
		}
		return token;
	case '}':
		token.type = TK_END;
		strcpy(token.text, "}");
		lexer->pos++;
		return token;
	case '(':
		token.type = TK_LARPEN;
		strcpy(token.text, "(");
		lexer->pos++;
		return token;
	case ')':
		token.type = TK_RARPEN;
		strcpy(token.text, ")");
		lexer->pos++;
		return token;
	default:
		break;
	}

	if (isdigit((unsigned char)current)) {
		lex_number(lexer, &token);
		return token;
	}

	if (isalpha((unsigned char)current) || current == '_') {
		lex_ident_or_keyword(lexer, &token);
		return token;
	}

	fprintf(stderr, "Unknown character: '%c'\n", current);
	token.type = TK_EOF;
	strcpy(token.text, "EOF");
	lexer->pos++;
	return token;
}
