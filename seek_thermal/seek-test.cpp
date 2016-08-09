#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <cstdint>
#include <chrono>
#include <signal.h>
#include <opencv2/highgui/highgui.hpp>

#include "seek.hpp"
#include "debug.h"

#define FPS 15

using namespace std;
using namespace cv;

bool running = true;
const chrono::milliseconds frameFreq(999/FPS);

void quitProgram(int sig);

int main(int argc, char** argv) {
	LibSeek::seekCam seek;
	Mat frame;
	Mat frame2;
	VideoCapture cap;
	//chrono::time_point<chrono::high_resolution_clock> start, end;
	
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
	
	signal(SIGTERM, &quitProgram);
	//signal(SIGABRT, &quit);
	signal(SIGINT, &quitProgram);
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	namedWindow( "RGB",  WINDOW_NORMAL);
	cout << endl;

	while(running) {
	//for(int z=0; z<200; z++){
		//start = chrono::high_resolution_clock::now();
		cap.grab();
		seek.grab();
		cap.retrieve(frame2);
		frame = seek.retrieve();	
		
		frame.convertTo(frame, CV_8UC1, 1.0/32.0);
		//frame.convertTo(frame, CV_8UC1, 1.0/256.0);
		equalizeHist(frame, frame); 

		
		//imwrite( "res2.png", tot );
		imshow("LWIR", frame);
		imshow("RGB", frame2);
		
		frame.release();
		frame2.release();
		//tot.release();
		
		/*end = chrono::high_resolution_clock::now();
		chrono::milliseconds toWait = chrono::duration_cast<chrono::milliseconds>(frameFreq - (end - start));
		cout << toWait.count() << endl;
		* */
		//waitKey(frameFreq.count());
		waitKey(1);
	}
}

void quitProgram(int sig)
{
	running = false;
}
