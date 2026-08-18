#pragma once

#include "raylib.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

struct modinfo;


using EnableFunc = bool(*)(modinfo&, bool);
bool ChangeModEnableDef(modinfo& mod, bool enabled);
struct modinfo {
    std::string name;
    std::string path;

    std::string id;
    std::string version;

    bool enabled = false;

    std::string authorName;
    std::string desc = "No Description";
    std::string game_version;

    const Texture2D* iconTexture = nullptr;

    std::vector<modinfo> childs;

    EnableFunc enableFunc = ChangeModEnableDef;

    std::vector<std::string> claimIds;
};

using Searcher = std::vector<modinfo>(*)();

#define REGISTER_SEARCHER(name, func) \
    __attribute__((section("modsearchers"), used)) \
    static Searcher name = func


#ifndef Bepinex_Searcher_Enabled
        #define Bepinex_Searcher_Enabled 1
#endif

#ifndef Gambonanza_Searcher_Enabled
        #define Gambonanza_Searcher_Enabled 1
#endif



extern "C" {
    extern Searcher __start_modsearchers[];
    extern Searcher __stop_modsearchers[];
}

inline bool IdsMatch(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    return true;
}

static std::vector<modinfo> RunAllSearchers()
{
    std::vector<modinfo> mods;

    for (Searcher* it = __start_modsearchers; it < __stop_modsearchers; ++it)
    {
        if (*it == nullptr) continue;
        std::vector<modinfo> found = (*it)();
        mods.insert(mods.end(), found.begin(), found.end());
    }

    // Ownership resolution
    std::vector<int> toRemove;

    for (size_t p = 0; p < mods.size(); ++p)
    {
        for (const std::string& claimId : mods[p].claimIds)
        {
            for (size_t c = 0; c < mods.size(); ++c)
            {
                if (c == p) continue;
                if (mods[c].id.empty()) continue;
                if (!IdsMatch(mods[c].id, claimId)) continue;

                mods[p].path = mods[c].path;
                mods[p].enabled = mods[c].enabled;
                if (mods[p].version.empty())
                    mods[p].version = mods[c].version;

                toRemove.push_back((int)c);
                break;
            }
        }
    }

    std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());
    for (int idx : toRemove)
        mods.erase(mods.begin() + idx);

    return mods;
}


inline std::string FindFirstImageInDir(const std::string& dir)
{
    static const std::vector<std::string> validExts =
        { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif" };

    std::error_code ec;
    if (dir.empty() || !std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
        return "";

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (ec || !entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                        [](unsigned char c) { return (char)std::tolower(c); });

        if (std::find(validExts.begin(), validExts.end(), ext) != validExts.end())
            return entry.path().string();
    }

    return "";
}