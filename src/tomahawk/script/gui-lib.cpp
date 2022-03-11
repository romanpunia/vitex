#include "gui-lib.h"
#ifdef TH_WITH_RMLUI
#include <RmlUi/Core.h>
#endif
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

namespace Tomahawk
{
	namespace Script
	{
#ifdef TH_WITH_RMLUI
		bool IElementDispatchEvent(Engine::GUI::IElement& Base, const std::string& Name, Core::Schema* Args)
		{
			Core::VariantArgs Data;
			if (Args != nullptr)
			{
				Data.reserve(Args->Size());
				for (auto Item : Args->GetChilds())
					Data[Item->Key] = Item->Value;
			}

			return Base.DispatchEvent(Name, Data);
		}
		STDArray* IElementQuerySelectorAll(Engine::GUI::IElement& Base, const std::string& Value)
		{
			VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl("Array<Element>@");
			return STDArray::Compose(Type.GetTypeInfo(), Base.QuerySelectorAll(Value));
		}

		bool IElementDocumentDispatchEvent(Engine::GUI::IElementDocument& Base, const std::string& Name, Core::Schema* Args)
		{
			Core::VariantArgs Data;
			if (Args != nullptr)
			{
				Data.reserve(Args->Size());
				for (auto Item : Args->GetChilds())
					Data[Item->Key] = Item->Value;
			}

			return Base.DispatchEvent(Name, Data);
		}
		STDArray* IElementDocumentQuerySelectorAll(Engine::GUI::IElementDocument& Base, const std::string& Value)
		{
			VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl("Array<Element>@");
			return STDArray::Compose(Type.GetTypeInfo(), Base.QuerySelectorAll(Value));
		}

		bool DataModelSetRecursive(Engine::GUI::DataNode* Node, Core::Schema* Data, size_t Depth)
		{
			size_t Index = 0;
			for (auto& Item : Data->GetChilds())
			{
				auto& Child = Node->Add(Item->Value);
				Child.SetTop((void*)(uintptr_t)Index++, Depth);
				if (Item->Value.IsObject())
					DataModelSetRecursive(&Child, Item, Depth + 1);
			}

			Node->SortTree();
			return true;
		}
		bool DataModelGetRecursive(Engine::GUI::DataNode* Node, Core::Schema* Data)
		{
			size_t Size = Node->GetSize();
			for (size_t i = 0; i < Size; i++)
			{
				auto& Child = Node->At(i);
				DataModelGetRecursive(&Child, Data->Push(Child.Get()));
			}

			return true;
		}
		bool DataModelSet(Engine::GUI::DataModel* Base, const std::string& Name, Core::Schema* Data)
		{
			if (!Data->Value.IsObject())
				return Base->SetProperty(Name, Data->Value) != nullptr;

			Engine::GUI::DataNode* Node = Base->SetArray(Name);
			if (!Node)
				return false;

			return DataModelSetRecursive(Node, Data, 0);
		}
		bool DataModelSetVar(Engine::GUI::DataModel* Base, const std::string& Name, const Core::Variant& Data)
		{
			return Base->SetProperty(Name, Data) != nullptr;
		}
		bool DataModelSetString(Engine::GUI::DataModel* Base, const std::string& Name, const std::string& Value)
		{
			return Base->SetString(Name, Value) != nullptr;
		}
		bool DataModelSetInteger(Engine::GUI::DataModel* Base, const std::string& Name, int64_t Value)
		{
			return Base->SetInteger(Name, Value) != nullptr;
		}
		bool DataModelSetFloat(Engine::GUI::DataModel* Base, const std::string& Name, float Value)
		{
			return Base->SetFloat(Name, Value) != nullptr;
		}
		bool DataModelSetDouble(Engine::GUI::DataModel* Base, const std::string& Name, double Value)
		{
			return Base->SetDouble(Name, Value) != nullptr;
		}
		bool DataModelSetBoolean(Engine::GUI::DataModel* Base, const std::string& Name, bool Value)
		{
			return Base->SetBoolean(Name, Value) != nullptr;
		}
		bool DataModelSetPointer(Engine::GUI::DataModel* Base, const std::string& Name, void* Value)
		{
			return Base->SetPointer(Name, Value) != nullptr;
		}
		bool DataModelSetCallback(Engine::GUI::DataModel* Base, const std::string& Name, VMCFunction* Callback)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return false;

			if (!Callback)
				return false;

			Callback->AddRef();
			Context->AddRef();

			VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl("Array<CE::Variant>@");
			Base->SetDetachCallback([Callback, Context]()
			{
				Callback->Release();
				Context->Release();
			});

			return Base->SetCallback(Name, [Type, Context, Callback](Engine::GUI::IEvent& Wrapper, const Core::VariantList& Args)
			{
				Rml::Event& Event = *Wrapper.GetEvent();
				Rml::Event* Ptr = Rml::Factory::InstanceEvent(Event.GetTargetElement(), Event.GetId(), Event.GetType(), Event.GetParameters(), Event.IsInterruptible()).release();
				if (Ptr != nullptr)
				{
					Ptr->SetCurrentElement(Event.GetCurrentElement());
					Ptr->SetPhase(Event.GetPhase());
				}

				STDArray* Data = STDArray::Compose(Type.GetTypeInfo(), Args);
				Context->TryExecuteAsync(Callback, [Ptr, &Data](VMContext* Context)
				{
					Engine::GUI::IEvent Event(Ptr);
					Context->SetArgObject(0, &Event);
					Context->SetArgObject(1, &Data);
				}, nullptr).Await([Ptr](int&&)
				{
					delete Ptr;
				});
			});
		}
		Core::Schema* DataModelGet(Engine::GUI::DataModel* Base, const std::string& Name)
		{
			Engine::GUI::DataNode* Node = Base->GetProperty(Name);
			if (!Node)
				return nullptr;

			Core::Schema* Result = new Core::Schema(Node->Get());
			if (Result->Value.IsObject())
				DataModelGetRecursive(Node, Result);

			return Result;
		}

		Engine::GUI::IElement ContextGetFocusElement(Engine::GUI::Context* Base, const Compute::Vector2& Value)
		{
			return Base->GetElementAtPoint(Value);
		}

