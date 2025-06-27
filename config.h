#pragma once
// ��������
#define GRABBER_CONFIG_PATH "C:\\LiuFeiyu\\camConfigFile\\8tip-2048.ccf" // �ɼ��������ļ�·��
#define SAVE_PATH "D:\\liufeiyu\\videos\\" // ��Ƶ����·��
#define VIDEO_FILE_NAME "test2.mp4"

#define GRABBER_INDEX 0
#define CAMERA_INDEX 1

// �������
#define BUFFER_COUNT 1000  // buffer�е�֡����

// ������ʾ����
#define CV_WAITKEY 1 // wait key������
#define VIEWER_SCALE 0.4 // ��ʾ����ͼ���ԭ�ֱ��ʵı�������ֹ��ͼ���󳬳���Ļ

#define FOCUS_PEAKING_LAYER 0 // �Ƿ���ʾ��ֵ�Խ�ͼ��
#define HIST_LAYER 0 // �Ƿ���ʾֱ��ͼ
#define MOTION_DETECTOR_LAYER 0 // �Ƿ���ʾ�˶�����ͼ��

// ¼������
#define RECORD_MODE 2 // ¼��ģʽ: 1=����¼��(���ܻᶪ֡;buffer=BUFFER_COUNT)��2=�̶�֡��¼��(���Զ�����buffer=RECORD_FRAME)
#define RECORD_FRAME 840 // ¼����֡��
#define PAUSE_VIEW 0 // ¼��ʱ�Ƿ���ͣ��ʾ���ͷ����ܣ�
#define FRAME_RATE 120 // ¼��֡��:����

// ѡ�������
#define ENCODER cv::VideoWriter::fourcc('H', '2', '6', '4') // .mp4 h264
//#define ENCODER cv::VideoWriter::fourcc('H', 'E', 'V', '1')  // .mp4 hevc
//#define ENCODER cv::VideoWriter::fourcc('M', 'J', 'P', 'G') // .avi (mjpeg)
//#define ENCODER cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V')  // .avi (uncompressed)
//#define QUICK_TIME_ENCODER cv::VideoWriter::fourcc('A', 'Z', 'P', 'R')  // .mov (Apple ProRes 422)

// ����
#define IS_ROUND_FRAMERATE 1 // �Ƿ��¼��֡��ȡ��


/*
q���˳�����
g ����ʼ������
p ����ͣ������
i����ʾͼ����Ϣ

r����ʼ¼��
s��ֹͣ¼��
*/