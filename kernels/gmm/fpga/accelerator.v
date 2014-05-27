//Old, high level implementation.

parameter comp_size = 32;
parameter feat_size = 29;
parameter senone_size = 5120;

parameter logZero               = 32'hFF7FFFFF; //-3.4028235E38
parameter maxLogValue           = 32'h4AD89559; // 7097004.5
parameter minLogValue           = 32'hCAE328A4; //-7443538.0
parameter naturalLogBase        = 32'h38D1BD51; // 1.00011595E-4
parameter inverseNaturalLogBase = 32'h461C3B5D; // 9998.841 
parameter FLT_MAX               =

module GMM_core //top-level module (currently)
 (
  input  wire [31:0] feature, means, precs, weight, factor, score_in;
  output wire [31:0] score_out;
 );
     
     wire [31:0] logDval_unconverted, logDval_converted, logDval_fact;
     
     getScore(feature1 , mean1 , prec1 , feature2 , mean2 , prec2 ,
              feature3 , mean3 , prec3 , feature4 , mean4 , prec4 ,
              feature5 , mean5 , prec5 , feature6 , mean6 , prec6 ,
              feature7 , mean7 , prec7 , feature8 , mean8 , prec8 ,
              feature9 , mean9 , prec9 , feature10, mean10, prec10,
              feature11, mean11, prec11, feature12, mean12, prec12,
              feature13, mean13, prec13, feature14, mean14, prec14,
              feature15, mean15, prec15, feature16, mean16, prec16,
              feature17, mean17, prec17, feature18, mean18, prec18,
              feature19, mean19, prec19, feature20, mean20, prec20,
              feature21, mean21, prec21, feature22, mean22, prec22,
              feature23, mean23, prec23, feature24, mean24, prec24,
              feature25, mean25, prec25, feature26, mean26, prec26,
              feature27, mean27, prec27, feature28, mean28, prec28,
              feature29, mean29, prec29, logDval_unconverted);
     base_converter(logDval_unconverted, logDval_converted);
     add_fact(logDval_converted, factor, logDval_fact);
     log_diff(logDval_fact, weight, score_in, score_out);

endmodule

module score_unit
 (
  input  wire [31:0] feature, mean, prec;
  output reg  [31:0] logDval;
 );
 
     wire [31:0] logDiff, logDiff_2;
     
     fp_subtractor fp_sub1(feature, mean, logDiff);
     fp_multiplier fp_mult1(logDiff, logDiff, logDiff_2);
     fp_multiplier fp_mult2(logDiff_2, prec, logDval);
     
endmodule

