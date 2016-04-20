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

#include "relPrintExpr.h"

/// Global variable for interpreter.
class relStatement;
extern map<string, relStatement*> gProcedures;

//////////////////////////////////////////////////////////////////////////////
class relStatement : public relObject
{
public:
  virtual void
  interpret(bddSymTab* pSymTab) = 0;
};

//////////////////////////////////////////////////////////////////////////////
class relStmtSeq : public relStatement
{
private:
  relStatement* mStmt1;
  relStatement* mStmt2;

public:
  relStmtSeq(relStatement* pStmt1, relStatement* pStmt2)
    : mStmt1(pStmt1),
      mStmt2(pStmt2)
  {}

  ~relStmtSeq()
  {
    delete mStmt1;
    delete mStmt2;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    mStmt1->interpret(pSymTab);
    mStmt2->interpret(pSymTab);
  }
};

//////////////////////////////////////////////////////////////////////////////
/// Statement for procedure call.
class relStmtCall : public relStatement
{
private:
  string* mProcName;

public:
  relStmtCall(string* pProcName)
    : mProcName(pProcName)
  {}

  ~relStmtCall()
  {
    delete mProcName;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    if (gProcedures.find(*mProcName) == gProcedures.end()) {
      cerr << "Error: Procedure '" << *mProcName << "' undefined." << endl;
      exit(EXIT_FAILURE);
    }      
    gProcedures[*mProcName]->interpret(pSymTab);
  } // end interpret.
};

//////////////////////////////////////////////////////////////////////////////
class relStmtAssign : public relStatement
{
private:
  /// LHS.
  string*           mRelVar;
  // LHS.
  vector<relTerm*>* mTermList;
  // RHS.
  relExpression*    mExpr;

public:
  relStmtAssign(string* pRelVar, 
                vector<relTerm*>* pTermList, 
                relExpression* pExpr)
    : mRelVar(pRelVar),
      mTermList(pTermList),
      mExpr(pExpr)
  {}

  ~relStmtAssign()
  {
    delete mRelVar;
    for( vector<relTerm*>::iterator 
         lIt = mTermList->begin();
         lIt != mTermList->end();
         ++lIt)
    {
      delete *lIt;
    }
    delete mTermList;
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    { // Check if the free attributes in the expression match
      //   the attributes on the left hand side.
      set<string> lFreeAttrs   = mExpr->collectFreeAttrs();
      
      // For all attributes on the left hand side ...
      for( vector<relTerm*>::const_iterator lIt = mTermList->begin();
           lIt != mTermList->end();
           ++lIt)
      {
        if (dynamic_cast<relTermAttribute*>(*lIt) != NULL) {
          // (*lIt) is an attribute.
          string lAttr = (*lIt)->interpret(pSymTab);
          set<string>::iterator found = lFreeAttrs.find(lAttr);
          if ( found == lFreeAttrs.end()) {
            cerr << "Error: Attribute '" << lAttr 
                 << "' occurs on the left hand side of an assignment," << endl
                 << "but does not occur free in the expression on the right hand side." << endl;
            exit(EXIT_FAILURE);
          }
          lFreeAttrs.erase(found);
        }
      }
      if (lFreeAttrs.size() > 0) {
        cerr << "Error: The following attributes occur free in the expression" << endl
             << "on the right hand side of an assignment, but do not occur on the left side: ";
        copy(lFreeAttrs.begin(), 
             lFreeAttrs.end(), 
             ostream_iterator<string>(cerr, " "));
        cerr << endl;
        exit(EXIT_FAILURE);
      }
    }
    
    bddRelation lExprResult  = mExpr->interpret(pSymTab);
    
    // Relation for elimination of the (one) cofactor specified 
    //   by the constants.
    bddRelation lEliminateCofactor(pSymTab, true);

    // Rename user attributes to the internal attributes.
    //   Ordering: From top to bottom, for efficiency.
    for(unsigned i = 0; i < mTermList->size(); ++i)
    {
      const string lTerm = (*mTermList)[i]->interpret(pSymTab);
      if ( dynamic_cast<relTermAttribute*>( (*mTermList)[i] ) != NULL )
      {
        // lTerm is an attribute.
        // Add internal attribute to SymTab, if new.
        pSymTab->addAttribute(gAttributePrefix + unsigned2string(i)); 

        // Rename.
        lExprResult.rename(lTerm,
                           gAttributePrefix + unsigned2string(i));
      }
      else if (dynamic_cast<relTermStrExpr*>((*mTermList)[i]) != NULL)
      {
        // lTerm is a string constant.
        //   We have to eliminate the old cofactor for the given constant.
        // Remember the cofactor of constant 'lTerm' 
        //   for elimination from old relation.
        lEliminateCofactor.intersect(bddRelation::mkAttributeValue(
                                         pSymTab, 
                                         gAttributePrefix + unsigned2string(i), 
                                         lTerm)
                                     );

        // Restrict new relation to cofactor of constant.
        lExprResult.intersect(bddRelation::mkAttributeValue(
                                 pSymTab, 
                                 gAttributePrefix + unsigned2string(i), 
                                 lTerm)
                              );
      }
      else
      {
        // Everything else is invalid term expression.
        assert(false);
      }
    }
    lEliminateCofactor.complement();

    // Fetch old value.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(*mRelVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    if (dynamic_cast<bddRelationConst*>(lVarIt->second) != NULL) {
      // For constant relations TRUE and FALSE.
      cerr << "Error: Constant relation '" << *mRelVar << "'" << endl
           << "must not appear on the left hand side of an assignment." << endl;
      exit(EXIT_FAILURE);
    }
    bddRelation* lResult = dynamic_cast<bddRelation*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a REL variable.
    // Change value.
    lResult->intersect(lEliminateCofactor);
    lResult->unite(lExprResult);

