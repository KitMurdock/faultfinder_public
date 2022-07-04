CREATE VIEW armory AS  SELECT * FROM results where run_name = "ARMory";
CREATE VIEW faultfinder AS  SELECT * FROM results where run_name = "faultfinder";
select count(*) from armory;
select count(*) from faultfinder;

--update results set mask_hex = '0xf3af8000', mask=4088365056 where mask_hex ='0x8000f3af'
