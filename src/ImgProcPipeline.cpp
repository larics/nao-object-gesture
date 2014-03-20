#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "ImgProcPipeline.hpp"
#include <iostream>
#include <cmath>

using namespace cv;

GaussianMixtureModel::GaussianMixtureModel(int dims, int K){
    dimensions=dims;
    components=K;
    for (int i=0; i<K; i++){
	covarianceMatrix.push_back(Mat::eye(dims, dims, CV_64F)*500);
	if (i==0){
	    meanVector.push_back(Mat::ones(dims, 1, CV_64F)*100);
	}
	else {
	  meanVector.push_back(Mat::ones(dims, 1, CV_64F)*50);
	}
	weight.push_back(1.0/K);
    }
    initialized = false;
}

/* matrix samples is a N X (M+1) matrix, consisting of N M-dimensional samples. The last row-element is the number of identical samples*/
void GaussianMixtureModel::runExpectationMaximization(const Mat samples, int maxIterations, double minStepIncrease){
    if (!initialized){
	//componentProbability = Mat::ones(samples.rows, components, CV_64F)/(1.0*components);
	initialized = true;
	
	int k = 0;
	int numEach = samples.rows / components;
	double num = 0;
	Mat tempVec = Mat::zeros(dimensions, 1, CV_64F);
	meanVector.clear();
	for (int i=0; i<samples.rows; i++){
	    double element = samples.at<double>(i,dimensions);
	    Mat elementx = samples(Range(i,i+1),Range(0,dimensions));
	    elementx = elementx.t();
	    tempVec += element * elementx;
	    num += element;
	    if (i!=0 && i%numEach==0){
		meanVector.push_back(tempVec/num);
		num=0;
		tempVec = Mat::zeros(dimensions, 1, CV_64F);
		k++;
	    }
	}
	meanVector.push_back(tempVec/num);
    }
    
    double increase = 0;
    double lastLogLikelihood = 0;
    bool start = true;
    double nDataPoints = 0;
    for (int i=0; i<samples.rows; i++){
	nDataPoints+=samples.at<double>(i,dimensions);
    }
    
    std::vector<double> newWeight; 
    std::vector<Mat> newCovarianceMatrix;
    std::vector<Mat> newMeanVector;
    Mat newComponentProbability;
    
    for (int step = 0; step<maxIterations; step++){
     
	newCovarianceMatrix.clear(); 
	newMeanVector.clear();
	newWeight.clear();
      
	for (int i = 0; i<components; i++){
	    newCovarianceMatrix.push_back(Mat::zeros(dimensions, dimensions, CV_64F));
	    newMeanVector.push_back(Mat::zeros(dimensions, 1, CV_64F));
	    newWeight.push_back(0.0);
	}
	newComponentProbability = Mat::zeros(samples.rows, components+1, CV_64F);
	
	for (int i = 0; i<samples.rows; i++){
	    Mat elementx = samples(Range(i,i+1),Range(0,dimensions));
	    elementx = elementx.t();
	    for (int k = 0; k<components; k++){
		double prob = weight[k]*gaussIdx(elementx, k);
		newComponentProbability.at<double>(i,k) = prob;
	    }
	}
	
	//std::cout << "Compprob: "<< newComponentProbability << std::endl;
	
	
	for (int i = 0; i<samples.rows; i++){
	    double sum = 0;
	    const double* tempRow = newComponentProbability.ptr<double>(i);
	    for (int k = 0; k<components; k++){
		sum += tempRow[k];
	    }
	    if (sum>1e-300){
		Mat rowMod = newComponentProbability.row(i);
		rowMod /= sum;
	    }
	    else {
		for (int k = 0; k<components; k++){
		    newComponentProbability.at<double>(i,k)=1.0/components;
		}
	    }
	}
	
	newComponentProbability.copyTo(componentProbability);
	
	//std::cout << "Compprob: "<< newComponentProbability << std::endl;
	
      
	for (int i = 0; i<samples.rows; i++){
	    double element = samples.at<double>(i,dimensions);
	    Mat elementx = samples(Range(i,i+1),Range(0,dimensions));
	    elementx = elementx.t();
	    for (int k = 0; k<components; k++){
		double prob = componentProbability.at<double>(i,k);
		newWeight[k] += element * prob;
		newMeanVector[k] += element * elementx * prob;
	    }
	}
	
	for (int k = 0; k<components; k++){
	    newMeanVector[k] /= newWeight[k];
	    newWeight[k] /= 1.0 * nDataPoints;
	}
	for (int i = 0; i<samples.rows; i++){
	    double element = samples.at<double>(i,dimensions);
	    Mat elementx = samples(Range(i,i+1),Range(0,dimensions));
	    elementx = elementx.t();
	    for (int k = 0; k<components; k++){
		double prob = componentProbability.at<double>(i,k);
		Mat diff = (elementx - newMeanVector[k]);
		newCovarianceMatrix[k] += element * prob * diff*diff.t();
	    }
	}
	
	for (int k = 0; k<components; k++){
	    newCovarianceMatrix[k] /= 1.0*nDataPoints * newWeight[k];
	}
	
	for (int k = 0; k<components; k++){
	    weight[k] = newWeight[k];
	    newMeanVector[k].copyTo(meanVector[k]);
	    newCovarianceMatrix[k].copyTo(covarianceMatrix[k]);
	    std::cout << "Component " << k << std::endl;
	    std::cout << "Weight: "<< newWeight[k] << std::endl;
	    std::cout << "Mean: "<< newMeanVector[k] << std::endl;
	    std::cout << "Covariance: "<< newCovarianceMatrix[k] << std::endl;  
	}
	
	double logLikelihood = 0.0;
	for (int i=0; i<samples.rows; i++){
	    int element = samples.at<int>(i,dimensions);
	    Mat elementx = samples.rowRange(0, dimensions);
	    elementx = elementx.t();
	    double prob = get(elementx);
	    logLikelihood += element * prob;
	}
	if (!start){
	    increase = logLikelihood/lastLogLikelihood;
	    if (increase < (1+minStepIncrease)) {break;}
	}
	start = false;
	lastLogLikelihood = logLikelihood;
    }
}

