/* Johann Hauswald
 * jahausw@umich.edu
 * 2014
 */

#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/gpu.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/stitching/stitcher.hpp"

using namespace cv;
using namespace std;

struct timeval tv1, tv2;

#define NTHREADS 8
#define VTHREADS 20
#define HTHREADS 20
#define OVERLAP 0

vector<Mat> segs;
vector< vector<KeyPoint> > keys;
FeatureDetector *detector = new SurfFeatureDetector();
DescriptorExtractor *extractor = new SurfDescriptorExtractor();
int iterations;

vector<KeyPoint> exec_feature(const Mat& img)
{
	vector<KeyPoint> keypoints;
	detector->detect(img, keypoints);

	return keypoints;
}

Mat exec_desc(const Mat& img, vector<KeyPoint> keypoints)
{
	Mat descriptors;

	extractor->compute(img, keypoints, descriptors);

	descriptors.convertTo(descriptors, CV_32F);

	return descriptors;
}

vector<Mat> segment(const Mat& img)
{ 
    // int height_inc = img.size().height/NTHREADS;
    // int width_inc = img.size().width/NTHREADS;

    int height_inc = img.size().height/VTHREADS;
    int width_inc = img.size().width/HTHREADS;
    vector<Mat> segments;

	for(int r = 0; r < img.size().height; r += height_inc) {
		for(int c = 0; c < img.size().width; c += width_inc) {
            int rectoverlap = OVERLAP*2;
            Rect roi;
			roi.x = c;
			roi.y = r;
			roi.width = (c + width_inc > img.size().width) ? (img.size().width - c) : width_inc;
			roi.height  = (r + height_inc > img.size().height) ? (img.size().height - r) : height_inc;
			if(r == 0) { //top row
				if(c != 0) //top row
					roi.x -= OVERLAP;
				if(c == 0 || c == img.size().width - width_inc) //corners
					roi.width += OVERLAP;
				else
					roi.width += rectoverlap;
				roi.height += OVERLAP;
			}
			else if(c == 0) { //first column
				roi.y -= OVERLAP;
				if(r == img.size().height - height_inc) //bot left corner
					roi.height += OVERLAP;
				else
					roi.height += rectoverlap; //any first col segment
				roi.width += OVERLAP; //cst width
			}
			else if(r == img.size().height - height_inc) { //bot row
				roi.x -= OVERLAP;
				roi.y -= OVERLAP;
				if(c == img.size().width - width_inc) //bot right corner
					roi.width += OVERLAP;
				else
					roi.width += rectoverlap;
				roi.height += OVERLAP;
			}
			else if(c == img.size().width - width_inc) { //last col, corners already accounted for
				roi.x -= OVERLAP;
				roi.y -= OVERLAP;
				roi.width += OVERLAP;
				roi.height += rectoverlap;
			}
			else { //any middle image
				roi.x -= OVERLAP;
				roi.y -= OVERLAP;
				roi.width += rectoverlap;
				roi.height += rectoverlap;
			}
            Mat imgroi(img, roi);
			segments.push_back(imgroi);
		}
	}

	return segments;
}

void * desc_thread(void *tid)
{
	int  start, *mytid, end;
	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;

	// printf ("Thread %d doing iterations %d to %d\n", *mytid, start, end-1);

    for(int i = start; i < end; ++i)
        Mat testDesc = exec_desc(segs[i], keys[i]);

}

void * feat_thread(void *tid)
{
	int  start, *mytid, end;
	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;

	// printf ("Thread %d doing iterations %d to %d\n", *mytid, start, end-1);

    for(int i = start; i < end; ++i)
        vector<KeyPoint> priv_keys = exec_feature(segs[i]);
}

int main( int argc, char** argv )
{
    // data
    unsigned int runtimecut = 0;
    unsigned int runtimefeat = 0, runtimefeatseg = 0;
    unsigned int runtimedesc = 0, runtimedescseg = 0;
    unsigned int totaltime = 0, totaltimepar = 0;
    struct timeval tot1, tot2;

    // Generate test keys
    Mat img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);

    gettimeofday(&tv1,NULL);
    segs = segment(img);
    gettimeofday(&tv2,NULL);
    runtimecut = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);

    gettimeofday(&tot1,NULL);

    gettimeofday(&tv1,NULL);
    vector<KeyPoint> key = exec_feature(img);
    gettimeofday(&tv2,NULL);
    runtimefeat = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);

    gettimeofday(&tv1,NULL);
    Mat testDesc = exec_desc(img, key);
    gettimeofday(&tv2,NULL);
    runtimedesc = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);

    gettimeofday(&tot2,NULL);
    totaltime = (tot2.tv_sec-tot1.tv_sec)*1000000 + (tot2.tv_usec-tot1.tv_usec);

    // Gen keys for desc_thread
    for(int i = 0; i < segs.size(); ++i){
        keys.push_back(exec_feature(segs[i]));
    }

    // Parallel
    gettimeofday(&tot1,NULL);
	int start, tids[NTHREADS];
	pthread_t threads[NTHREADS];
    pthread_attr_t attr;
	iterations =  (segs.size() / NTHREADS);
	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Keys
    gettimeofday(&tv1,NULL);
    for (int i = 0; i<NTHREADS; i++){
        tids[i] = i;
        pthread_create(&threads[i], &attr, feat_thread, (void *) &tids[i]);
    }

    for (int i=0; i<NTHREADS; i++)
     	 pthread_join(threads[i], NULL);

    gettimeofday(&tv2,NULL);
    runtimefeatseg = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);

    // Desc
    gettimeofday(&tv1,NULL);
    for (int i = 0; i<NTHREADS; i++){
        tids[i] = i;
        pthread_create(&threads[i], &attr, desc_thread, (void *) &tids[i]);
    }

    for (int i=0; i<NTHREADS; i++)
     	 pthread_join(threads[i], NULL);

    gettimeofday(&tv2,NULL);
    runtimedescseg = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);

    gettimeofday(&tot2,NULL);
    totaltimepar = (tot2.tv_sec-tot1.tv_sec)*1000000 + (tot2.tv_usec-tot1.tv_usec);

    // Clean up
    delete detector;
    delete extractor;
    
    cout << "cut: " << (double)runtimecut/1000 << endl;
    cout << "feat speedup: " << (double)runtimefeat/runtimefeatseg << endl;
    cout << "desc speedup: " << (double)runtimedesc/runtimedescseg << endl;
    cout << "total speedup: " << (double)totaltime/(totaltimepar+runtimecut) << endl;

	return 0;
}
