#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <cstdint>
#include <chrono>
#include <signal.h>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <opencv2/highgui/highgui.hpp>

#include "seek.hpp"
#include "debug.h"
#include "camSettings.hpp"

#define RGB 0
#define LWIR 1

using namespace std;
using namespace cv;

/* Functions */
static void help(char* name);
void task(VideoCapture *cap);

/* Global vars for threading */
bool running = true;
Mat buffer;
mutex mtxCam;

/* Main */
int main(int argc, char** argv) {
	LibSeek::seekCam seek;
	VideoCapture cap;
	Mat frame[2];
	thread t;
	int nimg = 0;
	
	/* Is there a smart person on the other side? */
	if(argc != 3){
		throw runtime_error("Check args");
		help(argv[0]);
		exit(1);
	}

	/* Hi there camera's. Get to work */
	cap.open(stoi(argv[1]));
	seek.open(argv[2]);
	
	/* Everyone onboard? */
	if(!(cap.isOpened() && seek.isOpened()))
	{
		throw runtime_error("Failed to open camera");
		exit(1);
	}
	
	/* Start threading for sync input */
	cap.read(buffer);
	t = thread(task, &cap);
    
    namedWindow("RGB",  WINDOW_NORMAL);
    namedWindow("LWIR",  WINDOW_NORMAL);
	

	cout << "Finished init." << endl << "Start recording." << endl << "--------------------" << endl;
	while(running) {
		/* Seek LWIR and Normal camera */
		mtxCam.lock();
			if(!(seek.grab())){
				cout << "no more LWIR img to grab" << endl;
				return false;
			}
			buffer.copyTo(frame[RGB]);
        mtxCam.unlock();
		if(!(seek.retrieve(frame[LWIR]))){
			cout << "no more LWIR img to retrieve" << endl;
			return false;
		}
		
		frame[LWIR].convertTo(frame[LWIR], CV_8UC1, 1.0/110.0);
		equalizeHist(frame[LWIR], frame[LWIR]); 
		

		/* Look at that art */
		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[RGB]);
		
		/* Save the art for future generations */
		char name [32];
		sprintf(name, "out/rgb/%05d.png", nimg);
		imwrite(name, frame[RGB]);
		sprintf(name, "out/lwir/%05d.png", nimg);
		imwrite(name, frame[LWIR]);
		nimg ++;
		
		/* Release the kraken 
		 * ... or just the memory, that's also fine */
		for(int i=0; i<2; i++){
			frame[i].release();
		}
		
		/* Let's wait on tht slow seek */
		char c = char(waitKey(124));
		if(c == 'q'){
			running = false;
		}
	}
	running = false;
	t.join();
	seek.release();
	cap.release();
	cout << "Finished recording" << endl;
	
	return 0;
}

static void help(char* name)
{
	/* Let's explain it for the not so smart people */
    cout
        << "------------------------------------------------------------------------------" << endl
        << "This program records the webcam and LWIR."                                      << endl
        << "Usage:"                                                                         << endl
        << "./" << name << " webcamnr gradient.png"                                               << endl
        << "------------------------------------------------------------------------------" << endl
        << endl;
}

void task(VideoCapture *cap)
{
	/* Thread to keep the linux video buffer empty */
	while(running){
		cap->grab();
		if(mtxCam.try_lock()){
			cap->retrieve(buffer);
			mtxCam.unlock();
		}
		usleep(100);
	}
}
