#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <unicorn/unicorn.h>
#include "utils.h"
#include "unicorn_consts.h"
#include "configuration.h"
#include <capstone/capstone.h>



static unicorn_const_t const ucc_arch[]={{"arm",UC_ARCH_ARM },{"arm64",UC_ARCH_ARM64},{"x86", UC_ARCH_X86},{"tricore", UC_ARCH_TRICORE},{"riscv",UC_ARCH_RISCV}};
static const size_t ucc_arch_size = sizeof(ucc_arch)/sizeof(ucc_arch[0]);

static unicorn_const_t const ucc_mode[]={{"16",UC_MODE_16 },{"32",UC_MODE_32},{"64", UC_MODE_64},{"ARM1176", UC_MODE_ARM1176},{"ARM946", UC_MODE_ARM946},{"ARM926", UC_MODE_ARM926},{"ARMBE8", UC_MODE_ARMBE8},{"big endian",UC_MODE_BIG_ENDIAN},{"little endian",UC_MODE_LITTLE_ENDIAN},{"MCLASS", UC_MODE_MCLASS},{"MICRO", UC_MODE_MICRO},{"mips32",UC_MODE_MIPS32},{"mips64",UC_MODE_MIPS32R6},{"mips3",UC_MODE_MIPS3},{"mips64",UC_MODE_MIPS64},{"ppc32",UC_MODE_PPC32},{"ppc64",UC_MODE_PPC64},{"qpx",UC_MODE_QPX},{"riscv32",UC_MODE_RISCV32},{"riscv64",UC_MODE_RISCV64},{"sparc32",UC_MODE_SPARC32},{"sparc64",UC_MODE_SPARC64},{"THUMB", UC_MODE_THUMB},{"V8",UC_MODE_V8},{"V9",UC_MODE_V9}};
static const size_t ucc_mode_size = sizeof(ucc_mode)/sizeof(ucc_mode[0]); 

static unicorn_const_t const cc_arch[]={{"arm",CS_ARCH_ARM},{"arm64",CS_ARCH_ARM64},{"mips",CS_ARCH_MIPS},{"x86",CS_ARCH_X86},{"ppc",CS_ARCH_PPC},{"sparc",CS_ARCH_SPARC},{"sysz",CS_ARCH_SYSZ},{"xcore",CS_ARCH_XCORE},{"m68K",CS_ARCH_M68K},{"TMS320C64X",CS_ARCH_TMS320C64X},{"M680X",CS_ARCH_M680X},{"evm",CS_ARCH_EVM},{"MOS65XX",CS_ARCH_MOS65XX},{"max",CS_ARCH_MAX},{"riscv",UC_ARCH_RISCV},{"none",MY_CS_ARCH_NONE}};           
static const size_t cc_arch_size = sizeof(cc_arch)/sizeof(cc_arch[0]);

static unicorn_const_t const cc_mode[]={{"LITTLE_ENDIAN",CS_MODE_LITTLE_ENDIAN},{"ARM",CS_MODE_ARM},{"16",CS_MODE_16},{"32",CS_MODE_32},{"64",CS_MODE_64},{"THUMB",CS_MODE_THUMB},{"MCLASS",CS_MODE_MCLASS},{"V8",CS_MODE_V8},{"MICRO",CS_MODE_MICRO},{"MIPS3",CS_MODE_MIPS3},{"MIPS32R6",CS_MODE_MIPS32R6},{"MIPS2",CS_MODE_MIPS2},{"V9",CS_MODE_V9},{"QPX",CS_MODE_QPX},{"M68K_000",CS_MODE_M68K_000},{"M68K_010",CS_MODE_M68K_010},{"M68K_020",CS_MODE_M68K_020},{"M68K_030",CS_MODE_M68K_030},{"M68K_040",CS_MODE_M68K_040},{"M68K_060",CS_MODE_M68K_060},{"BIG_ENDIAN",CS_MODE_BIG_ENDIAN},{"MIPS32",CS_MODE_MIPS32},{"MIPS64",CS_MODE_MIPS64},{"M680X_6301",CS_MODE_M680X_6301},{"M680X_6309",CS_MODE_M680X_6309},{"M680X_6800",CS_MODE_M680X_6800},{"M680X_6801",CS_MODE_M680X_6801},{"M680X_6805",CS_MODE_M680X_6805},{"M680X_6808",CS_MODE_M680X_6808},{"M680X_6809",CS_MODE_M680X_6809},{"M680X_6811",CS_MODE_M680X_6811},{"M680X_CPU12",CS_MODE_M680X_CPU12},{"M680X_HCS08",CS_MODE_M680X_HCS08},{"none",MY_CS_MODE_NONE}};
static const size_t cc_mode_size = sizeof(cc_mode)/sizeof(cc_mode[0]);


/********************************** ARCH ************************************/
uint64_t unicorn_arch_int_from_name(const char* unicorn_arch_name)
{
    for (size_t i=0;i<ucc_arch_size;i++)
    {
        if (strcasecmp(unicorn_arch_name,ucc_arch[i].name)==0)
        {
            return ucc_arch[i].unicorn_value;
        }
    }
    fprintf(stderr, "Error. %s is not a valid architecture.\n",unicorn_arch_name);
    fprintf(stderr, "Valid architectures are: ");
    for (size_t i=0;i<ucc_arch_size;i++)
    {
        printf("%s ",ucc_arch[i].name);
    }
    printf("\n");
    my_exit(-1);
    return 666; //for compiler
}

