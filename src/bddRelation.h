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

#ifndef _bddRelation_h_
#define _bddRelation_h_

#include "bddBdd.h"
#include "bddSymTab.h"
#include "relDataType.h"

/// Global functions.
extern string unsigned2string(unsigned pUnsigned);

/// Set of tupels represented by a BDD.
///
/// It is possible that there exist bit vectors which are no valid encodings
///   of tuples (because the cardinalities of ranges 
///   of attributes do not need to be powers of 2).
///   For speed, these out-of-range bit vectors are not generally excluded from
///   representation. 
///   Value mSymTab->getUniverseSize()-1 is represented by all values 
///   mSymTab->getUniverseSize()-1 ... 2**mSymTab->getBitNr()-1.
///   This invariant keeps the comparison operations 
///   (setEqual, setContains, isEmpty) correct.
///   However, the method for tuple count (getTupleNr) has to
///   restrict to the set of all range tuples (mkRange).
class bddRelation : public relDataType
{

private: // Private static methods.

  /// Returns all tuples satisfying pVI == pConst.
  static bddRelation
  mkEqualPure(const bddSymTab* pSymTab,
              unsigned pVarId, 
              reprNUMBER pConst)
  {
    return bddRelation(pSymTab,  
                       bddBdd(pVarId, pSymTab->getBitNr(), pConst) );
  }

public: // Public static methods.

  /// Returns all tuples satisfying pVI == pConst.
  ///   If (pConst) is the max value of pVI (pSymTab->getUniverseSize()-1)
  ///   then the completion for out-of-range-values is applied.
  ///   This is to fulfill the invariant of bddRelation 
  ///   (see comment on top of bddRelation.h).
  static bddRelation
  mkEqual(const bddSymTab* pSymTab,
          unsigned pVarId, 
          reprNUMBER pConst)
  {
    // Standard operation.
    assert(pConst < pSymTab->getUniverseSize());

    bddRelation result = mkEqualPure(pSymTab, pVarId, pConst);

    if( pConst+1 == pSymTab->getUniverseSize() )  // '+' because we use unsigned.
    {
      // If (pConst) is the value (pSymTab->getUniverseSize()-1)
      //   i.e., the maximal value,
      //   then (result) must contain all values greater than or equal to 
      //   (pSymTab->getUniverseSize()-1), i.e. from the interval 
      //   [ pSymTab->getUniverseSize()-1, 2^(pSymTab->getBitNr())-1 ].
      // We just need to add the values outside the 'valid range'.
      bddRelation lTmp = mkRange(pSymTab, pVarId);
      lTmp.complement();
      result.unite(lTmp);
    }
    return result;
  }

  /// Computes the range of valid values
  ///   (see comment on top of this class).
  static bddRelation
  mkRange(const bddSymTab* pSymTab,
          unsigned pVarId)
  {
    return bddRelation(pSymTab, 
                       bddBdd::mkLessEqual(pVarId,
                                           pSymTab->getBitNr(),
                                           pSymTab->getUniverseSize() - 1 ) 
                       );
  }

  /// Returns set of all tuples satisfying pVar1 == pVar2.
  static bddRelation
  mkEqual(const bddSymTab* pSymTab,
          const string& pVar1, 
          const string& pVar2)
  {
    bddRelation result(pSymTab, false);
    unsigned lVarId1 = pSymTab->getAttributePos(pVar1);
    unsigned lVarId2 = pSymTab->getAttributePos(pVar2);

    for(reprNUMBER lValueIt = 0;
        lValueIt < pSymTab->getUniverseSize();
        ++lValueIt)
    {
      bddRelation lValuePair = mkEqual(pSymTab, lVarId1, lValueIt);
      lValuePair.intersect(    mkEqual(pSymTab, lVarId2, lValueIt));
      result.unite(lValuePair);
    }
    return result;
  }

