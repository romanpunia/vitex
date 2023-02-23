#include "engine.h"
#include "../engine/components.h"
#include "../engine/processors.h"
#include "../engine/renderers.h"
#include "../network/http.h"
#include "../audio/effects.h"
#include "../audio/filters.h"
#include <sstream>
#ifdef ED_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#undef Complex
#endif

namespace
{
	template <typename T>
	struct Movable
	{
		mutable T value;

		Movable()
		{
		}
		Movable(T&& v) : value(std::move(v))
		{
		}
		Movable(const Movable<T>& rhs) : value(std::move(rhs.value))
		{
		}
		Movable(Movable<T>&& rhs) = default;
		Movable& operator=(const Movable<T>& rhs) = delete;
		Movable& operator=(Movable<T>&& rhs) = delete;
	};

	template <typename T>
	using AsMovable = Movable<typename std::remove_reference<T>::type>;

	template <typename T>
	inline AsMovable<T> InterpretAsMove(T&& r)
	{
		return AsMovable<T>(std::move(r));
	}
}

namespace Edge
{
	namespace Engine
	{
		Ticker::Ticker() noexcept : Time(0.0f), Delay(16.0f)
		{
		}
		bool Ticker::TickEvent(float ElapsedTime)
		{
			if (ElapsedTime - Time > Delay)
			{
				Time = ElapsedTime;
				return true;
			}

			return false;
		}
		float Ticker::GetTime()
		{
			return Time;
		}

		Event::Event(const std::string& NewName) noexcept : Name(NewName)
		{
		}
		Event::Event(const std::string& NewName, const Core::VariantArgs& NewArgs) noexcept : Name(NewName), Args(NewArgs)
		{
		}
		Event::Event(const std::string& NewName, Core::VariantArgs&& NewArgs) noexcept : Name(NewName), Args(std::move(NewArgs))
		{
		}
		Event::Event(const Event& Other) noexcept : Name(Other.Name), Args(Other.Args)
		{
		}
		Event::Event(Event&& Other) noexcept : Name(std::move(Other.Name)), Args(std::move(Other.Args))
		{
		}
		Event& Event::operator= (const Event& Other) noexcept
		{
			Name = Other.Name;
			Args = Other.Args;
			return *this;
		}
		Event& Event::operator= (Event&& Other) noexcept
		{
			Name = std::move(Other.Name);
			Args = std::move(Other.Args);
			return *this;
		}

		AssetFile::AssetFile(char* SrcBuffer, size_t SrcSize) noexcept : Buffer(SrcBuffer), Size(SrcSize)
		{
		}
		AssetFile::~AssetFile() noexcept
		{
			if (Buffer != nullptr)
				ED_FREE(Buffer);
		}
		char* AssetFile::GetBuffer()
		{
			return Buffer;
		}
		size_t AssetFile::GetSize()
		{
			return Size;
		}

