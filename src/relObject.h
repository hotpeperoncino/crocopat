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

#ifndef _relObject_h
#define _relObject_h

#include <iostream>
using namespace std;

/// The purpose of this class is to count the number
///   of objects instantiated within the CrocoPat library.
/// This is useful for checking memory deallocation.
class relObject
{
  // Static Methods.
public:
  // Accessor methods.
  static const int GetCount()
  {
    return(count);
  }

private:
  /// Counts the number of instatiated objects.
  static long count;

  /// It should be not allowed to use the standard operators.
  /// Uses the standard operator '='.
  void operator,(const relObject&);

public:
  // Constructor and destructor.
  relObject()
  {
  	// We count the number of relObjects.
    ++count;
  }
  
  /// We define a copy constructor for the case that
  ///   an implicitly defined copy constructor of
  ///   a subclass is used.
  relObject(const relObject& pObj)
  {
  	// We count the number of relObjects.
    ++count;
  }
  
  /// This virtual destructor forces that all derivated components
  /// also have virtual destructors without virtual declaration.
  virtual ~relObject()
  {
    --count;
  }
};

ostream& operator << (ostream& out, const relObject* obj);

#endif
