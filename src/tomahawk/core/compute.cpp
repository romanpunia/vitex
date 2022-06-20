#include "compute.h"
#include <cctype>
#include <random>
#ifdef TH_WITH_BULLET3
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#endif
#ifdef TH_WITH_SIMD
#include <vsimd.h>
#endif
#ifdef TH_MICROSOFT
#include <Windows.h>
#else
#include <time.h>
#endif
#ifdef TH_HAS_OPENSSL
extern "C"
{
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
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

namespace
{
#ifdef TH_WITH_BULLET3
	class FindContactsHandler : public btCollisionWorld::ContactResultCallback
	{
	public:
		int(*Callback)(Tomahawk::Compute::ShapeContact*, const Tomahawk::Compute::CollisionBody&, const Tomahawk::Compute::CollisionBody&) = nullptr;

	public:
		btScalar addSingleResult(btManifoldPoint& Point, const btCollisionObjectWrapper* Object1, int PartId0, int Index0, const btCollisionObjectWrapper* Object2, int PartId1, int Index1) override
		{
			using namespace Tomahawk::Compute;
			TH_ASSERT(Callback && Object1 && Object1->getCollisionObject() && Object2 && Object2->getCollisionObject(), 0, "collision objects are not in available condition");

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
		int(*Callback)(Tomahawk::Compute::RayContact*, const Tomahawk::Compute::CollisionBody&) = nullptr;

	public:
		btScalar addSingleResult(btCollisionWorld::LocalRayResult& RayResult, bool NormalInWorldSpace) override
		{
			using namespace Tomahawk::Compute;
			TH_ASSERT(Callback && RayResult.m_collisionObject, 0, "collision objects are not in available condition");

			RayContact Contact;
			Contact.HitNormalLocal = BT_TO_V3(RayResult.m_hitNormalLocal);
			Contact.NormalInWorldSpace = NormalInWorldSpace;
			Contact.HitFraction = RayResult.m_hitFraction;
			Contact.ClosestHitFraction = m_closestHitFraction;

			btCollisionObject* Body1 = (btCollisionObject*)RayResult.m_collisionObject;
			return (btScalar)Callback(&Contact, CollisionBody(Body1));
		}
	};

	Tomahawk::Compute::Matrix4x4 BT_TO_M16(const btTransform& In)
	{
		Tomahawk::Compute::Matrix4x4 Result;
		const btMatrix3x3& Offset = In.getBasis();
		Result.Row[0] = Offset[0][0];
		Result.Row[1] = Offset[1][0];
		Result.Row[2] = Offset[2][0];
		Result.Row[4] = Offset[0][1];
		Result.Row[5] = Offset[1][1];
		Result.Row[6] = Offset[2][1];
		Result.Row[8] = Offset[0][2];
		Result.Row[9] = Offset[1][2];
		Result.Row[10] = Offset[2][2];

		const btVector3& Origin = In.getOrigin();
		Result.Row[12] = Origin.x();
		Result.Row[13] = Origin.y();
		Result.Row[14] = -Origin.z();

		return Result;
	}
	btTransform M16_TO_BT(const Tomahawk::Compute::Matrix4x4& In)
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

		Tomahawk::Compute::Vector3 Position = In.Position();
		Result.setOrigin(V3_TO_BT(Position));

		return Result;
	}
#endif
	size_t OffsetOf64(const char* Source, char Dest)
	{
		TH_ASSERT(Source != nullptr, 63, "source should be set");
		for (size_t i = 0; i < 64; i++)
		{
			if (Source[i] == Dest)
				return i;
		}

		return 63;
	}
}

namespace Tomahawk
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

		Vector2::Vector2() : X(0.0f), Y(0.0f)
		{
		}
		Vector2::Vector2(float x, float y) : X(x), Y(y)
		{
		}
		Vector2::Vector2(float xy) : X(xy), Y(xy)
		{
		}
		Vector2::Vector2(const Vector2& Value) : X(Value.X), Y(Value.Y)
		{
		}
		Vector2::Vector2(const Vector3& Value) : X(Value.X), Y(Value.Y)
		{
		}
		Vector2::Vector2(const Vector4& Value) : X(Value.X), Y(Value.Y)
		{
		}
		float Vector2::Length() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1);
			return Common::FastSqrt(horizontal_add(square(_r1)));
#else
			return Common::FastSqrt(X * X + Y * Y);
#endif
		}
		float Vector2::Sum() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1);
			return horizontal_add(_r1);
#else
			return X + Y;
#endif
		}
		float Vector2::Dot(const Vector2& B) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, B);
			return horizontal_add(_r1 * _r2);
#else
			return X * B.X + Y * B.Y;
#endif
		}
		float Vector2::Distance(const Vector2& Point) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, Point);
			return Common::FastSqrt(horizontal_add(square(_r1 - _r2)));
#else
			float X1 = X - Point.X, Y1 = Y - Point.Y;
			return Common::FastSqrt(X1 * X1 + Y1 * Y1);
#endif
		}
		float Vector2::Hypotenuse() const
		{
			return Length();
		}
		float Vector2::LookAt(const Vector2& At) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, At); _r1 = _r2 - _r1;
			return atan2f(_r1.extract(0), _r1.extract(1));
#else
			return atan2f(At.X - X, At.Y - Y);
#endif
		}
		float Vector2::Cross(const Vector2& B) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, B); _r1 = _r1 * _r2;
			return _r1.extract(0) - _r1.extract(1);
#else
			return X * B.Y - Y * B.X;
#endif
		}
		Vector2 Vector2::Transform(const Matrix4x4& Matrix) const
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1);
			_r1 = _r1 * Common::FastInvSqrt(horizontal_add(square(_r1)));
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			float F = Length();
			return Vector2(X / F, Y / F);
#endif
		}
		Vector2 Vector2::sNormalize() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1);
			float F = Common::FastSqrt(horizontal_add(square(_r1)));
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1); _r1 = _r1 * xy;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X * xy, Y * xy);
#endif
		}
		Vector2 Vector2::Mul(float x, float y) const
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV2(_r1); LOD_V2(_r2, Value); _r1 = _r1 / _r2;
			return Vector2(_r1.extract(0), _r1.extract(1));
#else
			return Vector2(X / Value.X, Y / Value.Y);
#endif
		}
		Vector2 Vector2::Add(const Vector2& Value) const
		{
#ifdef TH_WITH_SIMD
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
			TH_ASSERT_V(In != nullptr, "in of size 2 should be set");
			In[0] = X;
			In[1] = Y;
		}
		Vector2& Vector2::operator *=(const Vector2& V)
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
		Vector2& Vector2::operator =(const Vector2& V)
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
			TH_ASSERT(Axis >= 0 && Axis <= 1, X, "index out of range");
			if (Axis == 0)
				return X;

			return Y;
		}
		float Vector2::operator [](uint32_t Axis) const
		{
			TH_ASSERT(Axis >= 0 && Axis <= 1, X, "index out of range");
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

		Vector3::Vector3() : X(0.0f), Y(0.0f), Z(0.0f)
		{
		}
		Vector3::Vector3(float x, float y) : X(x), Y(y), Z(0.0f)
		{
		}
		Vector3::Vector3(float x, float y, float z) : X(x), Y(y), Z(z)
		{
		}
		Vector3::Vector3(float xyzw) : X(xyzw), Y(xyzw), Z(xyzw)
		{
		}
		Vector3::Vector3(const Vector2& Value) : X(Value.X), Y(Value.Y), Z(0.0f)
		{
		}
		Vector3::Vector3(const Vector3& Value) : X(Value.X), Y(Value.Y), Z(Value.Z)
		{
		}
		Vector3::Vector3(const Vector4& Value) : X(Value.X), Y(Value.Y), Z(Value.Z)
		{
		}
		float Vector3::Length() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1);
			return Common::FastSqrt(horizontal_add(square(_r1)));
#else
			return Common::FastSqrt(X * X + Y * Y + Z * Z);
#endif
		}
		float Vector3::Sum() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1);
			return horizontal_add(_r1);
#else
			return X + Y + Z;
#endif
		}
		float Vector3::Dot(const Vector3& B) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, B);
			return horizontal_add(_r1 * _r2);
#else
			return X * B.X + Y * B.Y + Z * B.Z;
#endif
		}
		float Vector3::Distance(const Vector3& Point) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Point);
			return Common::FastSqrt(horizontal_add(square(_r1 - _r2)));
#else
			float X1 = X - Point.X, Y1 = Y - Point.Y, Z1 = Z - Point.Z;
			return Common::FastSqrt(X1 * X1 + Y1 * Y1 + Z1 * Z1);
#endif
		}
		float Vector3::Hypotenuse() const
		{
#ifdef TH_WITH_SIMD
			LOD_AV2(_r1, X, Z);
			float R = Common::FastSqrt(horizontal_add(square(_r1)));

			LOD_AV2(_r2, R, Y);
			return Common::FastSqrt(horizontal_add(square(_r2)));
#else
			float R = Common::FastSqrt(X * X + Z * Z);
			return Common::FastSqrt(R * R + Y * Y);
#endif
		}
		float Vector3::LookAtXY(const Vector3& At) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, At); _r1 = _r2 - _r1;
			return atan2f(_r1.extract(0), _r1.extract(1));
#else
			return atan2f(At.X - X, At.Y - Y);
#endif
		}
		float Vector3::LookAtXZ(const Vector3& At) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, At); _r1 = _r2 - _r1;
			return atan2f(_r1.extract(0), -_r1.extract(2));
#else
			return atan2f(At.X - X, Z - At.Z);
#endif
		}
		Vector3 Vector3::Cross(const Vector3& B) const
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
			return Matrix4x4::CreateLookAt(0, *this, Vector3::Up()).Rotation();
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
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1);
			_r1 = _r1 * Common::FastInvSqrt(horizontal_add(square(_r1)));
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			float F = Length();
			return Vector3(X / F, Y / F, Z / F);
#endif
		}
		Vector3 Vector3::sNormalize() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1);
			float F = Common::FastSqrt(horizontal_add(square(_r1)));
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); _r1 = _r1 * xyz;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X * xyz, Y * xyz, Z * xyz);
#endif
		}
		Vector3 Vector3::Mul(const Vector2& XY, float z) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_AV3(_r2, XY.X, XY.Y, z); _r1 = _r1 * _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X * XY.X, Y * XY.Y, Z * z);
#endif
		}
		Vector3 Vector3::Mul(const Vector3& Value) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Value); _r1 = _r1 * _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X * Value.X, Y * Value.Y, Z * Value.Z);
#endif
		}
		Vector3 Vector3::Div(const Vector3& Value) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV3(_r1); LOD_V3(_r2, Value); _r1 = _r1 / _r2;
			return Vector3(_r1.extract(0), _r1.extract(1), _r1.extract(2));
#else
			return Vector3(X / Value.X, Y / Value.Y, Z / Value.Z);
#endif
		}
		Vector3 Vector3::Add(const Vector3& Value) const
		{
#ifdef TH_WITH_SIMD
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
			TH_ASSERT_V(In != nullptr, "in of size 2 should be set");
			In[0] = X;
			In[1] = Y;
		}
		void Vector3::Get3(float* In) const
		{
			TH_ASSERT_V(In != nullptr, "in of size 3 should be set");
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
		}
		Vector3& Vector3::operator *=(const Vector3& V)
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
		Vector3& Vector3::operator =(const Vector3& V)
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
			TH_ASSERT(Axis >= 0 && Axis <= 2, X, "index out of range");
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;

			return Z;
		}
		float Vector3::operator [](uint32_t Axis) const
		{
			TH_ASSERT(Axis >= 0 && Axis <= 2, X, "index out of range");
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

		Vector4::Vector4() : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f)
		{
		}
		Vector4::Vector4(float x, float y) : X(x), Y(y), Z(0.0f), W(0.0f)
		{
		}
		Vector4::Vector4(float x, float y, float z) : X(x), Y(y), Z(z), W(0.0f)
		{
		}
		Vector4::Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w)
		{
		}
		Vector4::Vector4(float xyzw) : X(xyzw), Y(xyzw), Z(xyzw), W(xyzw)
		{
		}
		Vector4::Vector4(const Vector2& Value) : X(Value.X), Y(Value.Y), Z(0.0f), W(0.0f)
		{
		}
		Vector4::Vector4(const Vector3& Value) : X(Value.X), Y(Value.Y), Z(Value.Z), W(0.0f)
		{
		}
		Vector4::Vector4(const Vector4& Value) : X(Value.X), Y(Value.Y), Z(Value.Z), W(Value.W)
		{
		}
		float Vector4::Length() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			return Common::FastSqrt(horizontal_add(square(_r1)));
#else
			return Common::FastSqrt(X * X + Y * Y + Z * Z + W * W);
#endif
		}
		float Vector4::Sum() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			return horizontal_add(_r1);
#else
			return X + Y + Z + W;
#endif
		}
		float Vector4::Dot(const Vector4& B) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, B);
			return horizontal_add(_r1 * _r2);
#else
			return X * B.X + Y * B.Y + Z * B.Z + W * B.W;
#endif
		}
		float Vector4::Distance(const Vector4& Point) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Point);
			return Common::FastSqrt(horizontal_add(square(_r1 - _r2)));
#else
			float X1 = X - Point.X, Y1 = Y - Point.Y, Z1 = Z - Point.Z, W1 = W - Point.W;
			return Common::FastSqrt(X1 * X1 + Y1 * Y1 + Z1 * Z1 + W1 * W1);
#endif
		}
		Vector4 Vector4::Cross(const Vector4& B) const
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			_r1 = _r1 * Common::FastInvSqrt(horizontal_add(square(_r1)));
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			float F = Length();
			return Vector4(X / F, Y / F, Z / F, W / F);
#endif
		}
		Vector4 Vector4::sNormalize() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			float F = Common::FastSqrt(horizontal_add(square(_r1)));
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); _r1 = _r1 * xyzw;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * xyzw, Y * xyzw, Z * xyzw, W * xyzw);
#endif
		}
		Vector4 Vector4::Mul(const Vector2& XY, float z, float w) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); LOD_AV4(_r2, XY.X, XY.Y, z, w); _r1 = _r1 * _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * XY.X, Y * XY.Y, Z * z, W * w);
#endif
		}
		Vector4 Vector4::Mul(const Vector3& XYZ, float w) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); LOD_AV4(_r2, XYZ.X, XYZ.Y, XYZ.Z, w); _r1 = _r1 * _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * XYZ.X, Y * XYZ.Y, Z * XYZ.Z, W * w);
#endif
		}
		Vector4 Vector4::Mul(const Vector4& Value) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Value); _r1 = _r1 * _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X * Value.X, Y * Value.Y, Z * Value.Z, W * Value.W);
#endif
		}
		Vector4 Vector4::Div(const Vector4& Value) const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1); LOD_V4(_r2, Value); _r1 = _r1 / _r2;
			return Vector4(_r1.extract(0), _r1.extract(1), _r1.extract(2), _r1.extract(3));
#else
			return Vector4(X / Value.X, Y / Value.Y, Z / Value.Z, W / Value.W);