    // Some type checking: Track arity.
    lResult->mArity = mTermList->size();

    pSymTab->removeUserAttributes(gAttributePrefix);
  } // end interpret.
};

//////////////////////////////////////////////////////////////////////////////
class relStmtAssignNum : public relStatement
{
private:
  string* mVar;
  relNumExpr* mExpr;

public:
  relStmtAssignNum(string* pVar, 
                   relNumExpr* pExpr)
    : mVar(pVar),
      mExpr(pExpr)
  {}

  ~relStmtAssignNum()
  {
    delete mVar;
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    relNumber lExprResult = mExpr->interpret(pSymTab);

    // Fetch old value.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(*mVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    relNumber* lResult = dynamic_cast<relNumber*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a INT variable.
    // Change value.
    *lResult = lExprResult;

    pSymTab->removeUserAttributes(gAttributePrefix);
  } // end interpret.
};

//////////////////////////////////////////////////////////////////////////////
class relStmtAssignStr : public relStatement
{
private:
  string* mVar;
  relStrExpr* mExpr;

public:
  relStmtAssignStr(string* pVar, 
                   relStrExpr* pExpr)
    : mVar(pVar),
      mExpr(pExpr)
  {}

  ~relStmtAssignStr()
  {
    delete mVar;
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    relString lExprResult = mExpr->interpret(pSymTab);

    // Fetch old value.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(*mVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    relString* lResult = dynamic_cast<relString*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a STRING variable.
    // Change value.
    *lResult = lExprResult;

    pSymTab->removeUserAttributes(gAttributePrefix);
  } // end interpret.
};

//////////////////////////////////////////////////////////////////////////////
class relStmtPrint : public relStatement
{
private:
  relPrintExpr* mPrintExpr;
  ostream*      mOut;
  relStrExpr*   mFilename;

public:
  relStmtPrint(relPrintExpr* pPrintExpr, ostream* pOut, relStrExpr* pFilename)
    : mPrintExpr(pPrintExpr),
      mOut(pOut),
      mFilename(pFilename)
  {}

