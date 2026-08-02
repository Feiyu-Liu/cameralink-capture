#include "CaptureWindow.h"
#include "FrameBuffer.h"
#include "SaperaResource.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << std::endl;
    }
}

void TestMono8LayoutWithPadding() {
    FrameLayout layout;
    std::string error;
    Expect(FrameLayout::ValidateMono8(4, 3, 8, SapFormatMono8, 8, 1, layout, error),
        "valid padded Mono8 layout should be accepted");
    Expect(layout.cvType == CV_8UC1, "Mono8 must map to CV_8UC1");

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(layout.pitch * layout.height), 0xEE);
    for (int row = 0; row < layout.height; ++row) {
        for (int column = 0; column < layout.width; ++column) {
            bytes[static_cast<std::size_t>(row * layout.pitch + column)] =
                static_cast<std::uint8_t>(row * 10 + column);
        }
    }

    cv::Mat image;
    Expect(layout.Wrap(bytes.data(), image, error), "valid memory should wrap as a Mat");
    Expect(image.rows == 3 && image.cols == 4, "wrapped Mat dimensions should match layout");
    Expect(image.step[0] == 8, "wrapped Mat must preserve Sapera pitch");
    Expect(image.at<std::uint8_t>(2, 3) == 23, "wrapped Mat must address padded rows correctly");

    cv::Mat owned = image.clone();
    bytes[0] = 99;
    Expect(owned.at<std::uint8_t>(0, 0) == 0, "clone must own data independently");
}

void TestInvalidLayouts() {
    FrameLayout layout;
    std::string error;
    Expect(!FrameLayout::ValidateMono8(4, 3, 8, SapFormatMono16, 16, 2, layout, error),
        "Mono16 must be rejected");
    Expect(!FrameLayout::ValidateMono8(4, 3, 3, SapFormatMono8, 8, 1, layout, error),
        "pitch shorter than a row must be rejected");
    Expect(!FrameLayout::ValidateMono8(0, 3, 8, SapFormatMono8, 8, 1, layout, error),
        "zero width must be rejected");

    layout = { 4, 3, 4, CV_8UC1 };
    cv::Mat image;
    Expect(!layout.Wrap(nullptr, image, error), "null address must be rejected");

    SapBuffer buffer;
    MappedMono8Frame mapped;
    Expect(!mapped.Map(buffer, -1, layout, error), "negative buffer index must be rejected");
    Expect(!mapped.Map(buffer, buffer.GetCount(), layout, error),
        "buffer index at count must be rejected");
}

void TestCircularIndices() {
    std::vector<int> indices;
    std::string error;
    Expect(BuildCircularIndices(0, 2, 5, indices, error) && indices == std::vector<int>({ 0, 1 }),
        "window starting at first slot should build");
    Expect(BuildCircularIndices(2, 2, 5, indices, error) && indices == std::vector<int>({ 2, 3 }),
        "window starting at middle slot should build");
    Expect(BuildCircularIndices(4, 2, 5, indices, error) && indices == std::vector<int>({ 4, 0 }),
        "window starting at last slot should wrap");
    Expect(BuildCircularIndices(3, 5, 5, indices, error), "full wrapped window should build");
    Expect(indices == std::vector<int>({ 3, 4, 0, 1, 2 }), "wrapped indices should stay ordered");
    Expect(!BuildCircularIndices(5, 1, 5, indices, error), "out-of-range first index must fail");
    Expect(!BuildCircularIndices(0, 0, 5, indices, error), "zero frame count must fail");
}

void TestKeyboardWindowAcrossWrap() {
    FrameArrivalTracker tracker;
    tracker.Reset(5, 4);
    std::string error;
    Expect(tracker.Arm(3, 0, error), "keyboard window should arm");
    tracker.OnFrame(0, 100);
    tracker.OnFrame(1, 101);
    tracker.OnFrame(2, 102);
    const CaptureWindow window = tracker.WaitForCompletion(std::chrono::milliseconds(1));
    Expect(window.status == CaptureWindowStatus::Complete, "keyboard window should complete");
    Expect(window.firstIndex == 0 && window.frameCount == 3, "keyboard first frame should be next callback");
}

void TestTtlSkipAcrossWrap() {
    FrameArrivalTracker tracker;
    tracker.Reset(5, 3);
    std::string error;
    Expect(tracker.Arm(3, 1, error), "TTL window should arm");
    tracker.OnFrame(4, 200);
    tracker.OnFrame(0, 201);
    tracker.OnFrame(1, 202);
    tracker.OnFrame(2, 203);
    const CaptureWindow window = tracker.WaitForCompletion(std::chrono::milliseconds(1));
    Expect(window.status == CaptureWindowStatus::Complete, "TTL window should complete");
    Expect(window.firstIndex == 0, "TTL trigger frame should be skipped across wrap");
}

