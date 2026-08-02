#include "CaptureRuntime.h"
#include "PreviewMailbox.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

int failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << std::endl;
    }
}

void TestCommandParsing() {
    const CommandParseResult start = ParseCaptureCommand("  G  ");
    Expect(start.kind == ParsedCommandKind::Command && start.command == CaptureCommand::Start,
        "g should parse case-insensitively as Start");
    Expect(ParseCaptureCommand("P").command == CaptureCommand::Pause,
        "p should parse as Pause");
    Expect(ParseCaptureCommand("i").command == CaptureCommand::Info,
        "i should parse as Info");
    Expect(ParseCaptureCommand("r").command == CaptureCommand::Record,
        "r should parse as Record");
    Expect(ParseCaptureCommand("s").command == CaptureCommand::Stop,
        "s should parse as Stop");
    Expect(ParseCaptureCommand("Q").command == CaptureCommand::Quit,
        "q should parse as Quit");
    Expect(ParseCaptureCommand("HELP").kind == ParsedCommandKind::Help,
        "help should remain a UI-local command");
    Expect(ParseCaptureCommand("clear").kind == ParsedCommandKind::Clear,
        "clear should remain a UI-local command");
    Expect(ParseCaptureCommand("  ").kind == ParsedCommandKind::Empty,
        "whitespace should parse as an empty command");
    Expect(ParseCaptureCommand("record").kind == ParsedCommandKind::Unknown,
        "unsupported text should parse as unknown");
}

void TestCommandQueueCancellation() {
    CommandQueue queue;
    queue.Submit(CaptureCommand::Start);
    queue.Submit(CaptureCommand::Stop);
    Expect(queue.IsStopRequested(), "Stop must be immediately observable");
    Expect(queue.PendingCancellation() == CaptureCommand::Stop,
        "Stop should be the pending cancellation before Quit");

    queue.Submit(CaptureCommand::Quit);
    Expect(queue.IsQuitRequested(), "Quit must be immediately observable");
    Expect(queue.PendingCancellation() == CaptureCommand::Quit,
        "Quit must take cancellation priority over Stop");
    queue.ClearStop();
    Expect(queue.PendingCancellation() == CaptureCommand::Quit,
        "ClearStop must not erase Quit");

    CaptureCommand command = CaptureCommand::Info;
    Expect(queue.TryPop(command) && command == CaptureCommand::Start,
        "commands should otherwise retain FIFO ordering");

    CommandQueue waiting;
    std::thread submitter([&waiting]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waiting.Submit(CaptureCommand::Info);
    });
    Expect(waiting.WaitPop(command, std::chrono::milliseconds(250)) && command == CaptureCommand::Info,
        "WaitPop should wake when a command arrives");
    submitter.join();
}

void TestPreviewFrameDeepCopyWithPadding() {
    constexpr int width = 4;
    constexpr int height = 3;
    constexpr std::size_t pitch = 8;
    std::vector<std::uint8_t> storage(pitch * height, 0xEE);
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            storage[static_cast<std::size_t>(row) * pitch + column] =
                static_cast<std::uint8_t>(row * 10 + column);
        }
    }
    const cv::Mat source(height, width, CV_8UC1, storage.data(), pitch);

    PreviewFrame frame;
    std::string error;
    Expect(PreviewFrame::CopyFrom(source, frame, error), "padded Mono8 source should deep-copy");
    Expect(frame.IsValid() && frame.Stride() == width && frame.ByteCount() == width * height,
        "preview copy should own a tightly packed frame");
    Expect(frame.Data()[static_cast<std::size_t>(2) * width + 3] == 23,
        "preview copy should preserve padded-row pixels");
    storage[0] = 99;
    Expect(frame.Data()[0] == 0, "preview frame must not alias source memory");

    cv::Mat bgr(2, 3, CV_8UC3, cv::Scalar(1, 2, 3));
    PreviewFrame bgrFrame;
    Expect(PreviewFrame::CopyFrom(bgr, bgrFrame, error) &&
            bgrFrame.Format() == PreviewPixelFormat::Bgr8 && bgrFrame.Stride() == 9,
        "BGR8 preview frame should be supported");
}

void TestPreviewMailboxBackpressureAndDisable() {
    PreviewMailbox mailbox(0.0);
    cv::Mat first(2, 2, CV_8UC1, cv::Scalar(11));
    cv::Mat second(2, 2, CV_8UC1, cv::Scalar(22));
    std::string error;
    Expect(mailbox.PublishCopy(first, error) == PreviewPublishResult::Published,
        "first preview frame should publish");
    Expect(mailbox.PublishCopy(second, error) == PreviewPublishResult::Published,
        "newer preview frame should replace the unread frame");

    PreviewFrame taken;
    Expect(mailbox.TryTake(taken) && taken.Data()[0] == 22,
        "mailbox should expose only the latest frame");
    Expect(!mailbox.TryTake(taken), "taking a mailbox frame should empty the slot");

    mailbox.SetEnabled(false);
    Expect(mailbox.PublishCopy(first, error) == PreviewPublishResult::Disabled,
        "disabled mailbox must reject frames");
    Expect(!mailbox.TakeLatest().has_value(), "disabled mailbox must discard retained frames");

    mailbox.SetEnabled(true);
    mailbox.SetMaximumFramesPerSecond(30.0);
    Expect(mailbox.PublishCopy(first, error) == PreviewPublishResult::Published,
        "re-enabled mailbox should accept a frame");
    Expect(mailbox.PublishCopy(second, error) == PreviewPublishResult::Throttled,
        "mailbox should enforce its configured frame limit");
}

void TestPreviewMailboxConcurrency() {
    PreviewMailbox mailbox(0.0);
    std::atomic<bool> start { false };
    std::atomic<int> published { 0 };
    std::vector<std::thread> producers;
    for (int producer = 0; producer < 4; ++producer) {
        producers.emplace_back([&mailbox, &start, &published, producer]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            cv::Mat image(3, 3, CV_8UC1, cv::Scalar(producer));
            std::string error;
            for (int count = 0; count < 100; ++count) {
                if (mailbox.PublishCopy(image, error) == PreviewPublishResult::Published) {
                    ++published;
                }
            }
        });
    }

    std::thread consumer([&mailbox, &start]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int count = 0; count < 400; ++count) {
            PreviewFrame frame;
            mailbox.TryTake(frame);
        }
    });

    start.store(true, std::memory_order_release);
    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();

    Expect(published == 400, "concurrent publishers should safely publish every unthrottled frame");
    const auto latest = mailbox.TakeLatest();
    Expect(!latest.has_value() || latest->IsValid(), "concurrent mailbox frame must be valid when present");
}

} // namespace

int main() {
    TestCommandParsing();
    TestCommandQueueCancellation();
    TestPreviewFrameDeepCopyWithPadding();
    TestPreviewMailboxBackpressureAndDisable();
    TestPreviewMailboxConcurrency();

    if (failures == 0) {
        std::cout << "All capture runtime tests passed." << std::endl;
        return 0;
    }
    std::cerr << failures << " capture runtime test(s) failed." << std::endl;
    return 1;
}
