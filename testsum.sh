#!/bin/bash
#const
logFile='testout.log'
clientName='objstore_client'
serverName='objstore_server'

rm $logFile #remove existing logFile

function loopTest(){
    testN=$1
    min=$2
    max=$3
    
    for i in `seq $min $max`; do
        ./$clientName user$i $testN &>> $logFile;
    done;
    wait
    
    for i in `seq 1 30`; do
        ./$clientName user$i 2 &>> $logFile;
    done;
    
    for i in `seq 31 50`; do
        ./$clientName user$i 3 &>> $logFile;
    done;
    wait
}

function checkLog(){
    #What the output should be
    targetT1=1000
    targetT2=30
    targetT3=20
    clientTarget=100
    
    nclient=0
    #Using grep to count specific keyword
    tst1=$(grep -c "Test1 OK" $logFile)
    tst2=$(grep -c "Test2 OK" $logFile)
    tst3=$(grep -c "Test3 OK" $logFile)
    nclient=$(grep -c "CONNECTED" $logFile)
    
    echo "/------------------------------------------\\"
    echo "| Connected client: $nclient/$clientTarget"
    echo "| Result TEST 1: PASSED: $tst1 FAILED: $((targetT1-tst1))    |"
    echo "| Result TEST 2: PASSED: $tst2 FAILED: $((targetT2-tst2))      |"
    echo "| Result TEST 3: PASSED: $tst3 FAILED: $((targetT3-tst3))      |"
    echo "\------------------------------------------/"
}

echo -ne "waiting for the server to properly start up"
for i in `seq 1 3`; do
    sleep 0.1
    echo -ne "."
done;
echo -e "\n"

loopTest 1 1 50 #Exectue test1 with range 1-50
checkLog #Execture check of logFile and print the result

kill -USR1 $(pidof objstore_server)
wait
kill -SIGINT $(pidof objstore_server)