#ifndef THAWK_COMPUTE_H
#define THAWK_COMPUTE_H

#include "rest.h"
#include <cmath>
#include <map>

class btCollisionConfiguration;
class btBroadphaseInterface;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionDispatcher;
class btSoftBodySolver;
class btRigidBody;
class btSoftBody;
class btSliderConstraint;
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

namespace Tomahawk
{
    namespace Compute
    {
        class Simulator;

        class Transform;

        struct Matrix4x4;

        struct Vector2;

        struct Vector3;

        struct Vector4;

        typedef std::function<bool(class Preprocessor*, const struct IncludeResult& File, std::string* Out)> ProcIncludeCallback;
        typedef std::function<bool(class Preprocessor*, const std::string& Pragma)> ProcPragmaCallback;
        typedef std::function<void(const struct CollisionBody&)> CollisionCallback;

        enum Hybi10_Opcode
        {
            Hybi10_Opcode_Text,
            Hybi10_Opcode_Close,
            Hybi10_Opcode_Pong,
            Hybi10_Opcode_Ping,
            Hybi10_Opcode_Invalid
        };

        enum Shape
        {
            Shape_Box,
            Shape_Triangle,
            Shape_Tetrahedral,
            Shape_Convex_Triangle_Mesh,
            Shape_Convex_Hull,
            Shape_Convex_Point_Cloud,
            Shape_Convex_Polyhedral,
            Shape_Implicit_Convex_Start,
            Shape_Sphere,
            Shape_Multi_Sphere,
            Shape_Capsule,
            Shape_Cone,
            Shape_Convex,
            Shape_Cylinder,
            Shape_Uniform_Scaling,
            Shape_Minkowski_Sum,
            Shape_Minkowski_Difference,
            Shape_Box_2D,
            Shape_Convex_2D,
            Shape_Custom_Convex,
            Shape_Concaves_Start,
            Shape_Triangle_Mesh,
            Shape_Triangle_Mesh_Scaled,
            Shape_Fast_Concave_Mesh,
            Shape_Terrain,
            Shape_Gimpact,
            Shape_Triangle_Mesh_Multimaterial,
            Shape_Empty,
            Shape_Static_Plane,
            Shape_Custom_Concave,
            Shape_Concaves_End,
            Shape_Compound,
            Shape_Softbody,
            Shape_HF_Fluid,
            Shape_HF_Fluid_Bouyant_Convex,
            Shape_Invalid,
            Shape_Count
        };

        enum MotionState
        {
            MotionState_Active = 1,
            MotionState_Island_Sleeping = 2,
            MotionState_Deactivation_Needed = 3,
            MotionState_Disable_Deactivation = 4,
            MotionState_Disable_Simulation = 5,
        };

        enum RegexState
        {
            RegexState_Match_Found = -1,
            RegexState_No_Match = -2,
            RegexState_Unexpected_Quantifier = -3,
            RegexState_Unbalanced_Brackets = -4,
            RegexState_Internal_Error = -5,
            RegexState_Invalid_Character_Set = -6,
            RegexState_Invalid_Metacharacter = -7,
            RegexState_Sumatch_Array_Too_Small = -8,
            RegexState_Too_Many_Branches = -9,
            RegexState_Too_Many_Brackets = -10,
        };

        enum RegexFlags
        {
            RegexFlags_None = (1 << 0),
            RegexFlags_IgnoreCase = (1 << 1)
        };

        enum SoftFeature
        {
            SoftFeature_None,
            SoftFeature_Node,
            SoftFeature_Link,
            SoftFeature_Face,
            SoftFeature_Tetra
        };

        enum SoftAeroModel
        {
            SoftAeroModel_VPoint,
            SoftAeroModel_VTwoSided,
            SoftAeroModel_VTwoSidedLiftDrag,
            SoftAeroModel_VOneSided,
            SoftAeroModel_FTwoSided,
            SoftAeroModel_FTwoSidedLiftDrag,
            SoftAeroModel_FOneSided
        };

		enum SoftCollision
		{
			SoftCollision_RVS_Mask = 0x000f,
			SoftCollision_SDF_RS = 0x0001,
			SoftCollision_CL_RS = 0x0002,
			SoftCollision_SDF_RD = 0x0003,
			SoftCollision_SDF_RDF = 0x0004,
			SoftCollision_SVS_Mask = 0x00F0,
			SoftCollision_VF_SS = 0x0010,
			SoftCollision_CL_SS = 0x0020,
			SoftCollision_CL_Self = 0x0040,
			SoftCollision_VF_DD = 0x0050,
			SoftCollision_Default = SoftCollision_SDF_RS | SoftCollision_CL_SS
		};

		struct THAWK_OUT IncludeDesc
		{
			std::vector<std::string> Exts;
			std::string From;
			std::string Path;
			std::string Root;
		};

		struct THAWK_OUT IncludeResult
		{
			std::string Module;
			bool IsSystem = false;
			bool IsFile = false;
		};

        struct THAWK_OUT Vertex
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

        struct THAWK_OUT InfluenceVertex
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

        struct THAWK_OUT ShapeVertex
        {
            float PositionX;
            float PositionY;
            float PositionZ;
            float TexCoordX;
            float TexCoordY;
        };

        struct THAWK_OUT ElementVertex
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

		struct THAWK_OUT UnmanagedShape
		{
			std::vector<Vertex> Vertices;
			std::vector<int> Indices;
			btCollisionShape* Shape;
		};

        struct THAWK_OUT Vector2
        {
            float X;
            float Y;