double GaussianMixtureModel::gauss(const Mat x, Mat covMatrix, Mat meanVec){
    if (x.size()!=Size(1,dimensions) || covMatrix.size()!=Size(dimensions,dimensions) || meanVec.size()!=Size(1,dimensions)){
	return -1.0;
    }
    try{
      Mat temp(x.size(),CV_64F);
      temp = (x-meanVec);
      temp = temp.t();
      temp = temp*covMatrix.inv();
      temp = temp*(x-meanVec)/(-2.0);
      double val = temp.at<double>(0,0);
      double retVal = exp(val);
      double pi = 3.141592653589793238463;
      double det = determinant(covMatrix);
      retVal = retVal/(pow(sqrt(2*pi),dimensions)*sqrt(det));
      return retVal;
    } catch(Exception e){std::cout << e.msg << std::endl; return -1;}
}

double GaussianMixtureModel::gaussIdx(const Mat x, int componentIndex){
    if (componentIndex>=0 && componentIndex<components){
      double retVal = gauss(x, covarianceMatrix[componentIndex], meanVector[componentIndex]);
      return retVal;
    }
    else {return -1;}
}

double GaussianMixtureModel::get(Mat x){
    double retVal = 0;
    for (int k=0; k<components; k++){
	double prob = weight[k]*gaussIdx(x, k);
	if (prob<0) {return -1;}
        retVal += prob;
    }
    return retVal;
}

void GaussianMixtureModel::makeLookup(int histSize[2], float c1range[2], float c2range[2]){
    Mat newLookup(histSize[0], histSize[1], CV_64F);
    float dim1step = (c1range[1]-c1range[0])/(1.0*histSize[0]);
    float dim2step = (c2range[1]-c2range[0])/(1.0*histSize[1]);
    float dim1start = c1range[0]+dim1step/2.0;
    float dim2start = c2range[0]+dim2step/2.0;
    for (int i=0; i<histSize[0]; i++){
	for (int j=0; j<histSize[1]; j++){
	    double xarr[] = {dim1start+i*dim1step, dim2start+j*dim2step};
	    Mat x(2,1,CV_64F, xarr);
	    newLookup.at<double>(i,j)=get(x);
	}
    }
    newLookup=newLookup*dim1step*dim2step;
    newLookup.copyTo(lookup);
    double histMax = 0;
    double histMin = 0;
    minMaxLoc(lookup, &histMin, &histMax, NULL, NULL);
    lookup.convertTo(lookup,CV_64F,1/(histMax-histMin),-histMin/(histMax-histMin));
}

void GaussianMixtureModel::fromHistogram(const Mat histogram, int histSize[2], float c1range[2], float c2range[2]){
    float dim1step = (c1range[1]-c1range[0])/(1.0*histSize[0]);
    float dim2step = (c2range[1]-c2range[0])/(1.0*histSize[1]);
    float dim1start = c1range[0]+dim1step/2.0;
    float dim2start = c2range[0]+dim2step/2.0;
    Mat samples;
    for (int i=0; i<histSize[0]; i++){
	for (int j=0; j<histSize[1]; j++){
	    if (histogram.at<double>(i,j)>0){
		double xarr[] = {dim1start+i*dim1step, dim2start+j*dim2step, histogram.at<double>(i,j)};
		Mat x(1,3,CV_64F, xarr);
		samples.push_back(x);
	    }
	}
    }
    runExpectationMaximization(samples, 30, 0.01);
    makeLookup(histSize,c1range,c2range);
}