module getScore
 (
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
     
     score_unit score1 (feature1 , mean1 , prec1 , logDval_component1);
     score_unit score2 (feature2 , mean2 , prec2 , logDval_component2);
     score_unit score3 (feature3 , mean3 , prec3 , logDval_component3);
     score_unit score4 (feature4 , mean4 , prec4 , logDval_component4);
     score_unit score5 (feature5 , mean5 , prec5 , logDval_component5);
     score_unit score6 (feature6 , mean6 , prec6 , logDval_component6);
     score_unit score7 (feature7 , mean7 , prec7 , logDval_component7);
     score_unit score8 (feature8 , mean8 , prec8 , logDval_component8);
     score_unit score9 (feature9 , mean9 , prec9 , logDval_component9);
     score_unit score10(feature10, mean10, prec10, logDval_component10);
     score_unit score11(feature11, mean11, prec11, logDval_component11);
     score_unit score12(feature12, mean12, prec12, logDval_component12);
     score_unit score13(feature13, mean13, prec13, logDval_component13);
     score_unit score14(feature14, mean14, prec14, logDval_component14);
     score_unit score15(feature15, mean15, prec15, logDval_component15);
     score_unit score16(feature16, mean16, prec16, logDval_component16);
     score_unit score17(feature17, mean17, prec17, logDval_component17);
     score_unit score18(feature18, mean18, prec18, logDval_component18);
     score_unit score19(feature19, mean19, prec19, logDval_component19);
     score_unit score20(feature20, mean20, prec20, logDval_component20);
     score_unit score21(feature21, mean21, prec21, logDval_component21);
     score_unit score22(feature22, mean22, prec22, logDval_component22);
     score_unit score23(feature23, mean23, prec23, logDval_component23);
     score_unit score24(feature24, mean24, prec24, logDval_component24);
     score_unit score25(feature25, mean25, prec25, logDval_component25);
     score_unit score26(feature26, mean26, prec26, logDval_component26);
     score_unit score27(feature27, mean27, prec27, logDval_component27);
     score_unit score28(feature28, mean28, prec28, logDval_component28);
     score_unit score29(feature29, mean29, prec29, logDval_component29);

     fp_adder fp_adder1 (logDval_component1 , logDval_component2 , fp_adder_out1 );
     fp_adder fp_adder2 (logDval_component3 , logDval_component4 , fp_adder_out2 );
     fp_adder fp_adder3 (logDval_component5 , logDval_component6 , fp_adder_out3 );
     fp_adder fp_adder4 (logDval_component7 , logDval_component8 , fp_adder_out4 );
     fp_adder fp_adder5 (logDval_component9 , logDval_component10, fp_adder_out5 );
     fp_adder fp_adder6 (logDval_component11, logDval_component12, fp_adder_out6 );
     fp_adder fp_adder7 (logDval_component13, logDval_component14, fp_adder_out7 );
     fp_adder fp_adder8 (logDval_component15, logDval_component16, fp_adder_out8 );
     fp_adder fp_adder9 (logDval_component17, logDval_component18, fp_adder_out9 );
     fp_adder fp_adder10(logDval_component19, logDval_component20, fp_adder_out10);
     fp_adder fp_adder11(logDval_component21, logDval_component22, fp_adder_out11);
     fp_adder fp_adder12(logDval_component23, logDval_component24, fp_adder_out12);
     fp_adder fp_adder13(logDval_component25, logDval_component26, fp_adder_out13);
     fp_adder fp_adder14(logDval_component27, logDval_component28, fp_adder_out14);
     
     fp_adder fp_adder15(fp_adder_out1 , fp_adder_out2 , fp_adder_out15);
     fp_adder fp_adder16(fp_adder_out3 , fp_adder_out4 , fp_adder_out16);
     fp_adder fp_adder17(fp_adder_out5 , fp_adder_out6 , fp_adder_out17);
     fp_adder fp_adder18(fp_adder_out7 , fp_adder_out8 , fp_adder_out18);
     fp_adder fp_adder19(fp_adder_out9 , fp_adder_out10, fp_adder_out19);
     fp_adder fp_adder20(fp_adder_out11, fp_adder_out12, fp_adder_out20);
     fp_adder fp_adder21(fp_adder_out13, fp_adder_out14, fp_adder_out21);
     
     fp_adder fp_adder22(fp_adder_out15, fp_adder_out16, fp_adder_out22);
     fp_adder fp_adder23(fp_adder_out17, fp_adder_out18, fp_adder_out23);
     fp_adder fp_adder24(fp_adder_out19, fp_adder_out20, fp_adder_out24);
     fp_adder fp_adder25(fp_adder_out21, logDval_component29, fp_adder_out25);
     
     fp_adder fp_adder26(fp_adder_out22, fp_adder_out23, fp_adder_out26);
     fp_adder fp_adder27(fp_adder_out24, fp_adder_out25, fp_adder_out27);
     
     fp_adder fp_adder28(fp_adder_out26, fp_adder_out27, fp_adder_out28);
     
     assign logDval = fp_adder_out28;

endmodule

module base_converter
 (
  input  wire [31:0] logDval_in;
  output wire [31:0] logDval_out;
 );
     
     fp_multiplier fp_mult_1(logDval_in, inverseNaturalLogBase, logDval_converted);
     
     always @* begin     
          if(logDval_in == logZero) begin
               logDval_out = logDval_in;
          end
          else begin
               logDval_out = logDval_converted;
          end
     end
endmodule

module add_fact
 (
  input  wire [31:0] logDval_in, factor;
  output wire [31:0] logDval_out;
 );

     reg [31:0] logDval_sub;
 
     fp_lessthan fp_lt1(logDval_sub, logZero, fp_lt1_out);
 
     always @* begin
          logDval_sub = logDval_in - factor;
          
          if (fp_comp1_out == 1'b1) begin
               logDval_out = logZero
          end
          else begin
               logDval_out = logDval_sub;
          end
     end
endmodule

module log_diff
 (
  input  wire [31:0] logDval, weight, score_in;
  output wire [31:0] score_out;
 );
     
     reg [31:0] logVal2, logHighestValue, logDifference, logInnerSummation, returnLogValue;
     
     always @* begin
          logVal2 = logDval + weight;
          logHighestValue = score_in;
          logDifference = score_in - logVal2;
          
          if(logDifference < 0) begin
               logHigestValue = logVal2;
               logDifference = -logDifference;
          end
          
          logValue = -logDifference
          
          if (logValue < minLogValue) begin
               logInnerSummation = 32'h00000000; //0.0
          end
          else if (logValue > maxLogValue) begin
               logInnerSummation = FLT_MAX;
          end
          else begin
               if (logValue == logZero) begin
                    logValue = logZero;
               end
               else begin
                    logValue = logValue * naturalLogBase;
               end
               logInnerSummation = exp(logValue);
          end

          logInnerSummation = logInnerSummation + 1.0;

          if (logInnerSummation <= 0.0) begin
               returnLogValue = logZero;
          end
          else begin
               returnLogValue = (log(logInnerSummation) * inverseNaturalLogBase);
               if (returnLogValue > FLT_MAX) begin
                    returnLogValue = FLT_MAX;
               end
               else if (returnLogValue < -FLT_MAX) begin
                    returnLogValue = -FLT_MAX;
               end
          end
          // sum log
          score = logHighestValue + returnLogValue;
          
     end