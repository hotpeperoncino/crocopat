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

#ifndef _bddSymTab_h_
#define _bddSymTab_h_

#include "reprNUMBER.h"
#include "relObject.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <climits>
using namespace std;


/// Symbol table for relations to support the binary encoding
///   of values and attributes.
///
/// Each attribute has to be represented by some binary variables.
///   The number of binary variables depends on the number of
///   values in the value universe (the same for all attributes).
/// 'mBitNr' is the number of binary variables, let's call them bits.
/// 'getUniverseSize()-1' is the maximal of all encoded values.
/// Cf. comments on top of class bddRelation for that.
/// 'bddSymTab' is used in the following way:
///   Initialize the symtab with a set of all the values of the universe.
///   Attributes can be added and removed. 
///   Because the number of bits has to be assigned,
///   the value universe has to stay fixed after the first 
///   use of the actual position, i.e. do not reinitialize the value set.
class bddSymTab : private relObject
{
private: // Attributes.

  /// Number of bits needed to encode the attribute ("bit width").
  /// mBitNr is the smallest integer 
  /// that is greater than log2(getUniverseSize()-1).
  unsigned mBitNr;

  /// The position of the first bit in the binary encoded tupel 
  ///   for every attribute is given by 'getAttributePos()'.
  ///   The following two maps just map the successive numbers.
  map<string, unsigned> mAttributes;
  /// Reverse of the above. Simultaneously, this represents the varorder.
  map<unsigned, string> mPositions;

  /// This symbol table handles only one value range for all attributes.
  /// Values for attributes (Nominal scale to make the range 'dense').
  ///   It maps the name of a value within the application (string)
  ///   to a number for internal representation of the value.
  map<string, unsigned> mAttributeNumbers;
  /// And the reverse mapping for output.
  vector<string> mAttributeValues;

  /// The set of quoted values (in the RSF input file).
  set<string> mIsQuoted;

private:
  /// It should not be allowed to use standard operators.
  void operator,(const bddSymTab&);
  void operator=(const bddSymTab&);
  bddSymTab(const bddSymTab&);

public: // Constructors.
  
  bddSymTab()
  {}

  ~bddSymTab()
  {}  
  

public: // Initializers.

  /// See comment on top of this class.
  ///   Assign the bit encodings.
  void
  initValueUniverse(set<string>& pValueUniverse) 
  {
    if (pValueUniverse.size() <= 1) {
      mBitNr = 1;        // We need at least one bit for technical reasons.
    } 
    else {
      // Compute bit width (mBitNr).
      mBitNr = 0;
      unsigned lMaxValueCopy = pValueUniverse.size() - 1;
      while (lMaxValueCopy > 0)
      {
        lMaxValueCopy /= 2;
        ++mBitNr;
      }
    }
    
    // Prepare value-encoding mappings. Sort.
    mAttributeValues.clear();
    for( set<string>::const_iterator
           lIt = pValueUniverse.begin();
         lIt != pValueUniverse.end();
         ++lIt)
    {
      mAttributeNumbers[*lIt] = mAttributeValues.size();
      mAttributeValues.push_back(*lIt);
    }   
  }
  

public: // Accessors.

  /// See comment on top of this class.
  unsigned
  getBitNr() const 
  { return mBitNr; }

  /// See comment on top of this class.
  unsigned
  getUniverseSize() const
  { return mAttributeValues.size(); }

  void
  setQuoted(const string pAttribute)
  { mIsQuoted.insert(pAttribute); }

  bool
  isQuoted(const string pAttribute) const
  { return mIsQuoted.find(pAttribute) != mIsQuoted.end(); }

  /// Checks if 'pAttributeValue' exists in symtab.
  bool
  isValueGood(const string& pAttributeValue) const
  {
    return mAttributeNumbers.find(pAttributeValue) != mAttributeNumbers.end();
  }

