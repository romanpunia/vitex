namespace GUI
{
	enum Position
	{
		Static = 0,
		Relative = 1,
		Absolute = 2,
		Fixed = 3
	}

	enum EventPhase
	{
		None = 0,
		Capture = 1,
		Target = 2,
		Bubble = 4
	}

	enum Area
	{
		Margin = 0,
		Border = 1,
		Padding = 2,
		Content = 3
	}

	enum FocusFlag
	{
		None = 0,
		Document = 1,
		Keep = 2,
		Auto = 3
	}

	enum Float
	{
		None = 0,
		Left = 1,
		Right = 2
	}

	enum TimingFunc
	{
		None = 0,
		Back = 1,
		Bounce = 2,
		Circular = 3,
		Cubic = 4,
		Elastic = 5,
		Exponential = 6,
		Linear = 7,
		Quadratic = 8,
		Quartic = 9,
		Sine = 11,
		Callback = 12
	}

	enum Display
	{
		None = 0,
		Block = 1,
		Inline = 2,
		InlineBlock = 3,
		Table = 4,
		TableRow = 5,
		TableRowGroup = 6,
		TableColumn = 7,
		TableColumnGroup = 8,
		TableCell = 9
	}

	enum TimingDir
	{
		In = 1,
		Out = 2,
		InOut = 3
	}

	enum ModalFlag
	{
		None = 0,
		Modal = 1,
		Keep = 2
	}

	class Event
	{
		GUI::Event& opAssign(const GUI::Event&in);
		GUI::EventPhase GetPhase() const;
		void SetPhase(GUI::EventPhase Phase);
		void SetCurrentElement(const GUI::Element&in);
		GUI::Element GetCurrentElement() const;
		GUI::Element GetTargetElement() const;
		String GetType() const;
		void StopPropagation();
		void StopImmediatePropagation();
		bool IsInterruptible() const;
		bool IsPropagating() const;
		bool IsImmediatePropagating() const;
		bool GetBoolean(const String&in) const;
		int64 GetInteger(const String&in) const;
		double GetNumber(const String&in) const;
		String GetString(const String&in) const;
		CU::Vector2 GetVector2(const String&in) const;
		CU::Vector3 GetVector3(const String&in) const;
		CU::Vector4 GetVector4(const String&in) const;
		Address@ GetPointer(const String&in) const;
		Address@ GetEvent() const;
		bool IsValid() const;
	}

	class Element
	{
		GUI::Element& opAssign(const GUI::Element&in);
		GUI::Element Clone() const;
		void SetClass(const String&in, bool);
		bool IsClassSet(const String&in) const;
		void SetClassNames(const String&in);
		String GetClassNames() const;
		String GetAddress(bool = false, bool = true) const;
		void SetOffset(const CU::Vector2&in, const GUI::Element&in, bool = false);
		CU::Vector2 GetRelativeOffset(GUI::Area = Area :: Content) const;
		CU::Vector2 GetAbsoluteOffset(GUI::Area = Area :: Content) const;
		void SetClientArea(GUI::Area);
		GUI::Area GetClientArea() const;
		void SetContentBox(const CU::Vector2&in, const CU::Vector2&in);
		float GetBaseline() const;
		bool GetIntrinsicDimensions(CU::Vector2&out, float&out);
		bool IsPointWithinElement(const CU::Vector2&in);
		bool IsVisible() const;
		float GetZIndex() const;
		bool SetProperty(const String&in, const String&in);
		void RemoveProperty(const String&in);
		String GetProperty(const String&in) const;
		String GetLocalProperty(const String&in) const;
		float ResolveNumericProperty(const String&in) const;
		CU::Vector2 GetContainingBlock() const;
		GUI::Position GetPosition() const;
		GUI::Float GetFloat() const;
		GUI::Display GetDisplay() const;
		float GetLineHeight() const;
		bool Project(CU::Vector2&out) const;
		bool Animate(const String&in, const String&in, float, GUI::TimingFunc, GUI::TimingDir, int = - 1, bool = true, float = 0);
		bool AddAnimationKey(const String&in, const String&in, float, GUI::TimingFunc, GUI::TimingDir);
		void SetPseudoClass(const String&in, bool);
		bool IsPseudoClassSet(const String&in) const;
		void SetAttribute(const String&in, const String&in);
		String GetAttribute(const String&in) const;
		bool HasAttribute(const String&in) const;
		void RemoveAttribute(const String&in);
		GUI::Element GetFocusLeafNode();
		String GetTagName() const;
		String GetId() const;
		float GetAbsoluteLeft();
		float GetAbsoluteTop();
		float GetClientLeft();
		float GetClientTop();
		float GetClientWidth();
		float GetClientHeight();
		GUI::Element GetOffsetParent();
		float GetOffsetLeft();
		float GetOffsetTop();
		float GetOffsetWidth();
		float GetOffsetHeight();
		float GetScrollLeft();
		void SetScrollLeft(float);
		float GetScrollTop();
		void SetScrollTop(float);
		float GetScrollWidth();
		float GetScrollHeight();
		GUI::Document GetOwnerDocument() const;
		GUI::Element GetParentNode() const;
		GUI::Element GetNextSibling() const;
		GUI::Element GetPreviousSibling() const;
		GUI::Element GetFirstChild() const;
		GUI::Element GetLastChild() const;
		GUI::Element GetChild(int) const;
		int GetNumChildren(bool = false) const;
		void GetInnerHTML(String&out) const;
		String GetInnerHTML() const;
		void SetInnerHTML(const String&in);
		bool IsFocused();
		bool IsHovered();
		bool IsActive();
		bool IsChecked();
		bool Focus();
		void Blur();
		void Click();
		void AddEventListener(const String&in, GUI::Listener@, bool = false);
		void RemoveEventListener(const String&in, GUI::Listener@, bool = false);
		bool DispatchEvent(const String&in, CE::Document@);
		void ScrollIntoView(bool = true);
		GUI::Element AppendChild(const GUI::Element&in, bool = true);
		GUI::Element InsertBefore(const GUI::Element&in, const GUI::Element&in);
		GUI::Element ReplaceChild(const GUI::Element&in, const GUI::Element&in);
		GUI::Element RemoveChild(const GUI::Element&in);
		bool HasChildNodes() const;
		GUI::Element GetElementById(const String&in);
		GUI::Element[]@ QuerySelectorAll(const String&in);
		int GetClippingIgnoreDepth();
		bool IsClippingEnabled();
		bool CastFormColor(CU::Vector4&out, bool);
		bool CastFormString(String&out);
		bool CastFormPointer(Address@&out);
		bool CastFormInt32(int&out);
		bool CastFormUInt32(uint&out);
		bool CastFormFlag32(uint&out, uint);
		bool CastFormInt64(int64&out);
		bool CastFormUInt64(uint64&out);
		bool CastFormFlag64(uint64&out, uint64);
		bool CastFormFloat(float&out);
		bool CastFormFloat(float&out, float);
		bool CastFormDouble(double&out);
		bool CastFormBoolean(bool&out);
		String GetFormName() const;
		void SetFormName(const String&in);
		String GetFormValue() const;
		void SetFormValue(const String&in);
		bool IsFormDisabled() const;
		void SetFormDisabled(bool);
		Address@ GetElement() const;
		bool IsValid() const;
	}

	class Context
	{
		bool IsInputFocused();
		Address@ GetContext();
		CU::Vector2 GetDimensions() const;
		void SetDensityIndependentPixelRatio(float);
		float GetDensityIndependentPixelRatio() const;
		void EnableMouseCursor(bool);
		GUI::Document GetDocument(const String&in);
		GUI::Document GetDocument(int);
		int GetNumDocuments() const;
		GUI::Element GetElementById(int, const String&in);
		GUI::Element GetHoverElement();
		GUI::Element GetFocusElement();
		GUI::Element GetRootElement();
		GUI::Element GetElementAtPoint(const CU::Vector2&in);
		void PullDocumentToFront(const GUI::Document&in);
		void PushDocumentToBack(const GUI::Document&in);
		void UnfocusDocument(const GUI::Document&in);
		void AddEventListener(const String&in, GUI::Listener@, bool = false);
		void RemoveEventListener(const String&in, GUI::Listener@, bool = false);
		bool IsMouseInteracting();
		bool GetActiveClipRegion(CU::Vector2&out, CU::Vector2&out);
		void SetActiveClipRegion(const CU::Vector2&in, const CU::Vector2&in);
		GUI::Model@ SetModel(const String&in);
		GUI::Model@ GetModel(const String&in);
	}

	class Document
	{
		GUI::Document& opAssign(const GUI::Document&in);
		GUI::Element Clone() const;
		void SetClass(const String&in, bool);
		bool IsClassSet(const String&in) const;
		void SetClassNames(const String&in);
		String GetClassNames() const;
		String GetAddress(bool = false, bool = true) const;
		void SetOffset(const CU::Vector2&in, const GUI::Element&in, bool = false);
		CU::Vector2 GetRelativeOffset(GUI::Area = Area :: Content) const;
		CU::Vector2 GetAbsoluteOffset(GUI::Area = Area :: Content) const;
		void SetClientArea(GUI::Area);
		GUI::Area GetClientArea() const;
		void SetContentBox(const CU::Vector2&in, const CU::Vector2&in);
		float GetBaseline() const;
		bool GetIntrinsicDimensions(CU::Vector2&out, float&out);
		bool IsPointWithinElement(const CU::Vector2&in);
		bool IsVisible() const;
		float GetZIndex() const;
		bool SetProperty(const String&in, const String&in);
		void RemoveProperty(const String&in);
		String GetProperty(const String&in) const;
		String GetLocalProperty(const String&in) const;
		float ResolveNumericProperty(const String&in) const;
		CU::Vector2 GetContainingBlock() const;
		GUI::Position GetPosition() const;
		GUI::Float GetFloat() const;
		GUI::Display GetDisplay() const;
		float GetLineHeight() const;
		bool Project(CU::Vector2&out) const;
		bool Animate(const String&in, const String&in, float, GUI::TimingFunc, GUI::TimingDir, int = - 1, bool = true, float = 0);
		bool AddAnimationKey(const String&in, const String&in, float, GUI::TimingFunc, GUI::TimingDir);
		void SetPseudoClass(const String&in, bool);
		bool IsPseudoClassSet(const String&in) const;
		void SetAttribute(const String&in, const String&in);
		String GetAttribute(const String&in) const;
		bool HasAttribute(const String&in) const;
		void RemoveAttribute(const String&in);
		GUI::Element GetFocusLeafNode();
		String GetTagName() const;
		String GetId() const;
		float GetAbsoluteLeft();
		float GetAbsoluteTop();
		float GetClientLeft();
		float GetClientTop();
		float GetClientWidth();
		float GetClientHeight();
		GUI::Element GetOffsetParent();
		float GetOffsetLeft();
		float GetOffsetTop();
		float GetOffsetWidth();
		float GetOffsetHeight();
		float GetScrollLeft();
		void SetScrollLeft(float);
		float GetScrollTop();
		void SetScrollTop(float);
		float GetScrollWidth();
		float GetScrollHeight();
		GUI::Document GetOwnerDocument() const;
		GUI::Element GetParentNode() const;
		GUI::Element GetNextSibling() const;
		GUI::Element GetPreviousSibling() const;
		GUI::Element GetFirstChild() const;
		GUI::Element GetLastChild() const;
		GUI::Element GetChild(int) const;
		int GetNumChildren(bool = false) const;
		void GetInnerHTML(String&out) const;
		String GetInnerHTML() const;
		void SetInnerHTML(const String&in);
		bool IsFocused();
		bool IsHovered();
		bool IsActive();
		bool IsChecked();
		bool Focus();
		void Blur();
		void Click();
		void AddEventListener(const String&in, GUI::Listener@, bool = false);
		void RemoveEventListener(const String&in, GUI::Listener@, bool = false);
		bool DispatchEvent(const String&in, CE::Document@);
		void ScrollIntoView(bool = true);
		GUI::Element AppendChild(const GUI::Element&in, bool = true);
		GUI::Element InsertBefore(const GUI::Element&in, const GUI::Element&in);
		GUI::Element ReplaceChild(const GUI::Element&in, const GUI::Element&in);
		GUI::Element RemoveChild(const GUI::Element&in);
		bool HasChildNodes() const;
		GUI::Element GetElementById(const String&in);
		GUI::Element[]@ QuerySelectorAll(const String&in);
		int GetClippingIgnoreDepth();
		bool IsClippingEnabled();
		bool CastFormColor(CU::Vector4&out, bool);
		bool CastFormString(String&out);
		bool CastFormPointer(Address@&out);
		bool CastFormInt32(int&out);
		bool CastFormUInt32(uint&out);
		bool CastFormFlag32(uint&out, uint);
		bool CastFormInt64(int64&out);
		bool CastFormUInt64(uint64&out);
		bool CastFormFlag64(uint64&out, uint64);
		bool CastFormFloat(float&out);
		bool CastFormFloat(float&out, float);
		bool CastFormDouble(double&out);
		bool CastFormBoolean(bool&out);
		String GetFormName() const;
		void SetFormName(const String&in);
		String GetFormValue() const;
		void SetFormValue(const String&in);
		bool IsFormDisabled() const;
		void SetFormDisabled(bool);
		Address@ GetElement() const;
		bool IsValid() const;
		void SetTitle(const String&in);
		void PullToFront();
		void PushToBack();
		void Show(GUI::ModalFlag = ModalFlag :: None, GUI::FocusFlag = FocusFlag :: Auto);
		void Hide();
		void Close();
		String GetTitle() const;
		String GetSourceURL() const;
		GUI::Element CreateElement(const String&in);
		bool IsModal() const;
		Address@ GetElementDocument() const;
	}

	class Listener
	{
		GUI::Listener@ Listener(GUI::GUIListenerCallback@);
		GUI::Listener@ Listener(const String&in);
	}

	class Model
	{
		bool Set(const String&in, CE::Document@);
		bool SetVar(const String&in, const CE::Variant&in);
		bool SetString(const String&in, const String&in);
		bool SetInteger(const String&in, int64);
		bool SetFloat(const String&in, float);
		bool SetDouble(const String&in, double);
		bool SetBoolean(const String&in, bool);
		bool SetPointer(const String&in, Address@);
		bool SetCallback(const String&in, GUIDataCallback@);
		CE::Document@ Get(const String&in);
		String GetString(const String&in);
		int64 GetInteger(const String&in);
		float GetFloat(const String&in);
		double GetDouble(const String&in);
		bool GetBoolean(const String&in);
		Address@ GetPointer(const String&in);
		bool HasChanged(const String&in);
		void Change(const String&in);
		bool IsValid() const;
	}

	funcdef void GUIListenerCallback(Event&in);
}

namespace GUI::Variant
{
	CU::Vector4 ToColor4(const String&in);
	String FromColor4(const CU::Vector4&in, bool);
	CU::Vector4 ToColor3(const String&in);
	String FromColor3(const CU::Vector4&in, bool);
	int GetVectorType(const String&in);
	CU::Vector4 ToVector4(const String&in);
	String FromVector4(const CU::Vector4&in);
	CU::Vector3 ToVector3(const String&in);
	String FromVector3(const CU::Vector3&in);
	CU::Vector2 ToVector2(const String&in);
	String FromVector2(const CU::Vector2&in);
}