#include "TextureCache.hpp"
#include "raylib.h"
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace TextureCache{

std::map<std::string, Texture2D> textureCache;

const Texture& GetTexture(const std::string& path)
{
    auto it = textureCache.find(path);

    if (it != textureCache.end())
        return it->second;

    Texture2D tex = LoadTexture(path.c_str());
    auto [inserted, ok] = textureCache.emplace(path, tex);

    return inserted->second;
}








}