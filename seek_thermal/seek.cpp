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

	bugprintf("\nDevice Count : %zd\n-------------------------------\n",cnt);

	bool found(false);
	for (int idx_dev = 0; idx_dev < cnt; idx_dev++) {
		dev = devs[idx_dev];
		res = libusb_get_device_descriptor(dev, &desc);
		if (res < 0) {
			libusb_free_device_list(devs, 1);
			throw runtime_error("Failed to get device descriptor");
		}


		res = libusb_open(dev, &handle);
		if (res < 0) {
			libusb_free_device_list(devs, 1);
			throw runtime_error("Failed to open device");
			continue;
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

	bugprintf("\nDevice found\n");

	int config2;
	res = libusb_get_configuration(handle, &config2);
	if (res != 0) {
		libusb_free_device_list(devs, 1);
		throw runtime_error("Couldn't get device configuration");
	}
	bugprintf("\nConfigured value : %d\n",config2);

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
	bugprintf("\nClaimed Interface\n");

	// device setup sequence

	try {
		vector<uint8_t> data = {0x01};
		char req(Request::TARGET_PLATFORM);
		vendor_transfer(0, req, 0, 0, data);
	}
	catch (...) {
		// Try deinit device and repeat.
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
		char req(Request::SET_OPERATION_MODE);
		vector<uint8_t> data = {0x00, 0x00};
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		char req(Request::GET_FIRMWARE_INFO);
		vector<uint8_t> data(4);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}

	{
		char req(Request::READ_CHIP_ID);
		vector<uint8_t> data(12);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}


	{
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x20, 0x00, 0x30, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(64);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}

	{
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x20, 0x00, 0x50, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}


	{
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(64);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}

	{
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x0c, 0x00, 0x70, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}


	{
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(24);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}

	{
		char req(Request::SET_FACTORY_SETTINGS_FEATURES);
		vector<uint8_t> data = { 0x06, 0x00, 0x08, 0x00, 0x00, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		char req(Request::GET_FACTORY_SETTINGS);
		vector<uint8_t> data(12);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}

	{
		char req(Request::SET_IMAGE_PROCESSING_MODE);
		vector<uint8_t> data = { 0x08, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		char req(Request::GET_OPERATION_MODE);
		vector<uint8_t> data(2);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}

	{
		char req(Request::SET_IMAGE_PROCESSING_MODE);
		vector<uint8_t> data = { 0x08, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		char req(Request::SET_OPERATION_MODE);
		vector<uint8_t> data = { 0x01, 0x00 };
		vendor_transfer(0, req, 0, 0, data);
	}

	{
		char req(Request::GET_OPERATION_MODE);
		vector<uint8_t> data(2);
		vendor_transfer(1, req, 0, 0, data);
		bugprintf("Response: ");
		for (uint i = 0; i < data.size(); i++) {
			printf(" %2x", data[i]);
		}
		bugprintf(" \n");
	}
}

void caminterface::exit()
{
	int res;

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
		res = libusb_release_interface(handle, 0);
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
		
		if(DEBUG){
			bugprintf("ctrl_transfer(0x%x, 0x%x, 0x%x, 0x%x, %d)",
			 bmRequestType, bRequest, wValue, wIndex, wLength);
			fprintf(stderr, " [");
			for (int i = 0; i < wLength; i++) {
				fprintf(stderr, " %02x", data[i]);
			}
			fprintf(stderr, " ]\n");
		}

		res = libusb_control_transfer(handle, bmRequestType, bRequest,
		 wValue, wIndex, aData, wLength, timeout);

		if (res != wLength) {
			fprintf(stderr, "\x1B[31;1mBad returned length: %d\x1B[0m\n", res);
		}
		
	}
	else {
		// from device
		bugprintf("ctrl_transfer(0x%x, 0x%x, 0x%x, 0x%x, %d)",
		 bmRequestType, bRequest, wValue, wIndex, wLength);
		res = libusb_control_transfer(handle, bmRequestType, bRequest,
		 wValue, wIndex, aData, wLength, timeout);
		 
		if (res != wLength) {
			fprintf(stderr, "\x1B[31;1mBad returned length: %d\x1B[0m\n", res);
		}
		
		if(DEBUG){
			fprintf(stderr, " -> [");
			for (auto & x: data) {
				fprintf(stderr, " %02x", x);
			}
			fprintf(stderr, "]\n");
		}
	}
}

void caminterface::frame_get_one(uint8_t * frame)
{
	int res;

	int size = WIDTH * HEIGHT;

	{ // request a frame
		vector<uint8_t> data = { uint8_t(size & 0xff), uint8_t((size>>8)&0xff), 0, 0 };
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
	 
	uint8_t data[WIDTH*HEIGHT*2] = {0};
	 
	while (true) {
		_cam.frame_get_one(data);

		uint8_t status = data[20];
		bugprintf("Status: %d\n", status);
		
		uint16_t img[HEIGHT*WIDTH];

		if (status == 1) {
			bugprintf("Calib\n");
			for (int y = 0; y < HEIGHT; y++) {
				for (int x = 0; x < WIDTH; x++) {
					uint16_t v = reinterpret_cast<uint16_t*>(data)[y*WIDTH+x];
					v = le16toh(v);
					
					img[y * WIDTH + x] = v;
				}
			}	
			Mat frame(HEIGHT, WIDTH, CV_16UC1, (void *)img, Mat::AUTO_STEP);
			frame.copyTo(*getCalib());
			
			/* Calculating level shift */
			cv::Scalar mean = cv::mean(frame);
			level_shift = int(mean[0]);
			
			/* Builing Black spot list */
			buildBPList();
			
			break;
		}
	}
	 
}

seekCam::~seekCam()
{
}

cv::Mat seekCam::frame_acquire()
{
	uint8_t data[WIDTH*HEIGHT*2] = {0};
		
	while (true) {
		_cam.frame_get_one(data);

		uint8_t status = data[20];
		bugprintf("Status: %d\n", status);
		
		uint16_t img[HEIGHT*WIDTH];

		if (status == 1) {
			bugprintf("Calib\n");
			for (int y = 0; y < HEIGHT; y++) {
				for (int x = 0; x < WIDTH; x++) {
					uint16_t v = reinterpret_cast<uint16_t*>(data)[y*WIDTH+x];
					v = le16toh(v);
					
					img[y * WIDTH + x] = v;
				}
			}	
			Mat frame(HEIGHT, WIDTH, CV_16UC1, (void *)img, Mat::AUTO_STEP);
			frame.copyTo(*getCalib());
			continue;
		}

		if(status == 3){
			//real frame
			Mat *cal = getCalib();
			for (int y = 0; y < HEIGHT; y++) {
				for (int x = 0; x < WIDTH; x++) {
					int a;

					uint16_t v = reinterpret_cast<uint16_t*>(data)[y*WIDTH+x];
					v = le16toh(v);

					a = int(v) - int(cal->at<uint16_t>(y, x));
					
					// level shift
					a += level_shift;

					if (a < 0) {
						a = 0;
					}
					if (a > 0xFFFF) {
						a = 0xFFFF;
					}

					img[y * WIDTH + x] = (uint16_t)a;
				}
			}
			
			Mat frame(HEIGHT, WIDTH, CV_16UC1, (void *)img, Mat::AUTO_STEP);
			
			filterBP(frame);

			return frame;
		}
		
		bugprintf("Bad status: %d\n", status);
		
		
/*
		for (int idx_bp = 0; idx_bp < m->bpc_list.size(); idx_bp++) {
			int x, y, ik;
			tie(x, y, ik) = m->bpc_list[idx_bp];
			//intf("Correcting %d %d/%d\n", idx_bp, x, y);
			auto cnt = m->bpc_kinds[ik];
			float v = 0;
			for (int idx_pt = 0; idx_pt < cnt.size(); idx_pt++) {
				int dx, dy, iw;
				tie(dx, dy, iw) = cnt[idx_pt];
				v += data[(y+dy)*w+(x+dx)] * m->bpc_weights[iw];
			}
			data[y*w+x] = v;
		}
*/
	}
}

Mat *seekCam::getCalib()
{
	return &calib;
}

void seekCam::buildBPList()
{
	for(int y=0; y<calib.rows; y++){
		for(int x=0; x<calib.cols; x++){
			if(calib.at<uint16_t>(y, x) == 0){
				bp_list.push_back(Point(x, y));
			}
		}
	}
}

void seekCam::filterBP(Mat frame)
{
	for(int i=0; i<bp_list.size(); i++){
		float val;
		Point px = bp_list[i];
		
		// load the four neighboring pixels
		const uint16_t p1 = frame.at<uint16_t>(Point(px.x-1, px.y));
		const uint16_t p2 = frame.at<uint16_t>(Point(px.x+1, px.y));
		const uint16_t p3 = frame.at<uint16_t>(Point(px.x, px.y+1));
		const uint16_t p4 = frame.at<uint16_t>(Point(px.x, px.y-1));
		
		
		val = p1 * 0.25 + p2 * 0.25 + p3 * 0.25 + p4 * 0.25;
		
		
		frame.at<uint16_t>(px) = uint16_t(val) ;
		
	}
}
