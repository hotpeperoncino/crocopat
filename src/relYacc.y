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

/*************** Includes and Defines *****************************/

#include "FlexLexer.h"
#include "relStatement.h"
#include "relNumExpr.h"
#include "relStrExpr.h"
#include "relTerm.h"

// Global variables.
extern yyFlexLexer     gScanner;
extern int             gNoParseErrs;
extern relStatement*   gSyntaxTree;
extern set<string>     gValueUniverse;
extern bddSymTab*      gSymTab;
extern const unsigned  gAttributeNum;

// Global function.
extern string unsigned2string(unsigned pUnsigned);

//////////////////////////////////////////////////////////////////////////////
int yylex()
{
  return gScanner.yylex();
}

//////////////////////////////////////////////////////////////////////////////
void yyerror(const string pErrorMsg)
{
  cerr << "Error: Program parser error at line " << gScanner.lineno() << ": '"
       << gScanner.YYText() << "'." << endl
       << pErrorMsg << endl;
  ++gNoParseErrs;
}

//////////////////////////////////////////////////////////////////////////////

%}


// Operators with precedences.
// Relational Expressions.
%left t_RELSYM
%left t_EQUIV t_IMPLIES
%left '|'
%left '&'
%left '!'

// String/Numerical Expressions.
%left '+' '-'

// Numerical Expressions.
%left '*' '/' t_DIV t_MOD
%right '^'
%right UN_MINUS

// String Expressions.
%left '$'


%token t_ASSIGN
%token t_AVG
%token t_BDT
%token t_DIV
%token t_ELSE
%token t_ENDL
%token t_EQUIV
%token t_ELAPSED
%token t_ELEMENT
%token t_EXISTS
%token t_EXEC
%token t_EXIT
%token t_FORALL
%token t_FOR
%token t_GRAPH
%token t_IDENTIFIER
%token t_IF
%token t_IMPLIES
%token t_IN
%token t_MAX
%token t_MIN
%token t_MOD
%token t_NODESPERVAR
%token t_NUMBER
%token t_NUMBERCONSTANT
%token t_NUMVAR
%token t_PRINT
%token t_PROCEDURE
%token t_RELINFO
%token t_RELSYM
%token t_RELVAR
%token t_ROUND
%token t_STDERR
%token t_STRING
%token t_STRINGCONSTANT
%token t_STRVAR
%token t_SUM
%token t_TC
%token t_TCFAST
%token t_TO
%token t_TUPLEOF
%token t_WHILE

// define the types of rules
%union {
  relStatement*             rel_Stmt;
  relExpression*            rel_Expr;
  relExprRelVar*            rel_ExprRelVar;
  relTerm*                  rel_Term;
  vector<relTerm*>*         rel_TermList;
  relNumExpr*               rel_NumExpr;
  relStrExpr*               rel_StrExpr;
  relPrintExpr*             rel_PrintExpr;
  double                    rel_Number;
  string*                   rel_String;
}

%type <rel_Stmt>          Start StmtSeq Statement
%type <rel_Expr>          Expression
%type <rel_NumExpr>       NumExpr
%type <rel_StrExpr>       StrExpr
%type <rel_PrintExpr>     PrintExpr PrintExprList
%type <rel_Term>          Term TermLHS
%type <rel_TermList>      TermList TermListLHS
%type <rel_Number>        t_NUMBERCONSTANT
%type <rel_String>        t_IDENTIFIER StringVar t_STRINGCONSTANT 
                          t_RELSYM t_RELVAR t_STRVAR t_NUMVAR


// start symbol declaration
%start Start

/*************************************************************************/
%%

/* Terminals written in capitals and nonterminals with
   one capital at beginning. */

/* We have to use a left recursive grammar because we produce a
   bottom up parser. If we use a right recursive grammar we
   risk a stack overflow because of the large size of stack we need. */



// Start rule.
Start:
       StmtSeq
      {
        gSyntaxTree = $1;
      }
     ;

