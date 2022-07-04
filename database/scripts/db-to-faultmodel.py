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

def start_building(results_cursor, my_fault_model_filename):
    count=0
    try:
        instructions_cursor=results_cursor.execute('''SELECT distinct instruction from results order by instruction''')
        instructions_result = instructions_cursor.fetchall();
        
        f = open(my_fault_model_filename, 'w')

        # Loop through instructions
        for i in instructions_result:
            instruction=i[0]
            f.write ("Instructions: {}-{}\n".format(instruction, instruction))
            try:
                sql_to_execute = "SELECT distinct target_type from results where instruction={} order by target_type".format(instruction)
                targets_cursor=results_cursor.execute(sql_to_execute)
                targets_result = targets_cursor.fetchall();

                # Loop through the targets
                for t in targets_result:
                    target_type=t[0]
                    if target_type=="Instruction":
                        f.write ("   Instruction:\n")
                        f.write ("    Op_codes: ALL\n")

                        ################## Duplicated coded ########################## 
                        # Loop through lifespan
                        sql_to_execute = "SELECT distinct fault_lifespan from results where instruction={} and target_type='{}' order by fault_lifespan".format(instruction,target_type)
                        lifespan_cursor = results_cursor.execute(sql_to_execute)
                        lifespan_result = lifespan_cursor.fetchall();
                        for l in lifespan_result:
                            lifespan=l[0]
                            f.write ("      Lifespan: {},revert\n".format(lifespan))

                            # Loop through Operations
                            sql_to_execute = "SELECT distinct operation from results where instruction={} and target_type='{}' and fault_lifespan={} order by operation".format(instruction,target_type, lifespan)
                            operation_cursor = results_cursor.execute(sql_to_execute)
                            operation_result = operation_cursor.fetchall();
                            for o in operation_result:
                                operation=o[0]
                                f.write ("        Operation: {}\n".format(operation))

                                # Loop through Masks
                                sql_to_execute = "SELECT distinct mask_hex from results where instruction={} and target_type='{}' and fault_lifespan={} and operation = '{}' order by mask".format(instruction,target_type, lifespan,operation)
                                mask_cursor = results_cursor.execute(sql_to_execute)
                                mask_result = mask_cursor.fetchall();
                                mask_string = "          Masks: "
                                for m in mask_result:
                                    mask=m[0]
                                    mask_string=mask_string + mask + ","
                                f.write (mask_string[:-1] + "\n")  # Add a new line
                        ################## Duplicated coded ########################## 
                        count=count+1


                    if target_type=="Register":
                        
                        # Loop through registers
                        sql_to_execute = "SELECT distinct register_name from results where instruction={} and target_type='Register' order by register_name".format(instruction)
                        registers_cursor = results_cursor.execute(sql_to_execute)
                        registers_result = registers_cursor.fetchall();
                        for r in registers_result:
                            register = r[0]
                            f.write ("  Registers-force: {}\n".format(register))
                            f.write ("    Op_codes: ALL\n")

                            ################## Duplicated coded ##########################  addition is: 'register_name'
                            # Loop through lifespan
                            sql_to_execute = "SELECT distinct fault_lifespan from results where instruction={} and target_type='{}' and register_name='{}' order by fault_lifespan".format(instruction,target_type,register)
                            lifespan_cursor = results_cursor.execute(sql_to_execute)
                            lifespan_result = lifespan_cursor.fetchall();
                            for l in lifespan_result:
                                lifespan=l[0]
                                f.write ("      Lifespan: {},revert\n".format(lifespan))


                                # Loop through Operations
                                sql_to_execute = "SELECT distinct operation from results where instruction={} and target_type='{}'  and register_name='{}' and fault_lifespan={} order by operation".format(instruction,target_type,register,lifespan)
                                operation_cursor = results_cursor.execute(sql_to_execute)
                                operation_result = operation_cursor.fetchall();
                                for o in operation_result:
                                    operation=o[0]
                                    f.write ("        Operation: {}\n".format(operation))

                                    # Loop through Masks
                                    sql_to_execute = "SELECT distinct mask_hex from results where instruction={} and target_type='{}'  and register_name='{}' and fault_lifespan={} and operation = '{}' order by mask".format(instruction,target_type, register,lifespan,operation)
                                    mask_cursor = results_cursor.execute(sql_to_execute)
                                    mask_result = mask_cursor.fetchall();
                                    mask_string = "          Masks: "
                                    for m in mask_result:
                                        mask=m[0]
                                        mask_string=mask_string + mask + ","
                                    f.write (mask_string[:-1]+"\n")  # Add a new line

                            ################## Duplicated coded ########################## 
                            count=count+1
            except Exception as e:
                print (str(e))

    except Exception as e:
        print (str(e))

    print ("Count:",count)

if __name__ == '__main__':
    my_fault_model_filename = "fault_model_output.txt"
    
    num_args=len(sys.argv)
    if (num_args!=2  and num_args!=3):
        print("Usage",sys.argv[0],"database_filename [fault_model_filename]")
        exit (-1)
    my_database=sys.argv[1]
    if (num_args>2):
        my_fault_model_filename=sys.argv[2]


    print ("Fault model output    :",my_fault_model_filename)
    print ("Database file to use  :",my_database)

    # create a database connection
    conn = my_results_database.create_connection(my_database)

    # create tables
    if conn is not None:
        # create result table
        my_results_database.create_table(conn)
    else:
        print("Error! cannot create the database connection.")
        exit (-1)

    print ("Reading records to create fault model file")
    output_results=my_results_database.select_result_record(conn)
    start_building(output_results, my_fault_model_filename)

