#ifndef TH_COMPUTE_H
#define TH_COMPUTE_H
#include "core.h"
#include <cmath>
#include <map>
#include <stack>

class btCollisionConfiguration;
class btBroadphaseInterface;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionDispatcher;
class btSoftBodySolver;
class btRigidBody;
class btSoftBody;
class btTypedConstraint;
class btPoint2PointConstraint;
class btHingeConstraint;
class btSliderConstraint;
class btConeTwistConstraint;
class btGeneric6DofSpring2Constraint;
class btTransform;
class btCollisionObject;
class btBoxShape;
class btSphereShape;
class btCapsuleShape;
class btConeShape;
class btCylinderShape;
class btCollisionShape;
class btVector3;
typedef bool(* ContactDestroyedCallback)(void*);
typedef bool(* ContactProcessedCallback)(class btManifoldPoint&, void*, void*);
typedef void(* ContactStartedCallback)(class btPersistentManifold* const&);
typedef void(* ContactEndedCallback)(class btPersistentManifold* const&);

namespace Tomahawk
{
	namespace Compute
	{
		class RigidBody;

		class SoftBody;

		class Simulator;

		class WebToken;

		class Transform;

		struct Matrix4x4;

		struct Vector2;

		struct Vector3;

		struct Vector4;

		typedef std::function<void(class FiniteState*)> ActionCallback;
		typedef std::function<bool(class Preprocessor*, const struct IncludeResult& File, std::string* Out)> ProcIncludeCallback;
		typedef std::function<bool(class Preprocessor*, const std::string& Name, const std::vector<std::string>& Args)> ProcPragmaCallback;
		typedef std::function<void(const struct CollisionBody&)> CollisionCallback;
		typedef void* Cipher;
		typedef void* Digest;

		enum class Hybi10_Opcode
		{
			Text,
			Close,
			Pong,
			Ping,
			Invalid
		};

		enum class Shape
		{
			Box,
			Triangle,
			Tetrahedral,
			Convex_Triangle_Mesh,
			Convex_Hull,
			Convex_Point_Cloud,
			Convex_Polyhedral,
			Implicit_Convex_Start,
			Sphere,
			Multi_Sphere,
			Capsule,
			Cone,
			Convex,
			Cylinder,
			Uniform_Scaling,
			Minkowski_Sum,
			Minkowski_Difference,
			Box_2D,
			Convex_2D,
			Custom_Convex,
			Concaves_Start,
			Triangle_Mesh,
			Triangle_Mesh_Scaled,
			Fast_Concave_Mesh,
			Terrain,
			Gimpact,
			Triangle_Mesh_Multimaterial,
			Empty,
			Static_Plane,
			Custom_Concave,
			Concaves_End,
			Compound,
			Softbody,
			HF_Fluid,
			HF_Fluid_Bouyant_Convex,
			Invalid,
			Count
		};

		enum class MotionState
		{
			Active = 1,
			Island_Sleeping = 2,
			Deactivation_Needed = 3,
			Disable_Deactivation = 4,
			Disable_Simulation = 5,
		};

		enum class RegexState
		{
			Preprocessed = 0,
			Match_Found = -1,
			No_Match = -2,
			Unexpected_Quantifier = -3,
			Unbalanced_Brackets = -4,
			Internal_Error = -5,
			Invalid_Character_Set = -6,
			Invalid_Metacharacter = -7,
			Sumatch_Array_Too_Small = -8,
			Too_Many_Branches = -9,
			Too_Many_Brackets = -10,
		};

		enum class SoftFeature
		{
			None,
			Node,
			Link,
			Face,
			Tetra
		};

		enum class SoftAeroModel
		{
			VPoint,
			VTwoSided,
			VTwoSidedLiftDrag,
			VOneSided,
			FTwoSided,
			FTwoSidedLiftDrag,
			FOneSided
		};

		enum class SoftCollision
		{
			RVS_Mask = 0x000f,
			SDF_RS = 0x0001,
			CL_RS = 0x0002,
			SDF_RD = 0x0003,
			SDF_RDF = 0x0004,
			SVS_Mask = 0x00F0,
			VF_SS = 0x0010,
			CL_SS = 0x0020,
			CL_Self = 0x0040,
			VF_DD = 0x0050,
			Default = SDF_RS | CL_SS
		};

		enum class Rotator
		{
			XYZ = 0,
			XZY,
			YXZ,
			YZX,
			ZXY,
			ZYX
		};

		enum class Positioning
		{
			Local,
			Global
		};

		inline SoftCollision operator |(SoftCollision A, SoftCollision B)
		{
			return static_cast<SoftCollision>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		struct TH_OUT IncludeDesc
		{
			std::vector<std::string> Exts;
			std::string From;
			std::string Path;
			std::string Root;
		};

		struct TH_OUT IncludeResult
		{
			std::string Module;
			bool IsSystem = false;
			bool IsFile = false;
		};

		struct TH_OUT Vertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float TexCoordX;
			float TexCoordY;
			float NormalX;
			float NormalY;
			float NormalZ;
			float TangentX;
			float TangentY;
			float TangentZ;
			float BitangentX;
			float BitangentY;
			float BitangentZ;
		};

		struct TH_OUT SkinVertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float TexCoordX;
			float TexCoordY;
			float NormalX;
			float NormalY;
			float NormalZ;
			float TangentX;
			float TangentY;
			float TangentZ;
			float BitangentX;
			float BitangentY;
			float BitangentZ;
			float JointIndex0;
			float JointIndex1;
			float JointIndex2;
			float JointIndex3;
			float JointBias0;
			float JointBias1;
			float JointBias2;
			float JointBias3;
		};

