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

#ifndef _bddBdd_h
#define _bddBdd_h

#include "relObject.h"

#include <vector>
#include <set>
#include <map>
#include <cassert>

/// BDD node
struct bddNode 
{
  /// To mark the node in garbage collection and node count
  unsigned mark:1;
  /// Variable id. The ids of variables are multiples of 1, starting with 0!  
  unsigned var:31; 
  /// Index of low son. Also used for linking unused nodes in the node array.
  unsigned low;    
  /// Index of high son.
  unsigned high;
  /// Next node in mUniqueHash-list (0 for end of list).
  unsigned next;
};

/// Cache entry for binary operations
struct bddBinEntry 
{  
  /// Identifier of the operation
  unsigned op;     
  /// First argument of the operation
  unsigned root1;
  /// Second argument of the operation
  unsigned root2;
  /// Result of the operation
  unsigned result;
};

/// Cache entry for counting tuples.
struct bddStatEntry 
{
  /// Argument of the operation
  unsigned root;
  /// Result of the operation
  double result;  
};

/// Node for BDD output graph representation.
/// Only for output via getGraph().
struct bddGraphNode
{
  unsigned id;
  unsigned var;
  unsigned low;
  unsigned high;
};

/// One Binary Decision Diagram
/// and static data structures of the whole Shared BDD package
class bddBdd : private relObject
{
private: // Constants.

  /// Identifiers of the operations in the cache (mBinCache).
  enum { mComplement = 1, mRenameVars, mExists,
         mUnite, mIntersect, mSetContains };

private: // Static attributes.

  /// Node Array.
  /// mNodes[0] is the 0-terminal, mNodes[1] is the 1-terminal.
  /// Both are always marked (mark == 1) and have the variable id -1.
  static bddNode* mNodes;
  /// Number of elements of mNodes.
  static unsigned mMaxNodeNr;
  /// Index of the first unused node in mNodes. 
  /// Unused nodes are linked using their low-element.
  static unsigned mFree;

  /// Indices of externally (i.e. by the package user) reference nodes.
  /// Updated by constructors and destructors. Does not contain terminals.
  /// Used in garbage collections to recognise live nodes.
  static multiset<unsigned> mExtRefs;

  /// Hash table of all used nodes.
  /// Used by insert to ensure that mNodes contains no two equal nodes.
  /// mUniqueHash[i] contains the index of first node of a list of all nodes 
  /// with hash value i. Lists are linked by the next-element of the nodes.
  static unsigned* mUniqueHash;
  /// Number of elements of mUniqueHash == 2^mUniqueHBitNr == 1<<mUniqueHBitNr.
  static unsigned mUniqueHBitNr;
  /// Cache for the results of binary operations.
  static bddBinEntry* mBinCache;
  /// Number of elements of mBinCache == 2^mBinCBitNr.
  static unsigned mBinCBitNr;
  /// Cache for the results of getTupleNr().
  static bddStatEntry* mStatCache;
  /// Number of elements of mStatCache == 2^mStatCBitNr.
  static unsigned mStatCBitNr;

  /// Fresh, unique cache entry operand for the next call of renameVar(). 
  ///   Whenever renameVar() is called, it changes the value of mRenameCnt.
  static unsigned mRenameCnt; // = 0;

private: // Private static methods.

  /// Hash/cache functions with 1 and 3 arguments.
  /// Possible return values: 0..(1<<hashBitNr)-1.
  static inline unsigned 
  hash(unsigned p1, unsigned pHashBitNr);
  static inline unsigned 
  hash(unsigned p1, unsigned p2, unsigned p3, unsigned pHashBitNr);
  /// Ensures that p1 <= p2.
  static inline void 
  normalize(unsigned& p1, unsigned& p2);

  /// Marks (mark=1) all nodes of the BDD with the root pRoot.
  static void 
  mark(unsigned pRoot);
  /// Unmarks (mark=0) all nodes of the BDD with the root pRoot.
  /// Terminal always remain marked.
  static void 
  unMark(unsigned pRoot);

  /// Garbage collection: All dead nodes (i.e. all nodes which are not reachable
  /// from a node in mExtRefs) are freed (i.e. inserted into unused-list mFree).
  /// Terminal nodes are never freed.
  static void 
  gc();

  /// Returns the mNodes-index of the node with the passed var-, low- and
  /// high-values. If such node does not exists, it is inserted into
  /// mNodes and mUniqueHash. If there are no free nodes left, throws exception.
  static unsigned 
  insert(unsigned pVar, unsigned pLow, unsigned pHigh);

