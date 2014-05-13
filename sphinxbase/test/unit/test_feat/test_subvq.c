#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "feat.h"
#include "test_macros.h"
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
	static char const svspec[] = "1-12/14-25/0,13,26/27-38";
	int32 **subvecs, i, j, k, ncep;
	mfcc_t **in_feats, ***out_feats;
	feat_t *fcb;

	/* Test parsing of a subvector spec. */
	subvecs = parse_subvecs(svspec);
	TEST_ASSERT(subvecs);
	for (i = 0; i < 12; ++i) {
		TEST_EQUAL(subvecs[0][i], i+1);
	}
	for (i = 0; i < 12; ++i) {
		TEST_EQUAL(subvecs[1][i], i+14);
	}
	TEST_EQUAL(subvecs[2][0], 0);
	TEST_EQUAL(subvecs[2][1], 13);
	TEST_EQUAL(subvecs[2][2], 26);
	for (i = 0; i < 12; ++i) {
		TEST_EQUAL(subvecs[3][i], i+27);
	}

	/* Create a 1s_c_d_dd feature stream and split it into subvectors. */
	fcb = feat_init("1s_c_d_dd", CMN_NONE, 0, AGC_NONE, 1, 13);
	TEST_ASSERT(fcb);
	feat_set_subvecs(fcb, subvecs);

	in_feats = (mfcc_t **)ckd_alloc_2d_ptr(6, 13, data, sizeof(mfcc_t));
	out_feats = feat_array_alloc(fcb, 6);
	TEST_ASSERT(out_feats);

	ncep = 6;
	feat_s2mfc2feat_live(fcb, in_feats, &ncep, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < feat_dimension1(fcb); ++j) {
			for (k = 0; k < feat_dimension2(fcb, j); ++k) {
				printf("%.3f ", MFCC2FLOAT(out_feats[i][j][k]));
			}
			printf("\n");
		}
		printf("\n");
	}

	feat_array_free(out_feats);
	ckd_free(in_feats);
	feat_free(fcb);

	return 0;
}
