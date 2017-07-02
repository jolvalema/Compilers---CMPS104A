/* $Id: lexer.l,v 1.7 2016-10-27 19:45:59-07 - - $ */

%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

#define RETURN(SYMBOL) { return yylval_token(SYMBOL); }

%}

%option 8bit
%option debug
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
/*%option verbose*/

LETTER          [A-Za-z_]
DIGIT           [0-9]

IDENT           ({LETTER}({LETTER}|{DIGIT})*)
INTEGER         ({DIGIT}+)
CHAR            ('([^\\'\n]|\\[\\'"0nt])')
STRING          (\"([^\\"\n]|\\[\\'"0nt])*\")

BAD_IDENT       ({DIGIT}+{LETTER}({LETTER}|{DIGIT})*)
BAD_CHAR        ('([^\\'\n]|\\[\\'"0nt]?)[^']?)
BAD_STRING      (\"([^\\"\n]|\\[\\'"0nt])*[^\"]?)

%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }

"+"             { RETURN ('+');    }
"-"             { RETURN ('-');    }
"*"             { RETURN ('*');    }
"/"             { RETURN ('/');    }
"%"             { RETURN ('%');    }
"!"             { RETURN ('!');    }
"="             { RETURN ('=');    }
"=="            { RETURN (TOK_EQ); }
"!="            { RETURN (TOK_NE); }
"<"             { RETURN ('<');    }
"<="            { RETURN (TOK_LE); }
">"             { RETURN ('>');    }
">="            { RETURN (TOK_GE); }
","             { RETURN (',');    }
"."             { RETURN ('.');    }
";"             { RETURN (';');    }
"("             { RETURN ('(');    }
")"             { RETURN (')');    }
"["             { RETURN ('[');    }
"]"             { RETURN (']');    }
"{"             { RETURN ('{');    }
"}"             { RETURN ('}');    }
"[]"            { RETURN (TOK_ARRAY); }

"if"            { RETURN (TOK_IF);     }
"else"          { RETURN (TOK_ELSE);   }
"while"         { RETURN (TOK_WHILE);  }
"return"        { RETURN (TOK_RETURN); }
"int"           { RETURN (TOK_INT);    }
"string"        { RETURN (TOK_STRING); }
"struct"        { RETURN (TOK_STRUCT); }
"void"          { RETURN (TOK_VOID);   }
"new"           { RETURN (TOK_NEW);    }
"char"          { RETURN (TOK_CHAR);   }
"null"          { RETURN (TOK_NULL);   }

{IDENT}         { RETURN (TOK_IDENT);     }
{INTEGER}       { RETURN (TOK_INTCON);    }
{CHAR}          { RETURN (TOK_CHARCON);   }
{STRING}        { RETURN (TOK_STRINGCON); }

{BAD_IDENT}     { lexer::badtoken(yytext); }
{BAD_CHAR}      { lexer::badtoken(yytext); }
{BAD_STRING}    { lexer::badtoken(yytext); }
.               { lexer::badchar(*yytext); }

%%
