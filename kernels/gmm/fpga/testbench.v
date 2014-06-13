`timescale 1ns / 1ps
`include "GMM_accelerator.vh"
module testbench();
     
     // Inputs
     
     `ifdef ENABLE_ROM_1KB
     reg        aclk;
     `endif //ENABLE_ROM_1KB
     
     `ifdef ENABLE_GETSCORE
     reg        aclk;
     reg [(32*`score_units - 1):0] feature, mean, prec;
     `endif //ENABLE_GETSCORE
     
     `ifdef ENABLE_BASE_CNV
     reg        aclk;
     reg [31:0] logDval_in;
     `endif //ENABLE_BASE_CNV
     
     `ifdef ENABLE_ADD_FACT
     reg        aclk;
     reg [31:0] factor;
     reg [31:0] logDval_in;
     `endif //ENABLE_ADD_FACT
     
     `ifdef ENABLE_LOG_DIFF
     reg        aclk;
     reg [31:0] logDval_in;
     `endif //ENABLE_LOG_DIFF

     // Outputs
     
     `ifdef ENABLE_ROM_1KB
     wire [31:0] data;
     wire [ 4:0] count;
     `endif //ENABLE_ROM_1KB
     
     `ifdef ENABLE_GETSCORE
     wire [31:0] logDval;
     wire        valid;
     `ifdef DEBUG
     wire [(32*`score_units - 1):0] logDval_component;
     wire [31:0] fp_adder1_out;
     wire        score_unit1_counter;
     wire [31:0] score_unit1_reg;
     wire [31:0] fp_adder1_in1, fp_adder1_in2, fp_adder1_out;
     wire        fp_adder1_in2_valid;
     `endif //DEBUG
     `endif //ENABLE_GETSCORE
     
     `ifdef ENABLE_BASE_CNV
     wire        isLogZero;
     wire [31:0] logDval_converted;
     wire [(`base_cnv_reg_length - 1):0] logZero_shift_reg;
     `endif //ENABLE_BASE_CNV
     
     `ifdef ENABLE_ADD_FACT
     wire        overflow;
     wire [31:0] logDval_sub;
     wire [(`add_fact_reg_length - 1):0] logZero_shift_reg;
     `endif //ENABLE_ADD_FACT
     
     `ifdef ENABLE_LOG_DIFF

     `endif //ENABLE_LOG_DIFF

     // Instantiate the Unit Under Test (UUT)
     GMM_accelerator uut (

          `ifdef ENABLE_ROM_1KB
          .aclk(aclk),
          .data(data)
          `ifdef DEBUG
         ,.count(count)
          `endif //DEBUG
          `endif //ENABLE_ROM_1KB
          
          `ifdef ENABLE_GETSCORE
          .aclk(aclk),
          .feature(feature),
          .mean(mean),
          .prec(prec),
          .logDval(logDval),
          .valid(valid)
          `ifdef DEBUG
         ,.logDval_component(logDval_component),
          .fp_adder1_out(fp_adder1_out),
          .score_unit1_counter(score_unit1_counter),
          .score_unit1_reg(score_unit1_reg),
          .fp_adder1_in1(fp_adder1_in1),
          .fp_adder1_in2(fp_adder1_in2),
          .fp_adder1_out(fp_adder1_out),
          .fp_adder1_in2_valid(fp_adder1_in2_valid)
          `endif //DEBUG
          `endif //ENABLE_GETSCORE
          
          `ifdef ENABLE_BASE_CNV
          .aclk(aclk),
          .isLogZero(isLogZero),
          .logDval_converted(logDval_converted),
          .logZero_shift_reg(logZero_shift_reg)
          `endif //ENABLE_BASE_CNV
          
          `ifdef ENABLE_ADD_FACT
          .aclk(aclk),
          .factor(factor),
          .logDval_in(logDval_in),
          .logDval_out(logDval_out)
          `ifdef DEBUG
         ,.overflow(overflow),
          .logDval_sub(logDval_sub)
          `endif //DEBUG
          `endif //ENABLE_ADD_FACT
          
          `ifdef ENABLE_LOG_DIFF
          .aclk(aclk),
          .logDval_in(logDval_in);
          `endif //ENABLE_LOG_DIFF
     );
     
     //Test procedure
     
     `ifdef ENABLE_ROM_1KB
     initial begin
     
          // Initialize Inputs
          aclk = 0;

          // Wait 100 ns for global reset to finish
          #100;
          
          // Add stimulus here

          for (i = 0; i < 200; i = i + 1) begin
               aclk <= 1'b0; #7;
               aclk <= 1'b1; #7;
          end
     end
     `endif //ENABLE_ROM_1KB
     
     `ifdef ENABLE_GETSCORE
     integer i;
     initial begin

          // Initialize Inputs
          aclk = 0;
          feature = 0;
          mean = 0;
          prec = 0;

          // Wait 100 ns for global reset to finish
          #100;

          // Stimulus
          feature <= 32'h3f800000; mean <=32'h40000000; prec = 32'h3f800000;

          for (i = 0; i < 200; i = i + 1) begin

               aclk <= 1'b1; #1.5;
               aclk <= 1'b0; #1.5;
               if (!(i % 2)) begin
                    mean <= 32'h40000000;
               end
               else begin
                      mean <= 32'h40400000;
               end
          end
     end
     `endif //ENABLE_GETSCORE

     `ifdef ENABLE_BASE_CNV
     integer i;
     initial begin

          // Initialize Inputs
          aclk = 0;
          logDval_in = 0;

          // Wait 100 ns for global reset to finish
          #100;

          // Stimulus
          logDval_in = 32'h3f800000;

          for (i = 0; i < 200; i = i + 1) begin

               aclk <= 1'b0; #7;
               aclk <= 1'b1; #7;
               if ((i % 8) == 0) begin
                    logDval_in <= 32'h00000000;
               end
               else if ((i % 8) == 1) begin
                    logDval_in <= 32'h3f800000;
               end
               else if ((i % 8) == 2) begin
                    logDval_in <= 32'h40000000;
               end
               else if ((i % 8) == 3) begin
                    logDval_in <= 32'h40400000;
               end
               else if ((i % 8) == 4) begin
                    logDval_in <= 32'h40800000;
               end
               else if ((i % 8) == 5) begin
                    logDval_in <= 32'h40a00000;
               end
               else if ((i % 8) == 6) begin
                    logDval_in <= 32'h40c00000;
               end
               else begin
                    logDval_in <= 32'hFF7FFFFF;
               end
          end
     end
     `endif //ENABLE_BASE_CNV
     
     `ifdef ENABLE_ADD_FACT
     integer i;
     initial begin

          // Initialize Inputs
          aclk = 0;
          logDval_in = 0;
          factor = 0;

          // Wait 100 ns for global reset to finish
          #100;

          // Add stimulus here
          logDval_in = 32'h00000000;
          factor = 32'h3f800000;

          for (i = 0; i < 200; i = i + 1) begin

               aclk <= 1'b0; #7;
               aclk <= 1'b1; #7;
               if ((i % 8) == 0) begin
                    logDval_in <= 32'h00000000;
                      factor <= 32'h3f800000;
               end
               else if ((i % 8) == 1) begin
                    logDval_in <= 32'h3f800000;
                      factor <= 32'h40000000;
               end
               else if ((i % 8) == 2) begin
                    logDval_in <= 32'h40000000;
               end
               else if ((i % 8) == 3) begin
                    logDval_in <= 32'h40400000;
                      factor <= 32'h40800000;
               end
               else if ((i % 8) == 4) begin
                    logDval_in <= 32'h40800000;
               end
               else if ((i % 8) == 5) begin
                    logDval_in <= 32'h40a00000;
                      factor <= 32'h40c00000;
               end
               else if ((i % 8) == 6) begin
                    logDval_in <= 32'h40c00000;
               end
               else begin
                      logDval_in <= 32'hFF7FFFFF;
                      factor <= 32'h7b4097ce;
               end
          end
     end
     `endif //ENABLE_ADD_FACT

endmodule
