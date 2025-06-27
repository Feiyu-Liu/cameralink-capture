#pragma once
#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class AsyncVideoWriter {
public:
    AsyncVideoWriter();
    ~AsyncVideoWriter();

    bool open(const std::string& filename, int fourcc, double fps,
        const cv::Size& frameSize, bool isColor = true);
    void write(const cv::Mat& frame);
    void release();
    bool isOpened() const { return _isOpened; }

private:
    void _writerThread();

    cv::VideoWriter _writer;
    std::queue<cv::Mat> _frameQueue;
    std::mutex _mutex;
    std::condition_variable _condition;
    std::thread _thread;
    std::atomic<bool> _isOpened{ false };
    std::atomic<bool> _stopThread{ false };
    const size_t MAX_QUEUE_SIZE = 100; // 限制队列大小防止内存溢出
};