const char* unicorn_arch_name_from_int(uint64_t unicorn_arch_int)
{
    for (size_t i=0;i<ucc_arch_size;i++)
    {
        if (ucc_arch[i].unicorn_value==unicorn_arch_int)
        {
            return ucc_arch[i].name;
        }
    }
    fprintf(stderr, "Error. %li is not a valid architecture constant in unicorn.\n",unicorn_arch_int);
    my_exit(-1);
    return "Whoops"; //for compiler

}


/********************************** MODE ************************************/
uint64_t unicorn_mode_int_from_name(const char* unicorn_mode_name)
{
    for (size_t i=0;i<ucc_mode_size;i++)
    {
        if (strcasecmp(unicorn_mode_name,ucc_mode[i].name)==0)
        {
            return ucc_mode[i].unicorn_value;
        }
    }
    fprintf(stderr, "Error. %s is not a valid mode.\n",unicorn_mode_name);
    fprintf(stderr, "Valid modes are: ");
    for (size_t i=0;i<ucc_mode_size;i++)
    {
        printf("%s ",ucc_mode[i].name);
    }
    printf("\n");
    my_exit(-1);
    return 666; //for compiler
}

const char* unicorn_mode_name_from_int(uint64_t unicorn_mode_int)
{
    for (size_t i=0;i<ucc_mode_size;i++)
    {
        if (ucc_mode[i].unicorn_value==unicorn_mode_int)
        {
            return ucc_mode[i].name;
        }
    }
    fprintf(stderr, "Error. %li is not a valid mode constant in unicorn.\n",unicorn_mode_int);
    my_exit(-1);
    return "Whoops"; //for compiler
}


/********************************** CAPSTONE MODE ************************************/
uint64_t capstone_mode_int_from_name(const char* capstone_mode_name)
{
    for (size_t i=0;i<cc_mode_size;i++)
    {
        if (strcasecmp(capstone_mode_name,cc_mode[i].name)==0)
        {
            return cc_mode[i].unicorn_value;
        }
    }
    fprintf(stderr, "Error. %s is not a valid capstone mode.\n",capstone_mode_name);
    fprintf(stderr, "Valid modes are: ");
    for (size_t i=0;i<cc_mode_size;i++)
    {
        printf("%s ",cc_mode[i].name);
    }
    printf("\n");
    my_exit(-1);
    return 666; //for compiler
}

const char* capstone_mode_name_from_int(uint64_t capstone_mode_int)
{
    for (size_t i=0;i<cc_mode_size;i++)
    {
        if (cc_mode[i].unicorn_value==capstone_mode_int)
        {
            return cc_mode[i].name;
        }
    }
    fprintf(stderr, "Error. %li is not a valid mode constant in capstone.\n",capstone_mode_int);
    my_exit(-1);
    return "Whoops"; //for compiler
}

/********************************** CAPSTONE arch ************************************/
uint64_t capstone_arch_int_from_name(const char* capstone_arch_name)
{
    for (size_t i=0;i<cc_arch_size;i++)
    {
        if (strcasecmp(capstone_arch_name,cc_arch[i].name)==0)
        {
            return cc_arch[i].unicorn_value;
        }
    }
    fprintf(stderr, "Error. %s is not a valid capstone arch.\n",capstone_arch_name);
    fprintf(stderr, "Valid capstone archs are: ");
    for (size_t i=0;i<cc_arch_size;i++)
    {
        printf("%s ",cc_arch[i].name);
    }
    printf("\n");
    my_exit(-1);
    return 666; //for compiler
}

const char* capstone_arch_name_from_int(uint64_t capstone_arch_int)
{
    for (size_t i=0;i<cc_arch_size;i++)
    {
        if (cc_arch[i].unicorn_value==capstone_arch_int)
        {
            return cc_arch[i].name;
        }
    }
    fprintf(stderr, "Error. %li is not a valid arch constant in capstone.\n",capstone_arch_int);
    my_exit(-1);
    return "Whoops"; //for compiler
}

