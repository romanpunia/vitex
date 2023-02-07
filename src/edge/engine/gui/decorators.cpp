#include "../gui.h"
#ifdef ED_USE_RMLUI
#include <RmlUi/Core.h>
#include <Source/Core/TransformState.h>

namespace Edge
{
	namespace Engine
	{
		namespace GUI
		{
			static void SetWorldViewProjection(Graphics::GraphicsDevice* Device, Rml::Element* Element, const Rml::Vector2f& Position, const Rml::Vector2f& Size, const Compute::Vector2& Mul = 1.0f)
			{
				ED_ASSERT_V(Device != nullptr, "graphics device should be set");
				ED_ASSERT_V(Element != nullptr, "element should be set");

				Compute::Vector3 Scale(Size.x / 2.0f, Size.y / 2.0f);
				Compute::Vector3 Offset(Position.x + Scale.X, Position.y + Scale.Y);
				Compute::Matrix4x4& Ortho = *Subsystem::GetProjection();

				const Rml::TransformState* State = Element->GetTransformState();
				if (State != nullptr && State->GetTransform() != nullptr)
				{
					Compute::Matrix4x4 View = Subsystem::ToMatrix(State->GetTransform());
					Device->Render.Transform = Compute::Matrix4x4::CreateTranslatedScale(Offset, Scale + Mul) * View * Ortho;
					Device->Render.World = (Compute::Matrix4x4::CreateTranslation(Compute::Vector2(Position.x, Position.y)) * View * Ortho).Inv();
				}
				else
				{
					Device->Render.Transform = Compute::Matrix4x4::CreateTranslatedScale(Offset, Scale + Mul) * Ortho;
					Device->Render.World = (Compute::Matrix4x4::CreateTranslation(Compute::Vector2(Position.x, Position.y)) * Ortho).Inv();
				}
			}

			class BoxShadowInstancer : public Rml::DecoratorInstancer
			{
			public:
				struct RenderConstant
				{
					Compute::Vector4 Color;
					Compute::Vector4 Radius;
					Compute::Vector2 Size;
					Compute::Vector2 Position;
					Compute::Vector2 Offset;
					float Softness = 0.0f;
					float Padding = 0.0f;
				} RenderPass;

			public:
				Rml::PropertyId Color;
				Rml::PropertyId OffsetX;
				Rml::PropertyId OffsetY;
				Rml::PropertyId Softness;

			public:
				Graphics::ElementBuffer* VertexBuffer;
				Graphics::Shader* Shader;
				Graphics::GraphicsDevice* Device;

			public:
				BoxShadowInstancer(Graphics::GraphicsDevice* NewDevice);
				virtual ~BoxShadowInstancer() override;
				virtual Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& Name, const Rml::PropertyDictionary& Props, const Rml::DecoratorInstancerInterface& Interface) override;
			}*IBoxShadow = nullptr;

			class BoxBlurInstancer : public Rml::DecoratorInstancer
			{
			public:
				struct RenderConstant
				{
					Compute::Vector4 Color;
					Compute::Vector4 Radius;
					Compute::Vector2 Texel;
					Compute::Vector2 Size;
					Compute::Vector2 Position;
					float Softness = 0.0f;
					float Alpha = 1.0f;
				} RenderPass;

			public:
				Rml::PropertyId Color;
				Rml::PropertyId Softness;

			public:
				Graphics::Texture2D* Background;
				Graphics::ElementBuffer* VertexBuffer;
				Graphics::Shader* Shader;
				Graphics::GraphicsDevice* Device;

			public:
				BoxBlurInstancer(Graphics::GraphicsDevice* NewDevice);
				virtual ~BoxBlurInstancer() override;
				virtual Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& Name, const Rml::PropertyDictionary& Props, const Rml::DecoratorInstancerInterface& Interface) override;
			}*IBoxBlur = nullptr;

			class BoxShadow : public Rml::Decorator
			{
			private:
				Compute::Vector4 Color;
				Compute::Vector2 Offset;
				float Softness;

