//Module: fp_multiplier
//Description: This module takes in two IEEE-754 floating point numbers and multiplies them

module fp_multiplier
 (
  input  wire [31:0] fp_multiplier_op_1,
  input  wire [31:0] fp_multiplier_op_2,
  output reg  [31:0] fp_multiplier_out
 );
 
     //break the floats up into their components
     //mantissas and significands have extra bits to avoid overflow
     wire        s_bit_1, s_bit_2;
     reg         s_bit_out;
     wire [ 8:0] mantissa_1, mantissa_2;
     reg  [ 8:0] mantissa_out;
     wire [22:0] significand_1, significand_2;
     reg  [47:0] significand_out;
     
     assign s_bit_1 = fp_multiplier_op_1[31];
     assign s_bit_2 = fp_multiplier_op_2[31];
     assign mantissa_1 = {1'b0, fp_multiplier_op_1[30:23]};
     assign mantissa_2 = {1'b0, fp_multiplier_op_2[30:23]};
     assign significand_1 = fp_multiplier_op_1[22:0];
     assign significand_2 = fp_multiplier_op_2[22:0];
     
     always @* begin
          //calculate the sign bit
          s_bit_out = s_bit_1 ^ s_bit_2;
          
          //calculate the significand
          significand_out = {1'b1, significand_1} * {1'b1, significand_2};
                
          //if either input is exactly 0, then the output is 0
          if ((mantissa_1 == 8'b0) && (significand_1 == 23'b0) ||
              (mantissa_2 == 8'b0) && (significand_2 == 23'b0)) begin
               mantissa_out = 9'b0;
          end
          else begin
               //calculate the exponent, accounting for normalization and removing bias
               mantissa_out = mantissa_1 + mantissa_2 + significand_out[47] - 8'd127;
          end
                
          //assemble the output
          fp_multiplier_out[31] = s_bit_out;
          fp_multiplier_out[30:23] = mantissa_out[7:0];
                
          //normalize the significand
          if(significand_out[47] == 1'b1) begin
               fp_multiplier_out[22:0] = significand_out[46:24];
          end
          else begin
               fp_multiplier_out[22:0] = significand_out[45:23];
          end
     end
endmodule