/************************************************** uc_reg_from_int_arm and register_name_from_int_arm MUST MATCH ************************************/
uint64_t uc_reg_from_int_arm(uint64_t index)
{
    static uint64_t const arm_register_array[]={  
        UC_ARM_REG_APSR,UC_ARM_REG_APSR_NZCV,UC_ARM_REG_CPSR,UC_ARM_REG_FPEXC,UC_ARM_REG_FPINST,UC_ARM_REG_FPSCR,UC_ARM_REG_FPSCR_NZCV,UC_ARM_REG_FPSID,UC_ARM_REG_ITSTATE,UC_ARM_REG_LR,UC_ARM_REG_PC,UC_ARM_REG_SP,UC_ARM_REG_SPSR,UC_ARM_REG_D0,UC_ARM_REG_D1,UC_ARM_REG_D2,UC_ARM_REG_D3,UC_ARM_REG_D4,UC_ARM_REG_D5,UC_ARM_REG_D6,UC_ARM_REG_D7,UC_ARM_REG_D8,UC_ARM_REG_D9,UC_ARM_REG_D10,UC_ARM_REG_D11,UC_ARM_REG_D12,UC_ARM_REG_D13,UC_ARM_REG_D14,UC_ARM_REG_D15,UC_ARM_REG_D16,UC_ARM_REG_D17,UC_ARM_REG_D18,UC_ARM_REG_D19,UC_ARM_REG_D20,UC_ARM_REG_D21,UC_ARM_REG_D22,UC_ARM_REG_D23,UC_ARM_REG_D24,UC_ARM_REG_D25,UC_ARM_REG_D26,UC_ARM_REG_D27,UC_ARM_REG_D28,UC_ARM_REG_D29,UC_ARM_REG_D30,UC_ARM_REG_D31,UC_ARM_REG_FPINST2,UC_ARM_REG_MVFR0,UC_ARM_REG_MVFR1,UC_ARM_REG_MVFR2,UC_ARM_REG_Q0,UC_ARM_REG_Q1,UC_ARM_REG_Q2,UC_ARM_REG_Q3,UC_ARM_REG_Q4,UC_ARM_REG_Q5,UC_ARM_REG_Q6,UC_ARM_REG_Q7,UC_ARM_REG_Q8,UC_ARM_REG_Q9,UC_ARM_REG_Q10,UC_ARM_REG_Q11,UC_ARM_REG_Q12,UC_ARM_REG_Q13,UC_ARM_REG_Q14,UC_ARM_REG_Q15,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3,UC_ARM_REG_R4,UC_ARM_REG_R5,UC_ARM_REG_R6,UC_ARM_REG_R7,UC_ARM_REG_R8,UC_ARM_REG_R9,UC_ARM_REG_R10,UC_ARM_REG_R11,UC_ARM_REG_R12,UC_ARM_REG_S0,UC_ARM_REG_S1,UC_ARM_REG_S2,UC_ARM_REG_S3,UC_ARM_REG_S4,UC_ARM_REG_S5,UC_ARM_REG_S6,UC_ARM_REG_S7,UC_ARM_REG_S8,UC_ARM_REG_S9,UC_ARM_REG_S10,UC_ARM_REG_S11,UC_ARM_REG_S12,UC_ARM_REG_S13,UC_ARM_REG_S14,UC_ARM_REG_S15,UC_ARM_REG_S16,UC_ARM_REG_S17,UC_ARM_REG_S18,UC_ARM_REG_S19,UC_ARM_REG_S20,UC_ARM_REG_S21,UC_ARM_REG_S22,UC_ARM_REG_S23,UC_ARM_REG_S24,UC_ARM_REG_S25,UC_ARM_REG_S26,UC_ARM_REG_S27,UC_ARM_REG_S28,UC_ARM_REG_S29,UC_ARM_REG_S30,UC_ARM_REG_S31,UC_ARM_REG_IPSR,UC_ARM_REG_MSP,UC_ARM_REG_PSP,UC_ARM_REG_CONTROL,UC_ARM_REG_IAPSR,UC_ARM_REG_EAPSR,UC_ARM_REG_XPSR,UC_ARM_REG_EPSR,UC_ARM_REG_IEPSR,UC_ARM_REG_PRIMASK,UC_ARM_REG_BASEPRI,UC_ARM_REG_BASEPRI_MAX,UC_ARM_REG_FAULTMASK};

    /*  alias registers
    UC_ARM_REG_R13 = UC_ARM_REG_SP,    UC_ARM_REG_R14 = UC_ARM_REG_LR,    UC_ARM_REG_R15 = UC_ARM_REG_PC,          UC_ARM_REG_SB = UC_ARM_REG_R9,
    UC_ARM_REG_SL = UC_ARM_REG_R10,    UC_ARM_REG_FP = UC_ARM_REG_R11,    UC_ARM_REG_IP = UC_ARM_REG_R12,*/

    return arm_register_array[index];
}
const char* register_name_from_int_arm(uint64_t index)
{
    // can add more here //
    static const char* names[]=
    {   
        "apsr","apsr_nzcv","cpsr","fpexc","fpinst","fpscr","fpscr_nzcv","fpsid","itstate","lr","pc","sp","spsr","d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d11","d12","d13","d14","d15","d16","d17","d18","d19","d20","d21","d22","d23","d24","d25","d26","d27","d28","d29","d30","d31","fpinst2","mvfr0","mvfr1","mvfr2","q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","q13","q14","q15","r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","s0","s1","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11","s12","s13","s14","s15","s16","s17","s18","s19","s20","s21","s22","s23","s24","s25","s26","s27","s28","s29","s30","s31","ipsr","msp","psp","control","iapsr","eapsr","xpsr","epsr","iepsr","primask","basepri","basepri_max","faultmask"
    };  

    const int numEntries=sizeof(names) / sizeof(names[0]);
    return index < numEntries ? names[index] : "Invalid";
}
/************************************************** uc_reg_from_int_arm and register_name_from_int_arm MUST MATCH ************************************/