  unsigned
  getAttributePos(const string& pAttribute) const
  {
    map<string, unsigned>::const_iterator it = mAttributes.find(pAttribute);
    // Otherwise access for invalid attribute.
    assert(it != mAttributes.end());
    return it->second * getBitNr();
  }

  /// Return the number of value (pAttributeValue), 
  ///   i.e. its internal representation.
  unsigned
  getValueNum(const string& pAttributeValue) const
  {
    map<string, unsigned>::const_iterator
      lIt = mAttributeNumbers.find(pAttributeValue);
    // Value not found in symbol table.
    assert(lIt != mAttributeNumbers.end());
    return lIt->second;
  }

  string
  getAttributeValue(unsigned pNum) const
  {
    assert(pNum < mAttributeValues.size());
    return mAttributeValues[pNum];
  }

public: // Service methods.
  
  /// The method changes this symbol table!
  ///   Add attribute 'pAttribute' to SymTab.
  void
  addAttribute(const string& pAttribute)
  {
    assert(pAttribute != "");           // Avoid empty string as name.
    // Add only if new.
    if( mAttributes.find(pAttribute) == mAttributes.end() ) {
      unsigned lAttrNum = mAttributes.size();
      // Ensure that lAttrNum is new.
      while (mPositions.find(lAttrNum) != mPositions.end()) {
        if (lAttrNum < UINT_MAX) {
          ++lAttrNum;
        } else {
          cerr << "Error: Maximum number of attributes exceeded." 
               << endl;
          exit(EXIT_FAILURE);
        }
      }
      mAttributes[pAttribute] = lAttrNum;
      mPositions[lAttrNum] = pAttribute;
    }
  }

  /// This method changes this symbol table!
  /// Remove attribute 'pAttribute' from SymTab.
  void
  removeAttribute(const string& pAttribute) 
  {
    assert(mAttributes.find(pAttribute) != mAttributes.end());
    mPositions.erase(mAttributes[pAttribute]);
    mAttributes.erase(pAttribute);
  }

  /// This method changes this symbol table!
  /// Remove all attributes from SymTab
  ///   which do not start with 'pAttributePrefix'.
  void
  removeUserAttributes(const char pAttributePrefix)
  {
    vector<string> lToRemove;
    // Collect attributes to remove.
    for(map<unsigned, string>::const_iterator lIt = mPositions.begin();
        lIt != mPositions.end();
        ++lIt)
    {
      if( (lIt->second)[0] != pAttributePrefix ) 
      {
        lToRemove.push_back(lIt->second);
      }
    }
    // Actually remove the collected attributes.
    for(vector<string>::const_iterator lIt = lToRemove.begin();
        lIt != lToRemove.end();
        ++lIt)
    {
      removeAttribute(*lIt);
    }
  }  

  const map<unsigned,string>
  computeVariableOrder(const set<string>& pAttributes) const
  { 
    map<unsigned,string> result;
    for (set<string>::const_iterator it = pAttributes.begin();  
         it != pAttributes.end();  
         ++it) 
    {
      result[getAttributePos(*it)] = *it;
    }
    return result;
  }
  
public: // IO.

  void printValueNames(ostream& pS) const
  {
    pS << "VALUES -> NUMBERS :" << endl
       << "{" << endl;
    for (map<string, unsigned>::const_iterator 
         lValueIt = mAttributeNumbers.begin();
         lValueIt != mAttributeNumbers.end();
         ++lValueIt)
    {
      pS << "(\"" << lValueIt->first << "\" -> " 
         << lValueIt->second << "), ";
    }
    pS << "}" << endl;

    // Reverse mapping.
    pS << "NUMBERS -> VALUES :" << endl
       << "{" << endl;
    for (vector<string>::const_iterator 
           lValueIt = mAttributeValues.begin();
           lValueIt != mAttributeValues.end();
           ++lValueIt)
    {
      pS << "(" << lValueIt - mAttributeValues.begin() << " -> \""
         << *lValueIt << "\"), ";
    }
    pS << "}" << endl;
  }

};

#endif // _bddSymTab_h_
