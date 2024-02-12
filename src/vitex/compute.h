#ifndef VI_COMPUTE_H
#define VI_COMPUTE_H
#include "core.h"
#include <cmath>
#include <map>
#include <stack>
#include <limits>

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
typedef bool(*ContactDestroyedCallback)(void*);
typedef bool(*ContactProcessedCallback)(class btManifoldPoint&, void*, void*);
typedef void(*ContactStartedCallback)(class btPersistentManifold* const&);
typedef void(*ContactEndedCallback)(class btPersistentManifold* const&);

namespace Vitex
{
	namespace Compute
	{
		class RigidBody;

		class SoftBody;

		class Simulator;

		class WebToken;

		class Transform;

		struct Matrix4x4;

		struct Quaternion;

		struct Vector2;

		struct Vector3;

		struct Vector4;

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

		enum class CubeFace
		{
			PositiveX,
			NegativeX,
			PositiveY,
			NegativeY,
			PositiveZ,
			NegativeZ
		};

		enum class Compression
		{
			None = 0,
			BestSpeed = 1,
			BestCompression = 9,
			Default = -1
		};

		enum class IncludeType
		{
			Error,
			Preprocess,
			Unchanged,
			Virtual
		};

		enum class PreprocessorError
		{
			MacroDefinitionEmpty,
			MacroNameEmpty,
			MacroParenthesisDoubleClosed,
			MacroParenthesisNotClosed,
			MacroDefinitionError,
			MacroExpansionParenthesisDoubleClosed,
			MacroExpansionParenthesisNotClosed,
			MacroExpansionArgumentsError,
			MacroExpansionExecutionError,
			MacroExpansionError,
			ConditionNotOpened,
			ConditionNotClosed,
			ConditionError,
			DirectiveNotFound,
			DirectiveExpansionError,
			IncludeDenied,
			IncludeError,
			IncludeNotFound,
			PragmaNotFound,
			PragmaError,
			ExtensionError
		};

		inline SoftCollision operator |(SoftCollision A, SoftCollision B)
		{
			return static_cast<SoftCollision>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		typedef std::function<void(const struct CollisionBody&)> CollisionCallback;
		typedef void* Cipher;
		typedef void* Digest;

		struct VI_OUT IncludeDesc
		{
			Core::Vector<Core::String> Exts;
			Core::String From;
			Core::String Path;
			Core::String Root;
		};

		struct VI_OUT IncludeResult
		{
			Core::String Module;
			bool IsAbstract = false;
			bool IsRemote = false;
			bool IsFile = false;
		};

		struct VI_OUT Vertex
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

		struct VI_OUT SkinVertex
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

		struct VI_OUT ShapeVertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float TexCoordX;
			float TexCoordY;
		};

		struct VI_OUT ElementVertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float Scale;
			float VelocityX;
			float VelocityY;
			float VelocityZ;
			float Rotation;
			float Padding1;
			float Padding2;
			float Padding3;
			float Angular;
			float ColorX;
			float ColorY;
			float ColorZ;
			float ColorW;
		};

		struct VI_OUT Vector2
		{
			float X;
			float Y;

			Vector2() noexcept;
			Vector2(float x, float y) noexcept;
			Vector2(float xy) noexcept;
			Vector2(const Vector2& Value) noexcept;
			Vector2(const Vector3& Value) noexcept;
			Vector2(const Vector4& Value) noexcept;
			bool IsEquals(const Vector2& Other, float MaxDisplacement = 0.000001f) const;
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
			Vector2& operator =(const Vector2& V) noexcept;
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

		struct VI_OUT Vector3
		{
			float X;
			float Y;
			float Z;

			Vector3() noexcept;
			Vector3(const Vector2& Value) noexcept;
			Vector3(const Vector3& Value) noexcept;
			Vector3(const Vector4& Value) noexcept;
			Vector3(float x, float y) noexcept;
			Vector3(float x, float y, float z) noexcept;
			Vector3(float xyz) noexcept;
			bool IsEquals(const Vector3& Other, float MaxDisplacement = 0.000001f) const;
			float Length() const;
			float Sum() const;
			float Dot(const Vector3& B) const;
			float Distance(const Vector3& Target) const;
			float Hypotenuse() const;
			Vector3 LookAt(const Vector3& Vector) const;
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
			Vector3 ViewSpace() const;
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
			Vector3 Rotate(const Vector3& Origin, const Vector3& Rotation);
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
			Vector3& operator =(const Vector3& V) noexcept;
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

		struct VI_OUT Vector4
		{
			float X;
			float Y;
			float Z;
			float W;

			Vector4() noexcept;
			Vector4(const Vector2& Value) noexcept;
			Vector4(const Vector3& Value) noexcept;
			Vector4(const Vector4& Value) noexcept;
			Vector4(float x, float y) noexcept;
			Vector4(float x, float y, float z) noexcept;
			Vector4(float x, float y, float z, float w) noexcept;
			Vector4(float xyzw) noexcept;
			bool IsEquals(const Vector4& Other, float MaxDisplacement = 0.000001f) const;
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
			Vector4 ViewSpace() const;
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
			Vector4& operator =(const Vector4& V) noexcept;
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

		struct VI_OUT Matrix4x4
		{
		public:
			float Row[16];

		public:
			Matrix4x4() noexcept;
			Matrix4x4(float Array[16]) noexcept;
			Matrix4x4(const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3) noexcept;
			Matrix4x4(float Row00, float Row01, float Row02, float Row03, float Row10, float Row11, float Row12, float Row13, float Row20, float Row21, float Row22, float Row23, float Row30, float Row31, float Row32, float Row33) noexcept;
			Matrix4x4(const Matrix4x4& Other) noexcept;
			float& operator [](uint32_t Index);
			float operator [](uint32_t Index) const;
			bool operator ==(const Matrix4x4& Index) const;
			bool operator !=(const Matrix4x4& Index) const;
			Matrix4x4 operator *(const Matrix4x4& V) const;
			Vector4 operator *(const Vector4& V) const;
			Matrix4x4& operator =(const Matrix4x4& V) noexcept;
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
			Quaternion RotationQuaternion() const;
			Vector3 RotationEuler() const;
			Vector3 Position() const;
			Vector3 Scale() const;
			Vector2 XY() const;
			Vector3 XYZ() const;
			Vector4 XYZW() const;
			float Determinant() const;
			void Identify();
			void Set(const Matrix4x4& Value);

		private:
			Matrix4x4(bool) noexcept;

		public:
			static Matrix4x4 CreateLookAt(const Vector3& Position, const Vector3& Target, const Vector3& Up);
			static Matrix4x4 CreateRotationX(float Rotation);
			static Matrix4x4 CreateRotationY(float Rotation);
			static Matrix4x4 CreateRotationZ(float Rotation);
			static Matrix4x4 CreateScale(const Vector3& Scale);
			static Matrix4x4 CreateTranslatedScale(const Vector3& Position, const Vector3& Scale);
			static Matrix4x4 CreateTranslation(const Vector3& Position);
			static Matrix4x4 CreatePerspective(float FieldOfView, float AspectRatio, float NearZ, float FarZ);
			static Matrix4x4 CreatePerspectiveRad(float FieldOfView, float AspectRatio, float NearZ, float FarZ);
			static Matrix4x4 CreateOrthographic(float Width, float Height, float NearZ, float FarZ);
			static Matrix4x4 CreateOrthographicOffCenter(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ);
			static Matrix4x4 Create(const Vector3& Position, const Vector3& Scale, const Vector3& Rotation);
			static Matrix4x4 Create(const Vector3& Position, const Vector3& Rotation);
			static Matrix4x4 CreateRotation(const Vector3& Rotation);
			static Matrix4x4 CreateView(const Vector3& Position, const Vector3& Rotation);
			static Matrix4x4 CreateLookAt(CubeFace Face, const Vector3& Position);
			static Matrix4x4 CreateRotation(const Vector3& Forward, const Vector3& Up, const Vector3& Right);
			static Matrix4x4 Identity()
			{
				return Matrix4x4(
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1);
			};
		};

		struct VI_OUT Quaternion
		{
			float X, Y, Z, W;

			Quaternion() noexcept;
			Quaternion(float x, float y, float z, float w) noexcept;
			Quaternion(const Quaternion& In) noexcept;
			Quaternion(const Vector3& Axis, float Angle) noexcept;
			Quaternion(const Vector3& Euler) noexcept;
			Quaternion(const Matrix4x4& Value) noexcept;
			void SetAxis(const Vector3& Axis, float Angle);
			void SetEuler(const Vector3& Euler);
			void SetMatrix(const Matrix4x4& Value);
			void Set(const Quaternion& Value);
			Quaternion operator *(float r) const;
			Vector3 operator *(const Vector3& r) const;
			Quaternion operator *(const Quaternion& r) const;
			Quaternion operator -(const Quaternion& r) const;
			Quaternion operator +(const Quaternion& r) const;
			Quaternion& operator =(const Quaternion& r) noexcept;
			Quaternion Normalize() const;
			Quaternion sNormalize() const;
			Quaternion Conjugate() const;
			Quaternion Mul(float r) const;
			Quaternion Mul(const Quaternion& r) const;
			Vector3 Mul(const Vector3& r) const;
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
			bool operator ==(const Quaternion& V) const;
			bool operator !=(const Quaternion& V) const;

			static Quaternion CreateEulerRotation(const Vector3& Euler);
			static Quaternion CreateRotation(const Matrix4x4& Transform);
		};

		struct VI_OUT Rectangle
		{
			int64_t Left;
			int64_t Top;
			int64_t Right;
			int64_t Bottom;

			int64_t GetX() const;
			int64_t GetY() const;
			int64_t GetWidth() const;
			int64_t GetHeight() const;
		};

		struct VI_OUT Bounding
		{
		public:
			Vector3 Lower;
			Vector3 Upper;
			Vector3 Center;
			float Radius;
			float Volume;

		public:
			Bounding() noexcept;
			Bounding(const Vector3&, const Vector3&) noexcept;
			void Merge(const Bounding&, const Bounding&);
			bool Contains(const Bounding&) const;
			bool Overlaps(const Bounding&) const;
		};

		struct VI_OUT Ray
		{
			Vector3 Origin;
			Vector3 Direction;

