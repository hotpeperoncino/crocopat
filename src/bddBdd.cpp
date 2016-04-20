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

#include "bddBdd.h"

#include <vector>
#include <iostream>
#include <set>
#include <cstdlib>
#include <cstring>
#include <climits>

bddNode* bddBdd::mNodes = 0;
unsigned bddBdd::mMaxNodeNr;
unsigned bddBdd::mFree;

multiset<unsigned> bddBdd::mExtRefs;

unsigned* bddBdd::mUniqueHash = 0;
unsigned bddBdd::mUniqueHBitNr;
bddBinEntry* bddBdd::mBinCache = 0;
unsigned bddBdd::mBinCBitNr;
bddStatEntry* bddBdd::mStatCache = 0;
unsigned bddBdd::mStatCBitNr;

/// For renameVars().
unsigned bddBdd::mRenameCnt = 0;

/////////////////////////////////////////////////////////////////
/// Ensures that p1 <= p2.
inline void 
bddBdd::normalize(unsigned& p1, unsigned& p2) 
{
  if (p1 > p2) {
    unsigned tmp = p1;
    p1 = p2;
    p2 = tmp;
  }
}

/// Hash functions.
inline unsigned 
bddBdd::hash (unsigned i, unsigned hashBitNr) 
{
  unsigned mask = (1u << hashBitNr) - 1;
  return i & mask;
}

inline unsigned 
bddBdd::hash(unsigned i, unsigned j, unsigned k, unsigned hashBitNr) 
{
  unsigned mask = (1u << hashBitNr) - 1;
  j ^= 0x55555555;   // XOR.
  return (i + 
    (j<<(hashBitNr>>1)) + (j>>(hashBitNr>>1)) +
    (k<<(hashBitNr>>2)) + (k>>(hashBitNr>>2))) & mask;
}

/*
/// Additional hash functions for comparison.
inline unsigned 
bddBdd::hash(unsigned i, unsigned j, unsigned hashBitNr) {
  unsigned mask = (1u << hashBitNr) - 1;
  return (i + 
    ((j ^ 0x55555555)<<(hashBitNr>>1)) +
    (j<<(hashBitNr>>2)) + (j>>(hashBitNr>>2))) & mask;
}

inline unsigned 
bddBdd::hash(unsigned i, unsigned j, unsigned hashBitNr) {
  return ((i * 17765507 + j) * 9243337) >> (32-hashBitNr);
}

inline unsigned 
bddBdd::hash(unsigned i, unsigned j, unsigned k, unsigned hashBitNr) {
  return (((i * 14099753 + j) * 9243337 + k) * 3901787) >> (32-hashBitNr);
}
*/

/// Marks (mark=1) all nodes of the BDD with the root pRoot.
void 
bddBdd::mark(unsigned pRoot) 
{
  if(!mNodes[pRoot].mark) 
  {
    mNodes[pRoot].mark = 1;
    mark(mNodes[pRoot].low);
    mark(mNodes[pRoot].high); 
  }
}

/// Unmarks (mark=0) all nodes of the BDD with the root pRoot.
/// Terminal always remain marked.
void 
bddBdd::unMark(unsigned pRoot) 
{
  if(mNodes[pRoot].mark && pRoot != 0 && pRoot != 1)
  {
    mNodes[pRoot].mark = 0;
    unMark(mNodes[pRoot].low);
    unMark(mNodes[pRoot].high);
  }
}

/// Garbage collection: All dead nodes (i.e. all nodes which are not reachable
/// from a node in mExtRefs) are freed (i.e. inserted into unused-list mFree).
/// Terminal nodes are never freed.
void 
bddBdd::gc() 
{
  // Mark all live nodes.
  for(multiset<unsigned>::iterator lIt = mExtRefs.begin();
      lIt != mExtRefs.end();
      lIt++)
  {
    mark(*lIt);
  }
  
  // Clear hash and caches.
  memset(mUniqueHash, 0, (1u << mUniqueHBitNr) * sizeof(unsigned));
  memset(mStatCache, 0, (1u << mStatCBitNr) * sizeof(bddStatEntry));
  memset(mBinCache, 0, (1u << mBinCBitNr) * sizeof(bddBinEntry));

  mFree = 0;
  for(unsigned lCnt = mMaxNodeNr-1; lCnt >= 2; --lCnt) 
  {
    if(!mNodes[lCnt].mark) 
    { 
      // Free dead nodes.
      mNodes[lCnt].low = mFree;
      mFree = lCnt;
    } 
    else 
    {                
      mNodes[lCnt].mark = 0;
      // Insert live nodes into mUniqueHash.
      unsigned lHashIndex = hash(
        mNodes[lCnt].var, mNodes[lCnt].low, mNodes[lCnt].high, mUniqueHBitNr);
      mNodes[lCnt].next = mUniqueHash[lHashIndex];
      mUniqueHash[lHashIndex] = lCnt;
    }
  }
}


