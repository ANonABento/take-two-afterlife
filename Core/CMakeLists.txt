cmake_minimum_required(VERSION 3.16)
project(TakeTwoCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tta_core
  src/MatchSettings.cpp
  src/MatchStateMachine.cpp
  src/Placement.cpp
  src/TrapTimeline.cpp
  src/Scoring.cpp
  src/Replay.cpp
)
target_include_directories(tta_core PUBLIC include)
if(MSVC)
  target_compile_options(tta_core PRIVATE /W4)
else()
  target_compile_options(tta_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_executable(tta_core_tests
  tests/main.cpp
  tests/MatchSettingsTests.cpp
  tests/MatchStateMachineTests.cpp
  tests/PlacementTests.cpp
  tests/TrapTimelineTests.cpp
  tests/ScoringTests.cpp
  tests/ReplayTests.cpp
)
target_include_directories(tta_core_tests PRIVATE third_party)
target_link_libraries(tta_core_tests PRIVATE tta_core)
add_test(NAME tta_core_tests COMMAND tta_core_tests)