#endif
		}
		Vector4 Vector4::Add(const Vector4& Value) const
		{
#ifdef TH_WITH_SIMD
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
			TH_ASSERT_V(In != nullptr, "in of size 2 should be set");
			In[0] = X;
			In[1] = Y;
		}
		void Vector4::Get3(float* In) const
		{
			TH_ASSERT_V(In != nullptr, "in of size 3 should be set");
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
		}
		void Vector4::Get4(float* In) const
		{
			TH_ASSERT_V(In != nullptr, "in of size 4 should be set");
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
		Vector4& Vector4::operator =(const Vector4& V)
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
			TH_ASSERT(Axis >= 0 && Axis <= 3, X, "index outside of range");
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
			TH_ASSERT(Axis >= 0 && Axis <= 3, X, "index outside of range");
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

		Frustum::Frustum()
		{
			Common::CreateFrustum(Corners, 90.0f, 1.0f, 0.1f, 1.0f);
		}
		Frustum::Frustum(float FieldOfView, float Aspect, float NearZ, float FarZ)
		{
			Common::CreateFrustumRad(Corners, FieldOfView, Aspect, NearZ, FarZ);
		}
		void Frustum::Transform(const Matrix4x4& Value)
		{
			for (uint32_t i = 0; i < 8; i++)
			{
				Vector4& Corner = Corners[i];
				Corner = Corner * Value;
			}
		}
		void Frustum::GetBoundingBox(Vector2* X, Vector2* Y, Vector2* Z)
		{
			TH_ASSERT_V(X || Y || Z, "at least one vector of x, y, z should be set");
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

		Ray::Ray() : Direction(0, 0, 1)
		{
		}
		Ray::Ray(const Vector3& _Origin, const Vector3& _Direction) : Origin(_Origin), Direction(_Direction)
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
						*Hit = HitPoint;

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
						*Hit = HitPoint;

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
						*Hit = HitPoint;

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
						*Hit = HitPoint;

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
						*Hit = HitPoint;

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
						*Hit = HitPoint;

					return true;
				}
			}

			return false;
		}

		Matrix4x4::Matrix4x4() : Row { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
		{
		}
		Matrix4x4::Matrix4x4(float Array[16])
		{
			memcpy(Row, Array, sizeof(float) * 16);
		}
		Matrix4x4::Matrix4x4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3)
		{
			memcpy(Row + 0, &row0, sizeof(Vector4));
			memcpy(Row + 4, &row1, sizeof(Vector4));
			memcpy(Row + 8, &row2, sizeof(Vector4));
			memcpy(Row + 12, &row3, sizeof(Vector4));
		}
		Matrix4x4::Matrix4x4(float row00, float row01, float row02, float row03, float row10, float row11, float row12, float row13, float row20, float row21, float row22, float row23, float row30, float row31, float row32, float row33) :
			Row { row00, row01, row02, row03, row10, row11, row12, row13, row20, row21, row22, row23, row30, row31, row32, row33 }
		{
		}
		Matrix4x4::Matrix4x4(bool)
		{
		}
		Matrix4x4::Matrix4x4(const Matrix4x4& V)
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
			return Vector4(Row[0], Row[4], Row[8], Row[12]);
		}
		Matrix4x4& Matrix4x4::operator =(const Matrix4x4& V)
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
		Vector3 Matrix4x4::Up(bool ViewSpace) const
		{
			if (ViewSpace)
				return Vector3(Row[4], Row[5], -Row[6]);
			else
				return Vector3(Row[4], Row[5], Row[6]);
		}
		Vector3 Matrix4x4::Right(bool ViewSpace) const
		{
			if (ViewSpace)
				return Vector3(Row[0], -Row[1], -Row[2]);
			else
				return Vector3(Row[0], Row[1], Row[2]);
		}
		Vector3 Matrix4x4::Forward(bool ViewSpace) const
		{
			if (ViewSpace)
				return Vector3(Row[8], Row[9], -Row[10]);
			else
				return Vector3(Row[8], Row[9], Row[10]);
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
		Vector3 Matrix4x4::Rotation() const
		{
#ifdef TH_WITH_SIMD
			float X = -atan2(-Row[6], Row[10]);
			float sX = sin(X), cX = cos(X);
			LOD_AV2(_r1, Row[0], Row[1]);
			LOD_AV2(_r2, Row[4], Row[8]);
			LOD_AV2(_r3, Row[5], Row[9]);
			LOD_AV2(_r4, cX, sX);

			return Vector3(X,
				-atan2(Row[2], Common::FastSqrt(horizontal_add(square(_r1)))),
				-atan2(horizontal_add(_r4 * _r2), horizontal_add(_r4 * _r3)));
#else
			float X = -atan2(-Row[6], Row[10]);
			float sX = sin(X), cX = cos(X);
			return Vector3(X,
				-atan2(Row[2], Common::FastSqrt(Row[0] * Row[0] + Row[1] * Row[1])),
				-atan2(cX * Row[4] + sX * Row[8], cX * Row[5] + sX * Row[9]));
#endif
		}
		Vector3 Matrix4x4::Position() const
		{
			return Vector3(Row[12], Row[13], Row[14]);
		}
		Vector3 Matrix4x4::Scale() const
		{
			return Vector3(Row[0], Row[5], Row[10]);
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
			if (TH_LEFT_HANDED)
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
		Matrix4x4 Matrix4x4::CreateOrigin(const Vector3& Position, const Vector3& Rotation)
		{
			Matrix4x4 Result =
				Matrix4x4::CreateTranslation(-Position) *
				Matrix4x4::CreateRotationY(-Rotation.Y) *
				Matrix4x4::CreateRotationX(-Rotation.X);

			return Result;
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
					if (TH_LEFT_HANDED)
						return Matrix4x4::CreateLookAt(Position, Position + Vector3(0, 1, 0), Vector3::Backward());
					else
						return Matrix4x4::CreateLookAt(Position, Position - Vector3(0, 1, 0), Vector3::Forward());
				case CubeFace::NegativeY:
					if (TH_LEFT_HANDED)
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

		Quaternion::Quaternion() : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f)
		{
		}
		Quaternion::Quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w)
		{
		}
		Quaternion::Quaternion(const Quaternion& In) : X(In.X), Y(In.Y), Z(In.Z), W(In.W)
		{
		}
		Quaternion::Quaternion(const Vector3& Axis, float Angle)
		{
			SetAxis(Axis, Angle);
		}
		Quaternion::Quaternion(const Vector3& Euler)
		{
			SetEuler(Euler);
		}
		Quaternion::Quaternion(const Matrix4x4& Value)
		{
			SetMatrix(Value);
		}
		void Quaternion::SetAxis(const Vector3& Axis, float Angle)
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_AV3(_r1, Value[0], Value[5], Value[9]);
			float T = horizontal_add(_r1);
			if (T <= 0.0f)
			{
				if (Value[0] > Value[5] && Value[0] > Value[9])
				{
					LOD_AV4(_r2, 1.0f, Value[0], -Value[5], -Value[9]);
					LOD_AV4(_r3, 0.25f, Value[4], Value[7], Value[6]);
					LOD_AV4(_r4, 0.0f, Value[1], Value[2], -Value[8]);
					float F = 0.5f / Common::FastSqrt(horizontal_add(_r2));
					_r3 += _r4;
					_r3 *= F;
					_r3.store((float*)this);
					X = 0.25f / F;
				}
				else if (Value[5] > Value[9])
				{
					LOD_AV4(_r2, 1.0f, Value[5], -Value[0], -Value[9]);
					LOD_AV4(_r3, Value[4], 0.25f, Value[8], Value[7]);
					LOD_AV4(_r4, Value[1], 0.0f, Value[6], -Value[2]);
					float F = 0.5f / Common::FastSqrt(horizontal_add(_r2));
					_r3 += _r4;
					_r3 *= F;
					_r3.store((float*)this);
					Y = 0.25f / F;
				}
				else
				{
					LOD_AV4(_r2, 1.0f, Value[9], -Value[0], -Value[5]);
					LOD_AV4(_r3, Value[7], Value[6], 0.25f, Value[1]);
					LOD_AV4(_r4, Value[2], Value[8], 0.0f, -Value[4]);
					float F = 0.5f / Common::FastSqrt(horizontal_add(_r2));
					_r3 += _r4;
					_r3 *= F;
					_r3.store((float*)this);
					Z = 0.25f / F;
				}
			}
			else
			{
				LOD_AV4(_r2, 0.0f, Value[8], Value[2], Value[4]);
				LOD_AV4(_r3, 0.0f, Value[6], Value[7], Value[1]);
				float F = 0.5f / Common::FastSqrt(T + 1.0f);
				_r3 -= _r2;
				_r3 *= F;
				_r3.store((float*)this);
				X = 0.25f / F;
			}

			LOD_FV4(_r4);
			_r4 /= Common::FastSqrt(horizontal_add(square(_r4)));
			_r4.store((float*)this);
#else
			float T = Value[0] + Value[5] + Value[9];
			if (T <= 0.0f)
			{
				if (Value[0] > Value[5] && Value[0] > Value[9])
				{
					float F = 2.0f * Common::FastSqrt(1.0f + Value[0] - Value[5] - Value[9]);
					X = 0.25f * F;
					Y = (Value[4] + Value[1]) / F;
					Z = (Value[7] + Value[2]) / F;
					W = (Value[6] - Value[8]) / F;
				}
				else if (Value[5] > Value[9])
				{
					float F = 2.0f * Common::FastSqrt(1.0f + Value[5] - Value[0] - Value[9]);
					X = (Value[4] + Value[1]) / F;
					Y = 0.25f * F;
					Z = (Value[8] + Value[6]) / F;
					W = (Value[7] - Value[2]) / F;
				}
				else
				{
					float F = 2.0f * Common::FastSqrt(1.0f + Value[9] - Value[0] - Value[5]);
					X = (Value[7] + Value[2]) / F;
					Y = (Value[6] + Value[8]) / F;
					Z = 0.25f * F;
					W = (Value[1] - Value[4]) / F;
				}
			}
			else
			{
				float F = 0.5f / Common::FastSqrt(T + 1.0f);
				X = 0.25f / F;
				Y = (Value[6] - Value[8]) * F;
				Z = (Value[7] - Value[2]) * F;
				W = (Value[1] - Value[4]) * F;
			}

			float F = Common::FastSqrt(X * X + Y * Y + Z * Z + W * W);
			X /= F;
			Y /= F;
			Z /= F;
			W /= F;
#endif
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
		Quaternion& Quaternion::operator =(const Quaternion& R)
		{
			this->X = R.X;
			this->Y = R.Y;
			this->Z = R.Z;
			this->W = R.W;
			return *this;
		}
		Quaternion Quaternion::Normalize() const
		{
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			_r1 /= Common::FastSqrt(horizontal_add(square(_r1)));

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
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			float F = Common::FastSqrt(horizontal_add(square(_r1)));
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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

			float Sin = Common::FastSqrt(1.0f - Cos * Cos);
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
			return Matrix4x4::CreateRotation(Forward(), Up(), Right());
		}
		Vector3 Quaternion::GetEuler() const
		{
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
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
#ifdef TH_WITH_SIMD
			LOD_FV4(_r1);
			return Common::FastSqrt(horizontal_add(square(_r1)));
#else
			return Common::FastSqrt(X * X + Y * Y + Z * Z + W * W);
#endif
		}

		RandomVector2::RandomVector2() : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomVector2::RandomVector2(const Vector2& MinV, const Vector2& MaxV, bool IntensityV, float AccuracyV) : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
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

		RandomVector3::RandomVector3() : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomVector3::RandomVector3(const Vector3& MinV, const Vector3& MaxV, bool IntensityV, float AccuracyV) : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
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

		RandomVector4::RandomVector4() : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomVector4::RandomVector4(const Vector4& MinV, const Vector4& MaxV, bool IntensityV, float AccuracyV) : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
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

		RandomFloat::RandomFloat() : Min(0), Max(1), Intensity(false), Accuracy(1)
		{
		}
		RandomFloat::RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV) : Min(MinV), Max(MaxV), Intensity(IntensityV), Accuracy(AccuracyV)
		{
		}
		float RandomFloat::Generate()
		{
			return (Mathf::Random(Min * Accuracy, Max * Accuracy) / Accuracy) * (Intensity ? Mathf::Random() : 1);
		}

		Hybi10Request::Hybi10Request()
		{
			Type = -1;
			ExitCode = 0;
		}
		std::string Hybi10Request::GetTextType() const
		{
			if (Type == 0 || Type == 1 || Type == 2)
				return "text";

			if (Type == 8)
				return "close";

			if (Type == 9)
				return "ping";

			if (Type == 10)
				return "pong";

			return "unknown";
		}
		Hybi10_Opcode Hybi10Request::GetEnumType() const
		{
			if (Type == 0 || Type == 1 || Type == 2)
				return Hybi10_Opcode::Text;

			if (Type == 8)
				return Hybi10_Opcode::Close;

			if (Type == 9)
				return Hybi10_Opcode::Ping;

			if (Type == 10)
				return Hybi10_Opcode::Pong;

			return Hybi10_Opcode::Invalid;
		}

		RegexSource::RegexSource() :
			MaxBranches(128), MaxBrackets(128), MaxMatches(128),
			State(RegexState::No_Match), IgnoreCase(false)
		{
		}
		RegexSource::RegexSource(const std::string& Regexp, bool fIgnoreCase, int64_t fMaxMatches, int64_t fMaxBranches, int64_t fMaxBrackets) :
			Expression(Regexp),
			MaxBranches(fMaxBranches >= 1 ? fMaxBranches : 128),
			MaxBrackets(fMaxBrackets >= 1 ? fMaxBrackets : 128),
			MaxMatches(fMaxMatches >= 1 ? fMaxMatches : 128),
			State(RegexState::Preprocessed), IgnoreCase(fIgnoreCase)
		{
			Compile();
		}
		RegexSource::RegexSource(const RegexSource& Other) :
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
		RegexSource& RegexSource::operator=(const RegexSource& V)
		{
			TH_ASSERT(this != &V, *this, "cannot set to self");
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
			TH_ASSERT(this != &V, *this, "cannot set to self");
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
		const std::string& RegexSource::GetRegex() const
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

			for (size_t i = 0; i < B; i++)
				C += Brackets[i].Length;

			return (int64_t)Expression.size() + A * B * C;
		}
		RegexState RegexSource::GetState() const
		{
			return State;
		}
		bool RegexSource::IsSimple() const
		{
			Core::Parser Tx(&Expression);
			return !Tx.FindOf("\\+*?|[]").Found;
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
					Brackets[Idx].Length = (int64_t)(&vPtr[i] - Brackets[Idx].Pointer); Depth--;
					REGEX_FAIL_IN(Depth < 0, RegexState::Unbalanced_Brackets);
					REGEX_FAIL_IN(i > 0 && vPtr[i - 1] == '(', RegexState::No_Match);
				}
			}

			REGEX_FAIL_IN(Depth != 0, RegexState::Unbalanced_Brackets);

			RegexBranch Branch;
			int64_t i, j;

			for (i = 0; i < (int64_t)Branches.size(); i++)
			{
				for (j = i + 1; j < (int64_t)Branches.size(); j++)
				{
					if (Branches[i].BracketIndex > Branches[j].BracketIndex)
					{
						Branch = Branches[i];
						Branches[i] = Branches[j];
						Branches[j] = Branch;
					}
				}
			}

			for (i = j = 0; i < (int64_t)Brackets.size(); i++)
			{
				auto& Bracket = Brackets[i];
				Bracket.BranchesCount = 0;
				Bracket.Branches = j;

				while (j < (int64_t)Branches.size() && Branches[j].BracketIndex == i)
				{
					Bracket.BranchesCount++;
					j++;
				}
			}
		}

		RegexResult::RegexResult() : State(RegexState::No_Match), Steps(0), Src(nullptr)
		{
		}
		RegexResult::RegexResult(const RegexResult& Other) : Matches(Other.Matches), State(Other.State), Steps(Other.Steps), Src(Other.Src)
		{
		}
		RegexResult::RegexResult(RegexResult&& Other) noexcept : Matches(std::move(Other.Matches)), State(Other.State), Steps(Other.Steps), Src(Other.Src)
		{
		}
		RegexResult& RegexResult::operator =(const RegexResult& V)
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
		const std::vector<RegexMatch>& RegexResult::Get() const
		{
			return Matches;
		}
		std::vector<std::string> RegexResult::ToArray() const
		{
			std::vector<std::string> Array;
			Array.reserve(Matches.size());

			for (auto& Item : Matches)
				Array.emplace_back(Item.Pointer, (size_t)Item.Length);

			return Array;
		}

		bool Regex::Match(RegexSource* Value, RegexResult& Result, const std::string& Buffer)
		{
			return Match(Value, Result, Buffer.c_str(), Buffer.size());
		}
		bool Regex::Match(RegexSource* Value, RegexResult& Result, const char* Buffer, int64_t Length)
		{
			TH_ASSERT(Value != nullptr && Value->State == RegexState::Preprocessed, false, "invalid regex source");
			TH_ASSERT(Buffer != nullptr && Length > 0, false, "invalid buffer");

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
		bool Regex::Replace(std::string& Source, const std::string& From, const std::string& To)
		{
			Core::Parser Parser(&Source), Emplace;
			RegexSource Base('(' + From + ')');
			RegexResult Result;
			size_t Matches = 0;

			bool Expression = (!To.empty() && To.find('$') != std::string::npos);
			if (!Expression)
				Emplace.Assign(To);

			size_t Start = 0;
			while (Match(&Base, Result, Source.c_str() + Start, Source.size() - Start))
			{
				Matches++;
				if (Result.Matches.empty())
					continue;

				if (Expression)
				{
					Emplace.Assign(To);
					for (size_t i = 1; i < Result.Matches.size(); i++)
					{
						auto& Item = Result.Matches[i];
						Emplace.Replace("$" + std::to_string(i), std::string(Item.Pointer, (size_t)Item.Length));
					}
				}

				auto& Where = Result.Matches.front();
				Parser.ReplacePart(Where.Start + Start, Where.End + Start, Emplace.R());
				Start += (size_t)Where.Start + (size_t)Emplace.Size() - (Emplace.Empty() ? 0 : 1);
			}

			return Matches > 0;
		}
		int64_t Regex::Meta(const unsigned char* Buffer)
		{
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			static const char* Chars = "^$().[]*+?|\\Ssdbfnrtv";
			return strchr(Chars, *Buffer) != nullptr;
		}
		int64_t Regex::OpLength(const char* Value)
		{
			TH_ASSERT(Value != nullptr, 1, "invalid value");
			return Value[0] == '\\' && Value[1] == 'x' ? 4 : Value[0] == '\\' ? 2 : 1;
		}
		int64_t Regex::SetLength(const char* Value, int64_t ValueLength)
		{
			TH_ASSERT(Value != nullptr, -1, "invalid value");
			int64_t Length = 0;
			while (Length < ValueLength && Value[Length] != ']')
				Length += OpLength(Value + Length);

			return Length <= ValueLength ? Length + 1 : -1;
		}
		int64_t Regex::GetOpLength(const char* Value, int64_t ValueLength)
		{
			TH_ASSERT(Value != nullptr, -1, "invalid value");
			return Value[0] == '[' ? SetLength(Value + 1, ValueLength - 1) + 1 : OpLength(Value);
		}
		int64_t Regex::Quantifier(const char* Value)
		{
			TH_ASSERT(Value != nullptr, 0, "invalid value");
			return Value[0] == '*' || Value[0] == '+' || Value[0] == '?';
		}
		int64_t Regex::ToInt(int64_t x)
		{
			return (int64_t)(isdigit((int)x) ? (int)x - '0' : (int)x - 'W');
		}
		int64_t Regex::HexToInt(const unsigned char* Buffer)
		{
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			return (ToInt(tolower(Buffer[0])) << 4) | ToInt(tolower(Buffer[1]));
		}
		int64_t Regex::MatchOp(const unsigned char* Value, const unsigned char* Buffer, RegexResult* Info)
		{
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			TH_ASSERT(Value != nullptr, 0, "invalid value");
			TH_ASSERT(Info != nullptr, 0, "invalid regex result");

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
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			TH_ASSERT(Value != nullptr, 0, "invalid value");
			TH_ASSERT(Info != nullptr, 0, "invalid regex result");

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
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			TH_ASSERT(Value != nullptr, 0, "invalid value");
			TH_ASSERT(Info != nullptr, 0, "invalid regex result");

			int64_t i, j, n, Step;
			for (i = j = 0; i < ValueLength && j <= BufferLength; i += Step)
			{
				Step = Value[i] == '(' ? Info->Src->Brackets[Case + 1].Length + 2 : GetOpLength(Value + i, ValueLength - i);
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
							Match = &Info->Matches.at(Case - 1);

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
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			TH_ASSERT(Info != nullptr, 0, "invalid regex result");

			const RegexBracket* Bk = &Info->Src->Brackets[Case];
			int64_t i = 0, Length, Result;
			const char* Ptr;

			do
			{
				Ptr = i == 0 ? Bk->Pointer : Info->Src->Branches[Bk->Branches + i - 1].Pointer + 1;
				Length = Bk->BranchesCount == 0 ? Bk->Length : i == Bk->BranchesCount ? (int64_t)(Bk->Pointer + Bk->Length - Ptr) : (int64_t)(Info->Src->Branches[Bk->Branches + i].Pointer - Ptr);
				Result = ParseInner(Ptr, Length, Buffer, BufferLength, Info, Case);
				TH_PSIG();
			} while (Result <= 0 && i++ < Bk->BranchesCount);

			return Result;
		}
		int64_t Regex::Parse(const char* Buffer, int64_t BufferLength, RegexResult* Info)
		{
			TH_ASSERT(Buffer != nullptr, 0, "invalid buffer");
			TH_ASSERT(Info != nullptr, 0, "invalid regex result");
			TH_PPUSH("regex-parse", TH_PERF_FRAME);

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

			TH_PPOP();
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

		CollisionBody::CollisionBody(btCollisionObject* Object)
		{
#ifdef TH_WITH_BULLET3
			btRigidBody* RigidObject = btRigidBody::upcast(Object);
			if (RigidObject != nullptr)
				Rigid = (RigidBody*)RigidObject->getUserPointer();

			btSoftBody* SoftObject = btSoftBody::upcast(Object);
			if (SoftObject != nullptr)
				Soft = (SoftBody*)SoftObject->getUserPointer();
#endif
		}

		Adjacencies::Adjacencies() : NbEdges(0), CurrentNbFaces(0), Edges(nullptr), NbFaces(0), Faces(nullptr)
		{
		}
		Adjacencies::~Adjacencies()
		{
			TH_FREE(Faces);
			TH_FREE(Edges);
		}
		bool Adjacencies::Fill(Adjacencies::Desc& create)
		{
			NbFaces = create.NbFaces;
			Faces = (AdjTriangle*)TH_MALLOC(sizeof(AdjTriangle) * NbFaces);
			Edges = (AdjEdge*)TH_MALLOC(sizeof(AdjEdge) * NbFaces * 3);

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
			unsigned int* FaceNb = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * NbEdges);
			unsigned int* VRefs0 = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * NbEdges);
			unsigned int* VRefs1 = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * NbEdges);

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
						TH_FREE(FaceNb);
						TH_FREE(VRefs0);
						TH_FREE(VRefs1);
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
							TH_FREE(FaceNb);
							TH_FREE(VRefs0);
							TH_FREE(VRefs1);
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

			TH_FREE(FaceNb);
			TH_FREE(VRefs0);
			TH_FREE(VRefs1);
			TH_FREE(Edges);

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

		TriangleStrip::TriangleStrip() : Adj(nullptr), Tags(nullptr), NbStrips(0), TotalLength(0), OneSided(false), SGICipher(false), ConnectAllStrips(false)
		{
		}
		TriangleStrip::~TriangleStrip()
		{
			FreeBuffers();
		}
		TriangleStrip& TriangleStrip::FreeBuffers()
		{
			std::vector<unsigned int>().swap(SingleStrip);
			std::vector<unsigned int>().swap(StripRuns);
			std::vector<unsigned int>().swap(StripLengths);
			TH_FREE(Tags);
			Tags = nullptr;

			TH_DELETE(Adjacencies, Adj);
			Adj = nullptr;

			return *this;
		}
		bool TriangleStrip::Fill(const TriangleStrip::Desc& create)
		{
			Adjacencies::Desc ac;
			ac.NbFaces = create.NbFaces;
			ac.Faces = create.Faces;
			FreeBuffers();

			Adj = TH_NEW(Adjacencies);
			if (!Adj->Fill(ac))
			{
				TH_DELETE(Adjacencies, Adj);
				Adj = nullptr;
				return false;
			}

			if (!Adj->Resolve())
			{
				TH_DELETE(Adjacencies, Adj);
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
			TH_ASSERT(Adj != nullptr, false, "triangle strip should be initialized");
			Tags = (bool*)TH_MALLOC(sizeof(bool) * Adj->NbFaces);
			unsigned int* Connectivity = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * Adj->NbFaces);

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

			TH_FREE(Connectivity);
			TH_FREE(Tags);
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
				Strip[j] = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * (Adj->NbFaces + 2 + 1 + 2));
				Faces[j] = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * (Adj->NbFaces + 2));
				memset(Strip[j], 0xff, (Adj->NbFaces + 2 + 1 + 2) * sizeof(unsigned int));
				memset(Faces[j], 0xff, (Adj->NbFaces + 2) * sizeof(unsigned int));

				bool* vTags = (bool*)TH_MALLOC(sizeof(bool) * Adj->NbFaces);
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
				TH_FREE(vTags);
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
				TH_FREE(Faces[j]);
				TH_FREE(Strip[j]);
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
			result.Groups = std::vector<unsigned int>({ TotalLength });

			return true;
		}

		std::vector<int> TriangleStrip::Result::GetIndices(int Group)
		{
			std::vector<int> Indices;
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
		std::vector<int> TriangleStrip::Result::GetInvIndices(int Group)
		{
			std::vector<int> Indices = GetIndices(Group);
			std::reverse(Indices.begin(), Indices.end());

			return Indices;
		}

		RadixSorter::RadixSorter() : CurrentSize(0), Indices(nullptr), Indices2(nullptr)
		{
			Histogram = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * 256 * 4);
			Offset = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * 256);
			ResetIndices();
		}
		RadixSorter::RadixSorter(const RadixSorter& Other) : CurrentSize(0), Indices(nullptr), Indices2(nullptr)
		{
			Histogram = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * 256 * 4);
			Offset = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * 256);
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
		RadixSorter::~RadixSorter()
		{
			TH_FREE(Offset);
			TH_FREE(Histogram);
			TH_FREE(Indices2);
			TH_FREE(Indices);
		}
		RadixSorter& RadixSorter::operator =(const RadixSorter& V)
		{
			TH_FREE(Histogram);
			Histogram = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * 256 * 4);

			TH_FREE(Offset);
			Offset = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * 256);
			ResetIndices();

			return *this;
		}
		RadixSorter& RadixSorter::operator =(RadixSorter&& Other) noexcept
		{
			TH_FREE(Offset);
			TH_FREE(Histogram);
			TH_FREE(Indices2);
			TH_FREE(Indices);
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
				TH_FREE(Indices2);
				TH_FREE(Indices);
				Indices = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * nb);
				Indices2 = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * nb);
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
				TH_FREE(Indices2);
				TH_FREE(Indices);
				Indices = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * nb);
				Indices2 = (unsigned int*)TH_MALLOC(sizeof(unsigned int) * nb);
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

		MD5Hasher::MD5Hasher()
		{
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
			TH_ASSERT_V(Output != nullptr && Input != nullptr, "output and input should be set");
			for (unsigned int i = 0, j = 0; j < Length; i++, j += 4)
				Output[i] = ((UInt4)Input[j]) | (((UInt4)Input[j + 1]) << 8) | (((UInt4)Input[j + 2]) << 16) | (((UInt4)Input[j + 3]) << 24);
		}
		void MD5Hasher::Encode(UInt1* Output, const UInt4* Input, unsigned int Length)
		{
			TH_ASSERT_V(Output != nullptr && Input != nullptr, "output and input should be set");
			for (unsigned int i = 0, j = 0; j < Length; i++, j += 4)
			{
				Output[j] = Input[i] & 0xff;
				Output[j + 1] = (Input[i] >> 8) & 0xff;
				Output[j + 2] = (Input[i] >> 16) & 0xff;
				Output[j + 3] = (Input[i] >> 24) & 0xff;
			}
		}
		void MD5Hasher::Transform(const UInt1* Block, unsigned int Length)
		{
			TH_ASSERT_V(Block != nullptr, "block should be set");
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
#ifdef TH_MICROSOFT
			RtlSecureZeroMemory(X, sizeof(X));
#else
			memset(X, 0, sizeof(X));
#endif
		}
		void MD5Hasher::Update(const std::string& Input, unsigned int BlockSize)
		{
			Update(Input.c_str(), (unsigned int)Input.size(), BlockSize);
		}
		void MD5Hasher::Update(const unsigned char* Input, unsigned int Length, unsigned int BlockSize)
		{
			TH_ASSERT_V(Input != nullptr, "input should be set");
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
			TH_ASSERT(Finalized, nullptr, "md5 hash should be finalized");
			char* Output = (char*)TH_MALLOC(sizeof(char) * 33);
			memset((void*)Output, 0, 33);

			for (int i = 0; i < 16; i++)
				sprintf(Output + i * 2, "%02x", Digest[i]);
			Output[32] = '\0';

			return Output;
		}
		unsigned char* MD5Hasher::RawDigest() const
		{
			TH_ASSERT(Finalized, nullptr, "md5 hash should be finalized");
			UInt1* Output = (UInt1*)TH_MALLOC(sizeof(UInt1) * 17);
			memcpy(Output, Digest, 16);
			Output[16] = '\0';

			return Output;
		}
		std::string MD5Hasher::ToHex() const
		{
			TH_ASSERT(Finalized, std::string(), "md5 hash should be finalized");
			char Data[33];
			for (int i = 0; i < 16; i++)
				sprintf(Data + i * 2, "%02x", Digest[i]);
			Data[32] = 0;

			return Data;
		}
		std::string MD5Hasher::ToRaw() const
		{
			TH_ASSERT(Finalized, std::string(), "md5 hash should be finalized");
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB1()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB8()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB128()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_OFB()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CTR()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CCM()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_GCM()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_XTS()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_xts();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_Wrap()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_WrapPad()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_OCB()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CBC()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB1()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB8()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB128()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_OFB()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CTR()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CCM()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_GCM()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_192_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_Wrap()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_192_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_WrapPad()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_192_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_OCB()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB1()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB8()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB128()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_OFB()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CTR()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CCM()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_GCM()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_XTS()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_xts();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_Wrap()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_WrapPad()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_OCB()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_128_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC_HMAC_SHA1()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_aes_256_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC_HMAC_SHA256()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC_HMAC_SHA256()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_ECB()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
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
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_sha1();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA224()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_sha224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA256()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_sha256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA384()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_sha384();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512()
		{
#ifdef TH_HAS_OPENSSL
			return (Cipher)EVP_sha512();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512_224()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha512_224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512_256()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha512_256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_224()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_256()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_384()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_384();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_512()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_512();
#else
			return nullptr;
#endif
		}
		Digest Digests::Shake128()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_shake128();
#else
			return nullptr;
#endif
		}
		Digest Digests::Shake256()
		{
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_shake256();
#else
			return nullptr;
#endif
		}
		Digest Digests::MDC2()
		{
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#ifdef TH_HAS_OPENSSL
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
#if TH_HAS_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM3
			return (Cipher)EVP_sm3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		bool Common::IsCubeInFrustum(const Matrix4x4& WVP, float Radius)
		{
			Radius = -Radius;
#ifdef TH_WITH_SIMD
			LOD_AV4(_r1, WVP.Row[3], WVP.Row[7], WVP.Row[11], WVP.Row[15]);
			LOD_AV4(_r2, WVP.Row[0], WVP.Row[4], WVP.Row[8], WVP.Row[12]);
			LOD_VAL(_r3, _r1 + _r2);
			float F = _r3.extract(3); _r3.cutoff(3);
			F /= Common::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r3 = _r1 - _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Common::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r2 = Vec4f(WVP.Row[1], WVP.Row[5], WVP.Row[9], WVP.Row[13]);
			_r3 = _r1 + _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Common::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r3 = _r1 - _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Common::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r2 = Vec4f(WVP.Row[2], WVP.Row[6], WVP.Row[10], WVP.Row[14]);
			_r3 = _r1 + _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Common::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;

			_r2 = Vec4f(WVP.Row[2], WVP.Row[6], WVP.Row[10], WVP.Row[14]);
			_r3 = _r1 - _r2;
			F = _r3.extract(3); _r3.cutoff(3);
			F /= Common::FastSqrt(horizontal_add(square(_r3)));
			if (F <= Radius)
				return false;
#else
			float Plane[4];
			Plane[0] = WVP.Row[3] + WVP.Row[0];
			Plane[1] = WVP.Row[7] + WVP.Row[4];
			Plane[2] = WVP.Row[11] + WVP.Row[8];
			Plane[3] = WVP.Row[15] + WVP.Row[12];

			Plane[3] /= Common::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] - WVP.Row[0];
			Plane[1] = WVP.Row[7] - WVP.Row[4];
			Plane[2] = WVP.Row[11] - WVP.Row[8];
			Plane[3] = WVP.Row[15] - WVP.Row[12];

			Plane[3] /= Common::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] + WVP.Row[1];
			Plane[1] = WVP.Row[7] + WVP.Row[5];
			Plane[2] = WVP.Row[11] + WVP.Row[9];
			Plane[3] = WVP.Row[15] + WVP.Row[13];

			Plane[3] /= Common::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] - WVP.Row[1];
			Plane[1] = WVP.Row[7] - WVP.Row[5];
			Plane[2] = WVP.Row[11] - WVP.Row[9];
			Plane[3] = WVP.Row[15] - WVP.Row[13];

			Plane[3] /= Common::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] + WVP.Row[2];
			Plane[1] = WVP.Row[7] + WVP.Row[6];
			Plane[2] = WVP.Row[11] + WVP.Row[10];
			Plane[3] = WVP.Row[15] + WVP.Row[14];

			Plane[3] /= Common::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;

			Plane[0] = WVP.Row[3] - WVP.Row[2];
			Plane[1] = WVP.Row[7] - WVP.Row[6];
			Plane[2] = WVP.Row[11] - WVP.Row[10];
			Plane[3] = WVP.Row[15] - WVP.Row[14];

			Plane[3] /= Common::FastSqrt(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= Radius)
				return false;
