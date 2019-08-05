#!/bin/bash

output=sort_bench_result

g++ -O3 -std=c++11 sort-ubench.cpp -o sort-ubench.o

if [ -f $output ] ; then
		rm $output
fi

touch $output

for size in $(cat list_sizes); 
		do
			   	./sort-ubench.o $size >> $output ;
	   	done 

cat $output


