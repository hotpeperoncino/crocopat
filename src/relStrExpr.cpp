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

#include "relStrExpr.h"
#include "relExpression.h"

///////////////////////////////////////////////////////////////////////////
relStrExprElem::~relStrExprElem()
{
  delete mExpr;
}

relString
relStrExprElem::interpret(bddSymTab* pSymTab)
{
  bddRelation lRel = mExpr->interpret(pSymTab);
  const set<string> lFree = mExpr->collectFreeAttrs();
  if(lFree.size() != 1) {
    cerr << "Error: ELEMENT requires expression with one free attribute (i.e. a set)." 
         << endl;
    exit(EXIT_FAILURE);
  }
  const unsigned lVarId = pSymTab->getAttributePos(*lFree.begin());
  if (lRel.isEmpty()) {
    cerr << "Error: ELEMENT applied to empty set." << endl;
    exit(EXIT_FAILURE);
  }
  return relString( lRel.getElement(lVarId) );
}
