#include "tta/CutPlayback.h"

#include <sstream>
#include <stdexcept>

namespace tta {

namespace {
constexpr const char* kHeader = "TTACUT 1";
}

std::optional<CutFrame> cutFrameAt(const EditList& cut, std::uint32_t cutMs) {
    std::uint32_t clipStart = 0;
    const auto& clips = cut.clips();
    for (std::size_t i = 0; i < clips.size(); ++i) {
        const Clip& clip = clips[i];
        const std::uint32_t length = clip.durationMs();
        if (cutMs < clipStart + length) {
            const double replayMs = clip.startMs + (cutMs - clipStart) * clip.speed;
            return CutFrame{static_cast<int>(i), clip.camera, replayMs / 1000.0, clip.zoom};
        }
        clipStart += length;
    }
    return std::nullopt;
}

std::string saveCut(const EditList& cut) {
    std::ostringstream out;
    out.precision(17);
    out << kHeader << '\n';
    for (const Clip& c : cut.clips()) {
        out << "clip " << c.camera << ' ' << c.startMs << ' ' << c.endMs << ' ' << c.speed << ' '
            << c.zoom << '\n';
    }
    for (const Overlay& o : cut.overlays()) {
        if (o.content.find_first_of("\r\n") != std::string::npos) {
            throw std::invalid_argument("overlay content may not contain line breaks");
        }
        out << "overlay " << static_cast<int>(o.type) << ' ' << o.atMs << ' ' << o.durationMs << ' '
            << o.content << '\n';
    }
    return out.str();
}

EditList loadCut(const std::string& text) {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || line != kHeader) {
        throw std::invalid_argument("not a cut file (missing '" + std::string(kHeader) + "')");
    }
    EditList cut;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        std::string kind;
        row >> kind;
        if (kind == "clip") {
            Clip c;
            if (!(row >> c.camera >> c.startMs >> c.endMs >> c.speed >> c.zoom)) {
                throw std::invalid_argument("bad clip line: " + line);
            }
            cut.addClip(c);
        } else if (kind == "overlay") {
            int type = 0;
            Overlay o;
            if (!(row >> type >> o.atMs >> o.durationMs) || type < 0 ||
                type > static_cast<int>(OverlayType::WastedCard)) {
                throw std::invalid_argument("bad overlay line: " + line);
            }
            o.type = static_cast<OverlayType>(type);
            if (row.peek() == ' ') row.get();
            std::getline(row, o.content);
            cut.addOverlay(o);
        } else {
            throw std::invalid_argument("unknown cut line: " + line);
        }
    }
    return cut;
}

} // namespace tta
