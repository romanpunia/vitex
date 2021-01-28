#include "engine.h"
#include "../engine/components.h"
#include "../engine/processors.h"
#include "../engine/renderers.h"
#include "../network/http.h"
#include "../audio/effects.h"
#include "../audio/filters.h"
#include <sstream>
#ifdef TH_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#undef Complex
#endif

namespace Tomahawk
{
	namespace Engine
	{
		Event::Event(const std::string& NewName, SceneGraph* Target, const Compute::PropertyArgs& NewArgs) : Id(NewName), Args(NewArgs), TScene(Target), TEntity(nullptr), TComponent(nullptr)
		{
		}
		Event::Event(const std::string& NewName, Entity* Target, const Compute::PropertyArgs& NewArgs) : Id(NewName), Args(NewArgs), TScene(nullptr), TEntity(Target), TComponent(nullptr)
		{
		}
		Event::Event(const std::string& NewName, Component* Target, const Compute::PropertyArgs& NewArgs) : Id(NewName), Args(NewArgs), TScene(nullptr), TEntity(nullptr), TComponent(Target)
		{
		}
		bool Event::Is(const std::string& Name)
		{
			return Id == Name;
		}
		const std::string& Event::GetName()
		{
			return Id;
		}
		Component* Event::GetComponent()
		{
			return TComponent;
		}
		Entity* Event::GetEntity()
		{
			if (TComponent != nullptr)
				return TComponent->GetEntity();

			return TEntity;
		}
		SceneGraph* Event::GetScene()
		{
			if (TEntity != nullptr)
				return TEntity->GetScene();

			if (TComponent != nullptr)
				return TComponent->GetEntity()->GetScene();

			return TScene;
		}