/************************************************** uc_reg_from_int_x86 and register_name_from_int_x86 MUST MATCH ************************************/
uint64_t uc_reg_from_int_x86(uint64_t index)
{
    static uint64_t const x86_register_array[]={UC_X86_REG_EAX, UC_X86_REG_ECX,  UC_X86_REG_EDX,  UC_X86_REG_EBX,
                                            UC_X86_REG_ESP, UC_X86_REG_EBP,  UC_X86_REG_ESI,  UC_X86_REG_EDI,
                                            UC_X86_REG_R8,  UC_X86_REG_R9,  UC_X86_REG_R10, UC_X86_REG_R11,
                                            UC_X86_REG_R12, UC_X86_REG_R13, UC_X86_REG_R14, UC_X86_REG_R15,
                                            UC_X86_REG_AH };

    return x86_register_array[index];
}
const char* register_name_from_int_x86(uint64_t index)
{
    static const char* names[]=
    {   
        "ax",      "cx",      "dx",       "bx",     "sp",      "bp",      "si",       "di",
        "r8",      "r9",      "r10",      "r11",    "r12",     "r13",     "r14",      "r15",   // r15
        "mmx0",    "mmx1",    "mmx2",     "mmx3",   "mmx4",    "mmx5",    "mmx6",     "mmx7",
        "fpr0",    "fpr1",    "fpr2",     "fpr3",   "fpr4",    "fpr5",    "fpr6",     "fpr7",
        "xmm0",    "xmm1",    "xmm2",     "xmm3",   "xmm4",    "xmm5",    "xmm6",     "xmm7",
        "xmm8",    "xmm9",    "xmm10",    "xmm11",  "xmm12",   "xmm13"    "xmm14",    "xmm15"
    };  

    const int numEntries=sizeof(names) / sizeof(names[0]);
    return index < numEntries ? names[index] : "Invalid";
}
/************************************************** uc_reg_from_int_x86 and register_name_from_int_x86 MUST MATCH ************************************/


/************************************************** uc_reg_from_int_tricore and register_name_from_int_tricore MUST MATCH ************************************/
uint64_t uc_reg_from_int_tricore(uint64_t index)
{
    static uint64_t const tricore_register_array[]={UC_TRICORE_REG_A0,UC_TRICORE_REG_A1,UC_TRICORE_REG_A2,UC_TRICORE_REG_A3,UC_TRICORE_REG_A4,UC_TRICORE_REG_A5,UC_TRICORE_REG_A6,UC_TRICORE_REG_A7,UC_TRICORE_REG_A8,UC_TRICORE_REG_A9,UC_TRICORE_REG_A10,UC_TRICORE_REG_A11,UC_TRICORE_REG_A12,UC_TRICORE_REG_A13,UC_TRICORE_REG_A14,UC_TRICORE_REG_A15,UC_TRICORE_REG_D0,UC_TRICORE_REG_D1,UC_TRICORE_REG_D2,UC_TRICORE_REG_D3,UC_TRICORE_REG_D4,UC_TRICORE_REG_D5,UC_TRICORE_REG_D6,UC_TRICORE_REG_D7,UC_TRICORE_REG_D8,UC_TRICORE_REG_D9,UC_TRICORE_REG_D10,UC_TRICORE_REG_D11,UC_TRICORE_REG_D12,UC_TRICORE_REG_D13,UC_TRICORE_REG_D14,UC_TRICORE_REG_D15,UC_TRICORE_REG_PCXI,UC_TRICORE_REG_PSW,UC_TRICORE_REG_PSW_USB_C,UC_TRICORE_REG_PSW_USB_V,UC_TRICORE_REG_PSW_USB_SV,UC_TRICORE_REG_PSW_USB_AV,UC_TRICORE_REG_PSW_USB_SAV,UC_TRICORE_REG_PC,UC_TRICORE_REG_SYSCON,UC_TRICORE_REG_CPU_ID,UC_TRICORE_REG_BIV,UC_TRICORE_REG_BTV,UC_TRICORE_REG_ISP,UC_TRICORE_REG_ICR,UC_TRICORE_REG_FCX,UC_TRICORE_REG_LCX,UC_TRICORE_REG_COMPAT,UC_TRICORE_REG_DPR0_U,UC_TRICORE_REG_DPR1_U,UC_TRICORE_REG_DPR2_U,UC_TRICORE_REG_DPR3_U,UC_TRICORE_REG_DPR0_L,UC_TRICORE_REG_DPR1_L,UC_TRICORE_REG_DPR2_L,UC_TRICORE_REG_DPR3_L,UC_TRICORE_REG_CPR0_U,UC_TRICORE_REG_CPR1_U,UC_TRICORE_REG_CPR2_U,UC_TRICORE_REG_CPR3_U,UC_TRICORE_REG_CPR0_L,UC_TRICORE_REG_CPR1_L,UC_TRICORE_REG_CPR2_L,UC_TRICORE_REG_CPR3_L,UC_TRICORE_REG_DPM0,UC_TRICORE_REG_DPM1,UC_TRICORE_REG_DPM2,UC_TRICORE_REG_DPM3,UC_TRICORE_REG_CPM0,UC_TRICORE_REG_CPM1,UC_TRICORE_REG_CPM2,UC_TRICORE_REG_CPM3,UC_TRICORE_REG_MMU_CON,UC_TRICORE_REG_MMU_ASI,UC_TRICORE_REG_MMU_TVA,UC_TRICORE_REG_MMU_TPA,UC_TRICORE_REG_MMU_TPX,UC_TRICORE_REG_MMU_TFA,UC_TRICORE_REG_BMACON,UC_TRICORE_REG_SMACON,UC_TRICORE_REG_DIEAR,UC_TRICORE_REG_DIETR,UC_TRICORE_REG_CCDIER,UC_TRICORE_REG_MIECON,UC_TRICORE_REG_PIEAR,UC_TRICORE_REG_PIETR,UC_TRICORE_REG_CCPIER,UC_TRICORE_REG_DBGSR,UC_TRICORE_REG_EXEVT,UC_TRICORE_REG_CREVT,UC_TRICORE_REG_SWEVT,UC_TRICORE_REG_TR0EVT,UC_TRICORE_REG_TR1EVT,UC_TRICORE_REG_DMS,UC_TRICORE_REG_DCX,UC_TRICORE_REG_DBGTCR,UC_TRICORE_REG_CCTRL,UC_TRICORE_REG_CCNT,UC_TRICORE_REG_ICNT,UC_TRICORE_REG_M1CNT,UC_TRICORE_REG_M2CNT,UC_TRICORE_REG_M3CNT};
    /* alias registers
    UC_TRICORE_REG_GA0 = UC_TRICORE_REG_A0, UC_TRICORE_REG_GA1 = UC_TRICORE_REG_A1, UC_TRICORE_REG_GA8 = UC_TRICORE_REG_A8, UC_TRICORE_REG_GA9 = UC_TRICORE_REG_A9, UC_TRICORE_REG_SP = UC_TRICORE_REG_A10, UC_TRICORE_REG_LR = UC_TRICORE_REG_A11, UC_TRICORE_REG_IA = UC_TRICORE_REG_A15, UC_TRICORE_REG_ID = UC_TRICORE_REG_D15,
    */
    return tricore_register_array[index];
}
const char* register_name_from_int_tricore(uint64_t index)
{
    static const char* names[]=
    {   
        "A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","A10","A11","A12","A13","A14","A15","D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","D10","D11","D12","D13","D14","D15","PCXI","PSW","PSW_USB_C","PSW_USB_V","PSW_USB_SV","PSW_USB_AV","PSW_USB_SAV","PC","SYSCON","CPU_ID","BIV","BTV","ISP","ICR","FCX","LCX","COMPAT","DPR0_U","DPR1_U","DPR2_U","DPR3_U","DPR0_L","DPR1_L","DPR2_L","DPR3_L","CPR0_U","CPR1_U","CPR2_U","CPR3_U","CPR0_L","CPR1_L","CPR2_L","CPR3_L","DPM0","DPM1","DPM2","DPM3","CPM0","CPM1","CPM2","CPM3","MMU_CON","MMU_ASI","MMU_TVA","MMU_TPA","MMU_TPX","MMU_TFA","BMACON","SMACON","DIEAR","DIETR","CCDIER","MIECON","PIEAR","PIETR","CCPIER","DBGSR","EXEVT","CREVT","SWEVT","TR0EVT","TR1EVT","DMS","DCX","DBGTCR","CCTRL","CCNT","ICNT","M1CNT","M2CNT","M3CNT"
    };  

    const int numEntries=sizeof(names) / sizeof(names[0]);
    return index < numEntries ? names[index] : "Invalid";
}
/************************************************** uc_reg_from_int_tricore and register_name_from_int_tricore MUST MATCH ************************************/



