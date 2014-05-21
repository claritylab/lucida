`timescale 1ns / 1ps
module GMM_accelerator(

     input  wire        aclk,
     input  wire [31:0] feature1 , mean1 , prec1 , feature2 , mean2 , prec2 ,
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
                        feature29, mean29, prec29,
     output wire [31:0] logDval
     );

     getScore_unit getScore(aclk,
          feature1 , mean1 , prec1 , feature2 , mean2 , prec2 ,
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
          feature29, mean29, prec29, logDval);

endmodule

