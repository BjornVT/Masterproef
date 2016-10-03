#include "seek.hpp"

using namespace std;
using namespace cv;
using namespace LibSeek;

caminterface::caminterface()
{
	init();
}

caminterface::~caminterface()
{
	exit();
}

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
	/* This function is going to take frame in util he gets a calib frame.
	 * On this frame it is going to search for blank pixels to build a list for the interpolation
	 * It's also goning to find the mean value for the level shift
	 */
	 
	while (true) {
		_cam.frame_get_one(data);

		bugprintf("Status: %d\n", data[20]);

		if (data[20] == 1) {
			bugprintf("Calib\n");
			Mat *cal = getCalib();		
			Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
			frame.convertTo(*cal, CV_32SC1);
			frame.release();
			
			/* Calculating level shift */
			bugprintf("Get mean of calib frame\n");
			cv::Scalar mean = cv::mean(*cal);
			level_shift = int(mean[0]);
			//level_shift = 8000;
			
			/* Builing Black spot list */
			bugprintf("Building BP List\n");
			buildBPList();
			
			break;
		}
	}
	
	bugprintf("Init seek done\n-------------------------------\n");
}

seekCam::~seekCam()
{
	_cam.exit();
}

void seekCam::exit()
{
	_cam.exit();
}

bool seekCam::grab()
{
	while (true) {
		_cam.frame_get_one(data);

		bugprintf("Status: %d\n", data[20]);
		
		if (data[20] == 1) {
			bugprintf("Calib\n");
			Mat *cal = getCalib();
			cal->release();			
			Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
			frame.convertTo(*cal, CV_32SC1);
			frame.release();

			continue;
		}

		if(data[20] == 3){
			return true;
		}
		
		bugprintf("Bad status: %d\n", status);
	}
}

cv::Mat seekCam::retrieve()
{
	Mat out;
	Mat *cal = getCalib();
	Mat frame(HEIGHT, WIDTH, CV_16SC1, reinterpret_cast<uint16_t*>(data), Mat::AUTO_STEP);
	frame.convertTo(frame, CV_32SC1);

	frame += level_shift - *cal;
	
	frame.convertTo(out, CV_16UC1);
	frame.release();
	filterBP(out);
	return out;
}

cv::Mat seekCam::read()
{
	grab();
	return retrieve();
}

Mat *seekCam::getCalib()
{
	return &calib;
}

void seekCam::buildBPList()
{
	for(int y=0; y<calib.rows; y++){
		for(int x=0; x<calib.cols; x++){
			if(calib.at<int32_t>(y, x) <= 0x10){	/* Status bit is not 0 */
				bp_list.push_back(Point(x, y));
			}
		}
	}
}

void seekCam::filterBP(Mat frame)
{
	int size = bp_list.size();
	float val;
	int val2;
	
	for(int i=0; i<size; i++){
		Point px = bp_list[i];
		
		if(px.x>0 && px.x<WIDTH-1 && px.y>0 && px.y<HEIGHT-1){
			// load the four neighboring pixels
			const uint16_t p1 = frame.at<uint16_t>(px.y, px.x-1);
			const uint16_t p2 = frame.at<uint16_t>(px.y, px.x+1);
			const uint16_t p3 = frame.at<uint16_t>(px.y+1, px.x);
			const uint16_t p4 = frame.at<uint16_t>(px.y-1, px.x);
			
			val = p1 * 0.25 + p2 * 0.25 + p3 * 0.25 + p4 * 0.25;
			//val2 = p1 + p2 + p3 + p4;
			//val2 = val2 >>12;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==WIDTH-1 && px.y==0){
			//upper right corner
			const uint16_t p1 = frame.at<uint16_t>(px.y, px.x-1);
			const uint16_t p3 = frame.at<uint16_t>(px.y+1, px.x);
			
			val = p1 * 0.5 + p3 * 0.5;
			//val2 = p1 + p3;
			//val2 = val2 >> 2;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==WIDTH-1 && px.y==HEIGHT-1){
			//lower right corner
			const uint16_t p1 = frame.at<uint16_t>(px.y, px.x-1);
			const uint16_t p4 = frame.at<uint16_t>(px.y-1, px.x);
			
			val = p1 * 0.5 + p4 * 0.5;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==WIDTH-1){
			//right side
			//This side is a full line of pixels
			//Take corners instead of sides
			const uint16_t p1 = frame.at<uint16_t>(px.y, px.x-1);
			const uint16_t p3 = frame.at<uint16_t>(px.y+1, px.x-1);
			const uint16_t p4 = frame.at<uint16_t>(px.y-1, px.x-1);
			
			val = p1 * 0.5 + p3 * 0.25 + p4 * 0.25;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==0 && px.y==HEIGHT-1){
			//lower left corner
			const uint16_t p2 = frame.at<uint16_t>(px.y, px.x+1);
			const uint16_t p4 = frame.at<uint16_t>(px.y-1, px.x);
			
			val = p2 * 0.5 + p4 * 0.5;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.y==HEIGHT-1){
			//bottom side
			const uint16_t p1 = frame.at<uint16_t>(px.y, px.x-1);
			const uint16_t p2 = frame.at<uint16_t>(px.y, px.x+1);
			const uint16_t p4 = frame.at<uint16_t>(px.y-1, px.x);
			
			val = p1 * 0.25 + p2 * 0.25 + p4 * 0.5;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==0 && px.y==0){
			//Upper left corner
			const uint16_t p2 = frame.at<uint16_t>(px.y, px.x+1);
			const uint16_t p3 = frame.at<uint16_t>(px.y+1, px.x);
			val = p2 * 0.5 + p3 * 0.5;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.x==0){
			//left side
			const uint16_t p2 = frame.at<uint16_t>(px.y, px.x+1);
			const uint16_t p3 = frame.at<uint16_t>(px.y+1, px.x);
			const uint16_t p4 = frame.at<uint16_t>(px.y-1, px.x);
			
			val = p2 * 0.5 + p3 * 0.25 + p4 * 0.25;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		else if(px.y==0){
			//top side
			const uint16_t p1 = frame.at<uint16_t>(px.y, px.x-1);
			const uint16_t p2 = frame.at<uint16_t>(px.y, px.x+1);
			const uint16_t p3 = frame.at<uint16_t>(px.y+1, px.x);
			
			val = p1 * 0.25 + p2 * 0.25 + p3 * 0.5;
			frame.at<uint16_t>(px) = uint16_t(val);
		}
		
		
	
	}
}
