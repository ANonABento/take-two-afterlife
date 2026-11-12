#include "doctest/doctest.h"
#include "tta/JoinCode.h"

#include <cstring>
#include <set>

using namespace tta;

TEST_CASE("join codes are 6 chars from the alphabet and deterministic") {
    std::set<std::string> seen;
    for (std::uint64_t seed = 0; seed < 200; ++seed) {
        const std::string code = makeJoinCode(seed);
        CHECK(code.size() == 6);
        for (const char c : code) CHECK(std::strchr(kJoinCodeAlphabet, c) != nullptr);
        CHECK(code.find('I') == std::string::npos);
        CHECK(code.find('O') == std::string::npos);
        CHECK(makeJoinCode(seed) == code);
        seen.insert(code);
    }
    CHECK(seen.size() == 200);
}

TEST_CASE("typed codes are normalized") {
    CHECK(normalizeJoinCode("abc-def") == std::optional<std::string>("ABCDEF"));
    CHECK(normalizeJoinCode("  hjk mnp ") == std::optional<std::string>("HJKMNP"));
    CHECK(normalizeJoinCode("XYZ123") == std::nullopt);
    CHECK(normalizeJoinCode("ABCDE") == std::nullopt);
    CHECK(normalizeJoinCode("ABCDEFG") == std::nullopt);
    CHECK(normalizeJoinCode("ABCDEO") == std::nullopt);
    CHECK(normalizeJoinCode("ABCDEI") == std::nullopt);
    CHECK(normalizeJoinCode(makeJoinCode(42)) == std::optional<std::string>(makeJoinCode(42)));
}

TEST_CASE("direct addresses parse host and optional port") {
    auto a = parseDirectAddress(" 192.168.1.5 ");
    REQUIRE(a);
    CHECK(a->host == "192.168.1.5");
    CHECK(a->port == kDefaultGamePort);
    a = parseDirectAddress("kevin-pc:7778");
    REQUIRE(a);
    CHECK(a->host == "kevin-pc");
    CHECK(a->port == 7778);
    CHECK_FALSE(parseDirectAddress(""));
    CHECK_FALSE(parseDirectAddress("1.2.3.4:"));
    CHECK_FALSE(parseDirectAddress(":7777"));
    CHECK_FALSE(parseDirectAddress("1.2.3.4:0"));
    CHECK_FALSE(parseDirectAddress("1.2.3.4:70000"));
    CHECK_FALSE(parseDirectAddress("1.2.3.4:77a"));
    CHECK_FALSE(parseDirectAddress("1.2 .3.4"));
}
