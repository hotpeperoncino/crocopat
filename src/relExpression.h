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

#ifndef _relExpression_h
#define _relExpression_h

#include "bddRelation.h"
#include "relTerm.h"

#include <string>
#include <fstream>
#include <iterator>
#include <regex.h>

/// Global functions.
extern string unsigned2string(unsigned pUnsigned);
extern string double2string(double pNum);
extern double elapsed();

/// Global variables for interpreter.
extern map<string, relDataType*> gVariables;
extern const unsigned   gAttributeNum;    // Default:   1000 internal vars.
extern const char       gAttributePrefix; // Prefix for internal attributes.
extern bddSymTab*       gSymTab;
extern bool             gPrintWarnings;


//////////////////////////////////////////////////////////////////////////////
class relExpression : public relObject
{
public:
  virtual set<string>
  collectFreeAttrs() = 0;

  virtual bddRelation
  interpret(bddSymTab* pSymTab) = 0;
};

//////////////////////////////////////////////////////////////////////////////
class relExprRelVar : public relExpression
{
private:
  string*           mRelVar;
  vector<relTerm*>* mTermList;

public:
  relExprRelVar(string* pRelVar, vector<relTerm*>* pTermList)
    : mRelVar(pRelVar),
      mTermList(pTermList)
  {}

