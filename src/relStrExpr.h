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

#ifndef _relStrExpr_h
#define _relStrExpr_h

#include "relString.h"
#include "relNumExpr.h"

#include <string>
#include <sstream>

/// Global variables for interpreter.
extern map<string, relDataType*> gVariables;
/// Cmd line arg handling for the interpreter.
extern char**  gArgv;
extern int     gArgc;

/// Global function.
extern string double2string(double pNum);

//////////////////////////////////////////////////////////////////////////////
class relStrExpr : public relObject
{
public:
  virtual relString
  interpret(bddSymTab* pSymTab) = 0;
};


//////////////////////////////////////////////////////////////////////////////
class relStrExprVar : public relStrExpr
{
private:
  string*         mVar;

public:
  relStrExprVar(string* pVar)
    : mVar(pVar)
  {}

  ~relStrExprVar()
  {
    delete mVar;
  }

  virtual relString
  interpret(bddSymTab* pSymTab)
  {
    // Fetch result.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(*mVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    relString* lResult = dynamic_cast<relString*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a STRING variable.

    return *lResult;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStrExprConst : public relStrExpr
{
private:
  string*       mVal;

public:
  relStrExprConst(string* pVal)
    : mVal(pVal)
  {}

  ~relStrExprConst()
  {
    delete mVal;
  }

  virtual relString
  interpret(bddSymTab* pSymTab)
  {
    return relString(*mVal);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStrExprBinOp : public relStrExpr
{
public:
  typedef enum {CONCAT} relStrOP;

private:
  relStrExpr*   mExpr1;
  relStrOP      mOp;
  relStrExpr*   mExpr2;

public:
  relStrExprBinOp( relStrExpr* pExpr1, 
                                   relStrOP pOp, 
                               relStrExpr* pExpr2)
    : mExpr1(pExpr1),
      mOp(pOp),
      mExpr2(pExpr2)
  {}

  ~relStrExprBinOp()
  {
    delete mExpr1;
    delete mExpr2;
  }

  virtual relString
  interpret(bddSymTab* pSymTab)
  {
    relString lExpr1 = mExpr1->interpret(pSymTab);
    relString lExpr2 = mExpr2->interpret(pSymTab);
    relString result("");

    if (mOp == CONCAT) result.setValue(lExpr1.getValue() + lExpr2.getValue());
    else { 
      cerr << "Internal error: Unknown operator in relStrExprBinOp::interpret." << endl;
      abort(); 
    }
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStrExprElem : public relStrExpr
{
private:
  relExpression* mExpr;

public:
  relStrExprElem(relExpression* pExpr) 
    : mExpr(pExpr)
  {}

  ~relStrExprElem();

  virtual relString
  interpret(bddSymTab* pSymTab);
};

//////////////////////////////////////////////////////////////////////////////
class relStrExprNum : public relStrExpr
{
private:
  relNumExpr*   mExpr;

public:
  relStrExprNum(relNumExpr* pExpr) 
    : mExpr(pExpr)
  {}

  ~relStrExprNum()
  {
    delete mExpr;
  }

  virtual relString
  interpret(bddSymTab* pSymTab)
  {
    return relString(double2string(mExpr->interpret(pSymTab).getValue()));
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStrCmdArg : public relStrExpr
{
private:
  relNumExpr*   mExpr;

public:
  relStrCmdArg(relNumExpr* pExpr) 
    : mExpr(pExpr)
  {}

  ~relStrCmdArg()
  {
    delete mExpr;
  }

  virtual relString
  interpret(bddSymTab* pSymTab)
  {
    int lPos = (int) mExpr->interpret(pSymTab).getValue();
    if (lPos >= 0  &&  lPos < gArgc) {
      return relString(string(gArgv[lPos]));
    } else {
      cerr << "Error: Missing command line argument '$" << lPos << "'."
           << endl;
      exit(EXIT_FAILURE);
    }
  }
};

#endif