            Vector2();
            Vector2(float x, float y);
            Vector2(float xy);
            float Hypotenuse() const;
            float DotProduct(const Vector2& B) const;
            float Distance(const Vector2& Target) const;
            float Length() const;
            float Cross(const Vector2& Vector1) const;
            float LookAt(const Vector2& At) const;
            float ModLength() const;
            Vector2 Direction(float Rotation) const;
            Vector2 Invert() const;
            Vector2 InvertX() const;
            Vector2 InvertY() const;
            Vector2 Normalize() const;
            Vector2 NormalizeSafe() const;
            Vector2 SetX(float X) const;
            Vector2 SetY(float Y) const;
            Vector2 Mul(float X, float Y) const;
            Vector2 Mul(const Vector2& Value) const;
            Vector2 Add(const Vector2& Value) const;
            Vector2 Div(const Vector2& Value) const;
            Vector2 Lerp(const Vector2& B, float DeltaTime) const;
            Vector2 SphericalLerp(const Vector2& B, float DeltaTime) const;
            Vector2 AngularLerp(const Vector2& B, float DeltaTime) const;
            Vector2 SaturateRotation() const;
            Vector2 Transform(const Matrix4x4& V) const;
            Vector2 Abs() const;
            Vector2 Radians() const;
            Vector2 Degrees() const;
            Vector3 MtVector3() const;
            Vector4 MtVector4() const;
            Vector2 operator *(const Vector2& vec) const;
            Vector2 operator *(float vec) const;
            Vector2 operator /(const Vector2& vec) const;
            Vector2 operator /(float vec) const;
            Vector2 operator +(const Vector2& right) const;
            Vector2 operator +(float right) const;
            Vector2 operator -(const Vector2& right) const;
            Vector2 operator -(float right) const;
            Vector2 operator -() const;
            void ToInt32(unsigned int& Value) const;
            void Set(const Vector2& Value);
            void Get2(float* In) const;
            void operator -=(const Vector2& right);
            void operator -=(float right);
            void operator *=(const Vector2& vec);
            void operator *=(float vec);
            void operator /=(const Vector2& vec);
            void operator /=(float vec);
            void operator +=(const Vector2& right);
            void operator +=(float right);
            Vector2& operator =(const Vector2& right);
            bool operator ==(const Vector2& right) const;
            bool operator !=(const Vector2& right) const;
            bool operator <=(const Vector2& right) const;
            bool operator >=(const Vector2& right) const;
            bool operator >(const Vector2& right) const;
            bool operator <(const Vector2& right) const;
            float& operator [](int Axis);
            float operator [](int Axis) const;

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

        struct THAWK_OUT Vector3
        {
            float X;
            float Y;
            float Z;

            Vector3();
            Vector3(float x, float y);
            Vector3(float x, float y, float z);
            Vector3(float xyz);
            float Hypotenuse() const;
            float DotProduct(const Vector3& B) const;
            float Distance(const Vector3& Target) const;
            float Length() const;
            float ModLength() const;
            float LookAtXY(const Vector3& At) const;
            float LookAtXZ(const Vector3& At) const;
            Vector3 Invert() const;
            Vector3 InvertX() const;
            Vector3 InvertY() const;
            Vector3 InvertZ() const;
            Vector3 Cross(const Vector3& Vector) const;
            Vector3 Normalize() const;
            Vector3 NormalizeSafe() const;
            Vector3 SetX(float X) const;
            Vector3 SetY(float Y) const;
            Vector3 SetZ(float Z) const;
            Vector3 Mul(const Vector2& XY, float Z) const;
            Vector3 Mul(const Vector3& Value) const;
            Vector3 Add(const Vector3& Value) const;
            Vector3 Div(const Vector3& Value) const;
            Vector3 Lerp(const Vector3& B, float DeltaTime) const;
            Vector3 SphericalLerp(const Vector3& B, float DeltaTime) const;
            Vector3 AngularLerp(const Vector3& B, float DeltaTime) const;
            Vector3 SaturateRotation() const;
            Vector3 HorizontalDirection() const;
            Vector3 DepthDirection() const;
            Vector3 Transform(const Matrix4x4& V) const;
            Vector3 Abs() const;
            Vector3 Radians() const;
            Vector3 Degrees() const;
            Vector2 MtVector2() const;
            Vector4 MtVector4() const;
            Vector3 operator *(const Vector3& vec) const;
            Vector3 operator *(float vec) const;
            Vector3 operator /(const Vector3& vec) const;
            Vector3 operator /(float vec) const;
            Vector3 operator +(const Vector3& right) const;
            Vector3 operator +(float right) const;
            Vector3 operator -(const Vector3& right) const;
            Vector3 operator -(float right) const;
            Vector3 operator -() const;
            void operator -=(const Vector3& right);
            void operator -=(float right);
            void operator *=(const Vector3& vec);
            void operator *=(float vec);
            void operator /=(const Vector3& vec);
            void operator /=(float vec);
            void operator +=(const Vector3& right);
            void operator +=(float right);
            Vector3& operator =(const Vector3& right);
            void ToInt32(unsigned int& Value) const;
            void Set(const Vector3& Value);
            void Get2(float* In) const;
            void Get3(float* In) const;
            bool operator ==(const Vector3& right) const;
            bool operator !=(const Vector3& right) const;
            bool operator <=(const Vector3& right) const;
            bool operator >=(const Vector3& right) const;
            bool operator >(const Vector3& right) const;
            bool operator <(const Vector3& right) const;
            float& operator [](int Axis);
            float operator [](int Axis) const;

            static void ToBtVector3(const Vector3& In, btVector3* Out);
            static void FromBtVector3(const btVector3& In, Vector3* Out);
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

