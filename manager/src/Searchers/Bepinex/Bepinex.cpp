#include "../Searchers.hpp"
#include "../../TextureCache/TextureCache.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

extern std::string g_bepinex_scan_file;


std::vector<modinfo> Bepinex_Search()
{
    std::vector<modinfo> mods;
        
    if (g_bepinex_scan_file.empty())
        return mods;

    std::ifstream file(g_bepinex_scan_file);

    if (!file.is_open())
        return mods;

    while (true)
    {
        std::string path;
        std::string id;
        std::string name;
        std::string version;
        std::string enabled;
    
        if (!std::getline(file, path))
            break;
    
        if (!std::getline(file, id))
            break;
    
        if (!std::getline(file, name))
            break;
    
        if (!std::getline(file, version))
            break;
    
        if (!std::getline(file, enabled))
            break;
    
        modinfo mod;
    
        mod.path = path;
        mod.name = name.empty() ? path : name;
        mod.id = id;
        mod.version = version;
        mod.enabled = (enabled == "1");

        mod.desc = "A Bepinex mod";
        mod.game_version = "Unknown";

        
        std::string dir = std::filesystem::path(path).parent_path().string();
        
        std::string icon_path = FindFirstImageInDir(dir);

        if (!icon_path.empty())
        {
            mod.iconTexture = &TextureCache::GetTexture(icon_path);

            printf(
                "found icon for %s at %s\n",
                mod.name.c_str(),
                icon_path.c_str()
            );
        }
        else
        {
            mod.iconTexture = nullptr;

            printf(
                "no icon for %s\n",
                mod.name.c_str()
            );
        }

        mods.push_back(std::move(mod));
    }

    return mods;
}

REGISTER_SEARCHER(bepin, Bepinex_Search);