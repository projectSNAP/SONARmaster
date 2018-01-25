// SONARBackEnd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <fstream>
#include<windows.h>
#include<direct.h>




using namespace cv;
using namespace std;

#define verticalsources 16
#define AspectRatio 0.75
#define fovDeg 58.5
#define PI 3.1415926535


#define CASE_RETURN(err) case (err): return "##err"

#ifdef _MSC_VER
#    pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif


int debug = 1;
int xSize = 640;
int ySize = 480;

//User parameters for audio stuff. These are changed if the UserParameters file is successfully opened
float freq = 130.f; //Lowest frequency
int horizontal_steps = 20;
float freqInc = 1.3f;
int stepDelay = 45; //Delay between switching between horizontal steps in location
float AudioSpreadDeg = 180;
float AudioVolRollOff = 0.77;

const char* al_err_str(ALenum err) {
	switch (err) {
		CASE_RETURN(AL_NO_ERROR);
		CASE_RETURN(AL_INVALID_NAME);
		CASE_RETURN(AL_INVALID_ENUM);
		CASE_RETURN(AL_INVALID_VALUE);
		CASE_RETURN(AL_INVALID_OPERATION);
		CASE_RETURN(AL_OUT_OF_MEMORY);
	}
	return "unknown";
}
#undef CASE_RETURN

#define __al_check_error(file,line) \
    do { \
        ALenum err = alGetError(); \
        for(; err!=AL_NO_ERROR; err=alGetError()) { \
            std::cerr << "AL Error " << al_err_str(err) << " at " << file << ":" << line << std::endl; \
        } \
    }while(0)

#define al_check_error() \
    __al_check_error(__FILE__, __LINE__)


void init_al() {
	ALCdevice *dev = NULL;
	ALCcontext *ctx = NULL;

	const char *defname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	if(debug)
		std::cout << "Default device: " << defname << std::endl;

	dev = alcOpenDevice(defname);
	ctx = alcCreateContext(dev, NULL);
	alcMakeContextCurrent(ctx);
}

void exit_al() {
	ALCdevice *dev = NULL;
	ALCcontext *ctx = NULL;
	ctx = alcGetCurrentContext();
	dev = alcGetContextsDevice(ctx);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(ctx);
	alcCloseDevice(dev);
}

void CreateXY(Mat* Xmat, Mat* Ymat) {
	float thetaY;
	float thetaX;
	float PhiZX;
	float PhiZY;
	float a1;
	float a2;
	float PhiZ;
	float Xpos;
	float Ypos;

	for (int y = 0; y < xSize; y++) {
		for (int x = 0; x < ySize; x++) {
			thetaY = (-fovDeg / 2) + (fovDeg*y / xSize);
			thetaX = (-fovDeg / (2 / AspectRatio)) + (fovDeg*x / (xSize / AspectRatio));

			PhiZX = (90 - abs(thetaX)) * (180 / PI);
			PhiZY = (90 - abs(thetaY)) * (180 / PI);

			a1 = pow(1.0F / tan(PhiZX), 2);
			a2 = pow(1.0F / tan(PhiZY), 2);

			PhiZ = atan(1 / sqrt(a1 + a2));

			Xpos = cos(PhiZ) * cos(atan2(thetaY, (thetaX + 0.0001)));
			Ypos = cos(PhiZ) * sin(atan2(thetaY, (thetaX + 0.0001)));

			Xmat->at<float>(Point(y, x)) = Xpos;
			Ymat->at<float>(Point(y, x)) = Ypos;
		}
	}
}

void initDim() {
	int x; int y;
	ifstream readFile;
	readFile.open("./dat/dimensions.txt");
	if (!readFile.good())
	{
		readFile.close();
		readFile.open("..\\..\\..\\SonarGraphicsTestProj\\dimensions.txt");
	}
	if (!readFile.good())
	{
		readFile.close();
		readFile.open("..\\..\\SonarGraphicsTestProj\\dimensions.txt");
	}
	if (!readFile.good())
	{
		readFile.close();
		readFile.open("dimensions.txt");
	}
	while (!readFile.good())
	{
		char buff[100];
		_getcwd(buff, FILENAME_MAX);
		std::string current_working_dir(buff);
		cout << "loc is: " << current_working_dir << endl;
		cout << "Enter file location" << endl;
		string loc;
		cin >> loc;
		readFile.open(loc);
		/*cerr << "Bad file access";
		exit(-1);*/
	}

	readFile >> x;
	readFile >> y;
	xSize = x; ySize = y;
	if(debug) {
		printf("x size is: %d, y size is: %d\n", x, y);
	}
	readFile.close();

}

