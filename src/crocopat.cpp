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
#include "relStatement.h"
#include <FlexLexer.h>
#include "relReaderWriter.h"

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <csignal>
#include <unistd.h>
#include <time.h>
#include <cstring>
using namespace std;

/// Global function.
extern int yyparse(void);

/// Global variables for scanner, parser, interpreter..
yyFlexLexer               gScanner;
int                       gNoParseErrs       = 0;
string*                   gLibFileName       = NULL;
bddSymTab*                gSymTab            = NULL;  // For parser+relExpression.
relStatement*             gSyntaxTree        = NULL;
map<string, relDataType*> gVariables;
map<string, relStatement*> gProcedures;
bool                      gPrintWarnings     = true;

/// Global variable for BDD init, RSF reader, symtab init.
const unsigned gRSFLineLength   = 100000; // Default: 100000 chars per RSF line max.
const char*    gRSFDelimiter    = " \t\f\r\v\n";
char*          gReadPos         =   NULL; // Reading position for input data.
const unsigned gAttributeNum    =   1000; // Default:   1000 internal attributes.
const char     gAttributePrefix =    '.'; // Prefix for internal attributes.
set<string>    gValueUniverse;

/// Time measurement.
clock_t gStart = 0;

/// Cmd line arg handling for the interpreter.
char**  gArgv;
int     gArgc;


//////////////////////////////////////////////////////////////////////////////
double 
string2double(string pStr)
{
  return atof(pStr.c_str());
}

//////////////////////////////////////////////////////////////////////////////
string 
double2string(double pNum)
{
  string result;
  stringstream lStringStream;
  lStringStream << pNum;
  lStringStream >> result;
  return result;
}

//////////////////////////////////////////////////////////////////////////////
string 
unsigned2string(unsigned pUnsigned)
{
  string result;
  stringstream lStringStream;
  lStringStream << pUnsigned;
  lStringStream >> result;
  return result;
}

//////////////////////////////////////////////////////////////////////////////
/// Measure elapsed processor time.
double elapsed() {
  clock_t lNow = clock();
  double lElapsed = ((double) (lNow - gStart)) / CLOCKS_PER_SEC;
  gStart = lNow;
  return lElapsed;
}

//////////////////////////////////////////////////////////////////////////////
/// Signal handler.  'pSignal' is the signal number to handle.
/// Will install this signal handler for keyboard interrupt (C-c).
void 
signal_handler(int pSignal)
{
  cerr << "\nSignal 'program interrupt' (SIGINT) received." << endl
       << "Terminating the program." << endl;
  exit(EXIT_FAILURE);
}

//////////////////////////////////////////////////////////////////////////////
/// Reads a next value into 'pValue', 
///   uses global configuration set by file2vector (gReadPos).
///   Returns false, if no more values available at current line.
bool
readValue(string& pValue, int pLineNo)
{
  pValue = "";
  char* lEntry = NULL;
  bool inString = false;   // 'true' if we have seen the opening '"'.
  do {
    lEntry = strtok(gReadPos, gRSFDelimiter); // Read entry.
    gReadPos = NULL;                          // Use stored position next time.
    if (lEntry == NULL) {                     // No more values at this line.
      if (inString && gPrintWarnings) {
        cerr << "Warning: RSF reader warning at line " << pLineNo
             << ": Closing double quote for string missing." << endl;
      }
      return false;
    }

    if (inString) {
      pValue += ' ' + string(lEntry);         // Append.
    } else {
      pValue = string(lEntry);                // New.
    }
    if (inString) {
      // If the last char is the double quote.
      if (pValue[pValue.length()-1] == '"') {
        if (pValue[pValue.length()-2] != '\\') {
          inString = false;                     // Finish string scanning.
        }
      }
    } else {
      // If the first char is the double quote, and
      //   if size > 1  then  last char is not the double quote.
      if ( pValue[0] == '"' &&
           (!(pValue.size() > 1) || pValue[pValue.length()-1] != '"') ) {
        inString = true;                      // Start string scanning.
      } 
    }
  } while (inString);
  return true;
}

