#include "ToolBar.h"
#include <imgui.h> 
#include "App/Document/Document.h"
#include <imgui_internal.h>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
namespace YGsoftware
{
    const char* ToolBar::GetName() const
    {
        return "工具栏";
    }
    void ToolBar::OnRender(Document& document)
    {
        if (!g_show_ToolBarPanel)
            return;

        ImGui::Begin(GetName(), &g_show_ToolBarPanel);
        {
            auto& editor = document.GetEditor();
            ImGui::BeginGroup();
            if (ImGui::Button("直线", ImVec2(60, 32)))
            {
                editor.StartLineTool();
            }
            ImGui::SameLine();
            if (ImGui::Button("点", ImVec2(60, 32)))
            {
                editor.StartPointTool();
            }
            static bool showAxisWindow = false;
            if (ImGui::Button("一键轴网", ImVec2(60, 32)))
            {
                showAxisWindow = true;
            }
            ImGui::EndGroup();

            // 弹出窗口：一键轴网
            if (showAxisWindow)
            {
                // 简单轴网编辑器：左侧为绘制画布，右侧为轴列表和控制
                // 允许窗口在右下角缩放：设置默认大小但不要使用 AlwaysAutoResize
                ImGui::SetNextWindowSize(ImVec2(240, 240), ImGuiCond_FirstUseEver);
                ImGui::Begin("轴网", &showAxisWindow);
                {
                    // 本地数据结构（保持在函数内的静态变量中以便跨帧保存）
                    struct Axis { float pos; float realPos; bool vertical; bool visible; std::string name; };
                    static std::vector<Axis> axes;
                    static int axisCounter = 1;
                    // 0 = 横向, 1 = 纵向
                    static int addVertical = 1;
                    static int dragging = -1;
                    static float    axisExtend = 1.5f;   // 轴线出头（米）
                    static XMFLOAT2 gridOrigin = { 0.0f, 0.0f }; // 轴网左下角交叉点世界坐标
                    // ── 将 realPos 同步到归一化 pos ──────────────────────────────
                    // 分别取纵轴/横轴的 realPos 范围，然后把 [min,max] 映射到 [0.05, 0.95]
                    auto syncPos = [&]()
                        {
                            for (int dir = 0; dir <= 1; ++dir)
                            {
                                bool isV = (dir == 1);
                                float mn = FLT_MAX, mx = -FLT_MAX;
                                for (auto& a : axes)
                                    if (a.vertical == isV)
                                    {
                                        mn = std::min(mn, a.realPos); mx = std::max(mx, a.realPos);
                                    }
                                if (mn == FLT_MAX) continue;
                                float range = mx - mn;
                                for (auto& a : axes)
                                {
                                    if (a.vertical != isV) continue;
                                    a.pos = (range > 1e-6f)
                                        ? 0.05f + (a.realPos - mn) / range * 0.90f
                                        : 0.5f;
                                }
                            }
                        };

                    // ── 将归一化 pos 反算回 realPos（拖拽后用）────────────────────
                    auto syncRealFromPos = [&](int idx)
                        {
                            bool isV = axes[idx].vertical;
                            float mn = FLT_MAX, mx = -FLT_MAX;
                            float pmin = FLT_MAX, pmax = -FLT_MAX;
                            for (auto& a : axes)
                                if (a.vertical == isV)
                                {
                                    mn = std::min(mn, a.realPos); mx = std::max(mx, a.realPos);
                                    pmin = std::min(pmin, a.pos);  pmax = std::max(pmax, a.pos);
                                }
                            float prange = pmax - pmin;
                            float rrange = mx - mn;
                            if (prange > 1e-6f && rrange > 1e-6f)
                                axes[idx].realPos = mn + (axes[idx].pos - pmin) / prange * rrange;
                        };
                    // ── 纵/横轴世界坐标范围 ─────────────────────────────────────
                    auto calcRange = [&](float& vMin, float& vMax, float& hMin, float& hMax)
                        {
                            vMin = FLT_MAX; vMax = -FLT_MAX;
                            hMin = FLT_MAX; hMax = -FLT_MAX;
                            for (auto& a : axes)
                            {
                                if (a.vertical) { vMin = std::min(vMin, a.realPos); vMax = std::max(vMax, a.realPos); }
                                else { hMin = std::min(hMin, a.realPos); hMax = std::max(hMax, a.realPos); }
                            }
                            if (vMin == FLT_MAX) { vMin = 0.0f; vMax = 10.0f; }
                            if (hMin == FLT_MAX) { hMin = 0.0f; hMax = 10.0f; }
                        };
                    // 顶部工具条：选择方向，添加轴
                    ImGui::Text("方向："); ImGui::SameLine();
                    ImGui::RadioButton("纵向", &addVertical, 1); ImGui::SameLine();
                    ImGui::RadioButton("横向", &addVertical, 0);
                    ImGui::SameLine();
                    if (ImGui::Button("添加轴"))
                    {
                        Axis a;
                        a.vertical = (addVertical != 0);
                        a.visible = true;
                        float maxR = 0.0f;
                        for (auto& x : axes) if (x.vertical == a.vertical) maxR = std::max(maxR, x.realPos);
                        a.realPos = axes.empty() ? 0.0f : maxR + 1.0f;
                        a.pos = 0.5f;
                        std::ostringstream ss;
                        ss << "轴" << axisCounter++;
                        a.name = ss.str();
                        axes.push_back(std::move(a));
                        syncPos();
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100.0f);
                    ImGui::InputFloat("出头", &axisExtend, 0.5f, 1.0f, "%.1f");
                    axisExtend = std::max(0.0f, axisExtend);

                    // 原点坐标
                    ImGui::Text("原点："); ImGui::SameLine();
                    ImGui::SetNextItemWidth(80.0f);
                    ImGui::InputFloat("X##ox", &gridOrigin.x, 1.0f, 10.0f, "%.2f");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(80.0f);
                    ImGui::InputFloat("Y##oy", &gridOrigin.y, 1.0f, 10.0f, "%.2f");

                    ImGui::Columns(2, "axis_cols");


                    // 两列布局：左画布，右表格
                    ImGui::Columns(2, "axis_cols");

                    // 左侧画布
                    // 使画布宽度随窗口调整（使用 0 宽度填充列宽），高度设置为较小值以缩短黑框
                    ImGui::BeginChild("AxisCanvas", ImVec2(0, 220), true);
                    {
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        ImVec2 canvas_pos = ImGui::GetCursorScreenPos(); // top-left
                        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
                        if (canvas_size.x < 20) canvas_size.x = 480;
                        if (canvas_size.y < 20) canvas_size.y = 200;

                        // background
                        draw_list->AddRectFilled(canvas_pos,
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                            IM_COL32(10, 10, 10, 255));
                        // ── 画布布局 ──────────────────────────────────────────────
                        // 留出 margin 给轴号圆泡
                        const float MARGIN = 32.0f;   // 圆泡区域（像素）
                        const float BUBBLE_R = 10.0f;  // 圆泡半径

                        float gL = canvas_pos.x + MARGIN;     // grid left
                        float gR = canvas_pos.x + canvas_size.x - MARGIN; // grid right
                        float gT = canvas_pos.y + MARGIN;     // grid top
                        float gB = canvas_pos.y + canvas_size.y - MARGIN; // grid bottom
                        float gW = gR - gL;
                        float gH = gB - gT;

                        // 网格边框（淡色参考框）
                        draw_list->AddRect(ImVec2(gL, gT), ImVec2(gR, gB), IM_COL32(50, 50, 50, 180), 0, 0, 1.0f);

                        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                        bool hovered = ImGui::IsItemHovered();

                        // ── 辅助：按 pos 排好序的同向轴索引 ──────────────────────
                        auto sortedByDir = [&](bool vertical) -> std::vector<int>
                            {
                                std::vector<int> idx;
                                for (int i = 0; i < (int)axes.size(); ++i)
                                    if (axes[i].vertical == vertical && axes[i].visible)
                                        idx.push_back(i);
                                std::sort(idx.begin(), idx.end(),
                                    [&](int a, int b) { return axes[a].pos < axes[b].pos; });
                                return idx;
                            };
                        ImU32 lineCol = IM_COL32(200, 200, 50, 255);
                        ImU32 extCol = IM_COL32(200, 200, 50, 120); // 出头部分（淡）
                        ImU32 bubbleBg = IM_COL32(30, 30, 30, 230);
                        ImU32 bubbleFg = IM_COL32(220, 200, 60, 255);
                        ImU32 dimCol = IM_COL32(100, 220, 255, 220);
                        ImU32 tickCol = IM_COL32(100, 220, 255, 140);

                        // ── 辅助：画轴号圆泡 ──────────────────────────────────
                        auto drawBubble = [&](float cx, float cy, const std::string& name)
                            {
                                draw_list->AddCircleFilled(ImVec2(cx, cy), BUBBLE_R, bubbleBg);
                                draw_list->AddCircle(ImVec2(cx, cy), BUBBLE_R, bubbleFg, 0, 1.5f);
                                ImVec2 tsz = ImGui::CalcTextSize(name.c_str());
                                draw_list->AddText(ImVec2(cx - tsz.x * 0.5f, cy - tsz.y * 0.5f), bubbleFg, name.c_str());
                            };

                        // ── 绘制轴线 + 出头 + 圆泡 ───────────────────────────
                        for (size_t i = 0; i < axes.size(); ++i)
                        {
                            if (!axes[i].visible) continue;

                            if (axes[i].vertical)
                            {
                                float x = gL + axes[i].pos * gW;

                                // 主体：网格范围内（实线）
                                draw_list->AddLine(ImVec2(x, gT), ImVec2(x, gB), lineCol, 1.5f);
                                // 出头：超出网格到 margin 区（淡线）
                                draw_list->AddLine(ImVec2(x, canvas_pos.y + MARGIN * 0.15f), ImVec2(x, gT), extCol, 1.5f);
                                draw_list->AddLine(ImVec2(x, gB), ImVec2(x, canvas_pos.y + canvas_size.y - MARGIN * 0.15f), extCol, 1.5f);
                                // 圆泡
                                drawBubble(x, canvas_pos.y + BUBBLE_R + 2.0f, axes[i].name);
                                drawBubble(x, canvas_pos.y + canvas_size.y - BUBBLE_R - 2.0f, axes[i].name);

                                // 拖拽命中检测
                                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                                    if (std::abs(mouse_pos.x - x) < 6.0f && mouse_pos.y > gT && mouse_pos.y < gB)
                                        dragging = (int)i;
                            }
                            else
                            {
                                float y = gT + axes[i].pos * gH;

                                draw_list->AddLine(ImVec2(gL, y), ImVec2(gR, y), lineCol, 1.5f);
                                draw_list->AddLine(ImVec2(canvas_pos.x + MARGIN * 0.15f, y), ImVec2(gL, y), extCol, 1.5f);
                                draw_list->AddLine(ImVec2(gR, y), ImVec2(canvas_pos.x + canvas_size.x - MARGIN * 0.15f, y), extCol, 1.5f);
                                drawBubble(canvas_pos.x + BUBBLE_R + 2.0f, y, axes[i].name);
                                drawBubble(canvas_pos.x + canvas_size.x - BUBBLE_R - 2.0f, y, axes[i].name);

                                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                                    if (std::abs(mouse_pos.y - y) < 6.0f && mouse_pos.x > gL && mouse_pos.x < gR)
                                        dragging = (int)i;
                            }
                        }

                        // ── 距离标注（相邻同向轴之间）──────────────────────────
                        // 纵向：尺寸线画在网格顶部内侧
                        {
                            auto idx = sortedByDir(true);
                            for (int k = 1; k < (int)idx.size(); ++k)
                            {
                                int  a = idx[k - 1], b = idx[k];
                                float dist = std::abs(axes[b].realPos - axes[a].realPos);
                                float xa = gL + axes[a].pos * gW;
                                float xb = gL + axes[b].pos * gW;
                                float xm = (xa + xb) * 0.5f;
                                float y0 = gT + 6.0f;

                                draw_list->AddLine(ImVec2(xa, y0), ImVec2(xa, y0 + 8.0f), tickCol, 1.0f);
                                draw_list->AddLine(ImVec2(xb, y0), ImVec2(xb, y0 + 8.0f), tickCol, 1.0f);
                                draw_list->AddLine(ImVec2(xa, y0 + 4.0f), ImVec2(xb, y0 + 4.0f), tickCol, 1.0f);

                                char buf[32]; snprintf(buf, sizeof(buf), "%.2f", dist);
                                ImVec2 tsz = ImGui::CalcTextSize(buf);
                                draw_list->AddRectFilled(
                                    ImVec2(xm - tsz.x * 0.5f - 2, y0 + 10.0f),
                                    ImVec2(xm + tsz.x * 0.5f + 2, y0 + 10.0f + tsz.y), IM_COL32(12, 12, 12, 200));
                                draw_list->AddText(ImVec2(xm - tsz.x * 0.5f, y0 + 10.0f), dimCol, buf);
                            }
                        }

                        // 横向：尺寸线画在网格左侧内侧
                        {
                            auto idx = sortedByDir(false);
                            for (int k = 1; k < (int)idx.size(); ++k)
                            {
                                int  a = idx[k - 1], b = idx[k];
                                float dist = std::abs(axes[b].realPos - axes[a].realPos);
                                float ya = gT + axes[a].pos * gH;
                                float yb = gT + axes[b].pos * gH;
                                float ym = (ya + yb) * 0.5f;
                                float x0 = gL + 6.0f;

                                draw_list->AddLine(ImVec2(x0, ya), ImVec2(x0 + 8.0f, ya), tickCol, 1.0f);
                                draw_list->AddLine(ImVec2(x0, yb), ImVec2(x0 + 8.0f, yb), tickCol, 1.0f);
                                draw_list->AddLine(ImVec2(x0 + 4.0f, ya), ImVec2(x0 + 4.0f, yb), tickCol, 1.0f);

                                char buf[32]; snprintf(buf, sizeof(buf), "%.2f", dist);
                                ImVec2 tsz = ImGui::CalcTextSize(buf);
                                draw_list->AddRectFilled(
                                    ImVec2(x0 + 12.0f, ym - tsz.y * 0.5f - 1),
                                    ImVec2(x0 + 12.0f + tsz.x + 4, ym + tsz.y * 0.5f + 1), IM_COL32(12, 12, 12, 200));
                                draw_list->AddText(ImVec2(x0 + 14.0f, ym - tsz.y * 0.5f), dimCol, buf);
                            }
                        }

                        // ── 拖拽 ──────────────────────────────────────────────
                        if (dragging >= 0 && dragging < (int)axes.size() &&
                            ImGui::IsMouseDown(ImGuiMouseButton_Left))
                        {
                            if (axes[dragging].vertical)
                                axes[dragging].pos = std::clamp((mouse_pos.x - gL) / gW, 0.0f, 1.0f);
                            else
                                axes[dragging].pos = std::clamp((mouse_pos.y - gT) / gH, 0.0f, 1.0f);
                            syncRealFromPos(dragging);
                        }
                        if (dragging >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                            dragging = -1;

                        ImGui::EndChild();
                    }

                    ImGui::NextColumn();


                    // 右侧轴表
                    ImGui::BeginChild("AxisTable", ImVec2(0, 220), false);
                    {
                        // 5列：名称 | 方向 | 位置(m) | 显示 | 操作
                        bool needSync = false;

                        if (ImGui::BeginTable("atc", 4,
                            ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_BordersInnerV |
                            ImGuiTableFlags_ScrollY,
                            ImVec2(0, 0)))
                        {
                            ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                            ImGui::TableSetupColumn("方向", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                            ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                            ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();

                            for (int i = 0; i < (int)axes.size(); ++i)
                            {
                                ImGui::TableNextRow();
                                ImGui::PushID(i);

                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted(axes[i].name.c_str());

                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(axes[i].vertical ? "纵" : "横");

                                ImGui::TableSetColumnIndex(2);
                                ImGui::PushItemWidth(-1);
                                if (ImGui::InputFloat("##p", &axes[i].realPos, 0.5f, 1.0f, "%.2f"))
                                    needSync = true;
                                ImGui::PopItemWidth();

                                ImGui::TableSetColumnIndex(3);
                                if (ImGui::SmallButton("中")) { axes[i].realPos = 0.0f; needSync = true; }
                                ImGui::SameLine();
                                if (ImGui::SmallButton("X"))
                                {
                                    axes.erase(axes.begin() + i);
                                    ImGui::PopID();
                                    needSync = true;
                                    break;
                                }

                                ImGui::PopID();
                            }

                            ImGui::EndTable();
                        }

                        // ── 距离一览表（同向相邻轴） ──────────────────────────────
                        if (!axes.empty())
                        {
                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "间距一览");

                            // 纵向
                            std::vector<int> vIdx, hIdx;
                            for (int i = 0; i < (int)axes.size(); ++i)
                                (axes[i].vertical ? vIdx : hIdx).push_back(i);
                            auto cmp = [&](int a, int b) { return axes[a].realPos < axes[b].realPos; };
                            std::sort(vIdx.begin(), vIdx.end(), cmp);
                            std::sort(hIdx.begin(), hIdx.end(), cmp);

                            if (vIdx.size() >= 2)
                            {
                                ImGui::TextDisabled("纵轴：");
                                for (int k = 1; k < (int)vIdx.size(); ++k)
                                {
                                    float d = axes[vIdx[k]].realPos - axes[vIdx[k - 1]].realPos;
                                    ImGui::Text("  %s → %s : %.2f",
                                        axes[vIdx[k - 1]].name.c_str(),
                                        axes[vIdx[k]].name.c_str(), d);
                                }
                            }
                            if (hIdx.size() >= 2)
                            {
                                ImGui::TextDisabled("横轴：");
                                for (int k = 1; k < (int)hIdx.size(); ++k)
                                {
                                    float d = axes[hIdx[k]].realPos - axes[hIdx[k - 1]].realPos;
                                    ImGui::Text("  %s → %s : %.2f",
                                        axes[hIdx[k - 1]].name.c_str(),
                                        axes[hIdx[k]].name.c_str(), d);
                                }
                            }
                        }

                        if (needSync) syncPos();
                    }
                    ImGui::EndChild();

                    ImGui::Columns(1);

                    // 底部确认/取消
                    ImGui::Separator();
                    // ── 确定 / 取消 ───────────────────────────────────────────
                    if (ImGui::Button("确定"))
                    {
                        float vMin, vMax, hMin, hMax;
                        calcRange(vMin, vMax, hMin, hMax);

                        auto& scene = document.GetScene();
                        for (auto& a : axes)
                        {
                            if (!a.visible) continue;

                            // realPos 减去各方向最小值，使最左纵轴/最底横轴对齐到 gridOrigin
                            // 世界坐标 = gridOrigin + (realPos - min)
                            XMFLOAT3 start, end;
                            if (a.vertical)
                            {
                                float wx = gridOrigin.x + (a.realPos - vMin);
                                float wyMin = gridOrigin.y + 0.0f - axisExtend; // hMin - hMin = 0
                                float wyMax = gridOrigin.y + (hMax - hMin) + axisExtend;
                                start = { wx, wyMin, 0.0f };
                                end = { wx, wyMax, 0.0f };
                            }
                            else
                            {
                                float wy = gridOrigin.y + (a.realPos - hMin);
                                float wxMin = gridOrigin.x + 0.0f - axisExtend;
                                float wxMax = gridOrigin.x + (vMax - vMin) + axisExtend;
                                start = { wxMin, wy, 0.0f };
                                end = { wxMax, wy, 0.0f };
                            }

                            if (!YGsoftware::Line(start, end, true).IsValid()) continue;
                            scene.AddEntity(std::make_unique<LineEntity>(scene.NextObjectID(), start, end));
                        }
                        showAxisWindow = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("取消")) showAxisWindow = false;
                }
                ImGui::End();
            }
            ImGui::End();
        }
    }
}