void readUserParamFile() {

	ifstream readFile;
	/*ofstream testFile;
	testFile.open("TEST.txt");
	testFile << "Test";
	testFile.close();*/


	readFile.open("./dat/UserParameters.txt");
	
	
	if(!readFile.good()){
		readFile.open("../x64/Debug/UserParameters.txt");
		if (!readFile.good())
		{
			readFile.open("../SONARBackEnd/x64/Debug/UserParameters.txt");
		}
		if (!readFile.good())
		{
			readFile.open("UserParameters.txt");
		}
		while(!readFile.good()){
			char buff[100];
			_getcwd(buff, FILENAME_MAX);
			std::string current_working_dir(buff);
			cout << "loc is: " << current_working_dir << endl;
			cout << "Enter file location" << endl;
			string loc;
			cin >> loc;
			readFile.open(loc);
		}
	}

	if (readFile.good())
	{
		readFile >> freq;
		readFile >> freqInc;
		readFile >> horizontal_steps;
		readFile >> stepDelay;
		readFile >> AudioSpreadDeg;
		readFile >> AudioVolRollOff;
	}
	if(debug)
		printf("freq is %3.2f, freq increment is %2.2f\n horizontal steps is %d, step delay is %d ms\n", freq, freqInc, horizontal_steps, stepDelay);
	readFile.close();
}

