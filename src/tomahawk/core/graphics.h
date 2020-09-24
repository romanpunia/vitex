#ifndef TH_GRAPHICS_H
#define TH_GRAPHICS_H

#include "compute.h"
#include <iostream>
#include <functional>
#include <limits>

struct SDL_SysWMinfo;
struct SDL_Cursor;
struct SDL_Window;
struct SDL_Surface;

namespace Tomahawk
{
	namespace Graphics
	{
		enum AppState
		{
			AppState_Close_Window,
			AppState_Terminating,
			AppState_Low_Memory,
			AppState_Enter_Background_Start,
			AppState_Enter_Background_End,
			AppState_Enter_Foreground_Start,
			AppState_Enter_Foreground_End
		};

		enum WindowState
		{
			WindowState_Show,
			WindowState_Hide,
			WindowState_Expose,
			WindowState_Move,
			WindowState_Resize,
			WindowState_Size_Change,
			WindowState_Minimize,
			WindowState_Maximize,
			WindowState_Restore,
			WindowState_Enter,
			WindowState_Leave,
			WindowState_Focus,
			WindowState_Blur,
			WindowState_Close
		};

		enum KeyCode
		{
			KeyCode_A = 4,
			KeyCode_B = 5,
			KeyCode_C = 6,
			KeyCode_D = 7,
			KeyCode_E = 8,
			KeyCode_F = 9,
			KeyCode_G = 10,
			KeyCode_H = 11,
			KeyCode_I = 12,
			KeyCode_J = 13,
			KeyCode_K = 14,
			KeyCode_L = 15,
			KeyCode_M = 16,
			KeyCode_N = 17,
			KeyCode_O = 18,
			KeyCode_P = 19,
			KeyCode_Q = 20,
			KeyCode_R = 21,
			KeyCode_S = 22,
			KeyCode_T = 23,
			KeyCode_U = 24,
			KeyCode_V = 25,
			KeyCode_W = 26,
			KeyCode_X = 27,
			KeyCode_Y = 28,
			KeyCode_Z = 29,
			KeyCode_1 = 30,
			KeyCode_2 = 31,
			KeyCode_3 = 32,
			KeyCode_4 = 33,
			KeyCode_5 = 34,
			KeyCode_6 = 35,
			KeyCode_7 = 36,
			KeyCode_8 = 37,
			KeyCode_9 = 38,
			KeyCode_0 = 39,
			KeyCode_RETURN = 40,
			KeyCode_ESCAPE = 41,
			KeyCode_BACKSPACE = 42,
			KeyCode_TAB = 43,
			KeyCode_SPACE = 44,
			KeyCode_MINUS = 45,
			KeyCode_EQUALS = 46,
			KeyCode_LEFTBRACKET = 47,
			KeyCode_RIGHTBRACKET = 48,
			KeyCode_BACKSLASH = 49,
			KeyCode_NONUSHASH = 50,
			KeyCode_SEMICOLON = 51,
			KeyCode_APOSTROPHE = 52,
			KeyCode_GRAVE = 53,
			KeyCode_COMMA = 54,
			KeyCode_PERIOD = 55,
			KeyCode_SLASH = 56,
			KeyCode_CAPSLOCK = 57,
			KeyCode_F1 = 58,
			KeyCode_F2 = 59,
			KeyCode_F3 = 60,
			KeyCode_F4 = 61,
			KeyCode_F5 = 62,
			KeyCode_F6 = 63,
			KeyCode_F7 = 64,
			KeyCode_F8 = 65,
			KeyCode_F9 = 66,
			KeyCode_F10 = 67,
			KeyCode_F11 = 68,
			KeyCode_F12 = 69,
			KeyCode_PRINTSCREEN = 70,
			KeyCode_SCROLLLOCK = 71,
			KeyCode_PAUSE = 72,
			KeyCode_INSERT = 73,
			KeyCode_HOME = 74,
			KeyCode_PAGEUP = 75,
			KeyCode_DELETE = 76,
			KeyCode_END = 77,
			KeyCode_PAGEDOWN = 78,
			KeyCode_RIGHT = 79,
			KeyCode_LEFT = 80,
			KeyCode_DOWN = 81,
			KeyCode_UP = 82,
			KeyCode_NUMLOCKCLEAR = 83,
			KeyCode_KP_DIVIDE = 84,
			KeyCode_KP_MULTIPLY = 85,
			KeyCode_KP_MINUS = 86,
			KeyCode_KP_PLUS = 87,
			KeyCode_KP_ENTER = 88,
			KeyCode_KP_1 = 89,
			KeyCode_KP_2 = 90,
			KeyCode_KP_3 = 91,
			KeyCode_KP_4 = 92,
			KeyCode_KP_5 = 93,
			KeyCode_KP_6 = 94,
			KeyCode_KP_7 = 95,
			KeyCode_KP_8 = 96,
			KeyCode_KP_9 = 97,
			KeyCode_KP_0 = 98,
			KeyCode_KP_PERIOD = 99,
			KeyCode_NONUSBACKSLASH = 100,
			KeyCode_APPLICATION = 101,
			KeyCode_POWER = 102,
			KeyCode_KP_EQUALS = 103,
			KeyCode_F13 = 104,
			KeyCode_F14 = 105,
			KeyCode_F15 = 106,
			KeyCode_F16 = 107,
			KeyCode_F17 = 108,
			KeyCode_F18 = 109,
			KeyCode_F19 = 110,
			KeyCode_F20 = 111,
			KeyCode_F21 = 112,
			KeyCode_F22 = 113,
			KeyCode_F23 = 114,
			KeyCode_F24 = 115,
			KeyCode_EXECUTE = 116,
			KeyCode_HELP = 117,
			KeyCode_MENU = 118,
			KeyCode_SELECT = 119,
			KeyCode_STOP = 120,
			KeyCode_AGAIN = 121,
			KeyCode_UNDO = 122,
			KeyCode_CUT = 123,
			KeyCode_COPY = 124,
			KeyCode_PASTE = 125,
			KeyCode_FIND = 126,
			KeyCode_MUTE = 127,
			KeyCode_VOLUMEUP = 128,
			KeyCode_VOLUMEDOWN = 129,
			KeyCode_KP_COMMA = 133,
			KeyCode_KP_EQUALSAS400 = 134,
			KeyCode_INTERNATIONAL1 = 135,
			KeyCode_INTERNATIONAL2 = 136,
			KeyCode_INTERNATIONAL3 = 137,
			KeyCode_INTERNATIONAL4 = 138,
			KeyCode_INTERNATIONAL5 = 139,
			KeyCode_INTERNATIONAL6 = 140,
			KeyCode_INTERNATIONAL7 = 141,
			KeyCode_INTERNATIONAL8 = 142,
			KeyCode_INTERNATIONAL9 = 143,
			KeyCode_LANG1 = 144,
			KeyCode_LANG2 = 145,
			KeyCode_LANG3 = 146,
			KeyCode_LANG4 = 147,
			KeyCode_LANG5 = 148,
			KeyCode_LANG6 = 149,
			KeyCode_LANG7 = 150,
			KeyCode_LANG8 = 151,
			KeyCode_LANG9 = 152,
			KeyCode_ALTERASE = 153,
			KeyCode_SYSREQ = 154,
			KeyCode_CANCEL = 155,
			KeyCode_CLEAR = 156,
			KeyCode_PRIOR = 157,
			KeyCode_RETURN2 = 158,
			KeyCode_SEPARATOR = 159,
			KeyCode_OUT = 160,
			KeyCode_OPER = 161,
			KeyCode_CLEARAGAIN = 162,
			KeyCode_CRSEL = 163,
			KeyCode_EXSEL = 164,
			KeyCode_KP_00 = 176,
			KeyCode_KP_000 = 177,
			KeyCode_THOUSANDSSEPARATOR = 178,
			KeyCode_DECIMALSEPARATOR = 179,
			KeyCode_CURRENCYUNIT = 180,
			KeyCode_CURRENCYSUBUNIT = 181,
			KeyCode_KP_LEFTPAREN = 182,
			KeyCode_KP_RIGHTPAREN = 183,
			KeyCode_KP_LEFTBRACE = 184,
			KeyCode_KP_RIGHTBRACE = 185,
			KeyCode_KP_TAB = 186,
			KeyCode_KP_BACKSPACE = 187,
			KeyCode_KP_A = 188,
			KeyCode_KP_B = 189,
			KeyCode_KP_C = 190,
			KeyCode_KP_D = 191,
			KeyCode_KP_E = 192,
			KeyCode_KP_F = 193,
			KeyCode_KP_XOR = 194,
			KeyCode_KP_POWER = 195,
			KeyCode_KP_PERCENT = 196,
			KeyCode_KP_LESS = 197,
			KeyCode_KP_GREATER = 198,
			KeyCode_KP_AMPERSAND = 199,
			KeyCode_KP_DBLAMPERSAND = 200,
			KeyCode_KP_VERTICALBAR = 201,
			KeyCode_KP_DBLVERTICALBAR = 202,
			KeyCode_KP_COLON = 203,
			KeyCode_KP_HASH = 204,
			KeyCode_KP_SPACE = 205,
			KeyCode_KP_AT = 206,
			KeyCode_KP_EXCLAM = 207,
			KeyCode_KP_MEMSTORE = 208,
			KeyCode_KP_MEMRECALL = 209,
			KeyCode_KP_MEMCLEAR = 210,
			KeyCode_KP_MEMADD = 211,
			KeyCode_KP_MEMSUBTRACT = 212,
			KeyCode_KP_MEMMULTIPLY = 213,
			KeyCode_KP_MEMDIVIDE = 214,
			KeyCode_KP_PLUSMINUS = 215,
			KeyCode_KP_CLEAR = 216,
			KeyCode_KP_CLEARENTRY = 217,
			KeyCode_KP_BINARY = 218,
			KeyCode_KP_OCTAL = 219,
			KeyCode_KP_DECIMAL = 220,
			KeyCode_KP_HEXADECIMAL = 221,
			KeyCode_LCTRL = 224,
			KeyCode_LSHIFT = 225,
			KeyCode_LALT = 226,
			KeyCode_LGUI = 227,
			KeyCode_RCTRL = 228,
			KeyCode_RSHIFT = 229,
			KeyCode_RALT = 230,
			KeyCode_RGUI = 231,
			KeyCode_MODE = 257,
			KeyCode_AUDIONEXT = 258,
			KeyCode_AUDIOPREV = 259,
			KeyCode_AUDIOSTOP = 260,
			KeyCode_AUDIOPLAY = 261,
			KeyCode_AUDIOMUTE = 262,
			KeyCode_MEDIASELECT = 263,
			KeyCode_WWW = 264,
			KeyCode_MAIL = 265,
			KeyCode_CALCULATOR = 266,
			KeyCode_COMPUTER = 267,
			KeyCode_AC_SEARCH = 268,
			KeyCode_AC_HOME = 269,
			KeyCode_AC_BACK = 270,
			KeyCode_AC_FORWARD = 271,
			KeyCode_AC_STOP = 272,
			KeyCode_AC_REFRESH = 273,
			KeyCode_AC_BOOKMARKS = 274,
			KeyCode_BRIGHTNESSDOWN = 275,
			KeyCode_BRIGHTNESSUP = 276,
			KeyCode_DISPLAYSWITCH = 277,
			KeyCode_KBDILLUMTOGGLE = 278,
			KeyCode_KBDILLUMDOWN = 279,
			KeyCode_KBDILLUMUP = 280,
			KeyCode_EJECT = 281,
			KeyCode_SLEEP = 282,
			KeyCode_APP1 = 283,
			KeyCode_APP2 = 284,
			KeyCode_AUDIOREWIND = 285,
			KeyCode_AUDIOFASTFORWARD = 286,
			KeyCode_CURSORLEFT = 287,
			KeyCode_CURSORMIDDLE = 288,
			KeyCode_CURSORRIGHT = 289,
			KeyCode_CURSORX1 = 290,
			KeyCode_CURSORX2 = 291,
			KeyCode_NONE = 292
		};