/************************************************** uc_reg_from_int_riscv and register_name_from_int_riscv MUST MATCH ************************************/
uint64_t uc_reg_from_int_riscv(uint64_t index)
{
    static uint64_t const riscv_register_array[]={
        UC_RISCV_REG_X0,UC_RISCV_REG_X1,UC_RISCV_REG_X2,UC_RISCV_REG_X3,UC_RISCV_REG_X4,UC_RISCV_REG_X5,UC_RISCV_REG_X6,UC_RISCV_REG_X7,UC_RISCV_REG_X8,UC_RISCV_REG_X9,UC_RISCV_REG_X10,UC_RISCV_REG_X11,UC_RISCV_REG_X12,UC_RISCV_REG_X13,UC_RISCV_REG_X14,UC_RISCV_REG_X15,UC_RISCV_REG_X16,UC_RISCV_REG_X17,UC_RISCV_REG_X18,UC_RISCV_REG_X19,UC_RISCV_REG_X20,UC_RISCV_REG_X21,UC_RISCV_REG_X22,UC_RISCV_REG_X23,UC_RISCV_REG_X24,UC_RISCV_REG_X25,UC_RISCV_REG_X26,UC_RISCV_REG_X27,UC_RISCV_REG_X28,UC_RISCV_REG_X29,UC_RISCV_REG_X30,UC_RISCV_REG_X31,UC_RISCV_REG_USTATUS,UC_RISCV_REG_UIE,UC_RISCV_REG_UTVEC,UC_RISCV_REG_USCRATCH,UC_RISCV_REG_UEPC,UC_RISCV_REG_UCAUSE,UC_RISCV_REG_UTVAL,UC_RISCV_REG_UIP,UC_RISCV_REG_FFLAGS,UC_RISCV_REG_FRM,UC_RISCV_REG_FCSR,UC_RISCV_REG_CYCLE,UC_RISCV_REG_TIME,UC_RISCV_REG_INSTRET,UC_RISCV_REG_HPMCOUNTER3,UC_RISCV_REG_HPMCOUNTER4,UC_RISCV_REG_HPMCOUNTER5,UC_RISCV_REG_HPMCOUNTER6,UC_RISCV_REG_HPMCOUNTER7,UC_RISCV_REG_HPMCOUNTER8,UC_RISCV_REG_HPMCOUNTER9,UC_RISCV_REG_HPMCOUNTER10,UC_RISCV_REG_HPMCOUNTER11,UC_RISCV_REG_HPMCOUNTER12,UC_RISCV_REG_HPMCOUNTER13,UC_RISCV_REG_HPMCOUNTER14,UC_RISCV_REG_HPMCOUNTER15,UC_RISCV_REG_HPMCOUNTER16,UC_RISCV_REG_HPMCOUNTER17,UC_RISCV_REG_HPMCOUNTER18,UC_RISCV_REG_HPMCOUNTER19,UC_RISCV_REG_HPMCOUNTER20,UC_RISCV_REG_HPMCOUNTER21,UC_RISCV_REG_HPMCOUNTER22,UC_RISCV_REG_HPMCOUNTER23,UC_RISCV_REG_HPMCOUNTER24,UC_RISCV_REG_HPMCOUNTER25,UC_RISCV_REG_HPMCOUNTER26,UC_RISCV_REG_HPMCOUNTER27,UC_RISCV_REG_HPMCOUNTER28,UC_RISCV_REG_HPMCOUNTER29,UC_RISCV_REG_HPMCOUNTER30,UC_RISCV_REG_HPMCOUNTER31,UC_RISCV_REG_CYCLEH,UC_RISCV_REG_TIMEH,UC_RISCV_REG_INSTRETH,UC_RISCV_REG_MCYCLE,UC_RISCV_REG_MINSTRET,UC_RISCV_REG_MCYCLEH,UC_RISCV_REG_MINSTRETH,UC_RISCV_REG_MVENDORID,UC_RISCV_REG_MARCHID,UC_RISCV_REG_MIMPID,UC_RISCV_REG_MHARTID,UC_RISCV_REG_MSTATUS,UC_RISCV_REG_MISA,UC_RISCV_REG_MEDELEG,UC_RISCV_REG_MIDELEG,UC_RISCV_REG_MIE,UC_RISCV_REG_MTVEC,UC_RISCV_REG_MCOUNTEREN,UC_RISCV_REG_MSTATUSH,UC_RISCV_REG_MUCOUNTEREN,UC_RISCV_REG_MSCOUNTEREN,UC_RISCV_REG_MHCOUNTEREN,UC_RISCV_REG_MSCRATCH,UC_RISCV_REG_MEPC,UC_RISCV_REG_MCAUSE,UC_RISCV_REG_MTVAL,UC_RISCV_REG_MIP,UC_RISCV_REG_MBADADDR,UC_RISCV_REG_SSTATUS,UC_RISCV_REG_SEDELEG,UC_RISCV_REG_SIDELEG,UC_RISCV_REG_SIE,UC_RISCV_REG_STVEC,UC_RISCV_REG_SCOUNTEREN,UC_RISCV_REG_SSCRATCH,UC_RISCV_REG_SEPC,UC_RISCV_REG_SCAUSE,UC_RISCV_REG_STVAL,UC_RISCV_REG_SIP,UC_RISCV_REG_SBADADDR,UC_RISCV_REG_SPTBR,UC_RISCV_REG_SATP,UC_RISCV_REG_HSTATUS,UC_RISCV_REG_HEDELEG,UC_RISCV_REG_HIDELEG,UC_RISCV_REG_HIE,UC_RISCV_REG_HCOUNTEREN,UC_RISCV_REG_HTVAL,UC_RISCV_REG_HIP,UC_RISCV_REG_HTINST,UC_RISCV_REG_HGATP,UC_RISCV_REG_PC
    };

    return riscv_register_array[index];
}
const char* register_name_from_int_riscv(uint64_t index)
{
    static const char* names[]=
    {   
    "X0","X1","X2","X3","X4","X5","X6","X7","X8","X9","X10","X11","X12","X13","X14","X15","X16","X17","X18","X19","X20","X21","X22","X23","X24","X25","X26","X27","X28","X29","X30","X31","USTATUS","UIE","UTVEC","USCRATCH","UEPC","UCAUSE","UTVAL","UIP","FFLAGS","FRM","FCSR","CYCLE","TIME","INSTRET","HPMCOUNTER3","HPMCOUNTER4","HPMCOUNTER5","HPMCOUNTER6","HPMCOUNTER7","HPMCOUNTER8","HPMCOUNTER9","HPMCOUNTER10","HPMCOUNTER11","HPMCOUNTER12","HPMCOUNTER13","HPMCOUNTER14","HPMCOUNTER15","HPMCOUNTER16","HPMCOUNTER17","HPMCOUNTER18","HPMCOUNTER19","HPMCOUNTER20","HPMCOUNTER21","HPMCOUNTER22","HPMCOUNTER23","HPMCOUNTER24","HPMCOUNTER25","HPMCOUNTER26","HPMCOUNTER27","HPMCOUNTER28","HPMCOUNTER29","HPMCOUNTER30","HPMCOUNTER31","CYCLEH","TIMEH","INSTRETH","MCYCLE","MINSTRET","MCYCLEH","MINSTRETH","MVENDORID","MARCHID","MIMPID","MHARTID","MSTATUS","MISA","MEDELEG","MIDELEG","MIE","MTVEC","MCOUNTEREN","MSTATUSH","MUCOUNTEREN","MSCOUNTEREN","MHCOUNTEREN","MSCRATCH","MEPC","MCAUSE","MTVAL","MIP","MBADADDR","SSTATUS","SEDELEG","SIDELEG","SIE","STVEC","SCOUNTEREN","SSCRATCH","SEPC","SCAUSE","STVAL","SIP","SBADADDR","SPTBR","SATP","HSTATUS","HEDELEG","HIDELEG","HIE","HCOUNTEREN","HTVAL","HIP","HTINST","HGATP","PC"
    };  

    const int numEntries=sizeof(names) / sizeof(names[0]);
    return index < numEntries ? names[index] : "Invalid";
}
/************************************************** uc_reg_from_int_x86 and register_name_from_int_x86 MUST MATCH ************************************/



