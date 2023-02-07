#ifndef ED_ENGINE_GUI_H
#define ED_ENGINE_GUI_H
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

namespace Edge
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

			typedef std::function<std::string(const std::string&)> TranslationCallback;
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

			inline InputType operator |(InputType A, InputType B)
			{
				return static_cast<InputType>(static_cast<size_t>(A) | static_cast<size_t>(B));
			}

			class ED_OUT IVariant
			{
			public:
				static void Convert(Rml::Variant* From, Core::Variant* To);
				static void Revert(Core::Variant* From, Rml::Variant* To);
				static Compute::Vector4 ToColor4(const std::string& Value);
				static std::string FromColor4(const Compute::Vector4& Base, bool HEX);
				static Compute::Vector4 ToColor3(const std::string& Value);
				static std::string FromColor3(const Compute::Vector4& Base, bool HEX);
				static int GetVectorType(const std::string& Value);
				static Compute::Vector4 ToVector4(const std::string& Base);
				static std::string FromVector4(const Compute::Vector4& Base);
				static Compute::Vector3 ToVector3(const std::string& Base);
				static std::string FromVector3(const Compute::Vector3& Base);
				static Compute::Vector2 ToVector2(const std::string& Base);
				static std::string FromVector2(const Compute::Vector2& Base);
			};

			class ED_OUT IEvent
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
				std::string GetType() const;
				void StopPropagation();
				void StopImmediatePropagation();
				bool IsInterruptible() const;
				bool IsPropagating() const;
				bool IsImmediatePropagating() const;
				bool GetBoolean(const std::string& Key) const;
				int64_t GetInteger(const std::string& Key) const;
				double GetNumber(const std::string& Key) const;
				std::string GetString(const std::string& Key) const;
				Compute::Vector2 GetVector2(const std::string& Key) const;
				Compute::Vector3 GetVector3(const std::string& Key) const;
				Compute::Vector4 GetVector4(const std::string& Key) const;
				void* GetPointer(const std::string& Key) const;
				Rml::Event* GetEvent() const;
				bool IsValid() const;
			};

			class ED_OUT IElement
			{
			protected:
				Rml::Element* Base;

			public:
				IElement();
				IElement(Rml::Element* Ref);
				virtual ~IElement() = default;
				virtual void Release();
				IElement Clone() const;
				void SetClass(const std::string& ClassName, bool Activate);
				bool IsClassSet(const std::string& ClassName) const;
				void SetClassNames(const std::string& ClassNames);
				std::string GetClassNames() const;
				std::string GetAddress(bool IncludePseudoClasses = false, bool IncludeParents = true) const;
				void SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed = false);
				Compute::Vector2 GetRelativeOffset(Area Type = Area::Content);
				Compute::Vector2 GetAbsoluteOffset(Area Type = Area::Content);
				void SetClientArea(Area ClientArea);
				Area GetClientArea() const;
				void SetContentBox(const Compute::Vector2& ContentOffset, const Compute::Vector2& ContentBox);
				float GetBaseline() const;
				bool GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio);
				bool IsPointWithinElement(const Compute::Vector2& Point);
				bool IsVisible() const;
				float GetZIndex() const;
				bool SetProperty(const std::string& Name, const std::string& Value);
				void RemoveProperty(const std::string& Name);
				std::string GetProperty(const std::string& Name);
				std::string GetLocalProperty(const std::string& Name);
				float ResolveNumericProperty(const std::string& PropertyName);
				Compute::Vector2 GetContainingBlock();
				Position GetPosition();
				Float GetFloat();
				Display GetDisplay();
				float GetLineHeight();
				bool Project(Compute::Vector2& Point) const noexcept;
				bool Animate(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations = 1, bool AlternateDirection = true, float Delay = 0.0f);
				bool AddAnimationKey(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir);
				void SetPseudoClass(const std::string& PseudoClass, bool Activate);
				bool IsPseudoClassSet(const std::string& PseudoClass) const;
				void SetAttribute(const std::string& Name, const std::string& Value);
				std::string GetAttribute(const std::string& Name);
				bool HasAttribute(const std::string& Name) const;
				void RemoveAttribute(const std::string& Name);
				IElement GetFocusLeafNode();
				std::string GetTagName() const;
				std::string GetId() const;
				void SetId(const std::string& Id);
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
				void GetInnerHTML(std::string& Content) const;
				std::string GetInnerHTML() const;
				void SetInnerHTML(const std::string& HTML);
				bool IsFocused();
				bool IsHovered();
				bool IsActive();
				bool IsChecked();
				bool Focus();
				void Blur();
				void Click();
				void AddEventListener(const std::string& Event, Listener* Source, bool InCapturePhase = false);
				void RemoveEventListener(const std::string& Event, Listener* Source, bool InCapturePhase = false);
				bool DispatchEvent(const std::string& Type, const Core::VariantArgs& Args);
				void ScrollIntoView(bool AlignWithTop = true);
				IElement AppendChild(const IElement& Element, bool DOMElement = true);
				IElement InsertBefore(const IElement& Element, const IElement& AdjacentElement);
				IElement ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement);
				IElement RemoveChild(const IElement& Element);
				bool HasChildNodes() const;
				IElement GetElementById(const std::string& Id);
				IElement QuerySelector(const std::string& Selector);
				std::vector<IElement> QuerySelectorAll(const std::string& Selectors);
				bool CastFormColor(Compute::Vector4* Ptr, bool Alpha);
				bool CastFormString(std::string* Ptr);
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
				std::string GetFormName() const;
				void SetFormName(const std::string& Name);
				std::string GetFormValue() const;
				void SetFormValue(const std::string& Value);
				bool IsFormDisabled() const;
				void SetFormDisabled(bool Disable);
				Rml::Element* GetElement() const;
				bool IsValid() const;

			public:
				static std::string FromPointer(void* Ptr);
				static void* ToPointer(const std::string& Value);
			};

			class ED_OUT IElementDocument : public IElement
			{
			public:
				IElementDocument();
				IElementDocument(Rml::ElementDocument* Ref);
				virtual ~IElementDocument() = default;
				virtual void Release() override;
				void SetTitle(const std::string& Title);
				void PullToFront();
				void PushToBack();
				void Show(ModalFlag Modal = ModalFlag::None, FocusFlag Focus = FocusFlag::Auto);
				void Hide();
				void Close();
				std::string GetTitle() const;
				std::string GetSourceURL() const;
				IElement CreateElement(const std::string& Name);
				bool IsModal() const;
				Rml::ElementDocument* GetElementDocument() const;
			};

			class ED_OUT Subsystem
			{
				friend RenderSubsystem;
				friend DocumentSubsystem;
				friend ListenerSubsystem;
				friend Context;

			private:
				static Script::VMManager* ScriptInterface;
				static ContextInstancer* ContextFactory;
				static DocumentInstancer* DocumentFactory;
				static ListenerInstancer* ListenerFactory;
				static RenderSubsystem* RenderInterface;
				static FileSubsystem* FileInterface;
				static MainSubsystem* SystemInterface;
				static uint64_t Id;
				static bool HasDecorators;
				static int State;

			public:
				static bool Create();
				static bool Release();
				static void SetMetadata(Graphics::Activity* Activity, ContentManager* Content, Core::Timer* Time);
				static void SetTranslator(const std::string& Name, const TranslationCallback& Callback);
				static void SetManager(Script::VMManager* Manager);
				static RenderSubsystem* GetRenderInterface();
				static FileSubsystem* GetFileInterface();
				static MainSubsystem* GetSystemInterface();
				static Graphics::GraphicsDevice* GetDevice();
				static Graphics::Texture2D* GetBackground();
				static Compute::Matrix4x4* GetTransform();
				static Compute::Matrix4x4* GetProjection();
				static Compute::Matrix4x4 ToMatrix(const void* Matrix);
				static std::string EscapeHTML(const std::string& Text);

			private:
				static void ResizeDecorators(int Width, int Height);
				static void CreateDecorators(Graphics::GraphicsDevice* Device);
				static void ReleaseDecorators();
				static void CreateElements();
				static void ReleaseElements();
			};

			class ED_OUT DataModel : public Core::Object
			{
				friend Context;

			private:
				std::unordered_map<std::string, DataNode*> Props;
				std::vector<std::function<void()>> Callbacks;
				Rml::DataModelConstructor* Base;
				ModelCallback OnUnmount;

			private:
				DataModel(Rml::DataModelConstructor* Ref);

			public:
				virtual ~DataModel() override;
				DataNode* SetProperty(const std::string& Name, const Core::Variant& Value);
				DataNode* SetProperty(const std::string& Name, Core::Variant* Reference);
				DataNode* SetArray(const std::string& Name);
				DataNode* SetString(const std::string& Name, const std::string& Value);
				DataNode* SetInteger(const std::string& Name, int64_t Value);
				DataNode* SetFloat(const std::string& Name, float Value);
				DataNode* SetDouble(const std::string& Name, double Value);
				DataNode* SetBoolean(const std::string& Name, bool Value);
				DataNode* SetPointer(const std::string& Name, void* Value);
				DataNode* GetProperty(const std::string& Name);
				std::string GetString(const std::string& Name);
				int64_t GetInteger(const std::string& Name);
				float GetFloat(const std::string& Name);
				double GetDouble(const std::string& Name);
				bool GetBoolean(const std::string& Name);
				void* GetPointer(const std::string& Name);
				bool SetCallback(const std::string& Name, const DataCallback& Callback);
				bool SetUnmountCallback(const ModelCallback& Callback);
				bool HasChanged(const std::string& VariableName) const;
				void SetDetachCallback(std::function<void()>&& Callback);
				void Change(const std::string& VariableName);
				void Detach();
				bool IsValid() const;
				Rml::DataModelConstructor* Get();
			};

			class ED_OUT DataNode
			{
				friend Context;
				friend DataModel;

			private:
				std::vector<DataNode> Childs;
				std::string Name;
				Core::Variant* Ref;
				DataModel* Handle;
				void* Order;
				size_t Depth;
				bool Safe;

			private:
				DataNode(DataModel* Model, const std::string& TopName, const Core::Variant& Initial) noexcept;
				DataNode(DataModel* Model, const std::string& TopName, Core::Variant* Reference) noexcept;

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
				void SetString(const std::string& Value);
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
				std::string GetString();
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
				void SetValueStr(const std::string& Value);
				void SetValueNum(double Value);
				void SetValueInt(int64_t Value);
				int64_t GetValueSize();
			};

			class ED_OUT Listener : public Core::Object
			{
				friend IElement;
				friend Context;

			private:
				Rml::EventListener* Base;

			public:
				Listener(const EventCallback& NewCallback);
				Listener(const std::string& FunctionName);
				virtual ~Listener() override;
			};

			class ED_OUT Context : public Core::Object
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
				std::unordered_map<int, std::unordered_map<std::string, Rml::Element*>> Elements;
				std::unordered_map<std::string, DataModel*> Models;
				Script::VMCompiler* Compiler;
				Compute::Vector2 Cursor;
				ModelCallback OnMount;
				ScopedContext* Base;
				bool Loading;

			public:
				Context(const Compute::Vector2& Size);
				Context(Graphics::GraphicsDevice* Device);
				virtual ~Context() override;
				void EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
				void EmitInput(const char* Buffer, int Length);
				void EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod);
				void EmitResize(int Width, int Height);
				void UpdateEvents(Graphics::Activity* Activity);
				void RenderLists(Graphics::Texture2D* Target);
				void EnableMouseCursor(bool Enable);
				void ClearStyles();
				bool ClearDocuments();
				bool Initialize(const std::string& ConfPath);
				bool LoadFontFace(const std::string& Path, bool UseAsFallback = false);
				bool IsLoading();
				bool IsInputFocused();
				bool WasInputUsed(uint32_t InputTypeMask);
				int GetNumDocuments() const;
				void SetDensityIndependentPixelRatio(float DensityIndependentPixelRatio);
				float GetDensityIndependentPixelRatio() const;
				bool ReplaceHTML(const std::string& Selector, const std::string& HTML, int Index = 0);
				IElementDocument EvalHTML(const std::string& HTML, int Index = 0);
				IElementDocument AddCSS(const std::string& CSS, int Index = 0);
				IElementDocument LoadCSS(const std::string& Path, int Index = 0);
				IElementDocument LoadDocument(const std::string& Path);
				IElementDocument AddDocument(const std::string& HTML);
				IElementDocument AddDocumentEmpty(const std::string& InstancerName = "body");
				IElementDocument GetDocument(const std::string& Id);
				IElementDocument GetDocument(int Index);
				IElement GetElementById(const std::string& Id, int Index = 0);
				IElement GetHoverElement();
				IElement GetFocusElement();
				IElement GetRootElement();
				IElement GetElementAtPoint(const Compute::Vector2& Point, const IElement& IgnoreElement = nullptr, const IElement& Element = nullptr) const;
				void PullDocumentToFront(const IElementDocument& Schema);
				void PushDocumentToBack(const IElementDocument& Schema);
				void UnfocusDocument(const IElementDocument& Schema);
				void AddEventListener(const std::string& Event, Listener* Listener, bool InCapturePhase = false);
				void RemoveEventListener(const std::string& Event, Listener* Listener, bool InCapturePhase = false);
				bool IsMouseInteracting() const;
				bool GetActiveClipRegion(Compute::Vector2& Origin, Compute::Vector2& Dimensions) const;
				void SetActiveClipRegion(const Compute::Vector2& Origin, const Compute::Vector2& Dimensions);
				bool RemoveDataModel(const std::string& Name);
				bool RemoveDataModels();
				void SetDocumentsBaseTag(const std::string& Tag);
				void SetMountCallback(const ModelCallback& Callback);
				std::string GetDocumentsBaseTag();
				std::unordered_map<std::string, bool>* GetFontFaces();
				Compute::Vector2 GetDimensions() const;
				DataModel* SetDataModel(const std::string& Name);
				DataModel* GetDataModel(const std::string& Name);
				Rml::Context* GetContext();

			private:
				bool Initialize(Core::Schema* Conf, const std::string& Relative);
				bool Preprocess(const std::string& Path, std::string& Buffer);
				void Decompose(std::string& Buffer);
				void CreateVM();
				void ClearVM();

			private:
				static int GetKeyCode(Graphics::KeyCode Key);
				static int GetKeyMod(Graphics::KeyMod Mod);
			};
		}
	}
}
#endif
