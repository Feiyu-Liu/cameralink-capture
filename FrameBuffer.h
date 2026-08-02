#pragma once

#include <SapClassBasic.h>
#include <opencv2/core.hpp>

#include <string>

struct FrameLayout {
    int width = 0;
    int height = 0;
    int pitch = 0;
    int cvType = -1;

    static bool ValidateMono8(
        int width,
        int height,
        int pitch,
        SapFormat format,
        int pixelDepth,
        int bytesPerPixel,
        FrameLayout& layout,
        std::string& error);

    static bool FromSapBuffer(const SapBuffer& buffer, FrameLayout& layout, std::string& error);
    bool Wrap(void* address, cv::Mat& image, std::string& error) const;
};

class MappedMono8Frame {
public:
    MappedMono8Frame() noexcept = default;
    ~MappedMono8Frame() noexcept;

    MappedMono8Frame(const MappedMono8Frame&) = delete;
    MappedMono8Frame& operator=(const MappedMono8Frame&) = delete;

    MappedMono8Frame(MappedMono8Frame&& other) noexcept;
    MappedMono8Frame& operator=(MappedMono8Frame&& other) noexcept;

    bool Map(SapBuffer& buffer, int index, const FrameLayout& layout, std::string& error);
    void Reset() noexcept;

    bool IsValid() const noexcept { return !m_image.empty(); }
    cv::Mat& Image() noexcept { return m_image; }
    const cv::Mat& Image() const noexcept { return m_image; }

private:
    static bool RequiresRelease(SapBuffer::Type type) noexcept;

    SapBuffer* m_buffer = nullptr;
    int m_index = -1;
    void* m_address = nullptr;
    bool m_requiresRelease = false;
    cv::Mat m_image;
};
