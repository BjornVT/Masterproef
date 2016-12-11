#include "readSettings.hpp"


Settings::Settings()
{
	bugprintf("\n");
}

Settings::~Settings()
{
	bugprintf("\n");
}

void Settings::write(FileStorage& fs) const                        //Write serialization for this class
{
	fs << "{" << "BoardSize_Width"  << boardSize.width
		  << "BoardSize_Height" << boardSize.height
		  << "Square_Size"         << squareSize
		  << "Calibrate_NrOfFrameToUse" << nrFrames
		  << "Input_Delay" << delay
		  << "threshold_value" << threshold_value
		  << "InputRGB" << inputString[RGB]
		  << "InputLWIR" << inputString[LWIR]
		  << "CalibType" << calib
		  << "Write_outputFileName"  << outputFileName
   << "}";
   
   bugprintf("\n");
}
	
void Settings::read(const FileNode& node)                          //Read serialization for this class
{
	node["BoardSize_Width" ] >> boardSize.width;
	node["BoardSize_Height"] >> boardSize.height;
	node["Square_Size"]  >> squareSize;
	node["Calibrate_NrOfFrameToUse"] >> nrFrames;
	node["Input_Delay"] >> delay;
	node["threshold_value"] >> threshold_value;
	node["InputRGB"] >> inputString[RGB];
	node["InputLWIR"] >> inputString[LWIR];
	node["CalibType"] >> calib;
	node["Write_outputFileName"] >> outputFileName;
	interprate();
	
	bugprintf("\n");
}    

void Settings::interprate()
{
	goodInput = true;
	if(boardSize.width <= 0 || boardSize.height <= 0){
		cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
		goodInput = false;
	}
	if(squareSize <= 10e-6){
		cerr << "Invalid square size " << squareSize << endl;
		goodInput = false;
	}
	if(nrFrames <= 0){
		cerr << "Invalid number of frames " << nrFrames << endl;
		goodInput = false;
	}
	if(delay <= 0){
		cerr << "Invalid delay time " << delay << endl;
		goodInput = false;
	}
	if(threshold_value < 0 || threshold_value > 255){
		cerr << "Invalid threshold " << threshold_value << endl;
		goodInput = false;
	}
	if(calib > 2 || calib < 0){
		cerr << "Invalid calib type " << calib << endl;
		goodInput = false;
	}
	
		
	/* Input RGB */
	if(inputString[RGB].empty()){      // Check for valid input
		cerr << " Inexistent RGB input: " << input << endl;
		goodInput = false;
		type[RGB] = INVALID;
	}
	else{
		if(inputString[RGB][0] >= '0' && inputString[RGB][0] <= '9'){
			/* Cam with Linux backend */
			stringstream ss(inputString[RGB]);
			ss >> cameraID[RGB];
			video[RGB].open(cameraID[RGB]);
			input[RGB] = (void *)&video[RGB];
			type[RGB] = CAMERA;
		}
		else{
			/* Video */
			video[RGB].open(inputString[RGB]);
			input[RGB] = (void *)&video[RGB];
			type[RGB] = VIDEO_FILE;
		}
	}
	
	/* Input LWIR */
	if(inputString[LWIR].empty()){      // Check for valid input
		cerr << " Inexistent LWIR input: " << input << endl;
		goodInput = false;
		type[LWIR] = INVALID;
	}
	else{
		if (inputString[LWIR][0] >= '0' && inputString[LWIR][0] <= '9'){
			/* Cam with Linux backend */
			stringstream ss(inputString[LWIR]);
			ss >> cameraID[LWIR];
			video[LWIR].open(cameraID[LWIR]);
			input[LWIR] = (void *)&video[LWIR];
			type[LWIR] = CAMERA;
		}
		else{
			if(inputString[LWIR].compare("LIBSEEK") == 0){
				/* Cam with LibSeek */
				if(seek.open()){
					bugprintf("Openen seek gelukt\n");
				}
				input[LWIR] = (void *)&seek;
				type[LWIR] = LIBSEEK;
			}
			else{
				/* Video */
				video[LWIR].open(inputString[LWIR]);
				input[LWIR] = (void *)&video[LWIR];
				type[LWIR] = VIDEO_FILE;
			}
		}
	}
	
	bugprintf("\n");
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
	bugprintf("\n");
	
	for(int i=0; i<2; i++){
		if(type[i] == Settings::InputType::LIBSEEK){
			imageSize[i].height = ((LibSeek::seekCam *)(input[i]))->get(CV_CAP_PROP_FRAME_HEIGHT);
			imageSize[i].width = ((LibSeek::seekCam *)(input[i]))->get(CV_CAP_PROP_FRAME_WIDTH);
		}
		else{
			imageSize[i].height = ((VideoCapture *)(input[i]))->get(CV_CAP_PROP_FRAME_HEIGHT);
			imageSize[i].width = ((VideoCapture *)(input[i]))->get(CV_CAP_PROP_FRAME_WIDTH);
		}
	}
	bugprintf("\n");
}



