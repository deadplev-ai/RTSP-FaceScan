THIS DOES NOT WORK YET

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720
#define TOTAL_COUNT 3

using namespace cv;
using namespace std;

CascadeClassifier face_cascade;
VideoCapture cap_viewer;
VideoCapture cap_analysis;
pthread_t analysis_thread;
vector<Rect> faces;
int count = 0;

void* analyze_frame(void* arg) {
    Mat* frame = (Mat*)arg;
    // Placeholder for analysis logic
    // In a real implementation, you would call a face analysis library here
    printf("Analyzing frame...\n");
    return NULL;
}

void frame_update(int, void*) {
    Mat frame_viewer, frame_analysis;
    for (int i = 0; i < 5; i++) {
        cap_viewer.read(frame_viewer);
        cap_analysis.read(frame_analysis);
    }

    if (!frame_viewer.empty() && !frame_analysis.empty()) {
        resize(frame_viewer, frame_viewer, Size(FRAME_WIDTH * 0.75, FRAME_HEIGHT * 0.75));
        resize(frame_analysis, frame_analysis, Size(FRAME_WIDTH * 0.75, FRAME_HEIGHT * 0.75));

        Mat gray_frame;
        cvtColor(frame_analysis, gray_frame, COLOR_BGR2GRAY);
        face_cascade.detectMultiScale(gray_frame, faces, 1.05, 3, 0, Size(30, 30));

        if (!faces.empty()) {
            if (count >= TOTAL_COUNT) {
                pthread_create(&analysis_thread, NULL, analyze_frame, &frame_analysis);
                count = 0;
            } else {
                count++;
            }
        } else {
            faces.clear();
        }

        for (size_t i = 0; i < faces.size(); i++) {
            rectangle(frame_viewer, faces[i], Scalar(0, 255, 0), 2);
        }

        imshow("DeepFace RTSP Stream (Vivotek)", frame_viewer);
    }

    waitKey(100);
    frame_update(0, 0);
}

int main(int argc, char** argv) {
    if (!face_cascade.load("haarcascade_frontalface_default.xml")) {
        printf("Error loading face cascade\n");
        return -1;
    }

    cap_viewer.open("rtsp://root:vvtk1234@192.168.3.206:554/live1s1.sdp?tcp&buffer_size=4096");
    cap_analysis.open("rtsp://root:vvtk1234@192.168.3.206:554/live1s1.sdp?tcp&buffer_size=4096");

    if (!cap_viewer.isOpened() || !cap_analysis.isOpened()) {
        printf("Error: Unable to open the RTSP stream.\n");
        return -1;
    }

    namedWindow("DeepFace RTSP Stream (Vivotek)", WINDOW_AUTOSIZE);
    frame_update(0, 0);

    cap_viewer.release();
    cap_analysis.release();
    return 0;
}
