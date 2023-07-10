#ifndef VI_ENGINE_GUI_H
#define VI_ENGINE_GUI_H
#include "../core/engine.h"

namespace Rml
{
	class Context;

	class Element;

	class ElementDocument;

	class Decorator;

	class DataModelConstructor;

	class DataModelHandle;

	class Event;

	class EventListener;

	class Variant;
}

namespace Mavi
{
	namespace Engine
	{
		namespace GUI
		{
			class RenderSubsystem;

			class FileSubsystem;

			class MainSubsystem;

			class ScopedContext;

			class ContextInstancer;

			class ListenerSubsystem;

			class ListenerInstancer;

			class DocumentSubsystem;

			class DocumentInstancer;

			class EventSubsystem;

			class IEvent;

			class DataModel;

			class IElement;

			class IElementDocument;

			class DataNode;

			class Listener;

			class Context;

			typedef std::function<Core::String(const Core::String&)> TranslationCallback;
			typedef std::function<void(void*)> DestroyCallback;
			typedef std::function<void(IEvent&)> EventCallback;
			typedef std::function<void(IEvent&, const Core::VariantList&)> DataCallback;
			typedef std::function<void(Core::Variant&)> GetterCallback;
			typedef std::function<void(const Core::Variant&)> SetterCallback;
			typedef std::function<void(Context*)> ModelCallback;

			enum class ModalFlag
			{
				None,
				Modal,
				Keep
			};

			enum class FocusFlag
			{
				None,
				Document,
				Keep,
				Auto
			};

			enum class Area
			{
				Margin = 0,
				Border = 1,
				Padding = 2,
				Content = 3
			};

			enum class Display : uint8_t
			{
				None,
				Block,
				Inline,
				InlineBlock,
				Flex,
				Table,
				TableRow,
				TableRowGroup,
				TableColumn,
				TableColumnGroup,
				TableCell
			};

			enum class Position : uint8_t
			{
				Static,
				Relative,
				Absolute,
				Fixed
			};

			enum class Float : uint8_t
			{
				None,
				Left,
				Right
			};

			enum class TimingFunc
			{
				None,
				Back,
				Bounce,
				Circular,
				Cubic,
				Elastic,
				Exponential,
				Linear,
				Quadratic,
				Quartic,
				Quintic,
				Sine,
				Callback
			};

			enum class TimingDir
			{
				In = 1,
				Out = 2,
				InOut = 3
			};

			enum class EventPhase
			{
				None,
				Capture = 1,
				Target = 2,
				Bubble = 4
			};

			enum class InputType
			{
				Keys = 1,
				Scroll = 2,
				Text = 3,
				Cursor = 4,
				Any = (Keys | Scroll | Text | Cursor)
			};

			enum class NumericUnit
			{
				UNKNOWN = 0,
				KEYWORD = 1 << 0,
				STRING = 1 << 1,
				COLOUR = 1 << 2,
				RATIO = 1 << 3,
				NUMBER = 1 << 4,
				PERCENT = 1 << 5,
				PX = 1 << 6,
				DP = 1 << 7,
				VW = 1 << 8,
				VH = 1 << 9,
				X = 1 << 10,
				EM = 1 << 11,
				REM = 1 << 12,
				INCH = 1 << 13,
				CM = 1 << 14,
				MM = 1 << 15,
				PT = 1 << 16,
				PC = 1 << 17,
				PPI_UNIT = INCH | CM | MM | PT | PC,
				DEG = 1 << 18,
				RAD = 1 << 19,
				TRANSFORM = 1 << 20,
				TRANSITION = 1 << 21,
				ANIMATION = 1 << 22,
				DECORATOR = 1 << 23,
				FONTEFFECT = 1 << 24,
				COLORSTOPLIST = 1 << 25,
				SHADOWLIST = 1 << 26,
				LENGTH = PX | DP | VW | VH | EM | REM | PPI_UNIT,
				LENGTH_PERCENT = LENGTH | PERCENT,
				NUMBER_LENGTH_PERCENT = NUMBER | LENGTH | PERCENT,
				DP_SCALABLE_LENGTH = DP | PPI_UNIT,
				ANGLE = DEG | RAD,
				NUMERIC = NUMBER_LENGTH_PERCENT | ANGLE | X
			};

