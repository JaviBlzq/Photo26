#pragma once

#include "types.hpp"
#include <utility>

namespace photo26::core {

template <typename T>
struct Result {
    std::optional<T> value;
    OperationError error;

    [[nodiscard]] bool isOk() const noexcept {
        return error.code == ErrorCode::None && value.has_value();
    }

    static Result<T> success(T val) {
        Result<T> r;
        r.value = std::move(val);
        r.error = {ErrorCode::None, {}, {}};
        return r;
    }

    static Result<T> failure(ErrorCode code, QString message, QString detail = {}) {
        Result<T> r;
        r.error = {code, std::move(message), std::move(detail)};
        return r;
    }
};

struct StatusResult {
    OperationError error;

    [[nodiscard]] bool isOk() const noexcept {
        return error.code == ErrorCode::None;
    }

    static StatusResult success() {
        return StatusResult{{ErrorCode::None, {}, {}}};
    }

    static StatusResult failure(ErrorCode code, QString message, QString detail = {}) {
        return StatusResult{{code, std::move(message), std::move(detail)}};
    }
};

} // namespace photo26::core
