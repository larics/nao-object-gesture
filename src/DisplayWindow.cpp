#include "opencv2/highgui/highgui.hpp"
#include "DisplayWindow.hpp"
#include "ImgProcPipeline.hpp"
#include <chrono>

DisplayWindow::DisplayWindow(String name){
  windowName=name;
  dragStartL = Point(-1,-1);
  dragStartR = Point(-1,-1);
  currentPos = Point(-1,-1);
  leftDrag=false;
  rightDrag=false;
  dragging = false;
  clickTime = std::chrono::system_clock::now();
  namedWindow(name);
  setMouseCallback(name, staticMouseCallback, this);
}

DisplayWindow::DisplayWindow(String name, std::vector<ProcessingElement*> prcElm){
  windowName=name;
  dragStartL = Point(-1,-1);
  dragStartR = Point(-1,-1);
  currentPos = Point(-1,-1);
  leftDrag=false;
  rightDrag=false;
  dragging = false;
  processingElements = prcElm;
  clickTime = std::chrono::system_clock::now();
  namedWindow(name);
  setMouseCallback(name, staticMouseCallback, this);
}

void DisplayWindow::display(const Mat image){
   Mat endImage;
   image.copyTo(lastDispImg);
   image.copyTo(endImage);
   if (view.size()>0){
      for (int i=0; i<view.size(); i++){
	  if (processingElements[i]->initialized){
	      processingElements[i]->process(endImage, &endImage);
	  }
	  else{image.copyTo(endImage); break;}
      }
   }
   if (dragging && view.size()==0){
      rectangle(endImage, dragStartL, currentPos, Scalar(0,0,255));
   }
   imshow(windowName,endImage);
}

void DisplayWindow::staticMouseCallback(int event, int x, int y, int flags, void* param){
  DisplayWindow *self = static_cast<DisplayWindow*>(param);
  self->mouseCallback(event, x, y, flags, param);
}

void DisplayWindow::mouseCallback(int event, int x, int y, int flags, void* param){
  switch (event){
    case EVENT_LBUTTONDOWN:
      dragStartL = Point(x,y);
      leftDrag = true;
      clickTime = std::chrono::system_clock::now();
      break;
    case EVENT_RBUTTONDOWN:
      dragStartR = Point(x,y);
      rightDrag = true;
      clickTime = std::chrono::system_clock::now();
      break;
    case EVENT_LBUTTONUP:
      if (dragging) {onDragStop();} else {onLeftClick();}
      leftDrag = false;
      dragging = false;
      break;
    case EVENT_RBUTTONUP:
      if (dragging) {onDragStop();} else {onRightClick();}
      rightDrag = false;
      dragging = false;
      break;
    case EVENT_MOUSEMOVE:
      currentPos = Point(x,y);
      auto diff = std::chrono::system_clock::now()-clickTime;
      std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
      std::chrono::milliseconds oneClick(250);
      if ((leftDrag || rightDrag) && ms > oneClick) {dragging = true;}
      break;
  }
}

void DisplayWindow::onLeftClick(){
}

void DisplayWindow::onRightClick(){
    if (view.size()==0){
	view.push_back(0);;
    }
    else {
      view.clear();;
    }
}

void DisplayWindow:: onDragStop(){
  if (processingElements.size()>0 && leftDrag){
      try{
      int x=min(dragStartL.x,currentPos.x);
      int y=min(dragStartL.y,currentPos.y);
      int width=abs(dragStartL.x-currentPos.x);
      int height=abs(dragStartL.y-currentPos.y);
      Rect imageROI = Rect(x,y,width,height);
      Mat subimage(lastDispImg, imageROI);
      ColorSegmenter *temp = static_cast<ColorSegmenter*>(processingElements[0]);
      temp->histFromImage(subimage);
      }
      catch(Exception e)
      {
	std::cout << e.msg << std::endl;
      }
  }
}