        struct THAWK_OUT Vector4
        {
            float X;
            float Y;
            float Z;
            float W;

            Vector4();
            Vector4(float x, float y);
            Vector4(float x, float y, float z);
            Vector4(float x, float y, float z, float w);
            Vector4(float xyzw);
            float ModLength() const;
            float DotProduct(const Vector4& B) const;
            float Distance(const Vector4& Target) const;
            float Length() const;
            Vector4 Invert() const;
            Vector4 InvertX() const;
            Vector4 InvertY() const;
            Vector4 InvertZ() const;
            Vector4 InvertW() const;
            Vector4 Lerp(const Vector4& B, float DeltaTime) const;
            Vector4 SphericalLerp(const Vector4& B, float DeltaTime) const;
            Vector4 AngularLerp(const Vector4& B, float DeltaTime) const;
            Vector4 SaturateRotation() const;
            Vector4 Cross(const Vector4& Vector1) const;
            Vector4 Normalize() const;
            Vector4 NormalizeSafe() const;
            Vector4 Transform(const Matrix4x4& Matrix) const;
            Vector4 SetX(float X) const;
            Vector4 SetY(float Y) const;
            Vector4 SetZ(float Z) const;
            Vector4 SetW(float W) const;
            Vector4 Mul(const Vector2& XY, float Z, float W) const;
            Vector4 Mul(const Vector3& XYZ, float W) const;
            Vector4 Mul(const Vector4& Value) const;
            Vector4 Add(const Vector4& Value) const;
            Vector4 Div(const Vector4& Value) const;
            Vector4 Abs() const;
            Vector4 Radians() const;
            Vector4 Degrees() const;
            Vector2 MtVector2() const;
            Vector3 MtVector3() const;
			Vector4 operator *(const Matrix4x4& vec) const;
            Vector4 operator *(const Vector4& vec) const;
            Vector4 operator *(float vec) const;
            Vector4 operator /(const Vector4& vec) const;
            Vector4 operator /(float vec) const;
            Vector4 operator +(const Vector4& right) const;
            Vector4 operator +(float right) const;
            Vector4 operator -(const Vector4& right) const;
            Vector4 operator -(float right) const;
            Vector4 operator -() const;
            void ToInt32(unsigned int& Value) const;
            void Set(const Vector4& Value);
            void Get2(float* In) const;
            void Get3(float* In) const;
            void Get4(float* In) const;
            void operator -=(const Vector4& right);
            void operator -=(float right);
			void operator *=(const Matrix4x4& vec);
            void operator *=(const Vector4& vec);
            void operator *=(float vec);
            void operator /=(const Vector4& vec);
            void operator /=(float vec);
            void operator +=(const Vector4& right);
            void operator +=(float right);
            Vector4& operator =(const Vector4& right);
            bool operator ==(const Vector4& right) const;
            bool operator !=(const Vector4& right) const;
            bool operator <=(const Vector4& right) const;
            bool operator >=(const Vector4& right) const;
            bool operator >(const Vector4& right) const;
            bool operator <(const Vector4& right) const;
            float& operator [](int Axis);
            float operator [](int Axis) const;

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
            static Vector4 UnitW()
            {
                return Vector4(0, 0, 0, 1);
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
        };

		struct THAWK_OUT Ray
		{
			Vector3 Origin;
			Vector3 Direction;

		public:
			Ray();
			Ray(const Vector3& _Origin, const Vector3& _Direction);
			Vector3 GetPoint(float T) const;
			Vector3 operator *(float T) const;
			bool IntersectsPlane(const Vector3& Normal, float Diameter) const;
			bool IntersectsSphere(const Vector3& Position, float Radius, bool DiscardInside = true) const;
			bool IntersectsAABBAt(const Vector3& Min, const Vector3& Max) const;
			bool IntersectsAABB(const Vector3& Position, const Vector3& Scale) const;
		};

        struct THAWK_OUT Matrix4x4
        {
            float Row[16];

            Matrix4x4();
            Matrix4x4(btTransform* In);
            Matrix4x4(float Array[16]);
            Matrix4x4(const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3);
            Matrix4x4(float Row00, float Row01, float Row02, float Row03, float Row10, float Row11, float Row12, float Row13, float Row20, float Row21, float Row22, float Row23, float Row30, float Row31, float Row32, float Row33);
            float& operator [](int Index);
            float operator [](int Index) const;
            bool operator ==(const Matrix4x4& Index) const;
            bool operator !=(const Matrix4x4& Index) const;
            Matrix4x4 operator *(const Matrix4x4& right) const;
            Matrix4x4 operator *(const Vector4& right) const;
            Matrix4x4& operator =(const Matrix4x4& right);
            Matrix4x4 Mul(const Matrix4x4& Right) const;
            Matrix4x4 Mul(const Vector4& Right) const;
            Matrix4x4 Invert() const;
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
            Vector3 MtVector3() const;
            Vector4 MtVector4() const;
            void Identify();
            void Set(const Matrix4x4& Value);

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
            static Matrix4x4 Create(const Vector3& Position, const Vector3& Scale, const Vector3& Rotation);
            static Matrix4x4 Create(const Vector3& Position, const Vector3& Rotation);
            static Matrix4x4 CreateRotation(const Vector3& Rotation);
            static Matrix4x4 CreateCameraRotation(const Vector3& Rotation);
            static Matrix4x4 CreateOrthographic(float Left, float Right, float Bottom, float Top, float NearClip, float FarClip);
            static Matrix4x4 CreateCamera(const Vector3& Position, const Vector3& Rotation);
            static Matrix4x4 CreateOrigin(const Vector3& Position, const Vector3& Rotation);
            static Matrix4x4 CreateCubeMapLookAt(int Face, const Vector3& Position);
            static Matrix4x4 CreateLineLightLookAt(const Vector3& Position, const Vector3& Camera);
            static Matrix4x4 CreateRotation(const Vector3& Forward, const Vector3& Up, const Vector3& Right);
            static Matrix4x4 Identity()
            {
                return Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
            };
        };