			inline InputType operator |(InputType A, InputType B)
			{
				return static_cast<InputType>(static_cast<size_t>(A) | static_cast<size_t>(B));
			}

			class VI_OUT IVariant
			{
			public:
				static void Convert(Rml::Variant* From, Core::Variant* To);
				static void Revert(Core::Variant* From, Rml::Variant* To);
				static Compute::Vector4 ToColor4(const Core::String& Value);
				static Core::String FromColor4(const Compute::Vector4& Base, bool HEX);
				static Compute::Vector4 ToColor3(const Core::String& Value);
				static Core::String FromColor3(const Compute::Vector4& Base, bool HEX);
				static int GetVectorType(const Core::String& Value);
				static Compute::Vector4 ToVector4(const Core::String& Base);
				static Core::String FromVector4(const Compute::Vector4& Base);
				static Compute::Vector3 ToVector3(const Core::String& Base);
				static Core::String FromVector3(const Compute::Vector3& Base);
				static Compute::Vector2 ToVector2(const Core::String& Base);
				static Core::String FromVector2(const Compute::Vector2& Base);
			};

			class VI_OUT IEvent
			{
				friend EventSubsystem;
				friend DataModel;

			private:
				Rml::Event* Base;
				bool Owned;

			public:
				IEvent();
				IEvent(Rml::Event* Ref);
				IEvent Copy();
				EventPhase GetPhase() const;
				void Release();
				void SetPhase(EventPhase Phase);
				void SetCurrentElement(const IElement& Element);
				IElement GetCurrentElement() const;
				IElement GetTargetElement() const;
				Core::String GetType() const;
				void StopPropagation();
				void StopImmediatePropagation();
				bool IsInterruptible() const;
				bool IsPropagating() const;
				bool IsImmediatePropagating() const;
				bool GetBoolean(const Core::String& Key) const;
				int64_t GetInteger(const Core::String& Key) const;
				double GetNumber(const Core::String& Key) const;
				Core::String GetString(const Core::String& Key) const;
				Compute::Vector2 GetVector2(const Core::String& Key) const;
				Compute::Vector3 GetVector3(const Core::String& Key) const;
				Compute::Vector4 GetVector4(const Core::String& Key) const;
				void* GetPointer(const Core::String& Key) const;
				Rml::Event* GetEvent() const;
				bool IsValid() const;
			};

			class VI_OUT IElement
			{
			protected:
				Rml::Element* Base;

