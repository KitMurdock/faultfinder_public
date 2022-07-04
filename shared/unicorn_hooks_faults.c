#include <capstone/capstone.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "unicorn_engine.h"
#include "unicorn_consts.h"
#include "configuration.h"
#include "state.h"
#include "structs.h"
#include "utils.h"
#include "fileio.h"

#define FAULT_AFTER false  // Do you do the fault BEFORE or after the isntruction has completed?

extern bool is_equivalent(uc_engine *uc, current_run_state_t *current_run_state);

void delete_hook_count_instructions(uc_engine *uc, current_run_state_t *current_run_state)
{
    // DELETE the counting hook
    if (current_run_state->hk_count_instructions !=0)
    {
#ifdef DEBUG
        printf_debug("delete_hook_count_instructions\n");
#endif
        my_uc_hook_del(uc, current_run_state->hk_count_instructions, current_run_state);
        current_run_state->hk_count_instructions=0;
    }
}
void delete_hook_code_fault_address(uc_engine *uc, current_run_state_t *current_run_state)
{
    // DELETE the fault hook
    if (current_run_state->hk_fault_address !=0)
    {
#ifdef DEBUG
        printf_debug("delete_hook_code_fault_address\n");
#endif
        my_uc_hook_del(uc, current_run_state->hk_fault_address, current_run_state);
        current_run_state->hk_fault_address=0;
    }
}


void delete_hook_code_fault_it(uc_engine *uc, current_run_state_t *current_run_state)
{
    // DELETE the fault hook
    if (current_run_state->hk_fault_it !=0)
    {
#ifdef DEBUG
        printf_debug("delete_hook_code_fault_address\n");
#endif
        my_uc_hook_del(uc, current_run_state->hk_fault_it, current_run_state);
        current_run_state->hk_fault_it=0;
    }
}

void print_binary2(uint64_t number)
{
    printf("\n Value now: %" PRIx64 "",number);
    if (number >> 1) {
        print_binary2(number >> 1);
    }
    printf("%i",(number & 1) ? '1' : '0');
}

void add_hook_code_fault_address(uc_engine *uc, current_run_state_t* current_run_state, uint64_t address_to_fault)
{
    address_to_fault=thumb_check_address(address_to_fault);
#ifdef DEBUG
    printf_debug("add_hook_code_fault_address. Address to fault 0x%" PRIx64 ". Count: %li\n", 
        address_to_fault, current_run_state->instruction_count);
#endif
    if (current_run_state->fault_rule.target==instruction_ft)
    {
        my_uc_hook_add("hook_code_fault_it_instruction", uc, &current_run_state->hk_fault_it, UC_HOOK_CODE, hook_code_fault_it_instruction, current_run_state, address_to_fault, address_to_fault);
    }
    else
    {
        my_uc_hook_add("hook_code_fault_address", uc, &current_run_state->hk_fault_address, UC_HOOK_CODE, hook_code_fault_address, current_run_state, address_to_fault, address_to_fault);
    }
}

void hook_live_fault_lifespan(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    fault_rule_t* this_fault=&current_run_state->fault_rule;
    uint64_t fault_at_instruction=this_fault->instruction;
    uint64_t fault_address=current_run_state->line_details_array[fault_at_instruction].address;
    fault_address=thumb_check_address(fault_address);

    if (address==fault_address && this_fault->lifespan.count==this_fault->lifespan.live_count)
    {
        // The lifespan starts AFTER the faulted address.
        return;
    }
    fprintf_output(current_run_state->file_fprintf," Lifespan countdown: %li. (0x%" PRIx64 ")\n",this_fault->lifespan.count,address);
    this_fault->lifespan.count--; 

    // TO DO - write the repeat logic
    if( this_fault->lifespan.count==0 && this_fault->lifespan.mode==eREVERT_lsm)
    {
        // REVERT THE FAULT
        uint64_t value_to_revert=this_fault->lifespan.original_target_value;
        uint64_t thing_to_fault=0;

        switch (this_fault->target)
        {
            case reg_ft:
                //read it
                thing_to_fault=this_fault->number;
                fprintf_output(current_run_state->file_fprintf, "Reverting register (%s)        : 0x%016lx \n", register_name_from_int(thing_to_fault), value_to_revert);
                 //revert it
                uc_reg_write(uc, uc_reg_from_int(thing_to_fault), &value_to_revert); 
                break;
            case instruction_pointer_ft:
                //read it
                thing_to_fault=binary_file_details->my_pc_reg;
                fprintf_output(current_run_state->file_fprintf, "Reverting instruction pointer: 0x%016lx\n", value_to_revert);
                 //revert it
                uc_reg_write(uc, thing_to_fault, &value_to_revert); 
                break;
            default:
                fprintf(stderr, "Failed to find valid target to revert: %s\n", target_to_string(this_fault->target));
                my_exit(-1);
            }
            my_uc_hook_del(uc, current_run_state->hk_fault_lifespan,current_run_state);
            current_run_state->hk_fault_lifespan=0;
        }
}

