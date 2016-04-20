#!/bin/sh
#
# ./run-wcre.sh 2>& run-wcre_2008-02-1X.log

CROCOPAT=../src/crocopat

${CROCOPAT} -v | head -4
echo
echo Some system information:
echo uname -a: `uname -a`
echo OS: $OS
echo OSTYPE: $OSTYPE
echo

echo CrocoPat requires the following dynamic libraries:
objdump -p ${CROCOPAT} | grep dll
ldd ${CROCOPAT}
echo

projects="\
JHotDraw52 \
JDK140AWT \
JWAM16FullAndreas \
jdk14v2 \
Eclipse202a \
"


patterns="\
Composite \
ClassesInCycles \
Cycles \
SimilarClassesSort \
InhDegenerate \
SubclassKnowledge \
Closure \
"

for pat in $patterns 
do
    echo "=================================================="
    echo "    Processing pattern $pat ..."
    for proj in $projects 
    do
    echo "----------------------------------------------"
    echo "Processing project $proj ..."
	/bin/time -f "%e real,\t%U user,\t%S sys,\t%M max resid set size" ${CROCOPAT} programs/${pat}.rml < projects/${proj}.rsf > projects/${proj}_${pat}.rsf
	# grok saves using append-to-file.
##	rm projects/${proj}/${pat}_grok.rsf
##	/home/db/memtime_linux -m 409600 grok queries/$pat.grk projects/$proj/1Relations.rsf projects/${proj}/${pat}_grok.rsf
##    mv projects/${proj}/${pat}.rsf projects/${proj}/${pat}_old.rsf
    done;
done;
