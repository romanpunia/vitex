#ifndef THAWK_ENGINE_RENDERERS_H
#define THAWK_ENGINE_RENDERERS_H

#include "../engine.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			class GUIRenderer;

			namespace GUI
			{
				enum WindowFlags
				{
					WindowFlags_None = 0,
					WindowFlags_NoTitleBar = 1 << 0,
					WindowFlags_NoResize = 1 << 1,
					WindowFlags_NoMove = 1 << 2,
					WindowFlags_NoScrollbar = 1 << 3,
					WindowFlags_NoScrollWithMouse = 1 << 4,
					WindowFlags_NoCollapse = 1 << 5,
					WindowFlags_AlwaysAutoResize = 1 << 6,
					WindowFlags_NoSavedSettings = 1 << 8,
					WindowFlags_NoInputs = 1 << 9,
					WindowFlags_MenuBar = 1 << 10,
					WindowFlags_HorizontalScrollbar = 1 << 11,
					WindowFlags_NoFocusOnAppearing = 1 << 12,
					WindowFlags_NoBringToFrontOnFocus = 1 << 13,
					WindowFlags_AlwaysVerticalScrollbar = 1 << 14,
					WindowFlags_AlwaysHorizontalScrollbar = 1 << 15,
					WindowFlags_AlwaysUseWindowPadding = 1 << 16,
					WindowFlags_NoNavInputs = 1 << 18,
					WindowFlags_NoNavFocus = 1 << 19,
					WindowFlags_NoNav = WindowFlags_NoNavInputs | WindowFlags_NoNavFocus,
					WindowFlags_NavFlattened = 1 << 23,
					WindowFlags_ChildWindow = 1 << 24,
					WindowFlags_Tooltip = 1 << 25,
					WindowFlags_Popup = 1 << 26,
					WindowFlags_Modal = 1 << 27,
					WindowFlags_ChildMenu = 1 << 28
				};

				enum InputTextFlags
				{
					InputTextFlags_None = 0,
					InputTextFlags_CharsDecimal = 1 << 0,
					InputTextFlags_CharsHexadecimal = 1 << 1,
					InputTextFlags_CharsUppercase = 1 << 2,
					InputTextFlags_CharsNoBlank = 1 << 3,
					InputTextFlags_AutoSelectAll = 1 << 4,
					InputTextFlags_EnterReturnsTrue = 1 << 5,
					InputTextFlags_CallbackCompletion = 1 << 6,
					InputTextFlags_CallbackHistory = 1 << 7,
					InputTextFlags_CallbackAlways = 1 << 8,
					InputTextFlags_CallbackCharFilter = 1 << 9,
					InputTextFlags_AllowTabInput = 1 << 10,
					InputTextFlags_CtrlEnterForNewLine = 1 << 11,
					InputTextFlags_NoHorizontalScroll = 1 << 12,
					InputTextFlags_AlwaysInsertMode = 1 << 13,
					InputTextFlags_ReadOnly = 1 << 14,
					InputTextFlags_Password = 1 << 15,
					InputTextFlags_NoUndoRedo = 1 << 16,
					InputTextFlags_CharsScientific = 1 << 17,
					InputTextFlags_CallbackResize = 1 << 18,
					InputTextFlags_Multiline = 1 << 20
				};

				enum TreeNodeFlags
				{
					TreeNodeFlags_None = 0,
					TreeNodeFlags_Selected = 1 << 0,
					TreeNodeFlags_Framed = 1 << 1,
					TreeNodeFlags_AllowItemOverlap = 1 << 2,
					TreeNodeFlags_NoTreePushOnOpen = 1 << 3,
					TreeNodeFlags_NoAutoOpenOnLog = 1 << 4,
					TreeNodeFlags_DefaultOpen = 1 << 5,
					TreeNodeFlags_OpenOnDoubleClick = 1 << 6,
					TreeNodeFlags_OpenOnArrow = 1 << 7,
					TreeNodeFlags_Leaf = 1 << 8,
					TreeNodeFlags_Bullet = 1 << 9,
					TreeNodeFlags_FramePadding = 1 << 10,
					TreeNodeFlags_NavLeftJumpsBackHere = 1 << 13,
					TreeNodeFlags_CollapsingHeader = TreeNodeFlags_Framed | TreeNodeFlags_NoTreePushOnOpen | TreeNodeFlags_NoAutoOpenOnLog
				};

				enum SelectableFlags
				{
					SelectableFlags_None = 0,
					SelectableFlags_DontClosePopups = 1 << 0,
					SelectableFlags_SpanAllColumns = 1 << 1,
					SelectableFlags_AllowDoubleClick = 1 << 2,
					SelectableFlags_Disabled = 1 << 3
				};

				enum ComboFlags
				{
					ComboFlags_None = 0,
					ComboFlags_PopupAlignLeft = 1 << 0,
					ComboFlags_HeightSmall = 1 << 1,
					ComboFlags_HeightRegular = 1 << 2,
					ComboFlags_HeightLarge = 1 << 3,
					ComboFlags_HeightLargest = 1 << 4,
					ComboFlags_NoArrowButton = 1 << 5,
					ComboFlags_NoPreview = 1 << 6,
					ComboFlags_HeightMask_ = ComboFlags_HeightSmall | ComboFlags_HeightRegular | ComboFlags_HeightLarge | ComboFlags_HeightLargest
				};

				enum FocusedFlags
				{
					FocusedFlags_None = 0,
					FocusedFlags_ChildWindows = 1 << 0,
					FocusedFlags_RootWindow = 1 << 1,
					FocusedFlags_AnyWindow = 1 << 2,
					FocusedFlags_RootAndChildWindows = FocusedFlags_RootWindow | FocusedFlags_ChildWindows
				};

				enum HoveredFlags
				{
					HoveredFlags_None = 0,
					HoveredFlags_ChildWindows = 1 << 0,
					HoveredFlags_RootWindow = 1 << 1,
					HoveredFlags_AnyWindow = 1 << 2,
					HoveredFlags_AllowWhenBlockedByPopup = 1 << 3,
					HoveredFlags_AllowWhenBlockedByActiveItem = 1 << 5,
					HoveredFlags_AllowWhenOverlapped = 1 << 6,
					HoveredFlags_AllowWhenDisabled = 1 << 7,
					HoveredFlags_RectOnly = HoveredFlags_AllowWhenBlockedByPopup | HoveredFlags_AllowWhenBlockedByActiveItem | HoveredFlags_AllowWhenOverlapped,
					HoveredFlags_RootAndChildWindows = HoveredFlags_RootWindow | HoveredFlags_ChildWindows
				};

				enum DragDropFlags
				{
					DragDropFlags_None = 0,
					DragDropFlags_SourceNoPreviewTooltip = 1 << 0,
					DragDropFlags_SourceNoDisableHover = 1 << 1,
					DragDropFlags_SourceNoHoldToOpenOthers = 1 << 2,
					DragDropFlags_SourceAllowNullID = 1 << 3,
					DragDropFlags_SourceExtern = 1 << 4,
					DragDropFlags_SourceAutoExpirePayload = 1 << 5,
					DragDropFlags_AcceptBeforeDelivery = 1 << 10,
					DragDropFlags_AcceptNoDrawDefaultRect = 1 << 11,
					DragDropFlags_AcceptNoPreviewTooltip = 1 << 12,
					DragDropFlags_AcceptPeekOnly = DragDropFlags_AcceptBeforeDelivery | DragDropFlags_AcceptNoDrawDefaultRect
				};

				enum DataType
				{
					DataType_S32,
					DataType_U32,
					DataType_S64,
					DataType_U64,
					DataType_Float,
					DataType_Double,
					DataType_Count
				};

				enum Direction
				{
					Direction_None = -1,
					Direction_Left = 0,
					Direction_Right = 1,
					Direction_Up = 2,
					Direction_Down = 3,
					Direction_Count
				};

				enum NavInput
				{
					NavInput_Activate,
					NavInput_Cancel,
					NavInput_Input,
					NavInput_Menu,
					NavInput_DpadLeft,
					NavInput_DpadRight,
					NavInput_DpadUp,
					NavInput_DpadDown,
					NavInput_LStickLeft,
					NavInput_LStickRight,
					NavInput_LStickUp,
					NavInput_LStickDown,
					NavInput_FocusPrev,
					NavInput_FocusNext,
					NavInput_TweakSlow,
					NavInput_TweakFast,
					NavInput_KeyMenu_,
					NavInput_KeyLeft_,
					NavInput_KeyRight_,
					NavInput_KeyUp_,
					NavInput_KeyDown_,
					NavInput_Count,
					NavInput_InternalStart_ = NavInput_KeyMenu_
				};

				enum ConfigFlags
				{
					ConfigFlags_NavEnableKeyboard = 1 << 0,
					ConfigFlags_NavEnableGamepad = 1 << 1,
					ConfigFlags_NavEnableSetMousePos = 1 << 2,
					ConfigFlags_NavNoCaptureKeyboard = 1 << 3,
					ConfigFlags_NoMouse = 1 << 4,
					ConfigFlags_NoMouseCursorChange = 1 << 5,
					ConfigFlags_IsSRGB = 1 << 20,
					ConfigFlags_IsTouchScreen = 1 << 21
				};

				enum DisplayKey
				{
					DisplayKey_Tab,
					DisplayKey_LeftArrow,
					DisplayKey_RightArrow,
					DisplayKey_UpArrow,
					DisplayKey_DownArrow,
					DisplayKey_PageUp,
					DisplayKey_PageDown,
					DisplayKey_Home,
					DisplayKey_End,
					DisplayKey_Insert,
					DisplayKey_Delete,
					DisplayKey_Backspace,
					DisplayKey_Space,
					DisplayKey_Enter,
					DisplayKey_Escape,
					DisplayKey_A,
					DisplayKey_C,
					DisplayKey_V,
					DisplayKey_X,
					DisplayKey_Y,
					DisplayKey_Z,
					DisplayKey_Count
				};

				enum DisplayFeatures
				{
					DisplayFeatures_HasGamepad = 1 << 0,
					DisplayFeatures_HasMouseCursors = 1 << 1,
					DisplayFeatures_HasSetMousePos = 1 << 2
				};

				enum ColorValue
				{
					ColorValue_Text,
					ColorValue_TextDisabled,
					ColorValue_WindowBg,
					ColorValue_ChildBg,
					ColorValue_PopupBg,
					ColorValue_Border,
					ColorValue_BorderShadow,
					ColorValue_FrameBg,
					ColorValue_FrameBgHovered,
					ColorValue_FrameBgActive,
					ColorValue_TitleBg,
					ColorValue_TitleBgActive,
					ColorValue_TitleBgCollapsed,
					ColorValue_MenuBarBg,
					ColorValue_ScrollbarBg,
					ColorValue_ScrollbarGrab,
					ColorValue_ScrollbarGrabHovered,
					ColorValue_ScrollbarGrabActive,
					ColorValue_CheckMark,
					ColorValue_SliderGrab,
					ColorValue_SliderGrabActive,
					ColorValue_Button,
					ColorValue_ButtonHovered,
					ColorValue_ButtonActive,
					ColorValue_Header,
					ColorValue_HeaderHovered,
					ColorValue_HeaderActive,
					ColorValue_Separator,
					ColorValue_SeparatorHovered,
					ColorValue_SeparatorActive,
					ColorValue_ResizeGrip,
					ColorValue_ResizeGripHovered,
					ColorValue_ResizeGripActive,
					ColorValue_PlotLines,
					ColorValue_PlotLinesHovered,
					ColorValue_PlotHistogram,
					ColorValue_PlotHistogramHovered,
					ColorValue_TextSelectedBg,
					ColorValue_DragDropTarget,
					ColorValue_NavHighlight,
					ColorValue_NavWindowingHighlight,
					ColorValue_NavWindowingDimBg,
					ColorValue_ModalWindowDimBg,
					ColorValue_Count
				};

				enum StyleValue
				{
					StyleValue_Alpha,
					StyleValue_WindowPadding,
					StyleValue_WindowRounding,
					StyleValue_WindowBorderSize,
					StyleValue_WindowMinSize,
					StyleValue_WindowTitleAlign,
					StyleValue_ChildRounding,
					StyleValue_ChildBorderSize,
					StyleValue_PopupRounding,
					StyleValue_PopupBorderSize,
					StyleValue_FramePadding,
					StyleValue_FrameRounding,
					StyleValue_FrameBorderSize,
					StyleValue_ItemSpacing,
					StyleValue_ItemInnerSpacing,
					StyleValue_IndentSpacing,
					StyleValue_ScrollbarSize,
					StyleValue_ScrollbarRounding,
					StyleValue_GrabMinSize,
					StyleValue_GrabRounding,
					StyleValue_ButtonTextAlign,
					StyleValue_Count
				};

				enum ColorEditFlags
				{
					ColorEditFlags_None = 0,
					ColorEditFlags_NoAlpha = 1 << 1,
					ColorEditFlags_NoPicker = 1 << 2,
					ColorEditFlags_NoOptions = 1 << 3,
					ColorEditFlags_NoSmallPreview = 1 << 4,
					ColorEditFlags_NoInputs = 1 << 5,
					ColorEditFlags_NoTooltip = 1 << 6,
					ColorEditFlags_NoLabel = 1 << 7,
					ColorEditFlags_NoSidePreview = 1 << 8,
					ColorEditFlags_NoDragDrop = 1 << 9,
					ColorEditFlags_AlphaBar = 1 << 16,
					ColorEditFlags_AlphaPreview = 1 << 17,
					ColorEditFlags_AlphaPreviewHalf = 1 << 18,
					ColorEditFlags_HDR = 1 << 19,
					ColorEditFlags_RGB = 1 << 20,
					ColorEditFlags_HSV = 1 << 21,
					ColorEditFlags_HEX = 1 << 22,
					ColorEditFlags_Uint8 = 1 << 23,
					ColorEditFlags_Float = 1 << 24,
					ColorEditFlags_PickerHueBar = 1 << 25,
					ColorEditFlags_PickerHueWheel = 1 << 26,
					ColorEditFlags__InputsMask = ColorEditFlags_RGB | ColorEditFlags_HSV | ColorEditFlags_HEX,
					ColorEditFlags__DataTypeMask = ColorEditFlags_Uint8 | ColorEditFlags_Float,
					ColorEditFlags__PickerMask = ColorEditFlags_PickerHueWheel | ColorEditFlags_PickerHueBar,
					ColorEditFlags__OptionsDefault = ColorEditFlags_Uint8 | ColorEditFlags_RGB | ColorEditFlags_PickerHueBar
				};

				enum RenderCond
				{
					RenderCond_Always = 1 << 0,
					RenderCond_Once = 1 << 1,
					RenderCond_FirstUseEver = 1 << 2,
					RenderCond_Appearing = 1 << 3
				};

				enum CornerFlags
				{
					CornerFlags_TopLeft = 1 << 0,
					CornerFlags_TopRight = 1 << 1,
					CornerFlags_BotLeft = 1 << 2,
					CornerFlags_BotRight = 1 << 3,
					CornerFlags_Top = CornerFlags_TopLeft | CornerFlags_TopRight,
					CornerFlags_Bot = CornerFlags_BotLeft | CornerFlags_BotRight,
					CornerFlags_Left = CornerFlags_TopLeft | CornerFlags_BotLeft,
					CornerFlags_Right = CornerFlags_TopRight | CornerFlags_BotRight,
					CornerFlags_All = 0xF
				};

				enum ListFlags
				{
					ListFlags_AntiAliasedLines = 1 << 0,
					ListFlags_AntiAliasedFill = 1 << 1
				};

				struct THAWK_OUT Style
				{
					float Alpha = 0;
					Compute::Vector2 WindowPadding;
					float WindowRounding = 0;
					float WindowBorderSize = 0;
					Compute::Vector2 WindowMinSize;
					Compute::Vector2 WindowTitleAlign;
					float ChildRounding = 0;
					float ChildBorderSize = 0;
					float PopupRounding = 0;
					float PopupBorderSize = 0;
					Compute::Vector2 FramePadding;
					float FrameRounding = 0;
					float FrameBorderSize = 0;
					Compute::Vector2 ItemSpacing;
					Compute::Vector2 ItemInnerSpacing;
					Compute::Vector2 TouchExtraPadding;
					float IndentSpacing = 0;
					float ColumnsMinSpacing = 0;
					float ScrollbarSize = 0;
					float ScrollbarRounding = 0;
					float GrabMinSize = 0;
					float GrabRounding = 0;
					Compute::Vector2 ButtonTextAlign;
					Compute::Vector2 DisplayWindowPadding;
					Compute::Vector2 DisplaySafeAreaPadding;
					float MouseCursorScale = 0;
					bool AntiAliasedLines = 0;
					bool AntiAliasedFill = 0;
					float CurveTessellationTol = 0;
					Compute::Vector4 Colors[ColorValue_Count];
				};

				class THAWK_OUT Interface
				{
				private:
					GUIRenderer* Renderer;

				public:
					Interface(GUIRenderer* Parent);
					GUIRenderer* GetGUI();

				public:
					static bool IsMouseOver();
					static void ApplyInput(char* Buffer, int Length);
					static void ApplyKeyState(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
					static void ApplyCursorWheelState(int X, int Y, bool Normal);
					static void SetMouseDraw(bool Enabled);
					static void EndChild();
					static void End();
					static void Restyle(GUI::Style* Style);
					static void GetStyle(GUI::Style* Style);
					static void SetNextWindowPos(Compute::Vector2 pos, int cond = 0, Compute::Vector2 pivot = Compute::Vector2(0, 0));
					static void SetNextWindowSize(Compute::Vector2 size, int cond = 0);
					static void SetNextWindowContentSize(Compute::Vector2 size);
					static void SetNextWindowCollapsed(bool collapsed, int cond);
					static void SetNextWindowFocus();
					static void SetNextWindowBgAlpha(float alpha);
					static void SetWindowPos(Compute::Vector2 pos, int cond = 0);
					static void SetWindowSize(Compute::Vector2 size, int cond = 0);
					static void SetWindowCollapsed(bool collapsed, int cond = 0);
					static void SetWindowFocus();
					static void SetWindowFontScale(float scale);
					static void SetWindowPos(const char* name, Compute::Vector2 pos, int cond = 0);
					static void SetWindowSize(const char* name, Compute::Vector2 size, int cond = 0);
					static void SetWindowCollapsed(const char* name, bool collapsed, int cond = 0);
					static void SetWindowFocus(const char* name);
					static void SetScrollX(float scroll_x);
					static void SetScrollY(float scroll_y);
					static void SetScrollHere(float center_y_ratio = 0.5f);
					static void SetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f);
					static void PushStyleColor(int idx, unsigned int col);
					static void PushStyleColor(int idx, Compute::Vector4 col);
					static void PopStyleColor(int count = 1);
					static void PushStyleVar(int idx, float val);
					static void PushStyleVar(int idx, Compute::Vector2 val);
					static void PopStyleVar(int count = 1);
					static void PushItemWidth(float item_width);
					static void PopItemWidth();
					static void PushTextWrapPos(float wrap_pos_x = 0.0f);
					static void PopTextWrapPos();
					static void PushAllowKeyboardFocus(bool allow_keyboard_focus);
					static void PopAllowKeyboardFocus();
					static void PushButtonRepeat(bool repeat);
					static void PopButtonRepeat();
					static void Separator();
					static void SameLine(float pos_x = 0.0f, float spacing_w = -1.0f);
					static void NewLine();
					static void Spacing();
					static void Dummy(Compute::Vector2 size);
					static void Indent(float indent_w = 0.0f);
					static void Unindent(float indent_w = 0.0f);
					static void BeginGroup();
					static void EndGroup();
					static void SetCursorPos(Compute::Vector2 local_pos);
					static void SetCursorPosX(float x);
					static void SetCursorPosY(float y);
					static void SetCursorScreenPos(Compute::Vector2 screen_pos);
					static void AlignTextToFramePadding();
					static void PushID(const char* str_id);
					static void PushID(const char* str_id_begin, const char* str_id_end);
					static void PushID(const void* ptr_id);
					static void PushID(int int_id);
					static void PopID();
					static void TextUnformatted(const char* text, const char* text_end = nullptr);
					static void Text(const char* fmt, ...);
					static void TextV(const char* fmt, va_list args);
					static void TextColored(Compute::Vector4 col, const char* fmt, ...);
					static void TextColoredV(Compute::Vector4 col, const char* fmt, va_list args);
					static void TextDisabled(const char* fmt, ...);
					static void TextDisabledV(const char* fmt, va_list args);
					static void TextWrapped(const char* fmt, ...);
					static void TextWrappedV(const char* fmt, va_list args);
					static void LabelText(const char* label, const char* fmt, ...);
					static void LabelTextV(const char* label, const char* fmt, va_list args);
					static void BulletText(const char* fmt, ...);
					static void BulletTextV(const char* fmt, va_list args);
					static void ProgressBar(float fraction, Compute::Vector2 size_arg = Compute::Vector2(-1, 0), const char* overlay = nullptr);
					static void Bullet();
					static void EndCombo();
					static void Image(Graphics::Texture2D* user_texture_id, Compute::Vector2 size, Compute::Vector2 uv0 = Compute::Vector2(0, 0), Compute::Vector2 uv1 = Compute::Vector2(1, 1), Compute::Vector4 tint_col = Compute::Vector4(1, 1, 1, 1), Compute::Vector4 border_col = Compute::Vector4(0, 0, 0, 0));
					static void SetColorEditOptions(int flags);
					static void TreePush(const char* str_id);
					static void TreePush(const void* ptr_id = nullptr);
					static void TreePop();
					static void TreeAdvanceToLabelPos();
					static void SetNextTreeNodeOpen(bool is_open, int cond = 0);
					static void ListBoxFooter();
					static void PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0), int stride = sizeof(float));
					static void PlotLines(const char* label, float(* values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0));
					static void PlotHistogram(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0), int stride = sizeof(float));
					static void PlotHistogram(const char* label, float(* values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0));
					static void Value(const char* prefix, bool b);
					static void Value(const char* prefix, int v);
					static void Value(const char* prefix, unsigned int v);
					static void Value(const char* prefix, float v, const char* float_format = nullptr);
					static void EndMainMenuBar();
					static void EndMenuBar();
					static void EndMenu();
					static void BeginTooltip();
					static void EndTooltip();
					static void SetTooltip(const char* fmt, ...);
					static void SetTooltipV(const char* fmt, va_list args);
					static void OpenPopup(const char* str_id);
					static void CloseCurrentPopup();
					static void EndPopup();
					static void Columns(int count = 1, const char* id = nullptr, bool border = true);
					static void NextColumn();
					static void SetColumnWidth(int column_index, float width);
					static void SetColumnOffset(int column_index, float offset_x);
					static void EndDragDropSource();
					static void EndDragDropTarget();
					static void PushClipRect(Compute::Vector2 clip_rect_min, Compute::Vector2 clip_rect_max, bool intersect_with_current_clip_rect);
					static void PopClipRect();
					static void SetItemDefaultFocus();
					static void SetKeyboardFocusHere(int offset = 0);
					static void SetItemAllowOverlap();
					static void EndChildFrame();
					static void CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end);
					static void ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v);
					static void ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b);
					static void ResetMouseDragDelta(int button = 0);
					static void SetMouseCursor(int type);
					static void CaptureKeyboardFromApp(bool capture = true);
					static void CaptureMouseFromApp(bool capture = true);
					static void SetClipboardText(const char* text);
					static bool BeginCanvas(const char* name);
					static bool BeginCanvasFull(const char* name);
					static bool Begin(const char* name, bool* p_open = nullptr, int flags = 0);
					static bool BeginChild(const char* str_id, Compute::Vector2 size = Compute::Vector2(0, 0), bool border = false, int flags = 0);
					static bool BeginChild(unsigned int id, Compute::Vector2 size = Compute::Vector2(0, 0), bool border = false, int flags = 0);
					static bool IsWindowAppearing();
					static bool IsWindowCollapsed();
					static bool IsWindowFocused(int flags = 0);
					static bool IsWindowHovered(int flags = 0);
					static bool Button(const char* label, Compute::Vector2 size = Compute::Vector2(0, 0));
					static bool SmallButton(const char* label);
					static bool InvisibleButton(const char* str_id, Compute::Vector2 size);
					static bool ArrowButton(const char* str_id, int dir);
					static bool ImageButton(Graphics::Texture2D* user_texture_id, Compute::Vector2 size, Compute::Vector2 uv0 = Compute::Vector2(0, 0), Compute::Vector2 uv1 = Compute::Vector2(1, 1), int frame_padding = -1, Compute::Vector4 bg_col = Compute::Vector4(0, 0, 0, 0), Compute::Vector4 tint_col = Compute::Vector4(1, 1, 1, 1));
					static bool Checkbox(const char* label, bool* v);
					static bool CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
					static bool RadioButton(const char* label, bool active);
					static bool RadioButton(const char* label, int* v, int v_button);
					static bool BeginCombo(const char* label, const char* preview_value, int flags = 0);
					static bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
					static bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
					static bool Combo(const char* label, int* current_item, bool(* items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);
					static bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
					static bool DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
					static bool DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
					static bool DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
					static bool DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const char* format_max = nullptr, float power = 1.0f);
					static bool DragInt(const char* label, int* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
					static bool DragInt2(const char* label, int v[2], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
					static bool DragInt3(const char* label, int v[3], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
					static bool DragInt4(const char* label, int v[4], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
					static bool DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", const char* format_max = nullptr);
					static bool DragScalar(const char* label, int data_type, void* v, float v_speed, const void* v_min = nullptr, const void* v_max = nullptr, const char* format = nullptr, float power = 1.0f);
					static bool DragScalarN(const char* label, int data_type, void* v, int components, float v_speed, const void* v_min = nullptr, const void* v_max = nullptr, const char* format = nullptr, float power = 1.0f);
					static bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
					static bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
					static bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
					static bool SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
					static bool SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
					static bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
					static bool SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format = "%d");
					static bool SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format = "%d");
					static bool SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format = "%d");
					static bool SliderScalar(const char* label, int data_type, void* v, const void* v_min, const void* v_max, const char* format = nullptr, float power = 1.0f);
					static bool SliderScalarN(const char* label, int data_type, void* v, int components, const void* v_min, const void* v_max, const char* format = nullptr, float power = 1.0f);
					static bool VSliderFloat(const char* label, Compute::Vector2 size, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
					static bool VSliderInt(const char* label, Compute::Vector2 size, int* v, int v_min, int v_max, const char* format = "%d");
					static bool VSliderScalar(const char* label, Compute::Vector2 size, int data_type, void* v, const void* v_min, const void* v_max, const char* format = nullptr, float power = 1.0f);
					static bool InputText(const char* label, char* buf, size_t buf_size, int flags = 0);
					static bool InputTextMultiline(const char* label, char* buf, size_t buf_size, Compute::Vector2 size = Compute::Vector2(0, 0), int flags = 0);
					static bool InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", int extra_flags = 0);
					static bool InputFloat2(const char* label, float v[2], const char* format = "%.3f", int extra_flags = 0);
					static bool InputFloat3(const char* label, float v[3], const char* format = "%.3f", int extra_flags = 0);
					static bool InputFloat4(const char* label, float v[4], const char* format = "%.3f", int extra_flags = 0);
					static bool InputInt(const char* label, int* v, int step = 1, int step_fast = 100, int extra_flags = 0);
					static bool InputInt2(const char* label, int v[2], int extra_flags = 0);
					static bool InputInt3(const char* label, int v[3], int extra_flags = 0);
					static bool InputInt4(const char* label, int v[4], int extra_flags = 0);
					static bool InputDouble(const char* label, double* v, double step = 0.0f, double step_fast = 0.0f, const char* format = "%.6f", int extra_flags = 0);
					static bool InputScalar(const char* label, int data_type, void* v, const void* step = nullptr, const void* step_fast = nullptr, const char* format = nullptr, int extra_flags = 0);
					static bool InputScalarN(const char* label, int data_type, void* v, int components, const void* step = nullptr, const void* step_fast = nullptr, const char* format = nullptr, int extra_flags = 0);
					static bool ColorEdit3(const char* label, float col[3], int flags = 0);
					static bool ColorEdit4(const char* label, float col[4], int flags = 0);
					static bool ColorPicker3(const char* label, float col[3], int flags = 0);
					static bool ColorPicker4(const char* label, float col[4], int flags = 0, const float* ref_col = nullptr);
					static bool ColorButton(const char* desc_id, Compute::Vector4 col, int flags = 0, Compute::Vector2 size = Compute::Vector2(0, 0));
					static bool TreeNode(const char* label);
					static bool TreeNode(const char* str_id, const char* fmt, ...);
					static bool TreeNode(const void* ptr_id, const char* fmt, ...);
					static bool TreeNodeV(const char* str_id, const char* fmt, va_list args);
					static bool TreeNodeV(const void* ptr_id, const char* fmt, va_list args);
					static bool TreeNodeEx(const char* label, int flags);
					static bool TreeNodeEx(const char* str_id, int flags, const char* fmt, ...);
					static bool TreeNodeEx(const void* ptr_id, int flags, const char* fmt, ...);
					static bool TreeNodeExV(const char* str_id, int flags, const char* fmt, va_list args);
					static bool TreeNodeExV(const void* ptr_id, int flags, const char* fmt, va_list args);
					static bool CollapsingHeader(const char* label, int flags = 0);
					static bool CollapsingHeader(const char* label, bool* p_open, int flags = 0);
					static bool Selectable(const char* label, bool selected = false, int flags = 0, Compute::Vector2 size = Compute::Vector2(0, 0));
					static bool Selectable(const char* label, bool* p_selected, int flags = 0, Compute::Vector2 size = Compute::Vector2(0, 0));
					static bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);
					static bool ListBox(const char* label, int* current_item, bool(* items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1);
					static bool ListBoxHeader(const char* label, Compute::Vector2 size = Compute::Vector2(0, 0));
					static bool ListBoxHeader(const char* label, int items_count, int height_in_items = -1);
					static bool BeginMainMenuBar();
					static bool BeginMenuBar();
					static bool BeginMenu(const char* label, bool enabled = true);
					static bool MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
					static bool MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled = true);
					static bool BeginPopup(const char* str_id, int flags = 0);
					static bool BeginPopupContextItem(const char* str_id = nullptr, int mouse_button = 1);
					static bool BeginPopupContextWindow(const char* str_id = nullptr, int mouse_button = 1, bool also_over_items = true);
					static bool BeginPopupContextVoid(const char* str_id = nullptr, int mouse_button = 1);
					static bool BeginPopupModal(const char* name, bool* p_open = nullptr, int flags = 0);
					static bool OpenPopupOnItemClick(const char* str_id = nullptr, int mouse_button = 1);
					static bool IsPopupOpen(const char* str_id);
					static bool BeginDragDropSource(int flags = 0);
					static bool SetDragDropPayload(const char* type, const void* data, size_t size, int cond = 0);
					static bool BeginDragDropTarget();
					static bool IsItemHovered(int flags = 0);
					static bool IsItemActive();
					static bool IsItemFocused();
					static bool IsItemClicked(int mouse_button = 0);
					static bool IsItemVisible();
					static bool IsItemEdited();
					static bool IsItemDeactivated();
					static bool IsItemDeactivatedAfterEdit();
					static bool IsAnyItemHovered();
					static bool IsAnyItemActive();
					static bool IsAnyItemFocused();
					static bool IsRectVisible(Compute::Vector2 size);
					static bool IsRectVisible(Compute::Vector2 rect_min, Compute::Vector2 rect_max);
					static bool BeginChildFrame(unsigned int id, Compute::Vector2 size, int flags = 0);
					static bool IsKeyDown(int user_key_index);
					static bool IsKeyPressed(int user_key_index, bool repeat = true);
					static bool IsKeyReleased(int user_key_index);
					static bool IsMouseDown(int button);
					static bool IsAnyMouseDown();
					static bool IsMouseClicked(int button, bool repeat = false);
					static bool IsMouseDoubleClicked(int button);
					static bool IsMouseReleased(int button);
					static bool IsMouseDragging(int button = 0, float lock_threshold = -1.0f);
					static bool IsMouseHoveringRect(Compute::Vector2 r_min, Compute::Vector2 r_max, bool clip = true);
					static bool IsMousePosValid(Compute::Vector2* mouse_pos = nullptr);
					static bool IsTextFocused();
					static int GetMouseCursor();
					static int GetKeyIndex(int _key);
					static int GetFrameCount();
					static int GetColumnIndex();
					static int GetColumnsCount();
					static int GetKeyPressedAmount(int key_index, float repeat_delay, float rate);
					static float GetWindowWidth();
					static float GetWindowHeight();
					static float GetContentRegionAvailWidth();
					static float GetWindowContentRegionWidth();
					static float GetScrollX();
					static float GetScrollY();
					static float GetScrollMaxX();
					static float GetScrollMaxY();
					static float GetFontSize();
					static float CalcItemWidth();
					static float GetCursorPosX();
					static float GetCursorPosY();
					static float GetTextLineHeight();
					static float GetTextLineHeightWithSpacing();
					static float GetFrameHeight();
					static float GetFrameHeightWithSpacing();
					static float GetTreeNodeToLabelSpacing();
					static float GetColumnWidth(int column_index = -1);
					static float GetColumnOffset(int column_index = -1);
					static double GetTime();
					static unsigned int GetColorU32(int idx, float alpha_mul = 1.0f);
					static unsigned int GetColorU32(Compute::Vector4 col);
					static unsigned int GetColorU32(unsigned int col);
					static unsigned int ColorConvertFloat4ToU32(Compute::Vector4 in);
					static unsigned int GetID(const char* str_id);
					static unsigned int GetID(const char* str_id_begin, const char* str_id_end);
					static unsigned int GetID(const void* ptr_id);
					static Compute::Vector2 GetWindowPos();
					static Compute::Vector2 GetWindowSize();
					static Compute::Vector2 GetContentRegionMax();
					static Compute::Vector2 GetContentRegionAvail();
					static Compute::Vector2 GetWindowContentRegionMin();
					static Compute::Vector2 GetWindowContentRegionMax();
					static Compute::Vector2 GetFontTexUvWhitePixel();
					static Compute::Vector2 GetCursorPos();
					static Compute::Vector2 GetCursorStartPos();
					static Compute::Vector2 GetCursorScreenPos();
					static Compute::Vector2 GetItemRectMin();
					static Compute::Vector2 GetItemRectMax();
					static Compute::Vector2 GetItemRectSize();
					static Compute::Vector2 CalcTextSize(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);
					static Compute::Vector2 GetMousePos();
					static Compute::Vector2 GetMousePosOnOpeningCurrentPopup();
					static Compute::Vector2 GetMouseDragDelta(int button = 0, float lock_threshold = -1.0f);
					static Compute::Vector4 GetStyleColorVec4(int idx);
					static Compute::Vector4 ColorConvertU32ToFloat4(unsigned int in);
					static const char* GetClipboardText();
					static const char* GetStyleColorName(int idx);
				};

				typedef std::function<void(GUIRenderer*, Rest::Timer*)> RendererCallback;
			}

			class THAWK_OUT ModelRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360 = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* Models = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				ModelRenderer(RenderSystem* Lab);
				virtual ~ModelRenderer();
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(ModelRenderer);
			};

			class THAWK_OUT SkinModelRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360 = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* SkinModels = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				SkinModelRenderer(RenderSystem* Lab);
				virtual ~SkinModelRenderer();
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(SkinModelRenderer);
			};

			class THAWK_OUT SoftBodyRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360 = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* SoftBodies = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::ElementBuffer* VertexBuffer = nullptr;
				Graphics::ElementBuffer* IndexBuffer = nullptr;

			public:
				SoftBodyRenderer(RenderSystem* Lab);
				virtual ~SoftBodyRenderer();
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(SoftBodyRenderer);
			};

			class THAWK_OUT ElementSystemRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360Q = nullptr;
					Graphics::Shader* Depth360P = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceView[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* ElementSystems = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* AdditiveBlend = nullptr;
				Graphics::BlendState* OverwriteBlend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				ElementSystemRenderer(RenderSystem* Lab);
				virtual ~ElementSystemRenderer() override;
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(ElementSystemRenderer);
			};

			class THAWK_OUT DepthRenderer : public IntervalRenderer
			{
			public:
				struct
				{
					std::vector<Graphics::RenderTargetCube*> PointLight;
					std::vector<Graphics::RenderTarget2D*> SpotLight;
					std::vector<Graphics::RenderTarget2D*> LineLight;
					uint64_t PointLightResolution = 256;
					uint64_t PointLightLimits = 4;
					uint64_t SpotLightResolution = 1024;
					uint64_t SpotLightLimits = 16;
					uint64_t LineLightResolution = 2048;
					uint64_t LineLightLimits = 2;
				} Renderers;

			private:
				Rest::Pool<Engine::Component*>* PointLights = nullptr;
				Rest::Pool<Engine::Component*>* SpotLights = nullptr;
				Rest::Pool<Engine::Component*>* LineLights = nullptr;

			public:
				float ShadowDistance;

			public:
				DepthRenderer(RenderSystem* Lab);
				virtual ~DepthRenderer();
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnInitialize() override;
				void OnIntervalRender(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(DepthRenderer);
			};

			class THAWK_OUT ProbeRenderer : public Renderer
			{
			private:
				Rest::Pool<Engine::Component*>* ProbeLights = nullptr;
				uint64_t Map;

			public:
				Graphics::MultiRenderTarget2D* Surface = nullptr;
				Graphics::Texture2D* Face = nullptr;
				uint64_t Size, MipLevels;

			public:
				ProbeRenderer(RenderSystem* Lab);
				virtual ~ProbeRenderer();
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void CreateRenderTarget();
				bool ResourceBound(Graphics::TextureCube** Cube);

			public:
				THAWK_COMPONENT(ProbeRenderer);
			};

			class THAWK_OUT LightRenderer : public Renderer
			{
			public:
				struct
				{
					Compute::Matrix4x4 OwnWorldViewProjection;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float Mipping;
					Compute::Vector3 Scale;
					float Parallax;
				} ProbeLight;

				struct
				{
					Compute::Matrix4x4 OwnWorldViewProjection;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float Distance;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
				} PointLight;

				struct
				{
					Compute::Matrix4x4 OwnWorldViewProjection;
					Compute::Matrix4x4 OwnViewProjection;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float Diffuse;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
				} SpotLight;

				struct
				{
					Compute::Matrix4x4 OwnViewProjection;
					Compute::Vector3 Position;
					float ShadowDistance;
					Compute::Vector3 Lighting;
					float ShadowLength;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
				} LineLight;

				struct
				{
					Compute::Vector3 HighEmission = 0.05;
					float Padding1;
					Compute::Vector3 LowEmission = 0.025;
					float Padding2;
				} AmbientLight;

			protected:
				struct
				{
					Graphics::Shader* Probe = nullptr;
					Graphics::Shader* Point = nullptr;
					Graphics::Shader* PointOccluded = nullptr;
					Graphics::Shader* Spot = nullptr;
					Graphics::Shader* SpotOccluded = nullptr;
					Graphics::Shader* Line = nullptr;
					Graphics::Shader* LineOccluded = nullptr;
					Graphics::Shader* Ambient = nullptr;
				} Shaders;

				struct
				{
					float SpotLight = 1024;
					float LineLight = 2048;
				} Quality;

			private:
				Rest::Pool<Engine::Component*>* PointLights = nullptr;
				Rest::Pool<Engine::Component*>* ProbeLights = nullptr;
				Rest::Pool<Engine::Component*>* SpotLights = nullptr;
				Rest::Pool<Engine::Component*>* LineLights = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::RenderTarget2D* Output1 = nullptr;
				Graphics::RenderTarget2D* Output2 = nullptr;
				Graphics::RenderTarget2D* Input1 = nullptr;
				Graphics::RenderTarget2D* Input2 = nullptr;

			public:
				bool RecursiveProbes;

			public:
				LightRenderer(RenderSystem* Lab);
				virtual ~LightRenderer();
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnResizeBuffers() override;
				void CompilePointEffects();
				void CompileProbeEffects();
				void CompileSpotEffects();
				void CompileLineEffects();
				void CompileAmbientEffects();
				void CreateRenderTargets();

			public:
				THAWK_COMPONENT(LightRenderer);
			};

			class THAWK_OUT ImageRenderer : public Renderer
			{
			public:
				Graphics::RenderTarget2D* RenderTarget = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				ImageRenderer(RenderSystem* Lab);
				ImageRenderer(RenderSystem* Lab, Graphics::RenderTarget2D* Target);
				virtual ~ImageRenderer() override;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRender(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(ImageRenderer);
			};

			class THAWK_OUT ReflectionsRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float IterationCount = 24.0f;
					float MipLevels = 0.0f;
					float Intensity = 1.736f;
					float Padding = 0.0f;
				} Render;

			public:
				ReflectionsRenderer(RenderSystem* Lab);
				virtual ~ReflectionsRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(ReflectionsRenderer);
			};

			class THAWK_OUT DepthOfFieldRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float Texel[2] = { 1.0f / 512.0f };
					float Threshold = 0.5f;
					float Gain = 2.0f;
					float Fringe = 0.7f;
					float Bias = 0.5f;
					float Dither = 0.0001f;
					float Samples = 3.0f;
					float Rings = 3.0f;
					float FarDistance = 100.0f;
					float FarRange = 10.0f;
					float NearDistance = 1.0f;
					float NearRange = 1.0f;
					float FocalDepth = 1.0f;
					float Intensity = 1.0f;
					float Circular = 1.0f;
				} Render;

			public:
				DepthOfFieldRenderer(RenderSystem* Lab);
				virtual ~DepthOfFieldRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(DepthOfFieldRenderer);
			};

			class THAWK_OUT EmissionRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float Texel[2] = { 1.0f, 1.0f };
					float IterationCount = 24.0f;
					float Intensity = 1.736f;
					float Threshold = 0.38f;
					float Scale = 0.1f;
					float Padding1 = 0.0f;
					float Padding2 = 0.0f;
				} Render;

			public:
				EmissionRenderer(RenderSystem* Lab);
				virtual ~EmissionRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(EmissionRenderer);
			};

			class THAWK_OUT GlitchRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float ScanLineJitterDisplacement = 0;
					float ScanLineJitterThreshold = 0;
					float VerticalJumpAmount = 0;
					float VerticalJumpTime = 0;
					float ColorDriftAmount = 0;
					float ColorDriftTime = 0;
					float HorizontalShake = 0;
					float ElapsedTime = 0;
				} Render;

			public:
				float ScanLineJitter;
				float VerticalJump;
				float HorizontalShake;
				float ColorDrift;

			public:
				GlitchRenderer(RenderSystem* Lab);
				virtual ~GlitchRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(GlitchRenderer);
			};

			class THAWK_OUT AmbientOcclusionRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float Scale = 1.936f;
					float Intensity = 2.345f;
					float Bias = 0.467f;
					float Radius = 0.050885f;
					float Step = 0.030f;
					float Offset = 0.033f;
					float Distance = 3.000f;
					float Fading = 1.965f;
					float Power = 1.294;
					float Samples = 2.0f;
					float SampleCount = 16.0f;
					float Padding;
				} Render;

			public:
				AmbientOcclusionRenderer(RenderSystem* Lab);
				virtual ~AmbientOcclusionRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(AmbientOcclusionRenderer);
			};

			class THAWK_OUT IndirectOcclusionRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float Scale = 1.936f;
					float Intensity = 6.745f;
					float Bias = 0.467f;
					float Radius = 0.250885f;
					float Step = 0.030f;
					float Offset = 0.033f;
					float Distance = 3.000f;
					float Fading = 1.965f;
					float Power = 1.294;
					float Samples = 2.0f;
					float SampleCount = 16.0f;
					float Padding;
				} Render;

			public:
				IndirectOcclusionRenderer(RenderSystem* Lab);
				virtual ~IndirectOcclusionRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(IndirectOcclusionRenderer);
			};

			class THAWK_OUT ToneRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					Compute::Vector3 BlindVisionR = Compute::Vector3(1, 0, 0);
					float VignetteAmount = 1.0f;
					Compute::Vector3 BlindVisionG = Compute::Vector3(0, 1, 0);
					float VignetteCurve = 1.5f;
					Compute::Vector3 BlindVisionB = Compute::Vector3(0, 0, 1);
					float VignetteRadius = 1.0f;
					Compute::Vector3 VignetteColor;
					float LinearIntensity = 0.25f;
					Compute::Vector3 ColorGamma = Compute::Vector3(1, 1, 1);
					float GammaIntensity = 1.25f;
					Compute::Vector3 DesaturationGamma = Compute::Vector3(0.3f, 0.59f, 0.11f);
					float DesaturationIntensity = 0.5f;
				} Render;

			public:
				ToneRenderer(RenderSystem* Lab);
				virtual ~ToneRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(ToneRenderer);
			};

			class THAWK_OUT GUIRenderer : public Renderer
			{
			private:
				Compute::Matrix4x4 WorldViewProjection;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::ElementBuffer* VertexBuffer;
				Graphics::ElementBuffer* IndexBuffer;
				Graphics::Activity* Activity;
				Graphics::Shader* Shader;
				Graphics::Texture2D* Font;
				GUI::RendererCallback Callback;
				GUI::Interface Tree;
				uint64_t Time, Frequency;
				char* ClipboardTextData;
				void* Context;
				std::mutex Safe;
				bool TreeActive;

			public:
				bool AllowMouseOffset;

			public:
				GUIRenderer(RenderSystem* Lab);
				GUIRenderer(RenderSystem* Lab, Graphics::Activity* NewWindow);
				virtual ~GUIRenderer() override;
				void OnRender(Rest::Timer* Time) override;
				void Transform(const Compute::Matrix4x4& In);
				void Activate();
				void Deactivate();
				void Restore(void* FontAtlas = nullptr, void* Callback = nullptr);
				void Setup(const std::function<void(GUI::Interface*)>& Callback);
				void SetRenderCallback(const GUI::RendererCallback& NewCallback);
				void GetFontAtlas(unsigned char** Pixels, int* Width, int* Height);
				void* GetUi();
				const char* GetClipboardCopy();
				Compute::Matrix4x4& GetTransform();
				Graphics::Activity* GetActivity();
				GUI::Interface* GetTree();
				bool IsTreeActive();

			public:
				static Graphics::InputLayout* GetDrawVertexLayout();
				static size_t GetDrawVertexSize();
				static void DrawList(void* Context);

			public:
				THAWK_COMPONENT(GUIRenderer);
			};
		}
	}
}
#endif