#pragma once

#include "AxiomEngine/input/InputSystem.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace axiom::input {

enum class InputReplayMode : unsigned char { Idle, Recording, Playback };

class InputReplay {
  public:
    bool StartRecording(const std::string& path);
    bool StopRecording();
    bool StartPlayback(const std::string& path);
    void StopPlayback();

    bool RecordFixedStep(const InputSystem& input);
    bool ApplyNextFixedStep(InputSystem& input);

    [[nodiscard]] InputReplayMode Mode() const { return mode_; }
    [[nodiscard]] std::uint64_t FrameIndex() const { return frameIndex_; }
    [[nodiscard]] const std::string& Path() const { return path_; }
    [[nodiscard]] const std::string& LastError() const { return lastError_; }

  private:
    void StopStreams();
    void SetError(std::string message);

    InputReplayMode mode_{InputReplayMode::Idle};
    std::ifstream input_{};
    std::ofstream output_{};
    std::vector<NamedInputAction> actionStates_{};
    std::string path_{};
    std::string lastError_{};
    std::uint64_t frameIndex_{0};
};

} // namespace axiom::input