		enum KeyMod
		{
			KeyMod_NONE = 0x0000,
			KeyMod_LSHIFT = 0x0001,
			KeyMod_RSHIFT = 0x0002,
			KeyMod_LCTRL = 0x0040,
			KeyMod_RCTRL = 0x0080,
			KeyMod_LALT = 0x0100,
			KeyMod_RALT = 0x0200,
			KeyMod_LGUI = 0x0400,
			KeyMod_RGUI = 0x0800,
			KeyMod_NUM = 0x1000,
			KeyMod_CAPS = 0x2000,
			KeyMod_MODE = 0x4000,
			KeyMod_RESERVED = 0x8000,
			KeyMod_SHIFT = KeyMod_LSHIFT | KeyMod_RSHIFT,
			KeyMod_CTRL = KeyMod_LCTRL | KeyMod_RCTRL,
			KeyMod_ALT = KeyMod_LALT | KeyMod_RALT,
			KeyMod_GUI = KeyMod_LGUI | KeyMod_RGUI
		};

		enum AlertType
		{
			AlertType_None = 0,
			AlertType_Error = 0x00000010,
			AlertType_Warning = 0x00000020,
			AlertType_Info = 0x00000040
		};

		enum AlertConfirm
		{
			AlertConfirm_None = 0,
			AlertConfirm_Return = 0x00000002,
			AlertConfirm_Escape = 0x00000001
		};

		enum JoyStickHat
		{
			JoyStickHat_Center = 0x00,
			JoyStickHat_Up = 0x01,
			JoyStickHat_Right = 0x02,
			JoyStickHat_Down = 0x04,
			JoyStickHat_Left = 0x08,
			JoyStickHat_Right_Up = 0x02 | 0x01,
			JoyStickHat_Right_Down = 0x02 | 0x04,
			JoyStickHat_Left_Up = 0x08 | 0x01,
			JoyStickHat_Left_Down = 0x08 | 0x04
		};

		enum RenderBackend
		{
			RenderBackend_NONE,
			RenderBackend_D3D11,
			RenderBackend_OGL
		};

		enum VSync
		{
			VSync_Disabled,
			VSync_Frequency_X1,
			VSync_Frequency_X2,
			VSync_Frequency_X3,
			VSync_Frequency_X4
		};

		enum SurfaceTarget
		{
			SurfaceTarget0 = 1,
			SurfaceTarget1 = 2,
			SurfaceTarget2 = 3,
			SurfaceTarget3 = 4,
			SurfaceTarget4 = 5,
			SurfaceTarget5 = 6,
			SurfaceTarget6 = 7,
			SurfaceTarget7 = 8,
		};

		enum PrimitiveTopology
		{
			PrimitiveTopology_Invalid,
			PrimitiveTopology_Point_List,
			PrimitiveTopology_Line_List,
			PrimitiveTopology_Line_Strip,
			PrimitiveTopology_Triangle_List,
			PrimitiveTopology_Triangle_Strip,
			PrimitiveTopology_Line_List_Adj,
			PrimitiveTopology_Line_Strip_Adj,
			PrimitiveTopology_Triangle_List_Adj,
			PrimitiveTopology_Triangle_Strip_Adj
		};

