#!/usr/bin/python3

import my_results_database
import re
import string
import sys

def next_line(fp,hunt_for):
    l=""
    while (hunt_for not in l):
        l=fp.readline();
        if not l:
            print ("Run out of lines")
            return False
    return l

def clean_up(val):
    mytable = val.maketrans("","" ,":;.")
    val2=val.translate(mytable)
    return val2.strip()

def create_records(conn, file):
    # EXAMPLE LINE:
    # TransientNopInstructionModel 80000344(1/5): LDR r3, [r3] -> NOP otp_read32 + 24 C:\Users\micro\Source\Repos\FiSim\Content\Example\src\otp.c:32
    print ("File to import:",file)
    fp=open(file,"r")
    count=0
    line_count=0
    for l in fp.readlines():
        line_count=line_count+1
        try:
            rec=my_results_database.FaultResultRecord(my_run_name)
            # OPERATION and TARGET TYPE
            val=l.split()
            if val[0]=="TransientSingleBitFlipInstructionModel":
                rec.operation="XOR"
                rec.target_type="Instruction"
            else:
                rec.operation="SKIP"
                rec.target_type="InstructionPointer"
            

            try:
                # ADDRESS
                # sometimes this file includes (x/N) in the line, directly after the address value
                temp_address=val[1].split("(")[0]
                rec.address=int(clean_up(temp_address),16)
                rec.trigger_address=rec.address
            except:
                print ("Line number: ",line_count, "\n >> ", l)
                print ("Errored trying to get address", temp_address)
                break
            

            try:
                # ORIGINAL CODE - between : and ->
                found = re.search(':(.+?)->', l).group(1).strip()
                rec.original_target_instruction=found.lower()
            except:
                print ("Line number: ",line_count)
                print ("Errored trying to get original_target_instruction", found)
                break

            # UPDATED CODE - between : and ->
            found = re.search('->(.+?)\+', l).group(1).strip()
            # need to remove the last word as this is the function that it's in.
            found_tidy=found.rsplit(' ', 1)[0]
            rec.updated_target_instruction=found_tidy.lower()

            # store the function
            found_tidy=found.rsplit(' ', 1)[1]
            rec.extra_info=found_tidy.lower()

            my_results_database.create_result_record(conn,rec)
            count=count+1
            if count % 1000==0:
                conn.commit()
                print ("Created: ",count,"records")
        except:
            print ("Errored on this line: ",l)
    return count

if __name__ == '__main__':

    my_database = "dbs/fisim.db"
    my_fisim_filename = "output-other-software/fisim-output.txt"
    
    num_args=len(sys.argv)
    if (num_args!=2 and num_args!=3 and num_args!=4 ):
        print("Usage",sys.argv[0],"run_name fisim_output_file [database_file]")
        exit (-1)
    my_run_name=sys.argv[1]
    if (num_args>2):
        my_fisim_filename=sys.argv[2]
    if (num_args>3):
        my_database=sys.argv[3]

    print ("Run name in database:    ",my_run_name)
    print ("fisim output file to use:",my_fisim_filename)
    print ("Database to use:         ",my_database)

    # create a database connection
    conn = my_results_database.create_connection(my_database)

    # create tables
    if conn is not None:
        # create result table
        my_results_database.create_table(conn)
    else:
        print("Error! cannot create the database connection.")
        exit (-1)

    print ("Saving records now")
    count=create_records(conn,my_fisim_filename)
    conn.commit()
    print (count,"records created in database: ",my_database)
