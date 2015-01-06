`timescale 1ns / 1ps
`include "GMM_accelerator.vh"
module getScore_unit(
     input  wire        aclk,
     input  wire [(`score_units - 1):0] feature_valid, mean_valid, prec_valid,
                                        score_ready,
     input  wire [(32*`score_units - 1):0] feature, mean, prec,
     output wire        logDval_valid,
     output wire [31:0] logDval
     `ifdef DEBUG
    ,output wire [(32*`score_units - 1):0] logDval_component,
     output wire [`score_units - 1:0] feature_ready, mean_ready, prec_ready,
                  score_valid
     `endif // DEBUG
     );
     
     `ifndef DEBUG
     wire [(32*`score_units - 1):0] logDval_component;
     //wire [32*(`score_units / 2):0] fp_adder_out;
     `endif //DEBUG

     wire [(`score_units / 2) - 1:0] for_add1_valid, fp_adder_ready, for_add1_a_ready, for_add1_b_ready;
     wire [32*(`score_units / 2) - 1:0] for_add1_out;
     wire [(`score_units / 4) - 1:0] for_add2_valid, for_add2_ready;
     wire [32*(`score_units / 4) - 1:0] for_add2_out;
     wire fp_adder22_valid, fp_adder23_valid, fp_adder24_valid, fp_adder25_valid, fp_adder26_valid, fp_adder27_valid, fp_adder28_valid;
     wire [31:0] fp_adder22_out, fp_adder23_out, fp_adder24_out, fp_adder25_out, fp_adder26_out, fp_adder27_out, fp_adder28_out;
     
     genvar i, j, k;
     generate
          for(i = 1; i <= `score_units; i = i + 1) begin : for_score //and 7 years ago
               score_unit score (
                    aclk, 1'b1, 1'b1, 1'b1, 1'b1,
                    feature[(32*i - 1):(32*(i-1))],
                    mean[(32*i - 1):(32*(i-1))],
                    prec[(32*i - 1):(32*(i-1))],
                    feature_ready[i - 1],
                    mean_ready[i - 1],
                    prec_ready[i - 1],
                    score_valid[i - 1],
                    logDval_component[(32*i - 1):(32*(i - 1))]
                    );
          end
          
          for(j = 1; j <= (`score_units / 2); j = j + 1) begin : for_add1
               fp_adder fp_adder (
                    .aclk                    (aclk),                                      // input aclk
                    .s_axis_a_tvalid         (score_valid[2*(j-1)]),                      // input s_axis_a_tvalid
                    .s_axis_a_tready         (for_add1_a_ready[j-1]),                     // output s_axis_a_tready
                    .s_axis_a_tdata          (logDval_component[(64*j - 33):(64*(j-1))]), // input [31 : 0] s_axis_a_tdata
                    .s_axis_b_tvalid         (score_valid[(2*j)-1]),                      // input s_axis_b_tvalid
                    .s_axis_b_tready         (for_add1_b_ready[j-1]),                     // output s_axis_b_tready
                    .s_axis_b_tdata          (logDval_component[(64*j - 1):(64*j) - 32]), // input [31 : 0] s_axis_b_tdata
                    .m_axis_result_tvalid    (for_add1_valid[j - 1]),                     // output m_axis_result_tvalid
                    .m_axis_result_tready    (fp_adder_ready[j - 1]),                     // input m_axis_result_tready
                    .m_axis_result_tdata     (for_add1_out[32*j - 1:32*(j-1)])            // output [31 : 0] m_axis_result_tdata
                    );
          end
          
          for(k = 1; k <= (`score_units / 4); k = k + 1) begin : for_add2
               fp_adder fp_adder15 (
                    .aclk                    (aclk),                                      // input aclk
                    .s_axis_a_tvalid         (for_add1_valid[2*(k-1)]),                   // input s_axis_a_tvalid
                    .s_axis_a_tready         (fp_adder_ready[2*(k-1)]),                   // output s_axis_a_tready
                    .s_axis_a_tdata          (for_add1_out[(64*k - 33):(64*(k-1))]),      // input [31 : 0] s_axis_a_tdata
                    .s_axis_b_tvalid         (for_add1_valid[2*k - 1]),                   // input s_axis_b_tvalid
                    .s_axis_b_tready         (fp_adder_ready[2*k - 1]),                   // output s_axis_b_tready
                    .s_axis_b_tdata          (for_add1_out[(64*k - 1):(64*k) - 32]),      // input [31 : 0] s_axis_b_tdata
                    .m_axis_result_tvalid    (for_add2_valid[k - 1]),                     // output m_axis_result_tvalid
                    .m_axis_result_tready    (for_add2_ready[k - 1]),                     // input m_axis_result_tready
                    .m_axis_result_tdata     (for_add2_out[32*k - 1:32*(k-1)])            // output [31 : 0] m_axis_result_tdata
                    );
          end
/* TODO: finish this:
          //currently only works for all 29 units
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
/*
     wire [31:0] fp_adder_out1 , fp_adder_out2 , fp_adder_out3 ,
                 fp_adder_out4 , fp_adder_out5 , fp_adder_out6 ,
                 fp_adder_out7 , fp_adder_out8 , fp_adder_out9 ,
                 fp_adder_out10, fp_adder_out11, fp_adder_out12,
                 fp_adder_out13, fp_adder_out14, fp_adder_out15,
                 fp_adder_out16, fp_adder_out17, fp_adder_out18,
                 fp_adder_out19, fp_adder_out20, fp_adder_out21,
                 fp_adder_out22, fp_adder_out23, fp_adder_out24,
                 fp_adder_out25, fp_adder_out26, fp_adder_out27,
                 fp_adder_out28;
*/                 
/*                 
     fp_adder fp_adder1 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[0]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*1 - 33):(64*(1-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[1]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*1 - 1):(64*1) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
  
     fp_adder fp_adder2 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[2]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*2 - 33):(64*(2-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[3]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*2 - 1):(64*2) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder3 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[4]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*3 - 33):(64*(3-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[5]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*3 - 1):(64*3) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder4 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[6]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*4 - 33):(64*(4-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[7]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*4 - 1):(64*4) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder5 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[8]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*5 - 33):(64*(5-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[9]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*5 - 1):(64*5) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          );
          
     fp_adder fp_adder6 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[10]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*6 - 33):(64*(6-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[11]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*6 - 1):(64*6) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
  
     fp_adder fp_adder7 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[12]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*7 - 33):(64*(7-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[13]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*7 - 1):(64*7) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder8 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[14]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*8 - 33):(64*(8-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[15]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*8 - 1):(64*8) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder9 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[16]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*9 - 33):(64*(9-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[17]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*9 - 1):(64*9) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder10 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[18]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*10 - 33):(64*(10-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[19]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*10 - 1):(64*10) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
          
     fp_adder fp_adder11 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[20]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*11 - 33):(64*(11-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[21]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*11 - 1):(64*11) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
  
     fp_adder fp_adder12 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[22]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*12 - 33):(64*(12-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[23]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*12 - 1):(64*12) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder13 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[24]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*13 - 33):(64*(13-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[25]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*13 - 1):(64*13) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder14 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[26]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (logDval_component[(64*14 - 33):(64*(14-1))]),// input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[27]),         // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(64*14 - 1):(64*14) - 32]),// input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_valid),        // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder1_ready),        // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
*/ /*    
     fp_adder fp_adder15 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (for_add1_valid[0]),      // input s_axis_a_tvalid
          .s_axis_a_tready         (fp_adder_ready[0]),      // output s_axis_a_tready
          .s_axis_a_tdata          (for_add1_out[(64*1 - 33):(64*(1-1))]),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (for_add1_valid[1]),      // input s_axis_b_tvalid
          .s_axis_b_tready         (fp_adder_ready[1]),      // output s_axis_b_tready
          .s_axis_b_tdata          (for_add1_out[(64*1 - 1):(64*1) - 32]),    // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder15_out_valid),   // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder??_ready),       // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder15_out)          // output [31 : 0] m_axis_result_tdata
          );
          
     fp_adder fp_adder16 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (for_add1_valid[2]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder1_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder1_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder1_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
  
     fp_adder fp_adder17 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (fp_adder1_out_valid),    // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder2_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder2_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder2_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder2_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder2_out)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder18 (
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
     
     fp_adder fp_adder19 (
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
     
     fp_adder fp_adder20 (
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
     fp_adder fp_adder21 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (score_valid[0]),         // input s_axis_a_tvalid
          .s_axis_a_tready         (),                       // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder1_in1),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder1_in2_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),                       // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder1_in2),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder1_out_valid),    // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),                   // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder1_out)           // output [31 : 0] m_axis_result_tdata
          ); 
*/  
     fp_adder fp_adder22 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (for_add2_valid[0]),      // input s_axis_a_tvalid
          .s_axis_a_tready         (for_add2_ready[0]),      // output s_axis_a_tready
          .s_axis_a_tdata          (for_add2_out[(64*1 - 33):(64*(1-1))]),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (for_add2_valid[1]),      // input s_axis_b_tvalid
          .s_axis_b_tready         (for_add2_ready[1]),      // output s_axis_b_tready
          .s_axis_b_tdata          (for_add2_out[(64*1 - 33):(64*(1-1))]),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder22_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder26_a_ready),     // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder22_out)          // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder23 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (for_add2_valid[2]),      // input s_axis_a_tvalid
          .s_axis_a_tready         (for_add2_ready[2]),      // output s_axis_a_tready
          .s_axis_a_tdata          (for_add2_out[(64*2 - 33):(64*(2-1))]),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (for_add2_valid[3]),      // input s_axis_b_tvalid
          .s_axis_b_tready         (for_add2_ready[3]),      // output s_axis_b_tready
          .s_axis_b_tdata          (for_add2_out[(64*2 - 33):(64*(2-1))]),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder23_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder26_b_ready),       // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder23_out)          // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder24 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (for_add2_valid[4]),      // input s_axis_a_tvalid
          .s_axis_a_tready         (for_add2_ready[4]),      // output s_axis_a_tready
          .s_axis_a_tdata          (for_add2_out[(64*3 - 33):(64*(3-1))]),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (for_add2_valid[5]),      // input s_axis_b_tvalid
          .s_axis_b_tready         (for_add2_ready[5]),      // output s_axis_b_tready
          .s_axis_b_tdata          (for_add2_out[(64*3 - 33):(64*(3-1))]),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder24_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder27_a_ready),       // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder24_out)          // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder25 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (for_add2_valid[6]),      // input s_axis_a_tvalid
          .s_axis_a_tready         (for_add2_ready[6]),      // output s_axis_a_tready
          .s_axis_a_tdata          (for_add2_out[(64*4 - 33):(64*(4-1))]),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (score_valid[28]),        // input s_axis_b_tvalid
          .s_axis_b_tready         (/*TBD*/),      // output s_axis_b_tready
          .s_axis_b_tdata          (logDval_component[(32*29 - 1):(32*(29 - 1))]),   // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder25_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder27_b_ready),       // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder25_out)          // output [31 : 0] m_axis_result_tdata
          ); 
          
     fp_adder fp_adder26 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (fp_adder22_valid),      // input s_axis_a_tvalid
          .s_axis_a_tready         (fp_adder26_a_ready),      // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder22_out),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder23_valid),      // input s_axis_b_tvalid
          .s_axis_b_tready         (fp_adder26_b_ready),      // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder23_out),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder26_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder28_a_ready),       // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder26_out)          // output [31 : 0] m_axis_result_tdata
          ); 
  
     fp_adder fp_adder27 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (fp_adder24_valid),      // input s_axis_a_tvalid
          .s_axis_a_tready         (fp_adder27_a_ready),      // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder24_out),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder25_valid),      // input s_axis_b_tvalid
          .s_axis_b_tready         (fp_adder27_b_ready),      // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder25_out),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (fp_adder27_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (fp_adder28_b_ready),       // input m_axis_result_tready
          .m_axis_result_tdata     (fp_adder27_out)          // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder28 (
          .aclk                    (aclk),                   // input aclk
          .s_axis_a_tvalid         (fp_adder26_valid),      // input s_axis_a_tvalid
          .s_axis_a_tready         (fp_adder28_a_ready),      // output s_axis_a_tready
          .s_axis_a_tdata          (fp_adder26_out),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (fp_adder27_valid),      // input s_axis_b_tvalid
          .s_axis_b_tready         (fp_adder28_b_ready),      // output s_axis_b_tready
          .s_axis_b_tdata          (fp_adder27_out),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (logDval_valid),       // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),       // input m_axis_result_tready
          .m_axis_result_tdata     (logDval)          // output [31 : 0] m_axis_result_tdata
          ); 
