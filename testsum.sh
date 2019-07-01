#!/bin/bash
logFile='testout.log'

function loopTest(){
    testN=$1
    min=$2
    max=$3
    
    echo "#######/ Start Test $testN /#######" >> $logFile;
    for i in `seq $min $max`; do
        ./objstore_client user$i $testN &>> $logFile & clientpid+="$! ";
    done;
    echo -e "#######/ End Test $testN /####### \n" >> $logFile;
    wait $clientpid
    clientpid="";
}

function checkLog(){
    #What the output should be
    targetT1=1000
    targetT2=30
    targetT3=20
    totalClientTarget=100
    
    nclient=0
    #Counting special words that defines the end of a specifc action
    tst1=$(grep -c "Test1 OK" $logFile)
    tst2=$(grep -c "Test2 OK" $logFile)
    tst3=$(grep -c "Test3 OK" $logFile)
    nclient=$(grep -c "CONNECTED" $logFile)
    
    echo "/------------------------------------------\\"
    echo "| Connected client: $nclient"
    echo "| Result TEST 1: PASSED: $tst1 FAILED: $((targetT1-tst1))    |"
    echo "| Result TEST 2: PASSED: $tst2 FAILED: $((targetT2-tst2))      |"
    echo "| Result TEST 3: PASSED: $tst3 FAILED: $((targetT3-tst3))      |"
    
    if [ $nclient == $totalClientTarget ] && [ $tst1 == $targetT1 ] && [ $tst2 == $targetT2 ] && [ $tst3 == $targetT3 ] ;
    then echo "| ALL TEST PASSED                          |"
    else echo "| SOME TEST FAILED                          |"
    fi
    
    echo "\------------------------------------------/"
}

loopTest 1 1 50 #Exectue test1 with range 1-50
loopTest 2 1 30 #Exectue test2 with range 1-30
loopTest 3 31 50 #Exectue test3 with range 31-50
checkLog #Execture that check and print the log result

#kill -USR1 $(pidof objstore_server)