#endif
			return true;
		}
		bool Common::IsBase64(unsigned char Value)
		{
			return (isalnum(Value) || (Value == '+') || (Value == '/'));
		}
		bool Common::IsBase64URL(unsigned char Value)
		{
			return (isalnum(Value) || (Value == '-') || (Value == '_'));
		}
		bool Common::IsLeftHanded()
		{
			return LeftHanded;
		}
		bool Common::HasSphereIntersected(const Vector3& PositionR0, float RadiusR0, const Vector3& PositionR1, float RadiusR1)
		{
			if (PositionR0.Distance(PositionR1) < RadiusR0 + RadiusR1)
				return true;

			return false;
		}
		bool Common::HasLineIntersected(float Distance0, float Distance1, const Vector3& Point0, const Vector3& Point1, Vector3& Hit)
		{
			if ((Distance0 * Distance1) >= 0)
				return false;

			if (Distance0 == Distance1)
				return false;

			Hit = Point0 + (Point1 - Point0) * (-Distance0 / (Distance1 - Distance0));
			return true;
		}
		bool Common::HasLineIntersectedCube(const Vector3& Min, const Vector3& Max, const Vector3& Start, const Vector3& End)
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
		bool Common::HasPointIntersectedCube(const Vector3& LastHit, const Vector3& Min, const Vector3& Max, int Axis)
		{
			if (Axis == 1 && LastHit.Z > Min.Z && LastHit.Z < Max.Z && LastHit.Y > Min.Y && LastHit.Y < Max.Y)
				return true;

			if (Axis == 2 && LastHit.Z > Min.Z && LastHit.Z < Max.Z && LastHit.X > Min.X && LastHit.X < Max.X)
				return true;

			if (Axis == 3 && LastHit.X > Min.X && LastHit.X < Max.X && LastHit.Y > Min.Y && LastHit.Y < Max.Y)
				return true;

			return false;
		}
		bool Common::HasPointIntersectedCube(const Vector3& Position, const Vector3& Scale, const Vector3& P0)
		{
			return (P0.X) <= (Position.X + Scale.X) && (Position.X - Scale.X) <= (P0.X) && (P0.Y) <= (Position.Y + Scale.Y) && (Position.Y - Scale.Y) <= (P0.Y) && (P0.Z) <= (Position.Z + Scale.Z) && (Position.Z - Scale.Z) <= (P0.Z);
		}
		bool Common::HasPointIntersectedRectangle(const Vector3& Position, const Vector3& Scale, const Vector3& P0)
		{
			return P0.X >= Position.X - Scale.X && P0.X < Position.X + Scale.X && P0.Y >= Position.Y - Scale.Y && P0.Y < Position.Y + Scale.Y;
		}
		bool Common::HasSBIntersected(Transform* R0, Transform* R1)
		{
			if (!HasSphereIntersected(R0->GetPosition(), R0->GetScale().Hypotenuse(), R1->GetPosition(), R1->GetScale().Hypotenuse()))
				return false;

			return true;
		}
		bool Common::HasOBBIntersected(Transform* R0, Transform* R1)
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
		bool Common::HasAABBIntersected(Transform* R0, Transform* R1)
		{
			TH_ASSERT(R0 != nullptr && R1 != nullptr, false, "transforms should be set");
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
		bool Common::Hex(char c, int& v)
		{
			if (0x20 <= c && isdigit(c))
			{
				v = c - '0';
				return true;
			}
			else if ('A' <= c && c <= 'F')
			{
				v = c - 'A' + 10;
				return true;
			}
			else if ('a' <= c && c <= 'f')
			{
				v = c - 'a' + 10;
				return true;
			}

			return false;
		}
		bool Common::HexToString(void* Data, uint64_t Length, char* Buffer, uint64_t BufferLength)
		{
			TH_ASSERT(Data != nullptr && Length > 0, false, "data buffer should be set");
			TH_ASSERT(Buffer != nullptr && BufferLength > 0, false, "buffer should be set");
			TH_ASSERT(BufferLength >= (3 * Length), false, "buffer is too small");

			static const char HEX[] = "0123456789abcdef";
			for (int i = 0; i < Length; i++)
			{
				if (i > 0)
					Buffer[3 * i - 1] = ' ';

				Buffer[3 * i] = HEX[(((uint8_t*)Data)[i] >> 4) & 0xF];
				Buffer[3 * i + 1] = HEX[((uint8_t*)Data)[i] & 0xF];
			}

			Buffer[3 * Length - 1] = 0;
			return true;
		}
		bool Common::HexToDecimal(const std::string& s, uint64_t i, uint64_t cnt, int& Value)
		{
			TH_ASSERT(i < s.size(), false, "index outside of range");

			Value = 0;
			for (; cnt; i++, cnt--)
			{
				if (!s[i])
					return false;

				int v = 0;
				if (!Hex(s[i], v))
					return false;

				Value = Value * 16 + v;
			}

			return true;
		}
		void Common::FlipIndexWindingOrder(std::vector<int>& Indices)
		{
			std::reverse(Indices.begin(), Indices.end());
		}
		void Common::ComputeInfluenceNormals(std::vector<SkinVertex>& Vertices)
		{
			Vector3 Tangent, Bitangent;
			for (uint64_t i = 0; i < Vertices.size(); i += 3)
			{
				SkinVertex& V1 = Vertices[i], & V2 = Vertices[i + 1], & V3 = Vertices[i + 2];
				ComputeInfluenceTangentBitangent(V1, V2, V3, Tangent, Bitangent);
				V1.TangentX = Tangent.X;
				V1.TangentY = Tangent.Y;
				V1.TangentZ = Tangent.Z;
				V1.BitangentX = Bitangent.X;
				V1.BitangentY = Bitangent.Y;
				V1.BitangentZ = Bitangent.Z;
				V2.TangentX = Tangent.X;
				V2.TangentY = Tangent.Y;
				V2.TangentZ = Tangent.Z;
				V2.BitangentX = Bitangent.X;
				V2.BitangentY = Bitangent.Y;
				V2.BitangentZ = Bitangent.Z;
				V3.TangentX = Tangent.X;
				V3.TangentY = Tangent.Y;
				V3.TangentZ = Tangent.Z;
				V3.BitangentX = Bitangent.X;
				V3.BitangentY = Bitangent.Y;
				V3.BitangentZ = Bitangent.Z;
			}
		}
		void Common::ComputeInfluenceNormalsArray(SkinVertex* Vertices, uint64_t Count)
		{
			Vector3 Tangent, Bitangent;
			for (uint64_t i = 0; i < Count; i += 3)
			{
				SkinVertex& V1 = Vertices[i], & V2 = Vertices[i + 1], & V3 = Vertices[i + 2];
				ComputeInfluenceTangentBitangent(V1, V2, V3, Tangent, Bitangent);
				V1.TangentX = Tangent.X;
				V1.TangentY = Tangent.Y;
				V1.TangentZ = Tangent.Z;
				V1.BitangentX = Bitangent.X;
				V1.BitangentY = Bitangent.Y;
				V1.BitangentZ = Bitangent.Z;
				V2.TangentX = Tangent.X;
				V2.TangentY = Tangent.Y;
				V2.TangentZ = Tangent.Z;
				V2.BitangentX = Bitangent.X;
				V2.BitangentY = Bitangent.Y;
				V2.BitangentZ = Bitangent.Z;
				V3.TangentX = Tangent.X;
				V3.TangentY = Tangent.Y;
				V3.TangentZ = Tangent.Z;
				V3.BitangentX = Bitangent.X;
				V3.BitangentY = Bitangent.Y;
				V3.BitangentZ = Bitangent.Z;
			}
		}
		void Common::ComputeInfluenceTangentBitangent(const SkinVertex& V1, const SkinVertex& V2, const SkinVertex& V3, Vector3& Tangent, Vector3& Bitangent, Vector3& Normal)
		{
			Vector3 Face1(V2.PositionX - V1.PositionX, V2.PositionY - V1.PositionY, V2.PositionZ - V1.PositionZ);
			Vector3 Face2(V3.PositionX - V1.PositionX, V3.PositionY - V1.PositionY, V3.PositionZ - V1.PositionZ);
			Vector2 Coord1(V2.TexCoordX - V1.TexCoordX, V3.TexCoordX - V1.TexCoordX);
			Vector2 Coord2(V2.TexCoordY - V1.TexCoordY, V3.TexCoordY - V1.TexCoordY);
			float Ray = 1.0f / (Coord1.X * Coord2.Y - Coord1.Y * Coord2.X);

			Tangent.X = (Coord1.Y * Face1.X - Coord1.X * Face2.X) * Ray;
			Tangent.Y = (Coord1.Y * Face1.Y - Coord1.X * Face2.Y) * Ray;
			Tangent.Z = (Coord1.Y * Face1.Z - Coord1.X * Face2.Z) * Ray;
			Tangent = Tangent.sNormalize();

			Bitangent.X = (Coord2.X * Face2.X - Coord2.Y * Face1.X) * Ray;
			Bitangent.Y = (Coord2.X * Face2.Y - Coord2.Y * Face1.Y) * Ray;
			Bitangent.Z = (Coord2.X * Face2.Z - Coord2.Y * Face1.Z) * Ray;
			Bitangent = Bitangent.sNormalize();

			Normal.X = (Tangent.Y * Bitangent.Z) - (Tangent.Z * Bitangent.Y);
			Normal.Y = (Tangent.Z * Bitangent.X) - (Tangent.X * Bitangent.Z);
			Normal.Z = (Tangent.X * Bitangent.Y) - (Tangent.Y * Bitangent.X);
			Normal = -Normal.sNormalize();
		}
		void Common::ComputeInfluenceTangentBitangent(const SkinVertex& V1, const SkinVertex& V2, const SkinVertex& V3, Vector3& Tangent, Vector3& Bitangent)
		{
			Vector3 Face1(V2.PositionX - V1.PositionX, V2.PositionY - V1.PositionY, V2.PositionZ - V1.PositionZ);
			Vector3 Face2(V3.PositionX - V1.PositionX, V3.PositionY - V1.PositionY, V3.PositionZ - V1.PositionZ);
			Vector2 Coord1(V2.TexCoordX - V1.TexCoordX, V3.TexCoordX - V1.TexCoordX);
			Vector2 Coord2(V2.TexCoordY - V1.TexCoordY, V3.TexCoordY - V1.TexCoordY);
			float Ray = 1.0f / (Coord1.X * Coord2.Y - Coord1.Y * Coord2.X);

			Tangent.X = (Coord1.Y * Face1.X - Coord1.X * Face2.X) * Ray;
			Tangent.Y = (Coord1.Y * Face1.Y - Coord1.X * Face2.Y) * Ray;
			Tangent.Z = (Coord1.Y * Face1.Z - Coord1.X * Face2.Z) * Ray;
			Tangent = Tangent.sNormalize();

			Bitangent.X = (Coord2.X * Face2.X - Coord2.Y * Face1.X) * Ray;
			Bitangent.Y = (Coord2.X * Face2.Y - Coord2.Y * Face1.Y) * Ray;
			Bitangent.Z = (Coord2.X * Face2.Z - Coord2.Y * Face1.Z) * Ray;
			Bitangent = Bitangent.sNormalize();
		}
		void Common::Sha1CollapseBufferBlock(unsigned int* Buffer)
		{
			TH_ASSERT_V(Buffer != nullptr, "buffer should be set");
			for (int i = 16; --i >= 0;)
				Buffer[i] = 0;
		}
		void Common::Sha1ComputeHashBlock(unsigned int* Result, unsigned int* W)
		{
			TH_ASSERT_V(Result != nullptr, "result should be set");
			TH_ASSERT_V(W != nullptr, "salt should be set");
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
		void Common::Sha1Compute(const void* Value, const int Length, unsigned char* Hash20)
		{
			TH_ASSERT_V(Value != nullptr, "value should be set");
			TH_ASSERT_V(Hash20 != nullptr, "hash of size 20 should be set");

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
		void Common::Sha1Hash20ToHex(const unsigned char* Hash20, char* HexString)
		{
			TH_ASSERT_V(Hash20 != nullptr, "hash of size 20 should be set");
			TH_ASSERT_V(HexString != nullptr, "result hex should be set");

			const char Hex[] = { "0123456789abcdef" };
			for (int i = 20; --i >= 0;)
			{
				HexString[i << 1] = Hex[(Hash20[i] >> 4) & 0xf];
				HexString[(i << 1) + 1] = Hex[Hash20[i] & 0xf];
			}

			HexString[40] = 0;
		}
		void Common::MatrixRhToLh(Compute::Matrix4x4* Matrix)
		{
			TH_ASSERT_V(Matrix != nullptr, "matrix should be set");
			*Matrix = *Matrix * RH_TO_LH;
		}
		void Common::VertexRhToLh(std::vector<Vertex>& Vertices, std::vector<int>& Indices)
		{
			if (LeftHanded)
			{
				Matrix4x4 RhToLh = RH_TO_LH;
				for (auto& Item : Vertices)
				{
					Compute::Vector3 Position(Item.PositionX, Item.PositionY, Item.PositionZ);
					Position = (RhToLh * Compute::Matrix4x4::CreateTranslation(Position)).Position();
					Item.PositionX = Position.X;
					Item.PositionY = Position.Y;
					Item.PositionZ = Position.Z;
				}
			}
			else
			{
				for (auto& Item : Vertices)
				{
					Item.PositionY = -Item.PositionY;
					Item.NormalY = -Item.NormalY;
					Item.TexCoordY = 1.0f - Item.TexCoordY;
				}
			}

			FlipIndexWindingOrder(Indices);
		}
		void Common::VertexRhToLh(std::vector<SkinVertex>& Vertices, std::vector<int>& Indices)
		{
			if (!LeftHanded)
				return TexCoordRhToLh(Vertices);

			Matrix4x4 RhToLh = RH_TO_LH;
			for (auto& Item : Vertices)
			{
				Compute::Vector3 Position(Item.PositionX, Item.PositionY, Item.PositionZ);
				Position = (RhToLh * Compute::Matrix4x4::CreateTranslation(Position)).Position();
				Item.PositionX = Position.X;
				Item.PositionY = Position.Y;
				Item.PositionZ = Position.Z;
			}

			FlipIndexWindingOrder(Indices);
		}
		void Common::TexCoordRhToLh(std::vector<Vertex>& Vertices)
		{
			if (LeftHanded)
				return;

			for (auto& Item : Vertices)
				Item.TexCoordY = 1.0f - Item.TexCoordY;
		}
		void Common::TexCoordRhToLh(std::vector<SkinVertex>& Vertices)
		{
			if (LeftHanded)
				return;

			for (auto& Item : Vertices)
				Item.TexCoordY = 1.0f - Item.TexCoordY;
		}
		void Common::TexCoordRhToLh(std::vector<ShapeVertex>& Vertices)
		{
			if (LeftHanded)
				return;

			for (auto& Item : Vertices)
				Item.TexCoordY = 1.0f - Item.TexCoordY;
		}
		void Common::SetLeftHanded(bool IsLeftHanded)
		{
			LeftHanded = IsLeftHanded;
		}
		unsigned char Common::RandomUC()
		{
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
			return Alphabet[(size_t)(Random() % (sizeof(Alphabet) - 1))];
		}
		uint64_t Common::Utf8(int code, char* Buffer)
		{
			TH_ASSERT(Buffer != nullptr, 0, "buffer should be set");
			if (code < 0x0080)
			{
				Buffer[0] = (code & 0x7F);
				return 1;
			}
			else if (code < 0x0800)
			{
				Buffer[0] = (0xC0 | ((code >> 6) & 0x1F));
				Buffer[1] = (0x80 | (code & 0x3F));
				return 2;
			}
			else if (code < 0xD800)
			{
				Buffer[0] = (0xE0 | ((code >> 12) & 0xF));
				Buffer[1] = (0x80 | ((code >> 6) & 0x3F));
				Buffer[2] = (0x80 | (code & 0x3F));
				return 3;
			}
			else if (code < 0xE000)
			{
				return 0;
			}
			else if (code < 0x10000)
			{
				Buffer[0] = (0xE0 | ((code >> 12) & 0xF));
				Buffer[1] = (0x80 | ((code >> 6) & 0x3F));
				Buffer[2] = (0x80 | (code & 0x3F));
				return 3;
			}
			else if (code < 0x110000)
			{
				Buffer[0] = (0xF0 | ((code >> 18) & 0x7));
				Buffer[1] = (0x80 | ((code >> 12) & 0x3F));
				Buffer[2] = (0x80 | ((code >> 6) & 0x3F));
				Buffer[3] = (0x80 | (code & 0x3F));
				return 4;
			}

			return 0;
		}
		std::string Common::JWTSign(const std::string& Alg, const std::string& Payload, const char* Key)
		{
			Digest Hash = nullptr;
			if (Alg == "HS256")
				Hash = Digests::SHA256();
			else if (Alg == "HS384")
				Hash = Digests::SHA384();
			else if (Alg == "HS512")
				Hash = Digests::SHA512();

			return Common::HMAC(Hash, Payload, Key);
		}
		std::string Common::JWTEncode(WebToken* Src, const char* Key)
		{
			TH_ASSERT(Src != nullptr, std::string(), "web token should be set");
			TH_ASSERT(Src->Header != nullptr, std::string(), "web token header should be set");
			TH_ASSERT(Src->Payload != nullptr, std::string(), "web token payload should be set");
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");

			std::string Alg = Src->Header->GetVar("alg").GetBlob();
			if (Alg.empty())
				return "";

			std::string Header;
			Core::Schema::WriteJSON(Src->Header, [&Header](Core::VarForm, const char* Buffer, int64_t Size)
			{
				Header.append(Buffer, Size);
			});

			std::string Payload;
			Core::Schema::WriteJSON(Src->Payload, [&Payload](Core::VarForm, const char* Buffer, int64_t Size)
			{
				Payload.append(Buffer, Size);
			});

			std::string Data = Base64URLEncode(Header) + '.' + Base64URLEncode(Payload);
			Src->Signature = JWTSign(Alg, Data, Key);

			return Data + '.' + Base64URLEncode(Src->Signature);
		}
		WebToken* Common::JWTDecode(const std::string& Value, const char* Key)
		{
			TH_ASSERT(Key != nullptr && !Value.empty(), nullptr, "key should be set and value should not be empty");
			std::vector<std::string> Source = Core::Parser(&Value).Split('.');
			if (Source.size() != 3)
				return nullptr;

			size_t Offset = Source[0].size() + Source[1].size() + 1;
			Source[0] = Base64URLDecode(Source[0]);
			Core::Schema* Header = Core::Schema::ReadJSON(Source[0].c_str(), Source[0].size());

			if (!Header)
				return nullptr;

			Source[1] = Base64URLDecode(Source[1]);
			Core::Schema* Payload = Core::Schema::ReadJSON(Source[1].c_str(), Source[1].size());

			if (!Payload)
			{
				TH_RELEASE(Header);
				return nullptr;
			}

			Source[0] = Header->GetVar("alg").GetBlob();
			if (Base64URLEncode(JWTSign(Source[0], Value.substr(0, Offset), Key)) != Source[2])
			{
				TH_RELEASE(Header);
				return nullptr;
			}

			WebToken* Result = new WebToken();
			Result->Signature = Base64URLDecode(Source[2]);
			Result->Header = Header;
			Result->Payload = Payload;

			return Result;
		}
		std::string Common::DocEncrypt(Core::Schema* Src, const char* Key, const char* Salt)
		{
			TH_ASSERT(Src != nullptr, std::string(), "schema should be set");
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");
			TH_ASSERT(Salt != nullptr, std::string(), "salt should be set");

			std::string Result;
			Core::Schema::WriteJSON(Src, [&Result](Core::VarForm, const char* Buffer, int64_t Size)
			{
				Result.append(Buffer, Size);
			});

			Result = Bep45Encode(Encrypt(Ciphers::AES_256_CBC(), Result, Key, Salt));
			return Result;
		}
		Core::Schema* Common::DocDecrypt(const std::string& Value, const char* Key, const char* Salt)
		{
			TH_ASSERT(!Value.empty(), nullptr, "value should not be empty");
			TH_ASSERT(Key != nullptr, nullptr, "key should be set");
			TH_ASSERT(Salt != nullptr, nullptr, "salt should be set");

			std::string Source = Decrypt(Ciphers::AES_256_CBC(), Bep45Decode(Value), Key, Salt);
			return Core::Schema::ReadJSON(Source.c_str(), Source.size());
		}
		std::string Common::Base10ToBaseN(uint64_t Value, unsigned int BaseLessThan65)
		{
			TH_ASSERT(BaseLessThan65 >= 1 && BaseLessThan65 <= 64, std::string(), "base should be between 1 and 64");
			static const char* Base62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
			static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			if (Value == 0)
				return "0";

			const char* Base = (BaseLessThan65 == 64 ? Base64 : Base62);
			std::string Output;

			while (Value > 0)
			{
				Output.insert(0, 1, Base[Value % BaseLessThan65]);
				Value /= BaseLessThan65;
			}

			return Output;
		}
		std::string Common::DecimalToHex(uint64_t n)
		{
			const char* Set = "0123456789abcdef";
			std::string Result;

			do
			{
				Result = Set[n & 15] + Result;
				n >>= 4;
			} while (n > 0);

			return Result;
		}
		std::string Common::RandomBytes(uint64_t Length)
		{
#ifdef TH_HAS_OPENSSL
			unsigned char* Buffer = (unsigned char*)TH_MALLOC(sizeof(unsigned char) * Length);
			RAND_bytes(Buffer, (int)Length);

			std::string Output((const char*)Buffer, Length);
			TH_FREE(Buffer);

			return Output;
#else
			return "";
#endif
		}
		std::string Common::Move(const std::string& Text, int Offset)
		{
			std::string Result;
			for (uint64_t i = 0; i < (uint64_t)Text.size(); i++)
			{
				if (Text[i] != 0)
					Result += char(Text[i] + Offset);
				else
					Result += " ";
			}

			return Result;
		}
		std::string Common::MD5Hash(const std::string& Value)
		{
			MD5Hasher Hasher;
			Hasher.Update(Value);
			Hasher.Finalize();

			return Hasher.ToHex();
		}
		std::string Common::MD5HashBinary(const std::string& Value)
		{
			MD5Hasher Hasher;
			Hasher.Update(Value);
			Hasher.Finalize();

			return Hasher.ToRaw();
		}
		std::string Common::SHA256Hash(const std::string& Value)
		{
			return Common::HexEncode(Common::SHA256HashBinary(Value));
		}
		std::string Common::SHA256HashBinary(const std::string& Value)
		{
#ifdef TH_HAS_OPENSSL
			unsigned char Hash[SHA256_DIGEST_LENGTH];

			SHA256_CTX Context;
			SHA256_Init(&Context);
			SHA256_Update(&Context, Value.c_str(), Value.size());
			SHA256_Final(Hash, &Context);

			std::string Result(SHA256_DIGEST_LENGTH, ' ');
			for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
				Result[i] = Hash[i];

			return Result;
#else
			return Value;
#endif
		}
		std::string Common::Sign(Digest Type, const unsigned char* Value, uint64_t Length, const char* Key)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");
			TH_ASSERT(Type != nullptr, std::string(), "type should be set");
			TH_ASSERT(Length > 0, std::string(), "length should be greater than zero");
#ifdef TH_HAS_OPENSSL
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
			HMAC_CTX* Context = HMAC_CTX_new();
			if (!Context)
				return "";

			unsigned char Result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(Context, Key, (int)strlen(Key), (const EVP_MD*)Type, nullptr))
			{
				HMAC_CTX_free(Context);
				return "";
			}

			if (1 != HMAC_Update(Context, Value, (int)Length))
			{
				HMAC_CTX_free(Context);
				return "";
			}

			unsigned int Size = sizeof(Result);
			if (1 != HMAC_Final(Context, Result, &Size))
			{
				HMAC_CTX_free(Context);
				return "";
			}

			std::string Output((const char*)Result, Size);
			HMAC_CTX_free(Context);

			return Output;
#else
			HMAC_CTX Context;
			HMAC_CTX_init(&Context);

			unsigned char Result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(&Context, Key, (int)strlen(Key), (const EVP_MD*)Type, nullptr))
			{
				HMAC_CTX_cleanup(&Context);
				return "";
			}

			if (1 != HMAC_Update(&Context, Value, (int)Length))
			{
				HMAC_CTX_cleanup(&Context);
				return "";
			}

			unsigned int Size = sizeof(Result);
			if (1 != HMAC_Final(&Context, Result, &Size))
			{
				HMAC_CTX_cleanup(&Context);
				return "";
			}

			std::string Output((const char*)Result, Size);
			HMAC_CTX_cleanup(&Context);

			return Output;
#endif
#else
			return (const char*)Value;
#endif
		}
		std::string Common::Sign(Digest Type, const std::string& Value, const char* Key)
		{
			return Sign(Type, (const unsigned char*)Value.c_str(), (uint64_t)Value.size(), Key);
		}
		std::string Common::HMAC(Digest Type, const unsigned char* Value, uint64_t Length, const char* Key)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");
			TH_ASSERT(Type != nullptr, std::string(), "type should be set");
			TH_ASSERT(Length > 0, std::string(), "length should be greater than zero");