void hook_live_fault_lifespan_instruction(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    fault_rule_t* this_fault=&current_run_state->fault_rule;
    uint64_t fault_at_instruction=this_fault->instruction;
    uint64_t current_instruction_count=current_run_state->instruction_count;
    uint64_t fault_address=current_run_state->line_details_array[fault_at_instruction].address;

    if (current_instruction_count <=(fault_at_instruction+1))
    {
        // The lifespan starts AFTER the faulted address.
        return;
    }
    fprintf_output(current_run_state->file_fprintf," Lifespan countdown (instruction): %li. (0x%" PRIx64 ") \n",this_fault->lifespan.count,address);
    this_fault->lifespan.count--; 
    if (this_fault->lifespan.count !=0)
        return;

    fault_address=thumb_check_address(fault_address);
    // TO DO - write the repeat logic
    if( this_fault->lifespan.mode==eREVERT_lsm)
    {
        // we're reverting the instruction 
        fprintf_output(current_run_state->file_fprintf, "Reverting address              : 0x%" PRIx64 "\n",fault_address);
        fprintf_output(current_run_state->file_fprintf, "Reverting instruction          :  ");
        for (uint64_t i=0;i<this_fault->lifespan.original_instruction_value_size ;i++)
        {
            fprintf(current_run_state->file_fprintf,"%02x ",this_fault->lifespan.original_instruction_value[i]);
        }
        fprintf (current_run_state->file_fprintf,"\n");
        uc_ctl_remove_cache(uc, fault_address, fault_address);
        uc_mem_write(uc,fault_address,this_fault->lifespan.original_instruction_value,this_fault->lifespan.original_instruction_value_size);
        uc_ctl_remove_cache(uc, fault_address, fault_address);
    // delete this hook
    my_uc_hook_del(uc, current_run_state->hk_fault_lifespan,current_run_state);
    current_run_state->hk_fault_lifespan=0;
    }
    /* Fixing for thumb */
    uint64_t new_address=address;
    if ((binary_file_details->my_uc_arch==UC_ARCH_ARM || binary_file_details->my_uc_arch==UC_ARCH_ARM64) && size==2)
        new_address++;
    uc_ctl_remove_cache(uc, fault_address, fault_address);

    current_run_state->restart=true;
    current_run_state->restart_address=new_address; //includes the +1 if it's thumb
    //printf("Stopping emulation\n"); //TEMPTEMP

    // We have to stop and start the emulation for an instruction change.
    uc_emu_stop(uc);
    delete_hook_code_fault_it(uc, current_run_state); 

}
void hook_code_equivalent(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    if (is_equivalent(uc, current_run_state))
    {
            // If we've found an equivalence - stop the run - no point in doing something if we've done it before!
            current_run_state->run_state=EQUIVALENT_rs;
            my_uc_emu_stop(uc);
            return;
    }
    my_uc_hook_del(uc, current_run_state->hk_equivalent,current_run_state);
}

