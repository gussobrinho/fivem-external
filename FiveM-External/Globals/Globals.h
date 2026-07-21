#pragma once
#include <string>
#include <Windows.h>
#include "../Framework/ImGui/imgui.h"

struct Globals
{
    // System(Base)
    bool process_active = false;
    bool ShowMenu = false;

    // GameData
    RECT GameRect{};
    POINT GamePoint{};

    // Aim
    bool AimBot = true;
    bool AimBotPrediction = true;
    float AimFov = 100.f;
    bool ShowFov = true;

    // Visual
    bool ESP = true;
    bool ESP_Box = true;
    bool ESP_Line = false;
    bool ESP_Distance = true;
    bool ESP_HealthBar = true;
    bool ESP_ArmorBar = true;
    bool ESP_Skeleton = true;
    float ESP_MaxDistance = 1000.f;

    bool ESP_BoxOutline = true;          
    bool ESP_SkeletonOutline = true;

    // Thickness values
    float ESP_LineThickness = 1.0f;
    float ESP_BoxWidthScale = 1.0f;
    float ESP_BoxLineThickness = 1.7f;
    float ESP_SkeletonThickness = 1.5f;

    float ESP_BoxOutlineThickness = 1.8f;   
    float ESP_SkeletonOutlineThickness = 1.5f; 

    bool ShowNPC = false;
    bool ShowSelf = false;


    float ESP_HealthBar_OffsetX = 0.f;
    float ESP_HealthBar_OffsetY = 0.f;
    float ESP_Armor_OffsetX = 0.f;
    float ESP_Armor_OffsetY = 0.f;

    bool ShowDead = false;


    // Player
    bool GodMode = false;
    bool SpeedBoost = false;
    bool NoFallDamage = false;
    bool MoveByView = false;
    float MoveSpeed = 2.0f;

    // Vehicle
    bool NoBreak = false;
    bool InfiniteFuel = false;
    float SpeedMult = 1.0f;

    // Weapon
    bool NoRecoil = false;
    bool NoSpread = false;
    bool InfiniteAmmo = false;

    // Vehicles
    bool TeleportVH = false;
    bool VehicleESP = false;
    bool DestroyAllVehicles = false;
    bool RepairAllVehicles = false;

    // System(Cheat)
    bool Crosshair = false;
    bool StreamProof = false;

    // misc
    bool restoreHealth = false;
    bool restoreArmor = false;

    // Key
    int MenuKey = VK_INSERT;
    int AimKey1 = VK_RBUTTON;
};

extern Globals g;
extern bool IsKeyDown(int VK);
extern const char* KeyNames[];