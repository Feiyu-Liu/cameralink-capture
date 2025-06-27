#pragma once
//// 常用设置
#define GRABBER_CONFIG_PATH "C:\\LiuFeiyu\\camConfigFile\\8tip-2048.ccf" // 采集卡配置文件路径
#define SAVE_PATH "D:\\liufeiyu\\videos\\" // 视频保存路径
#define VIDEO_EXT ".mp4"
#define VIDEO_FILE_NAME "test6.mp4"

#define GRABBER_INDEX 0
#define CAMERA_INDEX 1


//// 画面显示设置
#define VIEWER_SCALE 0.4 // 显示的视图相比原分辨率的比例，防止视图过大超出屏幕
#define CV_PIXEL_FORMAT CV_8UC1 // cv像素格式
#define CV_WAITKEY 1 // wait key，毫秒

#define FOCUS_PEAKING_LAYER 0 // 是否显示峰值对焦图层
#define HIST_LAYER 0 // 是否显示直方图
#define MOTION_DETECTOR_LAYER 0 // 是否显示运动跟踪图层

//// 录制设置
#define RECORD_MODE 2 // 录制模式: 1=流式录制，2=非流式录制
#define FRAME_RATE 120 // 录制帧率:整数
#define SAVE_AS_FRAME_SEQUENCE 0 // 是否保存为帧序列
#define ENCODER cv::VideoWriter::fourcc('H', '2', '6', '4')  // 视频编码器
/*
可选编码器：
fourcc('H', '2', '6', '4') // .mp4 h264(兼容性好，推荐)
fourcc('H', 'E', 'V', '1') // .mp4 hevc
fourcc('M', 'J', 'P', 'G') // .avi (mjpeg)
fourcc('Y', 'U', 'Y', 'V')  // .avi (uncompressed)
fourcc('A', 'Z', 'P', 'R')  // .mov (Apple ProRes 422)
*/

// 1: 流式录制设置（需电脑性能好，否则会严重丢帧）
#define BUFFER_COUNT 1000  // buffer中的帧数量
#define PAUSE_VIEW 1 // 录制时是否暂停显示（流式录制下释放性能）

// 2：非流式录制设置（推荐，需电脑内存大，不会丢帧）
#define RECORD_FRAME 600 // 录制总帧数

// 其他
#define IS_ROUND_FRAMERATE 1 // 是否对显示帧率取整

/*
q：退出程序
g ：开始捕获画面
p ：暂停捕获画面
i：显示图像信息

r：开始录制
s：停止录制
*/