#pragma once
// 常用设置
#define GRABBER_CONFIG_PATH "C:\\LiuFeiyu\\camConfigFile\\8tip-2048.ccf" // 采集卡配置文件路径
#define SAVE_PATH "D:\\liufeiyu\\videos\\" // 视频保存路径
#define VIDEO_FILE_NAME "test2.mp4"

#define GRABBER_INDEX 0
#define CAMERA_INDEX 1

// 相机设置
#define BUFFER_COUNT 1000  // buffer中的帧数量

// 画面显示设置
#define CV_WAITKEY 1 // wait key，毫秒
#define VIEWER_SCALE 0.4 // 显示的视图相比原分辨率的比例，防止视图过大超出屏幕

#define FOCUS_PEAKING_LAYER 0 // 是否显示峰值对焦图层
#define HIST_LAYER 0 // 是否显示直方图
#define MOTION_DETECTOR_LAYER 0 // 是否显示运动跟踪图层

// 录制设置
#define RECORD_MODE 2 // 录制模式: 1=自由录制(可能会丢帧;buffer=BUFFER_COUNT)，2=固定帧数录制(将自动设置buffer=RECORD_FRAME)
#define RECORD_FRAME 840 // 录制总帧数
#define PAUSE_VIEW 0 // 录制时是否暂停显示（释放性能）
#define FRAME_RATE 120 // 录制帧率:整数

// 选择编码器
#define ENCODER cv::VideoWriter::fourcc('H', '2', '6', '4') // .mp4 h264
//#define ENCODER cv::VideoWriter::fourcc('H', 'E', 'V', '1')  // .mp4 hevc
//#define ENCODER cv::VideoWriter::fourcc('M', 'J', 'P', 'G') // .avi (mjpeg)
//#define ENCODER cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V')  // .avi (uncompressed)
//#define QUICK_TIME_ENCODER cv::VideoWriter::fourcc('A', 'Z', 'P', 'R')  // .mov (Apple ProRes 422)

// 其他
#define IS_ROUND_FRAMERATE 1 // 是否对录制帧率取整


/*
q：退出程序
g ：开始捕获画面
p ：暂停捕获画面
i：显示图像信息

r：开始录制
s：停止录制
*/