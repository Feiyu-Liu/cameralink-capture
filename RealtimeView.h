#pragma once
#ifndef _REALTIMEPROCESS_H_
#define _REALTIMEPROCESS_H_

#include "SapClassBasic.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <vector>

#include "conio.h"
#include <iostream>

#include "config.h"

//#include "VideoRecorder.h"

using namespace cv;

class RealtimeView : public SapProcessing
{
public:
	RealtimeView(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext);
	virtual ~RealtimeView();

	bool IsRecording() const { return _isRecording; }

	int keyControler = 0;
	/*
	0: 不处理
	1: 显示buffer图像中的信息
	2：开始录制
	3：结束录制
	*/

protected:
	virtual BOOL Run(); //在XferCallback中调用Execute()后执行此函数，执行完后自动调用ProCallback

private:

	//VideoWriter _writer();
	//VideoRecorder _videoWriter2;

	cv::VideoWriter _videoWriter;
	//AsyncVideoWriter _videoWriter;
	bool _InitVideoWriter(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor);
	void _WriteFrame(const cv::Mat& frame);
	void _ReleaseVideoWriter();

	void _BufferInfoDisplay();

	bool _isRecording = false;
	// std::string _videoSavePath = "C:\\LiuFeiyu\\videos\\12-10test1.mp4";

	//Mat _FocusPeaking(const Mat& image, double threshold, const Scalar& color);
	

	//opencv tool box
    cv::Mat _FocusPeakingLayer(const cv::Mat& frame);  // 峰值对焦图层
    cv::Mat _HistLayer(const cv::Mat& frame);  // 直方图图层
	cv::Mat _MotionDetectorLayer(bool& motionDetected,
		const cv::Mat& lastFrame, 
		const cv::Mat& currentFrame, 
		bool isDelta, 
		const int minSizeMovement);  // 运动检测图层(上一帧、当前帧、是否显示差分画面、最小检测区域)
	cv::Mat _lastFrame;  // 上一帧

	int _imageConter = 1; // 图片计数器
	// int _bufferIdxConter = 0; // buffer计数器
	int _imageWidth;
	int _imageHeight;
	
};



#endif // _REALTIMEPROCESS_H_