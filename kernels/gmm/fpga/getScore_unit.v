`timescale 1ns / 1ps
`include "GMM_accelerator.vh"
module getScore_unit(
     input  wire        aclk,
     input  wire [(32*`score_units - 1):0] feature, mean, prec,
     output wire        fp_adder1_out_valid,
     output wire [31:0] logDval
     `ifdef DEBUG
    ,output wire [(32*`score_units - 1):0] logDval_component,
     output wire [31:0] fp_adder1_out,
     output reg         score_unit1_counter,
     output reg  [31:0] score_unit1_reg,
     output wire [31:0] fp_adder1_in1, fp_adder1_in2, fp_adder1_out,
     output reg         fp_adder1_in2_valid
     `endif // DEBUG
     );
     
     `ifndef DEBUG
     wire [(32*`score_units - 1):0] logDval_component;
     //wire [32*(`score_units / 2):0] fp_adder_out;
     wire [31:0] fp_adder1_out;
     reg         score_unit1_counter;
     reg  [31:0] score_unit1_reg;
     wire [31:0] fp_adder1_in1, fp_adder1_in2, fp_adder1_out;
//               fp_adder2_in1, fp_adder2_in2, fp_adder2_out,
//               fp_adder3_in1, fp_adder3_in2, fp_adder3_out,
//               fp_adder4_in1, fp_adder4_in2, fp_adder4_out,
//               fp_adder5_in1, fp_adder5_in2, fp_adder5_out;
     reg         fp_adder1_in2_valid;
     `endif //DEBUG

     genvar i, j, k;
     generate
          for(i = 1; i <= `score_units; i = i + 1) begin : for_score //and 7 years ago
               score_unit score (
                    aclk, 1'b1, 
                    feature[(32*i - 1):(32*(i-1))],
                    mean[(32*i - 1):(32*(i-1))],
                    prec[(32*i - 1):(32*(i-1))],
                    logDval_component[(32*i - 1):(32*(i-1))],
                    );
          end
/* TODO: finish this:
                for(j = (`score_units / 2); j > 0; j = j / 2) begin : for_add_outer
                     for(k = 1; k <= j; k = k + 1) begin : for_add_inner
                              fp_adder fp_adder1 (
                         .aclk                    (aclk),                   // input aclk
                         .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
                         .s_axis_a_tready         (),                       // output s_axis_a_tready
                         .s_axis_a_tdata          (logDval_component[(64*j - 33):(64*(j-1))]),     // input [31 : 0] s_axis_a_tdata
                         .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
                         .s_axis_b_tready         (),                       // output s_axis_b_tready
                         .s_axis_b_tdata          (logDval_component[(64*j - 1):(64*j) - 32]),     // input [31 : 0] s_axis_b_tdata
                         .m_axis_result_tvalid    (),                       // output m_axis_result_tvalid
                         .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
                         .m_axis_result_tdata     (fp_adder_out[(32*j - 1): 32*(j - 1)])           // output [31 : 0] m_axis_result_tdata
                         );
                     end
                end
*/
     endgenerate
    
     fp_adder fp_adder1 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder1_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder1_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder1_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
/*  
     fp_adder fp_adder2 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder2_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder1_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder2_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder2_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder2_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder3 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder3_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder3_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder3_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder3_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder3_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder4 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder4_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder4_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder4_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder4_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder4_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder5 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder5_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder5_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder5_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder5_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder5_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder6 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component11),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component12),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out6)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder7 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component13),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component14),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out7)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder8 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component15),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component16),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out8)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder9 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component17),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component18),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out9)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder10(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component19),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component20),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out10)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder11(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component21),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component22),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out11)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder12(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component23),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component24),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out12)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder13(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component25),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component26),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out13)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder14(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component27),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component28),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out14)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder15(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out1),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out2),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out15)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder16(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out3),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out4),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out16)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder17(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out5),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out6),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out17)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder18(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out7),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out8),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out18)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder19(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out9),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out10),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out19)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder20(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out11),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out12),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out20)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder21(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out13),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out14),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out21)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder22(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out15),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out16),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out22)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder23(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out17),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out18),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out23)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder24(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out19),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out20),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out24)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder25(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out21),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component29),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out25)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder26(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out22),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out23),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out26)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder27(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out24),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out25),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out27)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder28(
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (1'b1),                   // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out26),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                   // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out27),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out28)           // output [31 : 0] m_axis_result_tdata
          ); 
          
     assign logDval = fp_adder_out28;
*/

     always @(negedge aclk) begin
          if(score_unit1_counter == 1'b0) begin
               score_unit1_reg <= logDval_component[31:0];
               score_unit1_counter <= 1'b1;
               fp_adder1_in2_valid <= 1'b1;
          end
          else begin
               score_unit1_counter <= 1'b0;
               fp_adder1_in2_valid <= 1'b0;
          end
     end
     
     assign fp_adder1_in1 = logDval_component[31:0];
     //assign fp_adder1_in2_valid = score_unit1_counter;
     assign fp_adder1_in2 = score_unit1_reg;
     
     assign logDval = fp_adder1_out;
endmodule