			public:
				IElement();
				IElement(Rml::Element* Ref);
				virtual ~IElement() = default;
				virtual void Release();
				IElement Clone() const;
				void SetClass(const Core::String& ClassName, bool Activate);
				bool IsClassSet(const Core::String& ClassName) const;
				void SetClassNames(const Core::String& ClassNames);
				Core::String GetClassNames() const;
				Core::String GetAddress(bool IncludePseudoClasses = false, bool IncludeParents = true) const;
				void SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed = false);
				Compute::Vector2 GetRelativeOffset(Area Type = Area::Content);
				Compute::Vector2 GetAbsoluteOffset(Area Type = Area::Content);
				void SetClientArea(Area ClientArea);
				Area GetClientArea() const;
				void SetContentBox(const Compute::Vector2& ContentBox);
				float GetBaseline() const;
				bool GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio);
				bool IsPointWithinElement(const Compute::Vector2& Point);
				bool IsVisible() const;
				float GetZIndex() const;
				bool SetProperty(const Core::String& Name, const Core::String& Value);
				void RemoveProperty(const Core::String& Name);
				Core::String GetProperty(const Core::String& Name);
				Core::String GetLocalProperty(const Core::String& Name);
				float ResolveNumericProperty(float Value, NumericUnit Unit, float BaseValue);
				Compute::Vector2 GetContainingBlock();
				Position GetPosition();
				Float GetFloat();
				Display GetDisplay();
				float GetLineHeight();
				bool Project(Compute::Vector2& Point) const noexcept;
				bool Animate(const Core::String& PropertyName, const Core::String& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations = 1, bool AlternateDirection = true, float Delay = 0.0f);
				bool AddAnimationKey(const Core::String& PropertyName, const Core::String& TargetValue, float Duration, TimingFunc Func, TimingDir Dir);
				void SetPseudoClass(const Core::String& PseudoClass, bool Activate);
				bool IsPseudoClassSet(const Core::String& PseudoClass) const;
				void SetAttribute(const Core::String& Name, const Core::String& Value);
				Core::String GetAttribute(const Core::String& Name);
				bool HasAttribute(const Core::String& Name) const;
				void RemoveAttribute(const Core::String& Name);
				IElement GetFocusLeafNode();
				Core::String GetTagName() const;
				Core::String GetId() const;
				void SetId(const Core::String& Id);
				float GetAbsoluteLeft();
				float GetAbsoluteTop();
				float GetClientLeft();
				float GetClientTop();
				float GetClientWidth();
				float GetClientHeight();
				IElement GetOffsetParent();
				float GetOffsetLeft();
				float GetOffsetTop();
				float GetOffsetWidth();
				float GetOffsetHeight();
				float GetScrollLeft();
				void SetScrollLeft(float ScrollLeft);
				float GetScrollTop();
				void SetScrollTop(float ScrollTop);
				float GetScrollWidth();
				float GetScrollHeight();
				IElementDocument GetOwnerDocument() const;
				IElement GetParentNode() const;
				IElement GetNextSibling() const;
				IElement GetPreviousSibling() const;
				IElement GetFirstChild() const;
				IElement GetLastChild() const;
				IElement GetChild(int Index) const;
				int GetNumChildren(bool IncludeNonDOMElements = false) const;
				void GetInnerHTML(Core::String& Content) const;
				Core::String GetInnerHTML() const;
				void SetInnerHTML(const Core::String& HTML);
				bool IsFocused();
				bool IsHovered();
				bool IsActive();
				bool IsChecked();
				bool Focus();
				void Blur();
				void Click();
				void AddEventListener(const Core::String& Event, Listener* Source, bool InCapturePhase = false);
				void RemoveEventListener(const Core::String& Event, Listener* Source, bool InCapturePhase = false);
				bool DispatchEvent(const Core::String& Type, const Core::VariantArgs& Args);
				void ScrollIntoView(bool AlignWithTop = true);
				IElement AppendChild(const IElement& Element, bool DOMElement = true);
				IElement InsertBefore(const IElement& Element, const IElement& AdjacentElement);
				IElement ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement);
				IElement RemoveChild(const IElement& Element);
				bool HasChildNodes() const;
				IElement GetElementById(const Core::String& Id);
				IElement QuerySelector(const Core::String& Selector);
				Core::Vector<IElement> QuerySelectorAll(const Core::String& Selectors);
				bool CastFormColor(Compute::Vector4* Ptr, bool Alpha);
				bool CastFormString(Core::String* Ptr);
				bool CastFormPointer(void** Ptr);
				bool CastFormInt32(int32_t* Ptr);
				bool CastFormUInt32(uint32_t* Ptr);
				bool CastFormFlag32(uint32_t* Ptr, uint32_t Mask);
				bool CastFormInt64(int64_t* Ptr);
				bool CastFormUInt64(uint64_t* Ptr);
				bool CastFormSize(size_t* Ptr);
				bool CastFormFlag64(uint64_t* Ptr, uint64_t Mask);
				bool CastFormFloat(float* Ptr);
				bool CastFormFloat(float* Ptr, float Mult);
				bool CastFormDouble(double* Ptr);
				bool CastFormBoolean(bool* Ptr);
				Core::String GetFormName() const;
				void SetFormName(const Core::String& Name);
				Core::String GetFormValue() const;
				void SetFormValue(const Core::String& Value);
				bool IsFormDisabled() const;
				void SetFormDisabled(bool Disable);
				Rml::Element* GetElement() const;
				bool IsValid() const;

