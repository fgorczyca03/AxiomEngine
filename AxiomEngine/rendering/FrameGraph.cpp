#include "AxiomEngine/rendering/FrameGraph.h"

#include "AxiomEngine/profiling/Profiling.h"

namespace axiom::rendering {

void FrameGraphBuilder::Read(std::uint32_t resourceId) { reads_.insert(resourceId); }

void FrameGraphBuilder::Write(std::uint32_t resourceId) { writes_.insert(resourceId); }

const std::unordered_set<std::uint32_t>& FrameGraphBuilder::Reads() const { return reads_; }

const std::unordered_set<std::uint32_t>& FrameGraphBuilder::Writes() const { return writes_; }

void FrameGraph::Reset() {
    AXIOM_PROFILE_FUNCTION();
    passes_.clear();
    compiled_ = false;
}

FrameGraphPass& FrameGraph::AddPass(const std::string& name, std::function<void(FrameGraphBuilder&)> setup,
                                    std::function<void(RenderContext&)> execute) {
    AXIOM_PROFILE_FUNCTION();
    FrameGraphPass pass{};
    pass.name = name;
    pass.execute = std::move(execute);
    setup(pass.builder);
    passes_.push_back(std::move(pass));
    return passes_.back();
}

void FrameGraph::Compile() {
    AXIOM_PROFILE_FUNCTION();
    compiled_ = true;
}

void FrameGraph::Execute(RenderContext& context) {
    AXIOM_PROFILE_FUNCTION();
    if (!compiled_) {
        Compile();
    }

    for (FrameGraphPass& pass : passes_) {
        AXIOM_PROFILE_ZONE(pass.name.c_str());
        if (pass.execute) {
            pass.execute(context);
        }
    }
}

} // namespace axiom::rendering
