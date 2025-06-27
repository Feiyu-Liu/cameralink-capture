#include "VideoRecorder.h"


VideoRecorder::VideoRecorder(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext)
	:SapProcessing(pBuffers, pCallback, pContext)
{

}

VideoRecorder::~VideoRecorder()
{
	if (m_bInitOK)
		Destroy();
}

BOOL VideoRecorder::Run()
{
	if (_isRecording) {
		int proIndex = this->GetIndex();
		void *_outAddress = NULL;
		m_pBuffers->GetAddress(proIndex, &_outAddress);
		cv::Mat image(_videoParams.width, _videoParams.height, CV_8UC1, _outAddress);

		if (_WriteFrame(image))
		{
			return TRUE;
		} else {
			_isRecording = false;
			return FALSE;
		}
	}
}

/*
bool VideoRecorder::InitVideoRecorder(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor)
{
	if (_videoRecorder.isOpened()) {
		_errorMessage = "Video recorder is already open";
		return false;
	}
	try {
		_videoRecorder.open(filename, codec, fps, frameSize, isColor);
	} catch (const cv::Exception& e) {
		_errorMessage = e.what();
		return false;
	}
	catch (...) {
		_errorMessage = "Unknown error";
		return false;
	}
	return true;
}
*/
void VideoRecorder::SetVideoParams(int w, int h, const std::string& fname,
	int codec, double frameRate, bool isColor, int format, int pixDepth)
{
	_videoParams = { w, h, fname, codec, frameRate, isColor, format, pixDepth };
}

bool VideoRecorder::AutoSetVideoParams(std::string fname, SapTransfer* pXfer)
{
	// ��ȡͼ��ߴ�
	int w = m_pBuffers->GetWidth();
	int h = m_pBuffers->GetHeight();

	// ����֧�ֶ��ʽͼ��
	if (m_pBuffers->IsMultiFormat()) {
		_errorMessage = "Multi-format image is not supported";
		return false;
	}
	
	// ��ȡͼ���ʽ
	bool isColor = false;
	int format = m_pBuffers->GetFormat();
	if (format == SapFormatMono8) {
		isColor = false;
	}
	else {
		_errorMessage = "Unsupported image format";
		return false;
	}

	// ��ȡλ��
	int pixDepth = GetPixelDepthMax(format);
	
	// ��ȡ֡��
	SapXferFrameRateInfo* pFrameRateInfo = pXfer->GetFrameRateStatistics();
	float frameRate = 0;
	// ��֡����Ϣ���ò���֡���ȶ�ʱ����ȡ֡����Ϣ
	if (pFrameRateInfo->IsLiveFrameRateAvailable() && pFrameRateInfo->IsLiveFrameRateStalled()) {
		frameRate = pFrameRateInfo->GetLiveFrameRate();
	} else {
		_errorMessage = "Failed to get frame rate or frame rate is not stable";
		//return false;
	}
	
	//_videoParams = { w, h, fname, H264_ENCODER, frameRate, isColor, format, pixDepth };
	_videoParams = { w, h, fname, ENCODER, frameRate, isColor, format, pixDepth };

	return true;
}

bool VideoRecorder::InitVideoRecorder()
{
	if (_videoRecorder.isOpened()) {
		_errorMessage = "Video recorder is already open";
		return false;
	}
	try {
		_videoRecorder.open(_videoParams.fileName, 
			_videoParams.encoder, 
			_videoParams.fps, 
			cv::Size(_videoParams.width, _videoParams.height), 
			_videoParams.isColor);
	} catch (const cv::Exception& e) {
		_errorMessage = e.what();
		return false;
	} catch (...) {
		_errorMessage = "Unknown error";
		return false;
	}
	return true;
}

void VideoRecorder::StartRecording() {
	_isRecording = true;
	std::cout << "��ʼ¼��\n" << std::endl;
}

bool VideoRecorder::StopRecording() {
	std::cout << "����¼��\n" << std::endl;
	return _Release();
}


bool VideoRecorder::_WriteFrame(const cv::Mat& frame)
{
	if (!_videoRecorder.isOpened()) {
		_errorMessage = "Video recorder is not start";
		return false;
	}
	try {
		_videoRecorder.write(frame);
	} catch (const cv::Exception& e) {
		_errorMessage = e.what();
		return false;
	} catch (...) {
		_errorMessage = "Unknown error";
		return false;
	}
	
	return true;
}

bool VideoRecorder::_Release() {
	if (_videoRecorder.isOpened()) 
	{
		try {
			_videoRecorder.release();
		} catch (const cv::Exception& e) {
			_errorMessage = e.what();
			return false;
		} catch (...) {
			_errorMessage = "Unknown error";
			return false;
		}
		return true;
	}
	else if(!_videoRecorder.isOpened()) 
	{
		_errorMessage = "Video recorder is not open";
		return false;
	}
}