		enum Format
		{
			Format_Invalid = 0,
			Format_R32G32B32A32_Typeless = 1,
			Format_R32G32B32A32_Float = 2,
			Format_R32G32B32A32_Uint = 3,
			Format_R32G32B32A32_Sint = 4,
			Format_R32G32B32_Typeless = 5,
			Format_R32G32B32_Float = 6,
			Format_R32G32B32_Uint = 7,
			Format_R32G32B32_Sint = 8,
			Format_R16G16B16A16_Typeless = 9,
			Format_R16G16B16A16_Float = 10,
			Format_R16G16B16A16_Unorm = 11,
			Format_R16G16B16A16_Uint = 12,
			Format_R16G16B16A16_Snorm = 13,
			Format_R16G16B16A16_Sint = 14,
			Format_R32G32_Typeless = 15,
			Format_R32G32_Float = 16,
			Format_R32G32_Uint = 17,
			Format_R32G32_Sint = 18,
			Format_R32G8X24_Typeless = 19,
			Format_D32_Float_S8X24_Uint = 20,
			Format_R32_Float_X8X24_Typeless = 21,
			Format_X32_Typeless_G8X24_Uint = 22,
			Format_R10G10B10A2_Typeless = 23,
			Format_R10G10B10A2_Unorm = 24,
			Format_R10G10B10A2_Uint = 25,
			Format_R11G11B10_Float = 26,
			Format_R8G8B8A8_Typeless = 27,
			Format_R8G8B8A8_Unorm = 28,
			Format_R8G8B8A8_Unorm_SRGB = 29,
			Format_R8G8B8A8_Uint = 30,
			Format_R8G8B8A8_Snorm = 31,
			Format_R8G8B8A8_Sint = 32,
			Format_R16G16_Typeless = 33,
			Format_R16G16_Float = 34,
			Format_R16G16_Unorm = 35,
			Format_R16G16_Uint = 36,
			Format_R16G16_Snorm = 37,
			Format_R16G16_Sint = 38,
			Format_R32_Typeless = 39,
			Format_D32_Float = 40,
			Format_R32_Float = 41,
			Format_R32_Uint = 42,
			Format_R32_Sint = 43,
			Format_R24G8_Typeless = 44,
			Format_D24_Unorm_S8_Uint = 45,
			Format_R24_Unorm_X8_Typeless = 46,
			Format_X24_Typeless_G8_Uint = 47,
			Format_R8G8_Typeless = 48,
			Format_R8G8_Unorm = 49,
			Format_R8G8_Uint = 50,
			Format_R8G8_Snorm = 51,
			Format_R8G8_Sint = 52,
			Format_R16_Typeless = 53,
			Format_R16_Float = 54,
			Format_D16_Unorm = 55,
			Format_R16_Unorm = 56,
			Format_R16_Uint = 57,
			Format_R16_Snorm = 58,
			Format_R16_Sint = 59,
			Format_R8_Typeless = 60,
			Format_R8_Unorm = 61,
			Format_R8_Uint = 62,
			Format_R8_Snorm = 63,
			Format_R8_Sint = 64,
			Format_A8_Unorm = 65,
			Format_R1_Unorm = 66,
			Format_R9G9B9E5_Share_Dexp = 67,
			Format_R8G8_B8G8_Unorm = 68,
			Format_G8R8_G8B8_Unorm = 69,
			Format_BC1_Typeless = 70,
			Format_BC1_Unorm = 71,
			Format_BC1_Unorm_SRGB = 72,
			Format_BC2_Typeless = 73,
			Format_BC2_Unorm = 74,
			Format_BC2_Unorm_SRGB = 75,
			Format_BC3_Typeless = 76,
			Format_BC3_Unorm = 77,
			Format_BC3_Unorm_SRGB = 78,
			Format_BC4_Typeless = 79,
			Format_BC4_Unorm = 80,
			Format_BC4_Snorm = 81,
			Format_BC5_Typeless = 82,
			Format_BC5_Unorm = 83,
			Format_BC5_Snorm = 84,
			Format_B5G6R5_Unorm = 85,
			Format_B5G5R5A1_Unorm = 86,
			Format_B8G8R8A8_Unorm = 87,
			Format_B8G8R8X8_Unorm = 88,
			Format_R10G10B10_XR_Bias_A2_Unorm = 89,
			Format_B8G8R8A8_Typeless = 90,
			Format_B8G8R8A8_Unorm_SRGB = 91,
			Format_B8G8R8X8_Typeless = 92,
			Format_B8G8R8X8_Unorm_SRGB = 93,
			Format_BC6H_Typeless = 94,
			Format_BC6H_UF16 = 95,
			Format_BC6H_SF16 = 96,
			Format_BC7_Typeless = 97,
			Format_BC7_Unorm = 98,
			Format_BC7_Unorm_SRGB = 99,
			Format_AYUV = 100,
			Format_Y410 = 101,
			Format_Y416 = 102,
			Format_NV12 = 103,
			Format_P010 = 104,
			Format_P016 = 105,
			Format_420_OPAQUE = 106,
			Format_YUY2 = 107,
			Format_Y210 = 108,
			Format_Y216 = 109,
			Format_NV11 = 110,
			Format_AI44 = 111,
			Format_IA44 = 112,
			Format_P8 = 113,
			Format_A8P8 = 114,
			Format_B4G4R4A4_Unorm = 115,
			Format_P208 = 130,
			Format_V208 = 131,
			Format_V408 = 132,
			Format_FOE_Uint = 0xffffffff
		};

		enum ResourceMap
		{
			ResourceMap_Read = 1,
			ResourceMap_Write = 2,
			ResourceMap_Read_Write = 3,
			ResourceMap_Write_Discard = 4,
			ResourceMap_Write_No_Overwrite = 5
		};

		enum ResourceUsage
		{
			ResourceUsage_Default = 0,
			ResourceUsage_Immutable = 1,
			ResourceUsage_Dynamic = 2,
			ResourceUsage_Staging = 3
		};

		enum ShaderModel
		{
			ShaderModel_Invalid,
			ShaderModel_Auto,
			ShaderModel_HLSL_1_0,
			ShaderModel_HLSL_2_0,
			ShaderModel_HLSL_3_0,
			ShaderModel_HLSL_4_1,
			ShaderModel_HLSL_4_0,
			ShaderModel_HLSL_5_0,
			ShaderModel_GLSL_1_1_0,
			ShaderModel_GLSL_1_2_0,
			ShaderModel_GLSL_1_3_0,
			ShaderModel_GLSL_1_4_0,
			ShaderModel_GLSL_1_5_0,
			ShaderModel_GLSL_3_3_0,
			ShaderModel_GLSL_4_0_0,
			ShaderModel_GLSL_4_1_0,
			ShaderModel_GLSL_4_2_0,
			ShaderModel_GLSL_4_3_0,
			ShaderModel_GLSL_4_4_0,
			ShaderModel_GLSL_4_5_0,
			ShaderModel_GLSL_4_6_0
		};

		enum ResourceBind
		{
			ResourceBind_Vertex_Buffer = 0x1L,
			ResourceBind_Index_Buffer = 0x2L,
			ResourceBind_Constant_Buffer = 0x4L,
			ResourceBind_Shader_Input = 0x8L,
			ResourceBind_Stream_Output = 0x10L,
			ResourceBind_Render_Target = 0x20L,
			ResourceBind_Depth_Stencil = 0x40L,
			ResourceBind_Unordered_Access = 0x80L
		};

		enum CPUAccess
		{
			CPUAccess_Invalid = 0,
			CPUAccess_Write = 0x10000L,
			CPUAccess_Read = 0x20000L
		};

		enum DepthWrite
		{
			DepthWrite_Zero,
			DepthWrite_All
		};

		enum Comparison
		{
			Comparison_Never = 1,
			Comparison_Less = 2,
			Comparison_Equal = 3,
			Comparison_Less_Equal = 4,
			Comparison_Greater = 5,
			Comparison_Not_Equal = 6,
			Comparison_Greater_Equal = 7,
			Comparison_Always = 8
		};

		enum StencilOperation
		{
			StencilOperation_Keep = 1,
			StencilOperation_Zero = 2,
			StencilOperation_Replace = 3,
			StencilOperation_SAT_Add = 4,
			StencilOperation_SAT_Subtract = 5,
			StencilOperation_Invert = 6,
			StencilOperation_Add = 7,
			StencilOperation_Subtract = 8
		};

		enum Blend
		{
			Blend_Zero = 1,
			Blend_One = 2,
			Blend_Source_Color = 3,
			Blend_Source_Color_Invert = 4,
			Blend_Source_Alpha = 5,
			Blend_Source_Alpha_Invert = 6,
			Blend_Destination_Alpha = 7,
			Blend_Destination_Alpha_Invert = 8,
			Blend_Destination_Color = 9,
			Blend_Destination_Color_Invert = 10,
			Blend_Source_Alpha_SAT = 11,
			Blend_Blend_Factor = 14,
			Blend_Blend_Factor_Invert = 15,
			Blend_Source1_Color = 16,
			Blend_Source1_Color_Invert = 17,
			Blend_Source1_Alpha = 18,
			Blend_Source1_Alpha_Invert = 19
		};

		enum SurfaceFill
		{
			SurfaceFill_Wireframe = 2,
			SurfaceFill_Solid = 3
		};

		enum PixelFilter
		{
			PixelFilter_Min_Mag_Mip_Point = 0,
			PixelFilter_Min_Mag_Point_Mip_Linear = 0x1,
			PixelFilter_Min_Point_Mag_Linear_Mip_Point = 0x4,
			PixelFilter_Min_Point_Mag_Mip_Linear = 0x5,
			PixelFilter_Min_Linear_Mag_Mip_Point = 0x10,
			PixelFilter_Min_Linear_Mag_Point_Mip_Linear = 0x11,
			PixelFilter_Min_Mag_Linear_Mip_Point = 0x14,
			PixelFilter_Min_Mag_Mip_Linear = 0x15,
			PixelFilter_Anistropic = 0x55,
			PixelFilter_Compare_Min_Mag_Mip_Point = 0x80,
			PixelFilter_Compare_Min_Mag_Point_Mip_Linear = 0x81,
			PixelFilter_Compare_Min_Point_Mag_Linear_Mip_Point = 0x84,
			PixelFilter_Compare_Min_Point_Mag_Mip_Linear = 0x85,
			PixelFilter_Compare_Min_Linear_Mag_Mip_Point = 0x90,
			PixelFilter_Compare_Min_Linear_Mag_Point_Mip_Linear = 0x91,
			PixelFilter_Compare_Min_Mag_Linear_Mip_Point = 0x94,
			PixelFilter_Compare_Min_Mag_Mip_Linear = 0x95,
			PixelFilter_Compare_Anistropic = 0xd5
		};

		enum TextureAddress
		{
			TextureAddress_Wrap = 1,
			TextureAddress_Mirror = 2,
			TextureAddress_Clamp = 3,
			TextureAddress_Border = 4,
			TextureAddress_Mirror_Once = 5
		};

		enum ColorWriteEnable
		{
			ColorWriteEnable_Red = 1,
			ColorWriteEnable_Green = 2,
			ColorWriteEnable_Blue = 4,
			ColorWriteEnable_Alpha = 8,
			ColorWriteEnable_All = (((ColorWriteEnable_Red | ColorWriteEnable_Green) | ColorWriteEnable_Blue) | ColorWriteEnable_Alpha)
		};

