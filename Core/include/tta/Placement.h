#pragma once

#include <array>
#include <string>
#include <vector>

namespace tta {

// Build-phase placement rules (plan req 2). Positions are stored as whole
// 0.5 m grid cells so snapping and JSON round-trips are exact.
constexpr double kGridMeters = 0.5;
constexpr int kRotationSteps = 8;            // 45 degree steps
constexpr double kCorridorLengthMeters = 60.0; // x in [0, 60]
constexpr double kCorridorWidthMeters = 8.0;   // y in [-4, 4]
constexpr int kBudget = 100;
constexpr int kCameraCap = 12;

enum class TrapType { Hammer, Cannon, Spinner, PusherWall, Trapdoor, Banana };
enum class CameraType { Fixed, Pan, Follow, TrapCam };

struct SettingSpec {
    const char* name;
    int min;
    int max;
    int defaultValue;
};

struct TrapSpec {
    TrapType type;
    const char* name;
    int cost;
    std::array<SettingSpec, 2> settings;
};

const std::vector<TrapSpec>& trapCatalogue();
const TrapSpec& trapSpec(TrapType type);
const char* cameraName(CameraType type);

int snapToCell(double meters);
double cellToMeters(int cell);
int snapRotation(double degrees); // nearest 45 degree step, 0..7

struct Placement {
    int id = 0;
    int owner = 0;
    bool isCamera = false;
    TrapType trap = TrapType::Hammer;     // when !isCamera
    CameraType camera = CameraType::Fixed; // when isCamera
    int cellX = 0;
    int cellY = 0;
    int rotation = 0;
    std::array<int, 2> settings{};        // traps only
    bool hidden = true;                   // hidden from other players until Run
};

enum class PlaceResult { Ok, OutOfBounds, OverBudget, CameraCapReached, BadSetting };

class Layout {
public:
    // Snaps the world position/rotation and places a trap with default settings.
    PlaceResult placeTrap(int owner, TrapType type, double xMeters, double yMeters, double degrees, int* outId = nullptr);
    PlaceResult placeCamera(int owner, CameraType type, double xMeters, double yMeters, double degrees, int* outId = nullptr);
    // Would place* succeed right now? Same snapping and rules, changes nothing (ghost preview).
    PlaceResult canPlace(int owner, bool isCamera, TrapType trap, double xMeters, double yMeters) const;
    PlaceResult setTrapSetting(int id, int index, int value);
    bool remove(int id);

    int spent(int owner) const;
    int remaining(int owner) const { return kBudget - spent(owner); }
    int cameraCount(int owner) const;
    int cameraCount(int owner, CameraType type) const;

    // Run starts: everything becomes visible to everyone.
    void revealAll();
    void markRevealed(int id);
    // What `viewer` may see: their own placements plus revealed ones.
    std::vector<Placement> visibleTo(int viewer) const;
    const std::vector<Placement>& all() const { return placements_; }

private:
    PlaceResult add(Placement p, double xMeters, double yMeters, double degrees, int* outId);
    std::vector<Placement> placements_;
    int nextId_ = 1;
};

std::string toJson(const Layout& layout);
// Throws std::invalid_argument on malformed JSON or placements that break the rules.
Layout parseLayout(const std::string& json);

} // namespace tta