/************************************************** uc_reg_from_int_x86_64 and register_name_from_int_x86_64 MUST MATCH ************************************/
uint64_t uc_reg_from_int_x86_64(uint64_t index)
{
    static uint64_t const x86_64_register_array[]={  UC_X86_REG_AH,UC_X86_REG_AL,UC_X86_REG_AX,UC_X86_REG_BH,UC_X86_REG_BL,UC_X86_REG_BP,UC_X86_REG_BPL,UC_X86_REG_BX,UC_X86_REG_CH,UC_X86_REG_CL,UC_X86_REG_CS,UC_X86_REG_CX,UC_X86_REG_DH,UC_X86_REG_DI,UC_X86_REG_DIL,UC_X86_REG_DL,UC_X86_REG_DS,UC_X86_REG_DX,UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_EDX,UC_X86_REG_EFLAGS,UC_X86_REG_EIP,UC_X86_REG_ES, UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_FPSW,UC_X86_REG_FS,UC_X86_REG_GS,UC_X86_REG_IP,UC_X86_REG_RAX,UC_X86_REG_RBP,UC_X86_REG_RBX,UC_X86_REG_RCX,UC_X86_REG_RDI,UC_X86_REG_RDX,UC_X86_REG_RIP,UC_X86_REG_RSI,UC_X86_REG_RSP,UC_X86_REG_SI,UC_X86_REG_SIL,UC_X86_REG_SP,UC_X86_REG_SPL,UC_X86_REG_SS,UC_X86_REG_CR0,UC_X86_REG_CR1,UC_X86_REG_CR2,UC_X86_REG_CR3,UC_X86_REG_CR4,UC_X86_REG_CR8,UC_X86_REG_DR0,UC_X86_REG_DR1,UC_X86_REG_DR2,UC_X86_REG_DR3,UC_X86_REG_DR4,UC_X86_REG_DR5,UC_X86_REG_DR6,UC_X86_REG_DR7,UC_X86_REG_FP0,UC_X86_REG_FP1,UC_X86_REG_FP2,UC_X86_REG_FP3,UC_X86_REG_FP4,UC_X86_REG_FP5,UC_X86_REG_FP6,UC_X86_REG_FP7,UC_X86_REG_K0,UC_X86_REG_K1,UC_X86_REG_K2,UC_X86_REG_K3,UC_X86_REG_K4,UC_X86_REG_K5,UC_X86_REG_K6,UC_X86_REG_K7,UC_X86_REG_MM0,UC_X86_REG_MM1,UC_X86_REG_MM2,UC_X86_REG_MM3,UC_X86_REG_MM4,UC_X86_REG_MM5,UC_X86_REG_MM6,UC_X86_REG_MM7,UC_X86_REG_R8,UC_X86_REG_R9,UC_X86_REG_R10,UC_X86_REG_R11,UC_X86_REG_R12,UC_X86_REG_R13,UC_X86_REG_R14,UC_X86_REG_R15,UC_X86_REG_ST0,UC_X86_REG_ST1,UC_X86_REG_ST2,UC_X86_REG_ST3,UC_X86_REG_ST4,UC_X86_REG_ST5,UC_X86_REG_ST6,UC_X86_REG_ST7,UC_X86_REG_R8B,UC_X86_REG_R9B,UC_X86_REG_R10B,UC_X86_REG_R11B,UC_X86_REG_R12B,UC_X86_REG_R13B,UC_X86_REG_R14B,UC_X86_REG_R15B,UC_X86_REG_R8D,UC_X86_REG_R9D,UC_X86_REG_R10D,UC_X86_REG_R11D,UC_X86_REG_R12D,UC_X86_REG_R13D,UC_X86_REG_R14D,UC_X86_REG_R15D,UC_X86_REG_R8W,UC_X86_REG_R9W,UC_X86_REG_R10W,UC_X86_REG_R11W,UC_X86_REG_R12W,UC_X86_REG_R13W,UC_X86_REG_R14W,UC_X86_REG_R15W};

    return x86_64_register_array[index];
}
const char* register_name_from_int_x86_64(uint64_t index)
{
    static const char* names[]=
    { "ah","al","ax","bh","bl","bp","bpl","bx","ch","cl","cs","cx","dh","di","dil","dl","ds","dx","eax","ebp","ebx","ecx","edi","edx","eflags","eip","es","esi","esp","fpsw","fs","gs","ip","rax","rbp","rbx","rcx","rdi","rdx","rip","rsi","rsp","si","sil","sp","spl","ss","cr0","cr1","cr2","cr3","cr4","cr8","dr0","dr1","dr2","dr3","dr4","dr5","dr6","dr7","fp0","fp1","fp2","fp3","fp4","fp5","fp6","fp7","k0","k1","k2","k3","k4","k5","k6","k7","mm0","mm1","mm2","mm3","mm4","mm5","mm6","mm7","r8","r9","r10","r11","r12","r13","r14","r15","st0","st1","st2","st3","st4","st5","st6","st7","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b","r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};  

    const int numEntries=sizeof(names) / sizeof(names[0]);
    return index < numEntries ? names[index] : "Invalid";
}
/************************************************** uc_reg_from_int_x86_64 and register_name_from_int_x86_64 MUST MATCH ************************************/

uint64_t uc_reg_from_int(uint64_t index)
{
    switch (binary_file_details->my_uc_arch)
    {
        case UC_ARCH_ARM:
            return uc_reg_from_int_arm(index);
        case UC_ARCH_X86:
            if (binary_file_details->my_uc_mode == UC_MODE_64)
            {
                return uc_reg_from_int_x86_64(index);
            }
            else
            {
                return uc_reg_from_int_x86(index);
            }
        case UC_ARCH_TRICORE:
            return uc_reg_from_int_tricore(index);
        case UC_ARCH_RISCV:
            return uc_reg_from_int_riscv(index);
        case UC_ARCH_ARM64:    
            fprintf(stderr, "Error. Not implemented.\n");
            my_exit(-1);
        default:
            fprintf(stderr, "Error. Cannot find register for architecture: %ld.\n",index);
            my_exit(-1);
    }
    return 666;  /// just here to keep the compiler happy.
}

const char* register_name_from_int(uint64_t index)
{
    switch (binary_file_details->my_uc_arch)
    {
        case UC_ARCH_X86:
            if (binary_file_details->my_uc_mode == UC_MODE_64)
            {
                return register_name_from_int_x86_64(index);
            }
            else
            {
                return register_name_from_int_x86(index);
            }
        case UC_ARCH_ARM:
            return register_name_from_int_arm(index);
        case UC_ARCH_TRICORE:
            return register_name_from_int_tricore(index);
        case UC_ARCH_RISCV:
            return register_name_from_int_riscv(index);
        case UC_ARCH_ARM64:
            fprintf(stderr, "Error. Not implemented.\n");
            my_exit(-1);
        default:
            fprintf(stderr, "Error. Cannot find register for architecture: %ld.\n",index);
            my_exit(-1);
    }
    return "";  /// just here to keep the compiler happy.
}

/**** getting the integers from the name  **/
uint64_t register_int_from_name_arm(const char* reg_name)
{
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        if (strcasecmp(reg_name,register_name_from_int_arm(i))==0)
        {
            return i;
        }
    }
    fprintf(stderr, "Error %s is not a valid register for arm.\n",reg_name);
    fprintf(stderr, "Valid choices are:");
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        fprintf(stderr, "%s ",register_name_from_int_arm(i));
    }
    fprintf(stderr, "\n");
    my_exit(-1);
    return 666;  /// just here to keep the compiler happy. 
}