		enum BlendOperation
		{
			BlendOperation_Add = 1,
			BlendOperation_Subtract = 2,
			BlendOperation_Subtract_Reverse = 3,
			BlendOperation_Min = 4,
			BlendOperation_Max = 5
		};

		enum VertexCull
		{
			VertexCull_Disabled = 1,
			VertexCull_Front = 2,
			VertexCull_Back = 3
		};

		enum ShaderCompile
		{
			ShaderCompile_Debug = 1ll << 0,
			ShaderCompile_Skip_Validation = 1ll << 1,
			ShaderCompile_Skip_Optimization = 1ll << 2,
			ShaderCompile_Matrix_Row_Major = 1ll << 3,
			ShaderCompile_Matrix_Column_Major = 1ll << 4,
			ShaderCompile_Partial_Precision = 1ll << 5,
			ShaderCompile_FOE_VS_No_OPT = 1ll << 6,
			ShaderCompile_FOE_PS_No_OPT = 1ll << 7,
			ShaderCompile_No_Preshader = 1ll << 8,
			ShaderCompile_Avoid_Flow_Control = 1ll << 9,
			ShaderCompile_Prefer_Flow_Control = 1ll << 10,
			ShaderCompile_Enable_Strictness = 1ll << 11,
			ShaderCompile_Enable_Backwards_Compatibility = 1ll << 12,
			ShaderCompile_IEEE_Strictness = 1ll << 13,
			ShaderCompile_Optimization_Level0 = 1ll << 14,
			ShaderCompile_Optimization_Level1 = 0,
			ShaderCompile_Optimization_Level2 = (1ll << 14) | (1ll << 15),
			ShaderCompile_Optimization_Level3 = 1ll << 15,
			ShaderCompile_Reseed_X16 = 1ll << 16,
			ShaderCompile_Reseed_X17 = 1ll << 17,
			ShaderCompile_Picky = 1ll << 18
		};

		enum RenderBufferType
		{
			RenderBufferType_Animation,
			RenderBufferType_Render,
			RenderBufferType_View
		};

		enum ResourceMisc
		{
			ResourceMisc_None = 0,
			ResourceMisc_Generate_Mips = 0x1L,
			ResourceMisc_Shared = 0x2L,
			ResourceMisc_Texture_Cube = 0x4L,
			ResourceMisc_Draw_Indirect_Args = 0x10L,
			ResourceMisc_Buffer_Allow_Raw_Views = 0x20L,
			ResourceMisc_Buffer_Structured = 0x40L,
			ResourceMisc_Clamp = 0x80L,
			ResourceMisc_Shared_Keyed_Mutex = 0x100L,
			ResourceMisc_GDI_Compatible = 0x200L,
			ResourceMisc_Shared_NT_Handle = 0x800L,
			ResourceMisc_Restricted_Content = 0x1000L,
			ResourceMisc_Restrict_Shared = 0x2000L,
			ResourceMisc_Restrict_Shared_Driver = 0x4000L,
			ResourceMisc_Guarded = 0x8000L,
			ResourceMisc_Tile_Pool = 0x20000L,
			ResourceMisc_Tiled = 0x40000L
		};

		enum DisplayCursor
		{
			DisplayCursor_None = -1,
			DisplayCursor_Arrow = 0,
			DisplayCursor_TextInput,
			DisplayCursor_ResizeAll,
			DisplayCursor_ResizeNS,
			DisplayCursor_ResizeEW,
			DisplayCursor_ResizeNESW,
			DisplayCursor_ResizeNWSE,
			DisplayCursor_Hand,
			DisplayCursor_Count
		};

		enum ShaderType
		{
			ShaderType_Vertex = 1,
			ShaderType_Pixel = 2,
			ShaderType_Geometry = 4,
			ShaderType_Hull = 8,
			ShaderType_Domain = 16,
			ShaderType_Compute = 32,
			ShaderType_All = ShaderType_Vertex | ShaderType_Pixel | ShaderType_Geometry | ShaderType_Hull | ShaderType_Domain | ShaderType_Compute
		};

		enum AttributeType
		{
			AttributeType_Uint,
			AttributeType_Int,
			AttributeType_Half,
			AttributeType_Float
		};

		typedef std::function<void(AppState)> AppStateChangeCallback;
		typedef std::function<void(WindowState, int, int)> WindowStateChangeCallback;
		typedef std::function<void(KeyCode, KeyMod, int, int, bool)> KeyStateCallback;
		typedef std::function<void(char*, int, int)> InputEditCallback;
		typedef std::function<void(char*, int)> InputCallback;
		typedef std::function<void(int, int, int, int)> CursorMoveCallback;
		typedef std::function<void(int, int, bool)> CursorWheelStateCallback;
		typedef std::function<void(int, int, int)> JoyStickAxisMoveCallback;
		typedef std::function<void(int, int, int, int)> JoyStickBallMoveCallback;
		typedef std::function<void(enum JoyStickHat, int, int)> JoyStickHatMoveCallback;
		typedef std::function<void(int, int, bool)> JoyStickKeyStateCallback;
		typedef std::function<void(int, bool)> JoyStickStateCallback;
		typedef std::function<void(int, int, int)> ControllerAxisMoveCallback;
		typedef std::function<void(int, int, bool)> ControllerKeyStateCallback;
		typedef std::function<void(int, int)> ControllerStateCallback;
		typedef std::function<void(int, int, float, float, float, float, float)> TouchMoveCallback;
		typedef std::function<void(int, int, float, float, float, float, float, bool)> TouchStateCallback;
		typedef std::function<void(int, int, int, float, float, float, bool)> GestureStateCallback;
		typedef std::function<void(int, int, float, float, float, float)> MultiGestureStateCallback;
		typedef std::function<void(const std::string&)> DropFileCallback;
		typedef std::function<void(const std::string&)> DropTextCallback;

		class Shader;

		class GraphicsDevice;

		class Activity;

		struct TH_OUT Alert
		{
			friend Activity;

		private:
			struct Element
			{
				std::string Name;
				int Id, Flags;
			};

		private:
			std::string Name;
			std::string Data;
			std::vector<Element> Buttons;
			std::function<void(int)> Done;
			AlertType View;
			Activity* Base;
			bool Waiting;

		public:
			Alert(Activity* From);
			void Setup(AlertType Type, const std::string& Title, const std::string& Text);
			void Button(AlertConfirm Confirm, const std::string& Text, int Id);
			void Result(const std::function<void(int)>& Callback);

		private:
			void Dispatch();
		};

		struct TH_OUT KeyMap
		{
			KeyCode Key;
			KeyMod Mod;
			bool Normal;

			KeyMap();
			KeyMap(const KeyCode& Value);
			KeyMap(const KeyMod& Value);
			KeyMap(const KeyCode& Value, const KeyMod& Control);
		};

		struct TH_OUT MappedSubresource
		{
			void* Pointer;
			unsigned int RowPitch;
			unsigned int DepthPitch;
		};

		struct TH_OUT Viewport
		{
			float TopLeftX;
			float TopLeftY;
			float Width;
			float Height;
			float MinDepth;
			float MaxDepth;
		};

		struct TH_OUT Rectangle
		{
			long Left;
			long Top;
			long Right;
			long Bottom;
		};

		struct TH_OUT RenderTargetBlendState
		{
			bool BlendEnable;
			Blend SrcBlend;
			Blend DestBlend;
			BlendOperation BlendOperationMode;
			Blend SrcBlendAlpha;
			Blend DestBlendAlpha;
			BlendOperation BlendOperationAlpha;
			unsigned char RenderTargetWriteMask;
		};

		struct TH_OUT PoseNode
		{	
			Compute::Vector3 Position;
			Compute::Vector3 Rotation;
		};

		struct TH_OUT AnimationBuffer
		{
			Compute::Matrix4x4 Offsets[96];
			float HasAnimation;
			Compute::Vector3 Padding;
		};

		struct TH_OUT RenderBuffer
		{
			Compute::Matrix4x4 WorldViewProjection;
			Compute::Matrix4x4 World;
			Compute::Vector3 Diffuse;
			float HasDiffuse = 0.0f;
			Compute::Vector2 TexCoord;
			float HasNormal = 0.0f;
			float MaterialId = 0.0f;
			float HasHeight = 0.0f;
			float HeightAmount = 0.0f;
			float HeightBias = 0.0f;
			float Alignment = 0.0f;
		};

		struct TH_OUT ViewBuffer
		{
			Compute::Matrix4x4 InvViewProjection;
			Compute::Matrix4x4 ViewProjection;
			Compute::Matrix4x4 Projection;
			Compute::Matrix4x4 View;
			Compute::Vector3 ViewPosition;
			float FarPlane;
		};

		struct TH_OUT PoseBuffer
		{
			std::unordered_map<int64_t, PoseNode> Pose;
			Compute::Matrix4x4 Transform[96];

			bool SetPose(class SkinModel* Model);
			bool GetPose(class SkinModel* Model, std::vector<Compute::AnimatorKey>* Result);
			Compute::Matrix4x4 GetOffset(PoseNode* Node);
			PoseNode* GetNode(int64_t Index);

