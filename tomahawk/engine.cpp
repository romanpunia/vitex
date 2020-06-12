#include "engine.h"
#include "engine/components.h"
#include "engine/processors.h"
#include "engine/renderers.h"
#include "network/http.h"
#include <sstream>
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#endif

namespace Tomahawk
{
	namespace Engine
	{
		void Appearance::UploadPhase(Graphics::GraphicsDevice* Device, Appearance* Surface)
		{
			if (!Device)
				return;

			if (!Surface)
			{
				Device->Render.HasDiffuse = 0.0f;
				Device->Render.HasNormal = 0.0f;
				Device->Render.MaterialId = 0.0f;
				Device->Render.Diffuse = 1.0f;
				Device->Render.TexCoord = 1.0f;

				return Device->FlushTexture2D(1, 6);
			}

			Device->Render.HasDiffuse = (float)(Surface->DiffuseMap != nullptr);
			Device->Render.HasNormal = (float)(Surface->NormalMap != nullptr);
			Device->Render.MaterialId = (float)Surface->Material;
			Device->Render.Diffuse = Surface->Diffuse;
			Device->Render.TexCoord = Surface->TexCoord;
			Device->SetTexture2D(Surface->DiffuseMap, 1);
			Device->SetTexture2D(Surface->NormalMap, 2);
			Device->SetTexture2D(Surface->MetallicMap, 3);
			Device->SetTexture2D(Surface->HeightMap, 4);
			Device->SetTexture2D(Surface->OcclusionMap, 5);
			Device->SetTexture2D(Surface->EmissionMap, 6);
		}
		void Appearance::UploadDepth(Graphics::GraphicsDevice* Device, Appearance* Surface)
		{
			if (!Device)
				return;

			if (!Surface)
			{
				Device->Render.HasDiffuse = 0.0f;
				Device->Render.Diffuse.X = 1.0f;
				Device->Render.Diffuse.Y = 0.0f;
				Device->Render.TexCoord = 1.0f;

				return Device->FlushTexture2D(1, 1);
			}

			Device->Render.HasDiffuse = (float)(Surface->DiffuseMap != nullptr);
			Device->Render.MaterialId = Surface->Material;
			Device->Render.TexCoord = Surface->TexCoord;
			Device->SetTexture2D(Surface->DiffuseMap, 1);
		}
		void Appearance::UploadCubicDepth(Graphics::GraphicsDevice* Device, Appearance* Surface)
		{
			if (!Device)
				return;

			if (!Surface)
			{
				Device->Render.HasDiffuse = 0.0f;
				Device->Render.Diffuse.X = 1.0f;
				Device->Render.Diffuse.Y = 0.0f;
				Device->Render.TexCoord = 1.0f;

				return Device->FlushTexture2D(1, 1);
			}

			Device->Render.HasDiffuse = (float)(Surface->DiffuseMap != nullptr);
			Device->Render.MaterialId = Surface->Material;
			Device->Render.TexCoord = Surface->TexCoord;
			Device->SetTexture2D(Surface->DiffuseMap, 1);
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
				NMake::Pack(V->SetDocument("clip"), It);

			return true;
		}
		bool NMake::Pack(Rest::Document* V, const std::vector<Compute::KeyAnimatorClip>& Value)
		{
			if (!V)
				return false;

			Rest::Document* Array = V->SetArray("clips");
			for (auto&& It : Value)
				NMake::Pack(V->SetDocument("clip"), It);

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
		void* FileProcessor::Load(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
		{
			return nullptr;
		}
		bool FileProcessor::Save(Rest::FileStream* Stream, void* Object, ContentArgs* Args)
		{
			return false;
		}
		ContentManager* FileProcessor::GetContent()
		{
			return Content;
		}

		Component::Component(Entity* Reference) : Parent(Reference), Active(true)
		{
		}
		Component::~Component()
		{
		}
		void Component::OnLoad(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Component::OnSave(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Component::OnAwake(Component* New)
		{
		}
		void Component::OnAsleep()
		{
		}
		void Component::OnSynchronize(Rest::Timer* Time)
		{
		}
		void Component::OnUpdate(Rest::Timer* Time)
		{
		}
		void Component::OnEvent(Event* Value)
		{
		}
		Component* Component::OnClone(Entity* New)
		{
			Component* Target = new Engine::Component(New);
			Target->Active = Active;

			return Target;
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
				OnAwake(nullptr);
			else
				OnAsleep();

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

		Entity::Entity(SceneGraph* Ref) : Scene(Ref), Tag(-1), Self(-1)
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
				Component.second->OnAwake(In == Component.second ? nullptr : In);

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

		Renderer::Renderer(RenderSystem* Lab) : System(Lab), Active(true), Geometric(true)
		{
		}
		Renderer::~Renderer()
		{
		}
		void Renderer::OnLoad(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Renderer::OnSave(ContentManager* Content, Rest::Document* Node)
		{
		}
		void Renderer::OnResizeBuffers()
		{
		}
		void Renderer::OnInitialize()
		{
		}
		void Renderer::OnRender(Rest::Timer* TimeStep)
		{
		}
		void Renderer::OnDepthRender(Rest::Timer* TimeStep)
		{
		}
		void Renderer::OnCubicDepthRender(Rest::Timer* TimeStep)
		{
		}
		void Renderer::OnCubicDepthRender(Rest::Timer* TimeStep, Compute::Matrix4x4* ViewProjection)
		{
		}
		void Renderer::OnPhaseRender(Rest::Timer* TimeStep)
		{
		}
		void Renderer::OnRelease()
		{
		}
		bool Renderer::IsGeometric()
		{
			return Geometric;
		}
		void Renderer::SetRenderer(RenderSystem* NewSystem)
		{
			System = NewSystem;
		}
		void Renderer::RenderCubicDepth(Rest::Timer* Time, const Compute::Matrix4x4& iProjection, const Compute::Vector4& iPosition)
		{
			if (System && System->GetScene())
				System->GetScene()->RenderCubicDepth(Time, iProjection, iPosition);
		}
		void Renderer::RenderDepth(Rest::Timer* Time, const Compute::Matrix4x4& iView, const Compute::Matrix4x4& iProjection, const Compute::Vector4& iPosition)
		{
			if (System && System->GetScene())
				System->GetScene()->RenderDepth(Time, iView, iProjection, iPosition);
		}
		void Renderer::RenderPhase(Rest::Timer* Time, const Compute::Matrix4x4& iView, const Compute::Matrix4x4& iProjection, const Compute::Vector4& iPosition)
		{
			if (System && System->GetScene())
				System->GetScene()->RenderPhase(Time, iView, iProjection, iPosition);
		}
		RenderSystem* Renderer::GetRenderer()
		{
			return System;
		}

		IntervalRenderer::IntervalRenderer(RenderSystem* Lab) : Renderer(Lab)
		{
		}
		IntervalRenderer::~IntervalRenderer()
		{
		}
		void IntervalRenderer::OnIntervalRender(Rest::Timer* Time)
		{
		}
		void IntervalRenderer::OnImmediateRender(Rest::Timer* Time)
		{
		}
		void IntervalRenderer::OnIntervalDepthRender(Rest::Timer* Time)
		{
		}
		void IntervalRenderer::OnImmediateDepthRender(Rest::Timer* Time)
		{
		}
		void IntervalRenderer::OnIntervalCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
		{
		}
		void IntervalRenderer::OnImmediateCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
		{
		}
		void IntervalRenderer::OnIntervalPhaseRender(Rest::Timer* Time)
		{
		}
		void IntervalRenderer::OnImmediatePhaseRender(Rest::Timer* Time)
		{
		}
		void IntervalRenderer::OnRender(Rest::Timer* Time)
		{
			if (Timer.OnTickEvent(Time->GetElapsedTime()))
				OnIntervalRender(Time);

			OnImmediateRender(Time);
		}
		void IntervalRenderer::OnDepthRender(Rest::Timer* Time)
		{
			if (Timer.OnTickEvent(Time->GetElapsedTime()))
				OnIntervalDepthRender(Time);

			OnImmediateDepthRender(Time);
		}
		void IntervalRenderer::OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
		{
			if (Timer.OnTickEvent(Time->GetElapsedTime()))
				OnIntervalCubicDepthRender(Time, ViewProjection);

			OnImmediateCubicDepthRender(Time, ViewProjection);
		}
		void IntervalRenderer::OnPhaseRender(Rest::Timer* Time)
		{
			if (Timer.OnTickEvent(Time->GetElapsedTime()))
				OnIntervalPhaseRender(Time);

			OnImmediatePhaseRender(Time);
		}

		PostProcessRenderer::PostProcessRenderer(RenderSystem* Lab) : Renderer(Lab), Output(nullptr)
		{
			Geometric = false;
			DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE");
			Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
			Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
			Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");
		}
		PostProcessRenderer::~PostProcessRenderer()
		{
			for (auto It = Shaders.begin(); It != Shaders.end(); It++)
				System->FreeShader(It->first, It->second);

			delete Output;
		}
		void PostProcessRenderer::PostProcess(const std::string& Name, void* Buffer)
		{
			auto It = Shaders.find(Name);
			if (It == Shaders.end() || !It->second)
				return;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetShader(It->second, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(It->second, Buffer);
				Device->SetBuffer(It->second, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			}

			Device->Draw(6, 0);
		}
		void PostProcessRenderer::PostProcess(void* Buffer)
		{
			auto It = Shaders.begin();
			if (It == Shaders.end() || !It->second)
				return;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetShader(It->second, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(It->second, Buffer);
				Device->SetBuffer(It->second, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
			}

			Device->Draw(6, 0);
		}
		void PostProcessRenderer::OnRenderEffect(Rest::Timer* Time)
		{
		}
		void PostProcessRenderer::OnInitialize()
		{
			OnResizeBuffers();
		}
		void PostProcessRenderer::OnRender(Rest::Timer* Time)
		{
			Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
			if (!Surface || Shaders.empty())
				return;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetDepthStencilState(DepthStencil);
			Device->SetSamplerState(Sampler);
			Device->SetBlendState(Blend);
			Device->SetRasterizerState(Rasterizer);
			Device->SetTarget(Output, 0, 0, 0);
			Device->SetTexture2D(Surface->GetTarget(1), 1);
			Device->SetTexture2D(Surface->GetTarget(2), 2);
			Device->SetTexture2D(Surface->GetTarget(0), 3);
			Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);

			OnRenderEffect(Time);

			Device->FlushTexture2D(1, 3);
			Device->CopyTargetFrom(Surface, 0, Output);
		}
		void PostProcessRenderer::OnResizeBuffers()
		{
			Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
			I.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
			I.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			I.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
			I.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
			I.MipLevels = System->GetDevice()->GetMipLevel(I.Width, I.Height);

			delete Output;
			Output = System->GetDevice()->CreateRenderTarget2D(I);
		}
		Graphics::Shader* PostProcessRenderer::CompileEffect(const std::string& Name, const std::string& Code, size_t BufferSize)
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

		RenderSystem::RenderSystem(Graphics::GraphicsDevice* Ref) : Device(Ref), Scene(nullptr), QuadVertex(nullptr), SphereVertex(nullptr), SphereIndex(nullptr)
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
			F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
			F.ElementCount = (unsigned int)Elements.size();
			F.ElementWidth = sizeof(Compute::ShapeVertex);
			F.Elements = &Elements[0];
			F.UseSubresource = true;

			SphereVertex = Device->CreateElementBuffer(F);

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Invalid;
			F.Usage = Graphics::ResourceUsage_Default;
			F.BindFlags = Graphics::ResourceBind_Index_Buffer;
			F.ElementCount = (unsigned int)Indices.size();
			F.ElementWidth = sizeof(int);
			F.Elements = &Indices[0];
			F.UseSubresource = true;

			SphereIndex = Device->CreateElementBuffer(F);

			Elements.resize(6);
			Elements[0] = { -1.0f, -1.0f, 0, -1, 0 };
			Elements[1] = { -1.0f, 1.0f, 0, -1, -1 };
			Elements[2] = { 1.0f, 1.0f, 0, 0, -1 };
			Elements[3] = { -1.0f, -1.0f, 0, -1, 0 };
			Elements[4] = { 1.0f, 1.0f, 0, 0, -1 };
			Elements[5] = { 1.0f, -1.0f, 0, 0, 0 };

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess_Invalid;
			F.Usage = Graphics::ResourceUsage_Default;
			F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
			F.ElementCount = 6;
			F.ElementWidth = sizeof(Compute::ShapeVertex);
			F.Elements = &Elements[0];
			F.UseSubresource = true;

			QuadVertex = Device->CreateElementBuffer(F);
		}
		RenderSystem::~RenderSystem()
		{
			delete QuadVertex;
			delete SphereVertex;
			delete SphereIndex;

			for (auto& RenderStage : RenderStages)
			{
				if (!RenderStage)
					continue;

				RenderStage->OnRelease();
				delete RenderStage;
			}
		}
		void RenderSystem::SetScene(SceneGraph* NewScene)
		{
			Scene = NewScene;
		}
		void RenderSystem::RemoveRenderer(uint64_t Id)
		{
			for (auto It = RenderStages.begin(); It != RenderStages.end(); It++)
			{
				if (*It && (*It)->Id() == Id)
				{
					(*It)->OnRelease();
					delete *It;
					RenderStages.erase(It);
					break;
				}
			}
		}
		Renderer* RenderSystem::AddRenderer(Renderer* In)
		{
			if (!In)
				return nullptr;

			for (auto It = RenderStages.begin(); It != RenderStages.end(); It++)
			{
				if (*It && (*It)->Id() == In->Id())
				{
					(*It)->OnRelease();
					delete (*It);
					RenderStages.erase(It);
					break;
				}
			}

			In->SetRenderer(this);
			In->OnInitialize();
			RenderStages.push_back(In);

			return In;
		}
		Renderer* RenderSystem::GetRenderer(uint64_t Id)
		{
			for (auto& RenderStage : RenderStages)
			{
				if (RenderStage->Id() == Id)
					return RenderStage;
			}

			return nullptr;
		}
		Graphics::ElementBuffer* RenderSystem::GetQuadVBuffer()
		{
			return QuadVertex;
		}
		Graphics::ElementBuffer* RenderSystem::GetSphereVBuffer()
		{
			return SphereVertex;
		}
		Graphics::ElementBuffer* RenderSystem::GetSphereIBuffer()
		{
			return SphereIndex;
		}
		std::vector<Renderer*>* RenderSystem::GetRenderers()
		{
			return &RenderStages;
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
		SceneGraph* RenderSystem::GetScene()
		{
			return Scene;
		}

		SceneGraph::SceneGraph(const Desc& I) : Conf(I)
		{
			Sync.Count = 0;
			Sync.Locked = false;
			for (int i = 0; i < ThreadId_Count; i++)
				Sync.Threads[i].State = 0;

			Materials.reserve(16);
			Materials.emplace_back();
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

			Lock();
			Conf = NewConf;
			Entities.Reserve(Conf.EntityCount);
			Pending.Reserve(Conf.ComponentCount);

			ResizeBuffers();
			if (Camera != nullptr)
				Camera->OnAwake(Camera);

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
		void SceneGraph::Render(Rest::Timer* Time)
		{
			RenderInject(Time, nullptr);
		}
		void SceneGraph::RenderInject(Rest::Timer* Time, const RenderCallback& Callback)
		{
			BeginThread(ThreadId_Render);
			if (Camera != nullptr)
			{
				Conf.Device->UpdateBuffer(Structure, Materials.data(), Materials.size() * sizeof(Graphics::Material));
				Conf.Device->SetBuffer(Structure, 0);
				RestoreViewBuffer(nullptr);
				SetSurfaceCleared();

				auto* RenderStages = View.Renderer->GetRenderers();
				for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
				{
					if ((*It)->Active)
						(*It)->OnRender(Time);
				}

				if (Callback)
					Callback(Time, &View);

				Conf.Device->SetTarget();
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
			for (auto It = Pending.Begin(); It != Pending.End(); It++)
				(*It)->OnSynchronize(Time);

			uint64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				Entity* V = *It;
				V->Transform->Synchronize();
				V->Self = Index;
				Index++;
			}
			EndThread(ThreadId_Synchronize);
		}
		void SceneGraph::Update(Rest::Timer* Time)
		{
			BeginThread(ThreadId_Update);
			for (auto It = Pending.Begin(); It != Pending.End(); It++)
				(*It)->OnUpdate(Time);

			DispatchEvents();
			EndThread(ThreadId_Update);
		}
		void SceneGraph::RenderCubicDepth(Rest::Timer* Time, Compute::Matrix4x4 iProjection, Compute::Vector4 iPosition)
		{
			View.ViewPosition = Compute::Vector3(-iPosition.X, -iPosition.Y, iPosition.Z);
			View.ViewProjection.Identify();
			View.Projection = iProjection;
			View.InvViewProjection = View.ViewProjection.Invert();
			View.RawPosition = iPosition.MtVector3();
			View.Position = View.RawPosition.InvertZ();
			View.ViewDistance = (iPosition.W < 0 ? 999999999 : iPosition.W);
			RestoreViewBuffer(&View);

			Compute::Matrix4x4 ViewProjection[6];
			ViewProjection[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, View.Position) * View.Projection;
			ViewProjection[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, View.Position) * View.Projection;
			ViewProjection[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, View.Position) * View.Projection;
			ViewProjection[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, View.Position) * View.Projection;
			ViewProjection[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, View.Position) * View.Projection;
			ViewProjection[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, View.Position) * View.Projection;

			if (!View.Renderer)
				return;

			auto* RenderStages = View.Renderer->GetRenderers();
			for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
			{
				if ((*It)->Active && (*It)->Geometric)
					(*It)->OnCubicDepthRender(Time, ViewProjection);
			}
		}
		void SceneGraph::RenderDepth(Rest::Timer* Time, Compute::Matrix4x4 iView, Compute::Matrix4x4 iProjection, Compute::Vector4 iPosition)
		{
			View.ViewPosition = Compute::Vector3(-iPosition.X, -iPosition.Y, iPosition.Z);
			View.View = iView;
			View.ViewProjection = iView * iProjection;
			View.InvViewProjection = View.ViewProjection.Invert();
			View.Projection = iProjection;
			View.RawPosition = iPosition.MtVector3();
			View.Position = View.RawPosition.InvertZ();
			View.ViewDistance = (iPosition.W < 0 ? 999999999 : iPosition.W);
			RestoreViewBuffer(&View);

			if (!View.Renderer)
				return;

			auto* RenderStages = View.Renderer->GetRenderers();
			for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
			{
				if ((*It)->Active && (*It)->Geometric)
					(*It)->OnDepthRender(Time);
			}
		}
		void SceneGraph::RenderPhase(Rest::Timer* Time, Compute::Matrix4x4 iView, Compute::Matrix4x4 iProjection, Compute::Vector4 iPosition)
		{
			View.ViewPosition = Compute::Vector3(-iPosition.X, -iPosition.Y, iPosition.Z);
			View.View = iView;
			View.Projection = iProjection;
			View.ViewProjection = iView * iProjection;
			View.InvViewProjection = View.ViewProjection.Invert();
			View.RawPosition = iPosition.MtVector3();
			View.Position = View.RawPosition.InvertZ();
			View.ViewDistance = (iPosition.W < 0 ? 999999999 : iPosition.W);
			RestoreViewBuffer(&View);

			Conf.Device->SetTarget(Surface, 0, 0, 0);
			Conf.Device->ClearDepth(Surface);

			auto* RenderStages = View.Renderer->GetRenderers();
			for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
			{
				if ((*It)->Active && (*It)->Geometric)
					(*It)->OnPhaseRender(Time);
			}
		}
		void SceneGraph::Rescale(const Compute::Vector3& Scaling)
		{
			Lock();
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				Entity* V = *It;
				if (V->Transform->GetRoot() == nullptr)
					V->Transform->Scale *= Scaling;
			}
			Unlock();
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
			GetCamera();
		}
		void SceneGraph::Reindex()
		{
			Lock();
			int64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); It++)
			{
				(*It)->Self = Index;
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
				float Distance1 = Entity1->Transform->Position.Distance(View.RawPosition);
				float Distance2 = Entity2->Transform->Position.Distance(View.RawPosition);

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
		void SceneGraph::SetCamera(Entity* In)
		{
			if (In != nullptr)
			{
				auto Viewer = In->GetComponent<Components::Camera>();
				if (Viewer != nullptr && Viewer->Active)
				{
					Viewer->OnAwake(nullptr);
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
			if (Material < 1 || Material >= Materials.size())
				return;

			NotifyEach<Graphics::Material>(nullptr, Materials[Material]);
			Materials[Material] = Materials.back();

			if (!Materials.empty())
				Materials.resize(Materials.size() - 1);

			for (uint64_t i = 0; i < Materials.size(); i++)
				Materials[i].Self = (float)i;
		}
		void SceneGraph::RegisterEntity(Entity* In)
		{
			if (!In)
				return;

			In->SetScene(this);
			for (auto& Component : In->Components)
			{
				Component.second->OnAwake(Component.second);

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
		bool SceneGraph::UnregisterEntity(Entity* Entity)
		{
			if (Camera != nullptr && Entity == Camera->Parent)
			{
				Lock();
				Camera = nullptr;
				Unlock();
			}

			if (!Entity || !Entity->GetScene())
				return false;

			for (auto& Component : Entity->Components)
			{
				Component.second->OnAsleep();

				auto Storage = &Components[Component.second->Id()];
				Storage->Remove(Component.second);
				Pending.Remove(Component.second);
			}

			Entities.Remove(Entity);
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
		void SceneGraph::AddMaterial(const Graphics::Material& From)
		{
			Graphics::Material Material = From;
			Material.Self = (float)Materials.size();

			if (Materials.size() + 1 < Materials.capacity())
				return Materials.push_back(Material);

			ExpandMaterialStructure();
			Materials.push_back(Material);
		}
		void SceneGraph::RestoreViewBuffer(Viewer* iView)
		{
			if (&View != iView)
			{
				if (iView == nullptr)
				{
					if (Camera != nullptr)
						Camera->As<Components::Camera>()->FillViewer(&View);
				}
				else
					View = *iView;
			}

			Conf.Device->View.InvViewProjection = View.InvViewProjection;
			Conf.Device->View.ViewProjection = View.ViewProjection;
			Conf.Device->View.Projection = View.Projection;
			Conf.Device->View.View = View.View;
			Conf.Device->View.ViewPosition = View.Position;
			Conf.Device->View.FarPlane = View.ViewDistance;
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType_View);
		}
		void SceneGraph::ExpandMaterialStructure()
		{
			Lock();
			for (size_t i = 0; i < Materials.size(); i++)
				Materials[i].Self = (float)i;
			Materials.reserve(Materials.capacity() * 2);

			Graphics::StructureBuffer::Desc F = Graphics::StructureBuffer::Desc();
			F.ElementCount = (unsigned int)Materials.capacity();
			F.ElementWidth = sizeof(Graphics::Material);
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
							K->second->OnEvent(Message);
					}
				}
				else
				{
					for (auto It = Message->Root->Parent->First(); It != Message->Root->Parent->Last(); It++)
						It->second->OnEvent(Message);
				}
			}
			else if (Message->Root)
				Message->Root->OnEvent(Message);

			if (Message->Context != nullptr)
				free(Message->Context);

			delete Message;
		}
		void SceneGraph::ResizeBuffers()
		{
			Graphics::MultiRenderTarget2D::Desc F = Graphics::MultiRenderTarget2D::Desc();
			F.FormatMode[0] = Graphics::Format_R8G8B8A8_Unorm;
			F.FormatMode[1] = Graphics::Format_R16G16B16A16_Float;
			F.FormatMode[2] = Graphics::Format_R32G32_Float;
			F.FormatMode[3] = Graphics::Format_Invalid;
			F.FormatMode[4] = Graphics::Format_Invalid;
			F.FormatMode[5] = Graphics::Format_Invalid;
			F.FormatMode[6] = Graphics::Format_Invalid;
			F.FormatMode[7] = Graphics::Format_Invalid;
			F.SVTarget = Graphics::SurfaceTarget2;
			F.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
			F.Width = (unsigned int)(Conf.Device->GetRenderTarget()->GetWidth() * Conf.RenderQuality);
			F.Height = (unsigned int)(Conf.Device->GetRenderTarget()->GetHeight() * Conf.RenderQuality);
			F.MipLevels = Conf.Device->GetMipLevel(F.Width, F.Height);

			if (Camera != nullptr)
			{
				Lock();
				delete Surface;
				Surface = Conf.Device->CreateMultiRenderTarget2D(F);

				auto* Array = GetComponents(THAWK_COMPONENT_ID(Camera));
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
		void SceneGraph::SetSurface(Graphics::MultiRenderTarget2D* NewSurface)
		{
			Surface = NewSurface;
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
			Conf.Device->ClearDepth(Surface);
		}
		void SceneGraph::ClearSurface()
		{
			Conf.Device->Clear(Surface, 0, 0, 0, 0);
			Conf.Device->Clear(Surface, 1, 0, 0, 0);
			Conf.Device->Clear(Surface, 2, 1, 0, 0);
			Conf.Device->ClearDepth(Surface);
		}
		Component* SceneGraph::GetCamera()
		{
			if (Camera != nullptr)
				return Camera;

			auto Viewer = GetComponent<Components::Camera>();
			if (Viewer != nullptr && Viewer->Active)
			{
				Viewer->OnAwake(Viewer);
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
			return GetCamera()->As<Components::Camera>()->GetRenderer();
		}
		Viewer SceneGraph::GetCameraViewer()
		{
			return GetCamera()->As<Components::Camera>()->GetViewer();
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
			Instance->Self = Entity->Self;
			Instance->Components = Entity->Components;

			for (auto& It : Instance->Components)
			{
				Component* Source = It.second;
				It.second = Source->OnClone(Instance);
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
		Graphics::Material& SceneGraph::CloneMaterial(uint64_t Material)
		{
			if (Material < 0 || Material >= Materials.capacity())
				return GetMaterialStandartLit();

			AddMaterial(Materials[Material]);
			return Materials.back();
		}
		Graphics::Material& SceneGraph::GetMaterial(uint64_t Material)
		{
			if (Material >= Materials.size())
				return Materials.front();

			return Materials[Material];
		}
		Graphics::Material& SceneGraph::GetMaterialStandartLit()
		{
			return Materials.front();
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
		uint64_t SceneGraph::HasEntity(Entity* Entity)
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
		uint64_t SceneGraph::HasEntity(uint64_t Entity)
		{
			return (Entity >= 0 && Entity < Entities.Size()) ? Entity : -1;
		}
		uint64_t SceneGraph::HasMaterial(uint64_t Material)
		{
			return (Material >= 0 && Material < Materials.size()) ? Material : -1;
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
		void ContentManager::SetEnvironment(const std::string& Path)
		{
			Mutex.lock();
			Environment = Rest::OS::ResolveDir(Path.c_str());
			Mutex.unlock();
		}
		void* ContentManager::LoadForward(const std::string& Path, FileProcessor* Processor, ContentMap* Map)
		{
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
			Object = Processor->Load(Stream, Stream->Size(), 0, &Args);
			delete Stream;

			return Object;
		}
		void* ContentManager::LoadStreaming(const std::string& Path, FileProcessor* Processor, ContentMap* Map)
		{
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

			return Processor->Load(Stream, Docker->second->Length, It->second + Docker->second->Offset, &Args);
		}
		bool ContentManager::SaveForward(const std::string& Path, FileProcessor* Processor, void* Object, ContentMap* Map)
		{
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

			bool Result = Processor->Save(Stream, Object, &Args);
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
						auto* GUI = (Renderers::GUI::Interface*)GetAnyGUI();
						if (GUI != nullptr)
							GUI->ApplyKeyState(Key, Mod, Virtual, Repeat, Pressed);

						OnKeyState(Key, Mod, Virtual, Repeat, Pressed);
					};
					Activity->Callbacks.Input = [this](char* Buffer, int Length)
					{
						auto* GUI = (Renderers::GUI::Interface*)GetAnyGUI();
						if (GUI != nullptr)
							GUI->ApplyInput(Buffer, Length);

						OnInput(Buffer, Length);
					};
					Activity->Callbacks.CursorWheelState = [this](int X, int Y, bool Normal)
					{
						auto* GUI = (Renderers::GUI::Interface*)GetAnyGUI();
						if (GUI != nullptr)
							GUI->ApplyCursorWheelState(X, Y, Normal);

						OnCursorWheelState(X, Y, Normal);
					};
					Activity->Callbacks.WindowStateChange = [this](Graphics::WindowState NewState, int X, int Y)
					{
						OnWindowState(NewState, X, Y);
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
				Content->AddProcessor<FileProcessors::SceneGraphProcessor, Engine::SceneGraph>();
				Content->AddProcessor<FileProcessors::FontProcessor, Renderers::GUIRenderer>();
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
		void Application::OnKeyState(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
		{
		}
		void Application::OnInput(char* Buffer, int Length)
		{
		}
		void Application::OnCursorWheelState(int X, int Y, bool Normal)
		{
		}
		void Application::OnWindowState(Graphics::WindowState NewState, int X, int Y)
		{
		}
		void Application::OnInteract(Engine::Renderer* GUI, Rest::Timer* Time)
		{
		}
		void Application::OnRender(Rest::Timer* Time)
		{
		}
		void Application::OnUpdate(Rest::Timer* Time)
		{
		}
		void Application::OnInitialize(Desc* I)
		{
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

			OnInitialize(I);
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
						OnUpdate(Time);
						OnRender(Time);
					}
				}
				else
				{
					while (State == ApplicationState_Multithreaded)
					{
						Time->Synchronize();
						OnUpdate(Time);
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
						OnUpdate(Time);
						OnRender(Time);
					}
				}
				else
				{
					while (State == ApplicationState_Singlethreaded)
					{
						Queue->Tick();
						Time->Synchronize();
						OnUpdate(Time);
					}
				}

				Queue->SetState(Rest::EventState_Idle);
			}

			delete Time;
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
		void* Application::GetCurrentGUI()
		{
			if (!Scene)
				return nullptr;

			Components::Camera* BaseCamera = (Components::Camera*)Scene->GetCamera();
			if (!BaseCamera)
				return nullptr;

			Renderers::GUIRenderer* BaseGui = BaseCamera->GetRenderer()->GetRenderer<Renderers::GUIRenderer>();
			if (!BaseGui || !BaseGui->IsTreeActive())
				return nullptr;

			return BaseGui->GetTree();
		}
		void* Application::GetAnyGUI()
		{
			if (!Scene)
				return nullptr;

			Components::Camera* BaseCamera = (Components::Camera*)Scene->GetCamera();
			if (!BaseCamera)
				return nullptr;

			Renderers::GUIRenderer* BaseGui = BaseCamera->GetRenderer()->GetRenderer<Renderers::GUIRenderer>();
			if (!BaseGui)
				return nullptr;

			return BaseGui->GetTree();
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
		Application* Application::Get()
		{
			return Host;
		}
		Application* Application::Host = nullptr;
	}
}