  /// Check if there is any BDD node between the variables.
  bool
  testVars_(unsigned pRoot, 
            unsigned pVarIdFirst, 
            unsigned pVarIdLast);
  /// Returns number of represented tuples of the BDD with root pRoot.
  /// The BDD must not contain nodes with other variable ids.
  /// pVar is the first and pMaxVar is the maximum id of a variable.
  static double 
  getTupleNr_(unsigned pRoot, unsigned pVar, unsigned pMaxVar);
  /// Returns the elements pVar ... pMaxVar of an arbitrary tuple of the BDD.
  static unsigned 
  getTuple_(unsigned pRoot, unsigned pVar, unsigned pMaxVar);
  /// Returns number of nodes of the BDD with root pRoot.
  /// (Terminal nodes are not counted). Side effect: counted nodes are marked.
  static unsigned 
  getNodeNr_(unsigned pRoot);
  /// Computes the number of nodes per variable id.
  static void
  getNodesPerVarId_(unsigned pRoot, 
                    map<unsigned, unsigned>& pBddNodesPerVar);
  /// Creates output graph representation for BDD with root pRoot.
  static void
  getGraph_(unsigned pRoot, 
            multimap<unsigned,bddGraphNode>& pGraph);
  /// Prints BDD with root pRoot as reduced binary decision tree.
  static void 
  print_(ostream& pS, unsigned pRoot);
  
  /// Checks if pRoot2 represents a subset of pRoot1
  static bool 
  setContains_(unsigned pRoot1, unsigned pRoot2);

  /// Like the equally named non-static functions, 
  /// except that the static versions do not catch exceptions.
  static unsigned 
  complement_(unsigned pRoot);
  static unsigned 
  unite_(unsigned pRoot1, unsigned pRoot2);
  static unsigned 
  intersect_(unsigned pRoot1, unsigned pRoot2);
  static unsigned 
  exists_(unsigned pRoot, unsigned pVar);
  static unsigned 
  renameVars_(unsigned pRoot, 
              unsigned pFirst, 
              unsigned pLast, 
              int      pOffset);

public: // Public static methods.

  /// Initialisation of BDD package. Must be called before any other 
  /// function of the package is used.
  /// Parameters: Values for the m... variables.
  ///   number of elements of mNodes == pNodes,
  ///   number of elements of mUniqueHash == 2^pUniqueHBitNr,
  ///   number of elements of mBinCache == 2^pBinCBitNr.
  ///   number of elements of mStatCache == 2^pStatCBitNr.
  static void
  init(unsigned pMaxNodeNr, 
    unsigned pUniqueHBitNr, unsigned pBinCBitNr, unsigned pStatCBitNr);
  /// Frees memory used by the static data structures.
  /// To be called after use of the BDD package.
  static void done ();

  /// Returns overall number of live nodes (Terminal nodes are not counted).
  static unsigned 
  getReachNodeNr();
  /// Returns number of external (user) references in mExtrefs.
  static unsigned 
  getExtRefNr();

  /// Prints list lengths in mUniqueHash.
  static void 
  analyseUniqueHash ();

private: // Attributes.

  /// Index (in mNodes) of the Root node of the BDD.
  unsigned mRoot;

private: // Private methods.

  /// Removes mRoot from mExtRefs (if mRoot is no terminal).
  inline void decRef() 
  {
    if(mRoot != 0 && mRoot != 1)
    {
      multiset<unsigned>::iterator lDelete = mExtRefs.find(mRoot);
      // Otherwise error in external BDD references.
      assert(lDelete != mExtRefs.end());
      mExtRefs.erase(lDelete);
    }
  }

  /// Inserts mRoot into mExtRefs (if mRoot is no terminal).
  inline void incRef() 
  {
    if(mRoot != 0 && mRoot != 1)
    {
      mExtRefs.insert(mRoot);
    }
  }

public: // Constructors and destructor.

  /// Creates BDD with root pRoot.
  bddBdd(unsigned pRoot = 0) 
  {
    mRoot = pRoot;
    incRef();
  }

  /// Creates BDD as a copy of pBdd.
  bddBdd(const bddBdd& pBdd) 
  {
    mRoot = pBdd.mRoot;
    incRef();
  }

  /// Creates BDD that assign the bit value 'pValue' to the variable 'pVarId'.
  bddBdd (unsigned pVarId, bool pValue);
  static unsigned 
  bddBdd_(unsigned pVarId, bool pValue);

