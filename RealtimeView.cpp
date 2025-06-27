#include "RealtimeView.h"


RealtimeView::RealtimeView(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext)
	:SapProcessing(pBuffers, pCallback, pContext)
{
	// 构造函数
	keyControler = 0;
	_imageConter = 1;
	
}

RealtimeView::~RealtimeView()
{
	if (m_bInitOK)
		Destroy();
}

BOOL RealtimeView::Run()
{
	// float frameRate = m_pBuffers->GetFrameRate();
	// std::cout  << frameRate << std::endl;
	
	int proIndex = this->GetIndex();  // 本类的获取索引方法与Execute/ExecuteNext相关联

	if (keyControler != 0) {
		switch (keyControler) {
            case 1:
				this->_BufferInfoDisplay();
				keyControler = 0;
				break;
			case 2:
				if (!_isRecording) {
					if (!SAVE_AS_FRAME_SEQUENCE) { // 录制视频
						std::stringstream ss;
						ss << SAVE_PATH << VIDEO_FILE_NAME;
						std::string filePath = ss.str();

						cv::Size frameSize(_imageWidth, _imageHeight);
						_isRecording = _InitVideoWriter(filePath, ENCODER, FRAME_RATE, frameSize, false);
							
						std::cout << "\n\n开始流式录制\n\n" << std::endl;
						return TRUE;
					}
					else { // 录制序列帧
						_isRecording = true;
						_imageConter = 1;

						std::cout << "\n\n开始流式录制(保存为序列帧)\n\n" << std::endl;
						return TRUE;
					}
				}
				keyControler = 0;
                break;
			case 3:
				if (!SAVE_AS_FRAME_SEQUENCE) { // 录制视频
                    _ReleaseVideoWriter();
				}
				_isRecording = false;
				keyControler = 0;
				std::cout << "\n\n停止流式录制\n" << "已保存至：" << SAVE_PATH << std::endl;
				break;
			default:
				break;
		}
	}

	// 处理图像
	void* outAddress = NULL;   // 从输出buffer获取-解压缩图像数据
	
	m_pBuffers->GetAddress(proIndex, &outAddress);
	// std::cout << proIndex << std::endl;
	// GetFormat\GetPixelDepth\GetBytesPerPixel\GetPitch
	// CV_8UC1对应SapFormatMono8
	// 
	/*
	if (_isRecording) { 
		cv::Mat image(_imageHeight, _imageWidth, CV_8UC1, outAddress);
		_WriteFrame(image);
	}
	*/
	if (_isRecording) {
		if (!SAVE_AS_FRAME_SEQUENCE) {
			cv::Mat image(_imageHeight, _imageWidth, CV_PIXEL_FORMAT, outAddress);
			_WriteFrame(image);
		}
		else {
			std::stringstream ss;
			ss << SAVE_PATH << std::setw(4) << std::setfill('0') << _imageConter << ".bmp";
			std::string filePath = ss.str();
			// std::cout << proIndex << std::endl;
			// std::cout << "Saving image to " << filePath << std::endl;
			m_pBuffers->Save(filePath.c_str(), "-format bmp", proIndex, 1);
			_imageConter += 1;
		}
	}


	if (_isRecording && PAUSE_VIEW) { // 录制时不显示画面
		return TRUE;
	}
	else { // 实时预览
		_imageWidth = m_pBuffers->GetWidth();
		_imageHeight = m_pBuffers->GetHeight();
		cv::Mat image(_imageHeight, _imageWidth, CV_PIXEL_FORMAT, outAddress);

		if (FOCUS_PEAKING_LAYER || HIST_LAYER || MOTION_DETECTOR_LAYER) {
			cv::Mat viewImage;
			cv::cvtColor(image, viewImage, cv::COLOR_GRAY2BGR); // 颜色格式转换，以便叠加显示
			if (FOCUS_PEAKING_LAYER) {
				cv::Mat focusPeakingLayer = _FocusPeakingLayer(image); // 峰值对焦图层
				cv::add(viewImage, focusPeakingLayer, viewImage);
			}

			if (HIST_LAYER) {
				cv::Mat hisLayer = _HistLayer(image); // 直方图图层
				cv::add(viewImage, hisLayer, viewImage);
			}

			if (MOTION_DETECTOR_LAYER) {
				bool isMotionDetected;
				cv::Mat motionLayer = _MotionDetectorLayer(isMotionDetected, this->_lastFrame, image, false, 3000); // 运动检测图层
				cv::add(viewImage, motionLayer, viewImage);
				this->_lastFrame = image; // 保存上一帧图像
				// std::cout << isMotionDetected << std::endl;
			}

			if (VIEWER_SCALE != 1) {
				cv::resize(viewImage, viewImage, cv::Size(image.cols * VIEWER_SCALE, image.rows * VIEWER_SCALE)); // scale 
			}
			cv::imshow("Captured Frame", viewImage);
			cv::waitKey(CV_WAITKEY);
			return TRUE;
		} else {
			if (VIEWER_SCALE != 1) {
				cv::resize(image, image, cv::Size(image.cols * VIEWER_SCALE, image.rows * VIEWER_SCALE)); // scale 
			}
			cv::imshow("Captured Frame", image);
			cv::waitKey(CV_WAITKEY);
			return TRUE;
		}
	}
	
}


bool RealtimeView::_InitVideoWriter(const std::string& filename, int codec, double fps, const cv::Size& frameSize, bool isColor)
{
	_videoWriter.open(filename, codec, fps, frameSize, isColor);
	if (!_videoWriter.isOpened()) {
		std::cerr << "Error: Could not open the video writer." << std::endl;
		return false;
	}
	return true;
}


void RealtimeView::_WriteFrame(const cv::Mat& frame)
{
	if (!_videoWriter.isOpened()) {
		throw std::runtime_error("Error: Video writer is not initialized.");
	}

	_videoWriter.write(frame);
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
	std::cout << "宽度：" << width << std::endl;

	int height = m_pBuffers->GetHeight();
	std::cout << "高度：" << height << std::endl;

	bool ismulti = m_pBuffers->IsMultiFormat();
	std::cout << "是否是多格式：" << ismulti << std::endl;

	int count = m_pBuffers->GetCount();
	std::cout << "缓冲区数量：" << count << std::endl;

	const auto format = m_pBuffers->GetFormat();
	bool a;
	if (format == SapFormatMono8) {
		a = 1;
	}
	else {
		a = 0;
	}
	std::cout << "格式（Mono8为1）：" << a << std::endl;
	int minDepth = GetPixelDepthMin(format);
	int maxDepth = GetPixelDepthMax(format);
	std::cout << "最小位深：" << minDepth << std::endl;
	std::cout << "最大位深：" << maxDepth << std::endl;

	int pixelDepth = m_pBuffers->GetPixelDepth();
	std::cout << "位深：" << format << std::endl;
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
		float bin_val = grayHist.at<float>(i);
		//cvRound返回跟参数最接近的整数值，即四舍五入
		int intensity = cvRound(bin_val * binHeight / grayMaxValue);

		// 绘制直线 这里用每scale条竖向直线代表一个bin
		for (size_t j = 0; j < grayScale; j++)
		{
			cv::line(grayHistImg,
				cv::Point(i * grayScale + j, histHeight - intensity),
				cv::Point(i * grayScale + j, histHeight - 1),
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