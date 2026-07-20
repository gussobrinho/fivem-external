#include "CPed.h"

bool CPed::IsPlayer()
{
    return player_info != NULL;
}

bool CPed::IsDead()
{
    return m_flHealth <= 0 || Vec3_Empty(m_vecPosition);
}

bool CPed::InVehicle()
{
    return false;
}

bool CPed::Update()
{
    m_flHealth = m.Read<float>(address + offset::m_flHealth);
    m_vecPosition = m.Read<Vector3>(address + offset::m_vecPosition);

    if (IsDead())
        return false;

    m_flArmor = m.Read<float>(address + offset::m_flArmor);
    m_bMatrix = m.Read<Matrix>(address + offset::m_bMatrix);

    return true;
}

void CPed::UpdateStatic()
{
    // pInfo
    player_info = m.Read<uintptr_t>(address + offset::m_pInfo);

    // Weapon
    uintptr_t weapon_mgr = m.Read<uintptr_t>(address + offset::m_pWeaponManager);
    current_weapon = m.Read<uintptr_t>(weapon_mgr + 0x20);

    // player static value
    m_flMaxHealth = m.Read<float>(address + offset::m_flHealthMax);
}

Vector3 CPed::GetVelocity()
{
    return m.Read<Vector3>(address + offset::m_vecVelocity);
}

Vector3 CPed::GetBoneByID(BoneID id)
{
    Vector3 pos = m.Read<Vector3>(address + offset::m_pBoneList + (id * 0x10));
    
    return Vec3_Transform(&pos, &m_bMatrix);
}

std::vector<Vector3> CPed::GetBoneList()
{
    std::vector<Vector3> list(10);
    AllBone AllBoneList = m.Read<AllBone>(address + offset::m_pBoneList);

    for (int b = 0; b < 10; b++) {
        list[b] = Vec3_Transform(&AllBoneList.bone[b].position, &m_bMatrix);
    }
    
    return list;
}