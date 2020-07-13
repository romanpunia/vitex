#ifndef THAWK_ENGINE_GUI_CONTEXT_H
#define THAWK_ENGINE_GUI_CONTEXT_H

#include "../../engine.h"
#include <unordered_set>

struct nk_context;
struct nk_font;
struct nk_font_atlas;
struct nk_user_font;
struct nk_buffer;
struct nk_draw_null_texture;
struct nk_style;
struct nk_style_window;
struct nk_style_button;
struct nk_style_text;
struct nk_style_toggle;
struct nk_style_selectable;
struct nk_style_slider;
struct nk_style_progress;
struct nk_style_property;
struct nk_style_edit;
struct nk_style_combo;
struct nk_style_tab;
struct nk_style_scrollbar;

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
			class Element;

			class Context;

			typedef std::function<bool(Element*, bool)> QueryCallback;
			typedef std::unordered_map<std::string, std::string> ClassBase;

			enum CursorButton
			{
				CursorButton_Left,
				CursorButton_Middle,
				CursorButton_Right,
				CursorButton_Double,
			};

			enum GlyphRanges
			{
				GlyphRanges_Manual,
				GlyphRanges_Chinese,
				GlyphRanges_Cyrillic,
				GlyphRanges_Korean
			};

			struct THAWK_OUT Actor
			{
				std::unordered_set<Element*> Watchers;
				std::string Value;
			};

			struct THAWK_OUT ClassProxy
			{
				std::unordered_set<Element*> Watchers;
				std::string ClassName;
				ClassBase Proxy;
			};

			struct THAWK_OUT FontConfig
			{
				std::vector<std::pair<uint32_t, uint32_t>> Ranges = { { 32, 255 } };
				Compute::Vector2 Spacing;
				char* Buffer = nullptr;
				uint64_t BufferSize = 0;
				uint32_t FallbackGlyph = '?';
				uint32_t OversampleV = 1;
				uint32_t OversampleH = 1;
				GlyphRanges PredefRanges = GlyphRanges_Manual;
				float Height = 13;
				bool Snapping = false;
			};

			class THAWK_OUT Element : public Rest::Object
			{
			private:
				QueryCallback Callback;
				ClassProxy* Class;
				bool Active;

			protected:
				std::unordered_map<std::string, std::function<void()>> Reactors;
				std::unordered_set<std::string> Dynamics;
				std::vector<Element*> Nodes;
				ContentManager* Content;
				Element* Parent;
				Context* Base;
				ClassBase Proxy;

			public:
				Element(Context* View);
				virtual ~Element() override;
				virtual bool BuildBegin(nk_context* C) = 0;
				virtual void BuildEnd(nk_context* C, bool Stated) = 0;
				virtual void BuildStyle(nk_context* C, nk_style* S) = 0;
				virtual float GetWidth() = 0;
				virtual float GetHeight() = 0;
				virtual std::string GetType() = 0;
				virtual bool Build();
				void Recompute(const std::string& Name);
				void Add(Element* Node);
				void Remove(Element* Node);
				void Copy(ContentManager* Content, Rest::Document* DOM);
				void SetActive(bool Enabled);
				void SetGlobal(const std::string& Name, const std::string& Value, bool Erase = false);
				void SetLocal(const std::string& Name, const std::string& Value, bool Erase = false);
				void SetClass(const std::string& Name);
				bool IsActive();
				float GetAreaWidth();
				float GetAreaHeight();
				bool GetBoolean(const std::string& Name, bool Default);
				float GetFloat(const std::string& Name, float Default);
				int GetInteger(const std::string& Name, int Default);
				std::string GetModel(const std::string& Name);
				std::string GetString(const std::string& Name, const std::string& Default);
				std::string GetClass();
				std::string GetId();
				std::string* GetGlobal(const std::string& Name);
				nk_font* GetFont(const std::string& Name, const std::string& Default);
				Compute::Vector2 GetV2(const std::string& Name, const Compute::Vector2& Default);
				Compute::Vector3 GetV3(const std::string& Name, const Compute::Vector3& Default);
				Compute::Vector4 GetV4(const std::string& Name, const Compute::Vector4& Default);
				std::vector<Element*>& GetNodes();
				Element* GetUpperNode(const std::string& Name);
				Element* GetLowerNode(const std::string& Name);
				Element* GetNodeByPath(const std::string& Path);
				Element* GetNodeById(const std::string& Id);
				Element* GetNode(const std::string& Name);
				Element* GetParent(const std::string& Type);
				Context* GetContext();

			protected:
				virtual bool Assign(const std::string& Path, const QueryCallback& Function);
				void Bind(const std::string& Name, const std::function<void()>& Callback);
				void ResolveTable(Rest::Stroke* Name);
				std::string ResolveValue(const std::string& Value);
				Element* ResolveName(const std::string& Name);
				std::string* GetStatic(const std::string& Name, int* Query);
				std::string GetDynamic(int Query, const std::string& Value);
				float GetFloatRelative(const std::string& Value);

			public:
				template <typename T>
				bool QueryById(const std::string& Id, const std::function<bool(T*, bool)>& Callback)
				{
					auto It = Base->Uniques.find(Id);
					if (It == Base->Uniques.end() || !It->second)
						return false;

					return It->second->Query<T>(Callback);
				}
				template <typename T>
				bool QueryByPath(const std::string& Path, const std::function<bool(T*, bool)>& Callback)
				{
					if (!Callback)
						return Assign(Path, nullptr);

					return Assign(Path, [Callback](Element* Target, bool Continue)
					{
						return Callback(Target ? Target->As<T>() : nullptr, Continue);
					});
				}
				template <typename T>
				bool Query(const std::function<bool(T*, bool)>& Callback)
				{
					return QueryByPath<T>("", Callback);
				}
			};

			class THAWK_OUT Widget : public Element
			{
			protected:
				std::function<void(Widget*)> Input;
				nk_user_font* Cache;
				nk_font* Font;

			public:
				Widget(Context* View);
				virtual ~Widget() override;
				virtual bool Build() override;
				virtual bool BuildBegin(nk_context* C) = 0;
				virtual void BuildEnd(nk_context* C, bool Stated) = 0;
				virtual void BuildStyle(nk_context* C, nk_style* S) = 0;
				virtual float GetWidth() = 0;
				virtual float GetHeight() = 0;
				virtual std::string GetType() = 0;
				void SetInputEvents(const std::function<void(Widget*)>& Callback);
				void GetWidgetBounds(float* X, float* Y, float* W, float* H);
				Compute::Vector2 GetWidgetPosition();
				Compute::Vector2 GetWidgetSize();
				float GetWidgetWidth();
				float GetWidgetHeight();
				bool IsWidgetHovered();
				bool IsWidgetClicked(CursorButton Key);
				bool IsWidgetClickedDown(CursorButton Key, bool Down);
				nk_font* GetFontHandle();

			protected:
				virtual void BuildFont(nk_context* C, nk_style* S);
			};

			class THAWK_OUT Stateful : public Widget
			{
			protected:
				struct
				{
					std::string Text;
					uint64_t Count;
					uint64_t Id;
				} Hash;

			public:
				Stateful(Context* View);
				virtual ~Stateful() override;
				virtual bool BuildBegin(nk_context* C) = 0;
				virtual void BuildEnd(nk_context* C, bool Stated) = 0;
				virtual void BuildStyle(nk_context* C, nk_style* S) = 0;
				virtual float GetWidth() = 0;
				virtual float GetHeight() = 0;
				virtual std::string GetType() = 0;
				std::string& GetHash();
				void Push();
				void Pop(uint64_t Count = 0);

			protected:
				virtual void BuildFont(nk_context* C, nk_style* S);
			};

			class THAWK_OUT Body : public Element
			{
			private:
				struct
				{
					nk_style_scrollbar* ScrollH;
					nk_style_scrollbar* ScrollV;
				} Style;

			public:
				Body(Context* View);
				virtual ~Body() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				bool IsHovered();
			};

			class THAWK_OUT Context : public Rest::Object
			{
				friend Element;

			private:
				std::unordered_map<std::string, std::function<Element*()>> Factories;
				std::unordered_map<std::string, std::pair<nk_font*, void*>> Fonts;
				std::unordered_map<std::string, ClassProxy*> Classes;
				std::unordered_map<std::string, Actor*> Args;
				std::unordered_map<std::string, Element*> Uniques;
				Graphics::DepthStencilState* DepthStencil;
				Graphics::RasterizerState* Rasterizer;
				Graphics::BlendState* Blend;
				Graphics::SamplerState* Sampler;
				Graphics::ElementBuffer* VertexBuffer;
				Graphics::ElementBuffer* IndexBuffer;
				Graphics::Activity* Activity;
				Graphics::Shader* Shader;
				Graphics::Texture2D* Font;
				Graphics::GraphicsDevice* Device;
				Body* DOM;
				nk_context* Engine;
				nk_font_atlas* Atlas;
				nk_buffer* Commands;
				nk_draw_null_texture* Null;
				nk_style* Style;
				Compute::Matrix4x4 Projection;
				uint64_t WidgetId;
				float Width, Height;
				bool FontBaking;
				bool Overflowing;

			public:
				Context(Graphics::GraphicsDevice* NewDevice, Graphics::Activity* NewActivity);
				virtual ~Context() override;
				void ResizeBuffers(size_t MaxVertices, size_t MaxIndices);
				void Prepare(unsigned int Width, unsigned int Height);
				void Render(const Compute::Matrix4x4& Offset, bool AA);
				void EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
				void EmitInput(char* Buffer, int Length);
				void EmitWheel(int X, int Y, bool Normal);
				void EmitCursor(int X, int Y);
				void EmitEscape();
				void Restore();
				void SetOverflow(bool Enabled);
				void SetClass(const std::string& ClassName, const std::string& Name, const std::string& Value);
				void GetCurrentWidgetBounds(float* X, float* Y, float* W, float* H);
				void ClearGlobals();
				void ClearClasses();
				void ClearFonts();
				void LoadGlobals(Rest::Document* Document);
				void LoadClasses(ContentManager* Content, Rest::Document* Document);
				void LoadFonts(ContentManager* Content, Rest::Document* Document);
				Body* Load(ContentManager* Content, Rest::Document* Document);
				bool FontBakingBegin();
				bool FontBake(const std::string& Name, FontConfig* Config);
				bool FontBakeDefault(const std::string& Name, FontConfig* Config);
				bool FontBakingEnd();
				bool HasOverflow();
				bool IsCurrentWidgetHovered();
				bool IsCurrentWidgetClicked();
				nk_font* GetFont(const std::string& Name);
				nk_context* GetContext();
				const nk_style* GetDefaultStyle();
				Body* GetLayout();
				Graphics::Activity* GetActivity();
				std::string GetClass(const std::string& ClassName, const std::string& Name);
				uint64_t GetNextWidgetId();
				float GetWidth();
				float GetHeight();

			public:
				template <typename T>
				void FactoryPush(const std::string& Name)
				{
					Factories[Name] = std::function<Element*()>([this]()
					{
						return (Element*)new T(this);
					});
				}
				template <typename T>
				bool QueryElementById(const std::string& Id, const std::function<bool(T*, bool)>& Callback)
				{
					auto It = Uniques.find(Id);
					if (It == Uniques.end() || !It->second)
						return false;

					return It->second->Query<T>(Callback);
				}
				template <typename T>
				bool QueryElementByPath(const std::string& Path, const std::function<bool(T*, bool)>& Callback)
				{
					if (!DOM)
						return false;

					return DOM->Query<T>(Path, Callback);
				}
				template <typename T>
				bool QueryElement(const std::function<bool(T*, bool)>& Callback)
				{
					return QueryElementByPath("", Callback);
				}
			};
		}
	}
}
#endif