#include "doctest/doctest.h"
#include "tta/MatchSettings.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>

using tta::MatchSettings;
using tta::parseMatchSettings;

TEST_CASE("empty object keeps defaults") {
    const MatchSettings s = parseMatchSettings("{}");
    CHECK(s.rounds == 3);
    CHECK(s.editSeconds == 90);
    CHECK(s.ratingSeconds == 10);
}

TEST_CASE("fields load from JSON") {
    const MatchSettings s = parseMatchSettings(
        " {\n \"rounds\": 5, \"buildSeconds\": 60,\"runSeconds\" : 45, \"editSeconds\": 30, \"ratingSeconds\": 8 }\n");
    CHECK(s.rounds == 5);
    CHECK(s.buildSeconds == 60);
    CHECK(s.runSeconds == 45);
    CHECK(s.editSeconds == 30);
    CHECK(s.ratingSeconds == 8);
}

TEST_CASE("bad settings are rejected") {
    CHECK_THROWS_AS(parseMatchSettings(""), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"rounds\": 3"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"rounds\": 3,}"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"rounds\": 0}"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"rounds\": -2}"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"rounds\": 2.5}"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"rounds\": \"3\"}"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{\"lives\": 3}"), std::invalid_argument);
    CHECK_THROWS_AS(parseMatchSettings("{} x"), std::invalid_argument);
}

TEST_CASE("toJson round-trips") {
    MatchSettings s;
    s.rounds = 4;
    s.runSeconds = 75;
    const MatchSettings back = parseMatchSettings(tta::toJson(s));
    CHECK(back.rounds == 4);
    CHECK(back.runSeconds == 75);
    CHECK(back.buildSeconds == s.buildSeconds);
}

TEST_CASE("settings load from a file") {
    const char* path = "tta_settings_test.json";
    {
        std::ofstream out(path);
        out << "{\"rounds\": 2}";
    }
    CHECK(tta::loadMatchSettings(path).rounds == 2);
    std::remove(path);
    CHECK_THROWS_AS(tta::loadMatchSettings("does_not_exist.json"), std::runtime_error);
}
