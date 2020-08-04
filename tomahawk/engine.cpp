#include "engine.h"
#include "engine/components.h"
#include "engine/processors.h"
#include "engine/renderers.h"
#include "network/http.h"
#include "audio/effects.h"
#include "audio/filters.h"
#include <sstream>
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#endif

namespace Tomahawk
{
	namespace Engine
	{
		bool Appearance::UploadPhase(Graphics::GraphicsDevice* Device, Appearance* Surface)
		{
			if (!Device || !Surface || Surface->Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(Surface->DiffuseMap != nullptr);
			Device->Render.HasNormal = (float)(Surface->NormalMap != nullptr);
			Device->Render.MaterialId = (float)Surface->Material;
			Device->Render.Diffuse = Surface->Diffuse;
			Device->Render.TexCoord = Surface->TexCoord;
			Device->SetTexture2D(Surface->DiffuseMap, 1);
			Device->SetTexture2D(Surface->NormalMap, 2);
			Device->SetTexture2D(Surface->MetallicMap, 3);
			Device->SetTexture2D(Surface->RoughnessMap, 4);
			Device->SetTexture2D(Surface->HeightMap, 5);
			Device->SetTexture2D(Surface->OcclusionMap, 6);
			Device->SetTexture2D(Surface->EmissionMap, 7);

			return true;
		}
		bool Appearance::UploadDepth(Graphics::GraphicsDevice* Device, Appearance* Surface)
		{
			if (!Device || !Surface || Surface->Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(Surface->DiffuseMap != nullptr);
			Device->Render.MaterialId = Surface->Material;
			Device->Render.TexCoord = Surface->TexCoord;
			Device->SetTexture2D(Surface->DiffuseMap, 1);

			return true;
		}
		bool Appearance::UploadCubicDepth(Graphics::GraphicsDevice* Device, Appearance* Surface)
		{
			if (!Device || !Surface || Surface->Material < 0)
				return false;

			Device->Render.HasDiffuse = (float)(Surface->DiffuseMap != nullptr);
			Device->Render.MaterialId = Surface->Material;
			Device->Render.TexCoord = Surface->TexCoord;
			Device->SetTexture2D(Surface->DiffuseMap, 1);

			return true;
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
		bool NMake::Pack(Rest::Document* V, long Value)
		{
			if (!V)
				return false;

			return V->SetInteger("[i]", Value) != nullptr;
		}
		bool NMake::Pack(Rest::Document* V, unsigned long Value)
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
		bool NMake::Pack(Rest::Document* V, const Material& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("emission"), Value.Emission);
			NMake::Pack(V->SetDocument("metallic"), Value.Metallic);
			NMake::Pack(V->SetDocument("roughness"), Value.Roughness);
			NMake::Pack(V->SetDocument("fresnel"), Value.Fresnel);
			NMake::Pack(V->SetDocument("refraction"), Value.Refraction);
			NMake::Pack(V->SetDocument("limpidity"), Value.Limpidity);
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

			return V->SetBoolean("[looped]", Value.Looped) != nullptr && V->SetBoolean("[paused]", Value.Paused) != nullptr && V->SetBoolean("[blended]", Value.Blended) != nullptr && V->SetInteger("[clip]", Value.Clip) != nullptr && V->SetInteger("[frame]", Value.Frame) != nullptr && V->SetNumber("[speed]", Value.Speed) != nullptr && V->SetNumber("[length]", Value.Length) != nullptr && V->SetNumber("[time]", Value.Time) != nullptr;
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

			AssetResource* Asset = Content->FindAsset(Value.DiffuseMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("diffuse-map"), Asset->Path);

			Asset = Content->FindAsset(Value.NormalMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("normal-map"), Asset->Path);

			Asset = Content->FindAsset(Value.MetallicMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("metallic-map"), Asset->Path);

			Asset = Content->FindAsset(Value.RoughnessMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("roughness-map"), Asset->Path);

			Asset = Content->FindAsset(Value.HeightMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("height-map"), Asset->Path);

			Asset = Content->FindAsset(Value.OcclusionMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("occlusion-map"), Asset->Path);

			Asset = Content->FindAsset(Value.EmissionMap);
			if (Asset != nullptr)
				NMake::Pack(V->SetDocument("emission-map"), Asset->Path);

			NMake::Pack(V->SetDocument("diffuse"), Value.Diffuse);
			NMake::Pack(V->SetDocument("texcoord"), Value.TexCoord);
			NMake::Pack(V->SetDocument("material"), Value.Material);
			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::SkinAnimatorClip& Value)
		{
			if (!V)
				return false;

			NMake::Pack(V->SetDocument("name"), Value.Name);
			NMake::Pack(V->SetDocument("playing-speed"), Value.PlayingSpeed);

			Rest::Document* Array = V->SetArray("frames");
			for (auto&& It : Value.Keys)
				NMake::Pack(Array->SetDocument("frame"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const Compute::KeyAnimatorClip& Value)
		{
			if (!V)
				return false;

			return NMake::Pack(V->SetDocument("name"), Value.Name) && NMake::Pack(V->SetDocument("playing-speed"), Value.PlayingSpeed) && NMake::Pack(V->SetDocument("frames"), Value.Keys);
		}
		bool NMake::Pack(Rest::Document* V, const Compute::AnimatorKey& Value)
		{
			if (!V)
				return false;

			return NMake::Pack(V->SetDocument("position"), Value.Position) && NMake::Pack(V->SetDocument("rotation"), Value.Rotation) && NMake::Pack(V->SetDocument("scale"), Value.Scale) && NMake::Pack(V->SetDocument("playing-speed"), Value.PlayingSpeed);
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
		bool NMake::Pack(Rest::Document* V, const std::vector<long>& Value)
		{
			if (!V)
				return false;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			return V->SetString("i-array", Stream.str().substr(0, Stream.str().size() - 1)) && V->SetInteger("[size]", Value.size());
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<unsigned long>& Value)
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
				Stream << It.Length << " ";
				Stream << It.Speed << " ";
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
				Stream << It.PlayingSpeed << " ";
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
		bool NMake::Unpack(Rest::Document* V, long* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetInteger("[i]");
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, unsigned long* O)
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
		bool NMake::Unpack(Rest::Document* V, Material* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Find("emission"), &O->Emission);
			NMake::Unpack(V->Find("metallic"), &O->Metallic);
			NMake::Unpack(V->Find("roughness"), &O->Roughness);
			NMake::Unpack(V->Find("fresnel"), &O->Fresnel);
			NMake::Unpack(V->Find("refraction"), &O->Refraction);
			NMake::Unpack(V->Find("limpidity"), &O->Limpidity);
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
			O->Speed = V->GetNumber("[speed]");
			O->Length = V->GetNumber("[length]");
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
				O->DiffuseMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			if (NMake::Unpack(V->Find("normal-map"), &Path))
				O->NormalMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			if (NMake::Unpack(V->Find("metallic-map"), &Path))
				O->MetallicMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			if (NMake::Unpack(V->Find("roughness-map"), &Path))
				O->RoughnessMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			if (NMake::Unpack(V->Find("height-map"), &Path))
				O->HeightMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			if (NMake::Unpack(V->Find("occlusion-map"), &Path))
				O->OcclusionMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			if (NMake::Unpack(V->Find("emission-map"), &Path))
				O->EmissionMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

			NMake::Unpack(V->Find("diffuse"), &O->Diffuse);
			NMake::Unpack(V->Find("texcoord"), &O->TexCoord);
			NMake::Unpack(V->Find("material"), &O->Material);
			return true;
		}
		bool NMake::Unpack(Rest::Document* V, Compute::SkinAnimatorClip* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Find("name"), &O->Name);
			NMake::Unpack(V->Find("playing-speed"), &O->PlayingSpeed);

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
			NMake::Unpack(V->Find("playing-speed"), &O->PlayingSpeed);
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
			NMake::Unpack(V->Get("playing-speed"), &O->PlayingSpeed);
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

			*O = V->GetString("[s]");
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
		bool NMake::Unpack(Rest::Document* V, std::vector<long>* O)
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
				long Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Rest::Document* V, std::vector<unsigned long>* O)
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
				unsigned long Item;
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
				Stream >> It.Length;
				Stream >> It.Speed;
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
				Stream >> It.Scale.X >> It.Scale.Y >> It.Scale.Z >> It.PlayingSpeed;
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

		ContentKey::ContentKey() : Type(ContentType_Null), Integer(0), Number(0), Boolean(false), Pointer(nullptr)
		{
		}
		ContentKey::ContentKey(const std::string& Value) : Type(ContentType_String), Integer(0), Number(0), Boolean(false), Pointer(nullptr), String(Value)
		{
		}
		ContentKey::ContentKey(int64_t Value) : Type(ContentType_Integer), Integer(Value), Number((double)Value), Boolean(false), Pointer(nullptr)
		{
		}
		ContentKey::ContentKey(double Value) : Type(ContentType_Number), Integer((int64_t)Value), Number(Value), Boolean(false), Pointer(nullptr)
		{
		}
		ContentKey::ContentKey(bool Value) : Type(ContentType_Boolean), Integer(0), Number(0), Boolean(Value), Pointer(nullptr)
		{
		}
		ContentKey::ContentKey(void* Value) : Type(ContentType_Pointer), Integer(0), Number(0), Boolean(false), Pointer(Value)
		{
			if (!Pointer)
				Type = ContentType_Null;
		}

		ContentArgs::ContentArgs(ContentMap* Map) : Args(Map)
		{
		}
		bool ContentArgs::Is(const std::string& Name, const ContentKey& Value)
		{
			ContentKey* V = Get(Name);
			if (!V || V->Type != Value.Type)
				return false;

			switch (V->Type)
			{
				case ContentType_Null:
					return true;
				case ContentType_String:
					return V->String == Value.String;
				case ContentType_Integer:
					return V->Integer == Value.Integer;
				case ContentType_Number:
					return V->Number == Value.Number;
				case ContentType_Boolean:
					return V->Boolean == Value.Boolean;
				case ContentType_Pointer:
					return V->Pointer == Value.Pointer;
			}

			return false;
		}
		ContentKey* ContentArgs::Get(const std::string& Name)
		{
			if (!Args)
				return nullptr;

			auto It = Args->find(Name);
			if (It == Args->end())
				return nullptr;

			return &It->second;
		}
		ContentKey* ContentArgs::Get(const std::string& Name, ContentType Type)
		{
			ContentKey* V = Get(Name);
			if (!V || V->Type != Type)
				return nullptr;

			return V;
		}

		AssetFile::AssetFile(char* SrcBuffer, size_t SrcSize) : Buffer(SrcBuffer), Size(SrcSize)
		{
		}
		AssetFile::~AssetFile()
		{
			if (Buffer != nullptr)
				free(Buffer);
		}
		char* AssetFile::GetBuffer()
		{
			return Buffer;
		}
		size_t AssetFile::GetSize()
		{
			return Size;
		}

		FileProcessor::FileProcessor(ContentManager* NewContent) : Content(NewContent)
		{
		}
		FileProcessor::~FileProcessor()
		{
		}
		void FileProcessor::Free(AssetResource* Asset)
		{
		}
		void* FileProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
		{
			return nullptr;
		}
		void* FileProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
		{
			return nullptr;
		}
		bool FileProcessor::Serialize(Rest::FileStream* Stream, void* Object, ContentArgs* Args)
		{
			return false;
		}
		ContentManager* FileProcessor::GetContent()
		{
			return Content;
		}
		
		void Viewer::Set(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Distance)
		{
			View = _View;
			Projection = _Projection;
			ViewProjection = _View * _Projection;
			InvViewProjection = ViewProjection.Invert();
			InvViewPosition = _Position.InvertZ();
			ViewPosition = InvViewPosition.Invert();
			WorldPosition = _Position;
			WorldRotation = -_View.Rotation();
			ViewDistance = (_Distance < 0 ? 999999999 : _Distance);
			CubicViewProjection[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, InvViewPosition) * Projection;
			CubicViewProjection[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, InvViewPosition) * Projection;
			CubicViewProjection[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, InvViewPosition) * Projection;
			CubicViewProjection[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, InvViewPosition) * Projection;
			CubicViewProjection[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, InvViewPosition) * Projection;
			CubicViewProjection[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, InvViewPosition) * Projection;
		}

		Component::Component(Entity* Reference) : Parent(Reference), Active(true)
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
		void Component::Pipe(Event* Value)
		{
		}
		Entity* Component::GetEntity()
		{
			return Parent;
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

			auto Components = Parent->GetScene()->GetComponents(Id());
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
			if (Parent->Transform->Position.Distance(View.WorldPosition) > View.ViewDistance + Parent->Transform->Scale.Length())
				return false;

			return Compute::MathCommon::IsClipping(View.ViewProjection, World ? *World : Parent->Transform->GetWorld(), 1.5f) == -1;
		}
		bool Cullable::IsNear(const Viewer& View)
		{
			return Parent->Transform->Position.Distance(View.WorldPosition) <= View.ViewDistance + Parent->Transform->Scale.Length();
		}

		Drawable::Drawable(Entity* Ref, bool _Complex) : Cullable(Ref), Static(true), Complex(_Complex)
		{
			if (!Complex)
				Surfaces[nullptr] = Appearance();
		}
		void Drawable::Pipe(Event* Value)
		{
			if (!Value || !Value->Is<Material>())
				return;

			uint64_t Material = (uint64_t)Value->Get<Engine::Material>()->Id;
			for (auto&& Surface : Surfaces)
			{
				if (Surface.second.Material == Material)
					Surface.second.Material = -1;
			}
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
		bool Drawable::IsLimpid()
		{
			SceneGraph* Scene = Parent->GetScene();
			for (auto& Surface : Surfaces)
			{
				Material* Result = Scene->GetMaterialById((uint64_t)Surface.second.Material);
				if (Result != nullptr && Result->Limpidity > 0.0f)
					return true;
			}

			return false;
		}

		Entity::Entity(SceneGraph* Ref) : Scene(Ref), Tag(-1), Id(-1)
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

			delete Transform;
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
				Scene->Unlock();
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
					delete Entity;

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
			if (!In || In == GetComponent(In->Id()))
				return In;

			RemoveComponent(In->Id());
			In->Active = false;
			In->Parent = this;

			Components.insert({ In->Id(), In });
			for (auto& Component : Components)
				Component.second->Awake(In == Component.second ? nullptr : In);

			In->SetActive(true);
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

		GeoRenderer::GeoRenderer(RenderSystem* Lab) : Renderer(Lab)
		{
		}
		GeoRenderer::~GeoRenderer()
		{
		}
		void GeoRenderer::RenderGBuffer(Rest::Timer* TimeStep, Rest::Pool<Component*>* Geometry, RenderOpt Options)
		{
		}
		void GeoRenderer::RenderDepthLinear(Rest::Timer* TimeStep, Rest::Pool<Component*>* Geometry)
		{
		}
		void GeoRenderer::RenderDepthCubic(Rest::Timer* TimeStep, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection)
		{
		}
		void GeoRenderer::Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options)
		{
			if (State == RenderState_GBuffer)
			{
				Rest::Pool<Component*>* Geometry;
				if (Options & RenderOpt_Limpid)
					Geometry = GetLimpid(0);
				else
					Geometry = GetOpaque();

				if (Geometry != nullptr && Geometry->Size() > 0)
					RenderGBuffer(TimeStep, Geometry, Options);
			}
			else if (State == RenderState_Depth_Linear)
			{
				if (!(Options & RenderOpt_Inner))
					return;

				Rest::Pool<Component*>* Opaque = GetOpaque();
				if (Opaque != nullptr && Opaque->Size() > 0)
					RenderDepthLinear(TimeStep, Opaque);

				Rest::Pool<Component*>* Limpid = GetLimpid(0);
				if (Limpid != nullptr && Limpid->Size() > 0)
					RenderDepthLinear(TimeStep, Limpid);
			}
			else if (State == RenderState_Depth_Cubic)
			{
				Viewer& View = System->GetScene()->View;
				if (!(Options & RenderOpt_Inner))
					return;

				Rest::Pool<Component*>* Opaque = GetOpaque();
				if (Opaque != nullptr && Opaque->Size() > 0)
					RenderDepthCubic(TimeStep, Opaque, View.CubicViewProjection);

				Rest::Pool<Component*>* Limpid = GetLimpid(0);
				if (Limpid != nullptr && Limpid->Size() > 0)
					RenderDepthCubic(TimeStep, Limpid, View.CubicViewProjection);
			}
		}
		Rest::Pool<Component*>* GeoRenderer::GetOpaque()
		{
			return nullptr;
		}
		Rest::Pool<Component*>* GeoRenderer::GetLimpid(uint64_t Layer)
		{
			return nullptr;
		}

		TickRenderer::TickRenderer(RenderSystem* Lab) : Renderer(Lab)
		{
		}
		TickRenderer::~TickRenderer()
		{
		}
		void TickRenderer::TickRender(Rest::Timer* TimeStep, RenderState State, RenderOpt Options)
		{
		}
		void TickRenderer::FrameRender(Rest::Timer* TimeStep, RenderState State, RenderOpt Options)
		{
		}
		void TickRenderer::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
		{
			if (Tick.TickEvent(Time->GetElapsedTime()))
				TickRender(Time, State, Options);

			FrameRender(Time, State, Options);
		}
		
		EffectRenderer::EffectRenderer(RenderSystem* Lab) : Renderer(Lab), Output(nullptr), Pass(nullptr)
		{
			DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE");
			Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
			Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
			Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");
		}
		EffectRenderer::~EffectRenderer()
		{
			for (auto It = Shaders.begin(); It != Shaders.end(); It++)
				System->FreeShader(It->first, It->second);

			delete Pass;
			delete Output;
		}
		void EffectRenderer::RenderMerge(Graphics::Shader* Effect, void* Buffer)
		{
			if (!Effect)
				Effect = Shaders.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture2D(Pass, 5);
			Device->SetShader(Effect, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			}

			Device->Draw(6, 0);
			Device->CopyTexture2D(Output, &Pass);
			Device->GenerateTexture(Pass);
		}
		void EffectRenderer::RenderResult(Graphics::Shader* Effect, void* Buffer)
		{
			if (!Effect)
				Effect = Shaders.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture2D(Pass, 5);
			Device->SetShader(Effect, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			}

			Device->Draw(6, 0);
		}
		void EffectRenderer::RenderEffect(Rest::Timer* Time)
		{
		}
		void EffectRenderer::Activate()
		{
			ResizeBuffers();
		}
		void EffectRenderer::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
		{
			if (State != RenderState_GBuffer || Options & RenderOpt_Inner)
				return;

			Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
			if (!Surface || Shaders.empty())
				return;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetDepthStencilState(DepthStencil);
			Device->SetSamplerState(Sampler);
			Device->SetBlendState(Blend);
			Device->SetRasterizerState(Rasterizer);
			Device->SetTarget(Output, 0, 0, 0);
			Device->SetTexture2D(Surface->GetTarget(0), 1);
			Device->SetTexture2D(Surface->GetTarget(1), 2);
			Device->SetTexture2D(Surface->GetTarget(2), 3);
			Device->SetTexture2D(Surface->GetTarget(3), 4);
			Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);

			RenderEffect(Time);

			Device->FlushTexture2D(1, 4);
			Device->CopyTargetFrom(Surface, 0, Output);
		}
		void EffectRenderer::ResizeBuffers()
		{
			Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
			I.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			I.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
			I.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
			I.MipLevels = System->GetDevice()->GetMipLevel(I.Width, I.Height);
			System->GetScene()->GetTargetFormat(&I.FormatMode, 1);

			delete Output;
			Output = System->GetDevice()->CreateRenderTarget2D(I);
		}
		Graphics::Shader* EffectRenderer::GetEffect(const std::string& Name)
		{
			auto It = Shaders.find(Name);
			if (It != Shaders.end())
				return It->second;

			return nullptr;
		}
		Graphics::Shader* EffectRenderer::CompileEffect(const std::string& Name, const std::string& Code, size_t BufferSize)
		{
			if (Name.empty())
			{
				THAWK_ERROR("cannot compile unnamed shader source");
				return nullptr;
			}

			Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
			Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
			Desc.LayoutSize = 2;
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
				delete Shader;
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
		bool ShaderCache::Free(const std::string& Name, Graphics::Shader* Shader)
		{
			Safe.lock();
			auto It = Cache.find(Name);
			if (It == Cache.end())
				return false;

			if (Shader != It->second.Shader)
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

			delete It->second.Shader;
			Cache.erase(It);
			Safe.unlock();

			return true;
		}
		void ShaderCache::ClearCache()
		{
			Safe.lock();
			for (auto It = Cache.begin(); It != Cache.end(); It++)
			{
				delete It->second.Shader;
				It->second.Shader = nullptr;
			}

			Cache.clear();
			Safe.unlock();
		}

		RenderSystem::RenderSystem(Graphics::GraphicsDevice* Ref) : Device(Ref), Scene(nullptr), QuadVertex(nullptr), SphereVertex(nullptr), SphereIndex(nullptr), CubeVertex(nullptr), CubeIndex(nullptr), EnableCull(true)
		{
		}
		RenderSystem::~RenderSystem()
		{
			for (auto& Renderer : Renderers)
			{
				if (!Renderer)
					continue;

				Renderer->Deactivate();
				delete Renderer;
			}

			delete QuadVertex;
			delete SphereVertex;
			delete SphereIndex;
			delete CubeVertex;
			delete CubeIndex;
		}
		void RenderSystem::SetScene(SceneGraph* NewScene)
		{
			Scene = NewScene;
		}
		void RenderSystem::Remount()
		{
			for (auto& Renderer : Renderers)
			{
				Renderer->Deactivate();
				Renderer->Activate();
			}
		}
		void RenderSystem::Synchronize(const Viewer& View)
		{
			if (!EnableCull)
				return;

			for (auto& Base : Cull)
			{
				for (auto It = Base.second->Begin(); It != Base.second->End(); ++It)
				{
					Cullable* Data = (Cullable*)*It;
					Data->Visibility = Data->Cull(View);
				}
			}
		}
		void RenderSystem::MoveRenderer(uint64_t Id, int64_t Offset)
		{
			if (Offset == 0)
				return;

			for (int64_t i = 0; i < Renderers.size(); i++)
			{
				if (Renderers[i]->Id() != Id)
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
				if (*It && (*It)->Id() == Id)
				{
					(*It)->Deactivate();
					delete *It;
					Renderers.erase(It);
					break;
				}
			}
		}
		void RenderSystem::FreeShader(const std::string& Name, Graphics::Shader* Shader)
		{
			ShaderCache* Cache = (Scene ? Scene->GetCache() : nullptr);
			if (Cache != nullptr)
			{
				if (Cache->Get(Name) == Shader)
					return;
			}

			delete Shader;
		}
		bool RenderSystem::Renderable(Cullable* Base, CullResult Mode, float* Result)
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
		Renderer* RenderSystem::AddRenderer(Renderer* In)
		{
			if (!In)
				return nullptr;

			for (auto It = Renderers.begin(); It != Renderers.end(); It++)
			{
				if (*It && (*It)->Id() == In->Id())
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
			Renderers.push_back(In);

			return In;
		}
		Renderer* RenderSystem::GetRenderer(uint64_t Id)
		{
			for (auto& RenderStage : Renderers)
			{
				if (RenderStage->Id() == Id)
					return RenderStage;
			}

			return nullptr;
		}
		size_t RenderSystem::GetQuadVSize()
		{
			return sizeof(Compute::ShapeVertex);
		}
		size_t RenderSystem::GetSphereVSize()
		{
			return sizeof(Compute::ShapeVertex);
		}
		size_t RenderSystem::GetSphereISize()
		{
			return sizeof(int);
		}
		size_t RenderSystem::GetCubeVSize()
		{
			return sizeof(Compute::Vertex);
		}
		size_t RenderSystem::GetCubeISize()
		{
			return sizeof(int);
		}
		Graphics::ElementBuffer* RenderSystem::GetQuadVBuffer()
		{
			if (QuadVertex != nullptr)
				return QuadVertex;

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
			F.UseSubresource = true;

			QuadVertex = Device->CreateElementBuffer(F);
			return QuadVertex;
		}
		Graphics::ElementBuffer* RenderSystem::GetSphereVBuffer()
		{
			if (SphereVertex != nullptr)
				return SphereVertex;

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
			F.UseSubresource = true;

			SphereVertex = Device->CreateElementBuffer(F);
			return SphereVertex;
		}
		Graphics::ElementBuffer* RenderSystem::GetSphereIBuffer()
		{
			if (SphereIndex != nullptr)
				return SphereIndex;

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
			F.UseSubresource = true;

			SphereIndex = Device->CreateElementBuffer(F);
			return SphereIndex;
		}
		Graphics::ElementBuffer* RenderSystem::GetCubeVBuffer()
		{
			if (CubeVertex != nullptr)
				return CubeVertex;

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
			F.UseSubresource = true;

			CubeVertex = Device->CreateElementBuffer(F);
			return CubeVertex;
		}
		Graphics::ElementBuffer* RenderSystem::GetCubeIBuffer()
		{
			if (CubeIndex != nullptr)
				return CubeIndex;

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
			F.UseSubresource = true;

			CubeIndex = Device->CreateElementBuffer(F);
			return CubeIndex;
		}
		std::vector<Renderer*>* RenderSystem::GetRenderers()
		{
			return &Renderers;
		}
		Graphics::GraphicsDevice* RenderSystem::GetDevice()
		{
			return Device;
		}
		Graphics::Shader* RenderSystem::CompileShader(const std::string& Name, Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			if (Name.empty() && Desc.Filename.empty())
			{
				THAWK_ERROR("shader must have name or filename");
				return nullptr;
			}

			Desc.Filename = Name;
			ShaderCache* Cache = (Scene ? Scene->GetCache() : nullptr);
			if (Cache != nullptr)
				return Cache->Compile(Name.empty() ? Desc.Filename : Name, Desc, BufferSize);

			Graphics::Shader* Shader = Device->CreateShader(Desc);
			if (BufferSize > 0)
				Device->UpdateBufferSize(Shader, BufferSize);

			return Shader;
		}
		SceneGraph* RenderSystem::GetScene()
		{
			return Scene;
		}

		SceneGraph::SceneGraph(const Desc& I) : Conf(I)
		{
			Image.DepthStencil = nullptr;
			Image.Rasterizer = nullptr;
			Image.Blend = nullptr;
			Image.Sampler = nullptr;

			Sync.Count = 0;
			Sync.Locked = false;
			for (int i = 0; i < ThreadId_Count; i++)
				Sync.Threads[i].State = 0;

			Materials.reserve(16);
			Configure(I);

			Simulator = new Compute::Simulator(I.Simulator);
			ExpandMaterialStructure();
		}
		SceneGraph::~SceneGraph()
		{
			Denotify();
			Lock();

			for (auto It = Entities.Begin(); It != Entities.End(); It++)
				delete *It;

			delete Structure;
			delete Surface;
			delete Simulator;

			Unlock();
		}
		void SceneGraph::Configure(const Desc& NewConf)
		{
			if (!Conf.Queue || !Conf.Device)
				return;

			Image.DepthStencil = Conf.Device->GetDepthStencilState("DEF_NONE");
			Image.Rasterizer = Conf.Device->GetRasterizerState("DEF_CULL_BACK");
			Image.Blend = Conf.Device->GetBlendState("DEF_OVERWRITE");
			Image.Sampler = Conf.Device->GetSamplerState("DEF_TRILINEAR_X16");

			Lock();
			Conf = NewConf;
			Entities.Reserve(Conf.EntityCount);
			Pending.Reserve(Conf.ComponentCount);

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
			Conf.Device->SetDepthStencilState(Image.DepthStencil);
			Conf.Device->SetSamplerState(Image.Sampler);
			Conf.Device->SetBlendState(Image.Blend);
			Conf.Device->SetRasterizerState(Image.Rasterizer);
			Conf.Device->SetShader(Conf.Device->GetBasicEffect(), Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			Conf.Device->SetVertexBuffer(View.Renderer->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);
			Conf.Device->SetTexture2D(Surface->GetTarget(0), 0);
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType_Render);
			Conf.Device->Draw(6, 0);
			Conf.Device->SetTexture2D(nullptr, 0);
		}
		void SceneGraph::RenderGBuffer(Rest::Timer* Time, RenderOpt Options)
		{
			if (!View.Renderer)
				return;

			auto* States = View.Renderer->GetRenderers();
			for (auto& Renderer : *States)
			{
				if (Renderer->Active)
					Renderer->Render(Time, RenderState_GBuffer, Options);
			}
		}
		void SceneGraph::RenderDepthLinear(Rest::Timer* Time)
		{
			if (!View.Renderer)
				return;

			auto* States = View.Renderer->GetRenderers();
			for (auto& Renderer : *States)
			{
				if (Renderer->Active)
					Renderer->Render(Time, RenderState_Depth_Linear, RenderOpt_Inner);
			}
		}
		void SceneGraph::RenderDepthCubic(Rest::Timer* Time)
		{
			if (!View.Renderer)
				return;

			auto* States = View.Renderer->GetRenderers();
			for (auto& Renderer : *States)
			{
				if (Renderer->Active)
					Renderer->Render(Time, RenderState_Depth_Cubic, RenderOpt_Inner);
			}
		}
		void SceneGraph::Render(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Render);
			if (Camera != nullptr)
			{
				Conf.Device->UpdateBuffer(Structure, Materials.data(), Materials.size() * sizeof(Material));
				Conf.Device->SetBuffer(Structure, 0);
				
				ClearSurface();
				RestoreViewBuffer(nullptr);
				RenderGBuffer(Time, RenderOpt_None);
			}
			EndThread(ThreadId_Render);
		}
		void SceneGraph::Simulation(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Simulation);
			Simulator->Simulate((float)Time->GetTimeStep());
			EndThread(ThreadId_Simulation);
		}
		void SceneGraph::Synchronize(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Synchronize);
			if (Camera != nullptr)
			{
				auto* Viewport = Camera->As<Components::Camera>();
				Viewport->GetRenderer()->Synchronize(Viewport->GetViewer());
			}

			for (auto It = Pending.Begin(); It != Pending.End(); It++)
				(*It)->Synchronize(Time);

			uint64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				Entity* V = *It;
				V->Transform->Synchronize();
				V->Id = Index;
				Index++;
			}
			EndThread(ThreadId_Synchronize);
		}
		void SceneGraph::Update(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Update);
			for (auto It = Pending.Begin(); It != Pending.End(); It++)
				(*It)->Update(Time);

