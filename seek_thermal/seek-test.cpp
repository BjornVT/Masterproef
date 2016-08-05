#include <stdio.h>
#include <string.h>
#include <opencv2/highgui/highgui.hpp>

#include "seek.hpp"
#include "debug.h"

using namespace std;
using namespace cv;


int ma=100, mi=0, col=11;

int main() {
	int _max = 0x8200;
	int _min = 0x7E00;

	LibSeek::seekCam seek;
	Mat frame;
	//seek.init();
	
	namedWindow( "Resultaat",  WINDOW_NORMAL);
	createTrackbar( "Min", "Resultaat", &mi, 500, 0 );
	createTrackbar( "Max", "Resultaat", &ma, 500, 0 );
	namedWindow( "med",  WINDOW_NORMAL);
	namedWindow( "colour",  WINDOW_NORMAL);
	createTrackbar( "colour", "colour", &col, 11, 0 );

	while(true) {
		frame = seek.frame_acquire();
	
		int max = 0;
		int min = 0xFFFF;

		//minMaxIdx(frame, &min, &max);

		for (int y = 0; y < frame.rows; y++) {
			for (int x = 0; x < frame.cols; x++) {
				uint16_t pixel = frame.at<uint16_t>(y, x);
				
				max = max < pixel ? pixel : max;
				min = min > pixel ? pixel : min;
				
				float v = float(pixel - _min) / (_max - _min);
				if (v < 0.0) { v = 0; }
				if (v > 1.0) { v = 1; }
				uint16_t o = 0xffff * v;

				frame.at<uint16_t>(y, x) = o;
			}
		}
		_min = min - mi;
		_max = max + ma;
		
		printf("max: %x\nmin: %x\n", max, min);
		
		transpose(frame, frame);  
		flip(frame, frame,0);

		imshow( "Resultaat", frame );
		//imwrite( "res.png", frame );
		
		Mat med;
		
		medianBlur ( frame, med, 3 );
		imshow( "med", med );
		//imwrite("med.png", med);
		
		
		
		Mat colour;
		med.convertTo(colour,CV_8UC1, 1.0/256.0 );
		equalizeHist( colour, colour ); 
		bitwise_not ( colour, colour );
		applyColorMap(colour, colour, col);
		imshow("colour", colour);
		
		
		
		med.release();
		frame.release();
		waitKey(25);
	}
}
