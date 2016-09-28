/*
 * http://docs.opencv.org/2.4/_downloads/camera_calibration.cpp
 */

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <cstdint>
#include <chrono>
#include <signal.h>
#include <opencv2/opencv.hpp>
//#include <opencv2/highgui/highgui.hpp>

//#include "seek.hpp"
#include "debug.h"


#define NAMELWIR 	"outlwir.avi"
#define NAMERGB		"outrgb.avi"
#define SQUARESIZE	15	/* diameter 15mm */
#define FPS 8
#define CAM 0
#define LWIR 1

using namespace std;
using namespace cv;

bool running = true;
const chrono::milliseconds frameFreq(999/FPS);

void quitProgram(int sig);

int main(int argc, char** argv) {
	//LibSeek::seekCam seek;
	Mat frame[2];
	//Mat frameCam;
	VideoCapture cam;
	VideoCapture seek;
	vector<vector<Point2f> > centers[2];
	vector<vector<Point3f> > objectPoints;
	Mat cameraMatrix[2], distCoeffs[2];
	int nimages=0;
	Size imageSize[2];
	vector<Mat> goodImageList[2];
	
	
	if(argc == 3){
		cam.open(argv[1]);
		seek.open(argv[2]);
	}
	else{
		cam.open(NAMERGB);
		seek.open(NAMELWIR);
	}
	
	// check of webcam device number is correct
	if(!cam.isOpened())
	{
		throw runtime_error("Failed to open RGB camera");
		exit(1);
	}
	cam.set(CV_CAP_PROP_FPS, 0); //Hack so the VideoCapture buffer doesn't fill
	imageSize[CAM].height = cam.get(CV_CAP_PROP_FRAME_HEIGHT);
	imageSize[CAM].width = cam.get(CV_CAP_PROP_FRAME_WIDTH);
		
	if(!seek.isOpened())
	{
		throw runtime_error("Failed to open seek camera");
		exit(1);
	}
	seek.set(CV_CAP_PROP_FPS, 0); //Hack so the VideoCapture buffer doesn't fill
	imageSize[LWIR].height = seek.get(CV_CAP_PROP_FRAME_HEIGHT);
	imageSize[LWIR].width = seek.get(CV_CAP_PROP_FRAME_WIDTH);
	
	namedWindow("LWIR",  WINDOW_NORMAL);
	namedWindow("RGB",  WINDOW_NORMAL);
	namedWindow("rectified",  WINDOW_NORMAL);
	cout << "init complete" << endl;

	//while(1) {
	while(nimages < 100){
		vector<Point2f> centersSolo[2];
		bool found[2];
		
		cam.grab();
		seek.grab();
		if(!cam.retrieve(frame[CAM])){
			cout << "No more RGB img" << endl;
			break;
		}
		if(!seek.retrieve(frame[LWIR])){
			cout << "no more LWIR img" << endl;
			break;
		}

		cvtColor(frame[CAM], frame[CAM], CV_BGR2GRAY);
		bitwise_not(frame[CAM], frame[CAM]);
		bitwise_not(frame[LWIR], frame[LWIR]);
		
		found[LWIR] = findCirclesGrid(frame[LWIR], Size(4, 11), centersSolo[LWIR], CALIB_CB_ASYMMETRIC_GRID);
		found[CAM] = findCirclesGrid(frame[CAM], Size(4, 11), centersSolo[CAM], CALIB_CB_ASYMMETRIC_GRID);
		
		/* Have we found in both img a pattern? */
		if(!(found[LWIR] && found[CAM])){
			imshow("LWIR", frame[LWIR]);
			imshow("RGB", frame[CAM]);
			waitKey(124);
			continue;
		}
		nimages ++;
		
		/* Adding the points to the vector */
		centers[CAM].push_back(centersSolo[CAM]);
		centers[LWIR].push_back(centersSolo[LWIR]);
		goodImageList[CAM].push_back(frame[CAM]);
        goodImageList[LWIR].push_back(frame[LWIR]);
		
		
		drawChessboardCorners(frame[LWIR], Size(4, 11), Mat(centersSolo[LWIR]), found[LWIR]);
		drawChessboardCorners(frame[CAM], Size(4, 11), Mat(centersSolo[CAM]), found[CAM]);
		
		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[CAM]);
		
		frame[CAM].release();
		frame[LWIR].release();
		waitKey(124);
	}
	cam.release();
	seek.release();
	
	cout << "Finished reading the img" << endl;
	
	//Calculating real coords of board
	objectPoints.resize(nimages);
	for(int k=0; k<nimages; k++){
		for( int i = 0; i < 11; i++ ){
			for( int j = 0; j < 4; j++ ){
				objectPoints[k].push_back(Point3f(float((2*j + i % 2)*15), float(i*SQUARESIZE), 0));
			}
		}
	}
	
	cout << "And here we go" << endl;
	
	
    cameraMatrix[CAM] = initCameraMatrix2D(objectPoints, centers[CAM], imageSize[CAM], 0);
    cameraMatrix[LWIR] = initCameraMatrix2D(objectPoints, centers[LWIR], imageSize[LWIR], 0);
    Mat R, T, E, F;

    double rms = stereoCalibrate(objectPoints, centers[LWIR], centers[CAM],
                    cameraMatrix[LWIR], distCoeffs[LWIR],
                    cameraMatrix[CAM], distCoeffs[CAM],
                    imageSize[CAM], R, T, E, F,
                    CALIB_FIX_ASPECT_RATIO +
                    CALIB_ZERO_TANGENT_DIST +
                    CV_CALIB_FIX_FOCAL_LENGTH +
                    CALIB_USE_INTRINSIC_GUESS +
                    CALIB_RATIONAL_MODEL +
                    CALIB_FIX_K3 + CALIB_FIX_K4 + CALIB_FIX_K5,
                    TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 100, 1e-5) );
    cout << "done with RMS error=" << rms << endl;
	
	// CALIBRATION QUALITY CHECK
	// because the output fundamental matrix implicitly
	// includes all the output information,
	// we can check the quality of calibration using the
	// epipolar geometry constraint: m2^t*F*m1=0
    double err = 0;
    int npoints = 0;
    vector<Vec3f> lines[2];
    for(int i = 0; i < nimages; i++ )
    {
        int npt = (int)centers[0][i].size();
        Mat imgpt[2];
        for(int k = 0; k < 2; k++ )
        {
            imgpt[k] = Mat(centers[k][i]);
            undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], Mat(), cameraMatrix[k]);
            computeCorrespondEpilines(imgpt[k], k+1, F, lines[k]);
        }
        for(int j = 0; j < npt; j++ )
        {
            double errij = fabs(centers[0][i][j].x*lines[1][j][0] +
                                centers[0][i][j].y*lines[1][j][1] + lines[1][j][2]) +
                           fabs(centers[1][i][j].x*lines[0][j][0] +
                                centers[1][i][j].y*lines[0][j][1] + lines[0][j][2]);
            err += errij;
        }
        npoints += npt;
    }
    cout << "average epipolar err = " <<  err/npoints << endl;
	
	Mat R1, R2, P1, P2, Q;
	Mat rmap[2][2];
    Rect validRoi[2];
    cout << "stereorectify" << endl;
	stereoRectify(cameraMatrix[CAM], distCoeffs[CAM],
                  cameraMatrix[LWIR], distCoeffs[LWIR],
                  imageSize[CAM], R, T, R1, R2, P1, P2, Q,
                  CALIB_ZERO_DISPARITY, 1, imageSize[CAM], &validRoi[CAM], &validRoi[LWIR]);

	
	cout << validRoi[CAM] << " " << validRoi[LWIR] << endl;


    //Precompute maps for cv::remap()
    initUndistortRectifyMap(cameraMatrix[CAM], distCoeffs[CAM], R1, P1, imageSize[CAM], CV_16SC1, rmap[CAM][0], rmap[CAM][1]);
    cout << "rectify" << endl;
    initUndistortRectifyMap(cameraMatrix[LWIR], distCoeffs[LWIR], R2, P2, imageSize[LWIR], CV_16SC1, rmap[LWIR][0], rmap[LWIR][1]);

    Mat canvas;
    double sf;
    int w, h;

    sf = 300./MAX(imageSize[CAM].width, imageSize[CAM].height);
    w = cvRound(imageSize[CAM].width*sf);
    h = cvRound(imageSize[CAM].height*sf);
    canvas.create(h*2, w, CV_16SC3);
    
	cout << "for" << endl;
    for(int i = 0; i < nimages; i++ )
    {
        for(int k = 0; k < 2; k++ )
        {
			cout << "img " << k << endl;
            Mat img = goodImageList[k][i];
            Mat rimg, cimg;
            cout << "remap" << endl;
            remap(img, rimg, rmap[k][0], rmap[k][1], INTER_LINEAR);
            imshow("rectified", rimg);
			waitKey(0);
            cout << "bgr" << endl;
            cvtColor(rimg, cimg, COLOR_GRAY2BGR);
            imshow("rectified", cimg);
			waitKey(0);
			cout << "canvas" << endl;
            Mat canvasPart = canvas(Rect(0, h*k, w, h));
            cout << "resize" << endl;
            resize(cimg, canvasPart, canvasPart.size(), 0, 0, INTER_AREA);
               // Rect vroi(cvRound(validRoi[k].x*sf), cvRound(validRoi[k].y*sf),
               //           cvRound(validRoi[k].width*sf), cvRound(validRoi[k].height*sf));
               // rectangle(canvasPart, vroi, Scalar(0,0,255), 3, 8);

        }
        cout << "lining" << endl;

         //   for(int j = 0; j < canvas.cols; j += 16 )
         //       line(canvas, Point(j, 0), Point(j, canvas.rows), Scalar(0, 255, 0), 1, 8);
        imshow("rectified", canvas);
        char c = (char)waitKey();
        if( c == 27 || c == 'q' || c == 'Q' )
            break;
    }


	
	/*runCalibrationAndSave!!!!!!*/
		/* runCalibration */
		
		/* SaveParam */
		
		
	/*Daarna stereo calibrate */
	/* save */
	
	/* Dan rectify => andere programma met de values die hier berekend worden. */
	
	cout << "Gevonden patronen beide: " << nimages << endl;
}

void quitProgram(int sig)
{
	running = false;
}
