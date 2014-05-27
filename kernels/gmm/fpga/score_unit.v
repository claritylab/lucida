`timescale 1ns / 1ps
module score_unit(
     input  wire        aclk,
     input  wire [31:0] feature, mean, prec,
     output wire [31:0] logDval
     );
 
     wire [31:0] logDiff, logDiff_2;
     wire        ready;
     
     assign ready = 1'b1;
     
     fp_subtractor fp_sub1 (
          .aclk                    (aclk),          // input aclk
          .s_axis_a_tvalid         (ready),         // input s_axis_a_tvalid
          .s_axis_a_tdata          (feature),       // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (ready),         // input s_axis_b_tvalid
          .s_axis_b_tdata          (mean),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (logDiff)        // output [31 : 0] m_axis_result_tdata
          );
                
     fp_multiplier fp_mult1 (
          .aclk                    (aclk),          // input aclk
          .s_axis_a_tvalid         (ready),         // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDiff),       // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (ready),         // input s_axis_b_tvalid
          .s_axis_b_tdata          (logDiff),       // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (logDiff_2)      // output [31 : 0] m_axis_result_tdata
          );
     
     fp_multiplier fp_mult2 (
          .aclk                    (aclk),          // input aclk
          .s_axis_a_tvalid         (ready),         // input s_axis_a_tvalid
          .s_axis_a_tdata          (logDiff_2),     // input [31 : 0] s_axis_a_tdata
          .s_axis_b_tvalid         (ready),         // input s_axis_b_tvalid
          .s_axis_b_tdata          (prec),          // input [31 : 0] s_axis_b_tdata
          .m_axis_result_tvalid    (),              // output m_axis_result_tvalid
          .m_axis_result_tdata     (logDval)        // output [31 : 0] m_axis_result_tdata
          );
     
endmodule