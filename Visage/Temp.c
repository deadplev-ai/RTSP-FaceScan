//doxygen
/*! \file */

#include "../include/visageVision.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <unistd.h>
#include "Common.h"
#include "term.h"
#include <math.h>
#include <sys/timeb.h>
#include "VisageDrawing.h"
#include "LicenseString.h"

using namespace std;
using namespace VisageSDK;
using namespace cv;

VisageTracker *m_Tracker = 0;

Mat framePtr;

const int MAX_FACES = 4;
static FaceData trackingData[MAX_FACES];

int *track_stat = 0;

long trackingTime;

//if user closes window used for displaying results, this will be set to 1 and tracking will stop
bool exitApp;

Mat imageBGRA;

/**
 * 
 */
void flushInput(void)
{
    int c;
	
    do 
	{
        c = getchar();
    } while (c != '\n' && c != EOF);
}

/**
* Function for clearing console window. 
*/
void clearScreen(void)
{
	if (!cur_term)
	{
		int result;
		setupterm( NULL, STDOUT_FILENO, &result );
		
		if (result <= 0) return;
	}

  putp( tigetstr( "clear" ) );
}

void displayResults()
{
	clearScreen();
	printf("Tracking in progress!\n\n");

	char *tstatus;
	
	for (int i = 0; i < MAX_FACES; i++)
	{
		if (track_stat[i] == TRACK_STAT_OFF)
			tstatus = "OFF";
	}
	
	for (int i = 0; i < MAX_FACES; i++)
	{
		if (track_stat[i] == TRACK_STAT_INIT)
			tstatus = "INITIALIZING";
	}
	
	for (int i = 0; i < MAX_FACES; i++)
	{
		if (track_stat[i] == TRACK_STAT_RECOVERING)
			tstatus = "RECOVERING";
	}
	
	for (int i = 0; i < MAX_FACES; i++)
	{
		if (track_stat[i] == TRACK_STAT_OK)
			tstatus = "OK";
	}

	printf(" Tracking status: %s\n\n", tstatus);

	static float eyeDistance = -1.0f;

	// Example code for accessing specific feature points, in this case nose tip and eyes
	if(track_stat[0] == TRACK_STAT_OK)
	{
		FDP *fp = trackingData[0].featurePoints3DRelative;
		float nosex = fp->getFPPos(9,3)[0];
		float nosey = fp->getFPPos(9,3)[1];
		float nosez = fp->getFPPos(9,3)[2];
		float lex = fp->getFPPos(3,5)[0];
		float ley = fp->getFPPos(3,5)[1];
		float lez = fp->getFPPos(3,5)[2];
		float rex = fp->getFPPos(3,6)[0];
		float rey = fp->getFPPos(3,6)[1];
		float rez = fp->getFPPos(3,6)[2];
		float eyedist = lex -rex;
		int dummy = 0;
	}

	// display the frame rate, position and other info
	float trackingFrameRate = trackingData[0].frameRate;

	if (!imageBGRA.empty())
		printf(" %4.1f FPS (track %ld ms) \n Resolution: input %dx%d\n\n",trackingFrameRate, trackingTime, framePtr.size().width, framePtr.size().height);
	
	imshow("SimpleTracker",imageBGRA);
	waitKey(10);
}


/**
* Function used for measuring tracker speed. 
* Used for video synchronization (if flag video_file_sync is set to 1)
*
*/
static long getCurrentTimeMs()
{
	struct timeb timebuffer;
	ftime( &timebuffer );
	
	long clockTime = 1000 * (long)timebuffer.time + timebuffer.millitm;

	return clockTime;
}


/**
* The main function for tracking from a video file. 
* It is called when the 'v' is chosen.
*
*/
void trackFromVideo(const char *strAviFile)
{
	double frameTime;
	int frameCount = 0;
	
	//If the flag is set to 1 playback is synchronised by skipping video frames 
	//or introducing delay as needed, so the video file is played at its normal (or desired) speed (set variable targetFps to desired fps)
	bool video_file_sync = true;
	
	long currentTime;
	
	//set to desired fps you wish to achieve from tracker
	float targetFps = 30.0;
	//float targetFps = cvGetCaptureProperty( capture, CV_CAP_PROP_FPS );
	
	namedWindow("SimpleTracker", WINDOW_AUTOSIZE);
	moveWindow("SimpleTracker",100,100);

	VideoCapture capture(strAviFile);

	if(!capture.isOpened())
	{
		printf("Can't read video file.");
		return;
	}
	
	// moment when video started
	long startTime = getCurrentTimeMs();
	
	//frame duration if video fps was equal to targetFps
	frameTime = 1000.0/targetFps;
	
	Mat logo = imread("../Samples/Linux/data/images/logo.png", IMREAD_UNCHANGED);
	
	//tracking is performed while user doesn't close window for displaying results
	while(!exitApp)
	{
		if(!capture.isOpened())
			break;
		
		if (!capture.grab())
			break;
		frameCount++;
		
		if (video_file_sync)
		{
			currentTime = getCurrentTimeMs();
			
			//checking if tracker is too slow
			while((currentTime-startTime) > frameTime*(1+frameCount) )
			{
				if(!capture.grab()) //end of stream
						return;
				frameCount++;
				currentTime = getCurrentTimeMs();
			}	
			
			currentTime = getCurrentTimeMs();
			
			//checking if tracker is too fast (we don't want to risk unnecessary delay so we allow some extra frames, that's why framecount-5)		
			while((currentTime-startTime) < frameTime*(frameCount-5))
			{
				sleep(0.001);
				currentTime = getCurrentTimeMs();
			}
		}
		
		if(!capture.retrieve(framePtr))
			break;
		long startTime = getCurrentTimeMs();
		track_stat = m_Tracker->track(framePtr.size().width, framePtr.size().height, (const char*)framePtr.data, trackingData, VISAGE_FRAMEGRABBER_FMT_BGR, VISAGE_FRAMEGRABBER_ORIGIN_TL, framePtr.step, frameTime*frameCount, MAX_FACES);

		long endTime = getCurrentTimeMs();
		trackingTime = endTime - startTime;
		
		for (int i= 0; i<MAX_FACES; i++)
		{
			if (track_stat[i] == TRACK_STAT_OK)
				VisageDrawing::drawResults(framePtr, &trackingData[i]);
		}
		cvtColor(framePtr, imageBGRA, COLOR_BGR2BGRA);
		
		if (!logo.empty())
			VisageDrawing::drawLogo(imageBGRA, logo);
		displayResults();
	}
	
	capture.release();
}


