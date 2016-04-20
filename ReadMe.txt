// CrocoPat is a tool for relational querying.
// This file is part of CrocoPat. 

// Copyright (C) 2002-2008  Dirk Beyer


----------------------------------------------------------------------------

New in release 2.1.5, ???

- Redesign of the syntax tree, in order to make input/output more flexible.
- In addition to C-like comments using // and /* ... */,
  Shell-like comments using # at the beginning of a line are now supported.

----------------------------------------------------------------------------

New in release 2.1.4, 2008-02-15

- The new relational expression 'TUPLEOF(rel_expr)' returns
  the lexicographically smallest tuple (as relation with one element)
  in the result of rel_expr.
  The result of rel_expr must be nonempty.
- The new numerical expression 'ROUND(num_expr)' returns the
  integer number closest to the result of num_expr. 
- Some error messages improved.
- Bug fix in a user message about the number of tuples in a relation.

----------------------------------------------------------------------------

New in release 2.1.3, 2005-10-11

- Makefile modification for MacOS, contributed by Derek Rayside (MIT).
- Small addition to GraphViz output of BDD graphs.
- Fixed bug in usage of STL interface.

----------------------------------------------------------------------------

New features in release 2.1.2, 2005-02-12

- The new numerical expression 'ELAPSED' returns the processor time
  (in seconds) elapsed since the previous interpretation of 'ELAPSED'
  or (in the first interpretation of 'ELAPSED') since the program start.
- The new string expression 'ELEMENT(rel_expr)' returns the lexicographically
  smallest element in the result of rel_expr.  The result of rel_expr
  must be nonempty and rel_expr must have exactly one free attribute.
- Improved performance for the cardinality operator # and 
  the built-in lexicographical order relations (=, <, >, etc).
- Improved performance for regular expression matching.
- Identifiers for relation variables can contain arbitrary characters 
  (except ') if quoted: '...'. In particular useful for processing RDF data.
- Output of BDDs:
  - NODESPERVAR(rel_expr)
    prints the number of nodes
    for each boolean variable in the BDD for rel_expr.
    For BDD shape visualization with VisuBDD (see below).
  - GRAPH(rel_expr)
    prints the graph for the BDD for rel_expr in the GraphViz format.
    For BDD visualization with GraphViz (http://www.graphviz.org/).
  - BDT(rel_expr)
    prints a binary decision tree for the BDD for rel_expr.
- Java program VisuBDD, which visualizes the shape of a BDD,
  using the output of 'PRINT GRAPH(rel_expr)'.

----------------------------------------------------------------------------

New features in release 2.1.3, 2005-10-11

- Only maintenance activities (make it available for MacOS, bug fixes).