			Ray() noexcept;
			Ray(const Vector3& _Origin, const Vector3& _Direction) noexcept;
			Vector3 GetPoint(float T) const;
			Vector3 operator *(float T) const;
			bool IntersectsPlane(const Vector3& Normal, float Diameter) const;
			bool IntersectsSphere(const Vector3& Position, float Radius, bool DiscardInside = true) const;
			bool IntersectsAABBAt(const Vector3& Min, const Vector3& Max, Vector3* Hit) const;
			bool IntersectsAABB(const Vector3& Position, const Vector3& Scale, Vector3* Hit) const;
			bool IntersectsOBB(const Matrix4x4& World, Vector3* Hit) const;
		};

		struct VI_OUT Frustum8C
		{
			Vector4 Corners[8];

			Frustum8C() noexcept;
			Frustum8C(float FieldOfView, float Aspect, float NearZ, float FarZ) noexcept;
			void Transform(const Matrix4x4& Value);
			void GetBoundingBox(Vector2* X, Vector2* Y, Vector2* Z);
		};

		struct VI_OUT Frustum6P
		{
			enum class Side : size_t
			{
				RIGHT = 0,
				LEFT = 1,
				BOTTOM = 2,
				TOP = 3,
				BACK = 4,
				FRONT = 5
			};

			Vector4 Planes[6];

			Frustum6P() noexcept;
			Frustum6P(const Matrix4x4& ViewProjection) noexcept;
			bool OverlapsAABB(const Bounding& Bounds) const;
			bool OverlapsSphere(const Vector3& Center, float Radius) const;

		private:
			void NormalizePlane(Vector4& Plane);
		};

		struct VI_OUT Joint
		{
			Core::Vector<Joint> Childs;
			Core::String Name;
			Matrix4x4 Global;
			Matrix4x4 Local;
			size_t Index;
		};

		struct VI_OUT AnimatorKey
		{
			Compute::Vector3 Position = 0.0f;
			Compute::Quaternion Rotation;
			Compute::Vector3 Scale = 1.0f;
			float Time = 1.0f;
		};

		struct VI_OUT SkinAnimatorKey
		{
			Core::Vector<AnimatorKey> Pose;
			float Time;
		};

		struct VI_OUT SkinAnimatorClip
		{
			Core::Vector<SkinAnimatorKey> Keys;
			Core::String Name;
			float Duration = 1.0f;
			float Rate = 1.0f;
		};

		struct VI_OUT KeyAnimatorClip
		{
			Core::Vector<AnimatorKey> Keys;
			Core::String Name;
			float Duration = 1.0f;
			float Rate = 1.0f;
		};

		struct VI_OUT RandomVector2
		{
			Vector2 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector2() noexcept;
			RandomVector2(const Vector2& MinV, const Vector2& MaxV, bool IntensityV, float AccuracyV) noexcept;
			Vector2 Generate();
		};

		struct VI_OUT RandomVector3
		{
			Vector3 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector3() noexcept;
			RandomVector3(const Vector3& MinV, const Vector3& MaxV, bool IntensityV, float AccuracyV) noexcept;
			Vector3 Generate();
		};

		struct VI_OUT RandomVector4
		{
			Vector4 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector4() noexcept;
			RandomVector4(const Vector4& MinV, const Vector4& MaxV, bool IntensityV, float AccuracyV) noexcept;
			Vector4 Generate();
		};

		struct VI_OUT RandomFloat
		{
			float Min, Max;
			bool Intensity;
			float Accuracy;

			RandomFloat() noexcept;
			RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV) noexcept;
			float Generate();
		};

		struct VI_OUT RegexBracket
		{
			const char* Pointer = nullptr;
			int64_t Length = 0;
			int64_t Branches = 0;
			int64_t BranchesCount = 0;
		};

		struct VI_OUT RegexBranch
		{
			int64_t BracketIndex;
			const char* Pointer;
		};

		struct VI_OUT RegexMatch
		{
			const char* Pointer;
			int64_t Start;
			int64_t End;
			int64_t Length;
			int64_t Steps;
		};

		struct VI_OUT RegexSource
		{
			friend class Regex;

		private:
			Core::String Expression;
			Core::Vector<RegexBracket> Brackets;
			Core::Vector<RegexBranch> Branches;
			int64_t MaxBranches;
			int64_t MaxBrackets;
			int64_t MaxMatches;
			RegexState State;

		public:
			bool IgnoreCase;

		public:
			RegexSource() noexcept;
			RegexSource(const std::string_view& Regexp, bool fIgnoreCase = false, int64_t fMaxMatches = -1, int64_t fMaxBranches = -1, int64_t fMaxBrackets = -1) noexcept;
			RegexSource(const RegexSource& Other) noexcept;
			RegexSource(RegexSource&& Other) noexcept;
			RegexSource& operator =(const RegexSource& V) noexcept;
			RegexSource& operator =(RegexSource&& V) noexcept;
			const Core::String& GetRegex() const;
			int64_t GetMaxBranches() const;
			int64_t GetMaxBrackets() const;
			int64_t GetMaxMatches() const;
			int64_t GetComplexity() const;
			RegexState GetState() const;
			bool IsSimple() const;

		private:
			void Compile();
		};

		struct VI_OUT RegexResult
		{
			friend class Regex;

		private:
			Core::Vector<RegexMatch> Matches;
			RegexState State;
			int64_t Steps;

		public:
			RegexSource* Src;

		public:
			RegexResult() noexcept;
			RegexResult(const RegexResult& Other) noexcept;
			RegexResult(RegexResult&& Other) noexcept;
			RegexResult& operator =(const RegexResult& V) noexcept;
			RegexResult& operator =(RegexResult&& V) noexcept;
			bool Empty() const;
			int64_t GetSteps() const;
			RegexState GetState() const;
			const Core::Vector<RegexMatch>& Get() const;
			Core::Vector<Core::String> ToArray() const;
		};

		struct VI_OUT PrivateKey
		{
		public:
			template <size_t MaxSize>
			struct Exposable
			{
				char Key[MaxSize];
				size_t Size;

				~Exposable() noexcept
				{
					PrivateKey::RandomizeBuffer(Key, Size);
				}
			};

		private:
			Core::Vector<void*> Blocks;
			Core::String Plain;

		private:
			PrivateKey(const std::string_view& Text, bool) noexcept;
			PrivateKey(Core::String&& Text, bool) noexcept;

		public:
			PrivateKey() noexcept;
			PrivateKey(const PrivateKey& Other) noexcept;
			PrivateKey(PrivateKey&& Other) noexcept;
			explicit PrivateKey(const std::string_view& Key) noexcept;
			~PrivateKey() noexcept;
			PrivateKey& operator =(const PrivateKey& V) noexcept;
			PrivateKey& operator =(PrivateKey&& V) noexcept;
			void Clear();
			void Secure(const std::string_view& Key);
			void ExposeToStack(char* Buffer, size_t MaxSize, size_t* OutSize = nullptr) const;
			Core::String ExposeToHeap() const;
			size_t Size() const;

		public:
			template <size_t MaxSize>
			Exposable<MaxSize> Expose() const
			{
				Exposable<MaxSize> Result;
				ExposeToStack(Result.Key, MaxSize, &Result.Size);
				return Result;
			}

		public:
			static PrivateKey GetPlain(Core::String&& Value);
			static PrivateKey GetPlain(const std::string_view& Value);
			static void RandomizeBuffer(char* Data, size_t Size);

		private:
			char LoadPartition(size_t* Dest, size_t Size, size_t Index) const;
			void RollPartition(size_t* Dest, size_t Size, size_t Index) const;
			void FillPartition(size_t* Dest, size_t Size, size_t Index, char Source) const;
			void CopyDistribution(const PrivateKey& Other);
		};

		struct VI_OUT ShapeContact
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

		struct VI_OUT RayContact
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

		struct VI_OUT CollisionBody
		{
			RigidBody* Rigid = nullptr;
			SoftBody* Soft = nullptr;

			CollisionBody(btCollisionObject* Object) noexcept;
		};

		struct VI_OUT AdjTriangle
		{
			uint32_t VRef[3];
			uint32_t ATri[3];

			uint8_t FindEdge(uint32_t VRef0, uint32_t VRef1);
			uint32_t OppositeVertex(uint32_t VRef0, uint32_t VRef1);
		};

		struct VI_OUT AdjEdge
		{
			uint32_t Ref0;
			uint32_t Ref1;
			uint32_t FaceNb;
		};

		struct VI_OUT ProcDirective
		{
			Core::String Name;
			Core::String Value;
			size_t Start = 0;
			size_t End = 0;
			bool Found = false;
			bool AsGlobal = false;
			bool AsScope = false;
		};

		struct VI_OUT UInt128
		{
		private:
#ifdef VI_ENDIAN_BIG
			uint64_t Upper, Lower;
#else
			uint64_t Lower, Upper;
#endif
		public:
			UInt128() = default;
			UInt128(const UInt128& Right) = default;
			UInt128(UInt128&& Right) = default;
			UInt128(const std::string_view& Text);
			UInt128(const std::string_view& Text, uint8_t Base);
			UInt128& operator=(const UInt128& Right) = default;
			UInt128& operator=(UInt128&& Right) = default;
			operator bool() const;
			operator uint8_t() const;
			operator uint16_t() const;
			operator uint32_t() const;
			operator uint64_t() const;
			UInt128 operator&(const UInt128& Right) const;
			UInt128& operator&=(const UInt128& Right);
			UInt128 operator|(const UInt128& Right) const;
			UInt128& operator|=(const UInt128& Right);
			UInt128 operator^(const UInt128& Right) const;
			UInt128& operator^=(const UInt128& Right);
			UInt128 operator~() const;
			UInt128 operator<<(const UInt128& Right) const;
			UInt128& operator<<=(const UInt128& Right);
			UInt128 operator>>(const UInt128& Right) const;
			UInt128& operator>>=(const UInt128& Right);
			bool operator!() const;
			bool operator&&(const UInt128& Right) const;
			bool operator||(const UInt128& Right) const;
			bool operator==(const UInt128& Right) const;
			bool operator!=(const UInt128& Right) const;
			bool operator>(const UInt128& Right) const;
			bool operator<(const UInt128& Right) const;
			bool operator>=(const UInt128& Right) const;
			bool operator<=(const UInt128& Right) const;
			UInt128 operator+(const UInt128& Right) const;
			UInt128& operator+=(const UInt128& Right);
			UInt128 operator-(const UInt128& Right) const;
			UInt128& operator-=(const UInt128& Right);
			UInt128 operator*(const UInt128& Right) const;
			UInt128& operator*=(const UInt128& Right);
			UInt128 operator/(const UInt128& Right) const;
			UInt128& operator/=(const UInt128& Right);
			UInt128 operator%(const UInt128& Right) const;
			UInt128& operator%=(const UInt128& Right);
			UInt128& operator++();
			UInt128 operator++(int);
			UInt128& operator--();
			UInt128 operator--(int);
			UInt128 operator+() const;
			UInt128 operator-() const;
			const uint64_t& High() const;
			const uint64_t& Low() const;
			uint8_t Bits() const;
			Core::Decimal ToDecimal() const;
			Core::String ToString(uint8_t Base = 10, uint32_t Length = 0) const;
			VI_OUT friend std::ostream& operator<<(std::ostream& Stream, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint64_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int64_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint64_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int64_t& Left, const UInt128& Right);

