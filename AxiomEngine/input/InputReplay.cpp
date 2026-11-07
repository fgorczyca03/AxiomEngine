#include "AxiomEngine/input/InputReplay.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <system_error>
#include <utility>

namespace axiom::input {

namespace {
constexpr const char* ReplayHeader = "AXIOM_INPUT_REPLAY_V1";
constexpr std::size_t MaxActionsPerFrame = 4096;
} // namespace

bool InputReplay::StartRecording(const std::string& path) {
    StopStreams();
    lastError_.clear();
    frameIndex_ = 0;
    path_ = path;

    const std::filesystem::path replayPath(path);
    std::error_code error;
    if (!replayPath.parent_path().empty()) {
        std::filesystem::create_directories(replayPath.parent_path(), error);
        if (error) {
            SetError("Could not create replay directory: " + error.message());
            return false;
        }
    }

    output_.open(replayPath, std::ios::out | std::ios::trunc);
    if (!output_.is_open()) {
        SetError("Could not open replay file for recording: " + path_);
        return false;
    }
    output_.imbue(std::locale::classic());
    output_ << ReplayHeader << '\n';
    if (!output_.good()) {
        SetError("Could not write replay header: " + path_);
        return false;
    }
    mode_ = InputReplayMode::Recording;
    return true;
}

bool InputReplay::StopRecording() {
    if (mode_ != InputReplayMode::Recording) {
        return false;
    }
    output_.flush();
    const bool succeeded = output_.good();
    output_.close();
    mode_ = InputReplayMode::Idle;
    if (!succeeded) {
        lastError_ = "Could not finish writing replay file: " + path_;
    }
    return succeeded;
}

bool InputReplay::StartPlayback(const std::string& path) {
    StopStreams();
    lastError_.clear();
    frameIndex_ = 0;
    path_ = path;

    input_.open(path_);
    if (!input_.is_open()) {
        SetError("Could not open replay file: " + path_);
        return false;
    }
    input_.imbue(std::locale::classic());

    std::string header;
    if (!std::getline(input_, header) || header != ReplayHeader) {
        SetError("Replay file has an unsupported or malformed header: " + path_);
        return false;
    }
    mode_ = InputReplayMode::Playback;
    return true;
}

void InputReplay::StopPlayback() {
    if (mode_ == InputReplayMode::Playback) {
        input_.close();
        mode_ = InputReplayMode::Idle;
    }
}

bool InputReplay::RecordFixedStep(const InputSystem& input) {
    if (mode_ != InputReplayMode::Recording) {
        return false;
    }

    input.CopyActionStates(actionStates_);
    if (actionStates_.size() > MaxActionsPerFrame) {
        SetError("Replay frame contains too many input actions.");
        return false;
    }

    output_ << "FRAME " << actionStates_.size() << '\n' << std::setprecision(9);
    for (const NamedInputAction& entry : actionStates_) {
        output_ << std::quoted(entry.name) << ' ' << static_cast<unsigned int>(entry.state.pressed) << ' '
                << static_cast<unsigned int>(entry.state.justPressed) << ' '
                << static_cast<unsigned int>(entry.state.justReleased) << ' ' << entry.state.value << '\n';
    }
    if (!output_.good()) {
        SetError("Could not write replay frame to: " + path_);
        return false;
    }
    ++frameIndex_;
    return true;
}

bool InputReplay::ApplyNextFixedStep(InputSystem& input) {
    if (mode_ != InputReplayMode::Playback) {
        return false;
    }

    std::string tag;
    if (!(input_ >> tag)) {
        if (!input_.eof()) {
            lastError_ = "Could not read replay frame from: " + path_;
        }
        return false;
    }

    std::size_t actionCount = 0;
    if (tag != "FRAME" || !(input_ >> actionCount) || actionCount > MaxActionsPerFrame) {
        lastError_ = "Malformed replay frame " + std::to_string(frameIndex_) + " in: " + path_;
        return false;
    }

    actionStates_.clear();
    actionStates_.reserve(std::max(actionStates_.capacity(), actionCount));
    std::string previousName;
    for (std::size_t i = 0; i < actionCount; ++i) {
        NamedInputAction entry{};
        unsigned int pressed = 0;
        unsigned int justPressed = 0;
        unsigned int justReleased = 0;
        if (!(input_ >> std::quoted(entry.name) >> pressed >> justPressed >> justReleased >> entry.state.value) ||
            entry.name.empty() || pressed > 1U || justPressed > 1U || justReleased > 1U ||
            !std::isfinite(entry.state.value) || entry.state.value < -1.0F || entry.state.value > 1.0F ||
            (justPressed != 0U && (pressed == 0U || justReleased != 0U)) ||
            (justReleased != 0U && pressed != 0U) ||
            ((std::abs(entry.state.value) > 0.0001F) != (pressed != 0U)) ||
            (!previousName.empty() && entry.name <= previousName)) {
            lastError_ = "Malformed action state in replay frame " + std::to_string(frameIndex_) + " in: " + path_;
            return false;
        }
        entry.state.pressed = pressed != 0U;
        entry.state.justPressed = justPressed != 0U;
        entry.state.justReleased = justReleased != 0U;
        previousName = entry.name;
        actionStates_.push_back(std::move(entry));
    }

    input.ApplyActionStates(actionStates_);
    ++frameIndex_;
    return true;
}

void InputReplay::StopStreams() {
    if (output_.is_open()) {
        output_.flush();
        output_.close();
    }
    if (input_.is_open()) {
        input_.close();
    }
    mode_ = InputReplayMode::Idle;
}

void InputReplay::SetError(std::string message) {
    lastError_ = std::move(message);
    StopStreams();
}

} // namespace axiom::input