#ifdef TH_HAS_OPENSSL
			unsigned char Result[EVP_MAX_MD_SIZE];
			unsigned int Size = sizeof(Result);

			if (!::HMAC((const EVP_MD*)Type, Key, (int)strlen(Key), Value, (size_t)Length, Result, &Size))
				return "";

			std::string Output((const char*)Result, Size);
			return Output;
#else
			return (const char*)Value;
#endif
		}
		std::string Common::HMAC(Digest Type, const std::string& Value, const char* Key)
		{
			return Common::HMAC(Type, (const unsigned char*)Value.c_str(), (uint64_t)Value.size(), Key);
		}
		std::string Common::Encrypt(Cipher Type, const unsigned char* Value, uint64_t Length, const char* Key, const char* Salt)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");
			TH_ASSERT(Type != nullptr, std::string(), "type should be set");

			if (!Length)
				return "";
#ifdef TH_HAS_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
				return "";

			if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const unsigned char*)Key, (const unsigned char*)Salt))
			{
				EVP_CIPHER_CTX_free(Context);
				return "";
			}

			int Size1 = (int)Length, Size2 = 0;
			unsigned char* Buffer = (unsigned char*)TH_MALLOC(sizeof(unsigned char) * (Size1 + 2048));

			if (1 != EVP_EncryptUpdate(Context, Buffer, &Size2, Value, Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				TH_FREE(Buffer);
				return "";
			}

			if (1 != EVP_EncryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				TH_FREE(Buffer);
				return "";
			}

			std::string Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			TH_FREE(Buffer);

			return Output;
#else
			return (const char*)Value;
#endif
		}
		std::string Common::Encrypt(Cipher Type, const std::string& Value, const char* Key, const char* Salt)
		{
			return Encrypt(Type, (const unsigned char*)Value.c_str(), (uint64_t)Value.size(), Key, Salt);
		}
		std::string Common::Decrypt(Cipher Type, const unsigned char* Value, uint64_t Length, const char* Key, const char* Salt)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");
			TH_ASSERT(Type != nullptr, std::string(), "type should be set");

			if (!Length)
				return "";
