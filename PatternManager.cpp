#include "PatternManager.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

using namespace geode::prelude;

namespace act {

std::string makeLevelKey(GJGameLevel* level) {
    if (!level) return "unknown_level";

    // COMPATIBILITY NOTE: `m_levelID` on GJGameLevel is exposed as a
    // Geode Setter/value-wrapped field in current bindings, hence
    // `.value()`. If your SDK version exposes it as a plain int instead,
    // drop the `.value()` call.
    const int id = level->m_levelID.value();
    if (id > 0) {
        // Uploaded/official levels: numeric ID is stable across attempts
        // and re-launches, so key directly on it.
        return "id_" + std::to_string(id);
    }

    // Local/editor/created levels frequently have ID 0 or a placeholder
    // negative ID, so fall back to a sanitized level name instead.
    const std::string name = level->m_levelName;
    std::string sanitized;
    sanitized.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            sanitized += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (c == ' ' || c == '-' || c == '_') {
            sanitized += '_';
        }
        // anything else (emoji, punctuation, etc.) is simply dropped
    }
    if (sanitized.empty()) sanitized = "unnamed";
    return "name_" + sanitized;
}

PatternManager& PatternManager::get() {
    static PatternManager instance;
    return instance;
}

void PatternManager::ensureExampleFilesExist() {
    const auto saveDir = Mod::get()->getSaveDir();
    const auto patternsDir = saveDir / "patterns";

    std::error_code ec;
    std::filesystem::create_directories(patternsDir, ec);

    const auto defaultClicks = saveDir / "clicks.txt";
    if (!std::filesystem::exists(defaultClicks)) {
        std::ofstream out(defaultClicks);
        out <<
            "# Audio Cue Trainer - default pattern\n"
            "# One click timestamp (seconds, decimal) per line.\n"
            "# Lines starting with # are ignored, blank lines are ignored,\n"
            "# and any line that isn't a valid non-negative number is\n"
            "# skipped. Timestamps are sorted automatically, so order in\n"
            "# this file does not matter.\n"
            "#\n"
            "# This file is the fallback pattern used for any level that\n"
            "# doesn't have its own file in the patterns/ folder next to\n"
            "# this one (see patterns/id_<levelID>.txt or\n"
            "# patterns/name_<levelname>.txt).\n"
            "1.235\n"
            "2.104\n"
            "2.877\n"
            "3.512\n";
    }
}

std::vector<double> PatternManager::parseFile(const std::string& path, int& rejectedCount) {
    std::vector<double> result;
    rejectedCount = 0;

    std::ifstream in(path);
    if (!in.is_open()) return result;

    std::string line;
    while (std::getline(in, line)) {
        const size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue; // blank line
        const size_t end = line.find_last_not_of(" \t\r\n");
        const std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.empty() || trimmed[0] == '#') continue;

        try {
            size_t consumed = 0;
            const double value = std::stod(trimmed, &consumed);

            // Reject trailing garbage ("1.23abc") and negative timestamps -
            // both count as "invalid entries" per the spec and are simply
            // skipped rather than aborting the whole file.
            if (consumed != trimmed.size() || value < 0.0) {
                rejectedCount++;
                continue;
            }
            result.push_back(value);
        } catch (...) {
            rejectedCount++;
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

LoadedPattern PatternManager::loadForLevel(const std::string& levelKey) {
    ensureExampleFilesExist();

    LoadedPattern out;
    const auto perLevelPath = Mod::get()->getSaveDir() / "patterns" / (levelKey + ".txt");
    const auto fallbackPath = Mod::get()->getSaveDir() / "clicks.txt";

    std::string chosenPath;
    if (std::filesystem::exists(perLevelPath)) {
        chosenPath = perLevelPath.string();
    } else if (std::filesystem::exists(fallbackPath)) {
        chosenPath = fallbackPath.string();
    } else {
        out.sourcePath = "(none found)";
        return out;
    }

    int rejected = 0;
    out.timestampsSeconds = parseFile(chosenPath, rejected);
    out.rejectedLineCount = rejected;
    out.sourcePath = chosenPath;
    return out;
}

} // namespace act
