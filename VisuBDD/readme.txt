// CrocoPat is a tool for relational querying.
// This file is part of CrocoPat. 

// Copyright (C) 2002-2008  Dirk Beyer


----------------------------------------------------------------------------

VisuBDD - Visualization of (size and shape of) BDDs.

Compile tool:
javac *.java

Run tool (from the parent directory of VisuBDD):
java -classpath . VisuBDD.VisualizeSize inputfile1 [inputfile2] ...
e.g.: java -classpath . VisuBDD.VisualizeSize VisuBDD/test1_AND4.dat
Requires JDK 1.3 or higher.


Reads files which are produced by CrocoPat with the command
PRINT NODESPERVAR( rel_expr );
Every number in the output of this command is the number of BDD nodes
for one particular Boolean variable (one level) within the BDD.

Produces a picture of the BDD shape which consists of centered horizontal
lines, one line for each number in the input file.
The length of a line is proportional to the corresponding number of BDD nodes.
If the maximal number of BDD nodes is greater than the width of the window
(in pixels), then all lines are shrinked with a constant factor to fit.
So, one pixel in the picture represents a fixed number of BDD nodes,
and the shape of the picture may help to understand the BDD representation.

If the command line provides more than one input file,
the program layers the corresponding BDD shape pictures with different colors.
It is also possible to provide more than one BDD shape in a single input file,
by separating the BDD shapes with a line consisting of raute '#'.

Preferences:
- Maximum BDD Width:
  Increase this number to further shrink the width of the shape,
  decrease this number to stretch the shape horizontally.
- Vertical Stretching:
  Thickness of each horizontal line (in pixels).
- Combine Colors:
  Combine the colors of two overlapping shapes to see both shapes.
