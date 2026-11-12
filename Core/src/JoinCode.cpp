#include "tta/JoinCode.h"

#include <cctype>
#include <cstring>

namespace tta {

namespace {

std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(' ') - first + 1);
}

}  // namespace

std::string makeJoinCode(std::uint64_t seed) {
    const std::size_t alphabetSize = std::strlen(kJoinCodeAlphabet);
    std::string code;
    for (int i = 0; i < kJoinCodeLength; ++i) {
        code += kJoinCodeAlphabet[splitmix64(seed) % alphabetSize];
    }
    return code;
}

std::optional<std::string> normalizeJoinCode(const std::string& typed) {
    std::string code;
    for (const char c : typed) {
        if (c == ' ' || c == '-') continue;
        const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (upper == '\0' || std::strchr(kJoinCodeAlphabet, upper) == nullptr) return std::nullopt;
        code += upper;
    }
    if (code.size() != static_cast<std::size_t>(kJoinCodeLength)) return std::nullopt;
    return code;
}

std::optional<DirectAddress> parseDirectAddress(const std::string& typed) {
    const std::string text = trim(typed);
    if (text.empty() || text.find(' ') != std::string::npos) return std::nullopt;
    DirectAddress address;
    const auto colon = text.rfind(':');
    if (colon == std::string::npos) {
        address.host = text;
        return address;
    }
    address.host = text.substr(0, colon);
    const std::string port = text.substr(colon + 1);
    if (address.host.empty() || port.empty() || port.size() > 5) return std::nullopt;
    for (const char c : port) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
    }
    address.port = std::stoi(port);
    if (address.port < 1 || address.port > 65535) return std::nullopt;
    return address;
}

}  // namespace tta
