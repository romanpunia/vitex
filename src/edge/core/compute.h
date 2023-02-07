#ifndef ED_COMPUTE_H
#define ED_COMPUTE_H
#include "core.h"
#include <cmath>
#include <map>
#include <stack>
#include <limits>
#define ED_LEFT_HANDED (Edge::Compute::Geometric::IsLeftHanded())

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

namespace Edge
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

		inline SoftCollision operator |(SoftCollision A, SoftCollision B)
		{
			return static_cast<SoftCollision>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		struct ED_OUT IncludeDesc
		{
			std::vector<std::string> Exts;
			std::string From;
			std::string Path;
			std::string Root;
		};

		struct ED_OUT IncludeResult
		{
			std::string Module;
			bool IsSystem = false;
			bool IsFile = false;
		};

		struct ED_OUT Vertex
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

		struct ED_OUT SkinVertex
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

		struct ED_OUT ShapeVertex
		{
			float PositionX;
			float PositionY;
			float PositionZ;
			float TexCoordX;
			float TexCoordY;
		};

		struct ED_OUT ElementVertex
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

		struct ED_OUT Vector2
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

		struct ED_OUT Vector3
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

		struct ED_OUT Vector4
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

		struct ED_OUT Matrix4x4
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
			Vector3 Up(bool ViewSpace) const;
			Vector3 Right(bool ViewSpace) const;
			Vector3 Forward(bool ViewSpace) const;
			Vector3 Rotation() const;
			Vector3 Position() const;
			Vector3 Scale() const;
			Vector2 XY() const;
			Vector3 XYZ() const;
			Vector4 XYZW() const;
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
			static Matrix4x4 CreateOrigin(const Vector3& Position, const Vector3& Rotation);
			static Matrix4x4 CreateLookAt(CubeFace Face, const Vector3& Position);
			static Matrix4x4 CreateRotation(const Vector3& Forward, const Vector3& Up, const Vector3& Right);
			static Matrix4x4 Identity()
			{
				return Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
			};
		};

		struct ED_OUT Quaternion
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

			static Quaternion CreateEulerRotation(const Vector3& Euler);
			static Quaternion CreateRotation(const Matrix4x4& Transform);
		};

		struct ED_OUT Rectangle
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

		struct ED_OUT Bounding
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

		struct ED_OUT Ray
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

		struct ED_OUT Frustum8C
		{
			Vector4 Corners[8];

			Frustum8C() noexcept;
			Frustum8C(float FieldOfView, float Aspect, float NearZ, float FarZ) noexcept;
			void Transform(const Matrix4x4& Value);
			void GetBoundingBox(Vector2* X, Vector2* Y, Vector2* Z);
		};

		struct ED_OUT Frustum6P
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

		struct ED_OUT Joint
		{
			std::vector<Joint> Childs;
			std::string Name;
			Matrix4x4 Transform;
			Matrix4x4 BindShape;
			int64_t Index = -1;
		};

		struct ED_OUT AnimatorKey
		{
			Compute::Vector3 Position = 0.0f;
			Compute::Vector3 Rotation = 0.0f;
			Compute::Vector3 Scale = 1.0f;
			float Time = 1.0f;
		};

		struct ED_OUT SkinAnimatorKey
		{
			std::vector<AnimatorKey> Pose;
			float Time = 1.0f;
		};

		struct ED_OUT SkinAnimatorClip
		{
			std::vector<SkinAnimatorKey> Keys;
			std::string Name;
			float Duration = 1.0f;
			float Rate = 1.0f;
		};

		struct ED_OUT KeyAnimatorClip
		{
			std::vector<AnimatorKey> Keys;
			std::string Name;
			float Duration = 1.0f;
			float Rate = 1.0f;
		};

		struct ED_OUT RandomVector2
		{
			Vector2 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector2() noexcept;
			RandomVector2(const Vector2& MinV, const Vector2& MaxV, bool IntensityV, float AccuracyV) noexcept;
			Vector2 Generate();
		};

		struct ED_OUT RandomVector3
		{
			Vector3 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector3() noexcept;
			RandomVector3(const Vector3& MinV, const Vector3& MaxV, bool IntensityV, float AccuracyV) noexcept;
			Vector3 Generate();
		};

		struct ED_OUT RandomVector4
		{
			Vector4 Min, Max;
			bool Intensity;
			float Accuracy;

			RandomVector4() noexcept;
			RandomVector4(const Vector4& MinV, const Vector4& MaxV, bool IntensityV, float AccuracyV) noexcept;
			Vector4 Generate();
		};

		struct ED_OUT RandomFloat
		{
			float Min, Max;
			bool Intensity;
			float Accuracy;

			RandomFloat() noexcept;
			RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV) noexcept;
			float Generate();
		};

		struct ED_OUT RegexBracket
		{
			const char* Pointer = nullptr;
			int64_t Length = 0;
			int64_t Branches = 0;
			int64_t BranchesCount = 0;
		};

		struct ED_OUT RegexBranch
		{
			int64_t BracketIndex;
			const char* Pointer;
		};

		struct ED_OUT RegexMatch
		{
			const char* Pointer;
			int64_t Start;
			int64_t End;
			int64_t Length;
			int64_t Steps;
		};

		struct ED_OUT RegexSource
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
			RegexSource() noexcept;
			RegexSource(const std::string& Regexp, bool fIgnoreCase = false, int64_t fMaxMatches = -1, int64_t fMaxBranches = -1, int64_t fMaxBrackets = -1) noexcept;
			RegexSource(const RegexSource& Other) noexcept;
			RegexSource(RegexSource&& Other) noexcept;
			RegexSource& operator =(const RegexSource& V) noexcept;
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

		struct ED_OUT RegexResult
		{
			friend class Regex;

		private:
			std::vector<RegexMatch> Matches;
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
			const std::vector<RegexMatch>& Get() const;
			std::vector<std::string> ToArray() const;
		};

		struct ED_OUT PrivateKey
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
			std::vector<void*> Blocks;
			std::string Plain;

		private:
			PrivateKey(const std::string& Text, bool) noexcept;
			PrivateKey(std::string&& Text, bool) noexcept;

		public:
			PrivateKey() noexcept;
			PrivateKey(const PrivateKey& Other) noexcept;
			PrivateKey(PrivateKey&& Other) noexcept;
			explicit PrivateKey(const std::string& Key) noexcept;
			explicit PrivateKey(const char* Buffer) noexcept;
			explicit PrivateKey(const char* Buffer, size_t Size) noexcept;
			~PrivateKey() noexcept;
			PrivateKey& operator =(const PrivateKey& V) noexcept;
			PrivateKey& operator =(PrivateKey&& V) noexcept;
			void Clear();
			void Secure(const std::string& Key);
			void Secure(const char* Buffer, size_t Size);
			void ExposeToStack(char* Buffer, size_t MaxSize, size_t* OutSize = nullptr) const;
			std::string ExposeToHeap() const;
			size_t GetSize() const;

		public:
			template <size_t MaxSize>
			Exposable<MaxSize> Expose() const
			{
				Exposable<MaxSize> Result;
				ExposeToStack(Result.Key, MaxSize, &Result.Size);
				return Result;
			}

		public:
			static PrivateKey GetPlain(std::string&& Value);
			static PrivateKey GetPlain(const std::string& Value);
			static void RandomizeBuffer(char* Data, size_t Size);

		private:
			char LoadPartition(size_t* Dest, size_t Size, size_t Index) const;
			void RollPartition(size_t* Dest, size_t Size, size_t Index) const;
			void FillPartition(size_t* Dest, size_t Size, size_t Index, char Source) const;
			void CopyDistribution(const PrivateKey& Other);
		};

		struct ED_OUT ShapeContact
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

		struct ED_OUT RayContact
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

		struct ED_OUT CollisionBody
		{
			RigidBody* Rigid = nullptr;
			SoftBody* Soft = nullptr;

			CollisionBody(btCollisionObject* Object) noexcept;
		};

		struct ED_OUT AdjTriangle
		{
			unsigned int VRef[3];
			unsigned int ATri[3];

			unsigned char FindEdge(unsigned int VRef0, unsigned int VRef1);
			unsigned int OppositeVertex(unsigned int VRef0, unsigned int VRef1);
		};

		struct ED_OUT AdjEdge
		{
			unsigned int Ref0;
			unsigned int Ref1;
			unsigned int FaceNb;
		};

		class ED_OUT Adjacencies
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
			Adjacencies() noexcept;
			~Adjacencies() noexcept;
			bool Fill(Adjacencies::Desc& I);
			bool Resolve();

		private:
			bool AddTriangle(unsigned int Ref0, unsigned int Ref1, unsigned int Ref2);
			bool AddEdge(unsigned int Ref0, unsigned int Ref1, unsigned int Face);
			bool UpdateLink(unsigned int FirstTri, unsigned int SecondTri, unsigned int Ref0, unsigned int Ref1);
		};

		class ED_OUT TriangleStrip
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
			TriangleStrip() noexcept;
			~TriangleStrip() noexcept;
			bool Fill(const TriangleStrip::Desc& I);
			bool Resolve(TriangleStrip::Result& Result);

		private:
			TriangleStrip& FreeBuffers();
			unsigned int ComputeStrip(unsigned int Face);
			unsigned int TrackStrip(unsigned int Face, unsigned int Oldest, unsigned int Middle, unsigned int* Strip, unsigned int* Faces, bool* Tags);
			bool ConnectStrips(TriangleStrip::Result& Result);
		};

		class ED_OUT RadixSorter
		{
		private:
			unsigned int* Histogram;
			unsigned int* Offset;
			unsigned int CurrentSize;
			unsigned int* Indices;
			unsigned int* Indices2;

		public:
			RadixSorter() noexcept;
			RadixSorter(const RadixSorter& Other) noexcept;
			RadixSorter(RadixSorter&& Other) noexcept;
			~RadixSorter() noexcept;
			RadixSorter& operator =(const RadixSorter& V);
			RadixSorter& operator =(RadixSorter&& V) noexcept;
			RadixSorter& Sort(unsigned int* Input, unsigned int Nb, bool SignedValues = true);
			RadixSorter& Sort(float* Input, unsigned int Nb);
			RadixSorter& ResetIndices();
			unsigned int* GetIndices();
		};

		class ED_OUT MD5Hasher
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
			MD5Hasher() noexcept;
			void Transform(const UInt1* Buffer, unsigned int Length = 64);
			void Update(const std::string& Buffer, unsigned int BlockSize = 64);
			void Update(const unsigned char* Buffer, unsigned int Length, unsigned int BlockSize = 64);
			void Update(const char* Buffer, unsigned int Length, unsigned int BlockSize = 64);
			void Finalize();
			Core::Unique<char> HexDigest() const;
			Core::Unique<unsigned char> RawDigest() const;
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

		class ED_OUT S8Hasher
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

		class ED_OUT_TS Ciphers
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

		class ED_OUT_TS Digests
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

		class ED_OUT_TS Crypto
		{
		public:
			static std::string RandomBytes(size_t Length);
			static std::string Hash(Digest Type, const std::string& Value);
			static std::string HashBinary(Digest Type, const std::string& Value);
			static std::string Sign(Digest Type, const char* Value, size_t Length, const PrivateKey& Key);
			static std::string Sign(Digest Type, const std::string& Value, const PrivateKey& Key);
			static std::string HMAC(Digest Type, const char* Value, size_t Length, const PrivateKey& Key);
			static std::string HMAC(Digest Type, const std::string& Value, const PrivateKey& Key);
			static std::string Encrypt(Cipher Type, const char* Value, size_t Length, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static std::string Encrypt(Cipher Type, const std::string& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static std::string Decrypt(Cipher Type, const char* Value, size_t Length, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static std::string Decrypt(Cipher Type, const std::string& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static std::string JWTSign(const std::string& Algo, const std::string& Payload, const PrivateKey& Key);
			static std::string JWTEncode(WebToken* Src, const PrivateKey& Key);
			static Core::Unique<WebToken> JWTDecode(const std::string& Value, const PrivateKey& Key);
			static std::string DocEncrypt(Core::Schema* Src, const PrivateKey& Key, const PrivateKey& Salt);
			static Core::Unique<Core::Schema> DocDecrypt(const std::string& Value, const PrivateKey& Key, const PrivateKey& Salt);
			static unsigned char RandomUC();
			static uint64_t CRC32(const std::string& Data);
			static uint64_t Random(uint64_t Min, uint64_t Max);
			static uint64_t Random();
			static void Sha1CollapseBufferBlock(unsigned int* Buffer);
			static void Sha1ComputeHashBlock(unsigned int* Result, unsigned int* W);
			static void Sha1Compute(const void* Value, int Length, char* Hash20);
			static void Sha1Hash20ToHex(const char* Hash20, char* HexString);
			static void DisplayCryptoLog();
		};

		class ED_OUT_TS Codec
		{
		public:
			static std::string Move(const std::string& Text, int Offset);
			static std::string Encode64(const char Alphabet[65], const unsigned char* Value, size_t Length, bool Padding);
			static std::string Decode64(const char Alphabet[65], const unsigned char* Value, size_t Length, bool(*IsAlphabetic)(unsigned char));
			static std::string Bep45Encode(const std::string& Value);
			static std::string Bep45Decode(const std::string& Value);
			static std::string Base45Encode(const std::string& Value);
			static std::string Base45Decode(const std::string& Value);
			static std::string Base64Encode(const unsigned char* Value, size_t Length);
			static std::string Base64Encode(const std::string& Value);
			static std::string Base64Decode(const unsigned char* Value, size_t Length);
			static std::string Base64Decode(const std::string& Value);
			static std::string Base64URLEncode(const unsigned char* Value, size_t Length);
			static std::string Base64URLEncode(const std::string& Value);
			static std::string Base64URLDecode(const unsigned char* Value, size_t Length);
			static std::string Base64URLDecode(const std::string& Value);
			static std::string Shuffle(const char* Value, size_t Size, uint64_t Mask);
			static std::string HexEncode(const char* Value, size_t Size);
			static std::string HexEncode(const std::string& Value);
			static std::string HexDecode(const char* Value, size_t Size);
			static std::string HexDecode(const std::string& Value);
			static std::string URIEncode(const std::string& Text);
			static std::string URIEncode(const char* Text, size_t Length);
			static std::string URIDecode(const std::string& Text);
			static std::string URIDecode(const char* Text, size_t Length);
			static std::string DecimalToHex(uint64_t V);
			static std::string Base10ToBaseN(uint64_t Value, unsigned int BaseLessThan65);
			static size_t Utf8(int Code, char* Buffer);
			static bool Hex(char Code, int& Value);
			static bool HexToString(void* Data, size_t Length, char* Buffer, size_t BufferLength);
			static bool HexToDecimal(const std::string& Text, size_t Index, size_t Size, int& Value);
			static bool IsBase64URL(unsigned char Value);
			static bool IsBase64(unsigned char Value);
		};

		class ED_OUT_TS Geometric
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
			static void FlipIndexWindingOrder(std::vector<int>& Indices);
			static void ComputeInfluenceNormals(std::vector<SkinVertex>& Vertices);
			static void ComputeInfluenceNormalsArray(SkinVertex* Vertices, size_t Count);
			static void ComputeInfluenceTangentBitangent(const SkinVertex& V1, const SkinVertex& V2, const SkinVertex& V3, Vector3& Tangent, Vector3& Bitangent, Vector3& Normal);
			static void ComputeInfluenceTangentBitangent(const SkinVertex& V1, const SkinVertex& V2, const SkinVertex& V3, Vector3& Tangent, Vector3& Bitangent);
			static void MatrixRhToLh(Compute::Matrix4x4* Matrix);
			static void VertexRhToLh(std::vector<Vertex>& Vertices, std::vector<int>& Indices);
			static void VertexRhToLh(std::vector<SkinVertex>& Vertices, std::vector<int>& Indices);
			static void TexCoordRhToLh(std::vector<Vertex>& Vertices);
			static void TexCoordRhToLh(std::vector<SkinVertex>& Vertices);
			static void TexCoordRhToLh(std::vector<ShapeVertex>& Vertices);
			static void SetLeftHanded(bool IsLeftHanded);
			static std::vector<int> CreateTriangleStrip(TriangleStrip::Desc& Desc, const std::vector<int>& Indices);
			static std::vector<int> CreateTriangleList(const std::vector<int>& Indices);
			static void CreateFrustum8CRad(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ);
			static void CreateFrustum8C(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ);
			static Ray CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView);
			static bool CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale, Vector3* Hit = nullptr);
			static bool CursorRayTest(const Ray& Cursor, const Matrix4x4& World, Vector3* Hit = nullptr);
			static float FastInvSqrt(float Value);
			static float FastSqrt(float Value);
			static float AabbVolume(const Vector3& Min, const Vector3& Max);
		};

		class ED_OUT_TS Regex
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
			static bool Replace(RegexSource* Value, const std::string& ToExpression, std::string& Buffer);
			static const char* Syntax();
		};

		class ED_OUT WebToken : public Core::Object
		{
		public:
			Core::Schema* Header;
			Core::Schema* Payload;
			Core::Schema* Token;
			std::string Refresher;
			std::string Signature;
			std::string Data;

		public:
			WebToken() noexcept;
			WebToken(const std::string& Issuer, const std::string& Subject, int64_t Expiration) noexcept;
			virtual ~WebToken() noexcept override;
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
			void SetRefreshToken(const std::string& Value, const PrivateKey& Key, const PrivateKey& Salt);
			bool Sign(const PrivateKey& Key);
			std::string GetRefreshToken(const PrivateKey& Key, const PrivateKey& Salt);
			bool IsValid() const;
		};

		class ED_OUT Preprocessor : public Core::Object
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
			Preprocessor() noexcept;
			virtual ~Preprocessor() noexcept = default;
			void SetIncludeOptions(const IncludeDesc& NewDesc);
			void SetIncludeCallback(const ProcIncludeCallback& Callback);
			void SetPragmaCallback(const ProcPragmaCallback& Callback);
			void SetFeatures(const Desc& Value);
			void Define(const std::string& Name);
			void Undefine(const std::string& Name);
			void Clear();
			bool IsDefined(const char* Name) const;
			bool Process(const std::string& Path, std::string& Buffer);
			const std::string& GetCurrentFilePath() const;

		private:
			bool SaveResult();
			bool ReturnResult(bool Result, bool WasNested);
			bool ProcessIncludeDirective(const std::string& Path, Core::Parser& Buffer);
			bool ProcessPragmaDirective(const std::string& Path, Core::Parser& Buffer);
			bool ProcessBlockDirective(Core::Parser& Buffer);
			bool ProcessDefineDirective(Core::Parser& Buffer, size_t Base, size_t& Offset, bool Endless);
			int FindDefineDirective(Core::Parser& Buffer, size_t& Offset, size_t* Size);
			int FindBlockDirective(Core::Parser& Buffer, size_t& Offset, bool Nested);
			int FindBlockNesting(Core::Parser& Buffer, const Core::Parser::Settle& Hash, size_t& Offset, bool Resolved);
			int FindDirective(Core::Parser& Buffer, const char* V, size_t* Offset, size_t* Base, size_t* Start, size_t* End);
			bool HasSet(const std::string& Path);

		public:
			static IncludeResult ResolveInclude(const IncludeDesc& Desc);
		};

		class ED_OUT FiniteState : public Core::Object
		{
		private:
			std::unordered_map<std::string, ActionCallback*> Actions;
			std::stack<ActionCallback*> State;
			std::mutex Mutex;

		public:
			FiniteState() noexcept;
			virtual ~FiniteState() noexcept override;
			FiniteState* Bind(const std::string& Name, const ActionCallback& Callback);
			FiniteState* Unbind(const std::string& Name);
			FiniteState* Push(const std::string& Name);
			FiniteState* Replace(const std::string& Name);
			FiniteState* Pop();
			void Update();

		private:
			ActionCallback* Find(const std::string& Name);
		};

		class ED_OUT Transform : public Core::Object
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
			std::vector<Transform*> Childs;
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
			virtual ~Transform() noexcept override;
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
			Vector3 Forward(bool ViewSpace = false) const;
			Vector3 Right(bool ViewSpace = false) const;
			Vector3 Up(bool ViewSpace = false) const;
			Spacing& GetSpacing();
			Spacing& GetSpacing(Positioning Space);
			Transform* GetRoot() const;
			Transform* GetUpperRoot() const;
			Transform* GetChild(size_t Child) const;
			size_t GetChildsCount() const;
			std::vector<Transform*>& GetChilds();

		protected:
			bool CanRootBeApplied(Transform* Root) const;
		};

		class ED_OUT Cosmos
		{
		public:
			typedef std::vector<size_t> Iterator;

		public:
			struct ED_OUT Node
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
			std::unordered_map<void*, size_t> Items;
			std::vector<Node> Nodes;
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
			const std::unordered_map<void*, size_t>& GetItems() const;
			const std::vector<Node>& GetNodes() const;
			size_t GetHeight() const;
			size_t GetMaxBalance() const;
			size_t GetRoot() const;
			const Node& GetRootNode() const;
			const Node& GetNode(size_t Id) const;
			float GetVolumeRatio() const;
			bool IsNull(size_t Id) const;
			bool IsEmpty() const;

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

		class ED_OUT HullShape : public Core::Object
		{
		public:
			std::vector<Vertex> Vertices;
			std::vector<int> Indices;
			btCollisionShape* Shape;

		public:
			HullShape() noexcept;
			virtual ~HullShape() noexcept = default;
		};

		class ED_OUT RigidBody : public Core::Object
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
			virtual ~RigidBody() noexcept override;
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

		class ED_OUT SoftBody : public Core::Object
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
			virtual ~SoftBody() noexcept override;
			Core::Unique<SoftBody> Copy();
			void Activate(bool Force);
			void Synchronize(Transform* Transform, bool Kinematic);
			void GetIndices(std::vector<int>* Indices) const;
			void GetVertices(std::vector<Vertex>* Vertices) const;
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

		class ED_OUT Constraint : public Core::Object
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

		class ED_OUT PConstraint : public Constraint
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
			virtual ~PConstraint() noexcept override;
			virtual Core::Unique<Constraint> Copy() const override;
			virtual btTypedConstraint* Get() const override;
			virtual bool HasCollisions() const override;
			void SetPivotA(const Vector3& Value);
			void SetPivotB(const Vector3& Value);
			Vector3 GetPivotA() const;
			Vector3 GetPivotB() const;
			Desc& GetState();
		};

		class ED_OUT HConstraint : public Constraint
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
			virtual ~HConstraint() noexcept override;
			virtual Core::Unique<Constraint> Copy() const override;
			virtual btTypedConstraint* Get() const override;
			virtual bool HasCollisions() const override;
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

		class ED_OUT SConstraint : public Constraint
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
			virtual ~SConstraint() noexcept override;
			virtual Core::Unique<Constraint> Copy() const override;
			virtual btTypedConstraint* Get() const override;
			virtual bool HasCollisions() const override;
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

		class ED_OUT CTConstraint : public Constraint
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
			virtual ~CTConstraint() noexcept override;
			virtual Core::Unique<Constraint> Copy() const override;
			virtual btTypedConstraint* Get() const override;
			virtual bool HasCollisions() const override;
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

		class ED_OUT DF6Constraint : public Constraint
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
			virtual ~DF6Constraint() noexcept override;
			virtual Core::Unique<Constraint> Copy() const override;
			virtual btTypedConstraint* Get() const override;
			virtual bool HasCollisions() const override;
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

		class ED_OUT Simulator : public Core::Object
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
			std::unordered_map<void*, size_t> Shapes;
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
			Simulator(const Desc& I) noexcept;
			virtual ~Simulator() noexcept override;
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
			void Simulate(int Interpolation, float TimeStep, float FixedTimeStep);
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
			btCollisionShape* CreateConvexHull(std::vector<SkinVertex>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vertex>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vector2>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vector3>& Mesh);
			btCollisionShape* CreateConvexHull(std::vector<Vector4>& Mesh);
			btCollisionShape* CreateConvexHull(btCollisionShape* From);
			btCollisionShape* TryCloneShape(btCollisionShape* Shape);
			btCollisionShape* ReuseShape(btCollisionShape* Shape);
			void FreeShape(Core::Unique<btCollisionShape*> Value);
			std::vector<Vector3> GetShapeVertices(btCollisionShape* Shape) const;
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
			static void FreeHullShape(btCollisionShape* Shape);
			static Simulator* Get(btDiscreteDynamicsWorld* From);
			static Core::Unique<btCollisionShape> CreateHullShape(std::vector<Vertex>& Mesh);
			static Core::Unique<btCollisionShape> CreateHullShape(btCollisionShape* From);
		};

		template <typename T>
		class ED_OUT_TS Math
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
				Vector2 Interpolation = ACircle.Lerp(BCircle, DeltaTime);

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

				return T((double)Number0 + ((double)Number1 - (double)Number0) / std::numeric_limits<uint64_t>::max() * Crypto::Random());
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
			static void Swap(T& Value0, T& Value1)
			{
				T Value2 = Value0;
				Value0 = Value1;
				Value1 = Value2;
			}
		};

		typedef Math<Core::Decimal> Mathb;
		typedef Math<float> Mathf;
		typedef Math<double> Mathd;
		typedef Math<int> Mathi;
	}
}
#endif