  ~relStmtPrint()
  {
    delete mPrintExpr;
    delete mFilename;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    // Set the current output stream.
    if (mOut == NULL) {  // Use not stdout or stderr.
      const string lFileName = mFilename->interpret(pSymTab).getValue();
      if(lFileName == "") {
            cerr << "Error: Empty filename given." << endl;
            exit(EXIT_FAILURE);
      }
      // Open extra file with the given name, append then to that file.
      ofstream lStream;
      lStream.open(lFileName.c_str(), ofstream::out | ofstream::app);
      mPrintExpr->interpret(pSymTab, &lStream);
      lStream.close();
    } else {
      mPrintExpr->interpret(pSymTab, mOut);
    }
    pSymTab->removeUserAttributes(gAttributePrefix);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStmtExec : public relStatement
{
private:
relStrExpr* mExpr;

public:
  relStmtExec(relStrExpr* pExpr)
    : mExpr(pExpr)
  {}

  ~relStmtExec()
  {
    delete mExpr;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    int lExitCode = system(mExpr->interpret(pSymTab).getValue().c_str());
    if (lExitCode == -1) {
      perror("\nExternal program execution error");
      exit(128);
    }
    // Fetch old value.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find("exitStatus");
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    relNumber* lResult = dynamic_cast<relNumber*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a NUM variable.
    // Change value.
    lResult->setValue(lExitCode);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStmtIf : public relStatement
{
private:
  relExpression*   mExpr;
  relStatement*    mStmtThen;
  relStatement*    mStmtElse;
public:
  relStmtIf(relExpression* pExpr, 
                        relStatement* pStmtThen,
                        relStatement* pStmtElse)
    : mExpr(pExpr),
      mStmtThen(pStmtThen),
      mStmtElse(pStmtElse)
  {}

  ~relStmtIf()
  {
    delete mExpr;
    delete mStmtThen;
    delete mStmtElse;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    bddRelation lCondition = mExpr->interpret(pSymTab);
    pSymTab->removeUserAttributes(gAttributePrefix);
    if ( ! lCondition.setEqual(bddRelation(pSymTab, true)) &&
         ! lCondition.setEqual(bddRelation(pSymTab, false))   ) {
      if (gPrintWarnings) {
            cerr << "Warning: Boolean expression expected after IF. "
                 << "Quantify free attributes." << endl;        
      }
    }
    if ( lCondition.setEqual(bddRelation(pSymTab, true)) ) {
      mStmtThen->interpret(pSymTab);
    } else {
      mStmtElse->interpret(pSymTab);
    }   
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStmtWhile : public relStatement
{
private:
  relExpression*  mExpr;
  relStatement*   mStmt;
public:
  relStmtWhile(relExpression* pExpr, 
               relStatement* pStmt)
    : mExpr(pExpr),
      mStmt(pStmt)
  {}

  ~relStmtWhile()
  {
    delete mExpr;
    delete mStmt;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    bddRelation lCondition = mExpr->interpret(pSymTab);
    pSymTab->removeUserAttributes(gAttributePrefix);
    if ( ! lCondition.setEqual(bddRelation(pSymTab, true)) &&
         ! lCondition.setEqual(bddRelation(pSymTab, false))   ) {
      if (gPrintWarnings) {
        cerr << "Warning: Boolean expression expected after WHILE. "
             << "Quantify free attributes." << endl;            
      }
    }
    while( lCondition.setEqual(bddRelation(pSymTab, true)) )
    {
      mStmt->interpret(pSymTab);
      
      lCondition = mExpr->interpret(pSymTab);
      pSymTab->removeUserAttributes(gAttributePrefix);
    } 
  }
};

//////////////////////////////////////////////////////////////////////////////
class relStmtFor : public relStatement
{
private:
  string*        mStrVar;
  relExpression* mExpr;
  relStatement*  mStmt;

public:
  relStmtFor(string*        pStrVar,
             relExpression* pExpr, 
             relStatement*  pStmt)
    : mStrVar(pStrVar),
      mExpr(pExpr),
      mStmt(pStmt)
  {}

  ~relStmtFor()
  {
    delete mStrVar;
    delete mExpr;
    delete mStmt;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
    // First evaluate, to set variables in the variable ordering.
    bddRelation lRel = mExpr->interpret(pSymTab);
    const set<string> lFree = mExpr->collectFreeAttrs();

    if(lFree.size() != 1) {
      cerr << "Error: FOR statement requires one free attribute (i.e. a set)." 
           << endl;
          exit(EXIT_FAILURE);
    }
    const unsigned lVarId = pSymTab->getAttributePos(*lFree.begin());

    // Allow free usage of all attributes within the body.
    pSymTab->removeUserAttributes(gAttributePrefix);

    // Fetch string value.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(*mStrVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    relString* lResult = dynamic_cast<relString*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a STRING variable.

    // For all elements of the set (all values for attribute at pos lVarId).
    while (!lRel.isEmpty()) {
      // Get next value of the attribute.
      string lValue = lRel.getElement(lVarId);

      // Change value of string variable.
      lResult->setValue(lValue);

      // Delete current element from set.
      bddRelation lCurrentValue( bddRelation::mkEqual(pSymTab, 
                                 lVarId, 
                                 pSymTab->getValueNum(lValue)) );
      lCurrentValue.complement();  
      lRel.intersect(lCurrentValue);

      // Execute body of FOR loop.
      mStmt->interpret(pSymTab);
    }
  }  // method
};

//////////////////////////////////////////////////////////////////////////////
class relStmtExit : public relStatement
{
private:
  relNumExpr* mExitCode;

public:
  relStmtExit(relNumExpr* pExitCode)
        : mExitCode(pExitCode)
  {}

  ~relStmtExit()
  {
    delete mExitCode;
  }

  virtual void
  interpret(bddSymTab* pSymTab)
  {
        exit((int) mExitCode->interpret(pSymTab).getValue());
  }
};

//////////////////////////////////////////////////////////////////////////////
/// Empty statement.
class relStmtEmpty : public relStatement
{
public:
  relStmtEmpty()
  {}

  ~relStmtEmpty()
  {}

  virtual void
  interpret(bddSymTab* pSymTab)
  {}
};

