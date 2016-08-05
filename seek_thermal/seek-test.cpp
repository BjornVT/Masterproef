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
	
	VideoCapture cap(1);
	// check of webcam device number is correct
	if(!cap.isOpened())
	{
		throw runtime_error("Failed to open RGB camera");
		exit(1);
	}
	

	while(true) {
	//for(int i=0; i<200; i++){
		for(int i=0; i<10; i++){
			cap >> frame2;
		}
		frame = seek.frame_acquire();
		
		for(int y=0; y<frame.rows; y++){
			for(int x=0; x<frame.cols; x++){
				frame.at<uint16_t>(Point(x, y)) = frame.at<uint16_t>(Point(x, y))<<shift;
			}
		}

		frame.convertTo(frame, CV_8UC1, 1.0/256.0 );
		equalizeHist( frame, frame ); 
		transpose(frame, frame);  
 		flip(frame, frame,0);
 		
 		cout << frame.cols << " " << frame.rows << endl;
 		cout << frame2.cols << " " << frame2.rows << endl;
		
		
		Mat tot(frame2.rows, frame.cols+frame2.cols, CV_8UC3);
		
		Mat left(tot, Rect(0, 0, frame2.cols, frame2.rows)); // Copy constructor
		frame2.copyTo(left);
		
		cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
		Mat right(tot, Rect(frame2.cols, 0, frame.cols, frame.rows)); // Copy constructor
		frame.copyTo(right);
		

		imshow( "LWIR", tot );
		imwrite( "res2.png", tot );
		imshow( "RGB", frame );
		
		frame.release();
		frame2.release();
		tot.release();
		
		waitKey(1000/30.0);
		//waitkey(0);
	}
	
}
