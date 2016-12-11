/*
 * http://docs.opencv.org/2.4/_downloads/camera_calibration.cpp
 */


#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <cstdint>
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <unistd.h>

#include "seek.hpp"
#include "debug.h"
#include "readSettings.hpp"
#include "camSettings.hpp"

#define RGB 0
#define LWIR 1

#define SUCCES			0
#define ERR_QUIT		1
#define ERR_NO_FRAMES	2


using namespace std;
using namespace cv;


Mat buffer;
mutex mtxCam;
void task(VideoCapture *cap, bool *running)
{
	while(*running){
		cap->grab();
		if(mtxCam.try_lock()){
			cap->retrieve(buffer);
			mtxCam.unlock();
		}
		usleep(100);
	}
}


static void read(const FileNode& node, Settings& x, const Settings& default_value = Settings())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}
static void read(const FileNode& node, camSettings& x, const camSettings& default_value = camSettings())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}

static void write(FileStorage& fs, const std::string&, const camSettings& x)
{
    x.write(fs);
}


int calibCam(const Settings *s, camSettings *cams);
int calHomography(const Settings *s, camSettings *cams);
int test (const Settings *s, camSettings *cams);
static double computeReprojectionErrors( const vector<vector<Point3f> >& objectPoints,
                                         const vector<vector<Point2f> >& imagePoints,
                                         const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                                         const Mat& cameraMatrix , const Mat& distCoeffs,
                                         vector<float>& perViewErrors);