void TestTrackerFailures() {
    std::string error;

    FrameArrivalTracker discontinuity;
    discontinuity.Reset(5, 0);
    discontinuity.Arm(2, 0, error);
    discontinuity.OnFrame(2, 1);
    Expect(discontinuity.Snapshot().status == CaptureWindowStatus::IndexDiscontinuity,
        "index discontinuity must fail an armed window");

    FrameArrivalTracker trash;
    trash.Reset(5, 0);
    trash.Arm(2, 0, error);
    trash.OnTrashFrame(2);
    Expect(trash.Snapshot().status == CaptureWindowStatus::TrashFrame,
        "trash callback must fail an armed window");

    FrameArrivalTracker timeout;
    timeout.Reset(5, 0);
    timeout.Arm(2, 0, error);
    Expect(timeout.WaitForCompletion(std::chrono::milliseconds(1)).status == CaptureWindowStatus::TimedOut,
        "unfinished window must time out");

    FrameArrivalTracker cancelled;
    cancelled.Reset(5, 0);
    cancelled.Arm(2, 0, error);
    cancelled.Cancel("test cancellation");
    Expect(cancelled.Snapshot().status == CaptureWindowStatus::Cancelled,
        "cancelled window must expose cancelled status");
}

void TestOverwriteDetection() {
    FrameArrivalTracker tracker;
    tracker.Reset(5, 4);
    std::string error;
    tracker.Arm(2, 0, error);
    tracker.OnFrame(0, 1);
    tracker.OnFrame(1, 2);
    const CaptureWindow window = tracker.Snapshot();
    Expect(IsCaptureWindowIntact(window, tracker.TotalNormalFrames(), 5, error),
        "newly completed window should be intact");
    Expect(!IsCaptureWindowIntact(window, window.firstSequence + 5, 5, error),
        "window older than a full ring must be rejected");
}

struct FakeSapObject {
    FakeSapObject(int value, std::vector<int>& destroyed)
        : id(value), destructionLog(destroyed) {
    }

    explicit operator bool() const noexcept { return created; }
    int Destroy() noexcept {
        if (created) {
            created = false;
            destructionLog.push_back(id);
        }
        return 1;
    }

    int id;
    std::vector<int>& destructionLog;
    bool created = true;
};

void TestSapOwnedLifetime() {
    std::vector<int> destroyed;
    {
        SapOwned<FakeSapObject> first(std::make_unique<FakeSapObject>(1, destroyed));
        SapOwned<FakeSapObject> moved(std::move(first));
        Expect(moved.Destroy(), "explicit destroy should succeed");
        Expect(moved.Destroy(), "repeated destroy should be idempotent");
    }
    Expect(destroyed == std::vector<int>({ 1 }), "moved SapOwned object must destroy exactly once");

    struct FakeSession {
        FakeSession(std::vector<int>& log, int initializedStages) {
            if (initializedStages >= 1) {
                acquisition.Reset(std::make_unique<FakeSapObject>(1, log));
            }
            if (initializedStages >= 2) {
                buffers.Reset(std::make_unique<FakeSapObject>(2, log));
            }
            if (initializedStages >= 3) {
                transfer.Reset(std::make_unique<FakeSapObject>(3, log));
            }
        }
        SapOwned<FakeSapObject> acquisition;
        SapOwned<FakeSapObject> buffers;
        SapOwned<FakeSapObject> transfer;
    };
    const std::vector<std::vector<int>> expectedByStage {
        { 1 },
        { 2, 1 },
        { 3, 2, 1 }
    };
    for (int stage = 1; stage <= 3; ++stage) {
        destroyed.clear();
        {
            FakeSession session(destroyed, stage);
        }
        Expect(destroyed == expectedByStage[static_cast<std::size_t>(stage - 1)],
            "partially initialized resources must unwind in reverse order");
    }
}

} // namespace

int main() {
    TestMono8LayoutWithPadding();
    TestInvalidLayouts();
    TestCircularIndices();
    TestKeyboardWindowAcrossWrap();
    TestTtlSkipAcrossWrap();
    TestTrackerFailures();
    TestOverwriteDetection();
    TestSapOwnedLifetime();

    if (failures == 0) {
        std::cout << "All core tests passed." << std::endl;
        return 0;
    }
    std::cerr << failures << " core test(s) failed." << std::endl;
    return 1;
}