		private:
			void SetJointPose(Compute::Joint* Root);
			void GetJointPose(Compute::Joint* Root, std::vector<Compute::AnimatorKey>* Result);
		};

		class TH_OUT Surface
		{
		private:
			SDL_Surface* Handle;

		public:
			Surface();
			Surface(SDL_Surface* From);
			~Surface();
			void SetHandle(SDL_Surface* From);
			void Lock();
			void Unlock();
			int GetWidth();
			int GetHeight();
			int GetPitch();
			void* GetPixels();
			void* GetResource();
		};

		class TH_OUT DepthStencilState : public Rest::Object
		{
		public:
			struct Desc
			{
				StencilOperation FrontFaceStencilFailOperation;
				StencilOperation FrontFaceStencilDepthFailOperation;
				StencilOperation FrontFaceStencilPassOperation;
				Comparison FrontFaceStencilFunction;
				StencilOperation BackFaceStencilFailOperation;
				StencilOperation BackFaceStencilDepthFailOperation;
				StencilOperation BackFaceStencilPassOperation;
				Comparison BackFaceStencilFunction;
				DepthWrite DepthWriteMask;
				Comparison DepthFunction;
				unsigned char StencilReadMask;
				unsigned char StencilWriteMask;
				bool DepthEnable;
				bool StencilEnable;
			};

		protected:
			Desc State;

		protected:
			DepthStencilState(const Desc& I);

		public:
			virtual ~DepthStencilState() override;
			virtual void* GetResource() = 0;
			Desc GetState();
		};

		class TH_OUT RasterizerState : public Rest::Object
		{
		public:
			struct Desc
			{
				SurfaceFill FillMode;
				VertexCull CullMode;
				float DepthBiasClamp;
				float SlopeScaledDepthBias;
				int DepthBias;
				bool FrontCounterClockwise;
				bool DepthClipEnable;
				bool ScissorEnable;
				bool MultisampleEnable;
				bool AntialiasedLineEnable;
			};

		protected:
			Desc State;

		protected:
			RasterizerState(const Desc& I);

		public:
			virtual ~RasterizerState() override;
			virtual void* GetResource() = 0;
			Desc GetState();
		};

		class TH_OUT BlendState : public Rest::Object
		{
		public:
			struct Desc
			{
				RenderTargetBlendState RenderTarget[8];
				bool AlphaToCoverageEnable;
				bool IndependentBlendEnable;
			};

		protected:
			Desc State;

		protected:
			BlendState(const Desc& I);

		public:
			virtual ~BlendState() override;
			virtual void* GetResource() = 0;
			Desc GetState();
		};

		class TH_OUT SamplerState : public Rest::Object
		{
		public:
			struct Desc
			{
				Comparison ComparisonFunction;
				TextureAddress AddressU;
				TextureAddress AddressV;
				TextureAddress AddressW;
				PixelFilter Filter;
				float MipLODBias;
				unsigned int MaxAnisotropy;
				float BorderColor[4];
				float MinLOD;
				float MaxLOD;
			};

		protected:
			Desc State;

		protected:
			SamplerState(const Desc& I);

		public:
			virtual ~SamplerState() override;
			virtual void* GetResource() = 0;
			Desc GetState();
		};

		class TH_OUT InputLayout : public Rest::Object
		{
		public:
			struct Attribute
			{
				const char* SemanticName;
				unsigned int SemanticIndex;
				AttributeType Format;
				unsigned int Components;
				unsigned int AlignedByteOffset;
			};

		public:
			struct Desc
			{
				std::vector<Attribute> Attributes;
				Shader* Source = nullptr;
			};

		protected:
			std::vector<Attribute> Layout;

		protected:
			InputLayout(const Desc& I);

		public:
			virtual ~InputLayout() override;
			virtual void* GetResource() = 0;
			const std::vector<Attribute>& GetAttributes();
		};

		class TH_OUT Shader : public Rest::Object
		{
		public:
			struct Desc
			{
				Compute::ProcIncludeCallback Include = nullptr;
				Compute::Preprocessor::Desc Features;
				std::vector<std::string> Defines;
				std::string Filename;
				std::string Data;
			};

		protected:
			Shader(const Desc& I);

		public:
			virtual ~Shader() = default;
			virtual bool IsValid() = 0;
		};

		class TH_OUT ElementBuffer : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = ResourceBind_Vertex_Buffer;
				ResourceMisc MiscFlags = ResourceMisc_None;
				unsigned int StructureByteStride = 0;
				unsigned int ElementWidth = 0;
				unsigned int ElementCount = 0;
				void* Elements = nullptr;
			};

		protected:
			uint64_t Elements;
			uint64_t Stride;

		protected:
			ElementBuffer(const Desc& I);