        struct THAWK_OUT Quaternion
        {
            float X, Y, Z, W;

            Quaternion();
            Quaternion(float x, float y, float z, float w);
            Quaternion(const Quaternion& In);
            Quaternion(const Vector3& Axis, float Angle);
            Quaternion(const Vector3& Euler);
            Quaternion(const Matrix4x4& Value);
            void Set(const Vector3& Axis, float Angle);
            void Set(const Vector3& Euler);
            void Set(const Matrix4x4& Value);
            void Set(const Quaternion& Value);
            Quaternion operator *(float r) const;
            Quaternion operator *(const Vector3& r) const;
            Quaternion operator *(const Quaternion& r) const;
            Quaternion operator -(const Quaternion& r) const;
            Quaternion operator +(const Quaternion& r) const;
            Quaternion& operator =(const Quaternion& r);
            Quaternion Normalize() const;
            Quaternion Conjugate() const;
            Quaternion Mul(float r) const;
            Quaternion Mul(const Quaternion& r) const;
            Quaternion Mul(const Vector3& r) const;
            Quaternion Sub(const Quaternion& r) const;
            Quaternion Add(const Quaternion& r) const;
            Quaternion Lerp(const Quaternion& B, float DeltaTime) const;
            Quaternion SphericalLerp(const Quaternion& B, float DeltaTime) const;
            Matrix4x4 Rotation() const;
            Vector3 Euler() const;
            float DotProduct(const Quaternion& r) const;
            float Length() const;

            static Quaternion CreateEulerRotation(const Vector3& Euler);
            static Quaternion CreateRotation(const Matrix4x4& Transform);
        };

        struct THAWK_OUT AnimatorKey
        {
            Compute::Vector3 Position = 0.0f;
            Compute::Vector3 Rotation = 0.0f;
            Compute::Vector3 Scale = 1.0f;
            float PlayingSpeed = 1.0f;
        };

        struct THAWK_OUT SkinAnimatorClip
        {
            std::vector<std::vector<AnimatorKey>> Keys;
            std::string Name;
            float PlayingSpeed = 1.0f;
        };

        struct THAWK_OUT KeyAnimatorClip
        {
            std::vector<AnimatorKey> Keys;
            std::string Name;
            float PlayingSpeed = 1.0f;
        };

        struct THAWK_OUT Joint
        {
            std::vector<Joint> Childs;
            std::string Name;
            Matrix4x4 Transform;
            Matrix4x4 BindShape;
            Int64 Index = -1;
        };

        struct THAWK_OUT RandomVector2
        {
            Vector2 Min, Max;
            bool Intensity;
            float Accuracy;

            RandomVector2();
            RandomVector2(Vector2 MinV, Vector2 MaxV, bool IntensityV, float AccuracyV);
            Vector2 Generate();
        };

        struct THAWK_OUT RandomVector3
        {
            Vector3 Min, Max;
            bool Intensity;
            float Accuracy;

            RandomVector3();
            RandomVector3(Vector3 MinV, Vector3 MaxV, bool IntensityV, float AccuracyV);
            Vector3 Generate();
        };

        struct THAWK_OUT RandomVector4
        {
            Vector4 Min, Max;
            bool Intensity;
            float Accuracy;

            RandomVector4();
            RandomVector4(Vector4 MinV, Vector4 MaxV, bool IntensityV, float AccuracyV);
            Vector4 Generate();
        };

        struct THAWK_OUT RandomFloat
        {
            float Min, Max;
            bool Intensity;
            float Accuracy;

            RandomFloat();
            RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV);
            float Generate();
        };

        struct THAWK_OUT Hybi10PayloadHeader
        {
            unsigned short Opcode : 4;
            unsigned short Rsv1 : 1;
            unsigned short Rsv2 : 1;
            unsigned short Rsv3 : 1;
            unsigned short Fin : 1;
            unsigned short PayloadLength : 7;
            unsigned short Mask : 1;
        };

        struct THAWK_OUT Hybi10Request
        {
            std::string Payload;
            int ExitCode, Type;

            Hybi10Request();
            std::string GetTextType();
            Hybi10_Opcode GetEnumType();
        };

        struct THAWK_OUT ShapeContact
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

        struct THAWK_OUT RayContact
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

        struct THAWK_OUT RegexBracket
        {
            const char* Pointer;
            Int64 Length;
            Int64 Branches;
            Int64 BranchesCount;
        };

        struct THAWK_OUT RegexBranch
        {
            Int64 BracketIndex;
            const char* Pointer;
        };

        struct THAWK_OUT RegexMatch
        {
            const char* Pointer;
            Int64 Start;
            Int64 End;
            Int64 Length;
            Int64 Steps;
        };

        struct THAWK_OUT RegExp
        {
            std::string Regex;
            Int64 MaxBranches = 128;
            Int64 MaxBrackets = 128;
            Int64 MaxMatches = 128;
            RegexFlags Flags = RegexFlags_None;
        };

        struct THAWK_OUT RegexResult
        {
            friend class Regex;

        private:
            std::vector<RegexMatch> Matches;
            std::vector<RegexBracket> Brackets;
            std::vector<RegexBranch> Branches;
            Int64 NextMatch = 0, Steps = 0;
            RegexState State = RegexState_No_Match;