StmtSeq:
       Statement
      {
        $$ = $1;
      }
     | StmtSeq Statement
      {
        $$ = new relStmtSeq($1, $2);
      }
     ;

Statement:
        // Procedure definition.
       t_PROCEDURE t_IDENTIFIER '{' StmtSeq '}' ';'
      {
        if (gProcedures.find(*$2) != gProcedures.end()) {
          yyerror("Procedure '" + *$2 + "' already defined.");
        }
        // Procedures are not stored in the syntax tree, but in a global map.
        gProcedures[*$2] = $4;
        delete $2;
        $$ = new relStmtEmpty();
      }
     |  // Procedure call.
       t_IDENTIFIER ';'
      {
        $$ = new relStmtCall($1);
      }
     |  // Assignment for INT.
       t_NUMVAR t_ASSIGN NumExpr ';'
      {
        // Declaration already done.
        $$ = new relStmtAssignNum($1, $3);
      }
     | t_IDENTIFIER t_ASSIGN NumExpr ';'
      {
        assert(gVariables.find(*$1) == gVariables.end());
        // Declare variable.
        gVariables[*$1] = new relNumber(0);             
        $$ = new relStmtAssignNum($1, $3);
      }
      
        // Assignment for STRING.
     | t_STRVAR t_ASSIGN StrExpr ';'
      {
        // Declaration already done.
        $$ = new relStmtAssignStr($1, $3);
      }
     | t_IDENTIFIER t_ASSIGN StrExpr ';'
      {
        assert(gVariables.find(*$1) == gVariables.end());
        // Declare variable.
        gVariables[*$1] = new relString("");            
        $$ = new relStmtAssignStr($1, $3);
      }
            
      // Assignment for REL.
     | t_RELVAR '(' TermListLHS ')' t_ASSIGN Expression ';'
      {
        // Declaration already done.
        $$ = new relStmtAssign($1, $3, $6);
      }
     | t_RELVAR '(' TermListLHS ')' ';'
      {
        // Declaration already done.
        // Special case: nothing on the right hand side means TRUE,
        //   i.e. 'R(...);' is a short cut for 'R(...) := TRUE();'.
        vector<relTerm*>* lTermList = new vector<relTerm*>();
        $$ = new relStmtAssign($1, $3, new relExprRelVar(new string("TRUE"), lTermList));
      }
     | t_IDENTIFIER '(' TermListLHS ')' t_ASSIGN Expression ';'
      {
        // $1 is of token t_IDENTIFIER because it was not declared at time of scanning.
        // This might have changed after 'Expression' is scanned, i.e.,
        // 'assert(gVariables.find(*$1) == gVariables.end());' might fail (Issue 4, 2008-04-14).
        // We need to check again if *$1 is declared.
        if (gVariables.find(*$1) == gVariables.end()) {
          // Declare variable.
          gVariables[*$1] = new bddRelation(gSymTab, false);              
        }
        $$ = new relStmtAssign($1, $3, $6);
      }
     | t_IDENTIFIER '(' TermListLHS ')' ';'
      {
        assert(gVariables.find(*$1) == gVariables.end());
        // Declare variable.
        gVariables[*$1] = new bddRelation(gSymTab, false);              
        // Special case: nothing means TRUE on the right hand side,
        //   i.e. 'R(...);' is a short cut for 'R(...) := TRUE();'.
        vector<relTerm*>* lTermList = new vector<relTerm*>();
        $$ = new relStmtAssign($1, $3, new relExprRelVar(new string("TRUE"), lTermList));
      }

      // Conditionals.
     | t_IF Expression '{' StmtSeq '}'
      {
        $$ = new relStmtIf($2, $4, new relStmtEmpty());
      }
     | t_IF Expression '{' StmtSeq '}' t_ELSE '{' StmtSeq '}'
      {
        $$ = new relStmtIf($2, $4, $8);
      }
     | t_WHILE Expression '{' StmtSeq '}'
      {
        $$ = new relStmtWhile($2, $4);
      }
     | t_FOR StringVar t_IN Expression '{' StmtSeq '}'
      {
        $$ = new relStmtFor($2, $4, $6);
      }
      
      // Output.
      // Standard output.
     | t_PRINT PrintExprList ';'
      {
        $$ = new relStmtPrint($2, &cout, new relStrExprConst(new string("")));
      }
      // Standard error output.
     | t_PRINT PrintExprList t_TO t_STDERR ';'
      {
        $$ = new relStmtPrint($2, &cerr, new relStrExprConst(new string("")));
      }
      // Append output to file given by StrExpr.
     | t_PRINT PrintExprList t_TO StrExpr ';'
      {
        $$ = new relStmtPrint($2, NULL, $4);
      }
      
      // External program execution.
     | t_EXEC StrExpr ';'
      {
        $$ = new relStmtExec($2);
      }
      // Program termination.
     | t_EXIT NumExpr ';' 
      {
        $$ = new relStmtExit($2);
      }
     | '{' StmtSeq '}'
      {
        $$ = $2;
      }
     | ';'
      {
        $$ = new relStmtEmpty();
      }
     | error ';'
      {
        $$ = new relStmtEmpty();
      }
     ;

