
#include "camSettings.hpp"


void camSettings::write(FileStorage& fs) const                        //Write serialization for this class
{
	fs 	<< "{" 
			<< "cameraMatrixRGB"	<< cameraMatrix[RGB]
			<< "cameraMatrixLWIR" 	<< cameraMatrix[LWIR]
			<< "distCoeffsRGB"      << distCoeffs[RGB]
			<< "distCoeffsLWIR" 	<< distCoeffs[LWIR]
			<< "totalAvgErrRGB" 	<< totalAvgErr[RGB]
			<< "totalAvgErrLWIR" 	<< totalAvgErr[LWIR]
			<< "rmsRGB"				<< rms[RGB]
			<< "rmsLWIR"			<< rms[LWIR]
			<< "H"					<< H
		<< "}";
		
}
	
void camSettings::read(const FileNode& node)                          //Read serialization for this class
{
	node["cameraMatrixRGB" ] 	>> cameraMatrix[RGB];
	node["cameraMatrixLWIR"] 	>> cameraMatrix[LWIR];
	node["distCoeffsRGB"]  		>> distCoeffs[RGB];
	node["distCoeffsLWIR"] 		>> distCoeffs[LWIR];
	node["totalAvgErrRGB"] 		>> totalAvgErr[RGB];
	node["totalAvgErrLWIR"] 	>> totalAvgErr[LWIR];
	node["rmsRGB"] 				>> rms[RGB];
	node["rmsLWIR"] 			>> rms[LWIR];
	node["H"]					>> H;
}    

bool camSettings::GoodCam()
{
	for(int i=0; i<2; i++){
		if(!(cameraMatrix[i].size() == Size(3, 3)))
		{
			return false;
		}
		if(!(distCoeffs[i].size() == Size(5, 1)))
		{
			return false;
		}
	}
	
	
	return true;
}

bool camSettings::GoodH()
{
	if(!(H.size() == Size(3, 3)))
	{
		return false;
	}
	
	return true;
}

/*
void camSettings::interprate()
{
	goodInput = true;
	
	/*
	if(!(cameraMatrix[RGB].size() == Size(3, 3) &&  cameraMatrix[RGB].at<double>(0, 0) != 0))
	{
		goodInput = false;
	}
	
	
	//Mat cameraMatrix[2] = {Mat::eye(3, 3, CV_64F)};
	//Mat distCoeffs[2] = {Mat::zeros(8, 1, CV_64F)};
	/*
	if(boardSize.width <= 0 || boardSize.height <= 0){
		cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
		goodInput = false;
	}
	if(squareSize <= 10e-6){
		cerr << "Invalid square size " << squareSize << endl;
		goodInput = false;
	}
	
}
*/

