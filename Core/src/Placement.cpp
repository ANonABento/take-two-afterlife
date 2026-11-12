#include "tta/Placement.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <sstream>
#include <stdexcept>

namespace tta {

const std::vector<TrapSpec>& trapCatalogue() {
    static const std::vector<TrapSpec> kTraps = {
        {TrapType::Hammer, "hammer", 20, {{{"swingSpeed", 1, 5, 3}, {"direction", 0, 1, 0}}}},
        {TrapType::Cannon, "cannon", 25, {{{"fireRate", 1, 5, 3}, {"ballSize", 1, 3, 2}}}},
        {TrapType::Spinner, "spinner", 20, {{{"speed", 1, 5, 3}, {"armLength", 1, 3, 2}}}},
        {TrapType::PusherWall, "pusherWall", 15, {{{"force", 1, 5, 3}, {"cycleTime", 1, 5, 3}}}},
        {TrapType::Trapdoor, "trapdoor", 15, {{{"openDelay", 0, 5, 1}, {"openDuration", 1, 5, 2}}}},
        {TrapType::Banana, "banana", 5, {{{"slideDistance", 1, 5, 3}, {"count", 1, 3, 1}}}},
    };
    return kTraps;
}

const TrapSpec& trapSpec(TrapType type) {
    for (const TrapSpec& s : trapCatalogue()) if (s.type == type) return s;
    throw std::invalid_argument("unknown trap type");
}

namespace {
constexpr CameraType kCameraTypes[] = {CameraType::Fixed, CameraType::Pan, CameraType::Follow, CameraType::TrapCam};
}

const char* cameraName(CameraType type) {
    switch (type) {
    case CameraType::Fixed: return "fixed";
    case CameraType::Pan: return "pan";
    case CameraType::Follow: return "follow";
    case CameraType::TrapCam: return "trapCam";
    }
    throw std::invalid_argument("unknown camera type");
}

int snapToCell(double meters) { return static_cast<int>(std::lround(meters / kGridMeters)); }
double cellToMeters(int cell) { return cell * kGridMeters; }

int snapRotation(double degrees) {
    const long step = std::lround(degrees / 45.0) % kRotationSteps;
    return static_cast<int>(step < 0 ? step + kRotationSteps : step);
}

namespace {
bool inCorridor(int cellX, int cellY) {
    const double x = cellToMeters(cellX), y = cellToMeters(cellY);
    return x >= 0.0 && x <= kCorridorLengthMeters && std::fabs(y) <= kCorridorWidthMeters / 2.0;
}
bool settingOk(TrapType type, int index, int value) {
    const SettingSpec& s = trapSpec(type).settings.at(static_cast<size_t>(index));
    return value >= s.min && value <= s.max;
}
}

PlaceResult Layout::add(Placement p, double xMeters, double yMeters, double degrees, int* outId) {
    p.cellX = snapToCell(xMeters);
    p.cellY = snapToCell(yMeters);
    p.rotation = snapRotation(degrees);
    if (!inCorridor(p.cellX, p.cellY)) return PlaceResult::OutOfBounds;
    if (p.isCamera) {
        if (cameraCount(p.owner) >= kCameraCap) return PlaceResult::CameraCapReached;
    } else {
        if (trapSpec(p.trap).cost > remaining(p.owner)) return PlaceResult::OverBudget;
    }
    p.id = nextId_++;
    placements_.push_back(p);
    if (outId) *outId = p.id;
    return PlaceResult::Ok;
}

PlaceResult Layout::placeTrap(int owner, TrapType type, double x, double y, double deg, int* outId) {
    Placement p;
    p.owner = owner;
    p.trap = type;
    const TrapSpec& spec = trapSpec(type);
    p.settings = {spec.settings[0].defaultValue, spec.settings[1].defaultValue};
    return add(p, x, y, deg, outId);
}

PlaceResult Layout::placeCamera(int owner, CameraType type, double x, double y, double deg, int* outId) {
    Placement p;
    p.owner = owner;
    p.isCamera = true;
    p.camera = type;
    return add(p, x, y, deg, outId);
}

PlaceResult Layout::setTrapSetting(int id, int index, int value) {
    for (Placement& p : placements_) {
        if (p.id != id) continue;
        if (p.isCamera || index < 0 || index > 1 || !settingOk(p.trap, index, value)) return PlaceResult::BadSetting;
        p.settings[static_cast<size_t>(index)] = value;
        return PlaceResult::Ok;
    }
    throw std::invalid_argument("no placement with id " + std::to_string(id));
}

bool Layout::remove(int id) {
    const auto it = std::find_if(placements_.begin(), placements_.end(), [id](const Placement& p) { return p.id == id; });
    if (it == placements_.end()) return false;
    placements_.erase(it);
    return true;
}

int Layout::spent(int owner) const {
    int total = 0;
    for (const Placement& p : placements_)
        if (p.owner == owner && !p.isCamera) total += trapSpec(p.trap).cost;
    return total;
}

int Layout::cameraCount(int owner) const {
    return static_cast<int>(std::count_if(placements_.begin(), placements_.end(),
        [owner](const Placement& p) { return p.owner == owner && p.isCamera; }));
}

int Layout::cameraCount(int owner, CameraType type) const {
    return static_cast<int>(std::count_if(placements_.begin(), placements_.end(),
        [owner, type](const Placement& p) { return p.owner == owner && p.isCamera && p.camera == type; }));
}

void Layout::markRevealed(int id) {
    for (Placement& p : placements_)
        if (p.id == id) p.hidden = false;
}

void Layout::revealAll() {
    for (Placement& p : placements_) p.hidden = false;
}

std::vector<Placement> Layout::visibleTo(int viewer) const {
    std::vector<Placement> out;
    for (const Placement& p : placements_)
        if (p.owner == viewer || !p.hidden) out.push_back(p);
    return out;
}

std::string toJson(const Layout& layout) {
    std::ostringstream o;
    o << "{\"placements\": [";
    bool first = true;
    for (const Placement& p : layout.all()) {
        o << (first ? "\n" : ",\n") << "  {\"id\": " << p.id << ", \"owner\": " << p.owner << ", \"kind\": \""
          << (p.isCamera ? "camera" : "trap") << "\", \"type\": \""
          << (p.isCamera ? cameraName(p.camera) : trapSpec(p.trap).name) << "\", \"cellX\": " << p.cellX
          << ", \"cellY\": " << p.cellY << ", \"rotation\": " << p.rotation;
        if (!p.isCamera) o << ", \"setting1\": " << p.settings[0] << ", \"setting2\": " << p.settings[1];
        o << ", \"hidden\": " << (p.hidden ? "true" : "false") << "}";
        first = false;
    }
    o << (first ? "" : "\n") << "]}\n";
    return o.str();
}

namespace {

// Minimal parser for the layout format written by toJson: one object holding
// an array of flat objects whose values are integers, strings or booleans.
class LayoutParser {
public:
    explicit LayoutParser(const std::string& text) : text_(text) {}