ColorHistBackProject::ColorHistBackProject(){
    histSize[0] = 64;
    histSize[1] = 64;
    colorspaceCode = CV_BGR2HSV;
    c1range[0]=0; c1range[1]=180; c2range[0]=0; c2range[1]=256;
    channels[0]=0; channels[1]=1;
    initialized=false;
}
  
ColorHistBackProject::ColorHistBackProject(int code, const int* histogramSize){
	histSize[0] = histogramSize[0];
	histSize[1] = histogramSize[1];
	colorspaceCode=code;
	c1range[0]=0; c1range[1]=256; c2range[0]=0; c2range[1]=256;
	channels[0]=0; channels[1]=1;
	switch (code){
	  case CV_BGR2HSV:
	      c1range[1]=180; break;
	  case CV_BGR2HLS:
	      c1range[1]=180; break;
	  case CV_BGR2YUV:
	      channels[0]=1; channels[1]=2;
	default:
	   break;
	}
	initialized=false;
}

ColorHistBackProject::ColorHistBackProject(int code, const int* histogramSize, String filename){
	histSize[0] = histogramSize[0];
	histSize[1] = histogramSize[1];
	colorspaceCode=code;
	c1range[0]=0; c1range[1]=256; c2range[0]=0; c2range[1]=256;
	channels[0]=0; channels[1]=1;
	switch (code){
	  case CV_BGR2HSV:
	      c1range[1]=180; break;
	  case CV_BGR2HLS:
	      c1range[1]=180; break;
	  case CV_BGR2YUV:
	      channels[0]=1; channels[1]=2;
	default:
	   break;
	}
	Mat img = imread(filename);
	histFromImage(img);
	initialized=true;
}

void ColorHistBackProject::preprocess(const Mat image, Mat* outputImage){
      //GaussianBlur(image, *outputImage, Size(15,15),0);
      //medianBlur(image, *outputImage, 7);
      cvtColor(image, *outputImage, colorspaceCode);
      medianBlur(*outputImage, *outputImage, 5);
}

