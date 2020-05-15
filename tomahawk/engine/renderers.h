#ifndef THAWK_ENGINE_RENDERERS_H
#define THAWK_ENGINE_RENDERERS_H

#include "../engine.h"

namespace Tomahawk
{
    namespace Engine
    {
        namespace Renderers
        {
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
                    float Alpha;
                    Compute::Vector2 WindowPadding;
                    float WindowRounding;
                    float WindowBorderSize;
                    Compute::Vector2 WindowMinSize;
                    Compute::Vector2 WindowTitleAlign;
                    float ChildRounding;
                    float ChildBorderSize;
                    float PopupRounding;
                    float PopupBorderSize;
                    Compute::Vector2 FramePadding;
                    float FrameRounding;
                    float FrameBorderSize;
                    Compute::Vector2 ItemSpacing;
                    Compute::Vector2 ItemInnerSpacing;
                    Compute::Vector2 TouchExtraPadding;
                    float IndentSpacing;
                    float ColumnsMinSpacing;
                    float ScrollbarSize;
                    float ScrollbarRounding;
                    float GrabMinSize;
                    float GrabRounding;
                    Compute::Vector2 ButtonTextAlign;
                    Compute::Vector2 DisplayWindowPadding;
                    Compute::Vector2 DisplaySafeAreaPadding;
                    float MouseCursorScale;
                    bool AntiAliasedLines;
                    bool AntiAliasedFill;
                    float CurveTessellationTol;
                    Compute::Vector4 Colors[ColorValue_Count];
                };
            }

            class THAWK_OUT ModelRenderer : public Renderer
            {
            protected:
                ModelRenderer(RenderSystem* Lab);

