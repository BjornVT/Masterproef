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

#define FPS 8
#define RGB		0
#define LWIR	1

using namespace std;
using namespace cv;

bool running = true;

void quitProgram(int sig);
void drawTemp(Mat img, Point pt, double temp, Scalar color);
static void onMouse(int event, int x, int y, int z, void* in);
void degradient(Mat *img);
void meanBlock(Mat img, Mat *means, Size s);

Mat frame [3];
Mat buffer;
mutex mtxCam;
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

int main(int argc, char** argv) {
	//LibSeek::seekCam seek;
	camSettings cams;
	VideoCapture cap;
	VideoCapture seek;
	
	thread t;
    FileStorage fs;
    Mat map1[2], map2[2];
    Point3f pt_rgb(100, 100, 1);
    int trackbar=240;
	

	cap.open("outrgb.avi");
	seek.open("outlwir.avi");
	/*
	cap.open(1);
	seek.open();
	// Starting the thread for reading cam img
	cap.read(buffer);
	t = thread(task, &cap);
	*/
	
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
	
	signal(SIGTERM, &quitProgram);
	signal(SIGINT, &quitProgram);
	
	namedWindow( "LWIR",  WINDOW_NORMAL);
	namedWindow( "RGB",  WINDOW_NORMAL);
	//setMouseCallback("RGB", onMouse, (void*)(&pt_rgb));
	setMouseCallback("LWIR", onMouse, NULL);
	createTrackbar( "LWIR", "LWIR", &trackbar, 0xFF, NULL );
	
	
	initUndistortRectifyMap(cams.cameraMatrix[RGB], cams.distCoeffs[RGB], Mat(),
            getOptimalNewCameraMatrix(cams.cameraMatrix[RGB], cams.distCoeffs[RGB], Size(640, 480), 1, Size(640, 480), 0),
            Size(640, 480), CV_16SC2, map1[RGB], map2[RGB]);
	initUndistortRectifyMap(cams.cameraMatrix[LWIR], cams.distCoeffs[LWIR], Mat(),
            getOptimalNewCameraMatrix(cams.cameraMatrix[LWIR], cams.distCoeffs[LWIR], Size(208, 155), 1, Size(208, 155), 0),
            Size(208, 155), CV_16SC2, map1[LWIR], map2[LWIR]);  
	
	fs.open("output.yml", FileStorage::WRITE);
	
	cout << "Finished init " << endl;

	while(true) {
	//for(int z=0; z<200; z++){
	
		/* Seek LWIR and Normal camera */
		/*mtxCam.lock();
			if(!(seek.grab())){
				cout << "no more LWIR img" << endl;
				return false;
			}
			buffer.copyTo(frame[RGB]);
        mtxCam.unlock();
		if(!(seek.retrieveRaw(frame[LWIR]))){
			cout << "no more LWIR img" << endl;
			return false;
		}
			
		degradient(&frame[LWIR]);
		frame[LWIR].convertTo(frame[LWIR], CV_8UC1, 1.0/128);
		equalizeHist(frame[LWIR], frame[LWIR]);		
		*/


		seek.grab();
		cap.grab();
		cap.retrieve(frame[RGB]);
		seek.retrieve(frame[LWIR]);



		remap(frame[RGB], frame[RGB], map1[RGB], map2[RGB], INTER_LINEAR);
        remap(frame[LWIR], frame[LWIR], map1[LWIR], map2[LWIR], INTER_LINEAR);
        
        vector<Vec3f> circles[2];
		
		cout << "----------------------------" << endl;
		cvtColor(frame[RGB], frame[RGB], CV_BGR2GRAY );
		GaussianBlur(frame[RGB], frame[RGB], Size(3, 3), 0, 0 );
		threshold(frame[RGB], frame[RGB], trackbar, 0xFF, 0 );
		HoughCircles(frame[RGB], circles[RGB], CV_HOUGH_GRADIENT, 2, frame[RGB].rows, 200, 20, 0, 0 );
		cvtColor(frame[RGB], frame[RGB], CV_GRAY2BGR );
		
		cvtColor(frame[LWIR], frame[LWIR], CV_BGR2GRAY );
		GaussianBlur(frame[LWIR], frame[LWIR], Size(3, 3), 0, 0 );
		threshold(frame[LWIR], frame[LWIR], trackbar, 0xFF, 0 );
		HoughCircles(frame[LWIR], circles[LWIR], CV_HOUGH_GRADIENT, 2, frame[LWIR].rows, 200, 20, 0, 0 );
		cvtColor(frame[LWIR], frame[LWIR], CV_GRAY2BGR );
		
		for(int j=0; j<2; j++){		
			/// Draw the circles detected
			for( size_t i = 0; i < circles[j].size(); i++ )
			{	
				// circle center
				Point center(cvRound(circles[j][i][0]), cvRound(circles[j][i][1]));
				circle( frame[j], center, 3-j, Scalar(0,255,0), -1, 8, 0 );
				
				cout << j << " detected " << center << endl;
				if(j == 0)
				{
					Point3f pt_rgb(circles [j][i][0], circles[j][i][1], 1);
					Mat m_rgb(pt_rgb);
					Mat m_lwir;
					m_lwir = cams.H * m_rgb;
					Point3f pt_lwir(m_lwir);
					circle(frame[LWIR], Point2f(pt_lwir.x, pt_lwir.y), 2, Scalar(0, 0, 255), -1, 8, 0);
					cout << "calculated " << Point2f(pt_lwir.x, pt_lwir.y) << endl;
				}
				
			}
		}
	
	
		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[RGB]);
		
		static int n=1;
		char c = char(waitKey(0));
		if(c == 'q'){
			break;
		}
		if( c == 's' || c == 'S'){
			
			char buffer [50];
			
			sprintf (buffer, "nauwkeurigRGB%d.png", n);
			imwrite( buffer, frame[RGB]);
			sprintf (buffer, "nauwkeurigLWIR%d.png", n);
			imwrite( buffer, frame[LWIR]);
			cout << "Saved img" << endl;
		}
		
		frame[RGB].release();
		frame[LWIR].release();
	}
	fs.release();
}

