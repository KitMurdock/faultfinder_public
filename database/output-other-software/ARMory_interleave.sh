#!/bin/bash

if [ "$#" -ne 1 ] 
then 
    echo "Useage: $0 <ARMory output file>"
    exit -1
fi

filename=$1

cat $1 | grep "Exploitable output" >eo.txt
cat $1 | grep "Exploitable with" >ew.txt
paste -d '\n' ew.txt eo.txt  > finale.txt
rm eo.txt
rm ew.txt