int main(int argc, char** argv) 
{
	Settings s;
	camSettings cams;
	thread t;
	bool threadRun = true;
	bool calibCamFailed = false;
	
    const string inputSettingsFile = argc > 1 ? argv[1] : "calibSettings.yml";
    FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
    if (!fs.isOpened())
    {
        cout << "Could not open the configuration file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }
    fs["Settings"] >> s;
    fs.release();                                         // close Settings file
    if(!s.goodInput){
        cout << "Invalid input detected. Application stopping. " << endl;
        return -1;
    }
    
    
    if(s.type[0] == Settings::InputType::CAMERA){
		/* Starting the thread for reading cam img */
		((VideoCapture *)(s.input[RGB]))->read(buffer);
		t = thread(task, ((VideoCapture *)(s.input[RGB])), &threadRun);
	}
	
	namedWindow("LWIR",  WINDOW_NORMAL);
	namedWindow("RGB",  WINDOW_NORMAL);
	
	cout << "init complete" << endl;

	/* Lens calib  */
	if(s.calib == Settings::CalibType::ALL || s.calib == Settings::CalibType::LENS){
		switch(calibCam(&s, &cams)){
			case SUCCES:
				cout << "Lens calibration complete." << endl;
				break;
			case ERR_QUIT:
				cout << "Lens calibration has been quit." << endl;
				calibCamFailed = true;
				if(s.calib == Settings::CalibType::LENS){
					cout << "Stopping program. Nothing else to do." << endl;
					if(s.type[0] == Settings::InputType::CAMERA){
						/* Stopping thread if running */
						threadRun = false;
						t.join();
					}
					return 1;
				}
				break;
			default:
				cerr << "Lens calibration failed" << endl;
				calibCamFailed = true;
				if(s.calib == Settings::CalibType::LENS){
					cout << "Stopping program. Nothing else to do." << endl;
					if(s.type[0] == Settings::InputType::CAMERA){
						/* Stopping thread if running */
						threadRun = false;
						t.join();
					}
					return 1;
				}
				break;
		}	
	}
	
	if(s.calib == Settings::CalibType::HOMOGRAPHY || calibCamFailed){
		/* Only calculation homography of lens calibration failed?
		 * Read the calibration file hoping there is something there */
		cout << "Trying to read lens calirations from calibration file." << endl;
		fs.open(s.outputFileName, FileStorage::READ); // Read the camSettings
		if (!fs.isOpened())
		{
			cout << "Could not open the outputfile: \"" << s.outputFileName << "\"" << endl;
			return -1;
		}
		fs["Calibration"] >> cams;
		fs.release();  
		if(!cams.GoodCam()){
			cerr << "Invalid settings to be read in. from calibration file." << endl;
			
			if(s.type[0] == Settings::InputType::CAMERA){
				/* Stopping thread if running */
				threadRun = false;
				t.join();
			}
			
			return -1;
		} 
		cout << "Read lens calirations from file" << endl;
	}
	
	/* Homography  */	
	if(s.calib == Settings::CalibType::ALL || s.calib == Settings::CalibType::HOMOGRAPHY){
		/* Calculation homography */
			/* Give the man a break */
		/*cout << "Get to your positions. Ready. Set." << endl
				<< "Press any key to continue with the homography calculation." << endl
				<< "Press q to quit." << endl;
		char c = (char)waitKey(10000);
		if(c == 'q' || c == 'Q'){
			return 1;
		}
		
		switch(calHomography(&s, &cams)){
			case SUCCES:
				cout << "Homography calculation complete." << endl;
				break;
			case ERR_QUIT:
				cout << "Homography calculation has been quit." << endl;
				break;	/* Only break, maybe the lens calib needs to be saved */
		/*	default:
				cerr << "Homography calculation failed" << endl;
				break;	/* Only break, maybe the lens calib needs to be saved */
		//}
	}
		


	cout << "Saving ..." << endl;
	fs.open(s.outputFileName, FileStorage::WRITE); // Write the camSettings
	if (!fs.isOpened())
	{
		cout << "Could not open the outputfile: \"" << s.outputFileName << "\"" << endl;
		return -1;
	}
	fs << "Calibration" << cams;
	fs.release();
	
	
	test(&s, &cams);

	
	
	
	if(s.type[0] == Settings::InputType::CAMERA){
		/* Stopping thread if running */
		threadRun = false;
		t.join();
	}
	return 0;
}


bool getFrames(const Settings *s, Mat frame[2])
{	
	if(s->type[LWIR] == Settings::InputType::LIBSEEK && s->type[RGB] == Settings::InputType::CAMERA){
		/* Seek LWIR and Normal camera */
		mtxCam.lock();
			((LibSeek::seekCam *)(s->input[LWIR]))->grab();
			buffer.copyTo(frame[RGB]);
        mtxCam.unlock();
		if(!((LibSeek::seekCam *)(s->input[LWIR]))->retrieve(frame[LWIR])){
			cout << "no more LWIR img" << endl;
			return false;
		}
		frame[LWIR].convertTo(frame[LWIR], CV_8UC1, 1.0/32.0);
		equalizeHist(frame[LWIR], frame[LWIR]); 
	}
	else if(s->type[LWIR] == Settings::InputType::CAMERA && s->type[RGB] == Settings::InputType::CAMERA){
		/* LWIR with opencv and normal camera */
		mtxCam.lock();
			((VideoCapture *)(s->input[LWIR]))->grab();
			buffer.copyTo(frame[RGB]);
        mtxCam.unlock();
        if(!((VideoCapture *)(s->input[LWIR]))->retrieve(frame[LWIR])){
			cout << "no more LWIR img" << endl;
			return false;
		}
	}
	else{
		/* Video file */
		if(!((VideoCapture *)(s->input[RGB]))->read(frame[RGB])){
			cout << "No more RGB img" << endl;
			return false;
		}
		if(!((VideoCapture *)(s->input[LWIR]))->read(frame[LWIR])){
			cout << "No more LWIR img" << endl;
			return false;
		}
		
	}
	
	return true;
}

int calibCam(const Settings *s, camSettings *cams)
{
	vector<vector<Point2f> > centers[2];
	vector<vector<Point3f> > objectPoints(1);
	int nimages=0;
	Mat frame[2];
	
	cout << "Reading and shearching input for circle grid pattern." << endl
			<< "Press s to skip the reading and shearching to the calculation." << endl
			<< "Press q to quit this calibration step." << endl;
			
	while(nimages < s->nrFrames){
		vector<Point2f> centersSolo[2];
		bool found[2];


		cvtColor(frame[RGB], frame[RGB], CV_BGR2GRAY);
		bitwise_not(frame[RGB], frame[RGB]);
		bitwise_not(frame[LWIR], frame[LWIR]);
		
		found[LWIR] = findCirclesGrid(frame[LWIR], s->boardSize, centersSolo[LWIR], CALIB_CB_ASYMMETRIC_GRID);
		found[RGB] = findCirclesGrid(frame[RGB], s->boardSize, centersSolo[RGB], CALIB_CB_ASYMMETRIC_GRID);
		
		/* Have we found in both img a pattern? */
		if(!(found[LWIR] && found[RGB])){
			imshow("LWIR", frame[LWIR]);
			imshow("RGB", frame[RGB]);
			char c = (char)waitKey(s->delay);
			if(c == 'q' || c == 'Q'){
				return ERR_QUIT;
			}
			else if(c == 's' || c == 'S'){
				break;
			}
			continue;
		}
		nimages ++;
		
		/* Adding the points to the vector */
		centers[RGB].push_back(centersSolo[RGB]);
		centers[LWIR].push_back(centersSolo[LWIR]);
		
		drawChessboardCorners(frame[LWIR], Size(4, 11), Mat(centersSolo[LWIR]), found[LWIR]);
		drawChessboardCorners(frame[RGB], Size(4, 11), Mat(centersSolo[RGB]), found[RGB]);
		
		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[RGB]);

		frame[RGB].release();
		frame[LWIR].release();
		char c = (char)waitKey(s->delay);
		if( c == 'q' || c == 'Q' )
			return ERR_QUIT;
	}
	
	cout << "We found: " << nimages  << " that could be used."<< endl;
	
	
	//calib and save
	//runcalibration 
	/* Check what fixaspect ratio does => flags*/
	for(int i=0; i<s->boardSize.height; i++){
        for(int j=0; j<s->boardSize.width; j++){
			objectPoints[0].push_back(Point3f(float((2*j + i % 2)*s->squareSize), float(i*s->squareSize), 0));
		}
	}
	objectPoints.resize(nimages, objectPoints[0]);
     
     
    vector<Mat> rvecs[2], tvecs[2];
	vector<float> reprojErrs;
	
	for(int i=0; i<2; i++){
		//Find intrinsic and extrinsic camera parameters
		cams->rms[i] = calibrateCamera(objectPoints, centers[i], s->imageSize[i], cams->cameraMatrix[i],
								cams->distCoeffs[i], rvecs[i], tvecs[i], CV_CALIB_FIX_K4|CV_CALIB_FIX_K5);

		cout << "Re-projection error for camera " << i << " reported by calibrateCamera: "<< cams->rms[i] << endl;
		
		cams->totalAvgErr[i] = computeReprojectionErrors(objectPoints, centers[i],
												 rvecs[i], tvecs[i], cams->cameraMatrix[i], cams->distCoeffs[i], reprojErrs);
												 
		cout << "Total avg Err: " << cams->totalAvgErr[i] << endl;
	}
	
	return SUCCES;
}

