#pragma once
//// ��������
#define GRABBER_CONFIG_PATH "C:\\LiuFeiyu\\camConfigFile\\8tip-2048.ccf" // �ɼ��������ļ�·��
#define SAVE_PATH "D:\\liufeiyu\\videos\\" // ��Ƶ����·��
#define VIDEO_EXT ".mp4"
#define VIDEO_FILE_NAME "test6.mp4"

#define GRABBER_INDEX 0
#define CAMERA_INDEX 1


//// ������ʾ����
#define VIEWER_SCALE 0.4 // ��ʾ����ͼ���ԭ�ֱ��ʵı�������ֹ��ͼ���󳬳���Ļ
#define CV_PIXEL_FORMAT CV_8UC1 // cv���ظ�ʽ
#define CV_WAITKEY 1 // wait key������

#define FOCUS_PEAKING_LAYER 0 // �Ƿ���ʾ��ֵ�Խ�ͼ��
#define HIST_LAYER 0 // �Ƿ���ʾֱ��ͼ
#define MOTION_DETECTOR_LAYER 0 // �Ƿ���ʾ�˶�����ͼ��

//// ¼������
#define RECORD_MODE 2 // ¼��ģʽ: 1=��ʽ¼�ƣ�2=����ʽ¼��
#define FRAME_RATE 120 // ¼��֡��:����
#define SAVE_AS_FRAME_SEQUENCE 0 // �Ƿ񱣴�Ϊ֡����
#define ENCODER cv::VideoWriter::fourcc('H', '2', '6', '4')  // ��Ƶ������
/*
��ѡ��������
fourcc('H', '2', '6', '4') // .mp4 h264(�����Ժã��Ƽ�)
fourcc('H', 'E', 'V', '1') // .mp4 hevc
fourcc('M', 'J', 'P', 'G') // .avi (mjpeg)
fourcc('Y', 'U', 'Y', 'V')  // .avi (uncompressed)
fourcc('A', 'Z', 'P', 'R')  // .mov (Apple ProRes 422)
*/

// 1: ��ʽ¼�����ã���������ܺã���������ض�֡��
#define BUFFER_COUNT 1000  // buffer�е�֡����
#define PAUSE_VIEW 1 // ¼��ʱ�Ƿ���ͣ��ʾ����ʽ¼�����ͷ����ܣ�

// 2������ʽ¼�����ã��Ƽ���������ڴ�󣬲��ᶪ֡��
#define RECORD_FRAME 600 // ¼����֡��

// ����
#define IS_ROUND_FRAMERATE 1 // �Ƿ����ʾ֡��ȡ��

/*
q���˳�����
g ����ʼ������
p ����ͣ������
i����ʾͼ����Ϣ

r����ʼ¼��
s��ֹͣ¼��
*/