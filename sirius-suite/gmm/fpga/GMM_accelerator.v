`timescale 1ns / 1ps
`include "GMM_accelerator.vh"
module GMM_accelerator(

     `ifdef ENABLE_ROM_1KB
     input  wire        aclk,
     output wire [31:0] data
     `ifdef DEBUG
    ,output wire [ 4:0] count
     `endif //DEBUG
     `endif //ENABLE_ROM_1KB
       
     `ifdef ENABLE_GETSCORE
     input  wire        aclk,
     input  wire [(32*`score_units - 1):0] feature, mean, prec,
     output wire [31:0] logDval,
     output wire        valid
     `ifdef DEBUG
    ,output wire [(32*`score_units - 1):0] logDval_component,
     output wire [31:0] fp_adder1_out,
     output wire        score_unit1_counter,
     output wire [31:0] score_unit1_reg,
     output wire [31:0] fp_adder1_in1, fp_adder1_in2, fp_adder1_out,
     output wire        fp_adder1_in2_valid
     `endif //DEBUG
     `endif //ENABLE_GETSCORE
       
     `ifdef ENABLE_BASE_CNV
     input  wire        aclk,
     output wire        isLogZero,
     output wire [31:0] logDval_converted,
     output wire [(`base_cnv_reg_length - 1):0] logZero_shift_reg
     `ifdef DEBUG
     `endif //DEBUG
     `endif //ENABLE_BASE_CNV
       
     `ifdef ENABLE_ADD_FACT
     input  wire        aclk,
     input  wire [31:0] factor
     `ifdef DEBUG
    ,output wire        overflow,
     output wire [31:0] logDval_sub
     `endif //DEBUG
     `endif //ENABLE_ADD_FACT
     );

     `ifdef ENABLE_ROM_1KB
     `ifndef DEBUG
     wire [4:0] count;
     `endif //DEBUG
     `endif //ENABLE_ROM_1KB
     
     `ifdef ENABLE_GETSCORE
     getScore_unit getScore (
          aclk, feature, mean, prec, valid, logDval
          `ifdef DEBUG
         ,logDval_component, fp_adder1_out, score_unit1_counter, score_unit1_reg,
          fp_adder1_in1, fp_adder1_in2, fp_adder1_out, fp_adder1_in2_valid
          `endif //DEBUG
          );
     `endif //ENABLE_GETSCORE                  

     `ifdef ENABLE_BASE_CNV
     base_converter base_cnv1(
          aclk, 1'b1, 1'b1, logDval_in, logDval_in_ready, valid, logDval
          `ifdef DEBUG
         ,logDval_converted, isLogZero, logZero_shift_reg
          `endif
          );
     `endif //ENABLE_BASE_CNV

     `ifdef ENABLE_ADD_FACT
       add_fact add_fact1(
          aclk, 1'b1, 1'b1, 1'b1, logDval_in, factor, logDval_in_ready,
          valid, logDval
          `ifdef DEBUG
         ,overflow, logDval_sub
          `endif //DEBUG
          );
       `endif //ENABLE_BASE_CNV
       
     `ifdef ENABLE_ROM_1KB
     rom_1kb your_instance_name (
          .clka   (aclk), // input clka
          .addra  (count), // input [4 : 0] addra
          .douta  (data) // output [31 : 0] douta
          );
     counter cntr (
          .clk (aclk), // input clk
          .q   (count) // output [4 : 0] q
          );
     `endif //ENABLE_ROM_1KB

endmodule