		Appearance::Appearance()
		{
		}
		Appearance::Appearance(const Appearance& Other)
		{
			if (Other.DiffuseMap != nullptr)
				DiffuseMap = (Graphics::Texture2D*)Other.DiffuseMap->AddRef();

			if (Other.NormalMap != nullptr)
				NormalMap = (Graphics::Texture2D*)Other.NormalMap->AddRef();

			if (Other.MetallicMap != nullptr)
				MetallicMap = (Graphics::Texture2D*)Other.MetallicMap->AddRef();

			if (Other.RoughnessMap != nullptr)
				RoughnessMap = (Graphics::Texture2D*)Other.RoughnessMap->AddRef();

			if (Other.HeightMap != nullptr)
				HeightMap = (Graphics::Texture2D*)Other.HeightMap->AddRef();

			if (Other.OcclusionMap != nullptr)
				OcclusionMap = (Graphics::Texture2D*)Other.OcclusionMap->AddRef();

			if (Other.EmissionMap != nullptr)
				EmissionMap = (Graphics::Texture2D*)Other.EmissionMap->AddRef();

			Diffuse = Other.Diffuse;
			TexCoord = Other.TexCoord;
			HeightAmount = Other.HeightAmount;
			HeightBias = Other.HeightBias;
			Material = Other.Material;
		}
		Appearance::~Appearance()
		{
			TH_RELEASE(DiffuseMap);
			TH_RELEASE(NormalMap);
			TH_RELEASE(MetallicMap);
			TH_RELEASE(RoughnessMap);
			TH_RELEASE(HeightMap);
			TH_RELEASE(OcclusionMap);
			TH_RELEASE(EmissionMap);
		}
		bool Appearance::FillGeometry(Graphics::GraphicsDevice* Device) const
		{
			if (!Device || Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(DiffuseMap != nullptr);
			Device->Render.HasNormal = (float)(NormalMap != nullptr);
			Device->Render.HasHeight = (float)(HeightMap != nullptr);
			Device->Render.HeightAmount = HeightAmount;
			Device->Render.HeightBias = HeightBias;
			Device->Render.MaterialId = (float)Material;
			Device->Render.Diffuse = Diffuse;
			Device->Render.TexCoord = TexCoord;
			Device->SetTexture2D(DiffuseMap, 1);
			Device->SetTexture2D(NormalMap, 2);
			Device->SetTexture2D(MetallicMap, 3);
			Device->SetTexture2D(RoughnessMap, 4);
			Device->SetTexture2D(HeightMap, 5);
			Device->SetTexture2D(OcclusionMap, 6);
			Device->SetTexture2D(EmissionMap, 7);

			return true;
		}
		bool Appearance::FillVoxels(Graphics::GraphicsDevice* Device) const
		{
			if (!Device || Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(DiffuseMap != nullptr);
			Device->Render.HasNormal = (float)(NormalMap != nullptr);
			Device->Render.MaterialId = (float)Material;
			Device->Render.Diffuse = Diffuse;
			Device->Render.TexCoord = TexCoord;
			Device->SetTexture2D(DiffuseMap, 5);
			Device->SetTexture2D(NormalMap, 6);
			Device->SetTexture2D(MetallicMap, 7);
			Device->SetTexture2D(RoughnessMap, 8);
			Device->SetTexture2D(OcclusionMap, 9);
			Device->SetTexture2D(EmissionMap, 10);

			return true;
		}
		bool Appearance::FillDepthLinear(Graphics::GraphicsDevice* Device) const
		{
			if (!Device || Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(DiffuseMap != nullptr);
			Device->Render.MaterialId = Material;
			Device->Render.TexCoord = TexCoord;
			Device->SetTexture2D(DiffuseMap, 1);

			return true;
		}
		bool Appearance::FillDepthCubic(Graphics::GraphicsDevice* Device) const
		{
			if (!Device || Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(DiffuseMap != nullptr);
			Device->Render.MaterialId = Material;
			Device->Render.TexCoord = TexCoord;
			Device->SetTexture2D(DiffuseMap, 1);

			return true;
		}
		void Appearance::SetDiffuseMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(DiffuseMap);
			DiffuseMap = New;
		}
		Graphics::Texture2D* Appearance::GetDiffuseMap() const
		{
			return DiffuseMap;
		}
		void Appearance::SetNormalMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(NormalMap);
			NormalMap = New;
		}
		Graphics::Texture2D* Appearance::GetNormalMap() const
		{
			return NormalMap;
		}
		void Appearance::SetMetallicMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(MetallicMap);
			MetallicMap = New;
		}
		Graphics::Texture2D* Appearance::GetMetallicMap() const
		{
			return MetallicMap;
		}
		void Appearance::SetRoughnessMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(RoughnessMap);
			RoughnessMap = New;
		}
		Graphics::Texture2D* Appearance::GetRoughnessMap() const
		{
			return RoughnessMap;
		}
		void Appearance::SetHeightMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(HeightMap);
			HeightMap = New;
		}
		Graphics::Texture2D* Appearance::GetHeightMap() const
		{
			return HeightMap;
		}
		void Appearance::SetOcclusionMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(OcclusionMap);
			OcclusionMap = New;
		}
		Graphics::Texture2D* Appearance::GetOcclusionMap() const
		{
			return OcclusionMap;
		}
		void Appearance::SetEmissionMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(EmissionMap);
			EmissionMap = New;
		}
		Graphics::Texture2D* Appearance::GetEmissionMap() const
		{
			return EmissionMap;
		}
		Appearance& Appearance::operator= (const Appearance& Other)
		{
			if (Other.DiffuseMap != nullptr)
				DiffuseMap = (Graphics::Texture2D*)Other.DiffuseMap->AddRef();

			if (Other.NormalMap != nullptr)
				NormalMap = (Graphics::Texture2D*)Other.NormalMap->AddRef();

			if (Other.MetallicMap != nullptr)
				MetallicMap = (Graphics::Texture2D*)Other.MetallicMap->AddRef();

			if (Other.RoughnessMap != nullptr)
				RoughnessMap = (Graphics::Texture2D*)Other.RoughnessMap->AddRef();

			if (Other.HeightMap != nullptr)
				HeightMap = (Graphics::Texture2D*)Other.HeightMap->AddRef();

			if (Other.OcclusionMap != nullptr)
				OcclusionMap = (Graphics::Texture2D*)Other.OcclusionMap->AddRef();

			if (Other.EmissionMap != nullptr)
				EmissionMap = (Graphics::Texture2D*)Other.EmissionMap->AddRef();

			Diffuse = Other.Diffuse;
			TexCoord = Other.TexCoord;
			HeightAmount = Other.HeightAmount;
			HeightBias = Other.HeightBias;
			Material = Other.Material;

			return *this;
		}

		bool NMake::Pack(Rest::Document* V, bool Value)
		{
			if (!V)
				return false;

			return V->SetBoolean("[b]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, int Value)
		{
			if (!V)
				return false;

			return V->SetInteger("[i]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, unsigned int Value)
		{
			if (!V)
				return false;

			return V->SetInteger("[i]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, float Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[n]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, double Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[n]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, int64_t Value)
		{
			if (!V)
				return false;

			return V->SetInteger("[i]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, long double Value)
		{
			if (!V)
				return false;

			return V->SetDecimal("[d]", std::to_string(Value)) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, uint64_t Value)
		{
			if (!V)
				return false;

			return V->SetInteger("[i]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const char* Value)
		{
			if (!V || !Value)
				return false;

			return V->SetString("[s]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::Vector2& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[x]", Value.X) != nullptr && V->SetNumber("[y]", Value.Y) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::Vector3& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[x]", Value.X) != nullptr && V->SetNumber("[y]", Value.Y) != nullptr && V->SetNumber("[z]", Value.Z) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::Vector4& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[x]", Value.X) != nullptr && V->SetNumber("[y]", Value.Y) != nullptr && V->SetNumber("[z]", Value.Z) != nullptr && V->SetNumber("[w]", Value.W) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::Matrix4x4& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[m11]", Value.Row[0]) != nullptr && V->SetNumber("[m12]", Value.Row[1]) != nullptr && V->SetNumber("[m13]", Value.Row[2]) != nullptr && V->SetNumber("[m14]", Value.Row[3]) != nullptr && V->SetNumber("[m21]", Value.Row[4]) != nullptr && V->SetNumber("[m22]", Value.Row[5]) != nullptr && V->SetNumber("[m23]", Value.Row[6]) != nullptr && V->SetNumber("[m24]", Value.Row[7]) != nullptr && V->SetNumber("[m31]", Value.Row[8]) != nullptr && V->SetNumber("[m32]", Value.Row[9]) != nullptr && V->SetNumber("[m33]", Value.Row[10]) != nullptr && V->SetNumber("[m34]", Value.Row[11]) != nullptr && V->SetNumber("[m41]", Value.Row[12]) != nullptr && V->SetNumber("[m42]", Value.Row[13]) != nullptr && V->SetNumber("[m43]", Value.Row[14]) != nullptr && V->SetNumber("[m44]", Value.Row[15]) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Attenuation& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("range"), Value.Range);
			NMake::Pack(V->SetDocument("c1"), Value.C1);
			NMake::Pack(V->SetDocument("c2"), Value.C2);
			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Material& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("emission"), Value.Emission);
			NMake::Pack(V->SetDocument("metallic"), Value.Metallic);
			NMake::Pack(V->SetDocument("scatter"), Value.Scatter);
			NMake::Pack(V->SetDocument("roughness"), Value.Roughness);
			NMake::Pack(V->SetDocument("fresnel"), Value.Fresnel);
			NMake::Pack(V->SetDocument("refraction"), Value.Refraction);
			NMake::Pack(V->SetDocument("transparency"), Value.Transparency);
			NMake::Pack(V->SetDocument("environment"), Value.Environment);
			NMake::Pack(V->SetDocument("occlusion"), Value.Occlusion);
			NMake::Pack(V->SetDocument("radius"), Value.Radius);
			NMake::Pack(V->SetDocument("id"), Value.Id);
			return true;
		}
		bool NMake::Pack(Rest::Document* V, const AnimatorState& Value)
		{
			if (!V)
				return false;

			return V->SetBoolean("[looped]", Value.Looped) != nullptr && V->SetBoolean("[paused]", Value.Paused) != nullptr && V->SetBoolean("[blended]", Value.Blended) != nullptr && V->SetInteger("[clip]", Value.Clip) != nullptr && V->SetInteger("[frame]", Value.Frame) != nullptr && V->SetNumber("[rate]", Value.Rate) != nullptr && V->SetNumber("[duration]", Value.Duration) != nullptr && V->SetNumber("[time]", Value.Time) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const SpawnerProperties& Value)
		{
			if (!V)
				return false;

			Rest::Document* Angular = V->SetDocument("angular");
			NMake::Pack(Angular->SetDocument("intensity"), Value.Angular.Intensity);
			NMake::Pack(Angular->SetDocument("accuracy"), Value.Angular.Accuracy);
			NMake::Pack(Angular->SetDocument("min"), Value.Angular.Min);
			NMake::Pack(Angular->SetDocument("max"), Value.Angular.Max);

			Rest::Document* Diffusion = V->SetDocument("diffusion");
			NMake::Pack(Diffusion->SetDocument("intensity"), Value.Diffusion.Intensity);
			NMake::Pack(Diffusion->SetDocument("accuracy"), Value.Diffusion.Accuracy);
			NMake::Pack(Diffusion->SetDocument("min"), Value.Diffusion.Min);
			NMake::Pack(Diffusion->SetDocument("max"), Value.Diffusion.Max);

			Rest::Document* Noise = V->SetDocument("noise");
			NMake::Pack(Noise->SetDocument("intensity"), Value.Noise.Intensity);
			NMake::Pack(Noise->SetDocument("accuracy"), Value.Noise.Accuracy);
			NMake::Pack(Noise->SetDocument("min"), Value.Noise.Min);
			NMake::Pack(Noise->SetDocument("max"), Value.Noise.Max);

			Rest::Document* Position = V->SetDocument("position");
			NMake::Pack(Position->SetDocument("intensity"), Value.Position.Intensity);
			NMake::Pack(Position->SetDocument("accuracy"), Value.Position.Accuracy);
			NMake::Pack(Position->SetDocument("min"), Value.Position.Min);
			NMake::Pack(Position->SetDocument("max"), Value.Position.Max);

			Rest::Document* Rotation = V->SetDocument("rotation");
			NMake::Pack(Rotation->SetDocument("intensity"), Value.Rotation.Intensity);
			NMake::Pack(Rotation->SetDocument("accuracy"), Value.Rotation.Accuracy);
			NMake::Pack(Rotation->SetDocument("min"), Value.Rotation.Min);
			NMake::Pack(Rotation->SetDocument("max"), Value.Rotation.Max);

			Rest::Document* Scale = V->SetDocument("scale");
			NMake::Pack(Scale->SetDocument("intensity"), Value.Scale.Intensity);
			NMake::Pack(Scale->SetDocument("accuracy"), Value.Scale.Accuracy);
			NMake::Pack(Scale->SetDocument("min"), Value.Scale.Min);
			NMake::Pack(Scale->SetDocument("max"), Value.Scale.Max);

			Rest::Document* Velocity = V->SetDocument("velocity");
			NMake::Pack(Velocity->SetDocument("intensity"), Value.Velocity.Intensity);
			NMake::Pack(Velocity->SetDocument("accuracy"), Value.Velocity.Accuracy);
			NMake::Pack(Velocity->SetDocument("min"), Value.Velocity.Min);
			NMake::Pack(Velocity->SetDocument("max"), Value.Velocity.Max);

			return V->SetInteger("[iterations]", Value.Iterations) && Angular && Diffusion && Noise && Position && Rotation && Scale && Velocity;
		}
		bool NMake::Pack(Rest::Document* V, const Appearance& Value, ContentManager* Content)
		{
			if (!V || !Content)
				return false;

			AssetCache* Asset = Content->Find<Graphics::Texture2D>(Value.GetDiffuseMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("diffuse-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value.GetNormalMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("normal-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value.GetMetallicMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("metallic-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value.GetRoughnessMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("roughness-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value.GetHeightMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("height-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value.GetOcclusionMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("occlusion-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value.GetEmissionMap());
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("emission-map"), Asset->Path);

			NMake::Pack(V->SetDocument("diffuse"), Value.Diffuse);
			NMake::Pack(V->SetDocument("texcoord"), Value.TexCoord);
			NMake::Pack(V->SetDocument("height-amount"), Value.HeightAmount);
			NMake::Pack(V->SetDocument("height-bias"), Value.HeightBias);
			NMake::Pack(V->SetDocument("material"), Value.Material);
			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::SkinAnimatorKey& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("pose"), Value.Pose);
			NMake::Pack(V->SetDocument("time"), Value.Time);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::SkinAnimatorClip& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("name"), Value.Name);
			NMake::Pack(V->SetDocument("duration"), Value.Duration);
			NMake::Pack(V->SetDocument("rate"), Value.Rate);

			Rest::Document* Array = V->SetArray("frames");
			for (auto&& It : Value.Keys)
				NMake::Pack(Array->SetDocument("frame"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::KeyAnimatorClip& Value)
		{
			if (!V)
				return false;

			return NMake::Pack(V->SetDocument("name"), Value.Name) && NMake::Pack(V->SetDocument("rate"), Value.Rate) && NMake::Pack(V->SetDocument("duration"), Value.Duration) && NMake::Pack(V->SetDocument("frames"), Value.Keys);
		}
		bool NMake::Pack(Rest::Document* V, const Compute::AnimatorKey& Value)
		{
			if (!V)
				return false;

			return NMake::Pack(V->SetDocument("position"), Value.Position) && NMake::Pack(V->SetDocument("rotation"), Value.Rotation) && NMake::Pack(V->SetDocument("scale"), Value.Scale) && NMake::Pack(V->SetDocument("time"), Value.Time);
		}
		bool NMake::Pack(Rest::Document* V, const Compute::ElementVertex& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[px]", Value.PositionX) != nullptr && V->SetNumber("[py]", Value.PositionY) != nullptr && V->SetNumber("[pz]", Value.PositionZ) != nullptr && V->SetNumber("[vx]", Value.VelocityX) != nullptr && V->SetNumber("[vy]", Value.VelocityY) != nullptr && V->SetNumber("[vz]", Value.VelocityZ) != nullptr && V->SetNumber("[cx]", Value.ColorX) != nullptr && V->SetNumber("[cy]", Value.ColorY) != nullptr && V->SetNumber("[cz]", Value.ColorZ) != nullptr && V->SetNumber("[cw]", Value.ColorW) != nullptr && V->SetNumber("[a]", Value.Angular) != nullptr && V->SetNumber("[s]", Value.Scale) != nullptr && V->SetNumber("[r]", Value.Rotation) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::Vertex& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[px]", Value.PositionX) != nullptr && V->SetNumber("[py]", Value.PositionY) != nullptr && V->SetNumber("[pz]", Value.PositionZ) != nullptr && V->SetNumber("[tx]", Value.TexCoordX) != nullptr && V->SetNumber("[ty]", Value.TexCoordY) != nullptr && V->SetNumber("[nx]", Value.NormalX) != nullptr && V->SetNumber("[ny]", Value.NormalY) != nullptr && V->SetNumber("[nz]", Value.NormalZ) != nullptr && V->SetNumber("[tnx]", Value.TangentX) != nullptr && V->SetNumber("[tny]", Value.TangentY) != nullptr && V->SetNumber("[tnz]", Value.TangentZ) != nullptr && V->SetNumber("[btx]", Value.BitangentX) != nullptr && V->SetNumber("[bty]", Value.BitangentY) != nullptr && V->SetNumber("[btz]", Value.BitangentZ) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::SkinVertex& Value)
		{
			if (!V)
				return false;

			return V->SetNumber("[px]", Value.PositionX) != nullptr && V->SetNumber("[py]", Value.PositionY) != nullptr && V->SetNumber("[pz]", Value.PositionZ) != nullptr && V->SetNumber("[tx]", Value.TexCoordX) != nullptr && V->SetNumber("[ty]", Value.TexCoordY) != nullptr && V->SetNumber("[nx]", Value.NormalX) != nullptr && V->SetNumber("[ny]", Value.NormalY) != nullptr && V->SetNumber("[nz]", Value.NormalZ) != nullptr && V->SetNumber("[tnx]", Value.TangentX) != nullptr && V->SetNumber("[tny]", Value.TangentY) != nullptr && V->SetNumber("[tnz]", Value.TangentZ) != nullptr && V->SetNumber("[btx]", Value.BitangentX) != nullptr && V->SetNumber("[bty]", Value.BitangentY) != nullptr && V->SetNumber("[btz]", Value.BitangentZ) != nullptr && V->SetNumber("[ji0]", Value.JointIndex0) != nullptr && V->SetNumber("[ji1]", Value.JointIndex1) != nullptr && V->SetNumber("[ji2]", Value.JointIndex2) != nullptr && V->SetNumber("[ji3]", Value.JointIndex3) != nullptr && V->SetNumber("[jb0]", Value.JointBias0) != nullptr && V->SetNumber("[jb1]", Value.JointBias1) != nullptr && V->SetNumber("[jb2]", Value.JointBias2) != nullptr && V->SetNumber("[jb3]", Value.JointBias3) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::Joint& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("index"), Value.Index);
			NMake::Pack(V->SetDocument("name"), Value.Name);
			NMake::Pack(V->SetDocument("transform"), Value.Transform);
			NMake::Pack(V->SetDocument("bind-shape"), Value.BindShape);

			Rest::Document* Joints = V->SetArray("childs");
			for (auto& It : Value.Childs)
				NMake::Pack(Joints->SetDocument("joint"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Rest::TickTimer& Value)
		{
			if (!V)
				return false;

			return V->SetInteger("[delay]", Value.Delay) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const std::string& Value)
		{
			if (!V)
				return false;

			return V->SetString("[s]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<bool>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("b-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<int>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("i-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<unsigned int>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("i-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<float>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("f-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<double>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("f-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<int64_t>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("f-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<long double>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("f-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<uint64_t>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("i-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::Vector2>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " ";

			return V->SetString("v2-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::Vector3>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " " << It.Z << " ";

			return V->SetString("v3-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::Vector4>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " " << It.Z << " " << It.W << " ";

			return V->SetString("v4-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::Matrix4x4>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				for (float i : It.Row)
					Stream << i << " ";
			}

			return V->SetString("m4x4-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<AnimatorState>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				Stream << It.Paused << " ";
				Stream << It.Looped << " ";
				Stream << It.Blended << " ";
				Stream << It.Duration << " ";
				Stream << It.Rate << " ";
				Stream << It.Time << " ";
				Stream << It.Frame << " ";
				Stream << It.Clip << " ";
			}

			return V->SetString("as-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<SpawnerProperties>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				Stream << It.Angular.Accuracy << " " << It.Angular.Min << " " << It.Angular.Max << " ";
				Stream << It.Rotation.Accuracy << " " << It.Rotation.Min << " " << It.Rotation.Max << " ";
				Stream << It.Scale.Accuracy << " " << It.Scale.Min << " " << It.Scale.Max << " ";
				Stream << It.Diffusion.Accuracy << " ";
				Stream << It.Diffusion.Min.X << " " << It.Diffusion.Min.Y << " " << It.Diffusion.Min.Z << " " << It.Diffusion.Min.W << " ";
				Stream << It.Diffusion.Max.X << " " << It.Diffusion.Max.Y << " " << It.Diffusion.Max.Z << " " << It.Diffusion.Max.W << " ";
				Stream << It.Noise.Accuracy << " ";
				Stream << It.Noise.Min.X << " " << It.Noise.Min.Y << " " << It.Noise.Min.Z << " ";
				Stream << It.Noise.Max.X << " " << It.Noise.Max.Y << " " << It.Noise.Max.Z << " ";
				Stream << It.Position.Accuracy << " ";
				Stream << It.Position.Min.X << " " << It.Position.Min.Y << " " << It.Position.Min.Z << " ";
				Stream << It.Position.Max.X << " " << It.Position.Max.Y << " " << It.Position.Max.Z << " ";
				Stream << It.Velocity.Accuracy << " ";
				Stream << It.Velocity.Min.X << " " << It.Velocity.Min.Y << " " << It.Velocity.Min.Z << " ";
				Stream << It.Velocity.Max.X << " " << It.Velocity.Max.Y << " " << It.Velocity.Max.Z << " ";
				Stream << It.Iterations << " ";
			}

			return V->SetString("sp-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::SkinAnimatorClip>& Value)
		{
			if (!V)
				return false;

			Rest::Document* Array = V->SetArray("clips");
			for (auto&& It : Value)
				NMake::Pack(Array->SetDocument("clip"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::KeyAnimatorClip>& Value)
		{
			if (!V)
				return false;

			Rest::Document* Array = V->SetArray("clips");
			for (auto&& It : Value)
				NMake::Pack(Array->SetDocument("clip"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::AnimatorKey>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				Stream << It.Position.X << " ";
				Stream << It.Position.Y << " ";
				Stream << It.Position.Z << " ";
				Stream << It.Rotation.X << " ";
				Stream << It.Rotation.Y << " ";
				Stream << It.Rotation.Z << " ";
				Stream << It.Scale.X << " ";
				Stream << It.Scale.Y << " ";
				Stream << It.Scale.Z << " ";
				Stream << It.Time << " ";
			}

			return V->SetString("ak-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::ElementVertex>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				Stream << It.PositionX << " ";
				Stream << It.PositionY << " ";
				Stream << It.PositionZ << " ";
				Stream << It.ColorX << " ";
				Stream << It.ColorY << " ";
				Stream << It.ColorZ << " ";
				Stream << It.ColorW << " ";
				Stream << It.VelocityX << " ";
				Stream << It.VelocityY << " ";
				Stream << It.VelocityZ << " ";
				Stream << It.Angular << " ";
				Stream << It.Rotation << " ";
				Stream << It.Scale << " ";
			}

			return V->SetString("ev-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::Joint>& Value)
		{
			if (!V)
				return false;

			for (auto&& It : Value)
				NMake::Pack(V->SetDocument("joint"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::Vertex>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				Stream << It.PositionX << " ";
				Stream << It.PositionY << " ";
				Stream << It.PositionZ << " ";
				Stream << It.TexCoordX << " ";
				Stream << It.TexCoordY << " ";
				Stream << It.NormalX << " ";
				Stream << It.NormalY << " ";
				Stream << It.NormalZ << " ";
				Stream << It.TangentX << " ";
				Stream << It.TangentY << " ";
				Stream << It.TangentZ << " ";
				Stream << It.BitangentX << " ";
				Stream << It.BitangentY << " ";
				Stream << It.BitangentZ << " ";
				Stream << "-1 -1 -1 -1 0 0 0 0 ";
			}

			return V->SetString("iv-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::SkinVertex>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				Stream << It.PositionX << " ";
				Stream << It.PositionY << " ";
				Stream << It.PositionZ << " ";
				Stream << It.TexCoordX << " ";
				Stream << It.TexCoordY << " ";
				Stream << It.NormalX << " ";
				Stream << It.NormalY << " ";
				Stream << It.NormalZ << " ";
				Stream << It.TangentX << " ";
				Stream << It.TangentY << " ";
				Stream << It.TangentZ << " ";
				Stream << It.BitangentX << " ";
				Stream << It.BitangentY << " ";
				Stream << It.BitangentZ << " ";
				Stream << It.JointIndex0 << " ";
				Stream << It.JointIndex1 << " ";
				Stream << It.JointIndex2 << " ";
				Stream << It.JointIndex3 << " ";
				Stream << It.JointBias0 << " ";
				Stream << It.JointBias1 << " ";
				Stream << It.JointBias2 << " ";
				Stream << It.JointBias3 << " ";
			}

			return V->SetString("iv-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Rest::TickTimer>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.Delay << " ";

			return V->SetString("tt-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<std::string>& Value)
		{
			if (!V)
				return false;

			Rest::Document* A = V->SetArray("s-array");
			for (auto&& It : Value)
				A->SetString("s", It);

			return V->SetInteger("[size]", Value.size());
		}
		bool NMake::Unpack(Rest::Document* V, bool* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetBoolean("[b]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, int* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetInteger("[i]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, unsigned int* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetInteger("[i]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, float* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetNumber("[n]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, double* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetNumber("[n]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, long double* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetNumber("[n]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, int64_t* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetInteger("[i]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, uint64_t* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetInteger("[i]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::Vector2* O)
		{
			if (!V || !O)
				return false;

			O->X = V->GetNumber("[x]");
			O->Y = V->GetNumber("[y]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::Vector3* O)
		{
			if (!V || !O)
				return false;

			O->X = V->GetNumber("[x]");
			O->Y = V->GetNumber("[y]");
			O->Z = V->GetNumber("[z]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::Vector4* O)
		{
			if (!V || !O)
				return false;

			O->X = V->GetNumber("[x]");
			O->Y = V->GetNumber("[y]");
			O->Z = V->GetNumber("[z]");
			O->W = V->GetNumber("[w]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::Matrix4x4* O)
		{
			if (!V || !O)
				return false;

			O->Row[0] = V->GetNumber("[m11]");
			O->Row[1] = V->GetNumber("[m12]");
			O->Row[2] = V->GetNumber("[m13]");
			O->Row[3] = V->GetNumber("[m14]");
			O->Row[4] = V->GetNumber("[m21]");
			O->Row[5] = V->GetNumber("[m22]");
			O->Row[6] = V->GetNumber("[m23]");
			O->Row[7] = V->GetNumber("[m24]");
			O->Row[8] = V->GetNumber("[m31]");
			O->Row[9] = V->GetNumber("[m32]");
			O->Row[10] = V->GetNumber("[m33]");
			O->Row[11] = V->GetNumber("[m34]");
			O->Row[12] = V->GetNumber("[m41]");
			O->Row[13] = V->GetNumber("[m42]");
			O->Row[14] = V->GetNumber("[m43]");
			O->Row[15] = V->GetNumber("[m44]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Attenuation* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Find("range"), &O->Range);
			NMake::Unpack(V->Find("c2"), &O->C1);
			NMake::Unpack(V->Find("c1"), &O->C2);
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Material* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Find("emission"), &O->Emission);
			NMake::Unpack(V->Find("metallic"), &O->Metallic);
			NMake::Unpack(V->Find("scatter"), &O->Scatter);
			NMake::Unpack(V->Find("roughness"), &O->Roughness);
			NMake::Unpack(V->Find("fresnel"), &O->Fresnel);
			NMake::Unpack(V->Find("refraction"), &O->Refraction);
			NMake::Unpack(V->Find("transparency"), &O->Transparency);
			NMake::Unpack(V->Find("environment"), &O->Environment);
			NMake::Unpack(V->Find("occlusion"), &O->Occlusion);
			NMake::Unpack(V->Find("radius"), &O->Radius);
			NMake::Unpack(V->Find("id"), &O->Id);
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, AnimatorState* O)
		{
			if (!V || !O)
				return false;

			O->Looped = V->GetBoolean("[looped]");
			O->Paused = V->GetBoolean("[paused]");
			O->Blended = V->GetBoolean("[blended]");
			O->Clip = V->GetInteger("[clip]");
			O->Frame = V->GetInteger("[frame]");
			O->Rate = V->GetNumber("[rate]");
			O->Duration = V->GetNumber("[duration]");
			O->Time = V->GetNumber("[time]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, SpawnerProperties* O)
		{
			if (!V || !O)
				return false;

			Rest::Document* Angular = V->Get("angular");
			NMake::Unpack(Angular->Get("intensity"), &O->Angular.Intensity);
			NMake::Unpack(Angular->Get("accuracy"), &O->Angular.Accuracy);
			NMake::Unpack(Angular->Get("min"), &O->Angular.Min);
			NMake::Unpack(Angular->Get("max"), &O->Angular.Max);

			Rest::Document* Diffusion = V->Get("diffusion");
			NMake::Unpack(Diffusion->Get("intensity"), &O->Diffusion.Intensity);
			NMake::Unpack(Diffusion->Get("accuracy"), &O->Diffusion.Accuracy);
			NMake::Unpack(Diffusion->Get("min"), &O->Diffusion.Min);
			NMake::Unpack(Diffusion->Get("max"), &O->Diffusion.Max);

			Rest::Document* Noise = V->Get("noise");
			NMake::Unpack(Noise->Get("intensity"), &O->Noise.Intensity);
			NMake::Unpack(Noise->Get("accuracy"), &O->Noise.Accuracy);
			NMake::Unpack(Noise->Get("min"), &O->Noise.Min);
			NMake::Unpack(Noise->Get("max"), &O->Noise.Max);

			Rest::Document* Position = V->Get("position");
			NMake::Unpack(Position->Get("intensity"), &O->Position.Intensity);
			NMake::Unpack(Position->Get("accuracy"), &O->Position.Accuracy);
			NMake::Unpack(Position->Get("min"), &O->Position.Min);
			NMake::Unpack(Position->Get("max"), &O->Position.Max);

			Rest::Document* Rotation = V->Get("rotation");
			NMake::Unpack(Rotation->Get("intensity"), &O->Rotation.Intensity);
			NMake::Unpack(Rotation->Get("accuracy"), &O->Rotation.Accuracy);
			NMake::Unpack(Rotation->Get("min"), &O->Rotation.Min);
			NMake::Unpack(Rotation->Get("max"), &O->Rotation.Max);

			Rest::Document* Scale = V->Get("scale");
			NMake::Unpack(Scale->Get("intensity"), &O->Scale.Intensity);
			NMake::Unpack(Scale->Get("accuracy"), &O->Scale.Accuracy);
			NMake::Unpack(Scale->Get("min"), &O->Scale.Min);
			NMake::Unpack(Scale->Get("max"), &O->Scale.Max);

			Rest::Document* Velocity = V->Get("velocity");
			NMake::Unpack(Velocity->Get("intensity"), &O->Velocity.Intensity);
			NMake::Unpack(Velocity->Get("accuracy"), &O->Velocity.Accuracy);
			NMake::Unpack(Velocity->Get("min"), &O->Velocity.Min);
			NMake::Unpack(Velocity->Get("max"), &O->Velocity.Max);

			O->Iterations = V->GetInteger("[iterations]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Appearance* O, ContentManager* Content)
		{
			if (!V || !O || !Content)
				return false;

			std::string Path;
			if (NMake::Unpack(V->Find("diffuse-map"), &Path))
				O->SetDiffuseMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Find("normal-map"), &Path))
				O->SetNormalMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Find("metallic-map"), &Path))
				O->SetMetallicMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Find("roughness-map"), &Path))
				O->SetRoughnessMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Find("height-map"), &Path))
				O->SetHeightMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Find("occlusion-map"), &Path))
				O->SetOcclusionMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Find("emission-map"), &Path))
				O->SetEmissionMap(Content->Load<Graphics::Texture2D>(Path));

			NMake::Unpack(V->Find("diffuse"), &O->Diffuse);
			NMake::Unpack(V->Find("texcoord"), &O->TexCoord);
			NMake::Unpack(V->Find("height-amount"), &O->HeightAmount);
			NMake::Unpack(V->Find("height-bias"), &O->HeightBias);
			NMake::Unpack(V->Find("material"), &O->Material);
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::SkinAnimatorKey* O)
		{
			if (!V)
				return false;

			NMake::Unpack(V->Find("pose"), &O->Pose);
			NMake::Unpack(V->Find("time"), &O->Time);

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::SkinAnimatorClip* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Find("name"), &O->Name);
			NMake::Unpack(V->Find("duration"), &O->Duration);
			NMake::Unpack(V->Find("rate"), &O->Rate);

			std::vector<Rest::Document*> Frames = V->FindCollectionPath("frames.frame", true);
			for (auto&& It : Frames)
			{
				O->Keys.emplace_back();
				NMake::Unpack(It, &O->Keys.back());
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::KeyAnimatorClip* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Find("name"), &O->Name);
			NMake::Unpack(V->Find("duration"), &O->Duration);
			NMake::Unpack(V->Find("rate"), &O->Rate);
			NMake::Unpack(V->Find("frames"), &O->Keys);
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::AnimatorKey* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("position"), &O->Position);
			NMake::Unpack(V->Get("rotation"), &O->Rotation);
			NMake::Unpack(V->Get("scale"), &O->Scale);
			NMake::Unpack(V->Get("time"), &O->Time);
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::Joint* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("index"), &O->Index);
			NMake::Unpack(V->Get("name"), &O->Name);
			NMake::Unpack(V->Get("transform"), &O->Transform);
			NMake::Unpack(V->Get("bind-shape"), &O->BindShape);

			std::vector<Rest::Document*> Joints = V->FindCollectionPath("childs.joint", true);
			for (auto& It : Joints)
			{
				O->Childs.emplace_back();
				NMake::Unpack(It, &O->Childs.back());
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::ElementVertex* O)
		{
			if (!V || !O)
				return false;

			O->PositionX = V->GetNumber("[px]");
			O->PositionY = V->GetNumber("[py]");
			O->PositionZ = V->GetNumber("[pz]");
			O->VelocityX = V->GetNumber("[vx]");
			O->VelocityY = V->GetNumber("[vy]");
			O->VelocityZ = V->GetNumber("[vz]");
			O->ColorX = V->GetNumber("[cx]");
			O->ColorY = V->GetNumber("[cy]");
			O->ColorZ = V->GetNumber("[cz]");
			O->ColorW = V->GetNumber("[cw]");
			O->Angular = V->GetNumber("[a]");
			O->Scale = V->GetNumber("[s]");
			O->Rotation = V->GetNumber("[r]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::Vertex* O)
		{
			if (!V || !O)
				return false;

			O->PositionX = V->GetNumber("[px]");
			O->PositionY = V->GetNumber("[py]");
			O->PositionZ = V->GetNumber("[pz]");
			O->TexCoordX = V->GetNumber("[tx]");
			O->TexCoordY = V->GetNumber("[ty]");
			O->NormalX = V->GetNumber("[nx]");
			O->NormalY = V->GetNumber("[ny]");
			O->NormalZ = V->GetNumber("[nz]");
			O->TangentX = V->GetNumber("[tnx]");
			O->TangentY = V->GetNumber("[tny]");
			O->TangentZ = V->GetNumber("[tnz]");
			O->BitangentX = V->GetNumber("[btx]");
			O->BitangentY = V->GetNumber("[bty]");
			O->BitangentZ = V->GetNumber("[btz]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::SkinVertex* O)
		{
			if (!V || !O)
				return false;

			O->PositionX = V->GetNumber("[px]");
			O->PositionY = V->GetNumber("[py]");
			O->PositionZ = V->GetNumber("[pz]");
			O->TexCoordX = V->GetNumber("[tx]");
			O->TexCoordY = V->GetNumber("[ty]");
			O->NormalX = V->GetNumber("[nx]");
			O->NormalY = V->GetNumber("[ny]");
			O->NormalZ = V->GetNumber("[nz]");
			O->TangentX = V->GetNumber("[tnx]");
			O->TangentY = V->GetNumber("[tny]");
			O->TangentZ = V->GetNumber("[tnz]");
			O->BitangentX = V->GetNumber("[btx]");
			O->BitangentY = V->GetNumber("[bty]");
			O->BitangentZ = V->GetNumber("[btz]");
			O->JointIndex0 = V->GetNumber("[ji0]");
			O->JointIndex1 = V->GetNumber("[ji1]");
			O->JointIndex2 = V->GetNumber("[ji2]");
			O->JointIndex3 = V->GetNumber("[ji3]");
			O->JointBias0 = V->GetNumber("[jb0]");
			O->JointBias1 = V->GetNumber("[jb1]");
			O->JointBias2 = V->GetNumber("[jb2]");
			O->JointBias3 = V->GetNumber("[jb3]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Rest::TickTimer* O)
		{
			if (!V || !O)
				return false;

			O->Delay = V->GetNumber("[delay]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::string* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetStringBlob("[s]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<bool>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("b-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				bool Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<int>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("i-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				int Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<unsigned int>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("i-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				unsigned int Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<float>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("f-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				float Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<double>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("f-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				double Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<int64_t>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("i-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				int64_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<long double>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("f-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				long double Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<uint64_t>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("i-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); It++)
			{
				uint64_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::Vector2>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("v2-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.X >> It.Y;

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::Vector3>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("v3-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.X >> It.Y >> It.Z;

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::Vector4>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("v4-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.X >> It.Y >> It.Z >> It.W;

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::Matrix4x4>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("m4x4-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
			{
				for (int64_t i = 0; i < 16; i++)
					Stream >> It.Row[i];
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<AnimatorState>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("as-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
			{
				Stream >> It.Paused;
				Stream >> It.Looped;
				Stream >> It.Blended;
				Stream >> It.Duration;
				Stream >> It.Rate;
				Stream >> It.Time;
				Stream >> It.Frame;
				Stream >> It.Clip;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<SpawnerProperties>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("sp-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
			{
				Stream >> It.Angular.Accuracy >> It.Angular.Min >> It.Angular.Max;
				Stream >> It.Rotation.Accuracy >> It.Rotation.Min >> It.Rotation.Max;
				Stream >> It.Scale.Accuracy >> It.Scale.Min >> It.Scale.Max;
				Stream >> It.Diffusion.Accuracy;
				Stream >> It.Diffusion.Min.X >> It.Diffusion.Min.Y >> It.Diffusion.Min.Z >> It.Diffusion.Min.W;
				Stream >> It.Diffusion.Max.X >> It.Diffusion.Max.Y >> It.Diffusion.Max.Z >> It.Diffusion.Max.W;
				Stream >> It.Noise.Accuracy;
				Stream >> It.Noise.Min.X >> It.Noise.Min.Y >> It.Noise.Min.Z;
				Stream >> It.Noise.Max.X >> It.Noise.Max.Y >> It.Noise.Max.Z;
				Stream >> It.Position.Accuracy;
				Stream >> It.Position.Min.X >> It.Position.Min.Y >> It.Position.Min.Z;
				Stream >> It.Position.Max.X >> It.Position.Max.Y >> It.Position.Max.Z;
				Stream >> It.Velocity.Accuracy;
				Stream >> It.Velocity.Min.X >> It.Velocity.Min.Y >> It.Velocity.Min.Z;
				Stream >> It.Velocity.Max.X >> It.Velocity.Max.Y >> It.Velocity.Max.Z;
				Stream >> It.Iterations;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::SkinAnimatorClip>* O)
		{
			if (!V || !O)
				return false;

			std::vector<Rest::Document*> Frames = V->FindCollectionPath("clips.clip", true);
			for (auto&& It : Frames)
			{
				O->push_back(Compute::SkinAnimatorClip());
				NMake::Unpack(It, &O->back());
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::KeyAnimatorClip>* O)
		{
			if (!V || !O)
				return false;

			std::vector<Rest::Document*> Frames = V->FindCollectionPath("clips.clip", true);
			for (auto&& It : Frames)
			{
				O->push_back(Compute::KeyAnimatorClip());
				NMake::Unpack(It, &O->back());
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::AnimatorKey>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("ak-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
			{
				Stream >> It.Position.X >> It.Position.Y >> It.Position.Z >> It.Rotation.X >> It.Rotation.Y >> It.Rotation.Z;
				Stream >> It.Scale.X >> It.Scale.Y >> It.Scale.Z >> It.Time;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::ElementVertex>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("ev-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
			{
				Stream >> It.PositionX;
				Stream >> It.PositionY;
				Stream >> It.PositionZ;
				Stream >> It.ColorX;
				Stream >> It.ColorY;
				Stream >> It.ColorZ;
				Stream >> It.ColorW;
				Stream >> It.VelocityX;
				Stream >> It.VelocityY;
				Stream >> It.VelocityZ;
				Stream >> It.Angular;
				Stream >> It.Rotation;
				Stream >> It.Scale;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::Joint>* O)
		{
			if (!V || !O)
				return false;

			O->reserve(V->GetNodes()->size());
			for (auto&& It : *V->GetNodes())
			{
				O->push_back(Compute::Joint());
				NMake::Unpack(It, &O->back());
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::Vertex>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("iv-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			float Dummy;
			for (auto& It : *O)
			{
				Stream >> It.PositionX;
				Stream >> It.PositionY;
				Stream >> It.PositionZ;
				Stream >> It.TexCoordX;
				Stream >> It.TexCoordY;
				Stream >> It.NormalX;
				Stream >> It.NormalY;
				Stream >> It.NormalZ;
				Stream >> It.TangentX;
				Stream >> It.TangentY;
				Stream >> It.TangentZ;
				Stream >> It.BitangentX;
				Stream >> It.BitangentY;
				Stream >> It.BitangentZ;
				Stream >> Dummy;
				Stream >> Dummy;
				Stream >> Dummy;
				Stream >> Dummy;
				Stream >> Dummy;
				Stream >> Dummy;
				Stream >> Dummy;
				Stream >> Dummy;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Compute::SkinVertex>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("iv-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
			{
				Stream >> It.PositionX;
				Stream >> It.PositionY;
				Stream >> It.PositionZ;
				Stream >> It.TexCoordX;
				Stream >> It.TexCoordY;
				Stream >> It.NormalX;
				Stream >> It.NormalY;
				Stream >> It.NormalZ;
				Stream >> It.TangentX;
				Stream >> It.TangentY;
				Stream >> It.TangentZ;
				Stream >> It.BitangentX;
				Stream >> It.BitangentY;
				Stream >> It.BitangentZ;
				Stream >> It.JointIndex0;
				Stream >> It.JointIndex1;
				Stream >> It.JointIndex2;
				Stream >> It.JointIndex3;
				Stream >> It.JointBias0;
				Stream >> It.JointBias1;
				Stream >> It.JointBias2;
				Stream >> It.JointBias3;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<Rest::TickTimer>* O)
		{
			if (!V || !O)
				return false;

			std::string& Array = V->GetStringBlob("tt-array");
			int64_t Size = V->GetInteger("[size]");
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.Delay;

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<std::string>* O)
		{
			if (!V || !O)
				return false;

			Rest::Document* Array = V->Get("tt-array");
			int64_t Size = V->GetInteger("[size]");
			if (!Array || !Size)
				return false;

			O->reserve((size_t)Size);
			for (auto&& It : *Array->GetNodes())
			{
				if (It->Name == "s" && It->Type == Rest::NodeType_String)
					O->push_back(It->String);
			}

			return true;
		}

		AssetFile::AssetFile(char* SrcBuffer, size_t SrcSize) : Buffer(SrcBuffer), Size(SrcSize)
		{
		}
		AssetFile::~AssetFile()
		{
			if (Buffer != nullptr)
				TH_FREE(Buffer);
		}
		char* AssetFile::GetBuffer()
		{
			return Buffer;
		}
		size_t AssetFile::GetSize()
		{
			return Size;
		}

		Reactor::Reactor(Application* Ref, double Limit, JobCallback Callback) : App(Ref), Src(Callback)
		{
			Time = new Rest::Timer();
			Time->FrameLimit = Limit;
		}
		Reactor::~Reactor()
		{
			TH_RELEASE(Time);
		}
		void Reactor::UpdateCore()
		{
			Time->Synchronize();
			if (Src != nullptr)
				Src(this, App);
		}
		void Reactor::UpdateTask()
		{
			Time->Synchronize();
			Src(this, App);
		}

		Processor::Processor(ContentManager* NewContent) : Content(NewContent)
		{
		}
		Processor::~Processor()
		{
		}
		void Processor::Free(AssetCache* Asset)
		{
		}
		void* Processor::Duplicate(AssetCache* Asset, const Compute::PropertyArgs& Args)
		{
			return nullptr;
		}
		void* Processor::Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Compute::PropertyArgs& Args)
		{
			return nullptr;
		}
		bool Processor::Serialize(Rest::Stream* Stream, void* Object, const Compute::PropertyArgs& Args)
		{
			return false;
		}
		ContentManager* Processor::GetContent()
		{
			return Content;
		}

		void Viewer::Set(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Near, float _Far)
		{
			View = _View;
			Projection = _Projection;
			ViewProjection = _View * _Projection;
			InvViewProjection = ViewProjection.Invert();
			InvViewPosition = _Position.InvertZ();
			ViewPosition = InvViewPosition.Invert();
			WorldPosition = _Position;
			WorldRotation = -_View.Rotation();
			FarPlane = (_Far < _Near ? 999999999 : _Far);
			NearPlane = _Near;
			CubicViewProjection[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, InvViewPosition) * Projection;
			CubicViewProjection[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, InvViewPosition) * Projection;
			CubicViewProjection[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, InvViewPosition) * Projection;
			CubicViewProjection[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, InvViewPosition) * Projection;
			CubicViewProjection[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, InvViewPosition) * Projection;
			CubicViewProjection[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, InvViewPosition) * Projection;
		}

		Component::Component(Entity* Reference) : Active(true), Parent(Reference)
		{
		}
		Component::~Component()
		{
		}
		void Component::Deserialize(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Component::Serialize(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Component::Awake(Component* New)
		{
		}
		void Component::Asleep()
		{
		}
		void Component::Synchronize(Rest::Timer* Time)
		{
		}
		void Component::Update(Rest::Timer* Time)
		{
		}
		void Component::Message(Event* Value)
		{
		}
		Entity* Component::GetEntity()
		{
			return Parent;
		}
		Compute::Matrix4x4 Component::GetBoundingBox()
		{
			return Parent->Transform->GetWorld();
		}
		void Component::SetActive(bool Enabled)
		{
			if (Active == Enabled)
				return;

			if ((Active = Enabled))
				Awake(nullptr);
			else
				Asleep();

			if (!Parent || !Parent->GetScene())
				return;

			auto Components = Parent->GetScene()->GetComponents(GetId());
			if (Active)
				Components->AddIfNotExists(this);
			else
				Components->Remove(this);

			if (Active)
				Parent->GetScene()->Pending.AddIfNotExists(this);
			else
				Parent->GetScene()->Pending.Remove(this);
		}
		bool Component::IsActive()
		{
			return Active;
		}

		Cullable::Cullable(Entity* Ref) : Component(Ref), Visibility(1.0f)
		{
		}
		void Cullable::ClearCull()
		{
		}
		float Cullable::GetRange()
		{
			float Max = Parent->Transform->Scale.X;
			if (Max < Parent->Transform->Scale.Y)
				Max = Parent->Transform->Scale.Y;

			if (Max < Parent->Transform->Scale.Z)
				return Parent->Transform->Scale.Z;

			return Max;
		}
		bool Cullable::IsVisible(const Viewer& View, Compute::Matrix4x4* World)
		{
			if (Parent->Transform->Position.Distance(View.WorldPosition) > View.FarPlane + Parent->Transform->Scale.Length())
				return false;

			return Compute::Common::IsCubeInFrustum((World ? *World : Parent->Transform->GetWorld()) * View.ViewProjection, 1.65f) == -1;
		}
		bool Cullable::IsNear(const Viewer& View)
		{
			return Parent->Transform->Position.Distance(View.WorldPosition) <= View.FarPlane + Parent->Transform->Scale.Length();
		}

		Drawable::Drawable(Entity* Ref, uint64_t Hash, bool vComplex) : Cullable(Ref), Category(GeoCategory_Opaque), Fragments(1), Satisfied(1), Source(Hash), Complex(vComplex), Static(true), Query(nullptr)
		{
			if (!Complex)
				Surfaces[nullptr] = Appearance();
		}
		Drawable::~Drawable()
		{
			TH_RELEASE(Query);
			Detach();
		}
		void Drawable::Message(Event* Value)
		{
			if (!Value || !Value->Is("materialchange"))
				return;

			Compute::Property& MaterialId = Value->Args["material-id"];
			if (!MaterialId)
				return;

			int64_t Material = MaterialId.GetInteger();
			for (auto&& Surface : Surfaces)
			{
				if (Surface.second.Material == Material)
					Surface.second.Material = -1;
			}
		}
		void Drawable::ClearCull()
		{
			Fragments = 1;
		}
		bool Drawable::SetTransparency(bool Enabled)
		{
			if (!Parent || !Parent->GetScene())
			{
				if (Enabled)
					Category = GeoCategory_Transparent;
				else
					Category = GeoCategory_Opaque;

				return false;
			}

			Detach();
			if (Enabled)
				Category = GeoCategory_Transparent;
			else
				Category = GeoCategory_Opaque;

			Parent->GetScene()->AddDrawable(this, Category);
			return true;
		}
		bool Drawable::Begin(Graphics::GraphicsDevice* Device)
		{
			if (!Device)
				return false;

			if (!Query)
			{
				Graphics::Query::Desc I;
				I.Predicate = false;
				I.AutoPass = false;

				Query = Device->CreateQuery(I);
			}

			if (Satisfied == 1)
			{
				Satisfied = 0;
				Device->QueryBegin(Query);
				return true;
			}

			if (Satisfied > 1)
				Satisfied--;

			return Fragments > 0;
		}
		void Drawable::End(Graphics::GraphicsDevice* Device)
		{
			if (Device && Satisfied == 0)
			{
				Satisfied = -1;
				Device->QueryEnd(Query);
			}
		}
		void Drawable::Attach()
		{
			SetTransparency(false);
		}
		void Drawable::Detach()
		{
			if (Parent && Parent->GetScene())
				Parent->GetScene()->RemoveDrawable(this, Category);
		}
		int Drawable::Fetch(RenderSystem* System)
		{
			if (!System || !Query || Satisfied != -1)
				return -1;

			if (!System->GetDevice()->GetQueryData(Query, &Fragments))
				return -1;

			Satisfied = 1 + System->StallFrames;
			return Fragments > 0;
		}
		uint64_t Drawable::GetFragmentsCount()
		{
			return Fragments;
		}
		const std::unordered_map<void*, Appearance>& Drawable::GetSurfaces()
		{
			return Surfaces;
		}
		Material* Drawable::GetMaterial(Appearance* Surface)
		{
			if (!Surface || Surface->Material < 0)
				return nullptr;

			return Parent->GetScene()->GetMaterialById((uint64_t)Surface->Material);
		}
		Material* Drawable::GetMaterial()
		{
			Appearance* Surface = GetSurface();
			return GetMaterial(Surface);
		}
		Appearance* Drawable::GetSurface(void* Instance)
		{
			if (!Complex)
				return nullptr;

			auto It = Surfaces.find(Instance);
			if (It == Surfaces.end())
				return &Surfaces[Instance];

			return &It->second;
		}
		Appearance* Drawable::GetSurface()
		{
			if (Complex || Surfaces.empty())
				return nullptr;

			return &Surfaces.begin()->second;
		}
		bool Drawable::HasTransparency()
		{
			return Category == GeoCategory_Transparent;
		}

		Entity::Entity(SceneGraph* Ref) : Scene(Ref), Id(-1), Tag(-1), Distance(0)
		{
			Transform = new Compute::Transform();
			Transform->UserPointer = this;
		}
		Entity::~Entity()
		{
			for (auto& Component : Components)
			{
				if (!Component.second)
					continue;

				Component.second->SetActive(false);
				delete Component.second;
			}

			TH_RELEASE(Transform);
		}
		void Entity::RemoveComponent(uint64_t Id)
		{
			std::unordered_map<uint64_t, Component*>::iterator It = Components.find(Id);
			if (It == Components.end())
				return;

			It->second->SetActive(false);
			if (Scene != nullptr)
			{
				Scene->Lock();
				if (Scene->Camera == It->second)
					Scene->Camera = nullptr;
			}

			delete It->second;
			Components.erase(It);
			if (Scene != nullptr)
			{
				Scene->Unlock();
				Scene->Mutate(this, false);
			}
		}
		void Entity::RemoveChilds()
		{
			if (!Transform->GetChilds())
				return;

			if (Scene != nullptr)
				Scene->Lock();

			uint64_t Count = Transform->GetChilds()->size();
			for (uint64_t i = 0; i < Count; i++)
			{
				if (!Transform->GetChilds())
				{
					if (Scene != nullptr)
						Scene->Unlock();

					return;
				}

				Entity* Entity = Transform->GetChild(i)->Ptr<Engine::Entity>();
				if (!Entity || Entity == this)
					continue;

				if (Scene != nullptr)
					Scene->RemoveEntity(Entity, true);
				else
					TH_RELEASE(Entity);

				if (Transform->GetChildCount() == 0)
				{
					if (Scene != nullptr)
						Scene->Unlock();

					return;
				}

				Count--;
				i--;
			}

			if (Scene != nullptr)
				Scene->Unlock();
		}
		void Entity::SetScene(SceneGraph* NewScene)
		{
			Scene = NewScene;
		}
		std::unordered_map<uint64_t, Component*>::iterator Entity::First()
		{
			return Components.begin();
		}
		std::unordered_map<uint64_t, Component*>::iterator Entity::Last()
		{
			return Components.end();
		}
		Component* Entity::AddComponent(Component* In)
		{
			if (!In || In == GetComponent(In->GetId()))
				return In;

			RemoveComponent(In->GetId());
			In->Active = false;
			In->Parent = this;

			Components.insert({ In->GetId(), In });
			for (auto& Component : Components)
				Component.second->Awake(In == Component.second ? nullptr : In);

			In->SetActive(true);
			if (Scene != nullptr)
				Scene->Mutate(this, true);

			return In;
		}
		Component* Entity::GetComponent(uint64_t Id)
		{
			std::unordered_map<uint64_t, Component*>::iterator It = Components.find(Id);
			if (It != Components.end())
				return It->second;

			return nullptr;
		}
		uint64_t Entity::GetComponentCount()
		{
			return Components.size();
		}
		SceneGraph* Entity::GetScene()
		{
			return Scene;
		}

		Renderer::Renderer(RenderSystem* Lab) : System(Lab), Active(true)
		{
		}
		Renderer::~Renderer()
		{
		}
		void Renderer::Deserialize(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Renderer::Serialize(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Renderer::CullGeometry(const Viewer& View)
		{
		}
		void Renderer::ResizeBuffers()
		{
		}
		void Renderer::Activate()
		{
		}
		void Renderer::Deactivate()
		{
		}
		void Renderer::Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options)
		{
		}
		void Renderer::SetRenderer(RenderSystem* NewSystem)
		{
			System = NewSystem;
		}
		RenderSystem* Renderer::GetRenderer()
		{
			return System;
		}

		ShaderCache::ShaderCache(Graphics::GraphicsDevice* NewDevice) : Device(NewDevice)
		{
		}
		ShaderCache::~ShaderCache()
		{
			ClearCache();
		}
		Graphics::Shader* ShaderCache::Compile(const std::string& Name, const Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			Graphics::Shader* Shader = Get(Name);
			if (Shader != nullptr)
				return Shader;

			Shader = Device->CreateShader(Desc);
			if (!Shader->IsValid())
			{
				TH_RELEASE(Shader);
				return nullptr;
			}
			else if (BufferSize > 0)
				Device->UpdateBufferSize(Shader, BufferSize);

			Safe.lock();
			SCache& Result = Cache[Name];
			Result.Shader = Shader;
			Result.Count = 1;
			Safe.unlock();

			return Shader;
		}
		Graphics::Shader* ShaderCache::Get(const std::string& Name)
		{
			Safe.lock();
			auto It = Cache.find(Name);
			if (It != Cache.end())
			{
				It->second.Count++;
				Safe.unlock();

				return It->second.Shader;
			}

			Safe.unlock();
			return nullptr;
		}
		std::string ShaderCache::Find(Graphics::Shader* Shader)
		{
			Safe.lock();
			for (auto& Item : Cache)
			{
				if (Item.second.Shader == Shader)
				{
					std::string Result = Item.first;
					Safe.unlock();
					return Result;
				}
			}

			Safe.unlock();
			return std::string();
		}
		bool ShaderCache::Has(const std::string& Name)
		{
			Safe.lock();
			auto It = Cache.find(Name);
			if (It != Cache.end())
			{
				Safe.unlock();
				return true;
			}

			Safe.unlock();
			return false;
		}
		bool ShaderCache::Free(const std::string& Name, Graphics::Shader* Shader)
		{
			Safe.lock();
			auto It = Cache.find(Name);
			if (It == Cache.end())
				return false;

			if (Shader != nullptr && Shader != It->second.Shader)
			{
				Safe.unlock();
				return false;
			}

			It->second.Count--;
			if (It->second.Count > 0)
			{
				Safe.unlock();
				return true;
			}

			TH_RELEASE(It->second.Shader);
			Cache.erase(It);
			Safe.unlock();

			return true;
		}
		void ShaderCache::ClearCache()
		{
			Safe.lock();
			for (auto It = Cache.begin(); It != Cache.end(); It++)
				TH_CLEAR(It->second.Shader);

			Cache.clear();
			Safe.unlock();
		}

		PrimitiveCache::PrimitiveCache(Graphics::GraphicsDevice* Ref) : Device(Ref), Quad(nullptr)
		{
			Sphere[0] = Sphere[1] = nullptr;
			Cube[0] = Cube[1] = nullptr;
			Box[0] = Box[1] = nullptr;
			SkinBox[0] = SkinBox[1] = nullptr;
		}
		PrimitiveCache::~PrimitiveCache()
		{
			ClearCache();
		}
		bool PrimitiveCache::Compile(Graphics::ElementBuffer** Results, const std::string& Name, size_t ElementSize, size_t ElementsCount)
		{
			if (!Results || Get(Results, Name))
				return false;

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Write;
			F.Usage = Graphics::ResourceUsage_Dynamic;
			F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
			F.ElementWidth = ElementSize;
			F.ElementCount = ElementsCount;

			Graphics::ElementBuffer* VertexBuffer = Device->CreateElementBuffer(F);
			if (!VertexBuffer)
				return false;

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Write;
			F.Usage = Graphics::ResourceUsage_Dynamic;
			F.BindFlags = Graphics::ResourceBind_Index_Buffer;
			F.ElementWidth = sizeof(int);
			F.ElementCount = ElementsCount * 3;

			Graphics::ElementBuffer* IndexBuffer = Device->CreateElementBuffer(F);
			if (!IndexBuffer)
			{
				TH_RELEASE(VertexBuffer);
				return false;
			}

			Safe.lock();
			SCache& Result = Cache[Name];
			Result.Buffers[BufferType_Index] = Results[BufferType_Index] = IndexBuffer;
			Result.Buffers[BufferType_Vertex] = Results[BufferType_Vertex] = VertexBuffer;
			Result.Count = 1;
			Safe.unlock();

			return true;
		}
		bool PrimitiveCache::Get(Graphics::ElementBuffer** Results, const std::string& Name)
		{
			if (!Results)
				return false;

			Safe.lock();
			auto It = Cache.find(Name);
			if (It != Cache.end())
			{
				It->second.Count++;
				Safe.unlock();

				Results[BufferType_Index] = It->second.Buffers[BufferType_Index];
				Results[BufferType_Vertex] = It->second.Buffers[BufferType_Vertex];
				return true;
			}

			Safe.unlock();
			return false;
		}
		bool PrimitiveCache::Has(const std::string& Name)
		{
			Safe.lock();
			auto It = Cache.find(Name);
			if (It != Cache.end())
			{
				Safe.unlock();
				return true;
			}

			Safe.unlock();
			return false;
		}
		bool PrimitiveCache::Free(const std::string& Name, Graphics::ElementBuffer** Buffers)
		{
			Safe.lock();
			auto It = Cache.find(Name);
			if (It == Cache.end())
				return false;

			if (Buffers != nullptr)
			{
				if ((Buffers[0] != nullptr && Buffers[0] != It->second.Buffers[0]) || (Buffers[1] != nullptr && Buffers[1] != It->second.Buffers[1]))
				{
					Safe.unlock();
					return false;
				}
			}

			It->second.Count--;
			if (It->second.Count > 0)
			{
				Safe.unlock();
				return true;
			}

			TH_RELEASE(It->second.Buffers[0]);
			TH_RELEASE(It->second.Buffers[1]);
			Cache.erase(It);
			Safe.unlock();

			return true;
		}
		std::string PrimitiveCache::Find(Graphics::ElementBuffer** Buffers)
		{
			if (!Buffers)
				return std::string();

			Safe.lock();
			for (auto& Item : Cache)
			{
				if (Item.second.Buffers[0] == Buffers[0] && Item.second.Buffers[1] == Buffers[1])
				{
					std::string Result = Item.first;
					Safe.unlock();
					return Result;
				}
			}

			Safe.unlock();
			return std::string();
		}
		Graphics::ElementBuffer* PrimitiveCache::GetQuad()
		{
			if (Quad != nullptr)
				return Quad;

			if (!Device)
				return nullptr;

			std::vector<Compute::ShapeVertex> Elements;
			Elements.push_back({ -1.0f, -1.0f, 0, -1, 0 });
			Elements.push_back({ -1.0f, 1.0f, 0, -1, -1 });
			Elements.push_back({ 1.0f, 1.0f, 0, 0, -1 });
			Elements.push_back({ -1.0f, -1.0f, 0, -1, 0 });
			Elements.push_back({ 1.0f, 1.0f, 0, 0, -1 });
			Elements.push_back({ 1.0f, -1.0f, 0, 0, 0 });

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Invalid;
			F.Usage = Graphics::ResourceUsage_Default;
			F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
			F.ElementCount = 6;
			F.ElementWidth = sizeof(Compute::ShapeVertex);
			F.Elements = &Elements[0];

			Safe.lock();
			Quad = Device->CreateElementBuffer(F);
			Safe.unlock();

			return Quad;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetSphere(BufferType Type)
		{
			if (Sphere[Type] != nullptr)
				return Sphere[Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType_Index)
			{
				std::vector<int> Indices;
				Indices.push_back(0);
				Indices.push_back(4);
				Indices.push_back(1);
				Indices.push_back(0);
				Indices.push_back(9);
				Indices.push_back(4);
				Indices.push_back(9);
				Indices.push_back(5);
				Indices.push_back(4);
				Indices.push_back(4);
				Indices.push_back(5);
				Indices.push_back(8);
				Indices.push_back(4);
				Indices.push_back(8);
				Indices.push_back(1);
				Indices.push_back(8);
				Indices.push_back(10);
				Indices.push_back(1);
				Indices.push_back(8);
				Indices.push_back(3);
				Indices.push_back(10);
				Indices.push_back(5);
				Indices.push_back(3);
				Indices.push_back(8);
				Indices.push_back(5);
				Indices.push_back(2);
				Indices.push_back(3);
				Indices.push_back(2);
				Indices.push_back(7);
				Indices.push_back(3);
				Indices.push_back(7);
				Indices.push_back(10);
				Indices.push_back(3);
				Indices.push_back(7);
				Indices.push_back(6);
				Indices.push_back(10);
				Indices.push_back(7);
				Indices.push_back(11);
				Indices.push_back(6);
				Indices.push_back(11);
				Indices.push_back(0);
				Indices.push_back(6);
				Indices.push_back(0);
				Indices.push_back(1);
				Indices.push_back(6);
				Indices.push_back(6);
				Indices.push_back(1);
				Indices.push_back(10);
				Indices.push_back(9);
				Indices.push_back(0);
				Indices.push_back(11);
				Indices.push_back(9);
				Indices.push_back(11);
				Indices.push_back(2);
				Indices.push_back(9);
				Indices.push_back(2);
				Indices.push_back(5);
				Indices.push_back(7);
				Indices.push_back(2);
				Indices.push_back(11);

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				Sphere[BufferType_Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Sphere[BufferType_Index];
			}
			else if (Type == BufferType_Vertex)
			{
				const float X = 0.525731112119133606;
				const float Z = 0.850650808352039932;
				const float N = 0.0f;

				std::vector<Compute::ShapeVertex> Elements;
				Elements.push_back({ -X, N, Z });
				Elements.push_back({ X, N, Z });
				Elements.push_back({ -X, N, -Z });
				Elements.push_back({ X, N, -Z });
				Elements.push_back({ N, Z, X });
				Elements.push_back({ N, Z, -X });
				Elements.push_back({ N, -Z, X });
				Elements.push_back({ N, -Z, -X });
				Elements.push_back({ Z, X, N });
				Elements.push_back({ -Z, X, N });
				Elements.push_back({ Z, -X, N });
				Elements.push_back({ -Z, -X, N });

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::ShapeVertex);
				F.Elements = &Elements[0];

				Safe.lock();
				Sphere[BufferType_Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Sphere[BufferType_Vertex];
			}

			return nullptr;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetCube(BufferType Type)
		{
			if (Cube[Type] != nullptr)
				return Cube[Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType_Index)
			{
				std::vector<int> Indices;
				Indices.push_back(0);
				Indices.push_back(1);
				Indices.push_back(2);
				Indices.push_back(0);
				Indices.push_back(18);
				Indices.push_back(1);
				Indices.push_back(3);
				Indices.push_back(4);
				Indices.push_back(5);
				Indices.push_back(3);
				Indices.push_back(19);
				Indices.push_back(4);
				Indices.push_back(6);
				Indices.push_back(7);
				Indices.push_back(8);
				Indices.push_back(6);
				Indices.push_back(20);
				Indices.push_back(7);
				Indices.push_back(9);
				Indices.push_back(10);
				Indices.push_back(11);
				Indices.push_back(9);
				Indices.push_back(21);
				Indices.push_back(10);
				Indices.push_back(12);
				Indices.push_back(13);
				Indices.push_back(14);
				Indices.push_back(12);
				Indices.push_back(22);
				Indices.push_back(13);
				Indices.push_back(15);
				Indices.push_back(16);
				Indices.push_back(17);
				Indices.push_back(15);
				Indices.push_back(23);
				Indices.push_back(16);

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				Cube[BufferType_Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Cube[BufferType_Index];
			}
			else if (Type == BufferType_Vertex)
			{
				std::vector<Compute::ShapeVertex> Elements;
				Elements.push_back({ -1, 1, 1, 0.875, -0.5 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75 });
				Elements.push_back({ -1, -1, -1, 0.375, -1 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75 });
				Elements.push_back({ -1, -1, 1, 0.625, -0 });
				Elements.push_back({ -1, 1, -1, 0.375, -0.25 });
				Elements.push_back({ -1, -1, -1, 0.375, -0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5 });
				Elements.push_back({ -1, -1, -1, 0.125, -0.75 });
				Elements.push_back({ -1, 1, -1, 0.125, -0.5 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5 });
				Elements.push_back({ -1, 1, 1, 0.625, -0.25 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5 });
				Elements.push_back({ -1, 1, -1, 0.375, -0.25 });
				Elements.push_back({ -1, -1, 1, 0.875, -0.75 });
				Elements.push_back({ -1, -1, 1, 0.625, -1 });
				Elements.push_back({ -1, 1, 1, 0.625, -0.25 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5 });

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::ShapeVertex);
				F.Elements = &Elements[0];

				Safe.lock();
				Cube[BufferType_Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Cube[BufferType_Vertex];
			}

			return nullptr;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetBox(BufferType Type)
		{
			if (Box[Type] != nullptr)
				return Box[Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType_Index)
			{
				std::vector<int> Indices;
				Indices.push_back(0);
				Indices.push_back(1);
				Indices.push_back(2);
				Indices.push_back(0);
				Indices.push_back(18);
				Indices.push_back(1);
				Indices.push_back(3);
				Indices.push_back(4);
				Indices.push_back(5);
				Indices.push_back(3);
				Indices.push_back(19);
				Indices.push_back(4);
				Indices.push_back(6);
				Indices.push_back(7);
				Indices.push_back(8);
				Indices.push_back(6);
				Indices.push_back(20);
				Indices.push_back(7);
				Indices.push_back(9);
				Indices.push_back(10);
				Indices.push_back(11);
				Indices.push_back(9);
				Indices.push_back(21);
				Indices.push_back(10);
				Indices.push_back(12);
				Indices.push_back(13);
				Indices.push_back(14);
				Indices.push_back(12);
				Indices.push_back(22);
				Indices.push_back(13);
				Indices.push_back(15);
				Indices.push_back(16);
				Indices.push_back(17);
				Indices.push_back(15);
				Indices.push_back(23);
				Indices.push_back(16);
				Compute::Common::ComputeIndexWindingOrderFlip(Indices);

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				Box[BufferType_Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Box[BufferType_Index];
			}
			else if (Type == BufferType_Vertex)
			{
				std::vector<Compute::Vertex> Elements;
				Elements.push_back({ -1, 1, 1, 0.875, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0 });
				Elements.push_back({ -1, -1, -1, 0.375, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0 });
				Elements.push_back({ -1, -1, 1, 0.625, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0 });
				Elements.push_back({ -1, 1, -1, 0.375, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0 });
				Elements.push_back({ -1, -1, -1, 0.375, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0 });
				Elements.push_back({ -1, -1, -1, 0.125, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0 });
				Elements.push_back({ -1, 1, -1, 0.125, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0 });
				Elements.push_back({ -1, 1, 1, 0.625, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0 });
				Elements.push_back({ -1, 1, -1, 0.375, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0 });
				Elements.push_back({ -1, -1, 1, 0.875, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0 });
				Elements.push_back({ -1, -1, 1, 0.625, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0 });
				Elements.push_back({ -1, 1, 1, 0.625, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0 });

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::Vertex);
				F.Elements = &Elements[0];

				Safe.lock();
				Box[BufferType_Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Box[BufferType_Vertex];
			}

			return nullptr;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetSkinBox(BufferType Type)
		{
			if (SkinBox[Type] != nullptr)
				return SkinBox[Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType_Index)
			{
				std::vector<int> Indices;
				Indices.push_back(0);
				Indices.push_back(1);
				Indices.push_back(2);
				Indices.push_back(0);
				Indices.push_back(18);
				Indices.push_back(1);
				Indices.push_back(3);
				Indices.push_back(4);
				Indices.push_back(5);
				Indices.push_back(3);
				Indices.push_back(19);
				Indices.push_back(4);
				Indices.push_back(6);
				Indices.push_back(7);
				Indices.push_back(8);
				Indices.push_back(6);
				Indices.push_back(20);
				Indices.push_back(7);
				Indices.push_back(9);
				Indices.push_back(10);
				Indices.push_back(11);
				Indices.push_back(9);
				Indices.push_back(21);
				Indices.push_back(10);
				Indices.push_back(12);
				Indices.push_back(13);
				Indices.push_back(14);
				Indices.push_back(12);
				Indices.push_back(22);
				Indices.push_back(13);
				Indices.push_back(15);
				Indices.push_back(16);
				Indices.push_back(17);
				Indices.push_back(15);
				Indices.push_back(23);
				Indices.push_back(16);
				Compute::Common::ComputeIndexWindingOrderFlip(Indices);

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				SkinBox[BufferType_Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return SkinBox[BufferType_Index];
			}
			else if (Type == BufferType_Vertex)
			{
				std::vector<Compute::SkinVertex> Elements;
				Elements.push_back({ -1, 1, 1, 0.875, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, -1, -1, 0.375, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, -1, 1, 0.625, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, 1, -1, 0.375, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, -1, -1, 0.375, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, -1, -1, 0.125, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, 1, -1, 0.125, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, 1, 1, 0.625, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, 1, -1, 0.375, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, 1, -1, 0.375, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, -1, 1, 0.875, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, -1, 1, 0.625, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ -1, 1, 1, 0.625, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, -1, -1, 0.375, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, -1, 1, 0.625, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 });
				Elements.push_back({ 1, 1, 1, 0.625, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 });

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Invalid;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::SkinVertex);
				F.Elements = &Elements[0];

				Safe.lock();
				SkinBox[BufferType_Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return SkinBox[BufferType_Vertex];
			}

			return nullptr;
		}
		void PrimitiveCache::GetSphereBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[BufferType_Index] = GetSphere(BufferType_Index);
				Result[BufferType_Vertex] = GetSphere(BufferType_Vertex);
			}
		}
		void PrimitiveCache::GetCubeBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[BufferType_Index] = GetCube(BufferType_Index);
				Result[BufferType_Vertex] = GetCube(BufferType_Vertex);
			}
		}
		void PrimitiveCache::GetBoxBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[BufferType_Index] = GetBox(BufferType_Index);
				Result[BufferType_Vertex] = GetBox(BufferType_Vertex);
			}
		}
		void PrimitiveCache::GetSkinBoxBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[BufferType_Index] = GetSkinBox(BufferType_Index);
				Result[BufferType_Vertex] = GetSkinBox(BufferType_Vertex);
			}
		}
		void PrimitiveCache::ClearCache()
		{
			Safe.lock();
			for (auto It = Cache.begin(); It != Cache.end(); It++)
			{
				TH_CLEAR(It->second.Buffers[0]);
				TH_CLEAR(It->second.Buffers[1]);
			}

			Cache.clear();
			TH_CLEAR(Sphere[0]);
			TH_CLEAR(Sphere[1]);
			TH_CLEAR(Cube[0]);
			TH_CLEAR(Cube[1]);
			TH_CLEAR(Box[0]);
			TH_CLEAR(Box[1]);
			TH_CLEAR(SkinBox[0]);
			TH_CLEAR(SkinBox[1]);
			TH_CLEAR(Quad);
			Safe.unlock();
		}

		RenderSystem::RenderSystem(Graphics::GraphicsDevice* Ref) : Device(Ref), Target(nullptr), Scene(nullptr), FrustumCulling(true), OcclusionCulling(false)
		{
			Occlusion.Delay = 5;
			Sorting.Delay = 5;
			StallFrames = 1;
			DepthSize = 256;
			Satisfied = true;
		}
		RenderSystem::~RenderSystem()
		{
			for (auto& Renderer : Renderers)
			{
				if (!Renderer)
					continue;

				Renderer->Deactivate();
				TH_RELEASE(Renderer);
			}

			TH_RELEASE(Target);
		}
		void RenderSystem::SetOcclusionCulling(bool Enabled, bool KeepResults)
		{
			OcclusionCulling = Enabled;
			if (!KeepResults)
				ClearCull();
		}
		void RenderSystem::SetFrustumCulling(bool Enabled, bool KeepResults)
		{
			FrustumCulling = Enabled;
			if (!KeepResults)
				ClearCull();
		}
		void RenderSystem::SetDepthSize(size_t Size)
		{
			if (!Scene || !Device)
				return;

			DepthStencil = Device->GetDepthStencilState("less-no-stencil");
			Blend = Device->GetBlendState("overwrite-colorless");
			Sampler = Device->GetSamplerState("point");
			DepthSize = Size;

			Graphics::MultiRenderTarget2D* MRT = Scene->GetMRT(TargetType_Main);
			float Aspect = MRT->GetWidth() / MRT->GetHeight();

			Graphics::DepthBuffer::Desc I;
			I.Width = (size_t)((float)Size * Aspect);
			I.Height = Size;

			TH_RELEASE(Target);
			Target = Device->CreateDepthBuffer(I);
		}
		void RenderSystem::SetScene(SceneGraph* NewScene)
		{
			Scene = NewScene;
			SetDepthSize(DepthSize);
		}
		void RenderSystem::Remount(Renderer* Target)
		{
			if (!Target)
				return;

			Target->Deactivate();
			Target->SetRenderer(this);
			Target->Activate();
			Target->ResizeBuffers();
		}
		void RenderSystem::Remount()
		{
			ClearCull();
			for (auto& Target : Renderers)
				Remount(Target);
		}
		void RenderSystem::Mount()
		{
			for (auto& Renderer : Renderers)
				Renderer->Activate();
		}
		void RenderSystem::Unmount()
		{
			for (auto& Renderer : Renderers)
				Renderer->Deactivate();
		}
		void RenderSystem::ClearCull()
		{
			for (auto& Base : Cull)
			{
				auto* Array = Scene->GetComponents(Base);
				for (auto It = Array->Begin(); It != Array->End(); ++It)
					(*It)->As<Cullable>()->ClearCull();
			}
		}
		void RenderSystem::Synchronize(Rest::Timer* Time, const Viewer& View)
		{
			if (!FrustumCulling)
				return;

			for (auto& Base : Cull)
			{
				auto* Array = Scene->GetComponents(Base);
				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Cullable* Data = (Cullable*)*It;
					Data->Visibility = Data->Cull(View);
				}
			}
		}
		void RenderSystem::CullGeometry(Rest::Timer* Time, const Viewer& View)
		{
			if (!OcclusionCulling || !Target)
				return;

			double ElapsedTime = Time->GetElapsedTime();
			if (Sorting.TickEvent(ElapsedTime))
			{
				for (auto& Base : Cull)
					Scene->SortOpaqueFrontToBack(Base);
			}

			if (!Occlusion.TickEvent(ElapsedTime))
				return;

			Device->SetSamplerState(Sampler, 0);
			Device->SetDepthStencilState(DepthStencil);
			Device->SetBlendState(Blend);
			Device->SetTarget(Target);
			Device->ClearDepth(Target);

			for (auto& Renderer : Renderers)
			{
				if (Renderer->Active)
					Renderer->CullGeometry(View);
			}
		}
		void RenderSystem::MoveRenderer(uint64_t Id, int64_t Offset)
		{
			if (Offset == 0)
				return;

			for (int64_t i = 0; i < Renderers.size(); i++)
			{
				if (Renderers[i]->GetId() != Id)
					continue;

				if (i + Offset < 0 || i + Offset >= Renderers.size())
					return;

				Renderer* Swap = Renderers[i + Offset];
				Renderers[i + Offset] = Renderers[i];
				Renderers[i] = Swap;
				return;
			}
		}
		void RenderSystem::RemoveRenderer(uint64_t Id)
		{
			for (auto It = Renderers.begin(); It != Renderers.end(); It++)
			{
				if (*It && (*It)->GetId() == Id)
				{
					(*It)->Deactivate();
					delete *It;
					Renderers.erase(It);
					break;
				}
			}
		}
		void RenderSystem::RestoreOutput()
		{
			if (Scene != nullptr)
				Scene->SetMRT(TargetType_Main, false);
		}
		void RenderSystem::FreeShader(const std::string& Name, Graphics::Shader* Shader)
		{
			ShaderCache* Cache = (Scene ? Scene->GetShaders() : nullptr);
			if (Cache != nullptr)
			{
				if (Cache->Has(Name))
					return;
			}

			TH_RELEASE(Shader);
		}
		void RenderSystem::FreeShader(Graphics::Shader* Shader)
		{
			ShaderCache* Cache = (Scene ? Scene->GetShaders() : nullptr);
			if (Cache != nullptr)
				return FreeShader(Cache->Find(Shader), Shader);

			TH_RELEASE(Shader);
		}
		void RenderSystem::FreeBuffers(const std::string& Name, Graphics::ElementBuffer** Buffers)
		{
			if (!Buffers)
				return;

			PrimitiveCache* Cache = (Scene ? Scene->GetPrimitives() : nullptr);
			if (Cache != nullptr)
			{
				if (Cache->Has(Name))
					return;
			}

			TH_RELEASE(Buffers[0]);
			TH_RELEASE(Buffers[1]);
		}
		void RenderSystem::FreeBuffers(Graphics::ElementBuffer** Buffers)
		{
			if (!Buffers)
				return;

			PrimitiveCache* Cache = (Scene ? Scene->GetPrimitives() : nullptr);
			if (Cache != nullptr)
				return FreeBuffers(Cache->Find(Buffers), Buffers);

			TH_RELEASE(Buffers[0]);
			TH_RELEASE(Buffers[1]);
		}
		bool RenderSystem::PassCullable(Cullable* Base, CullResult Mode, float* Result)
		{
			if (Mode == CullResult_Last)
				return Base->Visibility;

			float D = Base->Cull(Scene->View);
			if (Mode == CullResult_Cache)
				Base->Visibility = D;

			if (Result != nullptr)
				*Result = D;

			return D > 0.0f;
		}
		bool RenderSystem::PassDrawable(Drawable* Base, CullResult Mode, float* Result)
		{
			if (Mode == CullResult_Last)
			{
				if (OcclusionCulling)
				{
					int R = Base->Fetch(this);
					if (R != -1)
						return R > 0;
				}

				if (!Base->GetFragmentsCount())
					return false;
			}

			return PassCullable(Base, Mode, Result);
		}
		bool RenderSystem::HasOcclusionCulling()
		{
			return OcclusionCulling;
		}
		bool RenderSystem::HasFrustumCulling()
		{
			return FrustumCulling;
		}
		int64_t RenderSystem::GetOffset(uint64_t Id)
		{
			for (size_t i = 0; i < Renderers.size(); i++)
			{
				if (Renderers[i]->GetId() == Id)
					return (int64_t)i;
			}

			return -1;
		}
		Graphics::Shader* RenderSystem::CompileShader(const std::string& Name, Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			if (Name.empty() && Desc.Filename.empty())
			{
				TH_ERROR("shader must have name or filename");
				return nullptr;
			}

			Desc.Filename = Name;
			ShaderCache* Cache = (Scene ? Scene->GetShaders() : nullptr);
			if (Cache != nullptr)
				return Cache->Compile(Name.empty() ? Desc.Filename : Name, Desc, BufferSize);

			Graphics::Shader* Shader = Device->CreateShader(Desc);
			if (BufferSize > 0)
				Device->UpdateBufferSize(Shader, BufferSize);

			return Shader;
		}
		Graphics::Shader* RenderSystem::CompileShader(const std::string& SectionName, size_t BufferSize)
		{
			Graphics::Shader::Desc I = Graphics::Shader::Desc();
			if (!Device->GetSection(SectionName, &I.Data))
				return nullptr;

			return CompileShader(SectionName, I, BufferSize);
		}
		bool RenderSystem::CompileBuffers(Graphics::ElementBuffer** Result, const std::string& Name, size_t ElementSize, size_t ElementsCount)
		{
			if (Name.empty() || !Result)
			{
				TH_ERROR("buffers must have a name");
				return false;
			}

			PrimitiveCache* Cache = (Scene ? Scene->GetPrimitives() : nullptr);
			if (Cache != nullptr)
				return Cache->Compile(Result, Name, ElementSize, ElementsCount);

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Write;
			F.Usage = Graphics::ResourceUsage_Dynamic;
			F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
			F.ElementWidth = ElementSize;
			F.ElementCount = ElementsCount;

			Graphics::ElementBuffer* VertexBuffer = Device->CreateElementBuffer(F);
			if (!VertexBuffer)
				return false;

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Write;
			F.Usage = Graphics::ResourceUsage_Dynamic;
			F.BindFlags = Graphics::ResourceBind_Index_Buffer;
			F.ElementWidth = sizeof(int);
			F.ElementCount = ElementsCount * 3;

			Graphics::ElementBuffer* IndexBuffer = Device->CreateElementBuffer(F);
			if (!IndexBuffer)
			{
				TH_RELEASE(VertexBuffer);
				return false;
			}

			Result[BufferType_Index] = IndexBuffer;
			Result[BufferType_Vertex] = VertexBuffer;

			return true;
		}
		Renderer* RenderSystem::AddRenderer(Renderer* In)
		{
			if (!In)
				return nullptr;

			for (auto It = Renderers.begin(); It != Renderers.end(); It++)
			{
				if (*It && (*It)->GetId() == In->GetId())
				{
					if (*It == In)
						return In;

					(*It)->Deactivate();
					delete (*It);
					Renderers.erase(It);
					break;
				}
			}

			In->SetRenderer(this);
			In->Activate();
			In->ResizeBuffers();
			Renderers.push_back(In);

			return In;
		}
		Renderer* RenderSystem::GetRenderer(uint64_t Id)
		{
			for (auto& RenderStage : Renderers)
			{
				if (RenderStage->GetId() == Id)
					return RenderStage;
			}

			return nullptr;
		}
		Rest::Pool<Component*>* RenderSystem::GetSceneComponents(uint64_t Section)
		{
			return Scene ? Scene->GetComponents(Section) : nullptr;
		}
		size_t RenderSystem::GetDepthSize()
		{
			return DepthSize;
		}
		std::vector<Renderer*>* RenderSystem::GetRenderers()
		{
			return &Renderers;
		}
		Graphics::MultiRenderTarget2D* RenderSystem::GetMRT(TargetType Type)
		{
			if (!Scene)
				return nullptr;

			return Scene->GetMRT(Type);
		}
		Graphics::RenderTarget2D* RenderSystem::GetRT(TargetType Type)
		{
			if (!Scene)
				return nullptr;

			return Scene->GetRT(Type);
		}
		Graphics::GraphicsDevice* RenderSystem::GetDevice()
		{
			return Device;
		}
		Graphics::Texture2D** RenderSystem::GetMerger()
		{
			if (!Scene)
				return nullptr;

			return Scene->GetMerger();
		}
		PrimitiveCache* RenderSystem::GetPrimitives()
		{
			if (!Scene)
				return nullptr;

			return Scene->GetPrimitives();
		}
		SceneGraph* RenderSystem::GetScene()
		{
			return Scene;
		}

		GeometryDraw::GeometryDraw(RenderSystem* Lab, uint64_t Hash) : Renderer(Lab), Source(Hash)
		{
		}
		GeometryDraw::~GeometryDraw()
		{
		}
		void GeometryDraw::CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry)
		{
		}
		void GeometryDraw::CullGeometry(const Viewer& View)
		{
			Rest::Pool<Drawable*>* Opaque = GetOpaque();
			if (Opaque != nullptr && Opaque->Size() > 0)
				CullGeometry(View, Opaque);
		}
		void GeometryDraw::Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options)
		{
			if (State == RenderState_Geometry_Result)
			{
				Rest::Pool<Drawable*>* Geometry;
				if (Options & RenderOpt_Transparent)
					Geometry = GetTransparent();
				else
					Geometry = GetOpaque();

				if (Geometry != nullptr && Geometry->Size() > 0)
					RenderGeometryResult(TimeStep, Geometry, Options);
			}
			else if (State == RenderState_Geometry_Voxels)
			{
				if (Options & RenderOpt_Transparent)
					return;

				Rest::Pool<Drawable*>* Geometry = GetOpaque();
				if (Geometry != nullptr && Geometry->Size() > 0)
					RenderGeometryVoxels(TimeStep, Geometry, Options);
			}
			else if (State == RenderState_Depth_Linear)
			{
				if (!(Options & RenderOpt_Inner))
					return;

				Rest::Pool<Drawable*>* Opaque = GetOpaque();
				if (Opaque != nullptr && Opaque->Size() > 0)
					RenderDepthLinear(TimeStep, Opaque);

				Rest::Pool<Drawable*>* Transparent = GetTransparent();
				if (Transparent != nullptr && Transparent->Size() > 0)
					RenderDepthLinear(TimeStep, Transparent);
			}
			else if (State == RenderState_Depth_Cubic)
			{
				Viewer& View = System->GetScene()->View;
				if (!(Options & RenderOpt_Inner))
					return;

				Rest::Pool<Drawable*>* Opaque = GetOpaque();
				if (Opaque != nullptr && Opaque->Size() > 0)
					RenderDepthCubic(TimeStep, Opaque, View.CubicViewProjection);

				Rest::Pool<Drawable*>* Transparent = GetTransparent();
				if (Transparent != nullptr && Transparent->Size() > 0)
					RenderDepthCubic(TimeStep, Transparent, View.CubicViewProjection);
			}
		}
		Rest::Pool<Drawable*>* GeometryDraw::GetOpaque()
		{
			if (!System || !System->GetScene())
				return nullptr;

			return System->GetScene()->GetOpaque(Source);
		}
		Rest::Pool<Drawable*>* GeometryDraw::GetTransparent()
		{
			if (!System || !System->GetScene())
				return nullptr;

			return System->GetScene()->GetTransparent(Source);
		}

		EffectDraw::EffectDraw(RenderSystem* Lab) : Renderer(Lab)
		{
			DepthStencil = Lab->GetDevice()->GetDepthStencilState("none");
			Rasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
			Blend = Lab->GetDevice()->GetBlendState("overwrite-opaque");
			Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
			Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");
		}
		EffectDraw::~EffectDraw()
		{
			for (auto It = Shaders.begin(); It != Shaders.end(); It++)
				System->FreeShader(It->first, It->second);
		}
		void EffectDraw::RenderMerge(Graphics::Shader* Effect, void* Buffer)
		{
			if (!Effect)
				Effect = Shaders.begin()->second;

			Graphics::RenderTarget2D* Output = System->GetRT(TargetType_Main);
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();
			Device->SetTexture2D(*Merger, 5);
			Device->SetShader(Effect, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			}

			Device->Draw(6, 0);
			Device->CopyTexture2D(Output, 0, Merger);
		}
		void EffectDraw::RenderResult(Graphics::Shader* Effect, void* Buffer)
		{
			if (!Effect)
				Effect = Shaders.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture2D(*System->GetMerger(), 5);
			Device->SetShader(Effect, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			}

			Device->Draw(6, 0);
		}
		void EffectDraw::RenderEffect(Rest::Timer* Time)
		{
		}
		void EffectDraw::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
		{
			if (State != RenderState_Geometry_Result || Options & RenderOpt_Inner)
				return;

			if (Shaders.empty())
				return;

			Graphics::MultiRenderTarget2D* Input = System->GetMRT(TargetType_Main);
			Graphics::RenderTarget2D* Output = System->GetRT(TargetType_Main);
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetSamplerState(Sampler, 0);
			Device->SetDepthStencilState(DepthStencil);
			Device->SetBlendState(Blend);
			Device->SetRasterizerState(Rasterizer);
			Device->SetInputLayout(Layout);
			Device->SetTarget(Output, 0, 0, 0, 0);
			Device->SetTexture2D(Input->GetTarget(0), 1);
			Device->SetTexture2D(Input->GetTarget(1), 2);
			Device->SetTexture2D(Input->GetTarget(2), 3);
			Device->SetTexture2D(Input->GetTarget(3), 4);
			Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);

			RenderEffect(Time);

			Device->FlushTexture2D(1, 4);
			Device->CopyTarget(Output, 0, Input, 0);
		}
		Graphics::Shader* EffectDraw::GetEffect(const std::string& Name)
		{
			auto It = Shaders.find(Name);
			if (It != Shaders.end())
				return It->second;

			return nullptr;
		}
		Graphics::Shader* EffectDraw::CompileEffect(const std::string& Name, const std::string& Code, size_t BufferSize)
		{
			if (Name.empty())
			{
				TH_ERROR("cannot compile unnamed shader source");
				return nullptr;
			}

			Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
			Desc.Data = Code;

			Graphics::Shader* Shader = System->CompileShader(Name, Desc, BufferSize);
			if (!Shader)
				return nullptr;

			auto It = Shaders.find(Name);
			if (It != Shaders.end())
			{
				delete It->second;
				It->second = Shader;
			}
			else
				Shaders[Name] = Shader;

			return Shader;
		}
		Graphics::Shader* EffectDraw::CompileEffect(const std::string& SectionName, size_t BufferSize)
		{
			std::string Data;
			if (!System->GetDevice()->GetSection(SectionName, &Data))
				return nullptr;

			return CompileEffect(SectionName, Data, BufferSize);
		}
		unsigned int EffectDraw::GetMipLevels()
		{
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType_Main);
			return System->GetDevice()->GetMipLevel(RT->GetWidth(), RT->GetHeight());
		}
		unsigned int EffectDraw::GetWidth()
		{
			return System->GetRT(TargetType_Main)->GetWidth();
		}
		unsigned int EffectDraw::GetHeight()
		{
			return System->GetRT(TargetType_Main)->GetHeight();
		}

		SceneGraph::SceneGraph(const Desc& I) : Conf(I), Active(true), Invoked(false), Structure(nullptr), Simulator(nullptr), Camera(nullptr), Primitives(nullptr), Shaders(nullptr)
		{
			Sync.Count = 0; Sync.Locked = false;
			for (unsigned int i = 0; i < ThreadId_Count; i++)
				Sync.Threads[i].State = 0;

			for (unsigned int i = 0; i < TargetType_Count * 2; i++)
			{
				Display.MRT[i] = nullptr;
				Display.RT[i] = nullptr;
			}

			Display.Merger = nullptr;
			Display.DepthStencil = nullptr;
			Display.Rasterizer = nullptr;
			Display.Blend = nullptr;
			Display.Sampler = nullptr;
			Display.Layout = nullptr;

			Materials.reserve(16);
			Configure(I);

			Simulator = new Compute::Simulator(I.Simulator);
			ExpandMaterialStructure();
		}
		SceneGraph::~SceneGraph()
		{
			Dispatch();
			Lock();

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
				TH_RELEASE(*It);

			TH_RELEASE(Display.Merger);
			for (unsigned int i = 0; i < TargetType_Count; i++)
			{
				TH_RELEASE(Display.MRT[i]);
				TH_RELEASE(Display.RT[i]);
			}

			TH_RELEASE(Structure);
			TH_RELEASE(Simulator);
			TH_RELEASE(Shaders);
			TH_RELEASE(Primitives);

			Unlock();
		}
		void SceneGraph::Configure(const Desc& NewConf)
		{
			if (!Conf.Queue || !Conf.Device)
				return;

			Display.DepthStencil = Conf.Device->GetDepthStencilState("none");
			Display.Rasterizer = Conf.Device->GetRasterizerState("cull-back");
			Display.Blend = Conf.Device->GetBlendState("overwrite");
			Display.Sampler = Conf.Device->GetSamplerState("trilinear-x16");
			Display.Layout = Conf.Device->GetInputLayout("shape-vertex");

			Lock();
			Conf = NewConf;
			Entities.Reserve(Conf.EntityCount);
			Pending.Reserve(Conf.ComponentCount);

			TH_RELEASE(Shaders);
			Shaders = new ShaderCache(Conf.Device);

			TH_RELEASE(Primitives);
			Primitives = new PrimitiveCache(Conf.Device);

			for (auto& Array : Components)
				Array.second.Reserve(Conf.ComponentCount);

			for (auto& Array : Drawables)
			{
				Array.second.Opaque.Reserve(Conf.ComponentCount);
				Array.second.Transparent.Reserve(Conf.ComponentCount);
			}

			ResizeBuffers();
			if (Camera != nullptr)
				Camera->Awake(Camera);

			Rest::Pool<Component*>* Cameras = GetComponents<Components::Camera>();
			if (Cameras != nullptr)
			{
				for (auto It = Cameras->Begin(); It != Cameras->End(); It++)
				{
					Components::Camera* Base = (*It)->As<Components::Camera>();
					Base->GetRenderer()->Remount();
				}
			}

			Conf.Queue->Subscribe<Event>([this](Rest::EventQueue*, Rest::EventArgs* Args)
			{
				Event* Message = Args->Get<Event>();
				if (Message != nullptr)
				{
					Sync.Events.lock();
					Events.push_back(Message);
					Sync.Events.unlock();
				}
			});
			Unlock();
		}
		void SceneGraph::Submit()
		{
			if (!View.Renderer)
				return;

			Conf.Device->SetTarget();
			Conf.Device->Render.Diffuse = 1.0f;
			Conf.Device->Render.WorldViewProjection.Identify();
			Conf.Device->SetSamplerState(Display.Sampler, 0);
			Conf.Device->SetDepthStencilState(Display.DepthStencil);
			Conf.Device->SetBlendState(Display.Blend);
			Conf.Device->SetRasterizerState(Display.Rasterizer);
			Conf.Device->SetInputLayout(Display.Layout);
			Conf.Device->SetShader(Conf.Device->GetBasicEffect(), Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			Conf.Device->SetVertexBuffer(View.Renderer->GetPrimitives()->GetQuad(), 0);
			Conf.Device->SetTexture2D(Display.MRT[TargetType_Main]->GetTarget(0), 1);
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType_Render);
			Conf.Device->Draw(6, 0);
			Conf.Device->SetTexture2D(nullptr, 1);
		}
		void SceneGraph::Render(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Render);
			if (Camera != nullptr)
			{
				RestoreViewBuffer(nullptr);
				Conf.Device->UpdateBuffer(Structure, Materials.data(), Materials.size() * sizeof(Material));
				Conf.Device->SetStructureBuffer(Structure, 0);

				SetMRT(TargetType_Main, true);
				Render(Time, RenderState_Geometry_Result, RenderOpt_None);
				View.Renderer->CullGeometry(Time, View);
			}
			EndThread(ThreadId_Render);
		}
		void SceneGraph::Render(Rest::Timer* Time, RenderState Stage, RenderOpt Options)
		{
			if (!View.Renderer)
				return;

			auto* States = View.Renderer->GetRenderers();
			for (auto& Renderer : *States)
			{
				if (Renderer->Active)
					Renderer->Render(Time, Stage, Options);
			}
		}
		void SceneGraph::Simulation(Rest::Timer* Time)
		{
			if (!Active)
				return;

			BeginThread(ThreadId_Simulation);
			Simulator->Simulate((float)Time->GetTimeStep());
			EndThread(ThreadId_Simulation);
		}
		void SceneGraph::Synchronize(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Synchronize);
			for (auto It = Pending.Begin(); It != Pending.End(); It++)
				(*It)->Synchronize(Time);

			uint64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				Entity* Base = *It;
				Base->Transform->Synchronize();
				Base->Id = Index; Index++;
			}
			EndThread(ThreadId_Synchronize);
		}
		void SceneGraph::Update(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Update);
			if (Active)
			{
				for (auto It = Pending.Begin(); It != Pending.End(); It++)
					(*It)->Update(Time);
			}

			Compute::Vector3& Far = View.WorldPosition;
			if (Camera != nullptr)
				Far = Camera->Parent->Transform->Position;

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
				(*It)->Distance = (*It)->Transform->Position.Distance(Far);

			DispatchLastEvent();
			EndThread(ThreadId_Update);
		}
		void SceneGraph::Actualize()
		{
			Redistribute();
			Dispatch();
			Reindex();
		}
		void SceneGraph::Redistribute()
		{
			Lock();
			for (auto& Component : Components)
				Component.second.Clear();

			Pending.Clear();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
				RegisterEntity(*It);

			GetCamera();
			Unlock();
		}
		void SceneGraph::Reindex()
		{
			Lock();
			int64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				(*It)->Id = Index;
				Index++;
			}
			Unlock();
		}
		void SceneGraph::SortOpaqueBackToFront(uint64_t Section)
		{
			SortOpaqueBackToFront(GetOpaque(Section));
		}
		void SceneGraph::SortOpaqueBackToFront(Rest::Pool<Drawable*>* Array)
		{
			if (!Array)
				return;

			std::sort(Array->Begin(), Array->End(), [](Component* A, Component* B)
			{
				return A->Parent->Distance > B->Parent->Distance;
			});
		}
		void SceneGraph::SortOpaqueFrontToBack(uint64_t Section)
		{
			SortOpaqueFrontToBack(GetOpaque(Section));
		}
		void SceneGraph::SortOpaqueFrontToBack(Rest::Pool<Drawable*>* Array)
		{
			if (!Array)
				return;

			std::sort(Array->Begin(), Array->End(), [](Component* A, Component* B)
			{
				return A->Parent->Distance < B->Parent->Distance;
			});
		}
		void SceneGraph::SetCamera(Entity* NewCamera)
		{
			Components::Camera* Target = nullptr;
			if (NewCamera != nullptr)
			{
				Target = NewCamera->GetComponent<Components::Camera>();
				if (Target != nullptr && Target->Active)
				{
					if (Camera != nullptr)
						Camera->Awake(Camera);
					Target->Awake(nullptr);
				}
				else
					Target = nullptr;
			}

			Lock();
			Camera = Target;
			Unlock();
		}
		void SceneGraph::RemoveEntity(Entity* Entity, bool Release)
		{
			if (!Entity)
				return;

			Dispatch();
			if (!UnregisterEntity(Entity) || !Release)
				return;
			else
				Entity->RemoveChilds();

			Lock();
			TH_RELEASE(Entity);
			Unlock();
		}
		void SceneGraph::RemoveMaterial(uint64_t Material)
		{
			if (Material >= Materials.size() || Material >= Names.size())
				return;

			Compute::PropertyArgs Args;
			Args["material-id"] = Compute::Property((int64_t)Materials[Material].Id);

			if (!DispatchEvent("materialchange", Args))
				return;

			Lock();
			Materials[Material] = Materials.back();
			Names[Material] = Names.back();

			if (!Names.empty())
				Names.resize(Names.size() - 1);

			if (!Materials.empty())
				Materials.resize(Materials.size() - 1);

			for (uint64_t i = 0; i < Materials.size(); i++)
				Materials[i].Id = (float)i;
			Unlock();
		}
		void SceneGraph::RegisterEntity(Entity* In)
		{
			if (!In)
				return;

			for (auto& Component : In->Components)
			{
				Component.second->Awake(Component.second);

				auto Storage = GetComponents(Component.second->GetId());
				if (Component.second->Active)
					Storage->AddIfNotExists(Component.second);
				else
					Storage->Remove(Component.second);

				if (Component.second->Active)
					Pending.AddIfNotExists(Component.second);
				else
					Pending.Remove(Component.second);
			}

			Mutate(In, true);
		}
		bool SceneGraph::UnregisterEntity(Entity* In)
		{
			if (Camera != nullptr && In == Camera->Parent)
			{
				Lock();
				Camera = nullptr;
				Unlock();
			}

			if (!In || !In->GetScene())
				return false;

			for (auto& Component : In->Components)
			{
				Component.second->Asleep();

				auto Storage = &Components[Component.second->GetId()];
				Storage->Remove(Component.second);
				Pending.Remove(Component.second);
			}

			Entities.Remove(In);
			Mutate(In, false);

			return true;
		}
		void SceneGraph::CloneEntities(Entity* Instance, std::vector<Entity*>* Array)
		{
			if (!Instance || !Array)
				return;

			Lock();
			Entity* Clone = CloneEntity(Instance);
			Array->push_back(Clone);

			if (Clone->Transform->GetRoot() && Clone->Transform->GetRoot()->GetChilds())
				Clone->Transform->GetRoot()->GetChilds()->push_back(Clone->Transform);

			std::vector<Compute::Transform*>* Childs = Instance->Transform->GetChilds();
			if (Instance->Transform->GetChildCount() <= 0)
				return Unlock();

			Clone->Transform->GetChilds()->clear();
			for (uint64_t i = 0; i < Childs->size(); i++)
			{
				uint64_t Offset = Array->size();
				CloneEntities((*Childs)[i]->Ptr<Entity>(), Array);
				for (uint64_t j = Offset; j < Array->size(); j++)
				{
					if ((*Array)[j]->Transform->GetRoot() == Instance->Transform)
						(*Array)[j]->Transform->SetRoot(Clone->Transform);
				}
			}

			Unlock();
		}
		void SceneGraph::RestoreViewBuffer(Viewer* iView)
		{
			if (&View != iView)
			{
				if (iView == nullptr)
				{
					if (Camera != nullptr)
						Camera->As<Components::Camera>()->GetViewer(&View);
				}
				else
					View = *iView;
			}

			Conf.Device->View.InvViewProjection = View.InvViewProjection;
			Conf.Device->View.ViewProjection = View.ViewProjection;
			Conf.Device->View.Projection = View.Projection;
			Conf.Device->View.View = View.View;
			Conf.Device->View.ViewPosition = View.InvViewPosition;
			Conf.Device->View.ViewDirection = View.WorldRotation.DepthDirection();
			Conf.Device->View.FarPlane = View.FarPlane;
			Conf.Device->View.NearPlane = View.NearPlane;
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType_View);
		}
		void SceneGraph::ExpandMaterialStructure()
		{
			Lock();

			Materials.reserve(Materials.capacity() * 2);
			for (size_t i = 0; i < Materials.size(); i++)
				Materials[i].Id = (float)i;

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Write;
			F.MiscFlags = Graphics::ResourceMisc_Buffer_Structured;
			F.Usage = Graphics::ResourceUsage_Dynamic;
			F.BindFlags = Graphics::ResourceBind_Shader_Input;
			F.ElementCount = (unsigned int)Materials.capacity();
			F.ElementWidth = sizeof(Material);
			F.Elements = Materials.data();
			F.StructureByteStride = F.ElementWidth;

			TH_RELEASE(Structure);
			Structure = Conf.Device->CreateElementBuffer(F);
			Unlock();
		}
		void SceneGraph::Lock()
		{
			auto Id = std::this_thread::get_id();
			if (Sync.Id == Id)
			{
				Sync.Count++;
				return;
			}
			else if (Sync.Locked && Sync.Id != std::thread::id())
				return;

			Sync.Safe.lock();
			Sync.Id = Id;
			Sync.Locked = true;

			std::unique_lock<std::mutex> Timeout(Sync.Await);
			for (auto& i : Sync.Threads)
			{
				Thread* Entry = &i;
				if (Entry->Id != std::thread::id() && Entry->Id != Id)
				{
					Entry->State = 1;
					Sync.Condition.wait(Timeout);
					Entry->State = 2;
				}
			}
		}
		void SceneGraph::Unlock()
		{
			auto Id = std::this_thread::get_id();
			if (Sync.Id == Id)
			{
				if (Sync.Count > 0)
				{
					Sync.Count--;
					return;
				}
			}
			else if (Sync.Locked)
				return;

			Sync.Locked = false;
			Sync.Id = std::thread::id();
			Sync.Safe.unlock();
			Sync.Callback.notify_all();
		}
		void SceneGraph::BeginThread(ThreadId Id)
		{
			Thread* Entry = &Sync.Threads[(uint64_t)Id];
			if (Sync.Locked && Entry->State != 0)
			{
				if (Entry->Id == Sync.Id)
					return;

				while (Entry->State == 1)
					Sync.Condition.notify_all();

				std::unique_lock<std::mutex> Timeout(Sync.Global);
				Sync.Callback.wait(Timeout);
				Entry->State = 0;
			}

			Entry->Id = std::this_thread::get_id();
		}
		void SceneGraph::EndThread(ThreadId Id)
		{
			Thread* Entry = &Sync.Threads[(uint64_t)Id];
			Entry->Id = std::thread::id();
		}
		void SceneGraph::Dispatch()
		{
			if (Conf.Queue != nullptr)
				while (Conf.Queue->Pull<Event>(Rest::EventType_Events));

			while (DispatchLastEvent());
		}
		void SceneGraph::ResizeBuffers()
		{
			if (!Camera)
				return ResizeRenderBuffers();

			Lock();
			ResizeRenderBuffers();

			auto* Array = GetComponents<Components::Camera>();
			for (auto It = Array->Begin(); It != Array->End(); It++)
				(*It)->As<Components::Camera>()->ResizeBuffers();
			Unlock();
		}
		void SceneGraph::ResizeRenderBuffers()
		{
			Graphics::MultiRenderTarget2D::Desc MRT = GetDescMRT();
			Graphics::RenderTarget2D::Desc RT = GetDescRT();
			TH_CLEAR(Display.Merger);

			for (unsigned int i = 0; i < TargetType_Count; i++)
			{
				TH_RELEASE(Display.MRT[i]);
				Display.MRT[i] = Conf.Device->CreateMultiRenderTarget2D(MRT);

				TH_RELEASE(Display.RT[i]);
				Display.RT[i] = Conf.Device->CreateRenderTarget2D(RT);
			}
		}
		void SceneGraph::RayTest(uint64_t Section, const Compute::Ray& Origin, float MaxDistance, const RayCallback& Callback)
		{
			if (!Callback)
				return;

			Rest::Pool<Component*>* Array = GetComponents(Section);
			Compute::Ray Base = Origin;

			for (auto It = Array->Begin(); It != Array->End(); It++)
			{
				Component* Current = *It;
				if (MaxDistance > 0.0f && Current->Parent->Distance > MaxDistance)
					continue;

				if (Compute::Common::CursorRayTest(Base, Current->GetBoundingBox()) && !Callback(Current))
					break;
			}
		}
		void SceneGraph::ScriptHook(const std::string& Name)
		{
			auto* Array = GetComponents<Components::Scriptable>();
			for (auto It = Array->Begin(); It != Array->End(); It++)
			{
				Components::Scriptable* Base = (Components::Scriptable*)*It;
				Base->CallEntry(Name);
			}
		}
		void SceneGraph::SetActive(bool Enabled)
		{
			Active = Enabled;
			if (!Active)
				return;

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				Entity* V = *It;
				for (auto& Base : V->Components)
				{
					if (Base.second->Active)
						Base.second->Awake(nullptr);
				}
			}
		}
		void SceneGraph::SetView(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Near, float _Far, bool Upload)
		{
			View.Set(_View, _Projection, _Position, _Near, _Far);
			if (Upload)
				RestoreViewBuffer(&View);
		}
		void SceneGraph::SetMaterialName(uint64_t Material, const std::string& Name)
		{
			if (Material >= Names.size())
				return;

			Names[Material] = Name;
		}
		void SceneGraph::SetMRT(TargetType Type, bool Clear)
		{
			Graphics::MultiRenderTarget2D* Target = Display.MRT[Type];
			Conf.Device->SetTarget(Target);

			if (!Clear)
				return;

			Conf.Device->Clear(Target, 0, 0, 0, 0);
			Conf.Device->Clear(Target, 1, 0, 0, 0);
			Conf.Device->Clear(Target, 2, 1, 0, 0);
			Conf.Device->Clear(Target, 3, 0, 0, 0);
			Conf.Device->ClearDepth(Target);
		}
		void SceneGraph::SetRT(TargetType Type, bool Clear)
		{
			Graphics::RenderTarget2D* Target = Display.RT[Type];
			Conf.Device->SetTarget(Target);

			if (!Clear)
				return;

			Conf.Device->Clear(Target, 0, 0, 0, 0);
			Conf.Device->ClearDepth(Target);
		}
		void SceneGraph::SwapMRT(TargetType Type, Graphics::MultiRenderTarget2D* New)
		{
			if (Display.MRT[Type] == New)
				return;

			Graphics::MultiRenderTarget2D* Cache = Display.MRT[Type + TargetType_Count];
			if (New != nullptr)
			{
				Graphics::MultiRenderTarget2D* Base = Display.MRT[Type];
				Display.MRT[Type] = New;

				if (!Cache)
					Display.MRT[Type + TargetType_Count] = Base;
			}
			else if (Cache != nullptr)
			{
				Display.MRT[Type] = Cache;
				Display.MRT[Type + TargetType_Count] = nullptr;
			}
		}
		void SceneGraph::SwapRT(TargetType Type, Graphics::RenderTarget2D* New)
		{
			Graphics::RenderTarget2D* Cache = Display.RT[Type + TargetType_Count];
			if (New != nullptr)
			{
				Graphics::RenderTarget2D* Base = Display.RT[Type];
				Display.RT[Type] = New;

				if (!Cache)
					Display.RT[Type + TargetType_Count] = Base;
			}
			else if (Cache != nullptr)
			{
				Display.RT[Type] = Cache;
				Display.RT[Type + TargetType_Count] = nullptr;
			}
		}
		void SceneGraph::ClearMRT(TargetType Type, bool Color, bool Depth)
		{
			Graphics::MultiRenderTarget2D* Target = Display.MRT[Type];
			if (Color)
			{
				Conf.Device->Clear(Target, 0, 0, 0, 0);
				Conf.Device->Clear(Target, 1, 0, 0, 0);
				Conf.Device->Clear(Target, 2, 1, 0, 0);
				Conf.Device->Clear(Target, 3, 0, 0, 0);
			}

			if (Depth)
				Conf.Device->ClearDepth(Target);
		}
		void SceneGraph::ClearRT(TargetType Type, bool Color, bool Depth)
		{
			Graphics::RenderTarget2D* Target = Display.RT[Type];
			if (Color)
				Conf.Device->Clear(Target, 0, 0, 0, 0);

			if (Depth)
				Conf.Device->ClearDepth(Target);
		}
		bool SceneGraph::AddEventListener(const std::string& Name, const std::string& EventName, const MessageCallback& Callback)
		{
			if (!Callback || Invoked)
				return false;

			Sync.Listener.lock();
			Invoked = true;

			Listeners[Name] = std::make_pair(EventName, Callback);

			Invoked = false;
			Sync.Listener.unlock();

			return true;
		}
		bool SceneGraph::RemoveEventListener(const std::string& Name)
		{
			bool Result = false;
			if (Invoked)
				return Result;

			Sync.Listener.lock();
			Invoked = true;

			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				Listeners.erase(It);
				Result = true;
			}

			Invoked = false;
			Sync.Listener.unlock();

			return Result;
		}
		bool SceneGraph::DispatchEvent(const std::string& EventName, const Compute::PropertyArgs& Args)
		{
			return Conf.Queue ? Conf.Queue->Event<Event>(new Event(EventName, this, Args)) : false;
		}
		bool SceneGraph::DispatchEvent(Component* Target, const std::string& EventName, const Compute::PropertyArgs& Args)
		{
			return Conf.Queue ? Conf.Queue->Event<Event>(new Event(EventName, Target, Args)) : false;
		}
		bool SceneGraph::DispatchEvent(Entity* Target, const std::string& EventName, const Compute::PropertyArgs& Args)
		{
			return Conf.Queue ? Conf.Queue->Event<Event>(new Event(EventName, Target, Args)) : false;
		}
		bool SceneGraph::DispatchLastEvent()
		{
			Sync.Events.lock();
			if (Events.empty())
			{
				Sync.Events.unlock();
				return false;
			}

			Event* Message = *Events.begin();
			Events.erase(Events.begin());

			bool Result = !Events.empty();
			Sync.Events.unlock();

			if (Message->TScene != nullptr)
			{
				for (auto It = Entities.Begin(); It != Entities.End(); It++)
				{
					for (auto& Item : (*It)->Components)
						Item.second->Message(Message);
				}
			}
			else if (Message->TEntity != nullptr)
			{
				for (auto& Item : Message->TEntity->Components)
					Item.second->Message(Message);
			}
			else if (Message->TComponent != nullptr)
				Message->TComponent->Message(Message);

			if (!Listeners.empty())
			{
				Sync.Listener.lock();
				for (auto& Callback : Listeners)
				{
					if (Callback.second.first == Message->Id)
						Callback.second.second(Message);
				}
				Sync.Listener.unlock();
			}

			delete Message;
			return Result;
		}
		void SceneGraph::Mutate(Entity* Target, bool Added)
		{
			Compute::PropertyArgs Args;
			Args["entity-ptr"] = Compute::Property((void*)Target);
			Args["added"] = Compute::Property(Added);

			DispatchEvent("mutation", Args);
		}
		void SceneGraph::AddDrawable(Drawable* Source, GeoCategory Category)
		{
			if (!Source)
				return;

			if (Category == GeoCategory_Opaque)
				GetOpaque(Source->Source)->Add(Source);
			else if (Category == GeoCategory_Transparent)
				GetTransparent(Source->Source)->Add(Source);
		}
		void SceneGraph::RemoveDrawable(Drawable* Source, GeoCategory Category)
		{
			if (!Source)
				return;

			if (Category == GeoCategory_Opaque)
				GetOpaque(Source->Source)->Remove(Source);
			else if (Category == GeoCategory_Transparent)
				GetTransparent(Source->Source)->Remove(Source);
		}
		Material* SceneGraph::AddMaterial(const std::string& Name, const Material& From)
		{
			Material Material = From;
			Material.Id = (float)Materials.size();

			Lock();
			Names.push_back(Name);

			if (Materials.size() + 1 < Materials.capacity())
			{
				Materials.push_back(Material);
				Unlock();
				return &Materials.back();
			}

			ExpandMaterialStructure();
			Materials.push_back(Material);
			Unlock();

			return &Materials.back();
		}
		Component* SceneGraph::GetCamera()
		{
			if (Camera != nullptr)
				return Camera;

			auto Viewer = GetComponent<Components::Camera>();
			if (Viewer != nullptr && Viewer->Active)
			{
				Lock();
				Viewer->Awake(Viewer);
				Camera = Viewer;
				Unlock();
			}
			else
			{
				auto New = new Entity(this);
				New->AddComponent<Components::Camera>();
				AddEntity(New);
				SetCamera(New);
			}

			return Camera;
		}
		Component* SceneGraph::GetComponent(uint64_t Component, uint64_t Section)
		{
			auto* Array = GetComponents(Section);
			if (Component >= Array->Size())
				return nullptr;

			return *Array->At(Component);
		}
		RenderSystem* SceneGraph::GetRenderer()
		{
			if (!Camera)
				return nullptr;

			return Camera->As<Components::Camera>()->GetRenderer();
		}
		Viewer SceneGraph::GetCameraViewer()
		{
			if (!Camera)
				return Viewer();

			return Camera->As<Components::Camera>()->GetViewer();
		}
		std::string SceneGraph::GetMaterialName(uint64_t Material)
		{
			if (Material >= Names.size())
				return "";

			return Names[Material];
		}
		Material* SceneGraph::GetMaterialByName(const std::string& Name)
		{
			for (size_t i = 0; i < Names.size(); i++)
			{
				if (Names[i] == Name)
					return i >= Materials.size() ? nullptr : &Materials[i];
			}

			return nullptr;
		}
		Material* SceneGraph::GetMaterialById(uint64_t Material)
		{
			if (Material >= Materials.size())
				return nullptr;

			return &Materials[Material];
		}
		Entity* SceneGraph::GetEntity(uint64_t Entity)
		{
			if (Entity >= Entities.Size())
				return nullptr;

			return Entities[Entity];
		}
		Entity* SceneGraph::GetLastEntity()
		{
			if (Entities.Empty())
				return nullptr;

			return Entities.Back();
		}
		Entity* SceneGraph::FindNamedEntity(const std::string& Name)
		{
			Lock();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if ((*It)->Name == Name)
				{
					Unlock();
					return *It;
				}
			}

			Unlock();
			return nullptr;
		}
		Entity* SceneGraph::FindEntityAt(const Compute::Vector3& Position, float Radius)
		{
			Lock();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if ((*It)->Transform->Position.Distance(Position) <= Radius + (*It)->Transform->Scale.Length())
				{
					Unlock();
					return *It;
				}
			}

			Unlock();
			return nullptr;
		}
		Entity* SceneGraph::FindTaggedEntity(uint64_t Tag)
		{
			Lock();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if ((*It)->Tag == Tag)
				{
					Unlock();
					return *It;
				}
			}

			Unlock();
			return nullptr;
		}
		Entity* SceneGraph::CloneEntity(Entity* Entity)
		{
			if (!Entity)
				return nullptr;

			Engine::Entity* Instance = new Engine::Entity(this);
			Instance->Transform->Copy(Entity->Transform);
			Instance->Transform->UserPointer = Instance;
			Instance->Tag = Entity->Tag;
			Instance->Name = Entity->Name;
			Instance->Id = Entity->Id;
			Instance->Components = Entity->Components;

			for (auto& It : Instance->Components)
			{
				Component* Source = It.second;
				It.second = Source->Copy(Instance);
				It.second->Parent = Instance;
				It.second->Active = Source->Active;
			}

			AddEntity(Instance);
			return Instance;
		}
		Entity* SceneGraph::CloneEntities(Entity* Value)
		{
			if (!Value)
				return nullptr;

			std::vector<Entity*> Array;
			CloneEntities(Value, &Array);

			return !Array.empty() ? Array.front() : nullptr;
		}
		Rest::Pool<Component*>* SceneGraph::GetComponents(uint64_t Section)
		{
			Rest::Pool<Component*>* Array = &Components[Section];
			if (Array->Capacity() >= Conf.ComponentCount)
				return Array;

			Lock();
			Array->Reserve(Conf.ComponentCount);
			Unlock();

			return Array;
		}
		Rest::Pool<Drawable*>* SceneGraph::GetOpaque(uint64_t Section)
		{
			Rest::Pool<Drawable*>* Array = &Drawables[Section].Opaque;
			if (Array->Capacity() >= Conf.ComponentCount)
				return Array;

			Lock();
			Array->Reserve(Conf.ComponentCount);
			Unlock();

			return Array;
		}
		Rest::Pool<Drawable*>* SceneGraph::GetTransparent(uint64_t Section)
		{
			Rest::Pool<Drawable*>* Array = &Drawables[Section].Transparent;
			if (Array->Capacity() >= Conf.ComponentCount)
				return Array;

			Lock();
			Array->Reserve(Conf.ComponentCount);
			Unlock();

			return Array;
		}
		Graphics::RenderTarget2D::Desc SceneGraph::GetDescRT()
		{
			Graphics::RenderTarget2D* Target = Conf.Device->GetRenderTarget();
			if (!Target)
				return Graphics::RenderTarget2D::Desc();

			Graphics::RenderTarget2D::Desc Desc;
			Desc.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			Desc.Width = (unsigned int)(Target->GetWidth() * Conf.RenderQuality);
			Desc.Height = (unsigned int)(Target->GetHeight() * Conf.RenderQuality);
			Desc.MipLevels = Conf.Device->GetMipLevel(Desc.Width, Desc.Height);
			Desc.FormatMode = GetFormatMRT(0);

			return Desc;
		}
		Graphics::MultiRenderTarget2D::Desc SceneGraph::GetDescMRT()
		{
			Graphics::RenderTarget2D* Target = Conf.Device->GetRenderTarget();
			if (!Target)
				return Graphics::MultiRenderTarget2D::Desc();

			Graphics::MultiRenderTarget2D::Desc Desc;
			Desc.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			Desc.Width = (unsigned int)(Target->GetWidth() * Conf.RenderQuality);
			Desc.Height = (unsigned int)(Target->GetHeight() * Conf.RenderQuality);
			Desc.MipLevels = Conf.Device->GetMipLevel(Desc.Width, Desc.Height);
			Desc.Target = Graphics::SurfaceTarget3;
			Desc.FormatMode[0] = GetFormatMRT(0);
			Desc.FormatMode[1] = GetFormatMRT(1);
			Desc.FormatMode[2] = GetFormatMRT(2);
			Desc.FormatMode[3] = GetFormatMRT(3);

			return Desc;
		}
		Graphics::Format SceneGraph::GetFormatMRT(unsigned int Target)
		{
			if (Target == 0)
				return Conf.EnableHDR ? Graphics::Format_R16G16B16A16_Unorm : Graphics::Format_R8G8B8A8_Unorm;

			if (Target == 1)
				return Graphics::Format_R16G16B16A16_Float;

			if (Target == 2)
				return Graphics::Format_R32_Float;

			if (Target == 3)
				return Graphics::Format_R8G8B8A8_Unorm;

			return Graphics::Format_Unknown;
		}
		std::vector<Entity*> SceneGraph::FindParentFreeEntities(Entity* Entity)
		{
			std::vector<Engine::Entity*> Array;
			if (!Entity)
				return Array;

			Lock();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if (*It == Entity)
					continue;

				if (!(*It)->Transform->HasRoot(Entity->Transform))
					Array.push_back(*It);
			}
			Unlock();

			return Array;
		}
		std::vector<Entity*> SceneGraph::FindNamedEntities(const std::string& Name)
		{
			std::vector<Entity*> Array;
			Lock();

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if ((*It)->Name == Name)
					Array.push_back(*It);
			}

			Unlock();
			return Array;
		}
		std::vector<Entity*> SceneGraph::FindEntitiesAt(const Compute::Vector3& Position, float Radius)
		{
			std::vector<Entity*> Array;
			Lock();

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if ((*It)->Transform->Position.Distance(Position) <= Radius + (*It)->Transform->Scale.Length())
					Array.push_back(*It);
			}

			Unlock();
			return Array;
		}
		std::vector<Entity*> SceneGraph::FindTaggedEntities(uint64_t Tag)
		{
			std::vector<Entity*> Array;
			Lock();

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				if ((*It)->Tag == Tag)
					Array.push_back(*It);
			}

			Unlock();
			return Array;
		}
		bool SceneGraph::IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection)
		{
			if (!Camera || !Entity || Entity->Transform->Position.Distance(Camera->Parent->Transform->Position) > View.FarPlane + Entity->Transform->Scale.Length())
				return false;

			return (Compute::Common::IsCubeInFrustum(Entity->Transform->GetWorld() * ViewProjection, 2) == -1);
		}
		bool SceneGraph::IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection, const Compute::Vector3& ViewPosition, float DrawDistance)
		{
			if (!Entity || Entity->Transform->Position.Distance(ViewPosition) > DrawDistance + Entity->Transform->Scale.Length())
				return false;

			return (Compute::Common::IsCubeInFrustum(Entity->Transform->GetWorld() * ViewProjection, 2) == -1);
		}
		bool SceneGraph::AddEntity(Entity* Entity)
		{
			if (!Entity)
				return false;

			Entity->SetScene(this);
			if (Entities.Add(Entity) == Entities.End())
				return false;

			RegisterEntity(Entity);
			return true;
		}
		bool SceneGraph::HasEntity(Entity* Entity)
		{
			Lock();
			for (uint64_t i = 0; i < Entities.Size(); i++)
			{
				if (Entities[i] == Entity)
				{
					Unlock();
					return i + 1;
				}
			}

			Unlock();
			return 0;
		}
		bool SceneGraph::HasEntity(uint64_t Entity)
		{
			return (Entity >= 0 && Entity < Entities.Size()) ? Entity : -1;
		}
		bool SceneGraph::IsActive()
		{
			return Active;
		}
		uint64_t SceneGraph::GetEntityCount()
		{
			return Entities.Size();
		}
		uint64_t SceneGraph::GetComponentCount(uint64_t Section)
		{
			return Components[Section].Size();
		}
		uint64_t SceneGraph::GetMaterialCount()
		{
			return Materials.size();
		}
		uint64_t SceneGraph::GetOpaqueCount()
		{
			uint64_t Count = 0;
			for (auto& Array : Drawables)
				Count += Array.second.Opaque.Size();

			return Count;
		}
		uint64_t SceneGraph::GetTransparentCount()
		{
			uint64_t Count = 0;
			for (auto& Array : Drawables)
				Count += Array.second.Transparent.Size();

			return Count;
		}
		Graphics::MultiRenderTarget2D* SceneGraph::GetMRT(TargetType Type)
		{
			return Display.MRT[Type];
		}
		Graphics::RenderTarget2D* SceneGraph::GetRT(TargetType Type)
		{
			return Display.RT[Type];
		}
		Graphics::Texture2D** SceneGraph::GetMerger()
		{
			return &Display.Merger;
		}
		Graphics::ElementBuffer* SceneGraph::GetStructure()
		{
			return Structure;
		}
		Graphics::GraphicsDevice* SceneGraph::GetDevice()
		{
			return Conf.Device;
		}
		Rest::EventQueue* SceneGraph::GetQueue()
		{
			return Conf.Queue;
		}
		ShaderCache* SceneGraph::GetShaders()
		{
			return Shaders;
		}
		PrimitiveCache* SceneGraph::GetPrimitives()
		{
			return Primitives;
		}
		Compute::Simulator* SceneGraph::GetSimulator()
		{
			return Simulator;
		}
		SceneGraph::Desc& SceneGraph::GetConf()
		{
			return Conf;
		}

		ContentManager::ContentManager(Graphics::GraphicsDevice* NewDevice, Rest::EventQueue* NewQueue) : Device(NewDevice), Queue(NewQueue)
		{
			Base = Rest::OS::ResolveDir(Rest::OS::GetDirectory().c_str());
			SetEnvironment(Base);
		}
		ContentManager::~ContentManager()
		{
			InvalidateCache();
			InvalidateDockers();

			for (auto It = Streams.begin(); It != Streams.end(); It++)
				delete It->first;

			for (auto It = Processors.begin(); It != Processors.end(); It++)
				delete It->second;
		}
		void ContentManager::InvalidateDockers()
		{
			Mutex.lock();
			for (auto It = Dockers.begin(); It != Dockers.end(); It++)
				delete It->second;

			Dockers.clear();
			Mutex.unlock();
		}
		void ContentManager::InvalidateCache()
		{
			Mutex.lock();
			for (auto& Entries : Assets)
			{
				for (auto& Entry : Entries.second)
				{
					if (!Entry.second)
						continue;

					Mutex.unlock();
					if (Entry.first != nullptr)
						Entry.first->Free(Entry.second);
					Mutex.lock();

					delete Entry.second;
				}
			}

			Assets.clear();
			Mutex.unlock();
		}
		void ContentManager::InvalidatePath(const std::string& Path)
		{
			std::string File = Rest::OS::Resolve(Path, Environment);
			Mutex.lock();

			auto It = Assets.find(Rest::Stroke(File).Replace(Environment, "./").Replace('\\', '/').R());
			if (It != Assets.end())
				Assets.erase(It);

			Mutex.unlock();
		}
		void ContentManager::SetEnvironment(const std::string& Path)
		{
			Mutex.lock();
			Environment = Rest::OS::ResolveDir(Path.c_str());
			Rest::OS::SetDirectory(Environment.c_str());
			Mutex.unlock();
		}
		void* ContentManager::LoadForward(const std::string& Path, Processor* Processor, const Compute::PropertyArgs& Map)
		{
			if (Path.empty())
				return nullptr;

			if (!Processor)
			{
				TH_ERROR("file processor for \"%s\" wasn't found", Path.c_str());
				return nullptr;
			}

			void* Object = LoadStreaming(Path, Processor, Map);
			if (Object != nullptr)
				return Object;

			Mutex.lock();
			std::string File = Path;
			if (!Rest::OS::FileRemote(Path.c_str()))
			{
				File = Rest::OS::Resolve(Path, Environment);
				if (!Rest::OS::FileExists(File.c_str()))
				{
					if (!Rest::OS::FileExists(Path.c_str()))
					{
						Mutex.unlock();
						TH_ERROR("file \"%s\" wasn't found", File.c_str());
						return nullptr;
					}

					File = Path;
				}
			}
			Mutex.unlock();

			AssetCache* Asset = Find(Processor, Path);
			if (Asset != nullptr)
				return Processor->Duplicate(Asset, Map);

			auto* Stream = Rest::OS::Open(File, Rest::FileMode_Binary_Read_Only);
			if (!Stream)
				return nullptr;

			Object = Processor->Deserialize(Stream, Stream->GetSize(), 0, Map);
			TH_RELEASE(Stream);

			return Object;
		}
		void* ContentManager::LoadStreaming(const std::string& Path, Processor* Processor, const Compute::PropertyArgs& Map)
		{
			if (Path.empty())
				return nullptr;

			auto Docker = Dockers.find(Rest::Stroke(Path).Replace('\\', '/').Replace("./", "").R());
			if (Docker == Dockers.end() || !Docker->second || !Docker->second->Stream)
				return nullptr;

			AssetCache* Asset = Find(Processor, Path);
			if (Asset != nullptr)
				return Processor->Duplicate(Asset, Map);

			auto It = Streams.find(Docker->second->Stream);
			if (It == Streams.end())
			{
				TH_ERROR("cannot resolve stream offset for \"%s\"", Path.c_str());
				return nullptr;
			}

			auto* Stream = Docker->second->Stream;
			Stream->Seek(Rest::FileSeek_Begin, It->second + Docker->second->Offset);
			Stream->GetSource() = Path;

			return Processor->Deserialize(Stream, Docker->second->Length, It->second + Docker->second->Offset, Map);
		}
		bool ContentManager::SaveForward(const std::string& Path, Processor* Processor, void* Object, const Compute::PropertyArgs& Map)
		{
			if (Path.empty())
				return false;

			if (!Processor)
			{
				TH_ERROR("file processor for \"%s\" wasn't found", Path.c_str());
				return false;
			}

			if (!Object)
			{
				TH_ERROR("cannot save null object to \"%s\"", Path.c_str());
				return false;
			}

			Mutex.lock();
			std::string Directory = Rest::OS::FileDirectory(Path);
			std::string File = Rest::OS::Resolve(Directory, Environment);
			File.append(Path.substr(Directory.size()));
			Mutex.unlock();

			auto* Stream = Rest::OS::Open(File, Rest::FileMode_Binary_Write_Only);
			if (!Stream)
			{
				Stream = Rest::OS::Open(Path, Rest::FileMode_Binary_Write_Only);
				if (!Stream)
				{
					TH_ERROR("cannot open stream for writing at \"%s\" or \"%s\"", File.c_str(), Path.c_str());
					TH_RELEASE(Stream);
					return false;
				}
			}

			bool Result = Processor->Serialize(Stream, Object, Map);
			TH_RELEASE(Stream);

			return Result;
		}
		bool ContentManager::Import(const std::string& Path)
		{
			Mutex.lock();
			std::string File = Rest::OS::Resolve(Path, Environment);
			if (!Rest::OS::FileExists(File.c_str()))
			{
				if (!Rest::OS::FileExists(Path.c_str()))
				{
					TH_ERROR("file \"%s\" wasn't found", Path.c_str());
					return false;
				}

				File = Path;
			}
			Mutex.unlock();

			auto* Stream = new Rest::GzStream();
			if (!Stream->Open(File.c_str(), Rest::FileMode::FileMode_Binary_Read_Only))
			{
				TH_ERROR("cannot open \"%s\" for reading", File.c_str());
				TH_RELEASE(Stream);

				return false;
			}

			char Buffer[16];
			if (Stream->Read(Buffer, 16) != 16)
			{
				TH_ERROR("file \"%s\" has corrupted header", File.c_str());
				TH_RELEASE(Stream);

				return false;
			}

			if (memcmp(Buffer, "\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16) != 0)
			{
				TH_ERROR("file \"%s\" header version is corrupted", File.c_str());
				TH_RELEASE(Stream);

				return false;
			}

			uint64_t Size = 0;
			if (Stream->Read((char*)&Size, sizeof(uint64_t)) != sizeof(uint64_t))
			{
				TH_ERROR("file \"%s\" has corrupted dock size", File.c_str());
				TH_RELEASE(Stream);

				return false;
			}

			Mutex.lock();
			for (uint64_t i = 0; i < Size; i++)
			{
				AssetArchive* Docker = new AssetArchive();
				Docker->Stream = Stream;

				uint64_t Length;
				Stream->Read((char*)&Length, sizeof(uint64_t));
				Stream->Read((char*)&Docker->Offset, sizeof(uint64_t));
				Stream->Read((char*)&Docker->Length, sizeof(uint64_t));

				if (!Length)
				{
					delete Docker;
					continue;
				}

				Docker->Path.resize(Length);
				Stream->Read((char*)Docker->Path.c_str(), sizeof(char) * Length);
				Dockers[Docker->Path] = Docker;
			}

			Streams[Stream] = (int64_t)Stream->Tell();
			Mutex.unlock();

			return true;
		}
		bool ContentManager::Export(const std::string& Path, const std::string& Directory, const std::string& Name)
		{
			if (Path.empty() || Directory.empty())
			{
				TH_ERROR("cannot export to/from unknown location");
				return false;
			}

			auto* Stream = new Rest::GzStream();
			if (!Stream->Open(Rest::OS::Resolve(Path, Environment).c_str(), Rest::FileMode_Write_Only))
			{
				TH_ERROR("cannot open \"%s\" for writing", Path.c_str());
				delete Stream;
				return false;
			}

			std::string DBase = Rest::OS::Resolve(Directory, Environment);
			auto Tree = new Rest::FileTree(DBase);
			Stream->Write("\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16);

			uint64_t Size = Tree->GetFiles();
			Stream->Write((char*)&Size, sizeof(uint64_t));

			uint64_t Offset = 0;
			Tree->Loop([Stream, &Offset, &DBase, &Name](Rest::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					auto* File = Rest::OS::Open(Resource, Rest::FileMode_Binary_Read_Only);
					if (!File)
						continue;

					std::string Path = Rest::Stroke(Resource).Replace(DBase, Name).Replace('\\', '/').R();
					if (Name.empty())
						Path.assign(Path.substr(1));

					uint64_t Size = (uint64_t)Path.size();
					uint64_t Length = File->GetSize();

					Stream->Write((char*)&Size, sizeof(uint64_t));
					Stream->Write((char*)&Offset, sizeof(uint64_t));
					Stream->Write((char*)&Length, sizeof(uint64_t));

					Offset += Length;
					if (Size > 0)
						Stream->Write((char*)Path.c_str(), sizeof(char) * Size);

					TH_RELEASE(File);
				}

				return true;
			});
			Tree->Loop([Stream](Rest::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					auto* File = Rest::OS::Open(Resource, Rest::FileMode_Binary_Read_Only);
					if (!File)
						continue;

					int64_t Size = (int64_t)File->GetSize();
					while (Size > 0)
					{
						char Buffer[8192];
						int64_t Offset = File->Read(Buffer, Size > 8192 ? 8192 : Size);
						if (Offset <= 0)
							break;

						Stream->Write(Buffer, Offset);
						Size -= Offset;
					}

					TH_RELEASE(File);
				}

				return true;
			});

			TH_RELEASE(Tree);
			TH_RELEASE(Stream);

			return true;
		}
		bool ContentManager::Cache(Processor* Root, const std::string& Path, void* Resource)
		{
			if (Find(Root, Path) != nullptr)
				return false;

			AssetCache* Asset = new AssetCache();
			Asset->Path = Rest::Stroke(Path).Replace(Environment, "./").Replace('\\', '/').R();
			Asset->Resource = Resource;

			Mutex.lock();
			auto& Entries = Assets[Asset->Path];
			Entries[Root] = Asset;
			Mutex.unlock();

			return true;
		}
		AssetCache* ContentManager::Find(Processor* Target, const std::string& Path)
		{
			Mutex.lock();
			auto It = Assets.find(Rest::Stroke(Path).Replace(Environment, "./").Replace('\\', '/').R());
			if (It != Assets.end())
			{
				auto KIt = It->second.find(Target);
				if (KIt != It->second.end())
				{
					Mutex.unlock();
					return KIt->second;
				}
			}

			Mutex.unlock();
			return nullptr;
		}
		AssetCache* ContentManager::Find(Processor* Target, void* Resource)
		{
			Mutex.lock();
			for (auto& It : Assets)
			{
				auto KIt = It.second.find(Target);
				if (KIt == It.second.end())
					continue;

				if (KIt->second && KIt->second->Resource == Resource)
				{
					Mutex.unlock();
					return KIt->second;
				}
			}

			Mutex.unlock();
			return nullptr;
		}
		Graphics::GraphicsDevice* ContentManager::GetDevice()
		{
			return Device;
		}
		Rest::EventQueue* ContentManager::GetQueue()
		{
			return Queue;
		}
		std::string ContentManager::GetEnvironment()
		{
			return Environment;
		}

		Application::Application(Desc* I)
		{
			if (!I)
				return;

			Host = this;
#ifdef TH_HAS_SDL2
			if (I->Usage & ApplicationUse_Activity_Module)
			{
				if (I->Activity.Width <= 0 || I->Activity.Height <= 0)
				{
					SDL_DisplayMode Display;
					SDL_GetCurrentDisplayMode(0, &Display);
					I->Activity.Width = Display.w / 1.1;
					I->Activity.Height = Display.h / 1.2;
				}

				if (I->Activity.Width > 0 && I->Activity.Height > 0)
				{
					Activity = new Graphics::Activity(I->Activity);
					if (!Activity->GetHandle())
					{
						TH_ERROR("cannot create activity instance");
						return;
					}

					Activity->UserPointer = this;
					Activity->SetCursorVisibility(!I->DisableCursor);
					Activity->Callbacks.KeyState = [this](Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
					{
						GUI::Context* GUI = (GUI::Context*)GetGUI();
						if (GUI != nullptr)
							GUI->EmitKey(Key, Mod, Virtual, Repeat, Pressed);

						KeyEvent(Key, Mod, Virtual, Repeat, Pressed);
					};
					Activity->Callbacks.Input = [this](char* Buffer, int Length)
					{
						GUI::Context* GUI = (GUI::Context*)GetGUI();
						if (GUI != nullptr)
							GUI->EmitInput(Buffer, Length);

						InputEvent(Buffer, Length);
					};
					Activity->Callbacks.CursorWheelState = [this](int X, int Y, bool Normal)
					{
						GUI::Context* GUI = (GUI::Context*)GetGUI();
						if (GUI != nullptr)
							GUI->EmitWheel(X, Y, Normal, Activity->GetKeyModState());

						WheelEvent(X, Y, Normal);
					};
					Activity->Callbacks.WindowStateChange = [this](Graphics::WindowState NewState, int X, int Y)
					{
						if (NewState == Graphics::WindowState_Resize)
						{
							GUI::Context* GUI = (GUI::Context*)GetGUI();
							if (GUI != nullptr)
								GUI->EmitResize(X, Y);
						}

						WindowEvent(NewState, X, Y);
					};

					if (I->Usage & ApplicationUse_Graphics_Module)
					{
						Compute::Vector2 Size = Activity->GetSize();
						if (!I->GraphicsDevice.BufferWidth)
							I->GraphicsDevice.BufferWidth = (unsigned int)Size.X;

						if (!I->GraphicsDevice.BufferHeight)
							I->GraphicsDevice.BufferHeight = (unsigned int)Size.Y;
						I->GraphicsDevice.Window = Activity;

						Renderer = Graphics::GraphicsDevice::Create(I->GraphicsDevice);
						if (!Renderer || !Renderer->IsValid())
						{
							TH_ERROR("graphics device cannot be created");
							return;
						}
					}
				}
				else
					TH_ERROR("cannot detect display to create activity");
			}
#endif
			Queue = new Rest::EventQueue();
			if (I->Usage & ApplicationUse_Audio_Module)
			{
				Audio = new Audio::AudioDevice();
				if (!Audio->IsValid())
				{
					TH_ERROR("audio device cannot be created");
					return;
				}
			}

			if (I->Usage & ApplicationUse_Content_Module)
			{
				Content = new ContentManager(Renderer, Queue);
				Content->AddProcessor<Processors::Asset, Engine::AssetFile>();
				Content->AddProcessor<Processors::SceneGraph, Engine::SceneGraph>();
				Content->AddProcessor<Processors::AudioClip, Audio::AudioClip>();
				Content->AddProcessor<Processors::Texture2D, Graphics::Texture2D>();
				Content->AddProcessor<Processors::Shader, Graphics::Shader>();
				Content->AddProcessor<Processors::Model, Graphics::Model>();
				Content->AddProcessor<Processors::SkinModel, Graphics::SkinModel>();
				Content->AddProcessor<Processors::Document, Rest::Document>();
				Content->AddProcessor<Processors::Server, Network::HTTP::Server>();
				Content->AddProcessor<Processors::Shape, Compute::UnmanagedShape>();
				Content->SetEnvironment(I->Environment.empty() ? Rest::OS::GetDirectory() + I->Directory : I->Environment + I->Directory);
			}

			if (I->Usage & ApplicationUse_AngelScript_Module)
				VM = new Script::VMManager();

			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
			{
				GUI::Subsystem::SetMetadata(Activity, Content, nullptr);
				GUI::Subsystem::SetManager(VM);
			}

			State = ApplicationState_Staging;
		}
		Application::~Application()
		{
			if (Renderer != nullptr)
				Renderer->FlushState();

			TH_RELEASE(Scene);
			TH_RELEASE(Queue);
			TH_RELEASE(VM);
			TH_RELEASE(Audio);

			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
				Engine::GUI::Subsystem::Release();

			TH_RELEASE(Content);
			TH_RELEASE(Renderer);
			TH_RELEASE(Activity);

			for (auto& Job : Workers)
				delete Job;

			Host = nullptr;
		}
		void Application::KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
		{
		}
		void Application::InputEvent(char* Buffer, int Length)
		{
		}
		void Application::WheelEvent(int X, int Y, bool Normal)
		{
		}
		void Application::WindowEvent(Graphics::WindowState NewState, int X, int Y)
		{
		}
		void Application::ScriptHook(Script::VMGlobal* Global)
		{
		}
		bool Application::ComposeEvent()
		{
			return false;
		}
		void Application::Render(Rest::Timer* Time)
		{
		}
		void Application::Initialize(Desc* I)
		{
		}
		void Application::Start(Desc* I)
		{
			if (!I || !Queue)
			{
				TH_ERROR("(CONF): event queue was not found");
				return;
			}

			if (!ComposeEvent())
				Compose();

			if (I->Usage & ApplicationUse_Activity_Module && !Activity)
			{
				TH_ERROR("(CONF): activity was not found");
				return;
			}

			if (I->Usage & ApplicationUse_Graphics_Module && !Renderer)
			{
				TH_ERROR("(CONF): graphics device was not found");
				return;
			}

			if (I->Usage & ApplicationUse_Audio_Module && !Audio)
			{
				TH_ERROR("(CONF): audio device was not found");
				return;
			}

			if (I->Usage & ApplicationUse_AngelScript_Module)
			{
				if (!VM)
				{
					TH_ERROR("(CONF): VM was not found");
					return;
				}
				else
					ScriptHook(&VM->Global());
			}

			Initialize(I);
			if (State == ApplicationState_Terminated)
				return;

			if (Scene != nullptr)
				Scene->Dispatch();

			if (Workers.empty())
				Workers.push_back(new Reactor(this, 0.0, nullptr));

			for (auto It = Workers.begin() + 1; It != Workers.end(); It++)
				Queue->Task<Reactor>(*It, Callee);

			Reactor* Job = Workers.front();
			Job->Time->SetStepLimitation(I->MaxFrames, I->MinFrames);
			Job->Time->FrameLimit = I->FrameLimit;

			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
				GUI::Subsystem::SetMetadata(Activity, Content, Job->Time);

			if (I->Threading != Rest::EventWorkflow_Singlethreaded)
			{
				if (!I->TaskWorkersCount)
					TH_WARN("tasks will not be processed (no workers)");

				State = ApplicationState_Multithreaded;
				Queue->Start(I->Threading, std::max((uint64_t)Workers.size(), I->TaskWorkersCount), I->EventWorkersCount);

				if (Activity != nullptr)
				{
					while (State == ApplicationState_Multithreaded)
					{
						Activity->Dispatch();
						Job->UpdateCore();
						Render(Job->Time);
					}
				}
				else
				{
					while (State == ApplicationState_Multithreaded)
						Job->UpdateCore();
				}

				Queue->Stop();
			}
			else
			{
				State = ApplicationState_Singlethreaded;
				Queue->SetState(Rest::EventState_Working);

				if (Activity != nullptr)
				{
					while (State == ApplicationState_Singlethreaded)
					{
						Queue->Tick();
						Activity->Dispatch();
						Job->UpdateCore();
						Render(Job->Time);
					}
				}
				else
				{
					while (State == ApplicationState_Singlethreaded)
					{
						Queue->Tick();
						Job->UpdateCore();
					}
				}

				Queue->SetState(Rest::EventState_Idle);
			}
		}
		void Application::Stop()
		{
			State = ApplicationState_Terminated;
		}
		void* Application::GetGUI()
		{
			if (!Scene)
				return nullptr;

			Components::Camera* BaseCamera = (Components::Camera*)Scene->GetCamera();
			if (!BaseCamera)
				return nullptr;

			Renderers::UserInterface* Result = BaseCamera->GetRenderer()->GetRenderer<Renderers::UserInterface>();
			return Result != nullptr ? Result->GetContext() : nullptr;
		}
		void Application::Callee(Rest::EventQueue* Queue, Rest::EventArgs* Args)
		{
			Reactor* Job = Args->Get<Reactor>();
			do
			{
				Job->UpdateTask();
			} while (Job->App->State == ApplicationState_Multithreaded && Args->Blockable());

			if (Job->App->State == ApplicationState_Singlethreaded)
				Queue->Task<Reactor>(Job, Callee);
		}
		void Application::Compose()
		{
			Rest::Composer::Push<Components::Model, Entity*>();
			Rest::Composer::Push<Components::Skin, Entity*>();
			Rest::Composer::Push<Components::Emitter, Entity*>();
			Rest::Composer::Push<Components::SoftBody, Entity*>();
			Rest::Composer::Push<Components::Decal, Entity*>();
			Rest::Composer::Push<Components::SkinAnimator, Entity*>();
			Rest::Composer::Push<Components::KeyAnimator, Entity*>();
			Rest::Composer::Push<Components::EmitterAnimator, Entity*>();
			Rest::Composer::Push<Components::RigidBody, Entity*>();
			Rest::Composer::Push<Components::Acceleration, Entity*>();
			Rest::Composer::Push<Components::SliderConstraint, Entity*>();
			Rest::Composer::Push<Components::FreeLook, Entity*>();
			Rest::Composer::Push<Components::Fly, Entity*>();
			Rest::Composer::Push<Components::AudioSource, Entity*>();
			Rest::Composer::Push<Components::AudioListener, Entity*>();
			Rest::Composer::Push<Components::PointLight, Entity*>();
			Rest::Composer::Push<Components::SpotLight, Entity*>();
			Rest::Composer::Push<Components::LineLight, Entity*>();
			Rest::Composer::Push<Components::SurfaceLight, Entity*>();
			Rest::Composer::Push<Components::Camera, Entity*>();
			Rest::Composer::Push<Components::Scriptable, Entity*>();
			Rest::Composer::Push<Renderers::Model, RenderSystem*>();
			Rest::Composer::Push<Renderers::Skin, RenderSystem*>();
			Rest::Composer::Push<Renderers::SoftBody, RenderSystem*>();
			Rest::Composer::Push<Renderers::Emitter, RenderSystem*>();
			Rest::Composer::Push<Renderers::Decal, RenderSystem*>();
			Rest::Composer::Push<Renderers::Lighting, RenderSystem*>();
			Rest::Composer::Push<Renderers::Transparency, RenderSystem*>();
			Rest::Composer::Push<Renderers::Glitch, RenderSystem*>();
			Rest::Composer::Push<Renderers::Tone, RenderSystem*>();
			Rest::Composer::Push<Renderers::DoF, RenderSystem*>();
			Rest::Composer::Push<Renderers::Bloom, RenderSystem*>();
			Rest::Composer::Push<Renderers::SSR, RenderSystem*>();
			Rest::Composer::Push<Renderers::SSAO, RenderSystem*>();
			Rest::Composer::Push<Renderers::SSDO, RenderSystem*>();
			Rest::Composer::Push<Renderers::UserInterface, RenderSystem*>();
			Rest::Composer::Push<Audio::Effects::Reverb>();
			Rest::Composer::Push<Audio::Effects::Chorus>();
			Rest::Composer::Push<Audio::Effects::Distortion>();
			Rest::Composer::Push<Audio::Effects::Echo>();
			Rest::Composer::Push<Audio::Effects::Flanger>();
			Rest::Composer::Push<Audio::Effects::FrequencyShifter>();
			Rest::Composer::Push<Audio::Effects::VocalMorpher>();
			Rest::Composer::Push<Audio::Effects::PitchShifter>();
			Rest::Composer::Push<Audio::Effects::RingModulator>();
			Rest::Composer::Push<Audio::Effects::Autowah>();
			Rest::Composer::Push<Audio::Effects::Compressor>();
			Rest::Composer::Push<Audio::Effects::Equalizer>();
			Rest::Composer::Push<Audio::Filters::Lowpass>();
			Rest::Composer::Push<Audio::Filters::Bandpass>();
			Rest::Composer::Push<Audio::Filters::Highpass>();
		}
		Application* Application::Get()
		{
			return Host;
		}
		Application* Application::Host = nullptr;
	}
}
