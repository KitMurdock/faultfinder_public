select target_type,instruction, address_hex, hit_count, mask_hex, operation, output_result, run_name, updated_target_instruction, run_result, extra_info,count(*)
from results
group by target_type,address, hit_count, mask, output_result
having count(*)=2
order by target_type,address, hit_count, mask, output_result