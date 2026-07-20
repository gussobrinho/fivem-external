#include "Cheat.h"

void Cheat::RenderInfo()
{
    std::string text = "FiveM | " + std::to_string((int)ImGui::GetIO().Framerate) + "FPS";
    String(ImVec2(8.f, 8.f), ImColor(1.f, 1.f, 1.f, 1.f), text.c_str());

    // Crosshair
    if (g.Crosshair)
    {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2((float)g.GameRect.right / 2.f, (float)g.GameRect.bottom / 2.f), 3, ImColor(0.f, 0.f, 0.f, 1.f));
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2((float)g.GameRect.right / 2.f, (float)g.GameRect.bottom / 2.f), 2, ImColor(1.f, 1.f, 1.f, 1.f));
    }

    // AimFov
    if (g.ShowFov)
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(g.GameRect.right / 2.f, g.GameRect.bottom / 2.f), g.AimFov, ImColor(1.f, 1.f, 1.f, 1.f));
}

void Cheat::RenderESP()
{
    if (!pLocal->Update())
        return;

    CPed target;
    float MinFov = FLT_MAX;
    Vector2 Center = Vector2(g.GameRect.right / 2.f, g.GameRect.bottom / 2.f);
    Matrix ViewMatrix = m.Read<Matrix>(Game->GetViewPort() + 0x24C);

    for (auto& ped : EntityList)
    {
        CPed* pEntity = &ped;

        if (!pEntity->Update())
            continue;

        float pDistance = GetDistance(pEntity->m_vecPosition, pLocal->m_vecPosition);

        if (pDistance >= g.ESP_MaxDistance)
            continue;

        if (!g.ShowNPC && !pEntity->IsPlayer())
            continue;

        if (!g.ShowDead && pEntity->IsDead())
            continue;

        std::vector<Vector3> BoneList = pEntity->GetBoneList();

        if (BoneList.size() != 10)
            continue;

        // WorldToScreen
        Vector2 pBase{}, pHead{}, pNeck{}, pLeftFoot{}, pRightFoot{};
        if (!WorldToScreen(ViewMatrix, pEntity->m_vecPosition, pBase) ||
            !WorldToScreen(ViewMatrix, BoneList[HEAD], pHead) ||
            !WorldToScreen(ViewMatrix, BoneList[NECK], pNeck) ||
            !WorldToScreen(ViewMatrix, BoneList[LEFTFOOT], pLeftFoot) ||
            !WorldToScreen(ViewMatrix, BoneList[RIGHTFOOT], pRightFoot))
            continue;

        float HeadToNeck = pNeck.y - pHead.y;
        float pTop = pHead.y - (HeadToNeck * 2.5f);
        float pBottom = (pLeftFoot.y > pRightFoot.y ? pLeftFoot.y : pRightFoot.y) * 1.001f;
        float pHeight = pBottom - pTop;
        float pWidth = (pHeight / 2.5f) * g.ESP_BoxWidthScale;
        ImColor color = pEntity->IsPlayer() ? ESP_Player : ESP_NPC;

        // Line
        if (g.ESP_Line)
            DrawLine(ImVec2(g.GameRect.right / 2.f, g.GameRect.bottom), ImVec2(pBase.x, pBottom), color, g.ESP_LineThickness);

        // Box
        if (g.ESP_Box)
        {
            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(pBase.x - pWidth, pTop),
                ImVec2(pBase.x + pWidth, pBottom),
                ImColor(0.f, 0.f, 0.f, 0.3f)
            );

            float thick = g.ESP_BoxLineThickness;

            // Outline (drawn slightly larger, in black, before the colored box)
            if (g.ESP_BoxOutline)
            {
                float ot = g.ESP_BoxOutlineThickness;
                ImColor outline(0.f, 0.f, 0.f, 1.f);
                DrawLine(ImVec2(pBase.x - pWidth, pTop), ImVec2(pBase.x + pWidth, pTop), outline, thick + ot);
                DrawLine(ImVec2(pBase.x - pWidth, pTop), ImVec2(pBase.x - pWidth, pBottom), outline, thick + ot);
                DrawLine(ImVec2(pBase.x + pWidth, pTop), ImVec2(pBase.x + pWidth, pBottom), outline, thick + ot);
                DrawLine(ImVec2(pBase.x - pWidth, pBottom), ImVec2(pBase.x + pWidth, pBottom), outline, thick + ot);
            }

            DrawLine(ImVec2(pBase.x - pWidth, pTop), ImVec2(pBase.x + pWidth, pTop), color, thick);
            DrawLine(ImVec2(pBase.x - pWidth, pTop), ImVec2(pBase.x - pWidth, pBottom), color, thick);
            DrawLine(ImVec2(pBase.x + pWidth, pTop), ImVec2(pBase.x + pWidth, pBottom), color, thick);
            DrawLine(ImVec2(pBase.x - pWidth, pBottom), ImVec2(pBase.x + pWidth, pBottom), color, thick);
        }

        // Skeleton
        if (g.ESP_Skeleton)
        {
            Vector3 skeleton_pairs[6][2] = {
                { BoneList[NECK],  BoneList[HIP]       },
                { BoneList[NECK],  BoneList[LEFTHAND]  },
                { BoneList[NECK],  BoneList[RIGHTHAND] },
                { BoneList[HIP],   BoneList[LEFTFOOT]  },
                { BoneList[HIP],   BoneList[RIGHTFOOT] }
            };

            for (int j = 0; j < 6; j++)
            {
                Vector2 ScreenB1{}, ScreenB2{};

                if (Vec3_Empty(skeleton_pairs[j][0]) || Vec3_Empty(skeleton_pairs[j][1]))
                    continue;

                if (!WorldToScreen(ViewMatrix, skeleton_pairs[j][0], ScreenB1) ||
                    !WorldToScreen(ViewMatrix, skeleton_pairs[j][1], ScreenB2))
                    continue;

                // Outline pass
                if (g.ESP_SkeletonOutline)
                {
                    float ot = g.ESP_SkeletonOutlineThickness;
                    DrawLine(ImVec2(ScreenB1.x, ScreenB1.y), ImVec2(ScreenB2.x, ScreenB2.y),
                        ImColor(0.f, 0.f, 0.f, 1.f), g.ESP_SkeletonThickness + ot);
                }

                DrawLine(ImVec2(ScreenB1.x, ScreenB1.y), ImVec2(ScreenB2.x, ScreenB2.y),
                    ESP_Skeleton, g.ESP_SkeletonThickness);
            }
        }

        // Healthbar & Armor Bar (both on right side)
        if (g.ESP_HealthBar)
        {
            float barWidth = 3.f;
            float barSpacing = 2.f;
            float rightX = (pBase.x + pWidth) + 4.f;

            // Health bar (closer to box)
            HealthBar(rightX, pBottom, barWidth, -pHeight, (int)pEntity->m_flHealth, (int)pEntity->m_flMaxHealth);
        }

        if (g.ESP_ArmorBar)
        {
            float barWidth = 3.f;
            float barSpacing = 2.f;
            float rightX = (pBase.x + pWidth) + 4.f;


            if (pEntity->m_flArmor > 0.f)
                ArmorBar(rightX + barWidth + barSpacing, pBottom, barWidth, -pHeight, (int)pEntity->m_flArmor, 100);
        }

        // Distance
        if (g.ESP_Distance)
        {
            std::string dist = std::to_string((int)pDistance) + "m";
            String(ImVec2(pBase.x - ImGui::CalcTextSize(dist.c_str()).x / 2.f, pBottom), ImColor(1.f, 1.f, 1.f, 1.f), dist.c_str());
        }

        // Aimbot targeting
        float FOV = abs((Center - pBase).Length());

        if (FOV < g.AimFov)
        {
            if (FOV < MinFov)
            {
                target = ped;
                MinFov = FOV;
            }
        }
    }

    // Execute aimbot on closest target
    if (target.address != NULL)
    {
        AimBot(target);
    }
}