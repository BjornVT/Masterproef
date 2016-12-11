#include "seek.hpp"

#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;
using namespace LibSeek;

inline void printData(vector<uint8_t> data)
{
	if(DEBUG){
		fprintf(stderr, " [");
		for (uint i = 0; DEBUG && i < data.size(); i++) {
			fprintf(stderr, " %2x", data[i]);
		}
		fprintf(stderr, " ]\n");
	}
}

caminterface::caminterface()
{

}

caminterface::~caminterface()
{
	bugprintf("\n");
	exit();
}

void caminterface::init()
{
	int res;

	if (handle != NULL) {
		throw runtime_error("dev should be null");
	}

	// Init libusb
	res = libusb_init(&ctx);
	if (res < 0) {
		throw runtime_error("Failed to initialize libusb");
	}

	struct libusb_device **devs;
	ssize_t cnt;

	// Get a list os USB devices
	cnt = libusb_get_device_list(ctx, &devs);
	if (cnt < 0) {
		throw runtime_error("No devices");
	}

	bugprintf("Device Count : %zd\n",cnt);

	bool found(false);
	for (int idx_dev = 0; idx_dev < cnt; idx_dev++) {
		dev = devs[idx_dev];
		res = libusb_get_device_descriptor(dev, &desc);
		if (res < 0) {
			libusb_free_device_list(devs, 1);
			throw runtime_error("Failed to get device descriptor");
		}

		if (desc.idVendor == 0x289d && desc.idProduct == 0x0010) {
			found = true;
			break;
		}
	} // for each device

	if (!found) {
		libusb_free_device_list(devs, 1);
		throw runtime_error("Seek not found");
	}

	res = libusb_open(dev, &handle);
	if (res < 0) {
		libusb_free_device_list(devs, 1);
		throw runtime_error("Failed to open device");
	}

	bugprintf("Device found\n");

	int config2;
	res = libusb_get_configuration(handle, &config2);
	if (res != 0) {
		libusb_free_device_list(devs, 1);
		throw runtime_error("Couldn't get device configuration");
	}
	bugprintf("Configured value : %d\n",config2);

	if (config2 != 1) {
		res = libusb_set_configuration(handle, 1);
		if (res != 0) {
			libusb_free_device_list(devs, 1);
			throw runtime_error("Couldn't set device configuration");
		}
	}

	libusb_free_device_list(devs, 1);

	res = libusb_claim_interface(handle, 0);
	if (res < 0) {
		throw runtime_error("Couldn't claim interface");
	}
	bugprintf("Claimed Interface\n");

	// device setup sequence
	try {
		bugprintf("Setup sequence: start\n");
		vector<uint8_t> data = {0x01};
		char req(Request::TARGET_PLATFORM);
		vendor_transfer(0, req, 0, 0, data);
	}
	catch (...) {
		// Try deinit device and repeat.
		bugprintf("Setup sequence: deinit\n");
		vector<uint8_t> data = { 0x00, 0x00 };
		{
			char req(Request::SET_OPERATION_MODE);
			vendor_transfer(0, req, 0, 0, data);
			vendor_transfer(0, req, 0, 0, data);
			vendor_transfer(0, req, 0, 0, data);
		}

		{
			char req(Request::TARGET_PLATFORM);
			vendor_transfer(0, req, 0, 0, data);
		}
	}

	{
		bugprintf("Setup sequence: Set operation mode\n");
		char req(Request::SET_OPERATION_MODE);
		vector<uint8_t> data = {0x00, 0x00};
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Get firmware info\n");
		char req(Request::GET_FIRMWARE_INFO);
		vector<uint8_t> data(4);
		vendor_transfer(1, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Read chip ID\n");
		char req(Request::READ_CHIP_ID);
		vector<uint8_t> data(12);
		vendor_transfer(1, req, 0, 0, data);
	}


	{
		bugprintf("Setup sequence: Set factory settings features\n");
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x20, 0x00, 0x30, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Get factory settings\n");
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(64);
		vendor_transfer(1, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Set factory settings features\n");
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x20, 0x00, 0x50, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}


	{
		bugprintf("Setup sequence: Get factory settings\n");
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(64);
		vendor_transfer(1, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Set factory settings features\n");
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x0c, 0x00, 0x70, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}


	{
		bugprintf("Setup sequence: Get factory settings\n");
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(24);
		vendor_transfer(1, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Set factory settings features\n");
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x06, 0x00, 0x08, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Get factory settings\n");
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(12);
		vendor_transfer(1, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Set image processing mode\n");
		char req(Request::SET_IMAGE_PROCESSING_MODE);
		vector<uint8_t> data = { 0x08, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Get operation mode\n");
		char req(Request::GET_OPERATION_MODE);
		vector<uint8_t> data(2);
		vendor_transfer(1, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Set image processing mode\n");
		char req(Request::SET_IMAGE_PROCESSING_MODE);
		vector<uint8_t> data = { 0x08, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Set operation mode\n");
		char req(Request::SET_OPERATION_MODE);
		vector<uint8_t> data = { 0x01, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		bugprintf("Setup sequence: Get operation mode\n");
		char req(Request::GET_OPERATION_MODE);
		vector<uint8_t> data(2);
		vendor_transfer(1, req, 0, 0, data);
	}
}

void caminterface::exit()
{
	bugprintf("\n");
	if (handle == NULL) {
		return;
	}

	{
		char req(Request::SET_OPERATION_MODE);
		vector<uint8_t> data = { 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
		vendor_transfer(0, req, 0, 0, data);
		vendor_transfer(0, req, 0, 0, data);
	}

	if (handle != NULL) {
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		handle = NULL;
	}

	if (ctx != NULL) {
		libusb_exit(ctx);
		ctx = NULL;
	}
	bugprintf("\n");
}

void caminterface::vendor_transfer(bool direction,
 uint8_t req, uint16_t value, uint16_t index, vector<uint8_t> & data,
 int timeout)
{
	int res;
	uint8_t bmRequestType = (direction ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT)
	 | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE;
	uint8_t bRequest = req;
	uint16_t wValue = value;
	uint16_t wIndex = index;

	if (data.size() == 0) {
		data.reserve(16);
	}

	uint8_t * aData = data.data();
	uint16_t wLength = data.size();
	if (!direction) {
		// to device
		bugprintf("ctrl_transfer to dev(0x%x, 0x%x, 0x%x, 0x%x, %d)",
			 bmRequestType, bRequest, wValue, wIndex, wLength);

		res = libusb_control_transfer(handle, bmRequestType, bRequest,
			wValue, wIndex, aData, wLength, timeout);

		if (res != wLength) {
			fprintf(stderr, "\x1B[31;1mBad returned length: %d\x1B[0m\n", res);
		}

		//printData(data);
	}
	else {
		// from device
		bugprintf("ctrl_transfer from dev(0x%x, 0x%x, 0x%x, 0x%x, %d)",
			bmRequestType, bRequest, wValue, wIndex, wLength);
		res = libusb_control_transfer(handle, bmRequestType, bRequest,
			wValue, wIndex, aData, wLength, timeout);

		if (res != wLength) {
			fprintf(stderr, "\x1B[31;1mBad returned length: %d\x1B[0m\n", res);
		}

		//printData(data);
	}
}

void caminterface::frame_get_one(uint8_t * frame)
{
	int res;

	int size = WIDTH * HEIGHT;

	{ // request a frame
		//vector<uint8_t> data = { uint8_t(size & 0xff), uint8_t((size>>8)&0xff), 0, 0 };
		vector<uint8_t> data = { 0xC0, 0x7E, 0, 0 };
		vendor_transfer(0, 0x53, 0, 0, data);
	}
	int bufsize = size * sizeof(uint16_t);

	{
		int todo = bufsize;
		int done = 0;

		while (todo != 0) {
			int actual_length = 0;
			bugprintf("Asking for %d B of data at %d\n", todo, done);
			res = libusb_bulk_transfer(handle, 0x81, &frame[done],
			 todo, &actual_length, 500);
			if (res != 0) {
				fprintf(stderr, "\x1B[31;1m%s: libusb_bulk_transfer returned %d\x1B[0m\n", __func__, res);
			}
			bugprintf("Actual length %d\n", actual_length);
			todo -= actual_length;
			done += actual_length;
		}
	}

}

seekCam::seekCam()
{
	bugprintf("\n");
}

seekCam::~seekCam()
{
	bugprintf("\n");
	release();
}

bool seekCam::open()
{
	/* This function is going to take frame in util he gets a calib frame.
	 * On this frame it is going to search for blank pixels to build a list for the interpolation
	 */

	_cam.init();

	bool frame1 = false;
	bool frame4 = false;
	while (!(frame1 && frame4)) {
		_cam.frame_get_one(data);

		bugprintf("Status: %d\n", data[20]);

		if (data[20] == 1) {
			bugprintf("Calib\n");
			Mat *cal = getCalib();
			Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
			frame.convertTo(*cal, CV_32SC1);
			frame.release();

			/* Builing Black spot list */
			bugprintf("Building BP List\n");


			frame1 = true;
		}
		else if(data[20] == 4){
			Mat *cal = getCalib4();
			Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
			frame.convertTo(*cal, CV_32SC1);
			frame.release();
			buildBPList();

			frame4 = true;
		}
	}
	frameCounter = 0;
	isOpen = true;
	bugprintf("Init seek done\n-------------------------------\n");
	return true;
}

bool seekCam::isOpened()
{
	return isOpen;
}

void seekCam::release()
{
	bugprintf("\n");
	isOpen = false;
	_cam.exit();

}

bool seekCam::grab()
{
	for(int i=0; i<30; i++) {
		_cam.frame_get_one(data);

		bugprintf("Status: %d\n", data[20]);
		frameCounter = reinterpret_cast<uint16_t*>(&data[80])[0];



		if (data[20] == 1) {
			bugprintf("Calib\n");

			Mat *cal = getCalib();
			cal->release();
			Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
			frame.convertTo(frame, CV_32SC1);

			/*
			Scalar tempVal = mean(frame.col(206));
			double scl =  0.002 * tempVal[0];
			for(int i=0; i<HEIGHT; i++){
				for(int j=0; j<206; j++){
					frame.at<int32_t>(i, j) = frame.at<int32_t>(i, j) - (0.05 * frame.at<int32_t>(i, j)/scl - frame.at<int32_t>(i, 206)/scl);
				}
			}*/
			frame.copyTo(*cal);
			frame.release();
			continue;
		}

		if(data[20] == 4){
			Mat *cal = getCalib4();
			cal->release();
			Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
			frame.convertTo(frame, CV_32SC1);
			frame.copyTo(*cal);
			frame.release();
			continue;

		}

		if(data[20] == 3){
			return true;
		}

	}

	return false;
}

bool seekCam::retrieve(cv::OutputArray _dst)
{
	_dst.create( HEIGHT, WIDTH, CV_16SC1);
    Mat out = _dst.getMat();
	Mat *cal = getCalib();
	Mat *cal4 = getCalib4();
	Mat frame(HEIGHT, WIDTH, CV_16UC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);

	Mat div(HEIGHT, WIDTH, CV_32SC1);
	Mat r;

	div.setTo(2048);
	divide(frame, div, div, 1, CV_32SC1);
	div += 8;
	divide(*cal4, div, r, 1, CV_32SC1);

	frame.convertTo(frame, CV_32SC1);
	frame += level_shift - *cal;
	frame += r;
	frame.convertTo(out, CV_16UC1);

	frame.release();
	filterBP(out);

	out = out(Rect(0, 0, 206, 156)).clone();
	out.copyTo(_dst);
	out.copyTo(latestframe);

	return true;
}

bool seekCam::retrieveRaw(cv::OutputArray _dst)
{
	_dst.create( HEIGHT, WIDTH, CV_16SC1);
    Mat out = _dst.getMat();
	Mat *cal = getCalib();

	Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
	/*frame.convertTo(frame, CV_32SC1);
	frame += level_shift - *cal;

	out = frame(Rect(0, 0, 206, 156)).clone();
	*/
	frame.copyTo(_dst);
	frame.release();

	return true;
}

bool seekCam::read(OutputArray _dst)
{
	bool ret;
	ret = grab();
	ret = ret & retrieve(_dst);
	return ret;
}

seekCam& seekCam::operator >> (UMat& image)
{
    read(image);
    return *this;
}

seekCam& seekCam::operator >> (Mat& image)
{
    read(image);
    return *this;
}

double seekCam::get(int propId)
{
	switch(propId){
		case CV_CAP_PROP_POS_FRAMES:
			return frameCounter;
		case CV_CAP_PROP_POS_AVI_RATIO:
			return 0;
		case CV_CAP_PROP_FRAME_WIDTH:
			return 206;
		case CV_CAP_PROP_FRAME_HEIGHT:
			return 156;
		case CV_CAP_PROP_FORMAT:
			return CV_16UC1;

		default:
			return 0;
	}
}

bool seekCam::set(int propId, double value)
{
	return false;
}

double seekCam::getTemp(Point pt)
{
	/*
	 * Function to get the temperature of a specific point
	 * Warning: this only works if you have used retrieve or read() functions.
	 * Does not work when using retrieveRaw()
	 */
	double temp=0;

	temp = latestframe.at<int16_t>(pt) - 15000;
	temp /= 50.338;
	return temp;
}



/* Private */
Mat *seekCam::getCalib()
{
	return &calib;
}

Mat *seekCam::getCalib4()
{
	return &calib4;
}

void seekCam::buildBPList()
{
	//ofstream myfile;
	//myfile.open ("bp.txt");

	bp_list.clear();
	bp_list.push_back(Point(1, 0)); /* byte with temp vaue ?? */
	for(int y=0; y<calib4.rows; y++){
		for(int x=0; x<calib4.cols-2; x++){ /* Last 2 cols are a zero padding and some sort of mean */
			if(calib4.at<int32_t>(y, x) <= 30){	/* Status bit is not 0 && 1 calib frame as framenr 22*/
				bp_list.push_back(Point(x, y));
				//myfile << Point(x, y) << endl;
			}
		}
	}

  //myfile.close();

}

void seekCam::filterBP(Mat frame)
{
	int size = bp_list.size();
	float val;
	//int val2;

	for(int i=0; i<size; i++){
		Point px = bp_list[i];

		if(px.x>0 && px.x<WIDTH-3 && px.y>0 && px.y<HEIGHT-1){
			// load the four neighboring pixels
			const float p1 = frame.at<uint16_t>(px.y, px.x-1);
			const float p2 = frame.at<uint16_t>(px.y, px.x+1);
			const float p3 = frame.at<uint16_t>(px.y+1, px.x);
			const float p4 = frame.at<uint16_t>(px.y-1, px.x);

			//val = p1 * 0.25 + p2 * 0.25 + p3 * 0.25 + p4 * 0.25;
			val = (p1 + p2 + p3 + p4)/4;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==WIDTH-3 && px.y==0){
			//upper right corner
			const float p1 = frame.at<uint16_t>(px.y, px.x-1);
			const float p3 = frame.at<uint16_t>(px.y+1, px.x);

			//val = p1 * 0.5 + p3 * 0.5;
			val = (p1 + p3)/2;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==WIDTH-3 && px.y==HEIGHT-1){
			//lower right corner
			const float p1 = frame.at<uint16_t>(px.y, px.x-1);
			const float p4 = frame.at<uint16_t>(px.y-1, px.x);

			//val = p1 * 0.5 + p4 * 0.5;
			val = (p1 + p4)/2;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==WIDTH-3){
			//right side
			const float p1 = frame.at<uint16_t>(px.y, px.x-1);
			const float p3 = frame.at<uint16_t>(px.y+1, px.x);
			const float p4 = frame.at<uint16_t>(px.y-1, px.x);

			//val = p1 * 0.5 + p3 * 0.25 + p4 * 0.25;
			val = (p1 + p3 + p4)/3;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==0 && px.y==HEIGHT-1){
			//lower left corner
			const float p2 = frame.at<uint16_t>(px.y, px.x+1);
			const float p4 = frame.at<uint16_t>(px.y-1, px.x);

			//val = p2 * 0.5 + p4 * 0.5;
			val = (p2 + p4)/2;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.y==HEIGHT-1){
			//bottom side
			const float p1 = frame.at<uint16_t>(px.y, px.x-1);
			const float p2 = frame.at<uint16_t>(px.y, px.x+1);
			const float p4 = frame.at<uint16_t>(px.y-1, px.x);

			//val = p1 * 0.25 + p2 * 0.25 + p4 * 0.5;
			val = (p1 + p2 + p4)/3;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==0 && px.y==0){
			//Upper left corner
			const float p2 = frame.at<uint16_t>(px.y, px.x+1);
			const float p3 = frame.at<uint16_t>(px.y+1, px.x);

			//val = p2 * 0.5 + p3 * 0.5;
			val = (p2 + p3)/2;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==0){
			//left side
			const float p2 = frame.at<uint16_t>(px.y, px.x+1);
			const float p3 = frame.at<uint16_t>(px.y+1, px.x);
			const float p4 = frame.at<uint16_t>(px.y-1, px.x);

			//val = p2 * 0.5 + p3 * 0.25 + p4 * 0.25;
			val = (p2 + p3 + p4)/3;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.y==0){
			//top side
			const float p1 = frame.at<uint16_t>(px.y, px.x-1);
			const float p2 = frame.at<uint16_t>(px.y, px.x+1);
			const float p3 = frame.at<uint16_t>(px.y+1, px.x);

			//val = p1 * 0.25 + p2 * 0.25 + p3 * 0.5;
			val = (p1 + p2 + p3)/3;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
	}
}