void hook_code_fault_it_instruction(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    /*** This is where the faulting actually happens for INSTRUCTIONS ****/
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;

#ifdef DEBUG
        printf_debug("hook_code_fault_it_instruction. Address 0x%" PRIx64 ". Count: %li\n", address, current_run_state->instruction_count);
#endif
    if (current_run_state->in_fault_range==0)
    {
        return;
    }
    if (current_run_state->run_state==FAULTED_rs)
    {
        // Don't fault it more than once.
        return;
    }
    //For instructions we have to trigger it one before.
    if (current_run_state->fault_rule.instruction !=current_run_state->instruction_count)
    {
        // only fault the specific instruction
        return;
    }

    uint8_t* instruction_original=MY_STACK_ALLOC(sizeof(uint8_t)*(size+1));
    uint8_t* instruction_new=MY_STACK_ALLOC(sizeof(uint8_t)*(size+1));

    if (size > 8)
    {
        fprintf_output(current_run_state->file_fprintf," Warning. The instruction for this line is longer than 8 bytes - the mask may not be large enough.\n"); //TEMPTEMP //TODOTODO??
    }

    //read it
    uc_mem_read(uc,address,instruction_original,size);
    uint64_t fault_address=thumb_check_address(address);
    fprintf_output(current_run_state->file_fprintf, "Fault Address                  :  0x%" PRIx64 "\n",address);
    fprintf_output(current_run_state->file_fprintf, "Original instruction           :  ");
    for (int i=0;i<size;i++)
    {
        fprintf(current_run_state->file_fprintf,"%02x ",instruction_original[i]);
    }

    fault_rule_t *this_fault=&current_run_state->fault_rule;
    if (current_run_state->display_disassembly && binary_file_details->my_cs_arch != MY_CS_ARCH_NONE)
    {
        // Can be turned off to save time - although I've not done the time calculations to see if it saves much time
        disassemble_instruction_and_print(current_run_state->file_fprintf,instruction_original,size); 
    }
    else
    {
        fprintf(current_run_state->file_fprintf,"\n");
    }
    fprintf_output(current_run_state->file_fprintf, "Mask                           :  0x%" PRIx64 "\n", this_fault->mask);

    //fault it
    fault_instruction(this_fault->mask, this_fault->operation, instruction_original,instruction_new, size,current_run_state->file_fprintf);
    uc_mem_write(uc,fault_address,instruction_new,size);
    fprintf_output(current_run_state->file_fprintf, "Updated instruction            :  ");
    for (int i=0;i<size;i++)
    {
        fprintf(current_run_state->file_fprintf,"%02x ",instruction_new[i]);
    }
    if (current_run_state->display_disassembly && binary_file_details->my_cs_arch != MY_CS_ARCH_NONE )
    {
        // Can be turned off to save time - although I've not done the time calculations to see if it saves much time
        disassemble_instruction_and_print(current_run_state->file_fprintf,instruction_new,size); 
    }
    else
    {
        fprintf(current_run_state->file_fprintf,"\n");
    }
    // if we are using lifespan then we'll need the original instruction!
    this_fault->lifespan.original_instruction_value_size=size;
    memcpy(this_fault->lifespan.original_instruction_value,instruction_original,size);

    // set the address where this fault occurred
    this_fault->faulted_address=address;

    // we've done the fault - so set faulting_mode to faulted!!
    current_run_state->run_state=FAULTED_rs;

    // Check for equivalences
    if (current_run_state->stop_on_equivalence)
    {
        my_uc_hook_add("hook_equivalent", uc, &current_run_state->hk_equivalent, UC_HOOK_CODE, hook_code_equivalent, current_run_state, 1, 0);
    }

    if (this_fault->lifespan.count!=0)
    {
        this_fault->lifespan.live_count=this_fault->lifespan.count;
        my_uc_hook_add("hook_lifespan", uc, &current_run_state->hk_fault_lifespan, UC_HOOK_CODE, hook_live_fault_lifespan_instruction, current_run_state, 1, 0);
    }

    /* Fixing for thumb */
    uint64_t new_address=address;
    if ((binary_file_details->my_uc_arch==UC_ARCH_ARM || binary_file_details->my_uc_arch==UC_ARCH_ARM64) && size==2)
        new_address++;
    

    uc_ctl_remove_cache(uc, address, address);

    current_run_state->restart=true;
    current_run_state->restart_address=new_address; //includes the +1 if it's thumb

    // We have to stop and start the emulation for an instruction change.
    uc_emu_stop(uc);
    delete_hook_code_fault_it(uc, current_run_state); 
}


