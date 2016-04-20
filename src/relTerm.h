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

#ifndef _relTerm_h
#define _relTerm_h

#include "relStrExpr.h"

//////////////////////////////////////////////////////////////////////////////
class relTerm : public relObject
{
public:
  virtual string
  interpret(bddSymTab* pSymTab) = 0;
};


//////////////////////////////////////////////////////////////////////////////
class relTermAttribute : public relTerm
{
private:
  string*        mAttribute;

public:
  relTermAttribute(string* pAttribute)
    : mAttribute(pAttribute)
  {}

  ~relTermAttribute()
  {
    delete mAttribute;
  }

  virtual string
  interpret(bddSymTab* pSymTab)
  {
    if (pSymTab->getUniverseSize() == 0) {
      cerr << "Error: No values available (empty universe)." << endl;   
      exit(EXIT_FAILURE);
    }
    return *mAttribute;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relTermStrExpr : public relTerm
{
private:
  relStrExpr*       mExpr;

public:
  relTermStrExpr(relStrExpr* pExpr)
    : mExpr(pExpr)
  {}

  ~relTermStrExpr()
  {
    delete mExpr;
  }

  virtual string
  interpret(bddSymTab* pSymTab)
  {
    return mExpr->interpret(pSymTab).getValue();
  }
};

//////////////////////////////////////////////////////////////////////////////
class relTermExists : public relTerm
{
private:

public:
  relTermExists()
  {}

  ~relTermExists()
  {}

  virtual string
  interpret(bddSymTab* pSymTab)
  {
    if (pSymTab->getUniverseSize() == 0) {
      cerr << "Error: No values available (empty universe)." << endl;   
      exit(EXIT_FAILURE);
    }
    return "";
  }
};

#endif