uint64_t register_int_from_name_x86(const char* reg_name)
{
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        if (strcasecmp(reg_name,register_name_from_int_x86(i))==0)
        {
            return i;
        }
    }
    fprintf(stderr, "Error %s is not a valid register for x86.\n",reg_name);
    fprintf(stderr, "Valid choices are:");
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        fprintf(stderr, "%s ",register_name_from_int_x86(i));
    }
    fprintf(stderr, "\n");
    my_exit(-1);
    return 666;  /// just here to keep the compiler happy. 
}

uint64_t register_int_from_name_x86_64(const char* reg_name)
{
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        if (strcasecmp(reg_name,register_name_from_int_x86_64(i))==0)
        {
            return i;
        }
    }
    fprintf(stderr, "Error %s is not a valid register for x86. \n",reg_name);
    fprintf(stderr, "Valid choices are:");
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        fprintf(stderr, "%s ",register_name_from_int_x86_64(i));
    }
    fprintf(stderr, "\n");
    my_exit(-1);
    return 666;  /// just here to keep the compiler happy. 
}

uint64_t register_int_from_name_tricore(const char* reg_name)
{
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        if (strcasecmp(reg_name,register_name_from_int_tricore(i))==0)
        {
            return i;
        }
    }
    fprintf(stderr, "Error %s is not a valid register for tricore.\n",reg_name);
    fprintf(stderr, "Valid choices are:");
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        fprintf(stderr, "%s ",register_name_from_int_tricore(i));
    }
    fprintf(stderr, "\n");
    my_exit(-1);
    return 666;  /// just here to keep the compiler happy. 
}

