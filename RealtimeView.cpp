#include "RealtimeView.h"

#include <filesystem>
#include <sstream>
#include <utility>

namespace {

template <typename... Values>
void Log(ILogSink* sink, LogLevel level, Values&&... values) noexcept {
	try {
		std::ostringstream stream;
		(stream << ... << std::forward<Values>(values));
		WriteLog(sink, level, stream.str());
	}
	catch (...) {
	}
}

} // namespace


RealtimeView::RealtimeView(
	SapBuffer* pBuffers,
	const FrameLayout& layout,
	SapProCallback pCallback,
	void* pContext,
	ILogSink* logSink,
	IPreviewSink* previewSink)
	: SapProcessing(pBuffers, pCallback, pContext),
	  _frameLayout(layout),
	  _logSink(logSink),
	  _previewSink(previewSink)
{
	// 构造函数
	_imageConter = 1;

}

RealtimeView::~RealtimeView()
{
	Shutdown();
}

bool RealtimeView::Shutdown() noexcept
{
	bool success = true;
	if (m_bInitOK && !Destroy()) {
		success = false;
	}
	try {
		_ReleaseVideoWriter();
	}
	catch (...) {
		success = false;
	}
	_isRecording = false;
	return success;
}

void RealtimeView::SubmitControl(ControlCommand command) noexcept
{
	_pendingControl.store(command);
}

