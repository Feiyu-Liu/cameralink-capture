#include "FrameBuffer.h"

#include <sstream>
#include <utility>

bool FrameLayout::ValidateMono8(
    int width,
    int height,
    int pitch,
    SapFormat format,
    int pixelDepth,
    int bytesPerPixel,
    FrameLayout& layout,
    std::string& error) {
    if (format != SapFormatMono8) {
        std::ostringstream stream;
        stream << "Unsupported Sapera format " << format << "; only SapFormatMono8 is supported.";
        error = stream.str();
        return false;
    }
    if (pixelDepth != 8 || bytesPerPixel != 1) {
        std::ostringstream stream;
        stream << "Invalid Mono8 layout: depth=" << pixelDepth
               << ", bytesPerPixel=" << bytesPerPixel << '.';
        error = stream.str();
        return false;
    }
    if (width <= 0 || height <= 0) {
        error = "Frame width and height must be positive.";
        return false;
    }
    if (pitch < width * bytesPerPixel) {
        std::ostringstream stream;
        stream << "Frame pitch " << pitch << " is smaller than one image row "
               << width * bytesPerPixel << '.';
        error = stream.str();
        return false;
    }

    layout.width = width;
    layout.height = height;
    layout.pitch = pitch;
    layout.cvType = CV_8UC1;
    error.clear();
    return true;
}

bool FrameLayout::FromSapBuffer(const SapBuffer& buffer, FrameLayout& layout, std::string& error) {
    return ValidateMono8(
        buffer.GetWidth(),
        buffer.GetHeight(),
        buffer.GetPitch(),
        buffer.GetFormat(),
        buffer.GetPixelDepth(),
        buffer.GetBytesPerPixel(),
        layout,
        error);
}

bool FrameLayout::Wrap(void* address, cv::Mat& image, std::string& error) const {
    if (address == nullptr) {
        error = "Sapera returned a null frame address.";
        return false;
    }
    if (width <= 0 || height <= 0 || pitch < width || cvType != CV_8UC1) {
        error = "FrameLayout is not a valid Mono8 layout.";
        return false;
    }

    image = cv::Mat(height, width, cvType, address, static_cast<size_t>(pitch));
    error.clear();
    return true;
}

MappedMono8Frame::~MappedMono8Frame() noexcept {
    Reset();
}

MappedMono8Frame::MappedMono8Frame(MappedMono8Frame&& other) noexcept {
    *this = std::move(other);
}

MappedMono8Frame& MappedMono8Frame::operator=(MappedMono8Frame&& other) noexcept {
    if (this != &other) {
        Reset();
        m_buffer = other.m_buffer;
        m_index = other.m_index;
        m_address = other.m_address;
        m_requiresRelease = other.m_requiresRelease;
        m_image = std::move(other.m_image);

        other.m_buffer = nullptr;
        other.m_index = -1;
        other.m_address = nullptr;
        other.m_requiresRelease = false;
    }
    return *this;
}

bool MappedMono8Frame::Map(
    SapBuffer& buffer,
    int index,
    const FrameLayout& layout,
    std::string& error) {
    Reset();
    if (index < 0 || index >= buffer.GetCount()) {
        std::ostringstream stream;
        stream << "Buffer index " << index << " is outside [0, " << buffer.GetCount() << ").";
        error = stream.str();
        return false;
    }

    void* address = nullptr;
    if (!buffer.GetAddress(index, &address) || address == nullptr) {
        error = "SapBuffer::GetAddress failed.";
        return false;
    }

    cv::Mat image;
    if (!layout.Wrap(address, image, error)) {
        if (RequiresRelease(buffer.GetType())) {
            buffer.ReleaseAddress(index, address);
        }
        return false;
    }

    m_buffer = &buffer;
    m_index = index;
    m_address = address;
    m_requiresRelease = RequiresRelease(buffer.GetType());
    m_image = std::move(image);
    return true;
}

void MappedMono8Frame::Reset() noexcept {
    m_image.release();
    if (m_requiresRelease && m_buffer != nullptr && m_index >= 0) {
        m_buffer->ReleaseAddress(m_index, m_address);
    }
    m_buffer = nullptr;
    m_index = -1;
    m_address = nullptr;
    m_requiresRelease = false;
}

bool MappedMono8Frame::RequiresRelease(SapBuffer::Type type) noexcept {
    return type == SapBuffer::TypeUnmapped ||
           type == SapBuffer::TypeScatterGatherUnmapped ||
           type == SapBuffer::TypeOffscreenVideo ||
           type == SapBuffer::TypeOverlay;
}
