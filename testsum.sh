#!/bin/bash
logFile='testout.log'
nt1=0
nt1Idedali=1000
nt2=0
nt2Idedali=30
nt3=0
nt3Idedali=20
nclient=0
testNumber=0
n=1

while read line; do
    if [ -n "$line" ];
    then
        if [ "$line" == "#######/ Start Test 1 /#######" ] ;
        then    testNumber=1
        fi
        
        if [ "$line" == "#######/ Start Test 2 /#######" ] ;
        then    testNumber=2
        fi
        
        if [ "$line" == "#######/ Start Test 3 /#######" ] ;
        then     testNumber=3
        fi
        
        case "$testNumber" in
            1)  if [ "$line" == "Test1 OK" ]; then
                    nt1=$((nt1+1))
                fi
            ;;
            2) if [ "$line" == "Test2 OK" ]; then
                    nt2=$((nt2+1))
                fi
            ;;
            3) if [ "$line" == "Test3 OK" ]; then
                    nt3=$((nt3+1))
                fi
            ;;
        esac
        
        if [ "$line" == "CONNECTED" ] ;
        then nclient=$((nclient+1))
        fi
    fi
    
    n=$((n+1))
done < $logFile

echo "/------------------------------------------\\"
echo "| Connected client: $nclient"
echo "| Result TEST 1: PASSED: $nt1 FAILED: $((nt1Idedali-nt1))    |"
echo "| Result TEST 2: PASSED: $nt2 FAILED: $((nt2Idedali-nt2))      |"
echo "| Result TEST 3: PASSED: $nt3 FAILED: $((nt3Idedali-nt3))      |"

if [ $nclient == 100 ] && [ $nt1 == 1000 ] && [ $nt2 == 30 ] && [ $nt3 == 20 ] ;
then
    echo "| ALL TEST PASSED                          |"
else
    echo "| SOME TEST FAILED                          |"
fi

echo "\------------------------------------------/"

BPID="$(pidof objstore_server)"
kill -USR1 $BPID