/// Returns the mNodes-index of the node with the passed var-, low- and
/// high-values. If such node does not exists, it is inserted into
/// mNodes and mUniqueHash. If there are no free nodes left, throws exception.
unsigned 
bddBdd::insert(unsigned pVar, unsigned pLow, unsigned pHigh) 
{
  unsigned lHashIndex;
  unsigned lResult;

  // BDD-reduction of nodes with equal high- and low-child.
  if(pHigh == pLow) 
  {
    return pLow;
  }

  // Search mUniqueHash.
  lHashIndex = hash(pVar, pLow, pHigh, mUniqueHBitNr);
  lResult = mUniqueHash[lHashIndex];  
  while(lResult) 
  {
    if(   mNodes[lResult].var == pVar 
       && mNodes[lResult].low == pLow
       && mNodes[lResult].high == pHigh) 
    {
      return lResult;
    }
    lResult = mNodes[lResult].next;
  }

  // Create new node.
  if(mFree == 0)
  {
    throw "Error: BDD package out of memory\n";
  }

  lResult = mFree;
  mFree = mNodes[lResult].low;
  mNodes[lResult].var = pVar;
  mNodes[lResult].low = pLow;
  mNodes[lResult].high = pHigh;
  mNodes[lResult].next = mUniqueHash[lHashIndex];
  mUniqueHash[lHashIndex] = lResult;

  return lResult;
}

/// Check if there is any BDD node between the variable ids.
/// Returns 'true' if any such node is found.
bool 
bddBdd::testVars_(unsigned pRoot, 
                  unsigned pVarIdFirst, 
                  unsigned pVarIdLast)
{
  // Note: Terminal nodes are always marked.
  if(mNodes[pRoot].mark)
  {
    return false;
  }
  mNodes[pRoot].mark = 1;

  // Node after forbidden range.
  if(mNodes[pRoot].var > pVarIdLast)
  {
    return false;
  }
  
  // Node within forbidden range.
  if(    mNodes[pRoot].var <= pVarIdLast 
     &&  mNodes[pRoot].var >= pVarIdFirst )
  {
    return true;
  }

  // Process node in front of forbidden range.
  if( testVars_(mNodes[pRoot].low,  pVarIdFirst, pVarIdLast) )
  {
    return true;
  }

  return testVars_(mNodes[pRoot].high,  pVarIdFirst, pVarIdLast);
}

/// Returns number of represented tuples of the BDD with root pRoot.
/// pVar is the first and pMaxVar is the maximum id of a variable.
/// The BDD must not contain nodes with other variable ids.
double 
bddBdd::getTupleNr_(unsigned pRoot, unsigned pVar, unsigned pMaxVar)
{
  // Terminal case.
  if(pVar > pMaxVar)
  {
    return (pRoot == 0 ? 0.0 : 1.0);
  }

  // pVar <= pMaxVar.

  // Reduced node?
  if(pVar < mNodes[pRoot].var)
  {
    return 2 * getTupleNr_(pRoot, pVar + 1, pMaxVar);
  }

  // Forbid nodes with variable id less than 'pVar'.
  assert(pVar == mNodes[pRoot].var);

  // Now pVar == mNodes[pRoot].var holds.
  // Result in cache?
  unsigned lCacheIndex = hash(pRoot, mStatCBitNr);
  if(mStatCache[lCacheIndex].root == pRoot)
  {
    return mStatCache[lCacheIndex].result;
  }

  mStatCache[lCacheIndex].result 
    = getTupleNr_(mNodes[pRoot].low, pVar + 1, pMaxVar)
    + getTupleNr_(mNodes[pRoot].high, pVar + 1, pMaxVar);
  mStatCache[lCacheIndex].root = pRoot;

  return mStatCache[lCacheIndex].result;
}