  /// Returns set of all tuples satisfying pVar1 < pVar2.
  /// Assertion: pVar1's position  <=  pVar2's position in the variable order.
  static bddRelation
  mkLess(const bddSymTab* pSymTab,
         const string& pVar1, 
         const string& pVar2)
  {
    bddRelation result(pSymTab, false);
    unsigned lVarId1 = pSymTab->getAttributePos(pVar1);
    unsigned lVarId2 = pSymTab->getAttributePos(pVar2);
    assert(lVarId1 <= lVarId2);

    bddRelation lVar2Values(pSymTab, false);
    for(int lValueIt = pSymTab->getUniverseSize()-1;
        lValueIt > 0 ;
        --lValueIt)
    {
      // Construct set {lValueIt, ..., getUniverseSize()-1}
      lVar2Values.unite( mkEqual(pSymTab, lVarId2, lValueIt) );

      // Construct relation {lValueIt-1 < lValueIt, ..., lValueIt-1 < pSymTab->getUniverseSize()-1}.
      bddRelation lRel(lVar2Values);
      lRel.intersect( mkEqual(pSymTab, lVarId1, lValueIt-1) );
      result.unite(lRel);
    }
    return result;
  }

  /// Return all tupels where the
  ///   value of attribute (pAttributeName) is (pAttributeValue).
  static bddRelation
  mkAttributeValue(const bddSymTab* pSymTab,
                   const string& pAttributeName, 
                   const string& pAttributeValue)
  {
    return mkEqual(pSymTab, 
                   pSymTab->getAttributePos(pAttributeName), 
                   pSymTab->getValueNum(pAttributeValue));
  }
  
private: // Attributes.

  /// Symbol table (associated).
  const bddSymTab* mSymTab;
  /// Set of tuples.
  bddBdd mBdd;

public: // Attributes.

  /// For a stronger type check, remember the arity.
  /// '-1' for undefined arity, '0' for boolean, '1' for set, '2' for binary relation, ...
  /// This is only provided as a feature to more abstract layers;
  /// it is only partially used, and there is no guarantee that the arity is always correctly set.
  /// I.e., this attribute is not controlled by this class.
  int mArity;

public: // Constructors and destructor.
  
  bddRelation(const bddSymTab* pSymTab, bool full)
    : mSymTab(pSymTab),
      mBdd(full),
      mArity(-1)
  {}
  // Use the standard copy constructor. 
  // Use the standard destructor.
  // Use the standard operator '='.

private:

  bddRelation(const bddSymTab* pSymTab, const bddBdd& pBdd)
    : mSymTab(pSymTab),
      mBdd(pBdd),
      mArity(-1)
  {}
  /// Forbid implicite casts.
  bddRelation(void*);
  /// Forbid the use of some ugly standard operators.
  void operator,(const bddRelation&);

private: // Service methods.

  /// Renaming of attributes.
  ///   Safe variant, without preconditions.
  void
  renameSafe(unsigned pVarIdOld, unsigned pVarIdNew, unsigned pBitNr) 
  {
    // Walk through all BDD variables.

    // For all bits of the binary encoding of both attributes.
    if( pVarIdOld > pVarIdNew )
    {
      for (unsigned lIt = 0; 
           lIt < pBitNr;
           ++lIt)
      {
        unsigned lPosOld = pVarIdOld + lIt;     
        unsigned lPosNew = pVarIdNew + lIt;     
        
        mBdd.intersect(bddBdd(lPosOld, lPosNew));
        mBdd.exists(lPosOld);
      }
    }
    else
    {
      for (int lIt = pBitNr-1; 
           lIt >= 0;
           --lIt)
      {
        unsigned lPosOld = pVarIdOld + lIt;     
        unsigned lPosNew = pVarIdNew + lIt;     
        
        mBdd.intersect(bddBdd(lPosOld, lPosNew));
        mBdd.exists(lPosOld);
      }
    }
  }


public: // Service methods.

