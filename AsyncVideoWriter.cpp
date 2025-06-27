#include "AsyncVideoWriter.h"

AsyncVideoWriter::AsyncVideoWriter() {}

AsyncVideoWriter::~AsyncVideoWriter() {
    release();
}

bool AsyncVideoWriter::open(const std::string& filename, int fourcc,
    double fps, const cv::Size& frameSize, bool isColor) {
    if (_isOpened) {
        return false;
    }

    if (!_writer.open(filename, fourcc, fps, frameSize, isColor)) {
        return false;
    }

    _isOpened = true;
    _stopThread = false;
    _thread = std::thread(&AsyncVideoWriter::_writerThread, this);
    return true;
}

void AsyncVideoWriter::write(const cv::Mat& frame) {
    if (!_isOpened) {
        return;
    }

    std::unique_lock<std::mutex> lock(_mutex);
    // 如果队列已满，等待消费者处理
    while (_frameQueue.size() >= MAX_QUEUE_SIZE) {
        _condition.wait(lock);
    }

    _frameQueue.push(frame.clone()); // 必须克隆帧，因为原始帧可能会被释放
    _condition.notify_one();
}

void AsyncVideoWriter::release() {
    if (!_isOpened) {
        return;
    }

    _stopThread = true;
    _condition.notify_one();

    if (_thread.joinable()) {
        _thread.join();
    }

    std::unique_lock<std::mutex> lock(_mutex);
    while (!_frameQueue.empty()) {
        _frameQueue.pop();
    }

    _writer.release();
    _isOpened = false;
}

void AsyncVideoWriter::_writerThread() {
    while (true) {
        std::unique_lock<std::mutex> lock(_mutex);

        while (_frameQueue.empty() && !_stopThread) {
            _condition.wait(lock);
        }

        if (_stopThread && _frameQueue.empty()) {
            break;
        }

        cv::Mat frame = _frameQueue.front();
        _frameQueue.pop();

        // 通知生产者队列有空间了
        _condition.notify_one();

        lock.unlock();

        _writer.write(frame);
    }
}