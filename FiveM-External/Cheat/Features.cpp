#include "Cheat.h"

const int ReadCount = 256;
Vector3 GetPrediction(CPed& target, CPed& local);

uintptr_t localplayer = offset::World + 0x8;

struct Entity {
    uint64_t address;
    uint64_t junk0;
};

struct EntityList_t {
    Entity entity[ReadCount];
};

inline Vector3 GetDirectionFromAngle(const Vector3& angles)
{
    float pitch = angles.x * (3.14159f / 180.0f);
    float yaw = angles.y * (3.14159f / 180.0f);

    float cosPitch = cosf(pitch);

    Vector3 direction;
    direction.x = -sinf(yaw) * cosPitch;
    direction.y = cosf(yaw) * cosPitch;
    direction.z = sinf(pitch);

    return direction;
}

void Cheat::UpdateList()
{
    while (g.process_active)
    {
        std::vector<CPed> list;
        local.address = m.Read<uintptr_t>(Game->GetWorld() + 0x8);

        // Read EntityList
        uintptr_t pEntityList = m.ReadChain(Game->GetReplayInterface(), { 0x18, 0x100 });
        EntityList_t entitylist = m.Read<EntityList_t>(pEntityList);

        for (int i = 0; i < 256; i++)
        {
            if (entitylist.entity[i].address == NULL)
                continue;

            CPed p = CPed();
            p.address = entitylist.entity[i].address;
            p.UpdateStatic();

            list.push_back(p);
        }

        this->EntityList = list;
        Sleep(200);
    }
}

void Cheat::Misc()
{
    // GodMode - keep health at max every frame
    if (g.GodMode && local.address)
    {
        local.UpdateStatic();  // Refresh max health value
        float maxHealth = m.Read<float>(local.address + offset::m_flHealthMax);
        m.Write<float>(local.address + offset::m_flHealth, maxHealth);
    }
}

void Cheat::ApplyGodMode()
{
    if (!local.address)
        return;

    m.Write<float>(local.address + offset::m_flHealth, 100.0f);
    m.Write<float>(local.address + offset::m_flArmor, 100.f);
}

void Cheat::RestoreHealth()
{
    if (!local.address)
        return;

    float maxHealth = m.Read<float>(local.address + offset::m_flHealthMax);
    m.Write<float>(local.address + offset::m_flHealth, maxHealth);
}

void Cheat::RestoreArmor()
{
    if (!local.address)
        return;

    m.Write<float>(local.address + offset::m_flArmor, 100.f);
}

void Cheat::AimBot(CPed target)
{
    if (IsKeyDown(g.AimKey1) && g.AimBot)
    {
        if (Vec3_Empty(target.GetBoneByID(HEAD)))
            return;

        uintptr_t camera = Game->GetCamera();
        Vector3 ViewAngle = m.Read<Vector3>(camera + 0x3D0); // and 0x40
        Vector3 CameraPos = m.Read<Vector3>(camera + 0x60);
        Vector3 predict = g.AimBotPrediction ? GetPrediction(target, local) : Vector3();
        Vector3 targetPos = target.GetBoneByID(HEAD) + predict;
        Vector3 Angle = CalcAngle(CameraPos, targetPos);
        Vector3 Delta = Angle - ViewAngle;
        Vector3 WriteAngle = ViewAngle + Delta;

        if (!Vec3_Empty(WriteAngle)) {
            m.Write<Vector3>(camera + 0x3D0, WriteAngle);
        }
    }
}

struct TargetHistory {
    Vector3 lastVelocity = Vector3();
    Vector3 lastPos = Vector3();
};

// At class level, maintain history per target
std::unordered_map<uint64_t, TargetHistory> targetHistory;

Vector3 GetPrediction(CPed& target, CPed& local)
{
    Vector3 velocity = target.GetVelocity();
    Vector3 targetPos = target.m_vecPosition;
    Vector3 localPos = local.m_vecPosition;

    if (Vec3_Empty(velocity))
        return Vector3();

    float distance = GetDistance(targetPos, localPos);
    float bulletSpeed = 1800.f;

    float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);

    if (speed < 0.1f)
        return Vector3();

    float travelTime = distance / bulletSpeed;

    // Distance-based scaling
    float distanceScale = 1.0f;
    if (distance > 30.f) {
        float ratio = 30.f / distance;
        distanceScale = ratio * ratio;
        distanceScale = fmaxf(distanceScale, 0.1f);
    }

    // === UNIVERSAL PREDICTION (works for foot + vehicle) ===
    travelTime = fminf(travelTime, 1.5f);

    auto& hist = targetHistory[target.address];
    Vector3 acceleration = (velocity - hist.lastVelocity) * 30.f;

    float accelMagnitude = sqrtf(acceleration.x * acceleration.x +
        acceleration.y * acceleration.y +
        acceleration.z * acceleration.z);

    Vector3 prediction = velocity * travelTime * distanceScale;

    if (accelMagnitude > 0.5f) {
        float accelFactor = fminf(accelMagnitude / 800.f, 1.0f);
        prediction = prediction + (acceleration * travelTime * travelTime * 0.5f * accelFactor * distanceScale);
    }

    hist.lastVelocity = velocity;

    prediction.z -= (9.81f * travelTime * travelTime * 0.5f) * 0.3f;

    // Strafe compensation
    Vector3 dirToTarget = (targetPos - localPos);
    float distToTarget = GetDistance(targetPos, localPos);
    if (distToTarget > 0.001f) {
        dirToTarget = dirToTarget * (1.f / distToTarget);
        float lateralVel = (velocity.x * -dirToTarget.y + velocity.y * dirToTarget.x);
        prediction.x -= dirToTarget.y * lateralVel * travelTime * 0.8f * distanceScale;
        prediction.y += dirToTarget.x * lateralVel * travelTime * 0.8f * distanceScale;
    }

    return prediction;
}