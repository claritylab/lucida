`timescale 1ns / 1ps
module getScore_unit(
  input  wire        aclk,
  input  wire [31:0] feature1 , mean1 , prec1 ,
  input  wire [31:0] feature2 , mean2 , prec2 ,
  input  wire [31:0] feature3 , mean3 , prec3 ,
  input  wire [31:0] feature4 , mean4 , prec4 ,
  input  wire [31:0] feature5 , mean5 , prec5 ,
  input  wire [31:0] feature6 , mean6 , prec6 ,
  input  wire [31:0] feature7 , mean7 , prec7 ,
  input  wire [31:0] feature8 , mean8 , prec8 ,
  input  wire [31:0] feature9 , mean9 , prec9 ,
  input  wire [31:0] feature10, mean10, prec10,
  input  wire [31:0] feature11, mean11, prec11,
  input  wire [31:0] feature12, mean12, prec12,
  input  wire [31:0] feature13, mean13, prec13,
  input  wire [31:0] feature14, mean14, prec14,
  input  wire [31:0] feature15, mean15, prec15,
  input  wire [31:0] feature16, mean16, prec16,
  input  wire [31:0] feature17, mean17, prec17,
  input  wire [31:0] feature18, mean18, prec18,
  input  wire [31:0] feature19, mean19, prec19,
  input  wire [31:0] feature20, mean20, prec20,
  input  wire [31:0] feature21, mean21, prec21,
  input  wire [31:0] feature22, mean22, prec22,
  input  wire [31:0] feature23, mean23, prec23,
  input  wire [31:0] feature24, mean24, prec24,
  input  wire [31:0] feature25, mean25, prec25,
  input  wire [31:0] feature26, mean26, prec26,
  input  wire [31:0] feature27, mean27, prec27,
  input  wire [31:0] feature28, mean28, prec28,
  input  wire [31:0] feature29, mean29, prec29,
  output wire [31:0] logDval
  );
 
     /*
     wire [(32*feat_size - 1):0] logDval_component;
     genvar i;
     generate
          for(i = 0; i < feat_size; i = i + 1) begin
               score_unit score(feature, mean, prec, logDval_component[(32*i - 1):0]);
          end
     endgenerate
     */
     
     wire [31:0] logDval_component1 , logDval_component2 , logDval_component3 ,
                 logDval_component4 , logDval_component5 , logDval_component6 ,
                 logDval_component7 , logDval_component8 , logDval_component9 ,
                 logDval_component10, logDval_component11, logDval_component12,
                 logDval_component13, logDval_component14, logDval_component15,
                 logDval_component16, logDval_component17, logDval_component18,
                 logDval_component19, logDval_component20, logDval_component21,
                 logDval_component22, logDval_component23, logDval_component24,
                 logDval_component25, logDval_component26, logDval_component27,
                 logDval_component28, logDval_component29;

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
     wire        dontcare1;
       
     score_unit score1 (aclk, feature1 , mean1 , prec1 , logDval_component1);
     score_unit score2 (aclk, feature2 , mean2 , prec2 , logDval_component2);
     score_unit score3 (aclk, feature3 , mean3 , prec3 , logDval_component3);
     score_unit score4 (aclk, feature4 , mean4 , prec4 , logDval_component4);
     score_unit score5 (aclk, feature5 , mean5 , prec5 , logDval_component5);
     score_unit score6 (aclk, feature6 , mean6 , prec6 , logDval_component6);
     score_unit score7 (aclk, feature7 , mean7 , prec7 , logDval_component7);
     score_unit score8 (aclk, feature8 , mean8 , prec8 , logDval_component8);
     score_unit score9 (aclk, feature9 , mean9 , prec9 , logDval_component9);
     score_unit score10(aclk, feature10, mean10, prec10, logDval_component10);
     score_unit score11(aclk, feature11, mean11, prec11, logDval_component11);
     score_unit score12(aclk, feature12, mean12, prec12, logDval_component12);
     score_unit score13(aclk, feature13, mean13, prec13, logDval_component13);
     score_unit score14(aclk, feature14, mean14, prec14, logDval_component14);
     score_unit score15(aclk, feature15, mean15, prec15, logDval_component15);
     score_unit score16(aclk, feature16, mean16, prec16, logDval_component16);
     score_unit score17(aclk, feature17, mean17, prec17, logDval_component17);
     score_unit score18(aclk, feature18, mean18, prec18, logDval_component18);
     score_unit score19(aclk, feature19, mean19, prec19, logDval_component19);
     score_unit score20(aclk, feature20, mean20, prec20, logDval_component20);
     score_unit score21(aclk, feature21, mean21, prec21, logDval_component21);
     score_unit score22(aclk, feature22, mean22, prec22, logDval_component22);
     score_unit score23(aclk, feature23, mean23, prec23, logDval_component23);
     score_unit score24(aclk, feature24, mean24, prec24, logDval_component24);
     score_unit score25(aclk, feature25, mean25, prec25, logDval_component25);
     score_unit score26(aclk, feature26, mean26, prec26, logDval_component26);
     score_unit score27(aclk, feature27, mean27, prec27, logDval_component27);
     score_unit score28(aclk, feature28, mean28, prec28, logDval_component28);
     score_unit score29(aclk, feature29, mean29, prec29, logDval_component29);

     fp_adder fp_adder1 (
          .aclk                    (aclk),                    // input aclk
          .s_axis_a_tvalid         (1'b1),                    // input s_axis_a_tvalid
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

endmodule
