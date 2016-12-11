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

using namespace std;
using namespace cv;

#define RGB 0
#define LWIR 1

class Settings
{
public:
    enum InputType {INVALID, CAMERA, VIDEO_FILE, LIBSEEK};
    enum CalibType {ALL, LENS, HOMOGRAPHY};

	Settings();
	~Settings();

    void write(FileStorage& fs) const;
    void read(const FileNode& node);
    void interprate();

    
    Size boardSize;            // The size of the board -> Number of items by width and height
    float squareSize;          // The size of a square in your defined unit (point, millimeter,etc).
    int nrFrames;              // The number of frames to use from the input for calibration
    int delay;                 // In case of a video input
    int calib;
    string outputFileName;     // The name of the file where to write
    string inputString[2];          // The input ->
	Size imageSize[2];
    int cameraID[2];
    void * input[2];
    VideoCapture video[2];
    LibSeek::seekCam seek;
    bool goodInput = false;
    int type[2];
    int threshold_value;
    
};




