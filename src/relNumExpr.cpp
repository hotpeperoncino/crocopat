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

#include "relNumExpr.h"
#include "relExpression.h"
#include "relStrExpr.h"

#include <cfloat>

extern double string2double(string pStr);

///////////////////////////////////////////////////////////////////////////
relNumExprUnOp::~relNumExprUnOp()
{
  delete mExpr;
}

relNumber
relNumExprUnOp::interpret(bddSymTab* pSymTab)
{
  // First evaluate, to set variables in the variable ordering.
  bddRelation lRel = mExpr->interpret(pSymTab);
  const set<string> lFree = mExpr->collectFreeAttrs();
  relNumber result(0);

  // Cardinality.
  if (mOp == CARD) {
    result.setValue( lRel.getTupleNr(lFree) );
    return result;
  }

  // Min, Max, Sum.
  if(lFree.size() != 1) {
    cerr << "Error: MIN, MAX, SUM, or AVG requires expression with one free attribute (i.e. a set)." 
         << endl;
    exit(EXIT_FAILURE);
  }
  const unsigned lVarId = pSymTab->getAttributePos(*lFree.begin());

  if (lRel.isEmpty()) {
    if (mOp == SUM) {
      result.setValue(0);
      return result;
    }
    cerr << "Error: MIN, MAX, or AVG applied to empty set." << endl;
    exit(EXIT_FAILURE);
  }
  double lSum  = 0;
  double lCard = 0;
  double lMin  = DBL_MAX;
  double lMax  = -DBL_MAX;
  // For all elements of the set (all values for attribute lAttribute).
  while (!lRel.isEmpty()) {
    // Get next value of the attribute.
    string lValue = lRel.getElement(lVarId);
    double lNumValue = string2double(lValue);
    lSum += lNumValue;
    ++lCard;
    lMin = min(lMin, lNumValue);
    lMax = max(lMax, lNumValue);
    
    // Compute cofactor for current value in (lTmpRel).
    bddRelation lCurrentValue( 
                  bddRelation::mkEqual(pSymTab, 
                                       lVarId, 
                                       pSymTab->getValueNum(lValue)) );
    // Delete current element from set.
    lCurrentValue.complement();  
    lRel.intersect(lCurrentValue);
  }

  if (mOp == MIN)       result.setValue( lMin );
  else if (mOp == MAX)  result.setValue( lMax );
  else if (mOp == SUM)  result.setValue( lSum );
  else if (mOp == AVG)  result.setValue( lSum / lCard );
  else { 
    cerr << "Internal error: Unknown operator in relNumExprUnOp::interpret." 
         << endl;
    abort();
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////
relNumExprStr::~relNumExprStr()
{
  delete mExpr;
}

relNumber
relNumExprStr::interpret(bddSymTab* pSymTab)
{
  return relNumber( string2double(mExpr->interpret(pSymTab).getValue()) );
}