BOOL RealtimeView::Run()
{
	// float frameRate = m_pBuffers->GetFrameRate();
	// std::cout  << frameRate << std::endl;

	int proIndex = this->GetIndex();  // 本类的获取索引方法与Execute/ExecuteNext相关联

	const ControlCommand control = _pendingControl.exchange(ControlCommand::None);
	if (control != ControlCommand::None) {
		switch (control) {
            case ControlCommand::Info:
				this->_BufferInfoDisplay();
				break;
			case ControlCommand::StartRecording:
				if (!_isRecording.load()) {
					// 构建视频文件名
					std::stringstream ss;
					std::time_t t = std::time(0);
					struct tm now;
					localtime_s(&now, &t);
					ss << CONFIG.getSavePath() << CONFIG.getVideoPrefix() << std::put_time(&now, "%Y%m%dT%H%M%S") << CONFIG.getVideoExt();
					std::string filePath = ss.str();

					if (!CONFIG.getSaveAsFrameSequence()) { // 录制视频

						cv::Size frameSize(_frameLayout.width, _frameLayout.height);
						_isRecording.store(_InitVideoWriter(filePath, GetEncoder(CONFIG.getEncoder()), CONFIG.getFrameRate(), frameSize, false));

						Log(_logSink, LogLevel::Success, "开始流式录制");
						return TRUE;
					}
					else { // 录制序列帧
						std::filesystem::path frameFolder(filePath);
						frameFolder.replace_extension();
						std::error_code folderError;
						std::filesystem::create_directories(frameFolder, folderError);
						if (folderError) {
							Log(_logSink, LogLevel::Error, "Failed to create directory: ", folderError.message());
							return FALSE;
						}
						_FrameSaveFolder = frameFolder.string() + "\\";

						_isRecording.store(true);
						_imageConter = 1;

						Log(_logSink, LogLevel::Success, "开始流式录制(保存为序列帧)");
						return TRUE;
					}
				}
                break;
			case ControlCommand::StopRecording:
				if (!CONFIG.getSaveAsFrameSequence()) { // 录制视频
                    _ReleaseVideoWriter();
				}
				_isRecording.store(false);
				Log(_logSink, LogLevel::Success, "停止流式录制，已保存至：", CONFIG.getSavePath());
				break;
			default:
				break;
		}
	}

	MappedMono8Frame mappedFrame;
	std::string mapError;
	auto ensureMapped = [&]() -> bool {
		if (mappedFrame.IsValid()) {
			return true;
		}
		if (!mappedFrame.Map(*m_pBuffers, proIndex, _frameLayout, mapError)) {
			Log(_logSink, LogLevel::Error, "Frame mapping failed: ", mapError);
			return false;
		}
		return true;
	};
	// std::cout << proIndex << std::endl;
	// GetFormat\GetPixelDepth\GetBytesPerPixel\GetPitch
	// CV_8UC1对应SapFormatMono8
	//
	/*
	if (_isRecording.load()) {
		cv::Mat image(_imageHeight, _imageWidth, CV_8UC1, outAddress);
		_WriteFrame(image);
	}
	*/
	// 流式录制
	if (_isRecording) {
		if (!CONFIG.getSaveAsFrameSequence()) {
			if (!ensureMapped() || !_WriteFrame(mappedFrame.Image())) {
				Log(_logSink, LogLevel::Error, "Streaming recording stopped because a frame could not be written.");
				_ReleaseVideoWriter();
				_isRecording.store(false);
				return FALSE;
			}
		}
		else {
			std::stringstream ss;
			ss << _FrameSaveFolder << std::setw(4) << std::setfill('0') << _imageConter << ".bmp";
			std::string filePath = ss.str();
			try {
				if (!ensureMapped() || !cv::imwrite(filePath, mappedFrame.Image())) {
					Log(_logSink, LogLevel::Error, "Failed to save frame: ", filePath);
					_isRecording.store(false);
					return FALSE;
				}
			}
			catch (const cv::Exception& exception) {
				Log(_logSink, LogLevel::Error, "Failed to save frame: ", filePath, " - ", exception.what());
				_isRecording.store(false);
				return FALSE;
			}
			_imageConter += 1;
		}
	}


	if (_isRecording.load() && CONFIG.getPauseView()) { // 录制时不显示画面
		return TRUE;
	}
	else { // 实时预览
		// 跳帧显示减少资源占用
		if (_skipFrameSwitch) {
			_skipFrameSwitch = false;
			return TRUE;
		} else {
			_skipFrameSwitch = true;
		}

		if (!ensureMapped()) {
			return FALSE;
		}
		cv::Mat image = mappedFrame.Image();

		if (CONFIG.getFocusPeakingLayer() || CONFIG.getHistLayer() || CONFIG.getMotionDetectorLayer()) {
			cv::Mat viewImage;
			cv::cvtColor(image, viewImage, cv::COLOR_GRAY2BGR); // 颜色格式转换，以便叠加显示
			if (CONFIG.getFocusPeakingLayer()) {
				cv::Mat focusPeakingLayer = _FocusPeakingLayer(image); // 峰值对焦图层
				cv::add(viewImage, focusPeakingLayer, viewImage);
			}

			if (CONFIG.getHistLayer()) {
				cv::Mat hisLayer = _HistLayer(image); // 直方图图层
				cv::add(viewImage, hisLayer, viewImage);
			}

			if (CONFIG.getMotionDetectorLayer()) {
				bool isMotionDetected;
				cv::Mat motionLayer = _MotionDetectorLayer(isMotionDetected, this->_lastFrame, image, false, 3000); // 运动检测图层
				cv::add(viewImage, motionLayer, viewImage);
				this->_lastFrame = image.clone(); // 跨回调保存必须拥有图像数据
				// std::cout << isMotionDetected << std::endl;
			}

			if (_previewSink != nullptr) {
				try {
					_previewSink->Publish(viewImage);
				}
				catch (...) {
					Log(_logSink, LogLevel::Error, "Preview sink rejected a BGR8 frame.");
				}
			}
			else {
				const double scale = CONFIG.getViewerScale();
				if (scale != 1) {
					cv::resize(viewImage, viewImage, cv::Size(
						static_cast<int>(image.cols * scale),
						static_cast<int>(image.rows * scale)));
				}
				cv::imshow("Captured Frame", viewImage);
				cv::waitKey(CONFIG.getCvWaitKey());
			}
			return TRUE;
		} else {
			if (_previewSink != nullptr) {
				try {
					_previewSink->Publish(image);
				}
				catch (...) {
					Log(_logSink, LogLevel::Error, "Preview sink rejected a Mono8 frame.");
				}
			}
			else {
				const double scale = CONFIG.getViewerScale();
				if (scale != 1) {
					cv::resize(image, image, cv::Size(
						static_cast<int>(image.cols * scale),
						static_cast<int>(image.rows * scale)));
				}
				cv::imshow("Captured Frame", image);
				cv::waitKey(CONFIG.getCvWaitKey());
			}
			return TRUE;
		}
	}

}


bool RealtimeView::_InitVideoWriter(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor)
{
	_videoWriter.open(filename, codec, fps, frameSize, isColor);
	if (!_videoWriter.isOpened()) {
		Log(_logSink, LogLevel::Error, "Error: Could not open the video writer.");
		return false;
	}
	return true;
}