/// Returns the elements pVar ... pMaxVar of an arbitrary tuple of the BDD.
unsigned 
bddBdd::getTuple_(unsigned pRoot, unsigned pVar, unsigned pMaxVar)
{
  // Terminal case.
  if(pVar > pMaxVar)
  {
    return 0;
  }

  // pVar <= pMaxVar.

  // Reduced node?
  if(pVar < mNodes[pRoot].var)
  {
    return getTuple_(pRoot, pVar + 1, pMaxVar);
  }

  if(pVar > mNodes[pRoot].var)
  {
    if (mNodes[pRoot].low != 0) {
      return getTuple_(mNodes[pRoot].low, pVar, pMaxVar);
    } else { 
      return getTuple_(mNodes[pRoot].high, pVar, pMaxVar);
    } 
  }

  // pVar == mNodes[pRoot].var.

  if (mNodes[pRoot].low != 0) {
    return getTuple_(mNodes[pRoot].low, pVar + 1, pMaxVar);
  } else { 
    return getTuple_(mNodes[pRoot].high, pVar + 1, pMaxVar)
      + (1 << (pMaxVar - pVar));
  } 
}

/// Returns number of nodes of the BDD with root pRoot.
/// (Terminal nodes are not counted). Side effect: counted nodes are marked.
unsigned 
bddBdd::getNodeNr_(unsigned pRoot) 
{
  // Note: Terminal nodes are always marked.
  if(mNodes[pRoot].mark)
  {
    return 0;
  }
  mNodes[pRoot].mark = 1;

  return getNodeNr_(mNodes[pRoot].low) + getNodeNr_(mNodes[pRoot].high) + 1;
}

/// Computes the number of nodes per variable id.
void 
bddBdd::getNodesPerVarId_(unsigned pRoot, 
                          map<unsigned, unsigned>& pBddNodesPerVar)
{
  // Note: Terminal nodes are always marked.
  if( !mNodes[pRoot].mark )
  {
    mNodes[pRoot].mark = 1;
    if ( pBddNodesPerVar.find(mNodes[pRoot].var) != pBddNodesPerVar.end() ) {
      ++ pBddNodesPerVar[mNodes[pRoot].var];
    } else {
      pBddNodesPerVar[mNodes[pRoot].var] = 1;
    }
    getNodesPerVarId_(mNodes[pRoot].low,  pBddNodesPerVar); 
    getNodesPerVarId_(mNodes[pRoot].high, pBddNodesPerVar);
  }
}

/// Creates output graph representation for BDD with root pRoot.
void 
bddBdd::getGraph_(unsigned pRoot, 
                  multimap<unsigned,bddGraphNode>& pGraph)
{
  // Note: Terminal nodes are always marked.
  if( !mNodes[pRoot].mark )
  {
    mNodes[pRoot].mark = 1;

    bddGraphNode node;
    node.id   = pRoot;
    node.var  = mNodes[pRoot].var;
    node.low  = mNodes[pRoot].low;
    node.high = mNodes[pRoot].high;
    pGraph.insert( pair<unsigned,bddGraphNode>(node.var, node) );

    getGraph_(mNodes[pRoot].low,  pGraph); 
    getGraph_(mNodes[pRoot].high, pGraph);
  }
}

/// Prints BDD with root pRoot as reduced binary decision tree.
void 
bddBdd::print_(ostream& pS, unsigned pRoot)
{
  if(pRoot == 1) 
  {
    cout << 'T';
  }
  else 
  {
    if(pRoot == 0)
    {
      cout << 'F';
    }
    else 
    {
      cout << '(';
      print_(pS, mNodes[pRoot].low);
      cout << ' ' << mNodes[pRoot].var << ' ';
      print_(pS, mNodes[pRoot].high);
      cout << ')';
    }
  }
}