		public:
			virtual ~ElementBuffer() = default;
			virtual void* GetResource() = 0;
			uint64_t GetElements();
			uint64_t GetStride();
		};

		class TH_OUT MeshBuffer : public Rest::Object
		{
		public:
			struct Desc
			{
				std::vector<Compute::Vertex> Elements;
				std::vector<int> Indices;
				CPUAccess AccessFlags = CPUAccess_Invalid;
				ResourceUsage Usage = ResourceUsage_Default;
			};

		protected:
			ElementBuffer* VertexBuffer;
			ElementBuffer* IndexBuffer;

		public:
			Compute::Matrix4x4 World;
			std::string Name;

		protected:
			MeshBuffer(const Desc& I);

		public:
			virtual ~MeshBuffer() override;
			virtual Compute::Vertex* GetElements(GraphicsDevice* Device) = 0;
			ElementBuffer* GetVertexBuffer();
			ElementBuffer* GetIndexBuffer();
		};

		class TH_OUT SkinMeshBuffer : public Rest::Object
		{
		public:
			struct Desc
			{
				std::vector<Compute::SkinVertex> Elements;
				std::vector<int> Indices;
				CPUAccess AccessFlags = CPUAccess_Invalid;
				ResourceUsage Usage = ResourceUsage_Default;
			};

		protected:
			ElementBuffer* VertexBuffer;
			ElementBuffer* IndexBuffer;

		public:
			Compute::Matrix4x4 World;
			std::string Name;

		protected:
			SkinMeshBuffer(const Desc& I);

		public:
			virtual ~SkinMeshBuffer() override;
			virtual Compute::SkinVertex* GetElements(GraphicsDevice* Device) = 0;
			ElementBuffer* GetVertexBuffer();
			ElementBuffer* GetIndexBuffer();
		};

		class TH_OUT InstanceBuffer : public Rest::Object
		{
		public:
			struct Desc
			{
				GraphicsDevice* Device = nullptr;
				unsigned int ElementWidth = sizeof(Compute::ElementVertex);
				unsigned int ElementLimit = 100;
			};

		protected:
			Rest::Pool<Compute::ElementVertex> Array;
			ElementBuffer* Elements;
			GraphicsDevice* Device;
			uint64_t ElementLimit;
			uint64_t ElementWidth;
			bool Sync;

		protected:
			InstanceBuffer(const Desc& I);

		public:
			virtual ~InstanceBuffer();
			Rest::Pool <Compute::ElementVertex>* GetArray();
			ElementBuffer* GetElements();
			GraphicsDevice* GetDevice();
			uint64_t GetElementLimit();
		};

		class TH_OUT Texture2D : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				Format FormatMode = Format_R8G8B8A8_Unorm;
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = ResourceBind_Shader_Input;
				ResourceMisc MiscFlags = ResourceMisc_None;
				void* Data = nullptr;
				unsigned int RowPitch = 0;
				unsigned int DepthPitch = 0;
				unsigned int Width = 512;
				unsigned int Height = 512;
				int MipLevels = 1;
			};

		protected:
			CPUAccess AccessFlags;
			Format FormatMode;
			ResourceUsage Usage;
			unsigned int Width, Height;
			unsigned int MipLevels;

		protected:
			Texture2D();
			Texture2D(const Desc& I);

		public:
			virtual ~Texture2D() = default;
			virtual void* GetResource() = 0;
			CPUAccess GetAccessFlags();
			Format GetFormatMode();
			ResourceUsage GetUsage();
			unsigned int GetWidth();
			unsigned int GetHeight();
			unsigned int GetMipLevels();
		};

		class TH_OUT Texture3D : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				Format FormatMode = Format_R8G8B8A8_Unorm;
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = ResourceBind_Shader_Input;
				ResourceMisc MiscFlags = ResourceMisc_None;
				unsigned int Width = 512;
				unsigned int Height = 512;
				unsigned int Depth = 1;
				int MipLevels = 1;
			};

		protected:
			CPUAccess AccessFlags;
			Format FormatMode;
			ResourceUsage Usage;
			unsigned int Width, Height;
			unsigned int MipLevels;
			unsigned int Depth;

		protected:
			Texture3D();

		public:
			virtual ~Texture3D() = default;
			virtual void* GetResource() = 0;
			CPUAccess GetAccessFlags();
			Format GetFormatMode();
			ResourceUsage GetUsage();
			unsigned int GetWidth();
			unsigned int GetHeight();
			unsigned int GetDepth();
			unsigned int GetMipLevels();
		};

		class TH_OUT TextureCube : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				Format FormatMode = Format_R8G8B8A8_Unorm;
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = ResourceBind_Shader_Input;
				ResourceMisc MiscFlags = ResourceMisc_Texture_Cube;
				unsigned int Width = 128;
				unsigned int Height = 128;
				int MipLevels = 1;
			};

		protected:
			CPUAccess AccessFlags;
			Format FormatMode;
			ResourceUsage Usage;
			unsigned int Width, Height;
			unsigned int MipLevels;

		protected:
			TextureCube();
			TextureCube(const Desc& I);

		public:
			virtual ~TextureCube() = default;
			virtual void* GetResource() = 0;
			CPUAccess GetAccessFlags();
			Format GetFormatMode();
			ResourceUsage GetUsage();
			unsigned int GetWidth();
			unsigned int GetHeight();
			unsigned int GetMipLevels();
		};

		class TH_OUT DepthBuffer : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				ResourceUsage Usage = ResourceUsage_Default;
				unsigned int Width = 512;
				unsigned int Height = 512;
			};

		protected:
			DepthBuffer(const Desc& I);

		public:
			virtual ~DepthBuffer();
			virtual void* GetResource() = 0;
			virtual Viewport GetViewport() = 0;
			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
		};

		class TH_OUT RenderTarget2D : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				Format FormatMode = Format_B8G8R8A8_Unorm;
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = (ResourceBind)(ResourceBind_Render_Target | ResourceBind_Shader_Input);
				ResourceMisc MiscFlags = ResourceMisc_None;
				void* RenderSurface = nullptr;
				unsigned int Width = 512;
				unsigned int Height = 512;
				unsigned int MipLevels = 1;
			};

		protected:
			Texture2D* Resource;

		protected:
			RenderTarget2D(const Desc& I);

		public:
			virtual ~RenderTarget2D();
			virtual void* GetResource() = 0;
			virtual Viewport GetViewport() = 0;
			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
			Texture2D* GetTarget();
		};

		class TH_OUT MultiRenderTarget2D : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				SurfaceTarget SVTarget = SurfaceTarget0;
				Format FormatMode[8] = { Format_B8G8R8A8_Unorm };
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = (ResourceBind)(ResourceBind_Render_Target | ResourceBind_Shader_Input);
				ResourceMisc MiscFlags = ResourceMisc_None;
				unsigned int Width = 512;
				unsigned int Height = 512;
				unsigned int MipLevels = 1;
			};

		protected:
			SurfaceTarget SVTarget;
			Texture2D* Resource[8];

		protected:
			MultiRenderTarget2D(const Desc& I);

		public:
			virtual ~MultiRenderTarget2D();
			virtual Viewport GetViewport() = 0;
			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
			virtual void* GetResource(int Id) = 0;
			SurfaceTarget GetSVTarget();
			Texture2D* GetTarget(unsigned int Target);
		};

		class TH_OUT RenderTargetCube : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				Format FormatMode = Format_R8G8B8A8_Unorm;
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = (ResourceBind)(ResourceBind_Render_Target | ResourceBind_Shader_Input);
				ResourceMisc MiscFlags = (ResourceMisc)(ResourceMisc_Generate_Mips | ResourceMisc_Texture_Cube);
				unsigned int Size = 512;
				unsigned int MipLevels = 1;
			};

		protected:
			Texture2D* Resource;

		protected:
			RenderTargetCube(const Desc& I);

		public:
			virtual ~RenderTargetCube();
			virtual Viewport GetViewport() = 0;
			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
			virtual void* GetResource() = 0;
			Texture2D* GetTarget();
		};

		class TH_OUT MultiRenderTargetCube : public Rest::Object
		{
		public:
			struct Desc
			{
				CPUAccess AccessFlags = CPUAccess_Invalid;
				SurfaceTarget Target = SurfaceTarget0;
				Format FormatMode[8] = { Format_R8G8B8A8_Unorm };
				ResourceUsage Usage = ResourceUsage_Default;
				ResourceBind BindFlags = (ResourceBind)(ResourceBind_Render_Target | ResourceBind_Shader_Input);
				ResourceMisc MiscFlags = (ResourceMisc)(ResourceMisc_Generate_Mips | ResourceMisc_Texture_Cube);
				unsigned int MipLevels = 1;
				unsigned int Size = 512;
			};

		protected:
			SurfaceTarget SVTarget;
			Texture2D* Resource[8];

		protected:
			MultiRenderTargetCube(const Desc& I);

		public:
			virtual ~MultiRenderTargetCube();
			virtual Viewport GetViewport() = 0;
			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
			virtual void* GetResource(int Id) = 0;
			SurfaceTarget GetSVTarget();
			Texture2D* GetTarget(unsigned int Target);
		};

		class TH_OUT Query : public Rest::Object
		{
		public:
			struct Desc
			{
				bool Predicate = true;
				bool AutoPass = false;
			};

		protected:
			Query();

		public:
			virtual ~Query() = default;
			virtual void* GetResource() = 0;
		};

		class TH_OUT GraphicsDevice : public Rest::Object
		{
		protected:
			struct DirectBuffer
			{
				Compute::Matrix4x4 WorldViewProjection;
				Compute::Vector4 Padding;
			};

			struct Vertex
			{
				float PX, PY, PZ;
				float TX, TY;
				float CX, CY, CZ, CW;
			};

		public:
			struct Desc
			{
				RenderBackend Backend = Graphics::RenderBackend_NONE;
				ShaderModel ShaderMode = ShaderModel_Auto;
				Format BufferFormat = Format_R8G8B8A8_Unorm;
				VSync VSyncMode = VSync_Frequency_X1;
				int IsWindowed = 1;
				bool Debug = false;
				unsigned int PresentationFlags = 0;
				unsigned int CompilationFlags = ShaderCompile_Enable_Strictness | ShaderCompile_Optimization_Level3 | ShaderCompile_Matrix_Row_Major;
				unsigned int CreationFlags = 0;
				unsigned int BufferWidth = 0;
				unsigned int BufferHeight = 0;
				Activity* Window = nullptr;
			};

			struct Section
			{
				std::string Name;
				std::string Code;
			};

		protected:
			std::unordered_map<std::string, DepthStencilState*> DepthStencilStates;
			std::unordered_map<std::string, RasterizerState*> RasterizerStates;
			std::unordered_map<std::string, BlendState*> BlendStates;
			std::unordered_map<std::string, SamplerState*> SamplerStates;
			std::unordered_map<std::string, InputLayout*> InputLayouts;
			std::unordered_map<std::string, Section*> Sections;
			PrimitiveTopology Primitives;
			ShaderModel ShaderModelType;
			Texture2D* ViewResource = nullptr;
			RenderTarget2D* RenderTarget = nullptr;
			Shader* BasicEffect = nullptr;
			unsigned int PresentFlags = 0;
			unsigned int CompileFlags = 0;
			VSync VSyncMode = VSync_Frequency_X1;
			std::vector<Vertex> Elements;
			const void* Constants[4];
			uint64_t MaxElements;
			RenderBackend Backend;
			DirectBuffer Direct;
			std::mutex Mutex;
			bool Debug;

		public:
			RenderBuffer Render;
			ViewBuffer View;
			AnimationBuffer Animation;

		protected:
			GraphicsDevice(const Desc& I);

		public:
			virtual ~GraphicsDevice();
			virtual void SetConstantBuffers() = 0;
			virtual void SetShaderModel(ShaderModel Model) = 0;
			virtual void SetSamplerState(SamplerState* State) = 0;
			virtual void SetBlendState(BlendState* State) = 0;
			virtual void SetRasterizerState(RasterizerState* State) = 0;
			virtual void SetDepthStencilState(DepthStencilState* State) = 0;
			virtual void SetInputLayout(InputLayout* State) = 0;
			virtual void SetShader(Shader* Resource, unsigned int Type) = 0;
			virtual void SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type) = 0;
			virtual void SetBuffer(InstanceBuffer* Resource, unsigned int Slot) = 0;
			virtual void SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot) = 0;
			virtual void SetIndexBuffer(ElementBuffer* Resource, Format FormatMode) = 0;
			virtual void SetVertexBuffer(ElementBuffer* Resource, unsigned int Slot) = 0;
			virtual void SetTexture2D(Texture2D* Resource, unsigned int Slot) = 0;
			virtual void SetTexture3D(Texture3D* Resource, unsigned int Slot) = 0;
			virtual void SetTextureCube(TextureCube* Resource, unsigned int Slot) = 0;
			virtual void SetTarget(float R, float G, float B) = 0;
			virtual void SetTarget() = 0;
			virtual void SetTarget(DepthBuffer* Resource) = 0;
			virtual void SetTarget(RenderTarget2D* Resource, float R, float G, float B) = 0;
			virtual void SetTarget(RenderTarget2D* Resource) = 0;
			virtual void SetTarget(MultiRenderTarget2D* Resource, unsigned int Target, float R, float G, float B) = 0;
			virtual void SetTarget(MultiRenderTarget2D* Resource, unsigned int Target) = 0;
			virtual void SetTarget(MultiRenderTarget2D* Resource, float R, float G, float B) = 0;
			virtual void SetTarget(MultiRenderTarget2D* Resource) = 0;
			virtual void SetTarget(RenderTargetCube* Resource, float R, float G, float B) = 0;
			virtual void SetTarget(RenderTargetCube* Resource) = 0;
			virtual void SetTarget(MultiRenderTargetCube* Resource, unsigned int Target, float R, float G, float B) = 0;
			virtual void SetTarget(MultiRenderTargetCube* Resource, unsigned int Target) = 0;
			virtual void SetTarget(MultiRenderTargetCube* Resource, float R, float G, float B) = 0;
			virtual void SetTarget(MultiRenderTargetCube* Resource) = 0;;
			virtual void SetTargetMap(MultiRenderTarget2D* Resource, bool Enabled[8]) = 0;
			virtual void SetTargetMap(MultiRenderTargetCube* Resource, bool Enabled[8]) = 0;
			virtual void SetViewport(const Viewport& In) = 0;
			virtual void SetViewport(RenderTarget2D* Resource, const Viewport& In) = 0;
			virtual void SetViewport(MultiRenderTarget2D* Resource, const Viewport& In) = 0;
			virtual void SetViewport(RenderTargetCube* Resource, const Viewport& In) = 0;
			virtual void SetViewport(MultiRenderTargetCube* Resource, const Viewport& In) = 0;
			virtual void SetViewports(unsigned int Count, Viewport* Viewports) = 0;
			virtual void SetScissorRects(unsigned int Count, Rectangle* Value) = 0;
			virtual void SetPrimitiveTopology(PrimitiveTopology Topology) = 0;
			virtual void FlushTexture2D(unsigned int Slot, unsigned int Count) = 0;
			virtual void FlushTexture3D(unsigned int Slot, unsigned int Count) = 0;
			virtual void FlushTextureCube(unsigned int Slot, unsigned int Count) = 0;
			virtual void FlushState() = 0;
			virtual bool Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map) = 0;
			virtual bool Unmap(ElementBuffer* Resource, MappedSubresource* Map) = 0;
			virtual bool UpdateBuffer(ElementBuffer* Resource, void* Data, uint64_t Size) = 0;
			virtual bool UpdateBuffer(Shader* Resource, const void* Data) = 0;
			virtual bool UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data) = 0;
			virtual bool UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data) = 0;
			virtual bool UpdateBuffer(InstanceBuffer* Resource) = 0;
			virtual bool UpdateBuffer(RenderBufferType Buffer) = 0;
			virtual bool UpdateBufferSize(Shader* Resource, size_t Size) = 0;
			virtual bool UpdateBufferSize(InstanceBuffer* Resource, uint64_t Size) = 0;
			virtual void ClearBuffer(InstanceBuffer* Resource) = 0;
			virtual void Clear(float R, float G, float B) = 0;
			virtual void Clear(RenderTarget2D* Resource, float R, float G, float B) = 0;
			virtual void Clear(MultiRenderTarget2D* Resource, unsigned int Target, float R, float G, float B) = 0;
			virtual void Clear(RenderTargetCube* Resource, float R, float G, float B) = 0;
			virtual void Clear(MultiRenderTargetCube* Resource, unsigned int Target, float R, float G, float B) = 0;
			virtual void ClearDepth() = 0;
			virtual void ClearDepth(DepthBuffer* Resource) = 0;
			virtual void ClearDepth(RenderTarget2D* Resource) = 0;
			virtual void ClearDepth(MultiRenderTarget2D* Resource) = 0;
			virtual void ClearDepth(RenderTargetCube* Resource) = 0;
			virtual void ClearDepth(MultiRenderTargetCube* Resource) = 0;
			virtual void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation) = 0;
			virtual void DrawIndexed(MeshBuffer* Resource) = 0;
			virtual void DrawIndexed(SkinMeshBuffer* Resource) = 0;
			virtual void Draw(unsigned int Count, unsigned int Location) = 0;
			virtual bool CopyTexture2D(Texture2D** Result) = 0;
			virtual bool CopyTexture2D(RenderTarget2D* Resource, Texture2D** Result) = 0;
			virtual bool CopyTexture2D(MultiRenderTarget2D* Resource, unsigned int Target, Texture2D** Result) = 0;
			virtual bool CopyTexture2D(RenderTargetCube* Resource, unsigned int Face, Texture2D** Result) = 0;
			virtual bool CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, unsigned int Face, Texture2D** Result) = 0;
			virtual bool CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) = 0;
			virtual bool CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result) = 0;
			virtual bool CopyTargetTo(MultiRenderTarget2D* Resource, unsigned int Target, RenderTarget2D* To) = 0;
			virtual bool CopyTargetFrom(MultiRenderTarget2D* Resource, unsigned int Target, RenderTarget2D* From) = 0;
			virtual bool CopyTargetDepth(RenderTarget2D* From, RenderTarget2D* To) = 0;
			virtual bool CopyTargetDepth(MultiRenderTarget2D* From, MultiRenderTarget2D* To) = 0;
			virtual bool CopyTargetDepth(RenderTargetCube* From, RenderTargetCube* To) = 0;
			virtual bool CopyTargetDepth(MultiRenderTargetCube* From, MultiRenderTargetCube* To) = 0;
			virtual bool CopyBegin(MultiRenderTarget2D* Resource, unsigned int Target, unsigned int MipLevels, unsigned int Size) = 0;
			virtual bool CopyFace(MultiRenderTarget2D* Resource, unsigned int Target, unsigned int Face) = 0;
			virtual bool CopyEnd(MultiRenderTarget2D* Resource, TextureCube* Result) = 0;
			virtual void SwapTargetDepth(RenderTarget2D* From, RenderTarget2D* To) = 0;
			virtual void SwapTargetDepth(MultiRenderTarget2D* From, MultiRenderTarget2D* To) = 0;
			virtual void SwapTargetDepth(RenderTargetCube* From, RenderTargetCube* To) = 0;
			virtual void SwapTargetDepth(MultiRenderTargetCube* From, MultiRenderTargetCube* To) = 0;
			virtual void FetchViewports(unsigned int* Count, Viewport* Out) = 0;
			virtual void FetchScissorRects(unsigned int* Count, Rectangle* Out) = 0;
			virtual bool ResizeBuffers(unsigned int Width, unsigned int Height) = 0;
			virtual bool GenerateTexture(Texture2D* Resource) = 0;
			virtual bool GenerateTexture(Texture3D* Resource) = 0;
			virtual bool GenerateTexture(TextureCube* Resource) = 0;
			virtual bool GetQueryData(Query* Resource, uint64_t* Result, bool Flush = true) = 0;
			virtual bool GetQueryData(Query* Resource, bool* Result, bool Flush = true) = 0;
			virtual void QueryBegin(Query* Resource) = 0;
			virtual void QueryEnd(Query* Resource) = 0;
			virtual void GenerateMips(Texture2D* Resource) = 0;
			virtual void GenerateMips(Texture3D* Resource) = 0;
			virtual void GenerateMips(TextureCube* Resource) = 0;
			virtual bool Begin() = 0;
			virtual void Transform(const Compute::Matrix4x4& Transform) = 0;
			virtual void Topology(PrimitiveTopology Topology) = 0;
			virtual void Emit() = 0;
			virtual void Texture(Texture2D* In) = 0;
			virtual void Color(float X, float Y, float Z, float W) = 0;
			virtual void Intensity(float Intensity) = 0;
			virtual void TexCoord(float X, float Y) = 0;
			virtual void TexCoordOffset(float X, float Y) = 0;
			virtual void Position(float X, float Y, float Z) = 0;
			virtual bool End() = 0;
			virtual bool Submit() = 0;
			virtual DepthStencilState* CreateDepthStencilState(const DepthStencilState::Desc& I) = 0;
			virtual BlendState* CreateBlendState(const BlendState::Desc& I) = 0;
			virtual RasterizerState* CreateRasterizerState(const RasterizerState::Desc& I) = 0;
			virtual SamplerState* CreateSamplerState(const SamplerState::Desc& I) = 0;
			virtual InputLayout* CreateInputLayout(const InputLayout::Desc& I) = 0;
			virtual Shader* CreateShader(const Shader::Desc& I) = 0;
			virtual ElementBuffer* CreateElementBuffer(const ElementBuffer::Desc& I) = 0;
			virtual MeshBuffer* CreateMeshBuffer(const MeshBuffer::Desc& I) = 0;
			virtual SkinMeshBuffer* CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I) = 0;
			virtual InstanceBuffer* CreateInstanceBuffer(const InstanceBuffer::Desc& I) = 0;
			virtual Texture2D* CreateTexture2D() = 0;
			virtual Texture2D* CreateTexture2D(const Texture2D::Desc& I) = 0;
			virtual Texture3D* CreateTexture3D() = 0;
			virtual Texture3D* CreateTexture3D(const Texture3D::Desc& I) = 0;
			virtual TextureCube* CreateTextureCube() = 0;
			virtual TextureCube* CreateTextureCube(const TextureCube::Desc& I) = 0;
			virtual TextureCube* CreateTextureCube(Texture2D* Resource[6]) = 0;
			virtual TextureCube* CreateTextureCube(Texture2D* Resource) = 0;
			virtual DepthBuffer* CreateDepthBuffer(const DepthBuffer::Desc& I) = 0;
			virtual RenderTarget2D* CreateRenderTarget2D(const RenderTarget2D::Desc& I) = 0;
			virtual MultiRenderTarget2D* CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I) = 0;
			virtual RenderTargetCube* CreateRenderTargetCube(const RenderTargetCube::Desc& I) = 0;
			virtual MultiRenderTargetCube* CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I) = 0;
			virtual Query* CreateQuery(const Query::Desc& I) = 0;
			virtual PrimitiveTopology GetPrimitiveTopology() = 0;
			virtual ShaderModel GetSupportedShaderModel() = 0;
			virtual void* GetBackBuffer() = 0;
			virtual void* GetBackBufferMSAA() = 0;
			virtual void* GetBackBufferNoAA() = 0;
			virtual void* GetDevice() = 0;
			virtual void* GetContext() = 0;
			virtual bool IsValid() = 0;
			void Lock();
			void Unlock();
			void AddSection(const std::string& Name, const std::string& Code);
			void RemoveSection(const std::string& Name);
			DepthStencilState* GetDepthStencilState(const std::string& Name);
			BlendState* GetBlendState(const std::string& Name);
			RasterizerState* GetRasterizerState(const std::string& Name);
			SamplerState* GetSamplerState(const std::string& Name);
			InputLayout* GetInputLayout(const std::string& Name);
			ShaderModel GetShaderModel();
			RenderTarget2D* GetRenderTarget();
			Shader* GetBasicEffect();
			RenderBackend GetBackend();
			unsigned int GetPresentFlags();
			unsigned int GetCompileFlags();
			unsigned int GetMipLevel(unsigned int Width, unsigned int Height);
			bool GetSection(const std::string& Name, std::string* Out, bool Internal = false);
			VSync GetVSyncMode();
			bool IsDebug();

		protected:
			virtual TextureCube* CreateTextureCubeInternal(void* Resource[6]) = 0;
			bool Preprocess(Shader::Desc& ShaderCode);
			void InitStates();
			void InitSections();
			void FreeProxy();

		public:
			static GraphicsDevice* Create(const Desc& I);
		};

		class TH_OUT Activity : public Rest::Object
		{
		public:
			struct Desc
			{
				const char* Title = "Activity";
				unsigned int Width = 0;
				unsigned int Height = 0;
				unsigned int X = 0, Y = 0;
				bool Fullscreen = false;
				bool Hidden = false;
				bool Borderless = false;
				bool Resizable = true;
				bool Minimized = false;
				bool Maximized = false;
				bool Centered = false;
				bool FreePosition = false;
				bool Focused = false;
				bool AllowHighDPI = true;
			};

			struct
			{
				AppStateChangeCallback AppStateChange = nullptr;
				WindowStateChangeCallback WindowStateChange = nullptr;
				KeyStateCallback KeyState = nullptr;
				InputEditCallback InputEdit = nullptr;
				InputCallback Input = nullptr;
				CursorMoveCallback CursorMove = nullptr;
				CursorWheelStateCallback CursorWheelState = nullptr;
				JoyStickAxisMoveCallback JoyStickAxisMove = nullptr;
				JoyStickBallMoveCallback JoyStickBallMove = nullptr;
				JoyStickHatMoveCallback JoyStickHatMove = nullptr;
				JoyStickKeyStateCallback JoyStickKeyState = nullptr;
				JoyStickStateCallback JoyStickState = nullptr;
				ControllerAxisMoveCallback ControllerAxisMove = nullptr;
				ControllerKeyStateCallback ControllerKeyState = nullptr;
				ControllerStateCallback ControllerState = nullptr;
				TouchMoveCallback TouchMove = nullptr;
				TouchStateCallback TouchState = nullptr;
				GestureStateCallback GestureState = nullptr;
				MultiGestureStateCallback MultiGestureState = nullptr;
				DropFileCallback DropFile = nullptr;
				DropTextCallback DropText = nullptr;
			} Callbacks;

		private:
			struct
			{
				bool Captured = false;
				bool Mapped = false;
				bool Enabled = false;
				KeyMap Key;
			} Mapping;

		private:
			SDL_Cursor* Cursors[DisplayCursor_Count];
			SDL_Window* Handle;
			Desc Rest;
			bool Keys[2][1024];
			int Command, CX, CY;

		public:
			void* UserPointer = nullptr;
			Alert Message;

		public:
			Activity(const Desc& I);
			~Activity();
			void SetClipboardText(const std::string& Text);
			void SetCursorPosition(const Compute::Vector2& Position);
			void SetCursorPosition(float X, float Y);
			void SetGlobalCursorPosition(const Compute::Vector2& Position);
			void SetGlobalCursorPosition(float X, float Y);
			void SetKey(KeyCode KeyCode, bool Value);
			void SetCursor(DisplayCursor Style);
			void Cursor(bool Enabled);
			void Grab(bool Enabled);
			void Reset();
			void Hide();
			void Show();
			void Maximize();
			void Minimize();
			void Focus();
			void Fullscreen(bool Enabled);
			void Borderless(bool Enabled);
			void Move(int X, int Y);
			void Resize(int Width, int Height);
			void Title(const char* Value);
			void Icon(Surface* Icon);
			void Load(SDL_SysWMinfo* Base);
			bool CaptureKeyMap(KeyMap* Value);
			bool Dispatch();
			bool IsFullscreen();
			bool IsAnyKeyDown();
			bool IsKeyDown(const KeyMap& Key);
			bool IsKeyUp(const KeyMap& Key);
			bool IsKeyDownHit(const KeyMap& Key);
			bool IsKeyUpHit(const KeyMap& Key);
			float GetX();
			float GetY();
			float GetWidth();
			float GetHeight();
			float GetAspectRatio();
			KeyMod GetKeyModState();
			Graphics::Viewport GetViewport();
			Compute::Vector2 GetOffset();
			Compute::Vector2 GetSize();
			Compute::Vector2 GetClientSize();
			Compute::Vector2 GetGlobalCursorPosition();
			Compute::Vector2 GetCursorPosition();
			Compute::Vector2 GetCursorPosition(float ScreenWidth, float ScreenHeight);
			Compute::Vector2 GetCursorPosition(Compute::Vector2 ScreenDimensions);
			std::string GetClipboardText();
			SDL_Window* GetHandle();
			std::string GetError();

		private:
			bool* GetInputState();

		public:
			static const char* GetKeyName(KeyCode Code);
		};

		class TH_OUT Model : public Rest::Object
		{
		public:
			std::vector<MeshBuffer*> Meshes;
			Compute::Matrix4x4 Root;
			Compute::Vector4 Max;
			Compute::Vector4 Min;

		public:
			Model();
			virtual ~Model() override;
			MeshBuffer* Find(const std::string& Name);
		};

		class TH_OUT SkinModel : public Rest::Object
		{
		public:
			std::vector<SkinMeshBuffer*> Meshes;
			std::vector<Compute::Joint> Joints;
			Compute::Matrix4x4 Root;
			Compute::Vector4 Max;
			Compute::Vector4 Min;

		public:
			SkinModel();
			virtual ~SkinModel() override;
			void ComputePose(PoseBuffer* Map);
			SkinMeshBuffer* FindMesh(const std::string& Name);
			Compute::Joint* FindJoint(const std::string& Name, Compute::Joint* Root = nullptr);
			Compute::Joint* FindJoint(int64_t Index, Compute::Joint* Root = nullptr);

		private:
			void ComputePose(PoseBuffer* Map, Compute::Joint* Root, const Compute::Matrix4x4& World);
		};

		inline ResourceMap operator |(ResourceMap A, ResourceMap B)
		{
			return static_cast<ResourceMap>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		inline ShaderCompile operator |(ShaderCompile A, ShaderCompile B)
		{
			return static_cast<ShaderCompile>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		inline ResourceMisc operator |(ResourceMisc A, ResourceMisc B)
		{
			return static_cast<ResourceMisc>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		inline ResourceBind operator |(ResourceBind A, ResourceBind B)
		{
			return static_cast<ResourceBind>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}
	}
}
#endif