            public:
                virtual ~ModelRenderer() = default;
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static ModelRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT SkinnedModelRenderer : public Renderer
            {
            protected:
                SkinnedModelRenderer(RenderSystem* Lab);

            public:
                virtual ~SkinnedModelRenderer() = default;
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static SkinnedModelRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT DepthRenderer : public IntervalRenderer
            {
            public:
                struct
                {
                    std::vector<Graphics::RenderTargetCube*> PointLight;
                    std::vector<Graphics::RenderTarget2D*> SpotLight;
                    std::vector<Graphics::RenderTarget2D*> LineLight;
                    UInt64 PointLightResolution = 256;
                    UInt64 PointLightLimits = 4;
                    UInt64 SpotLightResolution = 1024;
                    UInt64 SpotLightLimits = 16;
                    UInt64 LineLightResolution = 2048;
                    UInt64 LineLightLimits = 2;
                } Renderers;

            public:
                float ShadowDistance;

            protected:
                DepthRenderer(RenderSystem* Lab);

            public:
                virtual ~DepthRenderer();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static DepthRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT ProbeRenderer : public Renderer
            {
            public:
                Graphics::MultiRenderTarget2D* Surface;
                UInt64 Size, MipLevels;

            protected:
                ProbeRenderer(RenderSystem* Lab);

            public:
                virtual ~ProbeRenderer();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static ProbeRenderer* Create(RenderSystem* Lab);
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

            public:
                bool RecursiveProbes;

            protected:
                LightRenderer(RenderSystem* Lab);

            public:
                virtual ~LightRenderer() = default;
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static LightRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT ElementSystemRenderer : public Renderer
            {
            protected:
                ElementSystemRenderer(RenderSystem* Lab);

            public:
                virtual ~ElementSystemRenderer() = default;
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static ElementSystemRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT ImageRenderer : public Renderer
            {
            public:
                Graphics::RenderTarget2D* RenderTarget = nullptr;

            protected:
                ImageRenderer(RenderSystem* Lab);
                ImageRenderer(RenderSystem* Lab, Graphics::RenderTarget2D* Target);

            public:
                virtual ~ImageRenderer();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static ImageRenderer* Create(RenderSystem* Lab);
                static ImageRenderer* Create(RenderSystem* Lab, Graphics::RenderTarget2D* Target);
            };

            class THAWK_OUT ReflectionsRenderer : public Renderer
            {
            public:
                struct RenderConstant
                {
                    Compute::Matrix4x4 ViewProjection;
                    float IterationCount = 24.0f;
                    float RayCorrection = 0.965f;
                    float RayLength = -0.1f;
                    float MipLevels = 0.0f;
                } Render;

            protected:
                ReflectionsRenderer(RenderSystem* Lab);

            public:
                virtual ~ReflectionsRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static ReflectionsRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT DepthOfFieldRenderer : public Renderer
            {
            public:
                struct RenderConstant
                {
                    Compute::Matrix4x4 ViewProjection;
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
                float HorizontalResolution = 512.0f;
                float VerticalResolution = 512.0f;
                bool AutoViewport = true;

            protected:
                DepthOfFieldRenderer(RenderSystem* Lab);

            public:
                virtual ~DepthOfFieldRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static DepthOfFieldRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT EmissionRenderer : public Renderer
            {
            public:
                struct RenderConstant
                {
                    float Texel[2] = { 512.0f };
                    float Intensity = 2.0f;
                    float Threshold = 0.75f;
                    float Scaling[2] = { 0.9f, 0.9f };
                    float Samples = 2.0f;
                    float SampleCount = 16.0f;
                } Render;

            public:
                bool AutoViewport;

            protected:
                EmissionRenderer(RenderSystem* Lab);

            public:
                virtual ~EmissionRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static EmissionRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT GlitchRenderer : public Renderer
            {
            public:
                struct RenderConstant
                {
                    float ScanLineJitterDisplacement;
                    float ScanLineJitterThreshold;
                    float VerticalJumpAmount;
                    float VerticalJumpTime;
                    float ColorDriftAmount;
                    float ColorDriftTime;
                    float HorizontalShake;
                    float ElapsedTime;
                } Render;

            public:
                float ScanLineJitter;
                float VerticalJump;
                float HorizontalShake;
                float ColorDrift;

            protected:
                GlitchRenderer(RenderSystem* Lab);

            public:
                virtual ~GlitchRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static GlitchRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT AmbientOcclusionRenderer : public Renderer
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

            protected:
                AmbientOcclusionRenderer(RenderSystem* Lab);

            public:
                virtual ~AmbientOcclusionRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static AmbientOcclusionRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT IndirectOcclusionRenderer : public Renderer
            {
            public:
                struct RenderConstant
                {
                    float Scale = 0.625f;
                    float Intensity = 5.0f;
                    float Bias = 0.3f;
                    float Radius = 0.008f;
                    float Step = 0.075f;
                    float Offset = 0.85f;
                    float Distance = 3.0f;
                    float Fading = 1.75f;
                    float Power = 1.25;
                    float Samples = 2.0f;
                    float SampleCount = 16.0f;
                    float Padding;
                } Render;

            protected:
                IndirectOcclusionRenderer(RenderSystem* Lab);

            public:
                virtual ~IndirectOcclusionRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static IndirectOcclusionRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT ToneRenderer : public Renderer
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

            protected:
                ToneRenderer(RenderSystem* Lab);

            public:
                virtual ~ToneRenderer() = default;
                void OnInitialize();
                void OnLoad(ContentManager* Content, Rest::Document* Node);
                void OnSave(ContentManager* Content, Rest::Document* Node);

            public:
                static ToneRenderer* Create(RenderSystem* Lab);
            };

            class THAWK_OUT GUIRenderer : public Renderer
            {
            protected:
                Compute::Matrix4x4 WorldViewProjection;
                std::function<void(GUIRenderer*)> Callback;
                UInt64 Time, Frequency;
                Graphics::Activity* Activity;
                char* ClipboardTextData;
                void* Context;

            public:
                bool AllowMouseOffset;

            protected:
                GUIRenderer(RenderSystem* Lab, Graphics::Activity* NewWindow);

            public:
                virtual ~GUIRenderer();
                void OnRasterization(Rest::Timer* Time);
                void KeyStateCallback(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
                void InputCallback(char* Buffer, int Length);
                void CursorWheelStateCallback(int X, int Y, bool Normal);
                void Transform(Compute::Matrix4x4 In);
                void Activate();
                void Deactivate();
                void Prepare();
                void Render();
                void Reset();
                void End();
                void EndChild();
                void Restyle(GUI::Style* Style);
                void GetStyle(GUI::Style* Style);
                void SetRenderCallback(const std::function<void(GUIRenderer*)>& NewCallback);
                void SetMouseDraw(bool Enabled);
                void SetNextWindowPos(Compute::Vector2 pos, int cond = 0, Compute::Vector2 pivot = Compute::Vector2(0, 0));
                void SetNextWindowSize(Compute::Vector2 size, int cond = 0);
                void SetNextWindowContentSize(Compute::Vector2 size);
                void SetNextWindowCollapsed(bool collapsed, int cond);
                void SetNextWindowFocus();
                void SetNextWindowBgAlpha(float alpha);
                void SetWindowPos(Compute::Vector2 pos, int cond = 0);
                void SetWindowSize(Compute::Vector2 size, int cond = 0);
                void SetWindowCollapsed(bool collapsed, int cond = 0);
                void SetWindowFocus();
                void SetWindowFontScale(float scale);
                void SetWindowPos(const char* name, Compute::Vector2 pos, int cond = 0);
                void SetWindowSize(const char* name, Compute::Vector2 size, int cond = 0);
                void SetWindowCollapsed(const char* name, bool collapsed, int cond = 0);
                void SetWindowFocus(const char* name);
                void SetScrollX(float scroll_x);
                void SetScrollY(float scroll_y);
                void SetScrollHere(float center_y_ratio = 0.5f);
                void SetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f);
                void PushStyleColor(int idx, unsigned int col);
                void PushStyleColor(int idx, Compute::Vector4 col);
                void PopStyleColor(int count = 1);
                void PushStyleVar(int idx, float val);
                void PushStyleVar(int idx, Compute::Vector2 val);
                void PopStyleVar(int count = 1);
                void PushItemWidth(float item_width);
                void PopItemWidth();
                void PushTextWrapPos(float wrap_pos_x = 0.0f);
                void PopTextWrapPos();
                void PushAllowKeyboardFocus(bool allow_keyboard_focus);
                void PopAllowKeyboardFocus();
                void PushButtonRepeat(bool repeat);
                void PopButtonRepeat();
                void Separator();
                void SameLine(float pos_x = 0.0f, float spacing_w = -1.0f);
                void NewLine();
                void Spacing();
                void Dummy(Compute::Vector2 size);
                void Indent(float indent_w = 0.0f);
                void Unindent(float indent_w = 0.0f);
                void BeginGroup();
                void EndGroup();
                void SetCursorPos(Compute::Vector2 local_pos);
                void SetCursorPosX(float x);
                void SetCursorPosY(float y);
                void SetCursorScreenPos(Compute::Vector2 screen_pos);
                void AlignTextToFramePadding();
                void PushID(const char* str_id);
                void PushID(const char* str_id_begin, const char* str_id_end);
                void PushID(const void* ptr_id);
                void PushID(int int_id);
                void PopID();
                void TextUnformatted(const char* text, const char* text_end = nullptr);
                void Text(const char* fmt, ...);
                void TextV(const char* fmt, va_list args);
                void TextColored(Compute::Vector4 col, const char* fmt, ...);
                void TextColoredV(Compute::Vector4 col, const char* fmt, va_list args);
                void TextDisabled(const char* fmt, ...);
                void TextDisabledV(const char* fmt, va_list args);
                void TextWrapped(const char* fmt, ...);
                void TextWrappedV(const char* fmt, va_list args);
                void LabelText(const char* label, const char* fmt, ...);
                void LabelTextV(const char* label, const char* fmt, va_list args);
                void BulletText(const char* fmt, ...);
                void BulletTextV(const char* fmt, va_list args);
                void ProgressBar(float fraction, Compute::Vector2 size_arg = Compute::Vector2(-1, 0), const char* overlay = nullptr);
                void Bullet();
                void EndCombo();
                void Image(Graphics::Texture2D* user_texture_id, Compute::Vector2 size, Compute::Vector2 uv0 = Compute::Vector2(0, 0), Compute::Vector2 uv1 = Compute::Vector2(1, 1), Compute::Vector4 tint_col = Compute::Vector4(1, 1, 1, 1), Compute::Vector4 border_col = Compute::Vector4(0, 0, 0, 0));
                void SetColorEditOptions(int flags);
                void TreePush(const char* str_id);
                void TreePush(const void* ptr_id = nullptr);
                void TreePop();
                void TreeAdvanceToLabelPos();
                void SetNextTreeNodeOpen(bool is_open, int cond = 0);
                void ListBoxFooter();
                void PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0), int stride = sizeof(float));
                void PlotLines(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0));
                void PlotHistogram(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0), int stride = sizeof(float));
                void PlotHistogram(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = nullptr, float scale_min = std::numeric_limits<float>::max(), float scale_max = std::numeric_limits<float>::max(), Compute::Vector2 graph_size = Compute::Vector2(0, 0));
                void Value(const char* prefix, bool b);
                void Value(const char* prefix, int v);
                void Value(const char* prefix, unsigned int v);
                void Value(const char* prefix, float v, const char* float_format = nullptr);
                void EndMainMenuBar();
                void EndMenuBar();
                void EndMenu();
                void BeginTooltip();
                void EndTooltip();
                void SetTooltip(const char* fmt, ...);
                void SetTooltipV(const char* fmt, va_list args);
                void OpenPopup(const char* str_id);
                void CloseCurrentPopup();
                void EndPopup();
                void Columns(int count = 1, const char* id = nullptr, bool border = true);
                void NextColumn();
                void SetColumnWidth(int column_index, float width);
                void SetColumnOffset(int column_index, float offset_x);
                void EndDragDropSource();
                void EndDragDropTarget();
                void PushClipRect(Compute::Vector2 clip_rect_min, Compute::Vector2 clip_rect_max, bool intersect_with_current_clip_rect);
                void PopClipRect();
                void SetItemDefaultFocus();
                void SetKeyboardFocusHere(int offset = 0);
                void SetItemAllowOverlap();
                void EndChildFrame();
                void CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end);
                void ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v);
                void ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b);
                void ResetMouseDragDelta(int button = 0);
                void SetMouseCursor(int type);
                void CaptureKeyboardFromApp(bool capture = true);
                void CaptureMouseFromApp(bool capture = true);
                void SetClipboardText(const char* text);
                bool BeginCanvas(const char* name);
                bool BeginCanvasFull(const char* name);
                bool Begin(const char* name, bool* p_open = nullptr, int flags = 0);
                bool BeginChild(const char* str_id, Compute::Vector2 size = Compute::Vector2(0, 0), bool border = false, int flags = 0);
                bool BeginChild(unsigned int id, Compute::Vector2 size = Compute::Vector2(0, 0), bool border = false, int flags = 0);
                bool IsWindowAppearing();
                bool IsWindowCollapsed();
                bool IsWindowFocused(int flags = 0);
                bool IsWindowHovered(int flags = 0);
                bool Button(const char* label, Compute::Vector2 size = Compute::Vector2(0, 0));
                bool SmallButton(const char* label);
                bool InvisibleButton(const char* str_id, Compute::Vector2 size);
                bool ArrowButton(const char* str_id, int dir);
                bool ImageButton(Graphics::Texture2D* user_texture_id, Compute::Vector2 size, Compute::Vector2 uv0 = Compute::Vector2(0, 0), Compute::Vector2 uv1 = Compute::Vector2(1, 1), int frame_padding = -1, Compute::Vector4 bg_col = Compute::Vector4(0, 0, 0, 0), Compute::Vector4 tint_col = Compute::Vector4(1, 1, 1, 1));
                bool Checkbox(const char* label, bool* v);
                bool CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
                bool RadioButton(const char* label, bool active);
                bool RadioButton(const char* label, int* v, int v_button);
                bool BeginCombo(const char* label, const char* preview_value, int flags = 0);
                bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
                bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
                bool Combo(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);
                bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
                bool DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
                bool DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
                bool DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", float power = 1.0f);
                bool DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const char* format_max = nullptr, float power = 1.0f);
                bool DragInt(const char* label, int* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
                bool DragInt2(const char* label, int v[2], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
                bool DragInt3(const char* label, int v[3], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
                bool DragInt4(const char* label, int v[4], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d");
                bool DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", const char* format_max = nullptr);
                bool DragScalar(const char* label, int data_type, void* v, float v_speed, const void* v_min = nullptr, const void* v_max = nullptr, const char* format = nullptr, float power = 1.0f);
                bool DragScalarN(const char* label, int data_type, void* v, int components, float v_speed, const void* v_min = nullptr, const void* v_max = nullptr, const char* format = nullptr, float power = 1.0f);
                bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
                bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
                bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
                bool SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
                bool SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
                bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
                bool SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format = "%d");
                bool SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format = "%d");
                bool SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format = "%d");
                bool SliderScalar(const char* label, int data_type, void* v, const void* v_min, const void* v_max, const char* format = nullptr, float power = 1.0f);
                bool SliderScalarN(const char* label, int data_type, void* v, int components, const void* v_min, const void* v_max, const char* format = nullptr, float power = 1.0f);
                bool VSliderFloat(const char* label, Compute::Vector2 size, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
                bool VSliderInt(const char* label, Compute::Vector2 size, int* v, int v_min, int v_max, const char* format = "%d");
                bool VSliderScalar(const char* label, Compute::Vector2 size, int data_type, void* v, const void* v_min, const void* v_max, const char* format = nullptr, float power = 1.0f);
                bool InputText(const char* label, char* buf, size_t buf_size, int flags = 0);
                bool InputTextMultiline(const char* label, char* buf, size_t buf_size, Compute::Vector2 size = Compute::Vector2(0, 0), int flags = 0);
                bool InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", int extra_flags = 0);
                bool InputFloat2(const char* label, float v[2], const char* format = "%.3f", int extra_flags = 0);
                bool InputFloat3(const char* label, float v[3], const char* format = "%.3f", int extra_flags = 0);
                bool InputFloat4(const char* label, float v[4], const char* format = "%.3f", int extra_flags = 0);
                bool InputInt(const char* label, int* v, int step = 1, int step_fast = 100, int extra_flags = 0);
                bool InputInt2(const char* label, int v[2], int extra_flags = 0);
                bool InputInt3(const char* label, int v[3], int extra_flags = 0);
                bool InputInt4(const char* label, int v[4], int extra_flags = 0);
                bool InputDouble(const char* label, Float64* v, Float64 step = 0.0f, Float64 step_fast = 0.0f, const char* format = "%.6f", int extra_flags = 0);
                bool InputScalar(const char* label, int data_type, void* v, const void* step = nullptr, const void* step_fast = nullptr, const char* format = nullptr, int extra_flags = 0);
                bool InputScalarN(const char* label, int data_type, void* v, int components, const void* step = nullptr, const void* step_fast = nullptr, const char* format = nullptr, int extra_flags = 0);
                bool ColorEdit3(const char* label, float col[3], int flags = 0);
                bool ColorEdit4(const char* label, float col[4], int flags = 0);
                bool ColorPicker3(const char* label, float col[3], int flags = 0);
                bool ColorPicker4(const char* label, float col[4], int flags = 0, const float* ref_col = nullptr);
                bool ColorButton(const char* desc_id, Compute::Vector4 col, int flags = 0, Compute::Vector2 size = Compute::Vector2(0, 0));
                bool TreeNode(const char* label);
                bool TreeNode(const char* str_id, const char* fmt, ...);
                bool TreeNode(const void* ptr_id, const char* fmt, ...);
                bool TreeNodeV(const char* str_id, const char* fmt, va_list args);
                bool TreeNodeV(const void* ptr_id, const char* fmt, va_list args);
                bool TreeNodeEx(const char* label, int flags);
                bool TreeNodeEx(const char* str_id, int flags, const char* fmt, ...);
                bool TreeNodeEx(const void* ptr_id, int flags, const char* fmt, ...);
                bool TreeNodeExV(const char* str_id, int flags, const char* fmt, va_list args);
                bool TreeNodeExV(const void* ptr_id, int flags, const char* fmt, va_list args);
                bool CollapsingHeader(const char* label, int flags = 0);
                bool CollapsingHeader(const char* label, bool* p_open, int flags = 0);
                bool Selectable(const char* label, bool selected = false, int flags = 0, Compute::Vector2 size = Compute::Vector2(0, 0));
                bool Selectable(const char* label, bool* p_selected, int flags = 0, Compute::Vector2 size = Compute::Vector2(0, 0));
                bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);
                bool ListBox(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1);
                bool ListBoxHeader(const char* label, Compute::Vector2 size = Compute::Vector2(0, 0));
                bool ListBoxHeader(const char* label, int items_count, int height_in_items = -1);
                bool BeginMainMenuBar();
                bool BeginMenuBar();
                bool BeginMenu(const char* label, bool enabled = true);
                bool MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
                bool MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled = true);
                bool BeginPopup(const char* str_id, int flags = 0);
                bool BeginPopupContextItem(const char* str_id = nullptr, int mouse_button = 1);
                bool BeginPopupContextWindow(const char* str_id = nullptr, int mouse_button = 1, bool also_over_items = true);
                bool BeginPopupContextVoid(const char* str_id = nullptr, int mouse_button = 1);
                bool BeginPopupModal(const char* name, bool* p_open = nullptr, int flags = 0);
                bool OpenPopupOnItemClick(const char* str_id = nullptr, int mouse_button = 1);
                bool IsPopupOpen(const char* str_id);
                bool BeginDragDropSource(int flags = 0);
                bool SetDragDropPayload(const char* type, const void* data, size_t size, int cond = 0);
                bool BeginDragDropTarget();
                bool IsItemHovered(int flags = 0);
                bool IsItemActive();
                bool IsItemFocused();
                bool IsItemClicked(int mouse_button = 0);
                bool IsItemVisible();
                bool IsItemEdited();
                bool IsItemDeactivated();
                bool IsItemDeactivatedAfterEdit();
                bool IsAnyItemHovered();
                bool IsAnyItemActive();
                bool IsAnyItemFocused();
                bool IsRectVisible(Compute::Vector2 size);
                bool IsRectVisible(Compute::Vector2 rect_min, Compute::Vector2 rect_max);
                bool BeginChildFrame(unsigned int id, Compute::Vector2 size, int flags = 0);
                bool IsKeyDown(int user_key_index);
                bool IsKeyPressed(int user_key_index, bool repeat = true);
                bool IsKeyReleased(int user_key_index);
                bool IsMouseDown(int button);
                bool IsAnyMouseDown();
                bool IsMouseClicked(int button, bool repeat = false);
                bool IsMouseDoubleClicked(int button);
                bool IsMouseReleased(int button);
                bool IsMouseDragging(int button = 0, float lock_threshold = -1.0f);
                bool IsMouseHoveringRect(Compute::Vector2 r_min, Compute::Vector2 r_max, bool clip = true);
                bool IsMousePosValid(Compute::Vector2* mouse_pos = nullptr);
                bool IsTextFocused();
                int GetMouseCursor();
                int GetKeyIndex(int _key);
                int GetFrameCount();
                int GetColumnIndex();
                int GetColumnsCount();
                int GetKeyPressedAmount(int key_index, float repeat_delay, float rate);
                float GetWindowWidth();
                float GetWindowHeight();
                float GetContentRegionAvailWidth();
                float GetWindowContentRegionWidth();
                float GetScrollX();
                float GetScrollY();
                float GetScrollMaxX();
                float GetScrollMaxY();
                float GetFontSize();
                float CalcItemWidth();
                float GetCursorPosX();
                float GetCursorPosY();
                float GetTextLineHeight();
                float GetTextLineHeightWithSpacing();
                float GetFrameHeight();
                float GetFrameHeightWithSpacing();
                float GetTreeNodeToLabelSpacing();
                float GetColumnWidth(int column_index = -1);
                float GetColumnOffset(int column_index = -1);
                Float64 GetTime();
                unsigned int GetColorU32(int idx, float alpha_mul = 1.0f);
                unsigned int GetColorU32(Compute::Vector4 col);
                unsigned int GetColorU32(unsigned int col);
                unsigned int ColorConvertFloat4ToU32(Compute::Vector4 in);
                unsigned int GetID(const char* str_id);
                unsigned int GetID(const char* str_id_begin, const char* str_id_end);
                unsigned int GetID(const void* ptr_id);
                Compute::Vector2 GetWindowPos();
                Compute::Vector2 GetWindowSize();
                Compute::Vector2 GetContentRegionMax();
                Compute::Vector2 GetContentRegionAvail();
                Compute::Vector2 GetWindowContentRegionMin();
                Compute::Vector2 GetWindowContentRegionMax();
                Compute::Vector2 GetFontTexUvWhitePixel();
                Compute::Vector2 GetCursorPos();
                Compute::Vector2 GetCursorStartPos();
                Compute::Vector2 GetCursorScreenPos();
                Compute::Vector2 GetItemRectMin();
                Compute::Vector2 GetItemRectMax();
                Compute::Vector2 GetItemRectSize();
                Compute::Vector2 CalcTextSize(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);
                Compute::Vector2 GetMousePos();
                Compute::Vector2 GetMousePosOnOpeningCurrentPopup();
                Compute::Vector2 GetMouseDragDelta(int button = 0, float lock_threshold = -1.0f);
                Compute::Vector4 GetStyleColorVec4(int idx);
                Compute::Vector4 ColorConvertU32ToFloat4(unsigned int in);
                const char* GetClipboardText();
                const char* GetStyleColorName(int idx);
                const char* CopyClipboard();
                void* GetUi();
                Compute::Matrix4x4 GetTransform();
                Graphics::Activity* GetActivity();

            public:
                static GUIRenderer* Create(RenderSystem* Lab, Graphics::Activity* Window);
            };
        }
    }
}
#endif