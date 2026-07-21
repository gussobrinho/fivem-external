#include "Cheat.h"
#include "../Framework/Overlay/Overlay.h"
#include "../Framework/ImGui/imgui_internal.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include "fontawesomeicons.h"

static int   TabIndex = 0;
static int   PrevTabIndex = 0;
static bool  bMenuWasOpen = false;

static float s_menuAnim = 0.f;
static float s_tabAnim = 1.f;
static int   s_tabDir = 1;

static float s_tabHighlight[8] = { 0.f };  // one per tab
static int   s_prevTabIndex = -1;

namespace Col
{
    static ImVec4 Accent = { 0.35f, 0.70f, 1.0f,  1.f };
    static ImVec4 AccentHov = { 0.45f, 0.75f, 1.0f,  1.f };
    static ImVec4 AccentDim = { 0.35f, 0.70f, 1.0f,  0.22f };
    static ImVec4 AccentBorder = { 0.35f, 0.70f, 1.0f,  0.40f };
    static ImVec4 AccentText = { 1.0f,  1.0f,  1.0f,  1.f };
    static ImVec4 BgDeep = { 0.031f,0.031f,0.039f,1.f };
    static ImVec4 BgBase = { 0.047f,0.047f,0.055f,1.f };
    static ImVec4 BgRow = { 1.f,   1.f,   1.f,   0.04f };
    static ImVec4 BgRowHov = { 1.f,   1.f,   1.f,   0.07f };
    static ImVec4 Border = { 1.f,   1.f,   1.f,   0.08f };
    static ImVec4 TextPri = { 0.90f, 0.90f, 0.92f, 1.f };
    static ImVec4 TextSec = { 0.60f, 0.60f, 0.65f, 1.f };
    static ImVec4 TextMuted = { 0.28f, 0.28f, 0.33f, 1.f };
    static ImVec4 TextLabel = { 0.30f, 0.30f, 0.35f, 1.f };
    static ImVec4 Green = { 0.29f, 0.86f, 0.50f, 1.f };
    static ImVec4 Red = { 0.97f, 0.43f, 0.43f, 1.f };
    static ImVec4 RedDim = { 0.86f, 0.15f, 0.15f, 0.18f };
    static ImVec4 RedBorder = { 0.86f, 0.15f, 0.15f, 0.35f };
    static ImVec4 RedText = { 0.97f, 0.55f, 0.55f, 1.f };
}

static ImU32  U32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }
static ImVec4 Lerp4(const ImVec4& a, const ImVec4& b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
}
static float EaseOut(float t) { return 1.f - (1.f - t) * (1.f - t); }

// ─────────────────────────────────────────────
//  Particle system
// ─────────────────────────────────────────────
struct Particle { float x, y, vx, vy, r; };
static std::vector<Particle> s_particles;
static bool s_particlesInit = false;

static void InitParticles(float W, float H, int count = 75)
{
    s_particles.clear();
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = (float)(rand() % (int)W);
        p.y = (float)(rand() % (int)H);
        p.vx = ((rand() % 100) - 50) * 0.004f;
        p.vy = ((rand() % 100) - 50) * 0.004f;
        p.r = 1.5f + (rand() % 100) * 0.015f;
        s_particles.push_back(p);
    }
    s_particlesInit = true;
}

static void DrawParticles(ImDrawList* dl, ImVec2 origin, float W, float H, float dt, float alpha)
{
    if (!s_particlesInit) InitParticles(W, H);
    const float CD = 80.f, CD2 = CD * CD;
    for (auto& p : s_particles) {
        p.x += p.vx * dt * 60.f; p.y += p.vy * dt * 60.f;
        if (p.x < 0) p.x += W; if (p.y < 0) p.y += H;
        if (p.x > W) p.x -= W; if (p.y > H) p.y -= H;
    }
    for (int i = 0; i < (int)s_particles.size(); i++)
        for (int j = i + 1; j < (int)s_particles.size(); j++) {
            float dx = s_particles[i].x - s_particles[j].x;
            float dy = s_particles[i].y - s_particles[j].y;
            float d2 = dx * dx + dy * dy;
            if (d2 < CD2) {
                ImVec4 lc = Col::Accent;
                lc.w = (1.f - d2 / CD2) * 0.18f * alpha;
                dl->AddLine({ origin.x + s_particles[i].x, origin.y + s_particles[i].y },
                    { origin.x + s_particles[j].x, origin.y + s_particles[j].y },
                    U32(lc), 0.5f);
            }
        }
    for (auto& p : s_particles) {
        ImVec4 dc = Col::Accent; dc.w = 0.35f * alpha;
        dl->AddCircleFilled({ origin.x + p.x, origin.y + p.y }, p.r, U32(dc));
    }
}

// ─────────────────────────────────────────────
//  Animated Checkbox
// ─────────────────────────────────────────────
struct CheckboxAnimState { float progress = 0.f; bool lastState = false; };
static std::unordered_map<ImGuiID, CheckboxAnimState> animStates;

