#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "feat.h"
#include "ckd_alloc.h"

const mfcc_t data[6][13] = {
	{ FLOAT2MFCC(15.114), FLOAT2MFCC(-1.424), FLOAT2MFCC(-0.953),
	  FLOAT2MFCC(0.186), FLOAT2MFCC(-0.656), FLOAT2MFCC(-0.226),
	  FLOAT2MFCC(-0.105), FLOAT2MFCC(-0.412), FLOAT2MFCC(-0.024),
	  FLOAT2MFCC(-0.091), FLOAT2MFCC(-0.124), FLOAT2MFCC(-0.158), FLOAT2MFCC(-0.197)},
	{ FLOAT2MFCC(14.729), FLOAT2MFCC(-1.313), FLOAT2MFCC(-0.892),
	  FLOAT2MFCC(0.140), FLOAT2MFCC(-0.676), FLOAT2MFCC(-0.089),
	  FLOAT2MFCC(-0.313), FLOAT2MFCC(-0.422), FLOAT2MFCC(-0.058),
	  FLOAT2MFCC(-0.101), FLOAT2MFCC(-0.100), FLOAT2MFCC(-0.128), FLOAT2MFCC(-0.123)},
	{ FLOAT2MFCC(14.502), FLOAT2MFCC(-1.351), FLOAT2MFCC(-1.028),
	  FLOAT2MFCC(-0.189), FLOAT2MFCC(-0.718), FLOAT2MFCC(-0.139),
	  FLOAT2MFCC(-0.121), FLOAT2MFCC(-0.365), FLOAT2MFCC(-0.139),
	  FLOAT2MFCC(-0.154), FLOAT2MFCC(0.041), FLOAT2MFCC(0.009), FLOAT2MFCC(-0.073)},
	{ FLOAT2MFCC(14.557), FLOAT2MFCC(-1.676), FLOAT2MFCC(-0.864),
	  FLOAT2MFCC(0.118), FLOAT2MFCC(-0.445), FLOAT2MFCC(-0.168),
	  FLOAT2MFCC(-0.069), FLOAT2MFCC(-0.503), FLOAT2MFCC(-0.013),
	  FLOAT2MFCC(0.007), FLOAT2MFCC(-0.056), FLOAT2MFCC(-0.075), FLOAT2MFCC(-0.237)},
	{ FLOAT2MFCC(14.665), FLOAT2MFCC(-1.498), FLOAT2MFCC(-0.582),
	  FLOAT2MFCC(0.209), FLOAT2MFCC(-0.487), FLOAT2MFCC(-0.247),
	  FLOAT2MFCC(-0.142), FLOAT2MFCC(-0.439), FLOAT2MFCC(0.059),
	  FLOAT2MFCC(-0.058), FLOAT2MFCC(-0.265), FLOAT2MFCC(-0.109), FLOAT2MFCC(-0.196)},
	{ FLOAT2MFCC(15.025), FLOAT2MFCC(-1.199), FLOAT2MFCC(-0.607),
	  FLOAT2MFCC(0.235), FLOAT2MFCC(-0.499), FLOAT2MFCC(-0.080),
	  FLOAT2MFCC(-0.062), FLOAT2MFCC(-0.554), FLOAT2MFCC(-0.209),
	  FLOAT2MFCC(-0.124), FLOAT2MFCC(-0.445), FLOAT2MFCC(-0.352), FLOAT2MFCC(-0.400)},
};

int
main(int argc, char *argv[])
{
	feat_t *fcb;
	mfcc_t **in_feats, ***out_feats;
	int32 i, j, ncep;

	/* Test "raw" features without concatenation */
	fcb = feat_init("13", CMN_NONE, 0, AGC_NONE, 1, 13);

	in_feats = (mfcc_t **)ckd_alloc_2d_ptr(6, 13, data, sizeof(mfcc_t));
	out_feats = (mfcc_t ***)ckd_calloc_3d(6, 1, 13, sizeof(mfcc_t));
	ncep = 6;
	feat_s2mfc2feat_live(fcb, in_feats, &ncep, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < 13; ++j) {
			printf("%.3f ", MFCC2FLOAT(out_feats[i][0][j]));
		}
		printf("\n");
	}
	feat_free(fcb);
	ckd_free(in_feats);
	ckd_free_3d(out_feats);

	/* Test "raw" features with concatenation */
	fcb = feat_init("13:1", CMN_NONE, 0, AGC_NONE, 1, 13);

	in_feats = (mfcc_t **)ckd_alloc_2d_ptr(6, 13, data, sizeof(mfcc_t));
	out_feats = (mfcc_t ***)ckd_calloc_3d(8, 1, 39, sizeof(mfcc_t));
	ncep = 6;
	feat_s2mfc2feat_live(fcb, in_feats, &ncep, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < 39; ++j) {
			printf("%.3f ", MFCC2FLOAT(out_feats[i][0][j]));
		}
		printf("\n");
	}
	feat_free(fcb);

	/* Test 1s_c_d_dd features */
	fcb = feat_init("1s_c_d_dd", CMN_NONE, 0, AGC_NONE, 1, 13);
	ncep = 6;
	feat_s2mfc2feat_live(fcb, in_feats, &ncep, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < 39; ++j) {
			printf("%.3f ", MFCC2FLOAT(out_feats[i][0][j]));
		}
		printf("\n");
	}

	/* Verify that the deltas are correct. */
	for (i = 2; i < 4; ++i) {
		for (j = 0; j < 13; ++j) {
			if (fabs(MFCC2FLOAT(out_feats[i][0][13+j] - 
					    (out_feats[i+2][0][j]
					     - out_feats[i-2][0][j]))) > 0.01) {
				printf("Delta mismatch in [%d][%d]\n", i, j);
				return 1;
			}
		}
	}
	feat_free(fcb);
	ckd_free(in_feats);
	ckd_free_3d(out_feats);

	return 0;
}
