#!/bin/bash
# this has to be /bin/bash, because of $[ 1+2 ] syntax


# ---------------------------------------------------------
# Adapted from a shell script used in CIL and BLAST

# default values for user parameters
skip=0
contin=0
perf=0
#CROCOPAT="time ../src/crocopat"
CROCOPAT="../src/crocopat"
block=""

# counters
curtest=0
success=0
failure=0
unexSuccess=0
unexFailure=0
dies=0

usage() {
cat <<EOF
usage: $0 [options]
  -skip n      skip the first n tests
  -perf	       do performance evaluation
  -contin      keep going even after a test fails (or succeeds) unexpectedly
  -extra str   add "str" to every CrocoPat command
  -help        print this message
  -block s     run the tests in block s (default "all")
  -all         run all the tests
EOF
}

# process args
while [ "$1" != "" ]; do
  case "$1" in
    -skip)
      shift
      skip="$1"
      ;;

    -perf)
      perf=1
      ;;

    -contin)
      contin=1
      ;;

    -extra)       
      shift
      CROCOPAT="$CROCOPAT $1"
      ;;

    -help)
      usage
      exit 0
      ;;

    -block)
      shift
      block="$block$1"
     ;;
     
    -all)
     block="all"
     ;;
     
    *)
      echo "unknown arg: $1"
      usage
      exit 2
      ;;
  esac

  shift
done

# clear the logfile
log=regrtest.log
stat=regrtest.stat
rm -f *.log # we clear all logfiles from individual tests as well
rm -f regrtest.stat
# write something to terminal and log
log() {
  echo "$@"
  echo "$@" >> $log
}

#takes  no args -- just does a bunch of greps on the test_*.log file
getstats() {
echo "getting stats"
echo "[$curtest]" >> $stat 
echo `grep "real:" test_${curtest}_full.log` >> $stat
echo `grep "user:" test_${curtest}_full.log` >> $stat
echo "" >> $stat
echo "" >> $stat
}


# bail, unless asked to continue
bail() {
  if [ $contin -eq 0 ]; then
    exit 2
  fi
}

# run a single test, and bail if it fails
runTest() {
  if ! runTestInternal "$@"; then
    bail
  fi
}

# run a single test, and return 0 if it succeeds
runTestInternal() {
  #result=0
  rm -f test_${curtest}*.log
  if [ "$curtest" -lt "$skip" ]; then
    echo "[$curtest]: skipping $*"
  else
    # print a visually distinct banner
    echo "------------ [$curtest] $* ------------"

    "$@" > test_${curtest}_full.log 2>&1
    #result=$?
    if grep "Error:" test_${curtest}_full.log ; then
      unexFailure=$[ $unexFailure + 1 ]
      echo ""
      log  "[$curtest] A regression test command failed:"
      log  "  $*"
      tail -200 test_${curtest}_full.log >test_${curtest}.log
      rm test_${curtest}_full.log
    else
      if grep ":-)" test_${curtest}_full.log ; then
	log "[$curtest] $@ succeeds"
        success=$[ $success + 1 ]
	getstats
        rm test_${curtest}_full.log
        if [ "$perf" -eq 1 ]; then 
          echo "Now running performace tests"
          rm -f tmp
#          for n in 1 2 3 4 5; do
          for n in 1 ; do
            if (time "$@" >test_${curtest}_${n}.log 2>&1) 2>times.out; then
              cat times.out | grep real | sed 's/real	0m/    /' \
                        | sed 's/s$//' | tee -a tmp
              rm times.out test_${curtest}_${n}.log
            else
              echo "Run $n of $@ failed."
              exit 4
            fi
         done

            # games with awk are to make sure sorting happens properly even when
            # the input times don't all have same # of digits (e.g. 9s vs 10s)
         log "    median:"`awk '{ printf("%9.3fs\n", $1); }' <tmp | sort | head -3 | tail -1`
         rm tmp
        fi
      else
        echo ""
	log "[$curtest] $@ Unexpected error"
	tail -200 test_${curtest}_full.log >test_${curtest}.log
	rm test_${curtest}_full.log
	dies=$[ $dies + 1]
      fi
    fi


  fi

  curtest=$[ $curtest + 1 ]
  return $result
}


