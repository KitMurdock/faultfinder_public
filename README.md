# FaultFinder
Welcome to FaultFinder - a working, anonymised version.

# Requirements:
* [json-c](https://github.com/json-c/json-c)
* [Unicorn engine](https://www.unicorn-engine.org/) 
* [Capstone](https://www.capstone-engine.org/)


# Examples with three different architectures producing identical results

## Fault tinyAES compiled for ARM
    ./faultfinder experiments/tiny-AES-arm/jsons/fault.json

To view the outputs:
* ```cat experiments/tiny-AES-arm/outputs/*  | grep Output| sort | uniq -c```



## Fault tinyAES compiled for Infineon tricore
    ./faultfinder experiments/tiny-AES-tricore/jsons/fault.json

To view the outputs:
* ```cat experiments/tiny-AES-tricore/outputs/*  | grep Output| sort | uniq -c```

## Fault tinyAES compiled for x86 (64)
    ./faultfinder experiments/tiny-AES-x86/jsons/fault.json

To view the outputs:
* ```cat experiments/tiny-AES-x86/outputs/*  | grep Output| sort | uniq -c```