int main()
{
	initDim();
	readUserParamFile();

	void* PointerToBuf = OpenDepthBufMapFileToRead(xSize,ySize);
	if(debug)
		printf("%X \n", ReadDepthMapBufFile(PointerToBuf));
	
	Mat Xmat = Mat(ySize, xSize, CV_32FC1);
	Mat Ymat = Mat(ySize, xSize, CV_32FC1);

	CreateXY(&Xmat, &Ymat);

	int* dst = new int[xSize * ySize];
	memcpy(dst, ReadDepthMapBufFile(PointerToBuf), xSize * ySize * 4);
	Mat image = Mat(ySize, xSize, CV_16UC2, dst);
	Mat planes[2];
	split(image, planes);

	if (!image.data)                              // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
		return -1;
	}

	//OpenAL stuff----------------------------------
	unsigned short pointdist;
	float pointdistnorm;
	float StartAngle = (180 - AudioSpreadDeg)/360;
	float AngleMult = AudioSpreadDeg / 180;
	float zdist;
	float xdist;
	float angle;

	init_al();

	ALuint buf[verticalsources];
	alGenBuffers(verticalsources, buf);
	al_check_error();

	/* Fill buffer with Sine-Wave */
	float seconds = 1;
	float amplitude = 0.1f;

	unsigned sample_rate = 22050;
	size_t buf_size = seconds * sample_rate;

	short *samples;
	samples = new short[buf_size];

	//Set up the frequencied buffers
	for (int q = 0; q < verticalsources; q++){
		for (int i = 0; i<buf_size; ++i) {
			samples[i] = 16380 * sin((2.f*float(PI)*freq) / sample_rate * i) * amplitude;
		}

		amplitude = amplitude * AudioVolRollOff;

		freq = freq*freqInc;
		/* Download buffer to OpenAL */
		alBufferData(buf[q], AL_FORMAT_MONO16, samples, buf_size * 2, sample_rate);
		al_check_error();
	}
	
	//Generate 16 sources
	ALuint *srclist;
	srclist = new ALuint[verticalsources];

	alGenSources(verticalsources, srclist);
	int y;
	int x;

	int xPix;

	int sourceMatCoords[verticalsources];
	float sourcePos[verticalsources];

	int64 tick = 0;
	float secondselapsed;
	int64 tick2 = 0;
	float secondselapsed2  = 0;

	for (int i = 0; i < verticalsources; ++i) {
		//Vertical position
		x = i;
		xPix = (ySize/(verticalsources *2))+(ySize/ verticalsources)*x;

		sourcePos[i] = -2;

		sourceMatCoords[i] = xPix;

		alSourcei(srclist[i], AL_BUFFER, buf[i]);
		alSourcef(srclist[i], AL_REFERENCE_DISTANCE, 1.0f);
		alSourcei(srclist[i], AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(srclist[i], AL_LOOPING, AL_TRUE);
		alSource3f(srclist[i], AL_POSITION, -2, 0, 1);
		alSourcePlay(srclist[i]);

	}


	//-------------------------------Back to openCV stuff

	//namedWindow("Display window", WINDOW_AUTOSIZE);// Create a window for display.
	//imshow("Display window", planes[0]);                   // Show our image inside it.

	int64 delay = 0;
	int64 delayTick = 0;
	int horizpos = 0;
	bool cPressed = false;
	int keyCode = 255;
	// 255: 'no key' press, < 0: 'no key registered', 99: 'c' pressed
	while(keyCode == 255 || keyCode < 0 || keyCode == 99){
		keyCode = waitKey(0);
		// Pressed c, hide/show window
		//Sleep(stepDelay);
		delayTick = getTickCount();

		if (keyCode == 99)
		{
			cPressed = !cPressed;
			printf("cpressed is: %d\n", cPressed);
		}
		//printf("key code is: %d\n", keyCode);
		if (horizpos == horizontal_steps) {
			//printf("new thing \n");
			horizpos = 0;
		}

		if (horizpos == 0) {
			memcpy(dst, ReadDepthMapBufFile(PointerToBuf), xSize * ySize * 4);
			split(image, planes);
			Mat flipped;
			flip(planes[0], flipped, 0);
			//if (!cPressed)
			//{
			//	imshow("Display window", flipped);
			//}
			//else
			//{
			//	Mat blank(1, 1, CV_8UC3, Scalar(0, 0, 0));
			//	imshow("Display window", blank);
			//}

			secondselapsed = (getTickCount() - tick) / getTickFrequency();
			if (debug) {
				printf("%2.4f \n", secondselapsed);
				printf("%2.6f \n\n", secondselapsed2);
			}
			secondselapsed2 = 0;
			tick = getTickCount();
		}
		
		//OpenAL Stuff-----------------------------------
		tick2 = getTickCount();
		
		for (int i = 0; i < verticalsources; ++i) {
			//defines region of interest for this source
			cv::Rect roi((xSize/horizontal_steps)*horizpos, (ySize/ verticalsources)*i, (xSize/horizontal_steps), (ySize/ verticalsources));

			//copies input image in roi
			cv::Mat image_roi = planes[1](roi);

			//computes mean over roi
			cv::Scalar avgPixelIntensity = cv::mean(image_roi);

			//prints out only .val[0] since image was grayscale
			int pointdist = avgPixelIntensity.val[0];
			
			pointdistnorm = float(pointdist) / 255;
			rectangle(planes[0], Point(horizpos*(xSize / horizontal_steps), sourceMatCoords[i]), Point(horizpos*(xSize / horizontal_steps) + 3, sourceMatCoords[i] + 3), Scalar(255));

			angle = PI * (StartAngle + AngleMult * (float(horizpos) / float(horizontal_steps-1)));

			xdist = -cos(angle);
			zdist = sin(angle);

			alSource3f(srclist[i], AL_POSITION, xdist, 0, zdist);

			alSourcef(srclist[i], AL_GAIN, exp( 6.908*(1-pointdistnorm) )/1000); //should be 6.908 for normal rolloff
		}
		//End openAL stuff-------------------------------------------

		horizpos++;

		delay = ((getTickCount() - delayTick) * 1000) / getTickFrequency();
		while (delay < stepDelay) { delay = ((getTickCount() - delayTick) * 1000) / getTickFrequency(); }
	}
	UnmapDepthBufFile(PointerToBuf);

	//OpenAL stuff-----------------
	alDeleteSources(verticalsources, srclist);
	exit_al();
	al_check_error();
	//Stop OpenAL stuff-------------------

	delete dst;
	return 0;
}
