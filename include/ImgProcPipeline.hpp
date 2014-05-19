#ifndef IMGPROCPIPELINE
#define IMGPROCPIPELINE

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include <iostream>

using namespace cv;

class GaussianMixtureModel{
    int dimensions;
    int components;
    std::vector<double> weight; 
    std::vector<Mat> covarianceMatrix;
    std::vector<Mat> meanVector;
    Mat componentProbability;
    void runExpectationMaximization(const Mat samples, int maxIterations, double minStepIncrease);
    double gauss(const Mat x, Mat covarianceMatrix, Mat meanVector);
    double gaussIdx(const Mat x, int componentIndex);
    public:
	Mat lookup;
	bool initialized;
	GaussianMixtureModel();
	GaussianMixtureModel(int dims, int K);
	GaussianMixtureModel(const GaussianMixtureModel& other);
	GaussianMixtureModel& operator=(const GaussianMixtureModel& other);
	double get(const Mat x);
	void makeLookup(int histSize[2], float c1range[2], float c2range[2]);
	void fromHistogram(const Mat histogram, int histSize[2], float c1range[2], float c2range[2], int maxIter, double minStepIncrease); 
  
};


class Histogram{
protected:
    Mat accumulator;
    int histSize[2];
    int channels[2];
    float c1range[2];
    float c2range[2];
    GaussianMixtureModel gmm;
    bool gmmReady;
public:
    Mat normalized;
    Histogram();
    Histogram(int channels[2], int histogramSize[2], float channel1range[2], float channel2range[2]);
    Histogram(const Histogram& other);
    Histogram& operator=(const Histogram& other);
    virtual void fromImage(Mat image, const Mat mask);
    virtual void update(Mat image, double alpha, const Mat mask);
    void backPropagate(Mat inputImage, Mat* outputImage);
    void makeGMM(int K, int maxIter, double minStepIncrease);
    void resize(int histogramSize[2]);
};


/**
 * An abstract base class for all image processing pipeline components.
 */
class ProcessingElement{
    public:
	bool initialized;
	virtual void process(const Mat inputImage, Mat* outputImage) = 0;
};

/**
 * 2D histogram-based image flattening class. Supports HSV, HLS and YUV colorspaces.
 */
class ColorHistBackProject : public ProcessingElement{
    protected:
	Mat histogramMask;
	int colorspaceCode;
	Histogram objHistogram;
	void preprocess(const Mat image, Mat* outputImage);
    public:
	//bool initialized;
	ColorHistBackProject();
	ColorHistBackProject(int code, const int* histogramSize);
	ColorHistBackProject(int code, const int* histogramSize, String filename);
	virtual void histFromImage(const Mat image);	
	void updateHistogram(const Mat image, const Mat mask);
    virtual void process(const Mat inputImage, Mat* outputImage);
};

class BayesColorHistBackProject : public ColorHistBackProject{
    public:
	void histFromImage(const Mat image);
	void process(const Mat inputImage, Mat* outputImage);
	BayesColorHistBackProject(int code, const int* histogramSize) : ColorHistBackProject(code, histogramSize) {};
    BayesColorHistBackProject(int code, const int* histogramSize, String filename) : ColorHistBackProject(code, histogramSize,filename) {};
};

class GMMColorHistBackProject : public ColorHistBackProject{
    public:
	void histFromImage(const Mat image);
	void process(const Mat inputImage, Mat* outputImage);
	GMMColorHistBackProject();
	GMMColorHistBackProject(int code, const int* histogramSize);
	GMMColorHistBackProject(int code, const int* histogramSize, String filename);
  
};

class SimpleThresholder : public ProcessingElement{
    float thresholdValue;
    public:
	SimpleThresholder();
	SimpleThresholder(float threshValue);
	void process(const Mat inputImage, Mat* outputImage);
};

class SimpleBlobDetect : public ProcessingElement{
    public:
	SimpleBlobDetect();
	void process(const Mat inputImage, Mat* outputImage);
  
};

class OpticalFlow : public ProcessingElement{
    protected:
        bool init;
        Mat old;
    public:
        OpticalFlow();
        void process(const Mat inputImage, Mat* outputImage);
};

class BGSubtractor : public ProcessingElement{
    protected:
        bool init;
        BackgroundSubtractorMOG bgsub;
        Mat old;
    public:
        BGSubtractor();
        void process(const Mat inputImage, Mat* outputImage);
};

#endif