		struct TH_OUT ShapeVertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float TexCoordX;
			float TexCoordY;
		};

		struct TH_OUT ElementVertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float VelocityX;
			float VelocityY;
			float VelocityZ;
			float ColorX;
			float ColorY;
			float ColorZ;
			float ColorW;
			float Scale;
			float Rotation;
			float Angular;
		};

		struct TH_OUT Rectangle
		{
			int64_t Left;
			int64_t Top;
			int64_t Right;
			int64_t Bottom;
		};

		struct TH_OUT Vector2
		{
			float X;
			float Y;

			Vector2();
			Vector2(float x, float y);
			Vector2(float xy);
			Vector2(const Vector2& Value);
			Vector2(const Vector3& Value);
			Vector2(const Vector4& Value);
			float Length() const;
			float Sum() const;
			float Dot(const Vector2& B) const;
			float Distance(const Vector2& Target) const;
			float Hypotenuse() const;
			float LookAt(const Vector2& At) const;
			float Cross(const Vector2& Vector1) const;
			Vector2 Transform(const Matrix4x4& V) const;
			Vector2 Direction(float Rotation) const;
			Vector2 Inv() const;
			Vector2 InvX() const;
			Vector2 InvY() const;
			Vector2 Normalize() const;
			Vector2 sNormalize() const;
			Vector2 Lerp(const Vector2& B, float DeltaTime) const;
			Vector2 sLerp(const Vector2& B, float DeltaTime) const;
			Vector2 aLerp(const Vector2& B, float DeltaTime) const;
			Vector2 rLerp() const;
			Vector2 Abs() const;
			Vector2 Radians() const;
			Vector2 Degrees() const;
			Vector2 XY() const;
			Vector3 XYZ() const;
			Vector4 XYZW() const;
			Vector2 Mul(float xy) const;
			Vector2 Mul(float X, float Y) const;
			Vector2 Mul(const Vector2& Value) const;
			Vector2 Div(const Vector2& Value) const;
			Vector2 Add(const Vector2& Value) const;
			Vector2 SetX(float X) const;
			Vector2 SetY(float Y) const;
			void Set(const Vector2& Value);
			void Get2(float* In) const;
			Vector2& operator *=(const Vector2& V);
			Vector2& operator *=(float V);
			Vector2& operator /=(const Vector2& V);
			Vector2& operator /=(float V);
			Vector2& operator +=(const Vector2& V);
			Vector2& operator +=(float V);
			Vector2& operator -=(const Vector2& V);
			Vector2& operator -=(float V);
			Vector2 operator *(const Vector2& V) const;
			Vector2 operator *(float V) const;
			Vector2 operator /(const Vector2& V) const;
			Vector2 operator /(float V) const;
			Vector2 operator +(const Vector2& V) const;
			Vector2 operator +(float V) const;
			Vector2 operator -(const Vector2& V) const;
			Vector2 operator -(float V) const;
			Vector2 operator -() const;
			Vector2& operator =(const Vector2& V);
			bool operator ==(const Vector2& V) const;
			bool operator !=(const Vector2& V) const;
			bool operator <=(const Vector2& V) const;
			bool operator >=(const Vector2& V) const;
			bool operator >(const Vector2& V) const;
			bool operator <(const Vector2& V) const;
			float& operator [](uint32_t Axis);
			float operator [](uint32_t Axis) const;

			static Vector2 Random();
			static Vector2 RandomAbs();
			static Vector2 One()
			{
				return Vector2(1, 1);
			}
			static Vector2 Zero()
			{
				return Vector2(0, 0);
			}
			static Vector2 Up()
			{
				return Vector2(0, 1);
			}
			static Vector2 Down()
			{
				return Vector2(0, -1);
			}
			static Vector2 Left()
			{
				return Vector2(-1, 0);
			}
			static Vector2 Right()
			{
				return Vector2(1, 0);
			}
		};

		struct TH_OUT Vector3
		{
			float X;
			float Y;
			float Z;

			Vector3();
			Vector3(const Vector2& Value);
			Vector3(const Vector3& Value);
			Vector3(const Vector4& Value);
			Vector3(float x, float y);
			Vector3(float x, float y, float z);
			Vector3(float xyz);
			float Length() const;
			float Sum() const;
			float Dot(const Vector3& B) const;
			float Distance(const Vector3& Target) const;
			float Hypotenuse() const;
			float LookAtXY(const Vector3& At) const;
			float LookAtXZ(const Vector3& At) const;
			Vector3 Cross(const Vector3& Vector) const;
			Vector3 Transform(const Matrix4x4& V) const;
			Vector3 hDirection() const;
			Vector3 dDirection() const;
			Vector3 Direction() const;
			Vector3 Inv() const;
			Vector3 InvX() const;
			Vector3 InvY() const;
			Vector3 InvZ() const;
			Vector3 Normalize() const;
			Vector3 sNormalize() const;
			Vector3 Lerp(const Vector3& B, float DeltaTime) const;
			Vector3 sLerp(const Vector3& B, float DeltaTime) const;
			Vector3 aLerp(const Vector3& B, float DeltaTime) const;
			Vector3 rLerp() const;
			Vector3 Abs() const;
			Vector3 Radians() const;
			Vector3 Degrees() const;
			Vector2 XY() const;
			Vector3 XYZ() const;
			Vector4 XYZW() const;
			Vector3 Mul(float xyz) const;
			Vector3 Mul(const Vector2& XY, float Z) const;
			Vector3 Mul(const Vector3& Value) const;
			Vector3 Div(const Vector3& Value) const;
			Vector3 Add(const Vector3& Value) const;
			Vector3 SetX(float X) const;
			Vector3 SetY(float Y) const;
			Vector3 SetZ(float Z) const;
			void Set(const Vector3& Value);
			void Get2(float* In) const;
			void Get3(float* In) const;
			Vector3& operator *=(const Vector3& V);
			Vector3& operator *=(float V);
			Vector3& operator /=(const Vector3& V);
			Vector3& operator /=(float V);
			Vector3& operator +=(const Vector3& V);
			Vector3& operator +=(float V);
			Vector3& operator -=(const Vector3& V);
			Vector3& operator -=(float V);
			Vector3 operator *(const Vector3& V) const;
			Vector3 operator *(float V) const;
			Vector3 operator /(const Vector3& V) const;
			Vector3 operator /(float V) const;
			Vector3 operator +(const Vector3& V) const;
			Vector3 operator +(float V) const;
			Vector3 operator -(const Vector3& V) const;
			Vector3 operator -(float V) const;
			Vector3 operator -() const;
			Vector3& operator =(const Vector3& V);
			bool operator ==(const Vector3& V) const;
			bool operator !=(const Vector3& V) const;
			bool operator <=(const Vector3& V) const;
			bool operator >=(const Vector3& V) const;
			bool operator >(const Vector3& V) const;
			bool operator <(const Vector3& V) const;
			float& operator [](uint32_t Axis);
			float operator [](uint32_t Axis) const;
			static Vector3 Random();
			static Vector3 RandomAbs();
			static Vector3 One()
			{
				return Vector3(1, 1, 1);
			}
			static Vector3 Zero()
			{
				return Vector3(0, 0, 0);
			}
			static Vector3 Up()
			{
				return Vector3(0, 1, 0);
			}
			static Vector3 Down()
			{
				return Vector3(0, -1, 0);
			}
			static Vector3 Left()
			{
				return Vector3(-1, 0, 0);
			}
			static Vector3 Right()
			{
				return Vector3(1, 0, 0);
			}
			static Vector3 Forward()
			{
				return Vector3(0, 0, 1);
			}
			static Vector3 Backward()
			{
				return Vector3(0, 0, -1);
			}
		};

		struct TH_OUT Vector4
		{
			float X;
			float Y;
			float Z;
			float W;

			Vector4();
			Vector4(const Vector2& Value);
			Vector4(const Vector3& Value);
			Vector4(const Vector4& Value);
			Vector4(float x, float y);
			Vector4(float x, float y, float z);
			Vector4(float x, float y, float z, float w);
			Vector4(float xyzw);
			float Length() const;
			float Sum() const;
			float Dot(const Vector4& B) const;
			float Distance(const Vector4& Target) const;
			Vector4 Cross(const Vector4& Vector1) const;
			Vector4 Transform(const Matrix4x4& Matrix) const;
			Vector4 Inv() const;
			Vector4 InvX() const;
			Vector4 InvY() const;
			Vector4 InvZ() const;
			Vector4 InvW() const;
			Vector4 Normalize() const;
			Vector4 sNormalize() const;
			Vector4 Lerp(const Vector4& B, float DeltaTime) const;
			Vector4 sLerp(const Vector4& B, float DeltaTime) const;
			Vector4 aLerp(const Vector4& B, float DeltaTime) const;
			Vector4 rLerp() const;
			Vector4 Abs() const;
			Vector4 Radians() const;
			Vector4 Degrees() const;
			Vector2 XY() const;
			Vector3 XYZ() const;
			Vector4 XYZW() const;
			Vector4 Mul(float xyzw) const;
			Vector4 Mul(const Vector2& XY, float Z, float W) const;
			Vector4 Mul(const Vector3& XYZ, float W) const;
			Vector4 Mul(const Vector4& Value) const;
			Vector4 Div(const Vector4& Value) const;
			Vector4 Add(const Vector4& Value) const;
			Vector4 SetX(float X) const;
			Vector4 SetY(float Y) const;
			Vector4 SetZ(float Z) const;
			Vector4 SetW(float W) const;
			void Set(const Vector4& Value);
			void Get2(float* In) const;
			void Get3(float* In) const;
			void Get4(float* In) const;
			Vector4& operator *=(const Matrix4x4& V);
			Vector4& operator *=(const Vector4& V);
			Vector4& operator *=(float V);
			Vector4& operator /=(const Vector4& V);
			Vector4& operator /=(float V);
			Vector4& operator +=(const Vector4& V);
			Vector4& operator +=(float V);
			Vector4& operator -=(const Vector4& V);
			Vector4& operator -=(float V);
			Vector4 operator *(const Matrix4x4& V) const;
			Vector4 operator *(const Vector4& V) const;
			Vector4 operator *(float V) const;
			Vector4 operator /(const Vector4& V) const;
			Vector4 operator /(float V) const;
			Vector4 operator +(const Vector4& V) const;
			Vector4 operator +(float V) const;
			Vector4 operator -(const Vector4& V) const;
			Vector4 operator -(float V) const;
			Vector4 operator -() const;
			Vector4& operator =(const Vector4& V);
			bool operator ==(const Vector4& V) const;
			bool operator !=(const Vector4& V) const;
			bool operator <=(const Vector4& V) const;
			bool operator >=(const Vector4& V) const;
			bool operator >(const Vector4& V) const;
			bool operator <(const Vector4& V) const;
			float& operator [](uint32_t Axis);
			float operator [](uint32_t Axis) const;

			static Vector4 Random();
			static Vector4 RandomAbs();
			static Vector4 One()
			{
				return Vector4(1, 1, 1, 1);
			};
			static Vector4 Zero()
			{
				return Vector4(0, 0, 0, 0);
			};
			static Vector4 Up()
			{
				return Vector4(0, 1, 0, 0);
			};
			static Vector4 Down()
			{
				return Vector4(0, -1, 0, 0);
			};
			static Vector4 Left()
			{
				return Vector4(-1, 0, 0, 0);
			};
			static Vector4 Right()
			{
				return Vector4(1, 0, 0, 0);
			};
			static Vector4 Forward()
			{
				return Vector4(0, 0, 1, 0);
			};
			static Vector4 Backward()
			{
				return Vector4(0, 0, -1, 0);
			};
			static Vector4 UnitW()
			{
				return Vector4(0, 0, 0, 1);
			};
		};

		struct TH_OUT Ray
		{
			Vector3 Origin;
			Vector3 Direction;

			Ray();
			Ray(const Vector3& _Origin, const Vector3& _Direction);
			Vector3 GetPoint(float T) const;
			Vector3 operator *(float T) const;
			bool IntersectsPlane(const Vector3& Normal, float Diameter) const;
			bool IntersectsSphere(const Vector3& Position, float Radius, bool DiscardInside = true) const;
			bool IntersectsAABBAt(const Vector3& Min, const Vector3& Max, Vector3* Hit) const;
			bool IntersectsAABB(const Vector3& Position, const Vector3& Scale, Vector3* Hit) const;
			bool IntersectsOBB(const Matrix4x4& World, Vector3* Hit) const;
		};

		struct TH_OUT Matrix4x4
		{
		public:
			float Row[16];

		public:
			Matrix4x4();
			Matrix4x4(float Array[16]);
			Matrix4x4(const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3);
			Matrix4x4(float Row00, float Row01, float Row02, float Row03, float Row10, float Row11, float Row12, float Row13, float Row20, float Row21, float Row22, float Row23, float Row30, float Row31, float Row32, float Row33);
			Matrix4x4(const Matrix4x4& Other);
			float& operator [](uint32_t Index);
			float operator [](uint32_t Index) const;
			bool operator ==(const Matrix4x4& Index) const;
			bool operator !=(const Matrix4x4& Index) const;
			Matrix4x4 operator *(const Matrix4x4& V) const;
			Matrix4x4 operator *(const Vector4& V) const;
			Matrix4x4& operator =(const Matrix4x4& V);
			Matrix4x4 Mul(const Matrix4x4& Right) const;
			Matrix4x4 Mul(const Vector4& Right) const;
			Matrix4x4 Inv() const;
			Matrix4x4 Transpose() const;
			Matrix4x4 SetScale(const Vector3& Value) const;
			Vector4 Row11() const;
			Vector4 Row22() const;
			Vector4 Row33() const;
			Vector4 Row44() const;
			Vector3 Up() const;
			Vector3 Right() const;
			Vector3 Forward() const;
			Vector3 Rotation() const;
			Vector3 Position() const;
			Vector3 Scale() const;
			Vector2 XY() const;
			Vector3 XYZ() const;
			Vector4 XYZW() const;
			void Identify();
			void Set(const Matrix4x4& Value);

		private:
			Matrix4x4(bool);

		public:
			static Matrix4x4 CreateLookAt(const Vector3& Position, const Vector3& Target, const Vector3& Up);
			static Matrix4x4 CreateRotationX(float Rotation);
			static Matrix4x4 CreateRotationY(float Rotation);
			static Matrix4x4 CreateRotationZ(float Rotation);
			static Matrix4x4 CreateScale(const Vector3& Scale);
			static Matrix4x4 CreateTranslatedScale(const Vector3& Position, const Vector3& Scale);
			static Matrix4x4 CreateTranslation(const Vector3& Position);
			static Matrix4x4 CreatePerspective(float FieldOfView, float AspectRatio, float NearClip, float FarClip);
			static Matrix4x4 CreatePerspectiveRad(float FieldOfView, float AspectRatio, float NearClip, float FarClip);
			static Matrix4x4 CreateOrthographic(float Width, float Height, float NearClip, float FarClip);
			static Matrix4x4 CreateOrthographicBox(float Width, float Height, float NearClip, float FarClip);
			static Matrix4x4 CreateOrthographicBox(float Left, float Right, float Bottom, float Top, float Near, float Far);
			static Matrix4x4 CreateOrthographicOffCenter(float ViewLeft, float ViewRight, float ViewBottom, float ViewTop, float NearZ, float FarZ);
			static Matrix4x4 Create(const Vector3& Position, const Vector3& Scale, const Vector3& Rotation);
			static Matrix4x4 Create(const Vector3& Position, const Vector3& Rotation);
			static Matrix4x4 CreateRotation(const Vector3& Rotation);
			static Matrix4x4 CreateCameraRotation(const Vector3& Rotation);
			static Matrix4x4 CreateOrthographic(float Left, float Right, float Bottom, float Top, float NearClip, float FarClip);
			static Matrix4x4 CreateCamera(const Vector3& Position, const Vector3& Rotation);
			static Matrix4x4 CreateOrigin(const Vector3& Position, const Vector3& Rotation);
			static Matrix4x4 CreateCubeMapLookAt(int Face, const Vector3& Position);
			static Matrix4x4 CreateLockedLookAt(const Vector3& Position, const Vector3& Origin, const Vector3& Up);
			static Matrix4x4 CreateRotation(const Vector3& Forward, const Vector3& Up, const Vector3& Right);
			static Matrix4x4 Identity()
			{
				return Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
			};
		};

		struct TH_OUT Quaternion
		{
			float X, Y, Z, W;

			Quaternion();
			Quaternion(float x, float y, float z, float w);
			Quaternion(const Quaternion& In);
			Quaternion(const Vector3& Axis, float Angle);
			Quaternion(const Vector3& Euler);
			Quaternion(const Matrix4x4& Value);
			void SetAxis(const Vector3& Axis, float Angle);
			void SetEuler(const Vector3& Euler);
			void SetMatrix(const Matrix4x4& Value);
			void Set(const Quaternion& Value);
			Quaternion operator *(float r) const;
			Quaternion operator *(const Vector3& r) const;
			Quaternion operator *(const Quaternion& r) const;
			Quaternion operator -(const Quaternion& r) const;
			Quaternion operator +(const Quaternion& r) const;
			Quaternion& operator =(const Quaternion& r);
			Quaternion Normalize() const;
			Quaternion sNormalize() const;
			Quaternion Conjugate() const;
			Quaternion Mul(float r) const;
			Quaternion Mul(const Quaternion& r) const;
			Quaternion Mul(const Vector3& r) const;
			Quaternion Sub(const Quaternion& r) const;
			Quaternion Add(const Quaternion& r) const;
			Quaternion Lerp(const Quaternion& B, float DeltaTime) const;
			Quaternion sLerp(const Quaternion& B, float DeltaTime) const;
			Vector3 Forward() const;
			Vector3 Up() const;
			Vector3 Right() const;
			Matrix4x4 GetMatrix() const;
			Vector3 GetEuler() const;
			float Dot(const Quaternion& r) const;
			float Length() const;

			static Quaternion CreateEulerRotation(const Vector3& Euler);
			static Quaternion CreateRotation(const Matrix4x4& Transform);
		};

		struct TH_OUT AnimatorKey
		{
			Compute::Vector3 Position = 0.0f;
			Compute::Vector3 Rotation = 0.0f;
			Compute::Vector3 Scale = 1.0f;
			float Time = 1.0f;
		};

		struct TH_OUT SkinAnimatorKey
		{
			std::vector<AnimatorKey> Pose;
			float Time = 1.0f;
		};

		struct TH_OUT SkinAnimatorClip
		{
			std::vector<SkinAnimatorKey> Keys;
			std::string Name;
			float Duration = 1.0f;
			float Rate = 1.0f;
		};

		struct TH_OUT KeyAnimatorClip
		{
			std::vector<AnimatorKey> Keys;
			std::string Name;
			float Duration = 1.0f;
			float Rate = 1.0f;
		};

		struct TH_OUT Joint
		{
			std::vector<Joint> Childs;
			std::string Name;
			Matrix4x4 Transform;
			Matrix4x4 BindShape;
			int64_t Index = -1;
		};

		struct TH_OUT RandomVector2
		{
			Vector2 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector2();
			RandomVector2(const Vector2& MinV, const Vector2& MaxV, bool IntensityV, float AccuracyV);
			Vector2 Generate();
		};

		struct TH_OUT RandomVector3
		{
			Vector3 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector3();
			RandomVector3(const Vector3& MinV, const Vector3& MaxV, bool IntensityV, float AccuracyV);
			Vector3 Generate();
		};

		struct TH_OUT RandomVector4
		{
			Vector4 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector4();
			RandomVector4(const Vector4& MinV, const Vector4& MaxV, bool IntensityV, float AccuracyV);
			Vector4 Generate();
		};

		struct TH_OUT RandomFloat
		{
			float Min, Max;
			bool Intensity;
			float Accuracy;

			RandomFloat();
			RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV);
			float Generate();
		};

		struct TH_OUT Hybi10PayloadHeader
		{
			unsigned short Opcode : 4;
			unsigned short Rsv1 : 1;
			unsigned short Rsv2 : 1;
			unsigned short Rsv3 : 1;
			unsigned short Fin : 1;
			unsigned short PayloadLength : 7;
			unsigned short Mask : 1;
		};

		struct TH_OUT Hybi10Request
		{
			std::string Payload;
			int ExitCode, Type;

			Hybi10Request();
			std::string GetTextType() const;
			Hybi10_Opcode GetEnumType() const;
		};

		struct TH_OUT ShapeContact
		{
			Vector3 LocalPoint1;
			Vector3 LocalPoint2;
			Vector3 PositionWorld1;
			Vector3 PositionWorld2;
			Vector3 NormalWorld;
			Vector3 LateralFrictionDirection1;
			Vector3 LateralFrictionDirection2;
			float Distance = 0.0f;
			float CombinedFriction = 0.0f;
			float CombinedRollingFriction = 0.0f;
			float CombinedSpinningFriction = 0.0f;
			float CombinedRestitution = 0.0f;
			float AppliedImpulse = 0.0f;
			float AppliedImpulseLateral1 = 0.0f;
			float AppliedImpulseLateral2 = 0.0f;
			float ContactMotion1 = 0.0f;
			float ContactMotion2 = 0.0f;
			float ContactCFM = 0.0f;
			float CombinedContactStiffness = 0.0f;
			float ContactERP = 0.0f;
			float CombinedContactDamping = 0.0f;
			float FrictionCFM = 0.0f;
			int PartId1 = 0;
			int PartId2 = 0;
			int Index1 = 0;
			int Index2 = 0;
			int ContactPointFlags = 0;
			int LifeTime = 0;
		};

		struct TH_OUT RayContact
		{
			Vector3 HitNormalLocal;
			Vector3 HitNormalWorld;
			Vector3 HitPointWorld;
			Vector3 RayFromWorld;
			Vector3 RayToWorld;
			float HitFraction = 0.0f;
			float ClosestHitFraction = 0.0f;
			bool NormalInWorldSpace = false;
		};

		struct TH_OUT RegexBracket
		{
			const char* Pointer = nullptr;
			int64_t Length = 0;
			int64_t Branches = 0;
			int64_t BranchesCount = 0;
		};

		struct TH_OUT RegexBranch
		{
			int64_t BracketIndex;
			const char* Pointer;
		};

		struct TH_OUT RegexMatch
		{
			const char* Pointer;
			int64_t Start;
			int64_t End;
			int64_t Length;
			int64_t Steps;
		};

		struct TH_OUT RegexSource
		{
			friend class Regex;

		private:
			std::string Expression;
			std::vector<RegexBracket> Brackets;
			std::vector<RegexBranch> Branches;
			int64_t MaxBranches;
			int64_t MaxBrackets;
			int64_t MaxMatches;
			RegexState State;

		public:
			bool IgnoreCase;

		public:
			RegexSource();
			RegexSource(const std::string& Regexp, bool fIgnoreCase = false, int64_t fMaxMatches = -1, int64_t fMaxBranches = -1, int64_t fMaxBrackets = -1);
			RegexSource(const RegexSource& Other);
			RegexSource(RegexSource&& Other) noexcept;
			RegexSource& operator =(const RegexSource& V);
			RegexSource& operator =(RegexSource&& V) noexcept;
			const std::string& GetRegex() const;
			int64_t GetMaxBranches() const;
			int64_t GetMaxBrackets() const;
			int64_t GetMaxMatches() const;
			int64_t GetComplexity() const;
			RegexState GetState() const;
			bool IsSimple() const;

		private:
			void Compile();
		};

		struct TH_OUT RegexResult
		{
			friend class Regex;

		private:
			std::vector<RegexMatch> Matches;
			RegexState State;
			int64_t Steps;

		public:
			RegexSource* Src;

		public:
			RegexResult();
			RegexResult(const RegexResult& Other);
			RegexResult(RegexResult&& Other) noexcept;
			RegexResult& operator =(const RegexResult& V);
			RegexResult& operator =(RegexResult&& V) noexcept;
			bool Empty() const;
			int64_t GetSteps() const;
			RegexState GetState() const;
			const std::vector<RegexMatch>& Get() const;
			std::vector<std::string> ToArray() const;
		};

		struct TH_OUT AdjTriangle
		{
			unsigned int VRef[3];
			unsigned int ATri[3];

			unsigned char FindEdge(unsigned int VRef0, unsigned int VRef1);
			unsigned int OppositeVertex(unsigned int VRef0, unsigned int VRef1);
		};

		struct TH_OUT AdjEdge
		{
			unsigned int Ref0;
			unsigned int Ref1;
			unsigned int FaceNb;
		};

		struct TH_OUT CollisionBody
		{
			RigidBody* Rigid = nullptr;
			SoftBody* Soft = nullptr;
			CollisionBody(btCollisionObject* Object);
		};

		class TH_OUT Adjacencies
		{
		public:
			struct Desc
			{
				unsigned int NbFaces = 0;
				unsigned int* Faces = nullptr;
			};

		private:
			unsigned int NbEdges;
			unsigned int CurrentNbFaces;
			AdjEdge* Edges;

		public:
			unsigned int NbFaces;
			AdjTriangle* Faces;

		public:
			Adjacencies();
			~Adjacencies();
			bool Fill(Adjacencies::Desc& I);
			bool Resolve();

		private:
			bool AddTriangle(unsigned int Ref0, unsigned int Ref1, unsigned int Ref2);
			bool AddEdge(unsigned int Ref0, unsigned int Ref1, unsigned int Face);
			bool UpdateLink(unsigned int FirstTri, unsigned int SecondTri, unsigned int Ref0, unsigned int Ref1);
		};

		class TH_OUT TriangleStrip
		{
		public:
			struct Desc
			{
				unsigned int* Faces = nullptr;
				unsigned int NbFaces = 0;
				bool OneSided = true;
				bool SGICipher = true;
				bool ConnectAllStrips = false;
			};

			struct Result
			{
				std::vector<unsigned int> Strips;
				std::vector<unsigned int> Groups;

				std::vector<int> GetIndices(int Group = -1);
				std::vector<int> GetInvIndices(int Group = -1);
			};

		private:
			std::vector<unsigned int> SingleStrip;
			std::vector<unsigned int> StripLengths;
			std::vector<unsigned int> StripRuns;
			Adjacencies* Adj;
			bool* Tags;
			unsigned int NbStrips;
			unsigned int TotalLength;
			bool OneSided;
			bool SGICipher;
			bool ConnectAllStrips;

		public:
			TriangleStrip();
			~TriangleStrip();
			bool Fill(const TriangleStrip::Desc& I);
			bool Resolve(TriangleStrip::Result& Result);

		private:
			TriangleStrip& FreeBuffers();
			unsigned int ComputeStrip(unsigned int Face);
			unsigned int TrackStrip(unsigned int Face, unsigned int Oldest, unsigned int Middle, unsigned int* Strip, unsigned int* Faces, bool* Tags);
			bool ConnectStrips(TriangleStrip::Result& Result);
		};

		class TH_OUT RadixSorter
		{
		private:
			unsigned int* Histogram;
			unsigned int* Offset;
			unsigned int CurrentSize;
			unsigned int* Indices;
			unsigned int* Indices2;

		public:
			RadixSorter();
			RadixSorter(const RadixSorter& Other);
			RadixSorter(RadixSorter&& Other) noexcept;
			~RadixSorter();
			RadixSorter& operator =(const RadixSorter& V);
			RadixSorter& operator =(RadixSorter&& V) noexcept;
			RadixSorter& Sort(unsigned int* Input, unsigned int Nb, bool SignedValues = true);
			RadixSorter& Sort(float* Input, unsigned int Nb);
			RadixSorter& ResetIndices();
			unsigned int* GetIndices();
		};

		class TH_OUT MD5Hasher
		{
		private:
			typedef unsigned char UInt1;
			typedef unsigned int UInt4;

		private:
			unsigned int S11 = 7;
			unsigned int S12 = 12;
			unsigned int S13 = 17;
			unsigned int S14 = 22;
			unsigned int S21 = 5;
			unsigned int S22 = 9;
			unsigned int S23 = 14;
			unsigned int S24 = 20;
			unsigned int S31 = 4;
			unsigned int S32 = 11;
			unsigned int S33 = 16;
			unsigned int S34 = 23;
			unsigned int S41 = 6;
			unsigned int S42 = 10;
			unsigned int S43 = 15;
			unsigned int S44 = 21;

		private:
			bool Finalized;
			UInt1 Buffer[64];
			UInt4 Count[2];
			UInt4 State[4];
			UInt1 Digest[16];

		public:
			MD5Hasher();
			void Transform(const UInt1* Buffer, unsigned int Length = 64);
			void Update(const std::string& Buffer, unsigned int BlockSize = 64);
			void Update(const unsigned char* Buffer, unsigned int Length, unsigned int BlockSize = 64);
			void Update(const char* Buffer, unsigned int Length, unsigned int BlockSize = 64);
			void Finalize();
			char* HexDigest() const;
			unsigned char* RawDigest() const;
			std::string ToHex() const;
			std::string ToRaw() const;

		private:
			static void Decode(UInt4* Output, const UInt1* Input, unsigned int Length);
			static void Encode(UInt1* Output, const UInt4* Input, unsigned int Length);
			static void FF(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static void GG(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static void HH(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static void II(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static UInt4 F(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 G(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 H(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 I(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 L(UInt4 X, int n);
		};

		class TH_OUT S8Hasher
		{
		public:
			S8Hasher() = default;
			S8Hasher(const S8Hasher&) = default;
			S8Hasher(S8Hasher&&) noexcept = default;
			~S8Hasher() = default;
			S8Hasher& operator=(const S8Hasher&) = default;
			S8Hasher& operator=(S8Hasher&&) noexcept = default;
			inline size_t operator()(uint64_t Value) const noexcept
			{
				Value ^= (size_t)(Value >> 33);
				Value *= 0xFF51AFD7ED558CCD;
				Value ^= (size_t)(Value >> 33);
				Value *= 0xC4CEB9FE1A85EC53;
				Value ^= (size_t)(Value >> 33);
				return (size_t)Value;
			}
			static inline uint64_t Parse(const char Data[8]) noexcept
			{
				uint64_t Result = 0;
				Result |= ((uint64_t)Data[0]);
				Result |= ((uint64_t)Data[1]) << 8;
				Result |= ((uint64_t)Data[2]) << 16;
				Result |= ((uint64_t)Data[3]) << 24;
				Result |= ((uint64_t)Data[4]) << 32;
				Result |= ((uint64_t)Data[5]) << 40;
				Result |= ((uint64_t)Data[6]) << 48;
				Result |= ((uint64_t)Data[7]) << 56;
				return Result;
			}
		};

		class TH_OUT Ciphers
		{
		public:
			static Cipher DES_ECB();
			static Cipher DES_EDE();
			static Cipher DES_EDE3();
			static Cipher DES_EDE_ECB();
			static Cipher DES_EDE3_ECB();
			static Cipher DES_CFB64();
			static Cipher DES_CFB1();
			static Cipher DES_CFB8();
			static Cipher DES_EDE_CFB64();
			static Cipher DES_EDE3_CFB64();
			static Cipher DES_EDE3_CFB1();
			static Cipher DES_EDE3_CFB8();
			static Cipher DES_OFB();
			static Cipher DES_EDE_OFB();
			static Cipher DES_EDE3_OFB();
			static Cipher DES_CBC();
			static Cipher DES_EDE_CBC();
			static Cipher DES_EDE3_CBC();
			static Cipher DES_EDE3_Wrap();
			static Cipher DESX_CBC();
			static Cipher RC4();
			static Cipher RC4_40();
			static Cipher RC4_HMAC_MD5();
			static Cipher IDEA_ECB();
			static Cipher IDEA_CFB64();
			static Cipher IDEA_OFB();
			static Cipher IDEA_CBC();
			static Cipher RC2_ECB();
			static Cipher RC2_CBC();
			static Cipher RC2_40_CBC();
			static Cipher RC2_64_CBC();
			static Cipher RC2_CFB64();
			static Cipher RC2_OFB();
			static Cipher BF_ECB();
			static Cipher BF_CBC();
			static Cipher BF_CFB64();
			static Cipher BF_OFB();
			static Cipher CAST5_ECB();
			static Cipher CAST5_CBC();
			static Cipher CAST5_CFB64();
			static Cipher CAST5_OFB();
			static Cipher RC5_32_12_16_CBC();
			static Cipher RC5_32_12_16_ECB();
			static Cipher RC5_32_12_16_CFB64();
			static Cipher RC5_32_12_16_OFB();
			static Cipher AES_128_ECB();
			static Cipher AES_128_CBC();
			static Cipher AES_128_CFB1();
			static Cipher AES_128_CFB8();
			static Cipher AES_128_CFB128();
			static Cipher AES_128_OFB();
			static Cipher AES_128_CTR();
			static Cipher AES_128_CCM();
			static Cipher AES_128_GCM();
			static Cipher AES_128_XTS();
			static Cipher AES_128_Wrap();
			static Cipher AES_128_WrapPad();
			static Cipher AES_128_OCB();
			static Cipher AES_192_ECB();
			static Cipher AES_192_CBC();
			static Cipher AES_192_CFB1();
			static Cipher AES_192_CFB8();
			static Cipher AES_192_CFB128();
			static Cipher AES_192_OFB();
			static Cipher AES_192_CTR();
			static Cipher AES_192_CCM();
			static Cipher AES_192_GCM();
			static Cipher AES_192_Wrap();
			static Cipher AES_192_WrapPad();
			static Cipher AES_192_OCB();
			static Cipher AES_256_ECB();
			static Cipher AES_256_CBC();
			static Cipher AES_256_CFB1();
			static Cipher AES_256_CFB8();
			static Cipher AES_256_CFB128();
			static Cipher AES_256_OFB();
			static Cipher AES_256_CTR();
			static Cipher AES_256_CCM();
			static Cipher AES_256_GCM();
			static Cipher AES_256_XTS();
			static Cipher AES_256_Wrap();
			static Cipher AES_256_WrapPad();
			static Cipher AES_256_OCB();
			static Cipher AES_128_CBC_HMAC_SHA1();
			static Cipher AES_256_CBC_HMAC_SHA1();
			static Cipher AES_128_CBC_HMAC_SHA256();
			static Cipher AES_256_CBC_HMAC_SHA256();
			static Cipher ARIA_128_ECB();
			static Cipher ARIA_128_CBC();
			static Cipher ARIA_128_CFB1();
			static Cipher ARIA_128_CFB8();
			static Cipher ARIA_128_CFB128();
			static Cipher ARIA_128_CTR();
			static Cipher ARIA_128_OFB();
			static Cipher ARIA_128_GCM();
			static Cipher ARIA_128_CCM();
			static Cipher ARIA_192_ECB();
			static Cipher ARIA_192_CBC();
			static Cipher ARIA_192_CFB1();
			static Cipher ARIA_192_CFB8();
			static Cipher ARIA_192_CFB128();
			static Cipher ARIA_192_CTR();
			static Cipher ARIA_192_OFB();
			static Cipher ARIA_192_GCM();
			static Cipher ARIA_192_CCM();
			static Cipher ARIA_256_ECB();
			static Cipher ARIA_256_CBC();
			static Cipher ARIA_256_CFB1();
			static Cipher ARIA_256_CFB8();
			static Cipher ARIA_256_CFB128();
			static Cipher ARIA_256_CTR();
			static Cipher ARIA_256_OFB();
			static Cipher ARIA_256_GCM();
			static Cipher ARIA_256_CCM();
			static Cipher Camellia_128_ECB();
			static Cipher Camellia_128_CBC();
			static Cipher Camellia_128_CFB1();
			static Cipher Camellia_128_CFB8();
			static Cipher Camellia_128_CFB128();
			static Cipher Camellia_128_OFB();
			static Cipher Camellia_128_CTR();
			static Cipher Camellia_192_ECB();
			static Cipher Camellia_192_CBC();
			static Cipher Camellia_192_CFB1();
			static Cipher Camellia_192_CFB8();
			static Cipher Camellia_192_CFB128();
			static Cipher Camellia_192_OFB();
			static Cipher Camellia_192_CTR();
			static Cipher Camellia_256_ECB();
			static Cipher Camellia_256_CBC();
			static Cipher Camellia_256_CFB1();
			static Cipher Camellia_256_CFB8();
			static Cipher Camellia_256_CFB128();
			static Cipher Camellia_256_OFB();
			static Cipher Camellia_256_CTR();
			static Cipher Chacha20();
			static Cipher Chacha20_Poly1305();
			static Cipher Seed_ECB();
			static Cipher Seed_CBC();
			static Cipher Seed_CFB128();
			static Cipher Seed_OFB();
			static Cipher SM4_ECB();
			static Cipher SM4_CBC();
			static Cipher SM4_CFB128();
			static Cipher SM4_OFB();
			static Cipher SM4_CTR();
		};

		class TH_OUT Digests
		{
		public:
			static Digest MD2();
			static Digest MD4();
			static Digest MD5();
			static Digest MD5_SHA1();
			static Digest Blake2B512();
			static Digest Blake2S256();
			static Digest SHA1();
			static Digest SHA224();
			static Digest SHA256();
			static Digest SHA384();
			static Digest SHA512();
			static Digest SHA512_224();
			static Digest SHA512_256();
			static Digest SHA3_224();
			static Digest SHA3_256();
			static Digest SHA3_384();
			static Digest SHA3_512();
			static Digest Shake128();
			static Digest Shake256();
			static Digest MDC2();
			static Digest RipeMD160();
			static Digest Whirlpool();
			static Digest SM3();
		};

		class TH_OUT Common
		{
		public:
			static std::string Base10ToBaseN(uint64_t Value, unsigned int BaseLessThan65);
			static bool IsCubeInFrustum(const Matrix4x4& WorldViewProjection, float Radius);
			static bool IsBase64URL(unsigned char Value);
			static bool IsBase64(unsigned char Value);
			static bool HasSphereIntersected(const Vector3& PositionR0, float RadiusR0, const Vector3& PositionR1, float RadiusR1);
			static bool HasLineIntersected(float DistanceF, float DistanceD, const Vector3& Start, const Vector3& End, Vector3& Hit);
			static bool HasLineIntersectedCube(const Vector3& Min, const Vector3& Max, const Vector3& Start, const Vector3& End);
			static bool HasPointIntersectedCube(const Vector3& Hit, const Vector3& Position, const Vector3& Scale, int Axis);
			static bool HasPointIntersectedRectangle(const Vector3& Position, const Vector3& Scale, const Vector3& P0);
			static bool HasPointIntersectedCube(const Vector3& Position, const Vector3& Scale, const Vector3& P0);
			static bool HasSBIntersected(Transform* BoxR0, Transform* BoxR1);
			static bool HasOBBIntersected(Transform* BoxR0, Transform* BoxR1);
			static bool HasAABBIntersected(Transform* BoxR0, Transform* BoxR1);
			static bool Hex(char c, int& v);
			static bool HexToString(void* Data, uint64_t Length, char* Buffer, uint64_t BufferLength);
			static bool HexToDecimal(const std::string& s, uint64_t i, uint64_t cnt, int& Value);
			static void ComputeJointOrientation(Compute::Joint* Matrix, bool LeftHanded);
			static void ComputeMatrixOrientation(Compute::Matrix4x4* Matrix, bool LeftHanded);
			static void ComputePositionOrientation(Compute::Vector3* Position, bool LeftHanded);
			static void ComputeIndexWindingOrderFlip(std::vector<int>& Indices);
			static void ComputeVertexOrientation(std::vector<Vertex>& Vertices, bool LeftHanded);
			static void ComputeInfluenceOrientation(std::vector<SkinVertex>& Vertices, bool LeftHanded);
			static void ComputeInfluenceNormals(std::vector<SkinVertex>& Vertices);
			static void ComputeInfluenceNormalsArray(SkinVertex* Vertices, uint64_t Count);
			static void ComputeInfluenceTangentBitangent(const SkinVertex& V1, const SkinVertex& V2, const SkinVertex& V3, Vector3& Tangent, Vector3& Bitangent, Vector3& Normal);
			static void ComputeInfluenceTangentBitangent(const SkinVertex& V1, const SkinVertex& V2, const SkinVertex& V3, Vector3& Tangent, Vector3& Bitangent);
			static void Sha1CollapseBufferBlock(unsigned int* Buffer);
			static void Sha1ComputeHashBlock(unsigned int* Result, unsigned int* W);
			static void Sha1Compute(const void* Value, int Length, unsigned char* Hash20);
			static void Sha1Hash20ToHex(const unsigned char* Hash20, char* HexString);
			static std::string JWTSign(const std::string& Algo, const std::string& Payload, const char* Key);
			static std::string JWTEncode(WebToken* Src, const char* Key);
			static WebToken* JWTDecode(const std::string& Value, const char* Key);
			static std::string DocEncrypt(Core::Document* Src, const char* Key, const char* Salt);
			static Core::Document* DocDecrypt(const std::string& Value, const char* Key, const char* Salt);
			static std::string RandomBytes(uint64_t Length);
			static std::string MD5Hash(const std::string& Value);
			static std::string Move(const std::string& Text, int Offset);
			static std::string Sign(Digest Type, const unsigned char* Value, uint64_t Length, const char* Key);
			static std::string Sign(Digest Type, const std::string& Value, const char* Key);
			static std::string HMAC(Digest Type, const unsigned char* Value, uint64_t Length, const char* Key);
			static std::string HMAC(Digest Type, const std::string& Value, const char* Key);
			static std::string Encrypt(Cipher Type, const unsigned char* Value, uint64_t Length, const char* Key, const char* Salt);
			static std::string Encrypt(Cipher Type, const std::string& Value, const char* Key, const char* Salt);
			static std::string Decrypt(Cipher Type, const unsigned char* Value, uint64_t Length, const char* Key, const char* Salt);
			static std::string Decrypt(Cipher Type, const std::string& Value, const char* Key, const char* Salt);
			static std::string Encode64(const char Alphabet[65], const unsigned char* Value, uint64_t Length, bool Padding);
			static std::string Decode64(const char Alphabet[65], const unsigned char* Value, uint64_t Length, bool(*IsAlphabetic)(unsigned char));
			static std::string Base64Encode(const unsigned char* Value, uint64_t Length);
			static std::string Base64Encode(const std::string& Value);
			static std::string Base64Decode(const unsigned char* Value, uint64_t Length);
			static std::string Base64Decode(const std::string& Value);
			static std::string Base64URLEncode(const unsigned char* Value, uint64_t Length);
			static std::string Base64URLEncode(const std::string& Value);
			static std::string Base64URLDecode(const unsigned char* Value, uint64_t Length);
			static std::string Base64URLDecode(const std::string& Value);
			static std::string HexEncode(const char* Value, size_t Size);
			static std::string HexEncode(const std::string& Value);
			static std::string HexDecode(const char* Value, size_t Size);
			static std::string HexDecode(const std::string& Value);
			static std::string URIEncode(const std::string& Text);
			static std::string URIEncode(const char* Text, uint64_t Length);
			static std::string URIDecode(const std::string& Text);
			static std::string URIDecode(const char* Text, uint64_t Length);
			static std::string Hybi10Encode(const Hybi10Request& hRequest, bool Masked);
			static std::string DecimalToHex(uint64_t V);
			static Hybi10Request Hybi10Decode(const std::string& Data);
			static unsigned char RandomUC();
			static uint64_t Utf8(int code, char* Buffer);
			static std::vector<int> CreateTriangleStrip(TriangleStrip::Desc& Desc, const std::vector<int>& Indices);
			static std::vector<int> CreateTriangleList(const std::vector<int>& Indices);
			static Ray CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView);
			static bool CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale, Vector3* Hit = nullptr);
			static bool CursorRayTest(const Ray& Cursor, const Matrix4x4& World, Vector3* Hit = nullptr);
			static uint64_t CRC32(const std::string& Data);
			static float FastInvSqrt(float Value);
			static uint64_t Random(uint64_t Min, uint64_t Max);
			static uint64_t Random();
		};

		class TH_OUT Regex
		{
			friend RegexSource;

		private:
			static int64_t Meta(const unsigned char* Buffer);
			static int64_t OpLength(const char* Value);
			static int64_t SetLength(const char* Value, int64_t ValueLength);
			static int64_t GetOpLength(const char* Value, int64_t ValueLength);
			static int64_t Quantifier(const char* Value);
			static int64_t ToInt(int64_t x);
			static int64_t HexToInt(const unsigned char* Buffer);
			static int64_t MatchOp(const unsigned char* Value, const unsigned char* Buffer, RegexResult* Info);
			static int64_t MatchSet(const char* Value, int64_t ValueLength, const char* Buffer, RegexResult* Info);
			static int64_t ParseDOH(const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case);
			static int64_t ParseInner(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case);
			static int64_t Parse(const char* Buffer, int64_t BufferLength, RegexResult* Info);

		public:
			static bool Match(RegexSource* Value, RegexResult& Result, const std::string& Buffer);
			static bool Match(RegexSource* Value, RegexResult& Result, const char* Buffer, int64_t Length);
			static const char* Syntax();
		};

		class TH_OUT WebToken : public Core::Object
		{
		public:
			Core::Document* Header;
			Core::Document* Payload;
			Core::Document* Token;
			std::string Refresher;
			std::string Signature;
			std::string Data;

		public:
			WebToken();
			WebToken(const std::string& Issuer, const std::string& Subject, int64_t Expiration);
			virtual ~WebToken() override;
			void Unsign();
			void SetAlgorithm(const std::string& Value);
			void SetType(const std::string& Value);
			void SetContentType(const std::string& Value);
			void SetIssuer(const std::string& Value);
			void SetSubject(const std::string& Value);
			void SetId(const std::string& Value);
			void SetAudience(const std::vector<std::string>& Value);
			void SetExpiration(int64_t Value);
			void SetNotBefore(int64_t Value);
			void SetCreated(int64_t Value);
			void SetRefreshToken(const std::string& Value, const char* Key, const char* Salt);
			bool Sign(const char* Key);
			std::string GetRefreshToken(const char* Key, const char* Salt);
			bool IsValid() const;
		};

		class TH_OUT Preprocessor : public Core::Object
		{
		public:
			struct Desc
			{
				bool Pragmas = true;
				bool Includes = true;
				bool Defines = true;
				bool Conditions = true;
			};

		private:
			std::vector<std::string> Defines;
			std::vector<std::string> Sets;
			std::string ExpandedPath;
			ProcIncludeCallback Include;
			ProcPragmaCallback Pragma;
			IncludeDesc FileDesc;
			Desc Features;
			bool Nested;

		public:
			Preprocessor();
			virtual ~Preprocessor() = default;
			void SetIncludeOptions(const IncludeDesc& NewDesc);
			void SetIncludeCallback(const ProcIncludeCallback& Callback);
			void SetPragmaCallback(const ProcPragmaCallback& Callback);
			void SetFeatures(const Desc& Value);
			void Define(const std::string& Name);
			void Undefine(const std::string& Name);
			void Clear();
			bool IsDefined(const char* Name) const;
			bool Process(const std::string& Path, std::string& Buffer);
			const std::string& GetCurrentFilePath();

		private:
			bool SaveResult();
			bool ReturnResult(bool Result, bool WasNested);
			bool ProcessIncludeDirective(const std::string& Path, Core::Parser& Buffer);
			bool ProcessPragmaDirective(const std::string& Path, Core::Parser& Buffer);
			bool ProcessBlockDirective(Core::Parser& Buffer);
			bool ProcessDefineDirective(Core::Parser& Buffer, uint64_t Base, uint64_t& Offset, bool Endless);
			int FindDefineDirective(Core::Parser& Buffer, uint64_t& Offset, uint64_t* Size);
			int FindBlockDirective(Core::Parser& Buffer, uint64_t& Offset, bool Nested);
			int FindBlockNesting(Core::Parser& Buffer, const Core::Parser::Settle& Hash, uint64_t& Offset, bool Resolved);
			int FindDirective(Core::Parser& Buffer, const char* V, uint64_t* Offset, uint64_t* Base, uint64_t* Start, uint64_t* End);
			bool HasSet(const std::string& Path);

		public:
			static IncludeResult ResolveInclude(const IncludeDesc& Desc);
		};

		class TH_OUT FiniteState : public Core::Object
		{
		private:
			std::unordered_map<std::string, ActionCallback*> Actions;
			std::stack<ActionCallback*> State;
			std::mutex Mutex;

		public:
			FiniteState();
			virtual ~FiniteState() override;
			FiniteState* Bind(const std::string& Name, const ActionCallback& Callback);
			FiniteState* Unbind(const std::string& Name);
			FiniteState* Push(const std::string& Name);
			FiniteState* Replace(const std::string& Name);
			FiniteState* Pop();
			void Update();

		private:
			ActionCallback* Find(const std::string& Name);
		};

		class TH_OUT Transform : public Core::Object
		{
			friend Common;

		public:
			struct Spacing
			{
				Matrix4x4 Offset;
				Vector3 Position;
				Vector3 Rotation;
				Vector3 Scale = 1.0f;
			};

		private:
			std::vector<Transform*> Childs;
			Matrix4x4 Temporary;
			Transform* Root;
			Spacing* Local;
			Spacing Global;
			bool Scaling;
			bool Dirty;

		public:
			void* UserPointer;

		public:
			Transform();
			virtual ~Transform() override;
			void Synchronize();
			void Move(const Vector3& Value);
			void Rotate(const Vector3& Value);
			void Rescale(const Vector3& Value);
			void Localize(Spacing& Where);
			void Globalize(Spacing& Where);
			void Specialize(Spacing& Where);
			void Copy(Transform* Target);
			void AddChild(Transform* Child);
			void RemoveChild(Transform* Child);
			void RemoveChilds();
			void MakeDirty();
			void SetScaling(bool Enabled);
			void SetPosition(const Vector3& Value);
			void SetRotation(const Vector3& Value);
			void SetScale(const Vector3& Value);
			void SetSpacing(Positioning Space, Spacing& Where);
			void SetPivot(Transform* Root, Spacing* Pivot);
			void SetRoot(Transform* Root);
			bool HasRoot(Transform* Target);
			bool HasChild(Transform* Target);
			bool HasScaling();
			bool IsDirty();
			const Matrix4x4& GetBias();
			const Matrix4x4& GetBiasUnscaled();
			const Vector3& GetPosition();
			const Vector3& GetRotation();
			const Vector3& GetScale();
			Vector3 Forward();
			Vector3 Right();
			Vector3 Up();
			Spacing& GetSpacing();
			Spacing& GetSpacing(Positioning Space);
			Transform* GetRoot();
			Transform* GetUpperRoot();
			Transform* GetChild(size_t Child);
			size_t GetChildsCount();
			std::vector<Transform*>& GetChilds();

		protected:
			bool CanRootBeApplied(Transform* Root);

		public:
			template <typename In>
			In* Ptr()
			{
				return (In*)UserPointer;
			}
		};

		class TH_OUT HullShape : public Core::Object
		{
		public:
			std::vector<Vertex> Vertices;
			std::vector<int> Indices;
			btCollisionShape* Shape;

		public:
			HullShape();
			virtual ~HullShape() = default;
		};

		class TH_OUT RigidBody : public Core::Object
		{
			friend Simulator;

		public:
			struct Desc
			{
				btCollisionShape* Shape = nullptr;
				float Anticipation = 0, Mass = 0;
				Vector3 Position;
				Vector3 Rotation;
				Vector3 Scale;
			};

		private:
			btRigidBody* Instance;
			Simulator* Engine;
			Desc Initial;

		public:
			CollisionCallback OnCollisionEnter;
			CollisionCallback OnCollisionExit;
			void* UserPointer;

		private:
			RigidBody(Simulator* Refer, const Desc& I);

		public:
			virtual ~RigidBody() override;
			RigidBody* Copy();
			void Push(const Vector3& Velocity);
			void Push(const Vector3& Velocity, const Vector3& Torque);
			void Push(const Vector3& Velocity, const Vector3& Torque, const Vector3& Center);
			void PushKinematic(const Vector3& Velocity);
			void PushKinematic(const Vector3& Velocity, const Vector3& Torque);
			void Synchronize(Transform* Transform, bool Kinematic);
			void SetCollisionFlags(uint64_t Flags);
			void SetActivity(bool Active);
			void SetAsGhost();
			void SetAsNormal();
			void SetSelfPointer();
			void SetWorldTransform(btTransform* Value);
			void SetCollisionShape(btCollisionShape* Shape, Transform* Transform);
			void SetMass(float Mass);
			void SetActivationState(MotionState Value);
			void SetAngularDamping(float Value);
			void SetAngularSleepingThreshold(float Value);
			void SetSpinningFriction(float Value);
			void SetContactStiffness(float Value);
			void SetContactDamping(float Value);
			void SetFriction(float Value);
			void SetRestitution(float Value);
			void SetHitFraction(float Value);
			void SetLinearDamping(float Value);
			void SetLinearSleepingThreshold(float Value);
			void SetCcdMotionThreshold(float Value);
			void SetCcdSweptSphereRadius(float Value);
			void SetContactProcessingThreshold(float Value);
			void SetDeactivationTime(float Value);
			void SetRollingFriction(float Value);
			void SetAngularFactor(const Vector3& Value);
			void SetAnisotropicFriction(const Vector3& Value);
			void SetGravity(const Vector3& Value);
			void SetLinearFactor(const Vector3& Value);
			void SetLinearVelocity(const Vector3& Value);
			void SetAngularVelocity(const Vector3& Value);
			MotionState GetActivationState();
			Shape GetCollisionShapeType();
			Vector3 GetAngularFactor();
			Vector3 GetAnisotropicFriction();
			Vector3 GetGravity();
			Vector3 GetLinearFactor();
			Vector3 GetLinearVelocity();
			Vector3 GetAngularVelocity();
			Vector3 GetScale();
			Vector3 GetPosition();
			Vector3 GetRotation();
			btTransform* GetWorldTransform();
			btCollisionShape* GetCollisionShape();
			btRigidBody* Get();
			bool IsActive();
			bool IsStatic();
			bool IsGhost();
			bool IsColliding();
			float GetSpinningFriction();
			float GetContactStiffness();
			float GetContactDamping();
			float GetAngularDamping();
			float GetAngularSleepingThreshold();
			float GetFriction();
			float GetRestitution();
			float GetHitFraction();
			float GetLinearDamping();
			float GetLinearSleepingThreshold();
			float GetCcdMotionThreshold();
			float GetCcdSweptSphereRadius();
			float GetContactProcessingThreshold();
			float GetDeactivationTime();
			float GetRollingFriction();
			float GetMass();
			uint64_t GetCollisionFlags();
			Desc& GetInitialState();
			Simulator* GetSimulator();

		public:
			static RigidBody* Get(btRigidBody* From);
		};

		class TH_OUT SoftBody : public Core::Object
		{
			friend Simulator;

		public:
			struct Desc
			{
				struct CV
				{
					struct SConvex
					{
						Compute::HullShape* Hull = nullptr;
						bool Enabled = false;
					} Convex;

					struct SRope
					{
						bool StartFixed = false;
						bool EndFixed = false;
						bool Enabled = false;
						int Count = 0;
						Vector3 Start = 0;
						Vector3 End = 1;
					} Rope;

					struct SPatch
					{
						bool GenerateDiagonals = false;
						bool Corner00Fixed = false;
						bool Corner10Fixed = false;
						bool Corner01Fixed = false;
						bool Corner11Fixed = false;
						bool Enabled = false;
						int CountX = 2;
						int CountY = 2;
						Vector3 Corner00 = Vector3(0, 0);
						Vector3 Corner10 = Vector3(1, 0);
						Vector3 Corner01 = Vector3(0, 1);
						Vector3 Corner11 = Vector3(1, 1);
					} Patch;

					struct SEllipsoid
					{
						Vector3 Center;
						Vector3 Radius = 1;
						int Count = 3;
						bool Enabled = false;
					} Ellipsoid;
				} Shape;

				struct SConfig
				{
					SoftAeroModel AeroModel = SoftAeroModel::VPoint;
					float VCF = 1;
					float DP = 0;
					float DG = 0;
					float LF = 0;
					float PR = 1.0f;
					float VC = 0.1f;
					float DF = 0.5f;
					float MT = 0.1f;
					float CHR = 1;
					float KHR = 0.1f;
					float SHR = 1;
					float AHR = 0.7f;
					float SRHR_CL = 0.1f;
					float SKHR_CL = 1;
					float SSHR_CL = 0.5f;
					float SR_SPLT_CL = 0.5f;
					float SK_SPLT_CL = 0.5f;
					float SS_SPLT_CL = 0.5f;
					float MaxVolume = 1;
					float TimeScale = 1;
					float Drag = 0;
					float MaxStress = 0;
					int Clusters = 0;
					int Constraints = 2;
					int VIterations = 10;
					int PIterations = 2;
					int DIterations = 0;
					int CIterations = 4;
					int Collisions = (int)(SoftCollision::Default | SoftCollision::VF_SS);
				} Config;

				float Anticipation = 0;
				Vector3 Position;
				Vector3 Rotation;
				Vector3 Scale;
			};

			struct RayCast
			{
				SoftBody* Body = nullptr;
				SoftFeature Feature = SoftFeature::None;
				float Fraction = 0;
				int Index = 0;
			};

		private:
			btSoftBody* Instance;
			Simulator* Engine;
			Vector3 Center;
			Desc Initial;

		public:
			CollisionCallback OnCollisionEnter;
			CollisionCallback OnCollisionExit;
			void* UserPointer;

		private:
			SoftBody(Simulator* Refer, const Desc& I);

		public:
			virtual ~SoftBody() override;
			SoftBody* Copy();
			void Activate(bool Force);
			void Synchronize(Transform* Transform, bool Kinematic);
			void GetIndices(std::vector<int>* Indices);
			void GetVertices(std::vector<Vertex>* Vertices);
			void GetBoundingBox(Vector3* Min, Vector3* Max);
			void SetContactStiffnessAndDamping(float Stiffness, float Damping);
			void AddAnchor(int Node, RigidBody* Body, bool DisableCollisionBetweenLinkedBodies = false, float Influence = 1);
			void AddAnchor(int Node, RigidBody* Body, const Vector3& LocalPivot, bool DisableCollisionBetweenLinkedBodies = false, float Influence = 1);
			void AddForce(const Vector3& Force);
			void AddForce(const Vector3& Force, int Node);
			void AddAeroForceToNode(const Vector3& WindVelocity, int NodeIndex);
			void AddAeroForceToFace(const Vector3& WindVelocity, int FaceIndex);
			void AddVelocity(const Vector3& Velocity);
			void SetVelocity(const Vector3& Velocity);
			void AddVelocity(const Vector3& Velocity, int Node);
			void SetMass(int Node, float Mass);
			void SetTotalMass(float Mass, bool FromFaces = false);
			void SetTotalDensity(float Density);
			void SetVolumeMass(float Mass);
			void SetVolumeDensity(float Density);
			void Translate(const Vector3& Position);
			void Rotate(const Vector3& Rotation);
			void Scale(const Vector3& Scale);
			void SetRestLengthScale(float RestLength);
			void SetPose(bool Volume, bool Frame);
			float GetMass(int Node) const;
			float GetTotalMass() const;
			float GetRestLengthScale();
			float GetVolume() const;
			int GenerateBendingConstraints(int Distance);
			void RandomizeConstraints();
			bool CutLink(int Node0, int Node1, float Position);
			bool RayTest(const Vector3& From, const Vector3& To, RayCast& Result);
			void SetWindVelocity(const Vector3& Velocity);
			Vector3 GetWindVelocity();
			void GetAabb(Vector3& Min, Vector3& Max) const;
			void IndicesToPointers(const int* Map = 0);
			void SetSpinningFriction(float Value);
			Vector3 GetLinearVelocity();
			Vector3 GetAngularVelocity();
			Vector3 GetCenterPosition();
			void SetActivity(bool Active);
			void SetAsGhost();
			void SetAsNormal();
			void SetSelfPointer();
			void SetWorldTransform(btTransform* Value);
			void SetActivationState(MotionState Value);
			void SetContactStiffness(float Value);
			void SetContactDamping(float Value);
			void SetFriction(float Value);
			void SetRestitution(float Value);
			void SetHitFraction(float Value);
			void SetCcdMotionThreshold(float Value);
			void SetCcdSweptSphereRadius(float Value);
			void SetContactProcessingThreshold(float Value);
			void SetDeactivationTime(float Value);
			void SetRollingFriction(float Value);
			void SetAnisotropicFriction(const Vector3& Value);
			void SetConfig(const Desc::SConfig& Conf);
			Shape GetCollisionShapeType();
			MotionState GetActivationState();
			Vector3 GetAnisotropicFriction();
			Vector3 GetScale();
			Vector3 GetPosition();
			Vector3 GetRotation();
			btTransform* GetWorldTransform();
			btSoftBody* Get();
			bool IsActive();
			bool IsStatic();
			bool IsGhost();
			bool IsColliding();
			float GetSpinningFriction();
			float GetContactStiffness();
			float GetContactDamping();
			float GetFriction();
			float GetRestitution();
			float GetHitFraction();
			float GetCcdMotionThreshold();
			float GetCcdSweptSphereRadius();
			float GetContactProcessingThreshold();
			float GetDeactivationTime();
			float GetRollingFriction();
			uint64_t GetCollisionFlags();
			uint64_t GetVerticesCount();
			Desc& GetInitialState();
			Simulator* GetSimulator();

		public:
			static SoftBody* Get(btSoftBody* From);
		};

		class TH_OUT Constraint : public Core::Object
		{
		protected:
			btRigidBody* First, *Second;
			Simulator* Engine;

		public:
			void* UserPointer;

		protected:
			Constraint(Simulator* Refer);

		public:
			virtual ~Constraint() = default;
			virtual Constraint* Copy() = 0;
			virtual btTypedConstraint* Get() = 0;
			virtual bool HasCollisions() = 0;
			void SetBreakingImpulseThreshold(float Value);
			void SetEnabled(bool Value);
			bool IsEnabled();
			bool IsActive();
			float GetBreakingImpulseThreshold();
			btRigidBody* GetFirst();
			btRigidBody* GetSecond();
			Simulator* GetSimulator();
		};

		class TH_OUT PConstraint : public Constraint
		{
			friend RigidBody;
			friend Simulator;

		public:
			struct Desc
			{
				RigidBody* TargetA = nullptr;
				RigidBody* TargetB = nullptr;
				Vector3 PivotA;
				Vector3 PivotB;
				bool Collisions = true;
			};

		private:
			btPoint2PointConstraint* Instance;
			Desc State;

		private:
			PConstraint(Simulator* Refer, const Desc& I);

		public:
			virtual ~PConstraint() override;
			virtual Constraint* Copy() override;
			virtual btTypedConstraint* Get() override;
			virtual bool HasCollisions() override;
			void SetPivotA(const Vector3& Value);
			void SetPivotB(const Vector3& Value);
			Vector3 GetPivotA();
			Vector3 GetPivotB();
			Desc& GetState();
		};

		class TH_OUT HConstraint : public Constraint
		{
			friend RigidBody;
			friend Simulator;

		public:
			struct Desc
			{
				RigidBody* TargetA = nullptr;
				RigidBody* TargetB = nullptr;
				bool References = false;
				bool Collisions = true;
			};

		private:
			btHingeConstraint* Instance;
			Desc State;

		private:
			HConstraint(Simulator* Refer, const Desc& I);

		public:
			virtual ~HConstraint() override;
			virtual Constraint* Copy() override;
			virtual btTypedConstraint* Get() override;
			virtual bool HasCollisions() override;
			void EnableAngularMotor(bool Enable, float TargetVelocity, float MaxMotorImpulse);
			void EnableMotor(bool Enable);
			void TestLimit(const Matrix4x4& A, const Matrix4x4& B);
			void SetFrames(const Matrix4x4& A, const Matrix4x4& B);
			void SetAngularOnly(bool Value);
			void SetMaxMotorImpulse(float Value);
			void SetMotorTargetVelocity(float Value);
			void SetMotorTarget(float TargetAngle, float Delta);
			void SetLimit(float Low, float High, float Softness = 0.9f, float BiasFactor = 0.3f, float RelaxationFactor = 1.0f);
			void SetOffset(bool Value);
			void SetReferenceToA(bool Value);
			void SetAxis(const Vector3& Value);
			int GetSolveLimit();
			float GetMotorTargetVelocity();
			float GetMaxMotorImpulse();
			float GetLimitSign();
			float GetHingeAngle();
			float GetHingeAngle(const Matrix4x4& A, const Matrix4x4& B);
			float GetLowerLimit();
			float GetUpperLimit();
			float GetLimitSoftness();
			float GetLimitBiasFactor();
			float GetLimitRelaxationFactor();
			bool HasLimit();
			bool IsOffset();
			bool IsReferenceToA();
			bool IsAngularOnly();
			bool IsAngularMotorEnabled();
			Desc& GetState();
		};

		class TH_OUT SConstraint : public Constraint
		{
			friend RigidBody;
			friend Simulator;

		public:
			struct Desc
			{
				RigidBody* TargetA = nullptr;
				RigidBody* TargetB = nullptr;
				bool Collisions = true;
				bool Linear = true;
			};

		private:
			btSliderConstraint* Instance;
			Desc State;

		private:
			SConstraint(Simulator* Refer, const Desc& I);

		public:
			virtual ~SConstraint() override;
			virtual Constraint* Copy() override;
			virtual btTypedConstraint* Get() override;
			virtual bool HasCollisions() override;
			void SetAngularMotorVelocity(float Value);
			void SetLinearMotorVelocity(float Value);
			void SetUpperLinearLimit(float Value);
			void SetLowerLinearLimit(float Value);
			void SetAngularDampingDirection(float Value);
			void SetLinearDampingDirection(float Value);
			void SetAngularDampingLimit(float Value);
			void SetLinearDampingLimit(float Value);
			void SetAngularDampingOrtho(float Value);
			void SetLinearDampingOrtho(float Value);
			void SetUpperAngularLimit(float Value);
			void SetLowerAngularLimit(float Value);
			void SetMaxAngularMotorForce(float Value);
			void SetMaxLinearMotorForce(float Value);
			void SetAngularRestitutionDirection(float Value);
			void SetLinearRestitutionDirection(float Value);
			void SetAngularRestitutionLimit(float Value);
			void SetLinearRestitutionLimit(float Value);
			void SetAngularRestitutionOrtho(float Value);
			void SetLinearRestitutionOrtho(float Value);
			void SetAngularSoftnessDirection(float Value);
			void SetLinearSoftnessDirection(float Value);
			void SetAngularSoftnessLimit(float Value);
			void SetLinearSoftnessLimit(float Value);
			void SetAngularSoftnessOrtho(float Value);
			void SetLinearSoftnessOrtho(float Value);
			void SetPoweredAngularMotor(bool Value);
			void SetPoweredLinearMotor(bool Value);
			float GetAngularMotorVelocity();
			float GetLinearMotorVelocity();
			float GetUpperLinearLimit();
			float GetLowerLinearLimit();
			float GetAngularDampingDirection();
			float GetLinearDampingDirection();
			float GetAngularDampingLimit();
			float GetLinearDampingLimit();
			float GetAngularDampingOrtho();
			float GetLinearDampingOrtho();
			float GetUpperAngularLimit();
			float GetLowerAngularLimit();
			float GetMaxAngularMotorForce();
			float GetMaxLinearMotorForce();
			float GetAngularRestitutionDirection();
			float GetLinearRestitutionDirection();
			float GetAngularRestitutionLimit();
			float GetLinearRestitutionLimit();
			float GetAngularRestitutionOrtho();
			float GetLinearRestitutionOrtho();
			float GetAngularSoftnessDirection();
			float GetLinearSoftnessDirection();
			float GetAngularSoftnessLimit();
			float GetLinearSoftnessLimit();
			float GetAngularSoftnessOrtho();
			float GetLinearSoftnessOrtho();
			bool GetPoweredAngularMotor();
			bool GetPoweredLinearMotor();
			Desc& GetState();
		};

		class TH_OUT CTConstraint : public Constraint
		{
			friend RigidBody;
			friend Simulator;

		public:
			struct Desc
			{
				RigidBody* TargetA = nullptr;
				RigidBody* TargetB = nullptr;
				bool Collisions = true;
			};

		private:
			btConeTwistConstraint* Instance;
			Desc State;

		private:
			CTConstraint(Simulator* Refer, const Desc& I);

		public:
			virtual ~CTConstraint() override;
			virtual Constraint* Copy() override;
			virtual btTypedConstraint* Get() override;
			virtual bool HasCollisions() override;
			void EnableMotor(bool Value);
			void SetFrames(const Matrix4x4& A, const Matrix4x4& B);
			void SetAngularOnly(bool Value);
			void SetLimit(int LimitIndex, float LimitValue);
			void SetLimit(float SwingSpan1, float SwingSpan2, float TwistSpan, float Softness = 1.f, float BiasFactor = 0.3f, float RelaxationFactor = 1.0f);
			void SetDamping(float Value);
			void SetMaxMotorImpulse(float Value);
			void SetMaxMotorImpulseNormalized(float Value);
			void SetFixThresh(float Value);
			void SetMotorTarget(const Quaternion& Value);
			void SetMotorTargetInConstraintSpace(const Quaternion& Value);
			Vector3 GetPointForAngle(float AngleInRadians, float Length);
			Quaternion GetMotorTarget();
			int GetSolveTwistLimit();
			int GetSolveSwingLimit();
			float GetTwistLimitSign();
			float GetSwingSpan1();
			float GetSwingSpan2();
			float GetTwistSpan();
			float GetLimitSoftness();
			float GetBiasFactor();
			float GetRelaxationFactor();
			float GetTwistAngle();
			float GetLimit(int Value);
			float GetDamping();
			float GetMaxMotorImpulse();
			float GetFixThresh();
			bool IsMotorEnabled();
			bool IsMaxMotorImpulseNormalized();
			bool IsPastSwingLimit();
			bool IsAngularOnly();
			Desc& GetState();
		};

		class TH_OUT DF6Constraint : public Constraint
		{
			friend RigidBody;
			friend Simulator;

		public:
			struct Desc
			{
				RigidBody* TargetA = nullptr;
				RigidBody* TargetB = nullptr;
				bool Collisions = true;
			};

		private:
			btGeneric6DofSpring2Constraint* Instance;
			Desc State;

		private:
			DF6Constraint(Simulator* Refer, const Desc& I);

		public:
			virtual ~DF6Constraint() override;
			virtual Constraint* Copy() override;
			virtual btTypedConstraint* Get() override;
			virtual bool HasCollisions() override;
			void EnableMotor(int Index, bool OnOff);
			void EnableSpring(int Index, bool OnOff);
			void SetFrames(const Matrix4x4& A, const Matrix4x4& B);
			void SetLinearLowerLimit(const Vector3& Value);
			void SetLinearUpperLimit(const Vector3& Value);
			void SetAngularLowerLimit(const Vector3& Value);
			void SetAngularLowerLimitReversed(const Vector3& Value);
			void SetAngularUpperLimit(const Vector3& Value);
			void SetAngularUpperLimitReversed(const Vector3& Value);
			void SetLimit(int Axis, float Low, float High);
			void SetLimitReversed(int Axis, float Low, float High);
			void SetRotationOrder(Rotator Order);
			void SetAxis(const Vector3& A, const Vector3& B);
			void SetBounce(int Index, float Bounce);
			void SetServo(int Index, bool OnOff);
			void SetTargetVelocity(int Index, float Velocity);
			void SetServoTarget(int Index, float Target);
			void SetMaxMotorForce(int Index, float Force);
			void SetStiffness(int Index, float Stiffness, bool LimitIfNeeded = true);
			void SetEquilibriumPoint();
			void SetEquilibriumPoint(int Index);
			void SetEquilibriumPoint(int Index, float Value);
			Vector3 GetAngularUpperLimit();
			Vector3 GetAngularUpperLimitReversed();
			Vector3 GetAngularLowerLimit();
			Vector3 GetAngularLowerLimitReversed();
			Vector3 GetLinearUpperLimit();
			Vector3 GetLinearLowerLimit();
			Vector3 GetAxis(int Value);
			Rotator GetRotationOrder();
			float GetAngle(int Value);
			float GetRelativePivotPosition(int Value);
			bool IsLimited(int LimitIndex);
			Desc& GetState();
		};

		class TH_OUT Simulator : public Core::Object
		{
		public:
			struct Desc
			{
				Vector3 WaterNormal;
				Vector3 Gravity = Vector3(0, -10, 0);
				float AirDensity = 1.2f;
				float WaterDensity = 0;
				float WaterOffset = 0;
				float MaxDisplacement = 1000;
				bool EnableSoftBody = false;
			};

		private:
			std::unordered_map<void*, uint64_t> Shapes;
			btCollisionConfiguration* Collision;
			btBroadphaseInterface* Broadphase;
			btConstraintSolver* Solver;
			btDiscreteDynamicsWorld* World;
			btCollisionDispatcher* Dispatcher;
			btSoftBodySolver* SoftSolver;
			std::mutex Safe;

		public:
			float TimeSpeed;
			int Interpolate;
			bool Active;

		public:
			Simulator(const Desc& I);
			virtual ~Simulator() override;
			void SetGravity(const Vector3& Gravity);
			void SetLinearImpulse(const Vector3& Impulse, bool RandomFactor = false);
			void SetLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor = false);
			void SetAngularImpulse(const Vector3& Impulse, bool RandomFactor = false);
			void SetAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor = false);
			void SetOnCollisionEnter(ContactStartedCallback Callback);
			void SetOnCollisionExit(ContactEndedCallback Callback);
			void CreateLinearImpulse(const Vector3& Impulse, bool RandomFactor = false);
			void CreateLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor = false);
			void CreateAngularImpulse(const Vector3& Impulse, bool RandomFactor = false);
			void CreateAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor = false);
			void AddSoftBody(SoftBody* Body);
			void RemoveSoftBody(SoftBody* Body);
			void AddRigidBody(RigidBody* Body);
			void RemoveRigidBody(RigidBody* Body);
			void AddConstraint(Constraint* Constraint);
			void RemoveConstraint(Constraint* Constraint);
			void RemoveAll();
			void Simulate(float TimeStep);
			void FindContacts(RigidBody* Body, int(* Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&));
			bool FindRayContacts(const Vector3& Start, const Vector3& End, int(* Callback)(RayContact*, const CollisionBody&));
			RigidBody* CreateRigidBody(const RigidBody::Desc& I);
			RigidBody* CreateRigidBody(const RigidBody::Desc& I, Transform* Transform);
			SoftBody* CreateSoftBody(const SoftBody::Desc& I);
			SoftBody* CreateSoftBody(const SoftBody::Desc& I, Transform* Transform);
			PConstraint* CreatePoint2PointConstraint(const PConstraint::Desc& I);
			HConstraint* CreateHingeConstraint(const HConstraint::Desc& I);
			SConstraint* CreateSliderConstraint(const SConstraint::Desc& I);
			CTConstraint* CreateConeTwistConstraint(const CTConstraint::Desc& I);
			DF6Constraint* Create6DoFConstraint(const DF6Constraint::Desc& I);
			btCollisionShape* CreateShape(Shape Type);
			btCollisionShape* CreateCube(const Vector3& Scale = Vector3(1, 1, 1));
			btCollisionShape* CreateSphere(float Radius = 1);
			btCollisionShape* CreateCapsule(float Radius = 1, float Height = 1);
			btCollisionShape* CreateCone(float Radius = 1, float Height = 1);
			btCollisionShape* CreateCylinder(const Vector3& Scale = Vector3(1, 1, 1));
			btCollisionShape* CreateConvexHull(std::vector<SkinVertex>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vertex>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vector2>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vector3>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vector4>& Mesh);
			btCollisionShape* CreateConvexHull(btCollisionShape* From);
			btCollisionShape* TryCloneShape(btCollisionShape* Shape);
			btCollisionShape* ReuseShape(btCollisionShape* Shape);
			void FreeShape(btCollisionShape** Value);
			std::vector<Vector3> GetShapeVertices(btCollisionShape* Shape);
			uint64_t GetShapeVerticesCount(btCollisionShape* Shape);
			float GetMaxDisplacement();
			float GetAirDensity();
			float GetWaterOffset();
			float GetWaterDensity();
			Vector3 GetWaterNormal();
			Vector3 GetGravity();
			ContactStartedCallback GetOnCollisionEnter();
			ContactEndedCallback GetOnCollisionExit();
			btCollisionConfiguration* GetCollision();
			btBroadphaseInterface* GetBroadphase();
			btConstraintSolver* GetSolver();
			btDiscreteDynamicsWorld* GetWorld();
			btCollisionDispatcher* GetDispatcher();
			btSoftBodySolver* GetSoftSolver();
			bool HasSoftBodySupport();
			int GetContactManifoldCount();

		public:
			static void FreeHullShape(btCollisionShape* Shape);
			static Simulator* Get(btDiscreteDynamicsWorld* From);
			static btCollisionShape* CreateHullShape(std::vector<Vertex>& Mesh);
			static btCollisionShape* CreateHullShape(btCollisionShape* From);
		};

		template <typename T>
		class TH_OUT Math
		{
		public:
			static T Rad2Deg()
			{
				return (T)57.2957795130f;
			}
			static T Deg2Rad()
			{
				return (T)0.01745329251f;
			}
			static T Pi()
			{
				return (T)3.14159265359f;
			}
			static T Sqrt(T Value)
			{
				return (T)std::sqrt((double)Value);
			}
			static T Abs(T Value)
			{
				return Value < 0 ? -Value : Value;
			}
			static T Atan(T Angle)
			{
				return (T)std::atan((double)Angle);
			}
			static T Atan2(T Angle0, T Angle1)
			{
				return (T)std::atan2((double)Angle0, (double)Angle1);
			}
			static T Acos(T Angle)
			{
				return (T)std::acos((double)Angle);
			}
			static T Asin(T Angle)
			{
				return (T)std::asin((double)Angle);
			}
			static T Cos(T Angle)
			{
				return (T)std::cos((double)Angle);
			}
			static T Sin(T Angle)
			{
				return (T)std::sin((double)Angle);
			}
			static T Tan(T Angle)
			{
				return (T)std::tan((double)Angle);
			}
			static T Acotan(T Angle)
			{
				return (T)std::atan(1.0 / (double)Angle);
			}
			static T Max(T Value1, T Value2)
			{
				return Value1 > Value2 ? Value1 : Value2;
			}
			static T Min(T Value1, T Value2)
			{
				return Value1 < Value2 ? Value1 : Value2;
			}
			static T Floor(T Value)
			{
				return (T)std::floor((double)Value);
			}
			static T Lerp(T A, T B, T DeltaTime)
			{
				return A + DeltaTime * (B - A);
			}
			static T StrongLerp(T A, T B, T Time)
			{
				return ((T)1.0 - Time) * A + Time * B;
			}
			static T SaturateAngle(T Angle)
			{
				return (T)std::atan2(std::sin((double)Angle), std::cos((double)Angle));
			}
			static T AngluarLerp(T A, T B, T DeltaTime)
			{
				if (A == B)
					return A;

				Vector2 ACircle = Vector2(cosf((float)A), sinf((float)A));
				Vector2 BCircle = Vector2(cosf((float)B), sinf((float)B));
				Vector2 Interpolation = ACircle.Lerp(BCircle, DeltaTime);

				return (T)std::atan2(Interpolation.Y, Interpolation.X);
			}
			static T AngleDistance(T A, T B)
			{
				return (T)Vector2(std::cos((float)A), std::sin((float)A)).Distance(Vector2(std::cos((float)B), std::sin((float)B)));
			}
			static T Saturate(T Value)
			{
				return Min(Max(Value, 0.0), 1.0);
			}
			static T Random(T Number0, T Number1)
			{
				if (Number0 == Number1)
					return Number0;

				return (T)((double)Number0 + ((double)Number1 - (double)Number0) / std::numeric_limits<uint64_t>::max() * Common::Random());
			}
			static T Round(T Value)
			{
				return (T)std::round((double)Value);
			}
			static T Random()
			{
				return ((T)Common::Random() / std::numeric_limits<uint64_t>::max()) + (T)1.0;
			}
			static T RandomMag()
			{
				return (T)2.0 / std::numeric_limits<uint64_t>::max() * Random() - (T)1.0;
			}
			static T Pow(T Value0, T Value1)
			{
				return (T)std::pow((double)Value0, (double)Value1);
			}
			static T Pow2(T Value0)
			{
				return Value0 * Value0;
			}
			static T Pow3(T Value0)
			{
				return Value0 * Value0 * Value0;
			}
			static T Pow4(T Value0)
			{
				T Value = Value0 * Value0;
				return Value * Value;
			}
			static T Clamp(T Value, T pMin, T pMax)
			{
				return Min(Max(Value, pMin), pMax);
			}
			static T Select(T A, T B)
			{
				if (Common::Random() < std::numeric_limits<uint64_t>::max() / 2)
					return B;

				return A;
			}
			static T Cotan(T Value)
			{
				return (T)(1.0 / std::tan((double)Value));
			}
			static bool NearEqual(T A, T B, T Factor = (T)1.0f)
			{
				T fMin = A - (A - std::nextafter(A, std::numeric_limits<T>::lowest())) * Factor;
				T fMax = A + (std::nextafter(A, std::numeric_limits<T>::max()) - A) * Factor;

				return fMin <= B && fMax >= B;
			}
			static void Swap(T& Value0, T& Value1)
			{
				T Value2 = Value0;
				Value0 = Value1;
				Value1 = Value2;
			}
		};

		typedef Math<float> Mathf;
		typedef Math<double> Mathd;
		typedef Math<int> Mathi;
	}
}
#endif