//////////////////////////////////////////////////////////////////////////////
/// Reads a relation from a stream into a vector.
void
file2vector(istream& pDataInStream, vector< vector<string> >& pRelationVector)
{
  char* lLine = new char[gRSFLineLength];
  while (pDataInStream.good()) {
    pDataInStream.getline(lLine, gRSFLineLength);
    const int lLineLength = strlen(lLine);
    if (lLine[0] == '.') {                      // '.' found -> EOF.
      delete lLine;
      return;
    }
    vector<string> lRow;
    if (lLine[0] == '#') {                      // '#' found -> comment.
      pRelationVector.push_back(lRow);          // Add empty row.
      continue;
    }
    if (lLineLength == gRSFLineLength-1) {
      cerr << "Error: RSF reader error at line " << pRelationVector.size() + 1 
           << ": Maximum line length exceeded." << endl;
      exit(EXIT_FAILURE);
    }
    gReadPos = lLine;
    string lValue;
    while (readValue(lValue, pRelationVector.size() + 1))
    {
      if (lValue[0] == '"'  &&  lValue[lValue.length()-1] == '"') {
        // Cut the quotes ('"') at begin and end of the string.
        lValue = string(lValue, 1, lValue.length() - 2);
        // Remember that it was quoted (for output of RSF relations).
        gSymTab->setQuoted(lValue);
      }
      if (lRow.size() == 0) {
        // (lRow.size() == 0) -> (lValue) is the name of the relation.
        map<string, relDataType*>::const_iterator 
          lVarIt = gVariables.find(lValue);
        if (lVarIt == gVariables.end()) {  // New relation.
          gVariables[lValue] = new bddRelation(gSymTab, false);
        }
      } else {
        // (lRow.size() > 0) -> Add value to value universe.
        gValueUniverse.insert(lValue);
      }
      // Add value to row vector.
      lRow.push_back(lValue);
    }
    pRelationVector.push_back(lRow);     // Add empty rows, too.
  }
  delete lLine;
  return;
}

//////////////////////////////////////////////////////////////////////////////
/// Create BDD representation for relation.
/// Parser already added symbols to the symbol table.
/// Load all relations and assign them to relation variables.
/// (pRelation) contains a set of triples of form (rel, x, y).
/// (rel) is the name of the relation (variable name),
///   (x, y) is the pair of the binary relation.
void 
createBddRelation(vector< vector<string> >& pRelation)
{
  // Insert relation tuples to BDD.
  for( unsigned i = 0; i < pRelation.size(); ++i )
  {
    if( pRelation[i].size() != 0 ) {    // Skip empty rows.
      // Find the BDD for the current relation variable with name (*it)[0].
      // Unite all tuples to the current relation.
      string lVar = pRelation[i][0];
      map<string, relDataType*>::const_iterator lVarIt = gVariables.find(lVar);
      assert(lVarIt != gVariables.end());  // Must be declared.
      assert(lVarIt->second != NULL);
      bddRelation* lResult = dynamic_cast<bddRelation*>(lVarIt->second);
      if (lResult == NULL) {
        cerr << "Error: RSF reader error at line " << i+1 << ":" << endl
             << "'" << lVarIt->first 
             << "' is a predefined standard variable." << endl;
        exit(EXIT_FAILURE);
      }

      // Some type checking: Track and check arity.
      int lArity = pRelation[i].size()-1;
      if (lResult->mArity == -1) {
      	lResult->mArity = lArity;
      } else {
        if (lResult->mArity != lArity) {
          if (gPrintWarnings) {
            cerr << "Warning: RSF reader warning at line " << i+1 << ": Arity mismatch." << endl
                 << "'" << lVar << "' was initialized with arity " << lResult->mArity 
                 << " but now gets a tuple of arity " << lArity << "." << endl;
          }
        }
      }

      // Build the tuple-BDD for one row of the input file.
      bddRelation bddTuple(gSymTab, true);
      // First position of the tuple is the relation name, don't include.
      for( unsigned j = 1; j < pRelation[i].size(); ++j)
      {
        if( j > gAttributeNum ) {
          cerr << "RSF reader error at line " << i+1 
               << ": Maximum arity (" << gAttributeNum << ") exceeded." << endl;
          exit(EXIT_FAILURE);
        }
        bddTuple.intersect( 
                           bddRelation::mkAttributeValue(gSymTab,
                                                         gAttributePrefix + unsigned2string(j-1),
                                                         pRelation[i][j] )
                           );
      }
      lResult->unite(bddTuple);
    }
  }
}

////////////////////////////////////////////////////////////////////////  
#define STRINGIFY(x) #x
#define EXPAND(x) STRINGIFY(x)
void
printVersion(void) 
{
  cout << "CrocoPat 2.1.4, 2008-02-15 "
       << "(Rev " << EXPAND(REVISION) << ", Build " << EXPAND(BUILDTIME) << ")." << endl
       << "Copyright (C) 2002-2008  Dirk Beyer (Simon Fraser University)." << endl
       << "CrocoPat is free software, released under the GNU LGPL." << endl;
}