/// Checks if pRoot2 represents a subset of pRoot1
bool 
bddBdd::setContains_(unsigned pRoot1, unsigned pRoot2) 
{
  unsigned lCacheIndex;
  unsigned lResult;

  // Terminal cases.
  if(pRoot1 == 1)
  {
    return true;
  }
  if(pRoot2 == 0)
  {
    return true;
  }
  if(pRoot1 == pRoot2)
  {
    return true;
  }
  if(pRoot1 == 0) // Assumes pRoot2 != 0.
  {
    return false;
  }
  if(pRoot2 == 1) // Assumes pRoot1 != 1.
  {
    return false;
  }
  
  // Result in Cache?
  lCacheIndex = hash(mSetContains, pRoot1, pRoot2, mBinCBitNr);
  if(mBinCache[lCacheIndex].op == mSetContains
    && mBinCache[lCacheIndex].root1 == pRoot1
    && mBinCache[lCacheIndex].root2 == pRoot2) 
  {
    return mBinCache[lCacheIndex].result;
  }

  // Compute result.
  if(mNodes[pRoot1].var < mNodes[pRoot2].var)
  {
    lResult = setContains_(mNodes[pRoot1].low, pRoot2)
      && setContains_(mNodes[pRoot1].high, pRoot2);
  }
  else 
  {
    if(mNodes[pRoot1].var == mNodes[pRoot2].var)
    {
      lResult = setContains_(mNodes[pRoot1].low, mNodes[pRoot2].low)
        && setContains_(mNodes[pRoot1].high, mNodes[pRoot2].high);
    }
    else 
    {
      lResult = setContains_(pRoot1, mNodes[pRoot2].low)
        && setContains_(pRoot1, mNodes[pRoot2].high);
    }
  }

  // Write result into cache.
  mBinCache[lCacheIndex].result = lResult;
  mBinCache[lCacheIndex].op = mSetContains;
  mBinCache[lCacheIndex].root1 = pRoot1;
  mBinCache[lCacheIndex].root2 = pRoot2;

  return lResult;
}

unsigned 
bddBdd::complement_(unsigned pRoot) 
{
  unsigned lCacheIndex;
  unsigned lResult;

  if(pRoot == 0)
  {
    return 1;
  }
  if(pRoot == 1)
  {
    return 0;
  }

  lCacheIndex = hash(mComplement, pRoot, pRoot, mBinCBitNr);
  if(mBinCache[lCacheIndex].op == mComplement
    && mBinCache[lCacheIndex].root1 == pRoot) 
  {
    return mBinCache[lCacheIndex].result;
  }

  lResult = insert(mNodes[pRoot].var,
                   complement_(mNodes[pRoot].low), 
                   complement_(mNodes[pRoot].high));

  mBinCache[lCacheIndex].result = lResult;
  mBinCache[lCacheIndex].op = mComplement;
  mBinCache[lCacheIndex].root1 = pRoot;

  return lResult;
}

unsigned 
bddBdd::unite_(unsigned pRoot1, unsigned pRoot2) 
{
  if(pRoot1 == 1) 
  {
    return 1;
  }
  if(pRoot2 == 1) 
  {
    return 1;
  }
  if(pRoot1 == 0)
  {
    return pRoot2;
  }
  if(pRoot2 == 0)
  {
    return pRoot1;
  }
  if(pRoot1 == pRoot2)
  {
    return pRoot1;
  }

  normalize(pRoot1, pRoot2);
  unsigned lResult;
  unsigned lCacheIndex = hash(mUnite, pRoot1, pRoot2, mBinCBitNr);
  if (mBinCache[lCacheIndex].op == mUnite
    && mBinCache[lCacheIndex].root1 == pRoot1
    && mBinCache[lCacheIndex].root2 == pRoot2) 
  {
    return mBinCache[lCacheIndex].result;
  }

  if(mNodes[pRoot1].var < mNodes[pRoot2].var)
  {
    lResult = insert(mNodes[pRoot1].var,
                     unite_(mNodes[pRoot1].low, pRoot2), 
                     unite_(mNodes[pRoot1].high, pRoot2));
  }
  else 
  {
    if(mNodes[pRoot1].var == mNodes[pRoot2].var)
    {
      lResult = insert(mNodes[pRoot1].var,
                       unite_(mNodes[pRoot1].low, mNodes[pRoot2].low), 
                       unite_(mNodes[pRoot1].high, mNodes[pRoot2].high));
    }
    else 
    {
      lResult = insert(mNodes[pRoot2].var,
                       unite_(pRoot1, mNodes[pRoot2].low), 
                       unite_(pRoot1, mNodes[pRoot2].high));
    }
  }

  mBinCache[lCacheIndex].result = lResult;
  mBinCache[lCacheIndex].op = mUnite;
  mBinCache[lCacheIndex].root1 = pRoot1;
  mBinCache[lCacheIndex].root2 = pRoot2;

  return lResult;
}