#ifdef TH_HAS_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
				return "";

			if (1 != EVP_DecryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const unsigned char*)Key, (const unsigned char*)Salt))
			{
				EVP_CIPHER_CTX_free(Context);
				return "";
			}

			int Size1 = (int)Length, Size2 = 0;
			unsigned char* Buffer = (unsigned char*)TH_MALLOC(sizeof(unsigned char) * (Size1 + 2048));

			if (1 != EVP_DecryptUpdate(Context, Buffer, &Size2, Value, Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				TH_FREE(Buffer);
				return "";
			}

			if (1 != EVP_DecryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				TH_FREE(Buffer);
				return "";
			}

			std::string Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			TH_FREE(Buffer);

			return Output;
#else
			return (const char*)Value;
#endif
		}
		std::string Common::Decrypt(Cipher Type, const std::string& Value, const char* Key, const char* Salt)
		{
			return Decrypt(Type, (const unsigned char*)Value.c_str(), (uint64_t)Value.size(), Key, Salt);
		}
		std::string Common::Encode64(const char Alphabet[65], const unsigned char* Value, uint64_t Length, bool Padding)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Length > 0, std::string(), "length should be greater than zero");

			std::string Result;
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
		std::string Common::Decode64(const char Alphabet[65], const unsigned char* Value, uint64_t Length, bool(*IsAlphabetic)(unsigned char))
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(IsAlphabetic != nullptr, std::string(), "callback should be set");
			TH_ASSERT(Length > 0, std::string(), "length should be greater than zero");

			std::string Result;
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
		std::string Common::Bep45Encode(const std::string& Data)
		{
			static const char From[] = " $%*+-./:";
			static const char To[] = "abcdefghi";

			Core::Parser Result(Base45Encode(Data));
			for (size_t i = 0; i < sizeof(From) - 1; i++)
				Result.Replace(From[i], To[i]);

			return Result.R();
		}
		std::string Common::Bep45Decode(const std::string& Data)
		{
			static const char From[] = "abcdefghi";
			static const char To[] = " $%*+-./:";

			Core::Parser Result(Data);
			for (size_t i = 0; i < sizeof(From) - 1; i++)
				Result.Replace(From[i], To[i]);

			return Base45Decode(Result.R());
		}
		std::string Common::Base45Encode(const std::string& Data)
		{
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
			std::string Result;
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
		std::string Common::Base45Decode(const std::string& Data)
		{
			static unsigned char CharToInt[256] =
			{
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				36, 255,255,255,  37, 38,255,255, 255,255, 39, 40, 255, 41, 42, 43,
				0,   1,  2,  3,   4,  5,  6,  7,   8,  9, 44,255, 255,255,255,255,
				255, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,
				25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 35, 255,255,255,255,
				255, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,
				25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 35, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
				255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255
			};

			size_t Size = Data.size();
			std::string Result(Size, ' ');
			size_t Offset = 0;

			for (size_t i = 0; i < Size; i += 3)
			{
				int x, a, b;
				if (Size - i < 2)
					break;

				if ((255 == (a = (char)CharToInt[Data[i]])) || (255 == (b = (char)CharToInt[Data[i + 1]])))
					break;

				x = a + 45 * b;
				if (Size - i >= 3)
				{
					if (255 == (a = (char)CharToInt[Data[i + 2]]))
						break;

					x += a * 45 * 45;
					Result[Offset++] = x / 256;
					x %= 256;
				}

				Result[Offset++] = x;
			}

			return std::string(Result.c_str(), Offset);
		}
		std::string Common::Base64Encode(const unsigned char* Value, uint64_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return Encode64(Set, Value, Length, true);
		}
		std::string Common::Base64Encode(const std::string& Value)
		{
			return Base64Encode((const unsigned char*)Value.c_str(), (uint64_t)Value.size());
		}
		std::string Common::Base64Decode(const unsigned char* Value, uint64_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return Decode64(Set, Value, Length, IsBase64);
		}
		std::string Common::Base64Decode(const std::string& Value)
		{
			return Base64Decode((const unsigned char*)Value.c_str(), (uint64_t)Value.size());
		}
		std::string Common::Base64URLEncode(const unsigned char* Value, uint64_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			return Encode64(Set, Value, Length, false);
		}
		std::string Common::Base64URLEncode(const std::string& Value)
		{
			return Base64URLEncode((const unsigned char*)Value.c_str(), (uint64_t)Value.size());
		}
		std::string Common::Base64URLDecode(const unsigned char* Value, uint64_t Length)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			int64_t Padding = (int64_t)Length % 4;
			if (Padding == 0)
				return Decode64(Set, Value, Length, IsBase64URL);

			std::string Padded((const char*)Value, (size_t)Length);
			Padded.append((size_t)(4 - Padding), '=');
			return Decode64(Set, (const unsigned char*)Padded.c_str(), (uint64_t)Padded.size(), IsBase64URL);
		}
		std::string Common::Base64URLDecode(const std::string& Value)
		{
			return Base64URLDecode((const unsigned char*)Value.c_str(), (uint64_t)Value.size());
		}
		std::string Common::Shuffle(const char* Value, size_t Size, uint64_t Mask)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");

			std::string Result;
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
		std::string Common::HexEncode(const char* Value, size_t Size)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Size > 0, std::string(), "length should be greater than zero");
			static const char Hex[17] = "0123456789abcdef";

			std::string Output;
			Output.reserve(Size * 2);

			for (size_t i = 0; i < Size; i++)
			{
				unsigned char C = static_cast<unsigned char>(Value[i]);
				Output += Hex[C >> 4];
				Output += Hex[C & 0xf];
			}

			return Output;
		}
		std::string Common::HexEncode(const std::string& Value)
		{
			return HexEncode(Value.c_str(), Value.size());
		}
		std::string Common::HexDecode(const char* Value, size_t Size)
		{
			TH_ASSERT(Value != nullptr, std::string(), "value should be set");
			TH_ASSERT(Size > 0, std::string(), "length should be greater than zero");

			std::string Output;
			Output.reserve(Size / 2);

			char Hex[3] = { 0, 0, 0 };
			for (size_t i = 0; i < Size; i += 2)
			{
				memcpy(Hex, Value + i, sizeof(char) * 2);
				Output.push_back((char)(int)strtol(Hex, nullptr, 16));
			}

			return Output;
		}
		std::string Common::HexDecode(const std::string& Value)
		{
			return HexDecode(Value.c_str(), Value.size());
		}
		std::string Common::URIEncode(const std::string& Text)
		{
			return URIEncode(Text.c_str(), Text.size());
		}
		std::string Common::URIEncode(const char* Text, uint64_t Length)
		{
			TH_ASSERT(Text != nullptr, std::string(), "text should be set");
			TH_ASSERT(Length > 0, std::string(), "length should be greater than zero");

			static const char* Unescape = "._-$,;~()";
			static const char* Hex = "0123456789abcdef";

			std::string Value;
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
		std::string Common::URIDecode(const std::string& Text)
		{
			return URIDecode(Text.c_str(), Text.size());
		}
		std::string Common::URIDecode(const char* Text, uint64_t Length)
		{
			TH_ASSERT(Text != nullptr, std::string(), "text should be set");
			TH_ASSERT(Length > 0, std::string(), "length should be greater than zero");
			std::string Value;
			int i = 0;

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
		std::string Common::Hybi10Encode(const Hybi10Request& Request, bool Masked)
		{
			std::string Stream;
			size_t Length = (size_t)Request.Payload.size();
			unsigned char Head = 0;

			switch (Request.GetEnumType())
			{
				case Hybi10_Opcode::Text:
					Head = 129;
					break;
				case Hybi10_Opcode::Close:
					Head = 136;
					break;
				case Hybi10_Opcode::Ping:
					Head = 137;
					break;
				case Hybi10_Opcode::Pong:
					Head = 138;
					break;
				default:
					break;
			}

			Stream += (char)Head;
			if (Length > 65535)
			{
				unsigned long long Offset = (unsigned long long)Length;
				Stream += (char)(unsigned char)(Masked ? 255 : 127);

				for (unsigned int i = 7; i > 0; i--)
				{
					unsigned char Block = 0;
					for (unsigned int j = 0; j < 8; j++)
					{
						unsigned long long Shift = 0x01ull << j;
						Shift = Shift << (8 * i);
						Block += (unsigned char)((unsigned long long)(1ull << j) * (Offset & Shift));
					}

					if (i == 7 && Block > 127)
						return "";

					Stream += (char)Block;
				}
			}
			else if (Length > 125)
			{
				unsigned long Offset = (unsigned long)Length;
				unsigned char Block2 = (unsigned char)(128 * (Offset & 0x8000) + 64 * (Offset & 0x4000) + 32 * (Offset & 0x2000) + 16 * (Offset & 0x1000) + 8 * (Offset & 0x800) + 4 * (Offset & 0x400) + 2 * (Offset & 0x200) + (Offset & 0x100));
				unsigned char Block3 = (unsigned char)(128 * (Offset & 0x80) + 64 * (Offset & 0x40) + 32 * (Offset & 0x20) + 16 * (Offset & 0x10) + 8 * (Offset & 0x08) + 4 * (Offset & 0x04) + 2 * (Offset & 0x02) + (Offset & 0x01));

				Stream += (char)(unsigned char)(Masked ? 254 : 126);
				Stream += Block2;
				Stream += Block3;
			}
			else
				Stream += (unsigned char)(Masked ? Length + 128 : Length);

			if (!Masked)
			{
				Stream += Request.Payload;
				return Stream;
			}

			unsigned char Mask[4];
			for (int i = 0; i < 4; i++)
			{
				Mask[i] = RandomUC();
				Stream += (char)Mask[i];
			}

			for (int i = 0; i < (int)Length; i++)
				Stream += (char)(unsigned char)(Request.Payload[i] ^ Mask[i % 4]);

			return Stream;
		}
		Hybi10Request Common::Hybi10Decode(const std::string& Value)
		{
			Hybi10PayloadHeader* Payload = (Hybi10PayloadHeader*)Value.substr(0, 2).c_str();
			Hybi10Request Decoded;
			Decoded.Type = Payload->Opcode;

			if (Decoded.GetEnumType() == Hybi10_Opcode::Invalid)
			{
				Decoded.ExitCode = 1003;
				return Decoded;
			}

			std::string PayloadBody = Value.substr(2);
			std::string PayloadMask;
			int PayloadOffset = 0;

			if (Payload->PayloadLength == 126)
				PayloadOffset += 2;
			else if (Payload->PayloadLength == 127)
				PayloadOffset += 8;

			if (Value[1] & 0x80)
			{
				PayloadMask = PayloadBody.substr(PayloadOffset, 4);
				PayloadOffset += 4;
			}

			PayloadBody = PayloadBody.substr(PayloadOffset);
			int Length = (int)PayloadBody.size();

			for (int i = 0; i < Length; i++)
				Decoded.Payload += (char)(unsigned char)(PayloadBody[i] ^ PayloadMask[i % 4]);

			return Decoded;
		}
		std::vector<int> Common::CreateTriangleStrip(TriangleStrip::Desc& Desc, const std::vector<int>& Indices)
		{
			std::vector<unsigned int> Src;
			Src.reserve(Indices.size());

			for (auto& Index : Indices)
				Src.push_back((unsigned int)Index);

			Desc.Faces = Src.data();
			Desc.NbFaces = Src.size() / 3;

			TriangleStrip Strip;
			if (!Strip.Fill(Desc))
			{
				std::vector<int> Empty;
				return Empty;
			}

			TriangleStrip::Result Result;
			if (!Strip.Resolve(Result))
			{
				std::vector<int> Empty;
				return Empty;
			}

			return Result.GetIndices();
		}
		std::vector<int> Common::CreateTriangleList(const std::vector<int>& Indices)
		{
			size_t Size = Indices.size() - 2;
			std::vector<int> Result;
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
		void Common::CreateFrustumRad(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ)
		{
			TH_ASSERT_V(Result8 != nullptr, "8 sized array should be set");
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
		void Common::CreateFrustum(Vector4* Result8, float FieldOfView, float Aspect, float NearZ, float FarZ)
		{
			return CreateFrustumRad(Result8, Mathf::Deg2Rad() * FieldOfView, Aspect, NearZ, FarZ);
		}
		Ray Common::CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView)
		{
			Vector2 Tmp = Cursor * 2.0f;
			Tmp /= Screen;

			Vector4 Eye = Vector4(Tmp.X - 1.0f, 1.0f - Tmp.Y, 1.0f, 1.0f) * InvProjection;
			Eye = (Vector4(Eye.X, Eye.Y, 1.0f, 0.0f) * InvView).sNormalize();
			return Ray(Origin, Vector3(Eye.X, Eye.Y, Eye.Z));
		}
		bool Common::CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale, Vector3* Hit)
		{
			return Cursor.IntersectsAABB(Position, Scale, Hit);
		}
		bool Common::CursorRayTest(const Ray& Cursor, const Matrix4x4& World, Vector3* Hit)
		{
			return Cursor.IntersectsOBB(World, Hit);
		}
		uint64_t Common::CRC32(const std::string& Data)
		{
			int64_t Result = 0xFFFFFFFF;
			int64_t Index = 0;
			int64_t Byte = 0;
			int64_t Mask = 0;
			int64_t It = 0;

			while (Data[Index] != 0)
			{
				Byte = Data[Index];
				Result = Result ^ Byte;

				for (It = 7; It >= 0; It--)
				{
					Mask = -(Result & 1);
					Result = (Result >> 1) ^ (0xEDB88320 & Mask);
				}

				Index++;
			}

			return (uint64_t)~Result;
		}
		float Common::FastInvSqrt(float Value)
		{
			float F = Value;
			long I = *(long*)&F;
			I = 0x5f3759df - (I >> 1);
			F = *(float*)&I;
			F = F * (1.5f - ((Value * 0.5f) * F * F));

			return F;
		}
		float Common::FastSqrt(float Value)
		{
			return 1.0f / FastInvSqrt(Value);
		}
		uint64_t Common::Random(uint64_t Min, uint64_t Max)
		{
			uint64_t Raw = 0;
			if (Min > Max)
				return Raw;
#ifdef TH_HAS_OPENSSL
			RAND_bytes((unsigned char*)&Raw, sizeof(uint64_t));
#else
			Raw = Random();
#endif
			return Raw % (Max - Min + 1) + Min;
		}
		uint64_t Common::Random()
		{
			static std::random_device Device;
			static std::mt19937 Engine(Device());
			static std::uniform_int_distribution<uint64_t> Range;

			return Range(Engine);
		}
		bool Common::LeftHanded = true;

		WebToken::WebToken() : Header(nullptr), Payload(nullptr), Token(nullptr)
		{
		}
		WebToken::WebToken(const std::string& Issuer, const std::string& Subject, int64_t Expiration) : Header(Core::Var::Set::Object()), Payload(Core::Var::Set::Object()), Token(nullptr)
		{
			Header->Set("alg", Core::Var::String("HS256"));
			Header->Set("typ", Core::Var::String("JWT"));
			Payload->Set("iss", Core::Var::String(Issuer));
			Payload->Set("sub", Core::Var::String(Subject));
			Payload->Set("exp", Core::Var::Integer(Expiration));
		}
		WebToken::~WebToken()
		{
			TH_RELEASE(Header);
			TH_RELEASE(Payload);
			TH_RELEASE(Token);
		}
		void WebToken::Unsign()
		{
			Signature.clear();
			Data.clear();
		}
		void WebToken::SetAlgorithm(const std::string& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("alg", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetType(const std::string& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("typ", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetContentType(const std::string& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("cty", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetIssuer(const std::string& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("iss", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetSubject(const std::string& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("sub", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetId(const std::string& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("jti", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetAudience(const std::vector<std::string>& Value)
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
		void WebToken::SetRefreshToken(const std::string& Value, const char* Key, const char* Salt)
		{
			TH_RELEASE(Token);
			Token = Common::DocDecrypt(Value, Key, Salt);
			Refresher.assign(Value);
		}
		bool WebToken::Sign(const char* Key)
		{
			TH_ASSERT(Header != nullptr, false, "header should be set");
			TH_ASSERT(Payload != nullptr, false, "payload should be set");
			TH_ASSERT(Key != nullptr, false, "key should be set");

			if (Data.empty())
				Data = Common::JWTEncode(this, Key);

			return !Data.empty();
		}
		std::string WebToken::GetRefreshToken(const char* Key, const char* Salt)
		{
			TH_ASSERT(Key != nullptr, std::string(), "key should be set");
			TH_ASSERT(Salt != nullptr, std::string(), "salt should be set");

			Refresher = Common::DocEncrypt(Token, Key, Salt);
			return Refresher;
		}
		bool WebToken::IsValid() const
		{
			if (!Header || !Payload || Signature.empty())
				return false;

			int64_t Expires = Payload->GetVar("exp").GetInteger();
			return time(nullptr) < Expires;
		}

		Preprocessor::Preprocessor() : Nested(false)
		{
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
		void Preprocessor::SetFeatures(const Desc& F)
		{
			Features = F;
		}
		void Preprocessor::Define(const std::string& Name)
		{
			Undefine(Name);
			Defines.push_back(Name);
		}
		void Preprocessor::Undefine(const std::string& Name)
		{
			for (auto It = Defines.begin(); It != Defines.end(); ++It)
			{
				if (*It == Name)
				{
					Defines.erase(It);
					return;
				}
			}
		}
		void Preprocessor::Clear()
		{
			Defines.clear();
			Sets.clear();
			ExpandedPath.clear();
		}
		bool Preprocessor::IsDefined(const char* Name) const
		{
			TH_ASSERT(Name != nullptr, false, "name should be set");
			for (auto& It : Defines)
			{
				if (It == Name)
					return true;
			}

			return false;
		}
		bool Preprocessor::Process(const std::string& Path, std::string& Data)
		{
			bool Nesting = SaveResult();
			if (Data.empty())
				return ReturnResult(false, Nesting);

			if (!Path.empty() && HasSet(Path))
				return ReturnResult(true, Nesting);

			Core::Parser Buffer(&Data);
			if (Features.Conditions && !ProcessBlockDirective(Buffer))
				return ReturnResult(false, Nesting);

			ExpandedPath = Path;
			if (Features.Includes)
			{
				if (!Path.empty())
					Sets.push_back(Path);

				if (!ProcessIncludeDirective(Path, Buffer))
					return ReturnResult(false, Nesting);
			}

			if (Features.Pragmas && !ProcessPragmaDirective(Path, Buffer))
				return ReturnResult(false, Nesting);

			uint64_t Offset;
			if (Features.Defines && !ProcessDefineDirective(Buffer, 0, Offset, true))
				return ReturnResult(false, Nesting);

			Buffer.Trim();
			Buffer.Resize(strlen(Buffer.Get()));
			return ReturnResult(true, Nesting);
		}
		bool Preprocessor::SaveResult()
		{
			bool Nesting = Nested;
			Nested = true;

			return Nesting;
		}
		bool Preprocessor::ReturnResult(bool Result, bool WasNested)
		{
			Nested = WasNested;
			if (!Nested)
			{
				Sets.clear();
				ExpandedPath.clear();
			}

			return Result;
		}
		bool Preprocessor::ProcessIncludeDirective(const std::string& Path, Core::Parser& Buffer)
		{
			if (Buffer.Empty())
				return true;

			Core::Parser::Settle Result;
			while (true)
			{
				Result = Buffer.Find("#include", Result.End);
				if (!Result.Found)
					return true;

				if (Result.Start > 0)
				{
					char P = Buffer.R()[Result.Start - 1];
					if (P != '\n' && P != '\r' && P != '\t' && P != ' ')
						continue;
				}

				uint64_t Start = Result.End;
				while (Start + 1 < Buffer.Size() && Buffer.R()[Start] != '\"')
					Start++;

				uint64_t End = Start + 1;
				while (End + 1 < Buffer.Size() && Buffer.R()[End] != '\"')
					End++;

				Start++;
				if (Start == End)
				{
					TH_ERR("%s: cannot process include directive", Path.c_str());
					return false;
				}

				Core::Parser Section(Buffer.Get() + Start, End - Start);
				Section.Trim();
				End++;

				FileDesc.Path = Section.R();
				FileDesc.From = Path;

				IncludeResult File = ResolveInclude(FileDesc);
				std::string Output;

				if (!HasSet(File.Module))
				{
					if (!Include || !Include(this, File, &Output))
					{
						TH_ERR("%s: cannot find \"%s\"", Path.c_str(), Section.Get());
						return false;
					}

					bool Failed = (!Output.empty() && !Process(File.Module, Output));
					ExpandedPath = Path;

					if (Failed)
						return false;
				}

				Buffer.ReplacePart(Result.Start, End, Output);
				Result.End = Result.Start + 1;
			}
		}
		bool Preprocessor::ProcessPragmaDirective(const std::string& Path, Core::Parser& Buffer)
		{
			if (Buffer.Empty())
				return true;

			std::vector<std::string> Args;
			uint64_t Offset = 0;

			while (true)
			{
				uint64_t Base, Start, End;
				int R = FindDirective(Buffer, "#pragma", &Offset, &Base, &Start, &End);
				if (R < 0)
				{
					TH_ERR("cannot process pragma directive");
					return false;
				}
				else if (R == 0)
					return true;

				Core::Parser Value(Buffer.Get() + Start, End - Start);
				Value.Trim().Replace("  ", " ");
				Args.clear();

				auto fStart = Value.Find('(');
				if (fStart.Found)
				{
					auto fEnd = Value.ReverseFind(')');
					if (fEnd.Found)
					{
						std::string Subvalue;
						bool Quoted = false;
						size_t Index = fStart.End;

						while (Index < fEnd.End)
						{
							char V = Value.R()[Index];
							if (!Quoted && (V == ',' || V == ')'))
							{
								Core::Parser(&Subvalue).Trim().Replace("\\\"", "\"");
								Args.push_back(Subvalue);
								Subvalue.clear();
							}
							else if (V == '"' && (!Index || Value.R()[Index - 1] != '\\'))
								Quoted = !Quoted;
							else
								Subvalue += V;
							Index++;
						}
					}
				}

				Value.Substring(0, fStart.Start);
				if (Pragma && !Pragma(this, Value.R(), Args))
				{
					TH_ERR("cannot process pragma \"%s\" directive", Value.Get());
					return false;
				}

				Buffer.ReplacePart(Base, End, "");
			}
		}
		bool Preprocessor::ProcessBlockDirective(Core::Parser& Buffer)
		{
			if (Buffer.Empty())
				return true;

			uint64_t Offset = 0;
			while (true)
			{
				int R = FindBlockDirective(Buffer, Offset, false);
				if (R < 0)
				{
					TH_ERR("cannot process ifdef/endif directive");
					return false;
				}
				else if (R == 0)
					return true;
			}
		}
		bool Preprocessor::ProcessDefineDirective(Core::Parser& Buffer, uint64_t Base, uint64_t& Offset, bool Endless)
		{
			if (Buffer.Empty())
				return true;

			uint64_t Size = 0;
			while (Endless || Base < Offset)
			{
				int R = FindDefineDirective(Buffer, Base, &Size);
				if (R < 0)
				{
					TH_ERR("cannot process define directive");
					return false;
				}
				else if (R == 0)
					return true;

				Offset -= Size;
			}

			return true;
		}
		int Preprocessor::FindDefineDirective(Core::Parser& Buffer, uint64_t& Offset, uint64_t* Size)
		{
			uint64_t Base, Start, End;
			Offset--;
			int R = FindDirective(Buffer, "#define", &Offset, &Base, &Start, &End);
			if (R < 0)
			{
				TH_ERR("cannot process define directive");
				return -1;
			}
			else if (R == 0)
				return 0;

			Core::Parser Value(Buffer.Get() + Start, End - Start);
			Define(Value.Trim().Replace("  ", " ").R());
			Buffer.ReplacePart(Base, End, "");

			if (Size != nullptr)
				*Size = End - Base;

			return 1;
		}
		int Preprocessor::FindBlockDirective(Core::Parser& Buffer, uint64_t& Offset, bool Nested)
		{
			uint64_t B1Start = 0, B1End = 0;
			uint64_t B2Start = 0, B2End = 0;
			uint64_t Start, End, Base, Size;
			uint64_t BaseOffset = Offset;
			bool Resolved = false, Negate = false;

			int R = FindDirective(Buffer, "#ifdef", &Offset, nullptr, &Start, &End);
			if (R <= 0)
			{
				R = FindDirective(Buffer, "#ifndef", &Offset, nullptr, &Start, &End);
				if (R < 0)
				{
					TH_ERR("cannot parse ifdef block directive");
					return -1;
				}
				else if (R == 0)
					return 0;

				Negate = true;
			}

			Base = Offset;
			ProcessDefineDirective(Buffer, BaseOffset, Base, false);
			Size = Offset - Base;
			Start -= Size;
			End -= Size;
			Offset = Base;

			Core::Parser Name(Buffer.Get() + Start, End - Start);
			Name.Trim().Replace("  ", " ");
			Resolved = IsDefined(Name.Get());
			Start = Offset - 1;

			if (Negate)
				Resolved = !Resolved;

			if (Name.Get()[0] == '!')
			{
				Name.Substring(1);
				Resolved = !Resolved;
			}

		ResolveDirective:
			Core::Parser::Settle Cond = Buffer.Find('#', Offset);
			R = FindBlockNesting(Buffer, Cond, Offset, B1Start + B1End == 0 ? Resolved : !Resolved);
			if (R < 0)
			{
				TH_ERR("cannot find endif directive of %s", Name.Get());
				return -1;
			}
			else if (R == 1)
			{
				int C = FindBlockDirective(Buffer, Offset, true);
				if (C == 0)
				{
					TH_ERR("cannot process nested ifdef/endif directive of %s", Name.Get());
					return -1;
				}
				else if (C < 0)
					return -1;

				goto ResolveDirective;
			}
			else if (R == 2)
			{
				if (B1Start + B1End != 0)
				{
					B2Start = B1End + 6;
					B2End = Cond.Start;
				}
				else
				{
					B1Start = End;
					B1End = Offset - 6;
				}
			}
			else if (R == 3)
			{
				B1Start = End;
				B1End = Offset - 5;
				goto ResolveDirective;
			}
			else if (R == 0)
				goto ResolveDirective;

			if (Resolved)
			{
				Core::Parser Section(Buffer.Get() + B1Start, B1End - B1Start);
				if (B2Start + B2End != 0)
					Buffer.ReplacePart(Start, B2End + 6, Section.R());
				else
					Buffer.ReplacePart(Start, B1End + 6, Section.R());
				Offset = Start + Section.Size() - 1;
			}
			else if (B2Start + B2End != 0)
			{
				Core::Parser Section(Buffer.Get() + B2Start, B2End - B2Start);
				Buffer.ReplacePart(Start, B2End + 6, Section.R());
				Offset = Start + Section.Size() - 1;
			}
			else
			{
				Buffer.ReplacePart(Start, B1End + 6, "");
				Offset = Start;
			}

			return 1;
		}
		int Preprocessor::FindBlockNesting(Core::Parser& Buffer, const Core::Parser::Settle& Hash, uint64_t& Offset, bool Resolved)
		{
			if (!Hash.Found)
				return -1;

			Offset = Hash.End;
			if (!Hash.Start || (Buffer.R()[Hash.Start - 1] != '\n' && Buffer.R()[Hash.Start - 1] != '\r'))
				return 0;

			if (Hash.Start + 5 < Buffer.Size() && strncmp(Buffer.Get() + Hash.Start, "#ifdef", 5) == 0)
			{
				Offset = Hash.Start;
				return 1;
			}

			if (Hash.Start + 5 < Buffer.Size() && strncmp(Buffer.Get() + Hash.Start, "#endif", 5) == 0)
			{
				Offset = Hash.End + 5;
				return 2;
			}

			if (Hash.Start + 4 < Buffer.Size() && strncmp(Buffer.Get() + Hash.Start, "#else", 5) == 0)
			{
				Offset = Hash.End + 4;
				return 3;
			}

			if (!Features.Defines || !Resolved)
				return 0;

			int R = FindDefineDirective(Buffer, Offset, nullptr);
			return R == 1 ? 0 : R;
		}
		int Preprocessor::FindDirective(Core::Parser& Buffer, const char* V, uint64_t* SOffset, uint64_t* SBase, uint64_t* SStart, uint64_t* SEnd)
		{
			auto Result = Buffer.Find(V, SOffset ? *SOffset : 0);
			if (!Result.Found)
				return 0;

			uint64_t Start = Result.End + 1;
			while (Start + 1 < Buffer.Size() && Buffer.R()[Start] == ' ')
				Start++;

			uint64_t End = Start;
			while (End + 1 < Buffer.Size() && (Buffer.R()[End] != '\n' && Buffer.R()[End] != '\r'))
				End++;

			if (Start == End)
				return -1;

			if (SOffset != nullptr)
				*SOffset = Result.Start + 1;

			if (SBase != nullptr)
				*SBase = Result.Start;

			if (SStart != nullptr)
				*SStart = Start;

			if (SEnd != nullptr)
				*SEnd = End + 1;

			return 1;
		}
		bool Preprocessor::HasSet(const std::string& Path)
		{
			for (auto& It : Sets)
			{
				if (It == Path)
					return true;
			}

			return false;
		}
		const std::string& Preprocessor::GetCurrentFilePath()
		{
			return ExpandedPath;
		}
		IncludeResult Preprocessor::ResolveInclude(const IncludeDesc& Desc)
		{
			std::string Base;
			if (!Desc.From.empty())
				Base.assign(Core::OS::Path::GetDirectory(Desc.From.c_str()));
			else
				Base.assign(Core::OS::Directory::Get());

			IncludeResult Result;
			if (!Core::Parser(Desc.Path).StartsOf("/."))
			{
				if (Desc.Root.empty())
				{
					Result.Module = Core::Parser(Desc.Path).Replace('\\', '/').R();
					Result.IsSystem = true;
					return Result;
				}

				Result.Module = Core::OS::Path::Resolve(Desc.Path, Desc.Root);
				if (Core::OS::File::IsExists(Result.Module.c_str()))
				{
					Result.IsSystem = true;
					Result.IsFile = true;
					return Result;
				}

				for (auto It : Desc.Exts)
				{
					std::string File(Result.Module);
					if (Result.Module.empty())
						File.assign(Core::OS::Path::Resolve(Desc.Path + It, Desc.Root));
					else
						File.append(It);

					if (!Core::OS::File::IsExists(File.c_str()))
						continue;

					Result.Module = std::move(File);
					Result.IsSystem = true;
					Result.IsFile = true;
					return Result;
				}

				Result.Module = Core::Parser(Desc.Path).Replace('\\', '/').R();;
				Result.IsSystem = true;
				return Result;
			}

			Result.Module = Core::OS::Path::Resolve(Desc.Path, Base);
			if (Core::OS::File::IsExists(Result.Module.c_str()))
			{
				Result.IsFile = true;
				return Result;
			}

			for (auto It : Desc.Exts)
			{
				std::string File(Result.Module);
				if (Result.Module.empty())
					File.assign(Core::OS::Path::Resolve(Desc.Path + It, Desc.Root));
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

		FiniteState::FiniteState()
		{
		}
		FiniteState::~FiniteState()
		{
			for (auto& Action : Actions)
				TH_DELETE(ActionCallback, Action.second);
		}
		FiniteState* FiniteState::Bind(const std::string& Name, const ActionCallback& Callback)
		{
			TH_ASSERT(Callback, nullptr, "callback should not be empty");
			ActionCallback* Result = TH_NEW(ActionCallback, Callback);
			Mutex.lock();
			auto It = Actions.find(Name);
			if (It != Actions.end())
			{
				TH_DELETE(ActionCallback, It->second);
				It->second = Result;
				Mutex.unlock();

				return this;
			}

			Actions.insert(std::make_pair(Name, Result));
			Mutex.unlock();

			return this;
		}
		FiniteState* FiniteState::Unbind(const std::string& Name)
		{
			Mutex.lock();
			auto It = Actions.find(Name);
			if (It == Actions.end())
			{
				Mutex.unlock();
				return this;
			}

			TH_DELETE(ActionCallback, It->second);
			Actions.erase(It);
			Mutex.unlock();

			return this;
		}
		FiniteState* FiniteState::Push(const std::string& Name)
		{
			Mutex.lock();
			ActionCallback* Callback = Find(Name);
			if (!Callback)
			{
				Mutex.unlock();
				return this;
			}

			State.push(Callback);
			Mutex.unlock();

			return this;
		}
		FiniteState* FiniteState::Replace(const std::string& Name)
		{
			Mutex.lock();
			ActionCallback* Callback = Find(Name);
			if (!Callback)
			{
				Mutex.unlock();
				return this;
			}

			if (!State.empty())
				State.pop();

			State.push(Callback);
			Mutex.unlock();

			return this;
		}
		FiniteState* FiniteState::Pop()
		{
			Mutex.lock();
			State.pop();
			Mutex.unlock();

			return this;
		}
		void FiniteState::Update()
		{
			Mutex.lock();
			ActionCallback* Next = (State.empty() ? nullptr : State.top());
			Mutex.unlock();

			if (Next != nullptr)
				(*Next)(this);
		}
		ActionCallback* FiniteState::Find(const std::string& Name)
		{
			auto It = Actions.find(Name);
			if (It != Actions.end())
				return It->second;

			return nullptr;
		}

		Transform::Transform() : Local(nullptr), Root(nullptr), Scaling(false), Dirty(true), UserPointer(nullptr)
		{
		}
		Transform::~Transform()
		{
			SetRoot(nullptr);
			RemoveChilds();

			TH_DELETE(Spacing, Local);
			UserPointer = nullptr;
			Local = nullptr;
		}
		void Transform::Synchronize()
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (!Dirty)
				return;

			Dirty = false;
			if (Root != nullptr)
			{
				Local->Offset = Matrix4x4::Create(Local->Position, Local->Rotation) * Root->GetBiasUnscaled();
				Global.Position = Local->Offset.Position();
				Global.Rotation = Local->Offset.Rotation();
				Global.Scale = (Scaling ? Local->Scale : Local->Scale * Root->Global.Scale);
				Temporary = Matrix4x4::CreateScale(Global.Scale) * Local->Offset;
			}
			else
			{
				Global.Offset = Matrix4x4::Create(Global.Position, Global.Scale, Global.Rotation);
				Temporary = Matrix4x4::CreateRotation(Global.Rotation) * Matrix4x4::CreateTranslation(Global.Position);
			}
		}
		void Transform::Move(const Vector3& Value)
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Position += Value;
			else
				Global.Position += Value;
			MakeDirty();
		}
		void Transform::Rotate(const Vector3& Value)
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Rotation += Value;
			else
				Global.Rotation += Value;
			MakeDirty();
		}
		void Transform::Rescale(const Vector3& Value)
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
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
				Where.Rotation = Where.Offset.Rotation();
				Where.Scale = (Scaling ? Where.Scale : Where.Scale / Root->Global.Scale);
			}
		}
		void Transform::Globalize(Spacing& Where)
		{
			if (Root != nullptr)
			{
				Where.Offset = Matrix4x4::Create(Where.Position, Where.Rotation) * Root->GetBiasUnscaled();
				Where.Position = Where.Offset.Position();
				Where.Rotation = Where.Offset.Rotation();
				Where.Scale = (Scaling ? Where.Scale : Where.Scale * Root->Global.Scale);
			}
		}
		void Transform::Specialize(Spacing& Where)
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
			{
				Where.Offset = Matrix4x4::Create(Local->Position, Local->Rotation) * Root->GetBiasUnscaled();
				Where.Position = Where.Offset.Position();
				Where.Rotation = Where.Offset.Rotation();
				Where.Scale = (Scaling ? Local->Scale : Local->Scale * Root->Global.Scale);
			}
			else if (&Where != &Global)
				Where = Global;
		}
		void Transform::Copy(Transform* Target)
		{
			TH_ASSERT_V(Target != nullptr && Target != this, "target should be set");
			TH_DELETE(Spacing, Local);

			if (Target->Root != nullptr)
				Local = TH_NEW(Spacing, *Target->Local);
			else
				Local = nullptr;

			UserPointer = Target->UserPointer;
			Childs = Target->Childs;
			Global = Target->Global;
			Scaling = Target->Scaling;
			Root = Target->Root;
			Dirty = true;
		}
		void Transform::AddChild(Transform* Child)
		{
			TH_ASSERT_V(Child != nullptr && Child != this, "child should be set");
			Childs.push_back(Child);
			Child->MakeDirty();
		}
		void Transform::RemoveChild(Transform* Child)
		{
			TH_ASSERT_V(Child != nullptr && Child != this, "child should be set");
			if (Child->Root == this)
				Child->SetRoot(nullptr);
		}
		void Transform::RemoveChilds()
		{
			std::vector<Transform*> Array;
			Array.swap(Childs);

			for (auto& Child : Array)
			{
				if (Child->Root == this)
					Child->SetRoot(nullptr);
			}
		}
		void Transform::MakeDirty()
		{
			if (Dirty)
				return;

			Dirty = true;
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
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Position = Value;
			else
				Global.Position = Value;
			MakeDirty();
		}
		void Transform::SetRotation(const Vector3& Value)
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Rotation = Value;
			else
				Global.Rotation = Value;
			MakeDirty();
		}
		void Transform::SetScale(const Vector3& Value)
		{
			TH_ASSERT_V(!Root || Local != nullptr, "corrupted root transform");
			if (Root != nullptr)
				Local->Scale = Value;
			else
				Global.Scale = Value;
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
			TH_DELETE(Spacing, Local);
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
					TH_DELETE(Spacing, Local);
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
					Local = TH_NEW(Spacing);

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
		bool Transform::HasRoot(Transform* Target)
		{
			TH_ASSERT(Target != nullptr, false, "target should be set");
			Compute::Transform* UpperRoot = Root;
			while (UpperRoot != nullptr)
			{
				if (UpperRoot == Target)
					return true;

				UpperRoot = UpperRoot->GetRoot();
			}

			return false;
		}
		bool Transform::HasChild(Transform* Target)
		{
			TH_ASSERT(Target != nullptr, false, "target should be set");
			for (auto& Child : Childs)
			{
				if (Child == Target)
					return true;

				if (Child->HasChild(Target))
					return true;
			}

			return false;
		}
		bool Transform::HasScaling()
		{
			return Scaling;
		}
		bool Transform::CanRootBeApplied(Transform* Parent)
		{
			if ((!Root && !Parent) || Root == Parent)
				return false;

			if (Parent == this)
				return false;

			if (Parent && Parent->HasRoot(this))
				return false;

			return true;
		}
		bool Transform::IsDirty()
		{
			return Dirty;
		}
		const Matrix4x4& Transform::GetBias()
		{
			TH_ASSERT(!Root || Local != nullptr, Global.Offset, "corrupted root transform");
			if (Root != nullptr)
				return Temporary;

			return Global.Offset;
		}
		const Matrix4x4& Transform::GetBiasUnscaled()
		{
			TH_ASSERT(!Root || Local != nullptr, Temporary, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset;

			return Temporary;
		}
		const Vector3& Transform::GetPosition()
		{
			return Global.Position;
		}
		const Vector3& Transform::GetRotation()
		{
			return Global.Rotation;
		}
		const Vector3& Transform::GetScale()
		{
			return Global.Scale;
		}
		Vector3 Transform::Forward(bool ViewSpace)
		{
			TH_ASSERT(!Root || Local != nullptr, 0, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset.Forward(ViewSpace);

			return Global.Offset.Forward(ViewSpace);
		}
		Vector3 Transform::Right(bool ViewSpace)
		{
			TH_ASSERT(!Root || Local != nullptr, 0, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset.Right(ViewSpace);

			return Global.Offset.Right(ViewSpace);
		}
		Vector3 Transform::Up(bool ViewSpace)
		{
			TH_ASSERT(!Root || Local != nullptr, 0, "corrupted root transform");
			if (Root != nullptr)
				return Local->Offset.Up(ViewSpace);

			return Global.Offset.Up(ViewSpace);
		}
		Transform::Spacing& Transform::GetSpacing()
		{
			TH_ASSERT(!Root || Local != nullptr, Global, "corrupted root transform");
			if (Root != nullptr)
				return *Local;

			return Global;
		}
		Transform::Spacing& Transform::GetSpacing(Positioning Space)
		{
			TH_ASSERT(!Root || Local != nullptr, Global, "corrupted root transform");
			if (Space == Positioning::Local)
				return *Local;

			return Global;
		}
		Transform* Transform::GetRoot()
		{
			return Root;
		}
		Transform* Transform::GetUpperRoot()
		{
			Compute::Transform* UpperRoot = Root;
			while (UpperRoot != nullptr)
				UpperRoot = UpperRoot->GetRoot();

			return nullptr;
		}
		Transform* Transform::GetChild(size_t Child)
		{
			TH_ASSERT(Child < Childs.size(), nullptr, "index outside of range");
			return Childs[Child];
		}
		size_t Transform::GetChildsCount()
		{
			return Childs.size();
		}
		std::vector<Transform*>& Transform::GetChilds()
		{
			return Childs;
		}

		Area::Area()
		{
		}
		Area::Area(const Vector3& LowerBound, const Vector3& UpperBound) : Lower(LowerBound), Upper(UpperBound)
		{
			TH_ASSERT_V(Lower <= Upper, "lower should be smaller than upper");
			Recompute();
		}
		void Area::Recompute()
		{
			Volume = 0;
			for (size_t i = 0; i < 3; i++)
			{
				float Product = 1;
				for (size_t j = 0; j < 3; j++)
				{
					if (i != j)
						Product *= Upper[j] - Lower[j];
				}
				Volume += Product;
			}

			Volume *= 2.0f;
			Center = (Lower + Upper) * 0.5f;
		}
		void Area::Merge(const Area& A, const Area& B)
		{
			Lower.X = std::min(A.Lower.X, B.Lower.X);
			Lower.Y = std::min(A.Lower.Y, B.Lower.Y);
			Lower.Z = std::min(A.Lower.Z, B.Lower.Z);
			Upper.X = std::max(A.Upper.X, B.Upper.X);
			Upper.Y = std::max(A.Upper.Y, B.Upper.Y);
			Upper.Z = std::max(A.Upper.Z, B.Upper.Z);
			Recompute();
		}
		bool Area::Contains(const Area& Box) const
		{
			return Box.Lower >= Lower && Box.Upper <= Upper;
		}
		bool Area::Overlaps(const Area& Box) const
		{
			return Box.Upper >= Lower && Box.Lower <= Upper;
		}

		Cosmos::Node::Node() : Item(nullptr)
		{
		}
		bool Cosmos::Node::IsLeaf() const
		{
			return (Left == NULL_NODE);
		}

		Cosmos::Cosmos(uint64_t DefaultSize)
		{
			Root = NULL_NODE;
			NodeCount = 0;
			NodeCapacity = DefaultSize;
			Nodes.resize(NodeCapacity);
			Stack.Reserve(256);

			for (uint64_t i = 0; i < NodeCapacity - 1; i++)
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
		void Cosmos::FreeNode(uint64_t NodeIndex)
		{
			TH_ASSERT_V(NodeIndex < NodeCapacity, "outside of borders");
			TH_ASSERT_V(NodeCount > 0, "there must be at least one node");

			auto& Node = Nodes[NodeIndex];
			Node.Next = FreeList;
			Node.Height = -1;
			FreeList = NodeIndex;
			NodeCount--;
		}
		void Cosmos::InsertItem(void* Item, const Vector3& Lower, const Vector3& Upper)
		{
			uint64_t NodeIndex = AllocateNode();
			auto& Node = Nodes[NodeIndex];
			Node.Box.Lower = Lower;
			Node.Box.Upper = Upper;
			Node.Box.Recompute();
			Node.Height = 0;
			Node.Item = Item;
			InsertLeaf(NodeIndex);

			Items.insert(std::unordered_map<void*, uint64_t>::value_type(Item, NodeIndex));
		}
		void Cosmos::RemoveItem(void* Item)
		{
			auto It = Items.find(Item);
			if (It == Items.end())
				return;

			uint64_t NodeIndex = It->second;
			Items.erase(It);

			TH_ASSERT_V(NodeIndex < NodeCapacity, "outside of borders");
			TH_ASSERT_V(Nodes[NodeIndex].IsLeaf(), "cannot remove root node");

			RemoveLeaf(NodeIndex);
			FreeNode(NodeIndex);
		}
		void Cosmos::InsertLeaf(uint64_t LeafIndex)
		{
			if (Root == NULL_NODE)
			{
				Root = LeafIndex;
				Nodes[Root].Parent = NULL_NODE;
				return;
			}

			uint64_t NextIndex = Root;
			Area LeafBox = Nodes[LeafIndex].Box;
			Area NextBox;

			while (!Nodes[NextIndex].IsLeaf())
			{
				auto& Next = Nodes[NextIndex];
				auto& Left = Nodes[Next.Left];
				auto& Right = Nodes[Next.Right];

				NextBox.Merge(LeafBox, Next.Box);
				float BaseCost = 2.0 * NextBox.Volume;
				float ParentCost = 2.0 * (NextBox.Volume - Next.Box.Volume);

				NextBox.Merge(LeafBox, Left.Box);
				float LeftCost = (NextBox.Volume - (Left.IsLeaf() ? 0.0f : Left.Box.Volume)) + ParentCost;

				NextBox.Merge(LeafBox, Right.Box);
				float RightCost = (NextBox.Volume - (Right.IsLeaf() ? 0.0f : Right.Box.Volume)) + ParentCost;

				if ((BaseCost < LeftCost) && (BaseCost < RightCost))
					break;

				if (LeftCost < RightCost)
					NextIndex = Next.Left;
				else
					NextIndex = Next.Right;
			}

			uint64_t NewParentIndex = AllocateNode();
			auto& Leaf = Nodes[LeafIndex];
			auto& Sibling = Nodes[NextIndex];
			auto& NewParent = Nodes[NewParentIndex];

			uint64_t OldParentIndex = Sibling.Parent;
			NewParent.Parent = OldParentIndex;
			NewParent.Box.Merge(LeafBox, Sibling.Box);
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
				Next.Box.Merge(Left.Box, Right.Box);
				NextIndex = Next.Parent;
			}
		}
		void Cosmos::RemoveLeaf(uint64_t LeafIndex)
		{
			if (LeafIndex == Root)
			{
				Root = NULL_NODE;
				return;
			}

			uint64_t ParentIndex = Nodes[LeafIndex].Parent;
			auto& Parent = Nodes[ParentIndex];

			uint64_t UpperParentIndex = Parent.Parent;
			uint64_t SiblingIndex = (Parent.Left == LeafIndex ? Parent.Right : Parent.Left);
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

				uint64_t NextIndex = Parent.Parent;
				while (NextIndex != NULL_NODE)
				{
					NextIndex = Balance(NextIndex);
					auto& Next = Nodes[NextIndex];
					auto& Left = Nodes[Next.Left];
					auto& Right = Nodes[Next.Right];

					Next.Box.Merge(Left.Box, Right.Box);
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
				uint64_t NodeIndex = It->second;
				TH_ASSERT_V(NodeIndex < NodeCapacity, "outside of borders");
				TH_ASSERT_V(Nodes[NodeIndex].IsLeaf(), "cannot remove root node");

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
				Area Box(Lower, Upper);
				if (Next.Box.Contains(Box))
					return true;

				RemoveLeaf(It->second);
				Next.Box = Box;
			}
			else
			{
				RemoveLeaf(It->second);
				Next.Box.Upper = Upper;
				Next.Box.Lower = Lower;
				Next.Box.Recompute();
			}

			InsertLeaf(It->second);
			return true;
		}
		void Cosmos::PushQuery()
		{
			Stack.Clear();
			if (!Items.empty())
				Stack.Add(Root);
		}
		void* Cosmos::NextQuery(const Area& Box)
		{
			while (!Stack.Empty())
			{
				auto& Next = Nodes[Stack.Back()];
				Stack.PopBack();

				if (!Box.Overlaps(Next.Box))
					continue;

				if (!Next.IsLeaf())
				{
					Stack.AddUnbounded(Next.Left);
					Stack.AddUnbounded(Next.Right);
				}
				else if (Next.Item != nullptr)
					return Next.Item;
			}

			return nullptr;
		}
		const Area& Cosmos::GetArea(void* Item)
		{
			auto It = Items.find(Item);
			if (It != Items.end())
				return Nodes[It->second].Box;

			return Nodes[Root].Box;
		}
		uint64_t Cosmos::AllocateNode()
		{
			if (FreeList == NULL_NODE)
			{
				TH_ASSERT(NodeCount == NodeCapacity, 0, "invalid capacity");

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

			uint64_t NodeIndex = FreeList;
			auto& Node = Nodes[NodeIndex];
			FreeList = Node.Next;
			Node.Parent = NULL_NODE;
			Node.Left = NULL_NODE;
			Node.Right = NULL_NODE;
			Node.Height = 0;
			NodeCount++;

			return NodeIndex;
		}
		uint64_t Cosmos::Balance(uint64_t NodeIndex)
		{
			TH_ASSERT(NodeIndex != NULL_NODE, NULL_NODE, "node should not be null");

			auto& Next = Nodes[NodeIndex];
			if (Next.IsLeaf() || (Next.Height < 2))
				return NodeIndex;

			TH_ASSERT(Next.Left < NodeCapacity, NULL_NODE, "left outside of borders");
			TH_ASSERT(Next.Right < NodeCapacity, NULL_NODE, "right outside of borders");

			uint64_t LeftIndex = Next.Left;
			uint64_t RightIndex = Next.Right;
			auto& Left = Nodes[LeftIndex];
			auto& Right = Nodes[RightIndex];

			int Balance = Right.Height - Left.Height;
			if (Balance > 1)
			{
				TH_ASSERT(Right.Left < NodeCapacity, NULL_NODE, "subleft outside of borders");
				TH_ASSERT(Right.Right < NodeCapacity, NULL_NODE, "subright outside of borders");

				uint64_t RightLeftIndex = Right.Left;
				uint64_t RightRightIndex = Right.Right;
				auto& RightLeft = Nodes[RightLeftIndex];
				auto& RightRight = Nodes[RightRightIndex];

				Right.Left = NodeIndex;
				Right.Parent = Next.Parent;
				Next.Parent = RightIndex;

				if (Right.Parent != NULL_NODE)
				{
					if (Nodes[Right.Parent].Left != NodeIndex)
					{
						TH_ASSERT(Nodes[Right.Parent].Right == NodeIndex, NULL_NODE, "invalid right spacing");
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
					Next.Box.Merge(Left.Box, RightRight.Box);
					Right.Box.Merge(Next.Box, RightLeft.Box);

					Next.Height = 1 + std::max(Left.Height, RightRight.Height);
					Right.Height = 1 + std::max(Next.Height, RightLeft.Height);
				}
				else
				{
					Right.Right = RightRightIndex;
					Next.Right = RightLeftIndex;
					RightLeft.Parent = NodeIndex;
					Next.Box.Merge(Left.Box, RightLeft.Box);
					Right.Box.Merge(Next.Box, RightRight.Box);

					Next.Height = 1 + std::max(Left.Height, RightLeft.Height);
					Right.Height = 1 + std::max(Next.Height, RightRight.Height);
				}

				return RightIndex;
			}
			else if (Balance < -1)
			{
				TH_ASSERT(Left.Left < NodeCapacity, NULL_NODE, "subleft outside of borders");
				TH_ASSERT(Left.Right < NodeCapacity, NULL_NODE, "subright outside of borders");

				uint64_t LeftLeftIndex = Left.Left;
				uint64_t LeftRightIndex = Left.Right;
				auto& LeftLeft = Nodes[LeftLeftIndex];
				auto& LeftRight = Nodes[LeftRightIndex];

				Left.Left = NodeIndex;
				Left.Parent = Next.Parent;
				Next.Parent = LeftIndex;

				if (Left.Parent != NULL_NODE)
				{
					if (Nodes[Left.Parent].Left != NodeIndex)
					{
						TH_ASSERT(Nodes[Left.Parent].Right == NodeIndex, NULL_NODE, "invalid left spacing");
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
					Next.Box.Merge(Right.Box, LeftRight.Box);
					Left.Box.Merge(Next.Box, LeftLeft.Box);
					Next.Height = 1 + std::max(Right.Height, LeftRight.Height);
					Left.Height = 1 + std::max(Next.Height, LeftLeft.Height);
				}
				else
				{
					Left.Right = LeftRightIndex;
					Next.Left = LeftLeftIndex;
					LeftLeft.Parent = NodeIndex;
					Next.Box.Merge(Right.Box, LeftLeft.Box);
					Left.Box.Merge(Next.Box, LeftRight.Box);
					Next.Height = 1 + std::max(Right.Height, LeftLeft.Height);
					Left.Height = 1 + std::max(Next.Height, LeftRight.Height);
				}

				return LeftIndex;
			}

			return NodeIndex;
		}
		uint64_t Cosmos::ComputeHeight() const
		{
			return ComputeHeight(Root);
		}
		uint64_t Cosmos::ComputeHeight(uint64_t NodeIndex) const
		{
			TH_ASSERT(NodeIndex < NodeCapacity, 0, "outside of borders");

			auto& Next = Nodes[NodeIndex];
			if (Next.IsLeaf())
				return 0;

			uint64_t Height1 = ComputeHeight(Next.Left);
			uint64_t Height2 = ComputeHeight(Next.Right);
			return 1 + std::max(Height1, Height2);
		}
		uint64_t Cosmos::GetHeight() const
		{
			return Root == NULL_NODE ? 0 : Nodes[Root].Height;
		}
		uint64_t Cosmos::GetMaxBalance() const
		{
			uint64_t MaxBalance = 0;
			for (uint64_t i = 0; i < NodeCapacity; i++)
			{
				auto& Next = Nodes[i];
				if (Next.Height <= 1)
					continue;

				TH_ASSERT(!Next.IsLeaf(), 0, "node should be leaf");
				uint64_t Balance = std::abs(Nodes[Next.Left].Height - Nodes[Next.Right].Height);
				MaxBalance = std::max(MaxBalance, Balance);
			}

			return MaxBalance;
		}
		const std::unordered_map<void*, uint64_t>& Cosmos::GetItems() const
		{
			return Items;
		}
		const std::vector<Cosmos::Node>& Cosmos::GetNodes() const
		{
			return Nodes;
		}
		float Cosmos::GetVolumeRatio() const
		{
			if (Root == NULL_NODE)
				return 0.0;

			float RootArea = Nodes[Root].Box.Volume;
			float TotalArea = 0.0;

			for (uint64_t i = 0; i < NodeCapacity; i++)
			{
				auto& Next = Nodes[i];
				if (Next.Height >= 0)
					TotalArea += Next.Box.Volume;
			}

			return TotalArea / RootArea;
		}

		HullShape::HullShape() : Shape(nullptr)
		{
		}

		RigidBody::RigidBody(Simulator* Refer, const Desc& I) : Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
			TH_ASSERT_V(Initial.Shape, "collision shape should be set");
			TH_ASSERT_V(Engine != nullptr, "simulator should be set");
#ifdef TH_WITH_BULLET3
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
			btRigidBody::btRigidBodyConstructionInfo Info(Initial.Mass, TH_NEW(btDefaultMotionState, BtTransform), Initial.Shape, LocalInertia);
			Instance = TH_NEW(btRigidBody, Info);
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
		RigidBody::~RigidBody()
		{
#ifdef TH_WITH_BULLET3
			if (!Instance)
				return;

			int Constrs = Instance->getNumConstraintRefs();
			for (int i = 0; i < Constrs; i++)
			{
				btTypedConstraint* Constr = Instance->getConstraintRef(i);
				if (Constr != nullptr)
				{
					void* Ptr = Constr->getUserConstraintPtr();
					if (Ptr != nullptr)
					{
						btTypedConstraintType Type = Constr->getConstraintType();
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

					Instance->removeConstraintRef(Constr);
					Constrs--; i--;
				}
			}

			if (Instance->getMotionState())
			{
				btMotionState* Object = Instance->getMotionState();
				TH_DELETE(btMotionState, Object);
				Instance->setMotionState(nullptr);
			}

			Instance->setCollisionShape(nullptr);
			Instance->setUserPointer(nullptr);
			if (Instance->getWorldArrayIndex() >= 0)
				Engine->GetWorld()->removeRigidBody(Instance);

			if (Initial.Shape)
				Engine->FreeShape(&Initial.Shape);

			TH_DELETE(btRigidBody, Instance);
#endif
		}
		RigidBody* RigidBody::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "rigidbody should be initialized");

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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->applyCentralImpulse(V3_TO_BT(Velocity));
#endif
		}
		void RigidBody::Push(const Vector3& Velocity, const Vector3& Torque)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->applyCentralImpulse(V3_TO_BT(Velocity));
			Instance->applyTorqueImpulse(V3_TO_BT(Torque));
#endif
		}
		void RigidBody::Push(const Vector3& Velocity, const Vector3& Torque, const Vector3& Center)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->applyImpulse(V3_TO_BT(Velocity), V3_TO_BT(Center));
			Instance->applyTorqueImpulse(V3_TO_BT(Torque));
#endif
		}
		void RigidBody::PushKinematic(const Vector3& Velocity)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");

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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");

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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");

			btTransform& Base = Instance->getWorldTransform();
			if (!Kinematic)
			{
				btScalar X, Y, Z;
				const btVector3& Position = Base.getOrigin();
				const btVector3& Scale = Instance->getCollisionShape()->getLocalScaling();
				Base.getRotation().getEulerZYX(Z, Y, X);
				Transform->SetPosition(BT_TO_V3(Position));
				Transform->SetRotation(Vector3(-X, -Y, Z));
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
#endif
		}
		void RigidBody::SetAsNormal()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCollisionFlags(0);
#endif
		}
		void RigidBody::SetSelfPointer()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setUserPointer(this);
#endif
		}
		void RigidBody::SetWorldTransform(btTransform* Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			TH_ASSERT_V(Value != nullptr, "transform should be set");
			Instance->setWorldTransform(*Value);
#endif
		}
		void RigidBody::SetActivationState(MotionState Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->forceActivationState((int)Value);
#endif
		}
		void RigidBody::SetAngularDamping(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setDamping(Instance->getLinearDamping(), Value);
#endif
		}
		void RigidBody::SetAngularSleepingThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setSleepingThresholds(Instance->getLinearSleepingThreshold(), Value);
#endif
		}
		void RigidBody::SetFriction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setFriction(Value);
#endif
		}
		void RigidBody::SetRestitution(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setRestitution(Value);
#endif
		}
		void RigidBody::SetSpinningFriction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setSpinningFriction(Value);
#endif
		}
		void RigidBody::SetContactStiffness(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setContactStiffnessAndDamping(Value, Instance->getContactDamping());
#endif
		}
		void RigidBody::SetContactDamping(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setContactStiffnessAndDamping(Instance->getContactStiffness(), Value);
#endif
		}
		void RigidBody::SetHitFraction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setHitFraction(Value);
#endif
		}
		void RigidBody::SetLinearDamping(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setDamping(Value, Instance->getAngularDamping());
#endif
		}
		void RigidBody::SetLinearSleepingThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setSleepingThresholds(Value, Instance->getAngularSleepingThreshold());
#endif
		}
		void RigidBody::SetCcdMotionThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCcdMotionThreshold(Value);
#endif
		}
		void RigidBody::SetCcdSweptSphereRadius(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCcdSweptSphereRadius(Value);
#endif
		}
		void RigidBody::SetContactProcessingThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setContactProcessingThreshold(Value);
#endif
		}
		void RigidBody::SetDeactivationTime(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setDeactivationTime(Value);
#endif
		}
		void RigidBody::SetRollingFriction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setRollingFriction(Value);
#endif
		}
		void RigidBody::SetAngularFactor(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setAngularFactor(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetAnisotropicFriction(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setAnisotropicFriction(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetGravity(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setGravity(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetLinearFactor(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setLinearFactor(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetLinearVelocity(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setLinearVelocity(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetAngularVelocity(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setAngularVelocity(V3_TO_BT(Value));
#endif
		}
		void RigidBody::SetCollisionShape(btCollisionShape* Shape, Transform* Transform)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			btCollisionShape* Collision = Instance->getCollisionShape();
			TH_DELETE(btCollisionShape, Collision);

			Instance->setCollisionShape(Shape);
			if (Transform)
				Synchronize(Transform, true);
#endif
		}
		void RigidBody::SetMass(float Mass)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr && Engine != nullptr, "rigidbody should be initialized");
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
		void RigidBody::SetCollisionFlags(uint64_t Flags)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "rigidbody should be initialized");
			Instance->setCollisionFlags((int)Flags);
#endif
		}
		MotionState RigidBody::GetActivationState()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, MotionState::Active, "rigidbody should be initialized");
			return (MotionState)Instance->getActivationState();
#else
			return MotionState::Island_Sleeping;
#endif
		}
		Shape RigidBody::GetCollisionShapeType()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr && Instance->getCollisionShape() != nullptr, Shape::Invalid, "rigidbody should be initialized");
			return (Shape)Instance->getCollisionShape()->getShapeType();
#else
			return Shape::Invalid;
#endif
		}
		Vector3 RigidBody::GetAngularFactor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getAngularFactor();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetAnisotropicFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getAnisotropicFriction();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetGravity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getGravity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetLinearFactor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getLinearFactor();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetLinearVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getLinearVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetAngularVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getAngularVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetScale()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr && Instance->getCollisionShape() != nullptr, 1, "rigidbody should be initialized");
			btVector3 Value = Instance->getCollisionShape()->getLocalScaling();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetPosition()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btVector3 Value = Instance->getWorldTransform().getOrigin();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 RigidBody::GetRotation()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			btScalar X, Y, Z;
			Instance->getWorldTransform().getBasis().getEulerZYX(Z, Y, X);
			return Vector3(-X, -Y, Z);
#else
			return 0;
#endif
		}
		btTransform* RigidBody::GetWorldTransform()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, nullptr, "rigidbody should be initialized");
			return &Instance->getWorldTransform();
#else
			return nullptr;
#endif
		}
		btCollisionShape* RigidBody::GetCollisionShape()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, nullptr, "rigidbody should be initialized");
			return Instance->getCollisionShape();
#else
			return nullptr;
#endif
		}
		btRigidBody* RigidBody::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool RigidBody::IsGhost()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "rigidbody should be initialized");
			return (Instance->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
#else
			return false;
#endif
		}
		bool RigidBody::IsActive()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "rigidbody should be initialized");
			return Instance->isActive();