bool RealtimeView::_WriteFrame(const cv::Mat& frame)
{
	if (!_videoWriter.isOpened()) {
		return false;
	}

	_videoWriter.write(frame);
	return true;
}

void RealtimeView::_ReleaseVideoWriter() {
	if (_videoWriter.isOpened()) {
		_videoWriter.release();
	}
}


/*
bool RealtimeView::_InitVideoWriter(const std::string& filename, int codec,
	double fps, const cv::Size& frameSize, bool isColor) {
	return _videoWriter.open(filename, codec, fps, frameSize, isColor);
}

void RealtimeView::_WriteFrame(const cv::Mat& frame) {
	if (!_videoWriter.isOpened()) {
		throw std::runtime_error("Error: Video writer is not initialized.");
	}
	_videoWriter.write(frame);
}

void RealtimeView::_ReleaseVideoWriter() {
	_videoWriter.release();
}
*/

void RealtimeView::_BufferInfoDisplay() {

	int width = m_pBuffers->GetWidth();
	Log(_logSink, LogLevel::Info, "宽度：", width);

	int height = m_pBuffers->GetHeight();
	Log(_logSink, LogLevel::Info, "高度：", height);

	bool ismulti = m_pBuffers->IsMultiFormat();
	Log(_logSink, LogLevel::Info, "是否是多格式：", ismulti);

	int count = m_pBuffers->GetCount();
	Log(_logSink, LogLevel::Info, "缓冲区数量：", count);

	const auto format = m_pBuffers->GetFormat();
	bool a;
	if (format == SapFormatMono8) {
		a = 1;
	}
	else {
		a = 0;
	}
	Log(_logSink, LogLevel::Info, "格式（Mono8为1）：", a);
	int minDepth = GetPixelDepthMin(format);
	int maxDepth = GetPixelDepthMax(format);
	Log(_logSink, LogLevel::Info, "最小位深：", minDepth);
	Log(_logSink, LogLevel::Info, "最大位深：", maxDepth);

	int pixelDepth = m_pBuffers->GetPixelDepth();
	Log(_logSink, LogLevel::Info, "位深：", pixelDepth);
}

/* 峰值对焦图层 */
cv::Mat RealtimeView::_FocusPeakingLayer(const cv::Mat& frame)
{
	// 图像平滑
	cv::Mat imgBlur;
	cv::GaussianBlur(frame, imgBlur, cv::Size(5, 5), 1.5); //相关参数在类中定义

	// 边缘检测
	cv::Mat edges;
	cv::Canny(imgBlur, edges, 50, 150);

	cv::Mat redEdges = cv::Mat::zeros(frame.size(), CV_8UC3);
	redEdges.setTo(cv::Scalar(0, 0, 255), edges);

	return redEdges;
}