/**
* Function for tracking from RTSP stream.
*
*/
void trackFromRTSP(const char *rtspUrl)
{
    namedWindow("SimpleTracker", WINDOW_AUTOSIZE);
    moveWindow("SimpleTracker", 100, 100);

    VideoCapture capture(rtspUrl);

    if (!capture.isOpened())
    {
        printf("Can't open RTSP stream: %s\n", rtspUrl);
        return;
    }

    Mat logo = imread("../Samples/Linux/data/images/logo.png", IMREAD_UNCHANGED);
    while (!exitApp)
    {
        if (!capture.grab())
            break;

        if (!capture.retrieve(framePtr))
            break;

        long startTime = getCurrentTimeMs();
        track_stat = m_Tracker->track(framePtr.size().width, framePtr.size().height,
            (const char*)framePtr.data, trackingData, VISAGE_FRAMEGRABBER_FMT_BGR,
            VISAGE_FRAMEGRABBER_ORIGIN_TL, framePtr.step, -1, MAX_FACES);

        long endTime = getCurrentTimeMs();
        trackingTime = endTime - startTime;

        for (int i = 0; i < MAX_FACES; i++)
        {
            if (track_stat[i] == TRACK_STAT_OK)
                VisageDrawing::drawResults(framePtr, &trackingData[i]);
        }

        cvtColor(framePtr, imageBGRA, COLOR_BGR2BGRA);

        if (!logo.empty())
            VisageDrawing::drawLogo(imageBGRA, logo);
        displayResults();
    }

    capture.release();
}

/**
* The main function for tracking from a video camera connected to the computer.
* It is called when the 'c' is chosen.
*
*/
void trackFromCam()
{
	double cam_width = 640;
	double cam_height = 480;
	double cam_device = 0;
	
	namedWindow("SimpleTracker", WINDOW_AUTOSIZE);
	moveWindow("SimpleTracker",100,100);

	VideoCapture capture( cam_device );
	capture.set(CAP_PROP_FRAME_WIDTH, cam_width);
	capture.set(CAP_PROP_FRAME_HEIGHT, cam_height);
	capture.set(CAP_PROP_FPS, 30);
	
	if (!capture.isOpened())
	{
		printf("Can't connect to the camera\n");
		return;
	}
	
	Mat logo = imread("../Samples/Linux/data/images/logo.png", IMREAD_UNCHANGED);
	
	while (!exitApp)
	{
		if (!capture.grab())
			break;

		if (!capture.retrieve(framePtr))
			break;

		long startTime = getCurrentTimeMs();
		track_stat = m_Tracker->track(framePtr.size().width, framePtr.size().height, 
			(const char*)framePtr.data, trackingData, VISAGE_FRAMEGRABBER_FMT_BGR, 
			VISAGE_FRAMEGRABBER_ORIGIN_TL, framePtr.step, -1, MAX_FACES);

		long endTime = getCurrentTimeMs();
		trackingTime = endTime - startTime;

		for (int i = 0; i < MAX_FACES; i++)
		{
			if (track_stat[i] == TRACK_STAT_OK)
				VisageDrawing::drawResults(framePtr, &trackingData[i]);
		}

		cvtColor(framePtr, imageBGRA, COLOR_BGR2BGRA);

		if (!logo.empty())
			VisageDrawing::drawLogo(imageBGRA, logo);
		displayResults();
	}

	capture.release();
}


/**
* The main function of the program. 
*
* This function will ask user to choose the source of the input (video file, camera, or RTSP stream)
* and then start tracking based on the selected option.
*
*/
int main(int argc, const char* argv[])
{
    char source;
    exitApp = false;

    // Initialize the tracker
    m_Tracker = new VisageTracker();
    if (!m_Tracker)
    {
        printf("Could not create Visage Tracker\n");
        return -1;
    }

    // Ask user for input source
    printf("Track from (v)ideo, (c)am, or (r)tsp? ");
    scanf(" %c", &source);
    flushInput();

    if (source == 'r')
    {
        std::string rtspUrl;
        printf("Enter RTSP URL: ");
        std::getline(std::cin, rtspUrl);
        trackFromRTSP(rtspUrl.c_str());
    }
    else if (source == 'v')
    {
        char videoFilename[256];
        printf("Enter video filename: ");
        scanf("%s", videoFilename);
        trackFromVideo(videoFilename);
    }
    else if (source == 'c')
    {
        trackFromCam();
    }
    else
    {
        printf("Wrong choice!\n");
    }

    delete m_Tracker;

    return 0;
}
