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

#define FPS		8
#define RGB		0
#define LWIR	1

using namespace std;
using namespace cv;

bool running = true;

void drawTemp(Mat img, Point pt, double temp, Scalar color);
static void onMouse(int event, int x, int y, int z, void* in);
void degradient(Mat *img);
void meanBlock(Mat img, Mat *means, Size s);

static void read(const FileNode& node, camSettings& x, const camSettings& default_value = camSettings())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}

int main(int argc, char** argv) {
	camSettings cams;
	VideoCapture cap;
	VideoCapture seek;
	Mat frame [3];
    FileStorage fs;
    Mat map1[2], map2[2];
    int nimg = 0;
	
	if(argc != 4){
		throw runtime_error("Check args");
		exit(1);
	}
	
	cap.open(argv[1]);
	seek.open(argv[2]);
	if(!(cap.isOpened() && seek.isOpened()))
	{
		throw runtime_error("Failed to open input");
		exit(1);
	}
	
	cout << "Trying to read lens calirations from calibration file." << endl;
	fs.open(argv[3], FileStorage::READ); // Read the camSettings
	if (!fs.isOpened())
	{
		cout << "Could not open the outputfile: \"" << argv[3] << "\"" << endl;
		return -1;
	}
	fs["Calibration"] >> cams;
	fs.release();  
	if(!(cams.GoodCam() && cams.GoodH())){
		cerr << "Invalid settings to be read in. from calibration file." << endl;	
		return -1;
	}
	cams.H.convertTo(cams.H,CV_32FC1,1,0); //NOW A IS FLOAT 
	
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	namedWindow( "RGB",  WINDOW_NORMAL);
	
	
	initUndistortRectifyMap(cams.cameraMatrix[RGB], cams.distCoeffs[RGB], Mat(),
            getOptimalNewCameraMatrix(cams.cameraMatrix[RGB], cams.distCoeffs[RGB], Size(640, 480), 1, Size(640, 480), 0),
            Size(640, 480), CV_16SC2, map1[RGB], map2[RGB]);
	initUndistortRectifyMap(cams.cameraMatrix[LWIR], cams.distCoeffs[LWIR], Mat(),
            getOptimalNewCameraMatrix(cams.cameraMatrix[LWIR], cams.distCoeffs[LWIR], Size(208, 155), 1, Size(208, 155), 0),
            Size(208, 155), CV_16SC2, map1[LWIR], map2[LWIR]);
            
    Size SRGB = Size((int) cap.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
					(int) cap.get(CV_CAP_PROP_FRAME_HEIGHT));
	
	
	cout << "Finished init " << endl;
	while(true) {
		seek.grab();
		cap.grab();
		cap.retrieve(frame[RGB]);
		seek.retrieve(frame[LWIR]);
		
		cvtColor(frame[LWIR], frame[LWIR], CV_BGR2GRAY);
		
		remap(frame[RGB], frame[RGB], map1[RGB], map2[RGB], INTER_LINEAR);
        remap(frame[LWIR], frame[LWIR], map1[LWIR], map2[LWIR], INTER_LINEAR);

        
        frame[2].create( SRGB.height, SRGB.width, CV_8UC1); 
        for(int i=0; i<SRGB.width; i++){
			for(int j=0; j<SRGB.height; j++){
				Point3f pt_rgb(i, j, 1);
				Mat m_rgb(pt_rgb);
				Mat m_lwir;
				m_lwir = cams.H * m_rgb;
				Point3f pt_lwir(m_lwir);
				if(pt_lwir.y < 156 && pt_lwir.x < 206){
					frame[2].at<uint8_t>(j, i) = frame[LWIR].at<uint8_t>(Point2f(pt_lwir.x, pt_lwir.y));
				}
				else{
					frame[2].at<uint8_t>(j, i) = 0;
				}
			}
		}
        
        /* Saving */
		char name [32];
		sprintf(name, "out/rgbSCaled/%5d.png", nimg);
		imwrite(name, frame[RGB]);
		sprintf(name, "out/lwirScaled/%5d.png", nimg);
		imwrite(name, frame[2]);
		nimg ++;


		imshow("LWIR", frame[2]);
		imshow("RGB", frame[RGB]);
		
		for(int i=0; i<3; i++){
			frame[i].release();
		}
		
		char c = char(waitKey(1));
		if(c == 'q'){
			break;
		}	
	}
}
