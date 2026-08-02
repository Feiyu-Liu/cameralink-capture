#pragma once

#include <opencv2/core.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

enum class PreviewPixelFormat {
    Mono8,
    Bgr8
};

class PreviewFrame {
public:
    PreviewFrame() = default;
    ~PreviewFrame() = default;

    PreviewFrame(const PreviewFrame&) = delete;
    PreviewFrame& operator=(const PreviewFrame&) = delete;
    PreviewFrame(PreviewFrame&&) noexcept = default;
    PreviewFrame& operator=(PreviewFrame&&) noexcept = default;

    static bool CopyFrom(const cv::Mat& source, PreviewFrame& destination, std::string& error);

    bool IsValid() const noexcept;
    int Width() const noexcept { return m_width; }
    int Height() const noexcept { return m_height; }
    std::size_t Stride() const noexcept { return m_stride; }
    PreviewPixelFormat Format() const noexcept { return m_format; }
    const std::uint8_t* Data() const noexcept { return m_pixels.empty() ? nullptr : m_pixels.data(); }
    std::size_t ByteCount() const noexcept { return m_pixels.size(); }

private:
    int m_width = 0;
    int m_height = 0;
    std::size_t m_stride = 0;
    PreviewPixelFormat m_format = PreviewPixelFormat::Mono8;
    std::vector<std::uint8_t> m_pixels;
};

class IPreviewSink {
public:
    virtual ~IPreviewSink() = default;

    // frame is borrowed synchronously and must not be retained by an implementation.
    virtual void Publish(const cv::Mat& frame) = 0;
};

enum class PreviewPublishResult {
    Published,
    Disabled,
    Throttled,
    InvalidFrame
};

class PreviewMailbox final : public IPreviewSink {
public:
    explicit PreviewMailbox(double maximumFramesPerSecond = 30.0);

    void Publish(const cv::Mat& frame) override;
    PreviewPublishResult PublishCopy(const cv::Mat& frame, std::string& error);

    bool TryTake(PreviewFrame& frame);
    std::optional<PreviewFrame> TakeLatest();
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void SetMaximumFramesPerSecond(double framesPerSecond);
    double MaximumFramesPerSecond() const;
    void Clear();

private:
    using Clock = std::chrono::steady_clock;

    bool IsThrottledLocked(Clock::time_point now) const;

    mutable std::mutex m_mutex;
    std::optional<PreviewFrame> m_latest;
    bool m_enabled = true;
    std::chrono::steady_clock::duration m_minimumInterval {};
    Clock::time_point m_lastPublished {};
};
