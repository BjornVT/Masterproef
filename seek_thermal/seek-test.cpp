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

#define FPS 8

using namespace std;
using namespace cv;

bool running = true;

void quitProgram(int sig);
void drawTemp(Mat img, Point pt, double temp, Scalar color);

int main(int argc, char** argv) {
	LibSeek::seekCam seek;
	VideoCapture cap;
	Mat frame [4];
	
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
	signal(SIGINT, &quitProgram);
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	namedWindow( "RGB",  WINDOW_NORMAL);
	cout << "Finished init " << endl;

	while(running) {
	//for(int z=0; z<200; z++){
		cap.grab();
		seek.grab();
		cap.retrieve(frame[0]);
		seek.retrieve(frame[3]);
			
		frame[3].convertTo(frame[1], CV_8UC1, 1.0/32.0);
		//frame[3].convertTo(frame[1], CV_8UC1, 1.0/256.0);
		equalizeHist(frame[1], frame[1]); 
		
		
		cvtColor(frame[1], frame[2], COLOR_GRAY2BGR);
		double minVal, maxVal;
		Point minLoc, maxLoc;
		minMaxLoc(frame[3], &minVal, &maxVal, &minLoc, &maxLoc);
		cout << "Min:" << minVal << "\tLoc:" << minLoc << endl;
		cout << "Max:" << maxVal << "\tLoc:" << maxLoc << endl;
		drawTemp(frame[2], minLoc, minVal, Scalar(255, 0, 0));
		drawTemp(frame[2], maxLoc, maxVal, Scalar(0, 0, 255));
		

		
		//imwrite( "res2.png", tot );
		imshow("LWIR", frame[2]);
		imshow("RGB", frame[1]);
		
		frame[0].release();
		frame[1].release();
		frame[2].release();
		frame[3].release();
		
		char c = char(waitKey(120));
		if(c == 'q'){
			break;
		}	
	}
}

void quitProgram(int sig)
{
	running = false;
}

void drawTemp(Mat img, Point pt, double temp, Scalar color)
{
	char text[20] = {0};
	Point p1(pt.x-3, pt.y);
	Point p2(pt.x+3, pt.y);
	Point p3(pt.x, pt.y-3);
	Point p4(pt.x, pt.y+3);
	Point p5(pt.x+2, pt.y+8);
	
	temp = (temp - 5724.136) / 68.966;
	temp = round(temp);
	
	line(img, p1, p2, color);
	line(img, p3, p4, color);

	sprintf(text, "%d", (int)temp);
	putText(img, text, p5, FONT_HERSHEY_SIMPLEX, 0.3, color);
	
}