        public:
            RegExp* Expression = nullptr;

        public:
            void ResetNextMatch();
            bool HasMatch();
            bool IsMatch(std::vector<RegexMatch>::iterator It);
            Int64 Start();
            Int64 End();
            Int64 Length();
            Int64 GetSteps();
            Int64 GetMatchesCount();
            RegexState GetState();
            std::vector<RegexMatch>::iterator GetMatch(Int64 Id);
            std::vector<RegexMatch>::iterator GetNextMatch();
            const char* Pointer();
        };

        struct THAWK_OUT CollisionBody
        {
            class RigidBody* Rigid = nullptr;

            class SoftBody* Soft = nullptr;

            CollisionBody(btCollisionObject* Object);
        };

        class THAWK_OUT MD5Hasher
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

        template <typename Precision>
        class THAWK_OUT Math
        {
        public:
            static Precision Rad2Deg()
            {
                return (Precision)57.2957795130f;
            }
            static Precision Deg2Rad()
            {
                return (Precision)0.01745329251f;
            }
            static Precision Pi()
            {
                return (Precision)3.14159265359f;
            }
            static Precision Sqrt(Precision Value)
            {
                return (Precision)std::sqrt((Float64)Value);
            }
            static Precision Abs(Precision Value)
            {
                return Value < 0 ? -Value : Value;
            }
            static Precision ATan(Precision Angle)
            {
                return (Precision)std::atan((Float64)Angle);
            }
            static Precision ATan2(Precision Angle0, Precision Angle1)
            {
                return (Precision)std::atan2((Float64)Angle0, (Float64)Angle1);
            }
            static Precision ACos(Precision Angle)
            {
                return (Precision)std::acos((Float64)Angle);
            }
            static Precision ASin(Precision Angle)
            {
                return (Precision)std::asin((Float64)Angle);
            }
            static Precision Cos(Precision Angle)
            {
                return (Precision)std::cos((Float64)Angle);
            }
            static Precision Sin(Precision Angle)
            {
                return (Precision)std::sin((Float64)Angle);
            }
            static Precision Tan(Precision Angle)
            {
                return (Precision)std::tan((Float64)Angle);
            }
            static Precision ACotan(Precision Angle)
            {
                return (Precision)std::atan(1.0 / (Float64)Angle);
            }
            static Precision Max(Precision Value1, Precision Value2)
            {
                return Value1 > Value2 ? Value1 : Value2;
            }
            static Precision Min(Precision Value1, Precision Value2)
            {
                return Value1 < Value2 ? Value1 : Value2;
            }
            static Precision Floor(Precision Value)
            {
                return (Precision)std::floor((Float64)Value);
            }
            static Precision Lerp(Precision A, Precision B, Precision DeltaTime)
            {
                return A + DeltaTime * (B - A);
            }
            static Precision StrongLerp(Precision A, Precision B, Precision Time)
            {
                return ((Precision)1.0 - Time) * A + Time * B;
            }
            static Precision SaturateAngle(Precision Angle)
            {
                return (Precision)std::atan2(std::sin((Float64)Angle), std::cos((Float64)Angle));
            }
            static Precision AngluarLerp(Precision A, Precision B, Precision DeltaTime)
            {
                if (A == B)
                    return A;

                Vector2 ACircle = Vector2(cosf((float)A), sinf((float)A));
                Vector2 BCircle = Vector2(cosf((float)B), sinf((float)B));
                Vector2 Interpolation = ACircle.Lerp(BCircle, DeltaTime);

                return (Precision)std::atan2(Interpolation.Y, Interpolation.X);
            }
            static Precision AngleDistance(Precision A, Precision B)
            {
                return (Precision)Vector2(std::cos((float)A), std::sin((float)A)).Distance(Vector2(std::cos((float)B), std::sin((float)B)));
            }
            static Precision Saturate(Precision Value)
            {
                return Min(Max(Value, 0.0), 1.0);
            }
            static Precision Random(Precision Number0, Precision Number1)
            {
                if (Number0 == Number1)
                    return Number0;

                return (Precision)((Float64)Number0 + ((Float64)Number1 - (Float64)Number0) / RAND_MAX * rand());
            }
            static Precision Round(Precision Value)
            {
                return (Precision)std::round((Float64)Value);
            }
            static Precision Random()
            {
                return (Precision)rand() / ((Precision)RAND_MAX + 1.0f);
            }
            static Precision RandomMag()
            {
                return (Precision)2.0 / RAND_MAX * rand() - (Precision)1.0;
            }
            static Precision Pow(Precision Value0, Precision Value1)
            {
                return (Precision)std::pow((Float64)Value0, (Float64)Value1);
            }
            static Precision Pow2(Precision Value0)
            {
                return Value0 * Value0;
            }
            static Precision Pow3(Precision Value0)
            {
                return Value0 * Value0 * Value0;
            }
            static Precision Pow4(Precision Value0)
            {
                Precision Value = Value0 * Value0;
                return Value * Value;
            }
            static Precision Clamp(Precision Value, Precision pMin, Precision pMax)
            {
                return Min(Max(Value, pMin), pMax);
            }
            static Precision Select(Precision A, Precision B)
            {
                if (rand() < RAND_MAX / 2)
                    return B;

                return A;
            }
            static Precision Cotan(Precision Value)
            {
                return (Precision)(1.0 / std::tan((Float64)Value));
            }
            static void Swap(Precision& Value0, Precision& Value1)
            {
                Precision Value2 = Value0;
                Value0 = Value1;
                Value1 = Value2;
            }
        };