    std::vector<std::map<std::string, std::string>> parse() {
        std::vector<std::map<std::string, std::string>> rows;
        expect('{');
        if (parseString() != "placements") fail("expected \"placements\"");
        expect(':');
        expect('[');
        if (peek() == ']') {
            ++pos_;
        } else {
            while (true) {
                rows.push_back(parseObject());
                if (peek() == ',') { ++pos_; continue; }
                expect(']');
                break;
            }
        }
        expect('}');
        skipSpace();
        if (pos_ != text_.size()) fail("trailing characters");
        return rows;
    }

private:
    [[noreturn]] void fail(const std::string& what) const {
        throw std::invalid_argument("layout JSON: " + what + " at offset " + std::to_string(pos_));
    }
    void skipSpace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    char peek() {
        skipSpace();
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }
    void expect(char c) {
        if (peek() != c) fail(std::string("expected '") + c + "'");
        ++pos_;
    }
    std::string parseString() {
        expect('"');
        std::string out;
        while (pos_ < text_.size() && text_[pos_] != '"') {
            if (text_[pos_] == '\\') fail("escapes are not supported");
            out += text_[pos_++];
        }
        if (pos_ >= text_.size()) fail("unterminated string");
        ++pos_;
        return out;
    }
    std::string parseScalar() {
        if (peek() == '"') return parseString();
        const size_t start = pos_;
        while (pos_ < text_.size() && (std::isalnum(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '-')) ++pos_;
        if (pos_ == start) fail("expected a value");
        return text_.substr(start, pos_ - start);
    }
    std::map<std::string, std::string> parseObject() {
        std::map<std::string, std::string> row;
        expect('{');
        while (true) {
            const std::string key = parseString();
            expect(':');
            if (!row.emplace(key, parseScalar()).second) fail("duplicate key " + key);
            if (peek() == ',') { ++pos_; continue; }
            expect('}');
            return row;
        }
    }

    const std::string& text_;
    size_t pos_ = 0;
};

const std::string& field(const std::map<std::string, std::string>& row, const char* key) {
    const auto it = row.find(key);
    if (it == row.end()) throw std::invalid_argument(std::string("layout JSON: missing field ") + key);
    return it->second;
}

int intField(const std::map<std::string, std::string>& row, const char* key) {
    const std::string& v = field(row, key);
    size_t used = 0;
    int n = 0;
    try { n = std::stoi(v, &used); } catch (const std::exception&) { used = 0; }
    if (used == 0 || used != v.size()) throw std::invalid_argument(std::string("layout JSON: ") + key + " is not an integer");
    return n;
}

} // namespace

Layout parseLayout(const std::string& json) {
    Layout layout;
    for (const auto& row : LayoutParser(json).parse()) {
        const std::string& kind = field(row, "kind");
        const std::string& type = field(row, "type");
        const double x = cellToMeters(intField(row, "cellX"));
        const double y = cellToMeters(intField(row, "cellY"));
        const double deg = intField(row, "rotation") * 45.0;
        const int owner = intField(row, "owner");
        const std::string& hidden = field(row, "hidden");
        if (hidden != "true" && hidden != "false") throw std::invalid_argument("layout JSON: hidden must be a boolean");
        PlaceResult r = PlaceResult::BadSetting;
        int id = 0;
        if (kind == "trap") {
            const TrapSpec* spec = nullptr;
            for (const TrapSpec& s : trapCatalogue()) if (type == s.name) spec = &s;
            if (!spec) throw std::invalid_argument("layout JSON: unknown trap " + type);
            r = layout.placeTrap(owner, spec->type, x, y, deg, &id);
            if (r == PlaceResult::Ok) r = layout.setTrapSetting(id, 0, intField(row, "setting1"));
            if (r == PlaceResult::Ok) r = layout.setTrapSetting(id, 1, intField(row, "setting2"));
        } else if (kind == "camera") {
            const CameraType* cam = nullptr;
            for (const CameraType& c : kCameraTypes) if (type == cameraName(c)) cam = &c;
            if (!cam) throw std::invalid_argument("layout JSON: unknown camera " + type);
            r = layout.placeCamera(owner, *cam, x, y, deg, &id);
        } else {
            throw std::invalid_argument("layout JSON: unknown kind " + kind);
        }
        if (r != PlaceResult::Ok) throw std::invalid_argument("layout JSON: placement breaks the rules: " + type);
        if (hidden == "false") layout.markRevealed(id);
    }
    return layout;
}

} // namespace tta