unsigned 
bddBdd::intersect_(unsigned pRoot1, unsigned pRoot2) 
{
  if(pRoot1 == 0) 
  {
    return 0;
  }
  if(pRoot2 == 0) 
  {
    return 0;
  }
  if(pRoot1 == 1) 
  {
    return pRoot2;
  }
  if(pRoot2 == 1) 
  {
    return pRoot1;
  }
  if(pRoot1 == pRoot2)
  {
    return pRoot1;
  }

  normalize(pRoot1, pRoot2);
  unsigned lResult;
  unsigned lCacheIndex = hash(mIntersect, pRoot1, pRoot2, mBinCBitNr);
  if(mBinCache[lCacheIndex].op == mIntersect
    && mBinCache[lCacheIndex].root1 == pRoot1
    && mBinCache[lCacheIndex].root2 == pRoot2) 
  {
    return mBinCache[lCacheIndex].result;
  }

  if(mNodes[pRoot1].var < mNodes[pRoot2].var) 
  {
    lResult = insert(mNodes[pRoot1].var,
                     intersect_(mNodes[pRoot1].low, pRoot2), 
                     intersect_(mNodes[pRoot1].high, pRoot2));
  }
  else 
  {
    if(mNodes[pRoot1].var == mNodes[pRoot2].var)
    {
      lResult = insert(mNodes[pRoot1].var,
                       intersect_(mNodes[pRoot1].low, mNodes[pRoot2].low), 
                       intersect_(mNodes[pRoot1].high, mNodes[pRoot2].high));
    }
    else 
    {
      lResult = insert(mNodes[pRoot2].var,
                       intersect_(pRoot1, mNodes[pRoot2].low), 
                       intersect_(pRoot1, mNodes[pRoot2].high));
    }
  }

  mBinCache[lCacheIndex].result = lResult;
  mBinCache[lCacheIndex].op = mIntersect;
  mBinCache[lCacheIndex].root1 = pRoot1;
  mBinCache[lCacheIndex].root2 = pRoot2;

  return lResult;
}

unsigned 
bddBdd::exists_(unsigned pRoot, unsigned pVar)
{
  unsigned lCacheIndex;
  unsigned lResult;

  if(mNodes[pRoot].var < pVar)
  {
    lCacheIndex = hash(mExists, pRoot, pVar, mBinCBitNr);
    if(mBinCache[lCacheIndex].op == mExists
       && mBinCache[lCacheIndex].root1 == pRoot
       && mBinCache[lCacheIndex].root2 == pVar) 
    {
      return mBinCache[lCacheIndex].result;
    }

    lResult = insert(mNodes[pRoot].var,
                     exists_(mNodes[pRoot].low, pVar), 
                     exists_(mNodes[pRoot].high, pVar));

    mBinCache[lCacheIndex].result = lResult;
    mBinCache[lCacheIndex].op = mExists;
    mBinCache[lCacheIndex].root1 = pRoot;
    mBinCache[lCacheIndex].root2 = pVar;
  }
  else 
  { 
    if(mNodes[pRoot].var == pVar)
    {
      lResult = unite_(mNodes[pRoot].low, mNodes[pRoot].high);
    }
    else
    {
      lResult = pRoot;
    }
  }

  return lResult;
}