  /// Creates BDD that assign bit values of 'pValue' 
  ///   to 'pBitNr' variables beginning at position 'pVarId'.
  bddBdd (unsigned pVarId, unsigned pBitNr, unsigned pValue);
  static unsigned 
  bddBdd_(unsigned pVarId, unsigned pBitNr, unsigned pValue);

  /// Creates BDD for pVarId1 == pVarId2.
  bddBdd (unsigned pVarId1, unsigned pVarId2);
  static unsigned 
  bddBdd_(unsigned pVarId1, unsigned pVarId2);
  
  ~bddBdd() 
  {
    decRef();
  }

  /// Assignment operator.
  bddBdd& 
  operator=(const bddBdd& aBdd);

  /// Non-standard named constructor.
  /// Creates BDD for 'x <= pValue' 
  ///   for 'pBitNr' variables of 'x' beginning at position 'pVarId'.
  static bddBdd
  mkLessEqual (unsigned pVarId, unsigned pBitNr, unsigned pValue);
  static unsigned 
  mkLessEqual_(unsigned pVarId, unsigned pBitNr, unsigned pValue);

public: // Accessors.

  /// Check if there is any BDD node within the given range of var positions.
  /// Returns 'true' if any such node is found.
  bool
  testVars(unsigned pVarIdFirst, unsigned pVarIdLast)
  {
    bool lResult = testVars_(mRoot, pVarIdFirst, pVarIdLast);
    unMark(mRoot);
    return lResult;
  }

  /// Returns number of represented tuples.
  /// The BDD must not contain nodes with other variables.
  /// pMinVar is the minimum and pMaxVar is the maximum id of a variable.
  double
  getTupleNr(unsigned pMinVar, unsigned pMaxVar) const
  { return getTupleNr_(mRoot, pMinVar, pMaxVar); }

  /// Returns the elements pMinVar ... pMaxVar of an arbitrary tuple of the BDD.
  unsigned
  getTuple(unsigned pMinVar, unsigned pMaxVar) const
  {
    assert(!isEmpty());
    return getTuple_(mRoot, pMinVar, pMaxVar); 
  }

  /// Returns number of nodes (Terminal nodes are not counted).
  unsigned 
  getNodeNr() const;

  /// Returns number of free nodes.
  ///   Includes garbage collection to get real values.
  unsigned
  getFreeNodeNr() const;

  /// Returns maximal number of nodes in BDD package.
  unsigned
  getMaxNodeNr() const
  { return mMaxNodeNr; }

  /// Computes the number of nodes per variable id.
  map<unsigned, unsigned>
  getNodesPerVarId() const
  { map<unsigned, unsigned> lBddNodesPerVar;
    getNodesPerVarId_(mRoot, lBddNodesPerVar);
    unMark(mRoot);
    return lBddNodesPerVar;  }

  /// Creates output graph representation for BDD.
  void
  getGraph(multimap<unsigned,bddGraphNode>& pGraph) const 
  { getGraph_(mRoot, pGraph);
    unMark(mRoot);  }

  /// Prints BDD as reduced binary decision tree.
  void 
  print(ostream& pS) const
  { print_(pS, mRoot);
    cout << endl;  }
  
public: // Service methods.

  /// Check if the sets represented by *this and pBdd are equal.
  bool 
  setEqual(const bddBdd& pBdd) const
  { return mRoot == pBdd.mRoot; }
  
  /// Check if pBdd represents a subset of *this.
  bool 
  setContains(const bddBdd& pBdd) const
  { return setContains_(mRoot, pBdd.mRoot); }

  /// Check if the represented set is empty.
  bool 
  isEmpty() const
  { return mRoot == 0; }


  /// All of the following operations catch exceptions thrown by insert(),
  ///   call a garbage collection gc(), and call the operation again.
  /// If the second try does not work too, the program is aborted.

  /// Computes complement.
  void 
  complement();

  /// Unites with pBdd.
  void 
  unite(const bddBdd& pBdd);

  /// Intersects with pBdd.
  void 
  intersect(const bddBdd& pBdd);

  /// Existantial quantification of the variable pVar.
  void 
  exists(unsigned pVar);

  /// Rename variable ids of all nodes from pFirst to pLast
  ///   by adding pOffset to the variable ids.
  /// Precondition: pLast - pFirst has to be the same for all calls of this method,
  ///   because the cache entries contain only pFirst and pOffset.
  void 
  renameVars(unsigned pFirst, unsigned pLast, int pOffset);
};

#endif