inline bool cb(const char* label, bool* value)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& ctx = *GImGui;
    const ImGuiStyle& style = ctx.Style;
    const ImGuiID id = window->GetID(label);

    const float  size = 19.f;
    const float  rounding = 3.5f;
    const ImVec2 pos = window->DC.CursorPos;

    const char* display_end = label;
    while (*display_end && !(display_end[0] == '#' && display_end[1] == '#'))
        display_end++;

    const ImVec2 label_size = ImGui::CalcTextSize(label, display_end, true);

    const ImRect total_bb(pos, ImVec2(pos.x + size + (label_size.x > 0 ? style.ItemInnerSpacing.x + label_size.x : 0), pos.y + size));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) { *value = !(*value); ImGui::MarkItemEdited(id); }

    CheckboxAnimState& anim = animStates[id];
    float target = *value ? 1.f : 0.f;
    anim.progress += (target - anim.progress) * ImClamp(ctx.IO.DeltaTime * 20.f, 0.f, 1.f);

    const ImRect box_bb(pos, ImVec2(pos.x + size, pos.y + size));
    window->DrawList->AddRectFilled(box_bb.Min, box_bb.Max,
        ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1.f)), rounding);

    if (anim.progress > 0.01f) {
        float p = anim.progress;
        float dp = (size * 0.5f) - (size * 0.5f * p);
        ImVec4 accent = style.Colors[ImGuiCol_CheckMark]; accent.w *= p;
        window->DrawList->AddRectFilled(
            ImVec2(box_bb.Min.x + dp, box_bb.Min.y + dp),
            ImVec2(box_bb.Max.x - dp, box_bb.Max.y - dp),
            ImGui::GetColorU32(accent), ImMax(0.f, rounding - 1.f));

        ImVec2 ca = ImVec2(box_bb.Min.x + size * 0.22f, box_bb.Min.y + size * 0.52f);
        ImVec2 cb_pt = ImVec2(box_bb.Min.x + size * 0.42f, box_bb.Min.y + size * 0.72f);
        ImVec2 cc = ImVec2(box_bb.Min.x + size * 0.78f, box_bb.Min.y + size * 0.28f);
        float  ck_thick = size * 0.13f;
        ImU32  ck_col = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, p));

        float t1 = ImClamp(p * 2.f, 0.f, 1.f);
        float t2 = ImClamp((p - 0.5f) * 2.f, 0.f, 1.f);
        if (t1 > 0.01f) {
            if (t2 > 0.01f) {
                ImVec2 pts[3] = { ca, cb_pt, ImLerp(cb_pt,cc,t2) };
                window->DrawList->AddPolyline(pts, 3, ck_col, 0, ck_thick);
            }
            else {
                ImVec2 pts[2] = { ca, ImLerp(ca,cb_pt,t1) };
                window->DrawList->AddPolyline(pts, 2, ck_col, 0, ck_thick);
            }
        }
    }

    if (display_end > label) {
        ImVec4 tc = ImLerp(style.Colors[ImGuiCol_TextDisabled], style.Colors[ImGuiCol_Text], anim.progress);
        window->DrawList->AddText(
            ImVec2(box_bb.Max.x + style.ItemInnerSpacing.x, pos.y + (size - label_size.y) * 0.5f),
            ImGui::GetColorU32(tc), label, display_end);
    }
    return pressed;
}

// ─────────────────────────────────────────────
//  ToggleRow
// ─────────────────────────────────────────────
static bool ToggleRow(const char* label, bool* v)
{
    const float rowH = 32.f, padX = 10.f, cbSize = 19.f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float  avail = ImGui::GetContentRegionAvail().x;

    bool hov = ImGui::IsMouseHoveringRect(pos, { pos.x + avail, pos.y + rowH });
    dl->AddRectFilled(pos, { pos.x + avail, pos.y + rowH }, U32(hov ? Col::BgRowHov : Col::BgRow), 5.f);
    dl->AddRect(pos, { pos.x + avail, pos.y + rowH }, U32(Col::Border), 5.f, 0, 0.5f);

    dl->AddText({ pos.x + padX, pos.y + (rowH - ImGui::GetFontSize()) * 0.5f }, U32(Col::TextSec), label);

    ImGui::SetCursorScreenPos({ pos.x + avail - cbSize - padX, pos.y + (rowH - cbSize) * 0.5f });
    char cid[128]; snprintf(cid, sizeof(cid), "##tr_%s", label);
    bool changed = cb(cid, v);

    ImGui::SetCursorScreenPos({ pos.x, pos.y + rowH + 4.f });
    ImGui::Dummy({ 0.f, 0.f });
    return changed;
}

// ─────────────────────────────────────────────
//  CustomSlider
// ─────────────────────────────────────────────
static bool CustomSlider(const char* label, float* v, float vmin, float vmax, const char* fmt = "%.0f")
{
    const float rowH = 28.f, trackH = 4.f, thumbR = 5.f, labelW = 110.f, valW = 46.f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float  avail = ImGui::GetContentRegionAvail().x;
    float  trackW = avail - labelW - valW - 8.f;
    if (trackW < 20.f) trackW = 20.f;
    float trackX0 = pos.x + labelW, trackCY = pos.y + rowH * 0.5f;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    const char* display_end = label;
    while (*display_end && !(display_end[0] == '#' && display_end[1] == '#'))
        display_end++;

    dl->AddText({ pos.x, pos.y + (rowH - ImGui::GetFontSize()) * 0.5f }, U32(Col::TextSec), label, display_end);

    ImGui::SetCursorScreenPos({ trackX0 - thumbR, pos.y });
    ImGui::InvisibleButton(label, { trackW + thumbR * 2.f, rowH });
    bool changed = false;
    if (ImGui::IsItemActive()) {
        float norm = ImClamp((ImGui::GetIO().MousePos.x - trackX0) / trackW, 0.f, 1.f);
        *v = vmin + norm * (vmax - vmin); changed = true;
    }
    float ty0 = trackCY - trackH * 0.5f;
    dl->AddRectFilled({ trackX0,ty0 }, { trackX0 + trackW,ty0 + trackH }, U32({ 0.13f,0.13f,0.16f,1.f }), trackH * 0.5f);
    float norm = ImClamp((*v - vmin) / (vmax - vmin), 0.f, 1.f);
    float fillX = trackX0 + norm * trackW;
    dl->AddRectFilled({ trackX0,ty0 }, { fillX,ty0 + trackH }, U32(Col::Accent), trackH * 0.5f);
    bool hov = ImGui::IsItemHovered() || ImGui::IsItemActive();
    dl->AddCircleFilled({ fillX,trackCY }, thumbR, U32(hov ? Col::AccentHov : Col::Accent));
    dl->AddCircleFilled({ fillX,trackCY }, thumbR * 0.4f, U32({ 1.f,1.f,1.f,0.9f }));
    char buf[32]; snprintf(buf, sizeof(buf), fmt, *v);
    dl->AddText({ trackX0 + trackW + 8.f, pos.y + (rowH - ImGui::GetFontSize()) * 0.5f }, U32(Col::AccentText), buf);
    ImGui::SetCursorScreenPos({ pos.x, pos.y + rowH + 4.f });
    ImGui::Dummy({ 0.f,0.f });
    return changed;
}