////////////////////////////////////////////////////////////////////////  
/// Print usage and info message.
void
printHelp(void) 
{
  cout << endl
       << "This is CrocoPat, a tool for relational programming." << endl
       << endl
       << "Usage: crocopat [OPTION]... FILE [ARGUMENT]..." << endl
       << "Execute RML (Relation Manipulation Language) program FILE." << endl
       << "ARGUMENTs are passed to the RML program." << endl
       << "Options:" << endl
       << "  -e           do not read RSF data from stdin." << endl
       << "  -h           display this help message and exit." << endl
       << "  -l FILE      use library file FILE." << endl
       << "  -m NUMBER    approximate memory for BDD package in MB (default 50)." << endl
       << "  -q           quiet mode, supress warnings." << endl
       << "  -v           print version information and exit." << endl
       << endl
       << "Input data are read from stdin, unless option -e is given." << endl
       << endl
       << "http://www.cs.sfu.ca/~dbeyer/CrocoPat/" << endl 
       << endl
       << "Report bugs to Dirk Beyer." << endl
       << endl;
}

////////////////////////////////////////////////////////////////////////  
/// Main program.
int 
crocopat(int argc, char *argv []) 
{
  // Time measurement.
  gStart = clock();

  // Initialize input stream. Default: stdin.
  istream* gDataInStream = &cin;

  // Initial value for BDD pkg size.
  int gBddPkgSizeMB = 50;   // Default: 50 MB. Changed by cmd line option.

  // Handle command line options.
  int c;
  while ( (c = getopt(argc, argv, "ehl:m:qv")) != -1 ) {
    switch (c) {
    case 'e':
      // No input data.
      gDataInStream = NULL;
      break;
    case 'h':
      printHelp();
      exit(EXIT_SUCCESS);
    case 'l':
      // Library file needs to be parsed.
      gLibFileName = new string(optarg);
      break;
    case 'm':
      // Memory size for BDD package.
      gBddPkgSizeMB = atoi(optarg);
      assert(gBddPkgSizeMB > 0);
      break;
    case 'q':   
      gPrintWarnings = false;
      break;
    case 'v':   
      printVersion();
      exit(EXIT_SUCCESS);
    }
  }

  
  // If no program file is given, print help text.
  if (optind >= argc) {
    printHelp();
    exit(EXIT_SUCCESS);
  }
  // Initialize program input stream.
  ifstream lProgramStream(argv[optind], ios::in);
  if ( !lProgramStream.good() ) {
    cerr << "Error: Cannot open program file '" << argv[optind] << "'." << endl;
    exit(EXIT_FAILURE);
  }
  
  // Ignore the rest of the command line now,
  //   may be the arguments are used by the program.
  // User should access argv[optind+1] by number 1.
  // Initialize global variables for access by the interpreter.
  gArgv = argv + optind;
  gArgc = argc - optind;

  // Install the signal handler for keyboard interrupt (C-c).
  signal(SIGINT, &signal_handler);

  // Initialize symbol table with (gAttributeNum) internal attributes.
  {
    gSymTab = new bddSymTab();
    // Add internal variables to symtab.
    //   '.X0', ..., '.Xn'.
    for( unsigned i = 0; i < gAttributeNum; ++i) {
      gSymTab->addAttribute( gAttributePrefix + unsigned2string(i) ); 
    }
  }
  // Initialize BDD package.
  {
    // Default: ca. 2000000 BDD nodes (50 MB);
    //unsigned lNrNodes  = gBddPkgSizeMB * (1024 * 1024) / 36;
    unsigned lNrNodes  = gBddPkgSizeMB * 30000;
    unsigned lHashSize = (unsigned) ( log((float)lNrNodes) / log(2.0) );
    // Initialize BDD package.
    bddBdd::init(lNrNodes, lHashSize, lHashSize, lHashSize - 4);
  }
  // Declare predefined (internal or constant) variables.
  {
    // Variables for exit status and number of arguments.
    gVariables["exitStatus"]  = new relNumber(0);
    gVariables["argCount"]    = new relNumber(gArgc);

    // Declare predefined relations. Constant, cannot be changed by user.
    gVariables["TRUE"]  = new bddRelationConst(bddRelation(gSymTab, true));
    gVariables["FALSE"] = new bddRelationConst(bddRelation(gSymTab, false));
    // These are initialized later.
    gVariables["="]     = new bddRelationConst(bddRelation(gSymTab, false));
    gVariables["!="]    = new bddRelationConst(bddRelation(gSymTab, false));
    gVariables["<"]     = new bddRelationConst(bddRelation(gSymTab, false));
    gVariables["<="]    = new bddRelationConst(bddRelation(gSymTab, false));
    gVariables[">"]     = new bddRelationConst(bddRelation(gSymTab, false));
    gVariables[">="]    = new bddRelationConst(bddRelation(gSymTab, false));
  }
  

  {
    vector< vector<string> > lRelationVector;

    // Read relation from data input stream.
    if (gDataInStream != NULL) {  // NULL pointer means don't read any input data.
      file2vector(*gDataInStream, lRelationVector);
    }


    // Read syntax tree (program parser).
    {
      // First read procedures from library, if necessary.
      if (gLibFileName != NULL) {
        // Initialize library input stream.
        ifstream lLibraryStream(gLibFileName->c_str(), ios::in);
        if ( !lLibraryStream.good() ) {
          cerr << "Error: Cannot open library file '" << *gLibFileName << "'." << endl;
          exit(EXIT_FAILURE);
        }
        // Initialize scanner input.
        gScanner.yyrestart( &lLibraryStream );
        // Parse error?
        if( yyparse() || gNoParseErrs > 0) {
          exit(EXIT_FAILURE);
        }
        lLibraryStream.close();
        // Clean up.
        delete gLibFileName;
        gLibFileName = NULL;
        // Free the library syntax tree; the parsed procedures are stored in gProcedures.
        delete gSyntaxTree;
      }    	
    	
      // Initialize scanner input.
      gScanner.yyrestart( &lProgramStream );
      // Parse error?
      if( yyparse() || gNoParseErrs > 0) {
        exit(EXIT_FAILURE);
      }
      lProgramStream.close();
    }
    // Now the syntax tree is contained in 'gSyntaxTree'.


    // Now we prepare the symbol table to support real BDD operations.
    gSymTab->initValueUniverse(gValueUniverse);

    // Transform relation from vector to BDD representation.
    createBddRelation(lRelationVector);
    // Now all relations are assigned to relational variables.
  }


  // Initialize predefined constant relations.
  {
    delete gVariables["="];
    delete gVariables["!="];
    delete gVariables["<"];
    delete gVariables["<="];
    delete gVariables[">"];
    delete gVariables[">="];

    bddRelation
    lRelEqual = bddRelation::mkEqual(gSymTab, 
                                     gAttributePrefix + unsigned2string(0),
                                     gAttributePrefix + unsigned2string(1));
    gVariables["="]     = new bddRelationConst(lRelEqual);

    bddRelation 
    lRelTmp = lRelEqual;
    lRelTmp.complement();
    gVariables["!="]     = new bddRelationConst(lRelTmp);

    bddRelation lRelLess  = bddRelation::mkLess(gSymTab, 
                                                gAttributePrefix + unsigned2string(0),
                                                gAttributePrefix + unsigned2string(1));
    gVariables["<"]     = new bddRelationConst(lRelLess);

    lRelTmp = lRelLess;
    lRelTmp.unite(lRelEqual);
    gVariables["<="]    = new bddRelationConst(lRelTmp);

    lRelTmp.complement();
    gVariables[">"]     = new bddRelationConst(lRelTmp);

    lRelTmp.unite(lRelEqual);
    gVariables[">="]    = new bddRelationConst(lRelTmp);
  }

  // Interpret syntax tree.
  gSyntaxTree->interpret(gSymTab);


  // Free memory.
  {
    // Already taken care of.
    assert(gLibFileName == NULL);

    // Free procedures.
    for( map<string, relStatement*>::iterator 
           lIt = gProcedures.begin();
         lIt != gProcedures.end();
         ++lIt)
    {
      delete lIt->second;
    }
    gProcedures.clear();

    // Free relation variables.
    for( map<string, relDataType*>::iterator 
           lIt = gVariables.begin();
         lIt != gVariables.end();
         ++lIt)
    {
      delete lIt->second;
    }
    gVariables.clear();
    
    // Free syntax tree.
    delete gSyntaxTree;
    
    // Uninitialize BDD storage.
    bddBdd::done();

    // Free symbol table.
    delete gSymTab;
  }

  // For controlling deallocation.
  if( relObject::GetCount() != 0 )
  {
    cerr << "######################################################" << endl
         << "Internal error: We still have " << relObject::GetCount()
         << " relObjects after deallocation (should be 0)." << endl;
  }
  
  return(EXIT_SUCCESS);
};

//////////////////////////////////////////////////////////////////////////////
  
