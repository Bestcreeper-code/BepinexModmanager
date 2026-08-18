#include "Searchers/Searchers.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "rlImGui.h"
#include <filesystem>
#include <string>
#include <vector>

constexpr ImVec4 ImRED   { 0.902f, 0.161f, 0.216f, 1.000f };
constexpr ImVec4 ImGREEN { 0.000f, 0.894f, 0.188f, 1.000f };
constexpr ImVec4 ImBLUE  { 0.000f, 0.475f, 0.945f, 1.000f };

typedef enum ManagerResult
{
    UseBepInEx = 0,
    SkipBepinex = 1,
    Abort = 2
} ManagerResult;

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

static float crt_warp = 0.0f;

std::string g_bepinex_scan_file;

std::vector<modinfo> mods;

static void ParseArguments(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--bepinex-scan-file" && i + 1 < argc)
        {
            g_bepinex_scan_file = argv[++i];
        }
    }
}

bool ChangeModEnableDef(modinfo& mod, bool enabled) {
    namespace fs = std::filesystem;

    try
    {
        fs::path currentPath(mod.path);
        fs::path newPath;

        if (enabled)
        {
            // .dll.off -> .dll
            std::string path = currentPath.string();

            if (path.size() >= 8 &&
                path.compare(
                    path.size() - 8,
                    8,
                    ".dll.off"
                ) == 0)
            {
                newPath = path.substr(
                    0,
                    path.size() - 4
                );
            }
            else
            {
                newPath = currentPath;
            }
        }
        else
        {
            // .dll to .dll.off
            std::string path = currentPath.string();

            if (path.size() >= 4 &&
                path.compare(
                    path.size() - 4,
                    4,
                    ".dll"
                ) == 0)
            {
                newPath = path + ".off";
            }
            else
            {
                newPath = currentPath;
            }
        }

        if (newPath != currentPath)
        {
            fs::rename(currentPath, newPath);

            mod.path = newPath.string();
        }

        mod.enabled = enabled;
    }
    catch (const std::exception& e)
    {
        printf(
            "Failed to toggle %s: %s\n",
            mod.name.c_str(),
            e.what()
        );

        enabled = mod.enabled;
        return false;
    }
    return true;
}

static void RenderModRow(modinfo& mod)
{
    bool enabled = mod.enabled;
    if (ImGui::Checkbox("", &enabled))
    {
        mod.enableFunc(mod, enabled);
    }
    ImGui::SameLine();

    // icon in the row
    if (mod.iconTexture) // ImTextureID, loaded elsewhere and cached per-mod
    {
        rlImGuiImageSize(mod.iconTexture, 16, 16);
        ImGui::SameLine();
    }

    ImGui::Text("%s", mod.name.c_str());
    if (!mod.authorName.empty())
    {
        ImGui::SameLine();
        ImGui::Text("by %s", mod.authorName.c_str());
    }
    ImGui::SameLine();
    ImGui::Text("(%s)", mod.version.c_str());
    ImGui::SameLine();
    ImGui::TextColored(mod.enabled ? ImGREEN : ImRED, "[%s]", mod.enabled ? "Enabled" : "Disabled");
    ImGui::SameLine();
    ImGui::Text("[HOVER FOR INFO]");

    
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltipEx(
            ImGuiTooltipFlags_None,
            ImGuiWindowFlags_AlwaysAutoResize
        );
        

        ImGui::PushTextWrapPos(-1.0f);
        if (mod.iconTexture)
        {
            rlImGuiImageSize(mod.iconTexture, 48, 48);
            ImGui::SameLine();
        }
        ImGui::BeginGroup();
        ImGui::Text("%s (%s)", mod.name.c_str(), mod.version.c_str());
        ImGui::Text("Supported game version: %s", mod.game_version.c_str());
        ImGui::Text("%s", mod.desc.c_str());
        
        ImGui::EndGroup();
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void RenderModTree(modinfo& mod)
{
    ImGui::PushID(&mod);

    if (mod.childs.empty())
    {
        RenderModRow(mod);
    }
    else
    {
        if (!mod.path.empty())
        {
            RenderModRow(mod);
        }

        if (mod.enabled)
        {
            ImGui::Indent(10);
            
            for (auto& child : mod.childs)
            {
                RenderModTree(child);
            }
            
            ImGui::Unindent(10);
        }
    }

    ImGui::PopID();
}

void RenderImgui() {
    rlImGuiBegin();
        if (ImGui::Begin("Mods", NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
            ImGui::Text("%s", g_bepinex_scan_file.c_str());
            if (ImGui::Button("Run Modded")) exit(ManagerResult::UseBepInEx);
            if (ImGui::Button("Run Vanilla")) exit(ManagerResult::SkipBepinex);

            for (auto& mod : mods)
            {
                RenderModTree(mod);
            }
        }
        if (ImGui::Button("ABORT!!!")) exit(ManagerResult::Abort);
        ImGui::End();
    rlImGuiEnd();
}

int main(int argc, char** argv)
{
    ParseArguments(argc, argv);

    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Manager");
    InitAudioDevice();
    
    rlImGuiSetup(true);
    
    mods = RunAllSearchers();

    // shader
    Shader crt_curve_shader = LoadShader(0, "GamboBootStrap/res/shaders/screen_shader.glsl");
    RenderTexture2D gameRenderTexture = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        //render start
        BeginDrawing();
        ClearBackground((Color){232, 207, 160, 255});

        // render scene to texture so it gets warped
        BeginTextureMode(gameRenderTexture);
        ClearBackground((Color){232, 207, 160, 255});

        //extra render perhaps
        RenderImgui();

        EndTextureMode();

        // config shader
        int warpLoc = GetShaderLocation(crt_curve_shader, "warpStrength");
        SetShaderValue(crt_curve_shader, warpLoc, &crt_warp, SHADER_UNIFORM_FLOAT);

        // draw full texture with crt shader
        BeginShaderMode(crt_curve_shader);
        DrawTexturePro(
            gameRenderTexture.texture,
            (Rectangle){0, 0, (float)WINDOW_WIDTH, -(float)WINDOW_HEIGHT},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0},
            0.0f,
            WHITE
        );
        EndShaderMode();

        EndDrawing();
    }

    UnloadShader(crt_curve_shader);
    UnloadRenderTexture(gameRenderTexture);

    rlImGuiShutdown();

    CloseWindow();

    return 0;
}