StringVar:   // String Variable for FOR stmt.
        t_IDENTIFIER
      {
        assert(gVariables.find(*$1) == gVariables.end());
        // Declare variable.
        gVariables[*$1] = new relString("");
        $$ = $1;
      }
     | t_STRVAR
      {
        // Declaration already done.
        $$ = $1;
      }
     ;

Expression:
       t_RELSYM '(' Term ',' Term ')'
      {
        vector<relTerm*>* lTermList = new vector<relTerm*>();
        lTermList->push_back($3);
        lTermList->push_back($5);
        $$ = new relExprRelVar($1, lTermList);
      }
     | t_RELSYM '(' NumExpr ',' NumExpr ')'
      {
            $$ = new relExprRelNumCmp($1, $3, $5);
      }
     | t_RELVAR '(' TermList ')'
      {
        $$ = new relExprRelVar($1, $3);
      }
     | t_IDENTIFIER '(' TermList ')'
      {
        if (gPrintWarnings) {
          cerr << "Warning: Undefined variable '" << *$1 << "'" << endl
               << "used in relational expression "
               << "at line " << gScanner.lineno() << "." << endl
               << *$1 << " has been initialized with FALSE(...)." << endl;
        }
        assert(gVariables.find(*$1) == gVariables.end());
        // Declare variable.
        gVariables[*$1] = new bddRelation(gSymTab, false);              
        $$ = new relExprRelVar($1, $3);
      }
     | Term t_RELSYM Term
      {
        vector<relTerm*>* lTermList = new vector<relTerm*>();
        lTermList->push_back($1);
        lTermList->push_back($3);
        $$ = new relExprRelVar($2, lTermList);
      }
     | NumExpr t_RELSYM NumExpr
      {
        $$ = new relExprRelNumCmp($2, $1, $3);
      }
     | Term t_RELVAR Term
      {
        vector<relTerm*>* lTermList = new vector<relTerm*>();
        lTermList->push_back($1);
        lTermList->push_back($3);
        $$ = new relExprRelVar($2, lTermList);
      }
     | Term t_IDENTIFIER Term
      {
        if (gPrintWarnings) {
          cerr << "Warning: Undefined variable '" << *$2 << "'" << endl
               << "used in relational expression "
               << "at line " << gScanner.lineno() << "." << endl
               << *$2 << " has been initialized with FALSE(...)." << endl;
        }
        assert(gVariables.find(*$2) == gVariables.end());
        // Declare variable.
        gVariables[*$2] = new bddRelation(gSymTab, false);              

        vector<relTerm*>* lTermList = new vector<relTerm*>();
        lTermList->push_back($1);
        lTermList->push_back($3);
        $$ = new relExprRelVar($2, lTermList);
      }
      
     | '!' Expression
      {
        $$ = new relExprNot($2);
      }
     | Expression '&' Expression
      {
        $$ = new relExprAnd($1, $3);
      }
     | Expression '|' Expression 
      {
        $$ = new relExprOr($1, $3);
      }
     | Expression t_IMPLIES Expression 
      {
        $$ = new relExprOr(new relExprNot($1), $3);
      }
     | Expression t_EQUIV Expression 
      {
        $$ = new relExprEquiv($1, $3);
      }
     | t_EXISTS '(' TermList ',' Expression ')'
      {
        $$ = new relExprExists($3, $5);
      }
     | t_FORALL '(' TermList ',' Expression ')'
      {
        $$ = new relExprNot( new relExprExists($3, new relExprNot($5)) );
      }
     | t_TC '(' Expression ')'
      {
        $$ = new relExprClosure($3, relExprClosure::WARSHALLII);
      }
     | t_TCFAST '(' Expression ')'
      {
        $$ = new relExprClosure($3, relExprClosure::EXPTRAVERS);
      }
          // Create a matching set of strings for a regular expression.
     | '@' StrExpr '(' Term ')'
      {
        $$ = new relExprRegExTerm($2, $4);
      }
     | '(' Expression ')'
      {
        $$ = $2;
      }

          // Expressions with 'Boolean' result.
     | Expression t_RELSYM Expression
      {
        $$ = new relExprRelOp($1, $2, $3);
      }

     | t_TUPLEOF '(' Expression ')'
      {
        $$ = new relExprTupleOf($3);
      }
    ;

