#include "PreviewMailbox.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace {

bool CheckedRowBytes(int width, int channels, std::size_t& rowBytes) {
    if (width <= 0 || channels <= 0) {
        return false;
    }
    const std::size_t unsignedWidth = static_cast<std::size_t>(width);
    const std::size_t unsignedChannels = static_cast<std::size_t>(channels);
    if (unsignedWidth > std::numeric_limits<std::size_t>::max() / unsignedChannels) {
        return false;
    }
    rowBytes = unsignedWidth * unsignedChannels;
    return true;
}

} // namespace

bool PreviewFrame::CopyFrom(const cv::Mat& source, PreviewFrame& destination, std::string& error) {
    if (source.empty() || source.data == nullptr) {
        error = "Preview source frame is empty.";
        return false;
    }

    PreviewPixelFormat format;
    int channels = 0;
    if (source.type() == CV_8UC1) {
        format = PreviewPixelFormat::Mono8;
        channels = 1;
    }
    else if (source.type() == CV_8UC3) {
        format = PreviewPixelFormat::Bgr8;
        channels = 3;
    }
    else {
        error = "Preview source must be CV_8UC1 or CV_8UC3.";
        return false;
    }

    std::size_t rowBytes = 0;
    if (!CheckedRowBytes(source.cols, channels, rowBytes) || source.rows <= 0) {
        error = "Preview source dimensions are invalid.";
        return false;
    }
    if (source.step[0] < rowBytes) {
        error = "Preview source pitch is shorter than a row.";
        return false;
    }
    const std::size_t unsignedHeight = static_cast<std::size_t>(source.rows);
    if (rowBytes > std::numeric_limits<std::size_t>::max() / unsignedHeight) {
        error = "Preview source size overflows the supported range.";
        return false;
    }

    PreviewFrame copy;
    copy.m_width = source.cols;
    copy.m_height = source.rows;
    copy.m_stride = rowBytes;
    copy.m_format = format;
    try {
        copy.m_pixels.resize(rowBytes * unsignedHeight);
    }
    catch (const std::bad_alloc&) {
        error = "Could not allocate preview frame pixels.";
        return false;
    }

    for (int row = 0; row < source.rows; ++row) {
        const auto* sourceRow = source.ptr<std::uint8_t>(row);
        auto* destinationRow = copy.m_pixels.data() + static_cast<std::size_t>(row) * rowBytes;
        std::memcpy(destinationRow, sourceRow, rowBytes);
    }

    destination = std::move(copy);
    error.clear();
    return true;
}

bool PreviewFrame::IsValid() const noexcept {
    return m_width > 0 && m_height > 0 && m_stride > 0 && Data() != nullptr &&
        m_pixels.size() == m_stride * static_cast<std::size_t>(m_height);
}

PreviewMailbox::PreviewMailbox(double maximumFramesPerSecond) {
    SetMaximumFramesPerSecond(maximumFramesPerSecond);
}

void PreviewMailbox::Publish(const cv::Mat& frame) {
    std::string ignoredError;
    PublishCopy(frame, ignoredError);
}

PreviewPublishResult PreviewMailbox::PublishCopy(const cv::Mat& frame, std::string& error) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_enabled) {
            error.clear();
            return PreviewPublishResult::Disabled;
        }
        if (IsThrottledLocked(Clock::now())) {
            error.clear();
            return PreviewPublishResult::Throttled;
        }
    }

    PreviewFrame copy;
    if (!PreviewFrame::CopyFrom(frame, copy, error)) {
        return PreviewPublishResult::InvalidFrame;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_enabled) {
        error.clear();
        return PreviewPublishResult::Disabled;
    }
    const Clock::time_point now = Clock::now();
    if (IsThrottledLocked(now)) {
        error.clear();
        return PreviewPublishResult::Throttled;
    }

    m_latest.emplace(std::move(copy));
    m_lastPublished = now;
    error.clear();
    return PreviewPublishResult::Published;
}

bool PreviewMailbox::TryTake(PreviewFrame& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_latest.has_value()) {
        return false;
    }
    frame = std::move(*m_latest);
    m_latest.reset();
    return true;
}

std::optional<PreviewFrame> PreviewMailbox::TakeLatest() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_latest.has_value()) {
        return std::nullopt;
    }
    std::optional<PreviewFrame> frame(std::move(*m_latest));
    m_latest.reset();
    return frame;
}

void PreviewMailbox::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = enabled;
    if (!m_enabled) {
        m_latest.reset();
        m_lastPublished = Clock::time_point {};
    }
}

bool PreviewMailbox::IsEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enabled;
}

void PreviewMailbox::SetMaximumFramesPerSecond(double framesPerSecond) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!std::isfinite(framesPerSecond) || framesPerSecond <= 0.0) {
        m_minimumInterval = Clock::duration::zero();
        return;
    }

    const std::chrono::duration<double> interval(1.0 / framesPerSecond);
    m_minimumInterval = std::chrono::duration_cast<Clock::duration>(interval);
}

double PreviewMailbox::MaximumFramesPerSecond() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_minimumInterval <= Clock::duration::zero()) {
        return 0.0;
    }
    return 1.0 / std::chrono::duration<double>(m_minimumInterval).count();
}

void PreviewMailbox::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_latest.reset();
}

bool PreviewMailbox::IsThrottledLocked(Clock::time_point now) const {
    return m_minimumInterval > Clock::duration::zero() &&
        m_lastPublished != Clock::time_point {} &&
        now - m_lastPublished < m_minimumInterval;
}