  ~relExprRelVar()
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
  }

  virtual set<string>
  collectFreeAttrs()
  {
    set<string> result;
    for( vector<relTerm*>::iterator 
         lIt = mTermList->begin();
         lIt != mTermList->end();
         ++lIt)
    {
      if (dynamic_cast<relTermAttribute*>(*lIt) != NULL) {
        result.insert((*lIt)->interpret(gSymTab));
      }
    }
    return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    // Add attributes to SymTab, if new.
    //   From top to bottom is important to get the right variable order.
    for(unsigned i = 0; i < mTermList->size(); ++i) {
      // Is it an attribute?
      if (dynamic_cast<relTermAttribute*>((*mTermList)[i]) != NULL)
      {
        pSymTab->addAttribute( (*mTermList)[i]->interpret(pSymTab) ); 
      }
    }
    
    // Warnings and return for unknown strings.
    for(unsigned i = 0; i < mTermList->size(); ++i) {
      if ( (dynamic_cast<relTermStrExpr*>((*mTermList)[i]) != NULL) &&
           (!pSymTab->isValueGood((*mTermList)[i]->interpret(pSymTab)))  ) {
        if (gPrintWarnings) {
          cerr << "Warning: String '" << (*mTermList)[i]->interpret(pSymTab)
               << "' is not in universe." << endl;
        }
        // Result for expression with non-existing value is always the empty set.
        return bddRelation(pSymTab, false);
      }
    }

    // Fetch result.
    string lRelVar = *mRelVar;                // Because we update.
    vector<relTerm*> lTermList = *mTermList;  // Because we update.
    map<string, relDataType*>::const_iterator lVarIt = gVariables.find(lRelVar);
    assert(lVarIt != gVariables.end());  // Must be declared.
    assert(lVarIt->second != NULL);
    if (dynamic_cast<bddRelationConst*>(lVarIt->second) != NULL) {
      // Predefined const relation.
      if (lRelVar == "="  ||  lRelVar == "!="  ||  
          lRelVar == "<"  ||  lRelVar == "<="   ||
          lRelVar == ">"  ||  lRelVar == ">=") {
        // These are binary relations.  If not, the parser has a problem.
        assert(lTermList.size() == 2);
        // Performance optimization: Constant always first.
        if (dynamic_cast<relTermStrExpr*>(lTermList[0]) == NULL &&
            dynamic_cast<relTermStrExpr*>(lTermList[1]) != NULL) {
          relTerm* lTermTmp = lTermList[0];
          lTermList[0] = lTermList[1];
          lTermList[1] = lTermTmp;
          if (lRelVar == "<")  { 
            lRelVar = ">"; 
          } else if (lRelVar == "<=") { 
            lRelVar = ">="; 
          } else if (lRelVar == ">")  { 
            lRelVar = "<"; 
          } else if (lRelVar == ">=") { 
            lRelVar = "<="; 
          }
          lVarIt = gVariables.find(lRelVar);
          assert(lVarIt != gVariables.end());  // Must be declared.
          assert(lVarIt->second != NULL);
        }
      }
    }
    bddRelation* lResult = dynamic_cast<bddRelation*>(lVarIt->second);
    assert(lResult != NULL);             // Must be a REL variable.
    // Make a copy of the relation.
    bddRelation result = *lResult;

    // Some type checking: Check arity.
    if (   lResult->mArity != -1 
        && lResult->mArity != mTermList->size() )
    {
      if (gPrintWarnings) {
        cerr << "Warning: Arity mismatch. '" 
             << *mRelVar << "' is of arity " << lResult->mArity 
             << " but is now used for arity " << mTermList->size() << "." << endl;
      }
    }

    // Quantify or intersect with value.
    //   Ordering: From bottom to top, for efficiency.
    for(int i = lTermList.size()-1; i >= 0; --i)
    {
      string lTerm = lTermList[i]->interpret(pSymTab);
      if (dynamic_cast<relTermExists*>(lTermList[i]) != NULL)
      {
        // Quantification.
        result.exists(gAttributePrefix + unsigned2string(i));
      }
      else if (dynamic_cast<relTermStrExpr*>(lTermList[i]) != NULL)
      {
        // lTerm is a string (constant) and exists in symtab (checked above).
        result.intersect(bddRelation::mkAttributeValue(
                                                       pSymTab, 
                                                       gAttributePrefix + unsigned2string(i), 
                                                       lTerm)
                         );
        result.exists(gAttributePrefix + unsigned2string(i)); 
      }
    }

    // Rename internal attributes to the given user attributes.
    //   Ordering: From bottom to top, for efficiency.
    for(int i = lTermList.size()-1; i >= 0; --i)
    {
      string lTerm = lTermList[i]->interpret(pSymTab);
      if (dynamic_cast<relTermAttribute*>(lTermList[i]) != NULL)
      {
        // lTerm is an attribute.
        // Rename.
        result.rename(gAttributePrefix + unsigned2string(i), lTerm);
      }
    }
    
    // Check if the arity of the stored relation was greater than
    //   the number of terms by looking for BDD nodes of internal attributes.
    if ( result.testVars(gAttributePrefix + unsigned2string(0),
                         gAttributePrefix + unsigned2string(gAttributeNum-1)) ) {
      cerr << "Error: The arity of relation '" << *mRelVar 
           << "' is greater than the number of terms." << endl;
      exit(EXIT_FAILURE);
    }
    
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprRelNumCmp : public relExpression
{
private:
  string*           mRelSym;
  relNumExpr*       mNumExpr1;
  relNumExpr*       mNumExpr2;

public:
  relExprRelNumCmp(string* pRelSym, relNumExpr* pNumExpr1, relNumExpr* pNumExpr2)
    : mRelSym(pRelSym),
      mNumExpr1(pNumExpr1),
      mNumExpr2(pNumExpr2)
  {}

  ~relExprRelNumCmp()
  {
    delete mRelSym;
    delete mNumExpr1;
    delete mNumExpr2;
  }

  virtual set<string>
  collectFreeAttrs()
  {
    set<string> result;
    return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    double lNum1 = mNumExpr1->interpret(pSymTab).getValue();
    double lNum2 = mNumExpr2->interpret(pSymTab).getValue();
    bool result = false;
    if (*mRelSym == "=")   result = (lNum1 == lNum2);
    else if (*mRelSym == "!=")  result = (lNum1 != lNum2);
    else if (*mRelSym == "<")   result = (lNum1 <  lNum2);
    else if (*mRelSym == "<=")  result = (lNum1 <= lNum2);
    else if (*mRelSym == ">")   result = (lNum1 >  lNum2);
    else if (*mRelSym == ">=")  result = (lNum1 >= lNum2);
    else { 
      cerr << "Internal error: Unknown operator in numerical comparison: '" 
           << *mRelSym << "'." << endl;
      abort(); 
    }
    return bddRelation(pSymTab, result);
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprExists : public relExpression
{
private:
  vector<relTerm*>* mTermList;
  relExpression*    mExpr;

public:
  relExprExists(vector<relTerm*>* pTermList, relExpression* pExpr)
    : mTermList(pTermList),
      mExpr(pExpr)
  {}

  ~relExprExists()
  {
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

  virtual set<string>
  collectFreeAttrs()
  {
    set<string> result = mExpr->collectFreeAttrs();
    for( vector<relTerm*>::iterator 
         lIt = mTermList->begin();
         lIt != mTermList->end();
         ++lIt)
    {
      if (dynamic_cast<relTermAttribute*>(*lIt) != NULL) {
        result.erase((*lIt)->interpret(gSymTab));
      }
    }
    return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    bddRelation result( mExpr->interpret(pSymTab) );
    const set<string> lFree = mExpr->collectFreeAttrs();
    for( vector<relTerm*>::iterator 
         lIt = mTermList->begin();
         lIt != mTermList->end();
         ++lIt)
    {
      const string lAttr = (*lIt)->interpret(pSymTab);
      if (dynamic_cast<relTermAttribute*>(*lIt) == NULL) {
        cerr << "Error: Only attributes allowed for quantification." << endl;
      } else if (lFree.find(lAttr) == lFree.end()) {
        cerr << "Error: Only free attributes allowed for quantification." 
             << endl 
             << "Attribute '" << lAttr 
             << "' does not occur free in the expression." << endl;
      } else {
        result.exists(lAttr);
      }
    }
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprNot : public relExpression
{
private:
  relExpression*        mExpr;

public:
  relExprNot( relExpression* pExpr)
    : mExpr(pExpr)
  {}

  ~relExprNot()
  {
        delete mExpr;
  }

  virtual set<string>
  collectFreeAttrs()
  {
        return mExpr->collectFreeAttrs();
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
        bddRelation result = mExpr->interpret(pSymTab);
        result.complement();
        return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprAnd : public relExpression
{
private:
  relExpression* mExpr1;
  relExpression* mExpr2;

public:
  relExprAnd(relExpression* pExpr1, relExpression* pExpr2)
    : mExpr1(pExpr1),
      mExpr2(pExpr2)
  {}

  ~relExprAnd()
  {
    delete mExpr1;
    delete mExpr2;
  }

  virtual set<string>
  collectFreeAttrs()
  {
        set<string> result = mExpr1->collectFreeAttrs();
        set<string> tmp    = mExpr2->collectFreeAttrs();
        result.insert(tmp.begin(), tmp.end());
        return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    bddRelation result( mExpr1->interpret(pSymTab) );
    result.intersect( mExpr2->interpret(pSymTab) );
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprOr : public relExpression
{
private:
  relExpression* mExpr1;
  relExpression* mExpr2;

public:
  relExprOr(relExpression* pExpr1, relExpression* pExpr2)
    : mExpr1(pExpr1),
      mExpr2(pExpr2)
  {}

  ~relExprOr()
  {
    delete mExpr1;
    delete mExpr2;
  }

  virtual set<string>
  collectFreeAttrs()
  {
        set<string> result = mExpr1->collectFreeAttrs();
        set<string> tmp    = mExpr2->collectFreeAttrs();
        result.insert(tmp.begin(), tmp.end());
        return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    bddRelation result( mExpr1->interpret(pSymTab) );
    result.unite( mExpr2->interpret(pSymTab) );
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprEquiv : public relExpression
{
private:
  relExpression* mExpr1;
  relExpression* mExpr2;

public:
  relExprEquiv(relExpression* pExpr1, relExpression* pExpr2)
    : mExpr1(pExpr1),
      mExpr2(pExpr2)
  {}

  ~relExprEquiv()
  {
    delete mExpr1;
    delete mExpr2;
  }

  virtual set<string>
  collectFreeAttrs()
  {
        set<string> result = mExpr1->collectFreeAttrs();
        set<string> tmp    = mExpr2->collectFreeAttrs();
        result.insert(tmp.begin(), tmp.end());
        return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    // (expr1 AND expr2) OR NOT(expr1 OR expr2).
    bddRelation lExpr1 = mExpr1->interpret(pSymTab);
    bddRelation lExpr2 = mExpr2->interpret(pSymTab);
    bddRelation lIntersect = lExpr1;
    lIntersect.intersect(lExpr2);
    bddRelation result = lExpr1;
    result.unite(lExpr2);
    result.complement();
    result.unite(lIntersect);
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprClosure : public relExpression
{
public:
  typedef enum {EXPTRAVERS, WARSHALLII} relTCAlg;

private:
  relExpression* mExpr;
  relTCAlg       mAlg;

public:
  relExprClosure(relExpression* pExpr, 
                 relTCAlg       pAlg)
    : mExpr(pExpr),
      mAlg(pAlg)
  {}

  ~relExprClosure()
  {
    delete mExpr;
  }

  virtual set<string>
  collectFreeAttrs()
  {
        return mExpr->collectFreeAttrs();
  }

  // Side effect: Adds a symbol to the symbol table.
  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    // First evaluate, to set variables in the variable ordering.
    bddRelation result( mExpr->interpret(pSymTab) );

    const set<string> lFree = mExpr->collectFreeAttrs();
    if(lFree.size() != 2) {
      cerr << "Error: Transitive closure requires two free attributes." << endl;
          exit(EXIT_FAILURE);
    }
    const map<unsigned,string> lVarOrd = pSymTab->computeVariableOrder(lFree);
    assert(lVarOrd.size() == 2);
    const string lAttributeX = lVarOrd.begin()->second;
    const string lAttributeY = (++lVarOrd.begin())->second;

    // Temporary attribute for computation of transitive closure.
    string tmpAttr = ".INTERNAL_TMPATTR.";
    // Add attribute to SymTab, if new.
    pSymTab->addAttribute(tmpAttr); 


    if (mAlg == EXPTRAVERS) {
    /*
    // Partitioning.
    //cerr << "0: " << elapsed() << endl;
    { // Part 1.
        bddRelation lDivider(pSymTab, false);
        for(reprNUMBER lValueIt = 0;
            lValueIt < pSymTab->getUniverseSize() * 2/3;
            ++lValueIt)
        {
          lDivider.unite(bddRelation::mkEqual(pSymTab, 
                                              pSymTab->getAttributePos(lAttributeX), 
                                              lValueIt));
        }
        bddRelation lFirstPart(result);
        lFirstPart.intersect(lDivider);
        
        // Variation 0 (for minimal time consumption)
        // Iteration to compute fixed point.
        bddRelation lPrevRel(pSymTab, false);
        while( ! lPrevRel.setEqual(lFirstPart) )
        {
          lPrevRel = lFirstPart;
          
          // Construct relation R(Y, TMPATTR) for one step.
          bddRelation lTmpRel(lFirstPart);                // R(X, Y)
          lTmpRel.rename(lAttributeY, tmpAttr);           // R(X, TMPATTR).
          lTmpRel.rename(lAttributeX, lAttributeY);       // R(Y, TMPATTR).
          
          // Step.
          lFirstPart.intersect(lTmpRel);                  // R(X, Y, TMPATTR).
          lFirstPart.exists(lAttributeY);                 // R(X, TMPATTR).
          lFirstPart.rename(tmpAttr, lAttributeY);        // R(X, Y).
          
          lFirstPart.unite( lPrevRel );
        }

        result.unite(lFirstPart);
    }
    //cerr << "1: " << elapsed() << endl;

    { // Part 2.
        bddRelation lDivider(pSymTab, false);
        for(reprNUMBER lValueIt = pSymTab->getUniverseSize() * 1/3;
            lValueIt < pSymTab->getUniverseSize();
            ++lValueIt)
        {
          lDivider.unite(bddRelation::mkEqual(pSymTab, 
                                              pSymTab->getAttributePos(lAttributeX), 
                                              lValueIt));
        }
        bddRelation lSecondPart(result);
        lSecondPart.intersect(lDivider);

        // Variation 0 (for minimal time consumption)
        // Iteration to compute fixed point.
        bddRelation lPrevRel(pSymTab, false);
        while( ! lPrevRel.setEqual(lSecondPart) )
        {
          lPrevRel = lSecondPart;
          
          // Construct relation R(Y, TMPATTR) for one step.
          bddRelation lTmpRel(lSecondPart);                // R(X, Y)
          lTmpRel.rename(lAttributeY, tmpAttr);       // R(X, TMPATTR).
          lTmpRel.rename(lAttributeX, lAttributeY);   // R(Y, TMPATTR).
          
          // Step.
          lSecondPart.intersect(lTmpRel);                  // R(X, Y, TMPATTR).
          lSecondPart.exists(lAttributeY);                 // R(X, TMPATTR).
          lSecondPart.rename(tmpAttr, lAttributeY);        // R(X, Y).
          
          lSecondPart.unite( lPrevRel );
        }

        result.unite(lSecondPart);
    }
    //cerr << "2: " << elapsed() << endl;
    */

    { // Main Part.
        // Variation 0 (for minimal time consumption)
        // Iteration to compute fixed point.
        bddRelation lPrevRel(pSymTab, false);
        while( ! lPrevRel.setEqual(result) )
        {
          lPrevRel = result;
          
          // Construct relation R(Y, TMPATTR) for one step.
          bddRelation lTmpRel(result);                // R(X, Y)
          lTmpRel.rename(lAttributeY, tmpAttr);       // R(X, TMPATTR).
          lTmpRel.rename(lAttributeX, lAttributeY);   // R(Y, TMPATTR).
          
          // Step.
          result.intersect(lTmpRel);                  // R(X, Y, TMPATTR).
          result.exists(lAttributeY);                 // R(X, TMPATTR).
          result.rename(tmpAttr, lAttributeY);        // R(X, Y).
          
          result.unite( lPrevRel );
        }
    }
    //cerr << "3: " << elapsed() << endl;
    }

/*
    // Variation 1
    // Iteration to compute fixed point.
    bddRelation lPrevRel(pSymTab);
    // Construct relation R(Y, TMPATTR) for one step.
    bddRelation lTmpRel(result);                // R(X, Y)
    lTmpRel.rename(lAttributeY, tmpAttr);       // R(X, TMPATTR).
    lTmpRel.rename(lAttributeX, lAttributeY);   // R(Y, TMPATTR).
    while( ! lPrevRel.setEqual(result) )
    {
      lPrevRel = result;

      // Step.
      result.intersect(lTmpRel);                 // R(X, Y, TMPATTR).
      result.exists(lAttributeY);                // R(X, TMPATTR).
      result.rename(tmpAttr, lAttributeY);       // R(X, Y).

      result.unite( lPrevRel );
    }
*/

/*
    // Variation 2
    // Iteration to compute fixed point.
    bddRelation lFront(result);
    // Construct relation R(Y, TMPATTR) for one step.
    bddRelation lTmpRel(result);                 // R(X, Y)
    lTmpRel.rename(lAttributeY, tmpAttr);        // R(X, TMPATTR).
    lTmpRel.rename(lAttributeX, lAttributeY);    // R(Y, TMPATTR).
    while( ! lFront.isEmpty() )
    {
      // Step.
      lFront.intersect(lTmpRel);                 // R(X, Y, TMPATTR).
      lFront.exists(lAttributeY);                // R(X, TMPATTR).
      lFront.rename(tmpAttr, lAttributeY);       // R(X, Y).

      bddRelation lResultComp(result);
      lResultComp.complement();
      lFront.intersect(lResultComp);
      result.unite( lFront );
    }
*/

/*
    // Variation 3: Floyd-Warshall
    for (reprNUMBER i = 0; i < pSymTab->getUniverseSize(); ++i)
    {
      bddRelation lStartNodes(result);
      lStartNodes.intersect(bddRelation::mkAttributeValue(pSymTab, lAttributeY, pSymTab->getAttributeValue(i)));
      lStartNodes.exists(lAttributeY);

      bddRelation lEndNodes(result);
      lEndNodes.intersect(bddRelation::mkAttributeValue(pSymTab, lAttributeX, pSymTab->getAttributeValue(i)));
      lEndNodes.exists(lAttributeX);
            
      lStartNodes.intersect(lEndNodes);

      result.unite(lStartNodes);
    }
*/


    if (mAlg == WARSHALLII) {
      // Variation 4: Floyd-Warshall II (for minimal memory consumption)
      // For efficiency,
      //   mAttributeX must be before mAttributeY in the variable order.
      //   Given by default.
    
      // Compute lInvResult, which is result with inverse variable order.
      bddRelation lInvResult(result);                // R(X, Y)
      lInvResult.rename(lAttributeY, tmpAttr);       // R(X, TMPATTR)
      lInvResult.rename(lAttributeX, lAttributeY);   // R(Y, TMPATTR)
      lInvResult.rename(tmpAttr,     lAttributeX);   // R(Y, X)
    

      bddRelation lValuesX(result);
      lValuesX.exists(lAttributeY);
      bddRelation lValuesY(result);
      lValuesY.exists(lAttributeX);
      lValuesY.rename(lAttributeY, lAttributeX);
      lValuesX.intersect(lValuesY);
      const unsigned lVarId = pSymTab->getAttributePos(lAttributeX);
      // For all elements of the set lValueX.
      while (!lValuesX.isEmpty()) {
            // Get next value of the attribute.
            string lValue = lValuesX.getElement(lVarId);
            // Compute cofactor for current value in (lTmpRel).
            bddRelation lCurrentValue( bddRelation::mkAttributeValue(pSymTab, lAttributeX, lValue) );
    

        bddRelation lStartNodesY(lInvResult);
        lStartNodesY.intersect(lCurrentValue);
        lStartNodesY.exists(lAttributeX);
        bddRelation lStartNodesX(lStartNodesY);
        lStartNodesX.rename(lAttributeY, lAttributeX);

        bddRelation lEndNodesY(result);
        lEndNodesY.intersect(lCurrentValue);
        lEndNodesY.exists(lAttributeX);
        bddRelation lEndNodesX(lEndNodesY);
        lEndNodesX.rename(lAttributeY, lAttributeX);
            
        lStartNodesX.intersect(lEndNodesY);
        lEndNodesX.intersect(lStartNodesY);

        result.unite(lStartNodesX);
        lInvResult.unite(lEndNodesX);


            // Delete current element from set.
            lCurrentValue.complement();
            lValuesX.intersect(lCurrentValue);
      }
    }

    pSymTab->removeAttribute(tmpAttr);
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////
class relExprRelOp : public relExpression
{
private:
  relExpression*        mExpr1;
  string*               mRelOp;
  relExpression*        mExpr2;

public:
  relExprRelOp( relExpression* pExpr1,
                string*        pRelOp, 
                relExpression* pExpr2)
    : mExpr1(pExpr1),
      mRelOp(pRelOp),
      mExpr2(pExpr2)
  {}

  ~relExprRelOp()
  {
    delete mExpr1;
    delete mRelOp;
    delete mExpr2;
  }

  virtual set<string>
  collectFreeAttrs()
  {
    set<string> result;
    return result;
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    bool result = false;
        bddRelation lExpr1 = mExpr1->interpret(pSymTab);
        bddRelation lExpr2 = mExpr2->interpret(pSymTab);

        if (*mRelOp == "=")  result =   lExpr1.setEqual(lExpr2);
        else if (*mRelOp == "!=") result = ! lExpr1.setEqual(lExpr2);
        else if (*mRelOp == ">")  result = lExpr1.setContains(lExpr2) && ! lExpr1.setEqual(lExpr2);
        else if (*mRelOp == ">=") result = lExpr1.setContains(lExpr2);
        else if (*mRelOp == "<")  result = lExpr2.setContains(lExpr1) && ! lExpr1.setEqual(lExpr2);
        else if (*mRelOp == "<=") result = lExpr2.setContains(lExpr1);
        else { 
          cerr << "Internal error: Unknown operator (relExprRelOp)." << endl;
          abort(); 
        }
        return bddRelation(pSymTab, result);
  }
};

//////////////////////////////////////////////////////////////////////////////
/// This expression matches regular expressions with terms.
///   If mTerm is a string, it returns TRUE or FALSE.
///   If mTerm is an attribute, it returns the set of matching strings.
class relExprRegExTerm : public relExpression
{
private:
  relStrExpr*    mRegExString;        // The string containing the regular expression.
  relTerm*       mTerm;               // The term to match.

public:
  relExprRegExTerm( relStrExpr*    pRegExString, 
                                    relTerm*       pTerm)
    : mRegExString(pRegExString),
      mTerm(pTerm)
  {}

  ~relExprRegExTerm()
  {
        delete mRegExString;
        delete mTerm;
  }

  virtual set<string>
  collectFreeAttrs()
  {
    set<string> result;
    if (dynamic_cast<relTermAttribute*>(mTerm) != NULL) {
      result.insert(mTerm->interpret(gSymTab));
    }
    return result;
  }
  
  /// Error handling for regex operation.
  void 
  errorproc(int pErrorCode, 
            regex_t* pRegExCompiled,
            const char* pRegEx,
            const char* pString)
  {
    size_t lLength = regerror(pErrorCode, pRegExCompiled, NULL, 0);
    char* lErrorMsg = (char*) malloc(lLength);
    if (lErrorMsg == NULL) {
          cerr << "Error: Virtual memory exhausted." << endl;
          exit(EXIT_FAILURE);
    }
    (void) regerror(pErrorCode, pRegExCompiled, lErrorMsg, lLength);
    cerr << "Error: Cannot apply regular expression '" << pRegEx
         << "' to string '" << pString << "': " << lErrorMsg << endl;
    delete lErrorMsg;
  }

  /// Service method for string matching.
  bool
  regexmatch(regex_t* pRegExCompiled,
             const char* pRegEx, 
             const char* pString)
  {
    int lErrorCode = regexec(pRegExCompiled, pString, 0, NULL, 0);
    if (lErrorCode == 0) {            // Matches.
      return true;
    }
    if (lErrorCode == REG_NOMATCH) {  // Does not match.
      return false;
    }
    // Error occured.
    errorproc(lErrorCode, pRegExCompiled, pRegEx, pString);
    regfree(pRegExCompiled);
    exit(EXIT_FAILURE);
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    const char* lTerm   = mTerm->interpret(pSymTab).c_str();
    const char* lRegExStr 
      = mRegExString->interpret(pSymTab).getValue().c_str();

    regex_t lRegExCompiled;        // The compiled regular expression.
    int lErrorCode
      = regcomp(&lRegExCompiled, lRegExStr, REG_EXTENDED | REG_NOSUB);
    if (lErrorCode != 0) {         // Compilation failed.
      errorproc(lErrorCode, &lRegExCompiled, lRegExStr, lTerm);
      regfree(&lRegExCompiled);
      exit(EXIT_FAILURE);
    }
    // Compilation successful.

    if (dynamic_cast<relTermStrExpr*>(mTerm) != NULL)
    {
      if (!pSymTab->isValueGood(string(lTerm))) {
        if (gPrintWarnings) {
          cerr << "Warning: String '" << lTerm
               << "' is not in universe." << endl;
        }
        // Result for expression with non-existing value is always the empty set.
        regfree(&lRegExCompiled);
        return bddRelation(pSymTab, false);
      }
      // String matching.
      bool result = regexmatch(&lRegExCompiled, 
                               lRegExStr, 
                               lTerm);
      regfree(&lRegExCompiled);
      return bddRelation(pSymTab, result);
    }

    if (dynamic_cast<relTermExists*>(mTerm) != NULL)
    {
      for (reprNUMBER i = 0; i < pSymTab->getUniverseSize(); ++i)
      {
        // String matching.
        bool result = regexmatch(&lRegExCompiled, 
                                 lRegExStr, 
                                 lTerm);
        if (result) {
          regfree(&lRegExCompiled);
          return bddRelation(pSymTab, true);
        }
      }
      regfree(&lRegExCompiled);
      return bddRelation(pSymTab, false);
    }

    // Nothing else left here, must be relTermAttribute.
    assert(dynamic_cast<relTermAttribute*>(mTerm) != NULL);

    // lTerm is an attribute.
    // Add internal attribute to SymTab, if new.
    pSymTab->addAttribute(mTerm->interpret(pSymTab)); 

    bddRelation result(pSymTab, false);
    const unsigned lVarId = pSymTab->getAttributePos(lTerm);
    for (reprNUMBER i = 0; i < pSymTab->getUniverseSize(); ++i)
    {
      const string lValue = pSymTab->getAttributeValue(i);
      // String matching.
      if (regexmatch(&lRegExCompiled,
                     lRegExStr, 
                     lValue.c_str())) {
        result.unite( bddRelation::mkEqual(pSymTab, lVarId, i) );
      }
    }
    regfree(&lRegExCompiled);
    return result;
  }
};


//////////////////////////////////////////////////////////////////////////////
/// This expression results in a relation that contains one tuple.
class relExprTupleOf : public relExpression
{
private:
  relExpression*        mExpr;

public:
  relExprTupleOf(relExpression* pExpr)
    : mExpr(pExpr)
  {}

  ~relExprTupleOf()
  {
    delete mExpr;
  }

  virtual set<string>
  collectFreeAttrs()
  {
        return mExpr->collectFreeAttrs();
  }

  virtual bddRelation
  interpret(bddSymTab* pSymTab)
  {
    // First evaluate, to set variables in the variable ordering.
    bddRelation lRel = mExpr->interpret(pSymTab);
    const set<string> lFree = mExpr->collectFreeAttrs();
    const map<unsigned,string> lVarOrd = pSymTab->computeVariableOrder(lFree);
    if( lRel.isEmpty() ) {
      cerr << "Error: TUPLEOF requires a non-empty relation as parameter." 
               << endl;
      exit(EXIT_FAILURE);
    }
    return lRel.getTupleOf(lVarOrd);
  }
};

#endif