TermList:
      /* empty */
     {
       $$ = new vector<relTerm*>();
     }
    | Term
     {
       $$ = new vector<relTerm*>();
       $$->push_back($1);
       if( $$->size() > gAttributeNum ) {  
         yyerror("Maximum arity (" + unsigned2string(gAttributeNum) + ") exceeded."); 
         exit(EXIT_FAILURE);  
       }
     }
    | TermList ',' Term
     {
       $$ = $1;
       $$->push_back($3);
       if( $$->size() > gAttributeNum ) {  
         yyerror("Maximum arity (" + unsigned2string(gAttributeNum) + ") exceeded."); 
         exit(EXIT_FAILURE);  
       }
     }
   ;

Term:
      t_IDENTIFIER
     {
       $$ = new relTermAttribute($1);
     }
    | StrExpr
     {
       $$ = new relTermStrExpr($1);
     }
    | '_'
     {
       $$ = new relTermExists();
     }
   ;

TermListLHS:
      /* empty */
     {
       $$ = new vector<relTerm*>();
     }
    | TermLHS
     {
       $$ = new vector<relTerm*>();
       $$->push_back($1);
       if( $$->size() > gAttributeNum ) {  
         yyerror("Maximum arity (" + unsigned2string(gAttributeNum) + ") exceeded."); 
         exit(EXIT_FAILURE);  
       }
     }
    | TermListLHS ',' TermLHS
     {
       $$ = $1;
       $$->push_back($3);
       if( $$->size() > gAttributeNum ) {  
         yyerror("Maximum arity (" + unsigned2string(gAttributeNum) + ") exceeded."); 
         exit(EXIT_FAILURE);  
       }
     }
   ;

TermLHS:
      t_IDENTIFIER
     {
       $$ = new relTermAttribute($1);
     }
    | t_STRINGCONSTANT
     {
       // Add value to value universe.
       gValueUniverse.insert(*$1);
       $$ = new relTermStrExpr(new relStrExprConst($1));
     }
   ;

