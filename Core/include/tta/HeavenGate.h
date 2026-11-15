#pragma once

#include <set>

namespace tta {

// Heaven lobby gate. The host's START opens it (Play Solo: at once); a player who walks or
// jumps through the trigger is ready. Travel to the course once everyone is ready, or
// kTimeoutSeconds after the first player went through.
class HeavenGate {
public:
    static constexpr double kTimeoutSeconds = 20.0;

    void open() { open_ = true; }
    bool isOpen() const { return open_; }
    // Players in the lobby, bots included; negative counts as 0.
    void setPlayers(int players);
    int players() const { return players_; }
    // False (nothing changes) while the gate is closed or when the player is already ready.
    bool markReady(int playerId);
    int readyCount() const { return static_cast<int>(ready_.size()); }
    // Advances the timeout clock; it only runs after the first ready player.
    void tick(double seconds);
    bool shouldTravel() const;

private:
    bool open_ = false;
    int players_ = 0;
    std::set<int> ready_;
    double sinceFirstReady_ = -1.0;
};

}  // namespace tta