# run a test that is expected to fail
failTest() {
  reason="$*"
  #shift
  rm -f test_${curtest}*.log
  if [ "$curtest" -lt "$skip" ]; then
    echo "[$curtest]: (fail) skipping $*"
  else
    echo "------------ [$curtest] (fail) $* ------------"
    "$@" > test_${curtest}_full.log 2>&1
    if grep ":-)" test_${curtest}_full.log; then
      unexSuccess=$[ $unexSuccess + 1 ]
      echo ""
      log  "[$curtest] BAD NEWS: A regression test that should fail ($reason) now succeeds:"
      log  "  $*"
      tail -200 test_${curtest}_full.log >test_${curtest}.log
      rm test_${curtest}_full.log
      if [ $contin = 0 ]; then
        exit 2
      fi
    else
      if grep "Error:" test_${curtest}_full.log; then
        failure=$[ $failure + 1 ]
        echo "Failed as expected: $reason"
	log "[$curtest] $@ fails as expected"
	getstats
        rm -f test_${curtest}_full.log

        if [ "$perf" -eq 1 ]; then 
          echo "Now running performance tests"
          rm -f tmp
          for n in 1 2 3 4 5; do
            if (time "$@" >test_${curtest}_${n}.log 2>&1) 2>times.out; then
              cat times.out | grep real | sed 's/real	0m/    /' \
                        | sed 's/s$//' | tee -a tmp
              rm times.out test_${curtest}_${n}.log
            else
              echo "Run $n of $@ failed."
              exit 4
            fi
         done

            # games with awk are to make sure sorting happens properly even when
            # the input times don't all have same # of digits (e.g. 9s vs 10s)
         log "    median:"`awk '{ printf("%9.3fs\n", $1); }' <tmp | sort | head -3 | tail -1`
         rm tmp
        fi
      else
        echo "Unexpected error"
        log "[$curtest] $@ Unexpected error"
	tail -200 test_${curtest}_full.log >test_${curtest}.log
	rm test_${curtest}_full.log
	dies=$[ $dies + 1]
      fi
    fi
  fi

  curtest=$[ $curtest + 1 ]
}


rw="-rw"

# self-contained tests of specific examples 

# easy microbenchmarks
blockMICRO(){
runTest $CROCOPAT -e -m 1 variable_init.rml
runTest $CROCOPAT -e -m 1 arity_check.rml
failTest $CROCOPAT -e -m 1 arity_check_fail.rml
runTest $CROCOPAT -m 1 arity_check_RSF.rml < arity_check_RSF.rsf
failTest $CROCOPAT -m 1 arity_check_RSF.rml < arity_check_RSF_fail.rsf
failTest $CROCOPAT -e -m 1 proc_redecl.rml
runTest $CROCOPAT -e -m 1 proc_call.rml
failTest $CROCOPAT -e -m 1 proc_call_lib.rml
runTest $CROCOPAT -e -m 1 -l proc_lib.rml proc_call_lib.rml
failTest $CROCOPAT -e -m 1 -l proc_lib.rml proc_call_libXXXX.rml
failTest $CROCOPAT -e -m 1 -l proc_libXXXX.rml proc_call_lib.rml
runTest $CROCOPAT -e -m 1 comment.rml

}


 
runAllTests(){

    blockMICRO
    
}

runShortRegress(){

    blockMICRO
    
}


# process args
#while [ "$block" != "" ]; do
  echo $block
  case "$block" in
      all)
	  shift
	  blockMICRO
	  ;;
      
      MICRO)
	  shift
	  blockMICRO
	  ;;
      
      *)
      echo "unknown block!: $block"
      usage
      exit 2
      ;;
  esac
  
  shift
#done

# final arithmetic to report result
rm -f test.log err.log
    if [ -f "$log" ]; then
    cat "$log"
    fi
echo ""
echo "Successful tests:      $success"
echo "Failed as expected:    $failure"    
echo "Unexpected success:    $unexSuccess"
echo "Unexpected failure:    $unexFailure"
echo "No Answer:             $dies"


