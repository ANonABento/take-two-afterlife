#include "doctest/doctest.h"
#include "tta/SpawnPlacement.h"

#include <utility>
#include <vector>

using namespace tta;

TEST_CASE("spawn goes 300 cm above the floor hit under the chosen start") {
    const SpawnPick p = pickSpawn({SpawnFloor{true, 120.0, 900.0}}, [](int, double) { return false; });
    CHECK(p.found);
    CHECK(p.start == 0);
    CHECK(p.z == doctest::Approx(420.0));
}

TEST_CASE("no floor hit falls back to the start's own height") {
    const SpawnPick p = pickSpawn({SpawnFloor{false, 0.0, 50.0}}, nullptr);
    CHECK(p.z == doctest::Approx(350.0));
}

TEST_CASE("blocked capsule tries the next offset, up to 400 cm more") {
    CHECK(spawnLifts() == std::vector<double>{300.0, 400.0, 500.0, 600.0, 700.0});
    std::vector<double> tried;
    const SpawnPick p = pickSpawn({SpawnFloor{true, 0.0, 0.0}}, [&](int, double z) {
        tried.push_back(z);
        return z < 550.0;
    });
    CHECK(p.found);
    CHECK(p.z == doctest::Approx(600.0));
    CHECK(tried == std::vector<double>{300.0, 400.0, 500.0, 600.0});
}

TEST_CASE("all offsets blocked moves on to the next start") {
    std::vector<std::pair<int, double>> tried;
    const SpawnPick p = pickSpawn({SpawnFloor{true, 0.0, 0.0}, SpawnFloor{true, 10.0, 0.0}}, [&](int i, double z) {
        tried.emplace_back(i, z);
        return i == 0;
    });
    CHECK(p.found);
    CHECK(p.start == 1);
    CHECK(p.z == doctest::Approx(310.0));
    CHECK(tried.size() == 6);  // five lifts at start 0, then the first at start 1
}

TEST_CASE("everything blocked finds nothing") {
    const SpawnPick p = pickSpawn({SpawnFloor{true, 0.0, 0.0}, SpawnFloor{}}, [](int, double) { return true; });
    CHECK_FALSE(p.found);
    CHECK(p.start == -1);
    CHECK_FALSE(pickSpawn({}, nullptr).found);
}