			public:
				BoxShadow(const Compute::Vector4& NewColor, const Compute::Vector2& NewOffset, float NewSoftness) : Color(NewColor.Div(255.0f)), Offset(NewOffset), Softness(NewSoftness)
				{
				}
				virtual ~BoxShadow() override
				{
				}
				virtual Rml::DecoratorDataHandle GenerateElementData(Rml::Element* Element) const override
				{
					return Rml::Decorator::INVALID_DECORATORDATAHANDLE;
				}
				virtual void ReleaseElementData(Rml::DecoratorDataHandle ElementData) const override
				{
				}
				virtual void RenderElement(Rml::Element* Element, Rml::DecoratorDataHandle ElementData) const override
				{
					ED_ASSERT_V(Element != nullptr, "element should be set");
					Rml::Vector2f Position = Element->GetAbsoluteOffset(Rml::Box::PADDING).Round();
					Rml::Vector2f Size = Element->GetBox().GetSize(Rml::Box::PADDING).Round();
					float Alpha = Element->GetProperty<float>("opacity");
					float Radius = Softness * 0.85f;

					Graphics::GraphicsDevice* Device = IBoxShadow->Device;
					IBoxShadow->RenderPass.Position = Compute::Vector2(Position.x, Position.y);
					IBoxShadow->RenderPass.Size = Compute::Vector2(Size.x, Size.y);
					IBoxShadow->RenderPass.Color = Color;
					IBoxShadow->RenderPass.Color.W *= Alpha;
					IBoxShadow->RenderPass.Offset = Offset;
					IBoxShadow->RenderPass.Softness = Softness;
					IBoxShadow->RenderPass.Radius.X = Element->GetProperty<float>("border-bottom-left-radius");
					IBoxShadow->RenderPass.Radius.Y = Element->GetProperty<float>("border-bottom-right-radius");
					IBoxShadow->RenderPass.Radius.Z = Element->GetProperty<float>("border-top-right-radius");
					IBoxShadow->RenderPass.Radius.W = Element->GetProperty<float>("border-top-left-radius");

					SetWorldViewProjection(Device, Element, Position, Size, Offset.Abs() + Radius + 4096.0f);
					Device->SetShader(IBoxShadow->Shader, ED_VS | ED_PS);
					Device->SetBuffer(IBoxShadow->Shader, 3, ED_PS);
					Device->SetVertexBuffer(IBoxShadow->VertexBuffer);
					Device->UpdateBuffer(IBoxShadow->Shader, &IBoxShadow->RenderPass);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw((unsigned int)IBoxShadow->VertexBuffer->GetElements(), 0);
				}
			};

			class BoxBlur : public Rml::Decorator
			{
			private:
				Compute::Vector4 Color;
				float Softness;