			DispatchEvents();
			EndThread(ThreadId_Update);
		}
		void SceneGraph::Redistribute()
		{
			Lock();
			for (auto& Component : Components)
				Component.second.Clear();

			Pending.Clear();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
				RegisterEntity(*It);

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
		void SceneGraph::SortEntitiesBackToFront()
		{
			Lock();
			for (size_t i = 0; i < Entities.Size(); i++)
			{
				if (i + 1 >= Entities.Size())
					break;

				Entity* Entity1 = Entities[i], * Entity2 = Entities[i + 1];
				float Distance1 = Entity1->Transform->Position.Distance(View.WorldPosition);
				float Distance2 = Entity2->Transform->Position.Distance(View.WorldPosition);

				if (Distance1 < Distance2)
				{
					Entity* Value = Entities[i];
					Entities[i] = Entities[i + 1];
					Entities[i + 1] = Value;
					i++;
				}
			}
			Unlock();
		}
		void SceneGraph::SortEntitiesBackToFront(uint64_t Section)
		{
			SortEntitiesBackToFront(GetComponents(Section));
		}
		void SceneGraph::SortEntitiesBackToFront(Rest::Pool<Component*>* Array)
		{
			if (!Array)
				return;

			Lock();
			for (size_t i = 0; i < Array->Size(); i++)
			{
				if (i + 1 >= Array->Size())
					break;

				Component* Entity1 = (*Array)[i], *Entity2 = (*Array)[i + 1];
				float Distance1 = Entity1->Parent->Transform->Position.Distance(View.WorldPosition);
				float Distance2 = Entity2->Parent->Transform->Position.Distance(View.WorldPosition);

				if (Distance1 < Distance2)
				{
					Component* Value = (*Array)[i];
					(*Array)[i] = (*Array)[i + 1];
					(*Array)[i + 1] = Value;
					i++;
				}
			}
			Unlock();
		}
		void SceneGraph::SetMutation(const std::string& Name, const MutationCallback& Callback)
		{
			if (Mutation.Invoked)
				return;

			Mutation.Safe.lock();
			Mutation.Invoked = true;

			if (!Callback)
			{
				auto It = Mutation.Callbacks.find(Name);
				if (It != Mutation.Callbacks.end())
					Mutation.Callbacks.erase(It);
			}
			else
				Mutation.Callbacks[Name] = Callback;

			Mutation.Invoked = true;
			Mutation.Safe.unlock();
		}
		void SceneGraph::SetCamera(Entity* In)
		{
			if (In != nullptr)
			{
				auto Viewer = In->GetComponent<Components::Camera>();
				if (Viewer != nullptr && Viewer->Active)
				{
					Viewer->Awake(nullptr);
					Lock();
					Camera = Viewer;
					Unlock();
				}
			}
			else
			{
				Lock();
				Camera = nullptr;
				Unlock();
			}
		}
		void SceneGraph::RemoveEntity(Entity* Entity, bool Release)
		{
			if (!Entity)
				return;

			Denotify();
			if (!UnregisterEntity(Entity) || !Release)
				return;
			else
				Entity->RemoveChilds();

			Lock();
			delete Entity;
			Unlock();
		}
		void SceneGraph::RemoveMaterial(uint64_t Material)
		{
			if (Material >= Materials.size() || Material >= Names.size())
				return;

			if (!NotifyEach<Engine::Material>(nullptr, Materials[Material]))
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

			InvokeMutation(In, nullptr, true);
			for (auto& Component : In->Components)
			{
				Component.second->Awake(Component.second);
				InvokeMutation(In, Component.second, true);

				auto Storage = GetComponents(Component.second->Id());
				if (Component.second->Active)
					Storage->AddIfNotExists(Component.second);
				else
					Storage->Remove(Component.second);

				if (Component.second->Active)
					Pending.AddIfNotExists(Component.second);
				else
					Pending.Remove(Component.second);
			}
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

			InvokeMutation(In, nullptr, false);
			for (auto& Component : In->Components)
			{
				Component.second->Asleep();
				InvokeMutation(In, Component.second, false);

				auto Storage = &Components[Component.second->Id()];
				Storage->Remove(Component.second);
				Pending.Remove(Component.second);
			}

			Entities.Remove(In);
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
			Conf.Device->View.FarPlane = View.ViewDistance;
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType_View);
		}
		void SceneGraph::ExpandMaterialStructure()
		{
			Lock();

			Materials.reserve(Materials.capacity() * 2);
			for (size_t i = 0; i < Materials.size(); i++)
				Materials[i].Id = (float)i;

			Graphics::StructureBuffer::Desc F = Graphics::StructureBuffer::Desc();
			F.ElementCount = (unsigned int)Materials.capacity();
			F.ElementWidth = sizeof(Material);
			F.Elements = Materials.data();

			delete Structure;
			Structure = Conf.Device->CreateStructureBuffer(F);
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
			if (Sync.Id == Id && Sync.Count > 0)
			{
				Sync.Count--;
				return;
			}

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
		void SceneGraph::DispatchEvents()
		{
			Sync.Events.lock();
			if (Events.empty())
				return Sync.Events.unlock();

			Event* Message = *Events.begin();
			Events.erase(Events.begin());
			Sync.Events.unlock();

			if (Message->Foreach)
			{
				if (!Message->Root || !Message->Root->Parent)
				{
					for (auto It = Entities.Begin(); It != Entities.End(); It++)
					{
						for (auto K = (*It)->First(); K != (*It)->Last(); K++)
							K->second->Pipe(Message);
					}
				}
				else
				{
					for (auto It = Message->Root->Parent->First(); It != Message->Root->Parent->Last(); It++)
						It->second->Pipe(Message);
				}
			}
			else if (Message->Root)
				Message->Root->Pipe(Message);

			if (Message->Context != nullptr)
				free(Message->Context);

			delete Message;
		}
		void SceneGraph::ResizeBuffers()
		{
			Graphics::MultiRenderTarget2D::Desc F;
			GetTargetDesc(&F);

			if (Camera != nullptr)
			{
				Lock();
				delete Surface;
				Surface = Conf.Device->CreateMultiRenderTarget2D(F);

				auto* Array = GetComponents<Components::Camera>();
				for (auto It = Array->Begin(); It != Array->End(); It++)
					(*It)->As<Components::Camera>()->ResizeBuffers();

				Unlock();
			}
			else
			{
				delete Surface;
				Surface = Conf.Device->CreateMultiRenderTarget2D(F);
			}
		}
		void SceneGraph::SwapSurface(Graphics::MultiRenderTarget2D* NewSurface)
		{
			Surface = NewSurface;
		}
		void SceneGraph::SetView(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Distance, bool Upload)
		{
			View.Set(_View, _Projection, _Position, _Distance);
			if (Upload)
				RestoreViewBuffer(&View);
		}
		void SceneGraph::SetSurface()
		{
			Conf.Device->SetTarget(Surface);
		}
		void SceneGraph::SetSurfaceCleared()
		{
			Conf.Device->SetTarget(Surface);
			Conf.Device->Clear(Surface, 0, 0, 0, 0);
			Conf.Device->Clear(Surface, 1, 0, 0, 0);
			Conf.Device->Clear(Surface, 2, 1, 0, 0);
			Conf.Device->Clear(Surface, 3, 0, 0, 0);
			Conf.Device->ClearDepth(Surface);
		}
		void SceneGraph::SetMaterialName(uint64_t Material, const std::string& Name)
		{
			if (Material >= Names.size())
				return;

			Names[Material] = Name;
		}
		void SceneGraph::ClearSurface()
		{
			Conf.Device->Clear(Surface, 0, 0, 0, 0);
			Conf.Device->Clear(Surface, 1, 0, 0, 0);
			Conf.Device->Clear(Surface, 2, 1, 0, 0);
			Conf.Device->Clear(Surface, 3, 0, 0, 0);
			Conf.Device->ClearDepth(Surface);
		}
		void SceneGraph::ClearSurfaceColor()
		{
			Conf.Device->Clear(Surface, 0, 0, 0, 0);
			Conf.Device->Clear(Surface, 1, 0, 0, 0);
			Conf.Device->Clear(Surface, 2, 1, 0, 0);
			Conf.Device->Clear(Surface, 3, 0, 0, 0);
		}
		void SceneGraph::ClearSurfaceDepth()
		{
			Conf.Device->ClearDepth(Surface);
		}
		void SceneGraph::InvokeMutation(Entity* Target, Component* Base, bool Push)
		{
			if (Mutation.Callbacks.empty())
				return;

			bool Invoked = Mutation.Invoked;
			if (!Invoked)
				Mutation.Safe.lock();
			Mutation.Invoked = true;

			for (auto& Callback : Mutation.Callbacks)
			{
				if (Callback.second)
					Callback.second(Target, Base, Push);
			}

			Mutation.Invoked = Invoked;
			if (!Invoked)
				Mutation.Safe.unlock();
		}
		void SceneGraph::GetTargetDesc(Graphics::RenderTarget2D::Desc* Result)
		{
			if (!Result)
				return;

			Result->MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			Result->Width = (unsigned int)(Conf.Device->GetRenderTarget()->GetWidth() * Conf.RenderQuality);
			Result->Height = (unsigned int)(Conf.Device->GetRenderTarget()->GetHeight() * Conf.RenderQuality);
			Result->MipLevels = Conf.Device->GetMipLevel(Result->Width, Result->Height);
			GetTargetFormat(&Result->FormatMode, 1);
		}
		void SceneGraph::GetTargetDesc(Graphics::MultiRenderTarget2D::Desc* Result)
		{
			if (!Result)
				return;

			Result->SVTarget = Graphics::SurfaceTarget3;
			Result->MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			Result->Width = (unsigned int)(Conf.Device->GetRenderTarget()->GetWidth() * Conf.RenderQuality);
			Result->Height = (unsigned int)(Conf.Device->GetRenderTarget()->GetHeight() * Conf.RenderQuality);
			Result->MipLevels = Conf.Device->GetMipLevel(Result->Width, Result->Height);
			GetTargetFormat(Result->FormatMode, 8);
		}
		void SceneGraph::GetTargetFormat(Graphics::Format* Result, uint64_t Size)
		{
			if (!Result || !Size)
				return;

			if (Size >= 1)
				Result[0] = Conf.EnableHDR ? Graphics::Format_R16G16B16A16_Unorm : Graphics::Format_R8G8B8A8_Unorm;

			if (Size >= 2)
				Result[1] = Graphics::Format_R16G16B16A16_Float;

			if (Size >= 3)
				Result[2] = Graphics::Format_R32_Float;

			if (Size >= 4)
				Result[3] = Graphics::Format_R8G8B8A8_Unorm;

			for (uint64_t i = 4; i < Size; i++)
				Result[i] = Graphics::Format_Invalid;
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
				Viewer->Awake(Viewer);
				Lock();
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
		Entity* SceneGraph::FindEntityAt(Compute::Vector3 Position, float Radius)
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
		std::vector<Entity*> SceneGraph::FindEntitiesAt(Compute::Vector3 Position, float Radius)
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
		bool SceneGraph::IsEntityVisible(Entity* Entity, Compute::Matrix4x4 ViewProjection)
		{
			if (!Camera || !Entity || Entity->Transform->Position.Distance(Camera->Parent->Transform->Position) > View.ViewDistance + Entity->Transform->Scale.Length())
				return false;

			return (Compute::MathCommon::IsClipping(ViewProjection, Entity->Transform->GetWorld(), 2) == -1);
		}
		bool SceneGraph::IsEntityVisible(Entity* Entity, Compute::Matrix4x4 ViewProjection, Compute::Vector3 ViewPosition, float DrawDistance)
		{
			if (!Entity || Entity->Transform->Position.Distance(ViewPosition) > DrawDistance + Entity->Transform->Scale.Length())
				return false;

			return (Compute::MathCommon::IsClipping(ViewProjection, Entity->Transform->GetWorld(), 2) == -1);
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
		bool SceneGraph::Denotify()
		{
			if (!Conf.Queue)
				return false;

			while (Conf.Queue->Pull<Event>(Rest::EventType_Events));
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
		Graphics::MultiRenderTarget2D* SceneGraph::GetSurface()
		{
			return Surface;
		}
		Graphics::StructureBuffer* SceneGraph::GetStructure()
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
		ShaderCache* SceneGraph::GetCache()
		{
			return Conf.Cache;
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
			for (auto It = Assets.begin(); It != Assets.end(); It++)
			{
				if (!It->second)
					continue;

				FileProcessor* Root = It->second->Processor;
				Mutex.unlock();
				if (Root != nullptr)
					Root->Free(It->second);
				Mutex.lock();

				delete It->second;
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
			Mutex.unlock();
		}
		void* ContentManager::LoadForward(const std::string& Path, FileProcessor* Processor, ContentMap* Map)
		{
			if (Path.empty())
				return nullptr;

			ContentArgs Args(Map);
			if (!Processor)
			{
				THAWK_ERROR("file processor for \"%s\" wasn't found", Path.c_str());
				return nullptr;
			}

			void* Object = LoadStreaming(Path, Processor, Map);
			if (Object != nullptr)
				return Object;

			Mutex.lock();
			std::string File = Rest::OS::Resolve(Path, Environment);
			if (!Rest::OS::FileExists(File.c_str()))
			{
				if (!Rest::OS::FileExists(Path.c_str()))
				{
					Mutex.unlock();
					THAWK_ERROR("file \"%s\" wasn't found", File.c_str());
					return nullptr;
				}

				File = Path;
			}
			Mutex.unlock();

			AssetResource* Asset = FindAsset(Path);
			if (Asset != nullptr && Asset->Processor == Processor)
				return Processor->Duplicate(Asset, &Args);

			auto Stream = new Rest::FileStream();
			Stream->Open(File.c_str(), Rest::FileMode_Binary_Read_Only);
			Object = Processor->Deserialize(Stream, Stream->Size(), 0, &Args);
			delete Stream;

			return Object;
		}
		void* ContentManager::LoadStreaming(const std::string& Path, FileProcessor* Processor, ContentMap* Map)
		{
			if (Path.empty())
				return nullptr;

			auto Docker = Dockers.find(Rest::Stroke(Path).Replace('\\', '/').Replace("./", "").R());
			if (Docker == Dockers.end() || !Docker->second || !Docker->second->Stream)
				return nullptr;

			AssetResource* Asset = FindAsset(Path);
			ContentArgs Args(Map);
			if (Asset != nullptr && Asset->Processor == Processor)
				return Processor->Duplicate(Asset, &Args);

			auto It = Streams.find(Docker->second->Stream);
			if (It == Streams.end())
			{
				THAWK_ERROR("cannot resolve stream offset for \"%s\"", Path.c_str());
				return nullptr;
			}

			Rest::FileStream* Stream = Docker->second->Stream;
			Stream->Seek(Rest::FileSeek_Begin, It->second + Docker->second->Offset);
			Stream->Filename() = Path;

			return Processor->Deserialize(Stream, Docker->second->Length, It->second + Docker->second->Offset, &Args);
		}
		bool ContentManager::SaveForward(const std::string& Path, FileProcessor* Processor, void* Object, ContentMap* Map)
		{
			if (Path.empty())
				return false;

			if (!Processor)
			{
				THAWK_ERROR("file processor for \"%s\" wasn't found", Path.c_str());
				return false;
			}

			if (!Object)
			{
				THAWK_ERROR("cannot save null object to \"%s\"", Path.c_str());
				return false;
			}

			Mutex.lock();
			ContentArgs Args(Map);
			std::string Directory = Rest::OS::FileDirectory(Path);
			std::string File = Rest::OS::Resolve(Directory, Environment);
			File.append(Path.substr(Directory.size()));
			Mutex.unlock();

			auto Stream = new Rest::FileStream();
			if (!Stream->Open(File.c_str(), Rest::FileMode_Binary_Write_Only))
			{
				if (!Stream->Open(Path.c_str(), Rest::FileMode_Binary_Write_Only))
				{
					THAWK_ERROR("cannot open stream for writing at \"%s\" or \"%s\"", File.c_str(), Path.c_str());
					delete Stream;
					return false;
				}
			}

			bool Result = Processor->Serialize(Stream, Object, &Args);
			delete Stream;
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
					THAWK_ERROR("file \"%s\" wasn't found", Path.c_str());
					return false;
				}

				File = Path;
			}
			Mutex.unlock();

			Rest::FileStream* Stream = new Rest::FileStream();
			if (!Stream->OpenZ(File.c_str(), Rest::FileMode::FileMode_Binary_Read_Only))
			{
				THAWK_ERROR("cannot open \"%s\" for reading", File.c_str());
				delete Stream;
				return false;
			}

			char Buffer[16];
			if (Stream->Read(Buffer, 16) != 16)
			{
				THAWK_ERROR("file \"%s\" has corrupted header", File.c_str());
				delete Stream;
				return false;
			}

			if (memcmp(Buffer, "\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16) != 0)
			{
				THAWK_ERROR("file \"%s\" header version is corrupted", File.c_str());
				delete Stream;
				return false;
			}

			uint64_t Size = 0;
			if (Stream->Read((char*)&Size, sizeof(uint64_t)) != sizeof(uint64_t))
			{
				THAWK_ERROR("file \"%s\" has corrupted dock size", File.c_str());
				delete Stream;
				return false;
			}

			Mutex.lock();
			for (uint64_t i = 0; i < Size; i++)
			{
				AssetDocker* Docker = new AssetDocker();
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
				THAWK_ERROR("cannot export to/from unknown location");
				return false;
			}

			auto Stream = new Rest::FileStream();
			if (!Stream->OpenZ(Rest::OS::Resolve(Path, Environment).c_str(), Rest::FileMode_Write_Only))
			{
				THAWK_ERROR("cannot open \"%s\" for writing", Path.c_str());
				delete Stream;
				return false;
			}

			std::string Base = Rest::OS::Resolve(Directory, Environment);
			auto Tree = new Rest::FileTree(Base);
			Stream->Write("\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16);

			uint64_t Size = Tree->GetFiles();
			Stream->Write((char*)&Size, sizeof(uint64_t));

			uint64_t Offset = 0;
			Tree->Loop([Stream, &Offset, &Base, &Name](Rest::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					Rest::FileStream* File = new Rest::FileStream();
					if (File->Open(Resource.c_str(), Rest::FileMode_Binary_Read_Only))
					{
						std::string Path = Rest::Stroke(Resource).Replace(Base, Name).Replace('\\', '/').R();
						if (Name.empty())
							Path.assign(Path.substr(1));

						uint64_t Size = (uint64_t)Path.size();
						uint64_t Length = File->Size();

						Stream->Write((char*)&Size, sizeof(uint64_t));
						Stream->Write((char*)&Offset, sizeof(uint64_t));
						Stream->Write((char*)&Length, sizeof(uint64_t));

						Offset += Length;
						if (Size > 0)
							Stream->Write((char*)Path.c_str(), sizeof(char) * Size);
					}
					delete File;
				}

				return true;
			});
			Tree->Loop([Stream](Rest::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					Rest::FileStream* File = new Rest::FileStream();
					if (File->Open(Resource.c_str(), Rest::FileMode_Binary_Read_Only))
					{
						int64_t Size = (int64_t)File->Size();
						while (Size > 0)
						{
							char Buffer[8192];
							int64_t Offset = File->Read(Buffer, Size > 8192 ? 8192 : Size);
							if (Offset <= 0)
								break;

							Stream->Write(Buffer, Offset);
							Size -= Offset;
						}
					}
					delete File;
				}

				return true;
			});

			delete Tree;
			delete Stream;

			return true;
		}
		bool ContentManager::Cache(FileProcessor* Root, const std::string& Path, void* Resource)
		{
			Mutex.lock();
			auto It = Assets.find(Path);
			if (It != Assets.end())
			{
				Mutex.unlock();
				return false;
			}

			AssetResource* Asset = new AssetResource();
			Asset->Processor = Root;
			Asset->Path = Rest::Stroke(Path).Replace(Environment, "./").Replace('\\', '/').R();
			Asset->Resource = Resource;
			Assets[Asset->Path] = Asset;
			Mutex.unlock();

			return true;
		}
		AssetResource* ContentManager::FindAsset(const std::string& Path)
		{
			Mutex.lock();
			auto It = Assets.find(Rest::Stroke(Path).Replace(Environment, "./").Replace('\\', '/').R());
			if (It != Assets.end())
			{
				Mutex.unlock();
				return It->second;
			}

			Mutex.unlock();
			return nullptr;
		}
		AssetResource* ContentManager::FindAsset(void* Resource)
		{
			Mutex.lock();
			for (auto& It : Assets)
			{
				if (It.second && It.second->Resource == Resource)
				{
					Mutex.unlock();
					return It.second;
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
			Compose();

#ifdef THAWK_HAS_SDL2
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
					Activity->UserPointer = this;
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
							GUI->EmitWheel(X, Y, Normal);

						WheelEvent(X, Y, Normal);
					};
					Activity->Callbacks.WindowStateChange = [this](Graphics::WindowState NewState, int X, int Y)
					{
						WindowEvent(NewState, X, Y);
					};

					Activity->Cursor(!I->DisableCursor);
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
							delete Renderer;
							Renderer = nullptr;

							THAWK_ERROR("graphics device cannot be created");
							return;
						}

						if (I->EnableShaderCache)
							Shaders = new ShaderCache(Renderer);
					}
				}
				else
					THAWK_ERROR("cannot detect display to create activity");
			}
#endif
			Queue = new Rest::EventQueue();
			if (I->Usage & ApplicationUse_Audio_Module)
			{
				Audio = new Audio::AudioDevice();
				if (!Audio->IsValid())
				{
					delete Audio;
					Audio = nullptr;

					THAWK_ERROR("audio device cannot be created");
					return;
				}
			}

			if (I->Usage & ApplicationUse_Content_Module)
			{
				Content = new ContentManager(Renderer, Queue);
				Content->AddProcessor<FileProcessors::AssetFileProcessor, Engine::AssetFile>();
				Content->AddProcessor<FileProcessors::SceneGraphProcessor, Engine::SceneGraph>();
				Content->AddProcessor<FileProcessors::AudioClipProcessor, Audio::AudioClip>();
				Content->AddProcessor<FileProcessors::Texture2DProcessor, Graphics::Texture2D>();
				Content->AddProcessor<FileProcessors::ShaderProcessor, Graphics::Shader>();
				Content->AddProcessor<FileProcessors::ModelProcessor, Graphics::Model>();
				Content->AddProcessor<FileProcessors::SkinModelProcessor, Graphics::SkinModel>();
				Content->AddProcessor<FileProcessors::DocumentProcessor, Rest::Document>();
				Content->AddProcessor<FileProcessors::ServerProcessor, Network::HTTP::Server>();
				Content->AddProcessor<FileProcessors::ShapeProcessor, Compute::UnmanagedShape>();
				Content->SetEnvironment(I->Environment.empty() ? Rest::OS::GetDirectory() + I->Directory : I->Environment + I->Directory);
			}

			if (I->Usage & ApplicationUse_AngelScript_Module)
				VM = new Script::VMManager();

			State = ApplicationState_Staging;
		}
		Application::~Application()
		{
			if (Renderer != nullptr)
				Renderer->FlushState();

			delete Scene;
			delete Queue;
			delete VM;
			delete Audio;
			delete Content;
			delete Shaders;
			delete Renderer;
			delete Activity;
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
		void Application::Render(Rest::Timer* Time)
		{
		}
		void Application::Update(Rest::Timer* Time)
		{
		}
		void Application::Initialize(Desc* I)
		{
		}
		void Application::Restate(ApplicationState Value)
		{
			State = Value;
		}
		void Application::Enqueue(const std::function<void(Rest::Timer*)>& Callback, double Limit)
		{
			if (!Queue || !Callback)
				return;

			ThreadEvent* Call = new ThreadEvent();
			Call->Callback = Callback;
			Call->App = this;
			Call->Timer = new Rest::Timer();
			Call->Timer->FrameLimit = Limit;

			if (State == ApplicationState_Staging)
				Workers++;

			Queue->Task<ThreadEvent>(Call, Callee);
		}
		void Application::Run(Desc* I)
		{
			if (!I || !Queue)
			{
				THAWK_ERROR("(CONF): event queue was not found");
				return;
			}

			if (I->Usage & ApplicationUse_Activity_Module && !Activity)
			{
				THAWK_ERROR("(CONF): activity was not found");
				return;
			}

			if (I->Usage & ApplicationUse_Graphics_Module && !Renderer)
			{
				THAWK_ERROR("(CONF): graphics device was not found");
				return;
			}

			if (I->Usage & ApplicationUse_Audio_Module && !Audio)
			{
				THAWK_ERROR("(CONF): audio device was not found");
				return;
			}

			if (I->Usage & ApplicationUse_AngelScript_Module && !VM)
			{
				THAWK_ERROR("(CONF): VM was not found");
				return;
			}

			Initialize(I);
			if (State == ApplicationState_Terminated)
				return;

			if (Scene != nullptr)
				Scene->Denotify();

			if (I->TaskWorkersCount < Workers)
				I->TaskWorkersCount = Workers;

			Rest::Timer* Time = new Rest::Timer();
			Time->SetStepLimitation(I->MaxFrames, I->MinFrames);
			Time->FrameLimit = I->FrameLimit;

			if (I->Threading != Rest::EventWorkflow_Singlethreaded)
			{
				if (!I->TaskWorkersCount)
					THAWK_WARN("tasks will not be processed (no workers)");

				State = ApplicationState_Multithreaded;
				Queue->Start(I->Threading, I->TaskWorkersCount, I->EventWorkersCount);

				if (Activity != nullptr)
				{
					while (State == ApplicationState_Multithreaded)
					{
						Activity->Dispatch();
						Time->Synchronize();
						Update(Time);
						Render(Time);
					}
				}
				else
				{
					while (State == ApplicationState_Multithreaded)
					{
						Time->Synchronize();
						Update(Time);
					}
				}
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
						Time->Synchronize();
						Activity->Dispatch();
						Update(Time);
						Render(Time);
					}
				}
				else
				{
					while (State == ApplicationState_Singlethreaded)
					{
						Queue->Tick();
						Time->Synchronize();
						Update(Time);
					}
				}

				Queue->SetState(Rest::EventState_Idle);
			}

			delete Time;
		}
		void* Application::GetGUI()
		{
			if (!Scene)
				return nullptr;

			Components::Camera* BaseCamera = (Components::Camera*)Scene->GetCamera();
			if (!BaseCamera)
				return nullptr;

			Renderers::GUIRenderer* Renderer = BaseCamera->GetRenderer()->GetRenderer<Renderers::GUIRenderer>();
			return Renderer != nullptr ? Renderer->GetContext() : nullptr;
		}
		void Application::Callee(Rest::EventQueue* Queue, Rest::EventArgs* Args)
		{
			ThreadEvent* Data = Args->Get<ThreadEvent>();
			do
			{
				Data->Callback(Data->Timer);
				Data->Timer->Synchronize();
			}while (Data->App->State == ApplicationState_Multithreaded && Args->Blockable());

			if (Data->App->State != ApplicationState_Singlethreaded)
			{
				delete Data->Timer;
				Args->Free<ThreadEvent>();
			}
			else
				Queue->Task<ThreadEvent>(Data, Callee);
		}
		void Application::Compose()
		{
			Rest::Composer::Push<Components::Model, Entity*>("Model");
			Rest::Composer::Push<Components::LimpidModel, Entity*>("LimpidModel");
			Rest::Composer::Push<Components::Skin, Entity*>("Skin");
			Rest::Composer::Push<Components::LimpidSkin, Entity*>("LimpidSkin");
			Rest::Composer::Push<Components::Emitter, Entity*>("Emitter");
			Rest::Composer::Push<Components::LimpidEmitter, Entity*>("LimpidEmitter");
			Rest::Composer::Push<Components::SoftBody, Entity*>("SoftBody");
			Rest::Composer::Push<Components::LimpidSoftBody, Entity*>("LimpidSoftBody");
			Rest::Composer::Push<Components::Decal, Entity*>("Decal");
			Rest::Composer::Push<Components::LimpidDecal, Entity*>("LimpidDecal");
			Rest::Composer::Push<Components::SkinAnimator, Entity*>("SkinAnimator");
			Rest::Composer::Push<Components::KeyAnimator, Entity*>("KeyAnimator");
			Rest::Composer::Push<Components::EmitterAnimator, Entity*>("EmitterAnimator");
			Rest::Composer::Push<Components::RigidBody, Entity*>("RigidBody");
			Rest::Composer::Push<Components::Acceleration, Entity*>("Acceleration");
			Rest::Composer::Push<Components::SliderConstraint, Entity*>("SliderConstraint");
			Rest::Composer::Push<Components::FreeLook, Entity*>("FreeLook");
			Rest::Composer::Push<Components::Fly, Entity*>("Fly");
			Rest::Composer::Push<Components::AudioSource, Entity*>("AudioSource");
			Rest::Composer::Push<Components::AudioListener, Entity*>("AudioListener");
			Rest::Composer::Push<Components::PointLight, Entity*>("PointLight");
			Rest::Composer::Push<Components::SpotLight, Entity*>("SpotLight");
			Rest::Composer::Push<Components::LineLight, Entity*>("LineLight");
			Rest::Composer::Push<Components::ProbeLight, Entity*>("ProbeLight");
			Rest::Composer::Push<Components::Camera, Entity*>("Camera");
			Rest::Composer::Push<Renderers::ModelRenderer, RenderSystem*>("ModelRenderer");
			Rest::Composer::Push<Renderers::SkinRenderer, RenderSystem*>("SkinRenderer");
			Rest::Composer::Push<Renderers::SoftBodyRenderer, RenderSystem*>("SoftBodyRenderer");
			Rest::Composer::Push<Renderers::EmitterRenderer, RenderSystem*>("EmitterRenderer");
			Rest::Composer::Push<Renderers::DecalRenderer, RenderSystem*>("DecalRenderer");
			Rest::Composer::Push<Renderers::LimpidRenderer, RenderSystem*>("LimpidRenderer");
			Rest::Composer::Push<Renderers::DepthRenderer, RenderSystem*>("DepthRenderer");
			Rest::Composer::Push<Renderers::ProbeRenderer, RenderSystem*>("ProbeRenderer");
			Rest::Composer::Push<Renderers::LightRenderer, RenderSystem*>("LightRenderer");
			Rest::Composer::Push<Renderers::ReflectionsRenderer, RenderSystem*>("ReflectionsRenderer");
			Rest::Composer::Push<Renderers::DepthOfFieldRenderer, RenderSystem*>("DepthOfFieldRenderer");
			Rest::Composer::Push<Renderers::EmissionRenderer, RenderSystem*>("EmissionRenderer");
			Rest::Composer::Push<Renderers::GlitchRenderer, RenderSystem*>("GlitchRenderer");
			Rest::Composer::Push<Renderers::AmbientOcclusionRenderer, RenderSystem*>("AmbientOcclusionRenderer");
			Rest::Composer::Push<Renderers::DirectOcclusionRenderer, RenderSystem*>("DirectOcclusionRenderer");
			Rest::Composer::Push<Renderers::ToneRenderer, RenderSystem*>("ToneRenderer");
			Rest::Composer::Push<Renderers::GUIRenderer, RenderSystem*>("GUIRenderer");
			Rest::Composer::Push<Audio::Effects::ReverbEffect>("ReverbEffect");
			Rest::Composer::Push<Audio::Effects::ChorusEffect>("ChorusEffect");
			Rest::Composer::Push<Audio::Effects::DistortionEffect>("DistortionEffect");
			Rest::Composer::Push<Audio::Effects::EchoEffect>("EchoEffect");
			Rest::Composer::Push<Audio::Effects::FlangerEffect>("FlangerEffect");
			Rest::Composer::Push<Audio::Effects::FrequencyShifterEffect>("FrequencyShifterEffect");
			Rest::Composer::Push<Audio::Effects::VocalMorpherEffect>("VocalMorpherEffect");
			Rest::Composer::Push<Audio::Effects::PitchShifterEffect>("PitchShifterEffect");
			Rest::Composer::Push<Audio::Effects::RingModulatorEffect>("RingModulatorEffect");
			Rest::Composer::Push<Audio::Effects::AutowahEffect>("AutowahEffect");
			Rest::Composer::Push<Audio::Effects::CompressorEffect>("CompressorEffect");
			Rest::Composer::Push<Audio::Effects::EqualizerEffect>("EqualizerEffect");
			Rest::Composer::Push<Audio::Filters::LowpassFilter>("LowpassFilter");
			Rest::Composer::Push<Audio::Filters::BandpassFilter>("BandpassFilter");
			Rest::Composer::Push<Audio::Filters::HighpassFilter>("HighpassFilter");
		}
		Application* Application::Get()
		{
			return Host;
		}
		Application* Application::Host = nullptr;
	}
}