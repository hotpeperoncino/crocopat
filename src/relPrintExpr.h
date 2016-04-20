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

#include "relExpression.h"

//////////////////////////////////////////////////////////////////////////////
class relPrintExpr : public relObject
{
public:
  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut) = 0;
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprSeq : public relPrintExpr
{
private:
  relPrintExpr* mPrintExpr1;
  relPrintExpr* mPrintExpr2;

public:
  relPrintExprSeq(relPrintExpr* pPrintExpr1, relPrintExpr* pPrintExpr2)
    : mPrintExpr1(pPrintExpr1),
      mPrintExpr2(pPrintExpr2)
  {}

  ~relPrintExprSeq()
  {
    delete mPrintExpr1;
    delete mPrintExpr2;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    mPrintExpr1->interpret(pSymTab, pOut);
    mPrintExpr2->interpret(pSymTab, pOut);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprEndLine : public relPrintExpr
{
private:

public:
  relPrintExprEndLine()
  {}

  ~relPrintExprEndLine()
  {}

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    *pOut << endl;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprString : public relPrintExpr
{
private:
  relStrExpr* mExpr;

public:
  relPrintExprString(relStrExpr* pExpr)
        : mExpr(pExpr)
  {}

  ~relPrintExprString()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    *pOut << mExpr->interpret(pSymTab).getValue();
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprInt : public relPrintExpr
{
private:
  relNumExpr* mExpr;

public:
  relPrintExprInt(relNumExpr* pExpr)
        : mExpr(pExpr)
  {}

  ~relPrintExprInt()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    *pOut << mExpr->interpret(pSymTab).getValue();
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprValues : public relPrintExpr
{
private:
  /// Row prefix for RSF.
  relStrExpr*     mPrefix;
  relExpression*  mExpr;

public:
  relPrintExprValues(relStrExpr* pPrefix, 
                     relExpression* pExpr)
    : mPrefix(pPrefix),
      mExpr(pExpr)
  {}

  ~relPrintExprValues()
  {
    delete mPrefix;
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    string lPrefix = mPrefix->interpret(pSymTab).getValue();
    if (lPrefix.length() != 0) {
      lPrefix += '\t';
    }
    const set<string> lFree = mExpr->collectFreeAttrs();
    bddRelation lRel = mExpr->interpret(pSymTab);
    map<unsigned,string> lVarOrd = pSymTab->computeVariableOrder(lFree);
    vector<string> lAttributeList;
    for(map<unsigned, string>::const_iterator lIt = lVarOrd.begin();
        lIt != lVarOrd.end();
        ++lIt)
    {
      lAttributeList.push_back(lIt->second);
    }
    lRel.printRelation(*pOut, lPrefix, lAttributeList );
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprRelInfo : public relPrintExpr
{
private:
  relExpression* mExpr;

public:
  relPrintExprRelInfo(relExpression* pExpr)
    : mExpr(pExpr)
  {}

  ~relPrintExprRelInfo()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    bddRelation lRel = mExpr->interpret(pSymTab);

    *pOut << "------------------------------------------------------" << endl;
        *pOut << "Information about the representing data structures." << endl;
        *pOut << endl;

    const set<string> lFree = mExpr->collectFreeAttrs();
    *pOut << "Number of tuples in the relation: " 
          << lRel.getTupleNr(lFree) 
          << endl;

    *pOut << "Number of values (universe): " 
          << pSymTab->getUniverseSize() << endl;

    // BDD info.
    lRel.printBddInfo(*pOut);

    *pOut << "Attribute order: ";
    const map<unsigned,string> lVarOrd = pSymTab->computeVariableOrder(lFree);
    for(map<unsigned, string>::const_iterator lIt = lVarOrd.begin();
        lIt != lVarOrd.end();
        ++lIt)
    {
      *pOut << lIt->second << " ";
    }
    *pOut << endl;

    *pOut << "------------------------------------------------------" << endl;

    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprNodesPerVarId : public relPrintExpr
{
private:
  relExpression* mExpr;

public:
  relPrintExprNodesPerVarId(relExpression* pExpr)
    : mExpr(pExpr)
  {}

  ~relPrintExprNodesPerVarId()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    const bddRelation lRel = mExpr->interpret(pSymTab);
    const set<string> lFree = mExpr->collectFreeAttrs();
    lRel.printNodesPerVarId(*pOut, lFree);
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprGraph : public relPrintExpr
{
private:
  relExpression* mExpr;

public:
  relPrintExprGraph(relExpression* pExpr)
    : mExpr(pExpr)
  {}

  ~relPrintExprGraph()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    const bddRelation lRel = mExpr->interpret(pSymTab);
    const set<string> lFree = mExpr->collectFreeAttrs();
    lRel.printGraph(*pOut, lFree);
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relPrintExprBDT : public relPrintExpr
{
private:
  relExpression* mExpr;

public:
  relPrintExprBDT(relExpression* pExpr)
    : mExpr(pExpr)
  {}

  ~relPrintExprBDT()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab, ostream* pOut)
  {
    const bddRelation lRel = mExpr->interpret(pSymTab);
    lRel.printBDT(*pOut);
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};