/*                 
     fp_adder fp_adder1 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (score_valid[0]),             // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component1),      // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component2),      // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out1)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder2 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component3),      // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component4),      // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out2)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder3 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component5),      // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component6),      // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out3)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder4 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component7),      // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component8),      // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out4)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder5 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component9),      // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component10),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out5)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder6 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component11),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component12),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out6)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder7 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component13),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component14),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out7)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder8 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component15),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component16),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out8)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder9 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component17),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component18),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out9)            // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder10(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component19),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component20),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out10)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder11(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component21),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component22),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out11)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder12(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component23),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component24),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out12)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder13(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component25),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component26),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out13)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder14(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDval_component27),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component28),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out14)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder15(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out1),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out2),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out15)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder16(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out3),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out4),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out16)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder17(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out5),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out6),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out17)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder18(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out7),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out8),           // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out18)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder19(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out9),           // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out10),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out19)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder20(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out11),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out12),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out20)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder21(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out13),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out14),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out21)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder22(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out15),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out16),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out22)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder23(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out17),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out18),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out23)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder24(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out19),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out20),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out24)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder25(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out21),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDval_component29),     // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out25)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder26(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out22),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out23),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out26)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder27(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out24),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out25),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out27)           // output [31 : 0] m_axis_result_tdata
          ); 
     
     fp_adder fp_adder28(
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
          .s_axis_a_tdata          (fp_adder_out26),          // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (1'b1),                    // input s_axis_b_tvalid
          .s_axis_b_tdata          (fp_adder_out27),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),                        // output m_axis_result_tvalid
          .m_axis_result_tdata     (fp_adder_out28)           // output [31 : 0] m_axis_result_tdata
          ); 
          
     assign logDval = fp_adder_out28;
*/
endmodule