/* 直方图显示*/
cv::Mat RealtimeView::_HistLayer(const cv::Mat& frame)
{
	// 参数定义
	int grayImgNum = 1; //图像数
	int grayChannels = 0; //需要计算的通道号 单通道只有0
	const int grayHistDim = 1; //直方图维数
	const int grayHistSize = 256; //直方图每一维度bin个数
	float grayRanges[2] = { 0, 255 };  //灰度值的统计范围
	const float* grayHistRanges[1] = { grayRanges }; //灰度值统计范围指针

	cv::Mat grayHist;

	//计算灰度图像的直方图
	cv::calcHist(&frame,
		grayImgNum,
		&grayChannels,
		cv::Mat(),
		grayHist,
		grayHistDim,
		&grayHistSize,
		grayHistRanges,
		true,  //是否均匀
		false); //是否累积

	int frameWidth = frame.cols;
	int frameHeight = frame.rows;


	int grayScale = 2;  //宽大小
	int histHeight = static_cast<int>(frameHeight/4); //高度
	int histWidth = histHeight * grayScale; //宽度
	int binHeight = static_cast<int>(histHeight*7/8);  //bin的最大高度

	// 直方图的图片，初始全像素值为0
	//cv::Mat grayHistImg(histHeight, histWidth, CV_8UC1, cv::Scalar(0));
	cv::Mat grayHistImg = cv::Mat::zeros(histHeight, histWidth, CV_8UC1);

	double grayMaxValue = 0;
	double grayMinValue = 0;
	cv::minMaxLoc(grayHist, &grayMinValue, &grayMaxValue, NULL, NULL);

	// 直方图绘制
	for (size_t i = 0; i < grayHistSize; i++)
	{
		float bin_val = grayHist.at<float>(static_cast<int>(i));
		//cvRound返回跟参数最接近的整数值，即四舍五入
		int intensity = cvRound(bin_val * binHeight / grayMaxValue);

		// 绘制直线 这里用每scale条竖向直线代表一个bin
		for (size_t j = 0; j < grayScale; j++)
		{
			cv::line(grayHistImg,
				cv::Point(static_cast<int>(i * grayScale + j), histHeight - intensity),
				cv::Point(static_cast<int>(i * grayScale + j), histHeight - 1),
				255);
		}

	}

	/*
	cv::Mat colorHistImg;
	cv::cvtColor(grayHistImg, colorHistImg, cv::COLOR_GRAY2BGR);
	colorHistImg.setTo(cv::Scalar(255, 255, 255), grayHistImg);
	cv::Mat backgroud(colorHistImg.cols, colorHistImg.rows, CV_8UC3, cv::Scalar(0, 0, 255));
	cv::addWeighted(backgroud, 1.0, colorHistImg, 1.0, 0.0, colorHistImg);
	cv::Mat background(histHeight, histWidth, CV_8UC3, cv::Scalar(1, 1, 1));
	cv::cvtColor(grayHistImg, colorHistImg, cv::COLOR_GRAY2BGR);
	cv::addWeighted(background, 1.0, colorHistImg, 1.0, 0.0, colorHistImg);
	*/

	// 灰度直方图转换为彩色并叠加背景
	cv::Mat colorHistImg = cv::Mat::zeros(grayHistImg.size(), CV_8UC3);
	colorHistImg.setTo(cv::Scalar(255, 0, 0), grayHistImg);  // 线条

	cv::Mat background = cv::Mat::zeros(frame.size(), CV_8UC3);

	//roi 实际上是 background 的一个视图（或引用），它并不创建一个新的图像，而是直接引用了 background 的某个区域
	cv::Mat roi = background(cv::Rect(frameWidth- histWidth, 0, histWidth, histHeight));

	//当对 roi 进行修改时，实际上是在修改 background 的相应部分
	cv::add(colorHistImg, roi, roi);

	return background;
}

/* 运动检测 */
cv::Mat RealtimeView::_MotionDetectorLayer(bool& motionDetected,const cv::Mat& lastFrame, const cv::Mat& currentFrame, bool isDelta, const int minSizeMovement)
{
	// 如果第一帧为空，直接退出
	if (lastFrame.empty()) {
		cv::Mat output;
		cvtColor(currentFrame, output, COLOR_GRAY2BGR);
		return output;
	}

	//cv::Mat outputFrame = currentFrame.clone();
	//GaussianBlur(lastFrame, lastFrame, Size(21, 21), 0); // 高斯模糊太影响处理速度
	//GaussianBlur(currentFrame, currentFrame, Size(21, 21), 0);

	// 比较两个帧，找到差异
	Mat frameDelta;
	absdiff(lastFrame, currentFrame, frameDelta);
	Mat thresh;
	threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);

	// 通过膨胀填充孔洞，并找到阈值的轮廓
	dilate(thresh, thresh, Mat(), Point(-1, -1), 2);
	std::vector<std::vector<Point>> contours;
	findContours(thresh.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	// 遍历轮廓
	cv::Mat boxLayer = cv::Mat::zeros(currentFrame.size(), CV_8UC3);
	motionDetected = false;
	for (size_t i = 0; i < contours.size(); i++) {
		// 保存所有找到的轮廓坐标
		Rect bounding_box = boundingRect(contours[i]);

		// 如果轮廓太小，则忽略它，否则，存在瞬时运动
		if (contourArea(contours[i]) > minSizeMovement) {
			// 绘制矩形框，以便显示足够大的运动
			rectangle(boxLayer, bounding_box.tl(), bounding_box.br(), Scalar(0, 255, 0), 3);
			motionDetected = true; // 检测到运动
		}
	}

	if (isDelta) {
		cv::Mat deltaLayer;
		cvtColor(frameDelta, deltaLayer, COLOR_GRAY2BGR);
		return frameDelta;
	}else {
		return boxLayer;
	}
}
