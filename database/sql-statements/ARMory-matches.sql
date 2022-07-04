select target_type,instruction,register_name, address_hex, fault_lifespan,operation, mask_hex
output_result, run_name, 
-- original_target_instruction, updated_target_instruction, 
original_target, updated_target,
run_result, extra_info,output_result, count(*)
from results
group by target_type,instruction, register_name, fault_lifespan, operation, mask, output_result
having count(*)=1 and not(extra_info like '%unmapped%') and not(updated_target_instruction='unable to disassemble') and register_name != 'xpsr' and instruction = 5352
order by target_type,instruction, register_name, fault_lifespan, operation, mask, output_result