        class THAWK_OUT MathCommon
        {
        public:
            static std::string Base10ToBaseN(UInt64 Value, unsigned int BaseLessThan65);
            static std::string URIEncode(const std::string& Text);
            static std::string URIEncode(const char* Text, UInt64 Length);
            static std::string URIDecode(const std::string& Text);
            static std::string URIDecode(const char* Text, UInt64 Length);
            static std::string Encrypt(const std::string& Text, int Offset);
            static std::string Decrypt(const std::string& Text, int Offset);
            static float IsClipping(Matrix4x4 ViewProjection, Matrix4x4 World, float Radius);
            static bool HasSphereIntersected(Vector3 PositionR0, float RadiusR0, Vector3 PositionR1, float RadiusR1);
            static bool HasLineIntersected(float DistanceF, float DistanceD, Vector3 Start, Vector3 End, Vector3& Hit);
            static bool HasLineIntersectedCube(Vector3 Min, Vector3 Max, Vector3 Start, Vector3 End);
            static bool HasPointIntersectedCube(Vector3 Hit, Vector3 Position, Vector3 Scale, int Axis);
            static bool HasPointIntersectedRectangle(Vector3 Position, Vector3 Scale, Vector3 P0);
            static bool HasPointIntersectedCube(Vector3 Position, Vector3 Scale, Vector3 P0);
            static bool HasSBIntersected(Transform* BoxR0, Transform* BoxR1);
            static bool HasOBBIntersected(Transform* BoxR0, Transform* BoxR1);
            static bool HasAABBIntersected(Transform* BoxR0, Transform* BoxR1);
            static bool IsBase64(unsigned char Value);
            static bool HexToString(void* Data, UInt64 Length, char* Buffer, UInt64 BufferLength);
            static bool Hex(char c, int& v);
            static bool HexToDecimal(const std::string& s, UInt64 i, UInt64 cnt, int& Value);
            static void ComputeJointOrientation(Compute::Joint* Matrix, bool LeftHanded);
            static void ComputeMatrixOrientation(Compute::Matrix4x4* Matrix, bool LeftHanded);
            static void ComputePositionOrientation(Compute::Vector3* Position, bool LeftHanded);
            static void ComputeIndexWindingOrderFlip(std::vector<int>& Indices);
            static void ComputeVertexOrientation(std::vector<Vertex>& Vertices, bool LeftHanded);
            static void ComputeInfluenceOrientation(std::vector<InfluenceVertex>& Vertices, bool LeftHanded);
            static void ComputeInfluenceNormals(std::vector<InfluenceVertex>& Vertices);
            static void ComputeInfluenceNormalsArray(InfluenceVertex* Vertices, UInt64 Count);
            static void ComputeInfluenceTangentBitangent(InfluenceVertex V1, InfluenceVertex V2, InfluenceVertex V3, Vector3& Tangent, Vector3& Bitangent, Vector3& Normal);
            static void ComputeInfluenceTangentBitangent(InfluenceVertex V1, InfluenceVertex V2, InfluenceVertex V3, Vector3& Tangent, Vector3& Bitangent);
            static void ConfigurateUnsafe(Transform* In, Matrix4x4* LocalTransform, Vector3* LocalPosition, Vector3* LocalRotation, Vector3* LocalScale);
            static void SetRootUnsafe(Transform* In, Transform* Root);
            static void Randomize();
            static void Sha1CollapseBufferBlock(unsigned int* Buffer);
            static void Sha1ComputeHashBlock(unsigned int* Result, unsigned int* W);
            static void Sha1Compute(const void* Value, int Length, unsigned char* Hash20);
            static void Sha1Hash20ToHex(const unsigned char* Hash20, char* HexString);
            static std::string BinToHex(const char* Value);
            static std::string RandomBytes(UInt64 Length);
            static std::string MD5Hash(const std::string& Value);
            static std::string Sha256Encode(const char* Value, const char* Key, const char* IV);
            static std::string Sha256Decode(const char* Value, const char* Key, const char* IV);
            static std::string Aes256Encode(const std::string& Value, const char* Key, const char* IV);
            static std::string Aes256Decode(const std::string& Value, const char* Key, const char* IV);
            static std::string Base64Encode(const unsigned char* Value, UInt64 Length);
            static std::string Base64Encode(const std::string& Value);
            static std::string Base64Decode(const std::string& Value);
            static std::string Hybi10Encode(Hybi10Request hRequest, bool Masked);
            static std::string DecimalToHex(UInt64 V);
            static Hybi10Request Hybi10Decode(const std::string& Data);
            static unsigned char RandomUC();
            static Int64 RandomNumber(Int64 Begin, Int64 End);
            static UInt64 Utf8(int code, char* Buffer);
			static Ray CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView);
			static bool CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale);
		};

        class THAWK_OUT Regex
        {
        private:
            static void Setup(RegexResult* Info);
            static Int64 Meta(const unsigned char* Buffer);
            static Int64 OpLength(const char* Value);
            static Int64 SetLength(const char* Value, Int64 ValueLength);
            static Int64 GetOpLength(const char* Value, Int64 ValueLength);
            static Int64 Quantifier(const char* Value);
            static Int64 ToInt(Int64 x);
            static Int64 HexToInt(const unsigned char* Buffer);
            static Int64 MatchOp(const unsigned char* Value, const unsigned char* Buffer, RegexResult* Info);
            static Int64 MatchSet(const char* Value, Int64 ValueLength, const char* Buffer, RegexResult* Info);
            static Int64 ParseDOH(const char* Buffer, Int64 BufferLength, RegexResult* Info, Int64 Case);
            static Int64 ParseInner(const char* Value, Int64 ValueLength, const char* Buffer, Int64 BufferLength, RegexResult* Info, Int64 Case);
            static Int64 ParseOuter(const char* Buffer, Int64 BufferLength, RegexResult* Info);
            static Int64 Parse(const char* Value, Int64 ValueLength, const char* Buffer, Int64 BufferLength, RegexResult* Info);