// ─────────────────────────────────────────────
//  SectionLabel
// ─────────────────────────────────────────────
static void SectionLabel(const char* text)
{
    ImGui::Spacing();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float  avail = ImGui::GetContentRegionAvail().x;
    dl->AddText(pos, U32(Col::TextLabel), text);
    float lineY = pos.y + ImGui::GetFontSize() + 4.f;
    dl->AddLine({ pos.x,lineY }, { pos.x + avail,lineY }, U32(Col::Border), 0.5f);
    ImGui::SetCursorScreenPos({ pos.x, lineY + 6.f });
    ImGui::Dummy({ 0.f,0.f });
    ImGui::Spacing();
}

static bool NavButton(const char* icon, const char* label, int myIdx, int* cur, float btnH, float btnW)
{
    bool active = (*cur == myIdx);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(label, { btnW, btnH });
    bool hov = ImGui::IsItemHovered();

    if (ImGui::IsItemClicked()) {
        if (*cur != myIdx) { s_tabDir = (myIdx > *cur) ? 1 : -1; s_tabAnim = 0.f; PrevTabIndex = *cur; }
        *cur = myIdx; active = true;
    }

    // background
    if (hov && !active)
        dl->AddRectFilled(pos, { pos.x + btnW, pos.y + btnH }, U32(Col::BgRowHov), 6.f);

    // left highlight bar – animated
    float highlight = s_tabHighlight[myIdx];


    // shift the icon/text slightly when active (but now we animate it too)
    float shiftX = active ? 6.f * highlight : 0.f;   // smooth shift
    ImVec4 col = Lerp4(Col::TextMuted, Col::AccentText, highlight);
    dl->AddText({ pos.x + 10.f + shiftX, pos.y + (btnH - ImGui::GetFontSize()) * 0.5f }, U32(col), icon);
    dl->AddText({ pos.x + 35.f + shiftX, pos.y + (btnH - ImGui::GetFontSize()) * 0.5f }, U32(col), label);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    return active;
}

static bool SmallButton(const char* label)
{
    ImGui::PushStyleColor(ImGuiCol_Button, U32(Col::AccentDim));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, U32(Col::AccentDim));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, U32(Col::AccentDim));
    ImGui::PushStyleColor(ImGuiCol_Text, U32(Col::AccentText));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 6.f,2.f });
    bool r = ImGui::Button(label);
    ImGui::PopStyleVar(2); ImGui::PopStyleColor(4);
    return r;
}

static bool ExitButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button, U32(Col::RedDim));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, U32(Col::RedDim));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, U32(Col::RedDim));
    ImGui::PushStyleColor(ImGuiCol_Border, U32(Col::RedBorder));
    ImGui::PushStyleColor(ImGuiCol_Text, U32(Col::RedText));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10.f,6.f });
    bool r = ImGui::Button("  Exit Cheat  ", { ImGui::GetContentRegionAvail().x, 32.f });
    ImGui::PopStyleVar(2); ImGui::PopStyleColor(5);
    return r;
}

// ─────────────────────────────────────────────
//  ESP Preview - Drag To Snap System
// ─────────────────────────────────────────────

enum EBarPosition { BAR_NONE = -1, BAR_ABOVE = 0, BAR_BELOW = 1, BAR_LEFT = 2, BAR_RIGHT = 3 };

struct ESPElem {
    ImVec2 offset = { 0.f, 0.f };
    bool inCanvas = true;
    float animIn = 1.f;
    float animHov = 0.f;
    EBarPosition snapPos = BAR_NONE;
    bool isDragging = false;
};

static std::unordered_map<std::string, ESPElem> s_esp;

static ESPElem& GetElem(const char* key) {
    return s_esp[key];
}

static ImVec2 GetSnapAnchor(EBarPosition pos, float cx, float boxLeft, float boxRight,
    float boxTop, float boxBot, float barW, float barH)
{
    if (pos == BAR_ABOVE)
        return { cx - barW * 0.5f, boxTop - barH - 8.f };
    if (pos == BAR_BELOW)
        return { cx - barW * 0.5f, boxBot + 8.f };
    if (pos == BAR_LEFT)
        return { boxLeft - barW - 8.f, boxTop + (boxBot - boxTop - barH) * 0.5f };
    if (pos == BAR_RIGHT)
        return { boxRight + 8.f, boxTop + (boxBot - boxTop - barH) * 0.5f };
    return { cx, boxTop };
}

static EBarPosition FindNearestSnapPos(ImVec2 currentPos, float cx, float boxLeft, float boxRight,
    float boxTop, float boxBot, float barW, float barH,
    EBarPosition occupiedPos)
{
    ImVec2 snapPositions[4] = {
        GetSnapAnchor(BAR_ABOVE, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH),
        GetSnapAnchor(BAR_BELOW, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH),
        GetSnapAnchor(BAR_LEFT, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH),
        GetSnapAnchor(BAR_RIGHT, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH)
    };

    float minDist = 999999.f;
    EBarPosition bestPos = BAR_ABOVE;

    for (int i = 0; i < 4; i++) {
        if ((EBarPosition)i == occupiedPos) continue;

        float dx = currentPos.x - snapPositions[i].x;
        float dy = currentPos.y - snapPositions[i].y;
        float dist = dx * dx + dy * dy;

        if (dist < minDist) {
            minDist = dist;
            bestPos = (EBarPosition)i;
        }
    }

    return bestPos;
}