		public:
			static UInt128 Min();
			static UInt128 Max();

		private:
			std::pair<UInt128, UInt128> Divide(const UInt128& Left, const UInt128& Right) const;

		public:
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128(const T& Right)
#ifdef VI_ENDIAN_BIG
				: Upper(0), Lower(Right)
#else
				: Lower(Right), Upper(0)
#endif
			{
				if (std::is_signed<T>::value && Right < 0)
					Upper = -1;
			}
			template <typename S, typename T, typename = typename std::enable_if<std::is_integral<S>::value&& std::is_integral<T>::value, void>::type>
			UInt128(const S& UpperRight, const T& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UpperRight), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UpperRight)
#endif
			{
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator=(const T& Right)
			{
				Upper = 0;
				if (std::is_signed<T>::value && Right < 0)
					Upper = -1;

				Lower = Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator&(const T& Right) const
			{
				return UInt128(0, Lower & (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator&=(const T& Right)
			{
				Upper = 0;
				Lower &= Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator|(const T& Right) const
			{
				return UInt128(Upper, Lower | (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator|=(const T& Right)
			{
				Lower |= (uint64_t)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator^(const T& Right) const
			{
				return UInt128(Upper, Lower ^ (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator^=(const T& Right)
			{
				Lower ^= (uint64_t)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator<<(const T& Right) const
			{
				return *this << UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator<<=(const T& Right)
			{
				*this = *this << UInt128(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator>>(const T& Right) const
			{
				return *this >> UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator>>=(const T& Right)
			{
				*this = *this >> UInt128(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator&&(const T& Right) const
			{
				return static_cast <bool> (*this && Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator||(const T& Right) const
			{
				return static_cast <bool> (*this || Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator==(const T& Right) const
			{
				return (!Upper && (Lower == (uint64_t)Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator!=(const T& Right) const
			{
				return (Upper || (Lower != (uint64_t)Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>(const T& Right) const
			{
				return (Upper || (Lower > (uint64_t)Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<(const T& Right) const
			{
				return (!Upper) ? (Lower < (uint64_t)Right) : false;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>=(const T& Right) const
			{
				return ((*this > Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<=(const T& Right) const
			{
				return ((*this < Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator+(const T& Right) const
			{
				return UInt128(Upper + ((Lower + (uint64_t)Right) < Lower), Lower + (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator+=(const T& Right)
			{
				return *this += UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator-(const T& Right) const
			{
				return UInt128((uint64_t)(Upper - ((Lower - Right) > Lower)), (uint64_t)(Lower - Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator-=(const T& Right)
			{
				return *this = *this - UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator*(const T& Right) const
			{
				return *this * UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator*=(const T& Right)
			{
				return *this = *this * UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator/(const T& Right) const
			{
				return *this / UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator/=(const T& Right)
			{
				return *this = *this / UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator%(const T& Right) const
			{
				return *this % UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator%=(const T& Right)
			{
				return *this = *this % UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator&(const T& Left, const UInt128& Right)
			{
				return Right & Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator&=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right & Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator|(const T& Left, const UInt128& Right)
			{
				return Right | Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator|=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right | Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator^(const T& Left, const UInt128& Right)
			{
				return Right ^ Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator^=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right ^ Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator<<=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) << Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator>>=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) >> Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator==(const T& Left, const UInt128& Right)
			{
				return (!Right.High() && ((uint64_t)Left == Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator!=(const T& Left, const UInt128& Right)
			{
				return (Right.High() || ((uint64_t)Left != Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>(const T& Left, const UInt128& Right)
			{
				return (!Right.High()) && ((uint64_t)Left > Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<(const T& Left, const UInt128& Right)
			{
				if (Right.High())
					return true;
				return ((uint64_t)Left < Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>=(const T& Left, const UInt128& Right)
			{
				if (Right.High())
					return false;
				return ((uint64_t)Left >= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<=(const T& Left, const UInt128& Right)
			{
				if (Right.High())
					return true;
				return ((uint64_t)Left <= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator+(const T& Left, const UInt128& Right)
			{
				return Right + Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator+=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right + Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator-(const T& Left, const UInt128& Right)
			{
				return -(Right - Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator-=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (-(Right - Left));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator*(const T& Left, const UInt128& Right)
			{
				return Right * Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator*=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right * Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator/(const T& Left, const UInt128& Right)
			{
				return UInt128(Left) / Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator/=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) / Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator%(const T& Left, const UInt128& Right)
			{
				return UInt128(Left) % Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator%=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) % Right);
			}
		};

		struct VI_OUT UInt256
		{
		private:
#ifdef VI_ENDIAN_BIG
			UInt128 Upper, Lower;
#else
			UInt128 Lower, Upper;
#endif

		public:
			UInt256() = default;
			UInt256(const UInt256& Right) = default;
			UInt256(UInt256&& Right) = default;
			UInt256(const std::string_view& Text);
			UInt256(const std::string_view& Text, uint8_t Base);
			UInt256(const UInt128& UpperRight, const UInt128& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UpperRight), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UpperRight)
#endif
			{
			}
			UInt256(const UInt128& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UInt128::Min()), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UInt128::Min())
#endif
			{
			}
			UInt256& operator=(const UInt256& Right) = default;
			UInt256& operator=(UInt256&& Right) = default;
			operator bool() const;
			operator uint8_t() const;
			operator uint16_t() const;
			operator uint32_t() const;
			operator uint64_t() const;
			operator UInt128() const;
			UInt256 operator&(const UInt128& Right) const;
			UInt256 operator&(const UInt256& Right) const;
			UInt256& operator&=(const UInt128& Right);
			UInt256& operator&=(const UInt256& Right);
			UInt256 operator|(const UInt128& Right) const;
			UInt256 operator|(const UInt256& Right) const;
			UInt256& operator|=(const UInt128& Right);
			UInt256& operator|=(const UInt256& Right);
			UInt256 operator^(const UInt128& Right) const;
			UInt256 operator^(const UInt256& Right) const;
			UInt256& operator^=(const UInt128& Right);
			UInt256& operator^=(const UInt256& Right);
			UInt256 operator~() const;
			UInt256 operator<<(const UInt128& Shift) const;
			UInt256 operator<<(const UInt256& Shift) const;
			UInt256& operator<<=(const UInt128& Shift);
			UInt256& operator<<=(const UInt256& Shift);
			UInt256 operator>>(const UInt128& Shift) const;
			UInt256 operator>>(const UInt256& Shift) const;
			UInt256& operator>>=(const UInt128& Shift);
			UInt256& operator>>=(const UInt256& Shift);
			bool operator!() const;
			bool operator&&(const UInt128& Right) const;
			bool operator&&(const UInt256& Right) const;
			bool operator||(const UInt128& Right) const;
			bool operator||(const UInt256& Right) const;
			bool operator==(const UInt128& Right) const;
			bool operator==(const UInt256& Right) const;
			bool operator!=(const UInt128& Right) const;
			bool operator!=(const UInt256& Right) const;
			bool operator>(const UInt128& Right) const;
			bool operator>(const UInt256& Right) const;
			bool operator<(const UInt128& Right) const;
			bool operator<(const UInt256& Right) const;
			bool operator>=(const UInt128& Right) const;
			bool operator>=(const UInt256& Right) const;
			bool operator<=(const UInt128& Right) const;
			bool operator<=(const UInt256& Right) const;
			UInt256 operator+(const UInt128& Right) const;
			UInt256 operator+(const UInt256& Right) const;
			UInt256& operator+=(const UInt128& Right);
			UInt256& operator+=(const UInt256& Right);
			UInt256 operator-(const UInt128& Right) const;
			UInt256 operator-(const UInt256& Right) const;
			UInt256& operator-=(const UInt128& Right);
			UInt256& operator-=(const UInt256& Right);
			UInt256 operator*(const UInt128& Right) const;
			UInt256 operator*(const UInt256& Right) const;
			UInt256& operator*=(const UInt128& Right);
			UInt256& operator*=(const UInt256& Right);
			UInt256 operator/(const UInt128& Right) const;
			UInt256 operator/(const UInt256& Right) const;
			UInt256& operator/=(const UInt128& Right);
			UInt256& operator/=(const UInt256& Right);
			UInt256 operator%(const UInt128& Right) const;
			UInt256 operator%(const UInt256& Right) const;
			UInt256& operator%=(const UInt128& Right);
			UInt256& operator%=(const UInt256& Right);
			UInt256& operator++();
			UInt256 operator++(int);
			UInt256& operator--();
			UInt256 operator--(int);
			UInt256 operator+() const;
			UInt256 operator-() const;
			const UInt128& High() const;
			const UInt128& Low() const;
			uint16_t Bits() const;
			Core::Decimal ToDecimal() const;
			Core::String ToString(uint8_t Base = 10, uint32_t Length = 0) const;
			VI_OUT friend UInt256 operator&(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator&=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator|(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator|=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator^(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator^=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint64_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int64_t& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator<<=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint64_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int64_t& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator>>=(UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator==(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator!=(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator>(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator<(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator>=(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator<=(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator+(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator+=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator-(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator-=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator*(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator*=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator/(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator/=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator%(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator%=(UInt128& Left, const UInt256& Right);
			VI_OUT friend std::ostream& operator<<(std::ostream& Stream, const UInt256& Right);

		public:
			static UInt256 Min();
			static UInt256 Max();

		private:
			std::pair<UInt256, UInt256> Divide(const UInt256& Left, const UInt256& Right) const;

		public:
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
			UInt256(const T& Right)
#ifdef VI_ENDIAN_BIG
				: Upper(UInt128::Min()), Lower(Right)
#else
				: Lower(Right), Upper(UInt128::Min())
#endif
			{
				if (std::is_signed<T>::value && Right < 0)
					Upper = UInt128(-1, -1);
			}
			template <typename S, typename T, typename = typename std::enable_if <std::is_integral<S>::value&& std::is_integral<T>::value, void>::type>
			UInt256(const S& UpperRight, const T& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UpperRight), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UpperRight)
#endif
			{
			}
			template <typename R, typename S, typename T, typename U, typename = typename std::enable_if<std::is_integral<R>::value&& std::is_integral<S>::value&& std::is_integral<T>::value&& std::is_integral<U>::value, void>::type>
			UInt256(const R& upper_lhs, const S& lower_lhs, const T& UpperRight, const U& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(upper_lhs, lower_lhs), Lower(UpperRight, LowerRight)
#else
				: Lower(UpperRight, LowerRight), Upper(upper_lhs, lower_lhs)
#endif
			{
			}
			template <typename T, typename = typename std::enable_if <std::is_integral<T>::value, T>::type>
			UInt256& operator=(const T& Right)
			{
				Upper = UInt128::Min();
				if (std::is_signed<T>::value && Right < 0)
					Upper = UInt128(-1, -1);

				Lower = Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator&(const T& Right) const
			{
				return UInt256(UInt128::Min(), Lower & (UInt128)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator&=(const T& Right)
			{
				Upper = UInt128::Min();
				Lower &= Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator|(const T& Right) const
			{
				return UInt256(Upper, Lower | UInt128(Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator|=(const T& Right)
			{
				Lower |= (UInt128)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator^(const T& Right) const
			{
				return UInt256(Upper, Lower ^ (UInt128)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator^=(const T& Right)
			{
				Lower ^= (UInt128)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator<<(const T& Right) const
			{
				return *this << UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator<<=(const T& Right)
			{
				*this = *this << UInt256(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator>>(const T& Right) const
			{
				return *this >> UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator>>=(const T& Right)
			{
				*this = *this >> UInt256(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator&&(const T& Right) const
			{
				return ((bool)*this && Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator||(const T& Right) const
			{
				return ((bool)*this || Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator==(const T& Right) const
			{
				return (!Upper && (Lower == UInt128(Right)));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator!=(const T& Right) const
			{
				return ((bool)Upper || (Lower != UInt128(Right)));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>(const T& Right) const
			{
				return ((bool)Upper || (Lower > UInt128(Right)));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<(const T& Right) const
			{
				return (!Upper) ? (Lower < UInt128(Right)) : false;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>=(const T& Right) const
			{
				return ((*this > Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<=(const T& Right) const
			{
				return ((*this < Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator+(const T& Right) const
			{
				return UInt256(Upper + ((Lower + (UInt128)Right) < Lower), Lower + (UInt128)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator+=(const T& Right)
			{
				return *this += UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator-(const T& Right) const
			{
				return UInt256(Upper - ((Lower - Right) > Lower), Lower - Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator-=(const T& Right)
			{
				return *this = *this - UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator*(const T& Right) const
			{
				return *this * UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator*=(const T& Right)
			{
				return *this = *this * UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator/(const T& Right) const
			{
				return *this / UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator/=(const T& Right)
			{
				return *this = *this / UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator%(const T& Right) const
			{
				return *this % UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator%=(const T& Right)
			{
				return *this = *this % UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator&(const T& Left, const UInt256& Right)
			{
				return Right & Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator&=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right & Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator|(const T& Left, const UInt256& Right)
			{
				return Right | Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator|=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right | Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator^(const T& Left, const UInt256& Right)
			{
				return Right ^ Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator^=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right ^ Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator<<=(T& Left, const UInt256& Right)
			{
				Left = static_cast <T> (UInt256(Left) << Right);
				return Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator>>=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (UInt256(Left) >> Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator==(const T& Left, const UInt256& Right)
			{
				return (!Right.High() && ((uint64_t)Left == Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator!=(const T& Left, const UInt256& Right)
			{
				return (Right.High() || ((uint64_t)Left != Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>(const T& Left, const UInt256& Right)
			{
				return Right.High() ? false : ((UInt128)Left > Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<(const T& Left, const UInt256& Right)
			{
				return Right.High() ? true : ((UInt128)Left < Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>=(const T& Left, const UInt256& Right)
			{
				return Right.High() ? false : ((UInt128)Left >= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<=(const T& Left, const UInt256& Right)
			{
				return Right.High() ? true : ((UInt128)Left <= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator+(const T& Left, const UInt256& Right)
			{
				return Right + Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator+=(T& Left, const UInt256& Right)
			{
				Left = static_cast <T> (Right + Left);
				return Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator-(const T& Left, const UInt256& Right)
			{
				return -(Right - Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator-=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (-(Right - Left));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator*(const T& Left, const UInt256& Right)
			{
				return Right * Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator*=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right * Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator/(const T& Left, const UInt256& Right)
			{
				return UInt256(Left) / Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator/=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (UInt256(Left) / Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator%(const T& Left, const UInt256& Right)
			{
				return UInt256(Left) % Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator%=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (UInt256(Left) % Right);
			}
		};

		class PreprocessorException final : public Core::BasicException
		{
		private:
			PreprocessorError Type;
			size_t Offset;

		public:
			VI_OUT PreprocessorException(PreprocessorError NewType);
			VI_OUT PreprocessorException(PreprocessorError NewType, size_t NewOffset);
			VI_OUT PreprocessorException(PreprocessorError NewType, size_t NewOffset, const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
			VI_OUT PreprocessorError status() const noexcept;
			VI_OUT size_t offset() const noexcept;
		};

		class CryptoException final : public Core::BasicException
		{
		private:
			size_t ErrorCode;

		public:
			VI_OUT CryptoException();
			VI_OUT CryptoException(size_t ErrorCode, const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
			VI_OUT size_t error_code() const noexcept;
		};

		class CompressionException final : public Core::BasicException
		{
		private:
			int ErrorCode;

		public:
			VI_OUT CompressionException(int ErrorCode, const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
			VI_OUT int error_code() const noexcept;
		};

		template <typename V>
		using ExpectsPreprocessor = Core::Expects<V, PreprocessorException>;

		template <typename V>
		using ExpectsCrypto = Core::Expects<V, CryptoException>;

		template <typename V>
		using ExpectsCompression = Core::Expects<V, CompressionException>;

		typedef std::function<ExpectsPreprocessor<IncludeType>(class Preprocessor*, const struct IncludeResult& File, Core::String& Output)> ProcIncludeCallback;
		typedef std::function<ExpectsPreprocessor<void>(class Preprocessor*, const std::string_view& Name, const Core::Vector<Core::String>& Args)> ProcPragmaCallback;
		typedef std::function<ExpectsPreprocessor<void>(class Preprocessor*, const struct ProcDirective&, Core::String& Output)> ProcDirectiveCallback;
		typedef std::function<ExpectsPreprocessor<Core::String>(class Preprocessor*, const Core::Vector<Core::String>& Args)> ProcExpansionCallback;

		class VI_OUT Adjacencies
		{
		public:
			struct Desc
			{
				uint32_t NbFaces = 0;
				uint32_t* Faces = nullptr;
			};

		private:
			uint32_t NbEdges;
			uint32_t CurrentNbFaces;
			AdjEdge* Edges;

		public:
			uint32_t NbFaces;
			AdjTriangle* Faces;

		public:
			Adjacencies() noexcept;
			~Adjacencies() noexcept;
			bool Fill(Adjacencies::Desc& I);
			bool Resolve();

		private:
			bool AddTriangle(uint32_t Ref0, uint32_t Ref1, uint32_t Ref2);
			bool AddEdge(uint32_t Ref0, uint32_t Ref1, uint32_t Face);
			bool UpdateLink(uint32_t FirstTri, uint32_t SecondTri, uint32_t Ref0, uint32_t Ref1);
		};

		class VI_OUT TriangleStrip
		{
		public:
			struct Desc
			{
				uint32_t* Faces = nullptr;
				uint32_t NbFaces = 0;
				bool OneSided = true;
				bool SGICipher = true;
				bool ConnectAllStrips = false;
			};

			struct Result
			{
				Core::Vector<uint32_t> Strips;
				Core::Vector<uint32_t> Groups;

				Core::Vector<int> GetIndices(int Group = -1);
				Core::Vector<int> GetInvIndices(int Group = -1);
			};

		private:
			Core::Vector<uint32_t> SingleStrip;
			Core::Vector<uint32_t> StripLengths;
			Core::Vector<uint32_t> StripRuns;
			Adjacencies* Adj;
			bool* Tags;
			uint32_t NbStrips;
			uint32_t TotalLength;
			bool OneSided;
			bool SGICipher;
			bool ConnectAllStrips;

		public:
			TriangleStrip() noexcept;
			~TriangleStrip() noexcept;
			bool Fill(const TriangleStrip::Desc& I);
			bool Resolve(TriangleStrip::Result& Result);

		private:
			TriangleStrip& FreeBuffers();
			uint32_t ComputeStrip(uint32_t Face);
			uint32_t TrackStrip(uint32_t Face, uint32_t Oldest, uint32_t Middle, uint32_t* Strip, uint32_t* Faces, bool* Tags);
			bool ConnectStrips(TriangleStrip::Result& Result);
		};

		class VI_OUT RadixSorter
		{
		private:
			uint32_t* Histogram;
			uint32_t* Offset;
			uint32_t CurrentSize;
			uint32_t* Indices;
			uint32_t* Indices2;

		public:
			RadixSorter() noexcept;
			RadixSorter(const RadixSorter& Other) noexcept;
			RadixSorter(RadixSorter&& Other) noexcept;
			~RadixSorter() noexcept;
			RadixSorter& operator =(const RadixSorter& V);
			RadixSorter& operator =(RadixSorter&& V) noexcept;
			RadixSorter& Sort(uint32_t* Input, uint32_t Nb, bool SignedValues = true);
			RadixSorter& Sort(float* Input, uint32_t Nb);
			RadixSorter& ResetIndices();
			uint32_t* GetIndices();
		};

		class VI_OUT MD5Hasher
		{
		private:
			typedef uint8_t UInt1;
			typedef uint32_t UInt4;

		private:
			uint32_t S11 = 7;
			uint32_t S12 = 12;
			uint32_t S13 = 17;
			uint32_t S14 = 22;
			uint32_t S21 = 5;
			uint32_t S22 = 9;
			uint32_t S23 = 14;
			uint32_t S24 = 20;
			uint32_t S31 = 4;
			uint32_t S32 = 11;
			uint32_t S33 = 16;
			uint32_t S34 = 23;
			uint32_t S41 = 6;
			uint32_t S42 = 10;
			uint32_t S43 = 15;
			uint32_t S44 = 21;

		private:
			bool Finalized;
			UInt1 Buffer[64];
			UInt4 Count[2];
			UInt4 State[4];
			UInt1 Digest[16];

		public:
			MD5Hasher() noexcept;
			void Transform(const UInt1* Buffer, uint32_t Length = 64);
			void Update(const std::string_view& Buffer, uint32_t BlockSize = 64);
			void Update(const uint8_t* Buffer, uint32_t Length, uint32_t BlockSize = 64);
			void Update(const char* Buffer, uint32_t Length, uint32_t BlockSize = 64);
			void Finalize();
			Core::Unique<char> HexDigest() const;
			Core::Unique<uint8_t> RawDigest() const;
			Core::String ToHex() const;
			Core::String ToRaw() const;

		private:
			static void Decode(UInt4* Output, const UInt1* Input, uint32_t Length);
			static void Encode(UInt1* Output, const UInt4* Input, uint32_t Length);
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

		class VI_OUT S8Hasher
		{
		public:
			S8Hasher() noexcept = default;
			S8Hasher(const S8Hasher&) noexcept = default;
			S8Hasher(S8Hasher&&) noexcept = default;
			~S8Hasher() noexcept = default;
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

		class VI_OUT_TS Ciphers
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

		class VI_OUT_TS Digests
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

		class VI_OUT_TS Crypto
		{
		public:
			typedef std::function<void(uint8_t**, size_t*)> BlockCallback;

		public:
			static Digest GetDigestByName(const std::string_view& Name);
			static Cipher GetCipherByName(const std::string_view& Name);
			static std::string_view GetDigestName(Digest Type);
			static std::string_view GetCipherName(Cipher Type);
			static ExpectsCrypto<void> FillRandomBytes(uint8_t* Buffer, size_t Length);
			static ExpectsCrypto<Core::String> RandomBytes(size_t Length);
			static ExpectsCrypto<Core::String> ChecksumHex(Digest Type, Core::Stream* Stream);
			static ExpectsCrypto<Core::String> ChecksumRaw(Digest Type, Core::Stream* Stream);
			static ExpectsCrypto<Core::String> HashHex(Digest Type, const std::string_view& Value);
			static ExpectsCrypto<Core::String> HashRaw(Digest Type, const std::string_view& Value);
			static ExpectsCrypto<Core::String> Sign(Digest Type, const std::string_view& Value, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> HMAC(Digest Type, const std::string_view& Value, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> Encrypt(Cipher Type, const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static ExpectsCrypto<Core::String> Decrypt(Cipher Type, const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static ExpectsCrypto<Core::String> JWTSign(const std::string_view& Algo, const std::string_view& Payload, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> JWTEncode(WebToken* Src, const PrivateKey& Key);
			static ExpectsCrypto<Core::Unique<WebToken>> JWTDecode(const std::string_view& Value, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> DocEncrypt(Core::Schema* Src, const PrivateKey& Key, const PrivateKey& Salt);
			static ExpectsCrypto<Core::Unique<Core::Schema>> DocDecrypt(const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt);
			static ExpectsCrypto<size_t> Encrypt(Cipher Type, Core::Stream* From, Core::Stream* To, const PrivateKey& Key, const PrivateKey& Salt, BlockCallback&& Callback = nullptr, size_t ReadInterval = 1, int ComplexityBytes = -1);
			static ExpectsCrypto<size_t> Decrypt(Cipher Type, Core::Stream* From, Core::Stream* To, const PrivateKey& Key, const PrivateKey& Salt, BlockCallback&& Callback = nullptr, size_t ReadInterval = 1, int ComplexityBytes = -1);
			static uint8_t RandomUC();
			static uint64_t CRC32(const std::string_view& Data);
			static uint64_t Random(uint64_t Min, uint64_t Max);
			static uint64_t Random();
			static void Sha1CollapseBufferBlock(uint32_t* Buffer);
			static void Sha1ComputeHashBlock(uint32_t* Result, uint32_t* W);
			static void Sha1Compute(const void* Value, int Length, char* Hash20);
			static void Sha1Hash20ToHex(const char* Hash20, char* HexString);
			static void DisplayCryptoLog();
		};

		class VI_OUT_TS Codec
		{
		public:
			static Core::String Move(const std::string_view& Text, int Offset);
			static Core::String Encode64(const char Alphabet[65], const uint8_t* Value, size_t Length, bool Padding);
			static Core::String Decode64(const char Alphabet[65], const uint8_t* Value, size_t Length, bool(*IsAlphabetic)(uint8_t));
			static Core::String Bep45Encode(const std::string_view& Value);
			static Core::String Bep45Decode(const std::string_view& Value);
			static Core::String Base32Encode(const std::string_view& Value);
			static Core::String Base32Decode(const std::string_view& Value);
			static Core::String Base45Encode(const std::string_view& Value);
			static Core::String Base45Decode(const std::string_view& Value);
			static Core::String Base64Encode(const std::string_view& Value);
			static Core::String Base64Decode(const std::string_view& Value);
			static Core::String Base64URLEncode(const std::string_view& Value);
			static Core::String Base64URLDecode(const std::string_view& Value);
			static Core::String Shuffle(const char* Value, size_t Size, uint64_t Mask);
			static ExpectsCompression<Core::String> Compress(const std::string_view& Data, Compression Type = Compression::Default);
			static ExpectsCompression<Core::String> Decompress(const std::string_view& Data);
			static Core::String HexEncodeOdd(const std::string_view& Value, bool UpperCase = false);
			static Core::String HexEncode(const std::string_view& Value, bool UpperCase = false);
			static Core::String HexDecode(const std::string_view& Value);
			static Core::String URLEncode(const std::string_view& Text);
			static Core::String URLDecode(const std::string_view& Text);
			static Core::String DecimalToHex(uint64_t V);
			static Core::String Base10ToBaseN(uint64_t Value, uint32_t BaseLessThan65);
			static size_t Utf8(int Code, char* Buffer);
			static bool Hex(char Code, int& Value);
			static bool HexToString(const std::string_view& Data, char* Buffer, size_t BufferLength);
			static bool HexToDecimal(const std::string_view& Text, size_t Index, size_t Size, int& Value);
			static bool IsBase64URL(uint8_t Value);
			static bool IsBase64(uint8_t Value);
		};

		class VI_OUT_TS Geometric
		{
		private:
			static bool LeftHanded;

		public:
			static bool IsCubeInFrustum(const Matrix4x4& WorldViewProjection, float Radius);
			static bool IsLeftHanded();
			static bool HasSphereIntersected(const Vector3& PositionR0, float RadiusR0, const Vector3& PositionR1, float RadiusR1);
			static bool HasLineIntersected(float DistanceF, float DistanceD, const Vector3& Start, const Vector3& End, Vector3& Hit);
			static bool HasLineIntersectedCube(const Vector3& Min, const Vector3& Max, const Vector3& Start, const Vector3& End);
			static bool HasPointIntersectedCube(const Vector3& Hit, const Vector3& Position, const Vector3& Scale, int Axis);
			static bool HasPointIntersectedRectangle(const Vector3& Position, const Vector3& Scale, const Vector3& P0);
			static bool HasPointIntersectedCube(const Vector3& Position, const Vector3& Scale, const Vector3& P0);
			static bool HasSBIntersected(Transform* BoxR0, Transform* BoxR1);
			static bool HasOBBIntersected(Transform* BoxR0, Transform* BoxR1);
			static bool HasAABBIntersected(Transform* BoxR0, Transform* BoxR1);
			static void FlipIndexWindingOrder(Core::Vector<int>& Indices);
			static void MatrixRhToLh(Compute::Matrix4x4* Matrix);
			static void SetLeftHanded(bool IsLeftHanded);
			static Core::Vector<int> CreateTriangleStrip(TriangleStrip::Desc& Desc, const Core::Vector<int>& Indices);
			static Core::Vector<int> CreateTriangleList(const Core::Vector<int>& Indices);
			static void CreateFrustum8CRad(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ);
			static void CreateFrustum8C(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ);
			static Ray CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView);
			static bool CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale, Vector3* Hit = nullptr);
			static bool CursorRayTest(const Ray& Cursor, const Matrix4x4& World, Vector3* Hit = nullptr);
			static float FastInvSqrt(float Value);
			static float FastSqrt(float Value);
			static float AabbVolume(const Vector3& Min, const Vector3& Max);

		public:
			template <typename T>
			static void TexCoordRhToLh(Core::Vector<T>& Vertices, bool Always = false)
			{
				if (IsLeftHanded() || Always)
					return;

				for (auto& Item : Vertices)
					Item.TexCoordY = 1.0f - Item.TexCoordY;
			}
		};

		class VI_OUT_TS Regex
		{
			friend RegexSource;

		private:
			static int64_t Meta(const uint8_t* Buffer);
			static int64_t OpLength(const char* Value);
			static int64_t SetLength(const char* Value, int64_t ValueLength);
			static int64_t GetOpLength(const char* Value, int64_t ValueLength);
			static int64_t Quantifier(const char* Value);
			static int64_t ToInt(int64_t x);
			static int64_t HexToInt(const uint8_t* Buffer);
			static int64_t MatchOp(const uint8_t* Value, const uint8_t* Buffer, RegexResult* Info);
			static int64_t MatchSet(const char* Value, int64_t ValueLength, const char* Buffer, RegexResult* Info);
			static int64_t ParseDOH(const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case);
			static int64_t ParseInner(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case);
			static int64_t Parse(const char* Buffer, int64_t BufferLength, RegexResult* Info);

		public:
			static bool Match(RegexSource* Value, RegexResult& Result, const std::string_view& Buffer);
			static bool Replace(RegexSource* Value, const std::string_view& ToExpression, Core::String& Buffer);
			static const char* Syntax();
		};

		class VI_OUT WebToken final : public Core::Reference<WebToken>
		{
		public:
			Core::Schema* Header;
			Core::Schema* Payload;
			Core::Schema* Token;
			Core::String Refresher;
			Core::String Signature;
			Core::String Data;

		public:
			WebToken() noexcept;
			WebToken(const std::string_view& Issuer, const std::string_view& Subject, int64_t Expiration) noexcept;
			virtual ~WebToken() noexcept;
			void Unsign();
			void SetAlgorithm(const std::string_view& Value);
			void SetType(const std::string_view& Value);
			void SetContentType(const std::string_view& Value);
			void SetIssuer(const std::string_view& Value);
			void SetSubject(const std::string_view& Value);
			void SetId(const std::string_view& Value);
			void SetAudience(const Core::Vector<Core::String>& Value);
			void SetExpiration(int64_t Value);
			void SetNotBefore(int64_t Value);
			void SetCreated(int64_t Value);
			void SetRefreshToken(const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt);
			bool Sign(const PrivateKey& Key);
			ExpectsCrypto<Core::String> GetRefreshToken(const PrivateKey& Key, const PrivateKey& Salt);
			bool IsValid() const;
		};

		class VI_OUT Preprocessor final : public Core::Reference<Preprocessor>
		{
		public:
			struct Desc
			{
				Core::String MultilineCommentBegin = "/*";
				Core::String MultilineCommentEnd = "*/";
				Core::String CommentBegin = "//";
				Core::String StringLiterals = "\"'`";
				bool Pragmas = true;
				bool Includes = true;
				bool Defines = true;
				bool Conditions = true;
			};

		private:
			enum class Controller
			{
				StartIf = 0,
				ElseIf = 1,
				Else = 2,
				EndIf = 3
			};

			enum class Condition
			{
				Exists = 1,
				Equals = 2,
				Greater = 3,
				GreaterEquals = 4,
				Less = 5,
				LessEquals = 6,
				Text = 0,
				NotExists = -1,
				NotEquals = -2,
				NotGreater = -3,
				NotGreaterEquals = -4,
				NotLess = -5,
				NotLessEquals = -6,
			};

			struct Conditional
			{
				Core::Vector<Conditional> Childs;
				Core::String Expression;
				bool Chaining = false;
				Condition Type = Condition::Text;
				size_t TokenStart = 0;
				size_t TokenEnd = 0;
				size_t TextStart = 0;
				size_t TextEnd = 0;
			};

			struct Definition
			{
				Core::Vector<Core::String> Tokens;
				Core::String Expansion;
				ProcExpansionCallback Callback;
			};

		private:
			struct FileContext
			{
				Core::String Path;
				size_t Line = 0;
			} ThisFile;

		private:
			Core::UnorderedMap<Core::String, std::pair<Condition, Controller>> ControlFlow;
			Core::UnorderedMap<Core::String, ProcDirectiveCallback> Directives;
			Core::UnorderedMap<Core::String, Definition> Defines;
			Core::UnorderedSet<Core::String> Sets;
			std::function<size_t()> StoreCurrentLine;
			ProcIncludeCallback Include;
			ProcPragmaCallback Pragma;
			IncludeDesc FileDesc;
			Desc Features;
			bool Nested;

		public:
			Preprocessor() noexcept;
			~Preprocessor() noexcept = default;
			void SetIncludeOptions(const IncludeDesc& NewDesc);
			void SetIncludeCallback(const ProcIncludeCallback& Callback);
			void SetPragmaCallback(const ProcPragmaCallback& Callback);
			void SetDirectiveCallback(const std::string_view& Name, ProcDirectiveCallback&& Callback);
			void SetFeatures(const Desc& Value);
			void AddDefaultDefinitions();
			ExpectsPreprocessor<void> Define(const std::string_view& Expression);
			ExpectsPreprocessor<void> DefineDynamic(const std::string_view& Expression, ProcExpansionCallback&& Callback);
			void Undefine(const std::string_view& Name);
			void Clear();
			bool IsDefined(const std::string_view& Name) const;
			bool IsDefined(const std::string_view& Name, const std::string_view& Value) const;
			ExpectsPreprocessor<void> Process(const std::string_view& Path, Core::String& Buffer);
			ExpectsPreprocessor<Core::String> ResolveFile(const std::string_view& Path, const std::string_view& Include);
			const Core::String& GetCurrentFilePath() const;
			size_t GetCurrentLineNumber();

		private:
			ProcDirective FindNextToken(Core::String& Buffer, size_t& Offset);
			ProcDirective FindNextConditionalToken(Core::String& Buffer, size_t& Offset);
			size_t ReplaceToken(ProcDirective& Where, Core::String& Buffer, const std::string_view& To);
			ExpectsPreprocessor<Core::Vector<Conditional>> PrepareConditions(Core::String& Buffer, ProcDirective& Next, size_t& Offset, bool Top);
			Core::String Evaluate(Core::String& Buffer, const Core::Vector<Conditional>& Conditions);
			std::pair<Core::String, Core::String> GetExpressionParts(const std::string_view& Value);
			std::pair<Core::String, Core::String> UnpackExpression(const std::pair<Core::String, Core::String>& Expression);
			int SwitchCase(const Conditional& Value);
			size_t GetLinesCount(Core::String& Buffer, size_t End);
			ExpectsPreprocessor<void> ExpandDefinitions(Core::String& Buffer, size_t& Size);
			ExpectsPreprocessor<void> ParseArguments(const std::string_view& Value, Core::Vector<Core::String>& Tokens, bool UnpackLiterals);
			ExpectsPreprocessor<void> ConsumeTokens(const std::string_view& Path, Core::String& Buffer);
			void ApplyResult(bool WasNested);
			bool HasResult(const std::string_view& Path);
			bool SaveResult();

		public:
			static IncludeResult ResolveInclude(const IncludeDesc& Desc, bool AsGlobal);
		};

		class VI_OUT Transform final : public Core::Reference<Transform>
		{
			friend Geometric;

		public:
			struct Spacing
			{
				Matrix4x4 Offset;
				Vector3 Position;
				Vector3 Rotation;
				Vector3 Scale = 1.0f;
			};

		private:
			Core::TaskCallback OnDirty;
			Core::Vector<Transform*> Childs;
			Matrix4x4 Temporary;
			Transform* Root;
			Spacing* Local;
			Spacing Global;
			bool Scaling;
			bool Dirty;

		public:
			void* UserData;

		public:
			Transform(void* NewUserData) noexcept;
			~Transform() noexcept;
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
			void WhenDirty(Core::TaskCallback&& Callback);
			void MakeDirty();
			void SetScaling(bool Enabled);
			void SetPosition(const Vector3& Value);
			void SetRotation(const Vector3& Value);
			void SetScale(const Vector3& Value);
			void SetSpacing(Positioning Space, Spacing& Where);
			void SetPivot(Transform* Root, Spacing* Pivot);
			void SetRoot(Transform* Root);
			void GetBounds(Matrix4x4& World, Vector3& Min, Vector3& Max);
			bool HasRoot(const Transform* Target) const;
			bool HasChild(Transform* Target) const;
			bool HasScaling() const;
			bool IsDirty() const;
			const Matrix4x4& GetBias() const;
			const Matrix4x4& GetBiasUnscaled() const;
			const Vector3& GetPosition() const;
			const Vector3& GetRotation() const;
			const Vector3& GetScale() const;
			Vector3 Forward() const;
			Vector3 Right() const;
			Vector3 Up() const;
			Spacing& GetSpacing();
			Spacing& GetSpacing(Positioning Space);
			Transform* GetRoot() const;
			Transform* GetUpperRoot() const;
			Transform* GetChild(size_t Child) const;
			size_t GetChildsCount() const;
			Core::Vector<Transform*>& GetChilds();

		protected:
			bool CanRootBeApplied(Transform* Root) const;
		};

		class VI_OUT Cosmos
		{
		public:
			typedef Core::Vector<size_t> Iterator;

		public:
			struct VI_OUT Node
			{
				Bounding Bounds;
				size_t Parent = 0;
				size_t Next = 0;
				size_t Left = 0;
				size_t Right = 0;
				void* Item = nullptr;
				int Height = 0;

				bool IsLeaf() const;
			};

		private:
			Core::UnorderedMap<void*, size_t> Items;
			Core::Vector<Node> Nodes;
			size_t Root;
			size_t NodeCount;
			size_t NodeCapacity;
			size_t FreeList;

		public:
			Cosmos(size_t DefaultSize = 16) noexcept;
			void Reserve(size_t Size);
			void Clear();
			void RemoveItem(void* Item);
			void InsertItem(void* Item, const Vector3& LowerBound, const Vector3& UpperBound);
			bool UpdateItem(void* Item, const Vector3& LowerBound, const Vector3& UpperBound, bool Always = false);
			const Bounding& GetArea(void* Item);
			const Core::UnorderedMap<void*, size_t>& GetItems() const;
			const Core::Vector<Node>& GetNodes() const;
			size_t GetNodesCount() const;
			size_t GetHeight() const;
			size_t GetMaxBalance() const;
			size_t GetRoot() const;
			const Node& GetRootNode() const;
			const Node& GetNode(size_t Id) const;
			float GetVolumeRatio() const;
			bool IsNull(size_t Id) const;
			bool Empty() const;

		private:
			size_t AllocateNode();
			void FreeNode(size_t);
			void InsertLeaf(size_t);
			void RemoveLeaf(size_t);
			size_t Balance(size_t);
			size_t ComputeHeight() const;
			size_t ComputeHeight(size_t) const;

		public:
			template <typename T, typename OverlapsFunction, typename MatchFunction>
			void QueryIndex(Iterator& Context, OverlapsFunction&& Overlaps, MatchFunction&& Match)
			{
				Context.clear();
				if (!Items.empty())
					Context.push_back(Root);

				while (!Context.empty())
				{
					auto& Next = Nodes[Context.back()];
					Context.pop_back();

					if (Overlaps(Next.Bounds))
					{
						if (!Next.IsLeaf())
						{
							Context.push_back(Next.Left);
							Context.push_back(Next.Right);
						}
						else if (Next.Item != nullptr)
							Match((T*)Next.Item);
					}
				}
			}
		};

		class VI_OUT HullShape final : public Core::Reference<HullShape>
		{
		private:
			Core::Vector<Vertex> Vertices;
			Core::Vector<int> Indices;
			btCollisionShape* Shape;

		public:
			HullShape(Core::Vector<Vertex>&& NewVertices, Core::Vector<int>&& Indices) noexcept;
			HullShape(Core::Vector<Vertex>&& NewVertices) noexcept;
			HullShape(btCollisionShape* From) noexcept;
			~HullShape() noexcept;
			const Core::Vector<Vertex>& GetVertices() const;
			const Core::Vector<int>& GetIndices() const;
			btCollisionShape* GetShape() const;
		};

		class VI_OUT RigidBody final : public Core::Reference<RigidBody>
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
			RigidBody(Simulator* Refer, const Desc& I) noexcept;

		public:
			~RigidBody() noexcept;
			Core::Unique<RigidBody> Copy();
			void Push(const Vector3& Velocity);
			void Push(const Vector3& Velocity, const Vector3& Torque);
			void Push(const Vector3& Velocity, const Vector3& Torque, const Vector3& Center);
			void PushKinematic(const Vector3& Velocity);
			void PushKinematic(const Vector3& Velocity, const Vector3& Torque);
			void Synchronize(Transform* Transform, bool Kinematic);
			void SetCollisionFlags(size_t Flags);
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
			MotionState GetActivationState() const;
			Shape GetCollisionShapeType() const;
			Vector3 GetAngularFactor() const;
			Vector3 GetAnisotropicFriction() const;
			Vector3 GetGravity() const;
			Vector3 GetLinearFactor() const;
			Vector3 GetLinearVelocity() const;
			Vector3 GetAngularVelocity() const;
			Vector3 GetScale() const;
			Vector3 GetPosition() const;
			Vector3 GetRotation() const;
			btTransform* GetWorldTransform() const;
			btCollisionShape* GetCollisionShape() const;
			btRigidBody* Get() const;
			bool IsActive() const;
			bool IsStatic() const;
			bool IsGhost() const;
			bool IsColliding() const;
			float GetSpinningFriction() const;
			float GetContactStiffness() const;
			float GetContactDamping() const;
			float GetAngularDamping() const;
			float GetAngularSleepingThreshold() const;
			float GetFriction() const;
			float GetRestitution() const;
			float GetHitFraction() const;
			float GetLinearDamping() const;
			float GetLinearSleepingThreshold() const;
			float GetCcdMotionThreshold() const;
			float GetCcdSweptSphereRadius() const;
			float GetContactProcessingThreshold() const;
			float GetDeactivationTime() const;
			float GetRollingFriction() const;
			float GetMass() const;
			size_t GetCollisionFlags() const;
			Desc& GetInitialState();
			Simulator* GetSimulator() const;

		public:
			static RigidBody* Get(btRigidBody* From);
		};

		class VI_OUT SoftBody final : public Core::Reference<SoftBody>
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
			SoftBody(Simulator* Refer, const Desc& I) noexcept;

		public:
			~SoftBody() noexcept;
			Core::Unique<SoftBody> Copy();
			void Activate(bool Force);
			void Synchronize(Transform* Transform, bool Kinematic);
			void GetIndices(Core::Vector<int>* Indices) const;
			void GetVertices(Core::Vector<Vertex>* Vertices) const;
			void GetBoundingBox(Vector3* Min, Vector3* Max) const;
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
			float GetRestLengthScale() const;
			float GetVolume() const;
			int GenerateBendingConstraints(int Distance);
			void RandomizeConstraints();
			bool CutLink(int Node0, int Node1, float Position);
			bool RayTest(const Vector3& From, const Vector3& To, RayCast& Result);
			void SetWindVelocity(const Vector3& Velocity);
			Vector3 GetWindVelocity() const;
			void GetAabb(Vector3& Min, Vector3& Max) const;
			void IndicesToPointers(const int* Map = 0);
			void SetSpinningFriction(float Value);
			Vector3 GetLinearVelocity() const;
			Vector3 GetAngularVelocity() const;
			Vector3 GetCenterPosition() const;
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
			Shape GetCollisionShapeType() const;
			MotionState GetActivationState() const;
			Vector3 GetAnisotropicFriction() const;
			Vector3 GetScale() const;
			Vector3 GetPosition() const;
			Vector3 GetRotation() const;
			btTransform* GetWorldTransform() const;
			btSoftBody* Get() const;
			bool IsActive() const;
			bool IsStatic() const;
			bool IsGhost() const;
			bool IsColliding() const;
			float GetSpinningFriction() const;
			float GetContactStiffness() const;
			float GetContactDamping() const;
			float GetFriction() const;
			float GetRestitution() const;
			float GetHitFraction() const;
			float GetCcdMotionThreshold() const;
			float GetCcdSweptSphereRadius() const;
			float GetContactProcessingThreshold() const;
			float GetDeactivationTime() const;
			float GetRollingFriction() const;
			size_t GetCollisionFlags() const;
			size_t GetVerticesCount() const;
			Desc& GetInitialState();
			Simulator* GetSimulator() const;

		public:
			static SoftBody* Get(btSoftBody* From);
		};

		class VI_OUT Constraint : public Core::Reference<Constraint>
		{
		protected:
			btRigidBody* First, * Second;
			Simulator* Engine;

		public:
			void* UserPointer;

		protected:
			Constraint(Simulator* Refer) noexcept;

		public:
			virtual ~Constraint() noexcept = default;
			virtual Core::Unique<Constraint> Copy() const = 0;
			virtual btTypedConstraint* Get() const = 0;
			virtual bool HasCollisions() const = 0;
			void SetBreakingImpulseThreshold(float Value);
			void SetEnabled(bool Value);
			bool IsEnabled() const;
			bool IsActive() const;
			float GetBreakingImpulseThreshold() const;
			btRigidBody* GetFirst() const;
			btRigidBody* GetSecond() const;
			Simulator* GetSimulator() const;
		};

		class VI_OUT PConstraint : public Constraint
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
			PConstraint(Simulator* Refer, const Desc& I) noexcept;

		public:
			~PConstraint() noexcept override;
			Core::Unique<Constraint> Copy() const override;
			btTypedConstraint* Get() const override;
			bool HasCollisions() const override;
			void SetPivotA(const Vector3& Value);
			void SetPivotB(const Vector3& Value);
			Vector3 GetPivotA() const;
			Vector3 GetPivotB() const;
			Desc& GetState();
		};

		class VI_OUT HConstraint : public Constraint
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
			HConstraint(Simulator* Refer, const Desc& I) noexcept;

		public:
			~HConstraint() noexcept override;
			Core::Unique<Constraint> Copy() const override;
			btTypedConstraint* Get() const override;
			bool HasCollisions() const override;
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
			int GetSolveLimit() const;
			float GetMotorTargetVelocity() const;
			float GetMaxMotorImpulse() const;
			float GetLimitSign() const;
			float GetHingeAngle() const;
			float GetHingeAngle(const Matrix4x4& A, const Matrix4x4& B) const;
			float GetLowerLimit() const;
			float GetUpperLimit() const;
			float GetLimitSoftness() const;
			float GetLimitBiasFactor() const;
			float GetLimitRelaxationFactor() const;
			bool HasLimit() const;
			bool IsOffset() const;
			bool IsReferenceToA() const;
			bool IsAngularOnly() const;
			bool IsAngularMotorEnabled() const;
			Desc& GetState();
		};

		class VI_OUT SConstraint : public Constraint
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
			SConstraint(Simulator* Refer, const Desc& I) noexcept;

		public:
			~SConstraint() noexcept override;
			Core::Unique<Constraint> Copy() const override;
			btTypedConstraint* Get() const override;
			bool HasCollisions() const override;
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
			float GetAngularMotorVelocity() const;
			float GetLinearMotorVelocity() const;
			float GetUpperLinearLimit() const;
			float GetLowerLinearLimit() const;
			float GetAngularDampingDirection() const;
			float GetLinearDampingDirection() const;
			float GetAngularDampingLimit() const;
			float GetLinearDampingLimit() const;
			float GetAngularDampingOrtho() const;
			float GetLinearDampingOrtho() const;
			float GetUpperAngularLimit() const;
			float GetLowerAngularLimit() const;
			float GetMaxAngularMotorForce() const;
			float GetMaxLinearMotorForce() const;
			float GetAngularRestitutionDirection() const;
			float GetLinearRestitutionDirection() const;
			float GetAngularRestitutionLimit() const;
			float GetLinearRestitutionLimit() const;
			float GetAngularRestitutionOrtho() const;
			float GetLinearRestitutionOrtho() const;
			float GetAngularSoftnessDirection() const;
			float GetLinearSoftnessDirection() const;
			float GetAngularSoftnessLimit() const;
			float GetLinearSoftnessLimit() const;
			float GetAngularSoftnessOrtho() const;
			float GetLinearSoftnessOrtho() const;
			bool GetPoweredAngularMotor() const;
			bool GetPoweredLinearMotor() const;
			Desc& GetState();
		};

		class VI_OUT CTConstraint : public Constraint
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
			CTConstraint(Simulator* Refer, const Desc& I) noexcept;

		public:
			~CTConstraint() noexcept override;
			Core::Unique<Constraint> Copy() const override;
			btTypedConstraint* Get() const override;
			bool HasCollisions() const override;
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
			Vector3 GetPointForAngle(float AngleInRadians, float Length) const;
			Quaternion GetMotorTarget() const;
			int GetSolveTwistLimit() const;
			int GetSolveSwingLimit() const;
			float GetTwistLimitSign() const;
			float GetSwingSpan1() const;
			float GetSwingSpan2() const;
			float GetTwistSpan() const;
			float GetLimitSoftness() const;
			float GetBiasFactor() const;
			float GetRelaxationFactor() const;
			float GetTwistAngle() const;
			float GetLimit(int Value) const;
			float GetDamping() const;
			float GetMaxMotorImpulse() const;
			float GetFixThresh() const;
			bool IsMotorEnabled() const;
			bool IsMaxMotorImpulseNormalized() const;
			bool IsPastSwingLimit() const;
			bool IsAngularOnly() const;
			Desc& GetState();
		};

		class VI_OUT DF6Constraint : public Constraint
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
			DF6Constraint(Simulator* Refer, const Desc& I) noexcept;

		public:
			~DF6Constraint() noexcept override;
			Core::Unique<Constraint> Copy() const override;
			btTypedConstraint* Get() const override;
			bool HasCollisions() const override;
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
			Vector3 GetAngularUpperLimit() const;
			Vector3 GetAngularUpperLimitReversed() const;
			Vector3 GetAngularLowerLimit() const;
			Vector3 GetAngularLowerLimitReversed() const;
			Vector3 GetLinearUpperLimit() const;
			Vector3 GetLinearLowerLimit() const;
			Vector3 GetAxis(int Value) const;
			Rotator GetRotationOrder() const;
			float GetAngle(int Value) const;
			float GetRelativePivotPosition(int Value) const;
			bool IsLimited(int LimitIndex) const;
			Desc& GetState();
		};

		class VI_OUT Simulator final : public Core::Reference<Simulator>
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
			struct
			{
				float LastElapsedTime = 0.0f;
			} Timing;

		private:
			Core::UnorderedMap<void*, size_t> Shapes;
			btCollisionConfiguration* Collision;
			btBroadphaseInterface* Broadphase;
			btConstraintSolver* Solver;
			btDiscreteDynamicsWorld* World;
			btCollisionDispatcher* Dispatcher;
			btSoftBodySolver* SoftSolver;
			std::mutex Exclusive;

		public:
			float Speedup;
			bool Active;

		public:
			Simulator(const Desc& I) noexcept;
			~Simulator() noexcept;
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
			void SimulateStep(float ElapsedTimeSeconds);
			void FindContacts(RigidBody* Body, int(*Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&));
			bool FindRayContacts(const Vector3& Start, const Vector3& End, int(*Callback)(RayContact*, const CollisionBody&));
			Core::Unique<RigidBody> CreateRigidBody(const RigidBody::Desc& I);
			Core::Unique<RigidBody> CreateRigidBody(const RigidBody::Desc& I, Transform* Transform);
			Core::Unique<SoftBody> CreateSoftBody(const SoftBody::Desc& I);
			Core::Unique<SoftBody> CreateSoftBody(const SoftBody::Desc& I, Transform* Transform);
			Core::Unique<PConstraint> CreatePoint2PointConstraint(const PConstraint::Desc& I);
			Core::Unique<HConstraint> CreateHingeConstraint(const HConstraint::Desc& I);
			Core::Unique<SConstraint> CreateSliderConstraint(const SConstraint::Desc& I);
			Core::Unique<CTConstraint> CreateConeTwistConstraint(const CTConstraint::Desc& I);
			Core::Unique<DF6Constraint> Create6DoFConstraint(const DF6Constraint::Desc& I);
			btCollisionShape* CreateShape(Shape Type);
			btCollisionShape* CreateCube(const Vector3& Scale = Vector3(1, 1, 1));
			btCollisionShape* CreateSphere(float Radius = 1);
			btCollisionShape* CreateCapsule(float Radius = 1, float Height = 1);
			btCollisionShape* CreateCone(float Radius = 1, float Height = 1);
			btCollisionShape* CreateCylinder(const Vector3& Scale = Vector3(1, 1, 1));
			btCollisionShape* CreateConvexHull(Core::Vector<SkinVertex>& Mesh);
			btCollisionShape* CreateConvexHull(Core::Vector<Vertex>& Mesh);
			btCollisionShape* CreateConvexHull(Core::Vector<Vector2>& Mesh);
			btCollisionShape* CreateConvexHull(Core::Vector<Vector3>& Mesh);
			btCollisionShape* CreateConvexHull(Core::Vector<Vector4>& Mesh);
			btCollisionShape* CreateConvexHull(btCollisionShape* From);
			btCollisionShape* TryCloneShape(btCollisionShape* Shape);
			btCollisionShape* ReuseShape(btCollisionShape* Shape);
			void FreeShape(Core::Unique<btCollisionShape*> Value);
			Core::Vector<Vector3> GetShapeVertices(btCollisionShape* Shape) const;
			size_t GetShapeVerticesCount(btCollisionShape* Shape) const;
			float GetMaxDisplacement() const;
			float GetAirDensity() const;
			float GetWaterOffset() const;
			float GetWaterDensity() const;
			Vector3 GetWaterNormal() const;
			Vector3 GetGravity() const;
			ContactStartedCallback GetOnCollisionEnter() const;
			ContactEndedCallback GetOnCollisionExit() const;
			btCollisionConfiguration* GetCollision() const;
			btBroadphaseInterface* GetBroadphase() const;
			btConstraintSolver* GetSolver() const;
			btDiscreteDynamicsWorld* GetWorld() const;
			btCollisionDispatcher* GetDispatcher() const;
			btSoftBodySolver* GetSoftSolver() const;
			bool HasSoftBodySupport() const;
			int GetContactManifoldCount() const;

		public:
			static Simulator* Get(btDiscreteDynamicsWorld* From);
		};

		template <typename T>
		class Math
		{
		public:
			template <typename F, bool = std::is_fundamental<F>::value>
			struct TypeTraits
			{
			};

			template <typename F>
			struct TypeTraits<F, true>
			{
				typedef F type;
			};

			template <typename F>
			struct TypeTraits<F, false>
			{
				typedef const F& type;
			};

		public:
			typedef typename TypeTraits<T>::type I;

		public:
			static T Rad2Deg()
			{
				return T(57.2957795130);
			}
			static T Deg2Rad()
			{
				return T(0.01745329251);
			}
			static T Pi()
			{
				return T(3.14159265359);
			}
			static T Sqrt(I Value)
			{
				return T(std::sqrt((double)Value));
			}
			static T Abs(I Value)
			{
				return Value < 0 ? -Value : Value;
			}
			static T Atan(I Angle)
			{
				return T(std::atan((double)Angle));
			}
			static T Atan2(I Angle0, I Angle1)
			{
				return T(std::atan2((double)Angle0, (double)Angle1));
			}
			static T Acos(I Angle)
			{
				return T(std::acos((double)Angle));
			}
			static T Asin(I Angle)
			{
				return T(std::asin((double)Angle));
			}
			static T Cos(I Angle)
			{
				return T(std::cos((double)Angle));
			}
			static T Sin(I Angle)
			{
				return T(std::sin((double)Angle));
			}
			static T Tan(I Angle)
			{
				return T(std::tan((double)Angle));
			}
			static T Acotan(I Angle)
			{
				return T(std::atan(1.0 / (double)Angle));
			}
			static T Max(I Value1, I Value2)
			{
				return Value1 > Value2 ? Value1 : Value2;
			}
			static T Min(I Value1, I Value2)
			{
				return Value1 < Value2 ? Value1 : Value2;
			}
			static T Log(I Value)
			{
				return T(std::log((double)Value));
			}
			static T Log2(I Value)
			{
				return T(std::log2((double)Value));
			}
			static T Log10(I Value)
			{
				return T(std::log10((double)Value));
			}
			static T Exp(I Value)
			{
				return T(std::exp((double)Value));
			}
			static T Ceil(I Value)
			{
				return T(std::ceil((double)Value));
			}
			static T Floor(I Value)
			{
				return T(std::floor((double)Value));
			}
			static T Lerp(I A, I B, I DeltaTime)
			{
				return A + DeltaTime * (B - A);
			}
			static T StrongLerp(I A, I B, I Time)
			{
				return (T(1.0) - Time) * A + Time * B;
			}
			static T SaturateAngle(I Angle)
			{
				return T(std::atan2(std::sin((double)Angle), std::cos((double)Angle)));
			}
			static T AngluarLerp(I A, I B, I DeltaTime)
			{
				if (A == B)
					return A;

				Vector2 ACircle = Vector2(cosf((float)A), sinf((float)A));
				Vector2 BCircle = Vector2(cosf((float)B), sinf((float)B));
				Vector2 Interpolation = ACircle.Lerp(BCircle, (float)DeltaTime);

				return T(std::atan2(Interpolation.Y, Interpolation.X));
			}
			static T AngleDistance(I A, I B)
			{
				return T(Vector2(std::cos((float)A), std::sin((float)A)).Distance(Vector2(std::cos((float)B), std::sin((float)B))));
			}
			static T Saturate(I Value)
			{
				return Min(Max(Value, T(0.0)), T(1.0));
			}
			static T Random(I Number0, I Number1)
			{
				if (Number0 == Number1)
					return Number0;

				return T((double)Number0 + ((double)Number1 - (double)Number0) / (double)std::numeric_limits<uint64_t>::max() * (double)Crypto::Random());
			}
			static T Round(I Value)
			{
				return T(std::round((double)Value));
			}
			static T Random()
			{
				return T((double)Crypto::Random() / (double)std::numeric_limits<uint64_t>::max());
			}
			static T RandomMag()
			{
				return T(2.0) * Random() - T(1.0);
			}
			static T Pow(I Value0, I Value1)
			{
				return T(std::pow((double)Value0, (double)Value1));
			}
			static T Pow2(I Value0)
			{
				return Value0 * Value0;
			}
			static T Pow3(I Value0)
			{
				return Value0 * Value0 * Value0;
			}
			static T Pow4(I Value0)
			{
				T Value = Value0 * Value0;
				return Value * Value;
			}
			static T Clamp(I Value, I pMin, I pMax)
			{
				return Min(Max(Value, pMin), pMax);
			}
			static T Select(I A, I B)
			{
				return Crypto::Random() < std::numeric_limits<uint64_t>::max() / 2 ? B : A;
			}
			static T Cotan(I Value)
			{
				return T(1.0 / std::tan((double)Value));
			}
			static T Map(I Value, I FromMin, I FromMax, I ToMin, I ToMax)
			{
				return ToMin + (Value - FromMin) * (ToMax - ToMin) / (FromMax - FromMin);
			}
			static void Swap(T& Value0, T& Value1)
			{
				T Value2 = Value0;
				Value0 = Value1;
				Value1 = Value2;
			}
		};

		typedef Math<Core::Decimal> Math0;
		typedef Math<UInt128> Math128u;
		typedef Math<UInt256> Math256u;
		typedef Math<int32_t> Math32;
		typedef Math<uint32_t> Math32u;
		typedef Math<int64_t> Math64;
		typedef Math<uint64_t> Math64u;
		typedef Math<float> Mathf;
		typedef Math<double> Mathd;
	}
}

using decimal_t = Vitex::Core::Decimal;
using uint128_t = Vitex::Compute::UInt128;
using uint256_t = Vitex::Compute::UInt256;
#endif
