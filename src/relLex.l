%{

/*
 * CrocoPat is a tool for relational programming.
 * This file is part of CrocoPat. 
 *
 * Copyright (C) 2002-2008  Dirk Beyer
 *
 * CrocoPat is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * CrocoPat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CrocoPat; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Please find the GNU Lesser General Public License in file
 * License_LGPL.txt or at http://www.gnu.org/licenses/lgpl.txt
 *
 * Author:
 * Dirk Beyer (firstname.lastname@sfu.ca)
 * Simon Fraser University
 *
 * With contributions of: Andreas Noack, Michael Vogel
 */

/* Included code before lex code */
/*************** Includes and Defines *****************************/

#include "relStatement.h"

/* YACC generated definitions based on CTA parser input*/
#include "relYacc.tab.hpp"

extern double string2double(string pStr);

%}

space            [ ]
horizontal_tab   [\t]
newline          [\n]
vertical_tab     [\v]
formfeed         [\f]
carriage_return  [\r]

blank_character  {space}|{horizontal_tab}

digits           [0-9]+
exponentpart     [Ee][\+\-]?[0-9]+

%x comment

%%


{blank_character}+                                          {}

({formfeed}|{carriage_return}|{vertical_tab})*{newline} { ++yylineno; }

:=           { return t_ASSIGN; }
\_           { return '_'; }
AVG          { return t_AVG; }
BDT          { return t_BDT; }
DIV          { return t_DIV; }
ELSE         { return t_ELSE; }
ENDL         { return t_ENDL; }
\<\-\>       { return t_EQUIV; }
ELAPSED      { return t_ELAPSED; }
ELEMENT      { return t_ELEMENT; }
EX           { return t_EXISTS; }
EXEC         { return t_EXEC; }
EXIT         { return t_EXIT; }
FA           { return t_FORALL; }
FOR          { return t_FOR; }
GRAPH        { return t_GRAPH; }
IF           { return t_IF; }
\-\>         { return t_IMPLIES; }
IN           { return t_IN; }
MAX          { return t_MAX; }
MIN          { return t_MIN; }
MOD          { return t_MOD; }
NODESPERVAR  { return t_NODESPERVAR; }
NUMBER       { return t_NUMBER; }
PRINT        { return t_PRINT; }
PROCEDURE    { return t_PROCEDURE; }
RELINFO      { return t_RELINFO; }
ROUND        { return t_ROUND; }
STRING       { return t_STRING; }
STDERR       { return t_STDERR; }
SUM          { return t_SUM; }
TC           { return t_TC; }
TCFAST       { return t_TCFAST; }
TO           { return t_TO; }
TUPLEOF      { return t_TUPLEOF; }
WHILE        { return t_WHILE; }

\=           { yylval.rel_String = new string(yytext); return t_RELSYM; }
\!\=         { yylval.rel_String = new string(yytext); return t_RELSYM; }
\<           { yylval.rel_String = new string(yytext); return t_RELSYM; }
\<\=         { yylval.rel_String = new string(yytext); return t_RELSYM; }
\>           { yylval.rel_String = new string(yytext); return t_RELSYM; }
\>\=         { yylval.rel_String = new string(yytext); return t_RELSYM; }

[a-zA-Z_][a-zA-Z_0-9]* {  // Identifier.
                yylval.rel_String = new string(yytext);
                map<string, relDataType*>::iterator
                  it = gVariables.find(*yylval.rel_String);
                if( it != gVariables.end() )
                {
                  if (dynamic_cast<bddRelation*>     (it->second) != NULL  ||
                      dynamic_cast<bddRelationConst*>(it->second) != NULL)
                  {
                    return t_RELVAR;
                  }
                  if (dynamic_cast<relString*>(it->second) != NULL)
                  {
                    return t_STRVAR;
                  }
                  if (dynamic_cast<relNumber*>(it->second) != NULL)
                  {
                    return t_NUMVAR;
                  }
                }
                return t_IDENTIFIER ;
             }

\'.*\' {  // Identifier for RDF.
                yylval.rel_String = new string(yytext);
                map<string, relDataType*>::iterator
                  it = gVariables.find(*yylval.rel_String);
                if( it != gVariables.end() )
                {
                  if (dynamic_cast<bddRelation*>(it->second) != NULL)
                  {
                    return t_RELVAR;
                  }
                }
                return t_IDENTIFIER ;
             }

{digits}{exponentpart}?             {  // Number constant.
            yylval.rel_Number = string2double(string(yytext));
                        return(t_NUMBERCONSTANT);
             }
{digits}\.{digits}?{exponentpart}?  {  // Number constant.
            yylval.rel_Number = string2double(string(yytext));
                        return(t_NUMBERCONSTANT);
             }
\.{digits}{exponentpart}?           {  // Number constant.
            yylval.rel_Number = string2double(string(yytext));
                        return(t_NUMBERCONSTANT);
             }

\"[^"]*\"         {  // String constant.
            // Cut the quotes.
            string lValue(yytext);
            yylval.rel_String = new string(lValue, 1, lValue.length() - 2);
                        return(t_STRINGCONSTANT);
             }

(\/\/).*$         {
                        /* ignore comments to end of line. */
             }
^#.*$         {
                        /* ignore comments to end of line. */
             }

"/*"        BEGIN(comment);
            <comment>[^*\n]*        /* eat anything that's not a '*' */
            <comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
            <comment>\n             ++yylineno;
            <comment>"*"+"/"        BEGIN(INITIAL);

^\.               {
                        return EOF;
             }    

.                 {
                        return yytext[0];
             }

%%


# ifdef FLEX_STD
int yyFlexLexer::yywrap()    // if we need some other files
# else
int yywrap()                 // if we need some other files
# endif
{ 
  return 1;
}