  /// Returns number of tuples, restricted to the given attributes.
  /// Assuming that all free attributes are contained in 'pFree'.
  double
  getTupleNr(const set<string>& pFree) const
  {
    if (pFree.size() == 0) {
      return isEmpty() ? 0 : 1;
    }

    map<unsigned,string> lVarOrd = mSymTab->computeVariableOrder(pFree);
    unsigned lFirstVar = lVarOrd.begin()->first;
    unsigned lLastVar  = (--lVarOrd.end())->first;
    assert(lFirstVar <= lLastVar);

    // First exclude out-of-range bit vectors 
    //   (see comment at top of the class).
    bddRelation lTmp = *this;
    { // For all attributes in the given range.
      for (unsigned lVarId = lFirstVar;  
           lVarId <= lLastVar;  
           lVarId += mSymTab->getBitNr() ) 
      {
        bddRelation lRange(mSymTab, false);
        if (lVarOrd.find(lVarId) == lVarOrd.end()) {
          lRange.unite(mkEqualPure(mSymTab, lVarId, 0));
        } else {
          lRange.unite(mkRange(mSymTab, lVarId));
        }
        lTmp.intersect(lRange);
      }
    }

    // No nodes allowed in front of the first variable id.
    assert( !lTmp.mBdd.testVars(0, lFirstVar - 1) );
    return lTmp.mBdd.getTupleNr(lFirstVar, lLastVar + mSymTab->getBitNr() - 1);
  }

  /// Returns a value for attribute at position 'pVarId'.
  /// Assumes that the relation is not empty.
  string
  getElement(unsigned pVarId)
  {
    unsigned lNumValue = mBdd.getTuple(pVarId, pVarId + mSymTab->getBitNr()-1);
    return mSymTab->getAttributeValue(lNumValue);
  }


public: // Operations.

  bool
  setEqual(const bddRelation& p) const {
    return mBdd.setEqual( p.mBdd );
  }

  /// Check if (*this) contains (p).
  bool
  setContains(const bddRelation& p) const {
    return mBdd.setContains( p.mBdd );
  }
  
  bool
  isEmpty() const {
    return mBdd.isEmpty();
  }
  
  void
  complement() {
    mBdd.complement();
  }

  void
  unite(const bddRelation& p) {
    mBdd.unite(p.mBdd);
  }

  void
  intersect(const bddRelation& p) {
    mBdd.intersect(p.mBdd);
  }

  /// Existential quantification of (pAttribute).
  void
  exists(const string pAttribute) {
    unsigned lVarId = mSymTab->getAttributePos(pAttribute);
    // Existential quantification of 'mSymTab->getBitNr()' bits,
    //   beginning at 'lVarId', i.e., for all bits of the encoding of 'pAttribute'.
    for (int i = mSymTab->getBitNr() - 1;  i >= 0;  --i)
    {
      mBdd.exists(lVarId + i);
    }
  }

  /// Check if there is any BDD node within the given range of attributes.
  /// Returns 'true' if any such node is found.
  bool
  testVars(string pVarFirst, string pVarLast)
  {
    unsigned lVarIdFirst = mSymTab->getAttributePos(pVarFirst);
    unsigned lVarIdLast  = mSymTab->getAttributePos(pVarLast) + mSymTab->getBitNr()-1;
    assert(lVarIdFirst <= lVarIdLast);
    return mBdd.testVars(lVarIdFirst, lVarIdLast);
  }

  /// Renaming of attributes.
  void
  rename(const string pAttributeOld, const string pAttributeNew) {
    unsigned lVarIdOld = mSymTab->getAttributePos(pAttributeOld);
    unsigned lVarIdNew = mSymTab->getAttributePos(pAttributeNew);

    // Renaming of attributes. 

    // For direct renaming:
    // Forbidden range of variable ids for renaming [min, max].
    // Case 1) pVarIdOld < pVarIdNew
    unsigned lVarIdFirst = lVarIdOld + mSymTab->getBitNr();
    unsigned lVarIdLast  = lVarIdNew + mSymTab->getBitNr()-1;
    // Case 2) pVarIdOld > pVarIdNew
    if( lVarIdOld > lVarIdNew )
    {
      lVarIdFirst = lVarIdNew;
      lVarIdLast  = lVarIdOld - 1;
    }

    // Check if there is any BDD node of a varId between the variables
    //   or of a varId of the new variable.
    if( mBdd.testVars(lVarIdFirst, lVarIdLast) )
    {
      // Safe variant, without preconditions.
      renameSafe(lVarIdOld, lVarIdNew, mSymTab->getBitNr());
    }
    else
    {
      // Direct variant, 
      //   with the precondition that no BDD node exists between the variables.
      // Parameters: First varId to rename.
      //             Last varId to rename.
      //             Offset, i.e. distance to new.
      mBdd.renameVars(lVarIdOld,
                      lVarIdOld + (mSymTab->getBitNr() - 1),
                      lVarIdNew - lVarIdOld
                      );
    }
  }


public: // IO.

