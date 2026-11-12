#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace tta {

// Lobby join codes (plan req 9): 6 chars from A-Z without I and O, so they read
// cleanly aloud and on stream.
constexpr int kJoinCodeLength = 6;
constexpr const char* kJoinCodeAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ";
constexpr int kDefaultGamePort = 7777;

// Same seed always gives the same code.
std::string makeJoinCode(std::uint64_t seed);

// What a player typed -> canonical code. Ignores case, spaces and dashes.
// Returns nullopt when the result isn't a valid code.
std::optional<std::string> normalizeJoinCode(const std::string& typed);

// Direct-IP dev fallback: "host" or "host:port".
struct DirectAddress {
    std::string host;
    int port = kDefaultGamePort;
};

// Trims surrounding spaces. Returns nullopt for an empty host, inner spaces,
// or a port outside 1-65535.
std::optional<DirectAddress> parseDirectAddress(const std::string& typed);

}  // namespace tta