void hook_code_fault_it(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    /*** This is where the faulting actually happens ****/
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    fault_rule_t *this_fault=&current_run_state->fault_rule;
    uint64_t fault_address = current_run_state->line_details_array[this_fault->instruction].address;
    fault_address=thumb_check_address(fault_address);

    #ifdef DEBUG
            printf_debug("hook_code_fault_it. Address 0x%" PRIx64 ". Count: %li\n", address, current_run_state->instruction_count);
    #endif

    // If we want to fault AFTER the instruction then we need to skip this once.
    if (fault_address == address && FAULT_AFTER)
    {
        return;
    }
    #ifdef DEBUG
            printf_debug("Faulting this line. Address 0x%" PRIx64 ". Count: %li\n", address, current_run_state->instruction_count);
    #endif

    // Do the faulting now
    uint64_t original_val=0, new_val=0, thing_to_fault=0;

    switch (this_fault->target)
    {
        case reg_ft:
            //read it
            thing_to_fault=uc_reg_from_int(this_fault->number);
            uc_reg_read(uc, thing_to_fault, &original_val);
            fprintf_output(current_run_state->file_fprintf, "Original register           : 0x%016lx \n", original_val);
            fprintf_output(current_run_state->file_fprintf, "Mask                        : 0x%016" PRIx64 "\n", this_fault->mask);

            //fault it
            new_val=fault_reg(this_fault->mask, this_fault->operation, original_val, size);
            break;
        case instruction_pointer_ft:  // THIS NEEDS PROPER TESTING!
            //read it
            thing_to_fault=binary_file_details->my_pc_reg;
            uc_reg_read(uc, thing_to_fault, &original_val);
            fprintf_output(current_run_state->file_fprintf, "Original instruction pointer: 0x%016lx\n", original_val);
            //fault it
            new_val=fault_instruction_reg(this_fault->operation, original_val, size);
            break;
        default:
            fprintf(stderr, "Failed to find valid target to fault: %s\n", target_to_string(this_fault->target));
            my_exit(-1);
    }
    
    // write it back (reg/instruction pointer all work the same)
    uc_reg_write(uc, thing_to_fault, &new_val);
    fprintf_output(current_run_state->file_fprintf, "Updated                     : 0x%016lx\n", new_val);
        
    // if we are using lifespan then we'll need the original value!
    this_fault->lifespan.original_target_value=original_val;
    if (this_fault->lifespan.count!=0)
    {
        this_fault->lifespan.live_count=this_fault->lifespan.count;
        my_uc_hook_add("hook_lifespan", uc, &current_run_state->hk_fault_lifespan, UC_HOOK_CODE, hook_live_fault_lifespan, current_run_state, 1, 0);
    }

    // we've done the fault - so set run state to faulted!!
    current_run_state->run_state=FAULTED_rs;

    // Check for equivalences
    if (current_run_state->stop_on_equivalence && (is_equivalent(uc, current_run_state)))
    {
            // If we've found an equivalence - stop the run - no point in doing something if we've done it before!
            current_run_state->run_state=EQUIVALENT_rs;
            my_uc_emu_stop(uc);
            return;
    }
    delete_hook_code_fault_it(uc, current_run_state);
}

void hook_code_fault_address(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    #ifdef DEBUG
            printf_debug("hook_code_fault_address - called Address 0x%" PRIx64 ". Count: %li\n", address, current_run_state->instruction_count);
    #endif

    if (current_run_state->in_fault_range==0)
    {
        return;
    }
    if (current_run_state->run_state==FAULTED_rs)
    {
        // Don't fault it more than once.
        return;
    }
    if (current_run_state->fault_rule.instruction != current_run_state->instruction_count)
    {
        // only fault the specific instruction
        return;
    }

    #ifdef DEBUG
            printf_debug("hook_code_fault_address - faulting. Address 0x%" PRIx64 ". Count: %li\n", address, current_run_state->instruction_count);
    #endif

    // set the address where this fault occurred
    current_run_state->fault_rule.faulted_address=address;

    // This next hook will do the actual fault. The reasoning for this: 
    // This hook is reached BEFORE the instruction has been executed. So if we want to bit flip the result of the execution we have to do it in the next instruction.
    my_uc_hook_add("hook_fault_it", uc, &current_run_state->hk_fault_it, UC_HOOK_CODE, hook_code_fault_it, current_run_state, 1, 0);
    // delete this hook
    delete_hook_code_fault_address(uc, current_run_state);
}

void hook_code_start_faults(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;

    #ifdef DEBUG
        printf_debug("hook_code_start_faults. Address: 0x%" PRIx64 "\n", address);
        fprintf(current_run_state->file_fprintf,"  <~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~>  IN fault range!!! 0x%" PRIx64 ". uc: %p\n",address,uc); //TEMPTEMP
    #endif
    // We've hit the first address for faulting.
    if (current_run_state->run_mode==eGOLDEN_rm)
    {
        // Only show the IN/OUT fault range if we're running the program to see all the instructions
        fprintf(current_run_state->file_fprintf,"  <~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~>  IN fault range!!! 0x%" PRIx64 "  <~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~>\n",address); //TEMPTEMP
    }
    current_run_state->in_fault_range=1;
    if (current_run_state->instruction_count==0)
    {
        // This is the first time we're in the faulting range
        current_run_state->instruction_count=1;  // We're off
    }
    // Add the hook that will start counting instructions. q
    if (current_run_state->hk_count_instructions==0)
    { 
        my_uc_hook_add("hook_count_instructions", uc, &current_run_state->hk_count_instructions, UC_HOOK_CODE, hook_count_instructions, current_run_state, 1, 0);
    }
}

