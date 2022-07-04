#!/usr/bin/python3
import my_results_database
from os import listdir
from os.path import isfile, join
import sys

def clean_up(val):
    mytable = val.maketrans("","" ,":;.")
    val2=val.translate(mytable)
    return val2.strip()

def create_records(conn, file, my_run_name):
    print (file)
    fp=open(file,"r")
    count=0
    for l in fp.readlines():
        try:
            # Starting line
            if (l.startswith("##### Starting new run")):
                rec=my_results_database.FaultResultRecord(my_run_name)
                val=l.split()
                for i in range(len(val)):
                    if val[i]=="Address:":
                        rec.address=int(clean_up(val[i+1]),16)
                        rec.trigger_address=int(clean_up(val[i+1]),16)
                    elif val[i]=="Instruction:":
                        rec.instruction=int(clean_up(val[i+1]))
                    elif val[i]=="Mask:":
                        rec.mask=int(clean_up(val[i+1]),16)
                    elif val[i]=="Operation:":
                        rec.operation=clean_up(val[i+1])
                    elif val[i]=="Reg#:":
                        rec.register_name=clean_up(val[i+1])
                    elif val[i]=="Flag#:":
                        rec.register_name=clean_up(val[i+1])
                    elif val[i]=="Target:":
                        rec.target_type=clean_up(val[i+1])
                    elif val[i]=="Hit:":
                        rec.hit_count=clean_up(val[i+1])
                    elif val[i]=="Lifespan:":
                        rec.fault_lifespan=clean_up(val[i+1])
                    elif val[i]=="Mode:":
                        rec.fault_lifespan_mode=clean_up(val[i+1])
            elif (l.startswith(" >> Original")):
                val=l.split(":")
                rec.original_target=clean_up(val[1])
                if (l.startswith(" >> Original instruction")):
                    try:
                        rec.original_target_instruction=clean_up(val[2])
                    except:
                        rec.original_target_instruction="Unable to disassemble"
            elif (l.startswith(" >> Updated")):
                val=l.split(":")
                rec.updated_target=clean_up(val[1])
                if (l.startswith(" >> Updated instruction")):
                    try:
                        rec.updated_target_instruction=clean_up(val[2])
                    except:
                        rec.updated_target_instruction="Unable to disassemble"
            elif (l.startswith(" >! Errored:")):
                # Get to the result
                val=l[12:]
                rec.extra_info=clean_up(val)
            elif (l.startswith(" >> Run result")):
                # Get to the result
                val=l.split(":")
                rec.run_result=clean_up(val[1])
            elif (l.startswith(" >>> Output")):
                # Get to the output
                val=l.split(":")
                rec.output_result=clean_up(val[1])
                my_results_database.create_result_record(conn,rec)
                count=count+1        
                if (count+1) % 1000==0:
                    conn.commit()
                    print ("Created: ",count+1,"records")
        except Exception as e:
            print ("Failed on line: ",l)
            print (str(e))
    return count

if __name__ == '__main__':

    my_database = "dbs/faultfinder.db"
    my_run_name = "faultfinder"
    my_directory = ""
    
    num_args=len(sys.argv)
    if (num_args!=2 and num_args!=3 and num_args!=4 ):
        print("Usage",sys.argv[0],"result_directory [run_name] [database_file]")
        exit (-1)

    my_directory=sys.argv[1]
    if (num_args>2):
        my_run_name=sys.argv[2]
    if (num_args>3):
        my_database=sys.argv[3]

    print ("Run name in database: ",my_run_name)
    print ("Directory to use:     ",my_directory)
    print ("Database to use:      ",my_database)
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
    count=0
    for f in listdir(my_directory):
        full_file_with_path=join(my_directory, f)
        if isfile(full_file_with_path):
            count=count+create_records(conn,full_file_with_path,my_run_name)
    conn.commit()
    print (count,"records created")
