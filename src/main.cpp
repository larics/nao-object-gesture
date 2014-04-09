/*
 * objectTracking.cpp
 * 
 * Copyright 2014 Kruno Hrvatinic <kruno.hrvatinic@fer.hr>
 * 
 */

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
#include "ImgProcPipeline.hpp"
#include "DisplayWindow.hpp"
#include "ObjectTracking.hpp"

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cv;

int main(void)
{
    VideoCapture capture;
    Mat frame;
    capture.open( -1 );
    if (!capture.isOpened()) 
    {
	printf("--(!)Error opening video capture\n");
	return -1; 
    }
    
    int hSize[] = {32,32};
    int colorCode = CV_BGR2HLS;
    vector<ProcessingElement*> pipeline;
    
    vector<double> num = {0.87};
    vector<double> den = {1, -0.13};
    LTIFilter ltifilt(num, den, 1/30.0);
    ProcessingElement *generalPtr = static_cast<ProcessingElement*>(&ltifilt);
    pipeline.push_back(generalPtr);
    
    ColorHistBackProject seg(colorCode, hSize);
    generalPtr = static_cast<ProcessingElement*>(&seg);
    pipeline.push_back(generalPtr);
    
    BayesColorHistBackProject bayesSeg(colorCode, hSize);
    generalPtr = static_cast<ProcessingElement*>(&bayesSeg);
    pipeline.push_back(generalPtr);
    
    int hSize2[] = {128, 128};
    GMMColorHistBackProject GMMSeg(colorCode, hSize2);
    generalPtr = static_cast<ProcessingElement*>(&GMMSeg);
    pipeline.push_back(generalPtr);
    
    //4
    SimpleThresholder thresh(0.2);
    generalPtr = static_cast<ProcessingElement*>(&thresh);
    pipeline.push_back(generalPtr);
    
    SimpleBlobDetect sbd;
    generalPtr = static_cast<ProcessingElement*>(&sbd);
    pipeline.push_back(generalPtr);
    
    ObjectTracker objtrack;
    generalPtr = static_cast<ProcessingElement*>(&objtrack);
    pipeline.push_back(generalPtr);
    
    vector<vector<int>> pipelineIdVector;
    vector<int> temp = {6};
    pipelineIdVector.push_back(temp);
    temp = {0,2,4,5};
    pipelineIdVector.push_back(temp);
    temp = {0,3,4,5};
    pipelineIdVector.push_back(temp);
    
    String windowname="Color Histogram Backpropagation";
    
    DisplayWindow window(windowname, pipeline,pipelineIdVector);
    
    while (capture.read(frame))
    {
        if(frame.empty())
        {
            printf(" --(!) No captured frame -- Break!");
            break;
        }
        
        window.display(frame);
        
        int c = waitKey(10);
	if((char)c == 27) {break;} // escape
    }
	
	return 0;
}