        public:
            static bool Match(RegExp* Value, RegexResult* Result, const std::string& Buffer);
            static bool Match(RegExp* Value, RegexResult* Result, const char* Buffer, Int64 Length);
            static bool MatchAll(RegExp* Value, RegexResult* Result, const std::string& Buffer);
            static bool MatchAll(RegExp* Value, RegexResult* Result, const char* Buffer, Int64 Length);
            static RegExp Create(const std::string& Regexp, RegexFlags Flags = RegexFlags_None, Int64 MaxMatches = -1, Int64 MaxBranches = -1, Int64 MaxBrackets = -1);
            static const char* Syntax();
        };

        class THAWK_OUT Preprocessor : public Rest::Object
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
            ProcIncludeCallback Include;
            ProcPragmaCallback Pragma;
			IncludeDesc FileDesc;
			Desc Features;
            int Resolve;

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

        private:
            void PushSet(const std::string& Path);
            void PopSet();
            bool ProcessIncludeDirective(const std::string& Path, Rest::Stroke& Buffer);
            bool ProcessPragmaDirective(Rest::Stroke& Buffer);
			bool ProcessBlockDirective(Rest::Stroke& Buffer);
			bool ProcessDefineDirective(Rest::Stroke& Buffer, UInt64 Base, UInt64& Offset, bool Endless);
			int FindDefineDirective(Rest::Stroke& Buffer, UInt64& Offset, UInt64* Size);
			int FindBlockDirective(Rest::Stroke& Buffer, UInt64& Offset, bool Nested);
			int FindBlockNesting(Rest::Stroke& Buffer, Rest::Stroke::Settle& Hash, UInt64& Offset, bool Resolved);
            int FindDirective(Rest::Stroke& Buffer, const char* V, UInt64* Offset, UInt64* Base, UInt64* Start, UInt64* End);
            bool HasSet(const std::string& Path);

		public:
			static IncludeResult ResolveInclude(const IncludeDesc& Desc);
        };

        class THAWK_OUT Transform : public Rest::Object
        {
            friend MathCommon;

        private:
            std::vector<Transform*>* Childs;
            Matrix4x4* LocalTransform;
            Vector3* LocalPosition;
            Vector3* LocalRotation;
            Vector3* LocalScale;
            Transform* Root;

        public:
            Vector3 Position;
            Vector3 Rotation;
            Vector3 Scale;
            bool ConstantScale;
            void* UserPointer;

        public:
            Transform();
            virtual ~Transform() override;
            void GetRootBasis(Vector3& Position, Vector3& Scale, Vector3& Rotation);
            void SetRoot(Transform* Root);
            void SetMatrix(Matrix4x4 Matrix);
            void SetLocals(Transform* Target);
            void Copy(Transform* Target);
            void AddChild(Transform* Child);
            void RemoveChild(Transform* Child);
            void RemoveChilds();
            void Synchronize();
            void GetWorld(btTransform* Out);
            void GetLocal(btTransform* Out);
			std::vector<Transform*>* GetChilds();
			Transform* GetChild(UInt64 Child);
			Vector3* GetLocalPosition();
			Vector3* GetLocalRotation();
			Vector3* GetLocalScale();
			Vector3 Up();
			Vector3 Right();
			Vector3 Forward();
			Vector3 Direction(const Vector3& Direction);
			Matrix4x4 GetWorld();
			Matrix4x4 GetWorld(const Vector3& Rescale);
			Matrix4x4 GetWorldUnscaled();
			Matrix4x4 GetWorldUnscaled(const Vector3& Rescale);
			Matrix4x4 GetLocal();
			Matrix4x4 GetLocal(const Vector3& Rescale);
			Matrix4x4 GetLocalUnscaled();
			Matrix4x4 GetLocalUnscaled(const Vector3& Rescale);
			Matrix4x4 Localize(const Matrix4x4& In);
			Matrix4x4 Globalize(const Matrix4x4& In);
			Transform* GetUpperRoot();
			Transform* GetRoot();
			bool HasRoot(Transform* Target);
			bool HasChild(Transform* Target);
			UInt64 GetChildCount();

        protected:
            bool CanRootBeApplied(Transform* Root);

        public:
            template <typename In>
            In* Ptr()
            {
                return (In*)UserPointer;
            }
        };

        class THAWK_OUT RigidBody : public Rest::Object
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
            void SetCollisionFlags(UInt64 Flags);
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
            btRigidBody* Bullet();
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
            UInt64 GetCollisionFlags();
            Desc& GetInitialState();
            Simulator* GetSimulator();

