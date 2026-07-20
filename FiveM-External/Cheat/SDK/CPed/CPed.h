#pragma once
#include "../Game/GameSDK.h"
#include "../Game/offset.h"

class CPed
{
public:
    uintptr_t address;

    uintptr_t player_info;
    uintptr_t current_weapon;

    float m_flHealth;
    float m_flArmor;
    float m_flMaxHealth;
    Vector3 m_vecPosition;
    Matrix m_bMatrix; // BoneMatrix

    bool Update();
    void UpdateStatic();
    Vector3 GetVelocity();
    Vector3 GetBoneByID(BoneID id);
    std::vector<Vector3> GetBoneList();

    bool IsDead();
    bool IsPlayer();
    bool InVehicle();
};

//namespace CMath
//{
//	Vector2 WorldToScreen(Vector3 ScreenPos)
//	{
//		auto& io = ImGui::GetIO();
//		Vector2 tempVec2(0.0f, 0.0f);
//		reinterpret_cast<bool(__fastcall*)(Vector3*, float*, float*)>(offset::m_bMatrix)(&ScreenPos, &tempVec2.x, &tempVec2.y);
//		tempVec2.x *= io.DisplaySize.x;
//		tempVec2.y *= io.DisplaySize.y;
//		return tempVec2;
//	}
//
//	Vector3 GetBone(DWORD64 CPed, unsigned int bone) {
//		__m128 a1;
//		Offsets::g_bone_mask(CPed, &a1, bone);
//		return Vector3::FromM128(a1);
//	}
//	Vector2 GetBoneWorldToScreen(DWORD64 CPed, unsigned int bone)
//	{
//		return WorldToScreen(GetBone(CPed, bone));
//	}
//	float pythag(Vector2 src, Vector2 dst)
//	{
//		return sqrtf(pow(src.x - dst.x, 2) + pow(src.y - dst.y, 2));
//	}
//}