#!/usr/bin/python3
from re import X
import my_results_database
from os import listdir
from os.path import isfile, join
import sys
import tables
import h5py

class Trigger:
    def __init__(self, trigger_address, trigger_hitcounter):
        """
        Define attributes for trigger
        """
        self.address = trigger_address
        self.hitcounter = trigger_hitcounter

class Fault:
    def __init__(
        self,
        fault_address: int,
        fault_lifespan: int,
        fault_mask: int,
        fault_model: int,
        num_bytes: int,
        fault_type: int,
        trigger_address: int,
        trigger_hitcounter: int,
    ):
        """
        Define attributes for fault types
        """
        self.trigger = Trigger(trigger_address, trigger_hitcounter)
        self.address = fault_address
        self.type = fault_type
        self.model = fault_model
        self.lifespan = fault_lifespan
        self.mask = fault_mask
        self.num_bytes = num_bytes

    def print_me(self):
        print ("****  FAULTS    ****")
        print ("fault address:         " ,hex(self.address))
        print ("fault livespan:        " ,self.lifespan)
        print ("fault mask:            " ,hex(self.mask))
        print ("fault model:           " ,self.model)
        print ("fault type:            " ,self.type)
        print ("fault number of bytes  " ,hex(self.num_bytes))
        print ("trigger address        " ,hex(self.trigger.address))
        print ("trigger hitcounter     " ,self.trigger.hitcounter)
    
class Memdump:
    def __init__(self, address, length, num_dumps, hex_output):
        """
        Define attributes for trigger
        """
        self.address = address
        self.length = length
        self.num_dumps = num_dumps
        self.hex_output=hex_output
    
    def print_me(self):
        print ("****  MEMDUMPS    ****")
        print ("memdump address:                " ,hex(self.address))
        print ("memdump length:                 " ,self.length)
        print ("memdump num_dumps:              " ,self.num_dumps)
        print ("memdump hex_output: " ,self.hex_output)

def clean_up(val):
    mytable = val.maketrans("","" ,"\n")
    val2=val.translate(mytable)
    return val2.strip()


def create_records_from_hdf5(conn, my_hdf5_filename, my_run_name):
    ff = tables.open_file(my_hdf5_filename, "r")
    fault_group = ff.root.fault

    count=0
    line_count=0
    for experiment in fault_group:
        line_count=line_count+1
        rec=my_results_database.FaultResultRecord(my_run_name)
        ## faults
        experiment_name=str(experiment).split()[0]
        rec.extra_info=experiment_name
        row_of_data=experiment["faults"][0]
        f=Fault(*row_of_data)
        rec.trigger_address=int(f.trigger.address)
        rec.hit_count=int(f.trigger.hitcounter)
        rec.mask=int(f.mask)
        rec.fault_lifespan=int(f.lifespan)
        rec.fault_lifespan_mode="revert"
        # operation
        # set to 1 = 1
        # set to 0 = 0
        # toggle = 2
        # overwrite = 3
        if f.model==0:
            rec.operation="CLEAR"
        elif f.model==1:
            rec.operation="SET"
        elif f.model==2: 
            rec.operation="TOGGLE"
        elif f.model==3: #overwrite?
            rec.operation="OVERWRITE"
        else:
            rec.operation=f.model + " - unknown (ARCHIE)"

        #type
        #sram or data = 0
        #flash or instruciton = 1
        #register = 2
        if f.type==0:
            rec.target_type="Data"
        elif f.type==1:
            rec.target_type="Instruction"
            rec.address=int(f.address)
            ## tb faulted
            fault_instruction=experiment["tbfaulted"]
            for f in fault_instruction:
                assembly_code_full_string=(f['assembler']).decode()
                assembly_code=clean_up(assembly_code_full_string.split(":")[1])
                rec.updated_target_instruction=assembly_code
        elif f.type==2:
            rec.target_type="Register"
            rec.address=rec.trigger_address
            rec.register_name=str(f.address)
        else:
            rec.target_type=f.type + " - unknown (ARCHIE)"

        ## Memdump
        groups=experiment["memdumps"]
        sub_groups=groups["memdumps"]
        for m in sub_groups:
            a=m['address']
            l=m['length']
            n=m['numdumps']
            searching_for="location_{:08x}_{:d}_{:d}".format(a,l,n)
            output_array=groups[searching_for]
            output_str=""
            for bytes in output_array:
                for b in bytes:
                    output_str+="{:02x}".format(b)
            md = Memdump(a,l,n,output_str)

            if md.address == 0x20000000: # HACK HACK
                rec.output_result_address=md.address
                rec.output_result=md.hex_output
        ## include tbexeclist or tbinfo if it's an instruction that was faulted??

        my_results_database.create_result_record(conn,rec)

        count=count+1
        if count % 1000==0:
            conn.commit()
            print ("Created: ",count,"records")
    return count

if __name__ == '__main__':

    my_database = "dbs/archie.db"
    my_run_name = "ARCHIE"
    my_hdf5_filename = "/home/kit/archie/archie-aes-example/output.hdf"
    
    num_args=len(sys.argv)
    if (num_args!=2 and num_args!=3 and num_args!=4 ):
        print("\nUsage",sys.argv[0]," hdf5_file [run_name] [database_file]\n")
        print("Default database_file: " , my_database)
        print("Default run_name:      " , my_run_name)
        exit (-1)

    my_hdf5_filename=sys.argv[1]
    if (num_args>2):
        my_run_name=sys.argv[2]
    if (num_args>3):
        my_database=sys.argv[3]

    print ("Run name in database: ",my_run_name)
    print ("hdf5 file to use:     ",my_hdf5_filename)
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

    count=0
    count=create_records_from_hdf5(conn, my_hdf5_filename,my_run_name)
    conn.commit()
    print (count,"records created")
