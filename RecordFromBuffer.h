#pragma once
#pragma once
#ifndef _RECORDFROMBUFFER_H_
#define _RECORDFROMBUFFER_H_

#include "SapClassBasic.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "config.h"
#include "FrameBuffer.h"

/* codec list */




class RecordFromBuffer
{
public:
	RecordFromBuffer(SapBufferWithTrash& buffers, const FrameLayout& layout);
	virtual ~RecordFromBuffer();


	bool SaveVideo(const std::string& filename, int codec, double fps, int width, int height, bool isColor, int totalFrames);
	bool SaveVideo(const std::string& filename, int codec, double fps, int width, int height, bool isColor, const std::vector<int>& idxArr);

	bool SaveFrames(const std::string& fileFolder, const std::vector<int>& idxArr);

private:
	cv::VideoWriter _videoWriter;
	SapBufferWithTrash& _buffers;
	FrameLayout _frameLayout;

	bool _InitVideoWriter(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor);
	bool _WriteFrame(const cv::Mat& frame);
	bool _ReleaseVideoWriter();
};


#endif // _VIDEORECORDER_H_