static void DrawESPPreview(ImVec2 wPos, float W, float H)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float dt = ImGui::GetIO().DeltaTime;

    const float titleH = 28.f;
    const float pillH = 20.f;
    const float pillPad = 5.f;
    const float stripH = pillH + pillPad * 2.f;
    const float canvasX0 = wPos.x + 8.f;
    const float canvasY0 = wPos.y + titleH + 8.f;
    const float canvasX1 = wPos.x + W - 8.f;
    const float canvasY1 = wPos.y + H - stripH - 8.f;
    const float cW = canvasX1 - canvasX0;
    const float cH = canvasY1 - canvasY0;

    dl->AddRectFilled(wPos, { wPos.x + W, wPos.y + H }, U32(Col::BgDeep), 10.f);
    dl->AddRect(wPos, { wPos.x + W, wPos.y + H }, U32(Col::Border), 10.f, 0, 0.5f);

    {
        const char* t = "ESP Preview";
        ImVec2 ts = ImGui::CalcTextSize(t);
        dl->AddText({ wPos.x + (W - ts.x) * 0.5f, wPos.y + (titleH - ts.y) * 0.5f }, U32(Col::TextMuted), t);
        dl->AddLine({ wPos.x + 8.f, wPos.y + titleH }, { wPos.x + W - 8.f, wPos.y + titleH }, U32(Col::Border), 0.5f);
    }

    dl->AddRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, U32(Col::Border), 5.f, 0, 0.5f);

    {
        float sy0 = canvasY1 + 4.f, sy1 = wPos.y + H - 4.f;
        dl->AddRectFilled({ wPos.x + 4.f, sy0 }, { wPos.x + W - 4.f, sy1 }, U32({ 1.f,1.f,1.f,0.03f }), 6.f);
        dl->AddRect({ wPos.x + 4.f, sy0 }, { wPos.x + W - 4.f, sy1 }, U32(Col::Border), 6.f, 0, 0.5f);
    }

    float cx = canvasX0 + cW * 0.5f;
    float figScale = 0.55f;
    float figH = cH * 0.70f * figScale;
    float figW = figH * 0.35f;
    float figTop = canvasY0 + (cH - figH) * 0.45f;
    float figBot = figTop + figH;

    ImVec2 HEAD = { cx, figTop + figH * 0.05f };
    ImVec2 NECK = { cx, figTop + figH * 0.18f };
    ImVec2 HIP = { cx, figTop + figH * 0.54f };
    ImVec2 LHAND = { cx - figW * 0.85f, figTop + figH * 0.46f };
    ImVec2 RHAND = { cx + figW * 0.85f, figTop + figH * 0.46f };
    ImVec2 LFOOT = { cx - figW * 0.32f, figBot };
    ImVec2 RFOOT = { cx + figW * 0.32f, figBot };

    float pillSlotW = (W - 16.f) / 6.f;
    float pillY = canvasY1 + 4.f + pillPad;
    auto PillRect = [&](int slot) -> std::pair<ImVec2, ImVec2> {
        float x0 = wPos.x + 8.f + slot * pillSlotW + 2.f;
        float x1 = x0 + pillSlotW - 4.f;
        return { {x0, pillY}, {x1, pillY + pillH} };
        };

    // ── BOX (CENTER-LOCKED) ──
    {
        float boxW = figW * 2.f, boxH = figH;
        ImVec2 anchor = { cx - figW, figTop };

        if (g.ESP_Box) {
            dl->PushClipRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, true);
            dl->AddRectFilled(anchor, { anchor.x + boxW, anchor.y + boxH }, U32({ 1.f, 1.f, 1.f, 0.08f }), 2.f);

            if (g.ESP_BoxOutline) {
                float ot = g.ESP_BoxLineThickness + g.ESP_BoxOutlineThickness;
                dl->AddLine(anchor, { anchor.x + boxW, anchor.y }, U32({ 0, 0, 0, 255 }), ot);
                dl->AddLine(anchor, { anchor.x, anchor.y + boxH }, U32({ 0, 0, 0, 255 }), ot);
                dl->AddLine({ anchor.x + boxW, anchor.y }, { anchor.x + boxW, anchor.y + boxH }, U32({ 0, 0, 0, 255 }), ot);
                dl->AddLine({ anchor.x, anchor.y + boxH }, { anchor.x + boxW, anchor.y + boxH }, U32({ 0, 0, 0, 255 }), ot);
            }

            ImU32 yel = U32({ 1.f, 1.f, 0.f, 1.f });
            float bt = g.ESP_BoxLineThickness;
            dl->AddLine(anchor, { anchor.x + boxW, anchor.y }, yel, bt);
            dl->AddLine(anchor, { anchor.x, anchor.y + boxH }, yel, bt);
            dl->AddLine({ anchor.x + boxW, anchor.y }, { anchor.x + boxW, anchor.y + boxH }, yel, bt);
            dl->AddLine({ anchor.x, anchor.y + boxH }, { anchor.x + boxW, anchor.y + boxH }, yel, bt);

            dl->PopClipRect();
        }

        auto [pMin, pMax] = PillRect(0);
        bool hovPill = ImGui::IsMouseHoveringRect(pMin, pMax);
        ESPElem& pe = GetElem("Box");
        pe.animHov += ((hovPill ? 1.f : 0.f) - pe.animHov) * ImClamp(dt * 12.f, 0.f, 1.f);
        ImVec4 pillBg = Lerp4({ 0.08f,0.08f,0.10f,1.f }, { 0.25f,0.50f,0.90f,0.25f }, pe.animHov);
        dl->AddRectFilled(pMin, pMax, U32(pillBg), 4.f);
        dl->AddRect(pMin, pMax, U32(Lerp4(Col::Border, Col::Accent, pe.animHov)), 4.f, 0, 0.5f);
        ImVec2 ts = ImGui::CalcTextSize("Box");
        dl->AddText({ pMin.x + (pMax.x - pMin.x - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f }, U32(Lerp4(Col::TextMuted, Col::AccentText, pe.animHov)), "Box");
    }

    // ── SKELETON ──
    {
        float skelW = RHAND.x - LHAND.x, skelH = LFOOT.y - HEAD.y;
        ImVec2 anchor = { LHAND.x, HEAD.y };

        if (g.ESP_Skeleton) {
            dl->PushClipRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, true);

            ImVec2 pairs[6][2] = {
                {NECK, HIP}, {NECK, LHAND}, {NECK, RHAND},
                {HIP, LFOOT}, {HIP, RFOOT}, {HEAD, NECK}
            };
            float st = g.ESP_SkeletonThickness;
            if (g.ESP_SkeletonOutline) {
                float ot = st + g.ESP_SkeletonOutlineThickness;
                for (auto& p : pairs) dl->AddLine(p[0], p[1], U32({ 0, 0, 0, 255 }), ot);
            }
            for (auto& p : pairs) dl->AddLine(p[0], p[1], U32({ 0.f, 1.f, 0.f, 1.f }), st);

            dl->PopClipRect();
        }

        auto [pMin, pMax] = PillRect(1);
        bool hovPill = ImGui::IsMouseHoveringRect(pMin, pMax);
        ESPElem& pe = GetElem("Skeleton");
        pe.animHov += ((hovPill ? 1.f : 0.f) - pe.animHov) * ImClamp(dt * 12.f, 0.f, 1.f);
        ImVec4 pillBg = Lerp4({ 0.08f,0.08f,0.10f,1.f }, { 0.25f,0.50f,0.90f,0.25f }, pe.animHov);
        dl->AddRectFilled(pMin, pMax, U32(pillBg), 4.f);
        dl->AddRect(pMin, pMax, U32(Lerp4(Col::Border, Col::Accent, pe.animHov)), 4.f, 0, 0.5f);
        ImVec2 ts = ImGui::CalcTextSize("Skeleton");
        dl->AddText({ pMin.x + (pMax.x - pMin.x - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f }, U32(Lerp4(Col::TextMuted, Col::AccentText, pe.animHov)), "Skeleton");
    }

    // ── HEALTH BAR ──
    {
        ESPElem& e = GetElem("HP Bar");
        ESPElem& armorElem = GetElem("Armor");

        float barW = 4.f, barH = figH * 0.85f;
        float boxLeft = cx - figW;
        float boxRight = cx + figW;
        float boxTop = figTop;
        float boxBot = figBot;

        ImVec2 anchor = GetSnapAnchor(e.snapPos, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH);
        ImVec2 op = anchor;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // Do nothing - let user drag freely
        }

        if (!e.isDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            auto [pMin, pMax] = PillRect(2);
            bool overPill = ImGui::IsMouseHoveringRect(pMin, pMax);
            if (overPill && e.inCanvas) {
                e.isDragging = true;
            }
        }

        // Handle dragging
        bool hovBar = ImGui::IsMouseHoveringRect({ anchor.x + e.offset.x, anchor.y + e.offset.y },
            { anchor.x + e.offset.x + barW, anchor.y + e.offset.y + barH });
        if (hovBar && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && e.inCanvas) {
            e.isDragging = true;
            ImVec2 d = ImGui::GetIO().MouseDelta;
            e.offset.x += d.x;
            e.offset.y += d.y;
        }

        op = { anchor.x + e.offset.x, anchor.y + e.offset.y };
        op.x = ImClamp(op.x, canvasX0, canvasX1 - barW);
        op.y = ImClamp(op.y, canvasY0, canvasY1 - barH);

        // Snap on release
        if (e.isDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            e.isDragging = false;
            ImVec2 currentPos = op;
            e.snapPos = FindNearestSnapPos(currentPos, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH, armorElem.snapPos);
            ImVec2 snapAnchor = GetSnapAnchor(e.snapPos, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH);
            e.offset = { currentPos.x - snapAnchor.x, currentPos.y - snapAnchor.y };
            op = currentPos;
        }

        if (g.ESP_HealthBar) {
            dl->PushClipRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, true);

            float hp = 0.72f;
            dl->AddRectFilled(op, { op.x + barW, op.y + barH }, U32({ 0.1f, 0.1f, 0.1f, 0.9f }), 2.f);
            ImVec4 hc = { 0.2f + hp * 0.5f, 0.7f - hp * 0.3f, 0.2f, 1.f };
            dl->AddRectFilled({ op.x, op.y + barH - barH * hp }, { op.x + barW, op.y + barH }, U32(hc), 2.f);

            g.ESP_HealthBar_OffsetX = op.x - anchor.x;
            g.ESP_HealthBar_OffsetY = op.y - anchor.y;

            dl->PopClipRect();
        }

        auto [pMin, pMax] = PillRect(2);
        bool hovPill = ImGui::IsMouseHoveringRect(pMin, pMax);
        ESPElem& pe = GetElem("HP Bar");
        pe.animHov += ((hovPill ? 1.f : 0.f) - pe.animHov) * ImClamp(dt * 12.f, 0.f, 1.f);
        ImVec4 pillBg = Lerp4({ 0.08f,0.08f,0.10f,1.f }, { 0.25f,0.50f,0.90f,0.25f }, pe.animHov);
        dl->AddRectFilled(pMin, pMax, U32(pillBg), 4.f);
        dl->AddRect(pMin, pMax, U32(Lerp4(Col::Border, Col::Accent, pe.animHov)), 4.f, 0, 0.5f);
        ImVec2 ts = ImGui::CalcTextSize("HP Bar");
        dl->AddText({ pMin.x + (pMax.x - pMin.x - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f }, U32(Lerp4(Col::TextMuted, Col::AccentText, pe.animHov)), "HP Bar");
    }

    // ── ARMOR BAR ──
    {
        ESPElem& e = GetElem("Armor");
        ESPElem& healthElem = GetElem("HP Bar");

        float barW = 4.f, barH = figH * 0.85f;
        float boxLeft = cx - figW;
        float boxRight = cx + figW;
        float boxTop = figTop;
        float boxBot = figBot;

        ImVec2 anchor = GetSnapAnchor(e.snapPos, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH);
        ImVec2 op = anchor;

        // Handle dragging
        bool hovBar = ImGui::IsMouseHoveringRect({ anchor.x + e.offset.x, anchor.y + e.offset.y },
            { anchor.x + e.offset.x + barW, anchor.y + e.offset.y + barH });
        if (hovBar && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && e.inCanvas) {
            e.isDragging = true;
            ImVec2 d = ImGui::GetIO().MouseDelta;
            e.offset.x += d.x;
            e.offset.y += d.y;
        }

        op = { anchor.x + e.offset.x, anchor.y + e.offset.y };
        op.x = ImClamp(op.x, canvasX0, canvasX1 - barW);
        op.y = ImClamp(op.y, canvasY0, canvasY1 - barH);

        // Snap on release
        if (e.isDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            e.isDragging = false;
            ImVec2 currentPos = op;
            e.snapPos = FindNearestSnapPos(currentPos, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH, healthElem.snapPos);
            ImVec2 snapAnchor = GetSnapAnchor(e.snapPos, cx, boxLeft, boxRight, boxTop, boxBot, barW, barH);
            e.offset = { currentPos.x - snapAnchor.x, currentPos.y - snapAnchor.y };
            op = currentPos;
        }

        if (g.ESP_HealthBar) {
            dl->PushClipRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, true);

            float arm = 0.40f;
            dl->AddRectFilled(op, { op.x + barW, op.y + barH }, U32({ 0.1f, 0.1f, 0.1f, 0.9f }), 2.f);
            dl->AddRectFilled({ op.x, op.y + barH - barH * arm }, { op.x + barW, op.y + barH }, U32({ 0.4f, 0.6f, 0.9f, 1.f }), 2.f);

            g.ESP_Armor_OffsetX = op.x - anchor.x;
            g.ESP_Armor_OffsetY = op.y - anchor.y;

            dl->PopClipRect();
        }

        auto [pMin, pMax] = PillRect(3);
        bool hovPill = ImGui::IsMouseHoveringRect(pMin, pMax);
        ESPElem& pe = GetElem("Armor");
        pe.animHov += ((hovPill ? 1.f : 0.f) - pe.animHov) * ImClamp(dt * 12.f, 0.f, 1.f);
        ImVec4 pillBg = Lerp4({ 0.08f,0.08f,0.10f,1.f }, { 0.25f,0.50f,0.90f,0.25f }, pe.animHov);
        dl->AddRectFilled(pMin, pMax, U32(pillBg), 4.f);
        dl->AddRect(pMin, pMax, U32(Lerp4(Col::Border, Col::Accent, pe.animHov)), 4.f, 0, 0.5f);
        ImVec2 ts = ImGui::CalcTextSize("Armor");
        dl->AddText({ pMin.x + (pMax.x - pMin.x - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f }, U32(Lerp4(Col::TextMuted, Col::AccentText, pe.animHov)), "Armor");
    }

    // ── DISTANCE ──
    {
        const char* dist = "2m";
        ImVec2 ds = ImGui::CalcTextSize(dist);
        float eW = ds.x + 6.f, eH = ds.y + 4.f;
        ImVec2 anchor = { cx - eW * 0.5f, figBot + 8.f };
        ImVec2 op = { anchor.x, anchor.y };

        if (g.ESP_Distance) {
            dl->PushClipRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, true);
            dl->AddText({ op.x + 3.f, op.y + 2.f }, U32({ 1.f, 1.f, 1.f, 0.9f }), dist);
            dl->PopClipRect();
        }

        auto [pMin, pMax] = PillRect(4);
        bool hovPill = ImGui::IsMouseHoveringRect(pMin, pMax);
        ESPElem& pe = GetElem("Distance");
        pe.animHov += ((hovPill ? 1.f : 0.f) - pe.animHov) * ImClamp(dt * 12.f, 0.f, 1.f);
        ImVec4 pillBg = Lerp4({ 0.08f,0.08f,0.10f,1.f }, { 0.25f,0.50f,0.90f,0.25f }, pe.animHov);
        dl->AddRectFilled(pMin, pMax, U32(pillBg), 4.f);
        dl->AddRect(pMin, pMax, U32(Lerp4(Col::Border, Col::Accent, pe.animHov)), 4.f, 0, 0.5f);
        ImVec2 ts = ImGui::CalcTextSize("Distance");
        dl->AddText({ pMin.x + (pMax.x - pMin.x - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f }, U32(Lerp4(Col::TextMuted, Col::AccentText, pe.animHov)), "Distance");
    }

    // ── SNAPLINE ──
    {
        float eW = 6.f, eH = 6.f;
        ImVec2 anchor = { cx - 3.f, HIP.y - 3.f };
        ImVec2 op = anchor;

        if (g.ESP_Line) {
            dl->PushClipRect({ canvasX0, canvasY0 }, { canvasX1, canvasY1 }, true);
            dl->AddLine({ cx, canvasY1 }, { op.x + 3.f, op.y + 3.f }, U32({ 1.f, 1.f, 0.f, 0.7f }), g.ESP_LineThickness);
            dl->PopClipRect();
        }

        auto [pMin, pMax] = PillRect(5);
        bool hovPill = ImGui::IsMouseHoveringRect(pMin, pMax);
        ESPElem& pe = GetElem("Snapline");
        pe.animHov += ((hovPill ? 1.f : 0.f) - pe.animHov) * ImClamp(dt * 12.f, 0.f, 1.f);
        ImVec4 pillBg = Lerp4({ 0.08f,0.08f,0.10f,1.f }, { 0.25f,0.50f,0.90f,0.25f }, pe.animHov);
        dl->AddRectFilled(pMin, pMax, U32(pillBg), 4.f);
        dl->AddRect(pMin, pMax, U32(Lerp4(Col::Border, Col::Accent, pe.animHov)), 4.f, 0, 0.5f);
        ImVec2 ts = ImGui::CalcTextSize("Snapline");
        dl->AddText({ pMin.x + (pMax.x - pMin.x - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f }, U32(Lerp4(Col::TextMuted, Col::AccentText, pe.animHov)), "Snapline");
    }
}

