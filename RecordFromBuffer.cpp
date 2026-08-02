#include "RecordFromBuffer.h"

#include <filesystem>


RecordFromBuffer::RecordFromBuffer(SapBufferWithTrash& buffers, const FrameLayout& layout)
	: _buffers(buffers), _frameLayout(layout)
{
}

RecordFromBuffer::~RecordFromBuffer()
{
	_ReleaseVideoWriter();
}

bool RecordFromBuffer::SaveVideo(const std::string& filename, int codec, double fps, int width, int height, bool isColor, int totalFrames)
{
	// 创建视频写入器
	cv::Size frameSize(width, height);
	if (!_InitVideoWriter(filename, codec, fps, frameSize, isColor)) {
		return false;
	}

	// 遍历缓冲区数据，写入视频帧
	// std::cout << "帧总数: " << totalFrames << std::endl;
    for (int i = 0; i < totalFrames; i++) {
		MappedMono8Frame frame;
		std::string error;
		if (!frame.Map(_buffers, i, _frameLayout, error) || !_WriteFrame(frame.Image())) {
			_ReleaseVideoWriter();
			return false;
		}
	}


	// 释放视频写入器
	if (!_ReleaseVideoWriter()) {
		return false;
	}
	return true;
}

bool RecordFromBuffer::SaveVideo(const std::string& filename, int codec, double fps, int width, int height, bool isColor, const std::vector<int>& idxArr)
{
	// 创建视频写入器
	cv::Size frameSize(width, height);
	if (!_InitVideoWriter(filename, codec, fps, frameSize, isColor)) {
		return false;
	}

	// 遍历缓冲区数据，写入视频帧
	// std::cout << "帧总数: " << totalFrames << std::endl;
	for (size_t i = 0; i < idxArr.size(); i++) {  // 使用 idxArr.size() 来遍历 vector
		MappedMono8Frame frame;
		std::string error;
		if (!frame.Map(_buffers, idxArr[i], _frameLayout, error) || !_WriteFrame(frame.Image())) {
			_ReleaseVideoWriter();
			return false;
		}
	}

	// 释放视频写入器
	if (!_ReleaseVideoWriter()) {
		return false;
	}

	return true;
}

// 保存帧序列图像
bool RecordFromBuffer::SaveFrames(const std::string& fileFolder, const std::vector<int>& idxArr) {

	int imageConter = 1;
	for (size_t i = 0; i < idxArr.size(); i++) {  // 使用 idxArr.size() 来遍历 vector
		std::stringstream name;
		name << std::setw(4) << std::setfill('0') << imageConter << ".bmp";
		const std::string fileName = (std::filesystem::path(fileFolder) / name.str()).string();
		MappedMono8Frame frame;
		std::string error;
		if (!frame.Map(_buffers, idxArr[i], _frameLayout, error)) {
			return false;
		}
		try {
			if (!cv::imwrite(fileName, frame.Image())) {
				return false;
			}
		}
		catch (const cv::Exception&) {
			return false;
		}
		imageConter += 1;
	}

	return true;
}

// 不设置码率
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
