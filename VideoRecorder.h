#pragma once
#ifndef _VIDEORECORDER_H_
#define _VIDEORECORDER_H_

#include "SapClassBasic.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "config.h"

/* codec list */




class VideoRecorder : public SapProcessing
{
public:
	VideoRecorder(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext);
	virtual ~VideoRecorder();

	//bool InitVideoRecorder(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor);
	
	void SetVideoParams(int w, int h, const std::string& fname, 
		int codec, double frameRate, bool isColor, int format, int pixDepth);

	bool AutoSetVideoParams(std::string fname, SapTransfer* pXfer);
	bool InitVideoRecorder();

	void StartRecording();
	bool StopRecording();
	bool IsRecording() const { return _isRecording; }

	std::string GetErrorMessage() const { return _errorMessage; }

protected:
	virtual BOOL Run();

private:
	cv::VideoWriter _videoRecorder;

	struct VideoParams
	{
		int width;
		int height;
		std::string fileName;
		int encoder;
		double fps;
		bool isColor;
		int SapFormat;  
		int pixelDepth;
	};
	VideoParams _videoParams;

	bool _WriteFrame(const cv::Mat& frame);
	bool _Release();

	bool _isRecording = false;
	std::string _errorMessage;

};


#endif // _VIDEORECORDER_H_