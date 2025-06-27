#include "RecordFromBuffer.h"


RecordFromBuffer::RecordFromBuffer(SapBufferWithTrash* pBuffers)
{
	_pBuffers = pBuffers;
}

RecordFromBuffer::~RecordFromBuffer()
{

}

bool RecordFromBuffer::SaveVideo(const std::string& filename, int codec, double fps, int width, int height, bool isColor, int totalFrames)
{
	// 创建视频写入器
	cv::Size frameSize(width, height);
	if (!_InitVideoWriter(filename, codec, fps, frameSize, isColor)) {
		return false;
	}

	// 遍历缓冲区数据，写入视频帧
	void* outAddress = NULL;   // 从输出buffer获取-解压缩图像数据

	// std::cout << "帧总数: " << totalFrames << std::endl;
    for (int i = 0; i < totalFrames; i++) {
		// 从缓冲区获取一帧图像数据
		_pBuffers->GetAddress(i, &outAddress);
		cv::Mat image(height, width, GetCvFormat(CONFIG.getCvPixelFormat()), outAddress);
		_WriteFrame(image);
	}


	// 释放视频写入器
	if (!_ReleaseVideoWriter()) {
		return false;
	}
}

bool RecordFromBuffer::SaveVideo(const std::string& filename, int codec, double fps, int width, int height, bool isColor, const std::vector<int>& idxArr)
{
	// 创建视频写入器
	cv::Size frameSize(width, height);
	if (!_InitVideoWriter(filename, codec, fps, frameSize, isColor)) {
		return false;
	}

	// 遍历缓冲区数据，写入视频帧
	void* outAddress = NULL;   // 从输出buffer获取-解压缩图像数据

	// std::cout << "帧总数: " << totalFrames << std::endl;
	for (size_t i = 0; i < idxArr.size(); i++) {  // 使用 idxArr.size() 来遍历 vector
		// 从缓冲区获取一帧图像数据
		_pBuffers->GetAddress(idxArr[i], &outAddress);
		cv::Mat image(height, width, GetCvFormat(CONFIG.getCvPixelFormat()), outAddress);
		_WriteFrame(image);
	}

	// 释放视频写入器
	if (!_ReleaseVideoWriter()) {
		return false;
	}

	return true;
}

bool RecordFromBuffer::SaveFrames(const std::string& fileFolder, const std::vector<int>& idxArr) {

	int imageConter = 1;
	for (size_t i = 0; i < idxArr.size(); i++) {  // 使用 idxArr.size() 来遍历 vector
		std::stringstream ss;
		ss << fileFolder << std::setw(4) << std::setfill('0') << imageConter << ".bmp";
		std::string fileName = ss.str();
		_pBuffers->Save(fileName.c_str(), "-format bmp", idxArr[i], 0);
		imageConter += 1;
	}

	return true;
}

bool RecordFromBuffer::_InitVideoWriter(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor)
{
	_videoWriter.open(filename, codec, fps, frameSize, isColor);
	if (!_videoWriter.isOpened()) {
		std::cerr << "Error: Could not open the video writer." << std::endl;
		return false;
	}
	return true;
}


bool RecordFromBuffer::_WriteFrame(const cv::Mat& frame)
{
	if (!_videoWriter.isOpened()) {
		return false;
	}

	_videoWriter.write(frame);
	return true;
}

bool RecordFromBuffer::_ReleaseVideoWriter() {
	if (!_videoWriter.isOpened()) {
		return false;
	}
	_videoWriter.release();
	return true;
}