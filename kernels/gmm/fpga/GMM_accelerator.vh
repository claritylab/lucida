`ifndef __GMM_ACCELERATOR_VH__
`define __GMM_ACCELERATOR_VH__

//Uncomment to enable debug
`define DEBUG

//Unit testing - Comment to disable modules
//`define ENABLE_ROM_1KB
`define ENABLE_GETSCORE
//`define ENABLE_BASE_CNV
//`define ENABLE_ADD_FACT
//`define ENABLE_LOG_DIFF

//Hardware settings
`define base_cnv_reg_length   7
`define add_fact_reg_length   11
`define score_units           1

//Acoustic model variables
`define comp_size   32
`define feat_size   29
`define senone_size 5120

//Constants
`define logZero               32'hFF7FFFFF //-3.4028235E38
`define maxLogValue           32'h4AD89559 // 7097004.5
`define minLogValue           32'hCAE328A4 //-7443538.0
`define naturalLogBase        32'h38D1BD51 // 1.00011595E-4
`define inverseNaturalLogBase 32'h461C3B5D // 9998.841 
`define negInf                32'hFF800000 // -infinity
//`define FLT_MAX             _            //Need to figure out what this is

`endif 