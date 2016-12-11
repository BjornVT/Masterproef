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

#define RGB		0
#define LWIR	1

using namespace std;
using namespace cv;

/* Functions */
void drawTemp(Mat img, Point pt, double temp, Scalar color);
static void onMouse(int event, int x, int y, int z, void* in);
void task(VideoCapture *cap);
static void read(const FileNode& node, camSettings& x, const camSettings& default_value = camSettings());


Point3f pt_rgb(100, 100, 1);
Mat buffer;
mutex mtxCam;
bool running = true;

/* Main */
int main(int argc, char** argv) {
	LibSeek::seekCam seek;
	camSettings cams;
	VideoCapture cap;
	//VideoCapture seek;
	Mat frame [6];
	thread t;
    FileStorage fs;
    Mat map1[2], map2[2];
    
    int trackbar=240;
	

	//cap.open("outrgb4.avi");
	//seek.open("outlwir4.avi");
	
	cap.open(1);
	seek.open("gradient.png");
	// Starting the thread for reading cam img
	cap.read(buffer);
	t = thread(task, &cap);
	
	
	const string inputSettingsFile = argc > 2 ? argv[2] : "calib.yml.bkp";
	cout << "Trying to read lens calirations from calibration file." << endl;
	fs.open(inputSettingsFile, FileStorage::READ); // Read the camSettings
	if (!fs.isOpened())
	{
		cout << "Could not open the outputfile: \"" << inputSettingsFile << "\"" << endl;
		return -1;
	}
	fs["Calibration"] >> cams;
	fs.release();  
	if(!(cams.GoodCam() && cams.GoodH())){
		cerr << "Invalid settings to be read in. from calibration file." << endl;	
		return -1;
	} 
	cout << "Read lens calirations from file" << endl;
	cams.H.convertTo(cams.H,CV_32FC1,1,0); //NOW A IS FLOAT 	// check of webcam device number is correct
	if(!cap.isOpened())
	{
		throw runtime_error("Failed to open RGB camera");
		exit(1);
	}
	
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	namedWindow( "RGB",  WINDOW_NORMAL);
	
	setMouseCallback("RGB", onMouse, NULL);
	createTrackbar( "LWIR", "LWIR", &trackbar, 0xFF, NULL );
	
	
	initUndistortRectifyMap(cams.cameraMatrix[RGB], cams.distCoeffs[RGB], Mat(),
            getOptimalNewCameraMatrix(cams.cameraMatrix[RGB], cams.distCoeffs[RGB], Size(640, 480), 1, Size(640, 480), 0),
            Size(640, 480), CV_16SC2, map1[RGB], map2[RGB]);
	initUndistortRectifyMap(cams.cameraMatrix[LWIR], cams.distCoeffs[LWIR], Mat(),
            getOptimalNewCameraMatrix(cams.cameraMatrix[LWIR], cams.distCoeffs[LWIR], Size(208, 155), 1, Size(208, 155), 0),
            Size(208, 155), CV_16SC2, map1[LWIR], map2[LWIR]);  
	
	fs.open("output.yml", FileStorage::WRITE);
	
	cout << "Finished init " << endl;

	while(running) {
		/* Seek LWIR and Normal camera */
		mtxCam.lock();
			if(!(seek.grab())){
				cout << "no more LWIR img" << endl;
				return false;
			}
			buffer.copyTo(frame[RGB]);
        mtxCam.unlock();
		if(!(seek.retrieve(frame[LWIR]))){
			cout << "no more LWIR img" << endl;
			return false;
		}
		
		frame[LWIR].convertTo(frame[LWIR], CV_8UC1, 1.0/128);
		equalizeHist(frame[LWIR], frame[LWIR]);	

		
		
		/*
		Point minLoc, maxLoc;
		double minVal, maxVal;
		minMaxLoc(frame[LWIR], &minVal, &maxVal, &minLoc, &maxLoc);
		//cout << "min " << minVal << " " << minLoc << endl;
		//cout << "max " << maxVal << " " << maxLoc << endl;
			
		
		
		/*
		seek.grab();
		cap.grab();
		cap.retrieve(frame[RGB]);
		seek.retrieve(frame[LWIR]);
		*/
		//drawTemp(frame[LWIR], maxLoc, seek.getTemp(maxLoc), Scalar(0, 0, 255));
		//drawTemp(frame[LWIR], minLoc, seek.getTemp(minLoc), Scalar(255, 0, 0));

		remap(frame[RGB], frame[RGB], map1[RGB], map2[RGB], INTER_LINEAR);
        remap(frame[LWIR], frame[LWIR], map1[LWIR], map2[LWIR], INTER_LINEAR);
        
        cvtColor(frame[LWIR], frame[LWIR], CV_GRAY2BGR);
        //cvtColor(frame[RGB], frame[RGB], CV_GRAY2BGR);
        
        Mat m_rgb(pt_rgb);
		Mat m_lwir;
		m_lwir = cams.H * m_rgb;
		Point3f pt_lwir(m_lwir);
		circle(frame[LWIR], Point2f(pt_lwir.x, pt_lwir.y), 2, Scalar(0, 0, 255), -1, 8, 0);
        //cout << pt_rgb << " " << Point2f(pt_lwir.x, pt_lwir.y) << endl;

		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[RGB]);
		
		char c = char(waitKey(120));
		if(c == 'q'){
			running = false;
		}
		if( c == 's' || c == 'S'){
			imwrite( "predetectrgb.png", frame[RGB]);
			imwrite( "predetectlwir.png", frame[LWIR]);
			imwrite( "postdetectrgb.png", frame[2]);
			imwrite( "postdetectlwir.png", frame[3]);
		}
		
		frame[RGB].release();
		frame[LWIR].release();
		frame[2].release();
		frame[3].release();
		frame[4].release();
	}
	
	running = false;
	t.join();
	
	seek.release();
	cap.release();
	
	return 0;
	
}


void drawTemp(Mat img, Point pt, double temp, Scalar color)
{
	char text[20] = {0};
	Point p1(pt.x-3, pt.y);
	Point p2(pt.x+3, pt.y);
	Point p3(pt.x, pt.y-3);
	Point p4(pt.x, pt.y+3);
	Point p5(pt.x+2, pt.y+8);
	
	line(img, p1, p2, color);
	line(img, p3, p4, color);

	sprintf(text, "%d", (int)temp);
	putText(img, text, p5, FONT_HERSHEY_SIMPLEX, 0.3, color);
	
}


static void onMouse(int event, int x, int y, int z, void* in)
{
	if(event == EVENT_LBUTTONDOWN){
		cout << "Clicked " << x << " " << y << endl;
	}
	
	pt_rgb.x = x;
	pt_rgb.y = y;
}

void task(VideoCapture *cap)
{
	while(running){
		cap->grab();
		if(mtxCam.try_lock()){
			cap->retrieve(buffer);
			mtxCam.unlock();
		}
		usleep(100);
	}
}

static void read(const FileNode& node, camSettings& x, const camSettings& default_value = camSettings())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}
