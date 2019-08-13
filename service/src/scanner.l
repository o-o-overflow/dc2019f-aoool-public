%{
	#include <iostream>
	#include <cstdlib>
	#include "scanner.h"
	#include "parserdriver.h"
	#include "utils.h"
	#include "parser.hpp"

	using namespace std;

	// Original yyterminate() macro returns int. Since we're using Bison 3 variants
	// as tokens, we must redefine it to change type from `int` to `Parser::semantic_type`
	#define yyterminate() aoool::Parser::make_END(aoool::location());

	// This will track current scanner location.
	// Action is called when length of the token is known.
	/* #define YY_USER_ACTION m_driver.increaseLocation(yyleng); */
	
	// !!!WARNING!!!
	// Location API is used, but the location is not initialized, 'cause I'm lazy. When making
	// a token with make_{something} method you can pass detailed token location. Current location
	// is accessible with m_driver.location() method. All puzzle elements are there - just
	// pass location value in every action code block below. I'm going to waste more time writing
	// this excuse than putting this boilerplate below...
	//
	// Location class can be found in configlocation.hh and posistion.hh files. It's just a bit too much
	// boilerplate for this small example. Bummer.
%}

%option nodefault
%option noyywrap
%option c++
%option yyclass="Scanner"
%option prefix="aoool_"

%%

main      { 
                DDLOG("Scanner: 'main' token\n");
                return aoool::Parser::make_MAIN(aoool::location()); 
            }

root      { 
                DDLOG("Scanner: 'root' token\n");
                return aoool::Parser::make_ROOT(aoool::location()); 
            }

log { 
                DDLOG("Scanner: 'log' token\n");
                return aoool::Parser::make_LOG(aoool::location()); 
            }

mode { 
                DDLOG("Scanner: 'mode' token\n");
                return aoool::Parser::make_MODE(aoool::location()); 
            }

text { 
                DDLOG("Scanner: 'text' token\n");
                return aoool::Parser::make_TEXT(aoool::location()); 
            }

osl { 
                DDLOG("Scanner: 'osl' token\n");
                return aoool::Parser::make_OSL(aoool::location()); 
            }

server_name      { 
                DDLOG("Scanner: 'server_name' token\n");
                return aoool::Parser::make_SERVERNAME(aoool::location()); 
            }

server      { 
                DDLOG("Scanner: 'server' token\n");
                return aoool::Parser::make_SERVER(aoool::location()); 
            }

location { 
                DDLOG("Scanner: 'location' token\n");
                return aoool::Parser::make_LOCATION(aoool::location()); 
            }

print { 
                DDLOG("Scanner: 'print' token\n");
                return aoool::Parser::make_PRINT(aoool::location()); 
            }

del { 
                DDLOG("Scanner: 'del' token\n");
                return aoool::Parser::make_DEL(aoool::location()); 
            }

\"[a-zA-Z0-9/\.\*\\]*\" { 
                DDLOG("Scanner: 'string value' token\n");
                // handle escapes like \x41 => A, in place
                char* s = (char*) yytext;
                char hex[3];
                size_t len = strlen(yytext);
                size_t i = 0;
                size_t new_idx = 0;
                for (i=0; i < len; ) {
                    if ( ((i+3) < len) && (s[i] == '\\') && (s[i+1] == 'x') && (
                                ((s[i+2] >= '0') && (s[i+2] <= '9')) || 
                                ((s[i+2] >= 'a') && (s[i+2] <= 'f'))) && (
                                ((s[i+3] >= '0') && (s[i+3] <= '9')) || 
                                ((s[i+3] >= 'a') && (s[i+3] <= 'f')))) {
                        // unescape
                        hex[0] = s[i+2];
                        hex[1] = s[i+3];
                        hex[2] = '\0';
                        char c = strtol(hex, NULL, 16);
                        s[new_idx] = c;
                        new_idx++;
                        i += 4;
                    } else {
                        if (new_idx != i) {
                            s[new_idx] = s[i];
                        }
                        new_idx++;
                        i++;
                    }
                }
                s[new_idx] = '\0';
                return aoool::Parser::make_STRINGVALUE(s, aoool::location()); 
            }

[a-zA-Z][a-zA-Z0-9]*     { 
                DDLOG("Scanner: string [%s]\n", yytext);
                return aoool::Parser::make_STRING(yytext, aoool::location()); 
            }
            
\(          {
                DDLOG("Scanner: '('\n");
                return aoool::Parser::make_LEFTPAR(aoool::location());
            }
            
\)          { 
                DDLOG("Scanner: ')'\n");
                return aoool::Parser::make_RIGHTPAR(aoool::location());
            }

\{          {
                DDLOG("Scanner: '{'\n");
                return aoool::Parser::make_LEFTCPAR(aoool::location());
            }
            
\}          { 
                DDLOG("Scanner: '}'\n");
                return aoool::Parser::make_RIGHTCPAR(aoool::location());
            }
            
=          { 
                DDLOG("Scanner: '='\n");
                return aoool::Parser::make_EQUAL(aoool::location());
            }
            
\+          { 
                DDLOG("Scanner: '+'\n");
                return aoool::Parser::make_PLUS(aoool::location());
            }
            
-          { 
                DDLOG("Scanner: '-'\n");
                return aoool::Parser::make_MINUS(aoool::location());
            }
            
\*          { 
                DDLOG("Scanner: '*'\n");
                return aoool::Parser::make_ASTERISK(aoool::location());
            }
            
;           {
                DDLOG("Scanner: ';'\n");
                return aoool::Parser::make_SEMICOLON(aoool::location());
            }
            
,           {
                DDLOG("Scanner: ','\n");
                return aoool::Parser::make_COMMA(aoool::location());
            }
            
[\n\t ]     {
                /* DDLOG("Scanner: whitechar (ignored)\n"); */
            }

[0-9]+ {
                DDLOG("Scanner: decimal number: %s\n", yytext);
                uint64_t number = strtoull(yytext, 0, 10);
                return aoool::Parser::make_NUMBER(number, aoool::location());
            }

.           { 
                DDLOG("Scanner: unknown character [%s]\n", yytext);
            }
            
<<EOF>>     { return yyterminate(); }


%%