		void Viewer::Set(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Fov, float _Ratio, float _Near, float _Far, RenderCulling _Type)
		{
			Set(_View, _Projection, _Position, -_View.Rotation(), _Fov, _Ratio, _Near, _Far, _Type);
		}
		void Viewer::Set(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, const Compute::Vector3& _Rotation, float _Fov, float _Ratio, float _Near, float _Far, RenderCulling _Type)
		{
			View = _View;
			Projection = _Projection;
			ViewProjection = _View * _Projection;
			InvViewProjection = ViewProjection.Inv();
			InvPosition = _Position.Inv();
			Position = _Position;
			Rotation = _Rotation;
			FarPlane = (_Far < _Near ? 999999999.0f : _Far);
			NearPlane = _Near;
			Ratio = _Ratio;
			Fov = _Fov;
			Culling = _Type;
			CubicViewProjection[0] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::PositiveX, Position) * Projection;
			CubicViewProjection[1] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::NegativeX, Position) * Projection;
			CubicViewProjection[2] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::PositiveY, Position) * Projection;
			CubicViewProjection[3] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::NegativeY, Position) * Projection;
			CubicViewProjection[4] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::PositiveZ, Position) * Projection;
			CubicViewProjection[5] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::NegativeZ, Position) * Projection;
		}

		void Series::Pack(Core::Schema* V, bool Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("b", Core::Var::Boolean(Value));
		}
		void Series::Pack(Core::Schema* V, int Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, unsigned int Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, unsigned long Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, float Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("n", Core::Var::Number(Value));
		}
		void Series::Pack(Core::Schema* V, double Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("n", Core::Var::Number(Value));
		}
		void Series::Pack(Core::Schema* V, int64_t Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, long double Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("n", Core::Var::Number(Value));
		}
		void Series::Pack(Core::Schema* V, unsigned long long Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, const char* Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("s", Core::Var::String(Value ? Value : ""));
		}
		void Series::Pack(Core::Schema* V, const Compute::Vector2& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("x", Core::Var::Number(Value.X));
			V->SetAttribute("y", Core::Var::Number(Value.Y));
		}
		void Series::Pack(Core::Schema* V, const Compute::Vector3& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("x", Core::Var::Number(Value.X));
			V->SetAttribute("y", Core::Var::Number(Value.Y));
			V->SetAttribute("z", Core::Var::Number(Value.Z));
		}
		void Series::Pack(Core::Schema* V, const Compute::Vector4& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("x", Core::Var::Number(Value.X));
			V->SetAttribute("y", Core::Var::Number(Value.Y));
			V->SetAttribute("z", Core::Var::Number(Value.Z));
			V->SetAttribute("w", Core::Var::Number(Value.W));
		}
		void Series::Pack(Core::Schema* V, const Compute::Matrix4x4& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const Attenuation& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("radius"), Value.Radius);
			Series::Pack(V->Set("c1"), Value.C1);
			Series::Pack(V->Set("c2"), Value.C2);
		}
		void Series::Pack(Core::Schema* V, const AnimatorState& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("looped"), Value.Looped);
			Series::Pack(V->Set("paused"), Value.Paused);
			Series::Pack(V->Set("blended"), Value.Blended);
			Series::Pack(V->Set("clip"), Value.Clip);
			Series::Pack(V->Set("frame"), Value.Frame);
			Series::Pack(V->Set("rate"), Value.Rate);
			Series::Pack(V->Set("duration"), Value.Duration);
			Series::Pack(V->Set("time"), Value.Time);
		}
		void Series::Pack(Core::Schema* V, const SpawnerProperties& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("iterations"), Value.Iterations);

			Core::Schema* Angular = V->Set("angular");
			Series::Pack(Angular->Set("intensity"), Value.Angular.Intensity);
			Series::Pack(Angular->Set("accuracy"), Value.Angular.Accuracy);
			Series::Pack(Angular->Set("min"), Value.Angular.Min);
			Series::Pack(Angular->Set("max"), Value.Angular.Max);

			Core::Schema* Diffusion = V->Set("diffusion");
			Series::Pack(Diffusion->Set("intensity"), Value.Diffusion.Intensity);
			Series::Pack(Diffusion->Set("accuracy"), Value.Diffusion.Accuracy);
			Series::Pack(Diffusion->Set("min"), Value.Diffusion.Min);
			Series::Pack(Diffusion->Set("max"), Value.Diffusion.Max);

			Core::Schema* Noise = V->Set("noise");
			Series::Pack(Noise->Set("intensity"), Value.Noise.Intensity);
			Series::Pack(Noise->Set("accuracy"), Value.Noise.Accuracy);
			Series::Pack(Noise->Set("min"), Value.Noise.Min);
			Series::Pack(Noise->Set("max"), Value.Noise.Max);

			Core::Schema* Position = V->Set("position");
			Series::Pack(Position->Set("intensity"), Value.Position.Intensity);
			Series::Pack(Position->Set("accuracy"), Value.Position.Accuracy);
			Series::Pack(Position->Set("min"), Value.Position.Min);
			Series::Pack(Position->Set("max"), Value.Position.Max);

			Core::Schema* Rotation = V->Set("rotation");
			Series::Pack(Rotation->Set("intensity"), Value.Rotation.Intensity);
			Series::Pack(Rotation->Set("accuracy"), Value.Rotation.Accuracy);
			Series::Pack(Rotation->Set("min"), Value.Rotation.Min);
			Series::Pack(Rotation->Set("max"), Value.Rotation.Max);

			Core::Schema* Scale = V->Set("scale");
			Series::Pack(Scale->Set("intensity"), Value.Scale.Intensity);
			Series::Pack(Scale->Set("accuracy"), Value.Scale.Accuracy);
			Series::Pack(Scale->Set("min"), Value.Scale.Min);
			Series::Pack(Scale->Set("max"), Value.Scale.Max);

			Core::Schema* Velocity = V->Set("velocity");
			Series::Pack(Velocity->Set("intensity"), Value.Velocity.Intensity);
			Series::Pack(Velocity->Set("accuracy"), Value.Velocity.Accuracy);
			Series::Pack(Velocity->Set("min"), Value.Velocity.Min);
			Series::Pack(Velocity->Set("max"), Value.Velocity.Max);
		}
		void Series::Pack(Core::Schema* V, const Compute::SkinAnimatorKey& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("pose"), Value.Pose);
			Series::Pack(V->Set("time"), Value.Time);
		}
		void Series::Pack(Core::Schema* V, const Compute::SkinAnimatorClip& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("name"), Value.Name);
			Series::Pack(V->Set("duration"), Value.Duration);
			Series::Pack(V->Set("rate"), Value.Rate);

			Core::Schema* Array = V->Set("frames", Core::Var::Array());
			for (auto&& It : Value.Keys)
				Series::Pack(Array->Set("frame"), It);
		}
		void Series::Pack(Core::Schema* V, const Compute::KeyAnimatorClip& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("name"), Value.Name);
			Series::Pack(V->Set("rate"), Value.Rate);
			Series::Pack(V->Set("duration"), Value.Duration);
			Series::Pack(V->Set("frames"), Value.Keys);
		}
		void Series::Pack(Core::Schema* V, const Compute::AnimatorKey& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("position"), Value.Position);
			Series::Pack(V->Set("rotation"), Value.Rotation);
			Series::Pack(V->Set("scale"), Value.Scale);
			Series::Pack(V->Set("time"), Value.Time);
		}
		void Series::Pack(Core::Schema* V, const Compute::ElementVertex& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const Compute::Vertex& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const Compute::SkinVertex& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const Compute::Joint& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Series::Pack(V->Set("index"), Value.Index);
			Series::Pack(V->Set("name"), Value.Name);
			Series::Pack(V->Set("transform"), Value.Transform);
			Series::Pack(V->Set("bind-shape"), Value.BindShape);

			Core::Schema* Joints = V->Set("childs", Core::Var::Array());
			for (auto& It : Value.Childs)
				Series::Pack(Joints->Set("joint"), It);
		}
		void Series::Pack(Core::Schema* V, const Ticker& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("delay", Core::Var::Number(Value.Delay));
		}
		void Series::Pack(Core::Schema* V, const std::string& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			V->SetAttribute("s", Core::Var::String(Value));
		}
		void Series::Pack(Core::Schema* V, const std::vector<bool>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("b-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<int>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<unsigned int>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<float>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<double>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<int64_t>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<long double>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<uint64_t>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::Vector2>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " ";

			V->Set("v2-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::Vector3>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " " << It.Z << " ";

			V->Set("v3-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::Vector4>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.X << " " << It.Y << " " << It.Z << " " << It.W << " ";

			V->Set("v4-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::Matrix4x4>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
			{
				for (float i : It.Row)
					Stream << i << " ";
			}

			V->Set("m4x4-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<AnimatorState>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const std::vector<SpawnerProperties>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const std::vector<Compute::SkinAnimatorClip>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Core::Schema* Array = V->Set("clips", Core::Var::Array());
			for (auto&& It : Value)
				Series::Pack(Array->Set("clip"), It);
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::KeyAnimatorClip>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Core::Schema* Array = V->Set("clips", Core::Var::Array());
			for (auto&& It : Value)
				Series::Pack(Array->Set("clip"), It);
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::AnimatorKey>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const std::vector<Compute::ElementVertex>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const std::vector<Compute::Joint>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			for (auto&& It : Value)
				Series::Pack(V->Set("joint"), It);
		}
		void Series::Pack(Core::Schema* V, const std::vector<Compute::Vertex>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const std::vector<Compute::SkinVertex>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
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
		void Series::Pack(Core::Schema* V, const std::vector<Ticker>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			std::stringstream Stream;
			for (auto&& It : Value)
				Stream << It.Delay << " ";

			V->Set("tt-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const std::vector<std::string>& Value)
		{
			ED_ASSERT_V(V != nullptr, "schema should be set");
			Core::Schema* Array = V->Set("s-array", Core::Var::Array());
			for (auto&& It : Value)
				Array->Set("s", Core::Var::String(It));

			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		bool Series::Unpack(Core::Schema* V, bool* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = V->GetVar("[b]").GetBoolean();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, int* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = (int)V->GetVar("[i]").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, unsigned int* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = (unsigned int)V->GetVar("[i]").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, unsigned long* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = (unsigned long)V->GetVar("[i]").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, float* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = (float)V->GetVar("[n]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, double* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = (int)V->GetVar("[n]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, long double* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = V->GetVar("[n]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, int64_t* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = V->GetVar("[i]").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, unsigned long long* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = (unsigned long long)V->GetVar("[i]").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::Vector2* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			O->X = (float)V->GetVar("[x]").GetNumber();
			O->Y = (float)V->GetVar("[y]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::Vector3* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			O->X = (float)V->GetVar("[x]").GetNumber();
			O->Y = (float)V->GetVar("[y]").GetNumber();
			O->Z = (float)V->GetVar("[z]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::Vector4* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			O->X = (float)V->GetVar("[x]").GetNumber();
			O->Y = (float)V->GetVar("[y]").GetNumber();
			O->Z = (float)V->GetVar("[z]").GetNumber();
			O->W = (float)V->GetVar("[w]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::Matrix4x4* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, Attenuation* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("radius"), &O->Radius);
			Series::Unpack(V->Get("c1"), &O->C1);
			Series::Unpack(V->Get("c2"), &O->C2);
			return true;
		}
		bool Series::Unpack(Core::Schema* V, AnimatorState* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("looped"), &O->Looped);
			Series::Unpack(V->Get("paused"), &O->Paused);
			Series::Unpack(V->Get("blended"), &O->Blended);
			Series::Unpack(V->Get("clip"), &O->Clip);
			Series::Unpack(V->Get("frame"), &O->Frame);
			Series::Unpack(V->Get("rate"), &O->Rate);
			Series::Unpack(V->Get("duration"), &O->Duration);
			Series::Unpack(V->Get("time"), &O->Time);
			return true;
		}
		bool Series::Unpack(Core::Schema* V, SpawnerProperties* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("iterations"), &O->Iterations);

			Core::Schema* Angular = V->Get("angular");
			Series::Unpack(Angular->Get("intensity"), &O->Angular.Intensity);
			Series::Unpack(Angular->Get("accuracy"), &O->Angular.Accuracy);
			Series::Unpack(Angular->Get("min"), &O->Angular.Min);
			Series::Unpack(Angular->Get("max"), &O->Angular.Max);

			Core::Schema* Diffusion = V->Get("diffusion");
			Series::Unpack(Diffusion->Get("intensity"), &O->Diffusion.Intensity);
			Series::Unpack(Diffusion->Get("accuracy"), &O->Diffusion.Accuracy);
			Series::Unpack(Diffusion->Get("min"), &O->Diffusion.Min);
			Series::Unpack(Diffusion->Get("max"), &O->Diffusion.Max);

			Core::Schema* Noise = V->Get("noise");
			Series::Unpack(Noise->Get("intensity"), &O->Noise.Intensity);
			Series::Unpack(Noise->Get("accuracy"), &O->Noise.Accuracy);
			Series::Unpack(Noise->Get("min"), &O->Noise.Min);
			Series::Unpack(Noise->Get("max"), &O->Noise.Max);

			Core::Schema* Position = V->Get("position");
			Series::Unpack(Position->Get("intensity"), &O->Position.Intensity);
			Series::Unpack(Position->Get("accuracy"), &O->Position.Accuracy);
			Series::Unpack(Position->Get("min"), &O->Position.Min);
			Series::Unpack(Position->Get("max"), &O->Position.Max);

			Core::Schema* Rotation = V->Get("rotation");
			Series::Unpack(Rotation->Get("intensity"), &O->Rotation.Intensity);
			Series::Unpack(Rotation->Get("accuracy"), &O->Rotation.Accuracy);
			Series::Unpack(Rotation->Get("min"), &O->Rotation.Min);
			Series::Unpack(Rotation->Get("max"), &O->Rotation.Max);

			Core::Schema* Scale = V->Get("scale");
			Series::Unpack(Scale->Get("intensity"), &O->Scale.Intensity);
			Series::Unpack(Scale->Get("accuracy"), &O->Scale.Accuracy);
			Series::Unpack(Scale->Get("min"), &O->Scale.Min);
			Series::Unpack(Scale->Get("max"), &O->Scale.Max);

			Core::Schema* Velocity = V->Get("velocity");
			Series::Unpack(Velocity->Get("intensity"), &O->Velocity.Intensity);
			Series::Unpack(Velocity->Get("accuracy"), &O->Velocity.Accuracy);
			Series::Unpack(Velocity->Get("min"), &O->Velocity.Min);
			Series::Unpack(Velocity->Get("max"), &O->Velocity.Max);

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::SkinAnimatorKey* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("pose"), &O->Pose);
			Series::Unpack(V->Get("time"), &O->Time);

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::SkinAnimatorClip* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("name"), &O->Name);
			Series::Unpack(V->Get("duration"), &O->Duration);
			Series::Unpack(V->Get("rate"), &O->Rate);

			std::vector<Core::Schema*> Frames = V->FetchCollection("frames.frame", false);
			for (auto&& It : Frames)
			{
				O->Keys.emplace_back();
				Series::Unpack(It, &O->Keys.back());
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::KeyAnimatorClip* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("name"), &O->Name);
			Series::Unpack(V->Get("duration"), &O->Duration);
			Series::Unpack(V->Get("rate"), &O->Rate);
			Series::Unpack(V->Get("frames"), &O->Keys);
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::AnimatorKey* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("position"), &O->Position);
			Series::Unpack(V->Get("rotation"), &O->Rotation);
			Series::Unpack(V->Get("scale"), &O->Scale);
			Series::Unpack(V->Get("time"), &O->Time);
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::Joint* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Series::Unpack(V->Get("index"), &O->Index);
			Series::Unpack(V->Get("name"), &O->Name);
			Series::Unpack(V->Get("transform"), &O->Transform);
			Series::Unpack(V->Get("bind-shape"), &O->BindShape);

			std::vector<Core::Schema*> Joints = V->FetchCollection("childs.joint", false);
			for (auto& It : Joints)
			{
				O->Childs.emplace_back();
				Series::Unpack(It, &O->Childs.back());
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Compute::ElementVertex* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, Compute::Vertex* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, Compute::SkinVertex* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, Ticker* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			O->Delay = (float)V->GetVar("[delay]").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, std::string* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			*O = V->GetVar("[s]").GetBlob();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, std::vector<bool>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<int>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<unsigned int>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<float>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<double>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<int64_t>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<long double>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<uint64_t>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::Vector2>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::Vector3>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::Vector4>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::Matrix4x4>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<AnimatorState>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<SpawnerProperties>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::SkinAnimatorClip>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			std::vector<Core::Schema*> Frames = V->FetchCollection("clips.clip", false);
			for (auto&& It : Frames)
			{
				O->push_back(Compute::SkinAnimatorClip());
				Series::Unpack(It, &O->back());
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::KeyAnimatorClip>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			std::vector<Core::Schema*> Frames = V->FetchCollection("clips.clip", false);
			for (auto&& It : Frames)
			{
				O->push_back(Compute::KeyAnimatorClip());
				Series::Unpack(It, &O->back());
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::AnimatorKey>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::ElementVertex>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::Joint>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			O->reserve(V->Size());
			for (auto&& It : V->GetChilds())
			{
				O->push_back(Compute::Joint());
				Series::Unpack(It, &O->back());
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::Vertex>* O)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Compute::SkinVertex>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<Ticker>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
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
		bool Series::Unpack(Core::Schema* V, std::vector<std::string>* O)
		{
			ED_ASSERT(O != nullptr, false, "output should be set");
			if (!V)
				return false;

			Core::Schema* Array = V->Get("s-array");
			if (!Array)
				return false;

			for (auto&& It : Array->GetChilds())
			{
				if (It->Key == "s" && It->Value.GetType() == Core::VarType::String)
					O->push_back(It->Value.GetBlob());
			}

			return true;
		}

		Parallel::Task Parallel::Enqueue(const Core::TaskCallback& Callback)
		{
			ED_ASSERT(Callback != nullptr, Task(), "callback should be set");
			auto* Queue = Core::Schedule::Get();
			if (Queue->GetThreads(Core::Difficulty::Heavy) > 0)
			{
				std::promise<void> Promise;
				std::future<void> Future = Promise.get_future();
				bool IsQueued = Queue->SetTask([Context = InterpretAsMove(Promise), Callback]() mutable
				{
					Callback();
				    Context.value.set_value();
				});

				if (IsQueued)
					return Future;
			}

			Callback();
			return Task();
		}
		std::vector<Parallel::Task> Parallel::EnqueueAll(const std::vector<Core::TaskCallback>& Callbacks)
		{
			ED_ASSERT(!Callbacks.empty(), std::vector<Task>(), "callbacks should not be empty");
			std::vector<Task> Result;
			Result.reserve(Callbacks.size());

			for (auto& Callback : Callbacks)
				Result.emplace_back(Enqueue(Callback));

			return Result;
		}
		void Parallel::Wait(const Task& Value)
		{
			if (Value.valid())
				Value.wait();
		}
		void Parallel::WailAll(const std::vector<Task>& Values)
		{
			for (auto& Value : Values)
				Wait(Value);
		}

		Material::Material(SceneGraph* NewScene) noexcept : DiffuseMap(nullptr), NormalMap(nullptr), MetallicMap(nullptr), RoughnessMap(nullptr), HeightMap(nullptr), OcclusionMap(nullptr), EmissionMap(nullptr), Scene(NewScene), Slot(0)
		{
		}
		Material::Material(const Material& Other) noexcept : Material(Other.Scene)
		{
			memcpy(&Surface, &Other.Surface, sizeof(Subsurface));
			if (Other.DiffuseMap != nullptr)
			{
				DiffuseMap = Other.DiffuseMap;
				DiffuseMap->AddRef();
			}

			if (Other.NormalMap != nullptr)
			{
				NormalMap = Other.NormalMap;
				NormalMap->AddRef();
			}

			if (Other.MetallicMap != nullptr)
			{
				MetallicMap = Other.MetallicMap;
				MetallicMap->AddRef();
			}

			if (Other.RoughnessMap != nullptr)
			{
				RoughnessMap = Other.RoughnessMap;
				RoughnessMap->AddRef();
			}

			if (Other.HeightMap != nullptr)
			{
				HeightMap = Other.HeightMap;
				HeightMap->AddRef();
			}

			if (Other.OcclusionMap != nullptr)
			{
				OcclusionMap = Other.OcclusionMap;
				OcclusionMap->AddRef();
			}

			if (Other.EmissionMap != nullptr)
			{
				EmissionMap = Other.EmissionMap;
				EmissionMap->AddRef();
			}
		}
		Material::~Material() noexcept
		{
			ED_RELEASE(DiffuseMap);
			ED_RELEASE(NormalMap);
			ED_RELEASE(MetallicMap);
			ED_RELEASE(RoughnessMap);
			ED_RELEASE(HeightMap);
			ED_RELEASE(OcclusionMap);
			ED_RELEASE(EmissionMap);
		}
		void Material::SetName(const std::string& Value)
		{
			Name = Value;
			if (Scene != nullptr)
				Scene->Mutate(this, "set");
		}
		const std::string& Material::GetName() const
		{
			return Name;
		}
		void Material::SetDiffuseMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(DiffuseMap);
			DiffuseMap = New;
		}
		Graphics::Texture2D* Material::GetDiffuseMap() const
		{
			return DiffuseMap;
		}
		void Material::SetNormalMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(NormalMap);
			NormalMap = New;
		}
		Graphics::Texture2D* Material::GetNormalMap() const
		{
			return NormalMap;
		}
		void Material::SetMetallicMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(MetallicMap);
			MetallicMap = New;
		}
		Graphics::Texture2D* Material::GetMetallicMap() const
		{
			return MetallicMap;
		}
		void Material::SetRoughnessMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(RoughnessMap);
			RoughnessMap = New;
		}
		Graphics::Texture2D* Material::GetRoughnessMap() const
		{
			return RoughnessMap;
		}
		void Material::SetHeightMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(HeightMap);
			HeightMap = New;
		}
		Graphics::Texture2D* Material::GetHeightMap() const
		{
			return HeightMap;
		}
		void Material::SetOcclusionMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(OcclusionMap);
			OcclusionMap = New;
		}
		Graphics::Texture2D* Material::GetOcclusionMap() const
		{
			return OcclusionMap;
		}
		void Material::SetEmissionMap(Graphics::Texture2D* New)
		{
			ED_RELEASE(EmissionMap);
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

		Processor::Processor(ContentManager* NewContent) noexcept : Content(NewContent)
		{
		}
		Processor::~Processor() noexcept
		{
		}
		void Processor::Free(AssetCache* Asset)
		{
		}
		void* Processor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
		{
			return nullptr;
		}
		void* Processor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
		{
			return nullptr;
		}
		bool Processor::Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args)
		{
			return false;
		}
		ContentManager* Processor::GetContent() const
		{
			return Content;
		}

		Component::Component(Entity* Reference, ActorSet Rule) noexcept : Parent(Reference), Set((size_t)Rule), Indexed(false), Active(false)
		{
			ED_ASSERT_V(Reference != nullptr, "entity should be set");
		}
		Component::~Component() noexcept
		{
		}
		void Component::Deserialize(Core::Schema* Node)
		{
		}
		void Component::Serialize(Core::Schema* Node)
		{
		}
		void Component::Activate(Component* New)
		{
		}
		void Component::Deactivate()
		{
		}
		void Component::Animate(Core::Timer* Time)
		{
		}
		void Component::Synchronize(Core::Timer* Time)
		{
		}
		void Component::Update(Core::Timer* Time)
		{
		}
		void Component::Message(const std::string& Name, Core::VariantArgs& Args)
		{
		}
		void Component::Movement()
		{
		}
		float Component::GetVisibility(const Viewer& View, float Distance) const
		{
			float Visibility = 1.0f - Distance / View.FarPlane;
			if (Visibility <= 0.0f)
				return 0.0f;

			const Compute::Matrix4x4& Box = Parent->GetBox();
			return Compute::Geometric::IsCubeInFrustum(Box * View.ViewProjection, 1.65f) ? Visibility : 0.0f;
		}
		size_t Component::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
		{
			Min = -1.0f;
			Max = 1.0f;
			return 0;
		}
		void Component::SetActive(bool Status)
		{
			auto* Scene = Parent->GetScene();
			if (Active == Status)
				return;

			Active = Status;
			if (Parent->IsActive())
			{
				if (Active)
					Scene->RegisterComponent(this, false);
				else
					Scene->UnregisterComponent(this);
			}

			Scene->NotifyCosmos(this);
		}
		bool Component::IsDrawable() const
		{
			return Set & (uint64_t)ActorSet::Drawable;
		}
		bool Component::IsCullable() const
		{
			return Set & (uint64_t)ActorSet::Cullable;
		}
		bool Component::IsActive() const
		{
			return Active;
		}
		Entity* Component::GetEntity() const
		{
			return Parent;
		}

		Entity::Entity(SceneGraph* NewScene) noexcept : Transform(new Compute::Transform(this)), Scene(NewScene), Active(false)
		{
			ED_ASSERT_V(Scene != nullptr, "entity should be created within a scene");
		}
		Entity::~Entity() noexcept
		{
			for (auto& Component : Type.Components)
			{
				if (Component.second != nullptr)
				{
					Component.second->SetActive(false);
					Scene->UnloadComponentAll(Component.second);
					Scene->ClearCosmos(Component.second);
					ED_RELEASE(Component.second);
				}
			}

			ED_RELEASE(Transform);
		}
		void Entity::SetName(const std::string& Value)
		{
			Type.Name = Value;
			Scene->Mutate(this, "set");
		}
		void Entity::SetRoot(Entity* Parent)
		{
			auto* Old = Transform->GetRoot();
			if (!Parent)
			{
				Transform->SetRoot(nullptr);
				if (Old != nullptr && Scene != nullptr)
					Scene->Mutate((Entity*)Old->UserData, this, "pop");
			}
			else
			{
				Transform->SetRoot(Parent->Transform);
				if (Old != Parent->Transform && Scene != nullptr)
					Scene->Mutate(Parent, this, "push");
			}
		}
		void Entity::UpdateBounds()
		{
			size_t Index = 0;
			for (auto& Item : Type.Components)
			{
				Compute::Vector3 Min, Max;
				size_t Offset = Item.second->GetUnitBounds(Min, Max);
				Item.second->Movement();

				if (Offset > Index)
				{
					Index = Offset;
					Snapshot.Min = Min;
					Snapshot.Max = Max;
				}
			}

			if (!Index)
			{
				Snapshot.Min = -0.5f;
				Snapshot.Max = 0.5f;
			}

			Transform->GetBounds(Snapshot.Box, Snapshot.Min, Snapshot.Max);
		}
		void Entity::RemoveComponent(uint64_t Id)
		{
			std::unordered_map<uint64_t, Component*>::iterator It = Type.Components.find(Id);
			if (It == Type.Components.end())
				return;

			Component* Base = It->second;
			Base->SetActive(false);
			Transform->MakeDirty();
			Type.Components.erase(It);
			if (Scene->Camera == Base)
				Scene->Camera = nullptr;

			auto* Top = Scene;
			Scene->Transaction([Top, Base]()
			{
				Top->ClearCosmos(Base);
				ED_RELEASE(Base);
			});
		}
		void Entity::RemoveChilds()
		{
			std::vector<Compute::Transform*>& Childs = Transform->GetChilds();
			for (size_t i = 0; i < Childs.size(); i++)
			{
				Entity* Next = (Entity*)Transform->GetChild(i)->UserData;
				if (Next != nullptr && Next != this)
				{
					Scene->DeleteEntity(Next);
					i--;
				}
			}
		}
		Component* Entity::AddComponent(Component* In)
		{
			ED_ASSERT(In != nullptr, nullptr, "component should be set");
			if (In == GetComponent(In->GetId()))
				return In;

			RemoveComponent(In->GetId());
			In->Active = false;
			In->Parent = this;

			Type.Components.insert({ In->GetId(), In });
			Scene->Transaction([this, In]()
			{
				for (auto& Component : Type.Components)
					Component.second->Activate(In == Component.second ? nullptr : In);
			});

			In->SetActive(true);
			Transform->MakeDirty();

			return In;
		}
		Component* Entity::GetComponent(uint64_t Id)
		{
			std::unordered_map<uint64_t, Component*>::iterator It = Type.Components.find(Id);
			if (It != Type.Components.end())
				return It->second;

			return nullptr;
		}
		size_t Entity::GetComponentsCount() const
		{
			return Type.Components.size();
		}
		SceneGraph* Entity::GetScene() const
		{
			return Scene;
		}
		Entity* Entity::GetParent() const
		{
			auto* Root = Transform->GetRoot();
			if (!Root)
				return nullptr;

			return (Entity*)Root->UserData;
		}
		Entity* Entity::GetChild(size_t Index) const
		{
			auto* Child = Transform->GetChild(Index);
			if (!Child)
				return nullptr;

			return (Entity*)Child->UserData;
		}
		Compute::Transform* Entity::GetTransform() const
		{
			return Transform;
		}
		const std::string& Entity::GetName() const
		{
			return Type.Name;
		}
		std::string Entity::GetSystemName() const
		{
			std::string Result;
			for (auto& Item : Type.Components)
			{
				Result.append(Item.second->GetName());
				Result.append(", ");
			}

			if (!Result.empty())
				Result = "[" + Result.substr(0, Result.size() - 2) + "] " + Type.Name;
			else
				Result = Type.Name;

			return Result;
		}
		float Entity::GetVisibility(const Viewer& Base) const
		{
			float Distance = Transform->GetPosition().Distance(Base.Position);
			return 1.0f - Distance / Base.FarPlane;
		}
		const Compute::Matrix4x4& Entity::GetBox() const
		{
			return Snapshot.Box;
		}
		const Compute::Vector3& Entity::GetMin() const
		{
			return Snapshot.Min;
		}
		const Compute::Vector3& Entity::GetMax() const
		{
			return Snapshot.Max;
		}
		size_t Entity::GetChildsCount() const
		{
			return Transform->GetChildsCount();
		}
		bool Entity::IsActive() const
		{
			return Active;
		}
		Compute::Vector3 Entity::GetRadius3() const
		{
			Compute::Vector3 Diameter3 = Snapshot.Max - Snapshot.Min;
			return Diameter3.Abs().Mul(0.5f);
		}
		float Entity::GetRadius() const
		{
			const Compute::Vector3& Radius = GetRadius3();
			float Max = (Radius.X > Radius.Y ? Radius.X : Radius.Y);
			return (Max > Radius.Z ? Radius.Z : Max);
		}

		Drawable::Drawable(Entity* Ref, ActorSet Rule, uint64_t Hash, bool vComplex) noexcept : Component(Ref, Rule | ActorSet::Cullable | ActorSet::Drawable | ActorSet::Message), Category(GeoCategory::Opaque), Source(Hash), Complex(vComplex), Overlapping(1.0f), Static(true)
		{
			if (!Complex)
				Materials[nullptr] = nullptr;
		}
		Drawable::~Drawable() noexcept
		{
		}
		void Drawable::Message(const std::string& Name, Core::VariantArgs& Args)
		{
			if (Name != "mutation")
				return;

			Material* Target = (Material*)Args["material"].GetPointer();
			if (!Target || !Args["type"].IsString("push"))
				return;

			for (auto&& Surface : Materials)
			{
				if (Surface.second == Target)
					Surface.second = nullptr;
			}
		}
		void Drawable::Movement()
		{
			Overlapping = 1.0f;
		}
		bool Drawable::SetCategory(GeoCategory NewCategory)
		{
			Category = NewCategory;
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
		bool Drawable::SetMaterialAll(Material* Value)
		{
			if (!Complex)
			{
				Materials[nullptr] = Value;
				return true;
			}

			for (auto& Item : Materials)
				Item.second = Value;

			return true;
		}
		GeoCategory Drawable::GetCategory() const
		{
			return Category;
		}
		int64_t Drawable::GetSlot(void* Surface)
		{
			Material* Base = GetMaterial(Surface);
			if (Base != nullptr)
				return (int64_t)Base->Slot;

			return -1;
		}
		int64_t Drawable::GetSlot()
		{
			Material* Base = GetMaterial();
			if (Base != nullptr)
				return (int64_t)Base->Slot;

			return -1;
		}
		Material* Drawable::GetMaterial(void* Instance)
		{
			if (!Complex || Materials.empty())
				return nullptr;

			if (Materials.size() == 1)
				return Materials.begin()->second;

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

		Renderer::Renderer(RenderSystem* Lab) noexcept : System(Lab), Active(true)
		{
			ED_ASSERT_V(Lab != nullptr, "render system should be set");
		}
		Renderer::~Renderer() noexcept
		{
		}
		void Renderer::SetRenderer(RenderSystem* NewSystem)
		{
			ED_ASSERT_V(NewSystem != nullptr, "render system should be set");
			System = NewSystem;
		}
		void Renderer::Deserialize(Core::Schema* Node)
		{
		}
		void Renderer::Serialize(Core::Schema* Node)
		{
		}
		void Renderer::ClearCulling()
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
		void Renderer::BeginPass()
		{
		}
		void Renderer::EndPass()
		{
		}
		bool Renderer::HasCategory(GeoCategory Category)
		{
			return false;
		}
		size_t Renderer::RenderPass(Core::Timer* TimeStep)
		{
			return 0;
		}
		RenderSystem* Renderer::GetRenderer() const
		{
			return System;
		}

		RenderSystem::RenderSystem(SceneGraph* NewScene, Component* NewComponent) noexcept : Device(nullptr), BaseMaterial(nullptr), Scene(NewScene), Owner(NewComponent), MaxQueries(16384), OcclusionSkips(2), OccluderSkips(8), OccludeeSkips(3), OverflowVisibility(0.0f), Threshold(0.1f), OcclusionCulling(false), PreciseCulling(true)
		{
			ED_ASSERT_V(NewScene != nullptr, "scene should be set");
			ED_ASSERT_V(NewScene->GetDevice() != nullptr, "graphics device should be set");	
			Device = NewScene->GetDevice();
		}
		RenderSystem::~RenderSystem() noexcept
		{
			for (auto& Next : Renderers)
			{
				if (!Next)
					continue;

				Next->Deactivate();
				ED_RELEASE(Next);
			}
		}
		void RenderSystem::SetView(const Compute::Matrix4x4& _View, const Compute::Matrix4x4& _Projection, const Compute::Vector3& _Position, float _Fov, float _Ratio, float _Near, float _Far, RenderCulling _Type)
		{
			View.Set(_View, _Projection, _Position, _Fov, _Ratio, _Near, _Far, _Type);
			RestoreViewBuffer(&View);
		}
		void RenderSystem::ClearCulling()
		{
			for (auto& Next : Renderers)
				Next->ClearCulling();
			Scene->ClearCulling();
		}
		void RenderSystem::RestoreViewBuffer(Viewer* Buffer)
		{
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			if (&View != Buffer)
			{
				if (Buffer == nullptr)
				{
					auto* Viewer = (Components::Camera*)Scene->Camera.load();
					if (Viewer != nullptr)
						Viewer->GetViewer(&View);
				}
				else
					View = *Buffer;
			}

			if (View.Culling == RenderCulling::Linear)
				Indexing.Frustum = Compute::Frustum6P(View.ViewProjection);
			else if (View.Culling == RenderCulling::Cubic)
				Indexing.Bounds = Compute::Bounding(View.Position - View.FarPlane, View.Position + View.FarPlane);

			Device->View.InvViewProj = View.InvViewProjection;
			Device->View.ViewProj = View.ViewProjection;
			Device->View.Proj = View.Projection;
			Device->View.View = View.View;
			Device->View.Position = View.Position;
			Device->View.Direction = View.Rotation.dDirection();
			Device->View.Far = View.FarPlane;
			Device->View.Near = View.NearPlane;
			Device->UpdateBuffer(Graphics::RenderBufferType::View);
		}
		void RenderSystem::Remount(Renderer* Target)
		{
			ED_ASSERT_V(Target != nullptr, "renderer should be set");
			Target->Deactivate();
			Target->SetRenderer(this);
			Target->Activate();
			Target->ResizeBuffers();
		}
		void RenderSystem::Remount()
		{
			ClearCulling();
			for (auto& Next : Renderers)
				Remount(Next);
		}
		void RenderSystem::Mount()
		{
			ClearCulling();
			for (auto& Next : Renderers)
				Next->Activate();
		}
		void RenderSystem::Unmount()
		{
			for (auto& Next : Renderers)
				Next->Deactivate();
		}
		void RenderSystem::MoveRenderer(uint64_t Id, size_t Offset)
		{
			if (Offset == 0)
				return;

			for (size_t i = 0; i < Renderers.size(); i++)
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
					ED_RELEASE(*It);
					Renderers.erase(It);
					break;
				}
			}
		}
		void RenderSystem::RestoreOutput()
		{
			Scene->SetMRT(TargetType::Main, false);
		}
		void RenderSystem::FreeShader(const std::string& Name, Graphics::Shader* Shader)
		{
			ShaderCache* Cache = Scene->GetShaders();
			if (Cache != nullptr)
			{
				if (Cache->Has(Name))
					return;
			}

			ED_RELEASE(Shader);
		}
		void RenderSystem::FreeShader(Graphics::Shader* Shader)
		{
			ShaderCache* Cache = Scene->GetShaders();
			if (Cache != nullptr)
				return FreeShader(Cache->Find(Shader), Shader);

			ED_RELEASE(Shader);
		}
		void RenderSystem::FreeBuffers(const std::string& Name, Graphics::ElementBuffer** Buffers)
		{
			if (!Buffers)
				return;

			PrimitiveCache* Cache = Scene->GetPrimitives();
			if (Cache != nullptr)
			{
				if (Cache->Has(Name))
					return;
			}

			ED_RELEASE(Buffers[0]);
			ED_RELEASE(Buffers[1]);
		}
		void RenderSystem::FreeBuffers(Graphics::ElementBuffer** Buffers)
		{
			if (!Buffers)
				return;

			PrimitiveCache* Cache = Scene->GetPrimitives();
			if (Cache != nullptr)
				return FreeBuffers(Cache->Find(Buffers), Buffers);

			ED_RELEASE(Buffers[0]);
			ED_RELEASE(Buffers[1]);
		}
		void RenderSystem::ClearMaterials()
		{
			BaseMaterial = nullptr;
		}
		void RenderSystem::FetchVisibility(Component* Base, VisibilityQuery& Data)
		{
			auto& Snapshot = Base->Parent->Snapshot;
			Snapshot.Distance = Base->Parent->Transform->GetPosition().Distance(View.Position);
			Snapshot.Visibility = std::max<float>(0.0f, 1.0f - Snapshot.Distance / (View.FarPlane + Base->Parent->GetRadius()));
			if (OcclusionCulling && Snapshot.Visibility >= Threshold && State.IsTop() && Base->IsDrawable())
			{
				auto* Varying = (Drawable*)Base;
				Snapshot.Visibility = Varying->Overlapping;
				Data.Category = Varying->GetCategory();
				Data.QueryPixels = (Data.Category == GeoCategory::Opaque);
			}
			else
			{
				Data.Category = GeoCategory::Opaque;
				Data.QueryPixels = false;
			}
			Data.BoundaryVisible = Snapshot.Visibility >= Threshold;
		}
		size_t RenderSystem::Render(Core::Timer* Time, RenderState Stage, RenderOpt Options)
		{
			ED_ASSERT(Time != nullptr, 0, "timer should be set");

			RenderOpt LastOptions = State.Options;
			RenderState LastTarget = State.Target;
			size_t Count = 0;

			State.Top++;
			State.Options = Options;
			State.Target = Stage;

			for (auto& Next : Renderers)
			{
				if (Next->Active)
					Next->BeginPass();
			}

			for (auto& Next : Renderers)
			{
				if (Next->Active)
					Count += Next->RenderPass(Time);
			}

			for (auto& Next : Renderers)
			{
				if (Next->Active)
					Next->EndPass();
			}

			State.Target = LastTarget;
			State.Options = LastOptions;
			State.Top--;

			return Count;
		}
		bool RenderSystem::TryInstance(Material* Next, Graphics::RenderBuffer::Instance& Target)
		{
			if (!Next)
				return false;

			Target.Diffuse = (float)(Next->DiffuseMap != nullptr);
			Target.Normal = (float)(Next->NormalMap != nullptr);
			Target.Height = (float)(Next->HeightMap != nullptr);
			Target.MaterialId = (float)Next->Slot;

			return true;
		}
		bool RenderSystem::TryGeometry(Material* Next, bool WithTextures)
		{
			if (!Next)
				return false;

			if (Next == BaseMaterial)
				return true;

			BaseMaterial = Next;
			Device->Render.Diffuse = (float)(Next->DiffuseMap != nullptr);
			Device->Render.Normal = (float)(Next->NormalMap != nullptr);
			Device->Render.Height = (float)(Next->HeightMap != nullptr);
			Device->Render.MaterialId = (float)Next->Slot;

			if (WithTextures)
			{
				if (Next->DiffuseMap != nullptr)
					Device->SetTexture2D(Next->DiffuseMap, 1, ED_PS);

				if (Next->NormalMap != nullptr)
					Device->SetTexture2D(Next->NormalMap, 2, ED_PS);

				if (Next->MetallicMap != nullptr)
					Device->SetTexture2D(Next->MetallicMap, 3, ED_PS);

				if (Next->RoughnessMap != nullptr)
					Device->SetTexture2D(Next->RoughnessMap, 4, ED_PS);

				if (Next->HeightMap != nullptr)
					Device->SetTexture2D(Next->HeightMap, 5, ED_PS);

				if (Next->OcclusionMap != nullptr)
					Device->SetTexture2D(Next->OcclusionMap, 6, ED_PS);

				if (Next->EmissionMap != nullptr)
					Device->SetTexture2D(Next->EmissionMap, 7, ED_PS);
			}

			return true;
		}
		bool RenderSystem::HasCategory(GeoCategory Category)
		{
			for (auto* Next : Renderers)
			{
				if (Next->Active && Next->HasCategory(Category))
					return true;
			}

			return false;
		}
		bool RenderSystem::CompileBuffers(Graphics::ElementBuffer** Result, const std::string& Name, size_t ElementSize, size_t ElementsCount)
		{
			ED_ASSERT(Result != nullptr, false, "result should be set");
			ED_ASSERT(!Name.empty(), false, "buffers must have a name");

			PrimitiveCache* Cache = Scene->GetPrimitives();
			if (Cache != nullptr)
				return Cache->Compile(Result, Name, ElementSize, ElementsCount);

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
			F.ElementWidth = (unsigned int)ElementSize;
			F.ElementCount = (unsigned int)ElementsCount;

			Graphics::ElementBuffer* VertexBuffer = Device->CreateElementBuffer(F);
			if (!VertexBuffer)
				return false;

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Index_Buffer;
			F.ElementWidth = (unsigned int)sizeof(int);
			F.ElementCount = (unsigned int)ElementsCount * 3;

			Graphics::ElementBuffer* IndexBuffer = Device->CreateElementBuffer(F);
			if (!IndexBuffer)
			{
				ED_RELEASE(VertexBuffer);
				return false;
			}

			Result[(size_t)BufferType::Index] = IndexBuffer;
			Result[(size_t)BufferType::Vertex] = VertexBuffer;

			return true;
		}
		Graphics::Shader* RenderSystem::CompileShader(Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			ED_ASSERT(!Desc.Filename.empty(), nullptr, "shader must have a name");
			ShaderCache* Cache = Scene->GetShaders();
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
		Renderer* RenderSystem::AddRenderer(Renderer* In)
		{
			ED_ASSERT(In != nullptr, nullptr, "renderer should be set");
			for (auto It = Renderers.begin(); It != Renderers.end(); ++It)
			{
				if (*It && (*It)->GetId() == In->GetId())
				{
					if (*It == In)
						return In;

					(*It)->Deactivate();
					ED_RELEASE(*It);
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
			for (auto& Next : Renderers)
			{
				if (Next->GetId() == Id)
					return Next;
			}

			return nullptr;
		}
		int64_t RenderSystem::GetOffset(uint64_t Id) const
		{
			for (size_t i = 0; i < Renderers.size(); i++)
			{
				if (Renderers[i]->GetId() == Id)
					return (int64_t)i;
			}

			return -1;
		}
		std::vector<Renderer*>& RenderSystem::GetRenderers()
		{
			return Renderers;
		}
		Graphics::MultiRenderTarget2D* RenderSystem::GetMRT(TargetType Type) const
		{
			return Scene->GetMRT(Type);
		}
		Graphics::RenderTarget2D* RenderSystem::GetRT(TargetType Type) const
		{
			return Scene->GetRT(Type);
		}
		Graphics::GraphicsDevice* RenderSystem::GetDevice() const
		{
			return Device;
		}
		Graphics::Texture2D** RenderSystem::GetMerger()
		{
			return Scene->GetMerger();
		}
		PrimitiveCache* RenderSystem::GetPrimitives() const
		{
			return Scene->GetPrimitives();
		}
		SceneGraph* RenderSystem::GetScene() const
		{
			return Scene;
		}
		Component* RenderSystem::GetComponent() const
		{
			return Owner;
		}
		SparseIndex& RenderSystem::GetStorageWrapper(uint64_t Section)
		{
			return Scene->GetStorage(Section);
		}

		ShaderCache::ShaderCache(Graphics::GraphicsDevice* NewDevice) noexcept : Device(NewDevice)
		{
		}
		ShaderCache::~ShaderCache() noexcept
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
				ED_RELEASE(Shader);
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
			ED_ASSERT(Shader != nullptr, std::string(), "shader should be set");
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

			ED_RELEASE(It->second.Shader);
			Cache.erase(It);
			Safe.unlock();

			return true;
		}
		void ShaderCache::ClearCache()
		{
			Safe.lock();
			for (auto It = Cache.begin(); It != Cache.end(); ++It)
				ED_CLEAR(It->second.Shader);

			Cache.clear();
			Safe.unlock();
		}

		PrimitiveCache::PrimitiveCache(Graphics::GraphicsDevice* Ref) noexcept : Device(Ref), Quad(nullptr)
		{
			Sphere[0] = Sphere[1] = nullptr;
			Cube[0] = Cube[1] = nullptr;
			Box[0] = Box[1] = nullptr;
			SkinBox[0] = SkinBox[1] = nullptr;
		}
		PrimitiveCache::~PrimitiveCache() noexcept
		{
			ED_RELEASE(Sphere[(size_t)BufferType::Index]);
			ED_RELEASE(Sphere[(size_t)BufferType::Vertex]);
			ED_RELEASE(Cube[(size_t)BufferType::Index]);
			ED_RELEASE(Cube[(size_t)BufferType::Vertex]);
			ED_RELEASE(Box[(size_t)BufferType::Index]);
			ED_RELEASE(Box[(size_t)BufferType::Vertex]);
			ED_RELEASE(SkinBox[(size_t)BufferType::Index]);
			ED_RELEASE(SkinBox[(size_t)BufferType::Vertex]);
			ED_RELEASE(Quad);
			ClearCache();
		}
		bool PrimitiveCache::Compile(Graphics::ElementBuffer** Results, const std::string& Name, size_t ElementSize, size_t ElementsCount)
		{
			ED_ASSERT(Results != nullptr, false, "results should be set");
			if (Get(Results, Name))
				return false;

			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
			F.ElementWidth = (unsigned int)ElementSize;
			F.ElementCount = (unsigned int)ElementsCount;

			Graphics::ElementBuffer* VertexBuffer = Device->CreateElementBuffer(F);
			if (!VertexBuffer)
				return false;

			F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Index_Buffer;
			F.ElementWidth = (unsigned int)sizeof(int);
			F.ElementCount = (unsigned int)ElementsCount * 3;

			Graphics::ElementBuffer* IndexBuffer = Device->CreateElementBuffer(F);
			if (!IndexBuffer)
			{
				ED_RELEASE(VertexBuffer);
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
			ED_ASSERT(Results != nullptr, false, "results should be set");
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

			ED_RELEASE(It->second.Buffers[0]);
			ED_RELEASE(It->second.Buffers[1]);
			Cache.erase(It);
			Safe.unlock();

			return true;
		}
		std::string PrimitiveCache::Find(Graphics::ElementBuffer** Buffers)
		{
			ED_ASSERT(Buffers != nullptr, std::string(), "buffers should be set");
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
			ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");
			if (Quad != nullptr)
				return Quad;

			std::vector<Compute::ShapeVertex> Elements =
			{
				{ -1.0f, -1.0f, 0, -1, 0 },
				{ -1.0f, 1.0f, 0, -1, -1 },
				{ 1.0f, 1.0f, 0, 0, -1 },
				{ -1.0f, -1.0f, 0, -1, 0 },
				{ 1.0f, 1.0f, 0, 0, -1 },
				{ 1.0f, -1.0f, 0, 0, 0 }
			};
			Compute::Geometric::TexCoordRhToLh(Elements);

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
			ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");
			if (Sphere[(size_t)Type] != nullptr)
				return Sphere[(size_t)Type];

			if (Type == BufferType::Index)
			{
				std::vector<int> Indices =
				{
					0, 4, 1, 0,
					9, 4, 9, 5,
					4, 4, 5, 8,
					4, 8, 1, 8,
					10, 1, 8, 3,
					10, 5, 3, 8,
					5, 2, 3, 2,
					7, 3, 7, 10,
					3, 7, 6, 10,
					7, 11, 6, 11,
					0, 6, 0, 1,
					6, 6, 1, 10,
					9, 0, 11, 9,
					11, 2, 9, 2,
					5, 7, 2, 11
				};

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
				const float X = 0.525731112119133606f;
				const float Z = 0.850650808352039932f;
				const float N = 0.0f;

				std::vector<Compute::ShapeVertex> Elements =
				{
					{ -X, N, Z },
					{ X, N, Z },
					{ -X, N, -Z },
					{ X, N, -Z },
					{ N, Z, X },
					{ N, Z, -X },
					{ N, -Z, X },
					{ N, -Z, -X },
					{ Z, X, N },
					{ -Z, X, N },
					{ Z, -X, N },
					{ -Z, -X, N }
				};

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
			ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");
			if (Cube[(size_t)Type] != nullptr)
				return Cube[(size_t)Type];

			if (Type == BufferType::Index)
			{
				std::vector<int> Indices =
				{
					0, 1, 2, 0,
					18, 1, 3, 4,
					5, 3, 19, 4,
					6, 7, 8, 6,
					20, 7, 9, 10,
					11, 9, 21, 10,
					12, 13, 14, 12,
					22, 13, 15, 16,
					17, 15, 23, 16
				};

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
				std::vector<Compute::ShapeVertex> Elements
				{
					{ -1, 1, 1, 0.875, -0.5 },
					{ 1, -1, 1, 0.625, -0.75 },
					{ 1, 1, 1, 0.625, -0.5 },
					{ 1, -1, 1, 0.625, -0.75 },
					{ -1, -1, -1, 0.375, -1 },
					{ 1, -1, -1, 0.375, -0.75 },
					{ -1, -1, 1, 0.625, -0 },
					{ -1, 1, -1, 0.375, -0.25 },
					{ -1, -1, -1, 0.375, -0 },
					{ 1, 1, -1, 0.375, -0.5 },
					{ -1, -1, -1, 0.125, -0.75 },
					{ -1, 1, -1, 0.125, -0.5 },
					{ 1, 1, 1, 0.625, -0.5 },
					{ 1, -1, -1, 0.375, -0.75 },
					{ 1, 1, -1, 0.375, -0.5 },
					{ -1, 1, 1, 0.625, -0.25 },
					{ 1, 1, -1, 0.375, -0.5 },
					{ -1, 1, -1, 0.375, -0.25 },
					{ -1, -1, 1, 0.875, -0.75 },
					{ -1, -1, 1, 0.625, -1 },
					{ -1, 1, 1, 0.625, -0.25 },
					{ 1, -1, -1, 0.375, -0.75 },
					{ 1, -1, 1, 0.625, -0.75 },
					{ 1, 1, 1, 0.625, -0.5 }
				};
				Compute::Geometric::TexCoordRhToLh(Elements);

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
			ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");
			if (Box[(size_t)Type] != nullptr)
				return Box[(size_t)Type];

			if (Type == BufferType::Index)
			{
				std::vector<int> Indices =
				{
					16, 23, 15, 17,
					16, 15, 13, 22,
					12, 14, 13, 12,
					10, 21, 9, 11,
					10, 9, 7, 20,
					6, 8, 7, 6,
					4, 19, 3, 5,
					4, 3, 1, 18,
					0, 2, 1, 0
				};

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
				std::vector<Compute::Vertex> Elements =
				{
					{ -1, 1, 1, 0.875, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0 },
					{ 1, -1, 1, 0.625, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0 },
					{ 1, 1, 1, 0.625, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0 },
					{ 1, -1, 1, 0.625, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0 },
					{ -1, -1, -1, 0.375, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0 },
					{ 1, -1, -1, 0.375, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0 },
					{ -1, -1, 1, 0.625, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0 },
					{ -1, 1, -1, 0.375, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0 },
					{ -1, -1, -1, 0.375, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0 },
					{ 1, 1, -1, 0.375, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0 },
					{ -1, -1, -1, 0.125, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0 },
					{ -1, 1, -1, 0.125, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0 },
					{ 1, 1, 1, 0.625, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0 },
					{ 1, -1, -1, 0.375, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0 },
					{ 1, 1, -1, 0.375, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0 },
					{ -1, 1, 1, 0.625, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0 },
					{ 1, 1, -1, 0.375, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0 },
					{ -1, 1, -1, 0.375, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0 },
					{ -1, -1, 1, 0.875, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0 },
					{ -1, -1, 1, 0.625, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0 },
					{ -1, 1, 1, 0.625, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0 },
					{ 1, -1, -1, 0.375, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0 },
					{ 1, -1, 1, 0.625, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0 },
					{ 1, 1, 1, 0.625, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0 }
				};
				Compute::Geometric::TexCoordRhToLh(Elements);

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
			ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");
			if (SkinBox[(size_t)Type] != nullptr)
				return SkinBox[(size_t)Type];

			if (Type == BufferType::Index)
			{
				std::vector<int> Indices =
				{
					16, 23, 15, 17,
					16, 15, 13, 22,
					12, 14, 13, 12,
					10, 21, 9, 11,
					10, 9, 7, 20,
					6, 8, 7, 6,
					4, 19, 3, 5,
					4, 3, 1, 18,
					0, 2, 1, 0
				};

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
				std::vector<Compute::SkinVertex> Elements =
				{
					{ -1, 1, 1, 0.875, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, -1, 1, 0.625, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, 1, 1, 0.625, -0.5, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, -1, 1, 0.625, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, -1, -1, 0.375, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, -1, -1, 0.375, -0.75, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, -1, 1, 0.625, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, 1, -1, 0.375, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, -1, -1, 0.375, -0, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, 1, -1, 0.375, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, -1, -1, 0.125, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, 1, -1, 0.125, -0.5, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, 1, 1, 0.625, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, -1, -1, 0.375, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, 1, -1, 0.375, -0.5, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, 1, 1, 0.625, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, 1, -1, 0.375, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, 1, -1, 0.375, -0.25, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, -1, 1, 0.875, -0.75, 0, 0, 1, -1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, -1, 1, 0.625, -1, 0, -1, 0, 0, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ -1, 1, 1, 0.625, -0.25, -1, 0, 0, 0, 0, 1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, -1, -1, 0.375, -0.75, 0, 0, -1, 1, 0, 0, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, -1, 1, 0.625, -0.75, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, -1, 0, 0, 0, 0 },
					{ 1, 1, 1, 0.625, -0.5, 0, 1, 0, 0, 0, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 }
				};
				Compute::Geometric::TexCoordRhToLh(Elements);

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
			ED_ASSERT_V(Result != nullptr, "result should be set");
			Result[(size_t)BufferType::Index] = GetSphere(BufferType::Index);
			Result[(size_t)BufferType::Vertex] = GetSphere(BufferType::Vertex);
		}
		void PrimitiveCache::GetCubeBuffers(Graphics::ElementBuffer** Result)
		{
			ED_ASSERT_V(Result != nullptr, "result should be set");
			Result[(size_t)BufferType::Index] = GetCube(BufferType::Index);
			Result[(size_t)BufferType::Vertex] = GetCube(BufferType::Vertex);
		}
		void PrimitiveCache::GetBoxBuffers(Graphics::ElementBuffer** Result)
		{
			ED_ASSERT_V(Result != nullptr, "result should be set");
			Result[(size_t)BufferType::Index] = GetBox(BufferType::Index);
			Result[(size_t)BufferType::Vertex] = GetBox(BufferType::Vertex);
		}
		void PrimitiveCache::GetSkinBoxBuffers(Graphics::ElementBuffer** Result)
		{
			ED_ASSERT_V(Result != nullptr, "result should be set");
			Result[(size_t)BufferType::Index] = GetSkinBox(BufferType::Index);
			Result[(size_t)BufferType::Vertex] = GetSkinBox(BufferType::Vertex);
		}
		void PrimitiveCache::ClearCache()
		{
			Safe.lock();
			for (auto It = Cache.begin(); It != Cache.end(); ++It)
			{
				ED_CLEAR(It->second.Buffers[0]);
				ED_CLEAR(It->second.Buffers[1]);
			}

			Cache.clear();
			ED_CLEAR(Sphere[0]);
			ED_CLEAR(Sphere[1]);
			ED_CLEAR(Cube[0]);
			ED_CLEAR(Cube[1]);
			ED_CLEAR(Box[0]);
			ED_CLEAR(Box[1]);
			ED_CLEAR(SkinBox[0]);
			ED_CLEAR(SkinBox[1]);
			ED_CLEAR(Quad);
			Safe.unlock();
		}

		template <typename T>
		static void UpgradeBuffer(Core::Pool<T>& Storage, float Grow)
		{
			double Size = (double)Storage.Capacity();
			Size *= 1.0 + Grow;

			Storage.Reserve((size_t)Size);
		}

		SceneGraph::Desc SceneGraph::Desc::Get(Application* Base)
		{
			SceneGraph::Desc I;
			if (!Base)
				return I;

			I.Shared.Parallel = Base->Control.Parallel;
			I.Shared.Shaders = Base->Cache.Shaders;
			I.Shared.Primitives = Base->Cache.Primitives;
			I.Shared.Content = Base->Content;
			I.Shared.Device = Base->Renderer;
			I.Shared.Activity = Base->Activity;
			I.Shared.Manager = Base->VM;
			return I;
		}

		SceneGraph::SceneGraph(const Desc& I) noexcept : Simulator(new Compute::Simulator(I.Simulator)), Camera(nullptr), Active(true), Conf(I), Snapshot(nullptr)
		{
			for (size_t i = 0; i < (size_t)TargetType::Count * 2; i++)
			{
				Display.MRT[i] = nullptr;
				Display.RT[i] = nullptr;
			}

			auto Components = Core::Composer::Fetch((uint64_t)ComposerTag::Component);
			for (uint64_t Section : Components)
			{
				Registry[Section] = ED_NEW(SparseIndex);
				Changes[Section].clear();
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

			Configure(I);
			ScriptHook();
		}
		SceneGraph::~SceneGraph() noexcept
		{
			ED_MEASURE(ED_TIMING_MAX);
			StepTransactions();

			auto Source = std::move(Listeners);
			for (auto& Item : Source)
			{
				for (auto* Listener : Item.second)
					ED_DELETE(function, Listener);
			}

			auto Begin1 = Entities.Begin(), End1 = Entities.End();
			for (auto It = Begin1; It != End1; ++It)
				ED_RELEASE(*It);

			auto Begin2 = Materials.Begin(), End2 = Materials.End();
			for (auto It = Begin2; It != End2; ++It)
				ED_RELEASE(*It);

			for (auto& Sparse : Registry)
				ED_DELETE(SparseIndex, Sparse.second);

			ED_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Diffuse]);
			ED_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Normal]);
			ED_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Surface]);
			ED_RELEASE(Display.Merger);

			for (auto& Item : Display.Voxels)
				ED_RELEASE(Item.first);

			for (auto* Item : Display.Points)
				ED_RELEASE(Item);

			for (auto* Item : Display.Spots)
				ED_RELEASE(Item);

			for (auto& Item : Display.Lines)
			{
				if (Item != nullptr)
				{
					for (auto* Target : *Item)
						ED_RELEASE(Target);
					ED_DELETE(vector, Item);
				}
			}

			for (size_t i = 0; i < (size_t)TargetType::Count; i++)
			{
				ED_RELEASE(Display.MRT[i]);
				ED_RELEASE(Display.RT[i]);
			}

			ED_RELEASE(Display.MaterialBuffer);
			ED_RELEASE(Simulator);
		}
		void SceneGraph::Configure(const Desc& NewConf)
		{
			ED_ASSERT_V(NewConf.Shared.Device != nullptr, "graphics device should be set");
			Transaction([this, NewConf]()
			{
				auto* Device = NewConf.Shared.Device;
				Display.DepthStencil = Device->GetDepthStencilState("none");
				Display.Rasterizer = Device->GetRasterizerState("cull-back");
				Display.Blend = Device->GetBlendState("overwrite");
				Display.Sampler = Device->GetSamplerState("trilinear-x16");
				Display.Layout = Device->GetInputLayout("shape-vertex");
				Conf = NewConf;

				Materials.Reserve(Conf.StartMaterials);
				Entities.Reserve(Conf.StartEntities);
				Dirty.Reserve(Conf.StartEntities);

				for (auto& Sparse : Registry)
					Sparse.second->Data.Reserve(Conf.StartComponents);

				for (size_t i = 0; i < (size_t)ActorType::Count; i++)
					Actors[i].Reserve(Conf.StartComponents);

				GenerateMaterialBuffer();
				GenerateVoxelBuffers();
				GenerateDepthBuffers();
				ResizeBuffers();
				Transaction([this]()
				{
					auto* Base = Camera.load();
					if (Base != nullptr)
						Base->Activate(Base);
				});

				auto& Cameras = GetComponents<Components::Camera>();
				for (auto It = Cameras.Begin(); It != Cameras.End(); ++It)
				{
					auto* Base = (Components::Camera*)*It;
					Base->GetRenderer()->Remount();
				}
			});
		}
		void SceneGraph::Actualize()
		{
			StepTransactions();
		}
		void SceneGraph::ResizeBuffers()
		{
			Transaction([this]()
			{
				ResizeRenderBuffers();
				if (!Camera.load())
					return;

				auto& Array = GetComponents<Components::Camera>();
				for (auto It = Array.Begin(); It != Array.End(); ++It)
					((Components::Camera*)*It)->ResizeBuffers();
			});
		}
		void SceneGraph::ResizeRenderBuffers()
		{
			Transaction([this]()
			{
				auto* Device = Conf.Shared.Device;
				ED_ASSERT_V(Device != nullptr, "graphics device should be set");
				Graphics::MultiRenderTarget2D::Desc MRT = GetDescMRT();
				Graphics::RenderTarget2D::Desc RT = GetDescRT();
				ED_CLEAR(Display.Merger);

				for (size_t i = 0; i < (size_t)TargetType::Count; i++)
				{
					ED_RELEASE(Display.MRT[i]);
					Display.MRT[i] = Device->CreateMultiRenderTarget2D(MRT);

					ED_RELEASE(Display.RT[i]);
					Display.RT[i] = Device->CreateRenderTarget2D(RT);
				}
			});
		}
		void SceneGraph::FillMaterialBuffers()
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			Graphics::MappedSubresource Stream;
			if (!Device->Map(Display.MaterialBuffer, Graphics::ResourceMap::Write_Discard, &Stream))
				return;

			size_t Size = 0;
			Subsurface* Array = (Subsurface*)Stream.Pointer;
			auto Begin = Materials.Begin(), End = Materials.End();
			for (auto It = Begin; It != End; ++It)
			{
				Subsurface& Next = Array[Size];
				(*It)->Slot = Size++;
				Next = (*It)->Surface;
			}

			Device->Unmap(Display.MaterialBuffer, &Stream);
			Device->SetStructureBuffer(Display.MaterialBuffer, 0, ED_PS | ED_CS);
		}
		void SceneGraph::Submit()
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			ED_ASSERT_V(Conf.Shared.Primitives != nullptr, "primitive cache should be set");

			Device->Render.TexCoord = 1.0f;
			Device->Render.Transform.Identify();
			Device->SetTarget();
			Device->SetDepthStencilState(Display.DepthStencil);
			Device->SetBlendState(Display.Blend);
			Device->SetRasterizerState(Display.Rasterizer);
			Device->SetInputLayout(Display.Layout);
			Device->SetSamplerState(Display.Sampler, 1, 1, ED_PS);
			Device->SetTexture2D(Display.MRT[(size_t)TargetType::Main]->GetTarget(0), 1, ED_PS);
			Device->SetShader(Device->GetBasicEffect(), ED_VS | ED_PS);
			Device->SetVertexBuffer(Conf.Shared.Primitives->GetQuad());
			Device->UpdateBuffer(Graphics::RenderBufferType::Render);
			Device->Draw(6, 0);
			Device->SetTexture2D(nullptr, 1, ED_PS);
			Statistics.DrawCalls++;
		}
		void SceneGraph::Dispatch(Core::Timer* Time)
		{
			ED_ASSERT_V(Time != nullptr, "time should be set");
			ED_MEASURE(ED_TIMING_FRAME);

			StepEvents();
			StepTransactions();
			StepGameplay(Time);
			StepSimulate(Time);
			StepAnimate(Time);
			StepSynchronize(Time);
			StepIndexing();
			StepFinalize();
		}
		void SceneGraph::Publish(Core::Timer* Time)
		{
			ED_ASSERT_V(Time != nullptr, "timer should be set");
			ED_MEASURE(ED_TIMING_CORE * 2);

			auto* Base = (Components::Camera*)Camera.load();
			auto* Renderer = (Base ? Base->GetRenderer() : nullptr);
			Statistics.Instances = 0;
			Statistics.DrawCalls = 0;

			if (Renderer != nullptr)
			{
				FillMaterialBuffers();
				SetMRT(TargetType::Main, true);
				Renderer->RestoreViewBuffer(nullptr);
				Statistics.DrawCalls += Renderer->Render(Time, RenderState::Geometry_Result, RenderOpt::None);
			}
		}
		void SceneGraph::StepSimulate(Core::Timer* Time)
		{
			ED_ASSERT_V(Time != nullptr, "timer should be set");
			ED_ASSERT_V(Simulator != nullptr, "simulator should be set");
			ED_MEASURE(ED_TIMING_CORE);

			if (!Active)
				return;

			Watch(Parallel::Enqueue([this, Time]()
			{
				Simulator->Simulate(4, Time->GetStep(), Time->GetFixedStep());
			}));
		}
		void SceneGraph::StepSynchronize(Core::Timer* Time)
		{
			ED_ASSERT_V(Time != nullptr, "timer should be set");
			ED_MEASURE(ED_TIMING_CORE);

			auto& Storage = Actors[(size_t)ActorType::Synchronize];
			if (!Storage.Empty())
			{
				WatchAll(Parallel::ForEach(Storage.Begin(), Storage.End(), [Time](Component* Next)
				{
					Next->Synchronize(Time);
				}));
			}
		}
		void SceneGraph::StepAnimate(Core::Timer* Time)
		{
			ED_ASSERT_V(Time != nullptr, "timer should be set");
			ED_MEASURE(ED_TIMING_CORE);

			auto& Storage = Actors[(size_t)ActorType::Animate];
			if (Active && !Storage.Empty())
			{
				WatchAll(Parallel::ForEach(Storage.Begin(), Storage.End(), [Time](Component* Next)
				{
					Next->Animate(Time);
				}));
			}
		}
		void SceneGraph::StepGameplay(Core::Timer* Time)
		{
			ED_ASSERT_V(Time != nullptr, "timer should be set");
			ED_MEASURE(ED_TIMING_FRAME);

			auto& Storage = Actors[(size_t)ActorType::Update];
			if (Active && !Storage.Empty())
			{
				std::for_each(Storage.Begin(), Storage.End(), [Time](Component* Next)
				{
					Next->Update(Time);
				});
			}
		}
		void SceneGraph::StepTransactions()
		{
			ED_MEASURE(ED_TIMING_FRAME);
			while (!Transactions.empty())
			{
				Transactions.front()();
				Transactions.pop();
			}
		}
		void SceneGraph::StepEvents()
		{
			ED_MEASURE(ED_TIMING_FRAME);
			while (!Events.empty())
			{
				auto& Source = Events.front();
				ResolveEvent(Source);
				Events.pop();
			}
		}
		void SceneGraph::StepIndexing()
		{
			ED_MEASURE(ED_TIMING_CORE);
			if (!Camera.load() || Dirty.Empty())
				return;

			auto Begin = Dirty.Begin();
			auto End = Dirty.End();
			Dirty.Clear();

			WatchAll(Parallel::ForEach(Begin, End, [this](Entity* Next)
			{
				Next->Transform->Synchronize();
				Next->UpdateBounds();

				if (Next->Type.Components.empty())
					return;

				std::unique_lock<std::mutex> Unique(Exclusive);
				for (auto& Item : *Next)
				{
					if (Item.second->IsCullable())
						Changes[Item.second->GetId()].insert(Item.second);
				}
			}));
		}
		void SceneGraph::StepFinalize()
		{
			ED_MEASURE(ED_TIMING_CORE);

			AwaitAll();
			if (!Camera.load())
				return;

			for (auto& Item : Changes)
			{
				if (Item.second.empty())
					continue;

				auto& Storage = GetStorage(Item.first);
				Storage.Index.Reserve(Item.second.size());

				size_t Count = 0;
				for (auto It = Item.second.begin(); It != Item.second.end(); ++It)
				{
					UpdateCosmos(Storage, *It);
					if (++Count > Conf.MaxUpdates)
					{
						Item.second.erase(Item.second.begin(), It);
						break;
					}
				}

				if (Count == Item.second.size())
					Item.second.clear();
			}
		}
		void SceneGraph::SetCamera(Entity* NewCamera)
		{
			if (!NewCamera)
			{
				Camera = nullptr;
				return;
			}

			Components::Camera* Target = NewCamera->GetComponent<Components::Camera>();
			if (!Target || !Target->Active)
			{
				Camera = nullptr;
				return;
			}

			Camera = Target;
			Transaction([Target]()
			{
				Target->Activate(Target);
			});
		}
		void SceneGraph::RemoveEntity(Entity* Entity)
		{
			ED_ASSERT_V(Entity != nullptr, "entity should be set");
			UnregisterEntity(Entity);
		}
		void SceneGraph::DeleteEntity(Entity* Entity)
		{
			ED_ASSERT_V(Entity != nullptr, "entity should be set");
			if (!UnregisterEntity(Entity))
				return;

			Entity->RemoveChilds();
			Transaction([Entity]()
			{
				ED_RELEASE(Entity);
			});
		}
		void SceneGraph::DeleteMaterial(Material* Value)
		{
			ED_ASSERT_V(Value != nullptr, "entity should be set");
			Mutate(Value, "pop");

			auto Begin = Materials.Begin(), End = Materials.End();
			for (auto It = Begin; It != End; ++It)
			{
				if (*It == Value)
				{
					Materials.RemoveAt(It);
					break;
				}
			}

			Transaction([Value]()
			{
				ED_RELEASE(Value);
			});
		}
		void SceneGraph::RegisterEntity(Entity* Target)
		{
			ED_ASSERT_V(Target != nullptr, "entity should be set");
			ED_ASSERT_V(Target->Scene == this, "entity be created within this scene");

			Target->Active = true;
			for (auto& Base : Target->Type.Components)
				RegisterComponent(Base.second, Target->Active);

			WatchMovement(Target);
			Mutate(Target, "push");
		}
		bool SceneGraph::UnregisterEntity(Entity* Target)
		{
			ED_ASSERT(Target != nullptr, false, "entity should be set");
			ED_ASSERT(Target->GetScene() == this, false, "entity should be attached to current scene");

			Component* Viewer = Camera.load();
			if (Viewer != nullptr && Target == Viewer->Parent)
				Camera = nullptr;

			for (auto& Base : Target->Type.Components)
				UnregisterComponent(Base.second);

			Target->Active = false;
			Entities.Remove(Target);
			UnwatchMovement(Target);
			Mutate(Target, "pop");

			return true;
		}
		void SceneGraph::RegisterComponent(Component* Base, bool Verify)
		{
			auto& Storage = GetComponents(Base->GetId());
			if (!Base->Active)
			{
				Transaction([Base]()
				{
					Base->Activate(nullptr);
				});
			}

			if (Verify)
			{
				Storage.AddIfNotExists(Base);
				if (Base->Set & (size_t)ActorSet::Update)
					GetActors(ActorType::Update).AddIfNotExists(Base);
				if (Base->Set & (size_t)ActorSet::Synchronize)
					GetActors(ActorType::Synchronize).AddIfNotExists(Base);
				if (Base->Set & (size_t)ActorSet::Animate)
					GetActors(ActorType::Animate).AddIfNotExists(Base);
				if (Base->Set & (size_t)ActorSet::Message)
					GetActors(ActorType::Message).AddIfNotExists(Base);
			}
			else
			{
				Storage.Add(Base);
				if (Base->Set & (size_t)ActorSet::Update)
					GetActors(ActorType::Update).Add(Base);
				if (Base->Set & (size_t)ActorSet::Synchronize)
					GetActors(ActorType::Synchronize).Add(Base);
				if (Base->Set & (size_t)ActorSet::Animate)
					GetActors(ActorType::Animate).Add(Base);
				if (Base->Set & (size_t)ActorSet::Message)
					GetActors(ActorType::Message).Add(Base);
			}
		}
		void SceneGraph::UnregisterComponent(Component* Base)
		{
			auto& Storage = GetComponents(Base->GetId());
			if (Base->Active)
				Base->Deactivate();

			Storage.Remove(Base);
			if (Base->Set & (size_t)ActorSet::Update)
				GetActors(ActorType::Update).Remove(Base);
			if (Base->Set & (size_t)ActorSet::Synchronize)
				GetActors(ActorType::Synchronize).Remove(Base);
			if (Base->Set & (size_t)ActorSet::Animate)
				GetActors(ActorType::Animate).Remove(Base);
			if (Base->Set & (size_t)ActorSet::Message)
				GetActors(ActorType::Message).Remove(Base);
		}
		void SceneGraph::LoadComponent(Component* Base)
		{
			ED_ASSERT_V(Base != nullptr, "component should be set");
			ED_ASSERT_V(Base->Parent != nullptr && Base->Parent->Scene == this, "component should be tied to this scene");
			std::unique_lock<std::mutex> Unique(Exclusive);
			++Incomplete[Base];
		}
		void SceneGraph::UnloadComponentAll(Component* Base)
		{
			std::unique_lock<std::mutex> Unique(Exclusive);
			auto It = Incomplete.find(Base);
			if (It != Incomplete.end())
				Incomplete.erase(It);
		}
		bool SceneGraph::UnloadComponent(Component* Base)
		{
			std::unique_lock<std::mutex> Unique(Exclusive);
			auto It = Incomplete.find(Base);

			if (It == Incomplete.end())
				return false;
			else if (!--It->second)
				Incomplete.erase(It);
			
			return true;
		}
		void SceneGraph::CloneEntities(Entity* Instance, std::vector<Entity*>* Array)
		{
			ED_ASSERT_V(Instance != nullptr, "entity should be set");
			ED_ASSERT_V(Array != nullptr, "array should be set");

			Entity* Clone = CloneEntityInstance(Instance);
			Array->push_back(Clone);

			Compute::Transform* Root = Clone->Transform->GetRoot();
			if (Root != nullptr)
				Root->GetChilds().push_back(Clone->Transform);

			if (!Instance->Transform->GetChildsCount())
				return;

			std::vector<Compute::Transform*>& Childs = Instance->Transform->GetChilds();
			Clone->Transform->GetChilds().clear();

			for (auto& Child : Childs)
			{
				size_t Offset = Array->size();
				CloneEntities((Entity*)Child->UserData, Array);
				for (size_t j = Offset; j < Array->size(); j++)
				{
					Entity* Next = (*Array)[j];
					if (Next->Transform->GetRoot() == Instance->Transform)
						Next->Transform->SetRoot(Clone->Transform);
				}
			}
		}
		void SceneGraph::RayTest(uint64_t Section, const Compute::Ray& Origin, const RayCallback& Callback)
		{
			ED_ASSERT_V(Callback, "callback should not be empty");
			ED_MEASURE(ED_TIMING_FRAME);

			auto& Array = GetComponents(Section);
			Compute::Ray Base = Origin;
			Compute::Vector3 Hit;

			for (auto& Next : Array)
			{
				if (Compute::Geometric::CursorRayTest(Base, Next->Parent->Snapshot.Box, &Hit) && !Callback(Next, Hit))
					break;
			}
		}
		void SceneGraph::ScriptHook(const std::string& Name)
		{
			Transaction([this, Name]()
			{
				auto& Array = GetComponents<Components::Scriptable>();
				for (auto It = Array.Begin(); It != Array.End(); ++It)
				{
					Components::Scriptable* Base = (Components::Scriptable*)*It;
					Base->CallEntry(Name);
				}
			});
		}
		void SceneGraph::SetActive(bool Enabled)
		{
			Active = Enabled;
			if (!Active)
				return;

			Transaction([this]()
			{
				auto Begin = Entities.Begin(), End = Entities.End();
				for (auto It = Begin; It != End; ++It)
				{
					Entity* Next = *It;
					for (auto& Base : Next->Type.Components)
					{
						if (Base.second->Active)
							Base.second->Activate(nullptr);
					}
				}
			});
		}
		void SceneGraph::SetMRT(TargetType Type, bool Clear)
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			Graphics::MultiRenderTarget2D* Target = Display.MRT[(size_t)Type];
			Device->SetTarget(Target);

			if (!Clear)
				return;

			Device->Clear(Target, 0, 0, 0, 0);
			Device->Clear(Target, 1, 0, 0, 0);
			Device->Clear(Target, 2, 1, 0, 0);
			Device->Clear(Target, 3, 0, 0, 0);
			Device->ClearDepth(Target);
		}
		void SceneGraph::SetRT(TargetType Type, bool Clear)
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			Graphics::RenderTarget2D* Target = Display.RT[(size_t)Type];
			Device->SetTarget(Target);

			if (!Clear)
				return;

			Device->Clear(Target, 0, 0, 0, 0);
			Device->ClearDepth(Target);
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
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			Graphics::MultiRenderTarget2D* Target = Display.MRT[(size_t)Type];
			if (Color)
			{
				Device->Clear(Target, 0, 0, 0, 0);
				Device->Clear(Target, 1, 0, 0, 0);
				Device->Clear(Target, 2, 1, 0, 0);
				Device->Clear(Target, 3, 0, 0, 0);
			}

			if (Depth)
				Device->ClearDepth(Target);
		}
		void SceneGraph::ClearRT(TargetType Type, bool Color, bool Depth)
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");
			Graphics::RenderTarget2D* Target = Display.RT[(size_t)Type];
			if (Color)
				Device->Clear(Target, 0, 0, 0, 0);

			if (Depth)
				Device->ClearDepth(Target);
		}
		bool SceneGraph::GetVoxelBuffer(Graphics::Texture3D** In, Graphics::Texture3D** Out)
		{
			ED_ASSERT(In && Out, false, "input and output should be set");
			ED_ASSERT(Display.VoxelBuffers[0] != nullptr, false, "first voxel buffer should be set");
			ED_ASSERT(Display.VoxelBuffers[1] != nullptr, false, "second voxel buffer should be set");
			ED_ASSERT(Display.VoxelBuffers[2] != nullptr, false, "third voxel buffer should be set");

			for (unsigned int i = 0; i < 3; i++)
			{
				In[i] = Display.VoxelBuffers[i];
				Out[i] = nullptr;
			}

			return true;
		}
		MessageCallback* SceneGraph::SetListener(const std::string& EventName, MessageCallback&& Callback)
		{
			MessageCallback* Id = ED_NEW(MessageCallback, std::move(Callback));
			std::unique_lock<std::mutex> Unique(Exclusive);
			Listeners[EventName].insert(Id);

			return Id;
		}
		bool SceneGraph::ClearListener(const std::string& EventName, MessageCallback* Id)
		{
			ED_ASSERT(!EventName.empty(), false, "event name should not be empty");
			ED_ASSERT(Id != nullptr, false, "callback id should be set");

			std::unique_lock<std::mutex> Unique(Exclusive);
			auto& Source = Listeners[EventName];
			auto It = Source.find(Id);
			if (It == Source.end())
				return false;

			Source.erase(It);
			ED_DELETE(function, Id);

			return true;
		}
		bool SceneGraph::PushEvent(const std::string& EventName, Core::VariantArgs&& Args, bool Propagate)
		{
			Event Next(EventName, std::move(Args));
			Next.Args["__vb"] = Core::Var::Integer((int64_t)(Propagate ? EventTarget::Scene : EventTarget::Listener));
			Next.Args["__vt"] = Core::Var::Pointer((void*)this);

			std::unique_lock<std::mutex> Unique(Exclusive);
			Events.push(std::move(Next));

			return true;
		}
		bool SceneGraph::PushEvent(const std::string& EventName, Core::VariantArgs&& Args, Component* Target)
		{
			ED_ASSERT(Target != nullptr, false, "target should be set");
			Event Next(EventName, std::move(Args));
			Next.Args["__vb"] = Core::Var::Integer((int64_t)EventTarget::Component);
			Next.Args["__vt"] = Core::Var::Pointer((void*)Target);

			std::unique_lock<std::mutex> Unique(Exclusive);
			Events.push(std::move(Next));

			return true;
		}
		bool SceneGraph::PushEvent(const std::string& EventName, Core::VariantArgs&& Args, Entity* Target)
		{
			ED_ASSERT(Target != nullptr, false, "target should be set");
			Event Next(EventName, std::move(Args));
			Next.Args["__vb"] = Core::Var::Integer((int64_t)EventTarget::Entity);
			Next.Args["__vt"] = Core::Var::Pointer((void*)Target);

			std::unique_lock<std::mutex> Unique(Exclusive);
			Events.push(std::move(Next));

			return true;
		}
		bool SceneGraph::IsLeftHanded() const
		{
			return Conf.Shared.Device->IsLeftHanded();
		}
		bool SceneGraph::IsIndexed() const
		{
			for (auto& Item : Changes)
			{
				if (!Item.second.empty())
					return false;
			}

			return true;
		}
		bool SceneGraph::IsBusy() const
		{
			return !Tasks.empty();
		}
		void SceneGraph::Mutate(Entity* Parent, Entity* Child, const char* Type)
		{
			ED_ASSERT_V(Parent != nullptr, "parent should be set");
			ED_ASSERT_V(Child != nullptr, "child should be set");
			ED_ASSERT_V(Type != nullptr, "type should be set");
			if (!Conf.Mutations)
				return;

			Core::VariantArgs Args;
			Args["parent"] = Core::Var::Pointer((void*)Parent);
			Args["child"] = Core::Var::Pointer((void*)Child);
			Args["type"] = Core::Var::String(Type, strlen(Type));

			PushEvent("mutation", std::move(Args), false);
		}
		void SceneGraph::Mutate(Entity* Target, const char* Type)
		{
			ED_ASSERT_V(Target != nullptr, "target should be set");
			ED_ASSERT_V(Type != nullptr, "type should be set");
			if (!Conf.Mutations)
				return;

			Core::VariantArgs Args;
			Args["entity"] = Core::Var::Pointer((void*)Target);
			Args["type"] = Core::Var::String(Type, strlen(Type));

			PushEvent("mutation", std::move(Args), false);
		}
		void SceneGraph::Mutate(Component* Target, const char* Type)
		{
			ED_ASSERT_V(Target != nullptr, "target should be set");
			ED_ASSERT_V(Type != nullptr, "type should be set");
			NotifyCosmos(Target);

			if (!Conf.Mutations)
				return;

			Core::VariantArgs Args;
			Args["component"] = Core::Var::Pointer((void*)Target);
			Args["type"] = Core::Var::String(Type, strlen(Type));

			PushEvent("mutation", std::move(Args), false);
		}
		void SceneGraph::Mutate(Material* Target, const char* Type)
		{
			ED_ASSERT_V(Target != nullptr, "target should be set");
			ED_ASSERT_V(Type != nullptr, "type should be set");
			if (!Conf.Mutations)
				return;

			Core::VariantArgs Args;
			Args["material"] = Core::Var::Pointer((void*)Target);
			Args["type"] = Core::Var::String(Type, strlen(Type));

			PushEvent("mutation", std::move(Args), false);
		}
		void SceneGraph::MakeSnapshot(IdxSnapshot* Result)
		{
			ED_ASSERT_V(Result != nullptr, "shapshot result should be set");
			Result->To.clear();
			Result->From.clear();

			size_t Index = 0;
			auto Begin = Entities.Begin(), End = Entities.End();
			for (auto It = Begin; It != End; ++It)
			{
				Result->To[*It] = Index;
				Result->From[Index] = *It;
				Index++;
			}
		}
		void SceneGraph::Transaction(Core::TaskCallback&& Callback)
		{
			ED_ASSERT_V(Callback != nullptr, "callback should be set");
			bool ExecuteNow = false;
			{
				std::unique_lock<std::mutex> Unique(Exclusive);
				if (!Tasks.empty() || !Events.empty())
					Transactions.emplace(std::move(Callback));
				else
					ExecuteNow = true;
			}
			if (ExecuteNow)
				Callback();
		}
		void SceneGraph::Watch(Parallel::Task&& Awaitable)
		{
			if (Awaitable.valid())
				Tasks.emplace(std::move(Awaitable));
		}
		void SceneGraph::WatchAll(std::vector<Parallel::Task>&& Awaitables)
		{
			for (auto& Awaitable : Awaitables)
				Watch(std::move(Awaitable));
		}
		void SceneGraph::AwaitAll()
		{
			ED_MEASURE(ED_TIMING_CORE);
			while (!Tasks.empty())
			{
				Tasks.front().wait();
				Tasks.pop();
			}
		}
		void SceneGraph::ClearCulling()
		{
			for (auto& Sparse : Registry)
			{
				auto Begin = Sparse.second->Data.Begin();
				auto End = Sparse.second->Data.End();
				for (auto* It = Begin; It != End; ++It)
				{
					Component* Target = *It;
					if (Target->IsDrawable())
					{
						Drawable* Base = (Drawable*)Target;
						Base->Overlapping = 1.0f;
					}
				}
			}
		}
		void SceneGraph::GenerateMaterialBuffer()
		{
			Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
			F.AccessFlags = Graphics::CPUAccess::Write;
			F.MiscFlags = Graphics::ResourceMisc::Buffer_Structured;
			F.Usage = Graphics::ResourceUsage::Dynamic;
			F.BindFlags = Graphics::ResourceBind::Shader_Input;
			F.ElementCount = (unsigned int)Materials.Capacity();
			F.ElementWidth = sizeof(Subsurface);
			F.StructureByteStride = F.ElementWidth;

			ED_RELEASE(Display.MaterialBuffer);
			Display.MaterialBuffer = Conf.Shared.Device->CreateElementBuffer(F);
		}
		void SceneGraph::GenerateVoxelBuffers()
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");

			Conf.VoxelsSize = Conf.VoxelsSize - Conf.VoxelsSize % 8;
			Conf.VoxelsMips = Device->GetMipLevel((unsigned int)Conf.VoxelsSize, (unsigned int)Conf.VoxelsSize);

			Graphics::Texture3D::Desc I;
			I.Width = I.Height = I.Depth = (unsigned int)Conf.VoxelsSize;
			I.MipLevels = 0;
			I.Writable = true;

			ED_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Diffuse]);
			Display.VoxelBuffers[(size_t)VoxelType::Diffuse] = Device->CreateTexture3D(I);

			I.FormatMode = Graphics::Format::R16G16B16A16_Float;
			ED_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Normal]);
			Display.VoxelBuffers[(size_t)VoxelType::Normal] = Device->CreateTexture3D(I);

			I.FormatMode = Graphics::Format::R8G8B8A8_Unorm;
			ED_RELEASE(Display.VoxelBuffers[(size_t)VoxelType::Surface]);
			Display.VoxelBuffers[(size_t)VoxelType::Surface] = Device->CreateTexture3D(I);

			I.MipLevels = (unsigned int)Conf.VoxelsMips;
			Display.Voxels.resize(Conf.VoxelsMax);
			for (auto& Item : Display.Voxels)
			{
				ED_RELEASE(Item.first);
				Item.first = Device->CreateTexture3D(I);
				Item.second = nullptr;
			}

			if (Entities.Empty())
				return;

			Core::VariantArgs Args;
			PushEvent("voxel-flush", std::move(Args), true);
		}
		void SceneGraph::GenerateDepthBuffers()
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT_V(Device != nullptr, "graphics device should be set");

			for (auto& Item : Display.Points)
				ED_CLEAR(Item);

			for (auto& Item : Display.Spots)
				ED_CLEAR(Item);

			for (auto& Item : Display.Lines)
			{
				if (Item != nullptr)
				{
					for (auto* Target : *Item)
						ED_CLEAR(Target);
					ED_DELETE(vector, Item);
					Item = nullptr;
				}
			}

			Display.Points.resize(Conf.PointsMax);
			for (auto& Item : Display.Points)
			{
				Graphics::DepthTargetCube::Desc F = Graphics::DepthTargetCube::Desc();
				F.Size = (unsigned int)Conf.PointsSize;
				F.FormatMode = Graphics::Format::D32_Float;
				Item = Device->CreateDepthTargetCube(F);
			}

			Display.Spots.resize(Conf.SpotsMax);
			for (auto& Item : Display.Spots)
			{
				Graphics::DepthTarget2D::Desc F = Graphics::DepthTarget2D::Desc();
				F.Width = F.Height = (unsigned int)Conf.SpotsSize;
				F.FormatMode = Graphics::Format::D32_Float;
				Item = Device->CreateDepthTarget2D(F);
			}

			Display.Lines.resize(Conf.LinesMax);
			for (auto& Item : Display.Lines)
				Item = nullptr;

			if (Entities.Empty())
				return;

			Core::VariantArgs Args;
			PushEvent("depth-flush", std::move(Args), true);
		}
		void SceneGraph::GenerateDepthCascades(CascadedDepthMap** Result, uint32_t Size) const
		{
			CascadedDepthMap* Target = (*Result ? *Result : ED_NEW(CascadedDepthMap));
			for (auto& Item : *Target)
				ED_RELEASE(Item);

			Target->resize(Size);
			for (auto& Item : *Target)
			{
				Graphics::DepthTarget2D::Desc F = Graphics::DepthTarget2D::Desc();
				F.Width = F.Height = (unsigned int)Conf.LinesSize;
				F.FormatMode = Graphics::Format::D32_Float;
				Item = Conf.Shared.Device->CreateDepthTarget2D(F);
			}

			*Result = Target;
		}
		void SceneGraph::NotifyCosmos(Component* Base)
		{
			if (!Base->IsCullable())
				return;

			std::unique_lock<std::mutex> Unique(Exclusive);
			Changes[Base->GetId()].insert(Base);
		}
		void SceneGraph::ClearCosmos(Component* Base)
		{
			if (!Base->IsCullable())
				return;

			uint64_t Id = Base->GetId();
			std::unique_lock<std::mutex> Unique(Exclusive);
			Changes[Id].erase(Base);

			if (Base->Indexed)
			{
				auto& Storage = GetStorage(Id);
				Storage.Index.RemoveItem((void*)Base);
			}
		}
		void SceneGraph::UpdateCosmos(SparseIndex& Storage, Component* Base)
		{
			if (Base->Active)
			{
				auto& Bounds = Base->Parent->Snapshot;
				if (!Base->Indexed)
				{
					Storage.Index.InsertItem((void*)Base, Bounds.Min, Bounds.Max);
					Base->Indexed = true;
				}
				else
					Storage.Index.UpdateItem((void*)Base, Bounds.Min, Bounds.Max);
			}
			else
			{
				Storage.Index.RemoveItem((void*)Base);
				Base->Indexed = false;
			}
		}
		void SceneGraph::WatchMovement(Entity* Base)
		{
			Base->Transform->WhenDirty([this, Base]()
			{
				Transaction([this, Base]()
				{
					if (Dirty.Size() + Conf.GrowMargin > Dirty.Capacity())
						UpgradeBuffer(Dirty, (float)Conf.GrowRate);
					Dirty.Add(Base);
				});
			});
		}
		void SceneGraph::UnwatchMovement(Entity* Base)
		{
			Base->Transform->WhenDirty(nullptr);
			Dirty.Remove(Base);
		}
		bool SceneGraph::ResolveEvent(Event& Source)
		{
			auto _Bubble = Source.Args.find("__vb"), _Target = Source.Args.find("__vt");
			if (_Bubble == Source.Args.end() || _Target == Source.Args.end())
				return false;

			EventTarget Bubble = (EventTarget)_Bubble->second.GetInteger();
			void* Target = _Target->second.GetPointer();

			if (Bubble == EventTarget::Scene)
			{
				auto Begin = Actors[(size_t)ActorType::Message].Begin();
				auto End = Actors[(size_t)ActorType::Message].End();
				for (auto It = Begin; It != End; ++It)
					(*It)->Message(Source.Name, Source.Args);
			}
			else if (Bubble == EventTarget::Entity)
			{
				Entity* Base = (Entity*)Target;
				for (auto& Item : Base->Type.Components)
					Item.second->Message(Source.Name, Source.Args);
			}
			else if (Bubble == EventTarget::Component)
			{
				Component* Base = (Component*)Target;
				Base->Message(Source.Name, Source.Args);
			}

			auto It = Listeners.find(Source.Name);
			if (It == Listeners.end() || It->second.empty())
				return false;

			auto Copy = It->second;
			for (auto* Callback : Copy)
				(*Callback)(Source.Name, Source.Args);

			return true;
		}
		bool SceneGraph::AddMaterial(Material* Base)
		{
			ED_ASSERT(Base != nullptr, false, "base should be set");
			if (Materials.Size() + Conf.GrowMargin > Materials.Capacity())
			{
				Transaction([this, Base]()
				{
					if (Materials.Size() + Conf.GrowMargin > Materials.Capacity())
					{
						UpgradeBuffer(Materials, (float)Conf.GrowRate);
						GenerateMaterialBuffer();
					}
					AddMaterial(Base);
				});
			}
			else
			{
				Base->Scene = this;
				Materials.AddIfNotExists(Base);
				Mutate(Base, "push");
			}

			return true;
		}
		Material* SceneGraph::AddMaterial()
		{
			Material* Result = new Material(this);
			AddMaterial(Result);
			return Result;
		}
		Material* SceneGraph::CloneMaterial(Material* Base)
		{
			ED_ASSERT(Base != nullptr, nullptr, "material should be set");
			Material* Result = new Material(*Base);
			AddMaterial(Result);
			return Result;
		}
		Component* SceneGraph::GetCamera()
		{
			Component* Base = Camera.load();
			if (Base != nullptr)
				return Base;

			Base = GetComponent<Components::Camera>();
			if (!Base || !Base->Active)
			{
				Entity* Next = new Entity(this);
				Base = Next->AddComponent<Components::Camera>();
				AddEntity(Next);
				SetCamera(Next);
			}
			else
			{
				Transaction([this, Base]()
				{
					Base->Activate(Base);
					Camera = Base;
				});
			}

			return Base;
		}
		Component* SceneGraph::GetComponent(size_t Component, uint64_t Section)
		{
			auto& Array = GetComponents(Section);
			if (Component >= Array.Size())
				return nullptr;

			return *Array.At(Component);
		}
		RenderSystem* SceneGraph::GetRenderer()
		{
			auto* Viewer = (Components::Camera*)Camera.load();
			if (!Viewer)
				return nullptr;

			return Viewer->GetRenderer();
		}
		Viewer SceneGraph::GetCameraViewer() const
		{
			auto* Result = (Components::Camera*)Camera.load();
			if (!Result)
				return Viewer();

			return Result->GetViewer();
		}
		SparseIndex& SceneGraph::GetStorage(uint64_t Section)
		{
			SparseIndex* Storage = Registry[Section];
			ED_ASSERT(Storage != nullptr, *Registry.begin()->second, "component should be registered by composer");
			if (Storage->Data.Size() + Conf.GrowMargin > Storage->Data.Capacity())
			{
				Transaction([this, Section]()
				{
					SparseIndex* Storage = Registry[Section];
					if (Storage->Data.Size() + Conf.GrowMargin > Storage->Data.Capacity())
					    UpgradeBuffer(Storage->Data, (float)Conf.GrowRate);
				});
			}

			return *Storage;
		}
		Material* SceneGraph::GetMaterial(const std::string& Name)
		{
			auto Begin = Materials.Begin(), End = Materials.End();
			for (auto It = Begin; It != End; ++It)
			{
				if ((*It)->Name == Name)
					return *It;
			}

			return nullptr;
		}
		Material* SceneGraph::GetMaterial(size_t Material)
		{
			ED_ASSERT(Material < Materials.Size(), nullptr, "index outside of range");
			return Materials[Material];
		}
		Entity* SceneGraph::GetEntity(size_t Entity)
		{
			ED_ASSERT(Entity < Entities.Size(), nullptr, "index outside of range");
			return Entities[Entity];
		}
		Entity* SceneGraph::GetLastEntity()
		{
			if (Entities.Empty())
				return nullptr;

			return Entities.Back();
		}
		Entity* SceneGraph::GetCameraEntity()
		{
			Component* Data = GetCamera();
			return Data ? Data->Parent : nullptr;
		}
		Entity* SceneGraph::CloneEntityInstance(Entity* Entity)
		{
			ED_ASSERT(Entity != nullptr, nullptr, "entity should be set");
			ED_MEASURE(ED_TIMING_FRAME);

			Engine::Entity* Instance = new Engine::Entity(this);
			Instance->Transform->Copy(Entity->Transform);
			Instance->Transform->UserData = Instance;
			Instance->Type.Name = Entity->Type.Name;
			Instance->Type.Components = Entity->Type.Components;

			for (auto& It : Instance->Type.Components)
			{
				Component* Source = It.second;
				It.second = Source->Copy(Instance);
				It.second->Parent = Instance;
				It.second->Active = Source->Active;
			}

			AddEntity(Instance);
			return Instance;
		}
		Entity* SceneGraph::CloneEntity(Entity* Value)
		{
			auto Array = CloneEntityAsArray(Value);
			return Array.empty() ? nullptr : Array.front();
		}
		Core::Pool<Component*>& SceneGraph::GetComponents(uint64_t Section)
		{
			SparseIndex& Storage = GetStorage(Section);
			return Storage.Data;
		}
		Core::Pool<Component*>& SceneGraph::GetActors(ActorType Type)
		{
			auto& Storage = Actors[(size_t)Type];
			if (Storage.Size() + Conf.GrowMargin > Storage.Capacity())
			{
				Transaction([this, Type]()
				{
					auto& Storage = Actors[(size_t)Type];
					if (Storage.Size() + Conf.GrowMargin > Storage.Capacity())
						UpgradeBuffer(Storage, (float)Conf.GrowRate);
				});
			}

			return Storage;
		}
		Graphics::RenderTarget2D::Desc SceneGraph::GetDescRT() const
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT(Device != nullptr, Graphics::RenderTarget2D::Desc(), "graphics device should be set");
			Graphics::RenderTarget2D* Target = Device->GetRenderTarget();

			ED_ASSERT(Target != nullptr, Graphics::RenderTarget2D::Desc(), "render target should be set");
			Graphics::RenderTarget2D::Desc Desc;
			Desc.MiscFlags = Graphics::ResourceMisc::Generate_Mips;
			Desc.Width = (unsigned int)(Target->GetWidth() * Conf.RenderQuality);
			Desc.Height = (unsigned int)(Target->GetHeight() * Conf.RenderQuality);
			Desc.MipLevels = Device->GetMipLevel(Desc.Width, Desc.Height);
			Desc.FormatMode = GetFormatMRT(0);

			return Desc;
		}
		Graphics::MultiRenderTarget2D::Desc SceneGraph::GetDescMRT() const
		{
			auto* Device = Conf.Shared.Device;
			ED_ASSERT(Device != nullptr, Graphics::MultiRenderTarget2D::Desc(), "graphics device should be set");
			Graphics::RenderTarget2D* Target = Device->GetRenderTarget();

			ED_ASSERT(Target != nullptr, Graphics::MultiRenderTarget2D::Desc(), "render target should be set");
			Graphics::MultiRenderTarget2D::Desc Desc;
			Desc.MiscFlags = Graphics::ResourceMisc::Generate_Mips;
			Desc.Width = (unsigned int)(Target->GetWidth() * Conf.RenderQuality);
			Desc.Height = (unsigned int)(Target->GetHeight() * Conf.RenderQuality);
			Desc.MipLevels = Device->GetMipLevel(Desc.Width, Desc.Height);
			Desc.Target = Graphics::SurfaceTarget::T3;
			Desc.FormatMode[0] = GetFormatMRT(0);
			Desc.FormatMode[1] = GetFormatMRT(1);
			Desc.FormatMode[2] = GetFormatMRT(2);
			Desc.FormatMode[3] = GetFormatMRT(3);

			return Desc;
		}
		Graphics::Format SceneGraph::GetFormatMRT(unsigned int Target) const
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
		std::vector<Entity*> SceneGraph::CloneEntityAsArray(Entity* Value)
		{
			std::vector<Entity*> Array;
			ED_ASSERT(Value != nullptr, Array, "entity should be set");
			CloneEntities(Value, &Array);
			return Array;
		}
		std::vector<Entity*> SceneGraph::QueryByParent(Entity* Entity) const
		{
			std::vector<Engine::Entity*> Array;
			ED_ASSERT(Entity != nullptr, Array, "entity should be set");

			auto Begin = Entities.Begin(), End = Entities.End();
			for (auto It = Begin; It != End; ++It)
			{
				if (*It != Entity && !(*It)->Transform->HasRoot(Entity->Transform))
					Array.push_back(*It);
			}

			return Array;
		}
		std::vector<Entity*> SceneGraph::QueryByName(const std::string& Name) const
		{
			std::vector<Entity*> Array;
			auto Begin = Entities.Begin(), End = Entities.End();
			for (auto It = Begin; It != End; ++It)
			{
				if ((*It)->Type.Name == Name)
					Array.push_back(*It);
			}

			return Array;
		}
		std::vector<Component*> SceneGraph::QueryByPosition(uint64_t Section, const Compute::Vector3& Position, float Radius)
		{

			return QueryByArea(Section, Position - Radius * 0.5f, Position + Radius * 0.5f);
		}
		std::vector<Component*> SceneGraph::QueryByArea(uint64_t Section, const Compute::Vector3& Min, const Compute::Vector3& Max)
		{
			std::vector<Component*> Result;
			Compute::Bounding Target(Min, Max);
			Compute::Cosmos::Iterator Context;
			auto& Storage = GetStorage(Section);
			Storage.Index.QueryIndex<Component>(Context, [&Target](const Compute::Bounding& Bounds)
			{
				return Target.Overlaps(Bounds);
			}, [&Result](Component* Item)
			{
				Result.push_back(Item);
			});

			return Result;
		}
		std::vector<std::pair<Component*, Compute::Vector3>> SceneGraph::QueryByRay(uint64_t Section, const Compute::Ray& Origin)
		{
			typedef std::pair<Component*, Compute::Vector3> RayHit;
			std::vector<RayHit> Result;
			Compute::Ray Target = Origin;
			Compute::Cosmos::Iterator Context;
			auto& Storage = GetStorage(Section);
			Storage.Index.QueryIndex<Component>(Context, [&Target](const Compute::Bounding& Bounds)
			{
				return Target.IntersectsAABBAt(Bounds.Lower, Bounds.Upper, nullptr);
			}, [&Result](Component* Item)
			{
				Result.emplace_back(Item, Compute::Vector3::Zero());
			});

			for (auto It = Result.begin(); It != Result.end();)
			{
				if (!Compute::Geometric::CursorRayTest(Target, It->first->Parent->Snapshot.Box, &It->second))
					It = Result.erase(It);
				else
					++It;
			}

			ED_SORT(Result.begin(), Result.end(), [&Target](RayHit& A, RayHit& B)
			{
				float D1 = Target.Origin.Distance(A.first->Parent->Transform->GetPosition());
				float D2 = Target.Origin.Distance(B.first->Parent->Transform->GetPosition());
				return D1 < D2;
			});

			return Result;
		}
		std::vector<CubicDepthMap*>& SceneGraph::GetPointsMapping()
		{
			return Display.Points;
		}
		std::vector<LinearDepthMap*>& SceneGraph::GetSpotsMapping()
		{
			return Display.Spots;
		}
		std::vector<CascadedDepthMap*>& SceneGraph::GetLinesMapping()
		{
			return Display.Lines;
		}
		std::vector<VoxelMapping>& SceneGraph::GetVoxelsMapping()
		{
			return Display.Voxels;
		}
		std::string SceneGraph::AsResourcePath(const std::string& Path)
		{
			ED_ASSERT(Conf.Shared.Content != nullptr, Path, "content manager should be set");
			return Core::Parser(Path).Replace(Conf.Shared.Content->GetEnvironment(), "./").Replace('\\', '/').R();
		}
		Entity* SceneGraph::AddEntity()
		{
			Entity* Next = new Entity(this);
			AddEntity(Next);
			return Next;
		}
		bool SceneGraph::AddEntity(Entity* Entity)
		{
			ED_ASSERT(Entity != nullptr, false, "entity should be set");
			ED_ASSERT(Entity->Scene == this, false, "entity should be created for this scene");

			if (Entities.Size() + Conf.GrowMargin <= Entities.Capacity())
			{
				Entities.Add(Entity);
				RegisterEntity(Entity);
				return true;
			}
			
			Transaction([this, Entity]()
			{
				if (Entities.Size() + Conf.GrowMargin > Entities.Capacity())
					UpgradeBuffer(Entities, (float)Conf.GrowRate);
				AddEntity(Entity);
			});

			return true;
		}
		bool SceneGraph::HasEntity(Entity* Entity) const
		{
			ED_ASSERT(Entity != nullptr, false, "entity should be set");
			for (size_t i = 0; i < Entities.Size(); i++)
			{
				if (Entities[i] == Entity)
					return true;
			}

			return false;
		}
		bool SceneGraph::HasEntity(size_t Entity) const
		{
			return Entity < Entities.Size() ? Entity : -1;
		}
		bool SceneGraph::IsActive() const
		{
			return Active;
		}
		size_t SceneGraph::GetEntitiesCount() const
		{
			return Entities.Size();
		}
		size_t SceneGraph::GetComponentsCount(uint64_t Section)
		{
			return GetComponents(Section).Size();
		}
		size_t SceneGraph::GetMaterialsCount() const
		{
			return Materials.Size();
		}
		Graphics::MultiRenderTarget2D* SceneGraph::GetMRT(TargetType Type) const
		{
			return Display.MRT[(size_t)Type];
		}
		Graphics::RenderTarget2D* SceneGraph::GetRT(TargetType Type) const
		{
			return Display.RT[(size_t)Type];
		}
		Graphics::Texture2D** SceneGraph::GetMerger()
		{
			return &Display.Merger;
		}
		Graphics::ElementBuffer* SceneGraph::GetStructure() const
		{
			return Display.MaterialBuffer;
		}
		Graphics::GraphicsDevice* SceneGraph::GetDevice() const
		{
			return Conf.Shared.Device;
		}
		Graphics::Activity* SceneGraph::GetActivity() const
		{
			return Conf.Shared.Activity;
		}
		ShaderCache* SceneGraph::GetShaders() const
		{
			return Conf.Shared.Shaders;
		}
		PrimitiveCache* SceneGraph::GetPrimitives() const
		{
			return Conf.Shared.Primitives;
		}
		Compute::Simulator* SceneGraph::GetSimulator() const
		{
			return Simulator;
		}
		SceneGraph::Desc& SceneGraph::GetConf()
		{
			return Conf;
		}

		ContentManager::ContentManager(Graphics::GraphicsDevice* NewDevice) noexcept : Device(NewDevice), Base(Core::OS::Path::ResolveDirectory(Core::OS::Directory::Get().c_str())), Queue(0)
		{
			SetEnvironment(Base);
		}
		ContentManager::~ContentManager() noexcept
		{
			InvalidateCache();
			InvalidateDockers();

			for (auto It = Streams.begin(); It != Streams.end(); ++It)
				ED_RELEASE(It->first);

			for (auto It = Processors.begin(); It != Processors.end(); ++It)
				ED_RELEASE(It->second);
		}
		void ContentManager::InvalidateDockers()
		{
			Mutex.lock();
			for (auto It = Dockers.begin(); It != Dockers.end(); ++It)
				ED_DELETE(AssetArchive, It->second);

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

					ED_DELETE(AssetCache, Entry.second);
				}
			}

			Assets.clear();
			Mutex.unlock();
		}
		void ContentManager::InvalidatePath(const std::string& Path)
		{
			std::string File = Core::OS::Path::Resolve(Path, Environment);
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
		void ContentManager::SetDevice(Graphics::GraphicsDevice* NewDevice)
		{
			Device = NewDevice;
		}
		void* ContentManager::LoadForward(const std::string& Path, Processor* Processor, const Core::VariantArgs& Map)
		{
			if (Path.empty())
				return nullptr;

			if (!Processor)
			{
				ED_ERR("[engine] file processor for \"%s\" wasn't found", Path.c_str());
				return nullptr;
			}

			void* Object = LoadStreaming(Path, Processor, Map);
			if (Object != nullptr)
				return Object;

			std::string File = Path;
			if (!Core::OS::Path::IsRemote(File.c_str()))
			{
				Mutex.lock();
				File = Core::OS::Path::Resolve(File, Environment);
				Mutex.unlock();

				if (File.empty())
				{
					ED_ERR("[engine] file \"%s\" wasn't found", Path.c_str());
					return nullptr;
				}
			}

			AssetCache* Asset = Find(Processor, File);
			if (Asset != nullptr)
				return Processor->Duplicate(Asset, Map);

			auto* Stream = Core::OS::File::Open(File, Core::FileMode::Binary_Read_Only);
			if (!Stream)
				return nullptr;

			Object = Processor->Deserialize(Stream, 0, Map);
			ED_RELEASE(Stream);

			return Object;
		}
		void* ContentManager::LoadStreaming(const std::string& Path, Processor* Processor, const Core::VariantArgs& Map)
		{
			if (Path.empty())
				return nullptr;

			Core::Parser File(Path);
			File.Replace('\\', '/').Replace("./", "");

			Mutex.lock();
			auto Docker = Dockers.find(File.R());
			if (Docker == Dockers.end() || !Docker->second || !Docker->second->Stream)
			{
				Mutex.unlock();
				return nullptr;
			}
			Mutex.unlock();

			AssetCache* Asset = Find(Processor, File.R());
			if (Asset != nullptr)
				return Processor->Duplicate(Asset, Map);

			Mutex.lock();
			auto It = Streams.find(Docker->second->Stream);
			if (It == Streams.end())
			{
				Mutex.unlock();
				ED_ERR("[engine] cannot resolve stream offset for \"%s\"", Path.c_str());
				return nullptr;
			}

			auto* Stream = Docker->second->Stream;
			Stream->SetVirtualSize(Docker->second->Length);
			Stream->Seek(Core::FileSeek::Begin, It->second + Docker->second->Offset);
			Stream->GetSource() = File.R();
			Mutex.unlock();

			return Processor->Deserialize(Stream, It->second + Docker->second->Offset, Map);
		}
		bool ContentManager::SaveForward(const std::string& Path, Processor* Processor, void* Object, const Core::VariantArgs& Map)
		{
			ED_ASSERT(Object != nullptr, false, "object should be set");
			if (Path.empty())
				return false;

			if (!Processor)
			{
				ED_ERR("[engine] file processor for \"%s\" wasn't found", Path.c_str());
				return false;
			}

			Mutex.lock();
			std::string Directory = Core::OS::Path::GetDirectory(Path.c_str());
			std::string File = Core::OS::Path::Resolve(Directory, Environment);
			File.append(Path.substr(Directory.size()));
			Mutex.unlock();

			if (!File.empty())
				Core::OS::Directory::Patch(Core::OS::Path::GetDirectory(File.c_str()));
			else
				Core::OS::Directory::Patch(Core::OS::Path::GetDirectory(Path.c_str()));

			auto* Stream = Core::OS::File::Open(File, Core::FileMode::Binary_Write_Only);
			if (!Stream)
			{
				Stream = Core::OS::File::Open(Path, Core::FileMode::Binary_Write_Only);
				if (!Stream)
				{
					ED_ERR("[engine] cannot open stream for writing at \"%s\" or \"%s\"", File.c_str(), Path.c_str());
					return false;
				}
			}

			bool Result = Processor->Serialize(Stream, Object, Map);
			ED_RELEASE(Stream);

			return Result;
		}
		bool ContentManager::Import(const std::string& Path)
		{
			Mutex.lock();
			std::string File = Core::OS::Path::Resolve(Path, Environment);
			Mutex.unlock();

			if (File.empty())
			{
				ED_ERR("[engine] file \"%s\" wasn't found", Path.c_str());
				return false;
			}

			auto* Stream = new Core::GzStream();
			if (!Stream->Open(File.c_str(), Core::FileMode::Binary_Read_Only))
			{
				ED_ERR("[engine] cannot open \"%s\" for reading", File.c_str());
				ED_RELEASE(Stream);

				return false;
			}

			char Buffer[16];
			if (Stream->Read(Buffer, 16) != 16)
			{
				ED_ERR("[engine] file \"%s\" has corrupted header", File.c_str());
				ED_RELEASE(Stream);

				return false;
			}

			if (memcmp(Buffer, "\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16) != 0)
			{
				ED_ERR("[engine] file \"%s\" header version is corrupted", File.c_str());
				ED_RELEASE(Stream);

				return false;
			}

			uint64_t Size = 0;
			if (Stream->Read((char*)&Size, sizeof(uint64_t)) != sizeof(uint64_t))
			{
				ED_ERR("[engine] file \"%s\" has corrupted dock size", File.c_str());
				ED_RELEASE(Stream);

				return false;
			}

			Mutex.lock();
			for (uint64_t i = 0; i < Size; i++)
			{
				AssetArchive* Docker = ED_NEW(AssetArchive);
				Docker->Stream = Stream;

				uint64_t Length;
				Stream->Read((char*)&Length, sizeof(uint64_t));
				Stream->Read((char*)&Docker->Offset, sizeof(uint64_t));
				Stream->Read((char*)&Docker->Length, sizeof(uint64_t));

				if (!Length)
				{
					ED_DELETE(AssetArchive, Docker);
					continue;
				}

				Docker->Path.resize((size_t)Length);
				Stream->Read((char*)Docker->Path.c_str(), sizeof(char) * (size_t)Length);
				Dockers[Docker->Path] = Docker;
			}

			Streams[Stream] = Stream->Tell();
			Mutex.unlock();

			return true;
		}
		bool ContentManager::Export(const std::string& Path, const std::string& Directory, const std::string& Name)
		{
			ED_ASSERT(!Path.empty() && !Directory.empty(), false, "path and directory should not be empty");
			auto* Stream = new Core::GzStream();
			if (!Stream->Open(Core::OS::Path::Resolve(Path, Environment).c_str(), Core::FileMode::Write_Only))
			{
				ED_ERR("[engine] cannot open \"%s\" for writing", Path.c_str());
				ED_RELEASE(Stream);
				return false;
			}

			Mutex.lock();
			std::string DirectoryBase = Core::OS::Path::Resolve(Directory, Environment);
			Mutex.unlock();

			auto Tree = new Core::FileTree(DirectoryBase);
			Stream->Write("\0d\0o\0c\0k\0h\0e\0a\0d", sizeof(char) * 16);

			uint64_t Size = Tree->GetFiles();
			Stream->Write((char*)&Size, sizeof(uint64_t));

			uint64_t Offset = 0;
			Tree->Loop([Stream, &Offset, &DirectoryBase, &Name](const Core::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					auto* File = Core::OS::File::Open(Resource, Core::FileMode::Binary_Read_Only);
					if (!File)
						continue;

					std::string Path = Core::Parser(Resource).Replace(DirectoryBase, Name).Replace('\\', '/').R();
					if (Name.empty())
						Path.assign(Path.substr(1));

					uint64_t Size = (uint64_t)Path.size();
					uint64_t Length = File->GetSize();

					Stream->Write((char*)&Size, sizeof(uint64_t));
					Stream->Write((char*)&Offset, sizeof(uint64_t));
					Stream->Write((char*)&Length, sizeof(uint64_t));

					Offset += Length;
					if (Size > 0)
						Stream->Write((char*)Path.c_str(), sizeof(char) * (size_t)Size);

					ED_RELEASE(File);
				}

				return true;
			});
			Tree->Loop([Stream](const Core::FileTree* Tree)
			{
				for (auto& Resource : Tree->Files)
				{
					auto* File = Core::OS::File::Open(Resource, Core::FileMode::Binary_Read_Only);
					if (!File)
						continue;

					size_t Size = File->GetSize();
					while (Size > 0)
					{
						char Buffer[ED_BIG_CHUNK_SIZE];
						size_t Offset = File->Read(Buffer, Size > ED_BIG_CHUNK_SIZE ? ED_BIG_CHUNK_SIZE : Size);
						if (Offset <= 0)
							break;

						Stream->Write(Buffer, Offset);
						Size -= Offset;
					}

					ED_RELEASE(File);
				}

				return true;
			});

			ED_RELEASE(Tree);
			ED_RELEASE(Stream);

			return true;
		}
		bool ContentManager::Cache(Processor* Root, const std::string& Path, void* Resource)
		{
			if (Find(Root, Path) != nullptr)
				return false;

			Mutex.lock();
			AssetCache* Asset = ED_NEW(AssetCache);
			Asset->Path = Core::Parser(Path).Replace('\\', '/').Replace(Environment, "./").R();
			Asset->Resource = Resource;

			auto& Entries = Assets[Asset->Path];
			Entries[Root] = Asset;
			Mutex.unlock();

			return true;
		}
		bool ContentManager::HasAnyPointToDispatch()
		{
			auto* App = Application::Get();
			if (!App || App->Content != this)
				return true;

			if (!Application::Queue()->CanEnqueue())
				return false;

			auto State = App->GetState();
			if (State == ApplicationState::Staging || State == ApplicationState::Active)
				return true;

			return false;
		}
		bool ContentManager::IsBusy()
		{
			Mutex.lock();
			bool Busy = Queue > 0;
			Mutex.unlock();
			return Busy;
		}
		void ContentManager::Enqueue()
		{
			Mutex.lock();
			++Queue;
			Mutex.unlock();
		}
		void ContentManager::Dequeue()
		{
			Mutex.lock();
			--Queue;
			Mutex.unlock();
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
		Graphics::GraphicsDevice* ContentManager::GetDevice() const
		{
			return Device;
		}
		const std::string& ContentManager::GetEnvironment() const
		{
			return Environment;
		}

		AppData::AppData(ContentManager* Manager, const std::string& NewPath) noexcept : Content(Manager), Data(nullptr)
		{
			ED_ASSERT_V(Manager != nullptr, "content manager should be set");
			Migrate(NewPath);
		}
		AppData::~AppData() noexcept
		{
			ED_RELEASE(Data);
		}
		void AppData::Migrate(const std::string& Next)
		{
			ED_ASSERT_V(!Next.empty(), "path should not be empty");

			Safe.lock();
			if (Data != nullptr)
			{
				if (!Path.empty())
					Core::OS::File::Remove(Path.c_str());

				WriteAppData(Next);
			}
			else
				ReadAppData(Next);
			Path = Next;
			Safe.unlock();
		}
		void AppData::SetKey(const std::string& Name, Core::Schema* Value)
		{
			Safe.lock();
			if (!Data)
				Data = Core::Var::Set::Object();

			Data->Set(Name, Value);
			WriteAppData(Path);
			Safe.unlock();
		}
		void AppData::SetText(const std::string& Name, const std::string& Value)
		{
			SetKey(Name, Core::Var::Set::String(Value));
		}
		Core::Schema* AppData::GetKey(const std::string& Name)
		{
			Safe.lock();
			if (!ReadAppData(Path))
			{
				Safe.unlock();
				return nullptr;
			}

			Core::Schema* Result = Data->Get(Name);
			if (Result != nullptr)
				Result = Result->Copy();

			Safe.unlock();
			return Result;
		}
		std::string AppData::GetText(const std::string& Name)
		{
			Safe.lock();
			if (!ReadAppData(Path))
			{
				Safe.unlock();
				return nullptr;
			}

			Core::Variant Result = Data->GetVar(Name);
			Safe.unlock();
			return Result.GetBlob();
		}
		bool AppData::Has(const std::string& Name)
		{
			Safe.lock();
			if (!ReadAppData(Path))
			{
				Safe.unlock();
				return false;
			}

			bool Result = Data->Has(Name);
			Safe.unlock();
			return Result;
		}
		bool AppData::ReadAppData(const std::string& Next)
		{
			if (Data != nullptr)
				return true;

			if (Next.empty())
				return false;

			Data = Content->Load<Core::Schema>(Next);
			return Data != nullptr;
		}
		bool AppData::WriteAppData(const std::string& Next)
		{
			if (Next.empty() || !Data)
				return false;

			const char* TypeId = Core::OS::Path::GetExtension(Next.c_str());
			std::string Type = "JSONB";

			if (TypeId != nullptr)
			{
				Type = Core::Parser(TypeId).ToUpper().R();
				if (Type != "JSON" && Type != "JSONB" && Type != "XML")
					Type = "JSONB";
			}

			Core::VariantArgs Args;
			Args["type"] = Core::Var::String(Type);

			return Content->Save<Core::Schema>(Next, Data, Args);
		}

		Application::Application(Desc* I) noexcept : Control(I ? *I : Desc())
		{
			ED_ASSERT_V(I != nullptr, "desc should be set");
			Host = this;

			if (I->Usage & (size_t)ApplicationSet::ContentSet)
			{
				Content = new ContentManager(nullptr);
				Content->AddProcessor<Processors::Asset, Engine::AssetFile>();
				Content->AddProcessor<Processors::Material, Engine::Material>();
				Content->AddProcessor<Processors::SceneGraph, Engine::SceneGraph>();
				Content->AddProcessor<Processors::AudioClip, Audio::AudioClip>();
				Content->AddProcessor<Processors::Texture2D, Graphics::Texture2D>();
				Content->AddProcessor<Processors::Shader, Graphics::Shader>();
				Content->AddProcessor<Processors::Model, Graphics::Model>();
				Content->AddProcessor<Processors::SkinModel, Graphics::SkinModel>();
				Content->AddProcessor<Processors::Schema, Core::Schema>();
				Content->AddProcessor<Processors::Server, Network::HTTP::Server>();
				Content->AddProcessor<Processors::HullShape, Compute::HullShape>();
				Content->SetEnvironment(I->Environment.empty() ? Core::OS::Directory::Get() + I->Directory : I->Environment + I->Directory);

				if (!I->Preferences.empty())
				{
					std::string Path = Core::OS::Path::Resolve(I->Preferences, Content->GetEnvironment());
					Database = new AppData(Content, Path);
				}
			}
#ifdef ED_HAS_SDL2
			if (I->Usage & (size_t)ApplicationSet::ActivitySet)
			{
				if (!I->Activity.Width || !I->Activity.Height)
				{
					SDL_DisplayMode Display;
					SDL_GetCurrentDisplayMode(0, &Display);
					I->Activity.Width = (unsigned int)(Display.w / 1.1);
					I->Activity.Height = (unsigned int)(Display.h / 1.2);
				}

				if (I->Activity.Width > 0 && I->Activity.Height > 0)
				{
					bool Maximized = I->Activity.Maximized;
					I->Activity.AllowGraphics = (I->Usage & (size_t)ApplicationSet::GraphicsSet);
					I->Activity.Maximized = false;

					Activity = new Graphics::Activity(I->Activity);
					if (I->Activity.AllowGraphics)
					{
						if (!I->GraphicsDevice.BufferWidth)
							I->GraphicsDevice.BufferWidth = I->Activity.Width;

						if (!I->GraphicsDevice.BufferHeight)
							I->GraphicsDevice.BufferHeight = I->Activity.Height;

						I->GraphicsDevice.Window = Activity;
						if (Content != nullptr && !I->GraphicsDevice.CacheDirectory.empty())
							I->GraphicsDevice.CacheDirectory = Core::OS::Path::ResolveDirectory(I->GraphicsDevice.CacheDirectory, Content->GetEnvironment());

						Renderer = Graphics::GraphicsDevice::Create(I->GraphicsDevice);
						if (!Renderer || !Renderer->IsValid())
						{
							ED_ERR("[engine] graphics device cannot be created");
							return;
						}

						Compute::Geometric::SetLeftHanded(Renderer->IsLeftHanded());
						if (Content != nullptr)
							Content->SetDevice(Renderer);

						Cache.Shaders = new ShaderCache(Renderer);
						Cache.Primitives = new PrimitiveCache(Renderer);
					}
					else if (!Activity->GetHandle())
					{
						ED_ERR("[engine] cannot create activity instance");
						return;
					}

					Activity->UserPointer = this;
					Activity->SetCursorVisibility(I->Cursor);
					Activity->Callbacks.KeyState = [this](Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
					{
#ifdef ED_USE_RMLUI
						GUI::Context* GUI = GetGUI();
						if (GUI != nullptr)
							GUI->EmitKey(Key, Mod, Virtual, Repeat, Pressed);
#endif
						KeyEvent(Key, Mod, Virtual, Repeat, Pressed);
					};
					Activity->Callbacks.Input = [this](char* Buffer, int Length)
					{
#ifdef ED_USE_RMLUI
						GUI::Context* GUI = GetGUI();
						if (GUI != nullptr)
							GUI->EmitInput(Buffer, Length);
#endif
						InputEvent(Buffer, Length);
					};
					Activity->Callbacks.CursorWheelState = [this](int X, int Y, bool Normal)
					{
#ifdef ED_USE_RMLUI
						GUI::Context* GUI = GetGUI();
						if (GUI != nullptr)
							GUI->EmitWheel(X, Y, Normal, Activity->GetKeyModState());
#endif
						WheelEvent(X, Y, Normal);
					};
					Activity->Callbacks.WindowStateChange = [this](Graphics::WindowState NewState, int X, int Y)
					{
#ifdef ED_USE_RMLUI
						if (NewState == Graphics::WindowState::Resize)
						{
							GUI::Context* GUI = GetGUI();
							if (GUI != nullptr)
								GUI->EmitResize(X, Y);
						}
#endif
						WindowEvent(NewState, X, Y);
					};
					I->Activity.Maximized = Maximized;
				}
				else
					ED_ERR("[engine] cannot detect display to create activity");
			}
#endif
			if (I->Usage & (size_t)ApplicationSet::AudioSet)
			{
				Audio = new Audio::AudioDevice();
				if (!Audio->IsValid())
				{
					ED_ERR("[engine] audio device cannot be created");
					return;
				}
			}

			if (I->Usage & (size_t)ApplicationSet::ScriptSet)
				VM = new Script::VMManager();
#ifdef ED_USE_RMLUI
			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
			{
				GUI::Subsystem::SetMetadata(Activity, Content, nullptr);
				GUI::Subsystem::SetManager(VM);
			}
#endif
			if (I->Usage & (size_t)ApplicationSet::NetworkSet)
				Network::Multiplexer::Create(I->PollingTimeout, I->PollingEvents);

			State = ApplicationState::Staging;
		}
		Application::~Application() noexcept
		{
			if (Renderer != nullptr)
				Renderer->FlushState();

			ED_RELEASE(Scene);
			ED_RELEASE(VM);
			ED_RELEASE(Audio);
#ifdef ED_USE_RMLUI
			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
				Engine::GUI::Subsystem::Release();
#endif
			ED_RELEASE(Cache.Shaders);
			ED_RELEASE(Cache.Primitives);
			ED_RELEASE(Content);
			ED_RELEASE(Renderer);
			ED_RELEASE(Activity);

			if (Control.Usage & (size_t)ApplicationSet::NetworkSet)
				Network::Multiplexer::Release();

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
		void Application::ComposeEvent()
		{
		}
		void Application::Dispatch(Core::Timer* Time)
		{
		}
		void Application::Publish(Core::Timer* Time)
		{
		}
		void Application::Initialize()
		{
		}
		int Application::Start()
		{
			ComposeEvent();
			Compose();

			if (Control.Usage & (size_t)ApplicationSet::ActivitySet && !Activity)
			{
				ED_ERR("[engine] activity was not found");
				return ED_EXIT_JUMP + 1;
			}

			if (Control.Usage & (size_t)ApplicationSet::GraphicsSet && !Renderer)
			{
				ED_ERR("[engine] graphics device was not found");
				return ED_EXIT_JUMP + 2;
			}

			if (Control.Usage & (size_t)ApplicationSet::AudioSet && !Audio)
			{
				ED_ERR("[engine] audio device was not found");
				return ED_EXIT_JUMP + 3;
			}

			if (Control.Usage & (size_t)ApplicationSet::ScriptSet)
			{
				if (!VM)
				{
					ED_ERR("[engine] vm was not found");
					return ED_EXIT_JUMP + 4;
				}
				else
					ScriptHook(&VM->Global());
			}

			Initialize();
			if (State == ApplicationState::Terminated)
				return ExitCode != 0 ? ExitCode : ED_EXIT_JUMP + 5;

			State = ApplicationState::Active;
			if (!Control.Threads)
			{
				auto Quantity = Core::OS::CPU::GetQuantityInfo();
				Control.Threads = std::max<uint32_t>(2, Quantity.Logical) - 1;
			}
		
			Core::Timer* Time = new Core::Timer();
			Time->SetFixedFrames(Control.Framerate.Stable);
			Time->SetMaxFrames(Control.Framerate.Limit);
#ifdef ED_USE_RMLUI
			if (Activity != nullptr && Renderer != nullptr && Content != nullptr)
				GUI::Subsystem::SetMetadata(Activity, Content, Time);
#endif
			Core::Schedule::Desc Policy;
			Policy.Coroutines = Control.Coroutines;
			Policy.Memory = Control.Stack;
			Policy.Parallel = Control.Parallel;
			Policy.Ping = Control.Daemon ? std::bind(&Application::Status, this) : (Core::ActivityCallback)nullptr;
			Policy.SetThreads(Control.Threads);

			Core::Schedule* Queue = Core::Schedule::Get();
			Queue->Start(Policy);

			if (Activity != nullptr)
			{
				Activity->Show();
				if (Control.Activity.Maximized)
					Activity->Maximize();
			}

			ED_MEASURE(ED_TIMING_INFINITE);
			if (Activity != nullptr && Control.Parallel)
			{
				while (State == ApplicationState::Active)
				{
					Activity->Dispatch();

					Time->Begin();
					Dispatch(Time);

					Time->Finish();
					Publish(Time);
				}
			}
			else if (Activity != nullptr && !Control.Parallel)
			{
				while (State == ApplicationState::Active)
				{
					Activity->Dispatch();
					Queue->Dispatch();

					Time->Begin();
					Dispatch(Time);

					Time->Finish();
					Publish(Time);
				}
			}
			else if (!Activity && Control.Parallel)
			{
				while (State == ApplicationState::Active)
				{
					Time->Begin();
					Dispatch(Time);

					Time->Finish();
					Publish(Time);
				}
			}
			else if (!Activity && !Control.Parallel)
			{
				while (State == ApplicationState::Active)
				{
					Queue->Dispatch();

					Time->Begin();
					Dispatch(Time);

					Time->Finish();
					Publish(Time);
				}
			}

			while (Control.Parallel && Content != nullptr && Content->IsBusy())
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

			ED_RELEASE(Time);
			CloseEvent();
			Queue->Stop();

			ExitCode = (State == ApplicationState::Restart ? ED_EXIT_RESTART : ExitCode);
			State = ApplicationState::Terminated;
			return ExitCode;
		}
		void Application::Stop(int Code)
		{
			Core::Schedule* Queue = Core::Schedule::Get();
			State = ApplicationState::Terminated;
			ExitCode = Code;
			Queue->Wakeup();
		}
		void Application::Restart()
		{
			Core::Schedule* Queue = Core::Schedule::Get();
			State = ApplicationState::Restart;
			Queue->Wakeup();
		}
		GUI::Context* Application::GetGUI() const
		{
#ifdef ED_USE_RMLUI
			if (!Scene)
				return nullptr;

			auto* Viewer = (Components::Camera*)Scene->GetCamera();
			if (!Viewer)
				return nullptr;

			Renderers::UserInterface* Result = Viewer->GetRenderer()->GetRenderer<Renderers::UserInterface>();
			return Result != nullptr ? Result->GetContext() : nullptr;
#else
			return nullptr;
#endif
		}
		ApplicationState Application::GetState() const
		{
			return State;
		}
		bool Application::Status(Application* App)
		{
			return App->State == ApplicationState::Active;
		}
		void Application::Compose()
		{
			static bool WasComposed = false;
			if (WasComposed)
				return;
			else
				WasComposed = true;

			uint64_t AsComponent = (uint64_t)ComposerTag::Component;
			Core::Composer::Push<Components::RigidBody, Entity*>(AsComponent);
			Core::Composer::Push<Components::SoftBody, Entity*>(AsComponent);
			Core::Composer::Push<Components::Acceleration, Entity*>(AsComponent);
			Core::Composer::Push<Components::SliderConstraint, Entity*>(AsComponent);
			Core::Composer::Push<Components::Model, Entity*>(AsComponent);
			Core::Composer::Push<Components::Skin, Entity*>(AsComponent);
			Core::Composer::Push<Components::Emitter, Entity*>(AsComponent);
			Core::Composer::Push<Components::Decal, Entity*>(AsComponent);
			Core::Composer::Push<Components::SkinAnimator, Entity*>(AsComponent);
			Core::Composer::Push<Components::KeyAnimator, Entity*>(AsComponent);
			Core::Composer::Push<Components::EmitterAnimator, Entity*>(AsComponent);
			Core::Composer::Push<Components::FreeLook, Entity*>(AsComponent);
			Core::Composer::Push<Components::Fly, Entity*>(AsComponent);
			Core::Composer::Push<Components::AudioSource, Entity*>(AsComponent);
			Core::Composer::Push<Components::AudioListener, Entity*>(AsComponent);
			Core::Composer::Push<Components::PointLight, Entity*>(AsComponent);
			Core::Composer::Push<Components::SpotLight, Entity*>(AsComponent);
			Core::Composer::Push<Components::LineLight, Entity*>(AsComponent);
			Core::Composer::Push<Components::SurfaceLight, Entity*>(AsComponent);
			Core::Composer::Push<Components::Illuminator, Entity*>(AsComponent);
			Core::Composer::Push<Components::Camera, Entity*>(AsComponent);
			Core::Composer::Push<Components::Scriptable, Entity*>(AsComponent);

			uint64_t AsRenderer = (uint64_t)ComposerTag::Renderer;
			Core::Composer::Push<Renderers::SoftBody, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Model, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Skin, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Emitter, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Decal, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Lighting, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Transparency, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Glitch, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Tone, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::DoF, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::Bloom, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::SSR, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::SSGI, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::SSAO, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::MotionBlur, RenderSystem*>(AsRenderer);
			Core::Composer::Push<Renderers::UserInterface, RenderSystem*>(AsRenderer);

			uint64_t AsEffect = (uint64_t)ComposerTag::Effect;
			Core::Composer::Push<Audio::Effects::Reverb>(AsEffect);
			Core::Composer::Push<Audio::Effects::Chorus>(AsEffect);
			Core::Composer::Push<Audio::Effects::Distortion>(AsEffect);
			Core::Composer::Push<Audio::Effects::Echo>(AsEffect);
			Core::Composer::Push<Audio::Effects::Flanger>(AsEffect);
			Core::Composer::Push<Audio::Effects::FrequencyShifter>(AsEffect);
			Core::Composer::Push<Audio::Effects::VocalMorpher>(AsEffect);
			Core::Composer::Push<Audio::Effects::PitchShifter>(AsEffect);
			Core::Composer::Push<Audio::Effects::RingModulator>(AsEffect);
			Core::Composer::Push<Audio::Effects::Autowah>(AsEffect);
			Core::Composer::Push<Audio::Effects::Compressor>(AsEffect);
			Core::Composer::Push<Audio::Effects::Equalizer>(AsEffect);

			uint64_t AsFilter = (uint64_t)ComposerTag::Filter;
			Core::Composer::Push<Audio::Filters::Lowpass>(AsFilter);
			Core::Composer::Push<Audio::Filters::Bandpass>(AsFilter);
			Core::Composer::Push<Audio::Filters::Highpass>(AsFilter);
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

		EffectRenderer::EffectRenderer(RenderSystem* Lab) noexcept : Renderer(Lab), Output(nullptr), Swap(nullptr), MaxSlot(0)
		{
			ED_ASSERT_V(Lab != nullptr, "render system should be set");
			ED_ASSERT_V(Lab->GetDevice() != nullptr, "graphics device should be set");

			auto* Device = Lab->GetDevice();
			DepthStencil = Device->GetDepthStencilState("none");
			Rasterizer = Device->GetRasterizerState("cull-back");
			Blend = Device->GetBlendState("overwrite-opaque");
			Sampler = Device->GetSamplerState("trilinear-x16");
			Layout = Device->GetInputLayout("shape-vertex");
		}
		EffectRenderer::~EffectRenderer() noexcept
		{
			for (auto It = Effects.begin(); It != Effects.end(); ++It)
				System->FreeShader(It->first, It->second);
		}
		void EffectRenderer::ResizeBuffers()
		{
			Output = nullptr;
			ResizeEffect();
		}
		void EffectRenderer::ResizeEffect()
		{
		}
		void EffectRenderer::RenderCopyMain(uint32_t Slot, Graphics::Texture2D* Target)
		{
			ED_ASSERT_V(Target != nullptr, "texture should be set");
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->CopyTexture2D(System->GetScene()->GetMRT(TargetType::Main), Slot, &Target);
		}
		void EffectRenderer::RenderCopyLast(Graphics::Texture2D* Target)
		{
			ED_ASSERT_V(Target != nullptr, "texture should be set");
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->CopyTexture2D(Output, 0, &Target);
		}
		void EffectRenderer::RenderOutput(Graphics::RenderTarget2D* Resource)
		{
			ED_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");
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
		void EffectRenderer::RenderTexture(uint32_t Slot6, Graphics::Texture2D* Resource)
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture2D(Resource, 6 + Slot6, ED_PS);

			if (Resource != nullptr)
				MaxSlot = std::max(MaxSlot, 6 + Slot6);
		}
		void EffectRenderer::RenderTexture(uint32_t Slot6, Graphics::Texture3D* Resource)
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTexture3D(Resource, 6 + Slot6, ED_PS);

			if (Resource != nullptr)
				MaxSlot = std::max(MaxSlot, 6 + Slot6);
		}
		void EffectRenderer::RenderTexture(uint32_t Slot6, Graphics::TextureCube* Resource)
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetTextureCube(Resource, 6 + Slot6, ED_PS);

			if (Resource != nullptr)
				MaxSlot = std::max(MaxSlot, 6 + Slot6);
		}
		void EffectRenderer::RenderMerge(Graphics::Shader* Effect, void* Buffer, size_t Count)
		{
			ED_ASSERT_V(Count > 0, "count should be greater than zero");
			if (!Effect)
				Effect = Effects.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();

			if (Swap != nullptr && Output != Swap)
				Device->SetTexture2D(Swap->GetTarget(), 5, ED_PS);
			else if (Merger != nullptr)
				Device->SetTexture2D(*Merger, 5, ED_PS);

			Device->SetShader(Effect, ED_VS | ED_PS);
			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, ED_VS | ED_PS);
			}

			for (size_t i = 0; i < Count; i++)
			{
				Device->Draw(6, 0);
				if (!Swap)
					Device->CopyTexture2D(Output, 0, Merger);
			}

			auto* Scene = System->GetScene();
			Scene->Statistics.DrawCalls += Count;

			if (Swap == Output)
				RenderOutput();
		}
		void EffectRenderer::RenderResult(Graphics::Shader* Effect, void* Buffer)
		{
			if (!Effect)
				Effect = Effects.begin()->second;

			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();

			if (Swap != nullptr && Output != Swap)
				Device->SetTexture2D(Swap->GetTarget(), 5, ED_PS);
			else if (Merger != nullptr)
				Device->SetTexture2D(*Merger, 5, ED_PS);

			Device->SetShader(Effect, ED_VS | ED_PS);
			if (Buffer != nullptr)
			{
				Device->UpdateBuffer(Effect, Buffer);
				Device->SetBuffer(Effect, 3, ED_VS | ED_PS);
			}

			Device->Draw(6, 0);
			Output = System->GetRT(TargetType::Main);

			auto* Scene = System->GetScene();
			Scene->Statistics.DrawCalls++;
		}
		void EffectRenderer::RenderResult()
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();

			if (Swap != nullptr && Output != Swap)
				Device->SetTexture2D(Swap->GetTarget(), 1, ED_PS);
			else if (Merger != nullptr)
				Device->SetTexture2D(*Merger, 1, ED_PS);

			Device->SetShader(Device->GetBasicEffect(), ED_VS | ED_PS);
			Device->Draw(6, 0);
			Output = System->GetRT(TargetType::Main);

			auto* Scene = System->GetScene();
			Scene->Statistics.DrawCalls++;
		}
		void EffectRenderer::RenderEffect(Core::Timer* Time)
		{
		}
		void EffectRenderer::GenerateMips()
		{
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Graphics::Texture2D** Merger = System->GetMerger();

			if (Swap != nullptr && Output != Swap)
				Device->GenerateMips(Swap->GetTarget());
			else if (Merger != nullptr)
				Device->GenerateMips(*Merger);
		}
		size_t EffectRenderer::RenderPass(Core::Timer* Time)
		{
			ED_ASSERT(System->GetPrimitives() != nullptr, 0, "primitive cache should be set");
			ED_ASSERT(System->GetMRT(TargetType::Main) != nullptr, 0, "main render target should be set");
			ED_MEASURE(ED_TIMING_FRAME);

			if (!System->State.Is(RenderState::Geometry_Result) || System->State.IsSubpass())
				return 0;

			MaxSlot = 5;
			if (Effects.empty())
				return 0;

			Swap = nullptr;
			if (!Output)
				Output = System->GetRT(TargetType::Main);

			Graphics::MultiRenderTarget2D* Input = System->GetMRT(TargetType::Main);
			PrimitiveCache* Cache = System->GetPrimitives();
			Graphics::GraphicsDevice* Device = System->GetDevice();
			Device->SetDepthStencilState(DepthStencil);
			Device->SetBlendState(Blend);
			Device->SetRasterizerState(Rasterizer);
			Device->SetInputLayout(Layout);
			Device->SetTarget(Output, 0, 0, 0, 0);
			Device->SetSamplerState(Sampler, 1, MaxSlot, ED_PS);
			Device->SetTexture2D(Input->GetTarget(0), 1, ED_PS);
			Device->SetTexture2D(Input->GetTarget(1), 2, ED_PS);
			Device->SetTexture2D(Input->GetTarget(2), 3, ED_PS);
			Device->SetTexture2D(Input->GetTarget(3), 4, ED_PS);
			Device->SetVertexBuffer(Cache->GetQuad());

			RenderEffect(Time);

			Device->FlushTexture(1, MaxSlot, ED_PS);
			Device->CopyTarget(Output, 0, Input, 0);
			System->RestoreOutput();
			return 1;
		}
		Graphics::Shader* EffectRenderer::GetEffect(const std::string& Name)
		{
			auto It = Effects.find(Name);
			if (It != Effects.end())
				return It->second;

			return nullptr;
		}
		Graphics::Shader* EffectRenderer::CompileEffect(Graphics::Shader::Desc& Desc, size_t BufferSize)
		{
			ED_ASSERT(!Desc.Filename.empty(), nullptr, "cannot compile unnamed shader source");
			Graphics::Shader* Shader = System->CompileShader(Desc, BufferSize);
			if (!Shader)
				return nullptr;

			auto It = Effects.find(Desc.Filename);
			if (It != Effects.end())
			{
				ED_RELEASE(It->second);
				It->second = Shader;
			}
			else
				Effects[Desc.Filename] = Shader;

			return Shader;
		}
		Graphics::Shader* EffectRenderer::CompileEffect(const std::string& SectionName, size_t BufferSize)
		{
			Graphics::Shader::Desc I = Graphics::Shader::Desc();
			if (!System->GetDevice()->GetSection(SectionName, &I))
				return nullptr;

			return CompileEffect(I, BufferSize);
		}
		unsigned int EffectRenderer::GetMipLevels() const
		{
			ED_ASSERT(System->GetRT(TargetType::Main) != nullptr, 0, "main render target should be set");
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Main);
			return System->GetDevice()->GetMipLevel(RT->GetWidth(), RT->GetHeight());
		}
		unsigned int EffectRenderer::GetWidth() const
		{
			ED_ASSERT(System->GetRT(TargetType::Main) != nullptr, 0, "main render target should be set");
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Main);
			return RT->GetWidth();
		}
		unsigned int EffectRenderer::GetHeight() const
		{
			ED_ASSERT(System->GetRT(TargetType::Main) != nullptr, 0, "main render target should be set");
			Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Main);
			return RT->GetHeight();
		}
	}
}