        public:
            static RigidBody* Get(btRigidBody* From);
        };

        class THAWK_OUT SoftBody : public Rest::Object
        {
            friend Simulator;

        public:
            struct Desc
            {
                struct CV
                {
                    struct SConvex
                    {
						Compute::UnmanagedShape* Hull;
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
                    SoftAeroModel AeroModel = SoftAeroModel_VPoint;
                    float VCF = 1;
                    float DP = 0;
                    float DG = 0;
                    float LF = 0;
                    float PR = 0;
                    float VC = 0;
					float DF = 0.2f;
                    float MT = 0;
                    float CHR = 1;
                    float KHR = 0.1f;
                    float SHR = 1;
                    float AHR = 0.7f;
                    float SRHR_CL = 0.1;
                    float SKHR_CL = 1;
                    float SSHR_CL = 0.5f;
                    float SR_SPLT_CL = 0.5f;
                    float SK_SPLT_CL = 0.5f;
                    float SS_SPLT_CL = 0.5f;
                    float MaxVolume = 1;
                    float TimeScale = 1;
					float Drag = 0;
					float MaxStress = 0;
					int Clusters = 16;
					int Constraints = 8;
                    int VIterations = 10;
                    int PIterations = 10;
                    int DIterations = 0;
                    int CIterations = 4;
                    int Collisions = SoftCollision_Default;
                } Config;

                float Anticipation = 0;
                Vector3 Position;
                Vector3 Rotation;
                Vector3 Scale;
            };

            struct RayCast
            {
                SoftBody* Body = nullptr;
                SoftFeature Feature = SoftFeature_None;
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
			void Reindex(std::vector<int>* Indices);
			void Retrieve(std::vector<Vertex>* Vertices);
            void Update(std::vector<Vertex>* Vertices);
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
            btSoftBody* Bullet();
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
            UInt64 GetCollisionFlags();
            UInt64 GetVerticesCount();
            Desc& GetInitialState();
            Simulator* GetSimulator();

        public:
            static SoftBody* Get(btSoftBody* From);
        };

        class THAWK_OUT SliderConstraint : public Rest::Object
        {
            friend Simulator;

        public:
            struct Desc
            {
                RigidBody* Target1 = nullptr;
                RigidBody* Target2 = nullptr;
                bool UseCollisions = true;
                bool UseLinearPower = true;
            };

        private:
            btRigidBody* First, *Second;
            btSliderConstraint* Instance;
            Simulator* Engine;
            Desc Initial;

        public:
            void* UserPointer;

        private:
            SliderConstraint(Simulator* Refer, const Desc& I);

        public:
            virtual ~SliderConstraint() override;
            SliderConstraint* Copy();
            void SetAngularMotorVelocity(float Value);
            void SetLinearMotorVelocity(float Value);
            void SetUpperLinearLimit(float Value);
            void SetLowerLinearLimit(float Value);
            void SetBreakingImpulseThreshold(float Value);
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
            void SetEnabled(bool Value);
            btSliderConstraint* Bullet();
            btRigidBody* GetFirst();
            btRigidBody* GetSecond();
            float GetAngularMotorVelocity();
            float GetLinearMotorVelocity();
            float GetUpperLinearLimit();
            float GetLowerLinearLimit();
            float GetBreakingImpulseThreshold();
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
            bool IsConnected();
            bool IsEnabled();
            Desc& GetInitialState();
            Simulator* GetSimulator();
        };

        class THAWK_OUT Simulator : public Rest::Object
        {
        public:
            struct Desc
            {
                bool EnableSoftBody = false;
                float AirDensity = 1.2f;
                float WaterDensity = 0;
                float WaterOffset = 0;
                float MaxDisplacement = 1000;
                Vector3 WaterNormal;
                Vector3 Gravity = Vector3(0, -10, 0);
            };

        private:
            std::unordered_map<void*, UInt64> Shapes;
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
            void AddSliderConstraint(SliderConstraint* Constraint);
            void RemoveSliderConstraint(SliderConstraint* Constraint);
            void RemoveAll();
            void Simulate(float TimeStep);
            void FindContacts(RigidBody* Body, int(* Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&));
            bool FindRayContacts(const Vector3& Start, const Vector3& End, int(* Callback)(RayContact*, const CollisionBody&));
            RigidBody* CreateRigidBody(const RigidBody::Desc& I);
            RigidBody* CreateRigidBody(const RigidBody::Desc& I, Transform* Transform);
            SoftBody* CreateSoftBody(const SoftBody::Desc& I);
            SoftBody* CreateSoftBody(const SoftBody::Desc& I, Transform* Transform);
            SliderConstraint* CreateSliderConstraint(const SliderConstraint::Desc& I);
            btCollisionShape* CreateShape(Shape Type);
            btCollisionShape* CreateCube(const Vector3& Scale = Vector3(1, 1, 1));
            btCollisionShape* CreateSphere(float Radius = 1);
            btCollisionShape* CreateCapsule(float Radius = 1, float Height = 1);
            btCollisionShape* CreateCone(float Radius = 1, float Height = 1);
            btCollisionShape* CreateCylinder(const Vector3& Scale = Vector3(1, 1, 1));
            btCollisionShape* CreateConvexHull(std::vector<InfluenceVertex>& Mesh);
            btCollisionShape* CreateConvexHull(std::vector<Vertex>& Mesh);
            btCollisionShape* CreateConvexHull(std::vector<Vector2>& Mesh);
            btCollisionShape* CreateConvexHull(std::vector<Vector3>& Mesh);
            btCollisionShape* CreateConvexHull(std::vector<Vector4>& Mesh);
            btCollisionShape* CreateConvexHull(btCollisionShape* From);
			btCollisionShape* TryCloneShape(btCollisionShape* Shape);
			btCollisionShape* ReuseShape(btCollisionShape* Shape);
			void FreeShape(btCollisionShape** Value);
            std::vector<Vector3> GetShapeVertices(btCollisionShape* Shape);
            UInt64 GetShapeVerticesCount(btCollisionShape* Shape);
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
			static void FreeUnmanagedShape(btCollisionShape* Shape);
            static Simulator* Get(btDiscreteDynamicsWorld* From);
			static btCollisionShape* CreateUnmanagedShape(std::vector<Vertex>& Mesh);
			static btCollisionShape* CreateUnmanagedShape(btCollisionShape* From);
        };
    }
}
#endif