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
	// ������Ƶд����
	cv::Size frameSize(width, height);
	if (!_InitVideoWriter(filename, codec, fps, frameSize, isColor)) {
		return false;
	}

	// �������������ݣ�д����Ƶ֡
	void* outAddress = NULL;   // �����buffer��ȡ-��ѹ��ͼ������

	// std::cout << "֡����: " << totalFrames << std::endl;
    for (int i = 0; i < totalFrames; i++) {
		// �ӻ�������ȡһ֡ͼ������
		_pBuffers->GetAddress(i, &outAddress);
		cv::Mat image(height, width, CV_8UC1, outAddress);
		_WriteFrame(image);
	}


	// �ͷ���Ƶд����
	if (!_ReleaseVideoWriter()) {
		return false;
	}


	

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