  void
  printBddInfo(ostream& pS) const {
    // Includes garbage collection to get real values.
    unsigned lFreeNodes = mBdd.getFreeNodeNr();
    unsigned lMaxNodes  = mBdd.getMaxNodeNr();

    // Infos about BDD package.
    pS << "Number of BDD nodes: " << mBdd.getNodeNr() << endl
       << "Percentage of free nodes in BDD package: " 
       << lFreeNodes << " / " << lMaxNodes << " = " 
       << (unsigned)(((double)lFreeNodes / (double)lMaxNodes) * 100) 
       << " %" << endl;
  }

  /// Prints the number of nodes per variable id for the BDD.
  ///   (For BDD visualization.)
  void
  printNodesPerVarId(ostream& pS, const set<string>& pFree) const {
    map<unsigned, string>   lVarOrd = mSymTab->computeVariableOrder(pFree);
    map<unsigned, unsigned> lBddNodesPerVar = mBdd.getNodesPerVarId();
    if (lVarOrd.empty() || lBddNodesPerVar.empty()) {  // BDD has only terminal nodes.
      pS << 0 << endl;
      return;
    }

    // Print the number of BDD nodes for all var ids 
    //   in the range [lPos, lLastId].
    // Invariant: BDD contains nodes only for user attributes,
    //   more precise, nodes with var ids >= lPos and <= lLastId.
    unsigned lPos = lVarOrd.begin()->first;
    unsigned lLastId = (--lVarOrd.end())->first + mSymTab->getBitNr() - 1;
    assert( lBddNodesPerVar.begin()->first >= lPos );
    assert( (--lBddNodesPerVar.end())->first <= lLastId );

    while (lPos <= lLastId) { 
      unsigned lPosFirst = (lPos / mSymTab->getBitNr()) * mSymTab->getBitNr();
      // Print name.
      if (lVarOrd.find(lPosFirst) != lVarOrd.end()) {
        pS << lVarOrd[lPosFirst] << '_' << lPos - lPosFirst 
           << '(' << lPos << ')' 
           << endl;
      }
      // Print value.
      if ( lBddNodesPerVar.find(lPos) != lBddNodesPerVar.end() ) {
        pS << lBddNodesPerVar[lPos] << endl;
      } else {
        pS << 0 << endl;
      }
      ++lPos;
    }
  }

  /// Print set of tuples (plain) according to the given order of attributes.
  /// Recursiv procedure.
  void 
  printRelation(ostream& pS, 
                const string pTuple,
                vector<string> pAttributeList) const 
  {
    // Empty relation?
    if( isEmpty() ) {
      return;
    }

    // End of Recursion.
    if( pAttributeList.empty() ) {
      pS << pTuple << endl;
      return;
    }

    // Proceed next attribute (i.e. column in relation) in given list.
    // Use a copy of this relation.
    bddRelation lRel(*this);
    
    // Output of the values of the first attribute in (pTermList),
    //   i.e. the given ordering is regarded.
    unsigned lVarId = mSymTab->getAttributePos( * pAttributeList.begin() );
    pAttributeList.erase(pAttributeList.begin());

    // For all values of the current attribute.
    while (!lRel.isEmpty()) {
      // Get next value of the attribute.
      unsigned lNumValue = lRel.mBdd.getTuple(lVarId, 
                                              lVarId + mSymTab->getBitNr()-1);

      // Compute cofactor for current value in (lTmpRel).
      bddRelation lTmpRel(lRel);
      lTmpRel.intersect( mkEqual(mSymTab, lVarId, lNumValue) );
      // Print value.
      string lStringValue( mSymTab->getAttributeValue(lNumValue) );
      if( mSymTab->isQuoted(lStringValue) )
      {
        lStringValue = '"' + lStringValue + '"';
      }
      // Recursive call for the next attribute.
      lTmpRel.printRelation(pS, pTuple + lStringValue + '\t', pAttributeList);
      
      // Delete printed tuples.
      lTmpRel.complement();  
      lRel.intersect( lTmpRel );
    }
  }

