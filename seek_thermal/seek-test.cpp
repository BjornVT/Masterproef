#include <stdio.h>
#include <string.h>
#include <opencv2/highgui/highgui.hpp>

#include "seek.hpp"
#include "debug.h"

using namespace std;
using namespace cv;


int shift=4;

int main() {
	LibSeek::seekCam seek;
	Mat frame;
	Mat frame2;
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	createTrackbar( "shift", "LWIR", &shift, 5, 0 );
	namedWindow( "RGB",  WINDOW_NORMAL);

	while(true) {
	//for(int i=0; i<200; i++){
		frame = seek.frame_acquire();
		
		
		for(int y=0; y<frame.rows; y++){
			for(int x=0; x<frame.cols; x++){
				frame.at<uint16_t>(Point(x, y)) = frame.at<uint16_t>(Point(x, y))<<shift;
			}
		}

		frame.convertTo(frame, CV_8UC1, 1.0/256.0 );
		equalizeHist( frame, frame ); 

		imshow( "LWIR", frame );
		//imwrite( "res.png", frame );
		
		frame.release();
		waitKey(25);
	}
}
