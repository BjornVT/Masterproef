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
#include <opencv2/opencv.hpp>

#include "seek.hpp"
#include "debug.h"

#define RGB 0
#define LWIR 1

using namespace std;
using namespace cv;

class Settings
{
public:
    enum InputType {INVALID, CAMERA, VIDEO_FILE, LIBSEEK};

    void write(FileStorage& fs) const                        //Write serialization for this class
    {
        fs << "{" << "BoardSize_Width"  << boardSize.width
                  << "BoardSize_Height" << boardSize.height
                  << "Square_Size"         << squareSize
                  << "Calibrate_NrOfFrameToUse" << nrFrames
                  << "Input_Delay" << delay
                  << "InputRGB" << inputString[RGB]
                  << "InputLWIR" << inputString[LWIR]
                  << "Write_outputFileName"  << outputFileName
           << "}";
    }
    
    void read(const FileNode& node)                          //Read serialization for this class
    {
        node["BoardSize_Width" ] >> boardSize.width;
        node["BoardSize_Height"] >> boardSize.height;
        node["Square_Size"]  >> squareSize;
        node["Calibrate_NrOfFrameToUse"] >> nrFrames;
        node["Input_Delay"] >> delay;
        node["InputRGB"] >> inputString[RGB];
        node["InputLWIR"] >> inputString[LWIR];
        node["Write_outputFileName"] >> outputFileName;
        interprate();
    }    
    
    void interprate()
    {
        goodInput = true;
        if (boardSize.width <= 0 || boardSize.height <= 0)
        {
            cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
            goodInput = false;
        }
        if (squareSize <= 10e-6)
        {
            cerr << "Invalid square size " << squareSize << endl;
            goodInput = false;
        }
        if (nrFrames <= 0)
        {
            cerr << "Invalid number of frames " << nrFrames << endl;
            goodInput = false;
        }
		/* Input RGB */
        if (inputString[RGB].empty()){      // Check for valid input
            cerr << " Inexistent RGB input: " << input << endl;
            goodInput = false;
            type[RGB] = INVALID;
		}
        else{
            if (inputString[RGB][0] >= '0' && inputString[RGB][0] <= '9')
            {
				/* Cam with Linux backend */
                stringstream ss(inputString[RGB]);
                ss >> cameraID[RGB];
                video[RGB].open(cameraID[RGB]);
                input[RGB] = (void *)&video[RGB];
                type[RGB] = CAMERA;
            }
            else
            {
				/* Video */
				video[RGB].open(inputString[RGB]);
				input[RGB] = (void *)&video[RGB];
				type[RGB] = VIDEO_FILE;
			}
        }
        cout << __LINE__ << " " << goodInput << endl;
        
        /* Input LWIR */
        if (inputString[LWIR].empty()){      // Check for valid input
            cerr << " Inexistent LWIR input: " << input << endl;
            goodInput = false;
            type[LWIR] = INVALID;
		}
        else{
            if (inputString[LWIR][0] >= '0' && inputString[LWIR][0] <= '9')
            {
				/* Cam with Linux backend */
                stringstream ss(inputString[LWIR]);
                ss >> cameraID[LWIR];
                video[LWIR].open(cameraID[LWIR]);
                input[LWIR] = (void *)&video[LWIR];
                type[LWIR] = CAMERA;
            }
            else
            {
				cout << __LINE__ << " " << goodInput << endl;
				if(inputString[LWIR].compare("LIBSEEK") == 0){
					/* Cam with LibSeek */
					cout << __LINE__ << " " << goodInput << endl;
					if(seek.open())
						cout << "gelukt" << endl;
					cout << __LINE__ << " " << goodInput << endl;
					input[LWIR] = (void *)&seek;
					cout << __LINE__ << " " << goodInput << endl;
					type[LWIR] = LIBSEEK;
					cout << __LINE__ << " " << goodInput << endl;
				}
				else{
					/* Video */
					video[LWIR].open(inputString[LWIR]);
					input[LWIR] = (void *)&video[LWIR];
					type[LWIR] = VIDEO_FILE;
				}
			}
        }
        cout << __LINE__ << " " << goodInput << endl;
        /* Check if input is open */
        for(int i=0; i<2; i++){
			if(type[i] == INVALID){
				cerr << "Output couldn't be openend " << i << endl;
				goodInput = false;
			}
			else if(type[i] == LIBSEEK){
				if(!((LibSeek::seekCam *)(input[LWIR]))->isOpened()){
					cerr << "Output couldn't be openend " << i << endl;
					goodInput = false;
				}
			}
			else{
				if(!((VideoCapture *)(input[i]))->isOpened()){
					cerr << "Output couldn't be openend " << i << endl;
					goodInput = false;
				}
			}
		}
		cout << __LINE__ << " " << goodInput << endl;
    }

    
public:
    Size boardSize;            // The size of the board -> Number of items by width and height
    float squareSize;          // The size of a square in your defined unit (point, millimeter,etc).
    int nrFrames;              // The number of frames to use from the input for calibration
    int delay;                 // In case of a video input
    string outputFileName;     // The name of the file where to write
    string inputString[2];          // The input ->