/// Rename variable ids of all nodes from pFirst to pLast
///   by adding pOffset to the variable ids.
unsigned 
bddBdd::renameVars_ (unsigned pRoot, 
                     unsigned pFirst, 
                     unsigned pLast, 
                     int      pOffset) 
{
  if(pRoot == 0 || pRoot == 1)
  {
    return pRoot;
  }

  if(mNodes[pRoot].var > pLast)
  {
    // Process variable ids after pLast.
    return pRoot;
  }

  // The variable (mRenameCnt) is set by renameVars(), 
  //   such that its value is unique in each execution of renameVars().
  // Lookup cache.
  unsigned lCacheIndex = hash(mRenameVars, pRoot, mRenameCnt, mBinCBitNr);
  if(mBinCache[lCacheIndex].op == mRenameVars
    && mBinCache[lCacheIndex].root1 == pRoot
    && mBinCache[lCacheIndex].root2 == mRenameCnt) 
  {
    return mBinCache[lCacheIndex].result;
  }

  unsigned lResult;
  if(mNodes[pRoot].var < pFirst)
  {
    // Process variable ids in front of pFirst.
    lResult = insert(mNodes[pRoot].var,
                     renameVars_(mNodes[pRoot].low,  pFirst, pLast, pOffset),
                     renameVars_(mNodes[pRoot].high, pFirst, pLast, pOffset));
  }
  else
  {
    // Rename node pRoot.
    lResult = insert(mNodes[pRoot].var + pOffset,
                     renameVars_(mNodes[pRoot].low, pFirst, pLast, pOffset),
                     renameVars_(mNodes[pRoot].high, pFirst, pLast, pOffset));
  }
  

  // Write result into cache.
  mBinCache[lCacheIndex].result = lResult;
  mBinCache[lCacheIndex].op = mRenameVars;
  mBinCache[lCacheIndex].root1 = pRoot;
  mBinCache[lCacheIndex].root2 = mRenameCnt;

  return lResult;
}

/// Initialization of BDD package. Must be called before any other 
/// function of the package is used.
/// Parameters: Values for the m... variables.
///   number of elements of mNodes == pNodes,
///   number of elements of mUniqueHash == 2^pUniqueHBitNr,
///   number of elements of mBinCache == 2^pBinCBitNr.
///   number of elements of mStatCache == 2^pStatCBitNr.
void 
bddBdd::init (unsigned pMaxNodeNr, 
              unsigned pUniqueHBitNr, 
              unsigned pBinCBitNr, 
              unsigned pStatCBitNr) {

  // Allocate memory.
  mMaxNodeNr = pMaxNodeNr+2;
  mNodes = new bddNode[mMaxNodeNr];

  mUniqueHBitNr = pUniqueHBitNr;
  mUniqueHash = new unsigned[1u << mUniqueHBitNr];

  mBinCBitNr = pBinCBitNr;
  mBinCache = new bddBinEntry[1u << mBinCBitNr];
  mStatCBitNr = pStatCBitNr;
  mStatCache = new bddStatEntry[1u << mStatCBitNr];

  if(!mNodes || !mUniqueHash || !mBinCache  || !mStatCache) 
  {
    cerr << "Error: "
         << "Not enough memory for initialization of BDD package." << endl;
    exit(EXIT_FAILURE);
  }

  // Initialise arrays.
  memset(mNodes, 0, mMaxNodeNr * sizeof(bddNode));
  memset(mUniqueHash, 0, (1u << mUniqueHBitNr) * sizeof(unsigned));
  memset(mBinCache, 0, (1u << mBinCBitNr) * sizeof(bddBinEntry));
  memset(mStatCache, 0, (1u << mStatCBitNr) * sizeof(bddStatEntry));

  // Initialise terminal nodes.
  //mNodes[0].var = (unsigned)-1;
  mNodes[0].var = (UINT_MAX >> 1);
  mNodes[0].mark = 1;
  //mNodes[1].var = (unsigned)-1;
  mNodes[1].var = (UINT_MAX >> 1);
  mNodes[1].mark = 1;

  // Initialise mFree list of unused nodes.
  mFree = 2;
  for(unsigned lCnt = 2; lCnt < mMaxNodeNr-1; lCnt++)
  {
    mNodes[lCnt].low = lCnt+1;
  }
}

/// Frees memory used by the static data structures.
void 
bddBdd::done () {
  delete mNodes;
  delete mUniqueHash;
  delete mBinCache;
  delete mStatCache;
}

/// Returns overall number of live nodes (Terminal nodes are not counted).
unsigned 
bddBdd::getReachNodeNr()
{
  unsigned lResult;

  lResult = 0;
  for(multiset<unsigned>::iterator lIt = mExtRefs.begin();
      lIt != mExtRefs.end(); 
      ++lIt)
  {
    lResult += getNodeNr_(*lIt);
  }

  for(multiset<unsigned>::iterator lIt = mExtRefs.begin();
      lIt != mExtRefs.end(); 
      ++lIt)
  {
    unMark(*lIt);
  }

  return lResult;
}