			public:
				BoxBlur(const Compute::Vector4& NewColor, float NewSoftness) : Color(NewColor.Div(255)), Softness(NewSoftness)
				{
				}
				virtual ~BoxBlur() override
				{
				}
				virtual Rml::DecoratorDataHandle GenerateElementData(Rml::Element* Element) const override
				{
					return Rml::Decorator::INVALID_DECORATORDATAHANDLE;
				}
				virtual void ReleaseElementData(Rml::DecoratorDataHandle ElementData) const override
				{
				}
				virtual void RenderElement(Rml::Element* Element, Rml::DecoratorDataHandle ElementData) const override
				{
					ED_ASSERT_V(Element != nullptr, "element should be set");
					Graphics::Texture2D* Background = Subsystem::GetBackground();
					if (!Background)
						return;

					Rml::Vector2i Screen = Element->GetContext()->GetDimensions();
					Rml::Vector2f Position = Element->GetAbsoluteOffset(Rml::Box::PADDING).Round();
					Rml::Vector2f Size = Element->GetBox().GetSize(Rml::Box::PADDING).Round();
					float Alpha = Element->GetProperty<float>("opacity");

					Graphics::GraphicsDevice* Device = IBoxBlur->Device;
					IBoxBlur->RenderPass.Color = Color;
					IBoxBlur->RenderPass.Texel = Compute::Vector2((float)Screen.x, (float)Screen.y);
					IBoxBlur->RenderPass.Position = Compute::Vector2(Position.x, Position.y);
					IBoxBlur->RenderPass.Size = Compute::Vector2(Size.x, Size.y);
					IBoxBlur->RenderPass.Softness = Softness;
					IBoxBlur->RenderPass.Alpha = Alpha;
					IBoxBlur->RenderPass.Radius.X = Element->GetProperty<float>("border-bottom-left-radius");
					IBoxBlur->RenderPass.Radius.Y = Element->GetProperty<float>("border-bottom-right-radius");
					IBoxBlur->RenderPass.Radius.Z = Element->GetProperty<float>("border-top-right-radius");
					IBoxBlur->RenderPass.Radius.W = Element->GetProperty<float>("border-top-left-radius");

					SetWorldViewProjection(Device, Element, Position, Size);
					Device->CopyTexture2D(Background, &IBoxBlur->Background);
					Device->SetTexture2D(IBoxBlur->Background, 1, ED_PS);
					Device->SetShader(IBoxBlur->Shader, ED_VS | ED_PS);
					Device->SetBuffer(IBoxBlur->Shader, 3, ED_PS);
					Device->SetVertexBuffer(IBoxBlur->VertexBuffer);
					Device->UpdateBuffer(IBoxBlur->Shader, &IBoxBlur->RenderPass);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw((unsigned int)IBoxBlur->VertexBuffer->GetElements(), 0);
				}
			};

