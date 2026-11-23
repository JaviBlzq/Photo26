#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <QString>

namespace photo26::core {

class CancellationToken {
public:
    CancellationToken()
        : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}

    void cancel() noexcept {
        if (cancelled_) {
            cancelled_->store(true, std::memory_order_relaxed);
        }
    }

    void reset() noexcept {
        if (cancelled_) {
            cancelled_->store(false, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled_ && cancelled_->load(std::memory_order_relaxed);
    }

private:
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

using ProgressCallback = std::function<void(int completed, int total, const QString& currentItem)>;

} // namespace photo26::core
