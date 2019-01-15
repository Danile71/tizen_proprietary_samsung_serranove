#!/bin/bash

count=0

echo "" > log.txt
while [ $count -lt 1000 ]
do
	echo "Test [$count]"
	echo "Test case [$count] ==============================" >> log.txt
	./iniexample_thread >> log.txt
	count=`expr $count + 1`
done
