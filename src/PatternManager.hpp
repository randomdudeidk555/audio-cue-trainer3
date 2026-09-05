#pragma once
#include <string>
#include <vector>

class GJGameLevel; // fwd decl only - keeps this header light

namespace act {

struct LoadedPattern {
    std::string sourcePath;                 // which file was actually used
    std::vector<double> timestampsSeconds;   // sorted ascending
    int rejectedLineCount = 0;               // invalid lines that were skipped
};

// ---------------------------------------------------------------------------
// PatternManager
//
// Reads plain-text click-timestamp files: one decimal number of seconds
// per line, '#' for comments, blank lines ignored, invalid lines skipped
// (and counted), auto-sorted ascending regardless of input order.
//
// Layout on disk (under the mod's save directory):
//   clicks.txt                  <- generic/default pattern (always created)
//   patterns/id_<levelID>.txt   <- per-level pattern for uploaded levels
//   patterns/name_<slug>.txt    <- per-level pattern for local/editor levels
//
// A per-level file, if present, always takes priority over clicks.txt.
// ---------------------------------------------------------------------------
class PatternManager {
public:
    static PatternManager& get();

    LoadedPattern loadForLevel(const std::string& levelKey);

    // Creates the patterns/ folder and a starter clicks.txt (with the
    // exact example from the spec) the first time the mod runs.
    void ensureExampleFilesExist();

private:
    PatternManager() = default;
    std::vector<double> parseFile(const std::string& path, int& rejectedCount);
};

// Builds a filesystem-safe key identifying a level, so different levels
// can have separate saved patterns. Free function (not a PatternManager
// method) so callers that only need a key don't need to touch the rest
// of this header's dependencies.
std::string makeLevelKey(GJGameLevel* level);

} // namespace act