int calHomography(const Settings *s, camSettings *cams)
{
	int nimages=0;
	vector<Point2f> centers[2];
	Mat frame[3];
	Mat map1[2], map2[2];
	//Mat rview[2];
	int threshold_value = s->threshold_value;
	bool ThresholLoop = true;
	
	
	initUndistortRectifyMap(cams->cameraMatrix[RGB], cams->distCoeffs[RGB], Mat(),
            getOptimalNewCameraMatrix(cams->cameraMatrix[RGB], cams->distCoeffs[RGB], s->imageSize[RGB], 1, s->imageSize[RGB], 0),
            s->imageSize[RGB], CV_16SC2, map1[RGB], map2[RGB]);
	initUndistortRectifyMap(cams->cameraMatrix[LWIR], cams->distCoeffs[LWIR], Mat(),
            getOptimalNewCameraMatrix(cams->cameraMatrix[LWIR], cams->distCoeffs[LWIR], s->imageSize[LWIR], 1, s->imageSize[LWIR], 0),
            s->imageSize[LWIR], CV_16SC2, map1[LWIR], map2[LWIR]);    


	cout << "Select the correct threshold value, press any key to validate." << endl
			<< "Press c to continue with the calibration using selected value." << endl
			<< "Press q to quit the calibration." << endl;
/*			
	while(ThresholLoop){
		vector<Point2f> centersSolo[2];
		bool found[2];
		
		if(!getFrames(s, frame)){
			return ERR_NO_FRAMES;
		}
		
		cvtColor(frame[RGB], frame[RGB], CV_BGR2GRAY);
		bitwise_not(frame[RGB], frame[RGB]);
		bitwise_not(frame[LWIR], frame[LWIR]);
	
		remap(frame[RGB], frame[RGB], map1[RGB], map2[RGB], INTER_LINEAR);
        remap(frame[LWIR], frame[LWIR], map1[LWIR], map2[LWIR], INTER_LINEAR);
		
		
		found[RGB] = findCirclesGrid(frame[RGB], Size(4, 11), centersSolo[RGB], CALIB_CB_ASYMMETRIC_GRID);
		drawChessboardCorners(frame[RGB], Size(4, 11), Mat(centersSolo[RGB]), found[RGB]);	
		
		
		createTrackbar( "value", "RGB", &threshold_value, 255, NULL);
		threshold( frame[LWIR], frame[2], threshold_value, 255, 1 );
		
		erode(frame[2], frame[2], Mat());
		dilate(frame[2], frame[2], Mat());
		dilate(frame[2], frame[2], Mat());
		erode(frame[2], frame[2], Mat());	
			
		bitwise_not(frame[2], frame[2]);
		found[LWIR] = findCirclesGrid(frame[2], Size(4, 5), centersSolo[LWIR], CALIB_CB_SYMMETRIC_GRID);
		drawChessboardCorners(frame[2], Size(4, 5), Mat(centersSolo[LWIR]), found[LWIR]);
		cout << "gevonden " << found[LWIR] << endl;
		imshow("RGB", frame[2]);
		
		imshow("LWIR", frame[LWIR]);
        
        char c = (char)waitKey();
        //char c = (char)waitKey();
        if(c == 'q' || c == 'Q'){
			return ERR_QUIT;
		}
		else if(c == 'c' || c == 'C'){
			ThresholLoop = false;
		}
    }
 */   
cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;

	namedWindow("tresh",  WINDOW_NORMAL);
    nimages = 0;
    while(nimages < s->nrFrames){
		vector<Point2f> centersSolo[2];
		bool found[2];
		
		if(!getFrames(s, frame)){
			//return ERR_NO_FRAMES;
			break;
		}
		
		cvtColor(frame[RGB], frame[RGB], CV_BGR2GRAY);
		bitwise_not(frame[RGB], frame[RGB]);
		bitwise_not(frame[LWIR], frame[LWIR]);
	
		remap(frame[RGB], frame[RGB], map1[RGB], map2[RGB], INTER_LINEAR);
        remap(frame[LWIR], frame[LWIR], map1[LWIR], map2[LWIR], INTER_LINEAR);
		
	
		found[RGB] = findCirclesGrid(frame[RGB], Size(4, 11), centersSolo[RGB], CALIB_CB_ASYMMETRIC_GRID);
		
					
		
		threshold( frame[LWIR], frame[2], threshold_value, 255, 1 );
		erode(frame[2], frame[2], Mat());
		dilate(frame[2], frame[2], Mat());
		dilate(frame[2], frame[2], Mat());
		erode(frame[2], frame[2], Mat());	
		bitwise_not(frame[2], frame[2]);
		found[LWIR] = findCirclesGrid(frame[2], Size(4, 5), centersSolo[LWIR], CALIB_CB_SYMMETRIC_GRID);
		
		
		
		
		/* Have we found in both img a pattern? */
		if(!(found[LWIR] && found[RGB])){
			imshow("LWIR", frame[LWIR]);
			imshow("RGB", frame[RGB]);
			imshow("tresh", frame[2]);
			char c = (char)waitKey(s->delay);
			if(c == 'q' || c == 'Q'){
				return ERR_QUIT;
			}
			else if(c == 's' || c == 'S'){
				break;
			}
			continue;
		}
		cout << nimages ++ << endl;
		
		cvtColor(frame[RGB], frame[RGB], CV_GRAY2BGR);
		cvtColor(frame[LWIR], frame[LWIR], CV_GRAY2BGR);
		
		drawChessboardCorners(frame[2], Size(4, 5), Mat(centersSolo[LWIR]), found[LWIR]);
		drawChessboardCorners(frame[RGB], Size(4, 11), Mat(centersSolo[RGB]), found[RGB]);
		
		/* Adding the points to the vector */
		for(int i=0; i<5; i++){
			for(int j=0; j<4; j++){
				centers[RGB].push_back(centersSolo[RGB][i*8+j+4]);
				centers[LWIR].push_back(centersSolo[LWIR][i*4+j]);
			}
		}
		
		imshow("RGB", frame[RGB]);
		imshow("LWIR", frame[LWIR]);
		imshow("tresh", frame[2]);
        
        char c = (char)waitKey(s->delay);
        if(c == 'q' || c == 'Q'){
			return ERR_QUIT;
		}
		if(c == 'c' || c == 'C'){
			break;
		}
    }
	cout << "We found: " << nimages  << " that could be used."<< endl;
    
    
    //Mat H;
	cams->H = findHomography(centers[RGB], centers[LWIR], CV_RANSAC, 4);
	//_H = findHomography(centersSolo[RGB], centersSolo[LWIR], 0);
	/*_H.convertTo(_H, CV_32FC1, 1, 0);
	cout << "_H " << _H << endl;
	//H = _H + H;
	cout << "H " << H << endl;
	if(i==0)
		H = findHomography(centersSolo[RGB], centersSolo[LWIR], 0);
	//	H /= 2;
	H = _H;
    */
    
    cout << cams->H << endl;
    
    
    
    
    
    /*
    

		Mat _H;
		//Mat 
		//H = findHomography(centersSolo[RGB], centersSolo[LWIR], CV_RANSAC, 4, match_mask);
		_H = findHomography(centersSolo[RGB], centersSolo[LWIR], 0);
		_H.convertTo(_H, CV_32FC1, 1, 0);
		cout << "_H " << _H << endl;
		//H = _H + H;
		cout << "H " << H << endl;
		if(i==0)
		H = findHomography(centersSolo[RGB], centersSolo[LWIR], 0);
		//	H /= 2;
		H = _H;
		
		/* Just a dull test */
		/* See if some centers can be recalculated from RGB */
		/*
		cout << "H " << H << endl << endl;
		
		
		
		
		//multiply(pt_rgb, H, pt_lwir);
		/*
		Mat test(pt_rgb);
		cout << "And mult?" << endl;
		Mat test2 = test * H;
		cout << test2 << endl;
		//Point3f pt_lwir = H * pt_rgb;
		//Point3f pt_old = centersSolo[LWIR][0].pt;
		* */
		//cout << pt_lwir << endl;
/*
	}
	
	for(int i=0; i<nimages; i++){
		goodImageList[RGB][i].copyTo(frame[RGB]);
		goodImageList[LWIR][i].copyTo(frame[LWIR]);
		cvtColor(frame[RGB], frame[RGB], CV_GRAY2BGR);
		drawChessboardCorners(frame[RGB], Size(4, 11), Mat(centers[RGB][i]), true);
	
		for(int j=0; j<44; j++){
			Point3f pt_rgb(centers[RGB][i][j].x, centers[RGB][i][j].y, 1);
		
		
			Mat m_rgb(pt_rgb);
			Mat m_lwir;
		
		//cout << "point rgb " << pt_rgb << endl;
		//cout << "mat rgb " << m_rgb << endl;;
		
			//H.convertTo(H,CV_32FC1,1,0); //NOW A IS FLOAT 
		
			m_lwir = H * m_rgb;
		//cout << "mat lwir" << m_rgb << endl;
			Point3f pt_lwir(m_lwir);
		
			circle(frame[RGB], Point2f(pt_rgb.x, pt_rgb.y), 2, Scalar(0, 255, 255), 1);
			circle(frame[LWIR], Point2f(pt_lwir.x, pt_lwir.y), 2, Scalar(255, 0, 125), 1);
		}
		
		imshow("RGB", frame[RGB]);
        imshow("LWIR", frame[LWIR]);
        char c = (char)waitKey();
        if( c == 'q' || c == 'Q' )
			break;
	}
	
        //line(frame, pt_new, pt_old, Scalar(125, 255, 125), 1);
        //circle(frame, pt_new, 2, Scalar(255, 0, 125), 1);
	*/
	

	
	
	return SUCCES;
}