void ColorHistBackProject::histFromImage(const Mat image){
	Mat cvtImage;
	preprocess(image, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	calcHist(&cvtImage, 1, channels, Mat(), histogram, 2, histSize, ranges, true, false);

	double histMax = 0;
	double histMin = 0;
	minMaxLoc(histogram, &histMin, &histMax, NULL, NULL);
	std::cout << "Range = (" << histMin << ", " << histMax <<  ")" << std::endl;
	
	histogram.convertTo(normalizedHistogram,CV_32F,1/(histMax-histMin),-histMin/(histMax-histMin));
	initialized=true;
};

void ColorHistBackProject::updateHistogram(const Mat image, const Mat mask){
	float alpha = 0.1; //learning coefficient
	
	Mat cvtImage;
	preprocess(image, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	calcHist(&cvtImage, 1, channels, mask, histogram, 2, histSize, ranges, true, false);

	
	double histMax = 0;
	double histMin = 0;
	minMaxLoc(histogram, &histMin, &histMax, NULL, NULL);
	
	Mat newNormalizedHistogram;
	histogram.convertTo(newNormalizedHistogram,CV_32F,1/(histMax-histMin),-histMin/(histMax-histMin));
	
	if (initialized){
	normalizedHistogram = normalizedHistogram * (1-alpha) 
							+ newNormalizedHistogram * alpha;
	} 
	else {
		normalizedHistogram = newNormalizedHistogram;
		}
}

void ColorHistBackProject::process(const Mat inputImage, Mat* outputImage){
	Mat cvtImage;
	preprocess(inputImage, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	calcBackProject(&cvtImage, 1, channels, normalizedHistogram, *outputImage, ranges, 255, true);
	
	outputImage->convertTo(*outputImage, CV_32FC1, 1.0/255.0);	
}



void BayesColorHistBackProject::process(const Mat inputImage, Mat* outputImage){
	Mat cvtImage;
	preprocess(inputImage, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	calcBackProject(&cvtImage, 1, channels, normalizedHistogram, *outputImage, ranges, 255, true);
	
	Mat imgHist;
	calcHist(&cvtImage, 1, channels, Mat(), imgHist, 2, histSize, ranges, true, false);
	double histMax = 0;
	double histMin = 0;
	minMaxLoc(imgHist, &histMin, &histMax, NULL, NULL);
	imgHist.convertTo(imgHist,CV_32F,1/(histMax-histMin),-histMin/(histMax-histMin));
	imgHist *= 1.0/norm(sum(imgHist));
	float temp = norm(sum(imgHist));
	Mat aprioriColor;
	calcBackProject(&cvtImage, 1, channels, imgHist, aprioriColor, ranges, 255, true);
	
	outputImage->convertTo(*outputImage, CV_32FC1, 1.0/255.0);
	aprioriColor.convertTo(aprioriColor, CV_32FC1, 1.0/255.0);
	
	minMaxLoc(aprioriColor, &histMin, &histMax, NULL, NULL);
	
	*outputImage = *outputImage*0.5/aprioriColor;
	minMaxLoc(*outputImage, &histMin, &histMax, NULL, NULL);

	//outputImage->convertTo(*outputImage,CV_32F,1/(histMax-histMin),-histMin/(histMax-histMin));
	
	//aprioriColor.copyTo(*outputImage);
	
	return;
}

void BayesColorHistBackProject::histFromImage(const Mat image){
	Mat cvtImage;
	preprocess(image, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	calcHist(&cvtImage, 1, channels, Mat(), histogram, 2, histSize, ranges, true, false);

	double histMax = 0;
	double histMin = 0;
	minMaxLoc(histogram, &histMin, &histMax, NULL, NULL);
	std::cout << "Range = (" << histMin << ", " << histMax <<  ")" << std::endl;
	
	histogram.convertTo(normalizedHistogram,CV_32F,1/(histMax-histMin),-histMin/(histMax-histMin));
	normalizedHistogram *= 1.0/norm(sum(normalizedHistogram));
	
	initialized=true;
}


GMMColorHistBackProject::GMMColorHistBackProject(int code, const int* histogramSize) : ColorHistBackProject(code, histogramSize){
    gmm = new GaussianMixtureModel(2,4);
}

GMMColorHistBackProject::GMMColorHistBackProject(int code, const int* histogramSize, String filename) : ColorHistBackProject(code, histogramSize,filename) {
    gmm = new GaussianMixtureModel(2,4);
}

void GMMColorHistBackProject::process(const Mat inputImage, Mat* outputImage){
	Mat cvtImage;
	preprocess(inputImage, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	
	cvtImage.convertTo(cvtImage, CV_32F);
	Mat temp;
	gmm->lookup.convertTo(temp, CV_32F);
	calcBackProject(&cvtImage, 1, channels, temp, *outputImage, ranges, 1.0, true);
	
	outputImage->convertTo(*outputImage, CV_32FC1);	
	//outputImage->convertTo(*outputImage, CV_64F);	
}

void GMMColorHistBackProject::histFromImage(const Mat image){
	Mat cvtImage;
	preprocess(image, &cvtImage);
	
	const float* ranges[] = {c1range, c2range};
	calcHist(&cvtImage, 1, channels, Mat(), histogram, 2, histSize, ranges, true, false);
	
	histogram.convertTo(histogram, CV_64F);
	gmm->fromHistogram(histogram, histSize, c1range, c2range);
	
	double histMax = 0;
	double histMin = 0;
	minMaxLoc(histogram, &histMin, &histMax, NULL, NULL);
	histogram.convertTo(normalizedHistogram,CV_32F,1/(histMax-histMin),-histMin/(histMax-histMin));
	
	Mat temp;
	resize(normalizedHistogram, temp, Size(300,300),0,0,INTER_NEAREST);
	imshow("Before", temp);
    
	
	resize(gmm->lookup, temp, Size(300,300),0,0,INTER_NEAREST);
	imshow("After", temp);
	
	initialized=true;
}



SimpleThresholder::SimpleThresholder(){
    thresholdValue = 0.5;
    initialized = true;
}

SimpleThresholder::SimpleThresholder(float threshValue){
    thresholdValue = threshValue;
    initialized = true;
}

void SimpleThresholder::process(const Mat inputImage, Mat* outputImage){
    threshold(inputImage, *outputImage, thresholdValue, 1.0, THRESH_BINARY);
    Mat element = getStructuringElement(MORPH_RECT, Size(5,5));
    erode(*outputImage, *outputImage, element);
    dilate(*outputImage, *outputImage, element);
    element = getStructuringElement(MORPH_ELLIPSE, Size(7,7));
    dilate(*outputImage, *outputImage, element);
    erode(*outputImage, *outputImage, element);
}