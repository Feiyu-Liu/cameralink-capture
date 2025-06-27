#include "RealtimeView.h"


RealtimeView::RealtimeView(SapBuffer* pBuffers, SapProCallback pCallback, void* pContext)
	:SapProcessing(pBuffers, pCallback, pContext)
{
	// ���캯��
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
	
	int proIndex = this->GetIndex();  // ����Ļ�ȡ����������Execute/ExecuteNext�����

	if (keyControler != 0) {
		switch (keyControler) {
            case 1:
				this->_BufferInfoDisplay();
				keyControler = 0;
				break;
			case 2:
				if (!_isRecording) {
					if (!SAVE_AS_FRAME_SEQUENCE) { // ¼����Ƶ
						std::stringstream ss;
						ss << SAVE_PATH << VIDEO_FILE_NAME;
						std::string filePath = ss.str();

						cv::Size frameSize(_imageWidth, _imageHeight);
						_isRecording = _InitVideoWriter(filePath, ENCODER, FRAME_RATE, frameSize, false);
							
						std::cout << "\n\n��ʼ��ʽ¼��\n\n" << std::endl;
						return TRUE;
					}
					else { // ¼������֡
						_isRecording = true;
						_imageConter = 1;

						std::cout << "\n\n��ʼ��ʽ¼��(����Ϊ����֡)\n\n" << std::endl;
						return TRUE;
					}
				}
				keyControler = 0;
                break;
			case 3:
				if (!SAVE_AS_FRAME_SEQUENCE) { // ¼����Ƶ
                    _ReleaseVideoWriter();
				}
				_isRecording = false;
				keyControler = 0;
				std::cout << "\n\nֹͣ��ʽ¼��\n" << "�ѱ�������" << SAVE_PATH << std::endl;
				break;
			default:
				break;
		}
	}

	// ����ͼ��
	void* outAddress = NULL;   // �����buffer��ȡ-��ѹ��ͼ������
	
	m_pBuffers->GetAddress(proIndex, &outAddress);
	// std::cout << proIndex << std::endl;
	// GetFormat\GetPixelDepth\GetBytesPerPixel\GetPitch
	// CV_8UC1��ӦSapFormatMono8
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


	if (_isRecording && PAUSE_VIEW) { // ¼��ʱ����ʾ����
		return TRUE;
	}
	else { // ʵʱԤ��
		_imageWidth = m_pBuffers->GetWidth();
		_imageHeight = m_pBuffers->GetHeight();
		cv::Mat image(_imageHeight, _imageWidth, CV_PIXEL_FORMAT, outAddress);

		if (FOCUS_PEAKING_LAYER || HIST_LAYER || MOTION_DETECTOR_LAYER) {
			cv::Mat viewImage;
			cv::cvtColor(image, viewImage, cv::COLOR_GRAY2BGR); // ��ɫ��ʽת�����Ա������ʾ
			if (FOCUS_PEAKING_LAYER) {
				cv::Mat focusPeakingLayer = _FocusPeakingLayer(image); // ��ֵ�Խ�ͼ��
				cv::add(viewImage, focusPeakingLayer, viewImage);
			}

			if (HIST_LAYER) {
				cv::Mat hisLayer = _HistLayer(image); // ֱ��ͼͼ��
				cv::add(viewImage, hisLayer, viewImage);
			}

			if (MOTION_DETECTOR_LAYER) {
				bool isMotionDetected;
				cv::Mat motionLayer = _MotionDetectorLayer(isMotionDetected, this->_lastFrame, image, false, 3000); // �˶����ͼ��
				cv::add(viewImage, motionLayer, viewImage);
				this->_lastFrame = image; // ������һ֡ͼ��
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
	std::cout << "��ȣ�" << width << std::endl;

	int height = m_pBuffers->GetHeight();
	std::cout << "�߶ȣ�" << height << std::endl;

	bool ismulti = m_pBuffers->IsMultiFormat();
	std::cout << "�Ƿ��Ƕ��ʽ��" << ismulti << std::endl;

	int count = m_pBuffers->GetCount();
	std::cout << "������������" << count << std::endl;

	const auto format = m_pBuffers->GetFormat();
	bool a;
	if (format == SapFormatMono8) {
		a = 1;
	}
	else {
		a = 0;
	}
	std::cout << "��ʽ��Mono8Ϊ1����" << a << std::endl;
	int minDepth = GetPixelDepthMin(format);
	int maxDepth = GetPixelDepthMax(format);
	std::cout << "��Сλ�" << minDepth << std::endl;
	std::cout << "���λ�" << maxDepth << std::endl;

	int pixelDepth = m_pBuffers->GetPixelDepth();
	std::cout << "λ�" << format << std::endl;
}

/* ��ֵ�Խ�ͼ�� */
cv::Mat RealtimeView::_FocusPeakingLayer(const cv::Mat& frame)
{
	// ͼ��ƽ��
	cv::Mat imgBlur;
	cv::GaussianBlur(frame, imgBlur, cv::Size(5, 5), 1.5); //��ز��������ж���

	// ��Ե���
	cv::Mat edges;
	cv::Canny(imgBlur, edges, 50, 150);

	cv::Mat redEdges = cv::Mat::zeros(frame.size(), CV_8UC3);
	redEdges.setTo(cv::Scalar(0, 0, 255), edges);

	return redEdges;
}


/* ֱ��ͼ��ʾ*/
cv::Mat RealtimeView::_HistLayer(const cv::Mat& frame)
{
	// ��������
	int grayImgNum = 1; //ͼ����
	int grayChannels = 0; //��Ҫ�����ͨ���� ��ͨ��ֻ��0
	const int grayHistDim = 1; //ֱ��ͼά��
	const int grayHistSize = 256; //ֱ��ͼÿһά��bin����
	float grayRanges[2] = { 0, 255 };  //�Ҷ�ֵ��ͳ�Ʒ�Χ
	const float* grayHistRanges[1] = { grayRanges }; //�Ҷ�ֵͳ�Ʒ�Χָ��                     

	cv::Mat grayHist; 

	//����Ҷ�ͼ���ֱ��ͼ
	cv::calcHist(&frame,
		grayImgNum,
		&grayChannels,
		cv::Mat(),
		grayHist,
		grayHistDim,
		&grayHistSize,
		grayHistRanges,
		true,  //�Ƿ����
		false); //�Ƿ��ۻ�

	int frameWidth = frame.cols;
	int frameHeight = frame.rows;

	
	int grayScale = 2;  //���С
	int histHeight = static_cast<int>(frameHeight/4); //�߶�
	int histWidth = histHeight * grayScale; //���
	int binHeight = static_cast<int>(histHeight*7/8);  //bin�����߶�

	// ֱ��ͼ��ͼƬ����ʼȫ����ֵΪ0
	//cv::Mat grayHistImg(histHeight, histWidth, CV_8UC1, cv::Scalar(0));
	cv::Mat grayHistImg = cv::Mat::zeros(histHeight, histWidth, CV_8UC1);

	double grayMaxValue = 0;
	double grayMinValue = 0;
	cv::minMaxLoc(grayHist, &grayMinValue, &grayMaxValue, NULL, NULL);

	// ֱ��ͼ����
	for (size_t i = 0; i < grayHistSize; i++)
	{
		float bin_val = grayHist.at<float>(i);
		//cvRound���ظ�������ӽ�������ֵ������������
		int intensity = cvRound(bin_val * binHeight / grayMaxValue);

		// ����ֱ�� ������ÿscale������ֱ�ߴ���һ��bin
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
	
	// �Ҷ�ֱ��ͼת��Ϊ��ɫ�����ӱ���
	cv::Mat colorHistImg = cv::Mat::zeros(grayHistImg.size(), CV_8UC3);
	colorHistImg.setTo(cv::Scalar(255, 0, 0), grayHistImg);  // ����

	cv::Mat background = cv::Mat::zeros(frame.size(), CV_8UC3);

	//roi ʵ������ background ��һ����ͼ�������ã�������������һ���µ�ͼ�񣬶���ֱ�������� background ��ĳ������
	cv::Mat roi = background(cv::Rect(frameWidth- histWidth, 0, histWidth, histHeight));

	//���� roi �����޸�ʱ��ʵ���������޸� background ����Ӧ����
	cv::add(colorHistImg, roi, roi);

	return background;
}

/* �˶���� */
cv::Mat RealtimeView::_MotionDetectorLayer(bool& motionDetected,const cv::Mat& lastFrame, const cv::Mat& currentFrame, bool isDelta, const int minSizeMovement)
{
	// �����һ֡Ϊ�գ�ֱ���˳�
	if (lastFrame.empty()) {
		cv::Mat output;
		cvtColor(currentFrame, output, COLOR_GRAY2BGR);
		return output;
	}

	//cv::Mat outputFrame = currentFrame.clone();
	//GaussianBlur(lastFrame, lastFrame, Size(21, 21), 0); // ��˹ģ��̫Ӱ�촦���ٶ�
	//GaussianBlur(currentFrame, currentFrame, Size(21, 21), 0);

	// �Ƚ�����֡���ҵ�����
	Mat frameDelta;
	absdiff(lastFrame, currentFrame, frameDelta);
	Mat thresh;
	threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);

	// ͨ���������׶������ҵ���ֵ������
	dilate(thresh, thresh, Mat(), Point(-1, -1), 2);
	std::vector<std::vector<Point>> contours;
	findContours(thresh.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	// ��������
	cv::Mat boxLayer = cv::Mat::zeros(currentFrame.size(), CV_8UC3);
	motionDetected = false; 
	for (size_t i = 0; i < contours.size(); i++) {
		// ���������ҵ�����������
		Rect bounding_box = boundingRect(contours[i]);

		// �������̫С��������������򣬴���˲ʱ�˶�
		if (contourArea(contours[i]) > minSizeMovement) {
			// ���ƾ��ο��Ա���ʾ�㹻����˶�
			rectangle(boxLayer, bounding_box.tl(), bounding_box.br(), Scalar(0, 255, 0), 3);
			motionDetected = true; // ��⵽�˶�
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