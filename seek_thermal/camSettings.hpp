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

class camSettings
{
public:
    void write(FileStorage& fs) const;
    void read(const FileNode& node);
    bool GoodCam();
    bool GoodH();

	Mat cameraMatrix[2];
	Mat distCoeffs[2];
	double totalAvgErr[2];
	double rms[2];
	
	Mat H;
    
};


