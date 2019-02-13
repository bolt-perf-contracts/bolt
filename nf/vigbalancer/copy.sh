#!/bin/bash

term=$1

rm -rf $term
mkdir $term

grep "$term" *.call_path | cut -d ":" -f1 > temp_file


while read -r line
do 
	cp $line $term/
	cp $(basename $line .call_path).tracelog $term/
	test=$(echo $line | cut -d "." -f1)
        echo $test >> $term/relevant_traces

done < temp_file

touch $term/stateful-error-log

rm temp_file
