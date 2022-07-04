import sqlite3
from sqlite3 import Error

create_table_sql = """CREATE TABLE IF NOT EXISTS results (
                                        id integer PRIMARY KEY,
                                        address integer NOT NULL,
                                        address_hex string NOT NULL,
                                        instruction integer,
                                        hit_count integer,
                                        register_name text,
                                        trigger_address integer,
                                        trigger_address_hex string,
                                        fault_lifespan integer,
                                        fault_lifespan_mode text,
                                        time_to_run integer,
                                        fault_num_bytes integer,
                                        target_type text NOT NULL,
                                        operation text NOT NULL, 
                                        mask integer NOT NULL,
                                        mask_hex integer NOT NULL,
                                        original_target text NOT NULL,
                                        original_target_instruction text,
                                        updated_target NOT NULL,
                                        updated_target_instruction text,
                                        output_result text NOT NULL,
                                        run_result text NOT NULL,
                                        run_name text NOT NULL,
                                        extra_info text
                                    ); """

class FaultResultRecord:
    def __init__(self,run_name):
        """
        Define attributes for trigger
        """
        self.address                     = 0
        self.address_hex                 = ""
        self.instruction                 = 0
        self.hit_count                   = 0
        self.register_name             = ""
        self.trigger_address             = 0
        self.trigger_address_hex         = ""
        self.fault_lifespan              = 0
        self.fault_lifespan_mode         = ""
        self.time_to_run                 = 0
        self.fault_num_bytes             = 0
        self.target_type                 = ""
        self.operation                   = ""
        self.mask                        = -1
        self.mask_hex                    = ""
        self.original_target             = ""
        self.original_target_instruction = ""
        self.updated_target              = ""
        self.updated_target_instruction  = ""
        self.output_result               = ""
        self.output_result_address       = 0
        self.output_result_address_hex   = "" 
        self.run_result                  = ""
        self.run_name                    = run_name
        self.extra_info                  = ""
    
    def print_me(self):
        print("address:                    ",self.address)
        print("address_hex:                ",self.address_hex)
        print("instruction:                ",self.instruction)
        print("hit_count:                  ",self.hit_count)
        print("register_name:              ",self.register_name)
        print("trigger_address:            ",self.trigger_address)
        print("trigger_address_hex:        ",self.trigger_address_hex)
        print("fault_lifespan:             ",self.fault_lifespan)
        print("fault_lifespan mode:        ",self.fault_lifespan_mode)
        print("time_to_run:                ",self.time_to_run)
        print("fault_num_bytes:            ",self.fault_num_bytes)
        print("target_type:                ",self.target_type)
        print("operation:                  ",self.operation)
        print("mask:                       ",self.mask)
        print("mask_hex:                   ",self.mask_hex)
        print("original_target             ",self.original_target)
        print("original_target_instruction ",self.original_target_instruction)
        print("updated_target              ",self.updated_target)
        print("updated_target_instruction  ",self.updated_target_instruction)
        print("output_result               ",self.output_result)
        print("output_result_address       ",self.output_result_address)
        print("run_result                  ",self.run_result)
        print("run_name                    ",self.run_name)
        print("extra_info                  ",self.extra_info)

    def return_fields(self):
        if self.address!=0:
            self.address_hex=hex(self.address)
        if self.trigger_address!=0:
            self.trigger_address_hex=hex(self.trigger_address)
        if self.mask!= -1:
            self.mask_hex=hex(self.mask)
        if self.mask == 0:
            self.mask_hex="0x00000000"

        return (self.address, 
                self.address_hex.lower(),
                self.instruction,
                self.hit_count,
                self.register_name.lower(),
                self.trigger_address,
                self.trigger_address_hex.lower(),
                self.fault_lifespan,
                self.fault_lifespan_mode,
                self.time_to_run,
                self.fault_num_bytes,
                self.target_type,
                self.operation.upper(),
                self.mask,
                self.mask_hex.lower(),
                self.original_target.lower(),
                self.original_target_instruction.lower(),
                self.updated_target.lower(),
                self.updated_target_instruction.lower(),
                self.output_result.lower(),
                self.run_result,
                self.run_name,
                self.extra_info)
        

def create_connection(db_file):
    """ create a database connection to the SQLite database
        specified by db_file
    :param db_file: database file
    :return: Connection object or None
    """
    conn = None
    try:
        conn = sqlite3.connect(db_file)
        # This speeds it up x6
        conn.execute('pragma journal_mode=wal')
        return conn
    except Error as e:
        print(e)

    return conn

def create_table(conn):
    """ create a table from the create_table_sql statement
    :param conn: Connection object
    :param create_table_sql: a CREATE TABLE statement
    :return:
    """
    try:
        c = conn.cursor()
        c.execute(create_table_sql)
        print("\n-- Created results table --")
        conn.commit()
    except Error as e:
        print(e)
        exit (-1)

def create_result_record(conn, result_record):
    sql = ''' INSERT INTO results(address,address_hex,instruction,hit_count,register_name,
                trigger_address,trigger_address_hex, fault_lifespan,fault_lifespan_mode,time_to_run, fault_num_bytes,
                target_type,operation,mask,mask_hex,
                original_target,original_target_instruction,
                updated_target, updated_target_instruction,
                output_result,run_result,run_name, extra_info)
                VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) '''
    cur = conn.cursor()
    cur.execute(sql, result_record.return_fields())

def select_result_record(conn):
    sql = ''' 
            select address,address_hex,instruction,hit_count,register_name,
            trigger_address,trigger_address_hex, fault_lifespan,fault_lifespan_mode,time_to_run, fault_num_bytes,
            target_type,operation,mask,mask_hex,
            original_target,original_target_instruction,
            updated_target, updated_target_instruction,
            output_result,run_result,run_name, extra_info from results
            '''
    cur = conn.cursor()
    output_results=cur.execute(sql)
    return output_results