void hook_code_stop_faults(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
#ifdef DEBUG
    printf("<~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~> OUT fault range!!! 0x%" PRIx64 ". uc: %p. Count: %li\n",address,uc,current_run_state->instruction_count); //TEMPTEMP
    printf_debug("hook_code_stop_faults. Address 0x%" PRIx64 "\n", address);
#endif

    if (current_run_state->run_mode==eGOLDEN_rm)
    {
        // Only show the IN/OUT fault range if we're running the program to see all the instructions
        fprintf(current_run_state->file_fprintf,"  <~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~> OUT fault range!!! 0x%" PRIx64 ". Count: %li\n",address,current_run_state->instruction_count); //TEMPTEMP
    }
    current_run_state->in_fault_range=0;
}

void hook_code_hard_stop(uc_engine *uc, uint64_t address, uint64_t size, void *user_data)
{
#ifdef DEBUG
    printf_debug("hook_code_hard_stop. Address 0x%" PRIx64 "\n", address);
#endif

    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    if (current_run_state->run_mode!=eFAULT_rm)
    {
        // The hard stops are areas that we shouldn't get to in 'normal mode' and so it's only of relevance if we're faulting.
        return;
    }

    current_run_state->run_state=HARD_STOP_rs;
    fprintf(current_run_state->file_fprintf, "~~~~ Reached a hard stop address 0x%" PRIx64 " ~~~~\n", address);

    delete_hook_count_instructions(uc, current_run_state);
    my_uc_emu_stop(uc);
}

void hook_intr(uc_engine *uc, uint64_t int_no, void *user_data)
{
#ifdef DEBUG
    printf_debug("hook_intr\n");
#endif
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    fprintf(current_run_state->file_fprintf, "Hook intr. Interrupt number: %li \n", int_no);
    current_run_state->run_state=INTERRUPT_rs;
    my_uc_emu_stop(uc);
}

void hook_mem_invalid(uc_engine *uc, uc_mem_type type, uint64_t address, uint64_t size, uint64_t value, void *user_data)
{
#ifdef DEBUG
    printf_debug("hook_mem_invalid. Address 0x%" PRIx64 "\n", address);
#endif
    current_run_state_t *current_run_state=(current_run_state_t *)user_data;
    if (type==UC_MEM_READ_UNMAPPED)
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: ʕ•ᴥ•ʔ  Oopsie    - reading from unmapped memory, address: 0x%016lx, value: 0x%" PRIx64 "!!\n", address, value);
    }
    else if (type==UC_MEM_WRITE_UNMAPPED)
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: /ᐠ.ꞈ.ᐟ\\  Hmm - writing to unmapped memory, address: 0x%" PRIx64 ", value: 0x%" PRIx64 "!! Count %li Skipping to end.\n", address, value,current_run_state->instruction_count);
    }
    else if (type==UC_MEM_FETCH_PROT)
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: ¯\\_(ツ)_/¯ Doh - invalid memory fetch from address: 0x%" PRIx64 ", value: 0x%" PRIx64 ". Count %" PRIx64 "!!\n", address, value,current_run_state->instruction_count);
    }
    else if (type==UC_MEM_FETCH_UNMAPPED)
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: Erm -  fetching from unmapped memory, address: 0x%" PRIx64 ", value: 0x%" PRIx64 ". Count %" PRIx64 "!! Skipping to end.\n", address, value,current_run_state->instruction_count);
    }
    else if (type==UC_MEM_WRITE_PROT)
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: WOT - write to non-writeable memory from address: 0x%" PRIx64 ", value: 0x%" PRIx64 "!!\n", address, value);
    }
    else if (type==UC_MEM_READ_PROT)
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: Huh - read from non-readable memory from address: 0x%" PRIx64 ", value: 0x%" PRIx64 "!!\n", address, value);
    }
    else
    {
        fprintf(current_run_state->file_fprintf, " >! Errored: Something something something - strange invalid memory error. Address: 0x%" PRIx64 ", value: 0x%" PRIx64 "!!\n", address, value);
    }

    current_run_state->run_state=ERRORED_rs;
    my_uc_emu_stop(uc);
}