			BoxShadowInstancer::BoxShadowInstancer(Graphics::GraphicsDevice* NewDevice) : Shader(nullptr), Device(NewDevice)
			{
				ED_ASSERT_V(Device != nullptr, "graphics device should be set");
				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (Device->GetSection("interface/box-shadow", &I))
				{
					Shader = Device->CreateShader(I);
					if (Shader != nullptr)
						Device->UpdateBufferSize(Shader, sizeof(RenderPass));
				}

				Rml::Vertex Elements[6];
				Elements[0] = { Rml::Vector2f(-1.0f, -1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(-1, 0) };
				Elements[1] = { Rml::Vector2f(-1.0f, 1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(-1, 1) };
				Elements[2] = { Rml::Vector2f(1.0f, 1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(0, 1) };
				Elements[3] = { Rml::Vector2f(-1.0f, -1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(-1, 0) };
				Elements[4] = { Rml::Vector2f(1.0f, 1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(0, 1) };
				Elements[5] = { Rml::Vector2f(1.0f, -1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(0, 0) };

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
				F.ElementCount = 6;
				F.ElementWidth = sizeof(Rml::Vertex);
				F.Elements = &Elements[0];

				VertexBuffer = Device->CreateElementBuffer(F);

				Color = RegisterProperty("color", "#000").AddParser("color").GetId();
				Softness = RegisterProperty("softness", "60").AddParser("number").GetId();
				OffsetX = RegisterProperty("x", "0").AddParser("number").GetId();
				OffsetY = RegisterProperty("y", "10").AddParser("number").GetId();
				RegisterShorthand("decorator", "x, y, softness, color", Rml::ShorthandType::FallThrough);
			}
			BoxShadowInstancer::~BoxShadowInstancer()
			{
				ED_RELEASE(Shader);
				ED_RELEASE(VertexBuffer);
			}
			Rml::SharedPtr<Rml::Decorator> BoxShadowInstancer::InstanceDecorator(const Rml::String& Name, const Rml::PropertyDictionary& Props, const Rml::DecoratorInstancerInterface& Interface)
			{
				const Rml::Property* SColor = Props.GetProperty(Color);
				const Rml::Property* SSoftness = Props.GetProperty(Softness);
				const Rml::Property* SOffsetX = Props.GetProperty(OffsetX);
				const Rml::Property* SOffsetY = Props.GetProperty(OffsetY);

				Rml::Colourb IColor = SColor->Get<Rml::Colourb>();
				float ISoftness = SSoftness->Get<float>();
				float IOffsetX = SOffsetX->Get<float>();
				float IOffsetY = SOffsetY->Get<float>();

				return Rml::MakeShared<BoxShadow>(
					Compute::Vector4(IColor.red, IColor.green, IColor.blue, IColor.alpha),
					Compute::Vector2(IOffsetX, IOffsetY), ISoftness);
			}

			BoxBlurInstancer::BoxBlurInstancer(Graphics::GraphicsDevice* NewDevice) : Background(nullptr), Shader(nullptr), Device(NewDevice)
			{
				ED_ASSERT_V(Device != nullptr, "graphics device should be set");
				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (Device->GetSection("interface/box-blur", &I))
				{
					Shader = Device->CreateShader(I);
					if (Shader != nullptr)
						Device->UpdateBufferSize(Shader, sizeof(RenderPass));
				}

				Rml::Vertex Elements[6];
				Elements[0] = { Rml::Vector2f(-1.0f, -1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(-1, 0) };
				Elements[1] = { Rml::Vector2f(-1.0f, 1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(-1, 1) };
				Elements[2] = { Rml::Vector2f(1.0f, 1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(0, 1) };
				Elements[3] = { Rml::Vector2f(-1.0f, -1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(-1, 0) };
				Elements[4] = { Rml::Vector2f(1.0f, 1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(0, 1) };
				Elements[5] = { Rml::Vector2f(1.0f, -1.0f), Rml::Colourb(0, 0, 0, 0), Rml::Vector2f(0, 0) };

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
				F.ElementCount = 6;
				F.ElementWidth = sizeof(Rml::Vertex);
				F.Elements = &Elements[0];

				VertexBuffer = Device->CreateElementBuffer(F);

				Color = RegisterProperty("color", "#fff").AddParser("color").GetId();
				Softness = RegisterProperty("softness", "8").AddParser("number").GetId();
				RegisterShorthand("decorator", "softness, color", Rml::ShorthandType::FallThrough);
			}
			BoxBlurInstancer::~BoxBlurInstancer()
			{
				ED_RELEASE(Shader);
				ED_RELEASE(VertexBuffer);
				ED_RELEASE(Background);
			}
			Rml::SharedPtr<Rml::Decorator> BoxBlurInstancer::InstanceDecorator(const Rml::String& Name, const Rml::PropertyDictionary& Props, const Rml::DecoratorInstancerInterface& Interface)
			{
				const Rml::Property* SColor = Props.GetProperty(Color);
				const Rml::Property* SSoftness = Props.GetProperty(Softness);

				Rml::Colourb IColor = SColor->Get<Rml::Colourb>();
				float ISoftness = SSoftness->Get<float>();

				return Rml::MakeShared<BoxBlur>(Compute::Vector4(IColor.red, IColor.green, IColor.blue, IColor.alpha), ISoftness);
			}

			void Subsystem::ResizeDecorators(int Width, int Height)
			{
				if (IBoxBlur != nullptr)
					ED_CLEAR(IBoxBlur->Background);
			}
			void Subsystem::CreateDecorators(Graphics::GraphicsDevice* Device)
			{
				ED_ASSERT_V(Device != nullptr, "graphics device should be set");
				if (!IBoxShadow)
				{
					IBoxShadow = ED_NEW(BoxShadowInstancer, Device);
					Rml::Factory::RegisterDecoratorInstancer("box-shadow", IBoxShadow);
				}

				if (!IBoxBlur)
				{
					IBoxBlur = ED_NEW(BoxBlurInstancer, Device);
					Rml::Factory::RegisterDecoratorInstancer("box-blur", IBoxBlur);
				}
			}
			void Subsystem::ReleaseDecorators()
			{
				ED_DELETE(BoxShadowInstancer, IBoxShadow);
				IBoxShadow = nullptr;

				ED_DELETE(BoxBlurInstancer, IBoxBlur);
				IBoxBlur = nullptr;
			}
		}
	}
}
#endif