		GUIListener::GUIListener(VMCFunction* NewCallback) : Engine::GUI::Listener(Bind(NewCallback)), Source(NewCallback), Context(nullptr)
		{
		}
		GUIListener::GUIListener(const std::string& FunctionName) : Engine::GUI::Listener(FunctionName), Source(nullptr), Context(nullptr)
		{
		}
		GUIListener::~GUIListener()
		{
			if (Source != nullptr)
				Source->Release();

			if (Context != nullptr)
				Context->Release();
		}
		Engine::GUI::EventCallback GUIListener::Bind(VMCFunction* Callback)
		{
			if (Context != nullptr)
				Context->Release();
			Context = VMContext::Get();

			if (Source != nullptr)
				Source->Release();
			Source = Callback;

			if (!Context || !Source)
				return nullptr;

			Source->AddRef();
			Context->AddRef();

			return [this](Engine::GUI::IEvent& Wrapper)
			{
				Rml::Event& Event = *Wrapper.GetEvent();
				Rml::Event* Ptr = Rml::Factory::InstanceEvent(Event.GetTargetElement(), Event.GetId(), Event.GetType(), Event.GetParameters(), Event.IsInterruptible()).release();
				if (Ptr != nullptr)
				{
					Ptr->SetCurrentElement(Event.GetCurrentElement());
					Ptr->SetPhase(Event.GetPhase());
				}

				Context->TryExecuteAsync(Source, [Ptr](VMContext* Context)
				{
					Engine::GUI::IEvent Event(Ptr);
					Context->SetArgObject(0, &Event);
				}, nullptr).Await([Ptr](int&&)
				{
					delete Ptr;
				});
			};
		}
#endif
		bool GUIRegisterVariant(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("GUI::Variant");
			Register.SetFunction("CU::Vector4 ToColor4(const String &in)", &Engine::GUI::IVariant::ToColor4);
			Register.SetFunction("String FromColor4(const CU::Vector4 &in, bool)", &Engine::GUI::IVariant::FromColor4);
			Register.SetFunction("CU::Vector4 ToColor3(const String &in)", &Engine::GUI::IVariant::ToColor3);
			Register.SetFunction("String FromColor3(const CU::Vector4 &in, bool)", &Engine::GUI::IVariant::ToColor3);
			Register.SetFunction("int GetVectorType(const String &in)", &Engine::GUI::IVariant::GetVectorType);
			Register.SetFunction("CU::Vector4 ToVector4(const String &in)", &Engine::GUI::IVariant::ToVector4);
			Register.SetFunction("String FromVector4(const CU::Vector4 &in)", &Engine::GUI::IVariant::FromVector4);
			Register.SetFunction("CU::Vector3 ToVector3(const String &in)", &Engine::GUI::IVariant::ToVector3);
			Register.SetFunction("String FromVector3(const CU::Vector3 &in)", &Engine::GUI::IVariant::FromVector3);
			Register.SetFunction("CU::Vector2 ToVector2(const String &in)", &Engine::GUI::IVariant::ToVector2);
			Register.SetFunction("String FromVector2(const CU::Vector2 &in)", &Engine::GUI::IVariant::FromVector2);
			Engine->EndNamespace();

			return true;
#else
			return false;
#endif
		}
		bool GUIRegisterControl(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();

			Engine->BeginNamespace("GUI");
			VMEnum VEventPhase = Register.SetEnum("EventPhase");
			VMEnum VArea = Register.SetEnum("Area");
			VMEnum VDisplay = Register.SetEnum("Display");
			VMEnum VPosition = Register.SetEnum("Position");
			VMEnum VFloat = Register.SetEnum("Float");
			VMEnum VTimingFunc = Register.SetEnum("TimingFunc");
			VMEnum VTimingDir = Register.SetEnum("TimingDir");
			VMEnum VFocusFlag = Register.SetEnum("FocusFlag");
			VMEnum VModalFlag = Register.SetEnum("ModalFlag");
			VMTypeClass VElement = Register.SetStructUnmanaged<Engine::GUI::IElement>("Element");
			VMTypeClass VDocument = Register.SetStructUnmanaged<Engine::GUI::IElementDocument>("Schema");
			VMTypeClass VEvent = Register.SetStructUnmanaged<Engine::GUI::IEvent>("Event");
			VMRefClass VListener = Register.SetClassUnmanaged<GUIListener>("Listener");
			Register.SetFunctionDef("void GUIListenerCallback(GUI::Event &in)");

			VModalFlag.SetValue("None", (int)Engine::GUI::ModalFlag::None);
			VModalFlag.SetValue("Modal", (int)Engine::GUI::ModalFlag::Modal);
			VModalFlag.SetValue("Keep", (int)Engine::GUI::ModalFlag::Keep);

			VFocusFlag.SetValue("None", (int)Engine::GUI::FocusFlag::None);
			VFocusFlag.SetValue("Schema", (int)Engine::GUI::FocusFlag::Schema);
			VFocusFlag.SetValue("Keep", (int)Engine::GUI::FocusFlag::Keep);
			VFocusFlag.SetValue("Auto", (int)Engine::GUI::FocusFlag::Auto);

			VEventPhase.SetValue("None", (int)Engine::GUI::EventPhase::None);
			VEventPhase.SetValue("Capture", (int)Engine::GUI::EventPhase::Capture);
			VEventPhase.SetValue("Target", (int)Engine::GUI::EventPhase::Target);
			VEventPhase.SetValue("Bubble", (int)Engine::GUI::EventPhase::Bubble);

			VEvent.SetConstructor<Engine::GUI::IEvent>("void f()");
			VEvent.SetConstructor<Engine::GUI::IEvent, Rml::Event*>("void f(Address@)");
			VEvent.SetMethod("EventPhase GetPhase() const", &Engine::GUI::IEvent::GetPhase);
			VEvent.SetMethod("void SetPhase(EventPhase Phase)", &Engine::GUI::IEvent::SetPhase);
			VEvent.SetMethod("void SetCurrentElement(const Element &in)", &Engine::GUI::IEvent::SetCurrentElement);
			VEvent.SetMethod("Element GetCurrentElement() const", &Engine::GUI::IEvent::GetCurrentElement);
			VEvent.SetMethod("Element GetTargetElement() const", &Engine::GUI::IEvent::GetTargetElement);
			VEvent.SetMethod("String GetType() const", &Engine::GUI::IEvent::GetType);
			VEvent.SetMethod("void StopPropagation()", &Engine::GUI::IEvent::StopPropagation);
			VEvent.SetMethod("void StopImmediatePropagation()", &Engine::GUI::IEvent::StopImmediatePropagation);
			VEvent.SetMethod("bool IsInterruptible() const", &Engine::GUI::IEvent::IsInterruptible);
			VEvent.SetMethod("bool IsPropagating() const", &Engine::GUI::IEvent::IsPropagating);
			VEvent.SetMethod("bool IsImmediatePropagating() const", &Engine::GUI::IEvent::IsImmediatePropagating);
			VEvent.SetMethod("bool GetBoolean(const String &in) const", &Engine::GUI::IEvent::GetBoolean);
			VEvent.SetMethod("int64 GetInteger(const String &in) const", &Engine::GUI::IEvent::GetInteger);
			VEvent.SetMethod("double GetNumber(const String &in) const", &Engine::GUI::IEvent::GetNumber);
			VEvent.SetMethod("String GetString(const String &in) const", &Engine::GUI::IEvent::GetString);
			VEvent.SetMethod("CU::Vector2 GetVector2(const String &in) const", &Engine::GUI::IEvent::GetVector2);
			VEvent.SetMethod("CU::Vector3 GetVector3(const String &in) const", &Engine::GUI::IEvent::GetVector3);
			VEvent.SetMethod("CU::Vector4 GetVector4(const String &in) const", &Engine::GUI::IEvent::GetVector4);
			VEvent.SetMethod("Address@ GetPointer(const String &in) const", &Engine::GUI::IEvent::GetPointer);
			VEvent.SetMethod("Address@ GetEvent() const", &Engine::GUI::IEvent::GetEvent);
			VEvent.SetMethod("bool IsValid() const", &Engine::GUI::IEvent::IsValid);

			VListener.SetUnmanagedConstructor<GUIListener, VMCFunction*>("Listener@ f(GUIListenerCallback@)");
			VListener.SetUnmanagedConstructor<GUIListener, const std::string&>("Listener@ f(const String &in)");

			VArea.SetValue("Margin", (int)Engine::GUI::Area::Margin);
			VArea.SetValue("Border", (int)Engine::GUI::Area::Border);
			VArea.SetValue("Padding", (int)Engine::GUI::Area::Padding);
			VArea.SetValue("Content", (int)Engine::GUI::Area::Content);

			VDisplay.SetValue("None", (int)Engine::GUI::Display::None);
			VDisplay.SetValue("Block", (int)Engine::GUI::Display::Block);
			VDisplay.SetValue("Inline", (int)Engine::GUI::Display::Inline);
			VDisplay.SetValue("InlineBlock", (int)Engine::GUI::Display::InlineBlock);
			VDisplay.SetValue("Table", (int)Engine::GUI::Display::Table);
			VDisplay.SetValue("TableRow", (int)Engine::GUI::Display::TableRow);
			VDisplay.SetValue("TableRowGroup", (int)Engine::GUI::Display::TableRowGroup);
			VDisplay.SetValue("TableColumn", (int)Engine::GUI::Display::TableColumn);
			VDisplay.SetValue("TableColumnGroup", (int)Engine::GUI::Display::TableColumnGroup);
			VDisplay.SetValue("TableCell", (int)Engine::GUI::Display::TableCell);

			VPosition.SetValue("Static", (int)Engine::GUI::Position::Static);
			VPosition.SetValue("Relative", (int)Engine::GUI::Position::Relative);
			VPosition.SetValue("Absolute", (int)Engine::GUI::Position::Absolute);
			VPosition.SetValue("Fixed", (int)Engine::GUI::Position::Fixed);

			VFloat.SetValue("None", (int)Engine::GUI::Float::None);
			VFloat.SetValue("Left", (int)Engine::GUI::Float::Left);
			VFloat.SetValue("Right", (int)Engine::GUI::Float::Right);

			VTimingFunc.SetValue("None", (int)Engine::GUI::TimingFunc::None);
			VTimingFunc.SetValue("Back", (int)Engine::GUI::TimingFunc::Back);
			VTimingFunc.SetValue("Bounce", (int)Engine::GUI::TimingFunc::Bounce);
			VTimingFunc.SetValue("Circular", (int)Engine::GUI::TimingFunc::Circular);
			VTimingFunc.SetValue("Cubic", (int)Engine::GUI::TimingFunc::Cubic);
			VTimingFunc.SetValue("Elastic", (int)Engine::GUI::TimingFunc::Elastic);
			VTimingFunc.SetValue("Exponential", (int)Engine::GUI::TimingFunc::Exponential);
			VTimingFunc.SetValue("Linear", (int)Engine::GUI::TimingFunc::Linear);
			VTimingFunc.SetValue("Quadratic", (int)Engine::GUI::TimingFunc::Quadratic);
			VTimingFunc.SetValue("Quartic", (int)Engine::GUI::TimingFunc::Quartic);
			VTimingFunc.SetValue("Sine", (int)Engine::GUI::TimingFunc::Sine);
			VTimingFunc.SetValue("Callback", (int)Engine::GUI::TimingFunc::Callback);

			VTimingDir.SetValue("In", (int)Engine::GUI::TimingDir::In);
			VTimingDir.SetValue("Out", (int)Engine::GUI::TimingDir::Out);
			VTimingDir.SetValue("InOut", (int)Engine::GUI::TimingDir::InOut);

			VElement.SetConstructor<Engine::GUI::IElement>("void f()");
			VElement.SetConstructor<Engine::GUI::IElement, Rml::Element*>("void f(Address@)");
			VElement.SetMethod("Element Clone() const", &Engine::GUI::IElement::Clone);
			VElement.SetMethod("void SetClass(const String &in, bool)", &Engine::GUI::IElement::SetClass);
			VElement.SetMethod("bool IsClassSet(const String &in) const", &Engine::GUI::IElement::IsClassSet);
			VElement.SetMethod("void SetClassNames(const String &in)", &Engine::GUI::IElement::SetClassNames);
			VElement.SetMethod("String GetClassNames() const", &Engine::GUI::IElement::GetClassNames);
			VElement.SetMethod("String GetAddress(bool = false, bool = true) const", &Engine::GUI::IElement::GetAddress);
			VElement.SetMethod("void SetOffset(const CU::Vector2 &in, const Element &in, bool = false)", &Engine::GUI::IElement::SetOffset);
			VElement.SetMethod("CU::Vector2 GetRelativeOffset(Area = Area::Content) const", &Engine::GUI::IElement::GetRelativeOffset);
			VElement.SetMethod("CU::Vector2 GetAbsoluteOffset(Area = Area::Content) const", &Engine::GUI::IElement::GetAbsoluteOffset);
			VElement.SetMethod("void SetClientArea(Area)", &Engine::GUI::IElement::SetClientArea);
			VElement.SetMethod("Area GetClientArea() const", &Engine::GUI::IElement::GetClientArea);
			VElement.SetMethod("void SetContentBox(const CU::Vector2 &in, const CU::Vector2 &in)", &Engine::GUI::IElement::SetContentBox);
			VElement.SetMethod("float GetBaseline() const", &Engine::GUI::IElement::GetBaseline);
			VElement.SetMethod("bool GetIntrinsicDimensions(CU::Vector2 &out, float &out)", &Engine::GUI::IElement::GetIntrinsicDimensions);
			VElement.SetMethod("bool IsPointWithinElement(const CU::Vector2 &in)", &Engine::GUI::IElement::IsPointWithinElement);
			VElement.SetMethod("bool IsVisible() const", &Engine::GUI::IElement::IsVisible);
			VElement.SetMethod("float GetZIndex() const", &Engine::GUI::IElement::GetZIndex);
			VElement.SetMethod("bool SetProperty(const String &in, const String &in)", &Engine::GUI::IElement::SetProperty);
			VElement.SetMethod("void RemoveProperty(const String &in)", &Engine::GUI::IElement::RemoveProperty);
			VElement.SetMethod("String GetProperty(const String &in) const", &Engine::GUI::IElement::GetProperty);
			VElement.SetMethod("String GetLocalProperty(const String &in) const", &Engine::GUI::IElement::GetLocalProperty);
			VElement.SetMethod("float ResolveNumericProperty(const String &in) const", &Engine::GUI::IElement::ResolveNumericProperty);
			VElement.SetMethod("CU::Vector2 GetContainingBlock() const", &Engine::GUI::IElement::GetContainingBlock);
			VElement.SetMethod("Position GetPosition() const", &Engine::GUI::IElement::GetPosition);
			VElement.SetMethod("Float GetFloat() const", &Engine::GUI::IElement::GetFloat);
			VElement.SetMethod("Display GetDisplay() const", &Engine::GUI::IElement::GetDisplay);
			VElement.SetMethod("float GetLineHeight() const", &Engine::GUI::IElement::GetLineHeight);
			VElement.SetMethod("bool Project(CU::Vector2 &out) const", &Engine::GUI::IElement::Project);
			VElement.SetMethod("bool Animate(const String &in, const String &in, float, TimingFunc, TimingDir, int = -1, bool = true, float = 0)", &Engine::GUI::IElement::Animate);
			VElement.SetMethod("bool AddAnimationKey(const String &in, const String &in, float, TimingFunc, TimingDir)", &Engine::GUI::IElement::AddAnimationKey);
			VElement.SetMethod("void SetPseudoClass(const String &in, bool)", &Engine::GUI::IElement::SetPseudoClass);
			VElement.SetMethod("bool IsPseudoClassSet(const String &in) const", &Engine::GUI::IElement::IsPseudoClassSet);
			VElement.SetMethod("void SetAttribute(const String &in, const String &in)", &Engine::GUI::IElement::SetAttribute);
			VElement.SetMethod("String GetAttribute(const String &in) const", &Engine::GUI::IElement::GetAttribute);
			VElement.SetMethod("bool HasAttribute(const String &in) const", &Engine::GUI::IElement::HasAttribute);
			VElement.SetMethod("void RemoveAttribute(const String &in)", &Engine::GUI::IElement::RemoveAttribute);
			VElement.SetMethod("Element GetFocusLeafNode()", &Engine::GUI::IElement::GetFocusLeafNode);
			VElement.SetMethod("String GetTagName() const", &Engine::GUI::IElement::GetTagName);
			VElement.SetMethod("String GetId() const", &Engine::GUI::IElement::GetId);
			VElement.SetMethod("float GetAbsoluteLeft()", &Engine::GUI::IElement::GetAbsoluteLeft);
			VElement.SetMethod("float GetAbsoluteTop()", &Engine::GUI::IElement::GetAbsoluteTop);
			VElement.SetMethod("float GetClientLeft()", &Engine::GUI::IElement::GetClientLeft);
			VElement.SetMethod("float GetClientTop()", &Engine::GUI::IElement::GetClientTop);
			VElement.SetMethod("float GetClientWidth()", &Engine::GUI::IElement::GetClientWidth);
			VElement.SetMethod("float GetClientHeight()", &Engine::GUI::IElement::GetClientHeight);
			VElement.SetMethod("Element GetOffsetParent()", &Engine::GUI::IElement::GetOffsetParent);
			VElement.SetMethod("float GetOffsetLeft()", &Engine::GUI::IElement::GetOffsetLeft);
			VElement.SetMethod("float GetOffsetTop()", &Engine::GUI::IElement::GetOffsetTop);
			VElement.SetMethod("float GetOffsetWidth()", &Engine::GUI::IElement::GetOffsetWidth);
			VElement.SetMethod("float GetOffsetHeight()", &Engine::GUI::IElement::GetOffsetHeight);
			VElement.SetMethod("float GetScrollLeft()", &Engine::GUI::IElement::GetScrollLeft);
			VElement.SetMethod("void SetScrollLeft(float)", &Engine::GUI::IElement::SetScrollLeft);
			VElement.SetMethod("float GetScrollTop()", &Engine::GUI::IElement::GetScrollTop);
			VElement.SetMethod("void SetScrollTop(float)", &Engine::GUI::IElement::SetScrollTop);
			VElement.SetMethod("float GetScrollWidth()", &Engine::GUI::IElement::GetScrollWidth);
			VElement.SetMethod("float GetScrollHeight()", &Engine::GUI::IElement::GetScrollHeight);
			VElement.SetMethod("Schema GetOwnerDocument() const", &Engine::GUI::IElement::GetOwnerDocument);
			VElement.SetMethod("Element GetParentNode() const", &Engine::GUI::IElement::GetParentNode);
			VElement.SetMethod("Element GetNextSibling() const", &Engine::GUI::IElement::GetNextSibling);
			VElement.SetMethod("Element GetPreviousSibling() const", &Engine::GUI::IElement::GetPreviousSibling);
			VElement.SetMethod("Element GetFirstChild() const", &Engine::GUI::IElement::GetFirstChild);
			VElement.SetMethod("Element GetLastChild() const", &Engine::GUI::IElement::GetLastChild);
			VElement.SetMethod("Element GetChild(int) const", &Engine::GUI::IElement::GetChild);
			VElement.SetMethod("int GetNumChildren(bool = false) const", &Engine::GUI::IElement::GetNumChildren);
			VElement.SetMethod<Engine::GUI::IElement, void, std::string&>("void GetInnerHTML(String &out) const", &Engine::GUI::IElement::GetInnerHTML);
			VElement.SetMethod<Engine::GUI::IElement, std::string>("String GetInnerHTML() const", &Engine::GUI::IElement::GetInnerHTML);
			VElement.SetMethod("void SetInnerHTML(const String &in)", &Engine::GUI::IElement::SetInnerHTML);
			VElement.SetMethod("bool IsFocused()", &Engine::GUI::IElement::IsFocused);
			VElement.SetMethod("bool IsHovered()", &Engine::GUI::IElement::IsHovered);
			VElement.SetMethod("bool IsActive()", &Engine::GUI::IElement::IsActive);
			VElement.SetMethod("bool IsChecked()", &Engine::GUI::IElement::IsChecked);
			VElement.SetMethod("bool Focus()", &Engine::GUI::IElement::Focus);
			VElement.SetMethod("void Blur()", &Engine::GUI::IElement::Blur);
			VElement.SetMethod("void Click()", &Engine::GUI::IElement::Click);
			VElement.SetMethod("void AddEventListener(const String &in, Listener@+, bool = false)", &Engine::GUI::IElement::AddEventListener);
			VElement.SetMethod("void RemoveEventListener(const String &in, Listener@+, bool = false)", &Engine::GUI::IElement::RemoveEventListener);
			VElement.SetMethodEx("bool DispatchEvent(const String &in, CE::Schema@+)", &IElementDispatchEvent);
			VElement.SetMethod("void ScrollIntoView(bool = true)", &Engine::GUI::IElement::ScrollIntoView);
			VElement.SetMethod("Element AppendChild(const Element &in, bool = true)", &Engine::GUI::IElement::AppendChild);
			VElement.SetMethod("Element InsertBefore(const Element &in, const Element &in)", &Engine::GUI::IElement::InsertBefore);
			VElement.SetMethod("Element ReplaceChild(const Element &in, const Element &in)", &Engine::GUI::IElement::ReplaceChild);
			VElement.SetMethod("Element RemoveChild(const Element &in)", &Engine::GUI::IElement::RemoveChild);
			VElement.SetMethod("bool HasChildNodes() const", &Engine::GUI::IElement::HasChildNodes);
			VElement.SetMethod("Element GetElementById(const String &in)", &Engine::GUI::IElement::GetElementById);
			VElement.SetMethodEx("Array<Element>@ QuerySelectorAll(const String &in)", &IElementQuerySelectorAll);
			VElement.SetMethod("int GetClippingIgnoreDepth()", &Engine::GUI::IElement::GetClippingIgnoreDepth);
			VElement.SetMethod("bool IsClippingEnabled()", &Engine::GUI::IElement::IsClippingEnabled);
			VElement.SetMethod("bool CastFormColor(CU::Vector4 &out, bool)", &Engine::GUI::IElement::CastFormColor);
			VElement.SetMethod("bool CastFormString(String &out)", &Engine::GUI::IElement::CastFormString);
			VElement.SetMethod("bool CastFormPointer(Address@ &out)", &Engine::GUI::IElement::CastFormPointer);
			VElement.SetMethod("bool CastFormInt32(int &out)", &Engine::GUI::IElement::CastFormInt32);
			VElement.SetMethod("bool CastFormUInt32(uint &out)", &Engine::GUI::IElement::CastFormUInt32);
			VElement.SetMethod("bool CastFormFlag32(uint &out, uint)", &Engine::GUI::IElement::CastFormFlag32);
			VElement.SetMethod("bool CastFormInt64(int64 &out)", &Engine::GUI::IElement::CastFormInt64);
			VElement.SetMethod("bool CastFormUInt64(uint64 &out)", &Engine::GUI::IElement::CastFormUInt64);
			VElement.SetMethod("bool CastFormFlag64(uint64 &out, uint64)", &Engine::GUI::IElement::CastFormFlag64);
			VElement.SetMethod<Engine::GUI::IElement, bool, float*>("bool CastFormFloat(float &out)", &Engine::GUI::IElement::CastFormFloat);
			VElement.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool CastFormFloat(float &out, float)", &Engine::GUI::IElement::CastFormFloat);
			VElement.SetMethod("bool CastFormDouble(double &out)", &Engine::GUI::IElement::CastFormDouble);
			VElement.SetMethod("bool CastFormBoolean(bool &out)", &Engine::GUI::IElement::CastFormBoolean);
			VElement.SetMethod("String GetFormName() const", &Engine::GUI::IElement::GetFormName);
			VElement.SetMethod("void SetFormName(const String &in)", &Engine::GUI::IElement::SetFormName);
			VElement.SetMethod("String GetFormValue() const", &Engine::GUI::IElement::GetFormValue);
			VElement.SetMethod("void SetFormValue(const String &in)", &Engine::GUI::IElement::SetFormValue);
			VElement.SetMethod("bool IsFormDisabled() const", &Engine::GUI::IElement::IsFormDisabled);
			VElement.SetMethod("void SetFormDisabled(bool)", &Engine::GUI::IElement::SetFormDisabled);
			VElement.SetMethod("Address@ GetElement() const", &Engine::GUI::IElement::GetElement);
			VElement.SetMethod("bool IsValid() const", &Engine::GUI::IElement::GetElement);

			VDocument.SetConstructor<Engine::GUI::IElementDocument>("void f()");
			VDocument.SetConstructor<Engine::GUI::IElementDocument, Rml::ElementDocument*>("void f(Address@)");
			VDocument.SetMethod("Element Clone() const", &Engine::GUI::IElementDocument::Clone);
			VDocument.SetMethod("void SetClass(const String &in, bool)", &Engine::GUI::IElementDocument::SetClass);
			VDocument.SetMethod("bool IsClassSet(const String &in) const", &Engine::GUI::IElementDocument::IsClassSet);
			VDocument.SetMethod("void SetClassNames(const String &in)", &Engine::GUI::IElementDocument::SetClassNames);
			VDocument.SetMethod("String GetClassNames() const", &Engine::GUI::IElementDocument::GetClassNames);
			VDocument.SetMethod("String GetAddress(bool = false, bool = true) const", &Engine::GUI::IElementDocument::GetAddress);
			VDocument.SetMethod("void SetOffset(const CU::Vector2 &in, const Element &in, bool = false)", &Engine::GUI::IElementDocument::SetOffset);
			VDocument.SetMethod("CU::Vector2 GetRelativeOffset(Area = Area::Content) const", &Engine::GUI::IElementDocument::GetRelativeOffset);
			VDocument.SetMethod("CU::Vector2 GetAbsoluteOffset(Area = Area::Content) const", &Engine::GUI::IElementDocument::GetAbsoluteOffset);
			VDocument.SetMethod("void SetClientArea(Area)", &Engine::GUI::IElementDocument::SetClientArea);
			VDocument.SetMethod("Area GetClientArea() const", &Engine::GUI::IElementDocument::GetClientArea);
			VDocument.SetMethod("void SetContentBox(const CU::Vector2 &in, const CU::Vector2 &in)", &Engine::GUI::IElementDocument::SetContentBox);
			VDocument.SetMethod("float GetBaseline() const", &Engine::GUI::IElementDocument::GetBaseline);
			VDocument.SetMethod("bool GetIntrinsicDimensions(CU::Vector2 &out, float &out)", &Engine::GUI::IElementDocument::GetIntrinsicDimensions);
			VDocument.SetMethod("bool IsPointWithinElement(const CU::Vector2 &in)", &Engine::GUI::IElementDocument::IsPointWithinElement);
			VDocument.SetMethod("bool IsVisible() const", &Engine::GUI::IElementDocument::IsVisible);
			VDocument.SetMethod("float GetZIndex() const", &Engine::GUI::IElementDocument::GetZIndex);
			VDocument.SetMethod("bool SetProperty(const String &in, const String &in)", &Engine::GUI::IElementDocument::SetProperty);
			VDocument.SetMethod("void RemoveProperty(const String &in)", &Engine::GUI::IElementDocument::RemoveProperty);
			VDocument.SetMethod("String GetProperty(const String &in) const", &Engine::GUI::IElementDocument::GetProperty);
			VDocument.SetMethod("String GetLocalProperty(const String &in) const", &Engine::GUI::IElementDocument::GetLocalProperty);
			VDocument.SetMethod("float ResolveNumericProperty(const String &in) const", &Engine::GUI::IElementDocument::ResolveNumericProperty);
			VDocument.SetMethod("CU::Vector2 GetContainingBlock() const", &Engine::GUI::IElementDocument::GetContainingBlock);
			VDocument.SetMethod("Position GetPosition() const", &Engine::GUI::IElementDocument::GetPosition);
			VDocument.SetMethod("Float GetFloat() const", &Engine::GUI::IElementDocument::GetFloat);
			VDocument.SetMethod("Display GetDisplay() const", &Engine::GUI::IElementDocument::GetDisplay);
			VDocument.SetMethod("float GetLineHeight() const", &Engine::GUI::IElementDocument::GetLineHeight);
			VDocument.SetMethod("bool Project(CU::Vector2 &out) const", &Engine::GUI::IElementDocument::Project);
			VDocument.SetMethod("bool Animate(const String &in, const String &in, float, TimingFunc, TimingDir, int = -1, bool = true, float = 0)", &Engine::GUI::IElementDocument::Animate);
			VDocument.SetMethod("bool AddAnimationKey(const String &in, const String &in, float, TimingFunc, TimingDir)", &Engine::GUI::IElementDocument::AddAnimationKey);
			VDocument.SetMethod("void SetPseudoClass(const String &in, bool)", &Engine::GUI::IElementDocument::SetPseudoClass);
			VDocument.SetMethod("bool IsPseudoClassSet(const String &in) const", &Engine::GUI::IElementDocument::IsPseudoClassSet);
			VDocument.SetMethod("void SetAttribute(const String &in, const String &in)", &Engine::GUI::IElementDocument::SetAttribute);
			VDocument.SetMethod("String GetAttribute(const String &in) const", &Engine::GUI::IElementDocument::GetAttribute);
			VDocument.SetMethod("bool HasAttribute(const String &in) const", &Engine::GUI::IElementDocument::HasAttribute);
			VDocument.SetMethod("void RemoveAttribute(const String &in)", &Engine::GUI::IElementDocument::RemoveAttribute);
			VDocument.SetMethod("Element GetFocusLeafNode()", &Engine::GUI::IElementDocument::GetFocusLeafNode);
			VDocument.SetMethod("String GetTagName() const", &Engine::GUI::IElementDocument::GetTagName);
			VDocument.SetMethod("String GetId() const", &Engine::GUI::IElementDocument::GetId);
			VDocument.SetMethod("float GetAbsoluteLeft()", &Engine::GUI::IElementDocument::GetAbsoluteLeft);
			VDocument.SetMethod("float GetAbsoluteTop()", &Engine::GUI::IElementDocument::GetAbsoluteTop);
			VDocument.SetMethod("float GetClientLeft()", &Engine::GUI::IElementDocument::GetClientLeft);
			VDocument.SetMethod("float GetClientTop()", &Engine::GUI::IElementDocument::GetClientTop);
			VDocument.SetMethod("float GetClientWidth()", &Engine::GUI::IElementDocument::GetClientWidth);
			VDocument.SetMethod("float GetClientHeight()", &Engine::GUI::IElementDocument::GetClientHeight);
			VDocument.SetMethod("Element GetOffsetParent()", &Engine::GUI::IElementDocument::GetOffsetParent);
			VDocument.SetMethod("float GetOffsetLeft()", &Engine::GUI::IElementDocument::GetOffsetLeft);
			VDocument.SetMethod("float GetOffsetTop()", &Engine::GUI::IElementDocument::GetOffsetTop);
			VDocument.SetMethod("float GetOffsetWidth()", &Engine::GUI::IElementDocument::GetOffsetWidth);
			VDocument.SetMethod("float GetOffsetHeight()", &Engine::GUI::IElementDocument::GetOffsetHeight);
			VDocument.SetMethod("float GetScrollLeft()", &Engine::GUI::IElementDocument::GetScrollLeft);
			VDocument.SetMethod("void SetScrollLeft(float)", &Engine::GUI::IElementDocument::SetScrollLeft);
			VDocument.SetMethod("float GetScrollTop()", &Engine::GUI::IElementDocument::GetScrollTop);
			VDocument.SetMethod("void SetScrollTop(float)", &Engine::GUI::IElementDocument::SetScrollTop);
			VDocument.SetMethod("float GetScrollWidth()", &Engine::GUI::IElementDocument::GetScrollWidth);
			VDocument.SetMethod("float GetScrollHeight()", &Engine::GUI::IElementDocument::GetScrollHeight);
			VDocument.SetMethod("Schema GetOwnerDocument() const", &Engine::GUI::IElementDocument::GetOwnerDocument);
			VDocument.SetMethod("Element GetParentNode() const", &Engine::GUI::IElementDocument::GetParentNode);
			VDocument.SetMethod("Element GetNextSibling() const", &Engine::GUI::IElementDocument::GetNextSibling);
			VDocument.SetMethod("Element GetPreviousSibling() const", &Engine::GUI::IElementDocument::GetPreviousSibling);
			VDocument.SetMethod("Element GetFirstChild() const", &Engine::GUI::IElementDocument::GetFirstChild);
			VDocument.SetMethod("Element GetLastChild() const", &Engine::GUI::IElementDocument::GetLastChild);
			VDocument.SetMethod("Element GetChild(int) const", &Engine::GUI::IElementDocument::GetChild);
			VDocument.SetMethod("int GetNumChildren(bool = false) const", &Engine::GUI::IElementDocument::GetNumChildren);
			VDocument.SetMethod<Engine::GUI::IElement, void, std::string&>("void GetInnerHTML(String &out) const", &Engine::GUI::IElementDocument::GetInnerHTML);
			VDocument.SetMethod<Engine::GUI::IElement, std::string>("String GetInnerHTML() const", &Engine::GUI::IElementDocument::GetInnerHTML);
			VDocument.SetMethod("void SetInnerHTML(const String &in)", &Engine::GUI::IElementDocument::SetInnerHTML);
			VDocument.SetMethod("bool IsFocused()", &Engine::GUI::IElementDocument::IsFocused);
			VDocument.SetMethod("bool IsHovered()", &Engine::GUI::IElementDocument::IsHovered);
			VDocument.SetMethod("bool IsActive()", &Engine::GUI::IElementDocument::IsActive);
			VDocument.SetMethod("bool IsChecked()", &Engine::GUI::IElementDocument::IsChecked);
			VDocument.SetMethod("bool Focus()", &Engine::GUI::IElementDocument::Focus);
			VDocument.SetMethod("void Blur()", &Engine::GUI::IElementDocument::Blur);
			VDocument.SetMethod("void Click()", &Engine::GUI::IElementDocument::Click);
			VDocument.SetMethod("void AddEventListener(const String &in, Listener@+, bool = false)", &Engine::GUI::IElementDocument::AddEventListener);
			VDocument.SetMethod("void RemoveEventListener(const String &in, Listener@+, bool = false)", &Engine::GUI::IElementDocument::RemoveEventListener);
			VDocument.SetMethodEx("bool DispatchEvent(const String &in, CE::Schema@+)", &IElementDocumentDispatchEvent);
			VDocument.SetMethod("void ScrollIntoView(bool = true)", &Engine::GUI::IElementDocument::ScrollIntoView);
			VDocument.SetMethod("Element AppendChild(const Element &in, bool = true)", &Engine::GUI::IElementDocument::AppendChild);
			VDocument.SetMethod("Element InsertBefore(const Element &in, const Element &in)", &Engine::GUI::IElementDocument::InsertBefore);
			VDocument.SetMethod("Element ReplaceChild(const Element &in, const Element &in)", &Engine::GUI::IElementDocument::ReplaceChild);
			VDocument.SetMethod("Element RemoveChild(const Element &in)", &Engine::GUI::IElementDocument::RemoveChild);
			VDocument.SetMethod("bool HasChildNodes() const", &Engine::GUI::IElementDocument::HasChildNodes);
			VDocument.SetMethod("Element GetElementById(const String &in)", &Engine::GUI::IElementDocument::GetElementById);
			VDocument.SetMethodEx("Array<Element>@ QuerySelectorAll(const String &in)", &IElementDocumentQuerySelectorAll);
			VDocument.SetMethod("int GetClippingIgnoreDepth()", &Engine::GUI::IElementDocument::GetClippingIgnoreDepth);
			VDocument.SetMethod("bool IsClippingEnabled()", &Engine::GUI::IElementDocument::IsClippingEnabled);
			VDocument.SetMethod("bool CastFormColor(CU::Vector4 &out, bool)", &Engine::GUI::IElementDocument::CastFormColor);
			VDocument.SetMethod("bool CastFormString(String &out)", &Engine::GUI::IElementDocument::CastFormString);
			VDocument.SetMethod("bool CastFormPointer(Address@ &out)", &Engine::GUI::IElementDocument::CastFormPointer);
			VDocument.SetMethod("bool CastFormInt32(int &out)", &Engine::GUI::IElementDocument::CastFormInt32);
			VDocument.SetMethod("bool CastFormUInt32(uint &out)", &Engine::GUI::IElementDocument::CastFormUInt32);
			VDocument.SetMethod("bool CastFormFlag32(uint &out, uint)", &Engine::GUI::IElementDocument::CastFormFlag32);
			VDocument.SetMethod("bool CastFormInt64(int64 &out)", &Engine::GUI::IElementDocument::CastFormInt64);
			VDocument.SetMethod("bool CastFormUInt64(uint64 &out)", &Engine::GUI::IElementDocument::CastFormUInt64);
			VDocument.SetMethod("bool CastFormFlag64(uint64 &out, uint64)", &Engine::GUI::IElementDocument::CastFormFlag64);
			VDocument.SetMethod<Engine::GUI::IElement, bool, float*>("bool CastFormFloat(float &out)", &Engine::GUI::IElementDocument::CastFormFloat);
			VDocument.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool CastFormFloat(float &out, float)", &Engine::GUI::IElementDocument::CastFormFloat);
			VDocument.SetMethod("bool CastFormDouble(double &out)", &Engine::GUI::IElementDocument::CastFormDouble);
			VDocument.SetMethod("bool CastFormBoolean(bool &out)", &Engine::GUI::IElementDocument::CastFormBoolean);
			VDocument.SetMethod("String GetFormName() const", &Engine::GUI::IElementDocument::GetFormName);
			VDocument.SetMethod("void SetFormName(const String &in)", &Engine::GUI::IElementDocument::SetFormName);
			VDocument.SetMethod("String GetFormValue() const", &Engine::GUI::IElementDocument::GetFormValue);
			VDocument.SetMethod("void SetFormValue(const String &in)", &Engine::GUI::IElementDocument::SetFormValue);
			VDocument.SetMethod("bool IsFormDisabled() const", &Engine::GUI::IElementDocument::IsFormDisabled);
			VDocument.SetMethod("void SetFormDisabled(bool)", &Engine::GUI::IElementDocument::SetFormDisabled);
			VDocument.SetMethod("Address@ GetElement() const", &Engine::GUI::IElementDocument::GetElement);
			VDocument.SetMethod("bool IsValid() const", &Engine::GUI::IElementDocument::GetElement);
			VDocument.SetMethod("void SetTitle(const String &in)", &Engine::GUI::IElementDocument::SetTitle);
			VDocument.SetMethod("void PullToFront()", &Engine::GUI::IElementDocument::PullToFront);
			VDocument.SetMethod("void PushToBack()", &Engine::GUI::IElementDocument::PushToBack);
			VDocument.SetMethod("void Show(ModalFlag = ModalFlag::None, FocusFlag = FocusFlag::Auto)", &Engine::GUI::IElementDocument::Show);
			VDocument.SetMethod("void Hide()", &Engine::GUI::IElementDocument::Hide);
			VDocument.SetMethod("void Close()", &Engine::GUI::IElementDocument::Close);
			VDocument.SetMethod("String GetTitle() const", &Engine::GUI::IElementDocument::GetTitle);
			VDocument.SetMethod("String GetSourceURL() const", &Engine::GUI::IElementDocument::GetSourceURL);
			VDocument.SetMethod("Element CreateElement(const String &in)", &Engine::GUI::IElementDocument::CreateElement);
			VDocument.SetMethod("bool IsModal() const", &Engine::GUI::IElementDocument::IsModal);;
			VDocument.SetMethod("Address@ GetElementDocument() const", &Engine::GUI::IElementDocument::GetElementDocument);
			Engine->EndNamespace();

			return true;
#else
			return false;
#endif
		}
		bool GUIRegisterModel(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Register.SetFunctionDef("void GUIDataCallback(GUI::Event &in, Array<CE::Variant>@+)");

			Engine->BeginNamespace("GUI");
			VMRefClass VModel = Register.SetClassUnmanaged<Engine::GUI::DataModel>("Model");
			VModel.SetMethodEx("bool Set(const String &in, CE::Schema@+)", &DataModelSet);
			VModel.SetMethodEx("bool SetVar(const String &in, const CE::Variant &in)", &DataModelSetVar);
			VModel.SetMethodEx("bool SetString(const String &in, const String &in)", &DataModelSetString);
			VModel.SetMethodEx("bool SetInteger(const String &in, int64)", &DataModelSetInteger);
			VModel.SetMethodEx("bool SetFloat(const String &in, float)", &DataModelSetFloat);
			VModel.SetMethodEx("bool SetDouble(const String &in, double)", &DataModelSetDouble);
			VModel.SetMethodEx("bool SetBoolean(const String &in, bool)", &DataModelSetBoolean);
			VModel.SetMethodEx("bool SetPointer(const String &in, Address@)", &DataModelSetPointer);
			VModel.SetMethodEx("bool SetCallback(const String &in, GUIDataCallback@)", &DataModelSetCallback);
			VModel.SetMethodEx("CE::Schema@+ Get(const String &in)", &DataModelGet);
			VModel.SetMethod("String GetString(const String &in)", &Engine::GUI::DataModel::GetString);
			VModel.SetMethod("int64 GetInteger(const String &in)", &Engine::GUI::DataModel::GetInteger);
			VModel.SetMethod("float GetFloat(const String &in)", &Engine::GUI::DataModel::GetFloat);
			VModel.SetMethod("double GetDouble(const String &in)", &Engine::GUI::DataModel::GetDouble);
			VModel.SetMethod("bool GetBoolean(const String &in)", &Engine::GUI::DataModel::GetBoolean);
			VModel.SetMethod("Address@ GetPointer(const String &in)", &Engine::GUI::DataModel::GetPointer);
			VModel.SetMethod("bool HasChanged(const String &in)", &Engine::GUI::DataModel::HasChanged);
			VModel.SetMethod("void Change(const String &in)", &Engine::GUI::DataModel::Change);
			VModel.SetMethod("bool IsValid() const", &Engine::GUI::DataModel::IsValid);
			Engine->EndNamespace();

			return true;
#else
			return false;
#endif
		}
		bool GUIRegisterContext(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("GUI");
			VMRefClass VContext = Register.SetClassUnmanaged<Engine::GUI::Context>("Context");
			VContext.SetMethod("bool IsInputFocused()", &Engine::GUI::Context::IsInputFocused);
			VContext.SetMethod("Address@ GetContext()", &Engine::GUI::Context::GetContext);
			VContext.SetMethod("CU::Vector2 GetDimensions() const", &Engine::GUI::Context::GetDimensions);
			VContext.SetMethod("void SetDensityIndependentPixelRatio(float)", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
			VContext.SetMethod("float GetDensityIndependentPixelRatio() const", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
			VContext.SetMethod("void EnableMouseCursor(bool)", &Engine::GUI::Context::EnableMouseCursor);
			VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, const std::string&>("Schema GetDocument(const String &in)", &Engine::GUI::Context::GetDocument);
			VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, int>("Schema GetDocument(int)", &Engine::GUI::Context::GetDocument);
			VContext.SetMethod("int GetNumDocuments() const", &Engine::GUI::Context::GetNumDocuments);
			VContext.SetMethod("Element GetElementById(int, const String &in)", &Engine::GUI::Context::GetElementById);
			VContext.SetMethod("Element GetHoverElement()", &Engine::GUI::Context::GetHoverElement);
			VContext.SetMethod("Element GetFocusElement()", &Engine::GUI::Context::GetFocusElement);
			VContext.SetMethod("Element GetRootElement()", &Engine::GUI::Context::GetRootElement);
			VContext.SetMethodEx("Element GetElementAtPoint(const CU::Vector2 &in)", &ContextGetFocusElement);
			VContext.SetMethod("void PullDocumentToFront(const Schema &in)", &Engine::GUI::Context::PullDocumentToFront);
			VContext.SetMethod("void PushDocumentToBack(const Schema &in)", &Engine::GUI::Context::PushDocumentToBack);
			VContext.SetMethod("void UnfocusDocument(const Schema &in)", &Engine::GUI::Context::UnfocusDocument);
			VContext.SetMethod("void AddEventListener(const String &in, Listener@+, bool = false)", &Engine::GUI::Context::AddEventListener);
			VContext.SetMethod("void RemoveEventListener(const String &in, Listener@+, bool = false)", &Engine::GUI::Context::RemoveEventListener);
			VContext.SetMethod("bool IsMouseInteracting()", &Engine::GUI::Context::IsMouseInteracting);
			VContext.SetMethod("bool GetActiveClipRegion(CU::Vector2 &out, CU::Vector2 &out)", &Engine::GUI::Context::GetActiveClipRegion);
			VContext.SetMethod("void SetActiveClipRegion(const CU::Vector2 &in, const CU::Vector2 &in)", &Engine::GUI::Context::SetActiveClipRegion);
			VContext.SetMethod("Model@ SetModel(const String &in)", &Engine::GUI::Context::SetDataModel);
			VContext.SetMethod("Model@ GetModel(const String &in)", &Engine::GUI::Context::GetDataModel);
			Engine->EndNamespace();

			return true;
#else
			return false;
#endif
		}
	}
}