static void onMouse(int event, int x, int y, int z, void* in)
{
	Point3f *pt_rgb = (Point3f*) in;
	
	pt_rgb->x = x;
	pt_rgb->y = y;
	
}

int test (const Settings *s, camSettings *cams)
{
	cout << "Dit is een test" << endl;
	Mat frame[2];
	Mat map1[2], map2[2];
	int nr = 0;
	
	Point3f pt_rgb(100, 100, 1);
	
	cout << cams->H << endl;
	
	initUndistortRectifyMap(cams->cameraMatrix[RGB], cams->distCoeffs[RGB], Mat(),
            getOptimalNewCameraMatrix(cams->cameraMatrix[RGB], cams->distCoeffs[RGB], s->imageSize[RGB], 1, s->imageSize[RGB], 0),
            s->imageSize[RGB], CV_16SC2, map1[RGB], map2[RGB]);
	initUndistortRectifyMap(cams->cameraMatrix[LWIR], cams->distCoeffs[LWIR], Mat(),
            getOptimalNewCameraMatrix(cams->cameraMatrix[LWIR], cams->distCoeffs[LWIR], s->imageSize[LWIR], 1, s->imageSize[LWIR], 0),
            s->imageSize[LWIR], CV_16SC2, map1[LWIR], map2[LWIR]);  
		
	setMouseCallback("RGB", onMouse, (void*)(&pt_rgb));
	
	cams->H.convertTo(cams->H,CV_32FC1,1,0); //NOW A IS FLOAT 
cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << endl;

	while(true){
		if(!getFrames(s, frame)){
			//return ERR_NO_FRAMES;
			break;
		}
	
		remap(frame[RGB], frame[RGB], map1[RGB], map2[RGB], INTER_LINEAR);
        remap(frame[LWIR], frame[LWIR], map1[LWIR], map2[LWIR], INTER_LINEAR);
        
		//cout << "point rgb " << pt_rgb << endl;
		//cout << "mat rgb " << m_rgb << endl;;
		
		//H.convertTo(H,CV_32FC1,1,0); //NOW A IS FLOAT 
		
		Mat m_rgb(pt_rgb);
		Mat m_lwir;
		
		//perspectiveTransform(m_rgb, m_lwir, cams->H);
		
		m_lwir = cams->H * m_rgb;
		//cout << "mat lwir" << m_rgb << endl;
		
		Point3f pt_lwir(m_lwir);

		cvtColor(frame[LWIR], frame[LWIR], CV_GRAY2BGR);
	
		circle(frame[RGB], Point2f(pt_rgb.x, pt_rgb.y), 5, Scalar(0, 0, 255), 1);
		circle(frame[LWIR], Point2f(pt_lwir.x, pt_lwir.y), 2, Scalar(0, 0, 255), 1);

		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[RGB]);
		
		
		char naam[20];
		char c = (char)waitKey(s->delay);
        if( c == 'q' || c == 'Q' )
			break;
		if( c == 's' || c == 'S'){
			sprintf(naam, "lwir_%d.jpg", nr);
			imwrite(naam, frame[LWIR]);
			sprintf(naam, "rgb_%d.jpg", nr);
			imwrite(naam, frame[RGB]);
			cout << "Saved img" << endl;
			nr++;
		}
	}
	
	return 0;
}


double computeReprojectionErrors( const vector<vector<Point3f> >& objectPoints,
                                         const vector<vector<Point2f> >& imagePoints,
                                         const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                                         const Mat& cameraMatrix , const Mat& distCoeffs,
                                         vector<float>& perViewErrors)
{
    vector<Point2f> imagePoints2;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for( i = 0; i < (int)objectPoints.size(); ++i )
    {
        projectPoints( Mat(objectPoints[i]), rvecs[i], tvecs[i], cameraMatrix,
                       distCoeffs, imagePoints2);
        err = norm(Mat(imagePoints[i]), Mat(imagePoints2), CV_L2);

        int n = (int)objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err*err/n);
        totalErr        += err*err;
        totalPoints     += n;
    }

    return std::sqrt(totalErr/totalPoints);
}