StrExpr:
      t_STRVAR
     {
       $$ = new relStrExprVar($1);
     }
    | t_STRINGCONSTANT
     {
       $$ = new relStrExprConst($1);
     }
    | t_ELEMENT '(' Expression ')'
     {
       $$ = new relStrExprElem($3);
     }
    | t_STRING '(' NumExpr ')'
     {
       $$ = new relStrExprNum($3);
     }
    | '$' NumExpr
     {
       $$ = new relStrCmdArg($2);
     }
    | StrExpr '+' StrExpr
     {
       $$ = new relStrExprBinOp($1, relStrExprBinOp::CONCAT, $3);
     }
    | '(' StrExpr ')'
     {
       $$ = $2;
     }
    ;

NumExpr:
      t_NUMBERCONSTANT
     {
       $$ = new relNumExprConst($1);
     }
    | t_NUMVAR
     {
       $$ = new relNumExprVar($1);
     }
    | '#' '(' Expression ')'
     {
       $$ = new relNumExprUnOp($3, relNumExprUnOp::CARD);
     }
    | t_MIN '(' Expression ')'
     {
       $$ = new relNumExprUnOp($3, relNumExprUnOp::MIN);
     }
    | t_MAX '(' Expression ')'
     {
       $$ = new relNumExprUnOp($3, relNumExprUnOp::MAX);
     }
    | t_SUM '(' Expression ')'
     {
       $$ = new relNumExprUnOp($3, relNumExprUnOp::SUM);
     }
    | t_AVG '(' Expression ')'
     {
       $$ = new relNumExprUnOp($3, relNumExprUnOp::AVG);
     }
    | '-' NumExpr    %prec UN_MINUS
     {
       $$ = new relNumExprBinOp(new relNumExprConst(0), relNumExprBinOp::MINUS, $2);
     }
    | NumExpr '+' NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::PLUS, $3);
     }
    | NumExpr '-' NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::MINUS, $3);
     }
    | NumExpr '*' NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::MULT, $3);
     }
    | NumExpr '/' NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::DDIV, $3);
     }
    | NumExpr t_DIV NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::IDIV, $3);
     }
    | NumExpr t_MOD NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::MOD, $3);
     }
    | NumExpr '^' NumExpr
     {
       $$ = new relNumExprBinOp($1, relNumExprBinOp::POW, $3);
     }
    | t_ROUND '(' NumExpr ')'
     {
       $$ = new relNumExprRound($3);
     }
    | t_NUMBER '(' StrExpr ')'
     {
       $$ = new relNumExprStr($3);
     }
    | t_ELAPSED
     {
       $$ = new relNumExprTimeElapsed();
     }
    | '(' NumExpr ')'
     {
       $$ = $2;
     }
    ;

PrintExprList:
       PrintExpr
      {
        $$ = $1;
      }
     | PrintExprList ',' PrintExpr
      {
        // Sequence.
        $$ = new relPrintExprSeq($1, $3);
      }
    ;
    
PrintExpr:
       NumExpr
      {
        // Integer.
        $$ = new relPrintExprInt($1);
      }
     | StrExpr
      {
        // String.
        $$ = new relPrintExprString($1);
      }
     | Expression
      {
        // Relation.
        $$ = new relPrintExprValues(new relStrExprConst(new string("")), $1);
      }
     | '[' StrExpr ']' Expression
      {
        // Relation.
        // Use the given StrExpr as line prefix (tuple prefix, relation name).
        $$ = new relPrintExprValues($2, $4);
      }
     | t_ENDL
      {
        $$ = new relPrintExprEndLine();
      }
     | t_RELINFO '(' Expression ')'
      {
        $$ = new relPrintExprRelInfo($3);
      }
     | t_NODESPERVAR '(' Expression ')'
      {
        $$ = new relPrintExprNodesPerVarId($3);
      }
     | t_GRAPH '(' Expression ')'
      {
        $$ = new relPrintExprGraph($3);
      }
     | t_BDT '(' Expression ')'
      {
        $$ = new relPrintExprBDT($3);
      }
    ;

%%