uint64_t register_int_from_name_riscv(const char* reg_name)
{
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        if (strcasecmp(reg_name,register_name_from_int_riscv(i))==0)
        {
            return i;
        }
    }
    fprintf(stderr, "Error %s is not a valid register for RISC-V.\n",reg_name);
    fprintf(stderr, "Valid choices are:");
    for (int i=0;i<MAX_REGISTERS;i++)
    {
        fprintf(stderr, "%s ",register_name_from_int_riscv(i));
    }
    fprintf(stderr, "\n");
    my_exit(-1);
    return 666;  /// just here to keep the compiler happy. 
}


uint64_t register_int_from_name(const char* reg_name)
{
    switch (binary_file_details->my_uc_arch)
    {
    case UC_ARCH_ARM:
        return register_int_from_name_arm(reg_name);
    case UC_ARCH_X86:
        if (binary_file_details->my_uc_mode == UC_MODE_64)
        {
            return register_int_from_name_x86_64(reg_name);
        }
        else
        {
            return register_int_from_name_x86(reg_name);
        }
    case UC_ARCH_TRICORE:
        return register_int_from_name_tricore(reg_name);
    case UC_ARCH_RISCV:
        return register_int_from_name_riscv(reg_name);
    case UC_ARCH_ARM64:    
        fprintf(stderr, "Error. Not implemented.\n");
        my_exit(-1);
    default:
        fprintf(stderr, "Error. Cannot find register for architecture for register name: %s.\n",reg_name);
        my_exit(-1);
    }
    return 666;  /// just here to keep the compiler happy.
}
