#!/usr/bin/python3

from contextlib import nullcontext
from email.headerregistry import Address
import my_results_database
import re
import string
import sys
from scanf import scanf

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
    # EXAMPLE LINE: Exploitable with 1 fault   81d8: 7863 --> bf00  [ldrb r3, [r4, #1] --> nop] | Permanent Instruction Skip | time=4327
    print ("File to import:",file)
    fp=open(file,"r")
    count=0
    line_count=0
    for ll in fp.readlines():

        if ll.startswith('Exploitable with'):
            try:
                rec=my_results_database.FaultResultRecord(my_run_name)
                position=ll.find('fault')
                l=ll[position+5:]
                rec.extra_info=l
                parts=l.split('|')
                part1=parts[0]
                part2=parts[1]
                part3=parts[2]

                # Working backwards
                # Part 4
                # Time may or may not be there
                if len(parts)==4:
                    part4=parts[3]
                    rec.instruction=clean_up(part4.split("=")[1])

                # Part 3
                # Position
                time_or_pos=clean_up(part3)
                time_or_pos_number=int(clean_up(time_or_pos.split("=")[1]))
                if time_or_pos.startswith("time="):
                    rec.instruction=time_or_pos_number
                if time_or_pos.startswith("position="):
                    position=time_or_pos_number

                # Part 2
                # OPERATION and TARGET TYPE
                pattern=" %s %s %s "
                data=scanf(pattern,part2)
                perm_or_trans_or_until=data[0]
                rec.target_type=data[1]
                operation=data[2]

                #Part1
                pattern=" %s: %s --> %s "
                data=scanf(pattern,part1)
                reg_or_address=clean_up(data[0])
                rec.original_target=clean_up(data[1])
                rec.updated_target=clean_up(data[2])

                if rec.target_type=="Instruction":
                    rec.address=int(reg_or_address,16)
                    start=part1.find("[")+1
                    end=part1.rfind("]")
                    instr_change=part1[start:end]
                    instr=instr_change.split(" --> ")
                    rec.original_target_instruction=instr[0].lower()
                    rec.updated_target_instruction=instr[1].lower()
                if rec.target_type=="Register":
                    rec.register_name=reg_or_address

                rec.fault_lifespan_mode=""
                # Permanent or Transient
                if perm_or_trans_or_until=="Permanent":
                        rec.fault_lifespan=0
                if perm_or_trans_or_until=="Transient":
                        rec.fault_lifespan=1
                        rec.fault_lifespan_mode="revert"
                if perm_or_trans_or_until=="Until-Overwrite":
                    rec.fault_lifespan=0

                rec.mask=-1
                if "Clear" in operation:
                    rec.mask = 0x0
                    rec.operation = "SET"
                if "Fill" in operation:
                    rec.mask = 0xFFFFFFFF
                    rec.operation = "SET"
                if "Byte-Clear" in operation:
                    rec.mask = 0xffffffff ^ (0xff<<(position*8))
                    rec.operation = "AND"
                if "Byte-Set" in operation:
                    rec.mask = (0xff << (position*8))
                    rec.operation = "OR"
                if "Byte-Flip" in operation:
                    rec.mask = (0xff << (position*8))
                    rec.operation = "XOR"
                if "Bit-Clear" in operation:
                    rec.mask = 0xffffffff ^ (0x1<<position)
                    rec.operation = "AND"
                if "Bit-Set" in operation:
                    rec.mask = (0x1 << position)
                    rec.operation = "OR"
                if "Bit-Flip" in operation:
                    rec.mask = (0x1 << position)
                    rec.operation = "XOR"
                if rec.updated_target_instruction == "nop":
                    rec.operation =  "SET"
                    rec.mask = int(rec.updated_target,16)

            except Exception as e:
                print ("Errored on this line: ",l)
                print (str(e))

        if ll.startswith('Exploitable output'):
            parts=ll.split(':')
            rec.output_result=clean_up(parts[1])

            # Urgh - added this because sometimes rec.instruction isn't viewed as a number
            rec.instruction=int(rec.instruction)


            my_results_database.create_result_record(conn,rec)
            count=count+1
            if count % 1000==0:
                conn.commit()
                print ("Created: ",count,"records")
    exit (-1)
    return count

if __name__ == '__main__':
    my_database = "dbs/ARMory.db"
    my_ARMory_filename = "output-other-software/finale.txt"
    my_run_name = "ARMory"

    num_args=len(sys.argv)
    if (num_args!=1 and num_args!=2 and num_args!=3 and num_args!=4 ):
        print("\nUsage",sys.argv[0],"[ARMory_output_file] [run_name] [database_file]\n")
        print("Default database:      ", my_database);
        print("Default ARMory output: ", my_ARMory_filename);
        print("Default run name:      ", my_run_name);
        exit (-1)

    if (num_args>1):
        my_ARMory_filename=sys.argv[1]
    if (num_args>2):
        my_run_name=sys.argv[2]
    if (num_args>3):
        my_database=sys.argv[3]

    print ("Run name in database:     ",my_run_name)
    print ("ARMory output file to use:",my_ARMory_filename)
    print ("Database to use:          ",my_database)

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
    count=create_records(conn,my_ARMory_filename)
    conn.commit()
    print (count,"records created in database: ",my_database)