// ─────────────────────────────────────────────
//  RenderMenu
// ─────────────────────────────────────────────
void Cheat::RenderMenu()
{
    constexpr float WW = 900.f;
    constexpr float WH = 500.f;
    constexpr float SBW = 150.f;
    constexpr float NavBtnH = 40.f;
    constexpr float PW = 320.f;
    constexpr float PH = 640.f;

    float dt = ImGui::GetIO().DeltaTime;

    bool bMenuOpen = g.ShowMenu;
    if (bMenuOpen && !bMenuWasOpen)  s_menuAnim = 0.f;
    else if (!bMenuOpen && bMenuWasOpen) s_menuAnim = 1.f;
    bMenuWasOpen = bMenuOpen;


    // update tab highlight animations
    for (int i = 0; i < 8; i++) {
        float target = (i == TabIndex) ? 1.f : 0.f;
        s_tabHighlight[i] += (target - s_tabHighlight[i]) * ImClamp(dt * 12.f, 0.f, 1.f);
    }


    float targetAnim = bMenuOpen ? 1.f : 0.f;
    s_menuAnim = ImClamp(s_menuAnim + dt / 0.18f * (targetAnim > s_menuAnim ? 1.f : -1.f), 0.f, 1.f);
    float menuE = EaseOut(s_menuAnim);

    s_tabAnim = ImClamp(s_tabAnim + dt / 0.14f, 0.f, 1.f);
    float tabE = EaseOut(s_tabAnim);

    ImGui::SetNextWindowSize({ WW, WH }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f,0.f });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, U32(Col::BgBase));
    ImGui::PushStyleColor(ImGuiCol_Border, U32(Col::Border));

    bool open = g.ShowMenu;
    ImGui::Begin("##cheat", &open,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    if (menuE < 0.01f) { ImGui::End(); return; }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wP = ImGui::GetWindowPos();
    float  mA = menuE;

    {
        ImVec4 base = Col::BgBase; base.w *= mA;
        ImVec4 deep = Col::BgDeep; deep.w *= mA;
        ImVec4 bord = Col::Border; bord.w *= mA;
        dl->AddRectFilled(wP, { wP.x + WW, wP.y + WH }, U32(base), 10.f);
        dl->AddRect(wP, { wP.x + WW, wP.y + WH }, U32(bord), 10.f, 0, 0.5f);
        dl->AddRectFilled(wP, { wP.x + SBW, wP.y + WH }, U32(deep), 10.f, ImDrawFlags_RoundCornersLeft);
        dl->AddLine({ wP.x + SBW, wP.y }, { wP.x + SBW, wP.y + WH }, U32(bord), 0.5f);
    }

    dl->PushClipRect({ wP.x + SBW, wP.y }, { wP.x + WW, wP.y + WH }, true);
    DrawParticles(dl, { wP.x + SBW, wP.y }, WW - SBW, WH, dt, mA);
    dl->PopClipRect();

    {
        ImVec2 lp = { wP.x + (SBW - 32.f) * 0.5f, wP.y + 14.f };
        ImVec4 ac = Col::Accent; ac.w *= mA;
        dl->AddRectFilled(lp, { lp.x + 32.f, lp.y + 32.f }, U32(ac), 8.f);
        ImVec4 wc = { 1.f,1.f,1.f,mA };
        dl->AddText({ lp.x + 10.f, lp.y + 7.f }, U32(wc), "L");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, mA);

    ImGui::SetCursorPos({ 8.f, 58.f });                    // left padding
    ImGui::BeginChild("##SB", { SBW - 8.f, WH - 58.f }, false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Spacing();
    NavButton(ICON_FA_CROSSHAIRS, "Aim", 0, &TabIndex, NavBtnH, SBW - 16.f);  // width reduced by 16px (8 each side)
    NavButton(ICON_FA_EYE, "Visuals", 1, &TabIndex, NavBtnH, SBW - 16.f);
    NavButton(ICON_FA_USER, "Player", 2, &TabIndex, NavBtnH, SBW - 16.f);
    NavButton(ICON_FA_CAR, "Vehicle", 3, &TabIndex, NavBtnH, SBW - 16.f);
    NavButton(ICON_FA_WAND_MAGIC, "Weapon", 4, &TabIndex, NavBtnH, SBW - 16.f);
    NavButton(ICON_FA_USERS, "Players", 5, &TabIndex, NavBtnH, SBW - 16.f);
    NavButton(ICON_FA_CAR, "Vehicles", 6, &TabIndex, NavBtnH, SBW - 16.f);
    NavButton(ICON_FA_COGS, "Settings", 7, &TabIndex, NavBtnH, SBW - 16.f);
    ImGui::EndChild();

    ImGui::SetCursorPos({ SBW, 0.f });
    ImGui::BeginChild("##CT", { WW - SBW, WH }, false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    float slideOffset = (1.f - tabE) * 30.f * (float)s_tabDir;
    float panelAlpha = tabE;

    ImGui::SetCursorPosX(12.f + slideOffset);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * panelAlpha);
    ImGui::BeginChild("##PN", { WW - SBW - 24.f, WH - 12.f }, false, ImGuiWindowFlags_NoScrollbar);

    switch (TabIndex)
    {
    case 0:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##AL", { half,0.f }, false);
        SectionLabel("CORE");
        ToggleRow("AimBot", &g.AimBot);
        ToggleRow("Prediction", &g.AimBotPrediction);
        ImGui::EndChild();
        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##AR", { half,0.f }, false);
        SectionLabel("FIELD OF VIEW");
        ToggleRow("Show FOV", &g.ShowFov);
        CustomSlider("FOV", &g.AimFov, 1.f, 1000.f, "%.0f");
        static float smooth = 50.f;
        CustomSlider("Smooth", &smooth, 0.f, 100.f, "%.0f%%");
        ImGui::EndChild();
        break;
    }
    case 1:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##VL", { half,0.f }, false);
        SectionLabel("OVERLAYS");
        ToggleRow("Box ESP", &g.ESP_Box);
        ToggleRow("Snapline", &g.ESP_Line);
        ToggleRow("Skeleton", &g.ESP_Skeleton);
        ToggleRow("Distance", &g.ESP_Distance);
        ToggleRow("Health Bar", &g.ESP_HealthBar);
        ToggleRow("Armor Bar", &g.ESP_ArmorBar);
        ToggleRow("Show NPC", &g.ShowNPC);
        ToggleRow("Show Self", &g.ShowSelf);
        ToggleRow("Show Dead", &g.ShowDead);
        ImGui::EndChild();

        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##VR", { half,0.f }, false);
        SectionLabel("RANGE");
        CustomSlider("Max dist", &g.ESP_MaxDistance, 50.f, 1000.f, "%.0fm");

        SectionLabel("STYLES");
        CustomSlider("Box Thick##bt", &g.ESP_BoxLineThickness, 0.1f, 3.0f, "%.1f");
        CustomSlider("Skeleton Thick##st", &g.ESP_SkeletonThickness, 0.1f, 3.0f, "%.1f");
        CustomSlider("Snapline Thick##lt", &g.ESP_LineThickness, 0.1f, 3.0f, "%.1f");

        SectionLabel("OUTLINES");
        ToggleRow("Box Outline", &g.ESP_BoxOutline);
        if (g.ESP_BoxOutline)
            CustomSlider("Box Outline Thick##bot", &g.ESP_BoxOutlineThickness, 0.5f, 5.0f, "%.1f");

        ToggleRow("Skeleton Outline", &g.ESP_SkeletonOutline);
        if (g.ESP_SkeletonOutline)
            CustomSlider("Skel Outline Thick##sot", &g.ESP_SkeletonOutlineThickness, 0.5f, 5.0f, "%.1f");

        ImGui::EndChild();
        break;
    }
    case 2:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##PL", { half,0.f }, false);
        SectionLabel("PLAYER");
        ToggleRow("GodMode", &g.GodMode);
        ImGui::EndChild();
        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##PR", { half,0.f }, false);
        SectionLabel("ABILITIES");
        ToggleRow("No Fall Dmg", &g.NoFallDamage);
        SectionLabel("ACTIONS");
        ImGui::Spacing();
        if (SmallButton("Armor to 100"))
            Cheat::RestoreArmor();
        if (SmallButton("Health to 100"))
            Cheat::RestoreHealth();
        ImGui::EndChild();
        break;
    }
    case 3:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##VhL", { half,0.f }, false);
        SectionLabel("VEHICLE");
        ToggleRow("No Break", &g.NoBreak);
        ToggleRow("Infinite Fuel", &g.InfiniteFuel);
        ImGui::EndChild();
        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##VhR", { half,0.f }, false);
        SectionLabel("HANDLING");
        CustomSlider("Speed Mult", &g.SpeedMult, 1.f, 5.f, "%.1fx");
        ImGui::EndChild();
        break;
    }
    case 4:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##WL", { half,0.f }, false);
        SectionLabel("WEAPON");
        ToggleRow("NoRecoil", &g.NoRecoil);
        ToggleRow("NoSpread", &g.NoSpread);
        ImGui::EndChild();
        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##WR", { half,0.f }, false);
        SectionLabel("AMMO");
        ToggleRow("Infinite Ammo", &g.InfiniteAmmo);
        if (SmallButton("Give All Weapons"))
            Cheat::GiveAllWeapons();
        ImGui::EndChild();
        break;
    }
    case 5:
    {
        static char Search[64] = {};
        ImGui::PushStyleColor(ImGuiCol_FrameBg, U32(Col::BgRow));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, U32(Col::BgRowHov));
        ImGui::PushStyleColor(ImGuiCol_Border, U32(Col::Border));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10.f,6.f });
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##srch", Search, IM_ARRAYSIZE(Search));
        ImGui::PopStyleVar(2); ImGui::PopStyleColor(3);
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, U32(Col::Border));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, U32(Col::Border));
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, U32(Col::BgRow));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 8.f,5.f });
        if (ImGui::BeginTable("##PLT", 5,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Alive", ImGuiTableColumnFlags_WidthFixed, 45.f);
            ImGui::TableSetupColumn("Vehicle", ImGuiTableColumnFlags_WidthFixed, 55.f);
            ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 36.f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for (int c = 0;c < 5;c++) { ImGui::TableSetColumnIndex(c);ImGui::TextColored(Col::TextMuted, "%s", ImGui::TableGetColumnName(c)); }
            for (auto& ped : EntityList) {
                std::string pName = "PLAYER";
                if (Search[0] && pName.find(Search) == std::string::npos) continue;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextColored(Col::TextPri, "%s", pName.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::TextColored(!ped.IsDead() ? Col::Green : Col::Red, !ped.IsDead() ? "Yes" : "No");
                ImGui::TableSetColumnIndex(2); ImGui::TextColored(ped.InVehicle() ? Col::Green : Col::Red, ped.InVehicle() ? "Yes" : "No");
                ImGui::TableSetColumnIndex(3); ImGui::TextColored(Col::TextSec, "%.0f, %.0f, %.0f", ped.m_vecPosition.x, ped.m_vecPosition.y, ped.m_vecPosition.z);
                ImGui::TableSetColumnIndex(4); SmallButton(("TP##" + pName).c_str());
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar(1); ImGui::PopStyleColor(3);
        break;
    }
    case 6:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##VcL", { half,0.f }, false);
        SectionLabel("VEHICLES");
        ToggleRow("Teleport to VH", &g.TeleportVH);
        ToggleRow("Vehicle ESP", &g.VehicleESP);
        ImGui::EndChild();
        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##VcR", { half,0.f }, false);
        SectionLabel("MANAGEMENT");
        if (SmallButton("Destroy All")) g.DestroyAllVehicles = true;
        ImGui::Spacing();
        if (SmallButton("Repair All"))  g.RepairAllVehicles = true;
        ImGui::EndChild();
        break;
    }
    case 7:
    {
        float half = (ImGui::GetContentRegionAvail().x - 12.f) * 0.5f;
        ImGui::BeginChild("##ST", { half,0.f }, false);
        SectionLabel("SYSTEM");
        ToggleRow("StreamProof", &g.StreamProof);
        ToggleRow("Crosshair", &g.Crosshair);
        ImGui::EndChild();
        ImGui::SameLine(0.f, 12.f);
        ImGui::BeginChild("##SB2", { half,0.f }, false);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 40.f);
        if (ExitButton()) g.process_active = false;
        ImGui::EndChild();
        break;
    }
    default: break;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();

    if (TabIndex == 1)
    {
        float previewSlide = (1.f - tabE) * 20.f;
        ImVec2 previewPos = { wP.x + WW + 12.f + previewSlide, wP.y + (WH - PH) * 0.5f };

        ImGui::SetNextWindowPos(previewPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize({ PW, PH }, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f,0.f });
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tabE * mA);

        ImGui::Begin("##ESPPreview", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
        ImGui::PopStyleVar(4);

        //DrawESPPreview(ImGui::GetWindowPos(), PW, PH);
        ImGui::End();
    }
}