#include "compute.h"
#include "../mavi.h"
#include <cctype>
#include <random>
#ifdef VI_BULLET3
#pragma warning(disable: 4244)
#pragma warning(disable: 4305)
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#endif
#ifdef VI_SIMD
#include <vc_simd.h>
#endif
#ifdef VI_ZLIB
extern "C"
{
#include <zlib.h>
}
#endif
#ifdef VI_MICROSOFT
#include <Windows.h>
#else
#include <time.h>
#endif
#ifdef VI_OPENSSL
extern "C"
{
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/err.h>
}
#endif
#define V3_TO_BT(V) btVector3(V.X, V.Y, V.Z)
#define BT_TO_V3(V) Vector3(V.getX(), V.getY(), V.getZ())
#define Q4_TO_BT(V) btQuaternion(V.X, V.Y, V.Z, V.W)
#define BT_TO_Q4(V) Quaternion(V.getX(), V.getY(), V.getZ(), V.getW())
#define REGEX_FAIL(A, B) if (A) return (B)
#define REGEX_FAIL_IN(A, B) if (A) { State = B; return; }
#define MAKE_ADJ_TRI(x) ((x) & 0x3fffffff)
#define IS_BOUNDARY(x) ((x) == 0xff)
#define RH_TO_LH (Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1))
#define NULL_NODE ((size_t)-1)
#define PRIVATE_KEY_SIZE (sizeof(size_t) * 8)

namespace
{
#ifdef VI_BULLET3
	class FindContactsHandler : public btCollisionWorld::ContactResultCallback
	{
	public:
		int(*Callback)(Mavi::Compute::ShapeContact*, const Mavi::Compute::CollisionBody&, const Mavi::Compute::CollisionBody&) = nullptr;

	public:
		btScalar addSingleResult(btManifoldPoint& Point, const btCollisionObjectWrapper* Object1, int PartId0, int Index0, const btCollisionObjectWrapper* Object2, int PartId1, int Index1) override
		{
			using namespace Mavi::Compute;
			VI_ASSERT(Callback && Object1 && Object1->getCollisionObject() && Object2 && Object2->getCollisionObject(), "collision objects are not in available condition");

			auto& PWOA = Point.getPositionWorldOnA();
			auto& PWOB = Point.getPositionWorldOnB();
			ShapeContact Contact;
			Contact.LocalPoint1 = Vector3(Point.m_localPointA.getX(), Point.m_localPointA.getY(), Point.m_localPointA.getZ());
			Contact.LocalPoint2 = Vector3(Point.m_localPointB.getX(), Point.m_localPointB.getY(), Point.m_localPointB.getZ());
			Contact.PositionWorld1 = Vector3(PWOA.getX(), PWOA.getY(), PWOA.getZ());
			Contact.PositionWorld2 = Vector3(PWOB.getX(), PWOB.getY(), PWOB.getZ());
			Contact.NormalWorld = Vector3(Point.m_normalWorldOnB.getX(), Point.m_normalWorldOnB.getY(), Point.m_normalWorldOnB.getZ());
			Contact.LateralFrictionDirection1 = Vector3(Point.m_lateralFrictionDir1.getX(), Point.m_lateralFrictionDir1.getY(), Point.m_lateralFrictionDir1.getZ());
			Contact.LateralFrictionDirection2 = Vector3(Point.m_lateralFrictionDir2.getX(), Point.m_lateralFrictionDir2.getY(), Point.m_lateralFrictionDir2.getZ());
			Contact.Distance = Point.m_distance1;
			Contact.CombinedFriction = Point.m_combinedFriction;
			Contact.CombinedRollingFriction = Point.m_combinedRollingFriction;
			Contact.CombinedSpinningFriction = Point.m_combinedSpinningFriction;
			Contact.CombinedRestitution = Point.m_combinedRestitution;
			Contact.AppliedImpulse = Point.m_appliedImpulse;
			Contact.AppliedImpulseLateral1 = Point.m_appliedImpulseLateral1;
			Contact.AppliedImpulseLateral2 = Point.m_appliedImpulseLateral2;
			Contact.ContactMotion1 = Point.m_contactMotion1;
			Contact.ContactMotion2 = Point.m_contactMotion2;
			Contact.ContactCFM = Point.m_contactCFM;
			Contact.CombinedContactStiffness = Point.m_combinedContactStiffness1;
			Contact.ContactERP = Point.m_contactERP;
			Contact.CombinedContactDamping = Point.m_combinedContactDamping1;
			Contact.FrictionCFM = Point.m_frictionCFM;
			Contact.PartId1 = Point.m_partId0;
			Contact.PartId2 = Point.m_partId1;
			Contact.Index1 = Point.m_index0;
			Contact.Index2 = Point.m_index1;
			Contact.ContactPointFlags = Point.m_contactPointFlags;
			Contact.LifeTime = Point.m_lifeTime;

			btCollisionObject* Body1 = (btCollisionObject*)Object1->getCollisionObject();
			btCollisionObject* Body2 = (btCollisionObject*)Object2->getCollisionObject();
			return (btScalar)Callback(&Contact, CollisionBody(Body1), CollisionBody(Body2));
		}
	};

	class FindRayContactsHandler : public btCollisionWorld::RayResultCallback
	{
	public:
		int(*Callback)(Mavi::Compute::RayContact*, const Mavi::Compute::CollisionBody&) = nullptr;

	public:
		btScalar addSingleResult(btCollisionWorld::LocalRayResult& RayResult, bool NormalInWorldSpace) override
		{
			using namespace Mavi::Compute;
			VI_ASSERT(Callback && RayResult.m_collisionObject, "collision objects are not in available condition");

			RayContact Contact;
			Contact.HitNormalLocal = BT_TO_V3(RayResult.m_hitNormalLocal);
			Contact.NormalInWorldSpace = NormalInWorldSpace;
			Contact.HitFraction = RayResult.m_hitFraction;
			Contact.ClosestHitFraction = m_closestHitFraction;

			btCollisionObject* Body1 = (btCollisionObject*)RayResult.m_collisionObject;
			return (btScalar)Callback(&Contact, CollisionBody(Body1));
		}
	};

	btTransform M16_TO_BT(const Mavi::Compute::Matrix4x4& In)
	{
		btMatrix3x3 Offset;
		Offset[0][0] = In.Row[0];
		Offset[1][0] = In.Row[1];
		Offset[2][0] = In.Row[2];
		Offset[0][1] = In.Row[4];
		Offset[1][1] = In.Row[5];
		Offset[2][1] = In.Row[6];
		Offset[0][2] = In.Row[8];
		Offset[1][2] = In.Row[9];
		Offset[2][2] = In.Row[10];

		btTransform Result;
		Result.setBasis(Offset);

		Mavi::Compute::Vector3 Position = In.Position();
		Result.setOrigin(V3_TO_BT(Position));

		return Result;
	}
#endif
	size_t OffsetOf64(const char* Source, char Dest)
	{
		VI_ASSERT(Source != nullptr, "source should be set");
		for (size_t i = 0; i < 64; i++)
		{
			if (Source[i] == Dest)
				return i;
		}

		return 63;
	}
	Mavi::Core::String EscapeText(const Mavi::Core::String& Data)
	{
		Mavi::Core::String Result = "\"";
		Result.append(Data).append("\"");
		return Result;
	}
}

namespace Mavi
{
	namespace Compute
	{
		int64_t Rectangle::GetX() const
		{
			return Left;
		}
		int64_t Rectangle::GetY() const
		{
			return Top;
		}
		int64_t Rectangle::GetWidth() const
		{
			return Right - Left;
		}
		int64_t Rectangle::GetHeight() const
		{
			return Bottom - Top;
		}

		Vector2::Vector2() noexcept : X(0.0f), Y(0.0f)
		{
		}
		Vector2::Vector2(float x, float y) noexcept : X(x), Y(y)
		{
		}
		Vector2::Vector2(float xy) noexcept : X(xy), Y(xy)
		{
		}
		Vector2::Vector2(const Vector2& Value) noexcept : X(Value.X), Y(Value.Y)
		{
		}
		Vector2::Vector2(const Vector3& Value) noexcept : X(Value.X), Y(Value.Y)
		{
		}
		Vector2::Vector2(const Vector4& Value) noexcept : X(Value.X), Y(Value.Y)
		{
		}
		bool Vector2::IsEquals(const Vector2& Other, float MaxDisplacement) const
		{
			return abs(X - Other.X) <= MaxDisplacement && abs(Y - Other.Y) <= MaxDisplacement;
		}
		float Vector2::Length() const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			return std::sqrt(horizontal_add(square(_r1)));
#else
			return std::sqrt(X * X + Y * Y);
#endif
		}
		float Vector2::Sum() const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			return horizontal_add(_r1);
#else
			return X + Y;
#endif
		}
		float Vector2::Dot(const Vector2& B) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, B);
			return horizontal_add(_r1 * _r2);
#else
			return X * B.X + Y * B.Y;
#endif
		}
		float Vector2::Distance(const Vector2& Point) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, Point);
			return Geometric::FastSqrt(horizontal_add(square(_r1 - _r2)));
#else
			float X1 = X - Point.X, Y1 = Y - Point.Y;
			return Geometric::FastSqrt(X1 * X1 + Y1 * Y1);
#endif
		}
		float Vector2::Hypotenuse() const
		{
			return Length();
		}
		float Vector2::LookAt(const Vector2& At) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, At); _r1 = _r2 - _r1;
			return atan2f(_r1.extract(0), _r1.extract(1));
#else
			return atan2f(At.X - X, At.Y - Y);
#endif
		}
		float Vector2::Cross(const Vector2& B) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, B); _r1 = _r1 * _r2;
			return _r1.extract(0) - _r1.extract(1);
#else
			return X * B.Y - Y * B.X;
#endif
		}
		Vector2 Vector2::Transform(const Matrix4x4& Matrix) const
		{
#ifdef VI_SIMD
			LOD_VAL(_r1, X);
			LOD_VAL(_r2, Y);
			LOD_VAR(_r3, Matrix.Row);
			LOD_VAR(_r4, Matrix.Row + 4);

			_r1 = _r1 * _r3 + _r2 * _r4;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(
				X * Matrix.Row[0] + Y * Matrix.Row[4],
				X * Matrix.Row[1] + Y * Matrix.Row[5]);
#endif
		}
		Vector2 Vector2::Direction(float Rotation) const
		{
			return Vector2(cos(-Rotation), sin(-Rotation));
		}
		Vector2 Vector2::Inv() const
		{
			return Vector2(-X, -Y);
		}
		Vector2 Vector2::InvX() const
		{
			return Vector2(-X, Y);
		}
		Vector2 Vector2::InvY() const
		{
			return Vector2(X, -Y);
		}
		Vector2 Vector2::Normalize() const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			_r1 = _r1 * Geometric::FastInvSqrt(horizontal_add(square(_r1)));
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			float F = Length();
			return Vector2(X / F, Y / F);
#endif
		}
		Vector2 Vector2::sNormalize() const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			float F = Geometric::FastSqrt(horizontal_add(square(_r1)));
			if (F == 0.0f)
				return Vector2();

			_r1 = _r1 / F;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			float F = Length();
			if (F == 0.0f)
				return Vector2();

			return Vector2(X / F, Y / F);
#endif
		}
		Vector2 Vector2::Lerp(const Vector2& B, float DeltaTime) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, B);
			_r1 = _r1 + (_r2 - _r1) * DeltaTime;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return *this + (B - *this) * DeltaTime;
#endif
		}
		Vector2 Vector2::sLerp(const Vector2& B, float DeltaTime) const
		{
			return Quaternion(Vector3()).sLerp(B.XYZ(), DeltaTime).GetEuler().XY();
		}
		Vector2 Vector2::aLerp(const Vector2& B, float DeltaTime) const
		{
			float Ax = Mathf::AngluarLerp(X, B.X, DeltaTime);
			float Ay = Mathf::AngluarLerp(Y, B.Y, DeltaTime);

			return Vector2(Ax, Ay);
		}
		Vector2 Vector2::rLerp() const
		{
			float Ax = Mathf::SaturateAngle(X);
			float Ay = Mathf::SaturateAngle(Y);

			return Vector2(Ax, Ay);
		}
		Vector2 Vector2::Abs() const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); _r1 = abs(_r1);
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X < 0 ? -X : X, Y < 0 ? -Y : Y);
#endif
		}
		Vector2 Vector2::Radians() const
		{
			return (*this) * Mathf::Deg2Rad();
		}
		Vector2 Vector2::Degrees() const
		{
			return (*this) * Mathf::Rad2Deg();
		}
		Vector2 Vector2::XY() const
		{
			return *this;
		}
		Vector3 Vector2::XYZ() const
		{
			return Vector3(X, Y, 0);
		}
		Vector4 Vector2::XYZW() const
		{
			return Vector4(X, Y, 0, 0);
		}
		Vector2 Vector2::Mul(float xy) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); _r1 = _r1 * xy;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X * xy, Y * xy);
#endif
		}
		Vector2 Vector2::Mul(float x, float y) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_AV2(_r2, x, y); _r1 = _r1 * _r2;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X * x, Y * y);
#endif
		}
		Vector2 Vector2::Mul(const Vector2& Value) const
		{
			return Mul(Value.X, Value.Y);
		}
		Vector2 Vector2::Div(const Vector2& Value) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, Value); _r1 = _r1 / _r2;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X / Value.X, Y / Value.Y);
#endif
		}
		Vector2 Vector2::Add(const Vector2& Value) const
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, Value); _r1 = _r1 + _r2;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X + Value.X, Y + Value.Y);
#endif
		}
		Vector2 Vector2::SetX(float Xf) const
		{
			return Vector2(Xf, Y);
		}
		Vector2 Vector2::SetY(float Yf) const
		{
			return Vector2(X, Yf);
		}
		void Vector2::Set(const Vector2& Value)
		{
			X = Value.X;
			Y = Value.Y;
		}
		void Vector2::Get2(float* In) const
		{
			VI_ASSERT(In != nullptr, "in of size 2 should be set");
			In[0] = X;
			In[1] = Y;
		}
		Vector2& Vector2::operator *=(const Vector2& V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, V);
			_r1 = _r1 * _r2;
			_r1.store_partial(2, (float*)this);
#else
			X *= V.X;
			Y *= V.Y;
#endif
			return *this;
		}
		Vector2& Vector2::operator *=(float V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			_r1 = _r1 * V;
			_r1.store_partial(2, (float*)this);
#else
			X *= V;
			Y *= V;
#endif
			return *this;
		}
		Vector2& Vector2::operator /=(const Vector2& V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, V);
			_r1 = _r1 / _r2;
			_r1.store_partial(2, (float*)this);
#else
			X /= V.X;
			Y /= V.Y;
#endif
			return *this;
		}
		Vector2& Vector2::operator /=(float V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			_r1 = _r1 / V;
			_r1.store_partial(2, (float*)this);
#else
			X /= V;
			Y /= V;
#endif
			return *this;
		}
		Vector2& Vector2::operator +=(const Vector2& V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, V);
			_r1 = _r1 + _r2;
			_r1.store_partial(2, (float*)this);
#else
			X += V.X;
			Y += V.Y;
#endif
			return *this;
		}
		Vector2& Vector2::operator +=(float V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			_r1 = _r1 + V;
			_r1.store_partial(2, (float*)this);
#else
			X += V;
			Y += V;
#endif
			return *this;
		}
		Vector2& Vector2::operator -=(const Vector2& V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, V);
			_r1 = _r1 - _r2;
			_r1.store_partial(2, (float*)this);
#else
			X -= V.X;
			Y -= V.Y;
#endif
			return *this;
		}
		Vector2& Vector2::operator -=(float V)
		{
#ifdef VI_SIMD
			LOD_FV2(_r1);
			_r1 = _r1 - V;
			_r1.store_partial(2, (float*)this);
#else
			X -= V;
			Y -= V;
#endif
			return *this;
		}
		Vector2 Vector2::operator *(const Vector2& V) const
		{
			return Mul(V);
		}
		Vector2 Vector2::operator *(float V) const
		{
			return Mul(V);
		}
		Vector2 Vector2::operator /(const Vector2& V) const
		{
			return Div(V);
		}
		Vector2 Vector2::operator /(float V) const
		{
			return Div(V);
		}
		Vector2 Vector2::operator +(const Vector2& V) const
		{
			return Add(V);
		}
		Vector2 Vector2::operator +(float V) const
		{
			return Add(V);
		}
		Vector2 Vector2::operator -(const Vector2& V) const
		{
			return Add(-V);
		}
		Vector2 Vector2::operator -(float V) const
		{
			return Add(-V);
		}
		Vector2 Vector2::operator -() const
		{
			return Inv();
		}
		Vector2& Vector2::operator =(const Vector2& V) noexcept
		{
			X = V.X;
			Y = V.Y;
			return *this;
		}
		bool Vector2::operator ==(const Vector2& R) const
		{
			return X == R.X && Y == R.Y;
		}
		bool Vector2::operator !=(const Vector2& R) const
		{
			return !(X == R.X && Y == R.Y);
		}
		bool Vector2::operator <=(const Vector2& R) const
		{
			return X <= R.X && Y <= R.Y;
		}
		bool Vector2::operator >=(const Vector2& R) const
		{
			return X >= R.X && Y >= R.Y;
		}
		bool Vector2::operator <(const Vector2& R) const
		{
			return X < R.X&& Y < R.Y;
		}
		bool Vector2::operator >(const Vector2& R) const
		{
			return X > R.X && Y > R.Y;
		}
		float& Vector2::operator [](uint32_t Axis)
		{
			VI_ASSERT(Axis >= 0 && Axis <= 1, "index out of range");
			if (Axis == 0)
				return X;

			return Y;
		}
		float Vector2::operator [](uint32_t Axis) const
		{
			VI_ASSERT(Axis >= 0 && Axis <= 1, "index out of range");
			if (Axis == 0)
				return X;

			return Y;
		}
		Vector2 Vector2::Random()
		{
			return Vector2(Mathf::RandomMag(), Mathf::RandomMag());
		}
		Vector2 Vector2::RandomAbs()
		{
			return Vector2(Mathf::Random(), Mathf::Random());
		}

		Vector3::Vector3() noexcept : X(0.0f), Y(0.0f), Z(0.0f)
		{
		}
		Vector3::Vector3(float x, float y) noexcept : X(x), Y(y), Z(0.0f)
		{
		}
		Vector3::Vector3(float x, float y, float z) noexcept : X(x), Y(y), Z(z)
		{
		}
		Vector3::Vector3(float xyzw) noexcept : X(xyzw), Y(xyzw), Z(xyzw)
		{
		}
		Vector3::Vector3(const Vector2& Value) noexcept : X(Value.X), Y(Value.Y), Z(0.0f)
		{
		}
		Vector3::Vector3(const Vector3& Value) noexcept : X(Value.X), Y(Value.Y), Z(Value.Z)
		{
		}
		Vector3::Vector3(const Vector4& Value) noexcept : X(Value.X), Y(Value.Y), Z(Value.Z)
		{
		}
		bool Vector3::IsEquals(const Vector3& Other, float MaxDisplacement) const
		{
			return abs(X - Other.X) <= MaxDisplacement && abs(Y - Other.Y) <= MaxDisplacement && abs(Z - Other.Z) <= MaxDisplacement;
		}
		float Vector3::Length() const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1);
			return sqrt(horizontal_add(square(_r1)));
#else
			return std::sqrt(X * X + Y * Y + Z * Z);
#endif
		}
		float Vector3::Sum() const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1);
			return horizontal_add(_r1);
#else
			return X + Y + Z;
#endif
		}
		float Vector3::Dot(const Vector3& B) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, B);
			return horizontal_add(_r1 * _r2);
#else
			return X * B.X + Y * B.Y + Z * B.Z;
#endif
		}
		float Vector3::Distance(const Vector3& Point) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Point);
			return Geometric::FastSqrt(horizontal_add(square(_r1 - _r2)));
#else
			float X1 = X - Point.X, Y1 = Y - Point.Y, Z1 = Z - Point.Z;
			return Geometric::FastSqrt(X1 * X1 + Y1 * Y1 + Z1 * Z1);
#endif
		}
		float Vector3::Hypotenuse() const
		{
#ifdef VI_SIMD
			LOD_AV2(_r1, X, Z);
			float R = Geometric::FastSqrt(horizontal_add(square(_r1)));

			LOD_AV2(_r2, R, Y);
			return Geometric::FastSqrt(horizontal_add(square(_r2)));
#else
			float R = Geometric::FastSqrt(X * X + Z * Z);
			return Geometric::FastSqrt(R * R + Y * Y);
#endif
		}
		Vector3 Vector3::LookAt(const Vector3& B) const
		{
			Vector2 H1(X, Z), H2(B.X, B.Z);
			return Vector3(0.0f, -H1.LookAt(H2), 0.0f);
		}
		Vector3 Vector3::Cross(const Vector3& B) const
		{
#ifdef VI_SIMD
			LOD_AV3(_r1, Y, Z, X);
			LOD_AV3(_r2, Z, X, Y);
			LOD_AV3(_r3, B.Z, B.X, B.Y);
			LOD_AV3(_r4, B.Y, B.Z, B.X);

			_r1 = _r1 * _r3 - _r2 * _r4;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(Y * B.Z - Z * B.Y, Z * B.X - X * B.Z, X * B.Y - Y * B.X);
#endif
		}
		Vector3 Vector3::Transform(const Matrix4x4& Matrix) const
		{
#ifdef VI_SIMD
			LOD_VAL(_r1, X);
			LOD_VAL(_r2, Y);
			LOD_VAL(_r3, Z);
			LOD_VAR(_r4, Matrix.Row);
			LOD_VAR(_r5, Matrix.Row + 4);
			LOD_VAR(_r6, Matrix.Row + 8);

			_r1 = _r1 * _r4 + _r2 * _r5 + _r3 * _r6;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(
				X * Matrix.Row[0] + Y * Matrix.Row[4] + Z * Matrix.Row[8],
				X * Matrix.Row[1] + Y * Matrix.Row[5] + Z * Matrix.Row[9],
				X * Matrix.Row[2] + Y * Matrix.Row[6] + Z * Matrix.Row[10]);
#endif
		}
		Vector3 Vector3::dDirection() const
		{
			float CosX = cos(X);
			return Vector3(sin(Y) * CosX, -sin(X), cos(Y) * CosX);
		}
		Vector3 Vector3::hDirection() const
		{
			return Vector3(-cos(Y), 0, sin(Y));
		}
		Vector3 Vector3::Direction() const
		{
			return Matrix4x4::CreateLookAt(0, *this, Vector3::Up()).RotationEuler();
		}
		Vector3 Vector3::Inv() const
		{
			return Vector3(-X, -Y, -Z);
		}
		Vector3 Vector3::InvX() const
		{
			return Vector3(-X, Y, Z);
		}
		Vector3 Vector3::InvY() const
		{
			return Vector3(X, -Y, Z);
		}
		Vector3 Vector3::InvZ() const
		{
			return Vector3(X, Y, -Z);
		}
		Vector3 Vector3::Normalize() const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1);
			_r1 = _r1 * Geometric::FastInvSqrt(horizontal_add(square(_r1)));
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			float F = Length();
			return Vector3(X / F, Y / F, Z / F);
#endif
		}
		Vector3 Vector3::sNormalize() const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1);
			float F = Geometric::FastSqrt(horizontal_add(square(_r1)));
			if (F == 0.0f)
				return Vector3();

			_r1 = _r1 / F;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			float F = Length();
			if (F == 0.0f)
				return Vector3();

			return Vector3(X / F, Y / F, Z / F);
#endif
		}
		Vector3 Vector3::Lerp(const Vector3& B, float DeltaTime) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, B);
			_r1 = _r1 + (_r2 - _r1) * DeltaTime;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return *this + (B - *this) * DeltaTime;
#endif
		}
		Vector3 Vector3::sLerp(const Vector3& B, float DeltaTime) const
		{
			return Quaternion(*this).sLerp(B, DeltaTime).GetEuler();
		}
		Vector3 Vector3::aLerp(const Vector3& B, float DeltaTime) const
		{
			float Ax = Mathf::AngluarLerp(X, B.X, DeltaTime);
			float Ay = Mathf::AngluarLerp(Y, B.Y, DeltaTime);
			float Az = Mathf::AngluarLerp(Z, B.Z, DeltaTime);

			return Vector3(Ax, Ay, Az);
		}
		Vector3 Vector3::rLerp() const
		{
			float Ax = Mathf::SaturateAngle(X);
			float Ay = Mathf::SaturateAngle(Y);
			float Az = Mathf::SaturateAngle(Z);

			return Vector3(Ax, Ay, Az);
		}
		Vector3 Vector3::Abs() const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); _r1 = abs(_r1);
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X < 0 ? -X : X, Y < 0 ? -Y : Y, Z < 0 ? -Z : Z);
#endif
		}
		Vector3 Vector3::Radians() const
		{
			return (*this) * Mathf::Deg2Rad();
		}
		Vector3 Vector3::Degrees() const
		{
			return (*this) * Mathf::Rad2Deg();
		}
		Vector3 Vector3::ViewSpace() const
		{
			return InvZ();
		}
		Vector2 Vector3::XY() const
		{
			return Vector2(X, Y);
		}
		Vector3 Vector3::XYZ() const
		{
			return *this;
		}
		Vector4 Vector3::XYZW() const
		{
			return Vector4(X, Y, Z, 0);
		}
		Vector3 Vector3::Mul(float xyz) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); _r1 = _r1 * xyz;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X * xyz, Y * xyz, Z * xyz);
#endif
		}
		Vector3 Vector3::Mul(const Vector2& XY, float z) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_AV3(_r2, XY.X, XY.Y, z); _r1 = _r1 * _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X * XY.X, Y * XY.Y, Z * z);
#endif
		}
		Vector3 Vector3::Mul(const Vector3& Value) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Value); _r1 = _r1 * _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X * Value.X, Y * Value.Y, Z * Value.Z);
#endif
		}
		Vector3 Vector3::Div(const Vector3& Value) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Value); _r1 = _r1 / _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X / Value.X, Y / Value.Y, Z / Value.Z);
#endif
		}
		Vector3 Vector3::Add(const Vector3& Value) const
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Value); _r1 = _r1 + _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X + Value.X, Y + Value.Y, Z + Value.Z);
#endif
		}
		Vector3 Vector3::SetX(float Xf) const
		{
			return Vector3(Xf, Y, Z);
		}
		Vector3 Vector3::SetY(float Yf) const
		{
			return Vector3(X, Yf, Z);
		}
		Vector3 Vector3::SetZ(float Zf) const
		{
			return Vector3(X, Y, Zf);
		}
		Vector3 Vector3::Rotate(const Vector3& Origin, const Vector3& Rotation)
		{
			return Transform(Matrix4x4::Create(Origin, Rotation));
		}
		void Vector3::Set(const Vector3& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
		}
		void Vector3::Get2(float* In) const
		{
			VI_ASSERT(In != nullptr, "in of size 2 should be set");
			In[0] = X;
			In[1] = Y;
		}
		void Vector3::Get3(float* In) const
		{
			VI_ASSERT(In != nullptr, "in of size 3 should be set");
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
		}
		Vector3& Vector3::operator *=(const Vector3& V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, V);
			_r1 = _r1 * _r2;
			_r1.store_partial(3, (float*)this);
#else
			X *= V.X;
			Y *= V.Y;
			Z *= V.Z;
#endif
			return *this;
		}
		Vector3& Vector3::operator *=(float V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 * _r2;
			_r1.store_partial(3, (float*)this);
#else
			X *= V;
			Y *= V;
			Z *= V;
#endif
			return *this;
		}
		Vector3& Vector3::operator /=(const Vector3& V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, V);
			_r1 = _r1 / _r2;
			_r1.store_partial(3, (float*)this);
#else
			X /= V.X;
			Y /= V.Y;
			Z /= V.Z;
#endif
			return *this;
		}
		Vector3& Vector3::operator /=(float V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 / _r2;
			_r1.store_partial(3, (float*)this);
#else
			X *= V;
			Y *= V;
			Z *= V;
#endif
			return *this;
		}
		Vector3& Vector3::operator +=(const Vector3& V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, V);
			_r1 = _r1 + _r2;
			_r1.store_partial(3, (float*)this);
#else
			X += V.X;
			Y += V.Y;
			Z += V.Z;
#endif
			return *this;
		}
		Vector3& Vector3::operator +=(float V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 + _r2;
			_r1.store_partial(3, (float*)this);
#else
			X += V;
			Y += V;
			Z += V;
#endif
			return *this;
		}
		Vector3& Vector3::operator -=(const Vector3& V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, V);
			_r1 = _r1 - _r2;
			_r1.store_partial(3, (float*)this);
#else
			X -= V.X;
			Y -= V.Y;
			Z -= V.Z;
#endif
			return *this;
		}
		Vector3& Vector3::operator -=(float V)
		{
#ifdef VI_SIMD
			LOD_FV3(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 - _r2;
			_r1.store_partial(3, (float*)this);
#else
			X -= V;
			Y -= V;
			Z -= V;
#endif
			return *this;
		}
		Vector3 Vector3::operator *(const Vector3& V) const
		{
			return Mul(V);
		}
		Vector3 Vector3::operator *(float V) const
		{
			return Mul(V);
		}
		Vector3 Vector3::operator /(const Vector3& V) const
		{
			return Div(V);
		}
		Vector3 Vector3::operator /(float V) const
		{
			return Div(V);
		}
		Vector3 Vector3::operator +(const Vector3& V) const
		{
			return Add(V);
		}
		Vector3 Vector3::operator +(float V) const
		{
			return Add(V);
		}
		Vector3 Vector3::operator -(const Vector3& V) const
		{
			return Add(-V);
		}
		Vector3 Vector3::operator -(float V) const
		{
			return Add(-V);
		}
		Vector3 Vector3::operator -() const
		{
			return Inv();
		}
		Vector3& Vector3::operator =(const Vector3& V) noexcept
		{
			X = V.X;
			Y = V.Y;
			Z = V.Z;
			return *this;
		}
		bool Vector3::operator ==(const Vector3& R) const
		{
			return X == R.X && Y == R.Y && Z == R.Z;
		}
		bool Vector3::operator !=(const Vector3& R) const
		{
			return !(X == R.X && Y == R.Y && Z == R.Z);
		}
		bool Vector3::operator <=(const Vector3& R) const
		{
			return X <= R.X && Y <= R.Y && Z <= R.Z;
		}
		bool Vector3::operator >=(const Vector3& R) const
		{
			return X >= R.X && Y >= R.Y && Z >= R.Z;
		}
		bool Vector3::operator <(const Vector3& R) const
		{
			return X < R.X&& Y < R.Y&& Z < R.Z;
		}
		bool Vector3::operator >(const Vector3& R) const
		{
			return X > R.X && Y > R.Y && Z > R.Z;
		}
		float& Vector3::operator [](uint32_t Axis)
		{
			VI_ASSERT(Axis >= 0 && Axis <= 2, "index out of range");
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;

			return Z;
		}
		float Vector3::operator [](uint32_t Axis) const
		{
			VI_ASSERT(Axis >= 0 && Axis <= 2, "index out of range");
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;

			return Z;
		}
		Vector3 Vector3::Random()
		{
			return Vector3(Mathf::RandomMag(), Mathf::RandomMag(), Mathf::RandomMag());
		}
		Vector3 Vector3::RandomAbs()
		{
			return Vector3(Mathf::Random(), Mathf::Random(), Mathf::Random());
		}

		Vector4::Vector4() noexcept : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f)
		{
		}
		Vector4::Vector4(float x, float y) noexcept : X(x), Y(y), Z(0.0f), W(0.0f)
		{
		}
		Vector4::Vector4(float x, float y, float z) noexcept : X(x), Y(y), Z(z), W(0.0f)
		{
		}
		Vector4::Vector4(float x, float y, float z, float w) noexcept : X(x), Y(y), Z(z), W(w)
		{
		}
		Vector4::Vector4(float xyzw) noexcept : X(xyzw), Y(xyzw), Z(xyzw), W(xyzw)
		{
		}
		Vector4::Vector4(const Vector2& Value) noexcept : X(Value.X), Y(Value.Y), Z(0.0f), W(0.0f)
		{
		}
		Vector4::Vector4(const Vector3& Value) noexcept : X(Value.X), Y(Value.Y), Z(Value.Z), W(0.0f)
		{
		}
		Vector4::Vector4(const Vector4& Value) noexcept : X(Value.X), Y(Value.Y), Z(Value.Z), W(Value.W)
		{
		}
		bool Vector4::IsEquals(const Vector4& Other, float MaxDisplacement) const
		{
			return abs(X - Other.X) <= MaxDisplacement && abs(Y - Other.Y) <= MaxDisplacement && abs(Z - Other.Z) <= MaxDisplacement && abs(W - Other.W) <= MaxDisplacement;
		}
		float Vector4::Length() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			return std::sqrt(horizontal_add(square(_r1)));
#else
			return std::sqrt(X * X + Y * Y + Z * Z + W * W);
#endif
		}
		float Vector4::Sum() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			return horizontal_add(_r1);
#else
			return X + Y + Z + W;
#endif
		}
		float Vector4::Dot(const Vector4& B) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, B);
			return horizontal_add(_r1 * _r2);
#else
			return X * B.X + Y * B.Y + Z * B.Z + W * B.W;
#endif
		}
		float Vector4::Distance(const Vector4& Point) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Point);
			return Geometric::FastSqrt(horizontal_add(square(_r1 - _r2)));
#else
			float X1 = X - Point.X, Y1 = Y - Point.Y, Z1 = Z - Point.Z, W1 = W - Point.W;
			return Geometric::FastSqrt(X1 * X1 + Y1 * Y1 + Z1 * Z1 + W1 * W1);
#endif
		}
		Vector4 Vector4::Cross(const Vector4& B) const
		{
#ifdef VI_SIMD
			LOD_AV3(_r1, Y, Z, X);
			LOD_AV3(_r2, Z, X, Y);
			LOD_AV3(_r3, B.Z, B.X, B.Y);
			LOD_AV3(_r4, B.Y, B.Z, B.X);

			_r1 = _r1 * _r3 - _r2 * _r4;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector4(Y * B.Z - Z * B.Y, Z * B.X - X * B.Z, X * B.Y - Y * B.X);
#endif
		}
		Vector4 Vector4::Transform(const Matrix4x4& Matrix) const
		{
#ifdef VI_SIMD
			LOD_VAL(_r1, X);
			LOD_VAL(_r2, Y);
			LOD_VAL(_r3, Z);
			LOD_VAL(_r4, W);
			LOD_VAR(_r5, Matrix.Row);
			LOD_VAR(_r6, Matrix.Row + 4);
			LOD_VAR(_r7, Matrix.Row + 8);
			LOD_VAR(_r8, Matrix.Row + 12);

			_r1 = _r1 * _r5 + _r2 * _r6 + _r3 * _r7 + _r4 * _r8;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(
				X * Matrix.Row[0] + Y * Matrix.Row[4] + Z * Matrix.Row[8] + W * Matrix.Row[12],
				X * Matrix.Row[1] + Y * Matrix.Row[5] + Z * Matrix.Row[9] + W * Matrix.Row[13],
				X * Matrix.Row[2] + Y * Matrix.Row[6] + Z * Matrix.Row[10] + W * Matrix.Row[14],
				X * Matrix.Row[3] + Y * Matrix.Row[7] + Z * Matrix.Row[11] + W * Matrix.Row[15]);
#endif
		}
		Vector4 Vector4::Inv() const
		{
			return Vector4(-X, -Y, -Z, -W);
		}
		Vector4 Vector4::InvX() const
		{
			return Vector4(-X, Y, Z, W);
		}
		Vector4 Vector4::InvY() const
		{
			return Vector4(X, -Y, Z, W);
		}
		Vector4 Vector4::InvZ() const
		{
			return Vector4(X, Y, -Z, W);
		}
		Vector4 Vector4::InvW() const
		{
			return Vector4(X, Y, Z, -W);
		}
		Vector4 Vector4::Normalize() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			_r1 = _r1 * Geometric::FastInvSqrt(horizontal_add(square(_r1)));
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			float F = Length();
			return Vector4(X / F, Y / F, Z / F, W / F);
#endif
		}
		Vector4 Vector4::sNormalize() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			float F = Geometric::FastSqrt(horizontal_add(square(_r1)));
			if (F == 0.0f)
				return Vector4();

			_r1 = _r1 / F;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			float F = Length();
			if (F == 0.0f)
				return Vector4();

			return Vector4(X / F, Y / F, Z / F, W / F);
#endif
		}
		Vector4 Vector4::Lerp(const Vector4& B, float DeltaTime) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, B);
			_r1 = _r1 + (_r2 - _r1) * DeltaTime;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return *this + (B - *this) * DeltaTime;
#endif
		}
		Vector4 Vector4::sLerp(const Vector4& B, float DeltaTime) const
		{
			Vector3 Lerp = Quaternion(Vector3()).sLerp(B.XYZ(), DeltaTime).GetEuler();
			return Vector4(Lerp.X, Lerp.Y, Lerp.Z, W + (B.W - W) * DeltaTime);
		}
		Vector4 Vector4::aLerp(const Vector4& B, float DeltaTime) const
		{
			float Ax = Mathf::AngluarLerp(X, B.X, DeltaTime);
			float Ay = Mathf::AngluarLerp(Y, B.Y, DeltaTime);
			float Az = Mathf::AngluarLerp(Z, B.Z, DeltaTime);
			float Aw = Mathf::AngluarLerp(W, B.W, DeltaTime);

			return Vector4(Ax, Ay, Az, Aw);
		}
		Vector4 Vector4::rLerp() const
		{
			float Ax = Mathf::SaturateAngle(X);
			float Ay = Mathf::SaturateAngle(Y);
			float Az = Mathf::SaturateAngle(Z);
			float Aw = Mathf::SaturateAngle(W);

			return Vector4(Ax, Ay, Az, Aw);
		}
		Vector4 Vector4::Abs() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); _r1 = abs(_r1);
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X < 0 ? -X : X, Y < 0 ? -Y : Y, Z < 0 ? -Z : Z, W < 0 ? -W : W);
#endif
		}
		Vector4 Vector4::Radians() const
		{
			return (*this) * Mathf::Deg2Rad();
		}
		Vector4 Vector4::Degrees() const
		{
			return (*this) * Mathf::Rad2Deg();
		}
		Vector4 Vector4::ViewSpace() const
		{
			return InvZ();
		}
		Vector2 Vector4::XY() const
		{
			return Vector2(X, Y);
		}
		Vector3 Vector4::XYZ() const
		{
			return Vector3(X, Y, Z);
		}
		Vector4 Vector4::XYZW() const
		{
			return *this;
		}
		Vector4 Vector4::SetX(float TexCoordX) const
		{
			return Vector4(TexCoordX, Y, Z, W);
		}
		Vector4 Vector4::SetY(float TexCoordY) const
		{
			return Vector4(X, TexCoordY, Z, W);
		}
		Vector4 Vector4::SetZ(float TZ) const
		{
			return Vector4(X, Y, TZ, W);
		}
		Vector4 Vector4::SetW(float TW) const
		{
			return Vector4(X, Y, Z, TW);
		}
		Vector4 Vector4::Mul(float xyzw) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); _r1 = _r1 * xyzw;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * xyzw, Y * xyzw, Z * xyzw, W * xyzw);
#endif
		}
		Vector4 Vector4::Mul(const Vector2& XY, float z, float w) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_AV4(_r2, XY.X, XY.Y, z, w); _r1 = _r1 * _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * XY.X, Y * XY.Y, Z * z, W * w);
#endif
		}
		Vector4 Vector4::Mul(const Vector3& XYZ, float w) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_AV4(_r2, XYZ.X, XYZ.Y, XYZ.Z, w); _r1 = _r1 * _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * XYZ.X, Y * XYZ.Y, Z * XYZ.Z, W * w);
#endif
		}
		Vector4 Vector4::Mul(const Vector4& Value) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Value); _r1 = _r1 * _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * Value.X, Y * Value.Y, Z * Value.Z, W * Value.W);
#endif
		}
		Vector4 Vector4::Div(const Vector4& Value) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Value); _r1 = _r1 / _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X / Value.X, Y / Value.Y, Z / Value.Z, W / Value.W);
#endif
		}
		Vector4 Vector4::Add(const Vector4& Value) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Value); _r1 = _r1 + _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X + Value.X, Y + Value.Y, Z + Value.Z, W + Value.W);
#endif
		}
		void Vector4::Set(const Vector4& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
			W = Value.W;
		}
		void Vector4::Get2(float* In) const
		{
			VI_ASSERT(In != nullptr, "in of size 2 should be set");
			In[0] = X;
			In[1] = Y;
		}
		void Vector4::Get3(float* In) const
		{
			VI_ASSERT(In != nullptr, "in of size 3 should be set");
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
		}
		void Vector4::Get4(float* In) const
		{
			VI_ASSERT(In != nullptr, "in of size 4 should be set");
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
			In[3] = W;
		}
		Vector4& Vector4::operator *=(const Matrix4x4& V)
		{
			Set(Transform(V));
			return *this;
		}
		Vector4& Vector4::operator *=(const Vector4& V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, V);
			_r1 = _r1 * _r2;
			_r1.store((float*)this);
#else
			X *= V.X;
			Y *= V.Y;
			Z *= V.Z;
			W *= V.W;
#endif
			return *this;
		}
		Vector4& Vector4::operator *=(float V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 * _r2;
			_r1.store((float*)this);
#else
			X *= V;
			Y *= V;
			Z *= V;
			W *= V;
#endif
			return *this;
		}
		Vector4& Vector4::operator /=(const Vector4& V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, V);
			_r1 = _r1 / _r2;
			_r1.store((float*)this);
#else
			X /= V.X;
			Y /= V.Y;
			Z /= V.Z;
			W /= V.W;
#endif
			return *this;
		}
		Vector4& Vector4::operator /=(float V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 / _r2;
			_r1.store((float*)this);
#else
			X /= V;
			Y /= V;
			Z /= V;
			W /= V;
#endif
			return *this;
		}
		Vector4& Vector4::operator +=(const Vector4& V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, V);
			_r1 = _r1 + _r2;
			_r1.store((float*)this);
#else
			X += V.X;
			Y += V.Y;
			Z += V.Z;
			W += V.W;
#endif
			return *this;
		}
		Vector4& Vector4::operator +=(float V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 + _r2;
			_r1.store((float*)this);
#else
			X += V;
			Y += V;
			Z += V;
			W += V;
#endif
			return *this;
		}
		Vector4& Vector4::operator -=(const Vector4& V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, V);
			_r1 = _r1 - _r2;
			_r1.store((float*)this);
#else
			X -= V.X;
			Y -= V.Y;
			Z -= V.Z;
			W -= V.W;
#endif
			return *this;
		}
		Vector4& Vector4::operator -=(float V)
		{
#ifdef VI_SIMD
			LOD_FV4(_r1); LOD_VAL(_r2, V);
			_r1 = _r1 - _r2;
			_r1.store((float*)this);
#else
			X -= V;
			Y -= V;
			Z -= V;
			W -= V;
#endif
			return *this;
		}
		Vector4 Vector4::operator *(const Matrix4x4& V) const
		{
			return Transform(V);
		}
		Vector4 Vector4::operator *(const Vector4& V) const
		{
			return Mul(V);
		}
		Vector4 Vector4::operator *(float V) const
		{
			return Mul(V);
		}
		Vector4 Vector4::operator /(const Vector4& V) const
		{
			return Div(V);
		}
		Vector4 Vector4::operator /(float V) const
		{
			return Div(V);
		}
		Vector4 Vector4::operator +(const Vector4& V) const
		{
			return Add(V);
		}
		Vector4 Vector4::operator +(float V) const
		{
			return Add(V);
		}
		Vector4 Vector4::operator -(const Vector4& V) const
		{
			return Add(-V);
		}
		Vector4 Vector4::operator -(float V) const
		{
			return Add(-V);
		}
		Vector4 Vector4::operator -() const
		{
			return Inv();
		}
		Vector4& Vector4::operator =(const Vector4& V) noexcept
		{
			X = V.X;
			Y = V.Y;
			Z = V.Z;
			W = V.W;
			return *this;
		}
		bool Vector4::operator ==(const Vector4& R) const
		{
			return X == R.X && Y == R.Y && Z == R.Z && W == R.W;
		}
		bool Vector4::operator !=(const Vector4& R) const
		{
			return !(X == R.X && Y == R.Y && Z == R.Z && W == R.W);
		}
		bool Vector4::operator <=(const Vector4& R) const
		{
			return X <= R.X && Y <= R.Y && Z <= R.Z && W <= R.W;
		}
		bool Vector4::operator >=(const Vector4& R) const
		{
			return X >= R.X && Y >= R.Y && Z >= R.Z && W >= R.W;
		}
		bool Vector4::operator <(const Vector4& R) const
		{
			return X < R.X&& Y < R.Y&& Z < R.Z&& W < R.W;
		}
		bool Vector4::operator >(const Vector4& R) const
		{
			return X > R.X && Y > R.Y && Z > R.Z && W > R.W;
		}
		float& Vector4::operator [](uint32_t Axis)
		{
			VI_ASSERT(Axis >= 0 && Axis <= 3, "index outside of range");
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;
			else if (Axis == 2)
				return Z;

			return W;
		}
		float Vector4::operator [](uint32_t Axis) const
		{
			VI_ASSERT(Axis >= 0 && Axis <= 3, "index outside of range");
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;
			else if (Axis == 2)
				return Z;

			return W;
		}
		Vector4 Vector4::Random()
		{
			return Vector4(Mathf::RandomMag(), Mathf::RandomMag(), Mathf::RandomMag(), Mathf::RandomMag());
		}
		Vector4 Vector4::RandomAbs()
		{
			return Vector4(Mathf::Random(), Mathf::Random(), Mathf::Random(), Mathf::Random());
		}
		
		Bounding::Bounding() noexcept
		{
		}
		Bounding::Bounding(const Vector3& LowerBound, const Vector3& UpperBound) noexcept : Lower(LowerBound), Upper(UpperBound)
		{
			VI_ASSERT(Lower <= Upper, "lower should be smaller than upper");
			Volume = Geometric::AabbVolume(Lower, Upper);
			Radius = ((Upper - Lower) * 0.5f).Sum();
			Center = (Lower + Upper) * 0.5f;
		}
		void Bounding::Merge(const Bounding& A, const Bounding& B)
		{
			Lower.X = std::min(A.Lower.X, B.Lower.X);
			Lower.Y = std::min(A.Lower.Y, B.Lower.Y);
			Lower.Z = std::min(A.Lower.Z, B.Lower.Z);
			Upper.X = std::max(A.Upper.X, B.Upper.X);
			Upper.Y = std::max(A.Upper.Y, B.Upper.Y);
			Upper.Z = std::max(A.Upper.Z, B.Upper.Z);
			Volume = Geometric::AabbVolume(Lower, Upper);
			Radius = ((Upper - Lower) * 0.5f).Sum();
			Center = (Lower + Upper) * 0.5f;
		}
		bool Bounding::Contains(const Bounding& Bounds) const
		{
			return Bounds.Lower >= Lower && Bounds.Upper <= Upper;
		}
		bool Bounding::Overlaps(const Bounding& Bounds) const
		{
			return Bounds.Upper >= Lower && Bounds.Lower <= Upper;
		}

		Frustum8C::Frustum8C() noexcept
		{
			Geometric::CreateFrustum8C(Corners, 90.0f, 1.0f, 0.1f, 1.0f);
		}
		Frustum8C::Frustum8C(float FieldOfView, float Aspect, float NearZ, float FarZ) noexcept
		{
			Geometric::CreateFrustum8CRad(Corners, FieldOfView, Aspect, NearZ, FarZ);
		}
		void Frustum8C::Transform(const Matrix4x4& Value)
		{
			for (uint32_t i = 0; i < 8; i++)
			{
				Vector4& Corner = Corners[i];
				Corner = Corner * Value;
			}
		}
		void Frustum8C::GetBoundingBox(Vector2* X, Vector2* Y, Vector2* Z)
		{
			VI_ASSERT(X || Y || Z, "at least one vector of x, y, z should be set");
			float MinX = std::numeric_limits<float>::max();
			float MaxX = std::numeric_limits<float>::min();
			float MinY = std::numeric_limits<float>::max();
			float MaxY = std::numeric_limits<float>::min();
			float MinZ = std::numeric_limits<float>::max();
			float MaxZ = std::numeric_limits<float>::min();

			for (uint32_t i = 0; i < 8; i++)
			{
				Vector4& Corner = Corners[i];
				MinX = std::min(MinX, Corner.X);
				MaxX = std::max(MaxX, Corner.X);
				MinY = std::min(MinY, Corner.Y);
				MaxY = std::max(MaxY, Corner.Y);
				MinZ = std::min(MinZ, Corner.Z);
				MaxZ = std::max(MaxZ, Corner.Z);
			}

			if (X != nullptr)
				*X = Vector2(MinX, MaxX);

			if (Y != nullptr)
				*Y = Vector2(MinY, MaxY);

			if (Z != nullptr)
				*Z = Vector2(MinZ, MaxZ);
		}

		Frustum6P::Frustum6P() noexcept
		{
		}
		Frustum6P::Frustum6P(const Matrix4x4& Clip) noexcept
		{
			Planes[(size_t)Side::RIGHT].X = Clip[3] - Clip[0];
			Planes[(size_t)Side::RIGHT].Y = Clip[7] - Clip[4];
			Planes[(size_t)Side::RIGHT].Z = Clip[11] - Clip[8];
			Planes[(size_t)Side::RIGHT].W = Clip[15] - Clip[12];
			NormalizePlane(Planes[(size_t)Side::RIGHT]);

			Planes[(size_t)Side::LEFT].X = Clip[3] + Clip[0];
			Planes[(size_t)Side::LEFT].Y = Clip[7] + Clip[4];
			Planes[(size_t)Side::LEFT].Z = Clip[11] + Clip[8];
			Planes[(size_t)Side::LEFT].W = Clip[15] + Clip[12];
			NormalizePlane(Planes[(size_t)Side::LEFT]);

			Planes[(size_t)Side::BOTTOM].X = Clip[3] + Clip[1];
			Planes[(size_t)Side::BOTTOM].Y = Clip[7] + Clip[5];
			Planes[(size_t)Side::BOTTOM].Z = Clip[11] + Clip[9];
			Planes[(size_t)Side::BOTTOM].W = Clip[15] + Clip[13];
			NormalizePlane(Planes[(size_t)Side::BOTTOM]);

			Planes[(size_t)Side::TOP].X = Clip[3] - Clip[1];
			Planes[(size_t)Side::TOP].Y = Clip[7] - Clip[5];
			Planes[(size_t)Side::TOP].Z = Clip[11] - Clip[9];
			Planes[(size_t)Side::TOP].W = Clip[15] - Clip[13];
			NormalizePlane(Planes[(size_t)Side::TOP]);

			Planes[(size_t)Side::BACK].X = Clip[3] - Clip[2];
			Planes[(size_t)Side::BACK].Y = Clip[7] - Clip[6];
			Planes[(size_t)Side::BACK].Z = Clip[11] - Clip[10];
			Planes[(size_t)Side::BACK].W = Clip[15] - Clip[14];
			NormalizePlane(Planes[(size_t)Side::BACK]);

			Planes[(size_t)Side::FRONT].X = Clip[3] + Clip[2];
			Planes[(size_t)Side::FRONT].Y = Clip[7] + Clip[6];
			Planes[(size_t)Side::FRONT].Z = Clip[11] + Clip[10];
			Planes[(size_t)Side::FRONT].W = Clip[15] + Clip[14];
			NormalizePlane(Planes[(size_t)Side::FRONT]);
		}
		void Frustum6P::NormalizePlane(Vector4& Plane)
		{
			float Magnitude = (float)sqrt(Plane.X * Plane.X + Plane.Y * Plane.Y + Plane.Z * Plane.Z);
			Plane.X /= Magnitude;
			Plane.Y /= Magnitude;
			Plane.Z /= Magnitude;
			Plane.W /= Magnitude;
		}
		bool Frustum6P::OverlapsAABB(const Bounding& Bounds) const
		{
			const Vector3& Mid = Bounds.Center;
			const Vector3& Min = Bounds.Lower;
			const Vector3& Max = Bounds.Upper;
			float Distance = -Bounds.Radius;
#ifdef VI_SIMD
			LOD_AV4(_rc, Mid.X, Mid.Y, Mid.Z, 1.0f);
			LOD_AV4(_m1, Min.X, Min.Y, Min.Z, 1.0f);
			LOD_AV4(_m2, Max.X, Min.Y, Min.Z, 1.0f);
			LOD_AV4(_m3, Min.X, Max.Y, Min.Z, 1.0f);
			LOD_AV4(_m4, Max.X, Max.Y, Min.Z, 1.0f);
			LOD_AV4(_m5, Min.X, Min.Y, Max.Z, 1.0f);
			LOD_AV4(_m6, Max.X, Min.Y, Max.Z, 1.0f);
			LOD_AV4(_m7, Min.X, Max.Y, Max.Z, 1.0f);
			LOD_AV4(_m8, Max.X, Max.Y, Max.Z, 1.0f);
#else
			Vector4 RC(Mid.X, Mid.Y, Mid.Z, 1.0f);
			Vector4 M1(Min.X, Min.Y, Min.Z, 1.0f);
			Vector4 M2(Max.X, Min.Y, Min.Z, 1.0f);
			Vector4 M3(Min.X, Max.Y, Min.Z, 1.0f);
			Vector4 M4(Max.X, Max.Y, Min.Z, 1.0f);
			Vector4 M5(Min.X, Min.Y, Max.Z, 1.0f);
			Vector4 M6(Max.X, Min.Y, Max.Z, 1.0f);
			Vector4 M7(Min.X, Max.Y, Max.Z, 1.0f);
			Vector4 M8(Max.X, Max.Y, Max.Z, 1.0f);
#endif
			for (size_t i = 0; i < 6; i++)
			{
#ifdef VI_SIMD
				LOD_V4(_rp, Planes[i]);
				if (horizontal_add(_rc * _rp) < Distance)
					return false;

				if (horizontal_add(_rp * _m1) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m2) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m3) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m4) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m5) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m6) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m7) > 0.0f)
					continue;

				if (horizontal_add(_rp * _m8) > 0.0f)
					continue;
#else
				auto& Plane = Planes[i];
				if (Plane.Dot(RC) < Distance)
					return false;

				if (Plane.Dot(M1) > 0)
					continue;

				if (Plane.Dot(M2) > 0)
					continue;

				if (Plane.Dot(M3) > 0)
					continue;

				if (Plane.Dot(M4) > 0)
					continue;

				if (Plane.Dot(M5) > 0)
					continue;

				if (Plane.Dot(M6) > 0)
					continue;

				if (Plane.Dot(M7) > 0)
					continue;

				if (Plane.Dot(M8) > 0)
					continue;
#endif
				return false;
			}

			return true;
		}
		bool Frustum6P::OverlapsSphere(const Vector3& Center, float Radius) const
		{
			Vector4 Position(Center.X, Center.Y, Center.Z, 1.0f);
			float Distance = -Radius;

			for (size_t i = 0; i < 6; i++)
			{
				if (Position.Dot(Planes[i]) < Distance)
					return false;
			}

			return true;
		}

		Ray::Ray() noexcept : Direction(0, 0, 1)
		{
		}
		Ray::Ray(const Vector3& _Origin, const Vector3& _Direction) noexcept : Origin(_Origin), Direction(_Direction)
		{
		}
		Vector3 Ray::GetPoint(float T) const
		{
			return Origin + (Direction * T);
		}
		Vector3 Ray::operator *(float T) const
		{
			return GetPoint(T);
		}
		bool Ray::IntersectsPlane(const Vector3& Normal, float Diameter) const
		{
			float D = Normal.Dot(Direction);
			if (Mathf::Abs(D) < std::numeric_limits<float>::epsilon())
				return false;

			float N = Normal.Dot(Origin) + Diameter;
			float T = -(N / D);
			return T >= 0;
		}
		bool Ray::IntersectsSphere(const Vector3& Position, float Radius, bool DiscardInside) const
		{
			Vector3 R = Origin - Position;
			float L = R.Length();

			if (L * L <= Radius * Radius && DiscardInside)
				return true;

			float A = Direction.Dot(Direction);
			float B = 2 * R.Dot(Direction);
			float C = R.Dot(R) - Radius * Radius;
			float D = (B * B) - (4 * A * C);

			return D >= 0.0f;
		}
		bool Ray::IntersectsAABBAt(const Vector3& Min, const Vector3& Max, Vector3* Hit) const
		{
			Vector3 HitPoint; float T;
			if (Origin > Min && Origin < Max)
				return true;

			if (Origin.X <= Min.X && Direction.X > 0)
			{
				T = (Min.X - Origin.X) / Direction.X;
				HitPoint = Origin + Direction * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint;

					return true;
				}
			}

			if (Origin.X >= Max.X && Direction.X < 0)
			{
				T = (Max.X - Origin.X) / Direction.X;
				HitPoint = Origin + Direction * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint;

					return true;
				}
			}

			if (Origin.Y <= Min.Y && Direction.Y > 0)
			{
				T = (Min.Y - Origin.Y) / Direction.Y;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint;

					return true;
				}
			}

			if (Origin.Y >= Max.Y && Direction.Y < 0)
			{
				T = (Max.Y - Origin.Y) / Direction.Y;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint;

					return true;
				}
			}

			if (Origin.Z <= Min.Z && Direction.Z > 0)
			{
				T = (Min.Z - Origin.Z) / Direction.Z;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
				{
					if (Hit != nullptr)
						*Hit = HitPoint;

					return true;
				}
			}

			if (Origin.Z >= Max.Z && Direction.Z < 0)
			{
				T = (Max.Z - Origin.Z) / Direction.Z;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
				{
					if (Hit != nullptr)
						*Hit = HitPoint;

					return true;
				}
			}

			return false;
		}
		bool Ray::IntersectsAABB(const Vector3& Position, const Vector3& Scale, Vector3* Hit) const
		{
			Vector3 Min = Position - Scale;
			Vector3 Max = Position + Scale;

			return IntersectsAABBAt(Min, Max, Hit);
		}
		bool Ray::IntersectsOBB(const Matrix4x4& World, Vector3* Hit) const
		{
			Matrix4x4 Offset = World.Inv();
			Vector3 Min = -1.0f, Max = 1.0f;
			Vector3 O = (Vector4(Origin.X, Origin.Y, Origin.Z, 1.0f) * Offset).XYZ();
			if (O > Min && O < Max)
				return true;

			Vector3 D = (Direction.XYZW() * Offset).sNormalize().XYZ();
			Vector3 HitPoint; float T;

			if (O.X <= Min.X && D.X > 0)
			{
				T = (Min.X - O.X) / D.X;
				HitPoint = O + D * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint.Transform(World);

					return true;
				}
			}

			if (O.X >= Max.X && D.X < 0)
			{
				T = (Max.X - O.X) / D.X;
				HitPoint = O + D * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint.Transform(World);

					return true;
				}
			}

			if (O.Y <= Min.Y && D.Y > 0)
			{
				T = (Min.Y - O.Y) / D.Y;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint.Transform(World);

					return true;
				}
			}

			if (O.Y >= Max.Y && D.Y < 0)
			{
				T = (Max.Y - O.Y) / D.Y;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
				{
					if (Hit != nullptr)
						*Hit = HitPoint.Transform(World);

					return true;
				}
			}

			if (O.Z <= Min.Z && D.Z > 0)
			{
				T = (Min.Z - O.Z) / D.Z;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
				{
					if (Hit != nullptr)
						*Hit = HitPoint.Transform(World);

					return true;
				}
			}

			if (O.Z >= Max.Z && D.Z < 0)
			{
				T = (Max.Z - O.Z) / D.Z;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
				{
					if (Hit != nullptr)
						*Hit = HitPoint.Transform(World);

					return true;
				}
			}

			return false;
		}

		Matrix4x4::Matrix4x4() noexcept : Row { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
		{
		}
		Matrix4x4::Matrix4x4(float Array[16]) noexcept
		{
			memcpy(Row, Array, sizeof(float) * 16);
		}
		Matrix4x4::Matrix4x4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3) noexcept
		{
			memcpy(Row + 0, &row0, sizeof(Vector4));
			memcpy(Row + 4, &row1, sizeof(Vector4));
			memcpy(Row + 8, &row2, sizeof(Vector4));
			memcpy(Row + 12, &row3, sizeof(Vector4));
		}
		Matrix4x4::Matrix4x4(float row00, float row01, float row02, float row03, float row10, float row11, float row12, float row13, float row20, float row21, float row22, float row23, float row30, float row31, float row32, float row33) noexcept :
			Row { row00, row01, row02, row03, row10, row11, row12, row13, row20, row21, row22, row23, row30, row31, row32, row33 }
		{
		}
		Matrix4x4::Matrix4x4(bool) noexcept
		{
		}
		Matrix4x4::Matrix4x4(const Matrix4x4& V) noexcept
		{
			memcpy(Row, V.Row, sizeof(float) * 16);
		}
		float& Matrix4x4::operator [](uint32_t Index)
		{
			return Row[Index];
		}
		float Matrix4x4::operator [](uint32_t Index) const
		{
			return Row[Index];
		}
		bool Matrix4x4::operator ==(const Matrix4x4& Equal) const
		{
			return memcmp(Row, Equal.Row, sizeof(float) * 16) == 0;
		}
		bool Matrix4x4::operator !=(const Matrix4x4& Equal) const
		{
			return memcmp(Row, Equal.Row, sizeof(float) * 16) != 0;
		}
		Matrix4x4 Matrix4x4::operator *(const Matrix4x4& V) const
		{
			return this->Mul(V);
		}
		Vector4 Matrix4x4::operator *(const Vector4& V) const
		{
			Matrix4x4 Result = this->Mul(V);
			return Vector4(Result.Row[0], Result.Row[4], Result.Row[8], Result.Row[12]);
		}
		Matrix4x4& Matrix4x4::operator =(const Matrix4x4& V) noexcept
		{
			memcpy(Row, V.Row, sizeof(float) * 16);
			return *this;
		}
		Vector4 Matrix4x4::Row11() const
		{
			return Vector4(Row[0], Row[1], Row[2], Row[3]);
		}
		Vector4 Matrix4x4::Row22() const
		{
			return Vector4(Row[4], Row[5], Row[6], Row[7]);
		}
		Vector4 Matrix4x4::Row33() const
		{
			return Vector4(Row[8], Row[9], Row[10], Row[11]);
		}
		Vector4 Matrix4x4::Row44() const
		{
			return Vector4(Row[12], Row[13], Row[14], Row[15]);
		}
		Vector3 Matrix4x4::Up() const
		{
			return Vector3(-Row[4], Row[5], Row[6]);
		}
		Vector3 Matrix4x4::Right() const
		{
			return Vector3(-Row[0], Row[1], Row[2]);
		}
		Vector3 Matrix4x4::Forward() const
		{
			return Vector3(-Row[8], Row[9], Row[10]);
		}
		Matrix4x4 Matrix4x4::Inv() const
		{
			Matrix4x4 Result(true);
			float A2323 = Row[10] * Row[15] - Row[11] * Row[14];
			float A1323 = Row[9] * Row[15] - Row[11] * Row[13];
			float A1223 = Row[9] * Row[14] - Row[10] * Row[13];
			float A0323 = Row[8] * Row[15] - Row[11] * Row[12];
			float A0223 = Row[8] * Row[14] - Row[10] * Row[12];
			float A0123 = Row[8] * Row[13] - Row[9] * Row[12];
			float A2313 = Row[6] * Row[15] - Row[7] * Row[14];
			float A1313 = Row[5] * Row[15] - Row[7] * Row[13];
			float A1213 = Row[5] * Row[14] - Row[6] * Row[13];
			float A2312 = Row[6] * Row[11] - Row[7] * Row[10];
			float A1312 = Row[5] * Row[11] - Row[7] * Row[9];
			float A1212 = Row[5] * Row[10] - Row[6] * Row[9];
			float A0313 = Row[4] * Row[15] - Row[7] * Row[12];
			float A0213 = Row[4] * Row[14] - Row[6] * Row[12];
			float A0312 = Row[4] * Row[11] - Row[7] * Row[8];
			float A0212 = Row[4] * Row[10] - Row[6] * Row[8];
			float A0113 = Row[4] * Row[13] - Row[5] * Row[12];
			float A0112 = Row[4] * Row[9] - Row[5] * Row[8];
#ifdef VI_SIMD
			LOD_AV4(_r1, Row[5], Row[4], Row[4], Row[4]);
			LOD_AV4(_r2, A2323, A2323, A1323, A1223);
			LOD_AV4(_r3, Row[6], Row[6], Row[5], Row[5]);
			LOD_AV4(_r4, A1323, A0323, A0323, A0223);
			LOD_AV4(_r5, Row[7], Row[7], Row[7], Row[6]);
			LOD_AV4(_r6, A1223, A0223, A0123, A0123);
			LOD_AV4(_r7, Row[0], -Row[1], Row[2], -Row[3]);
			LOD_AV4(_r8, 1.0f, -1.0f, 1.0f, -1.0f);
			_r7 *= _r1 * _r2 - _r3 * _r4 + _r5 * _r6;
			float F = horizontal_add(_r7);
			F = 1.0f / (F != 0.0f ? F : 1.0f);
			_r1 = Vec4f(Row[5], Row[1], Row[1], Row[1]);
			_r2 = Vec4f(A2323, A2323, A2313, A2312);
			_r3 = Vec4f(Row[6], Row[2], Row[2], Row[2]);
			_r4 = Vec4f(A1323, A1323, A1313, A1312);
			_r5 = Vec4f(Row[7], Row[3], Row[3], Row[3]);
			_r6 = Vec4f(A1223, A1223, A1213, A1212);
			_r7 = (_r1 * _r2 - _r3 * _r4 + _r5 * _r6) * _r8 * F;
			_r7.store(Result.Row + 0);
			_r1 = Vec4f(Row[4], Row[0], Row[0], Row[0]);
			_r4 = Vec4f(A0323, A0323, A0313, A0312);
			_r6 = Vec4f(A0223, A0223, A0213, A0212);
			_r7 = (_r1 * _r2 - _r3 * _r4 + _r5 * _r6) * -_r8 * F;
			_r7.store(Result.Row + 4);
			_r2 = Vec4f(A1323, A1323, A1313, A1312);
			_r3 = Vec4f(Row[5], Row[1], Row[1], Row[1]);
			_r6 = Vec4f(A0123, A0123, A0113, A0112);
			_r7 = (_r1 * _r2 - _r3 * _r4 + _r5 * _r6) * _r8 * F;
			_r7.store(Result.Row + 8);
			_r2 = Vec4f(A1223, A1223, A1213, A1212);
			_r4 = Vec4f(A0223, A0223, A0213, A0212);
			_r5 = Vec4f(Row[6], Row[2], Row[2], Row[2]);
			_r7 = (_r1 * _r2 - _r3 * _r4 + _r5 * _r6) * -_r8 * F;
			_r7.store(Result.Row + 12);
#else
			float F =
				Row[0] * (Row[5] * A2323 - Row[6] * A1323 + Row[7] * A1223)
				- Row[1] * (Row[4] * A2323 - Row[6] * A0323 + Row[7] * A0223)
				+ Row[2] * (Row[4] * A1323 - Row[5] * A0323 + Row[7] * A0123)
				- Row[3] * (Row[4] * A1223 - Row[5] * A0223 + Row[6] * A0123);
			F = 1.0f / (F != 0.0f ? F : 1.0f);

			Result.Row[0] = F * (Row[5] * A2323 - Row[6] * A1323 + Row[7] * A1223);
			Result.Row[1] = F * -(Row[1] * A2323 - Row[2] * A1323 + Row[3] * A1223);
			Result.Row[2] = F * (Row[1] * A2313 - Row[2] * A1313 + Row[3] * A1213);
			Result.Row[3] = F * -(Row[1] * A2312 - Row[2] * A1312 + Row[3] * A1212);
			Result.Row[4] = F * -(Row[4] * A2323 - Row[6] * A0323 + Row[7] * A0223);
			Result.Row[5] = F * (Row[0] * A2323 - Row[2] * A0323 + Row[3] * A0223);
			Result.Row[6] = F * -(Row[0] * A2313 - Row[2] * A0313 + Row[3] * A0213);
			Result.Row[7] = F * (Row[0] * A2312 - Row[2] * A0312 + Row[3] * A0212);
			Result.Row[8] = F * (Row[4] * A1323 - Row[5] * A0323 + Row[7] * A0123);
			Result.Row[9] = F * -(Row[0] * A1323 - Row[1] * A0323 + Row[3] * A0123);
			Result.Row[10] = F * (Row[0] * A1313 - Row[1] * A0313 + Row[3] * A0113);
			Result.Row[11] = F * -(Row[0] * A1312 - Row[1] * A0312 + Row[3] * A0112);
			Result.Row[12] = F * -(Row[4] * A1223 - Row[5] * A0223 + Row[6] * A0123);
			Result.Row[13] = F * (Row[0] * A1223 - Row[1] * A0223 + Row[2] * A0123);
			Result.Row[14] = F * -(Row[0] * A1213 - Row[1] * A0213 + Row[2] * A0113);
			Result.Row[15] = F * (Row[0] * A1212 - Row[1] * A0212 + Row[2] * A0112);
#endif
			return Result;
		}
		Matrix4x4 Matrix4x4::Transpose() const
		{
#ifdef VI_SIMD
			LOD_FV16(_r1);
			_r1 = permute16f<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>(_r1);

			Matrix4x4 Result(true);
			_r1.store(Result.Row);

			return Result;
#else
			return Matrix4x4(
				Vector4(Row[0], Row[4], Row[8], Row[12]),
				Vector4(Row[1], Row[5], Row[9], Row[13]),
				Vector4(Row[2], Row[6], Row[10], Row[14]),
				Vector4(Row[3], Row[7], Row[11], Row[15]));
#endif
		}
		Quaternion Matrix4x4::RotationQuaternion() const
		{
			Vector3 Scaling[3] =
			{
				Vector3(Row[0], Row[1], Row[2]),
				Vector3(Row[4], Row[5], Row[6]),
				Vector3(Row[8], Row[9], Row[10])
			};

			Vector3 Scale = { Scaling[0].Length(), Scaling[1].Length(), Scaling[2].Length() };
			if (Determinant() < 0)
				Scale = -Scale;

			if (Scale.X)
				Scaling[0] /= Scale.X;

			if (Scale.Y)
				Scaling[1] /= Scale.Y;

			if (Scale.Z)
				Scaling[2] /= Scale.Z;

			Matrix4x4 Rotated =
			{
				Scaling[0].X, Scaling[1].X, Scaling[2].X, 0.0f,
				Scaling[0].Y, Scaling[1].Y, Scaling[2].Y, 0.0f,
				Scaling[0].Z, Scaling[1].Z, Scaling[2].Z, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};

			return Quaternion(Rotated);
		}
		Vector3 Matrix4x4::RotationEuler() const
		{
			return RotationQuaternion().GetEuler();
		}
		Vector3 Matrix4x4::Position() const
		{
			return Vector3(Row[12], Row[13], Row[14]);
		}
		Vector3 Matrix4x4::Scale() const
		{
			Vector3 Scale =
			{
				Vector3(Row[0], Row[1], Row[2]).Length(),
				Vector3(Row[4], Row[5], Row[6]).Length(),
				Vector3(Row[8], Row[9], Row[10]).Length()
			};
			
			if (Determinant() < 0)
				Scale = -Scale;

			return Scale;
		}
		Matrix4x4 Matrix4x4::SetScale(const Vector3& Value) const
		{
			Matrix4x4 Local = *this;
			Local.Row[0] = Value.X;
			Local.Row[5] = Value.Y;
			Local.Row[10] = Value.Z;
			return Local;
		}
		Matrix4x4 Matrix4x4::Mul(const Matrix4x4& V) const
		{
			Matrix4x4 Result;
#ifdef VI_SIMD
			LOD_VAR(_r1, V.Row + 0);
			LOD_VAR(_r2, V.Row + 4);
			LOD_VAR(_r3, V.Row + 8);
			LOD_VAR(_r4, V.Row + 12);
			LOD_VAL(_r5, 0.0f);

			_r5 += _r1 * Row[0];
			_r5 += _r2 * Row[1];
			_r5 += _r3 * Row[2];
			_r5 += _r4 * Row[3];
			_r5.store(Result.Row + 0);
			_r5 = Vec4f(0.0f);
			_r5 += _r1 * Row[4];
			_r5 += _r2 * Row[5];
			_r5 += _r3 * Row[6];
			_r5 += _r4 * Row[7];
			_r5.store(Result.Row + 4);
			_r5 = Vec4f(0.0f);
			_r5 += _r1 * Row[8];
			_r5 += _r2 * Row[9];
			_r5 += _r3 * Row[10];
			_r5 += _r4 * Row[11];
			_r5.store(Result.Row + 8);
			_r5 = Vec4f(0.0f);
			_r5 += _r1 * Row[12];
			_r5 += _r2 * Row[13];
			_r5 += _r3 * Row[14];
			_r5 += _r4 * Row[15];
			_r5.store(Result.Row + 12);
#else
			Result.Row[0] = (Row[0] * V.Row[0]) + (Row[1] * V.Row[4]) + (Row[2] * V.Row[8]) + (Row[3] * V.Row[12]);
			Result.Row[1] = (Row[0] * V.Row[1]) + (Row[1] * V.Row[5]) + (Row[2] * V.Row[9]) + (Row[3] * V.Row[13]);
			Result.Row[2] = (Row[0] * V.Row[2]) + (Row[1] * V.Row[6]) + (Row[2] * V.Row[10]) + (Row[3] * V.Row[14]);
			Result.Row[3] = (Row[0] * V.Row[3]) + (Row[1] * V.Row[7]) + (Row[2] * V.Row[11]) + (Row[3] * V.Row[15]);
			Result.Row[4] = (Row[4] * V.Row[0]) + (Row[5] * V.Row[4]) + (Row[6] * V.Row[8]) + (Row[7] * V.Row[12]);
			Result.Row[5] = (Row[4] * V.Row[1]) + (Row[5] * V.Row[5]) + (Row[6] * V.Row[9]) + (Row[7] * V.Row[13]);
			Result.Row[6] = (Row[4] * V.Row[2]) + (Row[5] * V.Row[6]) + (Row[6] * V.Row[10]) + (Row[7] * V.Row[14]);
			Result.Row[7] = (Row[4] * V.Row[3]) + (Row[5] * V.Row[7]) + (Row[6] * V.Row[11]) + (Row[7] * V.Row[15]);
			Result.Row[8] = (Row[8] * V.Row[0]) + (Row[9] * V.Row[4]) + (Row[10] * V.Row[8]) + (Row[11] * V.Row[12]);
			Result.Row[9] = (Row[8] * V.Row[1]) + (Row[9] * V.Row[5]) + (Row[10] * V.Row[9]) + (Row[11] * V.Row[13]);
			Result.Row[10] = (Row[8] * V.Row[2]) + (Row[9] * V.Row[6]) + (Row[10] * V.Row[10]) + (Row[11] * V.Row[14]);
			Result.Row[11] = (Row[8] * V.Row[3]) + (Row[9] * V.Row[7]) + (Row[10] * V.Row[11]) + (Row[11] * V.Row[15]);
			Result.Row[12] = (Row[12] * V.Row[0]) + (Row[13] * V.Row[4]) + (Row[14] * V.Row[8]) + (Row[15] * V.Row[12]);
			Result.Row[13] = (Row[12] * V.Row[1]) + (Row[13] * V.Row[5]) + (Row[14] * V.Row[9]) + (Row[15] * V.Row[13]);
			Result.Row[14] = (Row[12] * V.Row[2]) + (Row[13] * V.Row[6]) + (Row[14] * V.Row[10]) + (Row[15] * V.Row[14]);
			Result.Row[15] = (Row[12] * V.Row[3]) + (Row[13] * V.Row[7]) + (Row[14] * V.Row[11]) + (Row[15] * V.Row[15]);
#endif
			return Result;
		}
		Matrix4x4 Matrix4x4::Mul(const Vector4& V) const
		{
			Matrix4x4 Result;
#ifdef VI_SIMD
			LOD_V4(_r1, V);
			LOD_VAR(_r2, Row + 0);
			LOD_VAR(_r3, Row + 4);
			LOD_VAR(_r4, Row + 8);
			LOD_VAR(_r5, Row + 12);
			LOD_VAL(_r6, 0.0f);

			_r6 = horizontal_add(_r1 * _r2);
			_r6.store(Result.Row + 0);
			_r6 = horizontal_add(_r1 * _r3);
			_r6.store(Result.Row + 4);
			_r6 = horizontal_add(_r1 * _r4);
			_r6.store(Result.Row + 8);
			_r6 = horizontal_add(_r1 * _r5);
			_r6.store(Result.Row + 12);
#else
			float X = (Row[0] * V.X) + (Row[1] * V.Y) + (Row[2] * V.Z) + (Row[3] * V.W);
			Result.Row[0] = Result.Row[1] = Result.Row[2] = Result.Row[3] = X;

			float Y = (Row[4] * V.X) + (Row[5] * V.Y) + (Row[6] * V.Z) + (Row[7] * V.W);
			Result.Row[4] = Result.Row[5] = Result.Row[6] = Result.Row[7] = Y;

			float Z = (Row[8] * V.X) + (Row[9] * V.Y) + (Row[10] * V.Z) + (Row[11] * V.W);
			Result.Row[8] = Result.Row[9] = Result.Row[10] = Result.Row[11] = Z;

			float W = (Row[12] * V.X) + (Row[13] * V.Y) + (Row[14] * V.Z) + (Row[15] * V.W);
			Result.Row[12] = Result.Row[13] = Result.Row[14] = Result.Row[15] = W;
#endif
			return Result;
		}
		Vector2 Matrix4x4::XY() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, Row[0], Row[4], Row[8], Row[12]);
			LOD_AV4(_r2, Row[1], Row[5], Row[9], Row[13]);
			return Vector2(horizontal_add(_r1), horizontal_add(_r2));
#else
			return Vector2(
				Row[0] + Row[4] + Row[8] + Row[12],
				Row[1] + Row[5] + Row[9] + Row[13]);
#endif
		}
		Vector3 Matrix4x4::XYZ() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, Row[0], Row[4], Row[8], Row[12]);
			LOD_AV4(_r2, Row[1], Row[5], Row[9], Row[13]);
			LOD_AV4(_r3, Row[2], Row[6], Row[10], Row[14]);
			return Vector3(horizontal_add(_r1), horizontal_add(_r2), horizontal_add(_r3));
#else
			return Vector3(
				Row[0] + Row[4] + Row[8] + Row[12],
				Row[1] + Row[5] + Row[9] + Row[13],
				Row[2] + Row[6] + Row[10] + Row[14]);
#endif
		}
		Vector4 Matrix4x4::XYZW() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, Row[0], Row[4], Row[8], Row[12]);
			LOD_AV4(_r2, Row[1], Row[5], Row[9], Row[13]);
			LOD_AV4(_r3, Row[2], Row[6], Row[10], Row[14]);
			LOD_AV4(_r4, Row[3], Row[7], Row[11], Row[15]);
			return Vector4(horizontal_add(_r1), horizontal_add(_r2), horizontal_add(_r3), horizontal_add(_r4));
#else
			return Vector4(
				Row[0] + Row[4] + Row[8] + Row[12],
				Row[1] + Row[5] + Row[9] + Row[13],
				Row[2] + Row[6] + Row[10] + Row[14],
				Row[3] + Row[7] + Row[11] + Row[15]);
#endif
		}
		float Matrix4x4::Determinant() const
		{
			return Row[0] * Row[5] * Row[10] * Row[15] - Row[0] * Row[5] * Row[11] * Row[14] + Row[0] * Row[6] * Row[11] * Row[13] - Row[0] * Row[6] * Row[9] * Row[15]
				+ Row[0] * Row[7] * Row[9] * Row[14] - Row[0] * Row[7] * Row[10] * Row[13] - Row[1] * Row[6] * Row[11] * Row[12] + Row[1] * Row[6] * Row[8] * Row[15]
				- Row[1] * Row[7] * Row[8] * Row[14] + Row[1] * Row[7] * Row[10] * Row[12] - Row[1] * Row[4] * Row[10] * Row[15] + Row[1] * Row[4] * Row[11] * Row[14]
				+ Row[2] * Row[7] * Row[8] * Row[13] - Row[2] * Row[7] * Row[9] * Row[12] + Row[2] * Row[4] * Row[9] * Row[15] - Row[2] * Row[4] * Row[11] * Row[13]
				+ Row[2] * Row[5] * Row[11] * Row[12] - Row[2] * Row[5] * Row[8] * Row[15] - Row[3] * Row[4] * Row[9] * Row[14] + Row[3] * Row[4] * Row[10] * Row[13]
				- Row[3] * Row[5] * Row[10] * Row[12] + Row[3] * Row[5] * Row[8] * Row[14] - Row[3] * Row[6] * Row[8] * Row[13] + Row[3] * Row[6] * Row[9] * Row[12];
		}
		void Matrix4x4::Identify()
		{
			static float Base[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
			memcpy(Row, Base, sizeof(float) * 16);
		}
		void Matrix4x4::Set(const Matrix4x4& Value)
		{
			memcpy(Row, Value.Row, sizeof(float) * 16);
		}
		Matrix4x4 Matrix4x4::CreateTranslatedScale(const Vector3& Position, const Vector3& Scale)
		{
			Matrix4x4 Value;
			Value.Row[12] = Position.X;
			Value.Row[13] = Position.Y;
			Value.Row[14] = Position.Z;
			Value.Row[0] = Scale.X;
			Value.Row[5] = Scale.Y;
			Value.Row[10] = Scale.Z;

			return Value;
		}
		Matrix4x4 Matrix4x4::CreateRotationX(float Rotation)
		{
			float Cos = cos(Rotation);
			float Sin = sin(Rotation);
			Matrix4x4 X;
			X.Row[5] = Cos;
			X.Row[6] = Sin;
			X.Row[9] = -Sin;
			X.Row[10] = Cos;

			return X;
		}
		Matrix4x4 Matrix4x4::CreateRotationY(float Rotation)
		{
			float Cos = cos(Rotation);
			float Sin = sin(Rotation);
			Matrix4x4 Y;
			Y.Row[0] = Cos;
			Y.Row[2] = -Sin;
			Y.Row[8] = Sin;
			Y.Row[10] = Cos;

			return Y;
		}
		Matrix4x4 Matrix4x4::CreateRotationZ(float Rotation)
		{
			float Cos = cos(Rotation);
			float Sin = sin(Rotation);
			Matrix4x4 Z;
			Z.Row[0] = Cos;
			Z.Row[1] = Sin;
			Z.Row[4] = -Sin;
			Z.Row[5] = Cos;

			return Z;
		}
		Matrix4x4 Matrix4x4::CreateRotation(const Vector3& Rotation)
		{
			return Matrix4x4::CreateRotationX(Rotation.X) * Matrix4x4::CreateRotationY(Rotation.Y) * Matrix4x4::CreateRotationZ(Rotation.Z);
		}
		Matrix4x4 Matrix4x4::CreateScale(const Vector3& Scale)
		{
			Matrix4x4 Result;
			Result.Row[0] = Scale.X;
			Result.Row[5] = Scale.Y;
			Result.Row[10] = Scale.Z;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& Position)
		{
			Matrix4x4 Result;
			Result.Row[12] = Position.X;
			Result.Row[13] = Position.Y;
			Result.Row[14] = Position.Z;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreatePerspectiveRad(float FieldOfView, float AspectRatio, float NearZ, float FarZ)
		{
			float Height = 1.0f / std::tan(0.5f * FieldOfView);
			float Width = Height / AspectRatio;
			float Depth = 1.0f / (FarZ - NearZ);

			return Matrix4x4(
				Vector4(Width, 0, 0, 0),
				Vector4(0, Height, 0, 0),
				Vector4(0, 0, FarZ * Depth, 1),
				Vector4(0, 0, -NearZ * FarZ * Depth, 0));
		}
		Matrix4x4 Matrix4x4::CreatePerspective(float FieldOfView, float AspectRatio, float NearZ, float FarZ)
		{
			return CreatePerspectiveRad(Mathf::Deg2Rad() * FieldOfView, AspectRatio, NearZ, FarZ);
		}
		Matrix4x4 Matrix4x4::CreateOrthographic(float Width, float Height, float NearZ, float FarZ)
		{
			if (Geometric::IsLeftHanded())
			{
				float Depth = 1.0f / (FarZ - NearZ);
				return Matrix4x4(
					Vector4(2 / Width, 0, 0, 0),
					Vector4(0, 2 / Height, 0, 0),
					Vector4(0, 0, Depth, 0),
					Vector4(0, 0, -Depth * NearZ, 1));
			}
			else
			{
				float Depth = 1.0f / (NearZ - FarZ);
				return Matrix4x4(
					Vector4(2 / Width, 0, 0, 0),
					Vector4(0, 2 / Height, 0, 0),
					Vector4(0, 0, Depth, 0),
					Vector4(0, 0, Depth * NearZ, 1));
			}
		}
		Matrix4x4 Matrix4x4::CreateOrthographicOffCenter(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ)
		{
			float Width = 1.0f / (Right - Left);
			float Height = 1.0f / (Top - Bottom);
			float Depth = 1.0f / (FarZ - NearZ);

			return Matrix4x4(
				Vector4(2 * Width, 0, 0, 0),
				Vector4(0, 2 * Height, 0, 0),
				Vector4(0, 0, Depth, 0),
				Vector4(-(Left + Right) * Width, -(Top + Bottom) * Height, -Depth * NearZ, 1));
		}
		Matrix4x4 Matrix4x4::Create(const Vector3& Position, const Vector3& Scale, const Vector3& Rotation)
		{
			return Matrix4x4::CreateScale(Scale) * Matrix4x4::Create(Position, Rotation);
		}
		Matrix4x4 Matrix4x4::Create(const Vector3& Position, const Vector3& Rotation)
		{
			return Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		Matrix4x4 Matrix4x4::CreateView(const Vector3& Position, const Vector3& Rotation)
		{
			return
				Matrix4x4::CreateTranslation(-Position) *
				Matrix4x4::CreateRotationY(Rotation.Y) *
				Matrix4x4::CreateRotationX(-Rotation.X) *
				Matrix4x4::CreateScale(Vector3(-1.0f, 1.0f, 1.0f)) *
				Matrix4x4::CreateRotationZ(Rotation.Z);
		}
		Matrix4x4 Matrix4x4::CreateLookAt(const Vector3& Position, const Vector3& Target, const Vector3& Up)
		{
			Vector3 Z = (Target - Position).Normalize();
			Vector3 X = Up.Cross(Z).Normalize();
			Vector3 Y = Z.Cross(X);

			Matrix4x4 Result(true);
			Result.Row[0] = X.X;
			Result.Row[1] = Y.X;
			Result.Row[2] = Z.X;
			Result.Row[3] = 0;
			Result.Row[4] = X.Y;
			Result.Row[5] = Y.Y;
			Result.Row[6] = Z.Y;
			Result.Row[7] = 0;
			Result.Row[8] = X.Z;
			Result.Row[9] = Y.Z;
			Result.Row[10] = Z.Z;
			Result.Row[11] = 0;
			Result.Row[12] = -X.Dot(Position);
			Result.Row[13] = -Y.Dot(Position);
			Result.Row[14] = -Z.Dot(Position);
			Result.Row[15] = 1;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreateRotation(const Vector3& Forward, const Vector3& Up, const Vector3& Right)
		{
			Matrix4x4 Rotation(true);
			Rotation.Row[0] = Right.X;
			Rotation.Row[1] = Right.Y;
			Rotation.Row[2] = Right.Z;
			Rotation.Row[3] = 0;
			Rotation.Row[4] = Up.X;
			Rotation.Row[5] = Up.Y;
			Rotation.Row[6] = Up.Z;
			Rotation.Row[7] = 0;
			Rotation.Row[8] = Forward.X;
			Rotation.Row[9] = Forward.Y;
			Rotation.Row[10] = Forward.Z;
			Rotation.Row[11] = 0;
			Rotation.Row[12] = 0;
			Rotation.Row[13] = 0;
			Rotation.Row[14] = 0;
			Rotation.Row[15] = 1;

			return Rotation;
		}
		Matrix4x4 Matrix4x4::CreateLookAt(CubeFace Face, const Vector3& Position)
		{
			switch (Face)
			{
				case CubeFace::PositiveX:
					return Matrix4x4::CreateLookAt(Position, Position + Vector3(1, 0, 0), Vector3::Up());
				case CubeFace::NegativeX:
					return Matrix4x4::CreateLookAt(Position, Position - Vector3(1, 0, 0), Vector3::Up());
				case CubeFace::PositiveY:
					if (Geometric::IsLeftHanded())
						return Matrix4x4::CreateLookAt(Position, Position + Vector3(0, 1, 0), Vector3::Backward());
					else
						return Matrix4x4::CreateLookAt(Position, Position - Vector3(0, 1, 0), Vector3::Forward());
				case CubeFace::NegativeY:
					if (Geometric::IsLeftHanded())
						return Matrix4x4::CreateLookAt(Position, Position - Vector3(0, 1, 0), Vector3::Forward());
					else
						return Matrix4x4::CreateLookAt(Position, Position + Vector3(0, 1, 0), Vector3::Backward());
				case CubeFace::PositiveZ:
					return Matrix4x4::CreateLookAt(Position, Position + Vector3(0, 0, 1), Vector3::Up());
				case CubeFace::NegativeZ:
					return Matrix4x4::CreateLookAt(Position, Position - Vector3(0, 0, 1), Vector3::Up());
				default:
					return Matrix4x4::Identity();
			}
		}

		Quaternion::Quaternion() noexcept : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f)
		{
		}
		Quaternion::Quaternion(float x, float y, float z, float w) noexcept : X(x), Y(y), Z(z), W(w)
		{
		}
		Quaternion::Quaternion(const Quaternion& In) noexcept : X(In.X), Y(In.Y), Z(In.Z), W(In.W)
		{
		}
		Quaternion::Quaternion(const Vector3& Axis, float Angle) noexcept
		{
			SetAxis(Axis, Angle);
		}
		Quaternion::Quaternion(const Vector3& Euler) noexcept
		{
			SetEuler(Euler);
		}
		Quaternion::Quaternion(const Matrix4x4& Value) noexcept
		{
			SetMatrix(Value);
		}
		void Quaternion::SetAxis(const Vector3& Axis, float Angle)
		{
#ifdef VI_SIMD
			LOD_V3(_r1, Axis);
			_r1 *= std::sin(Angle / 2);
			_r1.insert(3, std::cos(Angle / 2));
			_r1.store((float*)this);
#else
			float Sin = std::sin(Angle / 2);
			X = Axis.X * Sin;
			Y = Axis.Y * Sin;
			Z = Axis.Z * Sin;
			W = std::cos(Angle / 2);
#endif
		}
		void Quaternion::SetEuler(const Vector3& V)
		{
#ifdef VI_SIMD
			float _sx[4], _cx[4];
			LOD_V3(_r1, V);
			LOD_VAL(_r2, 0.0f);
			_r1 *= 0.5f;
			_r2 = cos(_r1);
			_r1 = sin(_r1);
			_r1.store(_sx);
			_r2.store(_cx);

			LOD_AV4(_r3, _sx[0], _cx[0], _sx[0], _cx[0]);
			LOD_AV4(_r4, _cx[1], _sx[1], _sx[1], _cx[1]);
			LOD_AV4(_r5, 1.0f, -1.0f, 1.0f, -1.0f);
			_r3 *= _r4;
			_r1 = _r3 * _cx[2];
			_r2 = _r3 * _sx[2];
			_r2 = permute4f<1, 0, 3, 2>(_r2);
			_r1 += _r2 * _r5;
			_r1.store((float*)this);
#else
			float SinX = std::sin(V.X / 2);
			float CosX = std::cos(V.X / 2);
			float SinY = std::sin(V.Y / 2);
			float CosY = std::cos(V.Y / 2);
			float SinZ = std::sin(V.Z / 2);
			float CosZ = std::cos(V.Z / 2);
			X = SinX * CosY;
			Y = CosX * SinY;
			Z = SinX * SinY;
			W = CosX * CosY;

			float fX = X * CosZ + Y * SinZ;
			float fY = Y * CosZ - X * SinZ;
			float fZ = Z * CosZ + W * SinZ;
			float fW = W * CosZ - Z * SinZ;
			X = fX;
			Y = fY;
			Z = fZ;
			W = fW;
#endif
		}
		void Quaternion::SetMatrix(const Matrix4x4& Value)
		{
			float T = Value.Row[0] + Value.Row[5] + Value.Row[10];
			if (T > 0.0f)
			{
				float S = std::sqrt(1 + T) * 2.0f;
				X = (Value.Row[9] - Value.Row[6]) / S;
				Y = (Value.Row[2] - Value.Row[8]) / S;
				Z = (Value.Row[4] - Value.Row[1]) / S;
				W = 0.25f * S;
			}
			else if (Value.Row[0] > Value.Row[5] && Value.Row[0] > Value.Row[10])
			{
				float S = std::sqrt(1.0f + Value.Row[0] - Value.Row[5] - Value.Row[10]) * 2.0f;
				X = 0.25f * S;
				Y = (Value.Row[4] + Value.Row[1]) / S;
				Z = (Value.Row[2] + Value.Row[8]) / S;
				W = (Value.Row[9] - Value.Row[6]) / S;
			}
			else if (Value.Row[5] > Value.Row[10])
			{
				float S = std::sqrt(1.0f + Value.Row[5] - Value.Row[0] - Value.Row[10]) * 2.0f;
				X = (Value.Row[4] + Value.Row[1]) / S;
				Y = 0.25f * S;
				Z = (Value.Row[9] + Value.Row[6]) / S;
				W = (Value.Row[2] - Value.Row[8]) / S;
			}
			else
			{
				float S = std::sqrt(1.0f + Value.Row[10] - Value.Row[0] - Value.Row[5]) * 2.0f;
				X = (Value.Row[2] + Value.Row[8]) / S;
				Y = (Value.Row[9] + Value.Row[6]) / S;
				Z = 0.25f * S;
				W = (Value.Row[4] - Value.Row[1]) / S;
			}
		}
		void Quaternion::Set(const Quaternion& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
			W = Value.W;
		}
		Quaternion Quaternion::operator *(float R) const
		{
			return Mul(R);
		}
		Vector3 Quaternion::operator *(const Vector3& R) const
		{
			return Mul(R);
		}
		Quaternion Quaternion::operator *(const Quaternion& R) const
		{
			return Mul(R);
		}
		Quaternion Quaternion::operator -(const Quaternion& R) const
		{
			return Sub(R);
		}
		Quaternion Quaternion::operator +(const Quaternion& R) const
		{
			return Add(R);
		}
		Quaternion& Quaternion::operator =(const Quaternion& R) noexcept
		{
			this->X = R.X;
			this->Y = R.Y;
			this->Z = R.Z;
			this->W = R.W;
			return *this;
		}
		Quaternion Quaternion::Normalize() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			_r1 /= Geometric::FastSqrt(horizontal_add(square(_r1)));

			Quaternion Result;
			_r1.store((float*)&Result);
			return Result;
#else
			float F = Length();
			return Quaternion(X / F, Y / F, Z / F, W / F);
#endif
		}
		Quaternion Quaternion::sNormalize() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			float F = Geometric::FastSqrt(horizontal_add(square(_r1)));
			if (F == 0.0f)
				return Quaternion();

			Quaternion Result;
			_r1 /= F;
			_r1.store((float*)&Result);
			return Result;
#else
			float F = Length();
			if (F == 0.0f)
				return Quaternion();

			return Quaternion(X / F, Y / F, Z / F, W / F);
#endif
		}
		Quaternion Quaternion::Conjugate() const
		{
			return Quaternion(-X, -Y, -Z, W);
		}
		Quaternion Quaternion::Mul(float R) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			_r1 *= R;

			Quaternion Result;
			_r1.store((float*)&Result);
			return Result;
#else
			return Quaternion(X * R, Y * R, Z * R, W * R);
#endif
		}
		Quaternion Quaternion::Mul(const Quaternion& R) const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, W, -X, -Y, -Z);
			LOD_AV4(_r2, X, W, Y, -Z);
			LOD_AV4(_r3, Y, W, Z, -X);
			LOD_AV4(_r4, Z, W, X, -Y);
			LOD_AV4(_r5, R.W, R.X, R.Y, R.Z);
			LOD_AV4(_r6, R.W, R.X, R.Z, R.Y);
			LOD_AV4(_r7, R.W, R.Y, R.X, R.Z);
			LOD_AV4(_r8, R.W, R.Z, R.Y, R.X);
			float W1 = horizontal_add(_r1 * _r5);
			float X1 = horizontal_add(_r2 * _r6);
			float Y1 = horizontal_add(_r3 * _r7);
			float Z1 = horizontal_add(_r4 * _r8);
#else
			float W1 = W * R.W - X * R.X - Y * R.Y - Z * R.Z;
			float X1 = X * R.W + W * R.X + Y * R.Z - Z * R.Y;
			float Y1 = Y * R.W + W * R.Y + Z * R.X - X * R.Z;
			float Z1 = Z * R.W + W * R.Z + X * R.Y - Y * R.X;
#endif
			return Quaternion(X1, Y1, Z1, W1);
		}
		Vector3 Quaternion::Mul(const Vector3& R) const
		{
			Vector3 UV0(X, Y, Z), UV1, UV2;
			UV1 = UV0.Cross(R);
			UV2 = UV0.Cross(UV1);
			UV1 *= (2.0f * W);
			UV2 *= 2.0f;

			return R + UV1 + UV2;
		}
		Quaternion Quaternion::Sub(const Quaternion& R) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			LOD_V4(_r2, R);
			_r1 -= _r2;

			Quaternion Result;
			_r1.store((float*)&Result);
			return Result;
#else
			return Quaternion(X - R.X, Y - R.Y, Z - R.Z, W - R.W);
#endif
		}
		Quaternion Quaternion::Add(const Quaternion& R) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			LOD_V4(_r2, R);
			_r1 += _r2;

			Quaternion Result;
			_r1.store((float*)&Result);
			return Result;
#else
			return Quaternion(X + R.X, Y + R.Y, Z + R.Z, W + R.W);
#endif
		}
		Quaternion Quaternion::Lerp(const Quaternion& B, float DeltaTime) const
		{
			Quaternion Correction = B;
			if (Dot(B) < 0.0f)
				Correction = Quaternion(-B.X, -B.Y, -B.Z, -B.W);

			return (Correction - *this) * DeltaTime + Normalize();
		}
		Quaternion Quaternion::sLerp(const Quaternion& B, float DeltaTime) const
		{
			Quaternion Correction = B;
			float Cos = Dot(B);

			if (Cos < 0.0f)
			{
				Correction = Quaternion(-B.X, -B.Y, -B.Z, -B.W);
				Cos = -Cos;
			}

			if (std::abs(Cos) >= 1.0f - 1e3f)
				return Lerp(Correction, DeltaTime);

			float Sin = Geometric::FastSqrt(1.0f - Cos * Cos);
			float Angle = std::atan2(Sin, Cos);
			float InvedSin = 1.0f / Sin;
			float Source = std::sin(Angle - DeltaTime * Angle) * InvedSin;
			float Destination = std::sin(DeltaTime * Angle) * InvedSin;

			return Mul(Source).Add(Correction.Mul(Destination));
		}
		Quaternion Quaternion::CreateEulerRotation(const Vector3& V)
		{
			Quaternion Result;
			Result.SetEuler(V);
			return Result;
		}
		Quaternion Quaternion::CreateRotation(const Matrix4x4& V)
		{
			Quaternion Result;
			Result.SetMatrix(V);
			return Result;
		}
		Vector3 Quaternion::Forward() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, X, -W, Y, W);
			LOD_AV4(_r2, Z, Y, Z, X);
			LOD_AV2(_r3, X, Y);
			_r1 *= _r2;
			_r2 = permute4f<-1, -1, 2, 3>(_r1);
			_r1 = permute4f<0, 1, -1, -1>(_r1);
			_r3 = square(_r3);

			Vector3 Result(horizontal_add(_r1), horizontal_add(_r2), horizontal_add(_r3));
			Result *= 2.0f;
			Result.Z = 1.0f - Result.Z;
			return Result;
#else
			return Vector3(
				2.0f * (X * Z - W * Y),
				2.0f * (Y * Z + W * X),
				1.0f - 2.0f * (X * X + Y * Y));
#endif
		}
		Vector3 Quaternion::Up() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, X, W, Y, -W);
			LOD_AV4(_r2, Y, Z, Z, X);
			LOD_AV2(_r3, X, Z);
			_r1 *= _r2;
			_r2 = permute4f<-1, -1, 2, 3>(_r1);
			_r1 = permute4f<0, 1, -1, -1>(_r1);
			_r3 = square(_r3);

			Vector3 Result(horizontal_add(_r1), horizontal_add(_r3), horizontal_add(_r2));
			Result *= 2.0f;
			Result.Y = 1.0f - Result.Y;
			return Result;
#else
			return Vector3(
				2.0f * (X * Y + W * Z),
				1.0f - 2.0f * (X * X + Z * Z),
				2.0f * (Y * Z - W * X));
#endif
		}
		Vector3 Quaternion::Right() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, X, -W, X, W);
			LOD_AV4(_r2, Y, Z, Z, Y);
			LOD_AV2(_r3, Y, Z);
			_r1 *= _r2;
			_r2 = permute4f<-1, -1, 2, 3>(_r1);
			_r1 = permute4f<0, 1, -1, -1>(_r1);
			_r3 = square(_r3);

			Vector3 Result(horizontal_add(_r3), horizontal_add(_r1), horizontal_add(_r2));
			Result *= 2.0f;
			Result.X = 1.0f - Result.X;
			return Result;
#else
			return Vector3(
				1.0f - 2.0f * (Y * Y + Z * Z),
				2.0f * (X * Y - W * Z),
				2.0f * (X * Z + W * Y));
#endif
		}
		Matrix4x4 Quaternion::GetMatrix() const
		{
			Matrix4x4 Result =
			{
				1.0f - 2.0f * (Y * Y + Z * Z), 2.0f * (X * Y + Z * W), 2.0f * (X * Z - Y * W), 0.0f,
				2.0f * (X * Y - Z * W), 1.0f - 2.0f * (X * X + Z * Z), 2.0f * (Y * Z + X * W), 0.0f,
				2.0f * (X * Z + Y * W), 2.0f * (Y * Z - X * W), 1.0f - 2.0f * (X * X + Y * Y), 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};

			return Result;
		}
		Vector3 Quaternion::GetEuler() const
		{
#ifdef VI_SIMD
			LOD_AV4(_r1, W, Y, W, -Z);
			LOD_AV4(_r2, X, Z, Y, X);
			LOD_FV3(_r3);
			LOD_AV2(_r4, W, Z);
			LOD_AV2(_r5, X, Y);
			float XYZW[4];
			_r1 *= _r2;
			_r4 *= _r5;
			_r2 = permute4f<-1, -1, 2, 3>(_r1);
			_r1 = permute4f<0, 1, -1, -1>(_r1);
			_r3 = square(_r3);
			_r3.store(XYZW);

			float T0 = +2.0f * horizontal_add(_r1);
			float T1 = +1.0f - 2.0f * (XYZW[0] + XYZW[1]);
			float Roll = Mathf::Atan2(T0, T1);

			float T2 = +2.0f * horizontal_add(_r2);
			T2 = ((T2 > 1.0f) ? 1.0f : T2);
			T2 = ((T2 < -1.0f) ? -1.0f : T2);
			float Pitch = Mathf::Asin(T2);

			float T3 = +2.0f * horizontal_add(_r4);
			float T4 = +1.0f - 2.0f * (XYZW[1] + XYZW[2]);
			float Yaw = Mathf::Atan2(T3, T4);

			return Vector3(Roll, Pitch, Yaw);
#else
			float Y2 = Y * Y;
			float T0 = +2.0f * (W * X + Y * Z);
			float T1 = +1.0f - 2.0f * (X * X + Y2);
			float Roll = Mathf::Atan2(T0, T1);

			float T2 = +2.0f * (W * Y - Z * X);
			T2 = ((T2 > 1.0f) ? 1.0f : T2);
			T2 = ((T2 < -1.0f) ? -1.0f : T2);
			float Pitch = Mathf::Asin(T2);

			float T3 = +2.0f * (W * Z + X * Y);
			float T4 = +1.0f - 2.0f * (Y2 + Z * Z);
			float Yaw = Mathf::Atan2(T3, T4);

			return Vector3(Roll, Pitch, Yaw);
#endif
		}
		float Quaternion::Dot(const Quaternion& R) const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			LOD_V4(_r2, R);
			_r1 *= _r2;
			return horizontal_add(_r1);
#else
			return X * R.X + Y * R.Y + Z * R.Z + W * R.W;
#endif
		}
		float Quaternion::Length() const
		{
#ifdef VI_SIMD
			LOD_FV4(_r1);
			return std::sqrt(horizontal_add(square(_r1)));
#else
			return std::sqrt(X * X + Y * Y + Z * Z + W * W);
#endif
		}
		bool Quaternion::operator ==(const Quaternion& V) const
		{
			return X == V.X && Y == V.Y && Z == V.Z && W == V.W;
		}
		bool Quaternion::operator !=(const Quaternion& V) const
		{
			return !(*this == V);
		}

		RandomVector2::RandomVector2() noexcept : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomVector2::RandomVector2(const Vector2& MinV, const Vector2& MaxV, bool IntensityV, float AccuracyV) noexcept : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
		{
		}
		Vector2 RandomVector2::Generate()
		{
			Vector2 fMin = Min * Accuracy;
			Vector2 fMax = Max * Accuracy;
			float InvAccuracy = 1.0f / Accuracy;
			if (Intensity)
				InvAccuracy *= Mathf::Random();

			return Vector2(
				Mathf::Random(fMin.X, fMax.X),
				Mathf::Random(fMin.Y, fMax.Y)) * InvAccuracy;
		}

		RandomVector3::RandomVector3() noexcept : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomVector3::RandomVector3(const Vector3& MinV, const Vector3& MaxV, bool IntensityV, float AccuracyV) noexcept : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
		{
		}
		Vector3 RandomVector3::Generate()
		{
			Vector3 fMin = Min * Accuracy;
			Vector3 fMax = Max * Accuracy;
			float InvAccuracy = 1.0f / Accuracy;
			if (Intensity)
				InvAccuracy *= Mathf::Random();

			return Vector3(
				Mathf::Random(fMin.X, fMax.X),
				Mathf::Random(fMin.Y, fMax.Y),
				Mathf::Random(fMin.Z, fMax.Z)) * InvAccuracy;
		}

		RandomVector4::RandomVector4() noexcept : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomVector4::RandomVector4(const Vector4& MinV, const Vector4& MaxV, bool IntensityV, float AccuracyV) noexcept : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
		{
		}
		Vector4 RandomVector4::Generate()
		{
			Vector4 fMin = Min * Accuracy;
			Vector4 fMax = Max * Accuracy;
			float InvAccuracy = 1.0f / Accuracy;
			if (Intensity)
				InvAccuracy *= Mathf::Random();

			return Vector4(
				Mathf::Random(fMin.X, fMax.X),
				Mathf::Random(fMin.Y, fMax.Y),
				Mathf::Random(fMin.Z, fMax.Z),
				Mathf::Random(fMin.W, fMax.W)) * InvAccuracy;
		}

		RandomFloat::RandomFloat() noexcept : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomFloat::RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV) noexcept : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
		{
		}
		float RandomFloat::Generate()
		{
			return (Mathf::Random(Min * Accuracy, Max * Accuracy) / Accuracy) * (Intensity ? Mathf::Random() : 1);
		}

		RegexSource::RegexSource() noexcept :
			MaxBranches(128), MaxBrackets(128), MaxMatches(128),
			State(RegexState::No_Match), IgnoreCase(false)
		{
		}
		RegexSource::RegexSource(const Core::String& Regexp, bool fIgnoreCase, int64_t fMaxMatches, int64_t fMaxBranches, int64_t fMaxBrackets) noexcept :
			Expression(Regexp),
			MaxBranches(fMaxBranches >= 1 ? fMaxBranches : 128),
			MaxBrackets(fMaxBrackets >= 1 ? fMaxBrackets : 128),
			MaxMatches(fMaxMatches >= 1 ? fMaxMatches : 128),
			State(RegexState::Preprocessed), IgnoreCase(fIgnoreCase)
		{
			Compile();
		}
		RegexSource::RegexSource(const RegexSource& Other) noexcept :
			Expression(Other.Expression),
			MaxBranches(Other.MaxBranches),
			MaxBrackets(Other.MaxBrackets),
			MaxMatches(Other.MaxMatches),
			State(Other.State), IgnoreCase(Other.IgnoreCase)
		{
			Compile();
		}
		RegexSource::RegexSource(RegexSource&& Other) noexcept :
			Expression(std::move(Other.Expression)),
			MaxBranches(Other.MaxBranches),
			MaxBrackets(Other.MaxBrackets),
			MaxMatches(Other.MaxMatches),
			State(Other.State), IgnoreCase(Other.IgnoreCase)
		{
			Brackets.reserve(Other.Brackets.capacity());
			Branches.reserve(Other.Branches.capacity());
			Compile();
		}
		RegexSource& RegexSource::operator=(const RegexSource& V) noexcept
		{
			VI_ASSERT(this != &V, "cannot set to self");
			Brackets.clear();
			Brackets.reserve(V.Brackets.capacity());
			Branches.clear();
			Branches.reserve(V.Branches.capacity());
			Expression = V.Expression;
			IgnoreCase = V.IgnoreCase;
			State = V.State;
			MaxBrackets = V.MaxBrackets;
			MaxBranches = V.MaxBranches;
			MaxMatches = V.MaxMatches;
			Compile();

			return *this;
		}
		RegexSource& RegexSource::operator=(RegexSource&& V) noexcept
		{
			VI_ASSERT(this != &V, "cannot set to self");
			Brackets.clear();
			Brackets.reserve(V.Brackets.capacity());
			Branches.clear();
			Branches.reserve(V.Branches.capacity());
			Expression = std::move(V.Expression);
			IgnoreCase = V.IgnoreCase;
			State = V.State;
			MaxBrackets = V.MaxBrackets;
			MaxBranches = V.MaxBranches;
			MaxMatches = V.MaxMatches;
			Compile();

			return *this;
		}
		const Core::String& RegexSource::GetRegex() const
		{
			return Expression;
		}
		int64_t RegexSource::GetMaxBranches() const
		{
			return MaxBranches;
		}
		int64_t RegexSource::GetMaxBrackets() const
		{
			return MaxBrackets;
		}
		int64_t RegexSource::GetMaxMatches() const
		{
			return MaxMatches;
		}
		int64_t RegexSource::GetComplexity() const
		{
			int64_t A = (int64_t)Branches.size() + 1;
			int64_t B = (int64_t)Brackets.size();
			int64_t C = 0;

			for (size_t i = 0; i < (size_t)B; i++)
				C += Brackets[i].Length;

			return (int64_t)Expression.size() + A * B * C;
		}
		RegexState RegexSource::GetState() const
		{
			return State;
		}
		bool RegexSource::IsSimple() const
		{
			return !Core::Stringify::FindOf(Expression, "\\+*?|[]").Found;
		}
		void RegexSource::Compile()
		{
			const char* vPtr = Expression.c_str();
			int64_t vSize = (int64_t)Expression.size();
			Brackets.reserve(8);
			Branches.reserve(8);

			RegexBracket Bracket;
			Bracket.Pointer = vPtr;
			Bracket.Length = vSize;
			Brackets.push_back(Bracket);

			int64_t Step = 0, Depth = 0;
			for (int64_t i = 0; i < vSize; i += Step)
			{
				Step = Regex::GetOpLength(vPtr + i, vSize - i);
				if (vPtr[i] == '|')
				{
					RegexBranch Branch;
					Branch.BracketIndex = (Brackets.back().Length == -1 ? Brackets.size() - 1 : Depth);
					Branch.Pointer = &vPtr[i];
					Branches.push_back(Branch);
				}
				else if (vPtr[i] == '\\')
				{
					REGEX_FAIL_IN(i >= vSize - 1, RegexState::Invalid_Metacharacter);
					if (vPtr[i + 1] == 'x')
					{
						REGEX_FAIL_IN(i >= vSize - 3, RegexState::Invalid_Metacharacter);
						REGEX_FAIL_IN(!(isxdigit(vPtr[i + 2]) && isxdigit(vPtr[i + 3])), RegexState::Invalid_Metacharacter);
					}
					else
					{
						REGEX_FAIL_IN(!Regex::Meta((const unsigned char*)vPtr + i + 1), RegexState::Invalid_Metacharacter);
					}
				}
				else if (vPtr[i] == '(')
				{
					Depth++;
					Bracket.Pointer = vPtr + i + 1;
					Bracket.Length = -1;
					Brackets.push_back(Bracket);
				}
				else if (vPtr[i] == ')')
				{
					int64_t Idx = (Brackets[Brackets.size() - 1].Length == -1 ? Brackets.size() - 1 : Depth);
					Brackets[(size_t)Idx].Length = (int64_t)(&vPtr[i] - Brackets[(size_t)Idx].Pointer); Depth--;
					REGEX_FAIL_IN(Depth < 0, RegexState::Unbalanced_Brackets);
					REGEX_FAIL_IN(i > 0 && vPtr[i - 1] == '(', RegexState::No_Match);
				}
			}

			REGEX_FAIL_IN(Depth != 0, RegexState::Unbalanced_Brackets);

			RegexBranch Branch; size_t i, j;
			for (i = 0; i < Branches.size(); i++)
			{
				for (j = i + 1; j < Branches.size(); j++)
				{
					if (Branches[i].BracketIndex > Branches[j].BracketIndex)
					{
						Branch = Branches[i];
						Branches[i] = Branches[j];
						Branches[j] = Branch;
					}
				}
			}

			for (i = j = 0; i < Brackets.size(); i++)
			{
				auto& Bracket = Brackets[i];
				Bracket.BranchesCount = 0;
				Bracket.Branches = j;

				while (j < Branches.size() && Branches[j].BracketIndex == i)
				{
					Bracket.BranchesCount++;
					j++;
				}
			}
		}

		RegexResult::RegexResult() noexcept : State(RegexState::No_Match), Steps(0), Src(nullptr)
		{
		}
		RegexResult::RegexResult(const RegexResult& Other) noexcept : Matches(Other.Matches), State(Other.State), Steps(Other.Steps), Src(Other.Src)
		{
		}
		RegexResult::RegexResult(RegexResult&& Other) noexcept : Matches(std::move(Other.Matches)), State(Other.State), Steps(Other.Steps), Src(Other.Src)
		{
		}
		RegexResult& RegexResult::operator =(const RegexResult& V) noexcept
		{
			Matches = V.Matches;
			Src = V.Src;
			Steps = V.Steps;
			State = V.State;

			return *this;
		}
		RegexResult& RegexResult::operator =(RegexResult&& V) noexcept
		{
			Matches.swap(V.Matches);
			Src = V.Src;
			Steps = V.Steps;
			State = V.State;

			return *this;
		}
		bool RegexResult::Empty() const
		{
			return Matches.empty();
		}
		int64_t RegexResult::GetSteps() const
		{
			return Steps;
		}
		RegexState RegexResult::GetState() const
		{
			return State;
		}
		const Core::Vector<RegexMatch>& RegexResult::Get() const
		{
			return Matches;
		}
		Core::Vector<Core::String> RegexResult::ToArray() const
		{
			Core::Vector<Core::String> Array;
			Array.reserve(Matches.size());

			for (auto& Item : Matches)
				Array.emplace_back(Item.Pointer, (size_t)Item.Length);

			return Array;
		}

		bool Regex::Match(RegexSource* Value, RegexResult& Result, const Core::String& Buffer)
		{
			return Match(Value, Result, Buffer.c_str(), Buffer.size());
		}
		bool Regex::Match(RegexSource* Value, RegexResult& Result, const char* Buffer, int64_t Length)
		{
			VI_ASSERT(Value != nullptr && Value->State == RegexState::Preprocessed, "invalid regex source");
			VI_ASSERT(Buffer != nullptr && Length > 0, "invalid buffer");

			Result.Src = Value;
			Result.State = RegexState::Preprocessed;
			Result.Matches.clear();
			Result.Matches.reserve(8);

			int64_t Code = Parse(Buffer, Length, &Result);
			if (Code <= 0)
			{
				Result.State = (RegexState)Code;
				Result.Matches.clear();
				return false;
			}

			for (auto It = Result.Matches.begin(); It != Result.Matches.end(); ++It)
			{
				It->Start = It->Pointer - Buffer;
				It->End = It->Start + It->Length;
			}

			Result.State = RegexState::Match_Found;
			return true;
		}
		bool Regex::Replace(RegexSource* Value, const Core::String& To, Core::String& Buffer)
		{
			Core::String Emplace;
			RegexResult Result;
			size_t Matches = 0;

			bool Expression = (!To.empty() && To.find('$') != Core::String::npos);
			if (!Expression)
				Emplace.assign(To);

			size_t Start = 0;
			while (Match(Value, Result, Buffer.c_str() + Start, Buffer.size() - Start))
			{
				Matches++;
				if (Result.Matches.empty())
					continue;

				if (Expression)
				{
					Emplace.assign(To);
					for (size_t i = 1; i < Result.Matches.size(); i++)
					{
						auto& Item = Result.Matches[i];
						Core::Stringify::Replace(Emplace, "$" + Core::ToString(i), Core::String(Item.Pointer, (size_t)Item.Length));
					}
				}

				auto& Where = Result.Matches.front();
				Core::Stringify::ReplacePart(Buffer, (size_t)Where.Start + Start, (size_t)Where.End + Start, Emplace);
				Start += (size_t)Where.Start + (size_t)Emplace.size() - (Emplace.empty() ? 0 : 1);
			}

			return Matches > 0;
		}
		int64_t Regex::Meta(const unsigned char* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			static const char* Chars = "^$().[]*+?|\\Ssdbfnrtv";
			return strchr(Chars, *Buffer) != nullptr;
		}
		int64_t Regex::OpLength(const char* Value)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			return Value[0] == '\\' && Value[1] == 'x' ? 4 : Value[0] == '\\' ? 2 : 1;
		}
		int64_t Regex::SetLength(const char* Value, int64_t ValueLength)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			int64_t Length = 0;
			while (Length < ValueLength && Value[Length] != ']')
				Length += OpLength(Value + Length);

			return Length <= ValueLength ? Length + 1 : -1;
		}
		int64_t Regex::GetOpLength(const char* Value, int64_t ValueLength)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			return Value[0] == '[' ? SetLength(Value + 1, ValueLength - 1) + 1 : OpLength(Value);
		}
		int64_t Regex::Quantifier(const char* Value)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			return Value[0] == '*' || Value[0] == '+' || Value[0] == '?';
		}
		int64_t Regex::ToInt(int64_t x)
		{
			return (int64_t)(isdigit((int)x) ? (int)x - '0' : (int)x - 'W');
		}
		int64_t Regex::HexToInt(const unsigned char* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			return (ToInt(tolower(Buffer[0])) << 4) | ToInt(tolower(Buffer[1]));
		}
		int64_t Regex::MatchOp(const unsigned char* Value, const unsigned char* Buffer, RegexResult* Info)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Value != nullptr, "invalid value");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			int64_t Result = 0;
			switch (*Value)
			{
				case '\\':
					switch (Value[1])
					{
						case 'S':
							REGEX_FAIL(isspace(*Buffer), (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 's':
							REGEX_FAIL(!isspace(*Buffer), (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'd':
							REGEX_FAIL(!isdigit(*Buffer), (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'b':
							REGEX_FAIL(*Buffer != '\b', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'f':
							REGEX_FAIL(*Buffer != '\f', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'n':
							REGEX_FAIL(*Buffer != '\n', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'r':
							REGEX_FAIL(*Buffer != '\r', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 't':
							REGEX_FAIL(*Buffer != '\t', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'v':
							REGEX_FAIL(*Buffer != '\v', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'x':
							REGEX_FAIL((unsigned char)HexToInt(Value + 2) != *Buffer, (int64_t)RegexState::No_Match);
							Result++;
							break;
						default:
							REGEX_FAIL(Value[1] != Buffer[0], (int64_t)RegexState::No_Match);
							Result++;
							break;
					}
					break;
				case '|':
					REGEX_FAIL(1, (int64_t)RegexState::Internal_Error);
					break;
				case '$':
					REGEX_FAIL(1, (int64_t)RegexState::No_Match);
					break;
				case '.':
					Result++;
					break;
				default:
					if (Info->Src->IgnoreCase)
					{
						REGEX_FAIL(tolower(*Value) != tolower(*Buffer), (int64_t)RegexState::No_Match);
					}
					else
					{
						REGEX_FAIL(*Value != *Buffer, (int64_t)RegexState::No_Match);
					}
					Result++;
					break;
			}

			return Result;
		}
		int64_t Regex::MatchSet(const char* Value, int64_t ValueLength, const char* Buffer, RegexResult* Info)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Value != nullptr, "invalid value");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			int64_t Length = 0, Result = -1, Invert = Value[0] == '^';
			if (Invert)
				Value++, ValueLength--;

			while (Length <= ValueLength && Value[Length] != ']' && Result <= 0)
			{
				if (Value[Length] != '-' && Value[Length + 1] == '-' && Value[Length + 2] != ']' && Value[Length + 2] != '\0')
				{
					Result = (Info->Src->IgnoreCase) ? tolower(*Buffer) >= tolower(Value[Length]) && tolower(*Buffer) <= tolower(Value[Length + 2]) : *Buffer >= Value[Length] && *Buffer <= Value[Length + 2];
					Length += 3;
				}
				else
				{
					Result = MatchOp((const unsigned char*)Value + Length, (const unsigned char*)Buffer, Info);
					Length += OpLength(Value + Length);
				}
			}

			return (!Invert && Result > 0) || (Invert && Result <= 0) ? 1 : -1;
		}
		int64_t Regex::ParseInner(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Value != nullptr, "invalid value");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			int64_t i, j, n, Step;
			for (i = j = 0; i < ValueLength && j <= BufferLength; i += Step)
			{
				Step = Value[i] == '(' ? Info->Src->Brackets[(size_t)Case + 1].Length + 2 : GetOpLength(Value + i, ValueLength - i);
				REGEX_FAIL(Quantifier(&Value[i]), (int64_t)RegexState::Unexpected_Quantifier);
				REGEX_FAIL(Step <= 0, (int64_t)RegexState::Invalid_Character_Set);
				Info->Steps++;

				if (i + Step < ValueLength && Quantifier(Value + i + Step))
				{
					if (Value[i + Step] == '?')
					{
						int64_t result = ParseInner(Value + i, Step, Buffer + j, BufferLength - j, Info, Case);
						j += result > 0 ? result : 0;
						i++;
					}
					else if (Value[i + Step] == '+' || Value[i + Step] == '*')
					{
						int64_t j2 = j, nj = j, n1, n2 = -1, ni, non_greedy = 0;
						ni = i + Step + 1;
						if (ni < ValueLength && Value[ni] == '?')
						{
							non_greedy = 1;
							ni++;
						}

						do
						{
							if ((n1 = ParseInner(Value + i, Step, Buffer + j2, BufferLength - j2, Info, Case)) > 0)
								j2 += n1;

							if (Value[i + Step] == '+' && n1 < 0)
								break;

							if (ni >= ValueLength)
								nj = j2;
							else if ((n2 = ParseInner(Value + ni, ValueLength - ni, Buffer + j2, BufferLength - j2, Info, Case)) >= 0)
								nj = j2 + n2;

							if (nj > j && non_greedy)
								break;
						} while (n1 > 0);

						if (n1 < 0 && n2 < 0 && Value[i + Step] == '*' && (n2 = ParseInner(Value + ni, ValueLength - ni, Buffer + j, BufferLength - j, Info, Case)) > 0)
							nj = j + n2;

						REGEX_FAIL(Value[i + Step] == '+' && nj == j, (int64_t)RegexState::No_Match);
						REGEX_FAIL(nj == j && ni < ValueLength&& n2 < 0, (int64_t)RegexState::No_Match);
						return nj;
					}

					continue;
				}

				if (Value[i] == '[')
				{
					n = MatchSet(Value + i + 1, ValueLength - (i + 2), Buffer + j, Info);
					REGEX_FAIL(n <= 0, (int64_t)RegexState::No_Match);
					j += n;
				}
				else if (Value[i] == '(')
				{
					n = (int64_t)RegexState::No_Match;
					Case++;

					REGEX_FAIL(Case >= (int64_t)Info->Src->Brackets.size(), (int64_t)RegexState::Internal_Error);
					if (ValueLength - (i + Step) > 0)
					{
						int64_t j2;
						for (j2 = 0; j2 <= BufferLength - j; j2++)
						{
							if ((n = ParseDOH(Buffer + j, BufferLength - (j + j2), Info, Case)) >= 0 && ParseInner(Value + i + Step, ValueLength - (i + Step), Buffer + j + n, BufferLength - (j + n), Info, Case) >= 0)
								break;
						}
					}
					else
						n = ParseDOH(Buffer + j, BufferLength - j, Info, Case);

					REGEX_FAIL(n < 0, n);
					if (n > 0)
					{
						RegexMatch* Match;
						if (Case - 1 >= (int64_t)Info->Matches.size())
						{
							Info->Matches.emplace_back();
							Match = &Info->Matches[Info->Matches.size() - 1];
						}
						else
							Match = &Info->Matches.at((size_t)Case - 1);

						Match->Pointer = Buffer + j;
						Match->Length = n;
						Match->Steps++;
					}
					j += n;
				}
				else if (Value[i] == '^')
				{
					REGEX_FAIL(j != 0, (int64_t)RegexState::No_Match);
				}
				else if (Value[i] == '$')
				{
					REGEX_FAIL(j != BufferLength, (int64_t)RegexState::No_Match);
				}
				else
				{
					REGEX_FAIL(j >= BufferLength, (int64_t)RegexState::No_Match);
					n = MatchOp((const unsigned char*)(Value + i), (const unsigned char*)(Buffer + j), Info);
					REGEX_FAIL(n <= 0, n);
					j += n;
				}
			}

			return j;
		}
		int64_t Regex::ParseDOH(const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			const RegexBracket* Bk = &Info->Src->Brackets[(size_t)Case];
			int64_t i = 0, Length, Result;
			const char* Ptr;

			do
			{
				Ptr = i == 0 ? Bk->Pointer : Info->Src->Branches[(size_t)(Bk->Branches + i - 1)].Pointer + 1;
				Length = Bk->BranchesCount == 0 ? Bk->Length : i == Bk->BranchesCount ? (int64_t)(Bk->Pointer + Bk->Length - Ptr) : (int64_t)(Info->Src->Branches[(size_t)(Bk->Branches + i)].Pointer - Ptr);
				Result = ParseInner(Ptr, Length, Buffer, BufferLength, Info, Case);
				VI_MEASURE_LOOP();
			} while (Result <= 0 && i++ < Bk->BranchesCount);

			return Result;
		}
		int64_t Regex::Parse(const char* Buffer, int64_t BufferLength, RegexResult* Info)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Info != nullptr, "invalid regex result");
			VI_MEASURE(Core::Timings::Frame);

			int64_t is_anchored = Info->Src->Brackets[0].Pointer[0] == '^', i, result = -1;
			for (i = 0; i <= BufferLength; i++)
			{
				result = ParseDOH(Buffer + i, BufferLength - i, Info, 0);
				if (result >= 0)
				{
					result += i;
					break;
				}

				if (is_anchored)
					break;
			}

			return result;
		}
		const char* Regex::Syntax()
		{
			return
				"\"^\" - Match beginning of a buffer\n"
				"\"$\" - Match end of a buffer\n"
				"\"()\" - Grouping and substring capturing\n"
				"\"\\s\" - Match whitespace\n"
				"\"\\S\" - Match non - whitespace\n"
				"\"\\d\" - Match decimal digit\n"
				"\"\\n\" - Match new line character\n"
				"\"\\r\" - Match line feed character\n"
				"\"\\f\" - Match form feed character\n"
				"\"\\v\" - Match vertical tab character\n"
				"\"\\t\" - Match horizontal tab character\n"
				"\"\\b\" - Match backspace character\n"
				"\"+\" - Match one or more times (greedy)\n"
				"\"+?\" - Match one or more times (non - greedy)\n"
				"\"*\" - Match zero or more times (greedy)\n"
				"\"*?\" - Match zero or more times(non - greedy)\n"
				"\"?\" - Match zero or once(non - greedy)\n"
				"\"x|y\" - Match x or y(alternation operator)\n"
				"\"\\meta\" - Match one of the meta character: ^$().[]*+?|\\\n"
				"\"\\xHH\" - Match byte with hex value 0xHH, e.g. \\x4a\n"
				"\"[...]\" - Match any character from set. Ranges like[a-z] are supported\n"
				"\"[^...]\" - Match any character but ones from set\n";
		}

		unsigned char AdjTriangle::FindEdge(unsigned int vref0, unsigned int vref1)
		{
			unsigned char EdgeNb = 0xff;
			if (VRef[0] == vref0 && VRef[1] == vref1)
				EdgeNb = 0;
			else if (VRef[0] == vref1 && VRef[1] == vref0)
				EdgeNb = 0;
			else if (VRef[0] == vref0 && VRef[2] == vref1)
				EdgeNb = 1;
			else if (VRef[0] == vref1 && VRef[2] == vref0)
				EdgeNb = 1;
			else if (VRef[1] == vref0 && VRef[2] == vref1)
				EdgeNb = 2;
			else if (VRef[1] == vref1 && VRef[2] == vref0)
				EdgeNb = 2;

			return EdgeNb;
		}
		unsigned int AdjTriangle::OppositeVertex(unsigned int vref0, unsigned int vref1)
		{
			unsigned int Ref = 0xffffffff;
			if (VRef[0] == vref0 && VRef[1] == vref1)
				Ref = VRef[2];
			else if (VRef[0] == vref1 && VRef[1] == vref0)
				Ref = VRef[2];
			else if (VRef[0] == vref0 && VRef[2] == vref1)
				Ref = VRef[1];
			else if (VRef[0] == vref1 && VRef[2] == vref0)
				Ref = VRef[1];
			else if (VRef[1] == vref0 && VRef[2] == vref1)
				Ref = VRef[0];
			else if (VRef[1] == vref1 && VRef[2] == vref0)
				Ref = VRef[0];

			return Ref;
		}

		CollisionBody::CollisionBody(btCollisionObject* Object) noexcept
		{
#ifdef VI_BULLET3
			btRigidBody* RigidObject = btRigidBody::upcast(Object);
			if (RigidObject != nullptr)
				Rigid = (RigidBody*)RigidObject->getUserPointer();

			btSoftBody* SoftObject = btSoftBody::upcast(Object);
			if (SoftObject != nullptr)
				Soft = (SoftBody*)SoftObject->getUserPointer();
#endif
		}

		PrivateKey::PrivateKey() noexcept
		{
			VI_TRACE("[crypto] init empty private key");
		}
		PrivateKey::PrivateKey(Core::String&& Text, bool) noexcept : Plain(std::move(Text))
		{
			VI_TRACE("[crypto] init plain private key on %" PRIu64 " bytes", (uint64_t)Plain.size());
		}
		PrivateKey::PrivateKey(const Core::String& Text, bool) noexcept : Plain(Text)
		{
			VI_TRACE("[crypto] init plain private key on %" PRIu64 " bytes", (uint64_t)Plain.size());
		}
		PrivateKey::PrivateKey(const Core::String& Key) noexcept
		{
			Secure(Key);
		}
		PrivateKey::PrivateKey(const char* Buffer) noexcept
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			Secure(Buffer, strlen(Buffer));
		}
		PrivateKey::PrivateKey(const char* Buffer, size_t Size) noexcept
		{
			Secure(Buffer, Size);
		}
		PrivateKey::PrivateKey(const PrivateKey& Other) noexcept
		{
			CopyDistribution(Other);
		}
		PrivateKey::PrivateKey(PrivateKey&& Other) noexcept : Blocks(std::move(Other.Blocks)), Plain(std::move(Other.Plain))
		{
		}
		PrivateKey::~PrivateKey() noexcept
		{
			Clear();
		}
		PrivateKey& PrivateKey::operator =(const PrivateKey& V) noexcept
		{
			CopyDistribution(V);
			return *this;
		}
		PrivateKey& PrivateKey::operator =(PrivateKey&& V) noexcept
		{
			Clear();
			Blocks = std::move(V.Blocks);
			Plain = std::move(V.Plain);
			return *this;
		}
		void PrivateKey::Clear()
		{
			size_t Size = Blocks.size();
			for (size_t i = 0; i < Size; i++)
			{
				size_t* Partition = (size_t*)Blocks[i];
				RollPartition(Partition, Size, i);
				VI_FREE(Partition);
			}
			Blocks.clear();
			Plain.clear();
		}
		void PrivateKey::Secure(const Core::String& Key)
		{
			Secure(Key.c_str(), Key.size());
		}
		void PrivateKey::Secure(const char* Buffer, size_t Size)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			VI_TRACE("[crypto] secure private key on %" PRIu64 " bytes", (uint64_t)Size);
			Blocks.reserve(Size);
			Clear();

			for (size_t i = 0; i < Size; i++)
			{
				size_t* Partition = VI_MALLOC(size_t, PRIVATE_KEY_SIZE);
				FillPartition(Partition, Size, i, Buffer[i]);
				Blocks.emplace_back(Partition);
			}
		}
		void PrivateKey::ExposeToStack(char* Buffer, size_t MaxSize, size_t* OutSize) const
		{
			VI_TRACE("[crypto] stack expose private key to 0x%" PRIXPTR, (void*)Buffer);
			size_t Size;
			if (Plain.empty())
			{
				Size = (--MaxSize > Blocks.size() ? Blocks.size() : MaxSize);
				for (size_t i = 0; i < Size; i++)
					Buffer[i] = LoadPartition((size_t*)Blocks[i], Size, i);
			}
			else
			{
				Size = (--MaxSize > Plain.size() ? Plain.size() : MaxSize);
				memcpy(Buffer, Plain.data(), sizeof(char) * Size);
			}

			Buffer[Size] = '\0';
			if (OutSize != nullptr)
				*OutSize = Size;
		}
		Core::String PrivateKey::ExposeToHeap() const
		{
			VI_TRACE("[crypto] heap expose private key from 0x%" PRIXPTR, (void*)this);
			Core::String Result = Plain;
			if (Result.empty())
			{
				Result.resize(Blocks.size());
				ExposeToStack((char*)Result.data(), Result.size() + 1);
			}
			return Result;
		}
		void PrivateKey::CopyDistribution(const PrivateKey& Other)
		{
			VI_TRACE("[crypto] copy private key from 0x%" PRIXPTR, (void*)&Other);
			Clear();
			if (Other.Plain.empty())
			{
				Blocks.reserve(Other.Blocks.size());
				for (auto* Partition : Other.Blocks)
				{
					void* CopiedPartition = VI_MALLOC(void, PRIVATE_KEY_SIZE);
					memcpy(CopiedPartition, Partition, PRIVATE_KEY_SIZE);
					Blocks.emplace_back(CopiedPartition);
				}
			}
			else
				Plain = Other.Plain;
		}
		size_t PrivateKey::Size() const
		{
			return Blocks.size();
		}
		char PrivateKey::LoadPartition(size_t* Dest, size_t Size, size_t Index) const
		{
			char* Buffer = (char*)Dest + sizeof(size_t);
			return Buffer[Dest[0]];
		}
		void PrivateKey::RollPartition(size_t* Dest, size_t Size, size_t Index) const
		{
			char* Buffer = (char*)Dest + sizeof(size_t);
			Buffer[Dest[0]] = (char)(Crypto::Random() % std::numeric_limits<unsigned char>::max());
		}
		void PrivateKey::FillPartition(size_t* Dest, size_t Size, size_t Index, char Source) const
		{
			Dest[0] = ((size_t(Source) << 16) + 17) % (PRIVATE_KEY_SIZE - sizeof(size_t));
			char* Buffer = (char*)Dest + sizeof(size_t);
			for (size_t i = 0; i < PRIVATE_KEY_SIZE - sizeof(size_t); i++)
				Buffer[i] = (char)(Crypto::Random() % std::numeric_limits<unsigned char>::max());;
			Buffer[Dest[0]] = Source;
		}
		void PrivateKey::RandomizeBuffer(char* Buffer, size_t Size)
		{
			for (size_t i = 0; i < Size; i++)
				Buffer[i] = Crypto::Random() % std::numeric_limits<char>::max();
		}
		PrivateKey PrivateKey::GetPlain(Core::String&& Value)
		{
			PrivateKey Key = PrivateKey(std::move(Value), true);
			return Key;
		}
		PrivateKey PrivateKey::GetPlain(const Core::String& Value)
		{
			PrivateKey Key = PrivateKey(Value, true);
			return Key;
		}

		UInt128::UInt128(const Core::String& Text) : UInt128(Text, 10)
		{
		}
		UInt128::UInt128(const Core::String& Text, uint8_t Base)
		{
			if (Text.empty())
			{
				Lower = Upper = 0;
				return;
			}

			size_t Size = Text.size();
			char* Data = (char*)Text.c_str();
			while (*Data && Size && std::isspace(*Data))
			{
				++Data;
				Size--;
			}

			switch (Base)
			{
				case 16:
				{
					static const size_t MAX_LEN = 32;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					const size_t double_lower = sizeof(Lower) * 2;
					const size_t lower_len = (max_len >= double_lower) ? double_lower : max_len;
					const size_t upper_len = (max_len >= double_lower) ? (max_len - double_lower) : 0;

					std::stringstream lower_s, upper_s;
					upper_s << std::hex << Core::String(Data + starting_index, upper_len);
					lower_s << std::hex << Core::String(Data + starting_index + upper_len, lower_len);
					upper_s >> Upper;
					lower_s >> Lower;
					break;
				}
				case 10:
				{
					static const size_t MAX_LEN = 39;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					Data += starting_index;

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '9') && (i < max_len); ++Data, ++i)
					{
						*this *= 10;
						*this += *Data - '0';
					}
					break;
				}
				case 8:
				{
					static const size_t MAX_LEN = 43;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					Data += starting_index;

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '7') && (i < max_len); ++Data, ++i)
					{
						*this *= 8;
						*this += *Data - '0';
					}
					break;
				}
				case 2:
				{
					static const size_t MAX_LEN = 128;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					const size_t eight_lower = sizeof(Lower) * 8;
					const size_t lower_len = (max_len >= eight_lower) ? eight_lower : max_len;
					const size_t upper_len = (max_len >= eight_lower) ? (max_len - eight_lower) : 0;
					Data += starting_index;

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '1') && (i < upper_len); ++Data, ++i)
					{
						Upper <<= 1;
						Upper |= *Data - '0';
					}

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '1') && (i < lower_len); ++Data, ++i)
					{
						Lower <<= 1;
						Lower |= *Data - '0';
					}
					break;
				}
				default:
					VI_ASSERT(false, "invalid from string base: %i", (int)Base);
					break;
			}
		}
		UInt128::operator bool() const
		{
			return (bool)(Upper | Lower);
		}
		UInt128::operator uint8_t() const
		{
			return (uint8_t)Lower;
		}
		UInt128::operator uint16_t() const
		{
			return (uint16_t)Lower;
		}
		UInt128::operator uint32_t() const
		{
			return (uint32_t)Lower;
		}
		UInt128::operator uint64_t() const
		{
			return (uint64_t)Lower;
		}
		UInt128 UInt128::operator&(const UInt128& Right) const
		{
			return UInt128(Upper & Right.Upper, Lower & Right.Lower);
		}
		UInt128& UInt128::operator&=(const UInt128& Right)
		{
			Upper &= Right.Upper;
			Lower &= Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator|(const UInt128& Right) const
		{
			return UInt128(Upper | Right.Upper, Lower | Right.Lower);
		}
		UInt128& UInt128::operator|=(const UInt128& Right)
		{
			Upper |= Right.Upper;
			Lower |= Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator^(const UInt128& Right) const
		{
			return UInt128(Upper ^ Right.Upper, Lower ^ Right.Lower);
		}
		UInt128& UInt128::operator^=(const UInt128& Right)
		{
			Upper ^= Right.Upper;
			Lower ^= Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator~() const
		{
			return UInt128(~Upper, ~Lower);
		}
		UInt128 UInt128::operator<<(const UInt128& Right) const
		{
			const uint64_t shift = Right.Lower;
			if (((bool)Right.Upper) || (shift >= 128))
				return UInt128(0);
			else if (shift == 64)
				return UInt128(Lower, 0);
			else if (shift == 0)
				return *this;
			else if (shift < 64)
				return UInt128((Upper << shift) + (Lower >> (64 - shift)), Lower << shift);
			else if ((128 > shift) && (shift > 64))
				return UInt128(Lower << (shift - 64), 0);

			return UInt128(0);
		}
		UInt128& UInt128::operator<<=(const UInt128& Right)
		{
			*this = *this << Right;
			return *this;
		}
		UInt128 UInt128::operator>>(const UInt128& Right) const
		{
			const uint64_t shift = Right.Lower;
			if (((bool)Right.Upper) || (shift >= 128))
				return UInt128(0);
			else if (shift == 64)
				return UInt128(0, Upper);
			else if (shift == 0)
				return *this;
			else if (shift < 64)
				return UInt128(Upper >> shift, (Upper << (64 - shift)) + (Lower >> shift));
			else if ((128 > shift) && (shift > 64))
				return UInt128(0, (Upper >> (shift - 64)));

			return UInt128(0);
		}
		UInt128& UInt128::operator>>=(const UInt128& Right)
		{
			*this = *this >> Right;
			return *this;
		}
		bool UInt128::operator!() const
		{
			return !(bool)(Upper | Lower);
		}
		bool UInt128::operator&&(const UInt128& Right) const
		{
			return ((bool)*this && Right);
		}
		bool UInt128::operator||(const UInt128& Right) const
		{
			return ((bool)*this || Right);
		}
		bool UInt128::operator==(const UInt128& Right) const
		{
			return ((Upper == Right.Upper) && (Lower == Right.Lower));
		}
		bool UInt128::operator!=(const UInt128& Right) const
		{
			return ((Upper != Right.Upper) | (Lower != Right.Lower));
		}
		bool UInt128::operator>(const UInt128& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower > Right.Lower);

			return (Upper > Right.Upper);
		}
		bool UInt128::operator<(const UInt128& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower < Right.Lower);

			return (Upper < Right.Upper);
		}
		bool UInt128::operator>=(const UInt128& Right) const
		{
			return ((*this > Right) | (*this == Right));
		}
		bool UInt128::operator<=(const UInt128& Right) const
		{
			return ((*this < Right) | (*this == Right));
		}
		UInt128 UInt128::operator+(const UInt128& Right) const
		{
			return UInt128(Upper + Right.Upper + ((Lower + Right.Lower) < Lower), Lower + Right.Lower);
		}
		UInt128& UInt128::operator+=(const UInt128& Right)
		{
			Upper += Right.Upper + ((Lower + Right.Lower) < Lower);
			Lower += Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator-(const UInt128& Right) const
		{
			return UInt128(Upper - Right.Upper - ((Lower - Right.Lower) > Lower), Lower - Right.Lower);
		}
		UInt128& UInt128::operator-=(const UInt128& Right)
		{
			*this = *this - Right;
			return *this;
		}
		UInt128 UInt128::operator*(const UInt128& Right) const
		{
			uint64_t top[4] = { Upper >> 32, Upper & 0xffffffff, Lower >> 32, Lower & 0xffffffff };
			uint64_t bottom[4] = { Right.Upper >> 32, Right.Upper & 0xffffffff, Right.Lower >> 32, Right.Lower & 0xffffffff };
			uint64_t products[4][4];

			for (int y = 3; y > -1; y--)
			{
				for (int x = 3; x > -1; x--)
					products[3 - x][y] = top[x] * bottom[y];
			}

			uint64_t fourth32 = (products[0][3] & 0xffffffff);
			uint64_t third32 = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
			uint64_t second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
			uint64_t first32 = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);
			third32 += (products[1][3] & 0xffffffff);
			second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
			first32 += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);
			second32 += (products[2][3] & 0xffffffff);
			first32 += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);
			first32 += (products[3][3] & 0xffffffff);
			third32 += fourth32 >> 32;
			second32 += third32 >> 32;
			first32 += second32 >> 32;
			fourth32 &= 0xffffffff;
			third32 &= 0xffffffff;
			second32 &= 0xffffffff;
			first32 &= 0xffffffff;
			return UInt128((first32 << 32) | second32, (third32 << 32) | fourth32);
		}
		UInt128& UInt128::operator*=(const UInt128& Right)
		{
			*this = *this * Right;
			return *this;
		}
		UInt128 UInt128::Min()
		{
			static UInt128 Value = UInt128(0);
			return Value;
		}
		UInt128 UInt128::Max()
		{
			static UInt128 Value = UInt128(-1, -1);
			return Value;
		}
		std::pair<UInt128, UInt128> UInt128::Divide(const UInt128& Left, const UInt128& Right) const
		{
			UInt128 Zero(0), One(1);
			if (Right == Zero)
			{
				VI_ASSERT(false, "division or modulus by zero");
				return std::pair<UInt128, UInt128>(Zero, Zero);
			}
			else if (Right == One)
				return std::pair <UInt128, UInt128>(Left, Zero);
			else if (Left == Right)
				return std::pair <UInt128, UInt128>(One, Zero);
			else if ((Left == Zero) || (Left < Right))
				return std::pair <UInt128, UInt128>(Zero, Left);

			std::pair <UInt128, UInt128> qr(Zero, Zero);
			for (uint8_t x = Left.Bits(); x > 0; x--)
			{
				qr.first <<= One;
				qr.second <<= One;
				if ((Left >> (x - 1U)) & 1)
					++qr.second;

				if (qr.second >= Right)
				{
					qr.second -= Right;
					++qr.first;
				}
			}
			return qr;
		}
		UInt128 UInt128::operator/(const UInt128& Right) const
		{
			return Divide(*this, Right).first;
		}
		UInt128& UInt128::operator/=(const UInt128& Right)
		{
			*this = *this / Right;
			return *this;
		}
		UInt128 UInt128::operator%(const UInt128& Right) const
		{
			return Divide(*this, Right).second;
		}
		UInt128& UInt128::operator%=(const UInt128& Right)
		{
			*this = *this % Right;
			return *this;
		}
		UInt128& UInt128::operator++()
		{
			return *this += UInt128(1);
		}
		UInt128 UInt128::operator++(int)
		{
			UInt128 temp(*this);
			++*this;
			return temp;
		}
		UInt128& UInt128::operator--()
		{
			return *this -= UInt128(1);
		}
		UInt128 UInt128::operator--(int)
		{
			UInt128 temp(*this);
			--*this;
			return temp;
		}
		UInt128 UInt128::operator+() const
		{
			return *this;
		}
		UInt128 UInt128::operator-() const
		{
			return ~*this + UInt128(1);
		}
		const uint64_t& UInt128::High() const
		{
			return Upper;
		}
		const uint64_t& UInt128::Low() const
		{
			return Lower;
		}
		uint8_t UInt128::Bits() const
		{
			uint8_t out = 0;
			if (Upper)
			{
				out = 64;
				uint64_t up = Upper;
				while (up)
				{
					up >>= 1;
					out++;
				}
			}
			else
			{
				uint64_t low = Lower;
				while (low)
				{
					low >>= 1;
					out++;
				}
			}
			return out;
		}
		Core::Decimal UInt128::ToDecimal() const
		{
			return Core::Decimal(ToString());
		}
		Core::String UInt128::ToString(uint8_t Base, uint32_t Length) const
		{
			VI_ASSERT(Base >= 2 && Base <= 16, "base must be in the range [2, 16]");
			Core::String Output;
			if (!!(*this))
			{
				std::pair <UInt128, UInt128> QR(*this, UInt128(0));
				do
				{
					QR = Divide(QR.first, Base);
					Output = "0123456789abcdef"[(uint8_t)QR.second] + Output;
				} while (QR.first);
			}
			else
				Output = "0";

			if (Output.size() < Length)
				Output = Core::String(Length - Output.size(), '0') + Output;
			return Output;
		}
		UInt128 operator<<(const uint8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const uint16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const uint32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const uint64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator>>(const uint8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const uint16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const uint32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const uint64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		std::ostream& operator<<(std::ostream& Stream, const UInt128& Right)
		{
			if (Stream.flags() & Stream.oct)
				Stream << Right.ToString(8);
			else if (Stream.flags() & Stream.dec)
				Stream << Right.ToString(10);
			else if (Stream.flags() & Stream.hex)
				Stream << Right.ToString(16);
			return Stream;
		}

		UInt256::UInt256(const Core::String& Text) : UInt256(Text, 10)
		{
		}
		UInt256::UInt256(const Core::String& Text, uint8_t Base)
		{
			*this = 0;
			UInt256 power(1);
			uint8_t digit;
			int64_t pos = (int64_t)Text.size() - 1;
			while (pos >= 0)
			{
				digit = 0;
				if ('0' <= Text[pos] && Text[pos] <= '9')
					digit = Text[pos] - '0';
				else if ('a' <= Text[pos] && Text[pos] <= 'z')
					digit = Text[pos] - 'a' + 10;
				*this += digit * power;
				power *= Base;
				pos--;
			}
		}
		UInt256 UInt256::Min()
		{
			static UInt256 Value = UInt256(0);
			return Value;
		}
		UInt256 UInt256::Max()
		{
			static UInt256 Value = UInt256(UInt128((uint64_t)-1, (uint64_t)-1), UInt128((uint64_t)-1, (uint64_t)-1));
			return Value;
		}
		UInt256::operator bool() const
		{
			return (bool)(Upper | Lower);
		}
		UInt256::operator uint8_t() const
		{
			return (uint8_t)Lower;
		}
		UInt256::operator uint16_t() const
		{
			return (uint16_t)Lower;
		}
		UInt256::operator uint32_t() const
		{
			return (uint32_t)Lower;
		}
		UInt256::operator uint64_t() const
		{
			return (uint64_t)Lower;
		}
		UInt256::operator UInt128() const
		{
			return Lower;
		}
		UInt256 UInt256::operator&(const UInt128& Right) const
		{
			return UInt256(UInt128::Min(), Lower & Right);
		}
		UInt256 UInt256::operator&(const UInt256& Right) const
		{
			return UInt256(Upper & Right.Upper, Lower & Right.Lower);
		}
		UInt256& UInt256::operator&=(const UInt128& Right)
		{
			Upper = UInt128::Min();
			Lower &= Right;
			return *this;
		}
		UInt256& UInt256::operator&=(const UInt256& Right)
		{
			Upper &= Right.Upper;
			Lower &= Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator|(const UInt128& Right) const
		{
			return UInt256(Upper, Lower | Right);
		}
		UInt256 UInt256::operator|(const UInt256& Right) const
		{
			return UInt256(Upper | Right.Upper, Lower | Right.Lower);
		}
		UInt256& UInt256::operator|=(const UInt128& Right)
		{
			Lower |= Right;
			return *this;
		}
		UInt256& UInt256::operator|=(const UInt256& Right)
		{
			Upper |= Right.Upper;
			Lower |= Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator^(const UInt128& Right) const
		{
			return UInt256(Upper, Lower ^ Right);
		}
		UInt256 UInt256::operator^(const UInt256& Right) const
		{
			return UInt256(Upper ^ Right.Upper, Lower ^ Right.Lower);
		}
		UInt256& UInt256::operator^=(const UInt128& Right)
		{
			Lower ^= Right;
			return *this;
		}
		UInt256& UInt256::operator^=(const UInt256& Right)
		{
			Upper ^= Right.Upper;
			Lower ^= Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator~() const
		{
			return UInt256(~Upper, ~Lower);
		}
		UInt256 UInt256::operator<<(const UInt128& Right) const
		{
			return *this << UInt256(Right);
		}
		UInt256 UInt256::operator<<(const UInt256& Right) const
		{
			const UInt128 shift = Right.Lower;
			if (((bool)Right.Upper) || (shift >= UInt128(256)))
				return Min();
			else if (shift == UInt128(128))
				return UInt256(Lower, UInt128::Min());
			else if (shift == UInt128::Min())
				return *this;
			else if (shift < UInt128(128))
				return UInt256((Upper << shift) + (Lower >> (UInt128(128) - shift)), Lower << shift);
			else if ((UInt128(256) > shift) && (shift > UInt128(128)))
				return UInt256(Lower << (shift - UInt128(128)), UInt128::Min());

			return Min();
		}
		UInt256& UInt256::operator<<=(const UInt128& Shift)
		{
			return *this <<= UInt256(Shift);
		}
		UInt256& UInt256::operator<<=(const UInt256& Shift)
		{
			*this = *this << Shift;
			return *this;
		}
		UInt256 UInt256::operator>>(const UInt128& Right) const
		{
			return *this >> UInt256(Right);
		}
		UInt256 UInt256::operator>>(const UInt256& Right) const
		{
			const UInt128 Shift = Right.Lower;
			if (((bool)Right.Upper) | (Shift >= UInt128(128)))
				return Min();
			else if (Shift == UInt128(128))
				return UInt256(Upper);
			else if (Shift == UInt128::Min())
				return *this;
			else if (Shift < UInt128(128))
				return UInt256(Upper >> Shift, (Upper << (UInt128(128) - Shift)) + (Lower >> Shift));
			else if ((UInt128(256) > Shift) && (Shift > UInt128(128)))
				return UInt256(Upper >> (Shift - UInt128(128)));

			return Min();
		}
		UInt256& UInt256::operator>>=(const UInt128& Shift)
		{
			return *this >>= UInt256(Shift);
		}
		UInt256& UInt256::operator>>=(const UInt256& Shift)
		{
			*this = *this >> Shift;
			return *this;
		}
		bool UInt256::operator!() const
		{
			return !(bool)*this;
		}
		bool UInt256::operator&&(const UInt128& Right) const
		{
			return (*this && UInt256(Right));
		}
		bool UInt256::operator&&(const UInt256& Right) const
		{
			return ((bool)*this && (bool)Right);
		}
		bool UInt256::operator||(const UInt128& Right) const
		{
			return (*this || UInt256(Right));
		}
		bool UInt256::operator||(const UInt256& Right) const
		{
			return ((bool)*this || (bool)Right);
		}
		bool UInt256::operator==(const UInt128& Right) const
		{
			return (*this == UInt256(Right));
		}
		bool UInt256::operator==(const UInt256& Right) const
		{
			return ((Upper == Right.Upper) && (Lower == Right.Lower));
		}
		bool UInt256::operator!=(const UInt128& Right) const
		{
			return (*this != UInt256(Right));
		}
		bool UInt256::operator!=(const UInt256& Right) const
		{
			return ((Upper != Right.Upper) | (Lower != Right.Lower));
		}
		bool UInt256::operator>(const UInt128& Right) const
		{
			return (*this > UInt256(Right));
		}
		bool UInt256::operator>(const UInt256& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower > Right.Lower);
			if (Upper > Right.Upper)
				return true;
			return false;
		}
		bool UInt256::operator<(const UInt128& Right) const
		{
			return (*this < UInt256(Right));
		}
		bool UInt256::operator<(const UInt256& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower < Right.Lower);
			if (Upper < Right.Upper)
				return true;
			return false;
		}
		bool UInt256::operator>=(const UInt128& Right) const
		{
			return (*this >= UInt256(Right));
		}
		bool UInt256::operator>=(const UInt256& Right) const
		{
			return ((*this > Right) | (*this == Right));
		}
		bool UInt256::operator<=(const UInt128& Right) const
		{
			return (*this <= UInt256(Right));
		}
		bool UInt256::operator<=(const UInt256& Right) const
		{
			return ((*this < Right) | (*this == Right));
		}
		UInt256 UInt256::operator+(const UInt128& Right) const
		{
			return *this + UInt256(Right);
		}
		UInt256 UInt256::operator+(const UInt256& Right) const
		{
			return UInt256(Upper + Right.Upper + (((Lower + Right.Lower) < Lower) ? UInt128(1) : UInt128::Min()), Lower + Right.Lower);
		}
		UInt256& UInt256::operator+=(const UInt128& Right)
		{
			return *this += UInt256(Right);
		}
		UInt256& UInt256::operator+=(const UInt256& Right)
		{
			Upper = Right.Upper + Upper + ((Lower + Right.Lower) < Lower);
			Lower = Lower + Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator-(const UInt128& Right) const
		{
			return *this - UInt256(Right);
		}
		UInt256 UInt256::operator-(const UInt256& Right) const
		{
			return UInt256(Upper - Right.Upper - ((Lower - Right.Lower) > Lower), Lower - Right.Lower);
		}
		UInt256& UInt256::operator-=(const UInt128& Right)
		{
			return *this -= UInt256(Right);
		}
		UInt256& UInt256::operator-=(const UInt256& Right)
		{
			*this = *this - Right;
			return *this;
		}
		UInt256 UInt256::operator*(const UInt128& Right) const
		{
			return *this * UInt256(Right);
		}
		UInt256 UInt256::operator*(const UInt256& Right) const
		{
			UInt128 top[4] = { Upper.High(), Upper.Low(), Lower.High(), Lower.Low() };
			UInt128 bottom[4] = { Right.High().High(), Right.High().Low(), Right.Low().High(), Right.Low().Low() };
			UInt128 products[4][4];

			for (int y = 3; y > -1; y--)
			{
				for (int x = 3; x > -1; x--)
					products[3 - y][x] = top[x] * bottom[y];
			}

			UInt128 fourth64 = UInt128(products[0][3].Low());
			UInt128 third64 = UInt128(products[0][2].Low()) + UInt128(products[0][3].High());
			UInt128 second64 = UInt128(products[0][1].Low()) + UInt128(products[0][2].High());
			UInt128 first64 = UInt128(products[0][0].Low()) + UInt128(products[0][1].High());
			third64 += UInt128(products[1][3].Low());
			second64 += UInt128(products[1][2].Low()) + UInt128(products[1][3].High());
			first64 += UInt128(products[1][1].Low()) + UInt128(products[1][2].High());
			second64 += UInt128(products[2][3].Low());
			first64 += UInt128(products[2][2].Low()) + UInt128(products[2][3].High());
			first64 += UInt128(products[3][3].Low());

			return UInt256(first64 << UInt128(64), UInt128::Min()) +
				UInt256(third64.High(), third64 << UInt128(64)) +
				UInt256(second64, UInt128::Min()) +
				UInt256(fourth64);
		}
		UInt256& UInt256::operator*=(const UInt128& Right)
		{
			return *this *= UInt256(Right);
		}
		UInt256& UInt256::operator*=(const UInt256& Right)
		{
			*this = *this * Right;
			return *this;
		}
		std::pair<UInt256, UInt256> UInt256::Divide(const UInt256& Left, const UInt256& Right) const
		{
			if (Right == Min())
			{
				VI_ASSERT(false, " division or modulus by zero");
				return std::pair <UInt256, UInt256>(Min(), Min());
			}
			else if (Right == UInt256(1))
				return std::pair <UInt256, UInt256>(Left, Min());
			else if (Left == Right)
				return std::pair <UInt256, UInt256>(UInt256(1), Min());
			else if ((Left == Min()) || (Left < Right))
				return std::pair <UInt256, UInt256>(Min(), Left);

			std::pair <UInt256, UInt256> qr(Min(), Left);
			UInt256 copyd = Right << (Left.Bits() - Right.Bits());
			UInt256 adder = UInt256(1) << (Left.Bits() - Right.Bits());
			if (copyd > qr.second)
			{
				copyd >>= UInt256(1);
				adder >>= UInt256(1);
			}
			while (qr.second >= Right)
			{
				if (qr.second >= copyd)
				{
					qr.second -= copyd;
					qr.first |= adder;
				}
				copyd >>= UInt256(1);
				adder >>= UInt256(1);
			}
			return qr;
		}
		UInt256 UInt256::operator/(const UInt128& Right) const
		{
			return *this / UInt256(Right);
		}
		UInt256 UInt256::operator/(const UInt256& Right) const
		{
			return Divide(*this, Right).first;
		}
		UInt256& UInt256::operator/=(const UInt128& Right)
		{
			return *this /= UInt256(Right);
		}
		UInt256& UInt256::operator/=(const UInt256& Right)
		{
			*this = *this / Right;
			return *this;
		}
		UInt256 UInt256::operator%(const UInt128& Right) const
		{
			return *this % UInt256(Right);
		}
		UInt256 UInt256::operator%(const UInt256& Right) const
		{
			return *this - (Right * (*this / Right));
		}
		UInt256& UInt256::operator%=(const UInt128& Right)
		{
			return *this %= UInt256(Right);
		}
		UInt256& UInt256::operator%=(const UInt256& Right)
		{
			*this = *this % Right;
			return *this;
		}
		UInt256& UInt256::operator++()
		{
			*this += UInt256(1);
			return *this;
		}
		UInt256 UInt256::operator++(int)
		{
			UInt256 temp(*this);
			++*this;
			return temp;
		}
		UInt256& UInt256::operator--()
		{
			*this -= UInt256(1);
			return *this;
		}
		UInt256 UInt256::operator--(int)
		{
			UInt256 temp(*this);
			--*this;
			return temp;
		}
		UInt256 UInt256::operator+() const
		{
			return *this;
		}
		UInt256 UInt256::operator-() const
		{
			return ~*this + UInt256(1);
		}
		const UInt128& UInt256::High() const
		{
			return Upper;
		}
		const UInt128& UInt256::Low() const
		{
			return Lower;
		}
		uint16_t UInt256::Bits() const
		{
			uint16_t out = 0;
			if (Upper)
			{
				out = 128;
				UInt128 up = Upper;
				while (up)
				{
					up >>= UInt128(1);
					out++;
				}
			}
			else
			{
				UInt128 low = Lower;
				while (low)
				{
					low >>= UInt128(1);
					out++;
				}
			}
			return out;
		}
		Core::Decimal UInt256::ToDecimal() const
		{
			return Core::Decimal(ToString());
		}
		Core::String UInt256::ToString(uint8_t Base, uint32_t Length) const
		{
			VI_ASSERT(Base >= 2 && Base <= 36, "Base must be in the range [2, 36]");
			Core::String Output;
			if (!!(*this))
			{
				std::pair <UInt256, UInt256> QR(*this, Min());
				do
				{
					QR = Divide(QR.first, Base);
					Output = "0123456789abcdefghijklmnopqrstuvwxyz"[(uint8_t)QR.second] + Output;
				} while (QR.first);
			}
			else
				Output = "0";

			if (Output.size() < Length)
				Output = Core::String(Length - Output.size(), '0') + Output;
			return Output;
		}
		UInt256 operator&(const UInt128& Left, const UInt256& Right)
		{
			return Right & Left;
		}
		UInt128& operator&=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right & Left).Low();
			return Left;
		}
		UInt256 operator|(const UInt128& Left, const UInt256& Right)
		{
			return Right | Left;
		}
		UInt128& operator|=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right | Left).Low();
			return Left;
		}
		UInt256 operator^(const UInt128& Left, const UInt256& Right)
		{
			return Right ^ Left;
		}
		UInt128& operator^=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right ^ Left).Low();
			return Left;
		}
		UInt256 operator<<(const uint8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const uint16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const uint32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const uint64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt128& operator<<=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) << Right).Low();
			return Left;
		}
		UInt256 operator>>(const uint8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const uint16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const uint32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const uint64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt128& operator>>=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) >> Right).Low();
			return Left;
		}
		bool operator==(const UInt128& Left, const UInt256& Right)
		{
			return Right == Left;
		}
		bool operator!=(const UInt128& Left, const UInt256& Right)
		{
			return Right != Left;
		}
		bool operator>(const UInt128& Left, const UInt256& Right)
		{
			return Right < Left;
		}
		bool operator<(const UInt128& Left, const UInt256& Right)
		{
			return Right > Left;
		}
		bool operator>=(const UInt128& Left, const UInt256& Right)
		{
			return Right <= Left;
		}
		bool operator<=(const UInt128& Left, const UInt256& Right)
		{
			return Right >= Left;
		}
		UInt256 operator+(const UInt128& Left, const UInt256& Right)
		{
			return Right + Left;
		}
		UInt128& operator+=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right + Left).Low();
			return Left;
		}
		UInt256 operator-(const UInt128& Left, const UInt256& Right)
		{
			return -(Right - Left);
		}
		UInt128& operator-=(UInt128& Left, const UInt256& Right)
		{
			Left = (-(Right - Left)).Low();
			return Left;
		}
		UInt256 operator*(const UInt128& Left, const UInt256& Right)
		{
			return Right * Left;
		}
		UInt128& operator*=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right * Left).Low();
			return Left;
		}
		UInt256 operator/(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) / Right;
		}
		UInt128& operator/=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) / Right).Low();
			return Left;
		}
		UInt256 operator%(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) % Right;
		}
		UInt128& operator%=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) % Right).Low();
			return Left;
		}
		std::ostream& operator<<(std::ostream& Stream, const UInt256& Right)
		{
			if (Stream.flags() & Stream.oct)
				Stream << Right.ToString(8);
			else if (Stream.flags() & Stream.dec)
				Stream << Right.ToString(10);
			else if (Stream.flags() & Stream.hex)
				Stream << Right.ToString(16);
			return Stream;
		}

		Adjacencies::Adjacencies() noexcept : NbEdges(0), CurrentNbFaces(0), Edges(nullptr), NbFaces(0), Faces(nullptr)
		{
		}
		Adjacencies::~Adjacencies() noexcept
		{
			VI_FREE(Faces);
			VI_FREE(Edges);
		}
		bool Adjacencies::Fill(Adjacencies::Desc& create)
		{
			NbFaces = create.NbFaces;
			Faces = VI_MALLOC(AdjTriangle, sizeof(AdjTriangle) * NbFaces);
			Edges = VI_MALLOC(AdjEdge, sizeof(AdjEdge) * NbFaces * 3);

			for (unsigned int i = 0; i < NbFaces; i++)
			{
				unsigned int Ref0 = create.Faces[i * 3 + 0];
				unsigned int Ref1 = create.Faces[i * 3 + 1];
				unsigned int Ref2 = create.Faces[i * 3 + 2];
				AddTriangle(Ref0, Ref1, Ref2);
			}

			return true;
		}
		bool Adjacencies::Resolve()
		{
			RadixSorter Core;
			unsigned int* FaceNb = VI_MALLOC(unsigned int, sizeof(unsigned int) * NbEdges);
			unsigned int* VRefs0 = VI_MALLOC(unsigned int, sizeof(unsigned int) * NbEdges);
			unsigned int* VRefs1 = VI_MALLOC(unsigned int, sizeof(unsigned int) * NbEdges);

			for (unsigned int i = 0; i < NbEdges; i++)
			{
				FaceNb[i] = Edges[i].FaceNb;
				VRefs0[i] = Edges[i].Ref0;
				VRefs1[i] = Edges[i].Ref1;
			}

			unsigned int* Sorted = Core.Sort(FaceNb, NbEdges).Sort(VRefs0, NbEdges).Sort(VRefs1, NbEdges).GetIndices();
			unsigned int LastRef0 = VRefs0[Sorted[0]];
			unsigned int LastRef1 = VRefs1[Sorted[0]];
			unsigned int Count = 0;
			unsigned int TmpBuffer[3];

			for (unsigned int i = 0; i < NbEdges; i++)
			{
				unsigned int Face = FaceNb[Sorted[i]];
				unsigned int Ref0 = VRefs0[Sorted[i]];
				unsigned int Ref1 = VRefs1[Sorted[i]];

				if (Ref0 == LastRef0 && Ref1 == LastRef1)
				{
					TmpBuffer[Count++] = Face;
					if (Count == 3)
					{
						VI_FREE(FaceNb);
						VI_FREE(VRefs0);
						VI_FREE(VRefs1);
						return false;
					}
				}
				else
				{
					if (Count == 2)
					{
						bool Status = UpdateLink(TmpBuffer[0], TmpBuffer[1], LastRef0, LastRef1);
						if (!Status)
						{
							VI_FREE(FaceNb);
							VI_FREE(VRefs0);
							VI_FREE(VRefs1);
							return Status;
						}
					}

					Count = 0;
					TmpBuffer[Count++] = Face;
					LastRef0 = Ref0;
					LastRef1 = Ref1;
				}
			}

			bool Status = true;
			if (Count == 2)
				Status = UpdateLink(TmpBuffer[0], TmpBuffer[1], LastRef0, LastRef1);

			VI_FREE(FaceNb);
			VI_FREE(VRefs0);
			VI_FREE(VRefs1);
			VI_FREE(Edges);

			return Status;
		}
		bool Adjacencies::AddTriangle(unsigned int ref0, unsigned int ref1, unsigned int ref2)
		{
			Faces[CurrentNbFaces].VRef[0] = ref0;
			Faces[CurrentNbFaces].VRef[1] = ref1;
			Faces[CurrentNbFaces].VRef[2] = ref2;
			Faces[CurrentNbFaces].ATri[0] = -1;
			Faces[CurrentNbFaces].ATri[1] = -1;
			Faces[CurrentNbFaces].ATri[2] = -1;

			if (ref0 < ref1)
				AddEdge(ref0, ref1, CurrentNbFaces);
			else
				AddEdge(ref1, ref0, CurrentNbFaces);

			if (ref0 < ref2)
				AddEdge(ref0, ref2, CurrentNbFaces);
			else
				AddEdge(ref2, ref0, CurrentNbFaces);

			if (ref1 < ref2)
				AddEdge(ref1, ref2, CurrentNbFaces);
			else
				AddEdge(ref2, ref1, CurrentNbFaces);

			CurrentNbFaces++;
			return true;
		}
		bool Adjacencies::AddEdge(unsigned int ref0, unsigned int ref1, unsigned int face)
		{
			Edges[NbEdges].Ref0 = ref0;
			Edges[NbEdges].Ref1 = ref1;
			Edges[NbEdges].FaceNb = face;
			NbEdges++;

			return true;
		}
		bool Adjacencies::UpdateLink(unsigned int firsttri, unsigned int secondtri, unsigned int ref0, unsigned int ref1)
		{
			AdjTriangle* Tri0 = &Faces[firsttri];
			AdjTriangle* Tri1 = &Faces[secondtri];
			unsigned char EdgeNb0 = Tri0->FindEdge(ref0, ref1);
			if (EdgeNb0 == 0xff)
				return false;

			unsigned char EdgeNb1 = Tri1->FindEdge(ref0, ref1);
			if (EdgeNb1 == 0xff)
				return false;

			Tri0->ATri[EdgeNb0] = secondtri | ((unsigned int)EdgeNb1 << 30);
			Tri1->ATri[EdgeNb1] = firsttri | ((unsigned int)EdgeNb0 << 30);

			return true;
		}

		TriangleStrip::TriangleStrip() noexcept : Adj(nullptr), Tags(nullptr), NbStrips(0), TotalLength(0), OneSided(false), SGICipher(false), ConnectAllStrips(false)
		{
		}
		TriangleStrip::~TriangleStrip() noexcept
		{
			FreeBuffers();
		}
		TriangleStrip& TriangleStrip::FreeBuffers()
		{
			Core::Vector<unsigned int>().swap(SingleStrip);
			Core::Vector<unsigned int>().swap(StripRuns);
			Core::Vector<unsigned int>().swap(StripLengths);
			VI_FREE(Tags);
			Tags = nullptr;

			VI_DELETE(Adjacencies, Adj);
			Adj = nullptr;

			return *this;
		}
		bool TriangleStrip::Fill(const TriangleStrip::Desc& create)
		{
			Adjacencies::Desc ac;
			ac.NbFaces = create.NbFaces;
			ac.Faces = create.Faces;
			FreeBuffers();

			Adj = VI_NEW(Adjacencies);
			if (!Adj->Fill(ac))
			{
				VI_DELETE(Adjacencies, Adj);
				Adj = nullptr;
				return false;
			}

			if (!Adj->Resolve())
			{
				VI_DELETE(Adjacencies, Adj);
				Adj = nullptr;
				return false;
			}

			OneSided = create.OneSided;
			SGICipher = create.SGICipher;
			ConnectAllStrips = create.ConnectAllStrips;

			return true;
		}
		bool TriangleStrip::Resolve(TriangleStrip::Result& result)
		{
			VI_ASSERT(Adj != nullptr, "triangle strip should be initialized");
			Tags = VI_MALLOC(bool, sizeof(bool) * Adj->NbFaces);
			unsigned int* Connectivity = VI_MALLOC(unsigned int, sizeof(unsigned int) * Adj->NbFaces);

			memset(Tags, 0, Adj->NbFaces * sizeof(bool));
			memset(Connectivity, 0, Adj->NbFaces * sizeof(unsigned int));

			if (SGICipher)
			{
				for (unsigned int i = 0; i < Adj->NbFaces; i++)
				{
					AdjTriangle* Tri = &Adj->Faces[i];
					if (!IS_BOUNDARY(Tri->ATri[0]))
						Connectivity[i]++;

					if (!IS_BOUNDARY(Tri->ATri[1]))
						Connectivity[i]++;

					if (!IS_BOUNDARY(Tri->ATri[2]))
						Connectivity[i]++;
				}

				RadixSorter RS;
				unsigned int* Sorted = RS.Sort(Connectivity, Adj->NbFaces).GetIndices();
				memcpy(Connectivity, Sorted, Adj->NbFaces * sizeof(unsigned int));
			}
			else
			{
				for (unsigned int i = 0; i < Adj->NbFaces; i++)
					Connectivity[i] = i;
			}

			NbStrips = 0;
			unsigned int TotalNbFaces = 0;
			unsigned int Index = 0;

			while (TotalNbFaces != Adj->NbFaces)
			{
				while (Tags[Connectivity[Index]])
					Index++;

				unsigned int FirstFace = Connectivity[Index];
				TotalNbFaces += ComputeStrip(FirstFace);
				NbStrips++;
			}

			VI_FREE(Connectivity);
			VI_FREE(Tags);
			result.Groups = StripLengths;
			result.Strips = StripRuns;

			if (ConnectAllStrips)
				ConnectStrips(result);

			return true;
		}
		unsigned int TriangleStrip::ComputeStrip(unsigned int face)
		{
			unsigned int* Strip[3];
			unsigned int* Faces[3];
			unsigned int Length[3];
			unsigned int FirstLength[3];
			unsigned int Refs0[3];
			unsigned int Refs1[3];

			Refs0[0] = Adj->Faces[face].VRef[0];
			Refs1[0] = Adj->Faces[face].VRef[1];

			Refs0[1] = Adj->Faces[face].VRef[2];
			Refs1[1] = Adj->Faces[face].VRef[0];

			Refs0[2] = Adj->Faces[face].VRef[1];
			Refs1[2] = Adj->Faces[face].VRef[2];

			for (unsigned int j = 0; j < 3; j++)
			{
				Strip[j] = VI_MALLOC(unsigned int, sizeof(unsigned int) * (Adj->NbFaces + 2 + 1 + 2));
				Faces[j] = VI_MALLOC(unsigned int, sizeof(unsigned int) * (Adj->NbFaces + 2));
				memset(Strip[j], 0xff, (Adj->NbFaces + 2 + 1 + 2) * sizeof(unsigned int));
				memset(Faces[j], 0xff, (Adj->NbFaces + 2) * sizeof(unsigned int));

				bool* vTags = VI_MALLOC(bool, sizeof(bool) * Adj->NbFaces);
				memcpy(vTags, Tags, Adj->NbFaces * sizeof(bool));

				Length[j] = TrackStrip(face, Refs0[j], Refs1[j], &Strip[j][0], &Faces[j][0], vTags);
				FirstLength[j] = Length[j];

				for (unsigned int i = 0; i < Length[j] / 2; i++)
				{
					Strip[j][i] ^= Strip[j][Length[j] - i - 1];
					Strip[j][Length[j] - i - 1] ^= Strip[j][i];
					Strip[j][i] ^= Strip[j][Length[j] - i - 1];
				}

				for (unsigned int i = 0; i < (Length[j] - 2) / 2; i++)
				{
					Faces[j][i] ^= Faces[j][Length[j] - i - 3];
					Faces[j][Length[j] - i - 3] ^= Faces[j][i];
					Faces[j][i] ^= Faces[j][Length[j] - i - 3];
				}

				unsigned int NewRef0 = Strip[j][Length[j] - 3];
				unsigned int NewRef1 = Strip[j][Length[j] - 2];
				unsigned int ExtraLength = TrackStrip(face, NewRef0, NewRef1, &Strip[j][Length[j] - 3], &Faces[j][Length[j] - 3], vTags);
				Length[j] += ExtraLength - 3;
				VI_FREE(vTags);
			}

			unsigned int Longest = Length[0];
			unsigned int Best = 0;
			if (Length[1] > Longest)
			{
				Longest = Length[1];
				Best = 1;
			}

			if (Length[2] > Longest)
			{
				Longest = Length[2];
				Best = 2;
			}

			unsigned int NbFaces = Longest - 2;
			for (unsigned int j = 0; j < Longest - 2; j++)
				Tags[Faces[Best][j]] = true;

			if (OneSided && FirstLength[Best] & 1)
			{
				if (Longest == 3 || Longest == 4)
				{
					Strip[Best][1] ^= Strip[Best][2];
					Strip[Best][2] ^= Strip[Best][1];
					Strip[Best][1] ^= Strip[Best][2];
				}
				else
				{
					for (unsigned int j = 0; j < Longest / 2; j++)
					{
						Strip[Best][j] ^= Strip[Best][Longest - j - 1];
						Strip[Best][Longest - j - 1] ^= Strip[Best][j];
						Strip[Best][j] ^= Strip[Best][Longest - j - 1];
					}

					unsigned int NewPos = Longest - FirstLength[Best];
					if (NewPos & 1)
					{
						for (unsigned int j = 0; j < Longest; j++)
							Strip[Best][Longest - j] = Strip[Best][Longest - j - 1];
						Longest++;
					}
				}
			}

			for (unsigned int j = 0; j < Longest; j++)
			{
				unsigned int Ref = Strip[Best][j];
				StripRuns.push_back(Ref);
			}

			StripLengths.push_back(Longest);
			for (unsigned int j = 0; j < 3; j++)
			{
				VI_FREE(Faces[j]);
				VI_FREE(Strip[j]);
			}

			return NbFaces;
		}
		unsigned int TriangleStrip::TrackStrip(unsigned int face, unsigned int oldest, unsigned int middle, unsigned int* strip, unsigned int* faces, bool* tags)
		{
			unsigned int Length = 2;
			strip[0] = oldest;
			strip[1] = middle;

			bool DoTheStrip = true;
			while (DoTheStrip)
			{
				unsigned int Newest = Adj->Faces[face].OppositeVertex(oldest, middle);
				strip[Length++] = Newest;
				*faces++ = face;
				tags[face] = true;

				unsigned char CurEdge = Adj->Faces[face].FindEdge(middle, Newest);
				if (!IS_BOUNDARY(CurEdge))
				{
					unsigned int Link = Adj->Faces[face].ATri[CurEdge];
					face = MAKE_ADJ_TRI(Link);
					if (tags[face])
						DoTheStrip = false;
				}
				else
					DoTheStrip = false;

				oldest = middle;
				middle = Newest;
			}

			return Length;
		}
		bool TriangleStrip::ConnectStrips(TriangleStrip::Result& result)
		{
			unsigned int* drefs = (unsigned int*)result.Strips.data();
			SingleStrip.clear();
			TotalLength = 0;

			for (unsigned int k = 0; k < result.Groups.size(); k++)
			{
				if (k)
				{
					unsigned int LastRef = drefs[-1];
					unsigned int FirstRef = drefs[0];
					SingleStrip.push_back(LastRef);
					SingleStrip.push_back(FirstRef);
					TotalLength += 2;

					if (OneSided && TotalLength & 1)
					{
						unsigned int SecondRef = drefs[1];
						if (FirstRef != SecondRef)
						{
							SingleStrip.push_back(FirstRef);
							TotalLength++;
						}
						else
						{
							result.Groups[k]--;
							drefs++;
						}
					}
				}

				for (unsigned int j = 0; j < result.Groups[k]; j++)
				{
					unsigned int Ref = drefs[j];
					SingleStrip.push_back(Ref);
				}

				drefs += result.Groups[k];
				TotalLength += result.Groups[k];
			}

			result.Strips = SingleStrip;
			result.Groups = Core::Vector<unsigned int>({ TotalLength });

			return true;
		}

		Core::Vector<int> TriangleStrip::Result::GetIndices(int Group)
		{
			Core::Vector<int> Indices;
			if (Group < 0)
			{
				Indices.reserve(Strips.size());
				for (auto& Index : Strips)
					Indices.push_back(Index);

				return Indices;
			}
			else if (Group < (int)Groups.size())
			{
				size_t Size = Groups[Group];
				Indices.reserve(Size);

				size_t Off = 0, Idx = 0;
				while (Off != Group)
					Idx += Groups[Off++];

				Size += Idx;
				for (size_t i = Idx; i < Size; i++)
					Indices.push_back(Strips[i]);

				return Indices;
			}

			return Indices;
		}
		Core::Vector<int> TriangleStrip::Result::GetInvIndices(int Group)
		{
			Core::Vector<int> Indices = GetIndices(Group);
			std::reverse(Indices.begin(), Indices.end());

			return Indices;
		}

		RadixSorter::RadixSorter() noexcept : CurrentSize(0), Indices(nullptr), Indices2(nullptr)
		{
			Histogram = VI_MALLOC(unsigned int, sizeof(unsigned int) * 256 * 4);
			Offset = VI_MALLOC(unsigned int, sizeof(unsigned int) * 256);
			ResetIndices();
		}
		RadixSorter::RadixSorter(const RadixSorter& Other) noexcept : CurrentSize(0), Indices(nullptr), Indices2(nullptr)
		{
			Histogram = VI_MALLOC(unsigned int, sizeof(unsigned int) * 256 * 4);
			Offset = VI_MALLOC(unsigned int, sizeof(unsigned int) * 256);
			ResetIndices();
		}
		RadixSorter::RadixSorter(RadixSorter&& Other) noexcept : Histogram(Other.Histogram), Offset(Other.Offset), CurrentSize(Other.CurrentSize), Indices(Other.Indices), Indices2(Other.Indices2)
		{
			Other.Indices = nullptr;
			Other.Indices2 = nullptr;
			Other.CurrentSize = 0;
			Other.Histogram = nullptr;
			Other.Offset = nullptr;
		}
		RadixSorter::~RadixSorter() noexcept
		{
			VI_FREE(Offset);
			VI_FREE(Histogram);
			VI_FREE(Indices2);
			VI_FREE(Indices);
		}
		RadixSorter& RadixSorter::operator =(const RadixSorter& V)
		{
			VI_FREE(Histogram);
			Histogram = VI_MALLOC(unsigned int, sizeof(unsigned int) * 256 * 4);

			VI_FREE(Offset);
			Offset = VI_MALLOC(unsigned int, sizeof(unsigned int) * 256);
			ResetIndices();

			return *this;
		}
		RadixSorter& RadixSorter::operator =(RadixSorter&& Other) noexcept
		{
			VI_FREE(Offset);
			VI_FREE(Histogram);
			VI_FREE(Indices2);
			VI_FREE(Indices);
			Indices = Other.Indices;
			Indices2 = Other.Indices2;
			CurrentSize = Other.CurrentSize;
			Histogram = Other.Histogram;
			Offset = Other.Offset;
			Other.Indices = nullptr;
			Other.Indices2 = nullptr;
			Other.CurrentSize = 0;
			Other.Histogram = nullptr;
			Other.Offset = nullptr;

			return *this;
		}
		RadixSorter& RadixSorter::Sort(unsigned int* input, unsigned int nb, bool signedvalues)
		{
			if (nb > CurrentSize)
			{
				VI_FREE(Indices2);
				VI_FREE(Indices);
				Indices = VI_MALLOC(unsigned int, sizeof(unsigned int) * nb);
				Indices2 = VI_MALLOC(unsigned int, sizeof(unsigned int) * nb);
				CurrentSize = nb;
				ResetIndices();
			}

			memset(Histogram, 0, 256 * 4 * sizeof(unsigned int));

			bool AlreadySorted = true;
			unsigned int* vIndices = Indices;
			unsigned char* p = (unsigned char*)input;
			unsigned char* pe = &p[nb * 4];
			unsigned int* h0 = &Histogram[0];
			unsigned int* h1 = &Histogram[256];
			unsigned int* h2 = &Histogram[512];
			unsigned int* h3 = &Histogram[768];

			if (!signedvalues)
			{
				unsigned int PrevVal = input[Indices[0]];
				while (p != pe)
				{
					unsigned int Val = input[*vIndices++];
					if (Val < PrevVal)
						AlreadySorted = false;

					PrevVal = Val;
					h0[*p++]++;
					h1[*p++]++;
					h2[*p++]++;
					h3[*p++]++;
				}
			}
			else
			{
				signed int PrevVal = (signed int)input[Indices[0]];
				while (p != pe)
				{
					signed int Val = (signed int)input[*vIndices++];
					if (Val < PrevVal)
						AlreadySorted = false;

					PrevVal = Val;
					h0[*p++]++;
					h1[*p++]++;
					h2[*p++]++;
					h3[*p++]++;
				}
			}

			if (AlreadySorted)
				return *this;

			unsigned int NbNegativeValues = 0;
			if (signedvalues)
			{
				unsigned int* h4 = &Histogram[768];
				for (unsigned int i = 128; i < 256; i++)
					NbNegativeValues += h4[i];
			}

			for (unsigned int j = 0; j < 4; j++)
			{
				unsigned int* CurCount = &Histogram[j << 8];
				bool PerformPass = true;

				for (unsigned int i = 0; i < 256; i++)
				{
					if (CurCount[i] == nb)
					{
						PerformPass = false;
						break;
					}

					if (CurCount[i])
						break;
				}

				if (PerformPass)
				{
					if (j != 3 || !signedvalues)
					{
						Offset[0] = 0;
						for (unsigned int i = 1; i < 256; i++)
							Offset[i] = Offset[i - 1] + CurCount[i - 1];
					}
					else
					{
						Offset[0] = NbNegativeValues;
						for (unsigned int i = 1; i < 128; i++)
							Offset[i] = Offset[i - 1] + CurCount[i - 1];

						Offset[128] = 0;
						for (unsigned int i = 129; i < 256; i++)
							Offset[i] = Offset[i - 1] + CurCount[i - 1];
					}

					unsigned char* InputBytes = (unsigned char*)input;
					unsigned int* IndicesStart = Indices;
					unsigned int* IndicesEnd = &Indices[nb];
					InputBytes += j;

					while (IndicesStart != IndicesEnd)
					{
						unsigned int id = *IndicesStart++;
						Indices2[Offset[InputBytes[id << 2]]++] = id;
					}

					unsigned int* Tmp = Indices;
					Indices = Indices2;
					Indices2 = Tmp;
				}
			}

			return *this;
		}
		RadixSorter& RadixSorter::Sort(float* input2, unsigned int nb)
		{
			unsigned int* input = (unsigned int*)input2;
			if (nb > CurrentSize)
			{
				VI_FREE(Indices2);
				VI_FREE(Indices);
				Indices = VI_MALLOC(unsigned int, sizeof(unsigned int) * nb);
				Indices2 = VI_MALLOC(unsigned int, sizeof(unsigned int) * nb);
				CurrentSize = nb;
				ResetIndices();
			}

			memset(Histogram, 0, 256 * 4 * sizeof(unsigned int));
			{
				float PrevVal = input2[Indices[0]];
				bool AlreadySorted = true;
				unsigned int* vIndices = Indices;
				unsigned char* p = (unsigned char*)input;
				unsigned char* pe = &p[nb * 4];
				unsigned int* h0 = &Histogram[0];
				unsigned int* h1 = &Histogram[256];
				unsigned int* h2 = &Histogram[512];
				unsigned int* h3 = &Histogram[768];

				while (p != pe)
				{
					float Val = input2[*vIndices++];
					if (Val < PrevVal)
						AlreadySorted = false;

					PrevVal = Val;
					h0[*p++]++;
					h1[*p++]++;
					h2[*p++]++;
					h3[*p++]++;
				}

				if (AlreadySorted)
					return *this;
			}

			unsigned int NbNegativeValues = 0;
			unsigned int* h3 = &Histogram[768];
			for (unsigned int i = 128; i < 256; i++)
				NbNegativeValues += h3[i];

			for (unsigned int j = 0; j < 4; j++)
			{
				unsigned int* CurCount = &Histogram[j << 8];
				bool PerformPass = true;

				for (unsigned int i = 0; i < 256; i++)
				{
					if (CurCount[i] == nb)
					{
						PerformPass = false;
						break;
					}

					if (CurCount[i])
						break;
				}

				if (PerformPass)
				{
					if (j != 3)
					{
						Offset[0] = 0;
						for (unsigned int i = 1; i < 256; i++)
							Offset[i] = Offset[i - 1] + CurCount[i - 1];

						unsigned char* InputBytes = (unsigned char*)input;
						unsigned int* IndicesStart = Indices;
						unsigned int* IndicesEnd = &Indices[nb];
						InputBytes += j;

						while (IndicesStart != IndicesEnd)
						{
							unsigned int id = *IndicesStart++;
							Indices2[Offset[InputBytes[id << 2]]++] = id;
						}
					}
					else
					{
						Offset[0] = NbNegativeValues;
						for (unsigned int i = 1; i < 128; i++)
							Offset[i] = Offset[i - 1] + CurCount[i - 1];

						Offset[255] = 0;
						for (unsigned int i = 0; i < 127; i++)
							Offset[254 - i] = Offset[255 - i] + CurCount[255 - i];

						for (unsigned int i = 128; i < 256; i++)
							Offset[i] += CurCount[i];

						for (unsigned int i = 0; i < nb; i++)
						{
							unsigned int Radix = input[Indices[i]] >> 24;
							if (Radix < 128)
								Indices2[Offset[Radix]++] = Indices[i];
							else
								Indices2[--Offset[Radix]] = Indices[i];
						}
					}

					unsigned int* Tmp = Indices;
					Indices = Indices2;
					Indices2 = Tmp;
				}
			}

			return *this;
		}
		RadixSorter& RadixSorter::ResetIndices()
		{
			for (unsigned int i = 0; i < CurrentSize; i++)
				Indices[i] = i;

			return *this;
		}
		unsigned int* RadixSorter::GetIndices()
		{
			return Indices;
		}

		MD5Hasher::MD5Hasher() noexcept
		{
			VI_TRACE("[crypto] md5 hasher init");
			memset(Buffer, 0, sizeof(Buffer));
			memset(Digest, 0, sizeof(Digest));
			Finalized = false;
			Count[0] = 0;
			Count[1] = 0;
			State[0] = 0x67452301;
			State[1] = 0xefcdab89;
			State[2] = 0x98badcfe;
			State[3] = 0x10325476;
		}
		void MD5Hasher::Decode(UInt4* Output, const UInt1* Input, unsigned int Length)
		{
			VI_ASSERT(Output != nullptr && Input != nullptr, "output and input should be set");
			for (unsigned int i = 0, j = 0; j < Length; i++, j += 4)
				Output[i] = ((UInt4)Input[j]) | (((UInt4)Input[j + 1]) << 8) | (((UInt4)Input[j + 2]) << 16) | (((UInt4)Input[j + 3]) << 24);
			VI_TRACE("[crypto] md5 hasher decode to 0x%" PRIXPTR, (void*)Output);
		}
		void MD5Hasher::Encode(UInt1* Output, const UInt4* Input, unsigned int Length)
		{
			VI_ASSERT(Output != nullptr && Input != nullptr, "output and input should be set");
			for (unsigned int i = 0, j = 0; j < Length; i++, j += 4)
			{
				Output[j] = Input[i] & 0xff;
				Output[j + 1] = (Input[i] >> 8) & 0xff;
				Output[j + 2] = (Input[i] >> 16) & 0xff;
				Output[j + 3] = (Input[i] >> 24) & 0xff;
			}
			VI_TRACE("[crypto] md5 hasher encode to 0x%" PRIXPTR, (void*)Output);
		}
		void MD5Hasher::Transform(const UInt1* Block, unsigned int Length)
		{
			VI_ASSERT(Block != nullptr, "block should be set");
			VI_TRACE("[crypto] md5 hasher transform from 0x%" PRIXPTR, (void*)Block);
			UInt4 A = State[0], B = State[1], C = State[2], D = State[3], X[16];
			Decode(X, Block, Length);

			FF(A, B, C, D, X[0], S11, 0xd76aa478);
			FF(D, A, B, C, X[1], S12, 0xe8c7b756);
			FF(C, D, A, B, X[2], S13, 0x242070db);
			FF(B, C, D, A, X[3], S14, 0xc1bdceee);
			FF(A, B, C, D, X[4], S11, 0xf57c0faf);
			FF(D, A, B, C, X[5], S12, 0x4787c62a);
			FF(C, D, A, B, X[6], S13, 0xa8304613);
			FF(B, C, D, A, X[7], S14, 0xfd469501);
			FF(A, B, C, D, X[8], S11, 0x698098d8);
			FF(D, A, B, C, X[9], S12, 0x8b44f7af);
			FF(C, D, A, B, X[10], S13, 0xffff5bb1);
			FF(B, C, D, A, X[11], S14, 0x895cd7be);
			FF(A, B, C, D, X[12], S11, 0x6b901122);
			FF(D, A, B, C, X[13], S12, 0xfd987193);
			FF(C, D, A, B, X[14], S13, 0xa679438e);
			FF(B, C, D, A, X[15], S14, 0x49b40821);
			GG(A, B, C, D, X[1], S21, 0xf61e2562);
			GG(D, A, B, C, X[6], S22, 0xc040b340);
			GG(C, D, A, B, X[11], S23, 0x265e5a51);
			GG(B, C, D, A, X[0], S24, 0xe9b6c7aa);
			GG(A, B, C, D, X[5], S21, 0xd62f105d);
			GG(D, A, B, C, X[10], S22, 0x2441453);
			GG(C, D, A, B, X[15], S23, 0xd8a1e681);
			GG(B, C, D, A, X[4], S24, 0xe7d3fbc8);
			GG(A, B, C, D, X[9], S21, 0x21e1cde6);
			GG(D, A, B, C, X[14], S22, 0xc33707d6);
			GG(C, D, A, B, X[3], S23, 0xf4d50d87);
			GG(B, C, D, A, X[8], S24, 0x455a14ed);
			GG(A, B, C, D, X[13], S21, 0xa9e3e905);
			GG(D, A, B, C, X[2], S22, 0xfcefa3f8);
			GG(C, D, A, B, X[7], S23, 0x676f02d9);
			GG(B, C, D, A, X[12], S24, 0x8d2a4c8a);
			HH(A, B, C, D, X[5], S31, 0xfffa3942);
			HH(D, A, B, C, X[8], S32, 0x8771f681);
			HH(C, D, A, B, X[11], S33, 0x6d9d6122);
			HH(B, C, D, A, X[14], S34, 0xfde5380c);
			HH(A, B, C, D, X[1], S31, 0xa4beea44);
			HH(D, A, B, C, X[4], S32, 0x4bdecfa9);
			HH(C, D, A, B, X[7], S33, 0xf6bb4b60);
			HH(B, C, D, A, X[10], S34, 0xbebfbc70);
			HH(A, B, C, D, X[13], S31, 0x289b7ec6);
			HH(D, A, B, C, X[0], S32, 0xeaa127fa);
			HH(C, D, A, B, X[3], S33, 0xd4ef3085);
			HH(B, C, D, A, X[6], S34, 0x4881d05);
			HH(A, B, C, D, X[9], S31, 0xd9d4d039);
			HH(D, A, B, C, X[12], S32, 0xe6db99e5);
			HH(C, D, A, B, X[15], S33, 0x1fa27cf8);
			HH(B, C, D, A, X[2], S34, 0xc4ac5665);
			II(A, B, C, D, X[0], S41, 0xf4292244);
			II(D, A, B, C, X[7], S42, 0x432aff97);
			II(C, D, A, B, X[14], S43, 0xab9423a7);
			II(B, C, D, A, X[5], S44, 0xfc93a039);
			II(A, B, C, D, X[12], S41, 0x655b59c3);
			II(D, A, B, C, X[3], S42, 0x8f0ccc92);
			II(C, D, A, B, X[10], S43, 0xffeff47d);
			II(B, C, D, A, X[1], S44, 0x85845dd1);
			II(A, B, C, D, X[8], S41, 0x6fa87e4f);
			II(D, A, B, C, X[15], S42, 0xfe2ce6e0);
			II(C, D, A, B, X[6], S43, 0xa3014314);
			II(B, C, D, A, X[13], S44, 0x4e0811a1);
			II(A, B, C, D, X[4], S41, 0xf7537e82);
			II(D, A, B, C, X[11], S42, 0xbd3af235);
			II(C, D, A, B, X[2], S43, 0x2ad7d2bb);
			II(B, C, D, A, X[9], S44, 0xeb86d391);

			State[0] += A;
			State[1] += B;
			State[2] += C;
			State[3] += D;
#ifdef VI_MICROSOFT
			RtlSecureZeroMemory(X, sizeof(X));
#else
			memset(X, 0, sizeof(X));
#endif
		}
		void MD5Hasher::Update(const Core::String& Input, unsigned int BlockSize)
		{
			Update(Input.c_str(), (unsigned int)Input.size(), BlockSize);
		}
		void MD5Hasher::Update(const unsigned char* Input, unsigned int Length, unsigned int BlockSize)
		{
			VI_ASSERT(Input != nullptr, "input should be set");
			VI_TRACE("[crypto] md5 hasher update from 0x%" PRIXPTR, (void*)Input);
			unsigned int Index = Count[0] / 8 % BlockSize;
			Count[0] += (Length << 3);
			if (Count[0] < Length << 3)
				Count[1]++;

			Count[1] += (Length >> 29);
			unsigned int Chunk = 64 - Index, i = 0;
			if (Length >= Chunk)
			{
				memcpy(&Buffer[Index], Input, Chunk);
				Transform(Buffer, BlockSize);

				for (i = Chunk; i + BlockSize <= Length; i += BlockSize)
					Transform(&Input[i]);
				Index = 0;
			}
			else
				i = 0;

			memcpy(&Buffer[Index], &Input[i], Length - i);
		}
		void MD5Hasher::Update(const char* Input, unsigned int Length, unsigned int BlockSize)
		{
			Update((const unsigned char*)Input, Length, BlockSize);
		}
		void MD5Hasher::Finalize()
		{
			VI_TRACE("[crypto] md5 hasher finalize at 0x%" PRIXPTR, (void*)this);
			static unsigned char Padding[64] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			if (Finalized)
				return;

			unsigned char Bits[8];
			Encode(Bits, Count, 8);

			unsigned int Index = Count[0] / 8 % 64;
			unsigned int Size = (Index < 56) ? (56 - Index) : (120 - Index);
			Update(Padding, Size);
			Update(Bits, 8);
			Encode(Digest, State, 16);

			memset(Buffer, 0, sizeof(Buffer));
			memset(Count, 0, sizeof(Count));
			Finalized = true;
		}
		char* MD5Hasher::HexDigest() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			char* Data = VI_MALLOC(char, sizeof(char) * 48);
			memset(Data, 0, sizeof(char) * 48);

			for (size_t i = 0; i < 16; i++)
				snprintf(Data + i * 2, 4, "%02x", (unsigned int)Digest[i]);

			return Data;
		}
		unsigned char* MD5Hasher::RawDigest() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			UInt1* Output = VI_MALLOC(UInt1, sizeof(UInt1) * 17);
			memcpy(Output, Digest, 16);
			Output[16] = '\0';

			return Output;
		}
		Core::String MD5Hasher::ToHex() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			char Data[48];
			memset(Data, 0, sizeof(Data));

			for (size_t i = 0; i < 16; i++)
				snprintf(Data + i * 2, 4, "%02x", (unsigned int)Digest[i]);

			return Data;
		}
		Core::String MD5Hasher::ToRaw() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			UInt1 Data[17];
			memcpy(Data, Digest, 16);
			Data[16] = '\0';

			return (const char*)Data;
		}
		MD5Hasher::UInt4 MD5Hasher::F(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return (X & Y) | (~X & Z);
		}
		MD5Hasher::UInt4 MD5Hasher::G(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return (X & Z) | (Y & ~Z);
		}
		MD5Hasher::UInt4 MD5Hasher::H(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return X ^ Y ^ Z;
		}
		MD5Hasher::UInt4 MD5Hasher::I(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return Y ^ (X | ~Z);
		}
		MD5Hasher::UInt4 MD5Hasher::L(UInt4 X, int n)
		{
			return (X << n) | (X >> (32 - n));
		}
		void MD5Hasher::FF(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + F(B, C, D) + X + AC, S) + B;
		}
		void MD5Hasher::GG(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + G(B, C, D) + X + AC, S) + B;
		}
		void MD5Hasher::HH(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + H(B, C, D) + X + AC, S) + B;
		}
		void MD5Hasher::II(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + I(B, C, D) + X + AC, S) + B;
		}

		Cipher Ciphers::DES_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_wrap();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DESX_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_desx_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC4()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
			return (Cipher)EVP_rc4();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC4_40()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
			return (Cipher)EVP_rc4_40();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC4_HMAC_MD5()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
#ifndef OPENSSL_NO_MD5
			return (Cipher)EVP_rc4_hmac_md5();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_40_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_40_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_64_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_64_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_ECB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB8()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB128()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_OFB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CTR()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_GCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_XTS()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_xts();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_WrapPad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_OCB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (Cipher)EVP_aes_128_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_ECB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CBC()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB8()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB128()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_OFB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CTR()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_GCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_192_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_WrapPad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_192_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_OCB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (Cipher)EVP_aes_192_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_ECB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB8()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB128()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_OFB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CTR()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_GCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_XTS()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_xts();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_WrapPad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_OCB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (Cipher)EVP_aes_256_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC_HMAC_SHA1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC_HMAC_SHA1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC_HMAC_SHA256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC_HMAC_SHA256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CFB1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CFB8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_GCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CFB1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CFB8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_GCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CFB1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CFB8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_GCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Chacha20()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CHACHA
			return (Cipher)EVP_chacha20();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Chacha20_Poly1305()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CHACHA
#ifndef OPENSSL_NO_POLY1305
			return (Cipher)EVP_chacha20_poly1305();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		Digest Digests::MD2()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD2
			return (Cipher)EVP_md2();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::MD4()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD4
			return (Cipher)EVP_md4();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::MD5()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD5
			return (Cipher)EVP_md5();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::MD5_SHA1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_MD5
			return (Cipher)EVP_md5_sha1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::Blake2B512()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_BLAKE2
			return (Cipher)EVP_blake2b512();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::Blake2S256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_BLAKE2
			return (Cipher)EVP_blake2s256();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha1();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA224()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA256()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA384()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha384();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha512();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512_224()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha512_224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512_256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha512_256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_224()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_384()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_384();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_512()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_512();
#else
			return nullptr;
#endif
		}
		Digest Digests::Shake128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_shake128();
#else
			return nullptr;
#endif
		}
		Digest Digests::Shake256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_shake256();
#else
			return nullptr;
#endif
		}
		Digest Digests::MDC2()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MDC2
			return (Cipher)EVP_mdc2();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::RipeMD160()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RMD160
			return (Cipher)EVP_ripemd160();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::Whirlpool()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_WHIRLPOOL
			return (Cipher)EVP_whirlpool();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::SM3()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM3
			return (Cipher)EVP_sm3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		Digest Crypto::GetDigestByName(const Core::String& Name)
		{
			VI_ASSERT(!Name.empty(), "digest name should not be empty");
#ifdef EVP_MD_name
			return (Digest)EVP_get_digestbyname(Name.c_str());
#else
			return nullptr;
#endif
		}
		Cipher Crypto::GetCipherByName(const Core::String& Name)
		{
			VI_ASSERT(!Name.empty(), "cipher name should not be empty");
#ifdef EVP_MD_name
			return (Cipher)EVP_get_cipherbyname(Name.c_str());
#else
			return nullptr;
#endif
		}
		const char* Crypto::GetDigestName(Digest Type)
		{
			VI_ASSERT(Type != nullptr, "digest should be set");
#ifdef EVP_MD_name
			const char* Name = EVP_MD_name((EVP_MD*)Type);
			return Name ? Name : "0x?";
#else
			return "0x?";
#endif
		}
		const char* Crypto::GetCipherName(Cipher Type)
		{
			VI_ASSERT(Type != nullptr, "cipher should be set");
#ifdef EVP_CIPHER_name
			const char* Name = EVP_CIPHER_name((EVP_CIPHER*)Type);
			return Name ? Name : "0x?";
#else
			return "0x?";
#endif
		}
		bool Crypto::FillRandomBytes(unsigned char* Buffer, size_t Length)
		{
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] fill random %" PRIu64 " bytes", (uint64_t)Length);
			if (RAND_bytes(Buffer, (int)Length) == 1)
				return true;

			DisplayCryptoLog();
#endif
			return false;
		}
		Core::Option<Core::String> Crypto::RandomBytes(size_t Length)
		{
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] generate random %" PRIu64 " bytes", (uint64_t)Length);
			unsigned char* Buffer = VI_MALLOC(unsigned char, sizeof(unsigned char) * Length);
			if (RAND_bytes(Buffer, (int)Length) != 1)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			Core::String Output((const char*)Buffer, Length);
			VI_FREE(Buffer);
			return Output;
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Crypto::Hash(Digest Type, const Core::String& Value)
		{
			auto Data = Crypto::HashBinary(Type, Value);
			if (!Data)
				return Data;

			return Codec::HexEncode(*Data);
		}
		Core::Option<Core::String> Crypto::HashBinary(Digest Type, const Core::String& Value)
		{
			VI_ASSERT(Type != nullptr, "type should be set");
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] %s hash %" PRIu64 " bytes", GetDigestName(Type), (uint64_t)Value.size());
			if (Value.empty())
				return Value;

			EVP_MD* Method = (EVP_MD*)Type;
			EVP_MD_CTX* Context = EVP_MD_CTX_create();
			if (!Context)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			Core::String Result;
			Result.resize(EVP_MD_size(Method));

			unsigned int Size = 0; bool OK = true;
			OK = EVP_DigestInit_ex(Context, Method, nullptr) == 1 ? OK : false;
			OK = EVP_DigestUpdate(Context, Value.c_str(), Value.size()) == 1 ? OK : false;
			OK = EVP_DigestFinal_ex(Context, (unsigned char*)Result.data(), &Size) == 1 ? OK : false;
			EVP_MD_CTX_destroy(Context);

			if (!OK)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			Result.resize((size_t)Size);
			return Result;
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Crypto::Sign(Digest Type, const char* Value, size_t Length, const PrivateKey& Key)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] HMAC-%s sign %" PRIu64 " bytes", GetDigestName(Type), (uint64_t)Length);
			if (!Length)
				return Core::String();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
#if OPENSSL_VERSION_MAJOR >= 3
			unsigned char Result[EVP_MAX_MD_SIZE];
			unsigned int Size = sizeof(Result);
			unsigned char* Pointer = ::HMAC((const EVP_MD*)Type, LocalKey.Key, (int)LocalKey.Size, (const unsigned char*)Value, Length, Result, &Size);

			if (!Pointer)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			return Core::String((const char*)Result, Size);
#elif OPENSSL_VERSION_NUMBER >= 0x1010000fL
			VI_TRACE("[crypto] HMAC-%s sign %" PRIu64 " bytes", GetDigestName(Type), (uint64_t)Length);
			HMAC_CTX* Context = HMAC_CTX_new();
			if (!Context)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			unsigned char Result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(Context, LocalKey.Key, (int)LocalKey.Size, (const EVP_MD*)Type, nullptr))
			{
				DisplayCryptoLog();
				HMAC_CTX_free(Context);
				return Core::Optional::None;
			}

			if (1 != HMAC_Update(Context, (const unsigned char*)Value, (int)Length))
			{
				DisplayCryptoLog();
				HMAC_CTX_free(Context);
				return Core::Optional::None;
			}

			unsigned int Size = sizeof(Result);
			if (1 != HMAC_Final(Context, Result, &Size))
			{
				DisplayCryptoLog();
				HMAC_CTX_free(Context);
				return Core::Optional::None;
			}

			Core::String Output((const char*)Result, Size);
			HMAC_CTX_free(Context);

			return Output;
#else
			VI_TRACE("[crypto] HMAC-%s sign %" PRIu64 " bytes", GetDigestName(Type), (uint64_t)Length);
			HMAC_CTX Context;
			HMAC_CTX_init(&Context);

			unsigned char Result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(&Context, LocalKey.Key, (int)LocalKey.Size, (const EVP_MD*)Type, nullptr))
			{
				DisplayCryptoLog();
				HMAC_CTX_cleanup(&Context);
				return Core::Optional::None;
			}

			if (1 != HMAC_Update(&Context, (const unsigned char*)Value, (int)Length))
			{
				DisplayCryptoLog();
				HMAC_CTX_cleanup(&Context);
				return Core::Optional::None;
			}

			unsigned int Size = sizeof(Result);
			if (1 != HMAC_Final(&Context, Result, &Size))
			{
				DisplayCryptoLog();
				HMAC_CTX_cleanup(&Context);
				return Core::Optional::None;
			}

			Core::String Output((const char*)Result, Size);
			HMAC_CTX_cleanup(&Context);

			return Output;
#endif
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Crypto::Sign(Digest Type, const Core::String& Value, const PrivateKey& Key)
		{
			return Sign(Type, Value.c_str(), (uint64_t)Value.size(), Key);
		}
		Core::Option<Core::String> Crypto::HMAC(Digest Type, const char* Value, size_t Length, const PrivateKey& Key)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] HMAC-%s sign %" PRIu64 " bytes", GetDigestName(Type), (uint64_t)Length);
			if (!Length)
				return Core::String();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			unsigned char Result[EVP_MAX_MD_SIZE];
			unsigned int Size = sizeof(Result);

			if (!::HMAC((const EVP_MD*)Type, LocalKey.Key, (int)LocalKey.Size, (const unsigned char*)Value, Length, Result, &Size))
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			Core::String Output((const char*)Result, Size);
			return Output;
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Crypto::HMAC(Digest Type, const Core::String& Value, const PrivateKey& Key)
		{
			return Crypto::HMAC(Type, Value.c_str(), Value.size(), Key);
		}
		Core::Option<Core::String> Crypto::Encrypt(Cipher Type, const char* Value, size_t Length, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes)
		{
			VI_ASSERT(ComplexityBytes < 0 || (ComplexityBytes > 0 && ComplexityBytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_TRACE("[crypto] %s encrypt%i %" PRIu64 " bytes", GetCipherName(Type), ComplexityBytes, (uint64_t)Length);
			if (!Length)
				return Core::String();
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			if (ComplexityBytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(Context, ComplexityBytes))
				{
					DisplayCryptoLog();
					EVP_CIPHER_CTX_free(Context);
					return Core::Optional::None;
				}
			}

			auto LocalSalt = Salt.Expose<Core::CHUNK_SIZE>();
			if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const unsigned char*)LocalKey.Key, (const unsigned char*)LocalSalt.Key))
			{
				DisplayCryptoLog();
				EVP_CIPHER_CTX_free(Context);
				return Core::Optional::None;
			}

			int Size1 = (int)Length, Size2 = 0;
			unsigned char* Buffer = VI_MALLOC(unsigned char, sizeof(unsigned char) * (Size1 + 2048));

			if (1 != EVP_EncryptUpdate(Context, Buffer, &Size2, (const unsigned char*)Value, Size1))
			{
				DisplayCryptoLog();
				EVP_CIPHER_CTX_free(Context);
				VI_FREE(Buffer);
				return Core::Optional::None;
			}

			if (1 != EVP_EncryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				DisplayCryptoLog();
				EVP_CIPHER_CTX_free(Context);
				VI_FREE(Buffer);
				return Core::Optional::None;
			}

			Core::String Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			VI_FREE(Buffer);

			return Output;
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Crypto::Encrypt(Cipher Type, const Core::String& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes)
		{
			return Encrypt(Type, Value.c_str(), Value.size(), Key, Salt, ComplexityBytes);
		}
		Core::Option<Core::String> Crypto::Decrypt(Cipher Type, const char* Value, size_t Length, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes)
		{
			VI_ASSERT(ComplexityBytes < 0 || (ComplexityBytes > 0 && ComplexityBytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_TRACE("[crypto] %s decrypt%i %" PRIu64 " bytes", GetCipherName(Type), ComplexityBytes, (uint64_t)Length);
			if (!Length)
				return Core::String();
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
			{
				DisplayCryptoLog();
				return Core::Optional::None;
			}

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			if (ComplexityBytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(Context, ComplexityBytes))
				{
					DisplayCryptoLog();
					EVP_CIPHER_CTX_free(Context);
					return Core::Optional::None;
				}
			}

			auto LocalSalt = Salt.Expose<Core::CHUNK_SIZE>();
			if (1 != EVP_DecryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const unsigned char*)LocalKey.Key, (const unsigned char*)LocalSalt.Key))
			{
				DisplayCryptoLog();
				EVP_CIPHER_CTX_free(Context);
				return Core::Optional::None;
			}

			int Size1 = (int)Length, Size2 = 0;
			unsigned char* Buffer = VI_MALLOC(unsigned char, sizeof(unsigned char) * (Size1 + 2048));

			if (1 != EVP_DecryptUpdate(Context, Buffer, &Size2, (const unsigned char*)Value, Size1))
			{
				DisplayCryptoLog();
				EVP_CIPHER_CTX_free(Context);
				VI_FREE(Buffer);
				return Core::Optional::None;
			}

			if (1 != EVP_DecryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				DisplayCryptoLog();
				EVP_CIPHER_CTX_free(Context);
				VI_FREE(Buffer);
				return Core::Optional::None;
			}

			Core::String Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			VI_FREE(Buffer);

			return Output;
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Crypto::Decrypt(Cipher Type, const Core::String& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes)
		{
			return Decrypt(Type, Value.c_str(), (uint64_t)Value.size(), Key, Salt, ComplexityBytes);
		}
		Core::Option<Core::String> Crypto::JWTSign(const Core::String& Alg, const Core::String& Payload, const PrivateKey& Key)
		{
			Digest Hash = nullptr;
			if (Alg == "HS256")
				Hash = Digests::SHA256();
			else if (Alg == "HS384")
				Hash = Digests::SHA384();
			else if (Alg == "HS512")
				Hash = Digests::SHA512();

			return Crypto::HMAC(Hash, Payload, Key);
		}
		Core::Option<Core::String> Crypto::JWTEncode(WebToken* Src, const PrivateKey& Key)
		{
			VI_ASSERT(Src != nullptr, "web token should be set");
			VI_ASSERT(Src->Header != nullptr, "web token header should be set");
			VI_ASSERT(Src->Payload != nullptr, "web token payload should be set");
			Core::String Alg = Src->Header->GetVar("alg").GetBlob();
			if (Alg.empty())
				return Core::Optional::None;

			Core::String Header;
			Core::Schema::ConvertToJSON(Src->Header, [&Header](Core::VarForm, const char* Buffer, size_t Size) { Header.append(Buffer, Size); });

			Core::String Payload;
			Core::Schema::ConvertToJSON(Src->Payload, [&Payload](Core::VarForm, const char* Buffer, size_t Size) { Payload.append(Buffer, Size); });

			Core::String Data = Codec::Base64URLEncode(Header) + '.' + Codec::Base64URLEncode(Payload);
			auto Signature = JWTSign(Alg, Data, Key);
			if (!Signature)
				return Core::Optional::None;

			Src->Signature = *Signature;
			return Data + '.' + Codec::Base64URLEncode(Src->Signature);
		}
		Core::Option<WebToken*> Crypto::JWTDecode(const Core::String& Value, const PrivateKey& Key)
		{
			Core::Vector<Core::String> Source = Core::Stringify::Split(Value, '.');
			if (Source.size() != 3)
				return Core::Optional::None;

			size_t Offset = Source[0].size() + Source[1].size() + 1;
			Source[0] = Codec::Base64URLDecode(Source[0]);
			auto Header = Core::Schema::ConvertFromJSON(Source[0].c_str(), Source[0].size());
			if (!Header)
				return Core::Optional::None;

			Source[1] = Codec::Base64URLDecode(Source[1]);
			auto Payload = Core::Schema::ConvertFromJSON(Source[1].c_str(), Source[1].size());
			if (!Payload)
			{
				VI_RELEASE(Header);
				return Core::Optional::None;
			}

			Source[0] = Header->GetVar("alg").GetBlob();
			auto Signature = JWTSign(Source[0], Value.substr(0, Offset), Key);
			if (!Signature || Codec::Base64URLEncode(*Signature) != Source[2])
			{
				VI_RELEASE(Header);
				return nullptr;
			}

			WebToken* Result = new WebToken();
			Result->Signature = Codec::Base64URLDecode(Source[2]);
			Result->Header = *Header;
			Result->Payload = *Payload;

			return Result;
		}
		Core::Option<Core::String> Crypto::DocEncrypt(Core::Schema* Src, const PrivateKey& Key, const PrivateKey& Salt)
		{
			VI_ASSERT(Src != nullptr, "schema should be set");
			Core::String Result;
			Core::Schema::ConvertToJSON(Src, [&Result](Core::VarForm, const char* Buffer, size_t Size) { Result.append(Buffer, Size); });

			auto Data = Encrypt(Ciphers::AES_256_CBC(), Result, Key, Salt);
			if (!Data)
				return Data;

			Result = Codec::Bep45Encode(*Data);
			return Result;
		}
		Core::Option<Core::Schema*> Crypto::DocDecrypt(const Core::String& Value, const PrivateKey& Key, const PrivateKey& Salt)
		{
			VI_ASSERT(!Value.empty(), "value should not be empty");
			if (Value.empty())
				return Core::Optional::None;

			auto Source = Decrypt(Ciphers::AES_256_CBC(), Codec::Bep45Decode(Value), Key, Salt);
			if (!Source)
				return Core::Optional::None;

			auto Result = Core::Schema::ConvertFromJSON(Source->c_str(), Source->size());
			if (!Result)
				return Core::Optional::None;

			return *Result;
		}
		unsigned char Crypto::RandomUC()
		{
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
			return Alphabet[(size_t)(Random() % (sizeof(Alphabet) - 1))];
		}
		uint64_t Crypto::CRC32(const Core::String& Data)
		{
			VI_TRACE("[crypto] crc32 %" PRIu64 " bytes", (uint64_t)Data.size());
			int64_t Result = 0xFFFFFFFF;
			int64_t Byte = 0;
			int64_t Mask = 0;
			int64_t Offset = 0;
			size_t Index = 0;

			while (Data[Index] != 0)
			{
				Byte = Data[Index];
				Result = Result ^ Byte;

				for (Offset = 7; Offset >= 0; Offset--)
				{
					Mask = -(Result & 1);
					Result = (Result >> 1) ^ (0xEDB88320 & Mask);
				}

				Index++;
			}

			return (uint64_t)~Result;
		}
		uint64_t Crypto::Random(uint64_t Min, uint64_t Max)
		{
			uint64_t Raw = 0;
			if (Min > Max)
				return Raw;
#ifdef VI_OPENSSL
			if (RAND_bytes((unsigned char*)&Raw, sizeof(uint64_t)) != 1)
				DisplayCryptoLog();
#else
			Raw = Random();
#endif
			return Raw % (Max - Min + 1) + Min;
		}
		uint64_t Crypto::Random()
		{
			static std::random_device Device;
			static std::mt19937 Engine(Device());
			static std::uniform_int_distribution<uint64_t> Range;

			return Range(Engine);
		}
		void Crypto::Sha1CollapseBufferBlock(unsigned int* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			for (int i = 16; --i >= 0;)
				Buffer[i] = 0;
		}
		void Crypto::Sha1ComputeHashBlock(unsigned int* Result, unsigned int* W)
		{
			VI_ASSERT(Result != nullptr, "result should be set");
			VI_ASSERT(W != nullptr, "salt should be set");
			unsigned int A = Result[0];
			unsigned int B = Result[1];
			unsigned int C = Result[2];
			unsigned int D = Result[3];
			unsigned int E = Result[4];
			int R = 0;

#define Sha1Roll(A1, A2) (((A1) << (A2)) | ((A1) >> (32 - (A2))))
#define Sha1Make(F, V) {const unsigned int T = Sha1Roll(A, 5) + (F) + E + V + W[R]; E = D; D = C; C = Sha1Roll(B, 30); B = A; A = T; R++;}

			while (R < 16)
				Sha1Make((B & C) | (~B & D), 0x5a827999);

			while (R < 20)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make((B & C) | (~B & D), 0x5a827999);
			}

			while (R < 40)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make(B ^ C ^ D, 0x6ed9eba1);
			}

			while (R < 60)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make((B & C) | (B & D) | (C & D), 0x8f1bbcdc);
			}

			while (R < 80)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make(B ^ C ^ D, 0xca62c1d6);
			}

#undef Sha1Roll
#undef Sha1Make

			Result[0] += A;
			Result[1] += B;
			Result[2] += C;
			Result[3] += D;
			Result[4] += E;
		}
		void Crypto::Sha1Compute(const void* Value, const int Length, char* Hash20)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Hash20 != nullptr, "hash of size 20 should be set");

			unsigned int Result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
			const unsigned char* ValueCUC = (const unsigned char*)Value;
			unsigned int W[80];

			const int EndOfFullBlocks = Length - 64;
			int EndCurrentBlock, CurrentBlock = 0;

			while (CurrentBlock <= EndOfFullBlocks)
			{
				EndCurrentBlock = CurrentBlock + 64;
				for (int i = 0; CurrentBlock < EndCurrentBlock; CurrentBlock += 4)
					W[i++] = (unsigned int)ValueCUC[CurrentBlock + 3] | (((unsigned int)ValueCUC[CurrentBlock + 2]) << 8) | (((unsigned int)ValueCUC[CurrentBlock + 1]) << 16) | (((unsigned int)ValueCUC[CurrentBlock]) << 24);
				Sha1ComputeHashBlock(Result, W);
			}

			EndCurrentBlock = Length - CurrentBlock;
			Sha1CollapseBufferBlock(W);

			int LastBlockBytes = 0;
			for (; LastBlockBytes < EndCurrentBlock; LastBlockBytes++)
				W[LastBlockBytes >> 2] |= (unsigned int)ValueCUC[LastBlockBytes + CurrentBlock] << ((3 - (LastBlockBytes & 3)) << 3);

			W[LastBlockBytes >> 2] |= 0x80 << ((3 - (LastBlockBytes & 3)) << 3);
			if (EndCurrentBlock >= 56)
			{
				Sha1ComputeHashBlock(Result, W);
				Sha1CollapseBufferBlock(W);
			}

			W[15] = Length << 3;
			Sha1ComputeHashBlock(Result, W);

			for (int i = 20; --i >= 0;)
				Hash20[i] = (Result[i >> 2] >> (((3 - i) & 0x3) << 3)) & 0xff;
		}
		void Crypto::Sha1Hash20ToHex(const char* Hash20, char* HexString)
		{
			VI_ASSERT(Hash20 != nullptr, "hash of size 20 should be set");
			VI_ASSERT(HexString != nullptr, "result hex should be set");

			const char Hex[] = { "0123456789abcdef" };
			for (int i = 20; --i >= 0;)
			{
				HexString[i << 1] = Hex[(Hash20[i] >> 4) & 0xf];
				HexString[(i << 1) + 1] = Hex[Hash20[i] & 0xf];
			}

			HexString[40] = 0;
		}
		void Crypto::DisplayCryptoLog()
		{
#ifdef VI_OPENSSL
			ERR_print_errors_cb([](const char* Message, size_t Size, void*)
			{
				while (Size > 0 && std::isspace(Message[Size - 1]))
					--Size;
				VI_ERR("[openssl] %.*s", (int)Size, Message);
				return 0;
			}, nullptr);
#endif
		}

		Core::String Codec::Move(const Core::String& Text, int Offset)
		{
			Core::String Result;
			Result.reserve(Text.size());
			for (size_t i = 0; i < Text.size(); i++)
			{
				if (Text[i] != 0)
					Result += char(Text[i] + Offset);
				else
					Result += " ";
			}

			return Result;
		}
		Core::String Codec::Encode64(const char Alphabet[65], const unsigned char* Value, size_t Length, bool Padding)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
			VI_TRACE("[codec] %s encode64 %" PRIu64 " bytes", Padding ? "padded" : "unpadded", (uint64_t)Length);

			Core::String Result;
			unsigned char Row3[3];
			unsigned char Row4[4];
			uint32_t Offset = 0, Step = 0;

			while (Length--)
			{
				Row3[Offset++] = *(Value++);
				if (Offset != 3)
					continue;

				Row4[0] = (Row3[0] & 0xfc) >> 2;
				Row4[1] = ((Row3[0] & 0x03) << 4) + ((Row3[1] & 0xf0) >> 4);
				Row4[2] = ((Row3[1] & 0x0f) << 2) + ((Row3[2] & 0xc0) >> 6);
				Row4[3] = Row3[2] & 0x3f;

				for (Offset = 0; Offset < 4; Offset++)
					Result += Alphabet[Row4[Offset]];

				Offset = 0;
			}

			if (!Offset)
				return Result;

			for (Step = Offset; Step < 3; Step++)
				Row3[Step] = '\0';

			Row4[0] = (Row3[0] & 0xfc) >> 2;
			Row4[1] = ((Row3[0] & 0x03) << 4) + ((Row3[1] & 0xf0) >> 4);
			Row4[2] = ((Row3[1] & 0x0f) << 2) + ((Row3[2] & 0xc0) >> 6);
			Row4[3] = Row3[2] & 0x3f;

			for (Step = 0; (Step < Offset + 1); Step++)
				Result += Alphabet[Row4[Step]];

			if (!Padding)
				return Result;

			while (Offset++ < 3)
				Result += '=';

			return Result;
		}
		Core::String Codec::Decode64(const char Alphabet[65], const unsigned char* Value, size_t Length, bool(*IsAlphabetic)(unsigned char))
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(IsAlphabetic != nullptr, "callback should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
			VI_TRACE("[codec] decode64 %" PRIu64 " bytes", (uint64_t)Length);

			Core::String Result;
			unsigned char Row4[4];
			unsigned char Row3[3];
			uint32_t Offset = 0, Step = 0;
			uint32_t Focus = 0;

			while (Length-- && (Value[Focus] != '=') && IsAlphabetic(Value[Focus]))
			{
				Row4[Offset++] = Value[Focus]; Focus++;
				if (Offset != 4)
					continue;

				for (Offset = 0; Offset < 4; Offset++)
					Row4[Offset] = (unsigned char)OffsetOf64(Alphabet, Row4[Offset]);

				Row3[0] = (Row4[0] << 2) + ((Row4[1] & 0x30) >> 4);
				Row3[1] = ((Row4[1] & 0xf) << 4) + ((Row4[2] & 0x3c) >> 2);
				Row3[2] = ((Row4[2] & 0x3) << 6) + Row4[3];

				for (Offset = 0; (Offset < 3); Offset++)
					Result += Row3[Offset];

				Offset = 0;
			}

			if (!Offset)
				return Result;

			for (Step = Offset; Step < 4; Step++)
				Row4[Step] = 0;

			for (Step = 0; Step < 4; Step++)
				Row4[Step] = (unsigned char)OffsetOf64(Alphabet, Row4[Step]);

			Row3[0] = (Row4[0] << 2) + ((Row4[1] & 0x30) >> 4);
			Row3[1] = ((Row4[1] & 0xf) << 4) + ((Row4[2] & 0x3c) >> 2);
			Row3[2] = ((Row4[2] & 0x3) << 6) + Row4[3];

			for (Step = 0; (Step < Offset - 1); Step++)
				Result += Row3[Step];

			return Result;
		}
		Core::String Codec::Bep45Encode(const Core::String& Data)
		{
			static const char From[] = " $%*+-./:";
			static const char To[] = "abcdefghi";

			Core::String Result(Base45Encode(Data));
			for (size_t i = 0; i < sizeof(From) - 1; i++)
				Core::Stringify::Replace(Result, From[i], To[i]);

			return Result;
		}
		Core::String Codec::Bep45Decode(const Core::String& Data)
		{
			static const char From[] = "abcdefghi";
			static const char To[] = " $%*+-./:";

			Core::String Result(Data);
			for (size_t i = 0; i < sizeof(From) - 1; i++)
				Core::Stringify::Replace(Result, From[i], To[i]);

			return Base45Decode(Result);
		}
		Core::String Codec::Base32Encode(const Core::String& Value)
		{
			static const char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
			Core::String Result;
			if (Value.empty())
				return Result;

			size_t Next = (size_t)Value[0], Offset = 1, Remainder = 8;
			Result.reserve((size_t)((double)Value.size() * 1.6));

			while (Remainder > 0 || Offset < Value.size())
			{
				if (Remainder < 5)
				{
					if (Offset < Value.size())
					{
						Next <<= 8;
						Next |= (size_t)Value[Offset++] & 0xFF;
						Remainder += 8;
					}
					else
					{
						size_t Padding = 5 - Remainder;
						Next <<= Padding;
						Remainder += Padding;
					}
				}

				Remainder -= 5;
				size_t Index = 0x1F & (Next >> Remainder);
				Result.push_back(Alphabet[Index % (sizeof(Alphabet) - 1)]);
			}

			return Result;
		}
		Core::String Codec::Base32Decode(const Core::String& Value)
		{
			Core::String Result;
			Result.reserve(Value.size());

			size_t Prev = 0, BitsLeft = 0;
			for (char Next : Value)
			{
				if (Next == ' ' || Next == '\t' || Next == '\r' || Next == '\n' || Next == '-')
					continue;

				Prev <<= 5;
				if (Next == '0')
					Next = 'O';
				else if (Next == '1')
					Next = 'L';
				else if (Next == '8')
					Next = 'B';

				if ((Next >= 'A' && Next <= 'Z') || (Next >= 'a' && Next <= 'z'))
					Next = (Next & 0x1F) - 1;
				else if (Next >= '2' && Next <= '7')
					Next -= '2' - 26;
				else
					break;

				Prev |= Next;
				BitsLeft += 5;
				if (BitsLeft < 8)
					continue;

				Result.push_back((char)(Prev >> (BitsLeft - 8)));
				BitsLeft -= 8;
			}

			return Result;
		}
		Core::String Codec::Base45Encode(const Core::String& Data)
		{
			VI_TRACE("[codec] base45 encode %" PRIu64 " bytes", (uint64_t)Data.size());
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
			Core::String Result;
			size_t Size = Data.size();
			Result.reserve(Size);

			for (size_t i = 0; i < Size; i += 2)
			{
				if (Size - i > 1)
				{
					int x = ((uint8_t)(Data[i]) << 8) + (uint8_t)Data[i + 1];
					unsigned char e = x / (45 * 45);
					x %= 45 * 45;

					unsigned char d = x / 45;
					unsigned char c = x % 45;
					Result += Alphabet[c];
					Result += Alphabet[d];
					Result += Alphabet[e];
				}
				else
				{
					int x = (uint8_t)Data[i];
					unsigned char d = x / 45;
					unsigned char c = x % 45;

					Result += Alphabet[c];
					Result += Alphabet[d];
				}
			}

			return Result;
		}
		Core::String Codec::Base45Decode(const Core::String& Data)
		{
			VI_TRACE("[codec] base45 decode %" PRIu64 " bytes", (uint64_t)Data.size());
			static unsigned char CharToInt[256] =
			{
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				36, 255, 255, 255, 37, 38, 255, 255, 255, 255, 39, 40, 255, 41, 42, 43,
				0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 44, 255, 255, 255, 255, 255,
				255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
				25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 35, 255, 255, 255, 255,
				255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
				25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 35, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
			};

			size_t Size = Data.size();
			Core::String Result(Size, ' ');
			size_t Offset = 0;

			for (size_t i = 0; i < Size; i += 3)
			{
				int x, a, b;
				if (Size - i < 2)
					break;

				if ((255 == (a = (char)CharToInt[(unsigned char)Data[i]])) || (255 == (b = (char)CharToInt[(unsigned char)Data[i + 1]])))
					break;

				x = a + 45 * b;
				if (Size - i >= 3)
				{
					if (255 == (a = (char)CharToInt[(unsigned char)Data[i + 2]]))
						break;

					x += a * 45 * 45;
					Result[Offset++] = x / 256;
					x %= 256;
				}

				Result[Offset++] = x;
			}

			return Core::String(Result.c_str(), Offset);
		}
		Core::String Codec::Base64Encode(const unsigned char* Value, size_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return Encode64(Set, Value, Length, true);
		}
		Core::String Codec::Base64Encode(const Core::String& Value)
		{
			return Base64Encode((const unsigned char*)Value.c_str(), Value.size());
		}
		Core::String Codec::Base64Decode(const unsigned char* Value, size_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return Decode64(Set, Value, Length, IsBase64);
		}
		Core::String Codec::Base64Decode(const Core::String& Value)
		{
			return Base64Decode((const unsigned char*)Value.c_str(), Value.size());
		}
		Core::String Codec::Base64URLEncode(const unsigned char* Value, size_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			return Encode64(Set, Value, Length, false);
		}
		Core::String Codec::Base64URLEncode(const Core::String& Value)
		{
			return Base64URLEncode((const unsigned char*)Value.c_str(), Value.size());
		}
		Core::String Codec::Base64URLDecode(const unsigned char* Value, size_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			size_t Padding = Length % 4;
			if (Padding == 0)
				return Decode64(Set, Value, Length, IsBase64URL);

			Core::String Padded((const char*)Value, Length);
			Padded.append(4 - Padding, '=');
			return Decode64(Set, (const unsigned char*)Padded.c_str(), Padded.size(), IsBase64URL);
		}
		Core::String Codec::Base64URLDecode(const Core::String& Value)
		{
			return Base64URLDecode((const unsigned char*)Value.c_str(), (uint64_t)Value.size());
		}
		Core::String Codec::Shuffle(const char* Value, size_t Size, uint64_t Mask)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[codec] shuffle %" PRIu64 " bytes", (uint64_t)Size);

			Core::String Result;
			Result.resize(Size);

			int64_t Hash = Mask;
			for (size_t i = 0; i < Size; i++)
			{
				Hash = ((Hash << 5) - Hash) + Value[i];
				Hash = Hash & Hash;
				Result[i] = (char)(Hash % 255);
			}

			return Result;
		}
		Core::Option<Core::String> Codec::Compress(const Core::String& Data, Compression Type)
		{
#ifdef VI_ZLIB
			VI_TRACE("[codec] compress %" PRIu64 " bytes", (uint64_t)Data.size());
			if (Data.empty())
				return Core::Optional::None;

			uLongf Size = compressBound((uLong)Data.size());
			Bytef* Buffer = VI_MALLOC(Bytef, Size);
			if (compress2(Buffer, &Size, (const Bytef*)Data.data(), (uLong)Data.size(), (int)Type) != Z_OK)
			{
				VI_FREE(Buffer);
				return Core::Optional::None;
			}

			Core::String Output((char*)Buffer, (size_t)Size);
			VI_FREE(Buffer);
			return Output;
#else
			return Core::Optional::None;
#endif
		}
		Core::Option<Core::String> Codec::Decompress(const Core::String& Data)
		{
#ifdef VI_ZLIB
			VI_TRACE("[codec] decompress %" PRIu64 " bytes", (uint64_t)Data.size());
			if (Data.empty())
				return Core::Optional::None;

			uLongf SourceSize = (uLong)Data.size();
			uLongf TotalSize = SourceSize * 2;
			while (true)
			{
				uLongf Size = TotalSize, FinalSize = SourceSize;
				Bytef* Buffer = VI_MALLOC(Bytef, Size);
				int Code = uncompress2(Buffer, &Size, (const Bytef*)Data.data(), &FinalSize);
				if (Code == Z_MEM_ERROR || Code == Z_BUF_ERROR)
				{
					VI_FREE(Buffer);
					TotalSize += SourceSize;
					continue;
				}
				else if (Code != Z_OK)
				{
					VI_FREE(Buffer);
					return Core::Optional::None;
				}

				Core::String Output((char*)Buffer, (size_t)Size);
				VI_FREE(Buffer);
				return Output;
			}
#endif
			return Core::Optional::None;
		}
		Core::String Codec::HexEncodeOdd(const char* Value, size_t Size, bool UpperCase)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Size > 0, "length should be greater than zero");
			VI_TRACE("[codec] hex encode odd %" PRIu64 " bytes", (uint64_t)Size);
			static const char HexLowerCase[17] = "0123456789abcdef";
			static const char HexUpperCase[17] = "0123456789ABCDEF";

			Core::String Output;
			Output.reserve(Size * 2);

			const char* Hex = UpperCase ? HexUpperCase : HexLowerCase;
			for (size_t i = 0; i < Size; i++)
			{
				unsigned char C = static_cast<unsigned char>(Value[i]);
				if (C >= 16)
					Output += Hex[C >> 4];
				Output += Hex[C & 0xf];
			}

			return Output;
		}
		Core::String Codec::HexEncodeOdd(const Core::String& Value, bool UpperCase)
		{
			return HexEncodeOdd(Value.c_str(), Value.size(), UpperCase);
		}
		Core::String Codec::HexEncode(const char* Value, size_t Size, bool UpperCase)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Size > 0, "length should be greater than zero");
			VI_TRACE("[codec] hex encode %" PRIu64 " bytes", (uint64_t)Size);
			static const char HexLowerCase[17] = "0123456789abcdef";
			static const char HexUpperCase[17] = "0123456789ABCDEF";

			Core::String Output;
			Output.reserve(Size * 2);

			const char* Hex = UpperCase ? HexUpperCase : HexLowerCase;
			for (size_t i = 0; i < Size; i++)
			{
				unsigned char C = static_cast<unsigned char>(Value[i]);
				Output += Hex[C >> 4];
				Output += Hex[C & 0xf];
			}

			return Output;
		}
		Core::String Codec::HexEncode(const Core::String& Value, bool UpperCase)
		{
			return HexEncode(Value.c_str(), Value.size(), UpperCase);
		}
		Core::String Codec::HexDecode(const char* Value, size_t Size)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Size > 0, "length should be greater than zero");
			VI_TRACE("[codec] hex decode %" PRIu64 " bytes", (uint64_t)Size);

			size_t Offset = 0;
			if (Size >= 2 && Value[0] == '0' && Value[1] == 'x')
				Offset = 2;

			Core::String Output;
			Output.reserve(Size / 2);

			char Hex[3] = { 0, 0, 0 };
			for (size_t i = Offset; i < Size; i += 2)
			{
				memcpy(Hex, Value + i, sizeof(char) * 2);
				Output.push_back((char)(int)strtol(Hex, nullptr, 16));
			}

			return Output;
		}
		Core::String Codec::HexDecode(const Core::String& Value)
		{
			return HexDecode(Value.c_str(), Value.size());
		}
		Core::String Codec::URIEncode(const Core::String& Text)
		{
			return URIEncode(Text.c_str(), Text.size());
		}
		Core::String Codec::URIEncode(const char* Text, size_t Length)
		{
			VI_ASSERT(Text != nullptr, "text should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
			VI_TRACE("[codec] uri encode %" PRIu64 " bytes", (uint64_t)Length);

			static const char* Unescape = "._-$,;~()";
			static const char* Hex = "0123456789abcdef";

			Core::String Value;
			Value.reserve(Length);

			while (*Text != '\0')
			{
				if (!isalnum(*(const unsigned char*)Text) && !strchr(Unescape, *(const unsigned char*)Text))
				{
					Value += '%';
					Value += (Hex[(*(const unsigned char*)Text) >> 4]);
					Value += (Hex[(*(const unsigned char*)Text) & 0xf]);
				}
				else
					Value += *Text;

				Text++;
			}

			return Value;
		}
		Core::String Codec::URIDecode(const Core::String& Text)
		{
			return URIDecode(Text.c_str(), Text.size());
		}
		Core::String Codec::URIDecode(const char* Text, size_t Length)
		{
			VI_ASSERT(Text != nullptr, "text should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
			VI_TRACE("[codec] uri encode %" PRIu64 " bytes", (uint64_t)Length);

			Core::String Value;
			size_t i = 0;

			while (i < Length)
			{
				if (Length >= 2 && i < Length - 2 && Text[i] == '%' && isxdigit(*(const unsigned char*)(Text + i + 1)) && isxdigit(*(const unsigned char*)(Text + i + 2)))
				{
					int A = tolower(*(const unsigned char*)(Text + i + 1)), B = tolower(*(const unsigned char*)(Text + i + 2));
					Value += (char)(((isdigit(A) ? (A - '0') : (A - 'W')) << 4) | (isdigit(B) ? (B - '0') : (B - 'W')));
					i += 2;
				}
				else if (Text[i] != '+')
					Value += Text[i];
				else
					Value += ' ';

				i++;
			}

			return Value;
		}
		Core::String Codec::Base10ToBaseN(uint64_t Value, unsigned int BaseLessThan65)
		{
			VI_ASSERT(BaseLessThan65 >= 1 && BaseLessThan65 <= 64, "base should be between 1 and 64");
			static const char* Base62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
			static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			if (Value == 0)
				return "0";

			const char* Base = (BaseLessThan65 == 64 ? Base64 : Base62);
			Core::String Output;

			while (Value > 0)
			{
				Output.insert(0, 1, Base[Value % BaseLessThan65]);
				Value /= BaseLessThan65;
			}

			return Output;
		}
		Core::String Codec::DecimalToHex(uint64_t n)
		{
			const char* Set = "0123456789abcdef";
			Core::String Result;

			do
			{
				Result = Set[n & 15] + Result;
				n >>= 4;
			} while (n > 0);

			return Result;
		}
		size_t Codec::Utf8(int Code, char* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			if (Code < 0x0080)
			{
				Buffer[0] = (Code & 0x7F);
				return 1;
			}
			else if (Code < 0x0800)
			{
				Buffer[0] = (0xC0 | ((Code >> 6) & 0x1F));
				Buffer[1] = (0x80 | (Code & 0x3F));
				return 2;
			}
			else if (Code < 0xD800)
			{
				Buffer[0] = (0xE0 | ((Code >> 12) & 0xF));
				Buffer[1] = (0x80 | ((Code >> 6) & 0x3F));
				Buffer[2] = (0x80 | (Code & 0x3F));
				return 3;
			}
			else if (Code < 0xE000)
			{
				return 0;
			}
			else if (Code < 0x10000)
			{
				Buffer[0] = (0xE0 | ((Code >> 12) & 0xF));
				Buffer[1] = (0x80 | ((Code >> 6) & 0x3F));
				Buffer[2] = (0x80 | (Code & 0x3F));
				return 3;
			}
			else if (Code < 0x110000)
			{
				Buffer[0] = (0xF0 | ((Code >> 18) & 0x7));
				Buffer[1] = (0x80 | ((Code >> 12) & 0x3F));
				Buffer[2] = (0x80 | ((Code >> 6) & 0x3F));
				Buffer[3] = (0x80 | (Code & 0x3F));
				return 4;
			}

			return 0;
		}
		bool Codec::Hex(char Code, int& Value)
		{
			if (0x20 <= Code && isdigit(Code))
			{
				Value = Code - '0';
				return true;
			}
			else if ('A' <= Code && Code <= 'F')
			{
				Value = Code - 'A' + 10;
				return true;
			}
			else if ('a' <= Code && Code <= 'f')
			{
				Value = Code - 'a' + 10;
				return true;
			}

			return false;
		}
		bool Codec::HexToString(void* Data, size_t Length, char* Buffer, size_t BufferLength)
		{
			VI_ASSERT(Data != nullptr && Length > 0, "data buffer should be set");
			VI_ASSERT(Buffer != nullptr && BufferLength > 0, "buffer should be set");
			VI_ASSERT(BufferLength >= (3 * Length), "buffer is too small");

			static const char HEX[] = "0123456789abcdef";
			for (size_t i = 0; i < Length; i++)
			{
				if (i > 0)
					Buffer[3 * i - 1] = ' ';

				Buffer[3 * i] = HEX[(((uint8_t*)Data)[i] >> 4) & 0xF];
				Buffer[3 * i + 1] = HEX[((uint8_t*)Data)[i] & 0xF];
			}

			Buffer[3 * Length - 1] = 0;
			return true;
		}
		bool Codec::HexToDecimal(const Core::String& Text, size_t Index, size_t Count, int& Value)
		{
			VI_ASSERT(Index < Text.size(), "index outside of range");

			Value = 0;
			for (; Count; Index++, Count--)
			{
				if (!Text[Index])
					return false;

				int v = 0;
				if (!Hex(Text[Index], v))
					return false;

				Value = Value * 16 + v;
			}

			return true;
		}
		bool Codec::IsBase64(unsigned char Value)
		{
			return (isalnum(Value) || (Value == '+') || (Value == '/'));
		}
		bool Codec::IsBase64URL(unsigned char Value)
		{
			return (isalnum(Value) || (Value == '-') || (Value == '_'));
		}

		bool Geometric::IsCubeInFrustum(const Matrix4x4& WVP, float Radius)
		{
			Radius = -Radius;
#ifdef VI_SIMD
			LOD_AV4(_r1, WVP.Row[3], WVP.Row[7], WVP.Row[11], WVP.Row[15]);
			LOD_AV4(_r2, WVP.Row[0], WVP.Row[4], WVP.Row[8], WVP.Row[12]);
			LOD_VAL(_r3, _r1 + _r2);
			float F = _r3.extract(3); _r3.cutoff(3);
			F /= Geometric::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r3 = _r1 - _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Geometric::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r2 = Vec4f(WVP.Row[1], WVP.Row[5], WVP.Row[9], WVP.Row[13]);
			_r3 = _r1 + _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Geometric::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r3 = _r1 - _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Geometric::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r2 = Vec4f(WVP.Row[2], WVP.Row[6], WVP.Row[10], WVP.Row[14]);
			_r3 = _r1 + _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Geometric::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r2 = Vec4f(WVP.Row[2], WVP.Row[6], WVP.Row[10], WVP.Row[14]);
			_r3 = _r1 - _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Geometric::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;
#else
			float Plane[4];
			Plane[0] = WVP.Row[3] + WVP.Row[0];
			Plane[1] = WVP.Row[7] + WVP.Row[4];
			Plane[2] = WVP.Row[11] + WVP.Row[8];
			Plane[3] = WVP.Row[15] + WVP.Row[12];

			Plane[3] /= Geometric::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] - WVP.Row[0];
			Plane[1] = WVP.Row[7] - WVP.Row[4];
			Plane[2] = WVP.Row[11] - WVP.Row[8];
			Plane[3] = WVP.Row[15] - WVP.Row[12];

			Plane[3] /= Geometric::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] + WVP.Row[1];
			Plane[1] = WVP.Row[7] + WVP.Row[5];
			Plane[2] = WVP.Row[11] + WVP.Row[9];
			Plane[3] = WVP.Row[15] + WVP.Row[13];

			Plane[3] /= Geometric::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] - WVP.Row[1];
			Plane[1] = WVP.Row[7] - WVP.Row[5];
			Plane[2] = WVP.Row[11] - WVP.Row[9];
			Plane[3] = WVP.Row[15] - WVP.Row[13];

			Plane[3] /= Geometric::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] + WVP.Row[2];
			Plane[1] = WVP.Row[7] + WVP.Row[6];
			Plane[2] = WVP.Row[11] + WVP.Row[10];
			Plane[3] = WVP.Row[15] + WVP.Row[14];

			Plane[3] /= Geometric::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] - WVP.Row[2];
			Plane[1] = WVP.Row[7] - WVP.Row[6];
			Plane[2] = WVP.Row[11] - WVP.Row[10];
			Plane[3] = WVP.Row[15] - WVP.Row[14];

			Plane[3] /= Geometric::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;
#endif
			return true;
		}
		bool Geometric::IsLeftHanded()
		{
			return LeftHanded;
		}
		bool Geometric::HasSphereIntersected(const Vector3& PositionR0, float RadiusR0, const Vector3& PositionR1, float RadiusR1)
		{
			if (PositionR0.Distance(PositionR1) < RadiusR0 + RadiusR1)
				return true;

			return false;
		}
		bool Geometric::HasLineIntersected(float Distance0, float Distance1, const Vector3& Point0, const Vector3& Point1, Vector3& Hit)
		{
			if ((Distance0 * Distance1) >= 0)
				return false;

			if (Distance0 == Distance1)
				return false;

			Hit = Point0 + (Point1 - Point0) * (-Distance0 / (Distance1 - Distance0));
			return true;
		}
		bool Geometric::HasLineIntersectedCube(const Vector3& Min, const Vector3& Max, const Vector3& Start, const Vector3& End)
		{
			if (End.X < Min.X && Start.X < Min.X)
				return false;

			if (End.X > Max.X && Start.X > Max.X)
				return false;

			if (End.Y < Min.Y && Start.Y < Min.Y)
				return false;

			if (End.Y > Max.Y && Start.Y > Max.Y)
				return false;

			if (End.Z < Min.Z && Start.Z < Min.Z)
				return false;

			if (End.Z > Max.Z && Start.Z > Max.Z)
				return false;

			if (Start.X > Min.X && Start.X < Max.X && Start.Y > Min.Y && Start.Y < Max.Y && Start.Z > Min.Z && Start.Z < Max.Z)
				return true;

			Vector3 LastHit;
			if ((HasLineIntersected(Start.X - Min.X, End.X - Min.X, Start, End, LastHit) && HasPointIntersectedCube(LastHit, Min, Max, 1)) || (HasLineIntersected(Start.Y - Min.Y, End.Y - Min.Y, Start, End, LastHit) && HasPointIntersectedCube(LastHit, Min, Max, 2)) || (HasLineIntersected(Start.Z - Min.Z, End.Z - Min.Z, Start, End, LastHit) && HasPointIntersectedCube(LastHit, Min, Max, 3)) || (HasLineIntersected(Start.X - Max.X, End.X - Max.X, Start, End, LastHit) && HasPointIntersectedCube(LastHit, Min, Max, 1)) || (HasLineIntersected(Start.Y - Max.Y, End.Y - Max.Y, Start, End, LastHit) && HasPointIntersectedCube(LastHit, Min, Max, 2)) || (HasLineIntersected(Start.Z - Max.Z, End.Z - Max.Z, Start, End, LastHit) && HasPointIntersectedCube(LastHit, Min, Max, 3)))
				return true;

			return false;
		}
		bool Geometric::HasPointIntersectedCube(const Vector3& LastHit, const Vector3& Min, const Vector3& Max, int Axis)
		{
			if (Axis == 1 && LastHit.Z > Min.Z && LastHit.Z < Max.Z && LastHit.Y > Min.Y && LastHit.Y < Max.Y)
				return true;

			if (Axis == 2 && LastHit.Z > Min.Z && LastHit.Z < Max.Z && LastHit.X > Min.X && LastHit.X < Max.X)
				return true;

			if (Axis == 3 && LastHit.X > Min.X && LastHit.X < Max.X && LastHit.Y > Min.Y && LastHit.Y < Max.Y)
				return true;

			return false;
		}
		bool Geometric::HasPointIntersectedCube(const Vector3& Position, const Vector3& Scale, const Vector3& P0)
		{
			return (P0.X) <= (Position.X + Scale.X) && (Position.X - Scale.X) <= (P0.X) && (P0.Y) <= (Position.Y + Scale.Y) && (Position.Y - Scale.Y) <= (P0.Y) && (P0.Z) <= (Position.Z + Scale.Z) && (Position.Z - Scale.Z) <= (P0.Z);
		}
		bool Geometric::HasPointIntersectedRectangle(const Vector3& Position, const Vector3& Scale, const Vector3& P0)
		{
			return P0.X >= Position.X - Scale.X && P0.X < Position.X + Scale.X && P0.Y >= Position.Y - Scale.Y && P0.Y < Position.Y + Scale.Y;
		}
		bool Geometric::HasSBIntersected(Transform* R0, Transform* R1)
		{
			if (!HasSphereIntersected(R0->GetPosition(), R0->GetScale().Hypotenuse(), R1->GetPosition(), R1->GetScale().Hypotenuse()))
				return false;

			return true;
		}
		bool Geometric::HasOBBIntersected(Transform* R0, Transform* R1)
		{
			const Vector3& Rotation0 = R0->GetRotation();
			const Vector3& Rotation1 = R1->GetRotation();
			if (Rotation0 == 0.0f && Rotation1 == 0.0f)
				return HasAABBIntersected(R0, R1);

			const Vector3& Position0 = R0->GetPosition();
			const Vector3& Position1 = R1->GetPosition();
			const Vector3& Scale0 = R0->GetScale();
			const Vector3& Scale1 = R1->GetScale();
			Matrix4x4 Temp0 = Matrix4x4::Create(Position0 - Position1, Scale0, Rotation0) * Matrix4x4::CreateRotation(Rotation1).Inv();
			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(-1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(-1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(0, 1, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(0, -1, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(1, 0, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 0, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale1, Scale1, Vector4(0, 0, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(0, 0, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			Temp0 = Matrix4x4::Create(Position1 - Position0, Scale1, Rotation1) * Matrix4x4::CreateRotation(Rotation0).Inv();
			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(-1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(-1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(0, 1, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(0, -1, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(1, 0, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 0, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-Scale0, Scale0, Vector4(0, 0, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(0, 0, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			return false;
		}
		bool Geometric::HasAABBIntersected(Transform* R0, Transform* R1)
		{
			VI_ASSERT(R0 != nullptr && R1 != nullptr, "transforms should be set");
			const Vector3& Position0 = R0->GetPosition();
			const Vector3& Position1 = R1->GetPosition();
			const Vector3& Scale0 = R0->GetScale();
			const Vector3& Scale1 = R1->GetScale();
			return
				(Position0.X - Scale0.X) <= (Position1.X + Scale1.X) &&
				(Position1.X - Scale1.X) <= (Position0.X + Scale0.X) &&
				(Position0.Y - Scale0.Y) <= (Position1.Y + Scale1.Y) &&
				(Position1.Y - Scale1.Y) <= (Position0.Y + Scale0.Y) &&
				(Position0.Z - Scale0.Z) <= (Position1.Z + Scale1.Z) &&
				(Position1.Z - Scale1.Z) <= (Position0.Z + Scale0.Z);
		}
		void Geometric::FlipIndexWindingOrder(Core::Vector<int>& Indices)
		{
			std::reverse(Indices.begin(), Indices.end());
		}
		void Geometric::MatrixRhToLh(Compute::Matrix4x4* Matrix)
		{
			VI_ASSERT(Matrix != nullptr, "matrix should be set");
			*Matrix = *Matrix * RH_TO_LH;
		}
		void Geometric::SetLeftHanded(bool IsLeftHanded)
		{
			VI_TRACE("[geometric] set left-handed: %s", IsLeftHanded ? "on" : "off");
			LeftHanded = IsLeftHanded;
		}
        Core::Vector<int> Geometric::CreateTriangleStrip(TriangleStrip::Desc& Desc, const Core::Vector<int>& Indices)
		{
			Core::Vector<unsigned int> Src;
			Src.reserve(Indices.size());

			for (auto& Index : Indices)
				Src.push_back((unsigned int)Index);

			Desc.Faces = Src.data();
			Desc.NbFaces = (unsigned int)Src.size() / 3;

			TriangleStrip Strip;
			if (!Strip.Fill(Desc))
			{
				Core::Vector<int> Empty;
				return Empty;
			}

			TriangleStrip::Result Result;
			if (!Strip.Resolve(Result))
			{
				Core::Vector<int> Empty;
				return Empty;
			}

			return Result.GetIndices();
		}
		Core::Vector<int> Geometric::CreateTriangleList(const Core::Vector<int>& Indices)
		{
			size_t Size = Indices.size() - 2;
			Core::Vector<int> Result;
			Result.reserve(Size * 3);

			for (size_t i = 0; i < Size; i++)
			{
				if (i % 2 == 0)
				{
					Result.push_back(Indices[i + 0]);
					Result.push_back(Indices[i + 1]);
					Result.push_back(Indices[i + 2]);
				}
				else
				{
					Result.push_back(Indices[i + 2]);
					Result.push_back(Indices[i + 1]);
					Result.push_back(Indices[i + 0]);
				}
			}

			return Result;
		}
		void Geometric::CreateFrustum8CRad(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ)
		{
			VI_ASSERT(Result8 != nullptr, "8 sized array should be set");
			float HalfHFov = std::tan(FieldOfView * 0.5f) * Aspect;
			float HalfVFov = std::tan(FieldOfView * 0.5f);
			float XN = NearZ * HalfHFov;
			float XF = FarZ * HalfHFov;
			float YN = NearZ * HalfVFov;
			float YF = FarZ * HalfVFov;

			Result8[0] = Vector4(XN, YN, NearZ, 1.0);
			Result8[1] = Vector4(-XN, YN, NearZ, 1.0);
			Result8[2] = Vector4(XN, -YN, NearZ, 1.0);
			Result8[3] = Vector4(-XN, -YN, NearZ, 1.0);
			Result8[4] = Vector4(XF, YF, FarZ, 1.0);
			Result8[5] = Vector4(-XF, YF, FarZ, 1.0);
			Result8[6] = Vector4(XF, -YF, FarZ, 1.0);
			Result8[7] = Vector4(-XF, -YF, FarZ, 1.0);
		}
		void Geometric::CreateFrustum8C(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ)
		{
			return CreateFrustum8CRad(Result8, Mathf::Deg2Rad() * FieldOfView, Aspect, NearZ, FarZ);
		}
		Ray Geometric::CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView)
		{
			Vector2 Tmp = Cursor * 2.0f;
			Tmp /= Screen;

			Vector4 Eye = Vector4(Tmp.X - 1.0f, 1.0f - Tmp.Y, 1.0f, 1.0f) * InvProjection;
			Eye = (Vector4(Eye.X, Eye.Y, 1.0f, 0.0f) * InvView).sNormalize();
			return Ray(Origin, Vector3(Eye.X, Eye.Y, Eye.Z));
		}
		bool Geometric::CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale, Vector3* Hit)
		{
			return Cursor.IntersectsAABB(Position, Scale, Hit);
		}
		bool Geometric::CursorRayTest(const Ray& Cursor, const Matrix4x4& World, Vector3* Hit)
		{
			return Cursor.IntersectsOBB(World, Hit);
		}
		float Geometric::FastInvSqrt(float Value)
		{
			float F = Value;
			long I = *(long*)&F;
			I = 0x5f3759df - (I >> 1);
			F = *(float*)&I;
			F = F * (1.5f - ((Value * 0.5f) * F * F));

			return F;
		}
		float Geometric::FastSqrt(float Value)
		{
			return 1.0f / FastInvSqrt(Value);
		}
		float Geometric::AabbVolume(const Vector3& Min, const Vector3& Max)
		{
			float Volume =
				(Max[1] - Min[1]) * (Max[2] - Min[2]) +
				(Max[0] - Min[0]) * (Max[2] - Min[2]) +
				(Max[0] - Min[0]) * (Max[1] - Min[1]);
			return Volume * 2.0f;
		}
		bool Geometric::LeftHanded = true;

		WebToken::WebToken() noexcept : Header(nullptr), Payload(nullptr), Token(nullptr)
		{
		}
		WebToken::WebToken(const Core::String& Issuer, const Core::String& Subject, int64_t Expiration) noexcept : Header(Core::Var::Set::Object()), Payload(Core::Var::Set::Object()), Token(nullptr)
		{
			Header->Set("alg", Core::Var::String("HS256"));
			Header->Set("typ", Core::Var::String("JWT"));
			Payload->Set("iss", Core::Var::String(Issuer));
			Payload->Set("sub", Core::Var::String(Subject));
			Payload->Set("exp", Core::Var::Integer(Expiration));
		}
		WebToken::~WebToken() noexcept
		{
			VI_CLEAR(Header);
			VI_CLEAR(Payload);
			VI_CLEAR(Token);
		}
		void WebToken::Unsign()
		{
			Signature.clear();
			Data.clear();
		}
		void WebToken::SetAlgorithm(const Core::String& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("alg", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetType(const Core::String& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("typ", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetContentType(const Core::String& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("cty", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetIssuer(const Core::String& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("iss", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetSubject(const Core::String& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("sub", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetId(const Core::String& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("jti", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetAudience(const Core::Vector<Core::String>& Value)
		{
			Core::Schema* Array = Core::Var::Set::Array();
			for (auto& Item : Value)
				Array->Push(Core::Var::String(Item));

			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("aud", Array);
			Unsign();
		}
		void WebToken::SetExpiration(int64_t Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("exp", Core::Var::Integer(Value));
			Unsign();
		}
		void WebToken::SetNotBefore(int64_t Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("nbf", Core::Var::Integer(Value));
			Unsign();
		}
		void WebToken::SetCreated(int64_t Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("iat", Core::Var::Integer(Value));
			Unsign();
		}
		void WebToken::SetRefreshToken(const Core::String& Value, const PrivateKey& Key, const PrivateKey& Salt)
		{
			VI_RELEASE(Token);
			auto NewToken = Crypto::DocDecrypt(Value, Key, Salt);
			if (NewToken)
			{
				Token = *NewToken;
				Refresher.assign(Value);
			}
			else
			{
				Token = nullptr;
				Refresher.clear();
			}
		}
		bool WebToken::Sign(const PrivateKey& Key)
		{
			VI_ASSERT(Header != nullptr, "header should be set");
			VI_ASSERT(Payload != nullptr, "payload should be set");
			if (!Data.empty())
				return true;

			auto NewData = Crypto::JWTEncode(this, Key);
			if (!NewData)
				return false;

			Data = *NewData;
			return !Data.empty();
		}
		Core::Option<Core::String> WebToken::GetRefreshToken(const PrivateKey& Key, const PrivateKey& Salt)
		{
			auto NewToken = Crypto::DocEncrypt(Token, Key, Salt);
			if (!NewToken)
				return NewToken;

			Refresher = *NewToken;
			return Refresher;
		}
		bool WebToken::IsValid() const
		{
			if (!Header || !Payload || Signature.empty())
				return false;

			int64_t Expires = Payload->GetVar("exp").GetInteger();
			return time(nullptr) < Expires;
		}

		Preprocessor::Preprocessor() noexcept : Nested(false)
		{
			Core::Vector<std::pair<Core::String, Condition>> Controls =
			{
				std::make_pair(Core::String("ifdef"), Condition::Exists),
				std::make_pair(Core::String("ifndef"), Condition::NotExists),
				std::make_pair(Core::String("ifeq"), Condition::Equals),
				std::make_pair(Core::String("ifneq"), Condition::NotEquals),
				std::make_pair(Core::String("ifgt"), Condition::Greater),
				std::make_pair(Core::String("ifngt"), Condition::NotGreater),
				std::make_pair(Core::String("ifgte"), Condition::GreaterEquals),
				std::make_pair(Core::String("ifngte"), Condition::NotGreaterEquals),
				std::make_pair(Core::String("iflt"), Condition::Less),
				std::make_pair(Core::String("ifnlt"), Condition::NotLess),
				std::make_pair(Core::String("iflte"), Condition::LessEquals),
				std::make_pair(Core::String("ifnlte"), Condition::NotLessEquals),
				std::make_pair(Core::String("iflte"), Condition::LessEquals),
				std::make_pair(Core::String("ifnlte"), Condition::NotLessEquals)
			};

			ControlFlow.reserve(Controls.size() * 2 + 2);
			ControlFlow["else"] = std::make_pair(Condition::Exists, Controller::Else);
			ControlFlow["endif"] = std::make_pair(Condition::NotExists, Controller::EndIf);

			for (auto& Item : Controls)
			{
				ControlFlow[Item.first] = std::make_pair(Item.second, Controller::StartIf);
				ControlFlow["el" + Item.first] = std::make_pair(Item.second, Controller::ElseIf);
			}
		}
		void Preprocessor::SetIncludeOptions(const IncludeDesc& NewDesc)
		{
			FileDesc = NewDesc;
		}
		void Preprocessor::SetIncludeCallback(const ProcIncludeCallback& Callback)
		{
			Include = Callback;
		}
		void Preprocessor::SetPragmaCallback(const ProcPragmaCallback& Callback)
		{
			Pragma = Callback;
		}
		void Preprocessor::SetDirectiveCallback(const Core::String& Name, ProcDirectiveCallback&& Callback)
		{
			if (Callback)
				Directives[Name] = std::move(Callback);
			else
				Directives.erase(Name);
		}
		void Preprocessor::SetFeatures(const Desc& F)
		{
			Features = F;
		}
		void Preprocessor::AddDefaultDefinitions()
		{
			Define("__VERSION__", [](Preprocessor*, const Core::Vector<Core::String>& Args)
			{
				return Core::ToString<size_t>(Mavi::VERSION);
			});
			Define("__DATE__", [](Preprocessor*, const Core::Vector<Core::String>& Args)
			{
				return EscapeText(Core::DateTime().Format("%b %d %Y"));
			});
			Define("__FILE__", [](Preprocessor* Base, const Core::Vector<Core::String>& Args)
			{
				Core::String Path = Base->GetCurrentFilePath();
				return EscapeText(Core::Stringify::Replace(Path, "\\", "\\\\"));
			});
			Define("__LINE__", [](Preprocessor* Base, const Core::Vector<Core::String>& Args)
			{
				return Core::ToString<size_t>(Base->GetCurrentLineNumber());
			});
			Define("__DIRECTORY__", [](Preprocessor* Base, const Core::Vector<Core::String>& Args)
			{
				Core::String Path = Core::OS::Path::GetDirectory(Base->GetCurrentFilePath().c_str());
				if (!Path.empty() && (Path.back() == '\\' || Path.back() == '/'))
					Path.erase(Path.size() - 1, 1);
				return EscapeText(Core::Stringify::Replace(Path, "\\", "\\\\"));
			});
		}
		bool Preprocessor::Define(const Core::String& Expression)
		{
			return Define(Expression, nullptr);
		}
		bool Preprocessor::Define(const Core::String& Expression, ProcExpansionCallback&& Callback)
		{
			if (Expression.empty())
			{
				VI_ERR("[proc] %s: empty macro definition is not allowed", ThisFile.Path.c_str());
				return false;
			}

			VI_TRACE("[proc] on 0x%" PRIXPTR " define %s", (void*)this, Expression.c_str());
			Core::String Name; size_t NameOffset = 0;
			while (NameOffset < Expression.size())
			{
				char V = Expression[NameOffset++];
				if ((!std::isalpha(V) && !std::isdigit(V) && V != '_'))
				{
					Name = Expression.substr(0, --NameOffset);
					break;
				}
				else if (NameOffset >= Expression.size())
				{
					Name = Expression.substr(0, NameOffset);
					break;
				}
			}

			bool EmptyParenthesis = false;
			if (Name.empty())
			{
				VI_ERR("[proc] %s: empty macro definition name is not allowed", ThisFile.Path.c_str());
				return false;
			}

			Definition Data; size_t TemplateBegin = NameOffset, TemplateEnd = NameOffset + 1;
			if (TemplateBegin < Expression.size() && Expression[TemplateBegin] == '(')
			{
				int32_t Pose = 1;
				while (TemplateEnd < Expression.size() && Pose > 0)
				{
					char V = Expression[TemplateEnd++];
					if (V == '(')
						++Pose;
					else if (V == ')')
						--Pose;
				}

				if (Pose < 0)
				{
					VI_ERR("[proc] %s: macro definition template parenthesis was closed twice at %s", ThisFile.Path.c_str(), Expression.c_str());
					return false;
				}
				else if (Pose > 1)
				{
					VI_ERR("[proc] %s: macro definition template parenthesis is not closed at %s", ThisFile.Path.c_str(), Expression.c_str());
					return false;
				}

				Core::String Template = Expression.substr(0, TemplateEnd);
				Core::Stringify::Trim(Template);

				if (!ParseArguments(Template, Data.Tokens, false) || Data.Tokens.empty())
				{
					VI_ERR("[proc] %s: invalid macro definition at %s", ThisFile.Path.c_str(), Template.c_str());
					return false;
				}

				Data.Tokens.erase(Data.Tokens.begin());
				EmptyParenthesis = Data.Tokens.empty();
			}

			Core::Stringify::Trim(Name);
			if (TemplateEnd < Expression.size())
			{
				Data.Expansion = Expression.substr(TemplateEnd);
				Core::Stringify::Trim(Data.Expansion);
			}

			if (EmptyParenthesis)
				Name += "()";

			size_t Size = Data.Expansion.size();
			Data.Callback = std::move(Callback);

			if (Size > 0)
				ExpandDefinitions(Data.Expansion, Size);

			Defines[Name] = std::move(Data);
			return true;
		}
		void Preprocessor::Undefine(const Core::String& Name)
		{
			VI_TRACE("[proc] on 0x%" PRIXPTR " undefine %s", (void*)this, Name.c_str());
			Defines.erase(Name);
		}
		void Preprocessor::Clear()
		{
			Defines.clear();
			Sets.clear();
			ThisFile.Path.clear();
			ThisFile.Line = 0;
		}
		bool Preprocessor::IsDefined(const Core::String& Name) const
		{
			bool Exists = Defines.count(Name) > 0;
			VI_TRACE("[proc] on 0x%" PRIXPTR " ifdef %s: %s", (void*)this, Name.c_str(), Exists ? "yes" : "no");
			return Exists;
		}
		bool Preprocessor::IsDefined(const Core::String& Name, const Core::String& Value) const
		{
			auto It = Defines.find(Name);
			if (It != Defines.end())
				return It->second.Expansion == Value;

			return Value.empty();
		}
		bool Preprocessor::Process(const Core::String& Path, Core::String& Data)
		{
			bool Nesting = SaveResult();
			if (Data.empty())
				return ReturnResult(false, Nesting);

			if (!Path.empty() && HasResult(Path))
				return ReturnResult(true, Nesting);

			FileContext LastFile = ThisFile;
			ThisFile.Path = Path;
			ThisFile.Line = 0;

			if (!Path.empty())
				Sets.insert(Path);

			if (!ConsumeTokens(Path, Data))
			{
				ThisFile = LastFile;
				return ReturnResult(false, Nesting);
			}

			size_t Size = Data.size();
			if (!ExpandDefinitions(Data, Size))
			{
				ThisFile = LastFile;
				return ReturnResult(false, Nesting);
			}

			ThisFile = LastFile;
			return ReturnResult(true, Nesting);
		}
		bool Preprocessor::ReturnResult(bool Result, bool WasNested)
		{
			Nested = WasNested;
			if (!Nested)
			{
				Sets.clear();
				ThisFile.Path.clear();
				ThisFile.Line = 0;
			}

			return Result;
		}
		bool Preprocessor::HasResult(const Core::String& Path)
		{
			return Sets.count(Path) > 0 && Path != ThisFile.Path;
		}
		bool Preprocessor::SaveResult()
		{
			bool Nesting = Nested;
			Nested = true;

			return Nesting;
		}
		ProcDirective Preprocessor::FindNextToken(Core::String& Buffer, size_t& Offset)
		{
			bool HasMultilineComments = !Features.MultilineCommentBegin.empty() && !Features.MultilineCommentEnd.empty();
			bool HasComments = !Features.CommentBegin.empty();
			bool HasStringLiterals = !Features.StringLiterals.empty();
			ProcDirective Result;

			while (Offset < Buffer.size())
			{
				char V = Buffer[Offset];
				if (V == '#' && Offset + 1 < Buffer.size() && !std::isspace(Buffer[Offset + 1]))
				{
					Result.Start = Offset;
					while (Offset < Buffer.size())
					{
						if (std::isspace(Buffer[++Offset]))
						{
							Result.Name = Buffer.substr(Result.Start + 1, Offset - Result.Start - 1);
							break;
						}
					}

					Result.End = Result.Start;
					while (Result.End < Buffer.size())
					{
						char N = Buffer[++Result.End];
						if (N == '\r' || N == '\n' || Result.End == Buffer.size())
						{
							Result.Value = Buffer.substr(Offset, Result.End - Offset);
							break;
						}
					}

					Core::Stringify::Trim(Result.Name);
					Core::Stringify::Trim(Result.Value);
					if (Result.Value.size() >= 2)
					{
						if (HasStringLiterals && Result.Value.front() == Result.Value.back() && Features.StringLiterals.find(Result.Value.front()) != Core::String::npos)
						{
							Result.Value = Result.Value.substr(1, Result.Value.size() - 2);
							Result.AsScope = true;
						}
						else if (Result.Value.front() == '<' && Result.Value.back() == '>')
						{
							Result.Value = Result.Value.substr(1, Result.Value.size() - 2);
							Result.AsGlobal = true;
						}
					}

					Result.Found = true;
					break;
				}
				else if (HasMultilineComments && V == Features.MultilineCommentBegin.front() && Offset + Features.MultilineCommentBegin.size() - 1 < Buffer.size())
				{
					if (memcmp(Buffer.c_str() + Offset, Features.MultilineCommentBegin.c_str(), sizeof(char) * Features.MultilineCommentBegin.size()) != 0)
						goto ParseCommentsOrLiterals;

					Offset += Features.MultilineCommentBegin.size();
					Offset = Buffer.find(Features.MultilineCommentEnd, Offset);
					if (Offset == Core::String::npos)
					{
						Offset = Buffer.size();
						break;
					}
					else
						Offset += Features.MultilineCommentBegin.size();
					continue;
				}
				
			ParseCommentsOrLiterals:
				if (HasComments && V == Features.CommentBegin.front() && Offset + Features.CommentBegin.size() - 1 < Buffer.size())
				{
					if (memcmp(Buffer.c_str() + Offset, Features.CommentBegin.c_str(), sizeof(char) * Features.CommentBegin.size()) != 0)
						goto ParseLiterals;

					while (Offset < Buffer.size())
					{
						char N = Buffer[++Offset];
						if (N == '\r' || N == '\n')
							break;
					}
					continue;
				}

			ParseLiterals:
				if (HasStringLiterals && Features.StringLiterals.find(V) != Core::String::npos)
				{
					while (Offset < Buffer.size())
					{
						if (Buffer[++Offset] == V)
							break;
					}
				}
				++Offset;
			}

			return Result;
		}
		ProcDirective Preprocessor::FindNextConditionalToken(Core::String& Buffer, size_t& Offset)
		{
		Retry:
			ProcDirective Next = FindNextToken(Buffer, Offset);
			if (!Next.Found)
				return Next;

			if (ControlFlow.find(Next.Name) == ControlFlow.end())
				goto Retry;

			return Next;
		}
		size_t Preprocessor::ReplaceToken(ProcDirective& Where, Core::String& Buffer, const Core::String& To)
		{
			Buffer.replace(Where.Start, Where.End - Where.Start, To);
			return Where.Start;
		}
		Core::Vector<Preprocessor::Conditional> Preprocessor::PrepareConditions(Core::String& Buffer, ProcDirective& Next, size_t& Offset, bool Top)
		{
			Core::Vector<Conditional> Conditions;
			size_t ChildEnding = 0;

			do
			{
				auto Control = ControlFlow.find(Next.Name);
				if (!Conditions.empty())
				{
					auto& Last = Conditions.back();
					if (!Last.TextEnd)
						Last.TextEnd = Next.Start;
				}

				Conditional Block;
				Block.Type = Control->second.first;
				Block.Chaining = Control->second.second != Controller::StartIf;
				Block.Expression = Next.Value;
				Block.TokenStart = Next.Start;
				Block.TokenEnd = Next.End;
				Block.TextStart = Next.End;

				if (ChildEnding > 0)
				{
					Core::String Text = Buffer.substr(ChildEnding, Block.TokenStart - ChildEnding);
					if (!Text.empty())
					{
						Conditional Subblock;
						Subblock.Expression = Text;
						Conditions.back().Childs.emplace_back(std::move(Subblock));
					}
					ChildEnding = 0;
				}

				if (Control->second.second == Controller::StartIf)
				{
					if (Conditions.empty())
					{
						Conditions.emplace_back(std::move(Block));
						continue;
					}

					auto& Base = Conditions.back();
					auto Listing = PrepareConditions(Buffer, Next, Offset, false);
					if (Listing.empty())
						return Core::Vector<Conditional>();

					size_t LastSize = Base.Childs.size();
					Base.Childs.reserve(LastSize + Listing.size());
					for (auto& Item : Listing)
						Base.Childs.push_back(Item);

					ChildEnding = Next.End;
					if (LastSize > 0)
						continue;

					Core::String Text = Buffer.substr(Base.TokenEnd, Block.TokenStart - Base.TokenEnd);
					if (!Text.empty())
					{
						Conditional Subblock;
						Subblock.Expression = Text;
						Base.Childs.insert(Base.Childs.begin() + LastSize, std::move(Subblock));
					}
					continue;
				}
				else if (Control->second.second == Controller::Else)
				{
					Block.Type = (Conditions.empty() ? Condition::Equals : (Condition)(-(int32_t)Conditions.back().Type));
					if (Conditions.empty())
					{
						VI_ERR("[proc] %s: #%s is has no opening #if block", ThisFile.Path.c_str(), Next.Name.c_str());
						return Core::Vector<Conditional>();
					}

					Conditions.emplace_back(std::move(Block));
					continue;
				}
				else if (Control->second.second == Controller::ElseIf)
				{
					if (Conditions.empty())
					{
						VI_ERR("[proc] %s: #%s is has no opening #if block", ThisFile.Path.c_str(), Next.Name.c_str());
						return Core::Vector<Conditional>();
					}

					Conditions.emplace_back(std::move(Block));
					continue;
				}
				else if (Control->second.second == Controller::EndIf)
					break;

				VI_ERR("[proc] %s: #%s is not a valid conditional block", ThisFile.Path.c_str(), Next.Name.c_str());
				return Core::Vector<Conditional>();
			} while ((Next = FindNextConditionalToken(Buffer, Offset)).Found);

			return Conditions;
		}
		Core::String Preprocessor::Evaluate(Core::String& Buffer, const Core::Vector<Conditional>& Conditions)
		{
			Core::String Result; bool SkipControlFlow = false;
			for (size_t i = 0; i < Conditions.size(); i++)
			{
				auto& Next = Conditions[i];
				int Case = SwitchCase(Next);
				if (Case == 1)
				{
					if (Next.Childs.empty())
						Result += Buffer.substr(Next.TextStart, Next.TextEnd - Next.TextStart);
					else
						Result += Evaluate(Buffer, Next.Childs);

					while (i + 1 < Conditions.size() && Conditions[i + 1].Chaining)
						++i;
				}
				else if (Case == -1)
					Result += Next.Expression;
			}

			return Result;
		}
		std::pair<Core::String, Core::String> Preprocessor::GetExpressionParts(const Core::String& Value)
		{
			size_t Offset = 0;
			while (Offset < Value.size())
			{
				char V = Value[Offset];
				if (!std::isspace(V))
				{
					++Offset;
					continue;
				}

				size_t Count = Offset;
				while (Offset < Value.size() && std::isspace(Value[++Offset]));

				Core::String Right = Value.substr(Offset);
				Core::String Left = Value.substr(0, Count);
				Core::Stringify::Trim(Right);
				Core::Stringify::Trim(Left);

				if (!Features.StringLiterals.empty() && Right.front() == Right.back() && Features.StringLiterals.find(Right.front()) != Core::String::npos)
				{
					Right = Right.substr(1, Right.size() - 2);
					Core::Stringify::Trim(Right);
				}

				return std::make_pair(Left, Right);
			}

			Core::String Expression = Value;
			Core::Stringify::Trim(Expression);
			return std::make_pair(Expression, Core::String());
		}
		std::pair<Core::String, Core::String> Preprocessor::UnpackExpression(const std::pair<Core::String, Core::String>& Expression)
		{
			auto Left = Defines.find(Expression.first);
			auto Right = Defines.find(Expression.second);
			return std::make_pair(Left == Defines.end() ? Expression.first : Left->second.Expansion, Right == Defines.end() ? Expression.second : Right->second.Expansion);
		}
		int Preprocessor::SwitchCase(const Conditional& Value)
		{
			if (Value.Expression.empty())
				return 1;

			switch (Value.Type)
			{
				case Condition::Exists:
					return IsDefined(Value.Expression) ? 1 : 0;
				case Condition::Equals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					return IsDefined(Parts.first, Parts.second) ? 1 : 0;
				}
				case Condition::Greater:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) > (Right ? *Right : 0.0);
				}
				case Condition::GreaterEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) >= (Right ? *Right : 0.0);
				}
				case Condition::Less:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) < (Right ? *Right : 0.0);
				}
				case Condition::LessEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) <= (Right ? *Right : 0.0);
				}
				case Condition::NotExists:
					return !IsDefined(Value.Expression) ? 1 : 0;
				case Condition::NotEquals:
				{
					auto Parts = GetExpressionParts(Value.Expression);
					return !IsDefined(Parts.first, Parts.second) ? 1 : 0;
				}
				case Condition::NotGreater:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) <= (Right ? *Right : 0.0);
				}
				case Condition::NotGreaterEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) < (Right ? *Right : 0.0);
				}
				case Condition::NotLess:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) >= (Right ? *Right : 0.0);
				}
				case Condition::NotLessEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) > (Right ? *Right : 0.0);
				}
				case Condition::Text:
				default:
					return -1;
			}
		}
		size_t Preprocessor::GetLinesCount(Core::String& Buffer, size_t End)
		{
			const char* Target = Buffer.c_str();
			size_t Offset = 0, Lines = 0;
			while (Offset < End)
			{
				if (Target[Offset++] == '\n')
					++Lines;
			}

			return Lines;
		}
		bool Preprocessor::ExpandDefinitions(Core::String& Buffer, size_t& Size)
		{
			if (!Size || Defines.empty())
				return true;

			Core::Vector<Core::String> Tokens;
			Core::String Formatter = Buffer.substr(0, Size);
			Buffer.erase(Buffer.begin(), Buffer.begin() + Size);

			for (auto& Item : Defines)
			{
				if (Size < Item.first.size())
					continue;

				if (Item.second.Tokens.empty())
				{
					Tokens.clear();
					if (Item.second.Callback != nullptr)
					{
						size_t FoundOffset = Formatter.find(Item.first);
						size_t TemplateSize = Item.first.size();
						while (FoundOffset != Core::String::npos)
						{
							StoreCurrentLine = [this, &Formatter, FoundOffset, TemplateSize]() { return GetLinesCount(Formatter, FoundOffset + TemplateSize); };
							Formatter.replace(FoundOffset, TemplateSize, Item.second.Callback(this, Tokens));
							FoundOffset = Formatter.find(Item.first, FoundOffset);
						}
					}
					else
						Core::Stringify::Replace(Formatter, Item.first, Item.second.Expansion);
					continue;
				}
				else if (Size < Item.first.size() + 1)
					continue;

				bool Stringify = Item.second.Expansion.find('#') != Core::String::npos;
				size_t TemplateStart, Offset = 0; Core::String Needle = Item.first + '(';
				while ((TemplateStart = Formatter.find(Needle, Offset)) != Core::String::npos)
				{
					int32_t Pose = 1; size_t TemplateEnd = TemplateStart + Needle.size();
					while (TemplateEnd < Formatter.size() && Pose > 0)
					{
						char V = Formatter[TemplateEnd++];
						if (V == '(')
							++Pose;
						else if (V == ')')
							--Pose;
					}

					if (Pose < 0)
					{
						VI_ERR("[proc] %s: macro definition expansion parenthesis was closed twice at %" PRIu64, ThisFile.Path.c_str(), (uint64_t)TemplateStart);
						return false;
					}
					else if (Pose > 1)
					{
						VI_ERR("[proc] %s: macro definition expansion parenthesis is not closed at %" PRIu64, ThisFile.Path.c_str(), (uint64_t)TemplateStart);
						return false;
					}

					Core::String Template = Formatter.substr(TemplateStart, TemplateEnd - TemplateStart);
					Tokens.reserve(Item.second.Tokens.size() + 1);
					Tokens.clear();

					if (!ParseArguments(Template, Tokens, false) || Tokens.empty())
					{
						VI_ERR("[proc] %s: macro definition expansion cannot be parsed at %" PRIu64, ThisFile.Path.c_str(), (uint64_t)TemplateStart);
						return false;
					}

					if (Tokens.size() - 1 != Item.second.Tokens.size())
					{
						VI_ERR("[proc] %s: macro definition expansion uses incorrect number of arguments (%i out of %i) at %" PRIu64, ThisFile.Path.c_str(), (int)Tokens.size() - 1, (int)Item.second.Tokens.size(), (uint64_t)TemplateStart);
						return false;
					}

					Core::String Body;
					if (Item.second.Callback != nullptr)
						Body = Item.second.Callback(this, Tokens);
					else
						Body = Item.second.Expansion;

					for (size_t i = 0; i < Item.second.Tokens.size(); i++)
					{
						auto& From = Item.second.Tokens[i];
						auto& To = Tokens[i + 1];
						Core::Stringify::Replace(Body, From, To);
						if (Stringify)
							Core::Stringify::Replace(Body, "#" + From, '\"' + To + '\"');
					}

					StoreCurrentLine = [this, &Formatter, TemplateEnd]() { return GetLinesCount(Formatter, TemplateEnd); };
					Core::Stringify::ReplacePart(Formatter, TemplateStart, TemplateEnd, Body);
					Offset = TemplateStart + Body.size();
				}
			}

			Size = Formatter.size();
			Buffer.insert(0, Formatter);
			return true;
		}
		bool Preprocessor::ParseArguments(const Core::String& Value, Core::Vector<Core::String>& Tokens, bool UnpackLiterals)
		{
			size_t Where = Value.find('(');
			if (Where == Core::String::npos || Value.back() != ')')
				return false;

			Core::String Data = Value.substr(Where + 1, Value.size() - Where - 2);
			Tokens.emplace_back(std::move(Value.substr(0, Where)));
			Where = 0;

			size_t Last = 0;
			while (Where < Data.size())
			{
				char V = Data[Where];
				if (V == '\"' || V == '\'')
				{
					while (Where < Data.size())
					{
						char N = Data[++Where];
						if (N == V)
							break;
					}

					if (Where + 1 >= Data.size())
					{
						++Where;
						goto AddValue;
					}
				}
				else if (V == ',' || Where + 1 >= Data.size())
				{
				AddValue:
					Core::String Subvalue = Data.substr(Last, Where + 1 >= Data.size() ? Core::String::npos : Where - Last);
					Core::Stringify::Trim(Subvalue);

					if (UnpackLiterals && Subvalue.size() >= 2)
					{
						if (!Features.StringLiterals.empty() && Subvalue.front() == Subvalue.back() && Features.StringLiterals.find(Subvalue.front()) != Core::String::npos)
							Tokens.emplace_back(std::move(Subvalue.substr(1, Subvalue.size() - 2)));
						else
							Tokens.emplace_back(std::move(Subvalue));
					}
					else
						Tokens.emplace_back(std::move(Subvalue));
					Last = Where + 1;
				}
				++Where;
			}

			return true;
		}
		bool Preprocessor::ConsumeTokens(const Core::String& Path, Core::String& Buffer)
		{
			size_t Offset = 0;
			while (true)
			{
				auto Next = FindNextToken(Buffer, Offset);
				if (!Next.Found)
					break;

				if (Next.Name == "include")
				{
					if (!Features.Includes)
					{
						VI_ERR("[proc] %s: not allowed to include \"%s\"", Path.c_str(), Next.Value.c_str());
						return false;
					}

					Core::String Subbuffer;
					FileDesc.Path = Next.Value;
					FileDesc.From = Path;

					IncludeResult File = ResolveInclude(FileDesc, Next.AsGlobal);
					if (HasResult(File.Module))
					{
					SuccessfulInclude:
						Offset = ReplaceToken(Next, Buffer, Subbuffer);
						continue;
					}

					switch (Include ? Include(this, File, Subbuffer) : IncludeType::Error)
					{
						case IncludeType::Preprocess:
							VI_TRACE("[proc] on 0x%" PRIXPTR " %sinclude preprocess %s%s%s", (void*)this, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str());
							if (Subbuffer.empty() || Process(File.Module, Subbuffer))
								goto SuccessfulInclude;

							VI_ERR("[proc] %s: cannot preprocess include \"%s\"", Path.c_str(), Next.Value.c_str());
							return false;
						case IncludeType::Unchanged:
							VI_TRACE("[proc] on 0x%" PRIXPTR " %sinclude as-is %s%s%s", (void*)this, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str());
							goto SuccessfulInclude;
						case IncludeType::Virtual:
							VI_TRACE("[proc] on 0x%" PRIXPTR " %sinclude virtual %s%s%s", (void*)this, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str());
							Subbuffer.clear();
							goto SuccessfulInclude;
						case IncludeType::Error:
						default:
							VI_ERR("[proc] %s: cannot find \"%s\"", Path.c_str(), Next.Value.c_str());
							return false;
					}
				}
				else if (Next.Name == "pragma")
				{
					Core::Vector<Core::String> Tokens;
					if (!ParseArguments(Next.Value, Tokens, true) || Tokens.empty())
					{
						VI_ERR("[proc] %s: cannot parse pragma definition at %s", Path.c_str(), Next.Value.c_str());
						return false;
					}

					Core::String Name = Tokens.front();
					Tokens.erase(Tokens.begin());

					if (Pragma && !Pragma(this, Name, Tokens))
					{
						VI_ERR("[proc] cannot process pragma \"%s\" directive", Name.c_str());
						return false;
					}

					if (!Pragma)
					{
						Core::String Value = Buffer.substr(Next.Start, Next.End - Next.Start);
						VI_TRACE("[proc] on 0x%" PRIXPTR " ignore pragma %s", (void*)this, Value.c_str());
						if (Next.Start > 0 && Buffer[Next.Start - 1] != '\n' && Buffer[Next.Start - 1] != '\r')
							Value.insert(Value.begin(), '\n');
						if (!Value.empty() && Value.back() != '\n' && Value.back() != '\r')
							Value.append(1, '\n');

						Offset = ReplaceToken(Next, Buffer, Value);
						Offset += Value.size();
					}
					else
					{
						VI_TRACE("[proc] on 0x%" PRIXPTR " apply pragma %s", (void*)this, Buffer.substr(Next.Start, Next.End - Next.Start).c_str());
						Offset = ReplaceToken(Next, Buffer, "");
					}
				}
				else if (Next.Name == "define")
				{
					Define(Next.Value);
					Offset = ReplaceToken(Next, Buffer, "");
				}
				else if (Next.Name == "undef")
				{
					Undefine(Next.Value);
					Offset = ReplaceToken(Next, Buffer, "");
					if (ExpandDefinitions(Buffer, Offset))
						continue;

					VI_ERR("[proc] %s: #%s cannot expand macro definitions", Path.c_str(), Next.Name.c_str());
					return false;
				}
				else if (Next.Name.size() >= 2 && Next.Name[0] == 'i' && Next.Name[1] == 'f' && ControlFlow.find(Next.Name) != ControlFlow.end())
				{
					size_t Start = Next.Start;
					Core::Vector<Conditional> Conditions = PrepareConditions(Buffer, Next, Offset, true);
					if (Conditions.empty())
					{
						VI_ERR("[proc] %s: #if has no closing #endif block", Path.c_str(), Next.Name.c_str());
						return false;
					}

					Core::String Result = Evaluate(Buffer, Conditions);
					Next.Start = Start; Next.End = Offset;
					Offset = ReplaceToken(Next, Buffer, Result);
				}
				else
				{
					auto It = Directives.find(Next.Name);
					if (It == Directives.end())
					{
						VI_ERR("[proc] %s: #%s directive is unknown", Path.c_str(), Next.Name.c_str());
						continue;
					}

					Core::String Result;
					if (!It->second(this, Next, Result))
					{
						VI_ERR("[proc] %s: #%s directive expansion failed", Path.c_str(), Next.Name.c_str());
						return false;
					}

					Offset = ReplaceToken(Next, Buffer, Result);
				}
			}

			return true;
		}
		Core::Option<Core::String> Preprocessor::ResolveFile(const Core::String& Path, const Core::String& IncludePath)
		{
			if (!Features.Includes)
			{
				VI_ERR("[proc] %s: not allowed to include \"%s\"", Path.c_str(), IncludePath.c_str());
				return Core::Optional::None;
			}

			FileContext LastFile = ThisFile;
			ThisFile.Path = Path;
			ThisFile.Line = 0;

			Core::String Subbuffer;
			FileDesc.Path = IncludePath;
			FileDesc.From = Path;

			IncludeResult File = ResolveInclude(FileDesc, !Core::Stringify::FindOf(IncludePath, ":/\\").Found && !Core::Stringify::Find(IncludePath, "./").Found);
			if (HasResult(File.Module))
			{
			IncludeResult:
				ThisFile = LastFile;
				return Subbuffer;
			}

			switch (Include ? Include(this, File, Subbuffer) : IncludeType::Error)
			{
				case IncludeType::Preprocess:
					VI_TRACE("[proc] on 0x%" PRIXPTR " %sinclude preprocess %s%s%s", (void*)this, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str());
					if (Subbuffer.empty() || Process(File.Module, Subbuffer))
						goto IncludeResult;

					VI_ERR("[proc] %s: cannot preprocess include \"%s\"", Path.c_str(), IncludePath.c_str());
					goto IncludeResult;
				case IncludeType::Unchanged:
					VI_TRACE("[proc] on 0x%" PRIXPTR " %sinclude as-is %s%s%s", (void*)this, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str());
					goto IncludeResult;
				case IncludeType::Virtual:
					VI_TRACE("[proc] on 0x%" PRIXPTR " %sinclude virtual %s%s%s", (void*)this, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str());
					Subbuffer.clear();
					goto IncludeResult;
				case IncludeType::Error:
				default:
					VI_ERR("[proc] %s: cannot find \"%s\"", Path.c_str(), IncludePath.c_str());
					ThisFile = LastFile;
					return Core::Optional::None;
			}
		}
		const Core::String& Preprocessor::GetCurrentFilePath() const
		{
			return ThisFile.Path;
		}
		size_t Preprocessor::GetCurrentLineNumber()
		{
			if (StoreCurrentLine != nullptr)
			{
				ThisFile.Line = StoreCurrentLine();
				StoreCurrentLine = nullptr;
			}

			return ThisFile.Line + 1;
		}
		IncludeResult Preprocessor::ResolveInclude(const IncludeDesc& Desc, bool AsGlobal)
		{
			IncludeResult Result;
			if (!AsGlobal)
			{
				Core::String Base;
				if (Desc.From.empty())
				{
					auto Directory = Core::OS::Directory::GetWorking();
					if (Directory)
						Base = *Directory;
				}
				else
					Base = Core::OS::Path::GetDirectory(Desc.From.c_str());

				bool IsOriginRemote = (Desc.From.empty() ? false : Core::OS::Path::IsRemote(Base.c_str()));
				bool IsPathRemote = (Desc.Path.empty() ? false : Core::OS::Path::IsRemote(Desc.Path.c_str()));
				if (IsOriginRemote || IsPathRemote)
				{
					Result.Module = Desc.Path;
					Result.IsRemote = true;
					Result.IsFile = true;
					if (!IsOriginRemote)
						return Result;

					Core::Stringify::Replace(Result.Module, "./", "");
					Result.Module.insert(0, Base);
					return Result;
				}
			}

			if (!Core::Stringify::StartsOf(Desc.Path, "/."))
			{
				if (AsGlobal || Desc.Path.empty() || Desc.Root.empty())
				{
					Result.Module = Desc.Path;
					Result.IsAbstract = true;
					Core::Stringify::Replace(Result.Module, '\\', '/');
					return Result;
				}

				auto Module = Core::OS::Path::Resolve(Desc.Path, Desc.Root, false);
				if (Module)
				{
					Result.Module = *Module;
					if (Core::OS::File::IsExists(Result.Module.c_str()))
					{
						Result.IsAbstract = true;
						Result.IsFile = true;
						return Result;
					}
				}

				for (auto It : Desc.Exts)
				{
					Core::String File(Result.Module);
					if (Result.Module.empty())
					{
						auto Target = Core::OS::Path::Resolve(Desc.Path + It, Desc.Root, false);
						if (!Target)
							continue;

						File.assign(*Target);
					}
					else
						File.append(It);

					if (!Core::OS::File::IsExists(File.c_str()))
						continue;

					Result.Module = std::move(File);
					Result.IsAbstract = true;
					Result.IsFile = true;
					return Result;
				}

				Result.Module = Desc.Path;
				Result.IsAbstract = true;
				Core::Stringify::Replace(Result.Module, '\\', '/');
				return Result;
			}
			else if (AsGlobal)
				return Result;

			Core::String Base;
			if (Desc.From.empty())
			{
				auto Directory = Core::OS::Directory::GetWorking();
				if (Directory)
					Base = *Directory;
			}
			else
				Base = Core::OS::Path::GetDirectory(Desc.From.c_str());

			auto Module = Core::OS::Path::Resolve(Desc.Path, Base, true);
			if (Module)
			{
				Result.Module = *Module;
				if (Core::OS::File::IsExists(Result.Module.c_str()))
				{
					Result.IsFile = true;
					return Result;
				}
			}

			for (auto It : Desc.Exts)
			{
				Core::String File(Result.Module);
				if (Result.Module.empty())
				{
					auto Target = Core::OS::Path::Resolve(Desc.Path + It, Desc.Root, true);
					if (!Target)
						continue;

					File.assign(*Target);
				}
				else
					File.append(It);

				if (!Core::OS::File::IsExists(File.c_str()))
					continue;

				Result.Module = std::move(File);
				Result.IsFile = true;
				return Result;
			}

			Result.Module.clear();
			return Result;
		}

		Transform::Transform(void* NewUserData) noexcept : Root(nullptr), Local(nullptr), Scaling(false), Dirty(true), UserData(NewUserData)
		{
		}
		Transform::~Transform() noexcept
		{
			SetRoot(nullptr);
			RemoveChilds();
			VI_DELETE(Spacing, Local);
		}
		void Transform::Synchronize()
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
			{
				Local->Offset = Matrix4x4::Create(Local->Position, Local->Rotation) * Root->GetBiasUnscaled();
				Global.Position = Local->Offset.Position();
				Global.Rotation = Local->Offset.RotationEuler();
				Global.Scale = (Scaling ? Local->Scale : Local->Scale * Root->Global.Scale);
				Temporary = Matrix4x4::CreateScale(Global.Scale) * Local->Offset;
			}
			else
			{
				Global.Offset = Matrix4x4::Create(Global.Position, Global.Scale, Global.Rotation);
				Temporary = Matrix4x4::CreateRotation(Global.Rotation) * Matrix4x4::CreateTranslation(Global.Position);
			}
			Dirty = false;
		}
		void Transform::Move(const Vector3& Value)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Position += Value;
			else
				Global.Position += Value;
			MakeDirty();
		}
		void Transform::Rotate(const Vector3& Value)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Rotation += Value;
			else
				Global.Rotation += Value;
			MakeDirty();
		}
		void Transform::Rescale(const Vector3& Value)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Scale += Value;
			else
				Global.Scale += Value;
			MakeDirty();
		}
		void Transform::Localize(Spacing& Where)
		{
			if (Root != nullptr)
			{
				Where.Offset = Matrix4x4::Create(Where.Position, Where.Rotation) * Root->GetBiasUnscaled().Inv();
				Where.Position = Where.Offset.Position();
				Where.Rotation = Where.Offset.RotationEuler();
				Where.Scale = (Scaling ? Where.Scale : Where.Scale / Root->Global.Scale);
			}
		}
		void Transform::Globalize(Spacing& Where)
		{
			if (Root != nullptr)
			{
				Where.Offset = Matrix4x4::Create(Where.Position, Where.Rotation) * Root->GetBiasUnscaled();
				Where.Position = Where.Offset.Position();
				Where.Rotation = Where.Offset.RotationEuler();
				Where.Scale = (Scaling ? Where.Scale : Where.Scale * Root->Global.Scale);
			}
		}
		void Transform::Specialize(Spacing& Where)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
			{
				Where.Offset = Matrix4x4::Create(Local->Position, Local->Rotation) * Root->GetBiasUnscaled();
				Where.Position = Where.Offset.Position();
				Where.Rotation = Where.Offset.RotationEuler();
				Where.Scale = (Scaling ? Local->Scale : Local->Scale * Root->Global.Scale);
			}
			else if (&Where != &Global)
				Where = Global;
		}
		void Transform::Copy(Transform* Target)
		{
			VI_ASSERT(Target != nullptr && Target != this, "target should be set");
			VI_DELETE(Spacing, Local);

			if (Target->Root != nullptr)
				Local = VI_NEW(Spacing, *Target->Local);
			else
				Local = nullptr;

			UserData = Target->UserData;
			Childs = Target->Childs;
			Global = Target->Global;
			Scaling = Target->Scaling;
			Root = Target->Root;
			Dirty = true;
		}
		void Transform::AddChild(Transform* Child)
		{
			VI_ASSERT(Child != nullptr && Child != this, "child should be set");
			Childs.push_back(Child);
			Child->MakeDirty();
		}
		void Transform::RemoveChild(Transform* Child)
		{
			VI_ASSERT(Child != nullptr && Child != this, "child should be set");
			if (Child->Root == this)
				Child->SetRoot(nullptr);
		}
		void Transform::RemoveChilds()
		{
			Core::Vector<Transform*> Array;
			Array.swap(Childs);

			for (auto& Child : Array)
			{
				if (Child->Root == this)
					Child->SetRoot(nullptr);
			}
		}
		void Transform::WhenDirty(Core::TaskCallback&& Callback)
		{
			OnDirty = std::move(Callback);
			if (Dirty && OnDirty)
				OnDirty();
		}
		void Transform::MakeDirty()
		{
			if (Dirty)
				return;

			Dirty = true;
			if (OnDirty)
				OnDirty();

			for (auto& Child : Childs)
				Child->MakeDirty();
		}
		void Transform::SetScaling(bool Enabled)
		{
			Scaling = Enabled;
			MakeDirty();
		}
		void Transform::SetPosition(const Vector3& Value)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			bool Updated = !(Root ? Local->Position.IsEquals(Value) : Global.Position.IsEquals(Value));
			if (Root != nullptr)
				Local->Position = Value;
			else
				Global.Position = Value;

			if (Updated)
				MakeDirty();
		}
		void Transform::SetRotation(const Vector3& Value)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			bool Updated = !(Root ? Local->Rotation.IsEquals(Value) : Global.Rotation.IsEquals(Value));
			if (Root != nullptr)
				Local->Rotation = Value;
			else
				Global.Rotation = Value;

			if (Updated)
				MakeDirty();
		}
		void Transform::SetScale(const Vector3& Value)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			bool Updated = !(Root ? Local->Scale.IsEquals(Value) : Global.Scale.IsEquals(Value));
			if (Root != nullptr)
				Local->Scale = Value;
			else
				Global.Scale = Value;

			if (Updated)
				MakeDirty();
		}
		void Transform::SetSpacing(Positioning Space, Spacing& Where)
		{
			if (Space == Positioning::Global)
				Localize(Where);

			GetSpacing() = Where;
			MakeDirty();
		}
		void Transform::SetPivot(Transform* Parent, Spacing* Pivot)
		{
			VI_DELETE(Spacing, Local);
			Root = Parent;
			Local = Pivot;

			if (Parent != nullptr)
				Parent->AddChild(this);
		}
		void Transform::SetRoot(Transform* Parent)
		{
			if (!CanRootBeApplied(Parent))
				return;

			if (Root != nullptr)
			{
				Specialize(Global);
				if (Parent != nullptr)
				{
					VI_DELETE(Spacing, Local);
					Local = nullptr;
				}

				for (auto It = Root->Childs.begin(); It != Root->Childs.end(); ++It)
				{
					if ((*It) == this)
					{
						Root->Childs.erase(It);
						break;
					}
				}
			}

			Root = Parent;
			if (Root != nullptr)
			{
				Root->AddChild(this);
				if (!Local)
					Local = VI_NEW(Spacing);

				*Local = Global;
				Localize(*Local);
			}
		}
		void Transform::GetBounds(Matrix4x4& World, Vector3& Min, Vector3& Max)
		{
			Transform::Spacing& Space = GetSpacing();
			Vector3 Scale = (Max - Min).Abs();
			Vector3 Radius = Scale * Space.Scale * 0.5f;
			Vector3 Top = Radius.Rotate(0, Space.Rotation);
			Vector3 Left = Radius.InvX().Rotate(0, Space.Rotation);
			Vector3 Right = Radius.InvY().Rotate(0, Space.Rotation);
			Vector3 Bottom = Radius.InvZ().Rotate(0, Space.Rotation);
			Vector3 Points[8] =
			{
				-Top, Top, -Left, Left,
				-Right, Right, -Bottom, Bottom
			};
			Vector3 Lower = Points[0];
			Vector3 Upper = Points[1];

			for (size_t i = 0; i < 8; ++i)
			{
				auto& Point = Points[i];
				if (Point.X > Upper.X)
					Upper.X = Point.X;
				if (Point.Y > Upper.Y)
					Upper.Y = Point.Y;
				if (Point.Z > Upper.Z)
					Upper.Z = Point.Z;

				if (Point.X < Lower.X)
					Lower.X = Point.X;
				if (Point.Y < Lower.Y)
					Lower.Y = Point.Y;
				if (Point.Z < Lower.Z)
					Lower.Z = Point.Z;
			}

			Min = Space.Position + Lower;
			Max = Space.Position + Upper;
			World = GetBias();
		}
		bool Transform::HasRoot(const Transform* Target) const
		{
			VI_ASSERT(Target != nullptr, "target should be set");
			Compute::Transform* UpperRoot = Root;
			while (UpperRoot != nullptr)
			{
				if (UpperRoot == Target)
					return true;

				UpperRoot = UpperRoot->GetRoot();
			}

			return false;
		}
		bool Transform::HasChild(Transform* Target) const
		{
			VI_ASSERT(Target != nullptr, "target should be set");
			for (auto& Child : Childs)
			{
				if (Child == Target)
					return true;

				if (Child->HasChild(Target))
					return true;
			}

			return false;
		}
		bool Transform::HasScaling() const
		{
			return Scaling;
		}
		bool Transform::CanRootBeApplied(Transform* Parent) const
		{
			if ((!Root && !Parent) || Root == Parent)
				return false;

			if (Parent == this)
				return false;

			if (Parent && Parent->HasRoot(this))
				return false;

			return true;
		}
		bool Transform::IsDirty() const
		{
			return Dirty;
		}
		const Matrix4x4& Transform::GetBias() const
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				return Temporary;

			return Global.Offset;
		}
		const Matrix4x4& Transform::GetBiasUnscaled() const
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset;

			return Temporary;
		}
		const Vector3& Transform::GetPosition() const
		{
			return Global.Position;
		}
		const Vector3& Transform::GetRotation() const
		{
			return Global.Rotation;
		}
		const Vector3& Transform::GetScale() const
		{
			return Global.Scale;
		}
		Vector3 Transform::Forward() const
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset.Forward();

			return Global.Offset.Forward();
		}
		Vector3 Transform::Right() const
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset.Right();

			return Global.Offset.Right();
		}
		Vector3 Transform::Up() const
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset.Up();

			return Global.Offset.Up();
		}
		Transform::Spacing& Transform::GetSpacing()
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				return *Local;

			return Global;
		}
		Transform::Spacing& Transform::GetSpacing(Positioning Space)
		{
			VI_ASSERT(!Root || Local != nullptr, "corrupted root transform");
			if (Space == Positioning::Local)
				return *Local;

			return Global;
		}
		Transform* Transform::GetRoot() const
		{
			return Root;
		}
		Transform* Transform::GetUpperRoot() const
		{
			Compute::Transform* UpperRoot = Root;
			while (UpperRoot != nullptr)
				UpperRoot = UpperRoot->GetRoot();

			return nullptr;
		}
		Transform* Transform::GetChild(size_t Child) const
		{
			VI_ASSERT(Child < Childs.size(), "index outside of range");
			return Childs[Child];
		}
		size_t Transform::GetChildsCount() const
		{
			return Childs.size();
		}
		Core::Vector<Transform*>& Transform::GetChilds()
		{
			return Childs;
		}

		bool Cosmos::Node::IsLeaf() const
		{
			return (Left == NULL_NODE);
		}

		Cosmos::Cosmos(size_t DefaultSize) noexcept
		{
			Root = NULL_NODE;
			NodeCount = 0;
			NodeCapacity = DefaultSize;
			Nodes.resize(NodeCapacity);

			for (size_t i = 0; i < NodeCapacity - 1; i++)
			{
				auto& Node = Nodes[i];
				Node.Next = i + 1;
				Node.Height = -1;
			}

			auto& Node = Nodes[NodeCapacity - 1];
			Node.Next = NULL_NODE;
			Node.Height = -1;
			FreeList = 0;
		}
		void Cosmos::FreeNode(size_t NodeIndex)
		{
			VI_ASSERT(NodeIndex < NodeCapacity, "outside of borders");
			VI_ASSERT(NodeCount > 0, "there must be at least one node");

			auto& Node = Nodes[NodeIndex];
			Node.Next = FreeList;
			Node.Height = -1;
			FreeList = NodeIndex;
			NodeCount--;
		}
		void Cosmos::InsertItem(void* Item, const Vector3& Lower, const Vector3& Upper)
		{
			size_t NodeIndex = AllocateNode();
			auto& Node = Nodes[NodeIndex];
			Node.Bounds = Bounding(Lower, Upper);
			Node.Height = 0;
			Node.Item = Item;
			InsertLeaf(NodeIndex);

			Items.insert(Core::UnorderedMap<void*, size_t>::value_type(Item, NodeIndex));
		}
		void Cosmos::RemoveItem(void* Item)
		{
			auto It = Items.find(Item);
			if (It == Items.end())
				return;

			size_t NodeIndex = It->second;
			Items.erase(It);

			VI_ASSERT(NodeIndex < NodeCapacity, "outside of borders");
			VI_ASSERT(Nodes[NodeIndex].IsLeaf(), "cannot remove root node");

			RemoveLeaf(NodeIndex);
			FreeNode(NodeIndex);
		}
		void Cosmos::InsertLeaf(size_t LeafIndex)
		{
			if (Root == NULL_NODE)
			{
				Root = LeafIndex;
				Nodes[Root].Parent = NULL_NODE;
				return;
			}

			size_t NextIndex = Root;
			Bounding LeafBounds = Nodes[LeafIndex].Bounds;
			Bounding NextBounds;

			while (!Nodes[NextIndex].IsLeaf())
			{
				auto& Next = Nodes[NextIndex];
				auto& Left = Nodes[Next.Left];
				auto& Right = Nodes[Next.Right];

				NextBounds.Merge(LeafBounds, Next.Bounds);
				float BaseCost = 2.0f * (float)NextBounds.Volume;
				float ParentCost = 2.0f * (float)(NextBounds.Volume - Next.Bounds.Volume);

				NextBounds.Merge(LeafBounds, Left.Bounds);
				float LeftCost = (NextBounds.Volume - (Left.IsLeaf() ? 0.0f : Left.Bounds.Volume)) + ParentCost;

				NextBounds.Merge(LeafBounds, Right.Bounds);
				float RightCost = (NextBounds.Volume - (Right.IsLeaf() ? 0.0f : Right.Bounds.Volume)) + ParentCost;

				if ((BaseCost < LeftCost) && (BaseCost < RightCost))
					break;

				if (LeftCost < RightCost)
					NextIndex = Next.Left;
				else
					NextIndex = Next.Right;
			}

			size_t NewParentIndex = AllocateNode();
			auto& Leaf = Nodes[LeafIndex];
			auto& Sibling = Nodes[NextIndex];
			auto& NewParent = Nodes[NewParentIndex];

			size_t OldParentIndex = Sibling.Parent;
			NewParent.Parent = OldParentIndex;
			NewParent.Bounds.Merge(LeafBounds, Sibling.Bounds);
			NewParent.Height = Sibling.Height + 1;

			if (OldParentIndex != NULL_NODE)
			{
				auto& OldParent = Nodes[OldParentIndex];
				if (OldParent.Left == NextIndex)
					OldParent.Left = NewParentIndex;
				else
					OldParent.Right = NewParentIndex;

				NewParent.Left = NextIndex;
				NewParent.Right = LeafIndex;
				Sibling.Parent = NewParentIndex;
				Leaf.Parent = NewParentIndex;
			}
			else
			{
				NewParent.Left = NextIndex;
				NewParent.Right = LeafIndex;
				Sibling.Parent = NewParentIndex;
				Leaf.Parent = NewParentIndex;
				Root = NewParentIndex;
			}

			NextIndex = Leaf.Parent;
			while (NextIndex != NULL_NODE)
			{
				NextIndex = Balance(NextIndex);
				auto& Next = Nodes[NextIndex];
				auto& Left = Nodes[Next.Left];
				auto& Right = Nodes[Next.Right];

				Next.Height = 1 + std::max(Left.Height, Right.Height);
				Next.Bounds.Merge(Left.Bounds, Right.Bounds);
				NextIndex = Next.Parent;
			}
		}
		void Cosmos::RemoveLeaf(size_t LeafIndex)
		{
			if (LeafIndex == Root)
			{
				Root = NULL_NODE;
				return;
			}

			size_t ParentIndex = Nodes[LeafIndex].Parent;
			auto& Parent = Nodes[ParentIndex];

			size_t SiblingIndex = (Parent.Left == LeafIndex ? Parent.Right : Parent.Left);
			auto& Sibling = Nodes[SiblingIndex];

			if (Parent.Parent != NULL_NODE)
			{
				auto& UpperParent = Nodes[Parent.Parent];
				if (UpperParent.Left == ParentIndex)
					UpperParent.Left = SiblingIndex;
				else
					UpperParent.Right = SiblingIndex;

				Sibling.Parent = Parent.Parent;
				FreeNode(ParentIndex);

				size_t NextIndex = Parent.Parent;
				while (NextIndex != NULL_NODE)
				{
					NextIndex = Balance(NextIndex);
					auto& Next = Nodes[NextIndex];
					auto& Left = Nodes[Next.Left];
					auto& Right = Nodes[Next.Right];

					Next.Bounds.Merge(Left.Bounds, Right.Bounds);
					Next.Height = 1 + std::max(Left.Height, Right.Height);
					NextIndex = Next.Parent;
				}
			}
			else
			{
				Root = SiblingIndex;
				Sibling.Parent = NULL_NODE;
				FreeNode(ParentIndex);
			}
		}
		void Cosmos::Reserve(size_t Size)
		{
			if (Size < Nodes.capacity())
				return;

			Items.reserve(Size);
			Nodes.reserve(Size);
		}
		void Cosmos::Clear()
		{
			auto It = Items.begin();
			while (It != Items.end())
			{
				size_t NodeIndex = It->second;
				VI_ASSERT(NodeIndex < NodeCapacity, "outside of borders");
				VI_ASSERT(Nodes[NodeIndex].IsLeaf(), "cannot remove root node");

				RemoveLeaf(NodeIndex);
				FreeNode(NodeIndex);
				It++;
			}
			Items.clear();
		}
		bool Cosmos::UpdateItem(void* Item, const Vector3& Lower, const Vector3& Upper, bool Always)
		{
			auto It = Items.find(Item);
			if (It == Items.end())
				return false;

			auto& Next = Nodes[It->second];
			if (!Always)
			{
				Bounding Bounds(Lower, Upper);
				if (Next.Bounds.Contains(Bounds))
					return true;

				RemoveLeaf(It->second);
				Next.Bounds = Bounds;
			}
			else
			{
				RemoveLeaf(It->second);
				Next.Bounds = Bounding(Lower, Upper);
			}

			InsertLeaf(It->second);
			return true;
		}
		const Bounding& Cosmos::GetArea(void* Item)
		{
			auto It = Items.find(Item);
			if (It != Items.end())
				return Nodes[It->second].Bounds;

			return Nodes[Root].Bounds;
		}
		size_t Cosmos::AllocateNode()
		{
			if (FreeList == NULL_NODE)
			{
				VI_ASSERT(NodeCount == NodeCapacity, "invalid capacity");

				NodeCapacity *= 2;
				Nodes.resize(NodeCapacity);

				for (size_t i = NodeCount; i < NodeCapacity - 1; i++)
				{
					auto& Next = Nodes[i];
					Next.Next = i + 1;
					Next.Height = -1;
				}

				Nodes[NodeCapacity - 1].Next = NULL_NODE;
				Nodes[NodeCapacity - 1].Height = -1;
				FreeList = NodeCount;
			}

			size_t NodeIndex = FreeList;
			auto& Node = Nodes[NodeIndex];
			FreeList = Node.Next;
			Node.Parent = NULL_NODE;
			Node.Left = NULL_NODE;
			Node.Right = NULL_NODE;
			Node.Height = 0;
			NodeCount++;

			return NodeIndex;
		}
		size_t Cosmos::Balance(size_t NodeIndex)
		{
			VI_ASSERT(NodeIndex != NULL_NODE, "node should not be null");

			auto& Next = Nodes[NodeIndex];
			if (Next.IsLeaf() || (Next.Height < 2))
				return NodeIndex;

			VI_ASSERT(Next.Left < NodeCapacity, "left outside of borders");
			VI_ASSERT(Next.Right < NodeCapacity, "right outside of borders");

			size_t LeftIndex = Next.Left;
			size_t RightIndex = Next.Right;
			auto& Left = Nodes[LeftIndex];
			auto& Right = Nodes[RightIndex];

			int Balance = Right.Height - Left.Height;
			if (Balance > 1)
			{
				VI_ASSERT(Right.Left < NodeCapacity, "subleft outside of borders");
				VI_ASSERT(Right.Right < NodeCapacity, "subright outside of borders");

				size_t RightLeftIndex = Right.Left;
				size_t RightRightIndex = Right.Right;
				auto& RightLeft = Nodes[RightLeftIndex];
				auto& RightRight = Nodes[RightRightIndex];

				Right.Left = NodeIndex;
				Right.Parent = Next.Parent;
				Next.Parent = RightIndex;

				if (Right.Parent != NULL_NODE)
				{
					if (Nodes[Right.Parent].Left != NodeIndex)
					{
						VI_ASSERT(Nodes[Right.Parent].Right == NodeIndex, "invalid right spacing");
						Nodes[Right.Parent].Right = RightIndex;
					}
					else
						Nodes[Right.Parent].Left = RightIndex;
				}
				else
					Root = RightIndex;

				if (RightLeft.Height > RightRight.Height)
				{
					Right.Right = RightLeftIndex;
					Next.Right = RightRightIndex;
					RightRight.Parent = NodeIndex;
					Next.Bounds.Merge(Left.Bounds, RightRight.Bounds);
					Right.Bounds.Merge(Next.Bounds, RightLeft.Bounds);

					Next.Height = 1 + std::max(Left.Height, RightRight.Height);
					Right.Height = 1 + std::max(Next.Height, RightLeft.Height);
				}
				else
				{
					Right.Right = RightRightIndex;
					Next.Right = RightLeftIndex;
					RightLeft.Parent = NodeIndex;
					Next.Bounds.Merge(Left.Bounds, RightLeft.Bounds);
					Right.Bounds.Merge(Next.Bounds, RightRight.Bounds);

					Next.Height = 1 + std::max(Left.Height, RightLeft.Height);
					Right.Height = 1 + std::max(Next.Height, RightRight.Height);
				}

				return RightIndex;
			}
			else if (Balance < -1)
			{
				VI_ASSERT(Left.Left < NodeCapacity, "subleft outside of borders");
				VI_ASSERT(Left.Right < NodeCapacity, "subright outside of borders");

				size_t LeftLeftIndex = Left.Left;
				size_t LeftRightIndex = Left.Right;
				auto& LeftLeft = Nodes[LeftLeftIndex];
				auto& LeftRight = Nodes[LeftRightIndex];

				Left.Left = NodeIndex;
				Left.Parent = Next.Parent;
				Next.Parent = LeftIndex;

				if (Left.Parent != NULL_NODE)
				{
					if (Nodes[Left.Parent].Left != NodeIndex)
					{
						VI_ASSERT(Nodes[Left.Parent].Right == NodeIndex, "invalid left spacing");
						Nodes[Left.Parent].Right = LeftIndex;
					}
					else
						Nodes[Left.Parent].Left = LeftIndex;
				}
				else
					Root = LeftIndex;

				if (LeftLeft.Height > LeftRight.Height)
				{
					Left.Right = LeftLeftIndex;
					Next.Left = LeftRightIndex;
					LeftRight.Parent = NodeIndex;
					Next.Bounds.Merge(Right.Bounds, LeftRight.Bounds);
					Left.Bounds.Merge(Next.Bounds, LeftLeft.Bounds);
					Next.Height = 1 + std::max(Right.Height, LeftRight.Height);
					Left.Height = 1 + std::max(Next.Height, LeftLeft.Height);
				}
				else
				{
					Left.Right = LeftRightIndex;
					Next.Left = LeftLeftIndex;
					LeftLeft.Parent = NodeIndex;
					Next.Bounds.Merge(Right.Bounds, LeftLeft.Bounds);
					Left.Bounds.Merge(Next.Bounds, LeftRight.Bounds);
					Next.Height = 1 + std::max(Right.Height, LeftLeft.Height);
					Left.Height = 1 + std::max(Next.Height, LeftRight.Height);
				}

				return LeftIndex;
			}

			return NodeIndex;
		}
		size_t Cosmos::ComputeHeight() const
		{
			return ComputeHeight(Root);
		}
		size_t Cosmos::ComputeHeight(size_t NodeIndex) const
		{
			VI_ASSERT(NodeIndex < NodeCapacity, "outside of borders");

			auto& Next = Nodes[NodeIndex];
			if (Next.IsLeaf())
				return 0;

			size_t Height1 = ComputeHeight(Next.Left);
			size_t Height2 = ComputeHeight(Next.Right);
			return 1 + std::max(Height1, Height2);
		}
		size_t Cosmos::GetNodesCount() const
		{
			return Nodes.size();
		}
		size_t Cosmos::GetHeight() const
		{
			return Root == NULL_NODE ? 0 : Nodes[Root].Height;
		}
		size_t Cosmos::GetMaxBalance() const
		{
			size_t MaxBalance = 0;
			for (size_t i = 0; i < NodeCapacity; i++)
			{
				auto& Next = Nodes[i];
				if (Next.Height <= 1)
					continue;

				VI_ASSERT(!Next.IsLeaf(), "node should be leaf");
				size_t Balance = std::abs(Nodes[Next.Left].Height - Nodes[Next.Right].Height);
				MaxBalance = std::max(MaxBalance, Balance);
			}

			return MaxBalance;
		}
		size_t Cosmos::GetRoot() const
		{
			return Root;
		}
		const Core::UnorderedMap<void*, size_t>& Cosmos::GetItems() const
		{
			return Items;
		}
		const Core::Vector<Cosmos::Node>& Cosmos::GetNodes() const
		{
			return Nodes;
		}
		const Cosmos::Node& Cosmos::GetRootNode() const
		{
			VI_ASSERT(Root < Nodes.size(), "index out of range");
			return Nodes[Root];
		}
		const Cosmos::Node& Cosmos::GetNode(size_t Id) const
		{
			VI_ASSERT(Id < Nodes.size(), "index out of range");
			return Nodes[Id];
		}
		float Cosmos::GetVolumeRatio() const
		{
			if (Root == NULL_NODE)
				return 0.0;

			float RootArea = Nodes[Root].Bounds.Volume;
			float TotalArea = 0.0;

			for (size_t i = 0; i < NodeCapacity; i++)
			{
				auto& Next = Nodes[i];
				if (Next.Height >= 0)
					TotalArea += Next.Bounds.Volume;
			}

			return TotalArea / RootArea;
		}
		bool Cosmos::IsNull(size_t Id) const
		{
			return Id == NULL_NODE;
		}
		bool Cosmos::Empty() const
		{
			return Items.empty();
		}

		HullShape::HullShape(Core::Vector<Vertex>&& NewVertices, Core::Vector<int>&& NewIndices) noexcept : Vertices(std::move(NewVertices)), Indices(std::move(NewIndices)), Shape(nullptr)
		{
#ifdef VI_BULLET3
			Shape = VI_NEW(btConvexHullShape);
			btConvexHullShape* Hull = (btConvexHullShape*)Shape;
			for (auto& Item : Vertices)
				Hull->addPoint(btVector3(Item.PositionX, Item.PositionY, Item.PositionZ), false);

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);
#endif
		}
		HullShape::HullShape(Core::Vector<Vertex>&& NewVertices) noexcept : Vertices(std::move(NewVertices)), Shape(nullptr)
		{
#ifdef VI_BULLET3
			Shape = VI_NEW(btConvexHullShape);
			btConvexHullShape* Hull = (btConvexHullShape*)Shape;
			Indices.reserve(Vertices.size());

			for (auto& Item : Vertices)
			{
				Hull->addPoint(btVector3(Item.PositionX, Item.PositionY, Item.PositionZ), false);
				Indices.push_back((int)Indices.size());
			}

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);
#endif
		}
		HullShape::HullShape(btCollisionShape* From) noexcept : Shape(nullptr)
		{
#ifdef VI_BULLET3
			VI_ASSERT(From != nullptr, "shape should be set");
			VI_ASSERT(From->getShapeType() == (int)Shape::Convex_Hull, "shape type should be convex hull");

			btConvexHullShape* Hull = VI_NEW(btConvexHullShape);
			btConvexHullShape* Base = (btConvexHullShape*)From;
			Vertices.reserve((size_t)Base->getNumPoints());
			Indices.reserve((size_t)Base->getNumPoints());

			for (size_t i = 0; i < (size_t)Base->getNumPoints(); i++)
			{
				auto& Position = *(Base->getUnscaledPoints() + i);
				Hull->addPoint(Position, false);
				Vertices.push_back({ Position.x(), Position.y(), Position.z() });
				Indices.push_back((int)i);
			}

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);
#endif
		}
		HullShape::~HullShape() noexcept
		{
#ifdef VI_BULLET3
			VI_DELETE(btCollisionShape, Shape);
#endif
		}
		const Core::Vector<Vertex>& HullShape::GetVertices() const
		{
			return Vertices;
		}
		const Core::Vector<int>& HullShape::GetIndices() const
		{
			return Indices;
		}
		btCollisionShape* HullShape::GetShape() const
		{
			return Shape;
		}

		RigidBody::RigidBody(Simulator* Refer, const Desc& I) noexcept : Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
			VI_ASSERT(Initial.Shape, "collision shape should be set");
			VI_ASSERT(Engine != nullptr, "simulator should be set");
#ifdef VI_BULLET3
			Initial.Shape = Engine->ReuseShape(Initial.Shape);
			if (!Initial.Shape)
			{
				Initial.Shape = Engine->TryCloneShape(I.Shape);
				if (!Initial.Shape)
					return;
			}

			btVector3 LocalInertia(0, 0, 0);
			Initial.Shape->setLocalScaling(V3_TO_BT(Initial.Scale));
			if (Initial.Mass > 0)
				Initial.Shape->calculateLocalInertia(Initial.Mass, LocalInertia);

			btQuaternion Rotation;
			Rotation.setEulerZYX(Initial.Rotation.Z, Initial.Rotation.Y, Initial.Rotation.X);

			btTransform BtTransform(Rotation, btVector3(Initial.Position.X, Initial.Position.Y, Initial.Position.Z));
			btRigidBody::btRigidBodyConstructionInfo Info(Initial.Mass, VI_NEW(btDefaultMotionState, BtTransform), Initial.Shape, LocalInertia);
			Instance = VI_NEW(btRigidBody, Info);
			Instance->setUserPointer(this);
			Instance->setGravity(Engine->GetWorld()->getGravity());

			if (Initial.Anticipation > 0 && Initial.Mass > 0)
			{
				Instance->setCcdMotionThreshold(Initial.Anticipation);
				Instance->setCcdSweptSphereRadius(Initial.Scale.Length() / 15.0f);
			}

			if (Instance->getWorldArrayIndex() == -1)
				Engine->GetWorld()->addRigidBody(Instance);
#endif
		}
		RigidBody::~RigidBody() noexcept
		{
#ifdef VI_BULLET3
			if (!Instance)
				return;

			int Constraints = Instance->getNumConstraintRefs();
			for (int i = 0; i < Constraints; i++)
			{
				btTypedConstraint* Constraint = Instance->getConstraintRef(i);
				if (Constraint != nullptr)
				{
					void* Ptr = Constraint->getUserConstraintPtr();
					if (Ptr != nullptr)
					{
						btTypedConstraintType Type = Constraint->getConstraintType();
						switch (Type)
						{
							case SLIDER_CONSTRAINT_TYPE:
								if (((SConstraint*)Ptr)->First == Instance)
									((SConstraint*)Ptr)->First = nullptr;
								else if (((SConstraint*)Ptr)->Second == Instance)
									((SConstraint*)Ptr)->Second = nullptr;
								break;
							default:
								break;
						}
					}

					Instance->removeConstraintRef(Constraint);
					Constraints--; i--;
				}
			}

			if (Instance->getMotionState())
			{
				btMotionState* Object = Instance->getMotionState();
				VI_DELETE(btMotionState, Object);
				Instance->setMotionState(nullptr);
			}

			Instance->setCollisionShape(nullptr);
			Instance->setUserPointer(nullptr);
			if (Instance->getWorldArrayIndex() >= 0)
				Engine->GetWorld()->removeRigidBody(Instance);

			if (Initial.Shape)
				Engine->FreeShape(&Initial.Shape);

			VI_DELETE(btRigidBody, Instance);
#endif
		}
		RigidBody* RigidBody::Copy()
		{
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");

			Desc I(Initial);
			I.Position = GetPosition();
			I.Rotation = GetRotation();
			I.Scale = GetScale();
			I.Mass = GetMass();
			I.Shape = Engine->TryCloneShape(I.Shape);

			RigidBody* Target = new RigidBody(Engine, I);
			Target->SetSpinningFriction(GetSpinningFriction());
			Target->SetContactDamping(GetContactDamping());
			Target->SetContactStiffness(GetContactStiffness());
			Target->SetActivationState(GetActivationState());
			Target->SetAngularDamping(GetAngularDamping());
			Target->SetAngularSleepingThreshold(GetAngularSleepingThreshold());
			Target->SetFriction(GetFriction());
			Target->SetRestitution(GetRestitution());
			Target->SetHitFraction(GetHitFraction());
			Target->SetLinearDamping(GetLinearDamping());
			Target->SetLinearSleepingThreshold(GetLinearSleepingThreshold());
			Target->SetCcdMotionThreshold(GetCcdMotionThreshold());
			Target->SetCcdSweptSphereRadius(GetCcdSweptSphereRadius());
			Target->SetContactProcessingThreshold(GetContactProcessingThreshold());
			Target->SetDeactivationTime(GetDeactivationTime());
			Target->SetRollingFriction(GetRollingFriction());
			Target->SetAngularFactor(GetAngularFactor());
			Target->SetAnisotropicFriction(GetAnisotropicFriction());
			Target->SetGravity(GetGravity());
			Target->SetLinearFactor(GetLinearFactor());
			Target->SetLinearVelocity(GetLinearVelocity());
			Target->SetAngularVelocity(GetAngularVelocity());
			Target->SetCollisionFlags(GetCollisionFlags());

			return Target;
		}
		void RigidBody::Push(const Vector3& Velocity)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->applyCentralImpulse(V3_TO_BT(Velocity));
#endif
		}
		void RigidBody::Push(const Vector3& Velocity, const Vector3& Torque)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->applyCentralImpulse(V3_TO_BT(Velocity));
			Instance->applyTorqueImpulse(V3_TO_BT(Torque));
#endif
		}
		void RigidBody::Push(const Vector3& Velocity, const Vector3& Torque, const Vector3& Center)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->applyImpulse(V3_TO_BT(Velocity), V3_TO_BT(Center));
			Instance->applyTorqueImpulse(V3_TO_BT(Torque));
#endif
		}
		void RigidBody::PushKinematic(const Vector3& Velocity)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");

			btTransform Offset;
			Instance->getMotionState()->getWorldTransform(Offset);

			btVector3 Origin = Offset.getOrigin();
			Origin.setX(Origin.getX() + Velocity.X);
			Origin.setY(Origin.getY() + Velocity.Y);
			Origin.setZ(Origin.getZ() + Velocity.Z);

			Offset.setOrigin(Origin);
			Instance->getMotionState()->setWorldTransform(Offset);
#endif
		}
		void RigidBody::PushKinematic(const Vector3& Velocity, const Vector3& Torque)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");

			btTransform Offset;
			Instance->getMotionState()->getWorldTransform(Offset);

			btScalar X, Y, Z;
			Offset.getRotation().getEulerZYX(Z, Y, X);

			Vector3 Rotation(-X, -Y, Z);
			Offset.getBasis().setEulerZYX(Rotation.X + Torque.X, Rotation.Y + Torque.Y, Rotation.Z + Torque.Z);

			btVector3 Origin = Offset.getOrigin();
			Origin.setX(Origin.getX() + Velocity.X);
			Origin.setY(Origin.getY() + Velocity.Y);
			Origin.setZ(Origin.getZ() + Velocity.Z);

			Offset.setOrigin(Origin);
			Instance->getMotionState()->setWorldTransform(Offset);
#endif
		}
		void RigidBody::Synchronize(Transform* Transform, bool Kinematic)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btTransform& Base = Instance->getWorldTransform();
			if (!Kinematic)
			{
				btScalar X, Y, Z;
				const btVector3& Position = Base.getOrigin();
				const btVector3& Scale = Instance->getCollisionShape()->getLocalScaling();
				Base.getRotation().getEulerZYX(Z, Y, X);
				Transform->SetPosition(BT_TO_V3(Position));
				Transform->SetRotation(Vector3(X, Y, Z));
				Transform->SetScale(BT_TO_V3(Scale));
			}
			else
			{
				Transform::Spacing& Space = Transform->GetSpacing(Positioning::Global);
				Base.setOrigin(V3_TO_BT(Space.Position));
				Base.getBasis().setEulerZYX(Space.Rotation.X, Space.Rotation.Y, Space.Rotation.Z);
				Instance->getCollisionShape()->setLocalScaling(V3_TO_BT(Space.Scale));
			}
#endif
		}
		void RigidBody::SetActivity(bool Active)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			if (GetActivationState() == MotionState::Disable_Deactivation)
				return;

			if (Active)
			{
				Instance->forceActivationState((int)MotionState::Active);
				Instance->activate(true);
			}
			else
				Instance->forceActivationState((int)MotionState::Deactivation_Needed);
#endif
		}
		void RigidBody::SetAsGhost()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
#endif
		}
		void RigidBody::SetAsNormal()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCollisionFlags(0);
#endif
		}
		void RigidBody::SetSelfPointer()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setUserPointer(this);
#endif
		}
		void RigidBody::SetWorldTransform(btTransform* Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			VI_ASSERT(Value != nullptr, "transform should be set");
			Instance->setWorldTransform(*Value);
#endif
		}
		void RigidBody::SetActivationState(MotionState Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->forceActivationState((int)Value);
#endif
		}
		void RigidBody::SetAngularDamping(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setDamping(Instance->getLinearDamping(), Value);
#endif
		}
		void RigidBody::SetAngularSleepingThreshold(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setSleepingThresholds(Instance->getLinearSleepingThreshold(), Value);
#endif
		}
		void RigidBody::SetFriction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setFriction(Value);
#endif
		}
		void RigidBody::SetRestitution(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setRestitution(Value);
#endif
		}
		void RigidBody::SetSpinningFriction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setSpinningFriction(Value);
#endif
		}
		void RigidBody::SetContactStiffness(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setContactStiffnessAndDamping(Value, Instance->getContactDamping());
#endif
		}
		void RigidBody::SetContactDamping(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setContactStiffnessAndDamping(Instance->getContactStiffness(), Value);
#endif
		}
		void RigidBody::SetHitFraction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setHitFraction(Value);
#endif
		}
		void RigidBody::SetLinearDamping(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setDamping(Value, Instance->getAngularDamping());
#endif
		}
		void RigidBody::SetLinearSleepingThreshold(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setSleepingThresholds(Value, Instance->getAngularSleepingThreshold());
#endif
		}
		void RigidBody::SetCcdMotionThreshold(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCcdMotionThreshold(Value);
#endif
		}
		void RigidBody::SetCcdSweptSphereRadius(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCcdSweptSphereRadius(Value);
#endif
		}
		void RigidBody::SetContactProcessingThreshold(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setContactProcessingThreshold(Value);
#endif
		}
		void RigidBody::SetDeactivationTime(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setDeactivationTime(Value);
#endif
		}
		void RigidBody::SetRollingFriction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setRollingFriction(Value);
#endif
		}
		void RigidBody::SetAngularFactor(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setAngularFactor(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetAnisotropicFriction(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setAnisotropicFriction(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetGravity(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setGravity(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetLinearFactor(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setLinearFactor(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetLinearVelocity(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setLinearVelocity(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetAngularVelocity(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setAngularVelocity(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetCollisionShape(btCollisionShape* Shape, Transform* Transform)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btCollisionShape* Collision = Instance->getCollisionShape();
			VI_DELETE(btCollisionShape, Collision);

			Instance->setCollisionShape(Shape);
			if (Transform)
				Synchronize(Transform, true);
#endif
		}
		void RigidBody::SetMass(float Mass)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr && Engine != nullptr, "rigidbody should be initialized");
			btVector3 Inertia = Engine->GetWorld()->getGravity();
			if (Instance->getWorldArrayIndex() >= 0)
				Engine->GetWorld()->removeRigidBody(Instance);

			Instance->setGravity(Inertia);
			Instance->getCollisionShape()->calculateLocalInertia(Mass, Inertia);
			Instance->setMassProps(Mass, Inertia);
			if (Instance->getWorldArrayIndex() == -1)
				Engine->GetWorld()->addRigidBody(Instance);

			SetActivity(true);
#endif
		}
		void RigidBody::SetCollisionFlags(size_t Flags)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCollisionFlags((int)Flags);
#endif
		}
		MotionState RigidBody::GetActivationState() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return (MotionState)Instance->getActivationState();
#else
			return MotionState::Island_Sleeping;
#endif
		}
		Shape RigidBody::GetCollisionShapeType() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr && Instance->getCollisionShape() != nullptr, "rigidbody should be initialized");
			return (Shape)Instance->getCollisionShape()->getShapeType();
#else
			return Shape::Invalid;
#endif
		}
		Vector3 RigidBody::GetAngularFactor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getAngularFactor();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetAnisotropicFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getAnisotropicFriction();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetGravity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getGravity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetLinearFactor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getLinearFactor();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetLinearVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getLinearVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetAngularVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getAngularVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetScale() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr && Instance->getCollisionShape() != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getCollisionShape()->getLocalScaling();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetPosition() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btVector3 Value = Instance->getWorldTransform().getOrigin();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetRotation() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			btScalar X, Y, Z;
			Instance->getWorldTransform().getBasis().getEulerZYX(Z, Y, X);
			return Vector3(-X, -Y, Z);
#else
			return 0;
#endif
		}
		btTransform* RigidBody::GetWorldTransform() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return &Instance->getWorldTransform();
#else
			return nullptr;
#endif
		}
		btCollisionShape* RigidBody::GetCollisionShape() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getCollisionShape();
#else
			return nullptr;
#endif
		}
		btRigidBody* RigidBody::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool RigidBody::IsGhost() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return (Instance->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
#else
			return false;
#endif
		}
		bool RigidBody::IsActive() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->isActive();
#else
			return false;
#endif
		}
		bool RigidBody::IsStatic() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->isStaticObject();
#else
			return true;
#endif
		}
		bool RigidBody::IsColliding() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->hasContactResponse();
#else
			return false;
#endif
		}
		float RigidBody::GetSpinningFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getSpinningFriction();
#else
			return 0;
#endif
		}
		float RigidBody::GetContactStiffness() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getContactStiffness();
#else
			return 0;
#endif
		}
		float RigidBody::GetContactDamping() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getContactDamping();
#else
			return 0;
#endif
		}
		float RigidBody::GetAngularDamping() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getAngularDamping();
#else
			return 0;
#endif
		}
		float RigidBody::GetAngularSleepingThreshold() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getAngularSleepingThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getFriction();
#else
			return 0;
#endif
		}
		float RigidBody::GetRestitution() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getRestitution();
#else
			return 0;
#endif
		}
		float RigidBody::GetHitFraction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getHitFraction();
#else
			return 0;
#endif
		}
		float RigidBody::GetLinearDamping() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getLinearDamping();
#else
			return 0;
#endif
		}
		float RigidBody::GetLinearSleepingThreshold() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getLinearSleepingThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetCcdMotionThreshold() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getCcdMotionThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetCcdSweptSphereRadius() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getCcdSweptSphereRadius();
#else
			return 0;
#endif
		}
		float RigidBody::GetContactProcessingThreshold() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getContactProcessingThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetDeactivationTime() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getDeactivationTime();
#else
			return 0;
#endif
		}
		float RigidBody::GetRollingFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getRollingFriction();
#else
			return 0;
#endif
		}
		float RigidBody::GetMass() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			float Mass = Instance->getInvMass();
			return (Mass != 0.0f ? 1.0f / Mass : 0.0f);
#else
			return 0;
#endif
		}
		size_t RigidBody::GetCollisionFlags() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "rigidbody should be initialized");
			return Instance->getCollisionFlags();
#else
			return 0;
#endif
		}
		RigidBody::Desc& RigidBody::GetInitialState()
		{
			return Initial;
		}
		Simulator* RigidBody::GetSimulator() const
		{
			return Engine;
		}
		RigidBody* RigidBody::Get(btRigidBody* From)
		{
#ifdef VI_BULLET3
			VI_ASSERT(From != nullptr, "rigidbody should be initialized");
			return (RigidBody*)From->getUserPointer();
#else
			return nullptr;
#endif
		}

		SoftBody::SoftBody(Simulator* Refer, const Desc& I) noexcept : Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(Engine->HasSoftBodySupport(), "soft body should be supported");

			btQuaternion Rotation;
			Rotation.setEulerZYX(Initial.Rotation.Z, Initial.Rotation.Y, Initial.Rotation.X);

			btTransform BtTransform(Rotation, btVector3(Initial.Position.X, Initial.Position.Y, -Initial.Position.Z));
			btSoftRigidDynamicsWorld* World = (btSoftRigidDynamicsWorld*)Engine->GetWorld();
			btSoftBodyWorldInfo& Info = World->getWorldInfo();
			HullShape* Hull = Initial.Shape.Convex.Hull;

			if (Initial.Shape.Convex.Enabled && Hull != nullptr)
			{
				auto& Positions = Hull->GetVertices();
				Core::Vector<btScalar> Vertices;
				Vertices.resize(Positions.size() * 3);

				for (size_t i = 0; i < Hull->GetVertices().size(); i++)
				{
					const Vertex& V = Positions[i];
					Vertices[i * 3 + 0] = (btScalar)V.PositionX;
					Vertices[i * 3 + 1] = (btScalar)V.PositionY;
					Vertices[i * 3 + 2] = (btScalar)V.PositionZ;
				}

				auto& Indices = Hull->GetIndices();
				Instance = btSoftBodyHelpers::CreateFromTriMesh(Info, Vertices.data(), Indices.data(), (int)Indices.size() / 3, false);
			}
			else if (Initial.Shape.Ellipsoid.Enabled)
			{
				Instance = btSoftBodyHelpers::CreateEllipsoid(Info, V3_TO_BT(Initial.Shape.Ellipsoid.Center), V3_TO_BT(Initial.Shape.Ellipsoid.Radius), Initial.Shape.Ellipsoid.Count);
			}
			else if (Initial.Shape.Rope.Enabled)
			{
				int FixedAnchors = 0;
				if (Initial.Shape.Rope.StartFixed)
					FixedAnchors |= 1;

				if (Initial.Shape.Rope.EndFixed)
					FixedAnchors |= 2;

				Instance = btSoftBodyHelpers::CreateRope(Info, V3_TO_BT(Initial.Shape.Rope.Start), V3_TO_BT(Initial.Shape.Rope.End), Initial.Shape.Rope.Count, FixedAnchors);
			}
			else
			{
				int FixedCorners = 0;
				if (Initial.Shape.Patch.Corner00Fixed)
					FixedCorners |= 1;

				if (Initial.Shape.Patch.Corner01Fixed)
					FixedCorners |= 2;

				if (Initial.Shape.Patch.Corner10Fixed)
					FixedCorners |= 4;

				if (Initial.Shape.Patch.Corner11Fixed)
					FixedCorners |= 8;

				Instance = btSoftBodyHelpers::CreatePatch(Info, V3_TO_BT(Initial.Shape.Patch.Corner00), V3_TO_BT(Initial.Shape.Patch.Corner10), V3_TO_BT(Initial.Shape.Patch.Corner01), V3_TO_BT(Initial.Shape.Patch.Corner11), Initial.Shape.Patch.CountX, Initial.Shape.Patch.CountY, FixedCorners, Initial.Shape.Patch.GenerateDiagonals);
			}

			if (Initial.Anticipation > 0)
			{
				Instance->setCcdMotionThreshold(Initial.Anticipation);
				Instance->setCcdSweptSphereRadius(Initial.Scale.Length() / 15.0f);
			}

			SetConfig(Initial.Config);
			Instance->randomizeConstraints();
			Instance->setPose(true, true);
			Instance->getCollisionShape()->setMargin(0.04f);
			Instance->transform(BtTransform);
			Instance->setUserPointer(this);
			Instance->setTotalMass(100.0f, true);

			if (Instance->getWorldArrayIndex() == -1)
				World->addSoftBody(Instance);
#endif
		}
		SoftBody::~SoftBody() noexcept
		{
#ifdef VI_BULLET3
			if (!Instance)
				return;

			btSoftRigidDynamicsWorld* World = (btSoftRigidDynamicsWorld*)Engine->GetWorld();
			if (Instance->getWorldArrayIndex() >= 0)
				World->removeSoftBody(Instance);

			Instance->setUserPointer(nullptr);
			VI_DELETE(btSoftBody, Instance);
#endif
		}
		SoftBody* SoftBody::Copy()
		{
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");

			Desc I(Initial);
			I.Position = GetCenterPosition();
			I.Rotation = GetRotation();
			I.Scale = GetScale();

			SoftBody* Target = new SoftBody(Engine, I);
			Target->SetSpinningFriction(GetSpinningFriction());
			Target->SetContactDamping(GetContactDamping());
			Target->SetContactStiffness(GetContactStiffness());
			Target->SetActivationState(GetActivationState());
			Target->SetFriction(GetFriction());
			Target->SetRestitution(GetRestitution());
			Target->SetHitFraction(GetHitFraction());
			Target->SetCcdMotionThreshold(GetCcdMotionThreshold());
			Target->SetCcdSweptSphereRadius(GetCcdSweptSphereRadius());
			Target->SetContactProcessingThreshold(GetContactProcessingThreshold());
			Target->SetDeactivationTime(GetDeactivationTime());
			Target->SetRollingFriction(GetRollingFriction());
			Target->SetAnisotropicFriction(GetAnisotropicFriction());
			Target->SetWindVelocity(GetWindVelocity());
			Target->SetContactStiffnessAndDamping(GetContactStiffness(), GetContactDamping());
			Target->SetTotalMass(GetTotalMass());
			Target->SetRestLengthScale(GetRestLengthScale());
			Target->SetVelocity(GetLinearVelocity());

			return Target;
		}
		void SoftBody::Activate(bool Force)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->activate(Force);
#endif
		}
		void SoftBody::Synchronize(Transform* Transform, bool Kinematic)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Transform != nullptr, "transform should be set");
#ifdef VI_SIMD
			LOD_VAL(_r1, 0.0f); LOD_VAL(_r2, 0.0f);
			for (int i = 0; i < Instance->m_nodes.size(); i++)
			{
				auto& Node = Instance->m_nodes[i];
				_r2.store(Node.m_x.m_floats);
				_r1 += _r2;
			}

			_r1 /= (float)Instance->m_nodes.size();
			_r1.store_partial(3, (float*)&Center);
#else
			Center.Set(0);
			for (int i = 0; i < Instance->m_nodes.size(); i++)
			{
				auto& Node = Instance->m_nodes[i];
				Center.X += Node.m_x.x();
				Center.Y += Node.m_x.y();
				Center.Z += Node.m_x.z();
			}
			Center /= (float)Instance->m_nodes.size();
#endif
			if (!Kinematic)
			{
				btScalar X, Y, Z;
				Instance->getWorldTransform().getRotation().getEulerZYX(Z, Y, X);
				Transform->SetPosition(Center.InvZ());
				Transform->SetRotation(Vector3(X, Y, Z));
			}
			else
			{
				Vector3 Position = Transform->GetPosition().InvZ() - Center;
				if (Position.Length() > 0.005f)
					Instance->translate(V3_TO_BT(Position));
			}
#endif
		}
		void SoftBody::GetIndices(Core::Vector<int>* Result) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Result != nullptr, "result should be set");

			Core::UnorderedMap<btSoftBody::Node*, int> Nodes;
			for (int i = 0; i < Instance->m_nodes.size(); i++)
				Nodes.insert(std::make_pair(&Instance->m_nodes[i], i));

			for (int i = 0; i < Instance->m_faces.size(); i++)
			{
				btSoftBody::Face& Face = Instance->m_faces[i];
				for (unsigned int j = 0; j < 3; j++)
				{
					auto It = Nodes.find(Face.m_n[j]);
					if (It != Nodes.end())
						Result->push_back(It->second);
				}
			}
#endif
		}
		void SoftBody::GetVertices(Core::Vector<Vertex>* Result) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Result != nullptr, "result should be set");

			static size_t PositionX = offsetof(Compute::Vertex, PositionX);
			static size_t NormalX = offsetof(Compute::Vertex, NormalX);

			size_t Size = (size_t)Instance->m_nodes.size();
			if (Result->size() != Size)
			{
				if (Initial.Shape.Convex.Enabled)
					*Result = Initial.Shape.Convex.Hull->GetVertices();
				else
					Result->resize(Size);
			}

			for (size_t i = 0; i < Size; i++)
			{
				auto* Node = &Instance->m_nodes[(int)i]; Vertex& Position = Result->at(i);
				memcpy(&Position + PositionX, Node->m_x.m_floats, sizeof(float) * 3);
				memcpy(&Position + NormalX, Node->m_n.m_floats, sizeof(float) * 3);
			}
#endif
		}
		void SoftBody::GetBoundingBox(Vector3* Min, Vector3* Max) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");

			btVector3 bMin, bMax;
			Instance->getAabb(bMin, bMax);
			if (Min != nullptr)
				*Min = BT_TO_V3(bMin).InvZ();

			if (Max != nullptr)
				*Max = BT_TO_V3(bMax).InvZ();
#endif
		}
		void SoftBody::SetContactStiffnessAndDamping(float Stiffness, float Damping)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setContactStiffnessAndDamping(Stiffness, Damping);
#endif
		}
		void SoftBody::AddAnchor(int Node, RigidBody* Body, bool DisableCollisionBetweenLinkedBodies, float Influence)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Body != nullptr, "body should be set");
			Instance->appendAnchor(Node, Body->Get(), DisableCollisionBetweenLinkedBodies, Influence);
#endif
		}
		void SoftBody::AddAnchor(int Node, RigidBody* Body, const Vector3& LocalPivot, bool DisableCollisionBetweenLinkedBodies, float Influence)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Body != nullptr, "body should be set");
			Instance->appendAnchor(Node, Body->Get(), V3_TO_BT(LocalPivot), DisableCollisionBetweenLinkedBodies, Influence);
#endif
		}
		void SoftBody::AddForce(const Vector3& Force)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->addForce(V3_TO_BT(Force));
#endif
		}
		void SoftBody::AddForce(const Vector3& Force, int Node)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->addForce(V3_TO_BT(Force), Node);
#endif
		}
		void SoftBody::AddAeroForceToNode(const Vector3& WindVelocity, int NodeIndex)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->addAeroForceToNode(V3_TO_BT(WindVelocity), NodeIndex);
#endif
		}
		void SoftBody::AddAeroForceToFace(const Vector3& WindVelocity, int FaceIndex)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->addAeroForceToFace(V3_TO_BT(WindVelocity), FaceIndex);
#endif
		}
		void SoftBody::AddVelocity(const Vector3& Velocity)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->addVelocity(V3_TO_BT(Velocity));
#endif
		}
		void SoftBody::SetVelocity(const Vector3& Velocity)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setVelocity(V3_TO_BT(Velocity));
#endif
		}
		void SoftBody::AddVelocity(const Vector3& Velocity, int Node)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->addVelocity(V3_TO_BT(Velocity), Node);
#endif
		}
		void SoftBody::SetMass(int Node, float Mass)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setMass(Node, Mass);
#endif
		}
		void SoftBody::SetTotalMass(float Mass, bool FromFaces)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setTotalMass(Mass, FromFaces);
#endif
		}
		void SoftBody::SetTotalDensity(float Density)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setTotalDensity(Density);
#endif
		}
		void SoftBody::SetVolumeMass(float Mass)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setVolumeMass(Mass);
#endif
		}
		void SoftBody::SetVolumeDensity(float Density)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setVolumeDensity(Density);
#endif
		}
		void SoftBody::Translate(const Vector3& Position)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->translate(btVector3(Position.X, Position.Y, -Position.Z));
#endif
		}
		void SoftBody::Rotate(const Vector3& Rotation)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btQuaternion Value;
			Value.setEulerZYX(Rotation.X, Rotation.Y, Rotation.Z);
			Instance->rotate(Value);
#endif
		}
		void SoftBody::Scale(const Vector3& Scale)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->scale(V3_TO_BT(Scale));
#endif
		}
		void SoftBody::SetRestLengthScale(float RestLength)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setRestLengthScale(RestLength);
#endif
		}
		void SoftBody::SetPose(bool Volume, bool Frame)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setPose(Volume, Frame);
#endif
		}
		float SoftBody::GetMass(int Node) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getMass(Node);
#else
			return 0;
#endif
		}
		float SoftBody::GetTotalMass() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getTotalMass();
#else
			return 0;
#endif
		}
		float SoftBody::GetRestLengthScale() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getRestLengthScale();
#else
			return 0;
#endif
		}
		float SoftBody::GetVolume() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getVolume();
#else
			return 0;
#endif
		}
		int SoftBody::GenerateBendingConstraints(int Distance)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->generateBendingConstraints(Distance);
#else
			return 0;
#endif
		}
		void SoftBody::RandomizeConstraints()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->randomizeConstraints();
#endif
		}
		bool SoftBody::CutLink(int Node0, int Node1, float Position)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->cutLink(Node0, Node1, Position);
#else
			return false;
#endif
		}
		bool SoftBody::RayTest(const Vector3& From, const Vector3& To, RayCast& Result)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btSoftBody::sRayCast Cast;
			bool R = Instance->rayTest(V3_TO_BT(From), V3_TO_BT(To), Cast);
			Result.Body = Get(Cast.body);
			Result.Feature = (SoftFeature)Cast.feature;
			Result.Index = Cast.index;
			Result.Fraction = Cast.fraction;

			return R;
#else
			return false;
#endif
		}
		void SoftBody::SetWindVelocity(const Vector3& Velocity)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setWindVelocity(V3_TO_BT(Velocity));
#endif
		}
		Vector3 SoftBody::GetWindVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 Value = Instance->getWindVelocity();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		void SoftBody::GetAabb(Vector3& Min, Vector3& Max) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 BMin, BMax;
			Instance->getAabb(BMin, BMax);
			Min = BT_TO_V3(BMin);
			Max = BT_TO_V3(BMax);
#endif
		}
		void SoftBody::IndicesToPointers(const int* Map)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Map != nullptr, "map should be set");
			Instance->indicesToPointers(Map);
#endif
		}
		void SoftBody::SetSpinningFriction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setSpinningFriction(Value);
#endif
		}
		Vector3 SoftBody::GetLinearVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 Value = Instance->getInterpolationLinearVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetAngularVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 Value = Instance->getInterpolationAngularVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetCenterPosition() const
		{
#ifdef VI_BULLET3
			return Center;
#else
			return 0;
#endif
		}
		void SoftBody::SetActivity(bool Active)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			if (GetActivationState() == MotionState::Disable_Deactivation)
				return;

			if (Active)
				Instance->forceActivationState((int)MotionState::Active);
			else
				Instance->forceActivationState((int)MotionState::Deactivation_Needed);
#endif
		}
		void SoftBody::SetAsGhost()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
#endif
		}
		void SoftBody::SetAsNormal()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setCollisionFlags(0);
#endif
		}
		void SoftBody::SetSelfPointer()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setUserPointer(this);
#endif
		}
		void SoftBody::SetWorldTransform(btTransform* Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			VI_ASSERT(Value != nullptr, "transform should be set");
			Instance->setWorldTransform(*Value);
#endif
		}
		void SoftBody::SetActivationState(MotionState Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->forceActivationState((int)Value);
#endif
		}
		void SoftBody::SetFriction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setFriction(Value);
#endif
		}
		void SoftBody::SetRestitution(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setRestitution(Value);
#endif
		}
		void SoftBody::SetContactStiffness(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setContactStiffnessAndDamping(Value, Instance->getContactDamping());
#endif
		}
		void SoftBody::SetContactDamping(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setContactStiffnessAndDamping(Instance->getContactStiffness(), Value);
#endif
		}
		void SoftBody::SetHitFraction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setHitFraction(Value);
#endif
		}
		void SoftBody::SetCcdMotionThreshold(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setCcdMotionThreshold(Value);
#endif
		}
		void SoftBody::SetCcdSweptSphereRadius(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setCcdSweptSphereRadius(Value);
#endif
		}
		void SoftBody::SetContactProcessingThreshold(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setContactProcessingThreshold(Value);
#endif
		}
		void SoftBody::SetDeactivationTime(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setDeactivationTime(Value);
#endif
		}
		void SoftBody::SetRollingFriction(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setRollingFriction(Value);
#endif
		}
		void SoftBody::SetAnisotropicFriction(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Instance->setAnisotropicFriction(V3_TO_BT(Value));
#endif
		}
		void SoftBody::SetConfig(const Desc::SConfig& Conf)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			Initial.Config = Conf;
			Instance->m_cfg.aeromodel = (btSoftBody::eAeroModel::_)Initial.Config.AeroModel;
			Instance->m_cfg.kVCF = Initial.Config.VCF;
			Instance->m_cfg.kDP = Initial.Config.DP;
			Instance->m_cfg.kDG = Initial.Config.DG;
			Instance->m_cfg.kLF = Initial.Config.LF;
			Instance->m_cfg.kPR = Initial.Config.PR;
			Instance->m_cfg.kVC = Initial.Config.VC;
			Instance->m_cfg.kDF = Initial.Config.DF;
			Instance->m_cfg.kMT = Initial.Config.MT;
			Instance->m_cfg.kCHR = Initial.Config.CHR;
			Instance->m_cfg.kKHR = Initial.Config.KHR;
			Instance->m_cfg.kSHR = Initial.Config.SHR;
			Instance->m_cfg.kAHR = Initial.Config.AHR;
			Instance->m_cfg.kSRHR_CL = Initial.Config.SRHR_CL;
			Instance->m_cfg.kSKHR_CL = Initial.Config.SKHR_CL;
			Instance->m_cfg.kSSHR_CL = Initial.Config.SSHR_CL;
			Instance->m_cfg.kSR_SPLT_CL = Initial.Config.SR_SPLT_CL;
			Instance->m_cfg.kSK_SPLT_CL = Initial.Config.SK_SPLT_CL;
			Instance->m_cfg.kSS_SPLT_CL = Initial.Config.SS_SPLT_CL;
			Instance->m_cfg.maxvolume = Initial.Config.MaxVolume;
			Instance->m_cfg.timescale = Initial.Config.TimeScale;
			Instance->m_cfg.viterations = Initial.Config.VIterations;
			Instance->m_cfg.piterations = Initial.Config.PIterations;
			Instance->m_cfg.diterations = Initial.Config.DIterations;
			Instance->m_cfg.citerations = Initial.Config.CIterations;
			Instance->m_cfg.collisions = Initial.Config.Collisions;
			Instance->m_cfg.m_maxStress = Initial.Config.MaxStress;
			Instance->m_cfg.drag = Initial.Config.Drag;

			if (Initial.Config.Constraints > 0)
				Instance->generateBendingConstraints(Initial.Config.Constraints);

			if (Initial.Config.Clusters > 0)
				Instance->generateClusters(Initial.Config.Clusters);
#endif
		}
		MotionState SoftBody::GetActivationState() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return (MotionState)Instance->getActivationState();
#else
			return MotionState::Island_Sleeping;
#endif
		}
		Shape SoftBody::GetCollisionShapeType() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			if (!Initial.Shape.Convex.Enabled)
				return Shape::Invalid;

			return Shape::Convex_Hull;
#else
			return Shape::Invalid;
#endif
		}
		Vector3 SoftBody::GetAnisotropicFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 Value = Instance->getAnisotropicFriction();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetScale() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 bMin, bMax;
			Instance->getAabb(bMin, bMax);
			btVector3 bScale = bMax - bMin;
			Vector3 Scale = BT_TO_V3(bScale);

			return Scale.Div(2.0f).Abs();
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetPosition() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btVector3 Value = Instance->getWorldTransform().getOrigin();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetRotation() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			btScalar X, Y, Z;
			Instance->getWorldTransform().getBasis().getEulerZYX(Z, Y, X);
			return Vector3(-X, -Y, Z);
#else
			return 0;
#endif
		}
		btTransform* SoftBody::GetWorldTransform() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return &Instance->getWorldTransform();
#else
			return nullptr;
#endif
		}
		btSoftBody* SoftBody::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool SoftBody::IsGhost() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return (Instance->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
#else
			return false;
#endif
		}
		bool SoftBody::IsActive() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->isActive();
#else
			return false;
#endif
		}
		bool SoftBody::IsStatic() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->isStaticObject();
#else
			return true;
#endif
		}
		bool SoftBody::IsColliding() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->hasContactResponse();
#else
			return false;
#endif
		}
		float SoftBody::GetSpinningFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getSpinningFriction();
#else
			return 0;
#endif
		}
		float SoftBody::GetContactStiffness() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getContactStiffness();
#else
			return 0;
#endif
		}
		float SoftBody::GetContactDamping() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getContactDamping();
#else
			return 0;
#endif
		}
		float SoftBody::GetFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getFriction();
#else
			return 0;
#endif
		}
		float SoftBody::GetRestitution() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getRestitution();
#else
			return 0;
#endif
		}
		float SoftBody::GetHitFraction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getHitFraction();
#else
			return 0;
#endif
		}
		float SoftBody::GetCcdMotionThreshold() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getCcdMotionThreshold();
#else
			return 0;
#endif
		}
		float SoftBody::GetCcdSweptSphereRadius() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getCcdSweptSphereRadius();
#else
			return 0;
#endif
		}
		float SoftBody::GetContactProcessingThreshold() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getContactProcessingThreshold();
#else
			return 0;
#endif
		}
		float SoftBody::GetDeactivationTime() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getDeactivationTime();
#else
			return 0;
#endif
		}
		float SoftBody::GetRollingFriction() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getRollingFriction();
#else
			return 0;
#endif
		}
		size_t SoftBody::GetCollisionFlags() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->getCollisionFlags();
#else
			return 0;
#endif
		}
		size_t SoftBody::GetVerticesCount() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "softbody should be initialized");
			return Instance->m_nodes.size();
#else
			return 0;
#endif
		}
		SoftBody::Desc& SoftBody::GetInitialState()
		{
			return Initial;
		}
		Simulator* SoftBody::GetSimulator() const
		{
#ifdef VI_BULLET3
			return Engine;
#else
			return nullptr;
#endif
		}
		SoftBody* SoftBody::Get(btSoftBody* From)
		{
#ifdef VI_BULLET3
			VI_ASSERT(From != nullptr, "softbody should be set");
			return (SoftBody*)From->getUserPointer();
#else
			return nullptr;
#endif
		}

		Constraint::Constraint(Simulator* Refer) noexcept : First(nullptr), Second(nullptr), Engine(Refer), UserPointer(nullptr)
		{
		}
		void Constraint::SetBreakingImpulseThreshold(float Value)
		{
#ifdef VI_BULLET3
			btTypedConstraint* Base = Get();
			VI_ASSERT(Base != nullptr, "typed constraint should be initialized");
			Base->setBreakingImpulseThreshold(Value);
#endif
		}
		void Constraint::SetEnabled(bool Value)
		{
#ifdef VI_BULLET3
			btTypedConstraint* Base = Get();
			VI_ASSERT(Base != nullptr, "typed constraint should be initialized");
			Base->setEnabled(Value);
#endif
		}
		btRigidBody* Constraint::GetFirst() const
		{
#ifdef VI_BULLET3
			return First;
#else
			return nullptr;
#endif
		}
		btRigidBody* Constraint::GetSecond() const
		{
#ifdef VI_BULLET3
			return Second;
#else
			return nullptr;
#endif
		}
		float Constraint::GetBreakingImpulseThreshold() const
		{
#ifdef VI_BULLET3
			btTypedConstraint* Base = Get();
			VI_ASSERT(Base != nullptr, "typed constraint should be initialized");
			return Base->getBreakingImpulseThreshold();
#else
			return 0;
#endif
		}
		bool Constraint::IsActive() const
		{
#ifdef VI_BULLET3
			btTypedConstraint* Base = Get();
			if (!Base || !First || !Second)
				return false;

			if (First != nullptr)
			{
				for (int i = 0; i < First->getNumConstraintRefs(); i++)
				{
					if (First->getConstraintRef(i) == Base)
						return true;
				}
			}

			if (Second != nullptr)
			{
				for (int i = 0; i < Second->getNumConstraintRefs(); i++)
				{
					if (Second->getConstraintRef(i) == Base)
						return true;
				}
			}

			return false;
#else
			return false;
#endif
		}
		bool Constraint::IsEnabled() const
		{
#ifdef VI_BULLET3
			btTypedConstraint* Base = Get();
			VI_ASSERT(Base != nullptr, "typed constraint should be initialized");
			return Base->isEnabled();
#else
			return false;
#endif
		}
		Simulator* Constraint::GetSimulator() const
		{
			return Engine;
		}

		PConstraint::PConstraint(Simulator* Refer, const Desc& I) noexcept : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef VI_BULLET3
			VI_ASSERT(I.TargetA != nullptr, "target A rigidbody should be set");
			VI_ASSERT(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = VI_NEW(btPoint2PointConstraint, *First, *Second, V3_TO_BT(I.PivotA), V3_TO_BT(I.PivotB));
			else
				Instance = VI_NEW(btPoint2PointConstraint, *First, V3_TO_BT(I.PivotA));

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		PConstraint::~PConstraint() noexcept
		{
#ifdef VI_BULLET3
			Engine->RemoveConstraint(this);
			VI_DELETE(btPoint2PointConstraint, Instance);
#endif
		}
		Constraint* PConstraint::Copy() const
		{
			VI_ASSERT(Instance != nullptr, "p2p constraint should be initialized");
			PConstraint* Target = new PConstraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->SetPivotA(GetPivotA());
			Target->SetPivotB(GetPivotB());

			return Target;
		}
		btTypedConstraint* PConstraint::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool PConstraint::HasCollisions() const
		{
			return State.Collisions;
		}
		void PConstraint::SetPivotA(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "p2p constraint should be initialized");
			Instance->setPivotA(V3_TO_BT(Value));
			State.PivotA = Value;
#endif
		}
		void PConstraint::SetPivotB(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "p2p constraint should be initialized");
			Instance->setPivotB(V3_TO_BT(Value));
			State.PivotB = Value;
#endif
		}
		Vector3 PConstraint::GetPivotA() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "p2p constraint should be initialized");
			const btVector3& Value = Instance->getPivotInA();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		Vector3 PConstraint::GetPivotB() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "p2p constraint should be initialized");
			const btVector3& Value = Instance->getPivotInB();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		PConstraint::Desc& PConstraint::GetState()
		{
			return State;
		}

		HConstraint::HConstraint(Simulator* Refer, const Desc& I) noexcept : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef VI_BULLET3
			VI_ASSERT(I.TargetA != nullptr, "target A rigidbody should be set");
			VI_ASSERT(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = VI_NEW(btHingeConstraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity(), I.References);
			else
				Instance = VI_NEW(btHingeConstraint, *First, btTransform::getIdentity(), I.References);

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		HConstraint::~HConstraint() noexcept
		{
#ifdef VI_BULLET3
			Engine->RemoveConstraint(this);
			VI_DELETE(btHingeConstraint, Instance);
#endif
		}
		Constraint* HConstraint::Copy() const
		{
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			HConstraint* Target = new HConstraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->EnableAngularMotor(IsAngularMotorEnabled(), GetMotorTargetVelocity(), GetMaxMotorImpulse());
			Target->SetAngularOnly(IsAngularOnly());
			Target->SetLimit(GetLowerLimit(), GetUpperLimit(), GetLimitSoftness(), GetLimitBiasFactor(), GetLimitRelaxationFactor());
			Target->SetOffset(IsOffset());

			return Target;
		}
		btTypedConstraint* HConstraint::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool HConstraint::HasCollisions() const
		{
			return State.Collisions;
		}
		void HConstraint::EnableAngularMotor(bool Enable, float TargetVelocity, float MaxMotorImpulse)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->enableAngularMotor(Enable, TargetVelocity, MaxMotorImpulse);
#endif
		}
		void HConstraint::EnableMotor(bool Enable)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->enableMotor(Enable);
#endif
		}
		void HConstraint::TestLimit(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->testLimit(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void HConstraint::SetFrames(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setFrames(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void HConstraint::SetAngularOnly(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setAngularOnly(Value);
#endif
		}
		void HConstraint::SetMaxMotorImpulse(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setMaxMotorImpulse(Value);
#endif
		}
		void HConstraint::SetMotorTargetVelocity(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setMotorTargetVelocity(Value);
#endif
		}
		void HConstraint::SetMotorTarget(float TargetAngle, float Delta)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setMotorTarget(TargetAngle, Delta);
#endif
		}
		void HConstraint::SetLimit(float Low, float High, float Softness, float BiasFactor, float RelaxationFactor)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setLimit(Low, High, Softness, BiasFactor, RelaxationFactor);
#endif
		}
		void HConstraint::SetOffset(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setUseFrameOffset(Value);
#endif
		}
		void HConstraint::SetReferenceToA(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setUseReferenceFrameA(Value);
#endif
		}
		void HConstraint::SetAxis(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			btVector3 Axis = V3_TO_BT(Value);
			Instance->setAxis(Axis);
#endif
		}
		int HConstraint::GetSolveLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getSolveLimit();
#else
			return 0;
#endif
		}
		float HConstraint::GetMotorTargetVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getMotorTargetVelocity();
#else
			return 0;
#endif
		}
		float HConstraint::GetMaxMotorImpulse() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getMaxMotorImpulse();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitSign() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLimitSign();
#else
			return 0;
#endif
		}
		float HConstraint::GetHingeAngle() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getHingeAngle();
#else
			return 0;
#endif
		}
		float HConstraint::GetHingeAngle(const Matrix4x4& A, const Matrix4x4& B) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getHingeAngle(M16_TO_BT(A), M16_TO_BT(B));
#else
			return 0;
#endif
		}
		float HConstraint::GetLowerLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLowerLimit();
#else
			return 0;
#endif
		}
		float HConstraint::GetUpperLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getUpperLimit();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitSoftness() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLimitSoftness();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitBiasFactor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLimitBiasFactor();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitRelaxationFactor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLimitRelaxationFactor();
#else
			return 0;
#endif
		}
		bool HConstraint::HasLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->hasLimit();
#else
			return 0;
#endif
		}
		bool HConstraint::IsOffset() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getUseFrameOffset();
#else
			return 0;
#endif
		}
		bool HConstraint::IsReferenceToA() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getUseReferenceFrameA();
#else
			return 0;
#endif
		}
		bool HConstraint::IsAngularOnly() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getAngularOnly();
#else
			return 0;
#endif
		}
		bool HConstraint::IsAngularMotorEnabled() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getEnableAngularMotor();
#else
			return 0;
#endif
		}
		HConstraint::Desc& HConstraint::GetState()
		{
			return State;
		}

		SConstraint::SConstraint(Simulator* Refer, const Desc& I) noexcept : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef VI_BULLET3
			VI_ASSERT(I.TargetA != nullptr, "target A rigidbody should be set");
			VI_ASSERT(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = VI_NEW(btSliderConstraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity(), I.Linear);
			else
				Instance = VI_NEW(btSliderConstraint, *First, btTransform::getIdentity(), I.Linear);

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		SConstraint::~SConstraint() noexcept
		{
#ifdef VI_BULLET3
			Engine->RemoveConstraint(this);
			VI_DELETE(btSliderConstraint, Instance);
#endif
		}
		Constraint* SConstraint::Copy() const
		{
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			SConstraint* Target = new SConstraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->SetAngularMotorVelocity(GetAngularMotorVelocity());
			Target->SetLinearMotorVelocity(GetLinearMotorVelocity());
			Target->SetUpperLinearLimit(GetUpperLinearLimit());
			Target->SetLowerLinearLimit(GetLowerLinearLimit());
			Target->SetAngularDampingDirection(GetAngularDampingDirection());
			Target->SetLinearDampingDirection(GetLinearDampingDirection());
			Target->SetAngularDampingLimit(GetAngularDampingLimit());
			Target->SetLinearDampingLimit(GetLinearDampingLimit());
			Target->SetAngularDampingOrtho(GetAngularDampingOrtho());
			Target->SetLinearDampingOrtho(GetLinearDampingOrtho());
			Target->SetUpperAngularLimit(GetUpperAngularLimit());
			Target->SetLowerAngularLimit(GetLowerAngularLimit());
			Target->SetMaxAngularMotorForce(GetMaxAngularMotorForce());
			Target->SetMaxLinearMotorForce(GetMaxLinearMotorForce());
			Target->SetAngularRestitutionDirection(GetAngularRestitutionDirection());
			Target->SetLinearRestitutionDirection(GetLinearRestitutionDirection());
			Target->SetAngularRestitutionLimit(GetAngularRestitutionLimit());
			Target->SetLinearRestitutionLimit(GetLinearRestitutionLimit());
			Target->SetAngularRestitutionOrtho(GetAngularRestitutionOrtho());
			Target->SetLinearRestitutionOrtho(GetLinearRestitutionOrtho());
			Target->SetAngularSoftnessDirection(GetAngularSoftnessDirection());
			Target->SetLinearSoftnessDirection(GetLinearSoftnessDirection());
			Target->SetAngularSoftnessLimit(GetAngularSoftnessLimit());
			Target->SetLinearSoftnessLimit(GetLinearSoftnessLimit());
			Target->SetAngularSoftnessOrtho(GetAngularSoftnessOrtho());
			Target->SetLinearSoftnessOrtho(GetLinearSoftnessOrtho());
			Target->SetPoweredAngularMotor(GetPoweredAngularMotor());
			Target->SetPoweredLinearMotor(GetPoweredLinearMotor());

			return Target;
		}
		btTypedConstraint* SConstraint::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool SConstraint::HasCollisions() const
		{
			return State.Collisions;
		}
		void SConstraint::SetAngularMotorVelocity(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setTargetAngMotorVelocity(Value);
#endif
		}
		void SConstraint::SetLinearMotorVelocity(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setTargetLinMotorVelocity(Value);
#endif
		}
		void SConstraint::SetUpperLinearLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setUpperLinLimit(Value);
#endif
		}
		void SConstraint::SetLowerLinearLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setLowerLinLimit(Value);
#endif
		}
		void SConstraint::SetAngularDampingDirection(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingDirAng(Value);
#endif
		}
		void SConstraint::SetLinearDampingDirection(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingDirLin(Value);
#endif
		}
		void SConstraint::SetAngularDampingLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingLimAng(Value);
#endif
		}
		void SConstraint::SetLinearDampingLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingLimLin(Value);
#endif
		}
		void SConstraint::SetAngularDampingOrtho(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingOrthoAng(Value);
#endif
		}
		void SConstraint::SetLinearDampingOrtho(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingOrthoLin(Value);
#endif
		}
		void SConstraint::SetUpperAngularLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setUpperAngLimit(Value);
#endif
		}
		void SConstraint::SetLowerAngularLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setLowerAngLimit(Value);
#endif
		}
		void SConstraint::SetMaxAngularMotorForce(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setMaxAngMotorForce(Value);
#endif
		}
		void SConstraint::SetMaxLinearMotorForce(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setMaxLinMotorForce(Value);
#endif
		}
		void SConstraint::SetAngularRestitutionDirection(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionDirAng(Value);
#endif
		}
		void SConstraint::SetLinearRestitutionDirection(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionDirLin(Value);
#endif
		}
		void SConstraint::SetAngularRestitutionLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionLimAng(Value);
#endif
		}
		void SConstraint::SetLinearRestitutionLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionLimLin(Value);
#endif
		}
		void SConstraint::SetAngularRestitutionOrtho(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionOrthoAng(Value);
#endif
		}
		void SConstraint::SetLinearRestitutionOrtho(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionOrthoLin(Value);
#endif
		}
		void SConstraint::SetAngularSoftnessDirection(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessDirAng(Value);
#endif
		}
		void SConstraint::SetLinearSoftnessDirection(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessDirLin(Value);
#endif
		}
		void SConstraint::SetAngularSoftnessLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessLimAng(Value);
#endif
		}
		void SConstraint::SetLinearSoftnessLimit(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessLimLin(Value);
#endif
		}
		void SConstraint::SetAngularSoftnessOrtho(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessOrthoAng(Value);
#endif
		}
		void SConstraint::SetLinearSoftnessOrtho(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessOrthoLin(Value);
#endif
		}
		void SConstraint::SetPoweredAngularMotor(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setPoweredAngMotor(Value);
#endif
		}
		void SConstraint::SetPoweredLinearMotor(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			Instance->setPoweredLinMotor(Value);
#endif
		}
		float SConstraint::GetAngularMotorVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getTargetAngMotorVelocity();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearMotorVelocity() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getTargetLinMotorVelocity();
#else
			return 0;
#endif
		}
		float SConstraint::GetUpperLinearLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getUpperLinLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetLowerLinearLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getLowerLinLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularDampingDirection() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getDampingDirAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearDampingDirection() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getDampingDirLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularDampingLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getDampingLimAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearDampingLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getDampingLimLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularDampingOrtho() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getDampingOrthoAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearDampingOrtho() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getDampingOrthoLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetUpperAngularLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getUpperAngLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetLowerAngularLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getLowerAngLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetMaxAngularMotorForce() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getMaxAngMotorForce();
#else
			return 0;
#endif
		}
		float SConstraint::GetMaxLinearMotorForce() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getMaxLinMotorForce();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularRestitutionDirection() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getRestitutionDirAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearRestitutionDirection() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getRestitutionDirLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularRestitutionLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getRestitutionLimAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearRestitutionLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getRestitutionLimLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularRestitutionOrtho() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getRestitutionOrthoAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearRestitutionOrtho() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getRestitutionOrthoLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularSoftnessDirection() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getSoftnessDirAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearSoftnessDirection() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getSoftnessDirLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularSoftnessLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getSoftnessLimAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearSoftnessLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getSoftnessLimLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularSoftnessOrtho() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getSoftnessOrthoAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearSoftnessOrtho() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getSoftnessOrthoLin();
#else
			return 0;
#endif
		}
		bool SConstraint::GetPoweredAngularMotor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getPoweredAngMotor();
#else
			return false;
#endif
		}
		bool SConstraint::GetPoweredLinearMotor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "slider constraint should be initialized");
			return Instance->getPoweredLinMotor();
#else
			return false;
#endif
		}
		SConstraint::Desc& SConstraint::GetState()
		{
			return State;
		}

		CTConstraint::CTConstraint(Simulator* Refer, const Desc& I) noexcept : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef VI_BULLET3
			VI_ASSERT(I.TargetA != nullptr, "target A rigidbody should be set");
			VI_ASSERT(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = VI_NEW(btConeTwistConstraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity());
			else
				Instance = VI_NEW(btConeTwistConstraint, *First, btTransform::getIdentity());

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		CTConstraint::~CTConstraint() noexcept
		{
#ifdef VI_BULLET3
			Engine->RemoveConstraint(this);
			VI_DELETE(btConeTwistConstraint, Instance);
#endif
		}
		Constraint* CTConstraint::Copy() const
		{
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			CTConstraint* Target = new CTConstraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->EnableMotor(IsMotorEnabled());
			Target->SetAngularOnly(IsAngularOnly());
			Target->SetLimit(GetSwingSpan1(), GetSwingSpan2(), GetTwistSpan(), GetLimitSoftness(), GetBiasFactor(), GetRelaxationFactor());
			Target->SetDamping(GetDamping());
			Target->SetFixThresh(GetFixThresh());
			Target->SetMotorTarget(GetMotorTarget());

			if (IsMaxMotorImpulseNormalized())
				Target->SetMaxMotorImpulseNormalized(GetMaxMotorImpulse());
			else
				Target->SetMaxMotorImpulse(GetMaxMotorImpulse());

			return Target;
		}
		btTypedConstraint* CTConstraint::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool CTConstraint::HasCollisions() const
		{
			return State.Collisions;
		}
		void CTConstraint::EnableMotor(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->enableMotor(Value);
#endif
		}
		void CTConstraint::SetFrames(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setFrames(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void CTConstraint::SetAngularOnly(bool Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setAngularOnly(Value);
#endif
		}
		void CTConstraint::SetLimit(int LimitIndex, float LimitValue)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setLimit(LimitIndex, LimitValue);
#endif
		}
		void CTConstraint::SetLimit(float SwingSpan1, float SwingSpan2, float TwistSpan, float Softness, float BiasFactor, float RelaxationFactor)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setLimit(SwingSpan1, SwingSpan2, TwistSpan, Softness, BiasFactor, RelaxationFactor);
#endif
		}
		void CTConstraint::SetDamping(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setDamping(Value);
#endif
		}
		void CTConstraint::SetMaxMotorImpulse(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMaxMotorImpulse(Value);
#endif
		}
		void CTConstraint::SetMaxMotorImpulseNormalized(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMaxMotorImpulseNormalized(Value);
#endif
		}
		void CTConstraint::SetFixThresh(float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setFixThresh(Value);
#endif
		}
		void CTConstraint::SetMotorTarget(const Quaternion& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMotorTarget(Q4_TO_BT(Value));
#endif
		}
		void CTConstraint::SetMotorTargetInConstraintSpace(const Quaternion& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMotorTargetInConstraintSpace(Q4_TO_BT(Value));
#endif
		}
		Vector3 CTConstraint::GetPointForAngle(float AngleInRadians, float Length) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "cone-twist constraint should be initialized");
			btVector3 Value = Instance->GetPointForAngle(AngleInRadians, Length);
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		Quaternion CTConstraint::GetMotorTarget() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			btQuaternion Value = Instance->getMotorTarget();
			return BT_TO_Q4(Value);
#else
			return Quaternion();
#endif
		}
		int CTConstraint::GetSolveTwistLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getSolveTwistLimit();
#else
			return 0;
#endif
		}
		int CTConstraint::GetSolveSwingLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getSolveSwingLimit();
#else
			return 0;
#endif
		}
		float CTConstraint::GetTwistLimitSign() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getTwistLimitSign();
#else
			return 0;
#endif
		}
		float CTConstraint::GetSwingSpan1() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getSwingSpan1();
#else
			return 0;
#endif
		}
		float CTConstraint::GetSwingSpan2() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getSwingSpan2();
#else
			return 0;
#endif
		}
		float CTConstraint::GetTwistSpan() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getTwistSpan();
#else
			return 0;
#endif
		}
		float CTConstraint::GetLimitSoftness() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLimitSoftness();
#else
			return 0;
#endif
		}
		float CTConstraint::GetBiasFactor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getBiasFactor();
#else
			return 0;
#endif
		}
		float CTConstraint::GetRelaxationFactor() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getRelaxationFactor();
#else
			return 0;
#endif
		}
		float CTConstraint::GetTwistAngle() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getTwistAngle();
#else
			return 0;
#endif
		}
		float CTConstraint::GetLimit(int Value) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getLimit(Value);
#else
			return 0;
#endif
		}
		float CTConstraint::GetDamping() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getDamping();
#else
			return 0;
#endif
		}
		float CTConstraint::GetMaxMotorImpulse() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getMaxMotorImpulse();
#else
			return 0;
#endif
		}
		float CTConstraint::GetFixThresh() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getFixThresh();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsMotorEnabled() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->isMotorEnabled();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsMaxMotorImpulseNormalized() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->isMaxMotorImpulseNormalized();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsPastSwingLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->isPastSwingLimit();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsAngularOnly() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "hinge constraint should be initialized");
			return Instance->getAngularOnly();
#else
			return 0;
#endif
		}
		CTConstraint::Desc& CTConstraint::GetState()
		{
			return State;
		}

		DF6Constraint::DF6Constraint(Simulator* Refer, const Desc& I) noexcept : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef VI_BULLET3
			VI_ASSERT(I.TargetA != nullptr, "target A rigidbody should be set");
			VI_ASSERT(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = VI_NEW(btGeneric6DofSpring2Constraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity());
			else
				Instance = VI_NEW(btGeneric6DofSpring2Constraint, *First, btTransform::getIdentity());

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		DF6Constraint::~DF6Constraint() noexcept
		{
#ifdef VI_BULLET3
			Engine->RemoveConstraint(this);
			VI_DELETE(btGeneric6DofSpring2Constraint, Instance);
#endif
		}
		Constraint* DF6Constraint::Copy() const
		{
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			DF6Constraint* Target = new DF6Constraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->SetLinearLowerLimit(GetLinearLowerLimit());
			Target->SetLinearUpperLimit(GetLinearUpperLimit());
			Target->SetAngularLowerLimit(GetAngularLowerLimit());
			Target->SetAngularUpperLimit(GetAngularUpperLimit());
			Target->SetRotationOrder(GetRotationOrder());
			Target->SetAxis(GetAxis(0), GetAxis(1));

			return Target;
		}
		btTypedConstraint* DF6Constraint::Get() const
		{
#ifdef VI_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool DF6Constraint::HasCollisions() const
		{
			return State.Collisions;
		}
		void DF6Constraint::EnableMotor(int Index, bool OnOff)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->enableMotor(Index, OnOff);
#endif
		}
		void DF6Constraint::EnableSpring(int Index, bool OnOff)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->enableSpring(Index, OnOff);
#endif
		}
		void DF6Constraint::SetFrames(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setFrames(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void DF6Constraint::SetLinearLowerLimit(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLinearLowerLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetLinearUpperLimit(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLinearUpperLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularLowerLimit(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularLowerLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularLowerLimitReversed(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularLowerLimitReversed(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularUpperLimit(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularUpperLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularUpperLimitReversed(const Vector3& Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularUpperLimitReversed(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetLimit(int Axis, float Low, float High)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLimit(Axis, Low, High);
#endif
		}
		void DF6Constraint::SetLimitReversed(int Axis, float Low, float High)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLimitReversed(Axis, Low, High);
#endif
		}
		void DF6Constraint::SetRotationOrder(Rotator Order)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setRotationOrder((RotateOrder)Order);
#endif
		}
		void DF6Constraint::SetAxis(const Vector3& A, const Vector3& B)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAxis(V3_TO_BT(A), V3_TO_BT(B));
#endif
		}
		void DF6Constraint::SetBounce(int Index, float Bounce)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setBounce(Index, Bounce);
#endif
		}
		void DF6Constraint::SetServo(int Index, bool OnOff)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setServo(Index, OnOff);
#endif
		}
		void DF6Constraint::SetTargetVelocity(int Index, float Velocity)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setTargetVelocity(Index, Velocity);
#endif
		}
		void DF6Constraint::SetServoTarget(int Index, float Target)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setServoTarget(Index, Target);
#endif
		}
		void DF6Constraint::SetMaxMotorForce(int Index, float Force)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setMaxMotorForce(Index, Force);
#endif
		}
		void DF6Constraint::SetStiffness(int Index, float Stiffness, bool LimitIfNeeded)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setStiffness(Index, Stiffness, LimitIfNeeded);
#endif
		}
		void DF6Constraint::SetEquilibriumPoint()
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setEquilibriumPoint();
#endif
		}
		void DF6Constraint::SetEquilibriumPoint(int Index)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setEquilibriumPoint(Index);
#endif
		}
		void DF6Constraint::SetEquilibriumPoint(int Index, float Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setEquilibriumPoint(Index, Value);
#endif
		}
		Vector3 DF6Constraint::GetAngularUpperLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularUpperLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAngularUpperLimitReversed() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularUpperLimitReversed(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAngularLowerLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularLowerLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAngularLowerLimitReversed() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularLowerLimitReversed(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetLinearUpperLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getLinearUpperLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetLinearLowerLimit() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getLinearLowerLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAxis(int Value) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			btVector3 Result = Instance->getAxis(Value);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Rotator DF6Constraint::GetRotationOrder() const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			return (Rotator)Instance->getRotationOrder();
#else
			return Rotator::XYZ;
#endif
		}
		float DF6Constraint::GetAngle(int Value) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			return Instance->getAngle(Value);
#else
			return 0;
#endif
		}
		float DF6Constraint::GetRelativePivotPosition(int Value) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			return Instance->getRelativePivotPosition(Value);
#else
			return 0;
#endif
		}
		bool DF6Constraint::IsLimited(int LimitIndex) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Instance != nullptr, "6-dof constraint should be initialized");
			return Instance->isLimited(LimitIndex);
#else
			return 0;
#endif
		}
		DF6Constraint::Desc& DF6Constraint::GetState()
		{
			return State;
		}

		Simulator::Simulator(const Desc& I) noexcept : SoftSolver(nullptr), Speedup(1.0f), Active(true)
		{
#ifdef VI_BULLET3
			Broadphase = VI_NEW(btDbvtBroadphase);
			Solver = VI_NEW(btSequentialImpulseConstraintSolver);

			if (I.EnableSoftBody)
			{
				SoftSolver = VI_NEW(btDefaultSoftBodySolver);
				Collision = VI_NEW(btSoftBodyRigidBodyCollisionConfiguration);
				Dispatcher = VI_NEW(btCollisionDispatcher, Collision);
				World = VI_NEW(btSoftRigidDynamicsWorld, Dispatcher, Broadphase, Solver, Collision, SoftSolver);

				btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
				SoftWorld->getDispatchInfo().m_enableSPU = true;

				btSoftBodyWorldInfo& Info = SoftWorld->getWorldInfo();
				Info.m_gravity = V3_TO_BT(I.Gravity);
				Info.water_normal = V3_TO_BT(I.WaterNormal);
				Info.water_density = I.WaterDensity;
				Info.water_offset = I.WaterOffset;
				Info.air_density = I.AirDensity;
				Info.m_maxDisplacement = I.MaxDisplacement;
			}
			else
			{
				Collision = VI_NEW(btDefaultCollisionConfiguration);
				Dispatcher = VI_NEW(btCollisionDispatcher, Collision);
				World = VI_NEW(btDiscreteDynamicsWorld, Dispatcher, Broadphase, Solver, Collision);
			}

			World->setWorldUserInfo(this);
			World->setGravity(V3_TO_BT(I.Gravity));
			gContactAddedCallback = nullptr;
			gContactDestroyedCallback = nullptr;
			gContactProcessedCallback = nullptr;
			gContactStartedCallback = [](btPersistentManifold* const& Manifold) -> void
			{
				btCollisionObject* Body1 = (btCollisionObject*)Manifold->getBody0();
				btRigidBody* Rigid1 = btRigidBody::upcast(Body1);
				btSoftBody* Soft1 = btSoftBody::upcast(Body1);

				if (Rigid1 != nullptr)
				{
					RigidBody* Body = (RigidBody*)Rigid1->getUserPointer();
					if (Body != nullptr && Body->OnCollisionEnter)
						Body->OnCollisionEnter(CollisionBody((btCollisionObject*)Manifold->getBody1()));
				}
				else if (Soft1 != nullptr)
				{
					SoftBody* Body = (SoftBody*)Soft1->getUserPointer();
					if (Body != nullptr && Body->OnCollisionEnter)
						Body->OnCollisionEnter(CollisionBody((btCollisionObject*)Manifold->getBody1()));
				}
			};
			gContactEndedCallback = [](btPersistentManifold* const& Manifold) -> void
			{
				btCollisionObject* Body1 = (btCollisionObject*)Manifold->getBody0();
				btRigidBody* Rigid1 = btRigidBody::upcast(Body1);
				btSoftBody* Soft1 = btSoftBody::upcast(Body1);

				if (Rigid1 != nullptr)
				{
					RigidBody* Body = (RigidBody*)Rigid1->getUserPointer();
					if (Body != nullptr && Body->OnCollisionEnter)
						Body->OnCollisionExit(CollisionBody((btCollisionObject*)Manifold->getBody1()));
				}
				else if (Soft1 != nullptr)
				{
					SoftBody* Body = (SoftBody*)Soft1->getUserPointer();
					if (Body != nullptr && Body->OnCollisionEnter)
						Body->OnCollisionExit(CollisionBody((btCollisionObject*)Manifold->getBody1()));
				}
			};
#endif
		}
		Simulator::~Simulator() noexcept
		{
#ifdef VI_BULLET3
			RemoveAll();
			for (auto It = Shapes.begin(); It != Shapes.end(); ++It)
			{
				btCollisionShape* Item = (btCollisionShape*)It->first;
				VI_DELETE(btCollisionShape, Item);
			}

			VI_DELETE(btCollisionDispatcher, Dispatcher);
			VI_DELETE(btCollisionConfiguration, Collision);
			VI_DELETE(btConstraintSolver, Solver);
			VI_DELETE(btBroadphaseInterface, Broadphase);
			VI_DELETE(btSoftBodySolver, SoftSolver);
			VI_DELETE(btDiscreteDynamicsWorld, World);
#endif
		}
		void Simulator::SetGravity(const Vector3& Gravity)
		{
#ifdef VI_BULLET3
			World->setGravity(V3_TO_BT(Gravity));
#endif
		}
		void Simulator::SetLinearImpulse(const Vector3& Impulse, bool RandomFactor)
		{
#ifdef VI_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(V3_TO_BT(Velocity));
			}
#endif
		}
		void Simulator::SetLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
#ifdef VI_BULLET3
			if (Start >= 0 && Start < World->getNumCollisionObjects() && End >= 0 && End < World->getNumCollisionObjects())
			{
				for (int i = Start; i < End; i++)
				{
					Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
					btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(V3_TO_BT(Velocity));
				}
			}
#endif
		}
		void Simulator::SetAngularImpulse(const Vector3& Impulse, bool RandomFactor)
		{
#ifdef VI_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(V3_TO_BT(Velocity));
			}
#endif
		}
		void Simulator::SetAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
#ifdef VI_BULLET3
			if (Start >= 0 && Start < World->getNumCollisionObjects() && End >= 0 && End < World->getNumCollisionObjects())
			{
				for (int i = Start; i < End; i++)
				{
					Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
					btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(V3_TO_BT(Velocity));
				}
			}
#endif
		}
		void Simulator::SetOnCollisionEnter(ContactStartedCallback Callback)
		{
#ifdef VI_BULLET3
			gContactStartedCallback = Callback;
#endif
		}
		void Simulator::SetOnCollisionExit(ContactEndedCallback Callback)
		{
#ifdef VI_BULLET3
			gContactEndedCallback = Callback;
#endif
		}
		void Simulator::CreateLinearImpulse(const Vector3& Impulse, bool RandomFactor)
		{
#ifdef VI_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btRigidBody* Body = btRigidBody::upcast(World->getCollisionObjectArray()[i]);
				btVector3 Velocity = Body->getLinearVelocity();
				Velocity.setX(Velocity.getX() + Impulse.X * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setY(Velocity.getY() + Impulse.Y * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setZ(Velocity.getZ() + Impulse.Z * (RandomFactor ? Mathf::Random() : 1));
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(Velocity);
			}
#endif
		}
		void Simulator::CreateLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
#ifdef VI_BULLET3
			if (Start >= 0 && Start < World->getNumCollisionObjects() && End >= 0 && End < World->getNumCollisionObjects())
			{
				for (int i = Start; i < End; i++)
				{
					btRigidBody* Body = btRigidBody::upcast(World->getCollisionObjectArray()[i]);
					btVector3 Velocity = Body->getLinearVelocity();
					Velocity.setX(Velocity.getX() + Impulse.X * (RandomFactor ? Mathf::Random() : 1));
					Velocity.setY(Velocity.getY() + Impulse.Y * (RandomFactor ? Mathf::Random() : 1));
					Velocity.setZ(Velocity.getZ() + Impulse.Z * (RandomFactor ? Mathf::Random() : 1));
					btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(Velocity);
				}
			}
#endif
		}
		void Simulator::CreateAngularImpulse(const Vector3& Impulse, bool RandomFactor)
		{
#ifdef VI_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btRigidBody* Body = btRigidBody::upcast(World->getCollisionObjectArray()[i]);
				btVector3 Velocity = Body->getAngularVelocity();
				Velocity.setX(Velocity.getX() + Impulse.X * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setY(Velocity.getY() + Impulse.Y * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setZ(Velocity.getZ() + Impulse.Z * (RandomFactor ? Mathf::Random() : 1));
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(Velocity);
			}
#endif
		}
		void Simulator::CreateAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
#ifdef VI_BULLET3
			if (Start >= 0 && Start < World->getNumCollisionObjects() && End >= 0 && End < World->getNumCollisionObjects())
			{
				for (int i = Start; i < End; i++)
				{
					btRigidBody* Body = btRigidBody::upcast(World->getCollisionObjectArray()[i]);
					btVector3 Velocity = Body->getAngularVelocity();
					Velocity.setX(Velocity.getX() + Impulse.X * (RandomFactor ? Mathf::Random() : 1));
					Velocity.setY(Velocity.getY() + Impulse.Y * (RandomFactor ? Mathf::Random() : 1));
					Velocity.setZ(Velocity.getZ() + Impulse.Z * (RandomFactor ? Mathf::Random() : 1));
					btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(Velocity);
				}
			}
#endif
		}
		void Simulator::AddSoftBody(SoftBody* Body)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Body != nullptr, "softbody should be set");
			VI_ASSERT(Body->Instance != nullptr, "softbody instance should be set");
			VI_ASSERT(Body->Instance->getWorldArrayIndex() == -1, "softbody should not be attached to other world");
			VI_ASSERT(HasSoftBodySupport(), "softbodies should be supported");
			VI_TRACE("[sim] on 0x%" PRIXPTR " add soft-body 0x%" PRIXPTR, (void*)this, (void*)Body);

			btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
			SoftWorld->addSoftBody(Body->Instance);
#endif
		}
		void Simulator::RemoveSoftBody(SoftBody* Body)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Body != nullptr, "softbody should be set");
			VI_ASSERT(Body->Instance != nullptr, "softbody instance should be set");
			VI_ASSERT(Body->Instance->getWorldArrayIndex() >= 0, "softbody should be attached to world");
			VI_ASSERT(HasSoftBodySupport(), "softbodies should be supported");
			VI_TRACE("[sim] on 0x%" PRIXPTR " remove soft-body 0x%" PRIXPTR, (void*)this, (void*)Body);

			btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
			SoftWorld->removeSoftBody(Body->Instance);
#endif
		}
		void Simulator::AddRigidBody(RigidBody* Body)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Body != nullptr, "rigidbody should be set");
			VI_ASSERT(Body->Instance != nullptr, "rigidbody instance should be set");
			VI_ASSERT(Body->Instance->getWorldArrayIndex() == -1, "rigidbody should not be attached to other world");
			VI_TRACE("[sim] on 0x%" PRIXPTR " add rigid-body 0x%" PRIXPTR, (void*)this, (void*)Body);
			World->addRigidBody(Body->Instance);
#endif
		}
		void Simulator::RemoveRigidBody(RigidBody* Body)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Body != nullptr, "rigidbody should be set");
			VI_ASSERT(Body->Instance != nullptr, "rigidbody instance should be set");
			VI_ASSERT(Body->Instance->getWorldArrayIndex() >= 0, "rigidbody should be attached to other world");
			VI_TRACE("[sim] on 0x%" PRIXPTR " remove rigid-body 0x%" PRIXPTR, (void*)this, (void*)Body);
			World->removeRigidBody(Body->Instance);
#endif
		}
		void Simulator::AddConstraint(Constraint* Constraint)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Constraint != nullptr, "slider constraint should be set");
			VI_ASSERT(Constraint->Get() != nullptr, "slider constraint instance should be set");
			VI_TRACE("[sim] on 0x%" PRIXPTR " add constraint 0x%" PRIXPTR, (void*)this, (void*)Constraint);
			World->addConstraint(Constraint->Get(), !Constraint->HasCollisions());
#endif
		}
		void Simulator::RemoveConstraint(Constraint* Constraint)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Constraint != nullptr, "slider constraint should be set");
			VI_ASSERT(Constraint->Get() != nullptr, "slider constraint instance should be set");
			VI_TRACE("[sim] on 0x%" PRIXPTR " remove constraint 0x%" PRIXPTR, (void*)this, (void*)Constraint);
			World->removeConstraint(Constraint->Get());
#endif
		}
		void Simulator::RemoveAll()
		{
#ifdef VI_BULLET3
			VI_TRACE("[sim] on 0x%" PRIXPTR " remove all collision objects", (void*)this);
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btCollisionObject* Object = World->getCollisionObjectArray()[i];
				btRigidBody* Body = btRigidBody::upcast(Object);
				if (Body != nullptr)
				{
					auto* State = Body->getMotionState();
					VI_DELETE(btMotionState, State);
					Body->setMotionState(nullptr);

					auto* Shape = Body->getCollisionShape();
					VI_DELETE(btCollisionShape, Shape);
					Body->setCollisionShape(nullptr);
				}

				World->removeCollisionObject(Object);
				VI_DELETE(btCollisionObject, Object);
			}
#endif
		}
		void Simulator::SimulateStep(float ElapsedTime)
		{
#ifdef VI_BULLET3
			if (!Active || Speedup <= 0.0f)
				return;

			VI_MEASURE(Core::Timings::Frame);
			float TimeStep = (Timing.LastElapsedTime > 0.0 ? std::max(0.0f, ElapsedTime - Timing.LastElapsedTime) : 0.0f);
			World->stepSimulation(TimeStep * Speedup, 0);
			Timing.LastElapsedTime = ElapsedTime;
#endif
		}
		void Simulator::FindContacts(RigidBody* Body, int(*Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&))
		{
#ifdef VI_BULLET3
			VI_ASSERT(Callback != nullptr, "callback should not be empty");
			VI_ASSERT(Body != nullptr, "body should be set");
			VI_MEASURE(Core::Timings::Pass);

			FindContactsHandler Handler;
			Handler.Callback = Callback;
			World->contactTest(Body->Get(), Handler);
#endif
		}
		bool Simulator::FindRayContacts(const Vector3& Start, const Vector3& End, int(*Callback)(RayContact*, const CollisionBody&))
		{
#ifdef VI_BULLET3
			VI_ASSERT(Callback != nullptr, "callback should not be empty");
			VI_MEASURE(Core::Timings::Pass);

			FindRayContactsHandler Handler;
			Handler.Callback = Callback;

			World->rayTest(btVector3(Start.X, Start.Y, Start.Z), btVector3(End.X, End.Y, End.Z), Handler);
			return Handler.m_collisionObject != nullptr;
#else
			return false;
#endif
		}
		RigidBody* Simulator::CreateRigidBody(const RigidBody::Desc& I, Transform* Transform)
		{
#ifdef VI_BULLET3
			if (!Transform)
				return CreateRigidBody(I);

			RigidBody::Desc F(I);
			F.Position = Transform->GetPosition();
			F.Rotation = Transform->GetRotation();
			F.Scale = Transform->GetScale();
			return CreateRigidBody(F);
#else
			return nullptr;
#endif
		}
		RigidBody* Simulator::CreateRigidBody(const RigidBody::Desc& I)
		{
#ifdef VI_BULLET3
			return new RigidBody(this, I);
#else
			return nullptr;
#endif
		}
		SoftBody* Simulator::CreateSoftBody(const SoftBody::Desc& I, Transform* Transform)
		{
#ifdef VI_BULLET3
			if (!Transform)
				return CreateSoftBody(I);

			SoftBody::Desc F(I);
			F.Position = Transform->GetPosition();
			F.Rotation = Transform->GetRotation();
			F.Scale = Transform->GetScale();
			return CreateSoftBody(F);
#else
			return nullptr;
#endif
		}
		SoftBody* Simulator::CreateSoftBody(const SoftBody::Desc& I)
		{
#ifdef VI_BULLET3
			if (!HasSoftBodySupport())
				return nullptr;

			return new SoftBody(this, I);
#else
			return nullptr;
#endif
		}
		PConstraint* Simulator::CreatePoint2PointConstraint(const PConstraint::Desc& I)
		{
#ifdef VI_BULLET3
			return new PConstraint(this, I);
#else
			return nullptr;
#endif
		}
		HConstraint* Simulator::CreateHingeConstraint(const HConstraint::Desc& I)
		{
#ifdef VI_BULLET3
			return new HConstraint(this, I);
#else
			return nullptr;
#endif
		}
		SConstraint* Simulator::CreateSliderConstraint(const SConstraint::Desc& I)
		{
#ifdef VI_BULLET3
			return new SConstraint(this, I);
#else
			return nullptr;
#endif
		}
		CTConstraint* Simulator::CreateConeTwistConstraint(const CTConstraint::Desc& I)
		{
#ifdef VI_BULLET3
			return new CTConstraint(this, I);
#else
			return nullptr;
#endif
		}
		DF6Constraint* Simulator::Create6DoFConstraint(const DF6Constraint::Desc& I)
		{
#ifdef VI_BULLET3
			return new DF6Constraint(this, I);
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCube(const Vector3& Scale)
		{
#ifdef VI_BULLET3
			btCollisionShape* Shape = VI_NEW(btBoxShape, V3_TO_BT(Scale));
			VI_TRACE("[sim] save cube shape 0x%" PRIXPTR, (void*)Shape);
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateSphere(float Radius)
		{
#ifdef VI_BULLET3
			btCollisionShape* Shape = VI_NEW(btSphereShape, Radius);
			VI_TRACE("[sim] save sphere shape 0x%" PRIXPTR, (void*)Shape);
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCapsule(float Radius, float Height)
		{
#ifdef VI_BULLET3
			btCollisionShape* Shape = VI_NEW(btCapsuleShape, Radius, Height);
			VI_TRACE("[sim] save capsule shape 0x%" PRIXPTR, (void*)Shape);
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCone(float Radius, float Height)
		{
#ifdef VI_BULLET3
			btCollisionShape* Shape = VI_NEW(btConeShape, Radius, Height);
			VI_TRACE("[sim] save cone shape 0x%" PRIXPTR, (void*)Shape);
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCylinder(const Vector3& Scale)
		{
#ifdef VI_BULLET3
			btCollisionShape* Shape = VI_NEW(btCylinderShape, V3_TO_BT(Scale));
			VI_TRACE("[sim] save cylinder shape 0x%" PRIXPTR, (void*)Shape);
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(Core::Vector<SkinVertex>& Vertices)
		{
#ifdef VI_BULLET3
			btConvexHullShape* Shape = VI_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			VI_TRACE("[sim] save convext-hull shape 0x%" PRIXPTR " (%" PRIu64 " vertices)", (void*)Shape, (uint64_t)Vertices.size());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(Core::Vector<Vertex>& Vertices)
		{
#ifdef VI_BULLET3
			btConvexHullShape* Shape = VI_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			VI_TRACE("[sim] save convext-hull shape 0x%" PRIXPTR " (%" PRIu64 " vertices)", (void*)Shape, (uint64_t)Vertices.size());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(Core::Vector<Vector2>& Vertices)
		{
#ifdef VI_BULLET3
			btConvexHullShape* Shape = VI_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->X, It->Y, 0), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			VI_TRACE("[sim] save convext-hull shape 0x%" PRIXPTR " (%" PRIu64 " vertices)", (void*)Shape, (uint64_t)Vertices.size());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(Core::Vector<Vector3>& Vertices)
		{
#ifdef VI_BULLET3
			btConvexHullShape* Shape = VI_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->X, It->Y, It->Z), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			VI_TRACE("[sim] save convext-hull shape 0x%" PRIXPTR " (%" PRIu64 " vertices)", (void*)Shape, (uint64_t)Vertices.size());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(Core::Vector<Vector4>& Vertices)
		{
#ifdef VI_BULLET3
			btConvexHullShape* Shape = VI_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->X, It->Y, It->Z), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			VI_TRACE("[sim] save convext-hull shape 0x%" PRIXPTR " (%" PRIu64 " vertices)", (void*)Shape, (uint64_t)Vertices.size());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Shape] = 1;
			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(btCollisionShape* From)
		{
#ifdef VI_BULLET3
			VI_ASSERT(From != nullptr, "shape should be set");
			VI_ASSERT(From->getShapeType() == (int)Shape::Convex_Hull, "shape type should be convex hull");

			btConvexHullShape* Hull = VI_NEW(btConvexHullShape);
			btConvexHullShape* Base = (btConvexHullShape*)From;

			for (size_t i = 0; i < (size_t)Base->getNumPoints(); i++)
				Hull->addPoint(*(Base->getUnscaledPoints() + i), false);

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);

			VI_TRACE("[sim] save convext-hull shape 0x%" PRIXPTR " (%" PRIu64 " vertices)", (void*)Hull, (uint64_t)Base->getNumPoints());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Shapes[Hull] = 1;
			return Hull;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateShape(Shape Wanted)
		{
#ifdef VI_BULLET3
			switch (Wanted)
			{
				case Shape::Box:
					return CreateCube();
				case Shape::Sphere:
					return CreateSphere();
				case Shape::Capsule:
					return CreateCapsule();
				case Shape::Cone:
					return CreateCone();
				case Shape::Cylinder:
					return CreateCylinder();
				default:
					return nullptr;
			}
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::TryCloneShape(btCollisionShape* Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Value != nullptr, "shape should be set");
			Shape Type = (Shape)Value->getShapeType();
			if (Type == Shape::Box)
			{
				btBoxShape* Box = (btBoxShape*)Value;
				btVector3 Size = Box->getHalfExtentsWithMargin() / Box->getLocalScaling();
				return CreateCube(BT_TO_V3(Size));
			}
			else if (Type == Shape::Sphere)
			{
				btSphereShape* Sphere = (btSphereShape*)Value;
				return CreateSphere(Sphere->getRadius());
			}
			else if (Type == Shape::Capsule)
			{
				btCapsuleShape* Capsule = (btCapsuleShape*)Value;
				return CreateCapsule(Capsule->getRadius(), Capsule->getHalfHeight() * 2.0f);
			}
			else if (Type == Shape::Cone)
			{
				btConeShape* Cone = (btConeShape*)Value;
				return CreateCone(Cone->getRadius(), Cone->getHeight());
			}
			else if (Type == Shape::Cylinder)
			{
				btCylinderShape* Cylinder = (btCylinderShape*)Value;
				btVector3 Size = Cylinder->getHalfExtentsWithMargin() / Cylinder->getLocalScaling();
				return CreateCylinder(BT_TO_V3(Size));
			}
			else if (Type == Shape::Convex_Hull)
				return CreateConvexHull(Value);

			return nullptr;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::ReuseShape(btCollisionShape* Value)
		{
#ifdef VI_BULLET3
			VI_ASSERT(Value != nullptr, "shape should be set");
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Shapes.find(Value);
			if (It == Shapes.end())
				return nullptr;

			It->second++;
			return Value;
#else
			return nullptr;
#endif
		}
		void Simulator::FreeShape(btCollisionShape** Value)
		{
#ifdef VI_BULLET3
			if (!Value || !*Value)
				return;

			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Shapes.find(*Value);
			if (It == Shapes.end())
				return;

			*Value = nullptr;
			if (It->second-- > 1)
				return;

			btCollisionShape* Item = (btCollisionShape*)It->first;
			VI_TRACE("[sim] free shape 0x%" PRIXPTR, (void*)Item);
			VI_DELETE(btCollisionShape, Item);
			Shapes.erase(It);
#endif
		}
		Core::Vector<Vector3> Simulator::GetShapeVertices(btCollisionShape* Value) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Value != nullptr, "shape should be set");
			auto Type = (Shape)Value->getShapeType();
			if (Type != Shape::Convex_Hull)
				return Core::Vector<Vector3>();

			btConvexHullShape* Hull = (btConvexHullShape*)Value;
			btVector3* Points = Hull->getUnscaledPoints();
			size_t Count = Hull->getNumPoints();
			Core::Vector<Vector3> Vertices;
			Vertices.reserve(Count);

			for (size_t i = 0; i < Count; i++)
			{
				btVector3& It = Points[i];
				Vertices.emplace_back(It.getX(), It.getY(), It.getZ());
			}

			return Vertices;
#else
			return Core::Vector<Vector3>();
#endif
		}
		size_t Simulator::GetShapeVerticesCount(btCollisionShape* Value) const
		{
#ifdef VI_BULLET3
			VI_ASSERT(Value != nullptr, "shape should be set");
			auto Type = (Compute::Shape)Value->getShapeType();
			if (Type != Shape::Convex_Hull)
				return 0;

			btConvexHullShape* Hull = (btConvexHullShape*)Value;
			return Hull->getNumPoints();
#else
			return 0;
#endif
		}
		float Simulator::GetMaxDisplacement() const
		{
#ifdef VI_BULLET3
			if (!SoftSolver || !World)
				return 1000;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().m_maxDisplacement;
#else
			return 1000;
#endif
		}
		float Simulator::GetAirDensity() const
		{
#ifdef VI_BULLET3
			if (!SoftSolver || !World)
				return 1.2f;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().air_density;
#else
			return 1.2f;
#endif
		}
		float Simulator::GetWaterOffset() const
		{
#ifdef VI_BULLET3
			if (!SoftSolver || !World)
				return 0;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_offset;
#else
			return 0;
#endif
		}
		float Simulator::GetWaterDensity() const
		{
#ifdef VI_BULLET3
			if (!SoftSolver || !World)
				return 0;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_density;
#else
			return 0;
#endif
		}
		Vector3 Simulator::GetWaterNormal() const
		{
#ifdef VI_BULLET3
			if (!SoftSolver || !World)
				return 0;

			btVector3 Value = ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_normal;
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		Vector3 Simulator::GetGravity() const
		{
#ifdef VI_BULLET3
			if (!World)
				return 0;

			btVector3 Value = World->getGravity();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		ContactStartedCallback Simulator::GetOnCollisionEnter() const
		{
#ifdef VI_BULLET3
			return gContactStartedCallback;
#else
			return nullptr;
#endif
		}
		ContactEndedCallback Simulator::GetOnCollisionExit() const
		{
#ifdef VI_BULLET3
			return gContactEndedCallback;
#else
			return nullptr;
#endif
		}
		btCollisionConfiguration* Simulator::GetCollision() const
		{
#ifdef VI_BULLET3
			return Collision;
#else
			return nullptr;
#endif
		}
		btBroadphaseInterface* Simulator::GetBroadphase() const
		{
#ifdef VI_BULLET3
			return Broadphase;
#else
			return nullptr;
#endif
		}
		btConstraintSolver* Simulator::GetSolver() const
		{
#ifdef VI_BULLET3
			return Solver;
#else
			return nullptr;
#endif
		}
		btDiscreteDynamicsWorld* Simulator::GetWorld() const
		{
#ifdef VI_BULLET3
			return World;
#else
			return nullptr;
#endif
		}
		btCollisionDispatcher* Simulator::GetDispatcher() const
		{
#ifdef VI_BULLET3
			return Dispatcher;
#else
			return nullptr;
#endif
		}
		btSoftBodySolver* Simulator::GetSoftSolver() const
		{
#ifdef VI_BULLET3
			return SoftSolver;
#else
			return nullptr;
#endif
		}
		bool Simulator::HasSoftBodySupport() const
		{
#ifdef VI_BULLET3
			return SoftSolver != nullptr;
#else
			return false;
#endif
		}
		int Simulator::GetContactManifoldCount() const
		{
#ifdef VI_BULLET3
			if (!Dispatcher)
				return 0;

			return Dispatcher->getNumManifolds();
#else
			return 0;
#endif
		}
		Simulator* Simulator::Get(btDiscreteDynamicsWorld* From)
		{
#ifdef VI_BULLET3
			if (!From)
				return nullptr;

			return (Simulator*)From->getWorldUserInfo();
#else
			return nullptr;
#endif
		}
	}
}
