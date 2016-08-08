#include <stdio.h>
#include <string.h>
#include <opencv2/highgui/highgui.hpp>

#include "seek.hpp"
#include "debug.h"

using namespace std;
using namespace cv;

int shift=4;

int main(int argc, char** argv) {
	LibSeek::seekCam seek;
	Mat frame;
	Mat frame2;
	VideoCapture cap;
	
	
	if(argc > 2){
		throw runtime_error("To many arg");
		exit(1);
	}
	else if (argc == 1){
		cap.open(0);
	}
	else {
		cap.open(argv[1]);
	}
	
	// check of webcam device number is correct
	if(!cap.isOpened())
	{
		throw runtime_error("Failed to open RGB camera");
		exit(1);
	}
	cap.set(CV_CAP_PROP_FPS, 0); //Hack so the VideoCapture buffer doesn't fill
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	createTrackbar( "shift", "LWIR", &shift, 5, 0 );
	namedWindow( "RGB",  WINDOW_NORMAL);

	while(true) {
	//for(int i=0; i<200; i++){
		cap.grab();
		seek.grab();
		cap.retrieve(frame2);
		frame = seek.retrieve();
		
		for(int y=0; y<frame.rows; y++){
			for(int x=0; x<frame.cols; x++){
				frame.at<uint16_t>(Point(x, y)) = frame.at<uint16_t>(Point(x, y))<<shift;
			}
		}

		frame.convertTo(frame, CV_8UC1, 1.0/256.0 );
		equalizeHist( frame, frame ); 
		//transpose(frame, frame);  
 		//flip(frame, frame,0);
		
		Mat tot(frame2.rows, frame.cols+frame2.cols, CV_8UC3, 0.0);
		
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
