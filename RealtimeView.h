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
	0: ������
	1: ��ʾbufferͼ���е���Ϣ
	2����ʼ¼��
	3������¼��
	*/

protected:
	virtual BOOL Run(); //��XferCallback�е���Execute()��ִ�д˺�����ִ������Զ�����ProCallback

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
    cv::Mat _FocusPeakingLayer(const cv::Mat& frame);  // ��ֵ�Խ�ͼ��
    cv::Mat _HistLayer(const cv::Mat& frame);  // ֱ��ͼͼ��
	cv::Mat _MotionDetectorLayer(bool& motionDetected,
		const cv::Mat& lastFrame, 
		const cv::Mat& currentFrame, 
		bool isDelta, 
		const int minSizeMovement);  // �˶����ͼ��(��һ֡����ǰ֡���Ƿ���ʾ��ֻ��桢��С�������)
	cv::Mat _lastFrame;  // ��һ֡

	int _imageConter = 1; // ͼƬ������
	// int _bufferIdxConter = 0; // buffer������
	int _imageWidth;
	int _imageHeight;
	
};



#endif // _REALTIMEPROCESS_H_