#include "../Searchers.hpp"
#include "../../TextureCache/TextureCache.hpp"
#include "cJSON.h"
#include "raylib.h"
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

static std::string GetJsonString(cJSON* item, const std::string& fallback = "")
{
    if (item == nullptr || !cJSON_IsString(item) || item->valuestring == nullptr)
        return fallback;
    return item->valuestring;
}

static bool GetJsonBool(cJSON* item, bool fallback = false)
{
    if (item == nullptr || !cJSON_IsBool(item))
        return fallback;
    return cJSON_IsTrue(item);
}

static bool GambonanzaModHost_Enable(modinfo& mod, bool enabled)
{
    // Read the existing JSON.
    std::ifstream in(mod.path, std::ios::binary);
    if (!in.is_open())
        return false;

    std::string jsondata(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );

    in.close();

    // Parse it.
    cJSON* root = cJSON_Parse(jsondata.c_str());
    if (!root)
        return false;

    // Replace/add "enabled".
    cJSON_DeleteItemFromObject(root, "enabled");

    if (!cJSON_AddBoolToObject(root, "enabled", enabled))
    {
        cJSON_Delete(root);
        return false;
    }

    // Serialize while root is still alive.
    char* out = cJSON_Print(root);

    if (!out)
    {
        cJSON_Delete(root);
        return false;
    }

    // root is no longer needed.
    cJSON_Delete(root);

    // IMPORTANT:
    // Don't use std::ofstream here until we know `out` is valid.
    const size_t outLen = std::strlen(out);

    std::ofstream ofs(
        mod.path,
        std::ios::binary | std::ios::trunc
    );

    if (!ofs.is_open())
    {
        cJSON_free(out);
        return false;
    }

    ofs.write(out, static_cast<std::streamsize>(outLen));

    const bool success = ofs.good();

    ofs.close();

    cJSON_free(out);

    if (!success)
        return false;

    // Only update the UI state after the file was successfully written.
    mod.enabled = enabled;

    return true;
}

std::vector<modinfo> GambonanzaModHost_Search()
{
    std::vector<modinfo> mods;
// return mods;
    modinfo rootmod;
    rootmod.path = "BepInEx/plugins/gamboSDK/Gambonanza.ModHost.dll";
    rootmod.name = "Gambonanza ModHost";
    rootmod.id = "gambonanza.modhost";
    rootmod.claimIds.push_back(rootmod.id);
    rootmod.desc = "Modding framework for Gambonanza";

    if (!FileExists("Gambonanza.exe"))
        return mods;

    
    std::vector<std::string> moddirs;
    {
        FilePathList moddirs_fpl = LoadDirectoryFiles("Mods");
        for (int i = 0; i < moddirs_fpl.count; i++)
            moddirs.push_back(moddirs_fpl.paths[i]);
        UnloadDirectoryFiles(moddirs_fpl);
    }

    
    for (const std::string& moddir : moddirs)
    {
        if (!DirectoryExists(moddir.c_str()))
            continue;

        std::vector<std::string> modfiles;
        {
            FilePathList modfiles_fpl = LoadDirectoryFiles(moddir.c_str());
            for (int j = 0; j < modfiles_fpl.count; j++)
                modfiles.push_back(modfiles_fpl.paths[j]);
            UnloadDirectoryFiles(modfiles_fpl);
        }

        for (const std::string& modfile : modfiles)
        {
            if (!FileExists(modfile.c_str()))
                continue;

            if (strcmp(GetFileName(modfile.c_str()), "mod.json") == 0)
            {
                std::ifstream modjson(modfile, std::ios::binary);
                if (!modjson.is_open())
                    continue;

                std::string jsondata((std::istreambuf_iterator<char>(modjson)),
                                       std::istreambuf_iterator<char>());

                cJSON* root = cJSON_Parse(jsondata.c_str());
                if (!root) continue;
        // return mods;
                modinfo info;
                info.id = GetJsonString(cJSON_GetObjectItem(root, "id"));
                info.name = GetJsonString(cJSON_GetObjectItem(root, "name"));
                info.version = GetJsonString(cJSON_GetObjectItem(root, "version"));
                info.authorName = GetJsonString(cJSON_GetObjectItem(root, "author"));
                info.desc = GetJsonString(cJSON_GetObjectItem(root, "description"));
                info.game_version = GetJsonString(cJSON_GetObjectItem(root, "gamever"));
                info.enabled = GetJsonBool(cJSON_GetObjectItem(root, "enabled"));

                info.path = modfile;
                info.enableFunc = GambonanzaModHost_Enable;

                std::string icon_path = FindFirstImageInDir(moddir);

                if (!icon_path.empty())
                {
                    info.iconTexture = &TextureCache::GetTexture(icon_path);

                    printf(
                        "found icon for %s at %s\n",
                        info.name.c_str(),
                        icon_path.c_str()
                    );
                }
                else
                {
                    info.iconTexture = nullptr;

                    printf(
                        "no icon for %s\n",
                        info.name.c_str()
                    );
                }
                
                rootmod.childs.push_back(std::move(info));
                cJSON_Delete(root);
            }
        }
    }

    mods.push_back(std::move(rootmod));
    return mods;
}

REGISTER_SEARCHER(gambonanza, GambonanzaModHost_Search);