			public:
				static Core::String FromPointer(void* Ptr);
				static void* ToPointer(const Core::String& Value);
			};

			class VI_OUT IElementDocument : public IElement
			{
			public:
				IElementDocument();
				IElementDocument(Rml::ElementDocument* Ref);
				void Release() override;
				void SetTitle(const Core::String& Title);
				void PullToFront();
				void PushToBack();
				void Show(ModalFlag Modal = ModalFlag::None, FocusFlag Focus = FocusFlag::Auto);
				void Hide();
				void Close();
				Core::String GetTitle() const;
				Core::String GetSourceURL() const;
				IElement CreateElement(const Core::String& Name);
				bool IsModal() const;
				Rml::ElementDocument* GetElementDocument() const;
			};

			class VI_OUT Utils
			{
			public:
				static Compute::Matrix4x4 ToMatrix(const void* Matrix) noexcept;
				static Core::String EscapeHTML(const Core::String& Text) noexcept;
			};

			class VI_OUT Subsystem final : public Core::Singleton<Subsystem>
			{
				friend RenderSubsystem;
				friend DocumentSubsystem;
				friend ListenerSubsystem;
				friend Context;

			private:
				struct
				{
					Scripting::VirtualMachine* VM = nullptr;
					Graphics::Activity* Activity = nullptr;
					RenderConstants* Constants = nullptr;
					ContentManager* Content = nullptr;
					Core::Timer* Time = nullptr;

					void AddRef()
					{
						if (VM != nullptr)
							VM->AddRef();
						if (Activity != nullptr)
							Activity->AddRef();
						if (Constants != nullptr)
							Constants->AddRef();
						if (Content != nullptr)
							Content->AddRef();
						if (Time != nullptr)
							Time->AddRef();
					}
					void Release()
					{
						VI_CLEAR(VM);
						VI_CLEAR(Activity);
						VI_CLEAR(Constants);
						VI_CLEAR(Content);
						VI_CLEAR(Time);
					}
				} Shared;

			private:
				ContextInstancer* ContextFactory;
				DocumentInstancer* DocumentFactory;
				ListenerInstancer* ListenerFactory;
				RenderSubsystem* RenderInterface;
				FileSubsystem* FileInterface;
				MainSubsystem* SystemInterface;
				uint64_t Id;
				bool HasDecorators;

			public:
				Subsystem() noexcept;
				virtual ~Subsystem() noexcept override;
				void SetShared(Scripting::VirtualMachine* VM, Graphics::Activity* Activity, RenderConstants* Constants, ContentManager* Content, Core::Timer* Time) noexcept;
				void SetTranslator(const Core::String& Name, const TranslationCallback& Callback) noexcept;
				RenderSubsystem* GetRenderInterface() noexcept;
				FileSubsystem* GetFileInterface() noexcept;
				MainSubsystem* GetSystemInterface() noexcept;
				Graphics::GraphicsDevice* GetDevice() noexcept;
				Graphics::Texture2D* GetBackground() noexcept;
				Compute::Matrix4x4* GetTransform() noexcept;
				Compute::Matrix4x4* GetProjection() noexcept;

			private:
				void ResizeDecorators(int Width, int Height) noexcept;
				void CreateDecorators(RenderConstants* Constants) noexcept;
				void ReleaseDecorators() noexcept;
				void CreateElements() noexcept;
				void ReleaseElements() noexcept;
			};

			class VI_OUT DataModel final : public Core::Reference<DataModel>
			{
				friend Context;

			private:
				Core::UnorderedMap<Core::String, DataNode*> Props;
				Core::Vector<std::function<void()>> Callbacks;
				Rml::DataModelConstructor* Base;
				ModelCallback OnUnmount;

			private:
				DataModel(Rml::DataModelConstructor* Ref);

			public:
				~DataModel() noexcept;
				DataNode* SetProperty(const Core::String& Name, const Core::Variant& Value);
				DataNode* SetProperty(const Core::String& Name, Core::Variant* Reference);
				DataNode* SetArray(const Core::String& Name);
				DataNode* SetString(const Core::String& Name, const Core::String& Value);
				DataNode* SetInteger(const Core::String& Name, int64_t Value);
				DataNode* SetFloat(const Core::String& Name, float Value);
				DataNode* SetDouble(const Core::String& Name, double Value);
				DataNode* SetBoolean(const Core::String& Name, bool Value);
				DataNode* SetPointer(const Core::String& Name, void* Value);
				DataNode* GetProperty(const Core::String& Name);
				Core::String GetString(const Core::String& Name);
				int64_t GetInteger(const Core::String& Name);
				float GetFloat(const Core::String& Name);
				double GetDouble(const Core::String& Name);
				bool GetBoolean(const Core::String& Name);
				void* GetPointer(const Core::String& Name);
				bool SetCallback(const Core::String& Name, const DataCallback& Callback);
				bool SetUnmountCallback(const ModelCallback& Callback);
				bool HasChanged(const Core::String& VariableName) const;
				void SetDetachCallback(std::function<void()>&& Callback);
				void Change(const Core::String& VariableName);
				void Detach();
				bool IsValid() const;
				Rml::DataModelConstructor* Get();
			};

			class VI_OUT DataNode
			{
				friend Context;
				friend DataModel;

			private:
				Core::Vector<DataNode> Childs;
				Core::String Name;
				Core::Variant* Ref;
				DataModel* Handle;
				void* Order;
				size_t Depth;
				bool Safe;

			private:
				DataNode(DataModel* Model, const Core::String& TopName, const Core::Variant& Initial) noexcept;
				DataNode(DataModel* Model, const Core::String& TopName, Core::Variant* Reference) noexcept;

			public:
				DataNode(DataNode&& Other) noexcept;
				DataNode(const DataNode& Other) noexcept;
				~DataNode();
				DataNode& Insert(size_t Where, const Core::VariantList& Initial, std::pair<void*, size_t>* Top = nullptr);
				DataNode& Insert(size_t Where, const Core::Variant& Initial, std::pair<void*, size_t>* Top = nullptr);
				DataNode& Insert(size_t Where, Core::Variant* Reference, std::pair<void*, size_t>* Top = nullptr);
				DataNode& Add(const Core::VariantList& Initial, std::pair<void*, size_t>* Top = nullptr);
				DataNode& Add(const Core::Variant& Initial, std::pair<void*, size_t>* Top = nullptr);
				DataNode& Add(Core::Unique<Core::Variant> Reference, std::pair<void*, size_t>* Top = nullptr);
				DataNode& At(size_t Index);
				bool Remove(size_t Index);
				bool Clear();
				void SortTree();
				void Replace(const Core::VariantList& Data, std::pair<void*, size_t>* Top = nullptr);
				void Set(const Core::Variant& NewValue);
				void Set(Core::Variant* NewReference);
				void SetTop(void* SeqId, size_t Depth);
				void SetString(const Core::String& Value);
				void SetVector2(const Compute::Vector2& Value);
				void SetVector3(const Compute::Vector3& Value);
				void SetVector4(const Compute::Vector4& Value);
				void SetInteger(int64_t Value);
				void SetFloat(float Value);
				void SetDouble(double Value);
				void SetBoolean(bool Value);
				void SetPointer(void* Value);
				size_t GetSize() const;
				void* GetSeqId() const;
				size_t GetDepth() const;
				const Core::Variant& Get();
				Core::String GetString();
				Compute::Vector2 GetVector2();
				Compute::Vector3 GetVector3();
				Compute::Vector4 GetVector4();
				int64_t GetInteger();
				float GetFloat();
				double GetDouble();
				bool GetBoolean();
				void* GetPointer();
				DataNode& operator= (const DataNode& Other) noexcept;
				DataNode& operator= (DataNode&& Other) noexcept;

			private:
				void GetValue(Rml::Variant& Result);
				void SetValue(const Rml::Variant& Result);
				void SetValueStr(const Core::String& Value);
				void SetValueNum(double Value);
				void SetValueInt(int64_t Value);
				int64_t GetValueSize();
			};

			class VI_OUT Listener final : public Core::Reference<Listener>
			{
				friend IElement;
				friend Context;

			private:
				Rml::EventListener* Base;

			public:
				Listener(const EventCallback& NewCallback);
				Listener(const Core::String& FunctionName);
				~Listener() noexcept;
			};

			class VI_OUT Context final : public Core::Reference<Context>
			{
				friend DocumentSubsystem;
				friend ListenerSubsystem;

			private:
				struct
				{
					bool Keys = false;
					bool Scroll = false;
					bool Text = false;
					bool Cursor = false;
				} Inputs;

			private:
				Core::UnorderedMap<int, Core::UnorderedMap<Core::String, Rml::Element*>> Elements;
				Core::UnorderedMap<Core::String, DataModel*> Models;
				Scripting::Compiler* Compiler;
				Compute::Vector2 Cursor;
				ModelCallback OnMount;
				ScopedContext* Base;
				uint32_t Busy;

			public:
				Context(const Compute::Vector2& Size);
				Context(Graphics::GraphicsDevice* Device);
				~Context() noexcept;
				void EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
				void EmitInput(const char* Buffer, int Length);
				void EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod);
				void EmitResize(int Width, int Height);
				void UpdateEvents(Graphics::Activity* Activity);
				void RenderLists(Graphics::Texture2D* Target);
				void EnableMouseCursor(bool Enable);
				void ClearStyles();
				bool ClearDocuments();
				bool LoadManifest(const Core::String& ConfPath);
				bool LoadFontFace(const Core::String& Path, bool UseAsFallback = false);
				bool IsLoading();
				bool IsInputFocused();
				bool WasInputUsed(uint32_t InputTypeMask);
				uint64_t GetIdleTimeoutMs(uint64_t MaxTimeout) const;
				int GetNumDocuments() const;
				void SetDensityIndependentPixelRatio(float DensityIndependentPixelRatio);
				float GetDensityIndependentPixelRatio() const;
				bool ReplaceHTML(const Core::String& Selector, const Core::String& HTML, int Index = 0);
				IElementDocument EvalHTML(const Core::String& HTML, int Index = 0);
				IElementDocument AddCSS(const Core::String& CSS, int Index = 0);
				IElementDocument LoadCSS(const Core::String& Path, int Index = 0);
				IElementDocument LoadDocument(const Core::String& Path, bool AllowPreprocessing = false);
				IElementDocument AddDocument(const Core::String& HTML);
				IElementDocument AddDocumentEmpty(const Core::String& InstancerName = "body");
				IElementDocument GetDocument(const Core::String& Id);
				IElementDocument GetDocument(int Index);
				IElement GetElementById(const Core::String& Id, int Index = 0);
				IElement GetHoverElement();
				IElement GetFocusElement();
				IElement GetRootElement();
				IElement GetElementAtPoint(const Compute::Vector2& Point, const IElement& IgnoreElement = nullptr, const IElement& Element = nullptr) const;
				void PullDocumentToFront(const IElementDocument& Schema);
				void PushDocumentToBack(const IElementDocument& Schema);
				void UnfocusDocument(const IElementDocument& Schema);
				void AddEventListener(const Core::String& Event, Listener* Listener, bool InCapturePhase = false);
				void RemoveEventListener(const Core::String& Event, Listener* Listener, bool InCapturePhase = false);
				bool IsMouseInteracting() const;
				bool GetActiveClipRegion(Compute::Vector2& Origin, Compute::Vector2& Dimensions) const;
				void SetActiveClipRegion(const Compute::Vector2& Origin, const Compute::Vector2& Dimensions);
				bool RemoveDataModel(const Core::String& Name);
				bool RemoveDataModels();
				void SetDocumentsBaseTag(const Core::String& Tag);
				void SetMountCallback(const ModelCallback& Callback);
				Core::String GetDocumentsBaseTag();
				Core::UnorderedMap<Core::String, bool>* GetFontFaces();
				Compute::Vector2 GetDimensions() const;
				DataModel* SetDataModel(const Core::String& Name);
				DataModel* GetDataModel(const Core::String& Name);
				Rml::Context* GetContext();

			private:
				bool LoadManifest(Core::Schema* Conf, const Core::String& Relative);
				bool Preprocess(const Core::String& Path, Core::String& Buffer);
				void Decompose(Core::String& Buffer);
				void ClearScope();

			private:
				static int GetKeyCode(Graphics::KeyCode Key);
				static int GetKeyMod(Graphics::KeyMod Mod);
			};
		}
	}
}
#endif
