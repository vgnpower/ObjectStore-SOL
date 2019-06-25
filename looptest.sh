#!/bin/bash

echo "#######/ Start Test 1 /#######" >> testout.log;
for i in `seq 1 50`; do ./objstore_client user$i 1 &>> testout.log; done;
echo -e "#######/ End Test 1 /#######\n" >> testout.log;

echo "#######/ Start Test 2 /#######" >> testout.log;
for i in `seq 1 30`; do ./objstore_client user$i 2 &>> testout.log; done;
echo -e "#######/ End test 2 /#######\n" >> testout.log;

echo "#######/ Start Test 3 /#######" >> testout.log;
for i in `seq 31 50`; do ./objstore_client user$i 3 &>> testout.log; done;
echo "#######/ End test 3 /#######" >> testout.log;