void quitProgram(int sig)
{
	running = false;
}


static void onMouse(int event, int x, int y, int z, void* in)
{
	if(event == EVENT_LBUTTONDOWN){
		cout << "Clicked " << x << " " << y << endl;
		circle( frame[LWIR], Point(x, y), 2, Scalar(0,255,0), -1, 8, 0 );
		imshow("LWIR", frame[LWIR]);
	}
}

void degradient(Mat *img)
{
	static Mat grad = Mat::zeros(HEIGHT, WIDTH-2, CV_16UC1);
	static int minGrad = 0;
	static bool needed = true;
	double minVal, maxVal;
	
	if(needed){
		/* Anything in frame? */
		Mat mean(img->rows/4, img->cols/4, CV_32SC1);
		meanBlock(*img, &mean, Size(4, 4));
		minMaxLoc(mean, &minVal, &maxVal, NULL, NULL);
		cout << maxVal - minVal << endl;
		if((maxVal-minVal) < 275){
			/* Nothing in frame
			 * We can use it for updating gradient :)
			 */
			
			grad = (grad*7 + *img)/8;
			grad -= minVal;
			//minGrad = minVal;

		}
	}
	
	*img = *img - grad;
	//*img += minGrad;
		
	if(needed){
		minMaxLoc(*img, &minVal, &maxVal, NULL, NULL);
		if((maxVal-minVal) < 65){
			needed = false;
		}
	}
}

void meanBlock(Mat img, Mat *means, Size s)
{
	for(int l=0; l<img.rows/s.height; l++){ //l+=s.height){
		for(int m=0; m<img.cols/s.width; m++){ //m+=s.width){
			int mean = 0;
			for(int i=0; i<s.height; i++){
				for(int j=0; j<s.width; j++){
					mean = mean + img.at<uint16_t>(l*s.height+i, m*s.width+j);
				}
			}
			
			mean = mean / (s.height * s.width);
			means->at<int>(l, m) = (int)(mean);
		}
	}
}