/// Returns number of external (user) references in mExtrefs.
unsigned 
bddBdd::getExtRefNr()
{
  return mExtRefs.size();
}

/// Prints list lengths in mUniqueHash.
void 
bddBdd::analyseUniqueHash() 
{
  vector<unsigned> lListLenCnts;
  unsigned lListLen;
  unsigned lUniqueElem;

  for(unsigned lCnt = 0; lCnt < (1u << mUniqueHBitNr); lCnt++) 
  {
    lListLen = 0;
    lUniqueElem = mUniqueHash[lCnt];
    while(lUniqueElem) 
    {
      lUniqueElem = mNodes[lUniqueElem].next;
      ++lListLen;
    }
    ++lListLenCnts[lListLen];
  }

  cout << "Size of uniqueHash: " << (1u << mUniqueHBitNr) << '\n';
  cout << "n | number of lists of length n in uniqueHash:\n";
  for(unsigned lCnt = 0; lCnt < lListLenCnts.size(); lCnt++)
    cout << lCnt << ' ' << lListLenCnts[lCnt] << '\n';
}

/// Creates BDD that assign the value pValue to the variable pVarId.
bddBdd::bddBdd(unsigned pVarId, bool pValue) 
{
  try 
  {
    mRoot = bddBdd::bddBdd_(pVarId, pValue);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      mRoot = bddBdd::bddBdd_(pVarId, pValue);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  incRef();
}

/// Creates BDD that assign the value pValue to the variable pVarId.
unsigned 
bddBdd::bddBdd_(unsigned pVarId, bool pValue) 
{
  if (pValue) 
  {
    return insert(pVarId, 0, 1);
  }
  else
  {
    return insert(pVarId, 1, 0);
  }
}

/// Creates BDD that assigns bit values of 'pValue' 
///   to 'pBitNr' variables beginning at position 'pVarId'.
bddBdd::bddBdd(unsigned pVarId, unsigned pBitNr, unsigned pValue)
{
  try 
  {
    mRoot = bddBdd::bddBdd_(pVarId, pBitNr, pValue);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      mRoot = bddBdd::bddBdd_(pVarId, pBitNr, pValue);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  incRef();
}

/// Creates BDD that assign bit values of 'pValue' 
///   to 'pBitNr' variables beginning at position 'pVarId'.
unsigned 
bddBdd::bddBdd_(unsigned pVarId, unsigned pBitNr, unsigned pValue)
{
  unsigned result = 1;

  // For all bits of the binary encoding of 'pValue'.
  for (unsigned lIt = 0; 
       lIt < pBitNr;
       ++lIt)
  {
    unsigned lPosition = pVarId + (pBitNr - lIt - 1);
    if ( (pValue & (1<<lIt)) > 0 )
    {
      result = insert(lPosition, 0, result);
    }
    else
    {
      result = insert(lPosition, result, 0);
    }
  }
  return result;
}

/// Creates BDD for pVarId1 == pVarId2.
bddBdd::bddBdd(unsigned pVarId1, unsigned pVarId2)
{
  try 
  {
    mRoot = bddBdd::bddBdd_(pVarId1, pVarId2);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      mRoot = bddBdd::bddBdd_(pVarId1, pVarId2);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  incRef();
}
  
/// Creates BDD for pVarId1 == pVarId2.
unsigned 
bddBdd::bddBdd_(unsigned pVarId1, unsigned pVarId2)
{
  unsigned topVarId    = pVarId1;
  unsigned bottomVarId = pVarId2;
  if(pVarId1 > pVarId2)
  {
    topVarId    = pVarId2;
    bottomVarId = pVarId1;
  }
  return insert(topVarId,
                insert(bottomVarId, 1, 0),
                insert(bottomVarId, 0, 1)
                );
}
  
bddBdd& 
bddBdd::operator=(const bddBdd& pBdd) 
{
  decRef();
  mRoot = pBdd.mRoot;
  incRef();
  return *this;
}

/// Creates BDD for 'x <= pValue' 
///   for 'pBitNr' variables of 'x' beginning at position 'pVarId'.
bddBdd
bddBdd::mkLessEqual(unsigned pVarId, unsigned pBitNr, unsigned pValue)
{
  // pValue < 2^pBitNr.
  assert(pValue < (unsigned)(1<<pBitNr));

  unsigned lRoot;
  try 
  {
    lRoot = bddBdd::mkLessEqual_(pVarId, pBitNr, pValue);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      lRoot = bddBdd::mkLessEqual_(pVarId, pBitNr, pValue);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  return bddBdd(lRoot);
}

/// Creates BDD for 'x <= pValue' 
///   for 'pBitNr' variables of 'x' beginning at position 'pVarId'.
unsigned 
bddBdd::mkLessEqual_(unsigned pVarId, unsigned pBitNr, unsigned pValue)
{
  unsigned result = 1;

  // For all bits of the binary encoding of 'pValue'.
  for (unsigned lIt = 0; 
       lIt < pBitNr;
       ++lIt)
  {
    unsigned lPosition = pVarId + (pBitNr - lIt - 1);
    if ( (pValue & (1<<lIt)) > 0 )
    {
      result = insert(lPosition, 1, result);
    }
    else
    {
      result = insert(lPosition, result, 0);
    }
  }
  return result;
}

/// Returns number of nodes (Terminal nodes are not counted).
unsigned 
bddBdd::getNodeNr() const
{
  unsigned lResult = getNodeNr_(mRoot);
  unMark(mRoot);  
  return lResult;
}

/// Returns number of free nodes.
unsigned 
bddBdd::getFreeNodeNr() const
{
  // Includes garbage collection to get real values.
  bddBdd::gc();

  unsigned result = 0;
  unsigned lFreePos = mFree;

  while( lFreePos != 0 )
  {
    ++result;
    lFreePos = mNodes[lFreePos].low;
  }
  return result;
}


/// Computes complement.
void 
bddBdd::complement() 
{
  unsigned lResult;
  try 
  {
    lResult = bddBdd::complement_(mRoot);
  }
  catch(...) 
  {
    // Out of free nodes in nodes array. Collect Garbage and try again.
    bddBdd::gc();
    try
    {
      lResult = bddBdd::complement_(mRoot);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  decRef();
  mRoot = lResult;
  incRef();
}

/// Unites with pBdd.
void 
bddBdd::unite(const bddBdd& pBdd) 
{
  unsigned lResult;
  try 
  {
    lResult = bddBdd::unite_(mRoot, pBdd.mRoot);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      lResult = bddBdd::unite_(mRoot, pBdd.mRoot);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  decRef();
  mRoot = lResult;
  incRef();
}

/// Intersects with pBdd.
void 
bddBdd::intersect(const bddBdd& pBdd) 
{
  unsigned lResult;
  try 
  {
    lResult = bddBdd::intersect_(mRoot, pBdd.mRoot);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      lResult = bddBdd::intersect_(mRoot, pBdd.mRoot);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  decRef();
  mRoot = lResult;
  incRef();
}

/// Existantial quantification of the variable pVar.
void 
bddBdd::exists(unsigned pVar) 
{
  unsigned lResult;
  try 
  {
    lResult = bddBdd::exists_(mRoot, pVar);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      lResult = bddBdd::exists_(mRoot, pVar);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  decRef();
  mRoot = lResult;
  incRef();
}

/// Rename variable ids of all nodes from pFirst to pLast
///   by adding pOffset to the variable ids.
void 
bddBdd::renameVars(unsigned pFirst, unsigned pLast, int pOffset)
{
  unsigned lResult;
  try 
  {
    lResult = bddBdd::renameVars_(mRoot, pFirst, pLast, pOffset);
  }
  catch(...) 
  {
    bddBdd::gc();
    try
    {
      lResult = bddBdd::renameVars_(mRoot, pFirst, pLast, pOffset);
    }
    catch(...)
    {
      cerr << "Error: BDD package out of memory." << endl;
      exit(EXIT_FAILURE);
    }
  }
  decRef();
  mRoot = lResult;
  incRef();

  // Create a fresh, unique cache entry operand for the next call. 
  if (mRenameCnt < UINT_MAX) {
    ++mRenameCnt;
  } else {
    mRenameCnt = 0;
    bddBdd::gc();
  }
}

