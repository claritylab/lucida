`timescale 1ns / 1ps
module score_unit(
     input  wire        aclk, data_valid,
     input  wire [31:0] feature, mean, prec,
     output wire [31:0] logDval,
     output wire        mult2_valid
     );
 
     wire [31:0] logDiff, logDiff_2;
     wire        sub1_valid, mult1_valid, mult1_a_ready, mult2_a_ready;
     
     fp_subtractor fp_sub1 (
          .aclk                    (aclk),          // input aclk
          .s_axis_a_tvalid         (data_valid),    // input s_axis_a_tvalid
          .s_axis_a_tready         (),              // output s_axis_a_tready
          .s_axis_a_tdata          (feature),       // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (data_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),              // output s_axis_b_tready
          .s_axis_b_tdata          (mean),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (sub_valid),     // output m_axis_result_tvalid
          .m_axis_result_tready    (mult1_a_ready), // input m_axis_result_tready
          .m_axis_result_tdata     (logDiff)        // output [31 : 0] m_axis_result_tdata
          );
                
     fp_multiplier fp_mult1 (
          .aclk                    (aclk),          // input aclk
          .s_axis_a_tvalid         (sub_valid),     // input s_axis_a_tvalid
          .s_axis_a_tready         (mult1_a_ready), // output s_axis_a_tready
          .s_axis_a_tdata          (logDiff),       // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (sub_valid),     // input s_axis_b_tvalid
          .s_axis_b_tready         (),              // output s_axis_b_tready
          .s_axis_b_tdata          (logDiff),       // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (mult1_valid),   // output m_axis_result_tvalid
          .m_axis_result_tready    (mult2_a_ready), // input m_axis_result_tready
          .m_axis_result_tdata     (logDiff_2)      // output [31 : 0] m_axis_result_tdata
          );
     
     fp_multiplier fp_mult2 (
          .aclk                    (aclk),          // input aclk
          .s_axis_a_tvalid         (mult1_valid),   // input s_axis_a_tvalid
          .s_axis_a_tready         (mult2_a_ready), // output s_axis_a_tready
          .s_axis_a_tdata          (logDiff_2),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (data_valid),    // input s_axis_b_tvalid
          .s_axis_b_tready         (),              // output s_axis_b_tready
          .s_axis_b_tdata          (prec),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (mult2_valid),   // output m_axis_result_tvalid
          .m_axis_result_tready    (1'b1),          // input m_axis_result_tready
          .m_axis_result_tdata     (logDval)        // output [31 : 0] m_axis_result_tdata
          );
     
endmodule