  /// Prints output graph for BDD.
  void
  printGraph(ostream& pS, const set<string>& pFree) const {
    // BDD contains nodes only for user attributes.
    map<unsigned,string> lVarOrd = mSymTab->computeVariableOrder(pFree);

    bool need1Terminal = true;
    bool need0Terminal = true;
    if (setEqual(bddRelation(mSymTab, true))) {  // Full set.
      need0Terminal = false;
    }
    if (setEqual(bddRelation(mSymTab, false))) { // Empty set.
      need1Terminal = false;
    }

    // Maps var ids to nodes.
    multimap<unsigned,bddGraphNode> lGraph;
    mBdd.getGraph(lGraph);

    string lEdgeStyle = " [arrowsize=\"1.0\",fontname=\"Helvetica\",fontsize=\"8\",";
    string lNodeStyle = " [fontname=\"Helvetica\",fontsize=\"16\",height=\"0.3\",width=\"0.5\",color=black,style=unfilled,";

    pS << "digraph BDD {" << endl;
    pS << "size=\"7.5,10\";" << endl << endl;

    // Nodes.
    if (lGraph.size() > 0) {
      pS << "{ rank=same;" << endl;
    }
    multimap<unsigned,bddGraphNode>::const_iterator lItPrev = lGraph.begin();
    for (multimap<unsigned,bddGraphNode>::const_iterator lIt = lGraph.begin();
         lIt != lGraph.end();
         ++lIt) {

      if (lIt->first != lItPrev->first) {
        pS << "}" << endl << endl 
           << "{ rank=same;" << endl;
      }
      lItPrev = lIt;

      pS << lIt->second.id << lNodeStyle;
      unsigned lPos = lIt->second.var;
      unsigned lPosFirst = (lPos / mSymTab->getBitNr()) * mSymTab->getBitNr();
      assert(lVarOrd.find(lPosFirst) != lVarOrd.end());
      pS << "label=\"" << lVarOrd[lPosFirst] << '_' << lPos - lPosFirst << "\"];" << endl;
    }
    if (lGraph.size() > 0) {
      pS << "}" << endl << endl;
    }

    // Terminal nodes.
    pS << endl
       << "{ rank=same;" << endl;
    if (need1Terminal) {
      pS << 1 << lNodeStyle << "shape=box,label=\"1\"];" << endl << endl;
    }
    if (need0Terminal) {
      pS << 0 << lNodeStyle << "shape=box,label=\"0\"];" << endl << endl;
    }
    pS << "}" << endl << endl << endl;

    // Edges.
    for (multimap<unsigned,bddGraphNode>::const_iterator lIt = lGraph.begin();
         lIt != lGraph.end();
         ++lIt) {
      // Low-edge.
      pS << lIt->second.id << " -> " << lIt->second.low 
         << lEdgeStyle << "label=\"0\"," << "style=dotted]" << endl;
      // High-edge.
      pS << lIt->second.id << " -> " << lIt->second.high
         << lEdgeStyle << "label=\"1\"," << "style=solid]" << endl;
      pS << endl;
    }

    pS << "}" << endl;
  }

  /// Prints mBdd as reduced binary decision tree.
  void
  printBDT(ostream& pS) const {
    mBdd.print(pS);
  }

  /// Cut given relation to single element for all given attributes.
  /// Recursiv procedure.
  bddRelation 
  getTupleOf(const map<unsigned, string>& pAttributeList) const 
  {
    assert(!isEmpty());
    // Compute cofactor for the (lexicographically) first value at each attribute.
    bddRelation lRel(*this);
    for(map<unsigned, string>::const_iterator lIt = pAttributeList.begin();
        lIt != pAttributeList.end();
        ++lIt)
    {
      unsigned lVarId = mSymTab->getAttributePos( lIt->second );
      // Consider one single value of attribute (*lIt),
      //   i.e., the given ordering in (pAttributeList) is regarded (just for efficiency).
      unsigned lNumValue = lRel.mBdd.getTuple(lVarId, 
                                              lVarId + mSymTab->getBitNr()-1);
      lRel.intersect( mkEqual(mSymTab, lVarId, lNumValue) );
    }
    return lRel;
  }

};

class bddRelationConst : public bddRelation
{
public:
  bddRelationConst(bddRelation pRel)
    : bddRelation(pRel)
  {}
};

#endif // _bddRelation_h_