#else
			return false;
#endif
		}
		bool RigidBody::IsStatic()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "rigidbody should be initialized");
			return Instance->isStaticObject();
#else
			return true;
#endif
		}
		bool RigidBody::IsColliding()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "rigidbody should be initialized");
			return Instance->hasContactResponse();
#else
			return false;
#endif
		}
		float RigidBody::GetSpinningFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getSpinningFriction();
#else
			return 0;
#endif
		}
		float RigidBody::GetContactStiffness()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getContactStiffness();
#else
			return 0;
#endif
		}
		float RigidBody::GetContactDamping()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getContactDamping();
#else
			return 0;
#endif
		}
		float RigidBody::GetAngularDamping()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getAngularDamping();
#else
			return 0;
#endif
		}
		float RigidBody::GetAngularSleepingThreshold()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getAngularSleepingThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getFriction();
#else
			return 0;
#endif
		}
		float RigidBody::GetRestitution()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getRestitution();
#else
			return 0;
#endif
		}
		float RigidBody::GetHitFraction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getHitFraction();
#else
			return 0;
#endif
		}
		float RigidBody::GetLinearDamping()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getLinearDamping();
#else
			return 0;
#endif
		}
		float RigidBody::GetLinearSleepingThreshold()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getLinearSleepingThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetCcdMotionThreshold()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getCcdMotionThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetCcdSweptSphereRadius()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getCcdSweptSphereRadius();
#else
			return 0;
#endif
		}
		float RigidBody::GetContactProcessingThreshold()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getContactProcessingThreshold();
#else
			return 0;
#endif
		}
		float RigidBody::GetDeactivationTime()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getDeactivationTime();
#else
			return 0;
#endif
		}
		float RigidBody::GetRollingFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getRollingFriction();
#else
			return 0;
#endif
		}
		float RigidBody::GetMass()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			float Mass = Instance->getInvMass();
			return (Mass != 0.0f ? 1.0f / Mass : 0.0f);
#else
			return 0;
#endif
		}
		uint64_t RigidBody::GetCollisionFlags()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "rigidbody should be initialized");
			return Instance->getCollisionFlags();
#else
			return 0;
#endif
		}
		RigidBody::Desc& RigidBody::GetInitialState()
		{
			return Initial;
		}
		Simulator* RigidBody::GetSimulator()
		{
			return Engine;
		}
		RigidBody* RigidBody::Get(btRigidBody* From)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(From != nullptr, 0, "rigidbody should be initialized");
			return (RigidBody*)From->getUserPointer();
#else
			return nullptr;
#endif
		}

		SoftBody::SoftBody(Simulator* Refer, const Desc& I) : Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Engine != nullptr, "engine should be set");
			TH_ASSERT_V(Engine->HasSoftBodySupport(), "soft body should be supported");

			btQuaternion Rotation;
			Rotation.setEulerZYX(Initial.Rotation.Z, Initial.Rotation.Y, Initial.Rotation.X);

			btTransform BtTransform(Rotation, btVector3(Initial.Position.X, Initial.Position.Y, -Initial.Position.Z));
			btSoftRigidDynamicsWorld* World = (btSoftRigidDynamicsWorld*)Engine->GetWorld();
			btSoftBodyWorldInfo& Info = World->getWorldInfo();
			HullShape* Hull = Initial.Shape.Convex.Hull;

			if (Initial.Shape.Convex.Enabled && Hull != nullptr)
			{
				std::vector<btScalar> Vertices;
				Vertices.resize(Hull->Vertices.size() * 3);

				for (size_t i = 0; i < Hull->Vertices.size(); i++)
				{
					Vertex& V = Hull->Vertices[i];
					Vertices[i * 3 + 0] = (btScalar)V.PositionX;
					Vertices[i * 3 + 1] = (btScalar)V.PositionY;
					Vertices[i * 3 + 2] = (btScalar)V.PositionZ;
				}

				Instance = btSoftBodyHelpers::CreateFromTriMesh(Info, Vertices.data(), Hull->Indices.data(), (int)Hull->Indices.size() / 3, false);
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
		SoftBody::~SoftBody()
		{
#ifdef TH_WITH_BULLET3
			if (!Instance)
				return;

			btSoftRigidDynamicsWorld* World = (btSoftRigidDynamicsWorld*)Engine->GetWorld();
			if (Instance->getWorldArrayIndex() >= 0)
				World->removeSoftBody(Instance);

			Instance->setUserPointer(nullptr);
			TH_DELETE(btSoftBody, Instance);
#endif
		}
		SoftBody* SoftBody::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "softbody should be initialized");

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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->activate(Force);
#endif
		}
		void SoftBody::Synchronize(Transform* Transform, bool Kinematic)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Transform != nullptr, "transform should be set");
#ifdef TH_WITH_SIMD
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
				Transform->SetRotation(Vector3(-X, -Y, Z));
			}
			else
			{
				Vector3 Position = Transform->GetPosition().InvZ() - Center;
				if (Position.Length() > 0.005f)
					Instance->translate(V3_TO_BT(Position));
			}
#endif
		}
		void SoftBody::GetIndices(std::vector<int>* Result)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Result != nullptr, "result should be set");

			std::unordered_map<btSoftBody::Node*, int> Nodes;
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
		void SoftBody::GetVertices(std::vector<Vertex>* Result)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Result != nullptr, "result should be set");

			static size_t PositionX = offsetof(Compute::Vertex, PositionX);
			static size_t NormalX = offsetof(Compute::Vertex, NormalX);

			size_t Size = (size_t)Instance->m_nodes.size();
			if (Result->size() != Size)
			{
				if (Initial.Shape.Convex.Enabled)
					*Result = Initial.Shape.Convex.Hull->Vertices;
				else
					Result->resize(Size);
			}

			for (size_t i = 0; i < Size; i++)
			{
				auto* Node = &Instance->m_nodes[i]; Vertex& Position = Result->at(i);
				memcpy(&Position + PositionX, Node->m_x.m_floats, sizeof(float) * 3);
				memcpy(&Position + NormalX, Node->m_n.m_floats, sizeof(float) * 3);
			}
#endif
		}
		void SoftBody::GetBoundingBox(Vector3* Min, Vector3* Max)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");

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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setContactStiffnessAndDamping(Stiffness, Damping);
#endif
		}
		void SoftBody::AddAnchor(int Node, RigidBody* Body, bool DisableCollisionBetweenLinkedBodies, float Influence)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Body != nullptr, "body should be set");
			Instance->appendAnchor(Node, Body->Get(), DisableCollisionBetweenLinkedBodies, Influence);
#endif
		}
		void SoftBody::AddAnchor(int Node, RigidBody* Body, const Vector3& LocalPivot, bool DisableCollisionBetweenLinkedBodies, float Influence)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Body != nullptr, "body should be set");
			Instance->appendAnchor(Node, Body->Get(), V3_TO_BT(LocalPivot), DisableCollisionBetweenLinkedBodies, Influence);
#endif
		}
		void SoftBody::AddForce(const Vector3& Force)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->addForce(V3_TO_BT(Force));
#endif
		}
		void SoftBody::AddForce(const Vector3& Force, int Node)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->addForce(V3_TO_BT(Force), Node);
#endif
		}
		void SoftBody::AddAeroForceToNode(const Vector3& WindVelocity, int NodeIndex)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->addAeroForceToNode(V3_TO_BT(WindVelocity), NodeIndex);
#endif
		}
		void SoftBody::AddAeroForceToFace(const Vector3& WindVelocity, int FaceIndex)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->addAeroForceToFace(V3_TO_BT(WindVelocity), FaceIndex);
#endif
		}
		void SoftBody::AddVelocity(const Vector3& Velocity)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->addVelocity(V3_TO_BT(Velocity));
#endif
		}
		void SoftBody::SetVelocity(const Vector3& Velocity)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setVelocity(V3_TO_BT(Velocity));
#endif
		}
		void SoftBody::AddVelocity(const Vector3& Velocity, int Node)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->addVelocity(V3_TO_BT(Velocity), Node);
#endif
		}
		void SoftBody::SetMass(int Node, float Mass)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setMass(Node, Mass);
#endif
		}
		void SoftBody::SetTotalMass(float Mass, bool FromFaces)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setTotalMass(Mass, FromFaces);
#endif
		}
		void SoftBody::SetTotalDensity(float Density)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setTotalDensity(Density);
