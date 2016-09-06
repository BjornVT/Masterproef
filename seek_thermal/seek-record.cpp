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

#define FPS 		8
#define NAMELWIR 	"outlwir.avi"
#define NAMERGB		"outrgb.avi"

using namespace std;
using namespace cv;

bool running = true;

void quitProgram(int sig);
static void help(char* name);

int main(int argc, char** argv) {
	LibSeek::seekCam seek;
	Mat frameLWIR;
	Mat frameRGB;
	VideoCapture cap;
	VideoWriter outLWIR;
	VideoWriter outRGB;
	
	signal(SIGTERM, &quitProgram);
	signal(SIGINT, &quitProgram);
	
	if(argc > 3 || argc == 1){
		throw runtime_error("Check args");
		help(argv[0]);
		exit(1);
	}
	else if (argc == 2){
		cap.open(0);
	}
	else {
		cap.open(argv[2]);
	}
	
	// check of webcam device number is correct
	if(!cap.isOpened())
	{
		throw runtime_error("Failed to open RGB camera");
		exit(1);
	}
	cap.set(CV_CAP_PROP_FPS, 0); //Hack so the VideoCapture buffer doesn't fill
	
	
	Size SRGB = Size((int) cap.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
					(int) cap.get(CV_CAP_PROP_FRAME_HEIGHT));
	Size SLWIR = Size((int)208, (int)156);
	
	//Opening video writers
	outLWIR.open(NAMELWIR, CV_FOURCC('X','2','6','4'), FPS, SLWIR, false);
	if (!outLWIR.isOpened())
    {
        throw runtime_error("Could not open the output video for write: LWIR");
        exit(1);
    }

	outRGB.open(NAMERGB, CV_FOURCC('X','2','6','4'), FPS, SRGB, true);
	if (!outRGB.isOpened())
    {
        throw runtime_error("Could not open the output video for write: RGB");
        exit(1);
    }

	cout << "Start recording" << endl;
	while(running) {
		cap.grab();
		seek.grab();
		cap.retrieve(frameRGB);
		frameLWIR = seek.retrieve();	
		
		frameLWIR.convertTo(frameLWIR, CV_8UC1, 1.0/32.0);
		equalizeHist(frameLWIR, frameLWIR); 

		outRGB << frameRGB;
		outLWIR << frameLWIR;
		
		frameLWIR.release();
		frameRGB.release();
		
		waitKey(1);
	}
	cout << "Finished recording" << endl;
}

void quitProgram(int sig)
{
	running = false;
}

static void help(char* name)
{
    cout
        << "------------------------------------------------------------------------------" << endl
        << "This program records the webcam and LWIR."                                      << endl
        << "Usage:"                                                                         << endl
        << "./" << name << " FPS [webcam nr]"                                               << endl
        << "------------------------------------------------------------------------------" << endl
        << endl;
}
