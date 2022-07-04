#!/bin/bash
num='4327'
num_long='00004527'
echo
echo " ARMory (faultfinder) golden run - finding results after $num"
echo "-------------------------------------------------"
./faultfinder experiments/ARMory/jsons/goldenrun-aes.json | grep -A500000 "$num_long hit:" | cut -d' ' -f3,1 > temp_golden.txt


echo
echo " ARMory lines in DB where instruction==4327"
echo "--------------------------------------------"

echo "select distinct address_hex from results where instruction=$num;" > temp.sql
sqlite3 database/dbs/ARMory.db.original < temp.sql  > incorrect_instructions.txt

echo
echo " Finding instruction numbers for addresses"
echo "------------------------------------------"
while read -r line; do
    # get rid of new line
    clean_line=`echo -n $line | tr -d '\n' | tr 'x' '0' `
    cat temp_golden.txt | grep -m 1 $clean_line  >> matching_pairs.txt
done < incorrect_instructions.txt


echo
echo " Building sql statements to fix the 4327 problem"
echo "------------------------------------------------"
awk '{printf "update results set instruction=" $1 " where address=" $2 " and instruction = 4327;\n"}' matching_pairs.txt > temp.sql



echo
echo " Running SQL Statement on database: ARMory.db.original"
echo "------------------------------------------------------"

sqlite3 database/dbs/ARMory.db.original < temp.sql 

echo
echo " FINISHED - go check the database!!! "
echo "-------------------------------------"


rm -f temp_golden.txt
rm -f incorrect_instructions.txt
rm -f matching_pairs.txt
rm -f temp.sql
rm -f temp.sql