#endif
		}
		void SoftBody::SetVolumeMass(float Mass)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setVolumeMass(Mass);
#endif
		}
		void SoftBody::SetVolumeDensity(float Density)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setVolumeDensity(Density);
#endif
		}
		void SoftBody::Translate(const Vector3& Position)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->translate(btVector3(Position.X, Position.Y, -Position.Z));
#endif
		}
		void SoftBody::Rotate(const Vector3& Rotation)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			btQuaternion Value;
			Value.setEulerZYX(Rotation.X, Rotation.Y, Rotation.Z);
			Instance->rotate(Value);
#endif
		}
		void SoftBody::Scale(const Vector3& Scale)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->scale(V3_TO_BT(Scale));
#endif
		}
		void SoftBody::SetRestLengthScale(float RestLength)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setRestLengthScale(RestLength);
#endif
		}
		void SoftBody::SetPose(bool Volume, bool Frame)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setPose(Volume, Frame);
#endif
		}
		float SoftBody::GetMass(int Node) const
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getMass(Node);
#else
			return 0;
#endif
		}
		float SoftBody::GetTotalMass() const
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getTotalMass();
#else
			return 0;
#endif
		}
		float SoftBody::GetRestLengthScale()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getRestLengthScale();
#else
			return 0;
#endif
		}
		float SoftBody::GetVolume() const
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getVolume();
#else
			return 0;
#endif
		}
		int SoftBody::GenerateBendingConstraints(int Distance)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->generateBendingConstraints(Distance);
#else
			return 0;
#endif
		}
		void SoftBody::RandomizeConstraints()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->randomizeConstraints();
#endif
		}
		bool SoftBody::CutLink(int Node0, int Node1, float Position)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "softbody should be initialized");
			return Instance->cutLink(Node0, Node1, Position);
#else
			return false;
#endif
		}
		bool SoftBody::RayTest(const Vector3& From, const Vector3& To, RayCast& Result)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "softbody should be initialized");
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setWindVelocity(V3_TO_BT(Velocity));
#endif
		}
		Vector3 SoftBody::GetWindVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			btVector3 Value = Instance->getWindVelocity();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		void SoftBody::GetAabb(Vector3& Min, Vector3& Max) const
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			btVector3 BMin, BMax;
			Instance->getAabb(BMin, BMax);
			Min = BT_TO_V3(BMin);
			Max = BT_TO_V3(BMax);
#endif
		}
		void SoftBody::IndicesToPointers(const int* Map)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Map != nullptr, "map should be set");
			Instance->indicesToPointers(Map);
#endif
		}
		void SoftBody::SetSpinningFriction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setSpinningFriction(Value);
#endif
		}
		Vector3 SoftBody::GetLinearVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			btVector3 Value = Instance->getInterpolationLinearVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetAngularVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			btVector3 Value = Instance->getInterpolationAngularVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetCenterPosition()
		{
#ifdef TH_WITH_BULLET3
			return Center;
#else
			return 0;
#endif
		}
		void SoftBody::SetActivity(bool Active)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
#endif
		}
		void SoftBody::SetAsNormal()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setCollisionFlags(0);
#endif
		}
		void SoftBody::SetSelfPointer()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setUserPointer(this);
#endif
		}
		void SoftBody::SetWorldTransform(btTransform* Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			TH_ASSERT_V(Value != nullptr, "transform should be set");
			Instance->setWorldTransform(*Value);
#endif
		}
		void SoftBody::SetActivationState(MotionState Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->forceActivationState((int)Value);
#endif
		}
		void SoftBody::SetFriction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setFriction(Value);
#endif
		}
		void SoftBody::SetRestitution(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setRestitution(Value);
#endif
		}
		void SoftBody::SetContactStiffness(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setContactStiffnessAndDamping(Value, Instance->getContactDamping());
#endif
		}
		void SoftBody::SetContactDamping(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setContactStiffnessAndDamping(Instance->getContactStiffness(), Value);
#endif
		}
		void SoftBody::SetHitFraction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setHitFraction(Value);
#endif
		}
		void SoftBody::SetCcdMotionThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setCcdMotionThreshold(Value);
#endif
		}
		void SoftBody::SetCcdSweptSphereRadius(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setCcdSweptSphereRadius(Value);
#endif
		}
		void SoftBody::SetContactProcessingThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setContactProcessingThreshold(Value);
#endif
		}
		void SoftBody::SetDeactivationTime(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setDeactivationTime(Value);
#endif
		}
		void SoftBody::SetRollingFriction(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setRollingFriction(Value);
#endif
		}
		void SoftBody::SetAnisotropicFriction(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
			Instance->setAnisotropicFriction(V3_TO_BT(Value));
#endif
		}
		void SoftBody::SetConfig(const Desc::SConfig& Conf)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "softbody should be initialized");
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
		MotionState SoftBody::GetActivationState()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, MotionState::Active, "softbody should be initialized");
			return (MotionState)Instance->getActivationState();
#else
			return MotionState::Island_Sleeping;
#endif
		}
		Shape SoftBody::GetCollisionShapeType()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, Shape::Invalid, "softbody should be initialized");
			if (!Initial.Shape.Convex.Enabled)
				return Shape::Invalid;

			return Shape::Convex_Hull;
#else
			return Shape::Invalid;
#endif
		}
		Vector3 SoftBody::GetAnisotropicFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			btVector3 Value = Instance->getAnisotropicFriction();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetScale()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 1, "softbody should be initialized");
			btVector3 bMin, bMax;
			Instance->getAabb(bMin, bMax);
			btVector3 bScale = bMax - bMin;
			Vector3 Scale = BT_TO_V3(bScale);

			return Scale.Div(2.0f).Abs();
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetPosition()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			btVector3 Value = Instance->getWorldTransform().getOrigin();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
#else
			return 0;
#endif
		}
		Vector3 SoftBody::GetRotation()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			btScalar X, Y, Z;
			Instance->getWorldTransform().getBasis().getEulerZYX(Z, Y, X);
			return Vector3(-X, -Y, Z);
#else
			return 0;
#endif
		}
		btTransform* SoftBody::GetWorldTransform()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, nullptr, "softbody should be initialized");
			return &Instance->getWorldTransform();
#else
			return nullptr;
#endif
		}
		btSoftBody* SoftBody::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool SoftBody::IsGhost()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "softbody should be initialized");
			return (Instance->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
#else
			return false;
#endif
		}
		bool SoftBody::IsActive()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "softbody should be initialized");
			return Instance->isActive();
#else
			return false;
#endif
		}
		bool SoftBody::IsStatic()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "softbody should be initialized");
			return Instance->isStaticObject();
#else
			return true;
#endif
		}
		bool SoftBody::IsColliding()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "softbody should be initialized");
			return Instance->hasContactResponse();
#else
			return false;
#endif
		}
		float SoftBody::GetSpinningFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getSpinningFriction();
#else
			return 0;
#endif
		}
		float SoftBody::GetContactStiffness()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getContactStiffness();
#else
			return 0;
#endif
		}
		float SoftBody::GetContactDamping()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getContactDamping();
#else
			return 0;
#endif
		}
		float SoftBody::GetFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getFriction();
#else
			return 0;
#endif
		}
		float SoftBody::GetRestitution()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getRestitution();
#else
			return 0;
#endif
		}
		float SoftBody::GetHitFraction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getHitFraction();
#else
			return 0;
#endif
		}
		float SoftBody::GetCcdMotionThreshold()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getCcdMotionThreshold();
#else
			return 0;
#endif
		}
		float SoftBody::GetCcdSweptSphereRadius()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getCcdSweptSphereRadius();
#else
			return 0;
#endif
		}
		float SoftBody::GetContactProcessingThreshold()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getContactProcessingThreshold();
#else
			return 0;
#endif
		}
		float SoftBody::GetDeactivationTime()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getDeactivationTime();
#else
			return 0;
#endif
		}
		float SoftBody::GetRollingFriction()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getRollingFriction();
#else
			return 0;
#endif
		}
		uint64_t SoftBody::GetCollisionFlags()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->getCollisionFlags();
#else
			return 0;
#endif
		}
		uint64_t SoftBody::GetVerticesCount()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "softbody should be initialized");
			return Instance->m_nodes.size();
#else
			return 0;
#endif
		}
		SoftBody::Desc& SoftBody::GetInitialState()
		{
			return Initial;
		}
		Simulator* SoftBody::GetSimulator()
		{
#ifdef TH_WITH_BULLET3
			return Engine;
#else
			return nullptr;
#endif
		}
		SoftBody* SoftBody::Get(btSoftBody* From)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(From != nullptr, nullptr, "softbody should be set");
			return (SoftBody*)From->getUserPointer();
#else
			return nullptr;
#endif
		}

		Constraint::Constraint(Simulator* Refer) : First(nullptr), Second(nullptr), Engine(Refer), UserPointer(nullptr)
		{
		}
		void Constraint::SetBreakingImpulseThreshold(float Value)
		{
#ifdef TH_WITH_BULLET3
			btTypedConstraint* Base = Get();
			TH_ASSERT_V(Base != nullptr, "typed constraint should be initialized");
			Base->setBreakingImpulseThreshold(Value);
#endif
		}
		void Constraint::SetEnabled(bool Value)
		{
#ifdef TH_WITH_BULLET3
			btTypedConstraint* Base = Get();
			TH_ASSERT_V(Base != nullptr, "typed constraint should be initialized");
			Base->setEnabled(Value);
#endif
		}
		btRigidBody* Constraint::GetFirst()
		{
#ifdef TH_WITH_BULLET3
			return First;
#else
			return nullptr;
#endif
		}
		btRigidBody* Constraint::GetSecond()
		{
#ifdef TH_WITH_BULLET3
			return Second;
#else
			return nullptr;
#endif
		}
		float Constraint::GetBreakingImpulseThreshold()
		{
#ifdef TH_WITH_BULLET3
			btTypedConstraint* Base = Get();
			TH_ASSERT(Base != nullptr, 0, "typed constraint should be initialized");
			return Base->getBreakingImpulseThreshold();
#else
			return 0;
#endif
		}
		bool Constraint::IsActive()
		{
#ifdef TH_WITH_BULLET3
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
		bool Constraint::IsEnabled()
		{
#ifdef TH_WITH_BULLET3
			btTypedConstraint* Base = Get();
			TH_ASSERT(Base != nullptr, false, "typed constraint should be initialized");
			return Base->isEnabled();
#else
			return false;
#endif
		}
		Simulator* Constraint::GetSimulator()
		{
			return Engine;
		}

		PConstraint::PConstraint(Simulator* Refer, const Desc& I) : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(I.TargetA != nullptr, "target A rigidbody should be set");
			TH_ASSERT_V(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = TH_NEW(btPoint2PointConstraint, *First, *Second, V3_TO_BT(I.PivotA), V3_TO_BT(I.PivotB));
			else
				Instance = TH_NEW(btPoint2PointConstraint, *First, V3_TO_BT(I.PivotA));

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		PConstraint::~PConstraint()
		{
#ifdef TH_WITH_BULLET3
			Engine->RemoveConstraint(this);
			TH_DELETE(btPoint2PointConstraint, Instance);
#endif
		}
		Constraint* PConstraint::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "p2p constraint should be initialized");
			PConstraint* Target = new PConstraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->SetPivotA(GetPivotA());
			Target->SetPivotB(GetPivotB());

			return Target;
		}
		btTypedConstraint* PConstraint::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool PConstraint::HasCollisions()
		{
			return State.Collisions;
		}
		void PConstraint::SetPivotA(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "p2p constraint should be initialized");
			Instance->setPivotA(V3_TO_BT(Value));
			State.PivotA = Value;
#endif
		}
		void PConstraint::SetPivotB(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "p2p constraint should be initialized");
			Instance->setPivotB(V3_TO_BT(Value));
			State.PivotB = Value;
#endif
		}
		Vector3 PConstraint::GetPivotA()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "p2p constraint should be initialized");
			const btVector3& Value = Instance->getPivotInA();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		Vector3 PConstraint::GetPivotB()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "p2p constraint should be initialized");
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

		HConstraint::HConstraint(Simulator* Refer, const Desc& I) : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(I.TargetA != nullptr, "target A rigidbody should be set");
			TH_ASSERT_V(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = TH_NEW(btHingeConstraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity(), I.References);
			else
				Instance = TH_NEW(btHingeConstraint, *First, btTransform::getIdentity(), I.References);

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		HConstraint::~HConstraint()
		{
#ifdef TH_WITH_BULLET3
			Engine->RemoveConstraint(this);
			TH_DELETE(btHingeConstraint, Instance);
#endif
		}
		Constraint* HConstraint::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "hinge constraint should be initialized");
			HConstraint* Target = new HConstraint(Engine, State);
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
			Target->SetEnabled(IsEnabled());
			Target->EnableAngularMotor(IsAngularMotorEnabled(), GetMotorTargetVelocity(), GetMaxMotorImpulse());
			Target->SetAngularOnly(IsAngularOnly());
			Target->SetLimit(GetLowerLimit(), GetUpperLimit(), GetLimitSoftness(), GetLimitBiasFactor(), GetLimitRelaxationFactor());
			Target->SetOffset(IsOffset());

			return Target;
		}
		btTypedConstraint* HConstraint::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool HConstraint::HasCollisions()
		{
			return State.Collisions;
		}
		void HConstraint::EnableAngularMotor(bool Enable, float TargetVelocity, float MaxMotorImpulse)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->enableAngularMotor(Enable, TargetVelocity, MaxMotorImpulse);
#endif
		}
		void HConstraint::EnableMotor(bool Enable)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->enableMotor(Enable);
#endif
		}
		void HConstraint::TestLimit(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->testLimit(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void HConstraint::SetFrames(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setFrames(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void HConstraint::SetAngularOnly(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setAngularOnly(Value);
#endif
		}
		void HConstraint::SetMaxMotorImpulse(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setMaxMotorImpulse(Value);
#endif
		}
		void HConstraint::SetMotorTargetVelocity(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setMotorTargetVelocity(Value);
#endif
		}
		void HConstraint::SetMotorTarget(float TargetAngle, float Delta)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setMotorTarget(TargetAngle, Delta);
#endif
		}
		void HConstraint::SetLimit(float Low, float High, float Softness, float BiasFactor, float RelaxationFactor)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setLimit(Low, High, Softness, BiasFactor, RelaxationFactor);
#endif
		}
		void HConstraint::SetOffset(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setUseFrameOffset(Value);
#endif
		}
		void HConstraint::SetReferenceToA(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			Instance->setUseReferenceFrameA(Value);
#endif
		}
		void HConstraint::SetAxis(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "hinge constraint should be initialized");
			btVector3 Axis = V3_TO_BT(Value);
			Instance->setAxis(Axis);
#endif
		}
		int HConstraint::GetSolveLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getSolveLimit();
#else
			return 0;
#endif
		}
		float HConstraint::GetMotorTargetVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getMotorTargetVelocity();
#else
			return 0;
#endif
		}
		float HConstraint::GetMaxMotorImpulse()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getMaxMotorImpulse();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitSign()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLimitSign();
#else
			return 0;
#endif
		}
		float HConstraint::GetHingeAngle()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getHingeAngle();
#else
			return 0;
#endif
		}
		float HConstraint::GetHingeAngle(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getHingeAngle(M16_TO_BT(A), M16_TO_BT(B));
#else
			return 0;
#endif
		}
		float HConstraint::GetLowerLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLowerLimit();
#else
			return 0;
#endif
		}
		float HConstraint::GetUpperLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getUpperLimit();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitSoftness()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLimitSoftness();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitBiasFactor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLimitBiasFactor();
#else
			return 0;
#endif
		}
		float HConstraint::GetLimitRelaxationFactor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLimitRelaxationFactor();
#else
			return 0;
#endif
		}
		bool HConstraint::HasLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->hasLimit();
#else
			return 0;
#endif
		}
		bool HConstraint::IsOffset()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getUseFrameOffset();
#else
			return 0;
#endif
		}
		bool HConstraint::IsReferenceToA()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getUseReferenceFrameA();
#else
			return 0;
#endif
		}
		bool HConstraint::IsAngularOnly()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getAngularOnly();
#else
			return 0;
#endif
		}
		bool HConstraint::IsAngularMotorEnabled()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getEnableAngularMotor();
#else
			return 0;
#endif
		}
		HConstraint::Desc& HConstraint::GetState()
		{
			return State;
		}

		SConstraint::SConstraint(Simulator* Refer, const Desc& I) : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(I.TargetA != nullptr, "target A rigidbody should be set");
			TH_ASSERT_V(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = TH_NEW(btSliderConstraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity(), I.Linear);
			else
				Instance = TH_NEW(btSliderConstraint, *First, btTransform::getIdentity(), I.Linear);

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		SConstraint::~SConstraint()
		{
#ifdef TH_WITH_BULLET3
			Engine->RemoveConstraint(this);
			TH_DELETE(btSliderConstraint, Instance);
#endif
		}
		Constraint* SConstraint::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "slider constraint should be initialized");
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
		btTypedConstraint* SConstraint::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool SConstraint::HasCollisions()
		{
			return State.Collisions;
		}
		void SConstraint::SetAngularMotorVelocity(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setTargetAngMotorVelocity(Value);
#endif
		}
		void SConstraint::SetLinearMotorVelocity(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setTargetLinMotorVelocity(Value);
#endif
		}
		void SConstraint::SetUpperLinearLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setUpperLinLimit(Value);
#endif
		}
		void SConstraint::SetLowerLinearLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setLowerLinLimit(Value);
#endif
		}
		void SConstraint::SetAngularDampingDirection(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingDirAng(Value);
#endif
		}
		void SConstraint::SetLinearDampingDirection(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingDirLin(Value);
#endif
		}
		void SConstraint::SetAngularDampingLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingLimAng(Value);
#endif
		}
		void SConstraint::SetLinearDampingLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingLimLin(Value);
#endif
		}
		void SConstraint::SetAngularDampingOrtho(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingOrthoAng(Value);
#endif
		}
		void SConstraint::SetLinearDampingOrtho(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setDampingOrthoLin(Value);
#endif
		}
		void SConstraint::SetUpperAngularLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setUpperAngLimit(Value);
#endif
		}
		void SConstraint::SetLowerAngularLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setLowerAngLimit(Value);
#endif
		}
		void SConstraint::SetMaxAngularMotorForce(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setMaxAngMotorForce(Value);
#endif
		}
		void SConstraint::SetMaxLinearMotorForce(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setMaxLinMotorForce(Value);
#endif
		}
		void SConstraint::SetAngularRestitutionDirection(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionDirAng(Value);
#endif
		}
		void SConstraint::SetLinearRestitutionDirection(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionDirLin(Value);
#endif
		}
		void SConstraint::SetAngularRestitutionLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionLimAng(Value);
#endif
		}
		void SConstraint::SetLinearRestitutionLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionLimLin(Value);
#endif
		}
		void SConstraint::SetAngularRestitutionOrtho(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionOrthoAng(Value);
#endif
		}
		void SConstraint::SetLinearRestitutionOrtho(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setRestitutionOrthoLin(Value);
#endif
		}
		void SConstraint::SetAngularSoftnessDirection(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessDirAng(Value);
#endif
		}
		void SConstraint::SetLinearSoftnessDirection(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessDirLin(Value);
#endif
		}
		void SConstraint::SetAngularSoftnessLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessLimAng(Value);
#endif
		}
		void SConstraint::SetLinearSoftnessLimit(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessLimLin(Value);
#endif
		}
		void SConstraint::SetAngularSoftnessOrtho(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessOrthoAng(Value);
#endif
		}
		void SConstraint::SetLinearSoftnessOrtho(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setSoftnessOrthoLin(Value);
#endif
		}
		void SConstraint::SetPoweredAngularMotor(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setPoweredAngMotor(Value);
#endif
		}
		void SConstraint::SetPoweredLinearMotor(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "slider constraint should be initialized");
			Instance->setPoweredLinMotor(Value);
#endif
		}
		float SConstraint::GetAngularMotorVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getTargetAngMotorVelocity();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearMotorVelocity()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getTargetLinMotorVelocity();
#else
			return 0;
#endif
		}
		float SConstraint::GetUpperLinearLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getUpperLinLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetLowerLinearLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getLowerLinLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularDampingDirection()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getDampingDirAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearDampingDirection()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getDampingDirLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularDampingLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getDampingLimAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearDampingLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getDampingLimLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularDampingOrtho()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getDampingOrthoAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearDampingOrtho()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getDampingOrthoLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetUpperAngularLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getUpperAngLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetLowerAngularLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getLowerAngLimit();
#else
			return 0;
#endif
		}
		float SConstraint::GetMaxAngularMotorForce()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getMaxAngMotorForce();
#else
			return 0;
#endif
		}
		float SConstraint::GetMaxLinearMotorForce()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getMaxLinMotorForce();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularRestitutionDirection()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getRestitutionDirAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearRestitutionDirection()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getRestitutionDirLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularRestitutionLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getRestitutionLimAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearRestitutionLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getRestitutionLimLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularRestitutionOrtho()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getRestitutionOrthoAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearRestitutionOrtho()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getRestitutionOrthoLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularSoftnessDirection()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getSoftnessDirAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearSoftnessDirection()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getSoftnessDirLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularSoftnessLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getSoftnessLimAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearSoftnessLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getSoftnessLimLin();
#else
			return 0;
#endif
		}
		float SConstraint::GetAngularSoftnessOrtho()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getSoftnessOrthoAng();
#else
			return 0;
#endif
		}
		float SConstraint::GetLinearSoftnessOrtho()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "slider constraint should be initialized");
			return Instance->getSoftnessOrthoLin();
#else
			return 0;
#endif
		}
		bool SConstraint::GetPoweredAngularMotor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "slider constraint should be initialized");
			return Instance->getPoweredAngMotor();
#else
			return false;
#endif
		}
		bool SConstraint::GetPoweredLinearMotor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, false, "slider constraint should be initialized");
			return Instance->getPoweredLinMotor();
#else
			return false;
#endif
		}
		SConstraint::Desc& SConstraint::GetState()
		{
			return State;
		}

		CTConstraint::CTConstraint(Simulator* Refer, const Desc& I) : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(I.TargetA != nullptr, "target A rigidbody should be set");
			TH_ASSERT_V(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = TH_NEW(btConeTwistConstraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity());
			else
				Instance = TH_NEW(btConeTwistConstraint, *First, btTransform::getIdentity());

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		CTConstraint::~CTConstraint()
		{
#ifdef TH_WITH_BULLET3
			Engine->RemoveConstraint(this);
			TH_DELETE(btConeTwistConstraint, Instance);
#endif
		}
		Constraint* CTConstraint::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "cone-twist constraint should be initialized");
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
		btTypedConstraint* CTConstraint::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool CTConstraint::HasCollisions()
		{
			return State.Collisions;
		}
		void CTConstraint::EnableMotor(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->enableMotor(Value);
#endif
		}
		void CTConstraint::SetFrames(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setFrames(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void CTConstraint::SetAngularOnly(bool Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setAngularOnly(Value);
#endif
		}
		void CTConstraint::SetLimit(int LimitIndex, float LimitValue)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setLimit(LimitIndex, LimitValue);
#endif
		}
		void CTConstraint::SetLimit(float SwingSpan1, float SwingSpan2, float TwistSpan, float Softness, float BiasFactor, float RelaxationFactor)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setLimit(SwingSpan1, SwingSpan2, TwistSpan, Softness, BiasFactor, RelaxationFactor);
#endif
		}
		void CTConstraint::SetDamping(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setDamping(Value);
#endif
		}
		void CTConstraint::SetMaxMotorImpulse(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMaxMotorImpulse(Value);
#endif
		}
		void CTConstraint::SetMaxMotorImpulseNormalized(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMaxMotorImpulseNormalized(Value);
#endif
		}
		void CTConstraint::SetFixThresh(float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setFixThresh(Value);
#endif
		}
		void CTConstraint::SetMotorTarget(const Quaternion& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMotorTarget(Q4_TO_BT(Value));
#endif
		}
		void CTConstraint::SetMotorTargetInConstraintSpace(const Quaternion& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "cone-twist constraint should be initialized");
			Instance->setMotorTargetInConstraintSpace(Q4_TO_BT(Value));
#endif
		}
		Vector3 CTConstraint::GetPointForAngle(float AngleInRadians, float Length)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "cone-twist constraint should be initialized");
			btVector3 Value = Instance->GetPointForAngle(AngleInRadians, Length);
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		Quaternion CTConstraint::GetMotorTarget()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, Quaternion(), "hinge constraint should be initialized");
			btQuaternion Value = Instance->getMotorTarget();
			return BT_TO_Q4(Value);
#else
			return Quaternion();
