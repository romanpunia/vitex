#include "renderers.h"
#include "renderers/d3d11.h"
#include "renderers/ogl.h"
#include "components.h"
#include <imgui.h>
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#define MAP_BUTTON(NAV_NO, BUTTON_NO) { Settings->NavInputs[NAV_NO] = (SDL_GameControllerGetButton(Controller, BUTTON_NO) != 0) ? 1.0f : 0.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float V = (float)(SDL_GameControllerGetAxis(Controller, AXIS_NO) - V0) / (float)(V1 - V0); if (V > 1.0f) V = 1.0f; if (V > 0.0f && Settings->NavInputs[NAV_NO] < V) Settings->NavInputs[NAV_NO] = V; }
#endif

namespace Tomahawk
{
    namespace Engine
    {
        namespace Renderers
        {
            namespace GUI
            {
                void Interface::ApplyKeyState(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
                {
                    ImGuiIO& Input = ImGui::GetIO();
                    if ((int)Key >= 0 && (int)Key <= IM_ARRAYSIZE(Input.KeysDown))
                    {
#ifdef THAWK_HAS_SDL2
                        Input.KeysDown[Key] = Pressed;
                        Input.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
                        Input.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
                        Input.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
                        Input.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
#endif
                    }
                }
                void Interface::ApplyInput(char* Buffer, int Length)
                {
                    ImGuiIO& Input = ImGui::GetIO();
                    Input.AddInputCharactersUTF8(Buffer);
                }
                void Interface::ApplyCursorWheelState(int X, int Y, bool Normal)
                {
                    ImGuiIO& Input = ImGui::GetIO();
                    if (X > 0)
                        Input.MouseWheelH += 1;

                    if (X < 0)
                        Input.MouseWheelH -= 1;

                    if (Y > 0)
                        Input.MouseWheel += 1;

                    if (Y < 0)
                        Input.MouseWheel -= 1;
                }
                void Interface::EndChild()
                {
                    ImGui::EndChild();
                }
                void Interface::End()
                {
                    ImGui::End();
                }
                void Interface::Restyle(GUI::Style* Style)
                {
                    if (!Style)
                        return;

                    ImGuiStyle* Source = &ImGui::GetStyle();
                    memcpy(Source, Style, sizeof(ImGuiStyle));
                }
                void Interface::GetStyle(GUI::Style* Style)
                {
                    if (!Style)
                        return;

                    ImGuiStyle* Source = &ImGui::GetStyle();
                    memcpy(Style, Source, sizeof(GUI::Style));
                }
                void Interface::SetMouseDraw(bool Enabled)
                {
                    ImGuiIO& Stream = ImGui::GetIO();
                    Stream.MouseDrawCursor = Enabled;
                }
                void Interface::SetNextWindowPos(Compute::Vector2 pos, int cond, Compute::Vector2 pivot)
                {
                    ImGui::SetNextWindowPos(ImVec2(pos.X, pos.Y), cond, ImVec2(pivot.X, pivot.Y));
                }
                void Interface::SetNextWindowSize(Compute::Vector2 size, int cond)
                {
                    ImGui::SetNextWindowSize(ImVec2(size.X, size.Y), cond);
                }
                void Interface::SetNextWindowContentSize(Compute::Vector2 size)
                {
                    ImGui::SetNextWindowContentSize(ImVec2(size.X, size.Y));
                }
                void Interface::SetNextWindowCollapsed(bool collapsed, int cond)
                {
                    ImGui::SetNextWindowCollapsed(collapsed, cond);
                }
                void Interface::SetNextWindowFocus()
                {
                    ImGui::SetNextWindowFocus();
                }
                void Interface::SetNextWindowBgAlpha(float alpha)
                {
                    ImGui::SetNextWindowBgAlpha(alpha);
                }
                void Interface::SetWindowPos(Compute::Vector2 pos, int cond)
                {
                    ImGui::SetWindowPos(ImVec2(pos.X, pos.Y), cond);
                }
                void Interface::SetWindowSize(Compute::Vector2 size, int cond)
                {
                    ImGui::SetWindowSize(ImVec2(size.X, size.Y), cond);
                }
                void Interface::SetWindowCollapsed(bool collapsed, int cond)
                {
                    ImGui::SetWindowCollapsed(collapsed, cond);
                }
                void Interface::SetWindowFocus()
                {
                    ImGui::SetWindowFocus();
                }
                void Interface::SetWindowFontScale(float scale)
                {
                    ImGui::SetWindowFontScale(scale);
                }
                void Interface::SetWindowPos(const char* name, Compute::Vector2 pos, int cond)
                {
                    ImGui::SetWindowPos(name, ImVec2(pos.X, pos.Y), cond);
                }
                void Interface::SetWindowSize(const char* name, Compute::Vector2 size, int cond)
                {
                    ImGui::SetWindowSize(name, ImVec2(size.X, size.Y), cond);
                }
                void Interface::SetWindowCollapsed(const char* name, bool collapsed, int cond)
                {
                    ImGui::SetWindowCollapsed(name, collapsed, cond);
                }
                void Interface::SetWindowFocus(const char* name)
                {
                    ImGui::SetWindowFocus(name);
                }
                void Interface::SetScrollX(float scroll_x)
                {
                    ImGui::SetScrollX(scroll_x);
                }
                void Interface::SetScrollY(float scroll_y)
                {
                    ImGui::SetScrollY(scroll_y);
                }
                void Interface::SetScrollHere(float center_y_ratio)
                {
                    ImGui::SetScrollHere(center_y_ratio);
                }
                void Interface::SetScrollFromPosY(float pos_y, float center_y_ratio)
                {
                    ImGui::SetScrollFromPosY(pos_y, center_y_ratio);
                }
                void Interface::PushStyleColor(int idx, unsigned int col)
                {
                    ImGui::PushStyleColor(idx, col);
                }
                void Interface::PushStyleColor(int idx, Compute::Vector4 col)
                {
                    ImGui::PushStyleColor(idx, ImVec4(col.X, col.Y, col.Z, col.W));
                }
                void Interface::PopStyleColor(int count)
                {
                    ImGui::PopStyleColor(count);
                }
                void Interface::PushStyleVar(int idx, float val)
                {
                    ImGui::PushStyleVar(idx, val);
                }
                void Interface::PushStyleVar(int idx, Compute::Vector2 val)
                {
                    ImGui::PushStyleVar(idx, ImVec2(val.X, val.Y));
                }
                void Interface::PopStyleVar(int count)
                {
                    ImGui::PopStyleVar(count);
                }
                void Interface::PushItemWidth(float item_width)
                {
                    ImGui::PushItemWidth(item_width);
                }
                void Interface::PopItemWidth()
                {
                    ImGui::PopItemWidth();
                }
                void Interface::PushTextWrapPos(float wrap_pos_x)
                {
                    ImGui::PushTextWrapPos(wrap_pos_x);
                }
                void Interface::PopTextWrapPos()
                {
                    ImGui::PopTextWrapPos();
                }
                void Interface::PushAllowKeyboardFocus(bool allow_keyboard_focus)
                {
                    ImGui::PushAllowKeyboardFocus(allow_keyboard_focus);
                }
                void Interface::PopAllowKeyboardFocus()
                {
                    ImGui::PopAllowKeyboardFocus();
                }
                void Interface::PushButtonRepeat(bool repeat)
                {
                    ImGui::PushButtonRepeat(repeat);
                }
                void Interface::PopButtonRepeat()
                {
                    ImGui::PopButtonRepeat();
                }
                void Interface::Separator()
                {
                    ImGui::Separator();
                }
                void Interface::SameLine(float pos_x, float spacing_w)
                {
                    ImGui::SameLine();
                }
                void Interface::NewLine()
                {
                    ImGui::NewLine();
                }
                void Interface::Spacing()
                {
                    ImGui::Spacing();
                }
                void Interface::Dummy(Compute::Vector2 size)
                {
                    ImGui::Dummy(ImVec2(size.X, size.Y));
                }
                void Interface::Indent(float indent_w)
                {
                    ImGui::Indent(indent_w);
                }
                void Interface::Unindent(float indent_w)
                {
                    ImGui::Unindent(indent_w);
                }
                void Interface::BeginGroup()
                {
                    ImGui::BeginGroup();
                }
                void Interface::EndGroup()
                {
                    ImGui::EndGroup();
                }
                void Interface::SetCursorPos(Compute::Vector2 local_pos)
                {
                    ImGui::SetCursorPos(ImVec2(local_pos.X, local_pos.Y));
                }
                void Interface::SetCursorPosX(float x)
                {
                    ImGui::SetCursorPosX(x);
                }
                void Interface::SetCursorPosY(float y)
                {
                    ImGui::SetCursorPosX(y);
                }
                void Interface::SetCursorScreenPos(Compute::Vector2 screen_pos)
                {
                    ImGui::SetCursorScreenPos(ImVec2(screen_pos.X, screen_pos.Y));
                }
                void Interface::AlignTextToFramePadding()
                {
                    ImGui::AlignTextToFramePadding();
                }
                void Interface::PushID(const char* str_id)
                {
                    ImGui::PushID(str_id);
                }
                void Interface::PushID(const char* str_id_begin, const char* str_id_end)
                {
                    ImGui::PushID(str_id_begin, str_id_end);
                }
                void Interface::PushID(const void* ptr_id)
                {
                    ImGui::PushID(ptr_id);
                }
                void Interface::PushID(int int_id)
                {
                    ImGui::PushID(int_id);
                }
                void Interface::PopID()
                {
                    ImGui::PopID();
                }
                void Interface::TextUnformatted(const char* text, const char* text_end)
                {
                    ImGui::TextUnformatted(text, text_end);
                }
                void Interface::Text(const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::TextV(fmt, args);
                            va_end(args);
                }
                void Interface::TextV(const char* fmt, va_list args)
                {
                    ImGui::TextV(fmt, args);
                }
                void Interface::TextColored(Compute::Vector4 col, const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::TextColoredV(ImVec4(col.X, col.Y, col.Z, col.W), fmt, args);
                            va_end(args);
                }
                void Interface::TextColoredV(Compute::Vector4 col, const char* fmt, va_list args)
                {
                    ImGui::TextColoredV(ImVec4(col.X, col.Y, col.Z, col.W), fmt, args);
                }
                void Interface::TextDisabled(const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::TextDisabledV(fmt, args);
                            va_end(args);
                }
                void Interface::TextDisabledV(const char* fmt, va_list args)
                {
                    ImGui::TextDisabledV(fmt, args);
                }
                void Interface::TextWrapped(const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::TextWrappedV(fmt, args);
                            va_end(args);
                }
                void Interface::TextWrappedV(const char* fmt, va_list args)
                {
                    ImGui::TextWrappedV(fmt, args);
                }
                void Interface::LabelText(const char* label, const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::LabelTextV(label, fmt, args);
                            va_end(args);
                }
                void Interface::LabelTextV(const char* label, const char* fmt, va_list args)
                {
                    ImGui::LabelTextV(label, fmt, args);
                }
                void Interface::BulletText(const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::BulletTextV(fmt, args);
                            va_end(args);
                }
                void Interface::BulletTextV(const char* fmt, va_list args)
                {
                    ImGui::BulletTextV(fmt, args);
                }
                void Interface::ProgressBar(float fraction, Compute::Vector2 size_arg, const char* overlay)
                {
                    ImGui::ProgressBar(fraction, ImVec2(size_arg.X, size_arg.Y), overlay);
                }
                void Interface::Bullet()
                {
                    ImGui::Bullet();
                }
                void Interface::EndCombo()
                {
                    ImGui::EndCombo();
                }
                void Interface::Image(Graphics::Texture2D* user_texture_id, Compute::Vector2 size, Compute::Vector2 uv0, Compute::Vector2 uv1, Compute::Vector4 tint_col, Compute::Vector4 border_col)
                {
                    ImGui::Image(user_texture_id ? user_texture_id->GetResource() : nullptr, ImVec2(size.X, size.Y), ImVec2(uv0.X, uv0.Y), ImVec2(uv1.X, uv1.Y), ImVec4(tint_col.X, tint_col.Y, tint_col.Z, tint_col.W), ImVec4(border_col.X, border_col.Y, border_col.Z, border_col.W));
                }
                void Interface::SetColorEditOptions(int flags)
                {
                    ImGui::SetColorEditOptions(flags);
                }
                void Interface::TreePush(const char* str_id)
                {
                    ImGui::TreePush(str_id);
                }
                void Interface::TreePush(const void* ptr_id)
                {
                    ImGui::TreePush(ptr_id);
                }
                void Interface::TreePop()
                {
                    ImGui::TreePop();
                }
                void Interface::TreeAdvanceToLabelPos()
                {
                    ImGui::TreeAdvanceToLabelPos();
                }
                void Interface::SetNextTreeNodeOpen(bool is_open, int cond)
                {
                    ImGui::SetNextTreeNodeOpen(is_open, cond);
                }
                void Interface::ListBoxFooter()
                {
                    ImGui::ListBoxFooter();
                }
                void Interface::PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, Compute::Vector2 graph_size, int stride)
                {
                    ImGui::PlotLines(label, values, values_count, values_offset, overlay_text, scale_min, scale_max, ImVec2(graph_size.X, graph_size.Y), stride);
                }
                void Interface::PlotLines(const char* label, float(* values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, Compute::Vector2 graph_size)
                {
                    ImGui::PlotLines(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, ImVec2(graph_size.X, graph_size.Y));
                }
                void Interface::PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, Compute::Vector2 graph_size, int stride)
                {
                    ImGui::PlotHistogram(label, values, values_count, values_offset, overlay_text, scale_min, scale_max, ImVec2(graph_size.X, graph_size.Y), stride);
                }
                void Interface::PlotHistogram(const char* label, float(* values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, Compute::Vector2 graph_size)
                {
                    ImGui::PlotHistogram(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, ImVec2(graph_size.X, graph_size.Y));
                }
                void Interface::Value(const char* prefix, bool b)
                {
                    ImGui::Value(prefix, b);
                }
                void Interface::Value(const char* prefix, int v)
                {
                    ImGui::Value(prefix, v);
                }
                void Interface::Value(const char* prefix, unsigned int v)
                {
                    ImGui::Value(prefix, v);
                }
                void Interface::Value(const char* prefix, float v, const char* float_format)
                {
                    ImGui::Value(prefix, v, float_format);
                }
                void Interface::EndMainMenuBar()
                {
                    ImGui::EndMainMenuBar();
                }
                void Interface::EndMenuBar()
                {
                    ImGui::EndMenuBar();
                }
                void Interface::EndMenu()
                {
                    ImGui::EndMenu();
                }
                void Interface::BeginTooltip()
                {
                    ImGui::BeginTooltip();
                }
                void Interface::EndTooltip()
                {
                    ImGui::EndTooltip();
                }
                void Interface::SetTooltip(const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    ImGui::SetTooltipV(fmt, args);
                            va_end(args);
                }
                void Interface::SetTooltipV(const char* fmt, va_list args)
                {
                    ImGui::SetTooltipV(fmt, args);
                }
                void Interface::OpenPopup(const char* str_id)
                {
                    ImGui::OpenPopup(str_id);
                }
                void Interface::CloseCurrentPopup()
                {
                    ImGui::CloseCurrentPopup();
                }
                void Interface::EndPopup()
                {
                    ImGui::EndPopup();
                }
                void Interface::Columns(int count, const char* id, bool border)
                {
                    ImGui::Columns(count, id, border);
                }
                void Interface::NextColumn()
                {
                    ImGui::NextColumn();
                }
                void Interface::SetColumnWidth(int column_index, float width)
                {
                    ImGui::SetColumnWidth(column_index, width);
                }
                void Interface::SetColumnOffset(int column_index, float offset_x)
                {
                    ImGui::SetColumnOffset(column_index, offset_x);
                }
                void Interface::EndDragDropSource()
                {
                    ImGui::EndDragDropSource();
                }
                void Interface::EndDragDropTarget()
                {
                    ImGui::EndDragDropTarget();
                }
                void Interface::PushClipRect(Compute::Vector2 clip_rect_min, Compute::Vector2 clip_rect_max, bool intersect_with_current_clip_rect)
                {
                    ImGui::PushClipRect(ImVec2(clip_rect_min.X, clip_rect_min.Y), ImVec2(clip_rect_max.X, clip_rect_max.Y), intersect_with_current_clip_rect);
                }
                void Interface::PopClipRect()
                {
                    ImGui::PopClipRect();
                }
                void Interface::SetItemDefaultFocus()
                {
                    ImGui::SetItemDefaultFocus();
                }
                void Interface::SetKeyboardFocusHere(int offset)
                {
                    ImGui::SetKeyboardFocusHere(offset);
                }
                void Interface::SetItemAllowOverlap()
                {
                    ImGui::SetItemAllowOverlap();
                }
                void Interface::EndChildFrame()
                {
                    ImGui::EndChildFrame();
                }
                void Interface::CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end)
                {
                    ImGui::CalcListClipping(items_count, items_height, out_items_display_start, out_items_display_end);
                }
                void Interface::ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
                {
                    ImGui::ColorConvertRGBtoHSV(r, g, b, out_h, out_s, out_v);
                }
                void Interface::ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
                {
                    ImGui::ColorConvertHSVtoRGB(h, s, v, out_r, out_g, out_b);
                }
                void Interface::ResetMouseDragDelta(int button)
                {
                    ImGui::ResetMouseDragDelta(button);
                }
                void Interface::SetMouseCursor(int type)
                {
                    ImGui::SetMouseCursor(type);
                }
                void Interface::CaptureKeyboardFromApp(bool capture)
                {
                    ImGui::CaptureKeyboardFromApp(capture);
                }
                void Interface::CaptureMouseFromApp(bool capture)
                {
                    ImGui::CaptureMouseFromApp(capture);
                }
                void Interface::SetClipboardText(const char* text)
                {
                    ImGui::SetClipboardText(text);
                }
                bool Interface::BeginCanvas(const char* name)
                {
                    ImGui::SetNextWindowBgAlpha(0);
                    return ImGui::Begin(name, 0, GUI::WindowFlags_NoBringToFrontOnFocus | GUI::WindowFlags_NoCollapse | GUI::WindowFlags_NoFocusOnAppearing | GUI::WindowFlags_NoMove | GUI::WindowFlags_NoTitleBar | GUI::WindowFlags_NoResize);
                }
                bool Interface::BeginCanvasFull(const char* name)
                {
                    ImGuiIO& IO = ImGui::GetIO();
                    ImGui::SetNextWindowPos(ImVec2(0, 0));
                    ImGui::SetNextWindowSize(IO.DisplaySize);
                    ImGui::SetNextWindowBgAlpha(0);
                    return ImGui::Begin(name, 0, GUI::WindowFlags_NoBringToFrontOnFocus | GUI::WindowFlags_NoCollapse | GUI::WindowFlags_NoFocusOnAppearing | GUI::WindowFlags_NoMove | GUI::WindowFlags_NoTitleBar | GUI::WindowFlags_NoResize);
                }
                bool Interface::Begin(const char* name, bool* p_open, int flags)
                {
                    if (!name)
                        return false;

                    return ImGui::Begin(name, p_open, flags);
                }
                bool Interface::BeginChild(const char* str_id, Compute::Vector2 size, bool border, int flags)
                {
                    return ImGui::BeginChild(str_id, ImVec2(size.X, size.Y), border, flags);
                }
                bool Interface::BeginChild(unsigned int id, Compute::Vector2 size, bool border, int flags)
                {
                    return ImGui::BeginChild(id, ImVec2(size.X, size.Y), border, flags);
                }
                bool Interface::IsWindowAppearing()
                {
                    return ImGui::IsWindowAppearing();
                }
                bool Interface::IsWindowCollapsed()
                {
                    return ImGui::IsWindowCollapsed();
                }
                bool Interface::IsWindowFocused(int flags)
                {
                    return ImGui::IsWindowFocused(flags);
                }
                bool Interface::IsWindowHovered(int flags)
                {
                    return ImGui::IsWindowHovered(flags);
                }
                bool Interface::Button(const char* label, Compute::Vector2 size)
                {
                    return ImGui::Button(label, ImVec2(size.X, size.Y));
                }
                bool Interface::SmallButton(const char* label)
                {
                    return ImGui::SmallButton(label);
                }
                bool Interface::InvisibleButton(const char* str_id, Compute::Vector2 size)
                {
                    return ImGui::InvisibleButton(str_id, ImVec2(size.X, size.Y));
                }
                bool Interface::ArrowButton(const char* str_id, int dir)
                {
                    return ImGui::ArrowButton(str_id, dir);
                }
                bool Interface::ImageButton(Graphics::Texture2D* user_texture_id, Compute::Vector2 size, Compute::Vector2 uv0, Compute::Vector2 uv1, int frame_padding, Compute::Vector4 bg_col, Compute::Vector4 tint_col)
                {
                    return ImGui::ImageButton(user_texture_id ? user_texture_id->GetResource() : nullptr, ImVec2(size.X, size.Y), ImVec2(uv0.X, uv0.Y), ImVec2(uv1.X, uv1.Y), frame_padding, ImVec4(bg_col.X, bg_col.Y, bg_col.Z, bg_col.W), ImVec4(tint_col.X, tint_col.Y, tint_col.Z, tint_col.W));
                }
                bool Interface::Checkbox(const char* label, bool* v)
                {
                    return ImGui::Checkbox(label, v);
                }
                bool Interface::CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
                {
                    return ImGui::CheckboxFlags(label, flags, flags_value);
                }
                bool Interface::RadioButton(const char* label, bool active)
                {
                    return ImGui::RadioButton(label, active);
                }
                bool Interface::RadioButton(const char* label, int* v, int v_button)
                {
                    return ImGui::RadioButton(label, v, v_button);
                }
                bool Interface::BeginCombo(const char* label, const char* preview_value, int flags)
                {
                    return ImGui::BeginCombo(label, preview_value, flags);
                }
                bool Interface::Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items)
                {
                    return ImGui::Combo(label, current_item, items, items_count, popup_max_height_in_items);
                }
                bool Interface::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items)
                {
                    return ImGui::Combo(label, current_item, items_separated_by_zeros, popup_max_height_in_items);
                }
                bool Interface::Combo(const char* label, int* current_item, bool(* items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items)
                {
                    return ImGui::Combo(label, current_item, items_getter, data, items_count, popup_max_height_in_items);
                }
                bool Interface::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, power);
                }
                bool Interface::DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::DragFloat2(label, v, v_speed, v_min, v_max, format, power);
                }
                bool Interface::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::DragFloat3(label, v, v_speed, v_min, v_max, format, power);
                }
                bool Interface::DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::DragFloat4(label, v, v_speed, v_min, v_max, format, power);
                }
                bool Interface::DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, float power)
                {
                    return ImGui::DragFloatRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max, power);
                }
                bool Interface::DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format)
                {
                    return ImGui::DragInt(label, v, v_speed, v_min, v_max, format);
                }
                bool Interface::DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format)
                {
                    return ImGui::DragInt2(label, v, v_speed, v_min, v_max, format);
                }
                bool Interface::DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format)
                {
                    return ImGui::DragInt3(label, v, v_speed, v_min, v_max, format);
                }
                bool Interface::DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format)
                {
                    return ImGui::DragInt4(label, v, v_speed, v_min, v_max, format);
                }
                bool Interface::DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max)
                {
                    return ImGui::DragIntRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max);
                }
                bool Interface::DragScalar(const char* label, int data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
                {
                    return ImGui::DragScalar(label, data_type, v, v_speed, v_min, v_max, format);
                }
                bool Interface::DragScalarN(const char* label, int data_type, void* v, int components, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
                {
                    return ImGui::DragScalarN(label, data_type, v, components, v_speed, v_min, v_max, format);
                }
                bool Interface::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::SliderFloat(label, v, v_min, v_max, format, power);
                }
                bool Interface::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::SliderFloat2(label, v, v_min, v_max, format, power);
                }
                bool Interface::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::SliderFloat3(label, v, v_min, v_max, format, power);
                }
                bool Interface::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::SliderFloat4(label, v, v_min, v_max, format, power);
                }
                bool Interface::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max)
                {
                    return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max);
                }
                bool Interface::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
                {
                    return ImGui::SliderInt(label, v, v_min, v_max, format);
                }
                bool Interface::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
                {
                    return ImGui::SliderInt2(label, v, v_min, v_max, format);
                }
                bool Interface::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
                {
                    return ImGui::SliderInt3(label, v, v_min, v_max, format);
                }
                bool Interface::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
                {
                    return ImGui::SliderInt4(label, v, v_min, v_max, format);
                }
                bool Interface::SliderScalar(const char* label, int data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
                {
                    return ImGui::SliderScalar(label, data_type, v, v_min, v_max, format, power);
                }
                bool Interface::SliderScalarN(const char* label, int data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, float power)
                {
                    return ImGui::SliderScalarN(label, data_type, v, components, v_min, v_max, format, power);
                }
                bool Interface::VSliderFloat(const char* label, Compute::Vector2 size, float* v, float v_min, float v_max, const char* format, float power)
                {
                    return ImGui::VSliderFloat(label, ImVec2(size.X, size.Y), v, v_min, v_max, format, power);
                }
                bool Interface::VSliderInt(const char* label, Compute::Vector2 size, int* v, int v_min, int v_max, const char* format)
                {
                    return ImGui::VSliderInt(label, ImVec2(size.X, size.Y), v, v_min, v_max, format);
                }
                bool Interface::VSliderScalar(const char* label, Compute::Vector2 size, int data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
                {
                    return ImGui::VSliderScalar(label, ImVec2(size.X, size.Y), data_type, v, v_min, v_max, format, power);
                }
                bool Interface::InputText(const char* label, char* buf, size_t buf_size, int flags)
                {
                    return ImGui::InputText(label, buf, buf_size, flags);
                }
                bool Interface::InputTextMultiline(const char* label, char* buf, size_t buf_size, Compute::Vector2 size, int flags)
                {
                    return ImGui::InputTextMultiline(label, buf, buf_size, ImVec2(size.X, size.Y), flags);
                }
                bool Interface::InputFloat(const char* label, float* v, float step, float step_fast, const char* format, int extra_flags)
                {
                    return ImGui::InputFloat(label, v, step, step_fast, format, extra_flags);
                }
                bool Interface::InputFloat2(const char* label, float v[2], const char* format, int extra_flags)
                {
                    return ImGui::InputFloat2(label, v, format, extra_flags);
                }
                bool Interface::InputFloat3(const char* label, float v[3], const char* format, int extra_flags)
                {
                    return ImGui::InputFloat3(label, v, format, extra_flags);
                }
                bool Interface::InputFloat4(const char* label, float v[4], const char* format, int extra_flags)
                {
                    return ImGui::InputFloat4(label, v, format, extra_flags);
                }
                bool Interface::InputInt(const char* label, int* v, int step, int step_fast, int extra_flags)
                {
                    return ImGui::InputInt(label, v, step, step_fast, extra_flags);
                }
                bool Interface::InputInt2(const char* label, int v[2], int extra_flags)
                {
                    return ImGui::InputInt2(label, v, extra_flags);
                }
                bool Interface::InputInt3(const char* label, int v[3], int extra_flags)
                {
                    return ImGui::InputInt3(label, v, extra_flags);
                }
                bool Interface::InputInt4(const char* label, int v[4], int extra_flags)
                {
                    return ImGui::InputInt4(label, v, extra_flags);
                }
                bool Interface::InputDouble(const char* label, Float64* v, Float64 step, Float64 step_fast, const char* format, int extra_flags)
                {
                    return ImGui::InputDouble(label, v, step, step_fast, format, extra_flags);
                }
                bool Interface::InputScalar(const char* label, int data_type, void* v, const void* step, const void* step_fast, const char* format, int extra_flags)
                {
                    return ImGui::InputScalar(label, data_type, v, step, step_fast, format, extra_flags);
                }
                bool Interface::InputScalarN(const char* label, int data_type, void* v, int components, const void* step, const void* step_fast, const char* format, int extra_flags)
                {
                    return ImGui::InputScalarN(label, data_type, v, components, step, step_fast, format, extra_flags);
                }
                bool Interface::ColorEdit3(const char* label, float col[3], int flags)
                {
                    return ImGui::ColorEdit3(label, col, flags);
                }
                bool Interface::ColorEdit4(const char* label, float col[4], int flags)
                {
                    return ImGui::ColorEdit4(label, col, flags);
                }
                bool Interface::ColorPicker3(const char* label, float col[3], int flags)
                {
                    return ImGui::ColorPicker3(label, col, flags);
                }
                bool Interface::ColorPicker4(const char* label, float col[4], int flags, const float* ref_col)
                {
                    return ImGui::ColorPicker4(label, col, flags, ref_col);
                }
                bool Interface::ColorButton(const char* desc_id, Compute::Vector4 col, int flags, Compute::Vector2 size)
                {
                    return ImGui::ColorButton(desc_id, ImVec4(col.X, col.Y, col.Z, col.W), flags, ImVec2(size.X, size.Y));
                }
                bool Interface::TreeNode(const char* label)
                {
                    return ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_FramePadding);
                }
                bool Interface::TreeNode(const char* str_id, const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    bool is_open = ImGui::TreeNodeExV(str_id, 0, fmt, args);
                            va_end(args);

                    return is_open;
                }
                bool Interface::TreeNode(const void* ptr_id, const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    bool is_open = ImGui::TreeNodeExV(ptr_id, 0, fmt, args);
                            va_end(args);

                    return is_open;
                }
                bool Interface::TreeNodeV(const char* str_id, const char* fmt, va_list args)
                {
                    return ImGui::TreeNodeV(str_id, fmt, args);
                }
                bool Interface::TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
                {
                    return ImGui::TreeNodeV(ptr_id, fmt, args);
                }
                bool Interface::TreeNodeEx(const char* label, int flags)
                {
                    return ImGui::TreeNodeEx(label, flags);
                }
                bool Interface::TreeNodeEx(const char* str_id, int flags, const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    bool is_open = ImGui::TreeNodeExV(str_id, flags, fmt, args);
                            va_end(args);

                    return is_open;
                }
                bool Interface::TreeNodeEx(const void* ptr_id, int flags, const char* fmt, ...)
                {
                    va_list args;
                            va_start(args, fmt);
                    bool is_open = ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
                            va_end(args);

                    return is_open;
                }
                bool Interface::TreeNodeExV(const char* str_id, int flags, const char* fmt, va_list args)
                {
                    return ImGui::TreeNodeExV(str_id, flags, fmt, args);
                }
                bool Interface::TreeNodeExV(const void* ptr_id, int flags, const char* fmt, va_list args)
                {
                    return ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
                }
                bool Interface::CollapsingHeader(const char* label, int flags)
                {
                    return ImGui::CollapsingHeader(label, flags);
                }
                bool Interface::CollapsingHeader(const char* label, bool* p_open, int flags)
                {
                    return ImGui::CollapsingHeader(label, p_open, flags);
                }
                bool Interface::Selectable(const char* label, bool selected, int flags, Compute::Vector2 size)
                {
                    return ImGui::Selectable(label, selected, flags, ImVec2(size.X, size.Y));
                }
                bool Interface::Selectable(const char* label, bool* p_selected, int flags, Compute::Vector2 size)
                {
                    return ImGui::Selectable(label, p_selected, flags, ImVec2(size.X, size.Y));
                }
                bool Interface::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
                {
                    return ImGui::ListBox(label, current_item, items, items_count, height_in_items);
                }
                bool Interface::ListBox(const char* label, int* current_item, bool(* items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items)
                {
                    return ImGui::ListBox(label, current_item, items_getter, data, items_count, height_in_items);
                }
                bool Interface::ListBoxHeader(const char* label, Compute::Vector2 size)
                {
                    return ImGui::ListBoxHeader(label, ImVec2(size.X, size.Y));
                }
                bool Interface::ListBoxHeader(const char* label, int items_count, int height_in_items)
                {
                    return ImGui::ListBoxHeader(label, items_count, height_in_items);
                }
                bool Interface::BeginMainMenuBar()
                {
                    return ImGui::BeginMainMenuBar();
                }
                bool Interface::BeginMenuBar()
                {
                    return ImGui::BeginMenuBar();
                }
                bool Interface::BeginMenu(const char* label, bool enabled)
                {
                    return ImGui::BeginMenu(label, enabled);
                }
                bool Interface::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)
                {
                    return ImGui::MenuItem(label, shortcut, selected, enabled);
                }
                bool Interface::MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled)
                {
                    return ImGui::MenuItem(label, shortcut, p_selected, enabled);
                }
                bool Interface::BeginPopup(const char* str_id, int flags)
                {
                    return ImGui::BeginPopup(str_id, flags);
                }
                bool Interface::BeginPopupContextItem(const char* str_id, int mouse_button)
                {
                    return ImGui::BeginPopupContextItem(str_id, mouse_button);
                }
                bool Interface::BeginPopupContextWindow(const char* str_id, int mouse_button, bool also_over_items)
                {
                    return ImGui::BeginPopupContextWindow(str_id, mouse_button, also_over_items);
                }
                bool Interface::BeginPopupContextVoid(const char* str_id, int mouse_button)
                {
                    return ImGui::BeginPopupContextVoid(str_id, mouse_button);
                }
                bool Interface::BeginPopupModal(const char* name, bool* p_open, int flags)
                {
                    return ImGui::BeginPopupModal(name, p_open, flags);
                }
                bool Interface::OpenPopupOnItemClick(const char* str_id, int mouse_button)
                {
                    return ImGui::OpenPopupOnItemClick(str_id, mouse_button);
                }
                bool Interface::IsPopupOpen(const char* str_id)
                {
                    return ImGui::IsPopupOpen(str_id);
                }
                bool Interface::BeginDragDropSource(int flags)
                {
                    return ImGui::BeginDragDropSource(flags);
                }
                bool Interface::SetDragDropPayload(const char* type, const void* data, size_t size, int cond)
                {
                    return ImGui::SetDragDropPayload(type, data, size, cond);
                }
                bool Interface::BeginDragDropTarget()
                {
                    return ImGui::BeginDragDropTarget();
                }
                bool Interface::IsItemHovered(int flags)
                {
                    return ImGui::IsItemHovered(flags);
                }
                bool Interface::IsItemActive()
                {
                    return ImGui::IsItemActive();
                }
                bool Interface::IsItemFocused()
                {
                    return ImGui::IsItemFocused();
                }
                bool Interface::IsItemClicked(int mouse_button)
                {
                    return ImGui::IsItemClicked();
                }
                bool Interface::IsItemVisible()
                {
                    return ImGui::IsItemVisible();
                }
                bool Interface::IsItemEdited()
                {
                    return ImGui::IsItemEdited();
                }
                bool Interface::IsItemDeactivated()
                {
                    return ImGui::IsItemDeactivated();
                }
                bool Interface::IsItemDeactivatedAfterEdit()
                {
                    return ImGui::IsItemDeactivatedAfterEdit();
                }
                bool Interface::IsAnyItemHovered()
                {
                    return ImGui::IsAnyItemHovered();
                }
                bool Interface::IsAnyItemActive()
                {
                    return ImGui::IsAnyItemActive();
                }
                bool Interface::IsAnyItemFocused()
                {
                    return ImGui::IsAnyItemFocused();
                }
                bool Interface::IsRectVisible(Compute::Vector2 size)
                {
                    return ImGui::IsRectVisible(ImVec2(size.X, size.Y));
                }
                bool Interface::IsRectVisible(Compute::Vector2 rect_min, Compute::Vector2 rect_max)
                {
                    return ImGui::IsRectVisible(ImVec2(rect_min.X, rect_min.Y), ImVec2(rect_max.X, rect_max.Y));
                }
                bool Interface::BeginChildFrame(unsigned int id, Compute::Vector2 size, int flags)
                {
                    return ImGui::BeginChildFrame(id, ImVec2(size.X, size.Y), flags);
                }
                bool Interface::IsKeyDown(int user_key_index)
                {
                    return ImGui::IsKeyDown(user_key_index);
                }
                bool Interface::IsKeyPressed(int user_key_index, bool repeat)
                {
                    return ImGui::IsKeyPressed(user_key_index, repeat);
                }
                bool Interface::IsKeyReleased(int user_key_index)
                {
                    return ImGui::IsKeyReleased(user_key_index);
                }
                bool Interface::IsMouseDown(int button)
                {
                    return ImGui::IsMouseDown(button);
                }
                bool Interface::IsAnyMouseDown()
                {
                    return ImGui::IsAnyMouseDown();
                }
                bool Interface::IsMouseClicked(int button, bool repeat)
                {
                    return ImGui::IsMouseClicked(button, repeat);
                }
                bool Interface::IsMouseDoubleClicked(int button)
                {
                    return ImGui::IsMouseDoubleClicked(button);
                }
                bool Interface::IsMouseReleased(int button)
                {
                    return ImGui::IsMouseReleased(button);
                }
                bool Interface::IsMouseDragging(int button, float lock_threshold)
                {
                    return ImGui::IsMouseDragging(button, lock_threshold);
                }
                bool Interface::IsMouseHoveringRect(Compute::Vector2 r_min, Compute::Vector2 r_max, bool clip)
                {
                    return ImGui::IsMouseHoveringRect(ImVec2(r_min.X, r_min.Y), ImVec2(r_max.X, r_max.Y), clip);
                }
                bool Interface::IsMousePosValid(Compute::Vector2* mouse_pos)
                {
                    return ImGui::IsMousePosValid((ImVec2*)mouse_pos);
                }
                bool Interface::IsTextFocused()
                {
                    return ImGui::GetIO().WantTextInput;
                }
                int Interface::GetMouseCursor()
                {
                    return ImGui::GetMouseCursor();
                }
                int Interface::GetKeyIndex(int _key)
                {
                    return ImGui::GetKeyIndex(_key);
                }
                int Interface::GetFrameCount()
                {
                    return ImGui::GetFrameCount();
                }
                int Interface::GetColumnIndex()
                {
                    return ImGui::GetColumnIndex();
                }
                int Interface::GetColumnsCount()
                {
                    return ImGui::GetColumnsCount();
                }
                int Interface::GetKeyPressedAmount(int key_index, float repeat_delay, float rate)
                {
                    return ImGui::GetKeyPressedAmount(key_index, repeat_delay, rate);
                }
                float Interface::GetWindowWidth()
                {
                    return ImGui::GetWindowWidth();
                }
                float Interface::GetWindowHeight()
                {
                    return ImGui::GetWindowHeight();
                }
                float Interface::GetContentRegionAvailWidth()
                {
                    return ImGui::GetContentRegionAvailWidth();
                }
                float Interface::GetWindowContentRegionWidth()
                {
                    return ImGui::GetWindowContentRegionWidth();
                }
                float Interface::GetScrollX()
                {
                    return ImGui::GetScrollX();
                }
                float Interface::GetScrollY()
                {
                    return ImGui::GetScrollY();
                }
                float Interface::GetScrollMaxX()
                {
                    return ImGui::GetScrollMaxX();
                }
                float Interface::GetScrollMaxY()
                {
                    return ImGui::GetScrollMaxY();
                }
                float Interface::GetFontSize()
                {
                    return ImGui::GetFontSize();
                }
                float Interface::CalcItemWidth()
                {
                    return ImGui::CalcItemWidth();
                }
                float Interface::GetCursorPosX()
                {
                    return ImGui::GetCursorPosX();
                }
                float Interface::GetCursorPosY()
                {
                    return ImGui::GetCursorPosY();
                }
                float Interface::GetTextLineHeight()
                {
                    return ImGui::GetTextLineHeight();
                }
                float Interface::GetTextLineHeightWithSpacing()
                {
                    return ImGui::GetTextLineHeightWithSpacing();
                }
                float Interface::GetFrameHeight()
                {
                    return ImGui::GetFrameHeight();
                }
                float Interface::GetFrameHeightWithSpacing()
                {
                    return ImGui::GetFrameHeightWithSpacing();
                }
                float Interface::GetTreeNodeToLabelSpacing()
                {
                    return ImGui::GetTreeNodeToLabelSpacing();
                }
                float Interface::GetColumnWidth(int column_index)
                {
                    return ImGui::GetColumnWidth(column_index);
                }
                float Interface::GetColumnOffset(int column_index)
                {
                    return ImGui::GetColumnOffset(column_index);
                }
                Float64 Interface::GetTime()
                {
                    return ImGui::GetTime();
                }
                unsigned int Interface::GetColorU32(int idx, float alpha_mul)
                {
                    return ImGui::GetColorU32(idx, alpha_mul);
                }
                unsigned int Interface::GetColorU32(Compute::Vector4 col)
                {
                    return ImGui::GetColorU32(ImVec4(col.X, col.Y, col.Z, col.W));
                }
                unsigned int Interface::GetColorU32(unsigned int col)
                {
                    return ImGui::GetColorU32(col);
                }
                unsigned int Interface::ColorConvertFloat4ToU32(Compute::Vector4 in)
                {
                    return ImGui::ColorConvertFloat4ToU32(ImVec4(in.X, in.Y, in.Z, in.W));
                }
                unsigned int Interface::GetID(const char* str_id)
                {
                    return ImGui::GetID(str_id);
                }
                unsigned int Interface::GetID(const char* str_id_begin, const char* str_id_end)
                {
                    return ImGui::GetID(str_id_begin, str_id_end);
                }
                unsigned int Interface::GetID(const void* ptr_id)
                {
                    return ImGui::GetID(ptr_id);
                }
                Compute::Vector2 Interface::GetWindowPos()
                {
                    ImVec2 Value = ImGui::GetWindowPos();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetWindowSize()
                {
                    ImVec2 Value = ImGui::GetWindowSize();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetContentRegionMax()
                {
                    ImVec2 Value = ImGui::GetContentRegionMax();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetContentRegionAvail()
                {
                    ImVec2 Value = ImGui::GetContentRegionAvail();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetWindowContentRegionMin()
                {
                    ImVec2 Value = ImGui::GetWindowContentRegionMin();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetWindowContentRegionMax()
                {
                    ImVec2 Value = ImGui::GetWindowContentRegionMax();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetFontTexUvWhitePixel()
                {
                    ImVec2 Value = ImGui::GetFontTexUvWhitePixel();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetCursorPos()
                {
                    ImVec2 Value = ImGui::GetCursorPos();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetCursorStartPos()
                {
                    ImVec2 Value = ImGui::GetCursorStartPos();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetCursorScreenPos()
                {
                    ImVec2 Value = ImGui::GetCursorScreenPos();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetItemRectMin()
                {
                    ImVec2 Value = ImGui::GetItemRectMin();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetItemRectMax()
                {
                    ImVec2 Value = ImGui::GetItemRectMax();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetItemRectSize()
                {
                    ImVec2 Value = ImGui::GetItemRectSize();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
                {
                    ImVec2 Value = ImGui::CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetMousePos()
                {
                    ImVec2 Value = ImGui::GetMousePos();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetMousePosOnOpeningCurrentPopup()
                {
                    ImVec2 Value = ImGui::GetMousePosOnOpeningCurrentPopup();
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector2 Interface::GetMouseDragDelta(int button, float lock_threshold)
                {
                    ImVec2 Value = ImGui::GetMouseDragDelta(button, lock_threshold);
                    return Compute::Vector2(Value.x, Value.y);
                }
                Compute::Vector4 Interface::GetStyleColorVec4(int idx)
                {
                    ImVec4 Value = ImGui::GetStyleColorVec4(idx);
                    return Compute::Vector4(Value.x, Value.y, Value.z, Value.w);
                }
                Compute::Vector4 Interface::ColorConvertU32ToFloat4(unsigned int in)
                {
                    ImVec4 Value = ImGui::ColorConvertU32ToFloat4(in);
                    return Compute::Vector4(Value.x, Value.y, Value.z, Value.w);
                }
                const char* Interface::GetClipboardText()
                {
                    return ImGui::GetClipboardText();
                }
                const char* Interface::GetStyleColorName(int idx)
                {
                    return ImGui::GetStyleColorName(idx);
                }
            }

            ModelRenderer::ModelRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Model)
            {
                Priority = true;
            }
            void ModelRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
            }
            void ModelRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
            }
            ModelRenderer* ModelRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ModelRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLModelRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            SkinnedModelRenderer::SkinnedModelRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Skinned_Model)
            {
                Priority = true;
            }
            void SkinnedModelRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
            }
            void SkinnedModelRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
            }
            SkinnedModelRenderer* SkinnedModelRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11SkinnedModelRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLSkinnedModelRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            ElementSystemRenderer::ElementSystemRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Element_System)
            {
                Priority = true;
            }
            void ElementSystemRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
            }
            void ElementSystemRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
            }
            ElementSystemRenderer* ElementSystemRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ElementSystemRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLElementSystemRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            DepthRenderer::DepthRenderer(RenderSystem* Lab) : IntervalRenderer(Lab, RendererId_Depth), ShadowDistance(0.5f)
            {
                Priority = false;
            }
            DepthRenderer::~DepthRenderer()
            {
                for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
                    delete *It;

                for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
                    delete *It;

                for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
                    delete *It;

                if (!System || !System->GetScene())
                    return;

                Rest::Pool<Component*>* Lights = System->GetScene()->GetComponents(Engine::ComponentId_Point_Light);
                for (auto It = Lights->Begin(); It != Lights->End(); It++)
                    (*It)->As<Components::PointLight>()->Occlusion = nullptr;

                Lights = System->GetScene()->GetComponents(Engine::ComponentId_Spot_Light);
                for (auto It = Lights->Begin(); It != Lights->End(); It++)
                    (*It)->As<Components::SpotLight>()->Occlusion = nullptr;

                Lights = System->GetScene()->GetComponents(Engine::ComponentId_Line_Light);
                for (auto It = Lights->Begin(); It != Lights->End(); It++)
                    (*It)->As<Components::LineLight>()->Occlusion = nullptr;
            }
            void DepthRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("point-light-resolution"), &Renderers.PointLightResolution);
                NMake::Unpack(Node->Find("point-light-limits"), &Renderers.PointLightLimits);
                NMake::Unpack(Node->Find("spot-light-resolution"), &Renderers.SpotLightResolution);
                NMake::Unpack(Node->Find("spot-light-limits"), &Renderers.SpotLightLimits);
                NMake::Unpack(Node->Find("line-light-resolution"), &Renderers.LineLightResolution);
                NMake::Unpack(Node->Find("line-light-limits"), &Renderers.LineLightLimits);
                NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
            }
            void DepthRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("point-light-resolution"), Renderers.PointLightResolution);
                NMake::Pack(Node->SetDocument("point-light-limits"), Renderers.PointLightLimits);
                NMake::Pack(Node->SetDocument("spot-light-resolution"), Renderers.SpotLightResolution);
                NMake::Pack(Node->SetDocument("spot-light-limits"), Renderers.SpotLightLimits);
                NMake::Pack(Node->SetDocument("line-light-resolution"), Renderers.LineLightResolution);
                NMake::Pack(Node->SetDocument("line-light-limits"), Renderers.LineLightLimits);
                NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
            }
            DepthRenderer* DepthRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11DepthRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLDepthRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            ProbeRenderer::ProbeRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Probe), Surface(nullptr), Size(128), MipLevels(7)
            {
                Priority = false;
            }
            ProbeRenderer::~ProbeRenderer()
            {
                delete Surface;
            }
            void ProbeRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("size"), &Size);
                NMake::Unpack(Node->Find("mip-levels"), &MipLevels);
            }
            void ProbeRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("size"), Size);
                NMake::Pack(Node->SetDocument("mip-levels"), MipLevels);
            }
            ProbeRenderer* ProbeRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ProbeRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLProbeRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            LightRenderer::LightRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Light), RecursiveProbes(true)
            {
                Priority = true;
            }
            void LightRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("high-emission"), &AmbientLight.HighEmission);
                NMake::Unpack(Node->Find("low-emission"), &AmbientLight.LowEmission);
                NMake::Unpack(Node->Find("recursive-probes"), &RecursiveProbes);
            }
            void LightRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("high-emission"), AmbientLight.HighEmission);
                NMake::Pack(Node->SetDocument("low-emission"), AmbientLight.LowEmission);
                NMake::Pack(Node->SetDocument("recursive-probes"), RecursiveProbes);
            }
            LightRenderer* LightRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11LightRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLLightRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            ImageRenderer::ImageRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Image), RenderTarget(nullptr)
            {
            }
            ImageRenderer::ImageRenderer(RenderSystem* Lab, Graphics::RenderTarget2D* Value) : Renderer(Lab, RendererId_Image), RenderTarget(Value)
            {
                Priority = false;
            }
            ImageRenderer::~ImageRenderer()
            {
            }
            void ImageRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
            }
            void ImageRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
            }
            ImageRenderer* ImageRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ImageRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLImageRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }
            ImageRenderer* ImageRenderer::Create(RenderSystem* Lab, Graphics::RenderTarget2D* Value)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ImageRenderer(Lab, Value);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLImageRenderer(Lab, Value);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            ReflectionsRenderer::ReflectionsRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Reflections)
            {
                Priority = false;
            }
            void ReflectionsRenderer::OnInitialize()
            {
            }
            void ReflectionsRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("iteration-count"), &Render.IterationCount);
                NMake::Unpack(Node->Find("ray-count"), &Render.RayCorrection);
                NMake::Unpack(Node->Find("ray-length"), &Render.RayLength);
            }
            void ReflectionsRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("iteration-count"), Render.IterationCount);
                NMake::Pack(Node->SetDocument("ray-count"), Render.RayCorrection);
                NMake::Pack(Node->SetDocument("ray-length"), Render.RayLength);
            }
            ReflectionsRenderer* ReflectionsRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ReflectionsRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLReflectionsRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            DepthOfFieldRenderer::DepthOfFieldRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Depth_Of_Field), HorizontalResolution(512.0f), VerticalResolution(512.0f), AutoViewport(true)
            {
                Priority = false;
            }
            void DepthOfFieldRenderer::OnInitialize()
            {
            }
            void DepthOfFieldRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("horizontal-resolution"), &HorizontalResolution);
                NMake::Unpack(Node->Find("vertical-resolution"), &VerticalResolution);
                NMake::Unpack(Node->Find("auto-viewport"), &AutoViewport);
                NMake::Unpack(Node->Find("threshold"), &Render.Threshold);
                NMake::Unpack(Node->Find("gain"), &Render.Gain);
                NMake::Unpack(Node->Find("fringe"), &Render.Fringe);
                NMake::Unpack(Node->Find("bias"), &Render.Bias);
                NMake::Unpack(Node->Find("dither"), &Render.Dither);
                NMake::Unpack(Node->Find("samples"), &Render.Samples);
                NMake::Unpack(Node->Find("rings"), &Render.Rings);
                NMake::Unpack(Node->Find("far-distance"), &Render.FarDistance);
                NMake::Unpack(Node->Find("far-range"), &Render.FarRange);
                NMake::Unpack(Node->Find("near-distance"), &Render.NearDistance);
                NMake::Unpack(Node->Find("near-range"), &Render.NearRange);
                NMake::Unpack(Node->Find("focal-depth"), &Render.FocalDepth);
                NMake::Unpack(Node->Find("intensity"), &Render.Intensity);
                NMake::Unpack(Node->Find("circular"), &Render.Circular);
            }
            void DepthOfFieldRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("horizontal-resolution"), HorizontalResolution);
                NMake::Pack(Node->SetDocument("vertical-resolution"), VerticalResolution);
                NMake::Pack(Node->SetDocument("auto-viewport"), AutoViewport);
                NMake::Pack(Node->SetDocument("threshold"), Render.Threshold);
                NMake::Pack(Node->SetDocument("gain"), Render.Gain);
                NMake::Pack(Node->SetDocument("fringe"), Render.Fringe);
                NMake::Pack(Node->SetDocument("bias"), Render.Bias);
                NMake::Pack(Node->SetDocument("dither"), Render.Dither);
                NMake::Pack(Node->SetDocument("samples"), Render.Samples);
                NMake::Pack(Node->SetDocument("rings"), Render.Rings);
                NMake::Pack(Node->SetDocument("far-distance"), Render.FarDistance);
                NMake::Pack(Node->SetDocument("far-range"), Render.FarRange);
                NMake::Pack(Node->SetDocument("near-distance"), Render.NearDistance);
                NMake::Pack(Node->SetDocument("near-range"), Render.NearRange);
                NMake::Pack(Node->SetDocument("focal-depth"), Render.FocalDepth);
                NMake::Pack(Node->SetDocument("intensity"), Render.Intensity);
                NMake::Pack(Node->SetDocument("circular"), Render.Circular);
            }
            DepthOfFieldRenderer* DepthOfFieldRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11DepthOfFieldRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLDepthOfFieldRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            EmissionRenderer::EmissionRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Emission), AutoViewport(true)
            {
                Priority = false;
            }
            void EmissionRenderer::OnInitialize()
            {
            }
            void EmissionRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("auto-viewport"), &AutoViewport);
                NMake::Unpack(Node->Find("intensity"), &Render.Intensity);
                NMake::Unpack(Node->Find("threshold"), &Render.Threshold);
                NMake::Unpack(Node->Find("scaling1"), &Render.Scaling[0]);
                NMake::Unpack(Node->Find("scaling2"), &Render.Scaling[1]);
                NMake::Unpack(Node->Find("samples"), &Render.Samples);
                NMake::Unpack(Node->Find("sample-count"), &Render.SampleCount);
            }
            void EmissionRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("auto-viewport"), AutoViewport);
                NMake::Pack(Node->SetDocument("intensity"), Render.Intensity);
                NMake::Pack(Node->SetDocument("threshold"), Render.Threshold);
                NMake::Pack(Node->SetDocument("scaling1"), Render.Scaling[0]);
                NMake::Pack(Node->SetDocument("scaling2"), Render.Scaling[1]);
                NMake::Pack(Node->SetDocument("samples"), Render.Samples);
                NMake::Pack(Node->SetDocument("sample-count"), Render.SampleCount);
            }
            EmissionRenderer* EmissionRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11EmissionRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLEmissionRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            GlitchRenderer::GlitchRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Glitch), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
            {
                Priority = false;
            }
            void GlitchRenderer::OnInitialize()
            {
            }
            void GlitchRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("scanline-jitter"), &ScanLineJitter);
                NMake::Unpack(Node->Find("vertical-jump"), &VerticalJump);
                NMake::Unpack(Node->Find("horizontal-shake"), &HorizontalShake);
                NMake::Unpack(Node->Find("color-drift"), &ColorDrift);
                NMake::Unpack(Node->Find("horizontal-shake"), &HorizontalShake);
                NMake::Unpack(Node->Find("elapsed-time"), &Render.ElapsedTime);
                NMake::Unpack(Node->Find("scanline-jitter-displacement"), &Render.ScanLineJitterDisplacement);
                NMake::Unpack(Node->Find("scanline-jitter-threshold"), &Render.ScanLineJitterThreshold);
                NMake::Unpack(Node->Find("vertical-jump-amount"), &Render.VerticalJumpAmount);
                NMake::Unpack(Node->Find("vertical-jump-time"), &Render.VerticalJumpTime);
                NMake::Unpack(Node->Find("color-drift-amount"), &Render.ColorDriftAmount);
                NMake::Unpack(Node->Find("color-drift-time"), &Render.ColorDriftTime);
            }
            void GlitchRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("scanline-jitter"), ScanLineJitter);
                NMake::Pack(Node->SetDocument("vertical-jump"), VerticalJump);
                NMake::Pack(Node->SetDocument("horizontal-shake"), HorizontalShake);
                NMake::Pack(Node->SetDocument("color-drift"), ColorDrift);
                NMake::Pack(Node->SetDocument("horizontal-shake"), HorizontalShake);
                NMake::Pack(Node->SetDocument("elapsed-time"), Render.ElapsedTime);
                NMake::Pack(Node->SetDocument("scanline-jitter-displacement"), Render.ScanLineJitterDisplacement);
                NMake::Pack(Node->SetDocument("scanline-jitter-threshold"), Render.ScanLineJitterThreshold);
                NMake::Pack(Node->SetDocument("vertical-jump-amount"), Render.VerticalJumpAmount);
                NMake::Pack(Node->SetDocument("vertical-jump-time"), Render.VerticalJumpTime);
                NMake::Pack(Node->SetDocument("color-drift-amount"), Render.ColorDriftAmount);
                NMake::Pack(Node->SetDocument("color-drift-time"), Render.ColorDriftTime);
            }
            GlitchRenderer* GlitchRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11GlitchRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLGlitchRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            AmbientOcclusionRenderer::AmbientOcclusionRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Ambient_Occlusion)
            {
                Priority = false;
            }
            void AmbientOcclusionRenderer::OnInitialize()
            {
            }
            void AmbientOcclusionRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("scale"), &Render.Scale);
                NMake::Unpack(Node->Find("intensity"), &Render.Intensity);
                NMake::Unpack(Node->Find("bias"), &Render.Bias);
                NMake::Unpack(Node->Find("radius"), &Render.Radius);
                NMake::Unpack(Node->Find("step"), &Render.Step);
                NMake::Unpack(Node->Find("offset"), &Render.Offset);
                NMake::Unpack(Node->Find("distance"), &Render.Distance);
                NMake::Unpack(Node->Find("fading"), &Render.Fading);
                NMake::Unpack(Node->Find("power"), &Render.Power);
                NMake::Unpack(Node->Find("samples"), &Render.Samples);
                NMake::Unpack(Node->Find("sample-count"), &Render.SampleCount);
            }
            void AmbientOcclusionRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("scale"), Render.Scale);
                NMake::Pack(Node->SetDocument("intensity"), Render.Intensity);
                NMake::Pack(Node->SetDocument("bias"), Render.Bias);
                NMake::Pack(Node->SetDocument("radius"), Render.Radius);
                NMake::Pack(Node->SetDocument("step"), Render.Step);
                NMake::Pack(Node->SetDocument("offset"), Render.Offset);
                NMake::Pack(Node->SetDocument("distance"), Render.Distance);
                NMake::Pack(Node->SetDocument("fading"), Render.Fading);
                NMake::Pack(Node->SetDocument("power"), Render.Power);
                NMake::Pack(Node->SetDocument("samples"), Render.Samples);
                NMake::Pack(Node->SetDocument("sample-count"), Render.SampleCount);
            }
            AmbientOcclusionRenderer* AmbientOcclusionRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11AmbientOcclusionRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLAmbientOcclusionRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            IndirectOcclusionRenderer::IndirectOcclusionRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Indirect_Occlusion)
            {
                Priority = false;
            }
            void IndirectOcclusionRenderer::OnInitialize()
            {
            }
            void IndirectOcclusionRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("scale"), &Render.Scale);
                NMake::Unpack(Node->Find("intensity"), &Render.Intensity);
                NMake::Unpack(Node->Find("bias"), &Render.Bias);
                NMake::Unpack(Node->Find("radius"), &Render.Radius);
                NMake::Unpack(Node->Find("step"), &Render.Step);
                NMake::Unpack(Node->Find("offset"), &Render.Offset);
                NMake::Unpack(Node->Find("distance"), &Render.Distance);
                NMake::Unpack(Node->Find("fading"), &Render.Fading);
                NMake::Unpack(Node->Find("power"), &Render.Power);
                NMake::Unpack(Node->Find("samples"), &Render.Samples);
                NMake::Unpack(Node->Find("sample-count"), &Render.SampleCount);
            }
            void IndirectOcclusionRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("scale"), Render.Scale);
                NMake::Pack(Node->SetDocument("intensity"), Render.Intensity);
                NMake::Pack(Node->SetDocument("bias"), Render.Bias);
                NMake::Pack(Node->SetDocument("radius"), Render.Radius);
                NMake::Pack(Node->SetDocument("step"), Render.Step);
                NMake::Pack(Node->SetDocument("offset"), Render.Offset);
                NMake::Pack(Node->SetDocument("distance"), Render.Distance);
                NMake::Pack(Node->SetDocument("fading"), Render.Fading);
                NMake::Pack(Node->SetDocument("power"), Render.Power);
                NMake::Pack(Node->SetDocument("samples"), Render.Samples);
                NMake::Pack(Node->SetDocument("sample-count"), Render.SampleCount);
            }
            IndirectOcclusionRenderer* IndirectOcclusionRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11IndirectOcclusionRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLIndirectOcclusionRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            ToneRenderer::ToneRenderer(RenderSystem* Lab) : Renderer(Lab, RendererId_Tone)
            {
                Priority = false;
            }
            void ToneRenderer::OnInitialize()
            {
            }
            void ToneRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Unpack(Node->Find("blind-vision-r"), &Render.BlindVisionR);
                NMake::Unpack(Node->Find("blind-vision-g"), &Render.BlindVisionG);
                NMake::Unpack(Node->Find("blind-vision-b"), &Render.BlindVisionB);
                NMake::Unpack(Node->Find("vignette-color"), &Render.VignetteColor);
                NMake::Unpack(Node->Find("color-gamma"), &Render.ColorGamma);
                NMake::Unpack(Node->Find("desaturation-gamma"), &Render.DesaturationGamma);
                NMake::Unpack(Node->Find("vignette-amount"), &Render.VignetteAmount);
                NMake::Unpack(Node->Find("vignette-curve"), &Render.VignetteCurve);
                NMake::Unpack(Node->Find("vignette-radius"), &Render.VignetteRadius);
                NMake::Unpack(Node->Find("linear-intensity"), &Render.LinearIntensity);
                NMake::Unpack(Node->Find("gamma-intensity"), &Render.GammaIntensity);
                NMake::Unpack(Node->Find("desaturation-intensity"), &Render.DesaturationIntensity);
            }
            void ToneRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("blind-vision-r"), Render.BlindVisionR);
                NMake::Pack(Node->SetDocument("blind-vision-g"), Render.BlindVisionG);
                NMake::Pack(Node->SetDocument("blind-vision-b"), Render.BlindVisionB);
                NMake::Pack(Node->SetDocument("vignette-color"), Render.VignetteColor);
                NMake::Pack(Node->SetDocument("color-gamma"), Render.ColorGamma);
                NMake::Pack(Node->SetDocument("desaturation-gamma"), Render.DesaturationGamma);
                NMake::Pack(Node->SetDocument("vignette-amount"), Render.VignetteAmount);
                NMake::Pack(Node->SetDocument("vignette-curve"), Render.VignetteCurve);
                NMake::Pack(Node->SetDocument("vignette-radius"), Render.VignetteRadius);
                NMake::Pack(Node->SetDocument("linear-intensity"), Render.LinearIntensity);
                NMake::Pack(Node->SetDocument("gamma-intensity"), Render.GammaIntensity);
                NMake::Pack(Node->SetDocument("desaturation-intensity"), Render.DesaturationIntensity);
            }
            ToneRenderer* ToneRenderer::Create(RenderSystem* Lab)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11ToneRenderer(Lab);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLToneRenderer(Lab);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }

            GUIRenderer::GUIRenderer(RenderSystem* Lab, Graphics::Activity* NewActivity) : Renderer(Lab, RendererId_GUI), Activity(NewActivity), ClipboardTextData(nullptr), Context(nullptr)
            {
                Priority = false;
            }
            GUIRenderer::~GUIRenderer()
            {
            }
            void GUIRenderer::OnRender(Rest::Timer* Timer)
            {
                auto* App = Engine::Application::Get();
                if (!App && !Callback)
                    return;

#ifdef THAWK_HAS_SDL2
                int DW, DH, W, H;
                SDL_GetWindowSize(Activity->GetHandle(), &W, &H);
                SDL_GL_GetDrawableSize(Activity->GetHandle(), &DW, &DH);

                ImGuiIO* Settings = &ImGui::GetIO();
                Settings->DisplaySize = ImVec2((float)W, (float)H);
                WorldViewProjection.Row[0] = 2.0f / Settings->DisplaySize.x;
                WorldViewProjection.Row[1] = 0.0f;
                WorldViewProjection.Row[2] = 0.0f;
                WorldViewProjection.Row[3] = 0.0f;
                WorldViewProjection.Row[4] = 0.0f;
                WorldViewProjection.Row[5] = 2.0f / -Settings->DisplaySize.y;
                WorldViewProjection.Row[6] = 0.0f;
                WorldViewProjection.Row[7] = 0.0f;
                WorldViewProjection.Row[8] = 0.0f;
                WorldViewProjection.Row[9] = 0.0f;
                WorldViewProjection.Row[10] = 0.5f;
                WorldViewProjection.Row[11] = 0.0f;
                WorldViewProjection.Row[12] = -1;
                WorldViewProjection.Row[13] = 1;
                WorldViewProjection.Row[14] = 0.5f;
                WorldViewProjection.Row[15] = 1.0f;

                if (W > 0 && H > 0)
                    Settings->DisplayFramebufferScale = ImVec2((float)DW / W, (float)DH / H);

                Uint64 CurrentTimeDate = SDL_GetPerformanceCounter();
                Settings->DeltaTime = Time > 0 ? (float)((double)(CurrentTimeDate - Time) / (Uint64)Frequency) : (float)(1.0f / 60.0f);
                Time = CurrentTimeDate;

                if (Settings->WantSetMousePos)
                    SDL_WarpMouseInWindow(Activity->GetHandle(), (int)Settings->MousePos.x, (int)Settings->MousePos.y);
                else
                    Settings->MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

                int X, Y;
                Uint32 Buttons = SDL_GetMouseState(&X, &Y);
                Settings->MouseDown[0] = (Buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
                Settings->MouseDown[1] = (Buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
                Settings->MouseDown[2] = (Buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;

                if (SDL_GetWindowFlags(Activity->GetHandle()) & SDL_WINDOW_INPUT_FOCUS)
                {
                    Settings->MousePos = ImVec2((float)X, (float)Y);
                    if (AllowMouseOffset)
                    {
                        Settings->MousePos.x *= Activity->GetOffset().X;
                        Settings->MousePos.y *= Activity->GetOffset().Y;
                    }
                }

                if (!(Settings->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
                {
                    ImGuiMouseCursor Cursor = ImGui::GetMouseCursor();
                    if (!Settings->MouseDrawCursor && Cursor != ImGuiMouseCursor_None)
                    {
                        Activity->SetCursor((Graphics::DisplayCursor)Cursor);
                        SDL_ShowCursor(SDL_TRUE);
                    }
                    else
                        SDL_ShowCursor(SDL_FALSE);
                }

                memset(Settings->NavInputs, 0, sizeof(Settings->NavInputs));
                if (Settings->ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)
                {
                    SDL_GameController* Controller = SDL_GameControllerOpen(0);
                    if (Controller != nullptr)
                    {
                        const int ThumbDeadZone = 8000;
                        MAP_BUTTON(ImGuiNavInput_Activate, SDL_CONTROLLER_BUTTON_A);
                        MAP_BUTTON(ImGuiNavInput_Cancel, SDL_CONTROLLER_BUTTON_B);
                        MAP_BUTTON(ImGuiNavInput_Menu, SDL_CONTROLLER_BUTTON_X);
                        MAP_BUTTON(ImGuiNavInput_Input, SDL_CONTROLLER_BUTTON_Y);
                        MAP_BUTTON(ImGuiNavInput_DpadLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                        MAP_BUTTON(ImGuiNavInput_DpadRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
                        MAP_BUTTON(ImGuiNavInput_DpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP);
                        MAP_BUTTON(ImGuiNavInput_DpadDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                        MAP_BUTTON(ImGuiNavInput_FocusPrev, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                        MAP_BUTTON(ImGuiNavInput_FocusNext, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                        MAP_BUTTON(ImGuiNavInput_TweakSlow, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                        MAP_BUTTON(ImGuiNavInput_TweakFast, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                        MAP_ANALOG(ImGuiNavInput_LStickLeft, SDL_CONTROLLER_AXIS_LEFTX, -ThumbDeadZone, -32768);
                        MAP_ANALOG(ImGuiNavInput_LStickRight, SDL_CONTROLLER_AXIS_LEFTX, +ThumbDeadZone, +32767);
                        MAP_ANALOG(ImGuiNavInput_LStickUp, SDL_CONTROLLER_AXIS_LEFTY, -ThumbDeadZone, -32767);
                        MAP_ANALOG(ImGuiNavInput_LStickDown, SDL_CONTROLLER_AXIS_LEFTY, +ThumbDeadZone, +32767);

                        Settings->BackendFlags |= ImGuiBackendFlags_HasGamepad;
                    }
                    else
                        Settings->BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
                }
#endif
                ImGui::NewFrame();
                if (App != nullptr)
                    App->OnInteract(this);

                if (Callback)
                    Callback(this);

                ImGui::Render();
            }
            void GUIRenderer::SetRenderCallback(const GUI::RendererCallback& NewCallback)
            {
                Callback = NewCallback;
            }
            void GUIRenderer::Transform(const Compute::Matrix4x4& In)
            {
                WorldViewProjection = In;
            }
            void GUIRenderer::Reset()
            {
                ImGuiIO& Input = ImGui::GetIO();
                Input.IniFilename = "";
                Input.UserData = this;
                Input.MouseDrawCursor = true;
                Input.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
                Input.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
                Input.BackendPlatformName = "";
                Input.ClipboardUserData = this;

#ifdef THAWK_HAS_SDL2
                Input.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
                Input.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
                Input.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
                Input.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
                Input.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
                Input.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
                Input.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
                Input.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
                Input.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
                Input.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
                Input.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
                Input.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
                Input.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
                Input.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
                Input.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
                Input.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_RETURN2;
                Input.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
                Input.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
                Input.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
                Input.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
                Input.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
                Input.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
                Input.SetClipboardTextFn = [](void*, const char* Data)
                {
                    SDL_SetClipboardText(Data);
                };
                Input.GetClipboardTextFn = [](void* C) -> const char*
                {
                    return ((GUIRenderer*)C)->GetClipboardCopy();
                };
                Frequency = (UInt64)SDL_GetPerformanceFrequency();

                SDL_SysWMinfo Info;
                Activity->Load(&Info);
#ifdef THAWK_MICROSOFT
                Input.ImeWindowHandle = (void*)Info.info.win.window;
#elif defined(THAWK_APPLE)
                Input.ImeWindowHandle = (void*)Info.info.cocoa.window;
#elif defined(THAWK_UNIX)
                Input.ImeWindowHandle = (void*)Info.info.x11.window;
#endif
#endif

                ImGuiStyle* Style = &ImGui::GetStyle();
                Style->WindowPadding = ImVec2(10, 10);
                Style->WindowRounding = 0;
                Style->FramePadding = ImVec2(4, 5);
                Style->FrameRounding = 2;
                Style->FrameBorderSize = 1;
                Style->ItemSpacing = ImVec2(10, 10);
                Style->ItemInnerSpacing = ImVec2(10, 10);
                Style->TouchExtraPadding = ImVec2(0, 0);
                Style->IndentSpacing = 20;
                Style->ScrollbarSize = 20;
                Style->ScrollbarRounding = 0;
                Style->GrabMinSize = 15;
                Style->GrabRounding = 2;
                Style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
                Style->ButtonTextAlign = ImVec2(0.5f, 0.5f);
                Style->DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
                Style->DisplayWindowPadding = ImVec2(0.0f, 0.0f);
                Style->Colors[ImGuiCol_WindowBg] = ImVec4(0.062745f, 0.062745f, 0.062745f, 1.00f);
                Style->Colors[ImGuiCol_ChildBg] = ImVec4(0.082352f, 0.082352f, 0.082352f, 1.00f);
                Style->Colors[ImGuiCol_PopupBg] = ImVec4(0.062745f, 0.062745f, 0.062745f, 1.00f);
                Style->Colors[ImGuiCol_FrameBg] = ImVec4(0.176470f, 0.176470f, 0.176470f, 1.00f);
                Style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.133333f, 0.133333f, 0.133333f, 1.00f);
                Style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.141176f, 0.141176f, 0.141176f, 1.00f);
                Style->Colors[ImGuiCol_TitleBg] = ImVec4(0.090196f, 0.090196f, 0.090196f, 1.00f);
                Style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.090196f, 0.090196f, 0.090196f, 0.75f);
                Style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.090196f, 0.090196f, 0.090196f, 1.00f);
                Style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.090196f, 0.090196f, 0.090196f, 1.00f);
                Style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.082352f, 0.082352f, 0.082352f, 1.00f);
                Style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.125490f, 0.125490f, 0.125490f, 1.00f);
                Style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.176470f, 0.176470f, 0.176470f, 1.00f);
                Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.133333f, 0.133333f, 0.133333f, 1.00f);
                Style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.141176f, 0.141176f, 0.141176f, 1.00f);
                Style->Colors[ImGuiCol_Text] = ImVec4(1.000000f, 1.000000f, 1.000000f, 1.00f);
                Style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.533333f, 0.533333f, 0.533333f, 1.00f);
                Style->Colors[ImGuiCol_Border] = ImVec4(0.062745f, 0.062745f, 0.062745f, 1.00f);
                Style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.000000f, 0.000000f, 0.000000f, 0.00f);
                Style->Colors[ImGuiCol_CheckMark] = ImVec4(1.000000f, 1.000000f, 1.000000f, 1.00f);
                Style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.176470f, 0.176470f, 0.176470f, 1.00f);
                Style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.141176f, 0.141176f, 0.141176f, 1.00f);
                Style->Colors[ImGuiCol_Button] = ImVec4(0.176470f, 0.176470f, 0.176470f, 1.00f);
                Style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.133333f, 0.133333f, 0.133333f, 1.00f);
                Style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.062745f, 0.062745f, 0.062745f, 1.00f);
                Style->Colors[ImGuiCol_Header] = ImVec4(0.176470f, 0.176470f, 0.176470f, 1.00f);
                Style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.133333f, 0.133333f, 0.133333f, 1.00f);
                Style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.141176f, 0.141176f, 0.141176f, 1.00f);
                Style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.176470f, 0.176470f, 0.176470f, 0.00f);
                Style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.133333f, 0.133333f, 0.133333f, 1.00f);
                Style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.082352f, 0.082352f, 0.082352f, 1.00f);
                Style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
                Style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
                Style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
                Style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
                Style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(-1.00f, -1.00f, -1.00f, 0.25f);
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            }
            void GUIRenderer::Deactivate()
            {
                ImGui::GetIO().UserData = nullptr;
                ImGui::SetCurrentContext(nullptr);
            }
            void GUIRenderer::Activate()
            {
#ifdef THAWK_HAS_SDL2
                SDL_SysWMinfo Info;
                Activity->Load(&Info);
#ifdef THAWK_MICROSOFT
                ImGui::GetIO().ImeWindowHandle = (void*)Info.info.win.window;
#elif defined(THAWK_APPLE)
                ImGui::GetIO().ImeWindowHandle = (void*)Info.info.cocoa.window;
#elif defined(THAWK_UNIX)
                ImGui::GetIO().ImeWindowHandle = (void*)Info.info.x11.window;
#endif
#endif
                ImGui::GetIO().UserData = this;
                ImGui::SetCurrentContext((ImGuiContext*)Context);
            }
            void* GUIRenderer::GetUi()
            {
                return (void*)&ImGui::GetIO();
            }
            const char* GUIRenderer::GetClipboardCopy()
            {
#ifdef THAWK_HAS_SDL2
                if (ClipboardTextData)
                    SDL_free(ClipboardTextData);

                ClipboardTextData = SDL_GetClipboardText();
                return ClipboardTextData;
#else
                return nullptr;
#endif
            }
            Compute::Matrix4x4& GUIRenderer::GetTransform()
            {
                return WorldViewProjection;
            }
            Graphics::Activity* GUIRenderer::GetActivity()
            {
                return Activity;
            }
            GUI::Interface* GUIRenderer::GetTree()
            {
                return &Tree;
            }
            GUIRenderer* GUIRenderer::Create(RenderSystem* Lab, Graphics::Activity* Window)
            {
#ifdef THAWK_MICROSOFT
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
                    return new Graphics::D3D11::D3D11GUIRenderer(Lab, Window);
#endif
#ifdef THAWK_HAS_GL
                if (Lab && Lab->GetDevice() && Lab->GetDevice()->GetBackend() == Graphics::RenderBackend_OGL)
                    return new Graphics::OGL::OGLGUIRenderer(Lab, Window);
#endif
                THAWK_ERROR("instance serialization wasn't found");
                return nullptr;
            }
        }
    }
}
