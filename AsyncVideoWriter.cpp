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
    // ��������������ȴ������ߴ���
    while (_frameQueue.size() >= MAX_QUEUE_SIZE) {
        _condition.wait(lock);
    }

    _frameQueue.push(frame.clone()); // �����¡֡����Ϊԭʼ֡���ܻᱻ�ͷ�
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

        // ֪ͨ�����߶����пռ���
        _condition.notify_one();

        lock.unlock();

        _writer.write(frame);
    }
}