#endif
		}
		int CTConstraint::GetSolveTwistLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getSolveTwistLimit();
#else
			return 0;
#endif
		}
		int CTConstraint::GetSolveSwingLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getSolveSwingLimit();
#else
			return 0;
#endif
		}
		float CTConstraint::GetTwistLimitSign()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getTwistLimitSign();
#else
			return 0;
#endif
		}
		float CTConstraint::GetSwingSpan1()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getSwingSpan1();
#else
			return 0;
#endif
		}
		float CTConstraint::GetSwingSpan2()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getSwingSpan2();
#else
			return 0;
#endif
		}
		float CTConstraint::GetTwistSpan()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getTwistSpan();
#else
			return 0;
#endif
		}
		float CTConstraint::GetLimitSoftness()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLimitSoftness();
#else
			return 0;
#endif
		}
		float CTConstraint::GetBiasFactor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getBiasFactor();
#else
			return 0;
#endif
		}
		float CTConstraint::GetRelaxationFactor()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getRelaxationFactor();
#else
			return 0;
#endif
		}
		float CTConstraint::GetTwistAngle()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getTwistAngle();
#else
			return 0;
#endif
		}
		float CTConstraint::GetLimit(int Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getLimit(Value);
#else
			return 0;
#endif
		}
		float CTConstraint::GetDamping()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getDamping();
#else
			return 0;
#endif
		}
		float CTConstraint::GetMaxMotorImpulse()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getMaxMotorImpulse();
#else
			return 0;
#endif
		}
		float CTConstraint::GetFixThresh()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getFixThresh();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsMotorEnabled()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->isMotorEnabled();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsMaxMotorImpulseNormalized()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->isMaxMotorImpulseNormalized();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsPastSwingLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->isPastSwingLimit();
#else
			return 0;
#endif
		}
		bool CTConstraint::IsAngularOnly()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "hinge constraint should be initialized");
			return Instance->getAngularOnly();
#else
			return 0;
#endif
		}
		CTConstraint::Desc& CTConstraint::GetState()
		{
			return State;
		}

		DF6Constraint::DF6Constraint(Simulator* Refer, const Desc& I) : Constraint(Refer), Instance(nullptr), State(I)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(I.TargetA != nullptr, "target A rigidbody should be set");
			TH_ASSERT_V(Engine != nullptr, "simulator should be set");

			First = I.TargetA->Get();
			Second = (I.TargetB ? I.TargetB->Get() : nullptr);

			if (Second != nullptr)
				Instance = TH_NEW(btGeneric6DofSpring2Constraint, *First, *Second, btTransform::getIdentity(), btTransform::getIdentity());
			else
				Instance = TH_NEW(btGeneric6DofSpring2Constraint, *First, btTransform::getIdentity());

			Instance->setUserConstraintPtr(this);
			Engine->AddConstraint(this);
#endif
		}
		DF6Constraint::~DF6Constraint()
		{
#ifdef TH_WITH_BULLET3
			Engine->RemoveConstraint(this);
			TH_DELETE(btGeneric6DofSpring2Constraint, Instance);
#endif
		}
		Constraint* DF6Constraint::Copy()
		{
			TH_ASSERT(Instance != nullptr, nullptr, "6-dof constraint should be initialized");
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
		btTypedConstraint* DF6Constraint::Get()
		{
#ifdef TH_WITH_BULLET3
			return Instance;
#else
			return nullptr;
#endif
		}
		bool DF6Constraint::HasCollisions()
		{
			return State.Collisions;
		}
		void DF6Constraint::EnableMotor(int Index, bool OnOff)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->enableMotor(Index, OnOff);
#endif
		}
		void DF6Constraint::EnableSpring(int Index, bool OnOff)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->enableSpring(Index, OnOff);
#endif
		}
		void DF6Constraint::SetFrames(const Matrix4x4& A, const Matrix4x4& B)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setFrames(M16_TO_BT(A), M16_TO_BT(B));
#endif
		}
		void DF6Constraint::SetLinearLowerLimit(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLinearLowerLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetLinearUpperLimit(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLinearUpperLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularLowerLimit(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularLowerLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularLowerLimitReversed(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularLowerLimitReversed(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularUpperLimit(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularUpperLimit(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetAngularUpperLimitReversed(const Vector3& Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAngularUpperLimitReversed(V3_TO_BT(Value));
#endif
		}
		void DF6Constraint::SetLimit(int Axis, float Low, float High)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLimit(Axis, Low, High);
#endif
		}
		void DF6Constraint::SetLimitReversed(int Axis, float Low, float High)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setLimitReversed(Axis, Low, High);
#endif
		}
		void DF6Constraint::SetRotationOrder(Rotator Order)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setRotationOrder((RotateOrder)Order);
#endif
		}
		void DF6Constraint::SetAxis(const Vector3& A, const Vector3& B)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setAxis(V3_TO_BT(A), V3_TO_BT(B));
#endif
		}
		void DF6Constraint::SetBounce(int Index, float Bounce)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setBounce(Index, Bounce);
#endif
		}
		void DF6Constraint::SetServo(int Index, bool OnOff)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setServo(Index, OnOff);
#endif
		}
		void DF6Constraint::SetTargetVelocity(int Index, float Velocity)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setTargetVelocity(Index, Velocity);
#endif
		}
		void DF6Constraint::SetServoTarget(int Index, float Target)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setServoTarget(Index, Target);
#endif
		}
		void DF6Constraint::SetMaxMotorForce(int Index, float Force)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setMaxMotorForce(Index, Force);
#endif
		}
		void DF6Constraint::SetStiffness(int Index, float Stiffness, bool LimitIfNeeded)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setStiffness(Index, Stiffness, LimitIfNeeded);
#endif
		}
		void DF6Constraint::SetEquilibriumPoint()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setEquilibriumPoint();
#endif
		}
		void DF6Constraint::SetEquilibriumPoint(int Index)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setEquilibriumPoint(Index);
#endif
		}
		void DF6Constraint::SetEquilibriumPoint(int Index, float Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Instance != nullptr, "6-dof constraint should be initialized");
			Instance->setEquilibriumPoint(Index, Value);
#endif
		}
		Vector3 DF6Constraint::GetAngularUpperLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularUpperLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAngularUpperLimitReversed()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularUpperLimitReversed(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAngularLowerLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularLowerLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAngularLowerLimitReversed()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getAngularLowerLimitReversed(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetLinearUpperLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getLinearUpperLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetLinearLowerLimit()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result;
			Instance->getLinearLowerLimit(Result);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Vector3 DF6Constraint::GetAxis(int Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			btVector3 Result = Instance->getAxis(Value);
			return BT_TO_V3(Result);
#else
			return 0;
#endif
		}
		Rotator DF6Constraint::GetRotationOrder()
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, Rotator::XYZ, "6-dof constraint should be initialized");
			return (Rotator)Instance->getRotationOrder();
#else
			return Rotator::XYZ;
#endif
		}
		float DF6Constraint::GetAngle(int Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			return Instance->getAngle(Value);
#else
			return 0;
#endif
		}
		float DF6Constraint::GetRelativePivotPosition(int Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			return Instance->getRelativePivotPosition(Value);
#else
			return 0;
#endif
		}
		bool DF6Constraint::IsLimited(int LimitIndex)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Instance != nullptr, 0, "6-dof constraint should be initialized");
			return Instance->isLimited(LimitIndex);
#else
			return 0;
#endif
		}
		DF6Constraint::Desc& DF6Constraint::GetState()
		{
			return State;
		}

		Simulator::Simulator(const Desc& I) : SoftSolver(nullptr), TimeSpeed(1), Interpolate(1), Active(true)
		{
#ifdef TH_WITH_BULLET3
			Broadphase = TH_NEW(btDbvtBroadphase);
			Solver = TH_NEW(btSequentialImpulseConstraintSolver);

			if (I.EnableSoftBody)
			{
				SoftSolver = TH_NEW(btDefaultSoftBodySolver);
				Collision = TH_NEW(btSoftBodyRigidBodyCollisionConfiguration);
				Dispatcher = TH_NEW(btCollisionDispatcher, Collision);
				World = TH_NEW(btSoftRigidDynamicsWorld, Dispatcher, Broadphase, Solver, Collision, SoftSolver);

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
				Collision = TH_NEW(btDefaultCollisionConfiguration);
				Dispatcher = TH_NEW(btCollisionDispatcher, Collision);
				World = TH_NEW(btDiscreteDynamicsWorld, Dispatcher, Broadphase, Solver, Collision);
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
		Simulator::~Simulator()
		{
#ifdef TH_WITH_BULLET3
			RemoveAll();
			for (auto It = Shapes.begin(); It != Shapes.end(); ++It)
			{
				btCollisionShape* Item = (btCollisionShape*)It->first;
				TH_DELETE(btCollisionShape, Item);
			}

			TH_DELETE(btCollisionDispatcher, Dispatcher);
			TH_DELETE(btCollisionConfiguration, Collision);
			TH_DELETE(btConstraintSolver, Solver);
			TH_DELETE(btBroadphaseInterface, Broadphase);
			TH_DELETE(btSoftBodySolver, SoftSolver);
			TH_DELETE(btDiscreteDynamicsWorld, World);
#endif
		}
		void Simulator::SetGravity(const Vector3& Gravity)
		{
#ifdef TH_WITH_BULLET3
			World->setGravity(V3_TO_BT(Gravity));
#endif
		}
		void Simulator::SetLinearImpulse(const Vector3& Impulse, bool RandomFactor)
		{
#ifdef TH_WITH_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(V3_TO_BT(Velocity));
			}
#endif
		}
		void Simulator::SetLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(V3_TO_BT(Velocity));
			}
#endif
		}
		void Simulator::SetAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
			gContactStartedCallback = Callback;
#endif
		}
		void Simulator::SetOnCollisionExit(ContactEndedCallback Callback)
		{
#ifdef TH_WITH_BULLET3
			gContactEndedCallback = Callback;
#endif
		}
		void Simulator::CreateLinearImpulse(const Vector3& Impulse, bool RandomFactor)
		{
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Body != nullptr, "softbody should be set");
			TH_ASSERT_V(Body->Instance != nullptr, "softbody instance should be set");
			TH_ASSERT_V(Body->Instance->getWorldArrayIndex() == -1, "softbody should not be attached to other world");
			TH_ASSERT_V(HasSoftBodySupport(), "softbodies should be supported");

			btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
			SoftWorld->addSoftBody(Body->Instance);
#endif
		}
		void Simulator::RemoveSoftBody(SoftBody* Body)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Body != nullptr, "softbody should be set");
			TH_ASSERT_V(Body->Instance != nullptr, "softbody instance should be set");
			TH_ASSERT_V(Body->Instance->getWorldArrayIndex() >= 0, "softbody should be attached to world");
			TH_ASSERT_V(HasSoftBodySupport(), "softbodies should be supported");

			btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
			SoftWorld->removeSoftBody(Body->Instance);
#endif
		}
		void Simulator::AddRigidBody(RigidBody* Body)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Body != nullptr, "rigidbody should be set");
			TH_ASSERT_V(Body->Instance != nullptr, "rigidbody instance should be set");
			TH_ASSERT_V(Body->Instance->getWorldArrayIndex() == -1, "rigidbody should not be attached to other world");
			World->addRigidBody(Body->Instance);
#endif
		}
		void Simulator::RemoveRigidBody(RigidBody* Body)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Body != nullptr, "rigidbody should be set");
			TH_ASSERT_V(Body->Instance != nullptr, "rigidbody instance should be set");
			TH_ASSERT_V(Body->Instance->getWorldArrayIndex() >= 0, "rigidbody should be attached to other world");
			World->removeRigidBody(Body->Instance);
#endif
		}
		void Simulator::AddConstraint(Constraint* Constraint)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Constraint != nullptr, "slider constraint should be set");
			TH_ASSERT_V(Constraint->Get() != nullptr, "slider constraint instance should be set");
			World->addConstraint(Constraint->Get(), !Constraint->HasCollisions());
#endif
		}
		void Simulator::RemoveConstraint(Constraint* Constraint)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Constraint != nullptr, "slider constraint should be set");
			TH_ASSERT_V(Constraint->Get() != nullptr, "slider constraint instance should be set");
			World->removeConstraint(Constraint->Get());
#endif
		}
		void Simulator::RemoveAll()
		{
#ifdef TH_WITH_BULLET3
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btCollisionObject* Object = World->getCollisionObjectArray()[i];
				btRigidBody* Body = btRigidBody::upcast(Object);
				if (Body != nullptr)
				{
					auto* State = Body->getMotionState();
					TH_DELETE(btMotionState, State);
					Body->setMotionState(nullptr);

					auto* Shape = Body->getCollisionShape();
					TH_DELETE(btCollisionShape, Shape);
					Body->setCollisionShape(nullptr);
				}

				World->removeCollisionObject(Object);
				TH_DELETE(btCollisionObject, Object);
			}
#endif
		}
		void Simulator::Simulate(float TimeStep)
		{
#ifdef TH_WITH_BULLET3
			if (!Active || TimeSpeed <= 0.0f)
				return;

			TH_PPUSH("simulator-step", TH_PERF_MIX);
			World->stepSimulation(TimeStep * TimeSpeed, Interpolate, TimeSpeed / 60.0f);
			TH_PPOP();
#endif
		}
		void Simulator::FindContacts(RigidBody* Body, int(*Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&))
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT_V(Callback != nullptr, "callback should not be empty");
			TH_ASSERT_V(Body != nullptr, "body should be set");
			TH_PPUSH("simulator-ctest", TH_PERF_CORE);

			FindContactsHandler Handler;
			Handler.Callback = Callback;
			World->contactTest(Body->Get(), Handler);
			TH_PPOP();
#endif
		}
		bool Simulator::FindRayContacts(const Vector3& Start, const Vector3& End, int(*Callback)(RayContact*, const CollisionBody&))
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Callback != nullptr, false, "callback should not be empty");
			TH_PPUSH("simulator-ray-test", TH_PERF_CORE);

			FindRayContactsHandler Handler;
			Handler.Callback = Callback;

			World->rayTest(btVector3(Start.X, Start.Y, Start.Z), btVector3(End.X, End.Y, End.Z), Handler);
			TH_PPOP();

			return Handler.m_collisionObject != nullptr;
#else
			return false;
#endif
		}
		RigidBody* Simulator::CreateRigidBody(const RigidBody::Desc& I, Transform* Transform)
		{
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
			return new RigidBody(this, I);
#else
			return nullptr;
#endif
		}
		SoftBody* Simulator::CreateSoftBody(const SoftBody::Desc& I, Transform* Transform)
		{
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
			if (!HasSoftBodySupport())
				return nullptr;

			return new SoftBody(this, I);
#else
			return nullptr;
#endif
		}
		PConstraint* Simulator::CreatePoint2PointConstraint(const PConstraint::Desc& I)
		{
#ifdef TH_WITH_BULLET3
			return new PConstraint(this, I);
#else
			return nullptr;
#endif
		}
		HConstraint* Simulator::CreateHingeConstraint(const HConstraint::Desc& I)
		{
#ifdef TH_WITH_BULLET3
			return new HConstraint(this, I);
#else
			return nullptr;
#endif
		}
		SConstraint* Simulator::CreateSliderConstraint(const SConstraint::Desc& I)
		{
#ifdef TH_WITH_BULLET3
			return new SConstraint(this, I);
#else
			return nullptr;
#endif
		}
		CTConstraint* Simulator::CreateConeTwistConstraint(const CTConstraint::Desc& I)
		{
#ifdef TH_WITH_BULLET3
			return new CTConstraint(this, I);
#else
			return nullptr;
#endif
		}
		DF6Constraint* Simulator::Create6DoFConstraint(const DF6Constraint::Desc& I)
		{
#ifdef TH_WITH_BULLET3
			return new DF6Constraint(this, I);
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCube(const Vector3& Scale)
		{
#ifdef TH_WITH_BULLET3
			btCollisionShape* Shape = TH_NEW(btBoxShape, V3_TO_BT(Scale));
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateSphere(float Radius)
		{
#ifdef TH_WITH_BULLET3
			btCollisionShape* Shape = TH_NEW(btSphereShape, Radius);
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCapsule(float Radius, float Height)
		{
#ifdef TH_WITH_BULLET3
			btCollisionShape* Shape = TH_NEW(btCapsuleShape, Radius, Height);
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCone(float Radius, float Height)
		{
#ifdef TH_WITH_BULLET3
			btCollisionShape* Shape = TH_NEW(btConeShape, Radius, Height);
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateCylinder(const Vector3& Scale)
		{
#ifdef TH_WITH_BULLET3
			btCollisionShape* Shape = TH_NEW(btCylinderShape, V3_TO_BT(Scale));
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<SkinVertex>& Vertices)
		{
#ifdef TH_WITH_BULLET3
			btConvexHullShape* Shape = TH_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vertex>& Vertices)
		{
#ifdef TH_WITH_BULLET3
			btConvexHullShape* Shape = TH_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vector2>& Vertices)
		{
#ifdef TH_WITH_BULLET3
			btConvexHullShape* Shape = TH_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->X, It->Y, 0), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vector3>& Vertices)
		{
#ifdef TH_WITH_BULLET3
			btConvexHullShape* Shape = TH_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->X, It->Y, It->Z), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vector4>& Vertices)
		{
#ifdef TH_WITH_BULLET3
			btConvexHullShape* Shape = TH_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->X, It->Y, It->Z), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateConvexHull(btCollisionShape* From)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(From != nullptr, nullptr, "shape should be set");
			TH_ASSERT(From->getShapeType() == (int)Shape::Convex_Hull, nullptr, "shape type should be convex hull");

			btConvexHullShape* Hull = TH_NEW(btConvexHullShape);
			btConvexHullShape* Base = (btConvexHullShape*)From;

			for (size_t i = 0; i < Base->getNumPoints(); i++)
				Hull->addPoint(*(Base->getUnscaledPoints() + i), false);

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);

			Safe.lock();
			Shapes[Hull] = 1;
			Safe.unlock();

			return Hull;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateShape(Shape Wanted)
		{
#ifdef TH_WITH_BULLET3
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Value != nullptr, nullptr, "shape should be set");
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
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Value != nullptr, nullptr, "shape should be set");
			Safe.lock();
			auto It = Shapes.find(Value);
			if (It == Shapes.end())
			{
				Safe.unlock();
				return nullptr;
			}

			It->second++;
			Safe.unlock();
			return Value;
#else
			return nullptr;
#endif
		}
		void Simulator::FreeShape(btCollisionShape** Value)
		{
#ifdef TH_WITH_BULLET3
			if (!Value || !*Value)
				return;

			Safe.lock();
			auto It = Shapes.find(*Value);
			if (It != Shapes.end())
			{
				*Value = nullptr;
				if (It->second-- <= 1)
				{
					btCollisionShape* Item = (btCollisionShape*)It->first;
					TH_DELETE(btCollisionShape, Item);
					Shapes.erase(It);
				}
			}
			Safe.unlock();
#endif
		}
		std::vector<Vector3> Simulator::GetShapeVertices(btCollisionShape* Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Value != nullptr, std::vector<Vector3>(), "shape should be set");
			auto Type = (Shape)Value->getShapeType();
			if (Type != Shape::Convex_Hull)
				return std::vector<Vector3>();

			btConvexHullShape* Hull = (btConvexHullShape*)Value;
			btVector3* Points = Hull->getUnscaledPoints();
			size_t Count = Hull->getNumPoints();
			std::vector<Vector3> Vertices;
			Vertices.reserve(Count);

			for (size_t i = 0; i < Count; i++)
			{
				btVector3& It = Points[i];
				Vertices.emplace_back(It.getX(), It.getY(), It.getZ());
			}

			return Vertices;
#else
			return std::vector<Vector3>();
#endif
		}
		uint64_t Simulator::GetShapeVerticesCount(btCollisionShape* Value)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(Value != nullptr, 0, "shape should be set");
			auto Type = (Compute::Shape)Value->getShapeType();
			if (Type != Shape::Convex_Hull)
				return 0;

			btConvexHullShape* Hull = (btConvexHullShape*)Value;
			return Hull->getNumPoints();
#else
			return 0;
#endif
		}
		float Simulator::GetMaxDisplacement()
		{
#ifdef TH_WITH_BULLET3
			if (!SoftSolver || !World)
				return 1000;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().m_maxDisplacement;
#else
			return 1000;
#endif
		}
		float Simulator::GetAirDensity()
		{
#ifdef TH_WITH_BULLET3
			if (!SoftSolver || !World)
				return 1.2f;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().air_density;
#else
			return 1.2f;
#endif
		}
		float Simulator::GetWaterOffset()
		{
#ifdef TH_WITH_BULLET3
			if (!SoftSolver || !World)
				return 0;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_offset;
#else
			return 0;
#endif
		}
		float Simulator::GetWaterDensity()
		{
#ifdef TH_WITH_BULLET3
			if (!SoftSolver || !World)
				return 0;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_density;
#else
			return 0;
#endif
		}
		Vector3 Simulator::GetWaterNormal()
		{
#ifdef TH_WITH_BULLET3
			if (!SoftSolver || !World)
				return 0;

			btVector3 Value = ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_normal;
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		Vector3 Simulator::GetGravity()
		{
#ifdef TH_WITH_BULLET3
			if (!World)
				return 0;

			btVector3 Value = World->getGravity();
			return BT_TO_V3(Value);
#else
			return 0;
#endif
		}
		ContactStartedCallback Simulator::GetOnCollisionEnter()
		{
#ifdef TH_WITH_BULLET3
			return gContactStartedCallback;
#else
			return nullptr;
#endif
		}
		ContactEndedCallback Simulator::GetOnCollisionExit()
		{
#ifdef TH_WITH_BULLET3
			return gContactEndedCallback;
#else
			return nullptr;
#endif
		}
		btCollisionConfiguration* Simulator::GetCollision()
		{
#ifdef TH_WITH_BULLET3
			return Collision;
#else
			return nullptr;
#endif
		}
		btBroadphaseInterface* Simulator::GetBroadphase()
		{
#ifdef TH_WITH_BULLET3
			return Broadphase;
#else
			return nullptr;
#endif
		}
		btConstraintSolver* Simulator::GetSolver()
		{
#ifdef TH_WITH_BULLET3
			return Solver;
#else
			return nullptr;
#endif
		}
		btDiscreteDynamicsWorld* Simulator::GetWorld()
		{
#ifdef TH_WITH_BULLET3
			return World;
#else
			return nullptr;
#endif
		}
		btCollisionDispatcher* Simulator::GetDispatcher()
		{
#ifdef TH_WITH_BULLET3
			return Dispatcher;
#else
			return nullptr;
#endif
		}
		btSoftBodySolver* Simulator::GetSoftSolver()
		{
#ifdef TH_WITH_BULLET3
			return SoftSolver;
#else
			return nullptr;
#endif
		}
		bool Simulator::HasSoftBodySupport()
		{
#ifdef TH_WITH_BULLET3
			return SoftSolver != nullptr;
#else
			return false;
#endif
		}
		int Simulator::GetContactManifoldCount()
		{
#ifdef TH_WITH_BULLET3
			if (!Dispatcher)
				return 0;

			return Dispatcher->getNumManifolds();
#else
			return 0;
#endif
		}
		void Simulator::FreeHullShape(btCollisionShape* Shape)
		{
#ifdef TH_WITH_BULLET3
			TH_DELETE(btCollisionShape, Shape);
#endif
		}
		Simulator* Simulator::Get(btDiscreteDynamicsWorld* From)
		{
#ifdef TH_WITH_BULLET3
			if (!From)
				return nullptr;

			return (Simulator*)From->getWorldUserInfo();
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateHullShape(std::vector<Vertex>& Vertices)
		{
#ifdef TH_WITH_BULLET3
			btConvexHullShape* Shape = TH_NEW(btConvexHullShape);
			for (auto It = Vertices.begin(); It != Vertices.end(); ++It)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			return Shape;
#else
			return nullptr;
#endif
		}
		btCollisionShape* Simulator::CreateHullShape(btCollisionShape* From)
		{
#ifdef TH_WITH_BULLET3
			TH_ASSERT(From != nullptr, nullptr, "shape should be set");
			TH_ASSERT(From->getShapeType() == (int)Shape::Convex_Hull, nullptr, "shape type should be convex hull");

			btConvexHullShape* Hull = TH_NEW(btConvexHullShape);
			btConvexHullShape* Base = (btConvexHullShape*)From;

			for (size_t i = 0; i < Base->getNumPoints(); i++)
				Hull->addPoint(*(Base->getUnscaledPoints() + i), false);

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);

			return Hull;
#else
			return nullptr;
#endif
		}
	}
}