    int cameraID[2];
    void * input[2];
    VideoCapture video[2];
    LibSeek::seekCam seek;
    bool goodInput = false;
    int type[2];
    
};

static void read(const FileNode& node, Settings& x, const Settings& default_value = Settings())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}

int main(int argc, char** argv) 
{
	Settings s;
    const string inputSettingsFile = argc > 1 ? argv[1] : "calibSettings.yml";
    FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
    if (!fs.isOpened())
    {
        cout << "Could not open the configuration file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }

    fs["Settings"] >> s;
    fs.release();                                         // close Settings file
	
	cout << s.goodInput << endl;
    if(!s.goodInput){
        cout << "Invalid input detected. Application stopping. " << endl;
        return -1;
    }
	
	
	//LibSeek::seekCam seek;
	Mat frame[2];
	vector<vector<Point2f> > centers[2];
	vector<vector<Point3f> > objectPoints;
	Mat cameraMatrix[2], distCoeffs[2];
	int nimages=0;
	Size imageSize[2];
	vector<Mat> goodImageList[2];
	
	imageSize[RGB].height = ((VideoCapture *)(s.input[RGB]))->get(CV_CAP_PROP_FRAME_HEIGHT);
	imageSize[RGB].width = ((VideoCapture *)(s.input[RGB]))->get(CV_CAP_PROP_FRAME_WIDTH);
	imageSize[LWIR].height = ((VideoCapture *)(s.input[LWIR]))->get(CV_CAP_PROP_FRAME_HEIGHT);
	imageSize[LWIR].width = ((VideoCapture *)(s.input[LWIR]))->get(CV_CAP_PROP_FRAME_WIDTH);
	
	
	namedWindow("LWIR",  WINDOW_NORMAL);
	namedWindow("RGB",  WINDOW_NORMAL);
	namedWindow("rectified",  WINDOW_NORMAL);
	cout << "init complete" << endl;

	//while(1) {
	while(nimages < 100){
		vector<Point2f> centersSolo[2];
		bool found[2];
		
		((VideoCapture *)(s.input[RGB]))->grab();
		if(s.type[LWIR] == Settings::InputType::LIBSEEK){
			((LibSeek::seekCam *)(s.input[LWIR]))->grab();
			if(!((LibSeek::seekCam *)(s.input[LWIR]))->retrieve(frame[LWIR])){
				cout << "no more LWIR img" << endl;
				break;
			}
		}
		else{
			((VideoCapture *)(s.input[LWIR]))->grab();
			if(!((VideoCapture *)(s.input[LWIR]))->retrieve(frame[LWIR])){
				cout << "no more LWIR img" << endl;
				break;
			}
		}
	
		if(!((VideoCapture *)(s.input[RGB]))->retrieve(frame[RGB])){
			cout << "No more RGB img" << endl;
			break;
		}
		

		cvtColor(frame[RGB], frame[RGB], CV_BGR2GRAY);
		bitwise_not(frame[RGB], frame[RGB]);
		bitwise_not(frame[LWIR], frame[LWIR]);
		
		found[LWIR] = findCirclesGrid(frame[LWIR], Size(4, 11), centersSolo[LWIR], CALIB_CB_ASYMMETRIC_GRID);
		found[RGB] = findCirclesGrid(frame[RGB], Size(4, 11), centersSolo[RGB], CALIB_CB_ASYMMETRIC_GRID);
		
		cout << found[LWIR] << "\t" << found[RGB] << endl;
		/* Have we found in both img a pattern? */
		if(!(found[LWIR] && found[RGB])){
			imshow("LWIR", frame[LWIR]);
			imshow("RGB", frame[RGB]);
			waitKey(124);
			continue;
		}
		nimages ++;
		
		/* Adding the points to the vector */
		centers[RGB].push_back(centersSolo[RGB]);
		centers[LWIR].push_back(centersSolo[LWIR]);
		goodImageList[RGB].push_back(frame[RGB]);
        goodImageList[LWIR].push_back(frame[LWIR]);
		
		
		drawChessboardCorners(frame[LWIR], Size(4, 11), Mat(centersSolo[LWIR]), found[LWIR]);
		drawChessboardCorners(frame[RGB], Size(4, 11), Mat(centersSolo[RGB]), found[RGB]);
		
		imshow("LWIR", frame[LWIR]);
		imshow("RGB", frame[RGB]);
		
		frame[RGB].release();
		frame[LWIR].release();
		waitKey(124);
	}
	
	((VideoCapture *)(s.input[RGB]))->release();
	if(s.type[LWIR] == Settings::InputType::LIBSEEK){
		((LibSeek::seekCam *)(s.input[LWIR]))->release();
	}
	else{
		((VideoCapture *)(s.input[LWIR]))->release();
	}
	
	cout << "Gevonden patronen beide: " << nimages << endl;
	

}


