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

#ifndef _relNumExpr_h
#define _relNumExpr_h

#include "relNumber.h"
#include "bddSymTab.h"
class relExpression;
class relStrExpr;

#include <string>
#include <map>
#include <cmath>

/// Global variables for interpreter.
extern map<string, relDataType*> gVariables;

/// Global function.
extern double elapsed();

//////////////////////////////////////////////////////////////////////////////
class relNumExpr : public relObject
{
public:
  virtual relNumber
  interpret(bddSymTab* pSymTab) = 0;
};


//////////////////////////////////////////////////////////////////////////////
class relNumExprVar : public relNumExpr
{
private:
  string*         mVar;

public:
  relNumExprVar(string* pVar)
    : mVar(pVar)
  {}

  ~relNumExprVar()
  {
    delete mVar;
  }

  virtual relNumber
  interpret(bddSymTab* pSymTab)
  {
    // Fetch result.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(*mVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    relNumber* lResult = dynamic_cast<relNumber*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a INT variable.
 
    return *lResult;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relNumExprConst : public relNumExpr
{
private:
  double         mVal;

public:
  relNumExprConst(double pVal)
    : mVal(pVal)
  {}

  ~relNumExprConst()
  {}

  virtual relNumber
  interpret(bddSymTab* pSymTab)
  {
    return relNumber(mVal);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relNumExprUnOp : public relNumExpr
{
public:
  typedef enum {CARD, MIN, MAX, SUM, AVG} relNumOP;

private:
  relExpression* mExpr;
  relNumOP       mOp;

public:
  relNumExprUnOp(relExpression* pExpr, relNumOP pOp)
    : mExpr(pExpr),
      mOp(pOp)
  {}

  ~relNumExprUnOp();

  virtual relNumber
  interpret(bddSymTab* pSymTab);
};


//////////////////////////////////////////////////////////////////////////////
class relNumExprBinOp : public relNumExpr
{
public:
  typedef enum {PLUS, MINUS, MULT, DDIV, IDIV, MOD, POW} relNumOP;

private:
  relNumExpr*   mExpr1;
  relNumOP      mOp;
  relNumExpr*   mExpr2;

public:
  relNumExprBinOp( relNumExpr* pExpr1, 
                   relNumOP pOp, 
                   relNumExpr* pExpr2)
    : mExpr1(pExpr1),
      mOp(pOp),
      mExpr2(pExpr2)
  {}

  ~relNumExprBinOp()
  {
        delete mExpr1;
        delete mExpr2;
  }

  virtual relNumber
  interpret(bddSymTab* pSymTab)
  {
        double lExpr1 = mExpr1->interpret(pSymTab).getValue();
        double lExpr2 = mExpr2->interpret(pSymTab).getValue();
        double result;

        if (mOp == PLUS)   result = lExpr1 + lExpr2;
        else if (mOp == MINUS)  result = lExpr1 - lExpr2;
        else if (mOp == MULT)   result = lExpr1 * lExpr2;
        else if (mOp == DDIV)   result = lExpr1 / lExpr2;
        else if (mOp == IDIV)   result = (int)lExpr1 / (int)lExpr2;
        else if (mOp == MOD)    result = (int)lExpr1 % (int)lExpr2;
        else if (mOp == POW)    result = pow(lExpr1, lExpr2);
        else { 
          cerr << "Internal error: Unknown operator (relNumExprBinOp)." << endl;
          abort(); 
        }
        return relNumber(result);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relNumExprRound : public relNumExpr
{
private:
  relNumExpr*   mExpr;

public:
  relNumExprRound(relNumExpr* pExpr) 
    : mExpr(pExpr)
  {}

  ~relNumExprRound()
  {
    delete mExpr;
  }

  virtual relNumber
  interpret(bddSymTab* pSymTab)
  {
    double lExpr = mExpr->interpret(pSymTab).getValue();
    return relNumber(floor(lExpr + 0.5));
  }
};

//////////////////////////////////////////////////////////////////////////////
class relNumExprStr : public relNumExpr
{
private:
  relStrExpr*   mExpr;

public:
  relNumExprStr(relStrExpr* pExpr) 
    : mExpr(pExpr)
  {}

  ~relNumExprStr();

  virtual relNumber
  interpret(bddSymTab* pSymTab);
};

//////////////////////////////////////////////////////////////////////////////
class relNumExprTimeElapsed : public relNumExpr
{
private:

public:
  relNumExprTimeElapsed()
  {}

  ~relNumExprTimeElapsed()
  {}

  virtual relNumber
  interpret(bddSymTab* pSymTab)
  {
    return elapsed();
  }
};

#endif

