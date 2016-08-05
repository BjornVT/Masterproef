#ifndef __SEEK_HPP_
#define __SEEK_HPP_

#include <opencv2/opencv.hpp>

#include <cinttypes>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <tuple>
#include <vector>
#include <endian.h>
#include <libusb.h>

#include "debug.h"

#define WIDTH	208
#define HEIGHT	156

using namespace std;
namespace LibSeek {
	
class caminterface {
	private:
		struct libusb_context * ctx = NULL;
		struct libusb_device_handle * handle = NULL;
		struct libusb_device * dev = NULL;
		struct libusb_device_descriptor desc;

	public:
		caminterface();
		~caminterface();
		void init();
		void exit();
		void vendor_transfer(bool direction, uint8_t req, uint16_t value,
			uint16_t index, vector<uint8_t> & data, int timeout=1000);

		void frame_get_one(uint8_t *frame);
};

class seekCam {
	private:
		class caminterface _cam;
		cv::Mat calib;
		int level_shift = 0;
		
		void buildBPList();
		void filterBP(cv::Mat frame);
		
	public:
		seekCam();
		~seekCam();
		cv::Mat frame_acquire();
		cv::Mat * getCalib();
		vector<cv::Point> bp_list;
		
		
};


struct Request {
	enum Enum {
	 BEGIN_MEMORY_WRITE              = 82,
	 COMPLETE_MEMORY_WRITE           = 81,
	 GET_BIT_DATA                    = 59,
	 GET_CURRENT_COMMAND_ARRAY       = 68,
	 GET_DATA_PAGE                   = 65,
	 GET_DEFAULT_COMMAND_ARRAY       = 71,
	 GET_ERROR_CODE                  = 53,
	 GET_FACTORY_SETTINGS            = 88,
	 GET_FIRMWARE_INFO               = 78,
	 GET_IMAGE_PROCESSING_MODE       = 63,
	 GET_OPERATION_MODE              = 61,
	 GET_RDAC_ARRAY                  = 77,
	 GET_SHUTTER_POLARITY            = 57,
	 GET_VDAC_ARRAY                  = 74,
	 READ_CHIP_ID                    = 54,
	 RESET_DEVICE                    = 89,
	 SET_BIT_DATA_OFFSET             = 58,
	 SET_CURRENT_COMMAND_ARRAY       = 67,
	 SET_CURRENT_COMMAND_ARRAY_SIZE  = 66,
	 SET_DATA_PAGE                   = 64,
	 SET_DEFAULT_COMMAND_ARRAY       = 70,
	 SET_DEFAULT_COMMAND_ARRAY_SIZE  = 69,
	 SET_FACTORY_SETTINGS            = 87,
	 SET_FACTORY_SETTINGS_FEATURES   = 86,
	 SET_FIRMWARE_INFO_FEATURES      = 85,
	 SET_IMAGE_PROCESSING_MODE       = 62,
	 SET_OPERATION_MODE              = 60,
	 SET_RDAC_ARRAY                  = 76,
	 SET_RDAC_ARRAY_OFFSET_AND_ITEMS = 75,
	 SET_SHUTTER_POLARITY            = 56,
	 SET_VDAC_ARRAY                  = 73,
	 SET_VDAC_ARRAY_OFFSET_AND_ITEMS = 72,
	 START_GET_IMAGE_TRANSFER        = 83,
	 TARGET_PLATFORM                 = 84,
	 TOGGLE_SHUTTER                  = 55,
	 UPLOAD_FIRMWARE_ROW_SIZE        = 79,
	 WRITE_MEMORY_DATA               = 80,
	};
};

} // LibSeek::

#endif
