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

		FragmentQuery::FragmentQuery() : Fragments(1), Query(nullptr), Satisfied(1)
		{

		}
		FragmentQuery::~FragmentQuery()
		{
			TH_RELEASE(Query);
		}
		bool FragmentQuery::Begin(Graphics::GraphicsDevice* Device)
		{
			if (!Device)
				return false;

			if (!Query)
			{
				Graphics::Query::Desc I;
				I.Predicate = false;

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
		void FragmentQuery::End(Graphics::GraphicsDevice* Device)
		{
			if (Device && Satisfied == 0)
			{
				Satisfied = -1;
				Device->QueryEnd(Query);
			}
		}
		void FragmentQuery::Clear()
		{
			Fragments = 1;
		}
		int FragmentQuery::Fetch(RenderSystem* System)
		{
			if (!System || !Query || Satisfied != -1)
				return -1;

			if (!System->GetDevice()->GetQueryData(Query, &Fragments))
				return -1;

			Satisfied = 1 + System->StallFrames;
			return Fragments > 0;
		}
		uint64_t FragmentQuery::GetPassed()
		{
			return Fragments;
		}

		void Viewer::Set(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Near, float _Far)
		{
			View = _View;
			Projection = _Projection;
			ViewProjection = _View * _Projection;
			InvViewProjection = ViewProjection.Inv();
			InvViewPosition = _Position.InvZ();
			ViewPosition = InvViewPosition.Inv();
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

		Reactor::Reactor(Application* Ref, double Limit, JobCallback Callback) : App(Ref), Src(Callback)
		{
			Time = new Core::Timer();
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

		void NMake::Pack(Core::Document* V, bool Value)
		{
			if (V != nullptr)
                V->SetAttribute("b", Core::Var::Boolean(Value));
		}
		void NMake::Pack(Core::Document* V, int Value)
		{
			if (V != nullptr)
                V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void NMake::Pack(Core::Document* V, unsigned int Value)
		{
			if (V != nullptr)
                V->SetAttribute("i", Core::Var::Integer(Value));
		}
        void NMake::Pack(Core::Document* V, unsigned long Value)
        {
            if (V != nullptr)
                V->SetAttribute("i", Core::Var::Integer(Value));
        }
		void NMake::Pack(Core::Document* V, float Value)
		{
			if (V != nullptr)
                V->SetAttribute("n", Core::Var::Number(Value));
		}
		void NMake::Pack(Core::Document* V, double Value)
		{
			if (V != nullptr)
                V->SetAttribute("n", Core::Var::Number(Value));
		}
		void NMake::Pack(Core::Document* V, int64_t Value)
		{
			if (V != nullptr)
                V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void NMake::Pack(Core::Document* V, long double Value)
		{
			if (V != nullptr)
                V->SetAttribute("n", Core::Var::Number(Value));
		}
		void NMake::Pack(Core::Document* V, unsigned long long Value)
		{
			if (V != nullptr)
                V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void NMake::Pack(Core::Document* V, const char* Value)
		{
			V->SetAttribute("s", Core::Var::String(Value ? Value : ""));
		}
		void NMake::Pack(Core::Document* V, const Compute::Vector2& Value)
		{
			if (!V)
				return;

            V->SetAttribute("x", Core::Var::Number(Value.X));
            V->SetAttribute("y", Core::Var::Number(Value.Y));
		}
		void NMake::Pack(Core::Document* V, const Compute::Vector3& Value)
		{
			if (!V)
				return;

            V->SetAttribute("x", Core::Var::Number(Value.X));
            V->SetAttribute("y", Core::Var::Number(Value.Y));
            V->SetAttribute("z", Core::Var::Number(Value.Z));
		}
		void NMake::Pack(Core::Document* V, const Compute::Vector4& Value)
		{
			if (!V)
				return;

            V->SetAttribute("x", Core::Var::Number(Value.X));
            V->SetAttribute("y", Core::Var::Number(Value.Y));
            V->SetAttribute("z", Core::Var::Number(Value.Z));
            V->SetAttribute("w", Core::Var::Number(Value.W));
		}
		void NMake::Pack(Core::Document* V, const Compute::Matrix4x4& Value)
		{
			if (!V)
				return;

            V->SetAttribute("m11", Core::Var::Number(Value.Row[0]));
            V->SetAttribute("m12", Core::Var::Number(Value.Row[1]));
            V->SetAttribute("m13", Core::Var::Number(Value.Row[2]));
            V->SetAttribute("m14", Core::Var::Number(Value.Row[3]));
            V->SetAttribute("m21", Core::Var::Number(Value.Row[4]));
            V->SetAttribute("m22", Core::Var::Number(Value.Row[5]));
            V->SetAttribute("m23", Core::Var::Number(Value.Row[6]));
            V->SetAttribute("m24", Core::Var::Number(Value.Row[7]));
            V->SetAttribute("m31", Core::Var::Number(Value.Row[8]));
            V->SetAttribute("m32", Core::Var::Number(Value.Row[9]));
            V->SetAttribute("m33", Core::Var::Number(Value.Row[10]));
            V->SetAttribute("m34", Core::Var::Number(Value.Row[11]));
            V->SetAttribute("m41", Core::Var::Number(Value.Row[12]));
            V->SetAttribute("m42", Core::Var::Number(Value.Row[13]));
            V->SetAttribute("m43", Core::Var::Number(Value.Row[14]));
            V->SetAttribute("m44", Core::Var::Number(Value.Row[15]));
		}
		void NMake::Pack(Core::Document* V, const Attenuation& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("range"), Value.Range);
			NMake::Pack(V->Set("c1"), Value.C1);
			NMake::Pack(V->Set("c2"), Value.C2);
		}
		void NMake::Pack(Core::Document* V, const AnimatorState& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("looped"), Value.Looped);
			NMake::Pack(V->Set("paused"), Value.Paused);
			NMake::Pack(V->Set("blended"), Value.Blended);
			NMake::Pack(V->Set("clip"), Value.Clip);
			NMake::Pack(V->Set("frame"), Value.Frame);
			NMake::Pack(V->Set("rate"), Value.Rate);
			NMake::Pack(V->Set("duration"), Value.Duration);
			NMake::Pack(V->Set("time"), Value.Time);
		}
		void NMake::Pack(Core::Document* V, const SpawnerProperties& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("iterations"), Value.Iterations);

			Core::Document* Angular = V->Set("angular");
			NMake::Pack(Angular->Set("intensity"), Value.Angular.Intensity);
			NMake::Pack(Angular->Set("accuracy"), Value.Angular.Accuracy);
			NMake::Pack(Angular->Set("min"), Value.Angular.Min);
			NMake::Pack(Angular->Set("max"), Value.Angular.Max);

			Core::Document* Diffusion = V->Set("diffusion");
			NMake::Pack(Diffusion->Set("intensity"), Value.Diffusion.Intensity);
			NMake::Pack(Diffusion->Set("accuracy"), Value.Diffusion.Accuracy);
			NMake::Pack(Diffusion->Set("min"), Value.Diffusion.Min);
			NMake::Pack(Diffusion->Set("max"), Value.Diffusion.Max);

			Core::Document* Noise = V->Set("noise");
			NMake::Pack(Noise->Set("intensity"), Value.Noise.Intensity);
			NMake::Pack(Noise->Set("accuracy"), Value.Noise.Accuracy);
			NMake::Pack(Noise->Set("min"), Value.Noise.Min);
			NMake::Pack(Noise->Set("max"), Value.Noise.Max);

			Core::Document* Position = V->Set("position");
			NMake::Pack(Position->Set("intensity"), Value.Position.Intensity);
			NMake::Pack(Position->Set("accuracy"), Value.Position.Accuracy);
			NMake::Pack(Position->Set("min"), Value.Position.Min);
			NMake::Pack(Position->Set("max"), Value.Position.Max);

			Core::Document* Rotation = V->Set("rotation");
			NMake::Pack(Rotation->Set("intensity"), Value.Rotation.Intensity);
			NMake::Pack(Rotation->Set("accuracy"), Value.Rotation.Accuracy);
			NMake::Pack(Rotation->Set("min"), Value.Rotation.Min);
			NMake::Pack(Rotation->Set("max"), Value.Rotation.Max);

			Core::Document* Scale = V->Set("scale");
			NMake::Pack(Scale->Set("intensity"), Value.Scale.Intensity);
			NMake::Pack(Scale->Set("accuracy"), Value.Scale.Accuracy);
			NMake::Pack(Scale->Set("min"), Value.Scale.Min);
			NMake::Pack(Scale->Set("max"), Value.Scale.Max);

			Core::Document* Velocity = V->Set("velocity");
			NMake::Pack(Velocity->Set("intensity"), Value.Velocity.Intensity);
			NMake::Pack(Velocity->Set("accuracy"), Value.Velocity.Accuracy);
			NMake::Pack(Velocity->Set("min"), Value.Velocity.Min);
			NMake::Pack(Velocity->Set("max"), Value.Velocity.Max);
		}
		void NMake::Pack(Core::Document* V, Material* Value, ContentManager* Content)
		{
			if (!V || !Value || !Content)
				return;

			AssetCache* Asset = Content->Find<Graphics::Texture2D>(Value->GetDiffuseMap());
			if (Asset)
				NMake::Pack(V->Set("diffuse-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value->GetNormalMap());
			if (Asset)
				NMake::Pack(V->Set("normal-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value->GetMetallicMap());
			if (Asset)
				NMake::Pack(V->Set("metallic-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value->GetRoughnessMap());
			if (Asset)
				NMake::Pack(V->Set("roughness-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value->GetHeightMap());
			if (Asset)
				NMake::Pack(V->Set("height-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value->GetOcclusionMap());
			if (Asset)
				NMake::Pack(V->Set("occlusion-map"), Asset->Path);

			Asset = Content->Find<Graphics::Texture2D>(Value->GetEmissionMap());
			if (Asset)
				NMake::Pack(V->Set("emission-map"), Asset->Path);

			NMake::Pack(V->Set("emission"), Value->Surface.Emission);
			NMake::Pack(V->Set("metallic"), Value->Surface.Metallic);
			NMake::Pack(V->Set("diffuse"), Value->Surface.Diffuse);
			NMake::Pack(V->Set("scatter"), Value->Surface.Scatter);
			NMake::Pack(V->Set("roughness"), Value->Surface.Roughness);
			NMake::Pack(V->Set("occlusion"), Value->Surface.Occlusion);
			NMake::Pack(V->Set("fresnel"), Value->Surface.Fresnel);
			NMake::Pack(V->Set("refraction"), Value->Surface.Refraction);
			NMake::Pack(V->Set("transparency"), Value->Surface.Transparency);
			NMake::Pack(V->Set("environment"), Value->Surface.Environment);
			NMake::Pack(V->Set("radius"), Value->Surface.Radius);
			NMake::Pack(V->Set("height"), Value->Surface.Height);
			NMake::Pack(V->Set("bias"), Value->Surface.Bias);
			NMake::Pack(V->Set("name"), Value->Name);
			NMake::Pack(V->Set("slot"), Value->Slot);
		}
		void NMake::Pack(Core::Document* V, const Compute::SkinAnimatorKey& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("pose"), Value.Pose);
			NMake::Pack(V->Set("time"), Value.Time);
		}
		void NMake::Pack(Core::Document* V, const Compute::SkinAnimatorClip& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("name"), Value.Name);
			NMake::Pack(V->Set("duration"), Value.Duration);
			NMake::Pack(V->Set("rate"), Value.Rate);

            Core::Document* Array = V->Set("frames", Core::Var::Array());
			for (auto&& It : Value.Keys)
				NMake::Pack(Array->Set("frame"), It);
		}
		void NMake::Pack(Core::Document* V, const Compute::KeyAnimatorClip& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("name"), Value.Name);
			NMake::Pack(V->Set("rate"), Value.Rate);
			NMake::Pack(V->Set("duration"), Value.Duration);
			NMake::Pack(V->Set("frames"), Value.Keys);
		}
		void NMake::Pack(Core::Document* V, const Compute::AnimatorKey& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("position"), Value.Position);
			NMake::Pack(V->Set("rotation"), Value.Rotation);
			NMake::Pack(V->Set("scale"), Value.Scale);
			NMake::Pack(V->Set("time"), Value.Time);
		}
		void NMake::Pack(Core::Document* V, const Compute::ElementVertex& Value)
		{
			if (!V)
				return;
			
			V->SetAttribute("px", Core::Var::Number(Value.PositionX));
			V->SetAttribute("py", Core::Var::Number(Value.PositionY));
			V->SetAttribute("pz", Core::Var::Number(Value.PositionZ));
			V->SetAttribute("vx", Core::Var::Number(Value.VelocityX));
			V->SetAttribute("vy", Core::Var::Number(Value.VelocityY));
			V->SetAttribute("vz", Core::Var::Number(Value.VelocityZ));
			V->SetAttribute("cx", Core::Var::Number(Value.ColorX));
			V->SetAttribute("cy", Core::Var::Number(Value.ColorY));
			V->SetAttribute("cz", Core::Var::Number(Value.ColorZ));
			V->SetAttribute("cw", Core::Var::Number(Value.ColorW));
			V->SetAttribute("a", Core::Var::Number(Value.Angular));
			V->SetAttribute("s", Core::Var::Number(Value.Scale));
			V->SetAttribute("r", Core::Var::Number(Value.Rotation));
		}
		void NMake::Pack(Core::Document* V, const Compute::Vertex& Value)
		{
			if (!V)
				return;

			V->SetAttribute("px", Core::Var::Number(Value.PositionX));
			V->SetAttribute("py", Core::Var::Number(Value.PositionY));
			V->SetAttribute("pz", Core::Var::Number(Value.PositionZ));
			V->SetAttribute("tx", Core::Var::Number(Value.TexCoordX));
			V->SetAttribute("ty", Core::Var::Number(Value.TexCoordY));
			V->SetAttribute("nx", Core::Var::Number(Value.NormalX));
			V->SetAttribute("ny", Core::Var::Number(Value.NormalY));
			V->SetAttribute("nz", Core::Var::Number(Value.NormalZ));
			V->SetAttribute("tnx", Core::Var::Number(Value.TangentX));
			V->SetAttribute("tny", Core::Var::Number(Value.TangentY));
			V->SetAttribute("tnz", Core::Var::Number(Value.TangentZ));
			V->SetAttribute("btx", Core::Var::Number(Value.BitangentX));
			V->SetAttribute("bty", Core::Var::Number(Value.BitangentY));
			V->SetAttribute("btz", Core::Var::Number(Value.BitangentZ));
		}
		void NMake::Pack(Core::Document* V, const Compute::SkinVertex& Value)
		{
			if (!V)
				return;

			V->SetAttribute("px", Core::Var::Number(Value.PositionX));
			V->SetAttribute("py", Core::Var::Number(Value.PositionY));
			V->SetAttribute("pz", Core::Var::Number(Value.PositionZ));
			V->SetAttribute("tx", Core::Var::Number(Value.TexCoordX));
			V->SetAttribute("ty", Core::Var::Number(Value.TexCoordY));
			V->SetAttribute("nx", Core::Var::Number(Value.NormalX));
			V->SetAttribute("ny", Core::Var::Number(Value.NormalY));
			V->SetAttribute("nz", Core::Var::Number(Value.NormalZ));
			V->SetAttribute("tnx", Core::Var::Number(Value.TangentX));
			V->SetAttribute("tny", Core::Var::Number(Value.TangentY));
			V->SetAttribute("tnz", Core::Var::Number(Value.TangentZ));
			V->SetAttribute("btx", Core::Var::Number(Value.BitangentX));
			V->SetAttribute("bty", Core::Var::Number(Value.BitangentY));
			V->SetAttribute("btz", Core::Var::Number(Value.BitangentZ));
			V->SetAttribute("ji0", Core::Var::Number(Value.JointIndex0));
			V->SetAttribute("ji1", Core::Var::Number(Value.JointIndex1));
			V->SetAttribute("ji2", Core::Var::Number(Value.JointIndex2));
			V->SetAttribute("ji3", Core::Var::Number(Value.JointIndex3));
			V->SetAttribute("jb0", Core::Var::Number(Value.JointBias0));
			V->SetAttribute("jb1", Core::Var::Number(Value.JointBias1));
			V->SetAttribute("jb2", Core::Var::Number(Value.JointBias2));
			V->SetAttribute("jb3", Core::Var::Number(Value.JointBias3));
		}
		void NMake::Pack(Core::Document* V, const Compute::Joint& Value)
		{
			if (!V)
				return;

			NMake::Pack(V->Set("index"), Value.Index);
			NMake::Pack(V->Set("name"), Value.Name);
			NMake::Pack(V->Set("transform"), Value.Transform);
			NMake::Pack(V->Set("bind-shape"), Value.BindShape);

            Core::Document* Joints = V->Set("childs", Core::Var::Array());
			for (auto& It : Value.Childs)
				NMake::Pack(Joints->Set("joint"), It);
		}
		void NMake::Pack(Core::Document* V, const Core::TickTimer& Value)
		{
			if (!V)
				return;

			V->SetAttribute("delay", Core::Var::Number(Value.Delay));
		}
		void NMake::Pack(Core::Document* V, const std::string& Value)
		{
			if (!V)
				return;

            V->SetAttribute("s", Core::Var::String(Value));
		}
		void NMake::Pack(Core::Document* V, const std::vector<bool>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("b-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<int>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<unsigned int>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<float>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<double>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<int64_t>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<long double>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<uint64_t>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

            V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::Vector2>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " ";

            V->Set("v2-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::Vector3>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " " << It.Z << " ";

            V->Set("v3-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::Vector4>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " " << It.Z << " " << It.W << " ";

            V->Set("v4-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::Matrix4x4>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
			{
				for (float i : It.Row)
					Stream << i << " ";
			}

            V->Set("m4x4-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<AnimatorState>& Value)
		{
			if (!V)
				return;

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

            V->Set("as-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<SpawnerProperties>& Value)
		{
			if (!V)
				return;

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

            V->Set("sp-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::SkinAnimatorClip>& Value)
		{
			if (!V)
				return;

            Core::Document* Array = V->Set("clips", Core::Var::Array());
			for (auto&& It : Value)
				NMake::Pack(Array->Set("clip"), It);
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::KeyAnimatorClip>& Value)
		{
			if (!V)
				return;

            Core::Document* Array = V->Set("clips", Core::Var::Array());
			for (auto&& It : Value)
				NMake::Pack(Array->Set("clip"), It);
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::AnimatorKey>& Value)
		{
			if (!V)
				return;

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

            V->Set("ak-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::ElementVertex>& Value)
		{
			if (!V)
				return;

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

            V->Set("ev-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::Joint>& Value)
		{
			if (!V)
				return;

			for (auto&& It : Value)
				NMake::Pack(V->Set("joint"), It);
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::Vertex>& Value)
		{
			if (!V)
				return;

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

            V->Set("iv-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Compute::SkinVertex>& Value)
		{
			if (!V)
				return;

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

            V->Set("iv-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<Core::TickTimer>& Value)
		{
			if (!V)
				return;

			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.Delay << " ";

            V->Set("tt-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void NMake::Pack(Core::Document* V, const std::vector<std::string>& Value)
		{
			if (!V)
				return;

            Core::Document* Array = V->Set("s-array", Core::Var::Array());
			for (auto&& It : Value)
                Array->Set("s", Core::Var::String(It));

            V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		bool NMake::Unpack(Core::Document* V, bool* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetVar("[b]").GetBoolean();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, int* O)
		{
			if (!V || !O)
				return false;

			*O = (int)V->GetVar("[i]").GetInteger();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, unsigned int* O)
		{
			if (!V || !O)
				return false;

			*O = (unsigned int)V->GetVar("[i]").GetInteger();
			return true;
		}
        bool NMake::Unpack(Core::Document* V, unsigned long* O)
        {
            if (!V || !O)
                return false;

            *O = (unsigned long)V->GetVar("[i]").GetInteger();
            return true;
        }
		bool NMake::Unpack(Core::Document* V, float* O)
		{
			if (!V || !O)
				return false;

			*O = (float)V->GetVar("[n]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, double* O)
		{
			if (!V || !O)
				return false;

			*O = (int)V->GetVar("[n]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, long double* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetVar("[n]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, int64_t* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetVar("[i]").GetInteger();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, unsigned long long* O)
		{
			if (!V || !O)
				return false;

			*O = (unsigned long long)V->GetVar("[i]").GetInteger();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::Vector2* O)
		{
			if (!V || !O)
				return false;

			O->X = (float)V->GetVar("[x]").GetNumber();
			O->Y = (float)V->GetVar("[y]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::Vector3* O)
		{
			if (!V || !O)
				return false;

			O->X = (float)V->GetVar("[x]").GetNumber();
			O->Y = (float)V->GetVar("[y]").GetNumber();
			O->Z = (float)V->GetVar("[z]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::Vector4* O)
		{
			if (!V || !O)
				return false;

			O->X = (float)V->GetVar("[x]").GetNumber();
			O->Y = (float)V->GetVar("[y]").GetNumber();
			O->Z = (float)V->GetVar("[z]").GetNumber();
			O->W = (float)V->GetVar("[w]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::Matrix4x4* O)
		{
			if (!V || !O)
				return false;

			O->Row[0] = (float)V->GetVar("[m11]").GetNumber();
			O->Row[1] = (float)V->GetVar("[m12]").GetNumber();
			O->Row[2] = (float)V->GetVar("[m13]").GetNumber();
			O->Row[3] = (float)V->GetVar("[m14]").GetNumber();
			O->Row[4] = (float)V->GetVar("[m21]").GetNumber();
			O->Row[5] = (float)V->GetVar("[m22]").GetNumber();
			O->Row[6] = (float)V->GetVar("[m23]").GetNumber();
			O->Row[7] = (float)V->GetVar("[m24]").GetNumber();
			O->Row[8] = (float)V->GetVar("[m31]").GetNumber();
			O->Row[9] = (float)V->GetVar("[m32]").GetNumber();
			O->Row[10] = (float)V->GetVar("[m33]").GetNumber();
			O->Row[11] = (float)V->GetVar("[m34]").GetNumber();
			O->Row[12] = (float)V->GetVar("[m41]").GetNumber();
			O->Row[13] = (float)V->GetVar("[m42]").GetNumber();
			O->Row[14] = (float)V->GetVar("[m43]").GetNumber();
			O->Row[15] = (float)V->GetVar("[m44]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Attenuation* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("range"), &O->Range);
			NMake::Unpack(V->Get("c2"), &O->C1);
			NMake::Unpack(V->Get("c1"), &O->C2);
			return true;
		}
		bool NMake::Unpack(Core::Document* V, AnimatorState* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("looped"), &O->Looped);
			NMake::Unpack(V->Get("paused"), &O->Paused);
			NMake::Unpack(V->Get("blended"), &O->Blended);
			NMake::Unpack(V->Get("clip"), &O->Clip);
			NMake::Unpack(V->Get("frame"), &O->Frame);
			NMake::Unpack(V->Get("rate"), &O->Rate);
			NMake::Unpack(V->Get("duration"), &O->Duration);
			NMake::Unpack(V->Get("time"), &O->Time);
			return true;
		}
		bool NMake::Unpack(Core::Document* V, SpawnerProperties* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("iterations"), &O->Iterations);

			Core::Document* Angular = V->Get("angular");
			NMake::Unpack(Angular->Get("intensity"), &O->Angular.Intensity);
			NMake::Unpack(Angular->Get("accuracy"), &O->Angular.Accuracy);
			NMake::Unpack(Angular->Get("min"), &O->Angular.Min);
			NMake::Unpack(Angular->Get("max"), &O->Angular.Max);

			Core::Document* Diffusion = V->Get("diffusion");
			NMake::Unpack(Diffusion->Get("intensity"), &O->Diffusion.Intensity);
			NMake::Unpack(Diffusion->Get("accuracy"), &O->Diffusion.Accuracy);
			NMake::Unpack(Diffusion->Get("min"), &O->Diffusion.Min);
			NMake::Unpack(Diffusion->Get("max"), &O->Diffusion.Max);

			Core::Document* Noise = V->Get("noise");
			NMake::Unpack(Noise->Get("intensity"), &O->Noise.Intensity);
			NMake::Unpack(Noise->Get("accuracy"), &O->Noise.Accuracy);
			NMake::Unpack(Noise->Get("min"), &O->Noise.Min);
			NMake::Unpack(Noise->Get("max"), &O->Noise.Max);

			Core::Document* Position = V->Get("position");
			NMake::Unpack(Position->Get("intensity"), &O->Position.Intensity);
			NMake::Unpack(Position->Get("accuracy"), &O->Position.Accuracy);
			NMake::Unpack(Position->Get("min"), &O->Position.Min);
			NMake::Unpack(Position->Get("max"), &O->Position.Max);

			Core::Document* Rotation = V->Get("rotation");
			NMake::Unpack(Rotation->Get("intensity"), &O->Rotation.Intensity);
			NMake::Unpack(Rotation->Get("accuracy"), &O->Rotation.Accuracy);
			NMake::Unpack(Rotation->Get("min"), &O->Rotation.Min);
			NMake::Unpack(Rotation->Get("max"), &O->Rotation.Max);

			Core::Document* Scale = V->Get("scale");
			NMake::Unpack(Scale->Get("intensity"), &O->Scale.Intensity);
			NMake::Unpack(Scale->Get("accuracy"), &O->Scale.Accuracy);
			NMake::Unpack(Scale->Get("min"), &O->Scale.Min);
			NMake::Unpack(Scale->Get("max"), &O->Scale.Max);

			Core::Document* Velocity = V->Get("velocity");
			NMake::Unpack(Velocity->Get("intensity"), &O->Velocity.Intensity);
			NMake::Unpack(Velocity->Get("accuracy"), &O->Velocity.Accuracy);
			NMake::Unpack(Velocity->Get("min"), &O->Velocity.Min);
			NMake::Unpack(Velocity->Get("max"), &O->Velocity.Max);

			return true;
		}
		bool NMake::Unpack(Core::Document* V, Material* O, ContentManager* Content)
		{
			if (!V || !O || !Content)
				return false;

			std::string Path;
			if (NMake::Unpack(V->Get("diffuse-map"), &Path))
				O->SetDiffuseMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Get("normal-map"), &Path))
				O->SetNormalMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Get("metallic-map"), &Path))
				O->SetMetallicMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Get("roughness-map"), &Path))
				O->SetRoughnessMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Get("height-map"), &Path))
				O->SetHeightMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Get("occlusion-map"), &Path))
				O->SetOcclusionMap(Content->Load<Graphics::Texture2D>(Path));

			if (NMake::Unpack(V->Get("emission-map"), &Path))
				O->SetEmissionMap(Content->Load<Graphics::Texture2D>(Path));

			NMake::Unpack(V->Get("emission"), &O->Surface.Emission);
			NMake::Unpack(V->Get("metallic"), &O->Surface.Metallic);
			NMake::Unpack(V->Get("diffuse"), &O->Surface.Diffuse);
			NMake::Unpack(V->Get("scatter"), &O->Surface.Scatter);
			NMake::Unpack(V->Get("roughness"), &O->Surface.Roughness);
			NMake::Unpack(V->Get("occlusion"), &O->Surface.Occlusion);
			NMake::Unpack(V->Get("fresnel"), &O->Surface.Fresnel);
			NMake::Unpack(V->Get("refraction"), &O->Surface.Refraction);
			NMake::Unpack(V->Get("transparency"), &O->Surface.Transparency);
			NMake::Unpack(V->Get("environment"), &O->Surface.Environment);
			NMake::Unpack(V->Get("radius"), &O->Surface.Radius);
			NMake::Unpack(V->Get("height"), &O->Surface.Height);
			NMake::Unpack(V->Get("bias"), &O->Surface.Bias);
			NMake::Unpack(V->Get("name"), &O->Name);
			NMake::Unpack(V->Get("slot"), &O->Slot);

			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::SkinAnimatorKey* O)
		{
			if (!V)
				return false;

			NMake::Unpack(V->Get("pose"), &O->Pose);
			NMake::Unpack(V->Get("time"), &O->Time);

			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::SkinAnimatorClip* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("name"), &O->Name);
			NMake::Unpack(V->Get("duration"), &O->Duration);
			NMake::Unpack(V->Get("rate"), &O->Rate);

			std::vector<Core::Document*> Frames = V->FetchCollection("frames.frame", false);
			for (auto&& It : Frames)
			{
				O->Keys.emplace_back();
				NMake::Unpack(It, &O->Keys.back());
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::KeyAnimatorClip* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("name"), &O->Name);
			NMake::Unpack(V->Get("duration"), &O->Duration);
			NMake::Unpack(V->Get("rate"), &O->Rate);
			NMake::Unpack(V->Get("frames"), &O->Keys);
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::AnimatorKey* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("position"), &O->Position);
			NMake::Unpack(V->Get("rotation"), &O->Rotation);
			NMake::Unpack(V->Get("scale"), &O->Scale);
			NMake::Unpack(V->Get("time"), &O->Time);
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::Joint* O)
		{
			if (!V || !O)
				return false;

			NMake::Unpack(V->Get("index"), &O->Index);
			NMake::Unpack(V->Get("name"), &O->Name);
			NMake::Unpack(V->Get("transform"), &O->Transform);
			NMake::Unpack(V->Get("bind-shape"), &O->BindShape);

			std::vector<Core::Document*> Joints = V->FetchCollection("childs.joint", false);
			for (auto& It : Joints)
			{
				O->Childs.emplace_back();
				NMake::Unpack(It, &O->Childs.back());
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::ElementVertex* O)
		{
			if (!V || !O)
				return false;

			O->PositionX = (float)V->GetVar("[px]").GetNumber();
			O->PositionY = (float)V->GetVar("[py]").GetNumber();
			O->PositionZ = (float)V->GetVar("[pz]").GetNumber();
			O->VelocityX = (float)V->GetVar("[vx]").GetNumber();
			O->VelocityY = (float)V->GetVar("[vy]").GetNumber();
			O->VelocityZ = (float)V->GetVar("[vz]").GetNumber();
			O->ColorX = (float)V->GetVar("[cx]").GetNumber();
			O->ColorY = (float)V->GetVar("[cy]").GetNumber();
			O->ColorZ = (float)V->GetVar("[cz]").GetNumber();
			O->ColorW = (float)V->GetVar("[cw]").GetNumber();
			O->Angular = (float)V->GetVar("[a]").GetNumber();
			O->Scale = (float)V->GetVar("[s]").GetNumber();
			O->Rotation = (float)V->GetVar("[r]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::Vertex* O)
		{
			if (!V || !O)
				return false;

			O->PositionX = (float)V->GetVar("[px]").GetNumber();
			O->PositionY = (float)V->GetVar("[py]").GetNumber();
			O->PositionZ = (float)V->GetVar("[pz]").GetNumber();
			O->TexCoordX = (float)V->GetVar("[tx]").GetNumber();
			O->TexCoordY = (float)V->GetVar("[ty]").GetNumber();
			O->NormalX = (float)V->GetVar("[nx]").GetNumber();
			O->NormalY = (float)V->GetVar("[ny]").GetNumber();
			O->NormalZ = (float)V->GetVar("[nz]").GetNumber();
			O->TangentX = (float)V->GetVar("[tnx]").GetNumber();
			O->TangentY = (float)V->GetVar("[tny]").GetNumber();
			O->TangentZ = (float)V->GetVar("[tnz]").GetNumber();
			O->BitangentX = (float)V->GetVar("[btx]").GetNumber();
			O->BitangentY = (float)V->GetVar("[bty]").GetNumber();
			O->BitangentZ = (float)V->GetVar("[btz]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Compute::SkinVertex* O)
		{
			if (!V || !O)
				return false;

			O->PositionX = (float)V->GetVar("[px]").GetNumber();
			O->PositionY = (float)V->GetVar("[py]").GetNumber();
			O->PositionZ = (float)V->GetVar("[pz]").GetNumber();
			O->TexCoordX = (float)V->GetVar("[tx]").GetNumber();
			O->TexCoordY = (float)V->GetVar("[ty]").GetNumber();
			O->NormalX = (float)V->GetVar("[nx]").GetNumber();
			O->NormalY = (float)V->GetVar("[ny]").GetNumber();
			O->NormalZ = (float)V->GetVar("[nz]").GetNumber();
			O->TangentX = (float)V->GetVar("[tnx]").GetNumber();
			O->TangentY = (float)V->GetVar("[tny]").GetNumber();
			O->TangentZ = (float)V->GetVar("[tnz]").GetNumber();
			O->BitangentX = (float)V->GetVar("[btx]").GetNumber();
			O->BitangentY = (float)V->GetVar("[bty]").GetNumber();
			O->BitangentZ = (float)V->GetVar("[btz]").GetNumber();
			O->JointIndex0 = (float)V->GetVar("[ji0]").GetNumber();
			O->JointIndex1 = (float)V->GetVar("[ji1]").GetNumber();
			O->JointIndex2 = (float)V->GetVar("[ji2]").GetNumber();
			O->JointIndex3 = (float)V->GetVar("[ji3]").GetNumber();
			O->JointBias0 = (float)V->GetVar("[jb0]").GetNumber();
			O->JointBias1 = (float)V->GetVar("[jb1]").GetNumber();
			O->JointBias2 = (float)V->GetVar("[jb2]").GetNumber();
			O->JointBias3 = (float)V->GetVar("[jb3]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, Core::TickTimer* O)
		{
			if (!V || !O)
				return false;

			O->Delay = (float)V->GetVar("[delay]").GetNumber();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::string* O)
		{
			if (!V || !O)
				return false;

			*O = V->GetVar("[s]").GetBlob();
			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<bool>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("b-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				bool Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<int>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				int Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<unsigned int>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				unsigned int Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<float>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("n-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				float Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<double>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("n-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				double Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<int64_t>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				int64_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<long double>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("n-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				long double Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<uint64_t>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				uint64_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::Vector2>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("v2-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.X >> It.Y;

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::Vector3>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("v3-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.X >> It.Y >> It.Z;

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::Vector4>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("v4-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.X >> It.Y >> It.Z >> It.W;

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::Matrix4x4>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("m4x4-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<AnimatorState>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("as-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<SpawnerProperties>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("sp-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::SkinAnimatorClip>* O)
		{
			if (!V || !O)
				return false;

			std::vector<Core::Document*> Frames = V->FetchCollection("clips.clip", false);
			for (auto&& It : Frames)
			{
				O->push_back(Compute::SkinAnimatorClip());
				NMake::Unpack(It, &O->back());
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::KeyAnimatorClip>* O)
		{
			if (!V || !O)
				return false;

			std::vector<Core::Document*> Frames = V->FetchCollection("clips.clip", false);
			for (auto&& It : Frames)
			{
				O->push_back(Compute::KeyAnimatorClip());
				NMake::Unpack(It, &O->back());
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::AnimatorKey>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("ak-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::ElementVertex>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("ev-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::Joint>* O)
		{
			if (!V || !O)
				return false;

			O->reserve(V->Size());
			for (auto&& It : V->GetChilds())
			{
				O->push_back(Compute::Joint());
				NMake::Unpack(It, &O->back());
			}

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::Vertex>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("iv-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<Compute::SkinVertex>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("iv-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
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
		bool NMake::Unpack(Core::Document* V, std::vector<Core::TickTimer>* O)
		{
			if (!V || !O)
				return false;

            std::string Array(V->GetVar("tt-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			std::stringstream Stream(Array);
			O->resize((size_t)Size);

			for (auto& It : *O)
				Stream >> It.Delay;

			return true;
		}
		bool NMake::Unpack(Core::Document* V, std::vector<std::string>* O)
		{
			if (!V || !O)
				return false;

			Core::Document* Array = V->Get("s-array");
			if (!Array)
				return false;

			for (auto&& It : Array->GetChilds())
			{
				if (It->Key == "s" && It->Value.GetType() == Core::VarType::String)
					O->push_back(It->Value.GetBlob());
			}

			return true;
		}

		Event::Event(const std::string& NewName, SceneGraph* Target, const Core::VariantArgs& NewArgs) : TComponent(nullptr), TEntity(nullptr), TScene(Target), Id(NewName), Args(NewArgs)
		{
		}
		Event::Event(const std::string& NewName, Entity* Target, const Core::VariantArgs& NewArgs) : TComponent(nullptr), TEntity(Target), TScene(nullptr), Id(NewName), Args(NewArgs)
		{
		}
		Event::Event(const std::string& NewName, Component* Target, const Core::VariantArgs& NewArgs) : TComponent(Target), TEntity(nullptr), TScene(nullptr), Id(NewName), Args(NewArgs)
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

		Material::Material(SceneGraph* Src, const std::string& Alias) : DiffuseMap(nullptr), NormalMap(nullptr), MetallicMap(nullptr), RoughnessMap(nullptr), HeightMap(nullptr), OcclusionMap(nullptr), EmissionMap(nullptr), Scene(Src), Slot(0), Name(Alias)
		{
		}
		Material::~Material()
		{
			TH_RELEASE(DiffuseMap);
			TH_RELEASE(NormalMap);
			TH_RELEASE(MetallicMap);
			TH_RELEASE(RoughnessMap);
			TH_RELEASE(HeightMap);
			TH_RELEASE(OcclusionMap);
			TH_RELEASE(EmissionMap);
		}
		void Material::SetDiffuseMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(DiffuseMap);
			DiffuseMap = New;
		}
		Graphics::Texture2D* Material::GetDiffuseMap() const
		{
			return DiffuseMap;
		}
		void Material::SetNormalMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(NormalMap);
			NormalMap = New;
		}
		Graphics::Texture2D* Material::GetNormalMap() const
		{
			return NormalMap;
		}
		void Material::SetMetallicMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(MetallicMap);
			MetallicMap = New;
		}
		Graphics::Texture2D* Material::GetMetallicMap() const
		{
			return MetallicMap;
		}
		void Material::SetRoughnessMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(RoughnessMap);
			RoughnessMap = New;
		}
		Graphics::Texture2D* Material::GetRoughnessMap() const
		{
			return RoughnessMap;
		}
		void Material::SetHeightMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(HeightMap);
			HeightMap = New;
		}
		Graphics::Texture2D* Material::GetHeightMap() const
		{
			return HeightMap;
		}
		void Material::SetOcclusionMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(OcclusionMap);
			OcclusionMap = New;
		}
		Graphics::Texture2D* Material::GetOcclusionMap() const
		{
			return OcclusionMap;
		}
		void Material::SetEmissionMap(Graphics::Texture2D* New)
		{
			TH_RELEASE(EmissionMap);
			EmissionMap = New;
		}
		Graphics::Texture2D* Material::GetEmissionMap() const
		{
			return EmissionMap;
		}
		SceneGraph* Material::GetScene() const
		{
			return Scene;
		}
		uint64_t Material::GetSlot() const
		{
			return Slot;
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
		void* Processor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
		{
			return nullptr;
		}
		void* Processor::Deserialize(Core::Stream* Stream, uint64_t Length, uint64_t Offset, const Core::VariantArgs& Args)
		{
			return nullptr;
		}
		bool Processor::Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args)
		{
			return false;
		}
		ContentManager* Processor::GetContent()
		{
			return Content;
		}

		Component::Component(Entity* Reference) : Active(true), Parent(Reference)
		{
		}
		Component::~Component()
		{
		}
		void Component::Deserialize(ContentManager* Content, Core::Document* Node)
		{
		}
		void Component::Serialize(ContentManager* Content, Core::Document* Node)
		{
		}
		void Component::Awake(Component* New)
		{
		}
		void Component::Asleep()
		{
		}
		void Component::Synchronize(Core::Timer* Time)
		{
		}
		void Component::Update(Core::Timer* Time)
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

			return Compute::Common::IsCubeInFrustum((World ? *World : Parent->Transform->GetWorld()) * View.ViewProjection, 1.65f) < 0.0f;
		}
		bool Cullable::IsNear(const Viewer& View)
		{
			return Parent->Transform->Position.Distance(View.WorldPosition) <= View.FarPlane + Parent->Transform->Scale.Length();
		}

		Drawable::Drawable(Entity* Ref, uint64_t Hash, bool vComplex) : Cullable(Ref), Category(GeoCategory::Opaque), Source(Hash), Complex(vComplex), Static(true)
		{
			if (!Complex)
				Materials[nullptr] = nullptr;
		}
		Drawable::~Drawable()
		{
			Detach();
		}
		void Drawable::Message(Event* Value)
		{
			if (!Value || !Value->Is("materialchange"))
				return;

			Material* Ptr = (Material*)Value->Args["material-id"].GetPointer();
			for (auto&& Surface : Materials)
			{
				if (Surface.second == Ptr)
					Surface.second = nullptr;
			}
		}
		void Drawable::ClearCull()
		{
			Query.Clear();
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
		bool Drawable::SetTransparency(bool Enabled)
		{
			if (!Parent || !Parent->GetScene())
			{
				if (Enabled)
					Category = GeoCategory::Transparent;
				else
					Category = GeoCategory::Opaque;

				return false;
			}

			Detach();
			if (Enabled)
				Category = GeoCategory::Transparent;
			else
				Category = GeoCategory::Opaque;

			Parent->GetScene()->AddDrawable(this, Category);
			return true;
		}
		bool Drawable::SetMaterial(void* Instance, Material* Value)
		{
			if (!Complex)
			{
				Materials[nullptr] = Value;
				return (!Instance);
			}

			auto It = Materials.find(Instance);
			if (It == Materials.end())
				Materials[Instance] = Value;
			else
				It->second = Value;
	
			return true;
		}
		bool Drawable::HasTransparency()
		{
			return Category == GeoCategory::Transparent;
		}
		int64_t Drawable::GetSlot(void* Surface)
		{
			Material* Base = GetMaterial(Surface);
			if (Base != nullptr)
				return (int64_t)Base->GetSlot();

			return -1;
		}
		int64_t Drawable::GetSlot()
		{
			Material* Base = GetMaterial();
			if (Base != nullptr)
				return (int64_t)Base->GetSlot();

			return -1;
		}
		Material* Drawable::GetMaterial(void* Instance)
		{
			if (!Complex)
				return nullptr;

			auto It = Materials.find(Instance);
			if (It == Materials.end())
				return nullptr;

			return It->second;
		}
		Material* Drawable::GetMaterial()
		{
			if (Complex || Materials.empty())
				return nullptr;

			return Materials.begin()->second;
		}
		const std::unordered_map<void*, Material*>& Drawable::GetMaterials()
		{
			return Materials;
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
				TH_RELEASE(Component.second);
			}

			TH_RELEASE(Transform);
		}
		void Entity::RemoveComponent(uint64_t fId)
		{
			std::unordered_map<uint64_t, Component*>::iterator It = Components.find(fId);
			if (It == Components.end())
				return;

			It->second->SetActive(false);
			if (Scene != nullptr)
			{
				Scene->Lock();
				if (Scene->Camera == It->second)
					Scene->Camera = nullptr;
			}

			TH_RELEASE(It->second);
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
		Component* Entity::GetComponent(uint64_t fId)
		{
			std::unordered_map<uint64_t, Component*>::iterator It = Components.find(fId);
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
		void Renderer::Deserialize(ContentManager* Content, Core::Document* Node)
		{
		}
		void Renderer::Serialize(ContentManager* Content, Core::Document* Node)
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
		void Renderer::Render(Core::Timer* TimeStep, RenderState State, RenderOpt Options)
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
			{
				Safe.unlock();
				return false;
			}

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
			for (auto It = Cache.begin(); It != Cache.end(); ++It)
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
			TH_RELEASE(Sphere[(size_t)BufferType::Index]);
			TH_RELEASE(Sphere[(size_t)BufferType::Vertex]);
			TH_RELEASE(Cube[(size_t)BufferType::Index]);
			TH_RELEASE(Cube[(size_t)BufferType::Vertex]);
			TH_RELEASE(Box[(size_t)BufferType::Index]);
			TH_RELEASE(Box[(size_t)BufferType::Vertex]);
			TH_RELEASE(SkinBox[(size_t)BufferType::Index]);
			TH_RELEASE(SkinBox[(size_t)BufferType::Vertex]);
			TH_RELEASE(Quad);
			ClearCache();
		}
		bool PrimitiveCache::Compile(Graphics::ElementBuffer** Results, const std::string& Name, size_t ElementSize, size_t ElementsCount)
		{
			if (!Results || Get(Results, Name))
				return false;

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
			F.ElementWidth = ElementSize;
			F.ElementCount = ElementsCount;

			Graphics::ElementBuffer* VertexBuffer = Device->CreateElementBuffer(F);
			if (!VertexBuffer)
				return false;

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Index_Buffer;
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
			Result.Buffers[(size_t)BufferType::Index] = Results[(size_t)BufferType::Index] = IndexBuffer;
			Result.Buffers[(size_t)BufferType::Vertex] = Results[(size_t)BufferType::Vertex] = VertexBuffer;
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

				Results[(size_t)BufferType::Index] = It->second.Buffers[(size_t)BufferType::Index];
				Results[(size_t)BufferType::Vertex] = It->second.Buffers[(size_t)BufferType::Vertex];
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
			{
				Safe.unlock();
				return false;
			}

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

			Compute::ShapeVertex Elements[6];
			Elements[0] = { -1.0f, -1.0f, 0, -1, 0 };
			Elements[1] = { -1.0f, 1.0f, 0, -1, -1 };
			Elements[2] = { 1.0f, 1.0f, 0, 0, -1 };
			Elements[3] = { -1.0f, -1.0f, 0, -1, 0 };
			Elements[4] = { 1.0f, 1.0f, 0, 0, -1 };
			Elements[5] = { 1.0f, -1.0f, 0, 0, 0 };

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Invalid;
			F.Usage = Graphics::ResourceUsage::Default;
			F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
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
			if (Sphere[(size_t)Type] != nullptr)
				return Sphere[(size_t)Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType::Index)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				Sphere[(size_t)BufferType::Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Sphere[(size_t)BufferType::Index];
			}
			else if (Type == BufferType::Vertex)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::ShapeVertex);
				F.Elements = &Elements[0];

				Safe.lock();
				Sphere[(size_t)BufferType::Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Sphere[(size_t)BufferType::Vertex];
			}

			return nullptr;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetCube(BufferType Type)
		{
			if (Cube[(size_t)Type] != nullptr)
				return Cube[(size_t)Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType::Index)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				Cube[(size_t)BufferType::Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Cube[(size_t)BufferType::Index];
			}
			else if (Type == BufferType::Vertex)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::ShapeVertex);
				F.Elements = &Elements[0];

				Safe.lock();
				Cube[(size_t)BufferType::Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Cube[(size_t)BufferType::Vertex];
			}

			return nullptr;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetBox(BufferType Type)
		{
			if (Box[(size_t)Type] != nullptr)
				return Box[(size_t)Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType::Index)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				Box[(size_t)BufferType::Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Box[(size_t)BufferType::Index];
			}
			else if (Type == BufferType::Vertex)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::Vertex);
				F.Elements = &Elements[0];

				Safe.lock();
				Box[(size_t)BufferType::Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return Box[(size_t)BufferType::Vertex];
			}

			return nullptr;
		}
		Graphics::ElementBuffer* PrimitiveCache::GetSkinBox(BufferType Type)
		{
			if (SkinBox[(size_t)Type] != nullptr)
				return SkinBox[(size_t)Type];

			if (!Device)
				return nullptr;

			if (Type == BufferType::Index)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Index_Buffer;
				F.ElementCount = (unsigned int)Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = &Indices[0];

				Safe.lock();
				SkinBox[(size_t)BufferType::Index] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return SkinBox[(size_t)BufferType::Index];
			}
			else if (Type == BufferType::Vertex)
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
				F.AccessFlags = Graphics::CPUAccess::Invalid;
				F.Usage = Graphics::ResourceUsage::Default;
				F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)Elements.size();
				F.ElementWidth = sizeof(Compute::SkinVertex);
				F.Elements = &Elements[0];

				Safe.lock();
				SkinBox[(size_t)BufferType::Vertex] = Device->CreateElementBuffer(F);
				Safe.unlock();

				return SkinBox[(size_t)BufferType::Vertex];
			}

			return nullptr;
		}
		void PrimitiveCache::GetSphereBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[(size_t)BufferType::Index] = GetSphere(BufferType::Index);
				Result[(size_t)BufferType::Vertex] = GetSphere(BufferType::Vertex);
			}
		}
		void PrimitiveCache::GetCubeBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[(size_t)BufferType::Index] = GetCube(BufferType::Index);
				Result[(size_t)BufferType::Vertex] = GetCube(BufferType::Vertex);
			}
		}
		void PrimitiveCache::GetBoxBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[(size_t)BufferType::Index] = GetBox(BufferType::Index);
				Result[(size_t)BufferType::Vertex] = GetBox(BufferType::Vertex);
			}
		}
		void PrimitiveCache::GetSkinBoxBuffers(Graphics::ElementBuffer** Result)
		{
			if (Result != nullptr)
			{
				Result[(size_t)BufferType::Index] = GetSkinBox(BufferType::Index);
				Result[(size_t)BufferType::Vertex] = GetSkinBox(BufferType::Vertex);
			}
		}
		void PrimitiveCache::ClearCache()
		{
			Safe.lock();
			for (auto It = Cache.begin(); It != Cache.end(); ++It)
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

		RenderSystem::RenderSystem(Graphics::GraphicsDevice* Ref) : DepthStencil(nullptr), Blend(nullptr), Sampler(nullptr), Target(nullptr), Device(Ref), BaseMaterial(nullptr), Scene(nullptr), OcclusionCulling(false), FrustumCulling(true)
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

			Graphics::MultiRenderTarget2D* MRT = Scene->GetMRT(TargetType::Main);
			float Aspect = (float)MRT->GetWidth() / MRT->GetHeight();

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
		void RenderSystem::Remount(Renderer* fTarget)
		{
			if (!fTarget)
				return;

			fTarget->Deactivate();
			fTarget->SetRenderer(this);
			fTarget->Activate();
			fTarget->ResizeBuffers();
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
		void RenderSystem::Synchronize(Core::Timer* Time, const Viewer& View)
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
		void RenderSystem::CullGeometry(Core::Timer* Time, const Viewer& View)
		{
			if (!OcclusionCulling || !Target)
				return;

			double ElapsedTime = Time->GetElapsedTime();
			if (Sorting.TickEvent(ElapsedTime))
			{
				for (auto& Base : Cull)
					Scene->SortFrontToBack(Scene->GetOpaque(Base));
			}

			if (!Occlusion.TickEvent(ElapsedTime))
				return;

			Device->SetSamplerState(Sampler, 0, TH_PS);
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
			for (auto It = Renderers.begin(); It != Renderers.end(); ++It)
			{
				if (*It && (*It)->GetId() == Id)
				{
					(*It)->Deactivate();
					TH_RELEASE(*It);
					Renderers.erase(It);
					break;
				}
			}
		}
		void RenderSystem::RestoreOutput()
		{
			if (Scene != nullptr)
				Scene->SetMRT(TargetType::Main, false);
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
		void RenderSystem::ClearMaterials()
		{
			BaseMaterial = nullptr;
		}
		bool RenderSystem::PushGeometryBuffer(Material* Next)
		{
			bool R = (Next != nullptr);
			if (!R || Next == BaseMaterial)
				return R;

			BaseMaterial = Next;
			Device->SetTexture2D(Next->DiffuseMap, 1, TH_PS);
			Device->SetTexture2D(Next->NormalMap, 2, TH_PS);
			Device->SetTexture2D(Next->MetallicMap, 3, TH_PS);
			Device->SetTexture2D(Next->RoughnessMap, 4, TH_PS);
			Device->SetTexture2D(Next->HeightMap, 5, TH_PS);
			Device->SetTexture2D(Next->OcclusionMap, 6, TH_PS);
			Device->SetTexture2D(Next->EmissionMap, 7, TH_PS);
			Device->Render.Diffuse = (float)(Next->DiffuseMap != nullptr);
			Device->Render.Normal = (float)(Next->NormalMap != nullptr);
			Device->Render.Height = (float)(Next->HeightMap != nullptr);
			Device->Render.Mid = (float)Next->Slot;

			return true;
		}
		bool RenderSystem::PushVoxelsBuffer(Material* Next)
		{
			bool R = (Next != nullptr);
			if (!R || Next == BaseMaterial)
				return R;

			BaseMaterial = Next;
			Device->SetTexture2D(Next->DiffuseMap, 4, TH_PS);
			Device->SetTexture2D(Next->NormalMap, 5, TH_PS);
			Device->SetTexture2D(Next->MetallicMap, 6, TH_PS);
			Device->SetTexture2D(Next->RoughnessMap, 7, TH_PS);
			Device->SetTexture2D(Next->OcclusionMap, 8, TH_PS);
			Device->SetTexture2D(Next->EmissionMap, 9, TH_PS);
			Device->Render.Diffuse = (float)(Next->DiffuseMap != nullptr);
			Device->Render.Normal = (float)(Next->NormalMap != nullptr);
			Device->Render.Mid = (float)Next->Slot;

			return true;
		}
		bool RenderSystem::PushDepthLinearBuffer(Material* Next)
		{
			bool R = (Next != nullptr);
			if (!R || Next == BaseMaterial)
				return R;

			BaseMaterial = Next;
			Device->SetTexture2D(Next->DiffuseMap, 1, TH_PS);
			Device->Render.Diffuse = (float)(Next->DiffuseMap != nullptr);
			Device->Render.Mid = (float)Next->Slot;

			return true;
		}
		bool RenderSystem::PushDepthCubicBuffer(Material* Next)
		{
			return PushDepthLinearBuffer(Next);
		}
		bool RenderSystem::PassCullable(Cullable* Base, CullResult Mode, float* Result)
		{
			if (Mode == CullResult::Last)
				return Base->Visibility;

			float D = Base->Cull(Scene->View);
			if (Mode == CullResult::Cache)
				Base->Visibility = D;

			if (Result != nullptr)
				*Result = D;

			return D > 0.0f;
		}
		bool RenderSystem::PassDrawable(Drawable* Base, CullResult Mode, float* Result)
		{
			if (Mode == CullResult::Last)
			{
				if (OcclusionCulling)
				{
					int R = Base->Query.Fetch(this);
					if (R != -1)
						return R > 0;
				}

				if (!Base->Query.GetPassed())
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
		Graphics::Shader* RenderSystem::CompileShader(Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			if (Desc.Filename.empty())
			{
				TH_ERROR("shader must have a name");
				return nullptr;
			}

			ShaderCache* Cache = (Scene ? Scene->GetShaders() : nullptr);
			if (Cache != nullptr)
				return Cache->Compile(Desc.Filename, Desc, BufferSize);

			Graphics::Shader* Shader = Device->CreateShader(Desc);
			if (BufferSize > 0)
				Device->UpdateBufferSize(Shader, BufferSize);

			return Shader;
		}
		Graphics::Shader* RenderSystem::CompileShader(const std::string& SectionName, size_t BufferSize)
		{
			Graphics::Shader::Desc I = Graphics::Shader::Desc();
			if (!Device->GetSection(SectionName, &I))
				return nullptr;

			return CompileShader(I, BufferSize);
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
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
			F.ElementWidth = ElementSize;
			F.ElementCount = ElementsCount;

			Graphics::ElementBuffer* VertexBuffer = Device->CreateElementBuffer(F);
			if (!VertexBuffer)
				return false;

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Index_Buffer;
			F.ElementWidth = sizeof(int);
			F.ElementCount = ElementsCount * 3;

			Graphics::ElementBuffer* IndexBuffer = Device->CreateElementBuffer(F);
			if (!IndexBuffer)
			{
				TH_RELEASE(VertexBuffer);
				return false;
			}

			Result[(size_t)BufferType::Index] = IndexBuffer;
			Result[(size_t)BufferType::Vertex] = VertexBuffer;

			return true;
		}
		Renderer* RenderSystem::AddRenderer(Renderer* In)
		{
			if (!In)
				return nullptr;

			for (auto It = Renderers.begin(); It != Renderers.end(); ++It)
			{
				if (*It && (*It)->GetId() == In->GetId())
				{
					if (*It == In)
						return In;

					(*It)->Deactivate();
					TH_RELEASE(*It);
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
		Core::Pool<Component*>* RenderSystem::GetSceneComponents(uint64_t Section)
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
		void GeometryDraw::CullGeometry(const Viewer& View, Core::Pool<Drawable*>* Geometry)
		{
		}
		void GeometryDraw::CullGeometry(const Viewer& View)
		{
			Core::Pool<Drawable*>* Opaque = GetOpaque();
			if (Opaque != nullptr && Opaque->Size() > 0)
				CullGeometry(View, Opaque);
		}
		void GeometryDraw::Render(Core::Timer* TimeStep, RenderState State, RenderOpt Options)
		{
			if (State == RenderState::Geometry_Result)
			{
				Core::Pool<Drawable*>* Geometry;
				if ((size_t)Options & (size_t)RenderOpt::Transparent)
					Geometry = GetTransparent();
				else
					Geometry = GetOpaque();

				if (Geometry != nullptr && Geometry->Size() > 0)
				{
					System->ClearMaterials();
					RenderGeometryResult(TimeStep, Geometry, Options);
				}
			}
			else if (State == RenderState::Geometry_Voxels)
			{
				if ((size_t)Options & (size_t)RenderOpt::Transparent)
					return;

				Core::Pool<Drawable*>* Geometry = GetOpaque();
				if (Geometry != nullptr && Geometry->Size() > 0)
				{
					System->ClearMaterials();
					RenderGeometryVoxels(TimeStep, Geometry, Options);
				}
			}
			else if (State == RenderState::Depth_Linear)
			{
				if (!((size_t)Options & (size_t)RenderOpt::Inner))
					return;

				System->ClearMaterials();

				Core::Pool<Drawable*>* Opaque = GetOpaque();
				if (Opaque != nullptr && Opaque->Size() > 0)
					RenderDepthLinear(TimeStep, Opaque);

				Core::Pool<Drawable*>* Transparent = GetTransparent();
				if (Transparent != nullptr && Transparent->Size() > 0)
					RenderDepthLinear(TimeStep, Transparent);
			}
			else if (State == RenderState::Depth_Cubic)
			{
				Viewer& View = System->GetScene()->View;
				if (!((size_t)Options & (size_t)RenderOpt::Inner))
					return;

				System->ClearMaterials();

				Core::Pool<Drawable*>* Opaque = GetOpaque();
				if (Opaque != nullptr && Opaque->Size() > 0)
					RenderDepthCubic(TimeStep, Opaque, View.CubicViewProjection);

				Core::Pool<Drawable*>* Transparent = GetTransparent();
				if (Transparent != nullptr && Transparent->Size() > 0)
					RenderDepthCubic(TimeStep, Transparent, View.CubicViewProjection);
			}
		}
		Core::Pool<Drawable*>* GeometryDraw::GetOpaque()
		{
			if (!System || !System->GetScene())
				return nullptr;

			return System->GetScene()->GetOpaque(Source);
		}
		Core::Pool<Drawable*>* GeometryDraw::GetTransparent()
		{
			if (!System || !System->GetScene())
				return nullptr;

			return System->GetScene()->GetTransparent(Source);
		}

		EffectDraw::EffectDraw(RenderSystem* Lab) : Renderer(Lab), Output(nullptr), Swap(nullptr), MaxSlot(0)
		{
			auto* Device = Lab->GetDevice();
			DepthStencil = Device->GetDepthStencilState("none");
			Rasterizer = Device->GetRasterizerState("cull-back");
			Blend = Device->GetBlendState("overwrite-opaque");
			Sampler = Device->GetSamplerState("trilinear-x16");
			Layout = Device->GetInputLayout("shape-vertex");
		}
		EffectDraw::~EffectDraw()
		{
			for (auto It = Effects.begin(); It != Effects.end(); ++It)
				System->FreeShader(It->first, It->second);
		}
		void EffectDraw::ResizeBuffers()
		{
			Output = nullptr;
			ResizeEffect();
		}
		void EffectDraw::ResizeEffect()
		{
		}
		void EffectDraw::RenderOutput(Graphics::RenderTarget2D* Resource)
		{
			if (Resource != nullptr)
			{
				Output = Resource;
				Swap = Resource;
			}
			else
				Output = System->GetRT(TargetType::Main);

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTarget(Output, 0, 0, 0, 0);
		}
		void EffectDraw::RenderTexture(uint32_t Slot6, Graphics::Texture2D* Resource)
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture2D(Resource, 6 + Slot6, TH_PS);

			if (Resource != nullptr)
				MaxSlot = std::max(MaxSlot, 6 + Slot6);
		}
		void EffectDraw::RenderTexture(uint32_t Slot6, Graphics::Texture3D* Resource)
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture3D(Resource, 6 + Slot6, TH_PS);

			if (Resource != nullptr)
				MaxSlot = std::max(MaxSlot, 6 + Slot6);
		}
		void EffectDraw::RenderTexture(uint32_t Slot6, Graphics::TextureCube* Resource)
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTextureCube(Resource, 6 + Slot6, TH_PS);

			if (Resource != nullptr)
				MaxSlot = std::max(MaxSlot, 6 + Slot6);
		}
		void EffectDraw::RenderMerge(Graphics::Shader* Effect, void* Buffer, size_t Count)
		{
			if (!Count)
				return;

			if (!Effect)
				Effect = Effects.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();

			if (Swap != nullptr && Output != Swap)
				Device->SetTexture2D(Swap->GetTarget(0), 5, TH_PS);
			else if (Merger != nullptr)
				Device->SetTexture2D(*Merger, 5, TH_PS);
				
			Device->SetShader(Effect, TH_VS | TH_PS);
			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, TH_VS | TH_PS);
			}

			for (size_t i = 0; i < Count; i++)
			{
				Device->Draw(6, 0);
				if (!Swap)
					Device->CopyTexture2D(Output, 0, Merger);
			}

			if (Swap == Output)
				RenderOutput();
		}
		void EffectDraw::RenderResult(Graphics::Shader* Effect, void* Buffer)
		{
			if (!Effect)
				Effect = Effects.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();

			if (Swap != nullptr && Output != Swap)
				Device->SetTexture2D(Swap->GetTarget(0), 5, TH_PS);
			else if (Merger != nullptr)
				Device->SetTexture2D(*Merger, 5, TH_PS);

			Device->SetShader(Effect, TH_VS | TH_PS);
			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, TH_VS | TH_PS);
			}

			Device->Draw(6, 0);
			Output = System->GetRT(TargetType::Main);
		}
		void EffectDraw::RenderEffect(Core::Timer* Time)
		{
		}
		void EffectDraw::Render(Core::Timer* Time, RenderState State, RenderOpt Options)
		{
			if (State != RenderState::Geometry_Result || (size_t)Options & (size_t)RenderOpt::Inner)
				return;

			MaxSlot = 5;
			if (Effects.empty())
				return;

			Swap = nullptr;
			if (!Output)
				Output = System->GetRT(TargetType::Main);

			Graphics::MultiRenderTarget2D* Input = System->GetMRT(TargetType::Main);
			PrimitiveCache* Cache = System->GetPrimitives();
			if (!Input || !Cache)
				return;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetSamplerState(Sampler, 0, TH_PS);
			Device->SetDepthStencilState(DepthStencil);
			Device->SetBlendState(Blend);
			Device->SetRasterizerState(Rasterizer);
			Device->SetInputLayout(Layout);
			Device->SetTarget(Output, 0, 0, 0, 0);
			Device->SetTexture2D(Input->GetTarget(0), 1, TH_PS);
			Device->SetTexture2D(Input->GetTarget(1), 2, TH_PS);
			Device->SetTexture2D(Input->GetTarget(2), 3, TH_PS);
			Device->SetTexture2D(Input->GetTarget(3), 4, TH_PS);
			Device->SetVertexBuffer(Cache->GetQuad(), 0);

			RenderEffect(Time);

			Device->FlushTexture2D(1, MaxSlot, TH_PS);
			Device->CopyTarget(Output, 0, Input, 0);
			System->RestoreOutput();
		}
		Graphics::Shader* EffectDraw::GetEffect(const std::string& Name)
		{
			auto It = Effects.find(Name);
			if (It != Effects.end())
				return It->second;

			return nullptr;
		}
		Graphics::Shader* EffectDraw::CompileEffect(Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			if (Desc.Filename.empty())
			{
				TH_ERROR("cannot compile unnamed shader source");
				return nullptr;
			}

			Graphics::Shader* Shader = System->CompileShader(Desc, BufferSize);
			if (!Shader)
				return nullptr;

			auto It = Effects.find(Desc.Filename);
			if (It != Effects.end())
			{
				TH_RELEASE(It->second);
				It->second = Shader;
			}
			else
				Effects[Desc.Filename] = Shader;

			return Shader;
		}
		Graphics::Shader* EffectDraw::CompileEffect(const std::string& SectionName, size_t BufferSize)
		{
			Graphics::Shader::Desc I = Graphics::Shader::Desc();
			if (!System->GetDevice()->GetSection(SectionName, &I))
				return nullptr;

			return CompileEffect(I, BufferSize);
		}
		unsigned int EffectDraw::GetMipLevels()
		{
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Main);
			if (!RT)
				return 0;

			return System->GetDevice()->GetMipLevel(RT->GetWidth(), RT->GetHeight());
		}
		unsigned int EffectDraw::GetWidth()
		{
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Main);
			if (!RT)
				return 0;

			return RT->GetWidth();
		}
		unsigned int EffectDraw::GetHeight()
		{
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Main);
			if (!RT)
				return 0;

			return RT->GetHeight();
		}

		SceneGraph::Desc SceneGraph::Desc::Get(Application* Base)
		{
			SceneGraph::Desc I;
			if (Base != nullptr)
			{
				I.Shaders = Base->Cache.Shaders;
				I.Primitives = Base->Cache.Primitives;
				I.Device = Base->Renderer;
				I.Manager = Base->VM;
			}
			
			return I;
		}

		SceneGraph::SceneGraph(const Desc& I) : Simulator(nullptr), Camera(nullptr), Conf(I), Surfaces(16), Invoked(false), Active(true)
		{
			Sync.Count = 0; Sync.Locked = false;
			for (size_t i = 0; i < (size_t)ThreadId::Count; i++)
				Sync.Threads[i].State = 0;

			for (size_t i = 0; i < (size_t)TargetType::Count * 2; i++)
			{
				Display.MRT[i] = nullptr;
				Display.RT[i] = nullptr;
			}

			Display.VoxelBuffers[(size_t)VoxelType::Diffuse] = nullptr;
			Display.VoxelBuffers[(size_t)VoxelType::Normal] = nullptr;
			Display.VoxelBuffers[(size_t)VoxelType::Surface] = nullptr;
			Display.MaterialBuffer = nullptr;
			Display.Merger = nullptr;
			Display.DepthStencil = nullptr;
			Display.Rasterizer = nullptr;
			Display.Blend = nullptr;
			Display.Sampler = nullptr;
			Display.Layout = nullptr;
			Display.VoxelSize = 0;
			Configure(I);

			Simulator = new Compute::Simulator(I.Simulator);
			ExpandMaterials();
		}
		SceneGraph::~SceneGraph()
		{
			Dispatch();
			Lock();

			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
				TH_RELEASE(*It);
			
			for (auto It = Materials.Begin(); It != Materials.End(); ++It)
				TH_RELEASE(*It);

			TH_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Diffuse]);
			TH_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Normal]);
			TH_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Surface]);
			TH_RELEASE(Display.Merger);

			for (size_t i = 0; i < (size_t)TargetType::Count; i++)
			{
				TH_RELEASE(Display.MRT[i]);
				TH_RELEASE(Display.RT[i]);
			}

			TH_RELEASE(Display.MaterialBuffer);
			TH_RELEASE(Simulator);

			Core::Schedule::Get()->ClearListener("scene-event", Listener);
			Unlock();
		}
		void SceneGraph::Configure(const Desc& NewConf)
		{
			if (!Conf.Device)
				return;

			Display.DepthStencil = Conf.Device->GetDepthStencilState("none");
			Display.Rasterizer = Conf.Device->GetRasterizerState("cull-back");
			Display.Blend = Conf.Device->GetBlendState("overwrite");
			Display.Sampler = Conf.Device->GetSamplerState("trilinear-x16");
			Display.Layout = Conf.Device->GetInputLayout("shape-vertex");

			Lock();
			Conf = NewConf;
			Materials.Reserve(Conf.MaterialCount);
			Entities.Reserve(Conf.EntityCount);
			Pending.Reserve(Conf.ComponentCount);

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

			Core::Pool<Component*>* Cameras = GetComponents<Components::Camera>();
			if (Cameras != nullptr)
			{
				for (auto It = Cameras->Begin(); It != Cameras->End(); ++It)
				{
					Components::Camera* Base = (*It)->As<Components::Camera>();
					Base->GetRenderer()->Remount();
				}
			}

			auto* Queue = Core::Schedule::Get();
			Queue->ClearListener("scene-event", Listener);
			Listener = Queue->SetListener("scene-event", [this](Core::VariantArgs& Args)
			{
				Event* Message = (Event*)Args["event"].GetPointer();
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

			PrimitiveCache* Cache = View.Renderer->GetPrimitives();
			if (!Cache)
				return;

			Conf.Device->SetTarget();
			Conf.Device->Render.TexCoord = 1.0f;
			Conf.Device->Render.WorldViewProj.Identify();
			Conf.Device->SetSamplerState(Display.Sampler, 0, TH_PS);
			Conf.Device->SetDepthStencilState(Display.DepthStencil);
			Conf.Device->SetBlendState(Display.Blend);
			Conf.Device->SetRasterizerState(Display.Rasterizer);
			Conf.Device->SetInputLayout(Display.Layout);
			Conf.Device->SetShader(Conf.Device->GetBasicEffect(), TH_VS | TH_PS);
			Conf.Device->SetVertexBuffer(Cache->GetQuad(), 0);
			Conf.Device->SetTexture2D(Display.MRT[(size_t)TargetType::Main]->GetTarget(0), 1, TH_PS);
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType::Render);
			Conf.Device->Draw(6, 0);
			Conf.Device->SetTexture2D(nullptr, 1, TH_PS);
		}
		void SceneGraph::Render(Core::Timer* Time)
		{
			BeginThread(ThreadId::Render);
			if (Camera != nullptr)
			{
				RestoreViewBuffer(nullptr);
				FillMaterialBuffers();

				SetMRT(TargetType::Main, true);
				Render(Time, RenderState::Geometry_Result, RenderOpt::None);
				View.Renderer->CullGeometry(Time, View);
			}
			EndThread(ThreadId::Render);
		}
		void SceneGraph::Render(Core::Timer* Time, RenderState Stage, RenderOpt Options)
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
		void SceneGraph::Simulation(Core::Timer* Time)
		{
			if (!Active)
				return;

			BeginThread(ThreadId::Simulation);
			Simulator->Simulate((float)Time->GetTimeStep());
			EndThread(ThreadId::Simulation);
		}
		void SceneGraph::Synchronize(Core::Timer* Time)
		{
			BeginThread(ThreadId::Synchronize);
			for (auto It = Pending.Begin(); It != Pending.End(); ++It)
				(*It)->Synchronize(Time);

			uint64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
			{
				Entity* Base = *It;
				Base->Transform->Synchronize();
				Base->Id = Index; Index++;
			}
			EndThread(ThreadId::Synchronize);
		}
		void SceneGraph::Update(Core::Timer* Time)
		{
			BeginThread(ThreadId::Update);
			if (Active)
			{
				for (auto It = Pending.Begin(); It != Pending.End(); ++It)
					(*It)->Update(Time);
			}

			Compute::Vector3& Far = View.WorldPosition;
			if (Camera != nullptr)
				Far = Camera->Parent->Transform->Position;

			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
				(*It)->Distance = (*It)->Transform->Position.Distance(Far);

			DispatchLastEvent();
			EndThread(ThreadId::Update);
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
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
				RegisterEntity(*It);

			GetCamera();
			Unlock();
		}
		void SceneGraph::Reindex()
		{
			Lock();
			int64_t Index = 0;
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
			{
				(*It)->Id = Index;
				Index++;
			}

			Index = 0;
			for (auto It = Materials.Begin(); It != Materials.End(); ++It)
			{
				(*It)->Slot = Index;
				Index++;
			}
			Unlock();
		}
		void SceneGraph::SortBackToFront(Core::Pool<Drawable*>* Array)
		{
			if (!Array)
				return;

			std::sort(Array->Begin(), Array->End(), [](Component* A, Component* B)
			{
				return A->Parent->Distance > B->Parent->Distance;
			});
		}
		void SceneGraph::SortFrontToBack(Core::Pool<Drawable*>* Array)
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
		void SceneGraph::RemoveMaterial(Material* Value)
		{
			if (!Value)
				return;

			Core::VariantArgs Args;
			Args["material-id"] = Core::Var::Pointer(Value);

			if (!DispatchEvent("materialchange", Args))
				return;

			Lock();
			for (auto It = Materials.Begin(); It != Materials.End(); ++It)
			{
				if (*It == Value)
				{
					TH_RELEASE(Value);
					Materials.RemoveAt(It);
					break;
				}
			}
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
			if (!Instance->Transform->GetChildCount())
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

			Conf.Device->View.InvViewProj = View.InvViewProjection;
			Conf.Device->View.ViewProj = View.ViewProjection;
			Conf.Device->View.Proj = View.Projection;
			Conf.Device->View.View = View.View;
			Conf.Device->View.Position = View.InvViewPosition;
			Conf.Device->View.Direction = View.WorldRotation.dDirection();
			Conf.Device->View.Far = View.FarPlane;
			Conf.Device->View.Near = View.NearPlane;
			Conf.Device->UpdateBuffer(Graphics::RenderBufferType::View);
		}
		void SceneGraph::ExpandMaterials()
		{
			Lock();
			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.MiscFlags = Graphics::ResourceMisc::Buffer_Structured;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Shader_Input;
			F.ElementCount = (unsigned int)Surfaces;
			F.ElementWidth = sizeof(Subsurface);
			F.StructureByteStride = F.ElementWidth;

			TH_RELEASE(Display.MaterialBuffer);
			Display.MaterialBuffer = Conf.Device->CreateElementBuffer(F);
			Surfaces *= 2;
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
			while (DispatchLastEvent());
		}
		void SceneGraph::ResizeBuffers()
		{
			if (!Camera)
				return ResizeRenderBuffers();

			Lock();
			ResizeRenderBuffers();

			auto* Array = GetComponents<Components::Camera>();
			for (auto It = Array->Begin(); It != Array->End(); ++It)
				(*It)->As<Components::Camera>()->ResizeBuffers();
			Unlock();
		}
		void SceneGraph::ResizeRenderBuffers()
		{
			Graphics::MultiRenderTarget2D::Desc MRT = GetDescMRT();
			Graphics::RenderTarget2D::Desc RT = GetDescRT();
			TH_CLEAR(Display.Merger);

			for (size_t i = 0; i < (size_t)TargetType::Count; i++)
			{
				TH_RELEASE(Display.MRT[i]);
				Display.MRT[i] = Conf.Device->CreateMultiRenderTarget2D(MRT);

				TH_RELEASE(Display.RT[i]);
				Display.RT[i] = Conf.Device->CreateRenderTarget2D(RT);
			}
		}
		void SceneGraph::FillMaterialBuffers()
		{
			Graphics::MappedSubresource Stream;
			if (!Conf.Device->Map(Display.MaterialBuffer, Graphics::ResourceMap::Write_Discard, &Stream))
				return;

			Subsurface* Array = (Subsurface*)Stream.Pointer; uint64_t Size = 0;
			for (auto It = Materials.Begin(); It != Materials.End(); ++It)
			{
				Subsurface& Next = Array[Size];
				(*It)->Slot = (int64_t)Size;
				Next = (*It)->Surface;

				if (++Size >= Surfaces)
					break;
			}

			Conf.Device->Unmap(Display.MaterialBuffer, &Stream);
			Conf.Device->SetStructureBuffer(Display.MaterialBuffer, 0, TH_PS | TH_CS);
		}
		void SceneGraph::RayTest(uint64_t Section, const Compute::Ray& Origin, float MaxDistance, const RayCallback& Callback)
		{
			if (!Callback)
				return;

			Core::Pool<Component*>* Array = GetComponents(Section);
			Compute::Ray Base = Origin;
			Compute::Vector3 Hit;

			for (auto It = Array->Begin(); It != Array->End(); ++It)
			{
				Component* Current = *It;
				if (MaxDistance > 0.0f && Current->Parent->Distance > MaxDistance)
					continue;

				if (Compute::Common::CursorRayTest(Base, Current->GetBoundingBox(), &Hit) && !Callback(Current, Hit))
					break;
			}
		}
		void SceneGraph::ScriptHook(const std::string& Name)
		{
			auto* Array = GetComponents<Components::Scriptable>();
			for (auto It = Array->Begin(); It != Array->End(); ++It)
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

			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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
		void SceneGraph::SetVoxelBufferSize(size_t Size)
		{
			if (Size % 8 != 0)
				Size = Display.VoxelSize;

			Graphics::Texture3D::Desc I;
			I.Width = I.Height = I.Depth = Display.VoxelSize = Size;
			I.MipLevels = 0;
			I.Writable = true;

			TH_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Diffuse]);
			Display.VoxelBuffers[(size_t)VoxelType::Diffuse] = Conf.Device->CreateTexture3D(I);

			I.FormatMode = Graphics::Format::R16G16B16A16_Float;
			TH_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Normal]);
			Display.VoxelBuffers[(size_t)VoxelType::Normal] = Conf.Device->CreateTexture3D(I);

			I.FormatMode = Graphics::Format::R8G8B8A8_Unorm;
			TH_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Surface]);
			Display.VoxelBuffers[(size_t)VoxelType::Surface] = Conf.Device->CreateTexture3D(I);
		}
		void SceneGraph::SetMRT(TargetType Type, bool Clear)
		{
			Graphics::MultiRenderTarget2D* Target = Display.MRT[(size_t)Type];
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
			Graphics::RenderTarget2D* Target = Display.RT[(size_t)Type];
			Conf.Device->SetTarget(Target);

			if (!Clear)
				return;

			Conf.Device->Clear(Target, 0, 0, 0, 0);
			Conf.Device->ClearDepth(Target);
		}
		void SceneGraph::SwapMRT(TargetType Type, Graphics::MultiRenderTarget2D* New)
		{
			size_t Index = (size_t)Type;
			if (Display.MRT[Index] == New)
				return;

			Graphics::MultiRenderTarget2D* Cache = Display.MRT[Index + (size_t)TargetType::Count];
			if (New != nullptr)
			{
				Graphics::MultiRenderTarget2D* Base = Display.MRT[Index];
				Display.MRT[Index] = New;

				if (!Cache)
					Display.MRT[Index + (size_t)TargetType::Count] = Base;
			}
			else if (Cache != nullptr)
			{
				Display.MRT[Index] = Cache;
				Display.MRT[Index + (size_t)TargetType::Count] = nullptr;
			}
		}
		void SceneGraph::SwapRT(TargetType Type, Graphics::RenderTarget2D* New)
		{
			size_t Index = (size_t)Type;
			Graphics::RenderTarget2D* Cache = Display.RT[Index + (size_t)TargetType::Count];
			if (New != nullptr)
			{
				Graphics::RenderTarget2D* Base = Display.RT[Index];
				Display.RT[Index] = New;

				if (!Cache)
					Display.RT[Index + (size_t)TargetType::Count] = Base;
			}
			else if (Cache != nullptr)
			{
				Display.RT[Index] = Cache;
				Display.RT[Index + (size_t)TargetType::Count] = nullptr;
			}
		}
		void SceneGraph::ClearMRT(TargetType Type, bool Color, bool Depth)
		{
			Graphics::MultiRenderTarget2D* Target = Display.MRT[(size_t)Type];
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
			Graphics::RenderTarget2D* Target = Display.RT[(size_t)Type];
			if (Color)
				Conf.Device->Clear(Target, 0, 0, 0, 0);

			if (Depth)
				Conf.Device->ClearDepth(Target);
		}
		bool SceneGraph::GetVoxelBuffer(Graphics::Texture3D** In, Graphics::Texture3D** Out)
		{
			if (!In || !Out || !Display.VoxelBuffers[0] || !Display.VoxelBuffers[1] || !Display.VoxelBuffers[2])
				return false;

			for (unsigned int i = 0; i < 3; i++)
			{
				In[i] = Display.VoxelBuffers[i];
				Out[i] = nullptr;
			}

			return true;
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
		bool SceneGraph::DispatchEvent(const std::string& EventName, const Core::VariantArgs& Args)
		{
			Core::VariantArgs Subargs =
			{
				{ "event", Core::Var::Pointer(TH_NEW(Event, EventName, this, Args)) }
			};
			return Core::Schedule::Get()->SetEvent("scene-event", std::move(Subargs));
		}
		bool SceneGraph::DispatchEvent(Component* Target, const std::string& EventName, const Core::VariantArgs& Args)
		{
			Core::VariantArgs Subargs =
			{
				{ "event", Core::Var::Pointer(TH_NEW(Event, EventName, Target, Args)) }
			};
			return Core::Schedule::Get()->SetEvent("scene-event", std::move(Subargs));
		}
		bool SceneGraph::DispatchEvent(Entity* Target, const std::string& EventName, const Core::VariantArgs& Args)
		{
			Core::VariantArgs Subargs =
			{
				{ "event", Core::Var::Pointer(TH_NEW(Event, EventName, Target, Args)) }
			};
			return Core::Schedule::Get()->SetEvent("scene-event", std::move(Subargs));
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
				for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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

			TH_DELETE(Event, Message);
			return Result;
		}
		void SceneGraph::Mutate(Entity* Target, bool Added)
		{
			Core::VariantArgs Args;
			Args["entity-ptr"] = Core::Var::Pointer((void*)Target);
			Args["added"] = Core::Var::Boolean(Added);

			DispatchEvent("mutation", Args);
		}
		void SceneGraph::AddDrawable(Drawable* Source, GeoCategory Category)
		{
			if (!Source)
				return;

			if (Category == GeoCategory::Opaque)
				GetOpaque(Source->Source)->Add(Source);
			else if (Category == GeoCategory::Transparent)
				GetTransparent(Source->Source)->Add(Source);
		}
		void SceneGraph::RemoveDrawable(Drawable* Source, GeoCategory Category)
		{
			if (!Source)
				return;

			if (Category == GeoCategory::Opaque)
				GetOpaque(Source->Source)->Remove(Source);
			else if (Category == GeoCategory::Transparent)
				GetTransparent(Source->Source)->Remove(Source);
		}
		Material* SceneGraph::AddMaterial(const std::string& Name)
		{
			if (Materials.Size() >= Materials.Capacity())
				return nullptr;

			if (Materials.Size() > Surfaces)
				ExpandMaterials();

			Material* Result = new Material(this, Name);
			Materials.Add(Result);

			return Result;
		}
		Material* SceneGraph::CloneMaterial(Material* Base, const std::string& Name)
		{
			if (!Base)
				return nullptr;

			Material* Copy = AddMaterial(Name);
			if (!Copy)
				return nullptr;

			memcpy(&Copy->Surface, &Base->Surface, sizeof(Subsurface));
			if (Base->DiffuseMap != nullptr)
				Copy->DiffuseMap = (Graphics::Texture2D*)Base->DiffuseMap->AddRef();

			if (Base->NormalMap != nullptr)
				Copy->NormalMap = (Graphics::Texture2D*)Base->NormalMap->AddRef();

			if (Base->MetallicMap != nullptr)
				Copy->MetallicMap = (Graphics::Texture2D*)Base->MetallicMap->AddRef();

			if (Base->RoughnessMap != nullptr)
				Copy->RoughnessMap = (Graphics::Texture2D*)Base->RoughnessMap->AddRef();

			if (Base->HeightMap != nullptr)
				Copy->HeightMap = (Graphics::Texture2D*)Base->HeightMap->AddRef();

			if (Base->OcclusionMap != nullptr)
				Copy->OcclusionMap = (Graphics::Texture2D*)Base->OcclusionMap->AddRef();

			if (Base->EmissionMap != nullptr)
				Copy->EmissionMap = (Graphics::Texture2D*)Base->EmissionMap->AddRef();

			return Copy;
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
		Material* SceneGraph::GetMaterial(const std::string& Name)
		{
			for (auto It = Materials.Begin(); It != Materials.End(); ++It)
			{
				if ((*It)->Name == Name)
					return *It;
			}

			return nullptr;
		}
		Material* SceneGraph::GetMaterial(uint64_t Material)
		{
			if (Material >= Materials.Size())
				return nullptr;

			return Materials[Material];
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
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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
		Core::Pool<Component*>* SceneGraph::GetComponents(uint64_t Section)
		{
			Core::Pool<Component*>* Array = &Components[Section];
			if (Array->Capacity() >= Conf.ComponentCount)
				return Array;

			Lock();
			Array->Reserve(Conf.ComponentCount);
			Unlock();

			return Array;
		}
		Core::Pool<Drawable*>* SceneGraph::GetOpaque(uint64_t Section)
		{
			Core::Pool<Drawable*>* Array = &Drawables[Section].Opaque;
			if (Array->Capacity() >= Conf.ComponentCount)
				return Array;

			Lock();
			Array->Reserve(Conf.ComponentCount);
			Unlock();

			return Array;
		}
		Core::Pool<Drawable*>* SceneGraph::GetTransparent(uint64_t Section)
		{
			Core::Pool<Drawable*>* Array = &Drawables[Section].Transparent;
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
			Desc.MiscFlags = Graphics::ResourceMisc::Generate_Mips;
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
			Desc.MiscFlags = Graphics::ResourceMisc::Generate_Mips;
			Desc.Width = (unsigned int)(Target->GetWidth() * Conf.RenderQuality);
			Desc.Height = (unsigned int)(Target->GetHeight() * Conf.RenderQuality);
			Desc.MipLevels = Conf.Device->GetMipLevel(Desc.Width, Desc.Height);
			Desc.Target = Graphics::SurfaceTarget::T3;
			Desc.FormatMode[0] = GetFormatMRT(0);
			Desc.FormatMode[1] = GetFormatMRT(1);
			Desc.FormatMode[2] = GetFormatMRT(2);
			Desc.FormatMode[3] = GetFormatMRT(3);

			return Desc;
		}
		Graphics::Format SceneGraph::GetFormatMRT(unsigned int Target)
		{
			if (Target == 0)
				return Conf.EnableHDR ? Graphics::Format::R16G16B16A16_Unorm : Graphics::Format::R8G8B8A8_Unorm;

			if (Target == 1)
				return Graphics::Format::R16G16B16A16_Float;

			if (Target == 2)
				return Graphics::Format::R32_Float;

			if (Target == 3)
				return Graphics::Format::R8G8B8A8_Unorm;

			return Graphics::Format::Unknown;
		}
		std::vector<Entity*> SceneGraph::FindParentFreeEntities(Entity* Entity)
		{
			std::vector<Engine::Entity*> Array;
			if (!Entity)
				return Array;

			Lock();
			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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

			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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

			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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

			for (auto It = Entities.Begin(); It != Entities.End(); ++It)
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

			return (Compute::Common::IsCubeInFrustum(Entity->Transform->GetWorld() * ViewProjection, 2) < 0.0f);
		}
		bool SceneGraph::IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection, const Compute::Vector3& ViewPosition, float DrawDistance)
		{
			if (!Entity || Entity->Transform->Position.Distance(ViewPosition) > DrawDistance + Entity->Transform->Scale.Length())
				return false;

			return (Compute::Common::IsCubeInFrustum(Entity->Transform->GetWorld() * ViewProjection, 2) < 0.0f);
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
			for (uint64_t i = 0; i < Entities.Size(); i++)
			{
				if (Entities[i] == Entity)
					return true;
			}

			return false;
		}
		bool SceneGraph::HasEntity(uint64_t Entity)
		{
			return Entity < Entities.Size() ? Entity : -1;
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
			return Materials.Size();
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
		size_t SceneGraph::GetVoxelBufferSize()
		{
			return Display.VoxelSize;
		}
		Graphics::MultiRenderTarget2D* SceneGraph::GetMRT(TargetType Type)
		{
			return Display.MRT[(size_t)Type];
		}
		Graphics::RenderTarget2D* SceneGraph::GetRT(TargetType Type)
		{
			return Display.RT[(size_t)Type];
		}
		Graphics::Texture2D** SceneGraph::GetMerger()
		{
			return &Display.Merger;
		}
		Graphics::ElementBuffer* SceneGraph::GetStructure()
		{
			return Display.MaterialBuffer;
		}
		Graphics::GraphicsDevice* SceneGraph::GetDevice()
		{
			return Conf.Device;
		}
		ShaderCache* SceneGraph::GetShaders()
		{
			return Conf.Shaders;
		}
		PrimitiveCache* SceneGraph::GetPrimitives()
		{
			return Conf.Primitives;
		}
		Compute::Simulator* SceneGraph::GetSimulator()
		{
			return Simulator;
		}
		SceneGraph::Desc& SceneGraph::GetConf()
		{
			return Conf;
		}

		ContentManager::ContentManager(Graphics::GraphicsDevice* NewDevice) : Device(NewDevice), Base(Core::OS::Path::ResolveDirectory(Core::OS::Directory::Get().c_str()))
		{
			SetEnvironment(Base);
		}
		ContentManager::~ContentManager()
		{
			InvalidateCache();
			InvalidateDockers();

			for (auto It = Streams.begin(); It != Streams.end(); ++It)
				TH_RELEASE(It->first);

			for (auto It = Processors.begin(); It != Processors.end(); ++It)
				TH_RELEASE(It->second);
		}
		void ContentManager::InvalidateDockers()
		{
			Mutex.lock();
			for (auto It = Dockers.begin(); It != Dockers.end(); ++It)
				TH_DELETE(AssetArchive, It->second);

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

					TH_DELETE(AssetCache, Entry.second);
				}
			}

			Assets.clear();
			Mutex.unlock();
		}
		void ContentManager::InvalidatePath(const std::string& Path)
		{
			std::string File = Core::OS::Path::ResolveResource(Path, Environment);
			if (File.empty())
				return;

			Mutex.lock();
			auto It = Assets.find(Core::Parser(File).Replace('\\', '/').Replace(Environment, "./").R());
			if (It != Assets.end())
				Assets.erase(It);
			Mutex.unlock();
		}
		void ContentManager::SetEnvironment(const std::string& Path)
		{
			Mutex.lock();
			Environment = Core::OS::Path::ResolveDirectory(Path.c_str());
			Core::Parser(&Environment).Replace('\\', '/');
			Core::OS::Directory::Set(Environment.c_str());
			Mutex.unlock();
		}
		void* ContentManager::LoadForward(const std::string& Path, Processor* Processor, const Core::VariantArgs& Map)
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

			std::string File = Path;
			if (!Core::OS::Path::IsRemote(Path.c_str()))
			{
				Mutex.lock();
				File = Core::OS::Path::ResolveResource(Path, Environment);
				Mutex.unlock();

				if (File.empty())
				{
					TH_ERROR("file \"%s\" wasn't found", Path.c_str());
					return nullptr;
				}
			}

			AssetCache* Asset = Find(Processor, File);
			if (Asset != nullptr)
				return Processor->Duplicate(Asset, Map);

			auto* Stream = Core::OS::File::Open(File, Core::FileMode::Binary_Read_Only);
			if (!Stream)
				return nullptr;

			Object = Processor->Deserialize(Stream, Stream->GetSize(), 0, Map);
			TH_RELEASE(Stream);

			return Object;
		}
		void* ContentManager::LoadStreaming(const std::string& Path, Processor* Processor, const Core::VariantArgs& Map)
		{
			if (Path.empty())
				return nullptr;

			Core::Parser File(Path);
			File.Replace('\\', '/').Replace("./", "");

			auto Docker = Dockers.find(File.R());
			if (Docker == Dockers.end() || !Docker->second || !Docker->second->Stream)
				return nullptr;

			AssetCache* Asset = Find(Processor, File.R());
			if (Asset != nullptr)
				return Processor->Duplicate(Asset, Map);

			auto It = Streams.find(Docker->second->Stream);
			if (It == Streams.end())
			{
				TH_ERROR("cannot resolve stream offset for \"%s\"", Path.c_str());
				return nullptr;
			}

			auto* Stream = Docker->second->Stream;
			Stream->Seek(Core::FileSeek::Begin, It->second + Docker->second->Offset);
			Stream->GetSource() = File.R();

			return Processor->Deserialize(Stream, Docker->second->Length, It->second + Docker->second->Offset, Map);
		}
		bool ContentManager::SaveForward(const std::string& Path, Processor* Processor, void* Object, const Core::VariantArgs& Map)
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
			std::string Directory = Core::OS::Path::GetDirectory(Path.c_str());
			std::string File = Core::OS::Path::Resolve(Directory, Environment);
			File.append(Path.substr(Directory.size()));
			Mutex.unlock();

			auto* Stream = Core::OS::File::Open(File, Core::FileMode::Binary_Write_Only);
			if (!Stream)
			{
				Stream = Core::OS::File::Open(Path, Core::FileMode::Binary_Write_Only);
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
			std::string File = Core::OS::Path::ResolveResource(Path, Environment);
			Mutex.unlock();

			if (File.empty())
			{
				TH_ERROR("file \"%s\" wasn't found", Path.c_str());
				return false;
			}

			auto* Stream = new Core::GzStream();
			if (!Stream->Open(File.c_str(), Core::FileMode::Binary_Read_Only))
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
				AssetArchive* Docker = TH_NEW(AssetArchive);
				Docker->Stream = Stream;

				uint64_t Length;
				Stream->Read((char*)&Length, sizeof(uint64_t));
				Stream->Read((char*)&Docker->Offset, sizeof(uint64_t));
				Stream->Read((char*)&Docker->Length, sizeof(uint64_t));

				if (!Length)
				{
					TH_DELETE(AssetArchive, Docker);
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

			auto* Stream = new Core::GzStream();
			if (!Stream->Open(Core::OS::Path::Resolve(Path, Environment).c_str(), Core::FileMode::Write_Only))
			{
				TH_ERROR("cannot open \"%s\" for writing", Path.c_str());
				TH_RELEASE(Stream);
				return false;
			}

			std::string DBase = Core::OS::Path::Resolve(Directory, Environment);
			auto Tree = new Core::FileTree(DBase);
			Stream->Write("\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16);

			uint64_t Size = Tree->GetFiles();
			Stream->Write((char*)&Size, sizeof(uint64_t));

			uint64_t Offset = 0;
			Tree->Loop([Stream, &Offset, &DBase, &Name](Core::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					auto* File = Core::OS::File::Open(Resource, Core::FileMode::Binary_Read_Only);
					if (!File)
						continue;

					std::string Path = Core::Parser(Resource).Replace(DBase, Name).Replace('\\', '/').R();
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
			Tree->Loop([Stream](Core::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					auto* File = Core::OS::File::Open(Resource, Core::FileMode::Binary_Read_Only);
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

			AssetCache* Asset = TH_NEW(AssetCache);
			Asset->Path = Core::Parser(Path).Replace('\\', '/').Replace(Environment, "./").R();
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
			auto It = Assets.find(Core::Parser(Path).Replace('\\', '/').Replace(Environment, "./").R());
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
			if (I->Usage & (size_t)ApplicationSet::ActivitySet)
			{
				if (!I->Activity.Width || !I->Activity.Height)
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
					Activity->SetCursorVisibility(I->Cursor);
					Activity->Callbacks.KeyState = [this](Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
					{
#ifdef TH_WITH_RMLUI
						GUI::Context* GUI = (GUI::Context*)GetGUI();
						if (GUI != nullptr)
							GUI->EmitKey(Key, Mod, Virtual, Repeat, Pressed);
#endif
						KeyEvent(Key, Mod, Virtual, Repeat, Pressed);
					};
					Activity->Callbacks.Input = [this](char* Buffer, int Length)
					{
#ifdef TH_WITH_RMLUI
						GUI::Context* GUI = (GUI::Context*)GetGUI();
						if (GUI != nullptr)
							GUI->EmitInput(Buffer, Length);
#endif
						InputEvent(Buffer, Length);
					};
					Activity->Callbacks.CursorWheelState = [this](int X, int Y, bool Normal)
					{
#ifdef TH_WITH_RMLUI
						GUI::Context* GUI = (GUI::Context*)GetGUI();
						if (GUI != nullptr)
							GUI->EmitWheel(X, Y, Normal, Activity->GetKeyModState());
#endif
						WheelEvent(X, Y, Normal);
					};
					Activity->Callbacks.WindowStateChange = [this](Graphics::WindowState NewState, int X, int Y)
					{
#ifdef TH_WITH_RMLUI
						if (NewState == Graphics::WindowState::Resize)
						{
							GUI::Context* GUI = (GUI::Context*)GetGUI();
							if (GUI != nullptr)
								GUI->EmitResize(X, Y);
						}
#endif
						WindowEvent(NewState, X, Y);
					};

					if (I->Usage & (size_t)ApplicationSet::GraphicsSet)
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

						Cache.Shaders = new ShaderCache(Renderer);
						Cache.Primitives = new PrimitiveCache(Renderer);
					}
				}
				else
					TH_ERROR("cannot detect display to create activity");
			}
#endif
			if (I->Usage & (size_t)ApplicationSet::AudioSet)
			{
				Audio = new Audio::AudioDevice();
				if (!Audio->IsValid())
				{
					TH_ERROR("audio device cannot be created");
					return;
				}
			}

			if (I->Usage & (size_t)ApplicationSet::ContentSet)
			{
				Content = new ContentManager(Renderer);
				Content->AddProcessor<Processors::Asset, Engine::AssetFile>();
				Content->AddProcessor<Processors::SceneGraph, Engine::SceneGraph>();
				Content->AddProcessor<Processors::AudioClip, Audio::AudioClip>();
				Content->AddProcessor<Processors::Texture2D, Graphics::Texture2D>();
				Content->AddProcessor<Processors::Shader, Graphics::Shader>();
				Content->AddProcessor<Processors::Model, Graphics::Model>();
				Content->AddProcessor<Processors::SkinModel, Graphics::SkinModel>();
				Content->AddProcessor<Processors::Document, Core::Document>();
				Content->AddProcessor<Processors::Server, Network::HTTP::Server>();
				Content->AddProcessor<Processors::Shape, Compute::UnmanagedShape>();
				Content->SetEnvironment(I->Environment.empty() ? Core::OS::Directory::Get() + I->Directory : I->Environment + I->Directory);
			}

			if (I->Usage & (size_t)ApplicationSet::ScriptSet)
				VM = new Script::VMManager();
#ifdef TH_WITH_RMLUI
			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
			{
				GUI::Subsystem::SetMetadata(Activity, Content, nullptr);
				GUI::Subsystem::SetManager(VM);
			}
#endif
            NetworkQueue = (I->Usage & (size_t)ApplicationSet::NetworkSet);
            if (NetworkQueue)
                Network::Driver::Create(256, I->Async ? 100 : 0);

			State = ApplicationState::Staging;
		}
		Application::~Application()
		{
			if (Renderer != nullptr)
				Renderer->FlushState();

			TH_RELEASE(Scene);
			TH_RELEASE(VM);
			TH_RELEASE(Audio);
#ifdef TH_WITH_RMLUI
			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
				Engine::GUI::Subsystem::Release();
#endif
			TH_RELEASE(Cache.Shaders);
			TH_RELEASE(Cache.Primitives);
			TH_RELEASE(Content);
			TH_RELEASE(Renderer);
			TH_RELEASE(Activity);

			for (auto& Job : Workers)
				TH_DELETE(Reactor, Job);

            if (NetworkQueue)
                Network::Driver::Release();
            
			Host = nullptr;
		}
		void Application::ScriptHook(Script::VMGlobal* Global)
		{
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
		void Application::CloseEvent()
		{
		}
		bool Application::ComposeEvent()
		{
			return false;
		}
		void Application::Publish(Core::Timer* Time)
		{
		}
		void Application::Initialize(Desc* I)
		{
		}
		void Application::Start(Desc* I)
		{
			if (I == nullptr)
				return;

			if (!ComposeEvent())
				Compose();

			if (I->Usage & (size_t)ApplicationSet::ActivitySet && !Activity)
			{
				TH_ERROR("[conf] activity was not found");
				return;
			}

			if (I->Usage & (size_t)ApplicationSet::GraphicsSet && !Renderer)
			{
				TH_ERROR("[conf] graphics device was not found");
				return;
			}

			if (I->Usage & (size_t)ApplicationSet::AudioSet && !Audio)
			{
				TH_ERROR("[conf] audio device was not found");
				return;
			}

			if (I->Usage & (size_t)ApplicationSet::ScriptSet)
			{
				if (!VM)
				{
					TH_ERROR("[conf] VM was not found");
					return;
				}
				else
					ScriptHook(&VM->Global());
			}

			Initialize(I);
			if (State == ApplicationState::Terminated)
				return;
            
			if (Scene != nullptr)
				Scene->Dispatch();

			if (Workers.empty())
				Workers.push_back(TH_NEW(Reactor, this, 0.0, nullptr));

			auto* Queue = Core::Schedule::Get();
			for (auto It = Workers.begin() + 1; It != Workers.end(); ++It)
			{
				Reactor* Job = *It;
				Queue->SetTask([Job]() { Application::Callee(Job); });
			}

			if (NetworkQueue)
				Queue->SetTask(Network::Driver::Multiplex);

			Reactor* Job = Workers.front();
			Job->Time->SetStepLimitation(I->MaxFrames, I->MinFrames);
			Job->Time->FrameLimit = I->Framerate;
#ifdef TH_WITH_RMLUI
			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
				GUI::Subsystem::SetMetadata(Activity, Content, Job->Time);
#endif
			ApplicationState OK;
			if (I->Async)
				OK = State = ApplicationState::Multithreaded;
			else
				OK = State = ApplicationState::Singlethreaded;

			Queue->Start(I->Async, std::max((uint64_t)Workers.size(), I->Threads), I->Coroutines, I->Stack);
			if (Activity != nullptr && I->Async)
			{
				while (State == OK)
				{
					Activity->Dispatch();
					Job->UpdateCore();
					Publish(Job->Time);
				}
			}
			else if (Activity != nullptr && !I->Async)
			{
				while (State == OK)
				{
					Queue->Dispatch();
					Activity->Dispatch();
					Job->UpdateCore();
					Publish(Job->Time);
				}
			}
			else if (!Activity && I->Async)
			{
				while (State == OK)
				{
					Job->UpdateCore();
					Publish(Job->Time);
				}
			}
			else if (!Activity && !I->Async)
			{
				while (State == OK)
				{
					Queue->Dispatch();
					Job->UpdateCore();
					Publish(Job->Time);
				}
			}

			CloseEvent();
			Queue->Stop();
		}
		void Application::Stop()
		{
			State = ApplicationState::Terminated;
		}
		void* Application::GetGUI()
		{
#ifdef TH_WITH_RMLUI
			if (!Scene)
				return nullptr;

			Components::Camera* BaseCamera = (Components::Camera*)Scene->GetCamera();
			if (!BaseCamera)
				return nullptr;

			Renderers::UserInterface* Result = BaseCamera->GetRenderer()->GetRenderer<Renderers::UserInterface>();
			return Result != nullptr ? Result->GetContext() : nullptr;
#else
			return nullptr;
#endif
		}
		ApplicationState Application::GetState()
		{
			return State;
		}
		void Application::Callee(Reactor* Job)
		{
			auto* Queue = Core::Schedule::Get();
			do
			{
				Job->UpdateTask();
			} while (Job->App->State == ApplicationState::Multithreaded && Queue->IsBlockable());

			if (Job->App->State == ApplicationState::Singlethreaded && Queue->IsActive() && !Queue->IsBlockable())
				Queue->SetTask([Job]() { Callee(Job); });
		}
		void Application::Compose()
		{
			Core::Composer::Push<Components::RigidBody, Entity*>();
			Core::Composer::Push<Components::SoftBody, Entity*>();
			Core::Composer::Push<Components::Acceleration, Entity*>();
			Core::Composer::Push<Components::SliderConstraint, Entity*>();
			Core::Composer::Push<Renderers::SoftBody, RenderSystem*>();
			Core::Composer::Push<Components::Model, Entity*>();
			Core::Composer::Push<Components::Skin, Entity*>();
			Core::Composer::Push<Components::Emitter, Entity*>();
			Core::Composer::Push<Components::Decal, Entity*>();
			Core::Composer::Push<Components::SkinAnimator, Entity*>();
			Core::Composer::Push<Components::KeyAnimator, Entity*>();
			Core::Composer::Push<Components::EmitterAnimator, Entity*>();
			Core::Composer::Push<Components::FreeLook, Entity*>();
			Core::Composer::Push<Components::Fly, Entity*>();
			Core::Composer::Push<Components::AudioSource, Entity*>();
			Core::Composer::Push<Components::AudioListener, Entity*>();
			Core::Composer::Push<Components::PointLight, Entity*>();
			Core::Composer::Push<Components::SpotLight, Entity*>();
			Core::Composer::Push<Components::LineLight, Entity*>();
			Core::Composer::Push<Components::SurfaceLight, Entity*>();
			Core::Composer::Push<Components::Illuminator, Entity*>();
			Core::Composer::Push<Components::Camera, Entity*>();
			Core::Composer::Push<Components::Scriptable, Entity*>();
			Core::Composer::Push<Renderers::Model, RenderSystem*>();
			Core::Composer::Push<Renderers::Skin, RenderSystem*>();
			Core::Composer::Push<Renderers::Emitter, RenderSystem*>();
			Core::Composer::Push<Renderers::Decal, RenderSystem*>();
			Core::Composer::Push<Renderers::Lighting, RenderSystem*>();
			Core::Composer::Push<Renderers::Transparency, RenderSystem*>();
			Core::Composer::Push<Renderers::Glitch, RenderSystem*>();
			Core::Composer::Push<Renderers::Tone, RenderSystem*>();
			Core::Composer::Push<Renderers::DoF, RenderSystem*>();
			Core::Composer::Push<Renderers::Bloom, RenderSystem*>();
			Core::Composer::Push<Renderers::SSR, RenderSystem*>();
			Core::Composer::Push<Renderers::SSAO, RenderSystem*>();
			Core::Composer::Push<Renderers::MotionBlur, RenderSystem*>();
			Core::Composer::Push<Renderers::UserInterface, RenderSystem*>();
			Core::Composer::Push<Audio::Effects::Reverb>();
			Core::Composer::Push<Audio::Effects::Chorus>();
			Core::Composer::Push<Audio::Effects::Distortion>();
			Core::Composer::Push<Audio::Effects::Echo>();
			Core::Composer::Push<Audio::Effects::Flanger>();
			Core::Composer::Push<Audio::Effects::FrequencyShifter>();
			Core::Composer::Push<Audio::Effects::VocalMorpher>();
			Core::Composer::Push<Audio::Effects::PitchShifter>();
			Core::Composer::Push<Audio::Effects::RingModulator>();
			Core::Composer::Push<Audio::Effects::Autowah>();
			Core::Composer::Push<Audio::Effects::Compressor>();
			Core::Composer::Push<Audio::Effects::Equalizer>();
			Core::Composer::Push<Audio::Filters::Lowpass>();
			Core::Composer::Push<Audio::Filters::Bandpass>();
			Core::Composer::Push<Audio::Filters::Highpass>();
		}
		Core::Schedule* Application::Queue()
		{
			return Core::Schedule::Get();
		}
		Application* Application::Get()
		{
			return Host;
		}
		Application* Application::Host = nullptr;
	}
}
