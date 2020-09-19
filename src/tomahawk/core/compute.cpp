#include "compute.h"
#include <cctype>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#ifdef THAWK_HAS_OPENSSL
extern "C"
{
#include <openssl/evp.h>
#include <openssl/rand.h>
}
#endif
#define BtV3(V) btVector3(V.X, V.Y, V.Z)
#define V3Bt(V) Vector3(V.getX(), V.getY(), V.getZ())
#define REGEX_FAIL(A, B) if (A) return (B)

namespace Tomahawk
{
	namespace Compute
	{
		class FindContactsHandler : public btCollisionWorld::ContactResultCallback
		{
		public:
			int (* Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&) = nullptr;

		public:
			btScalar addSingleResult(btManifoldPoint& Point, const btCollisionObjectWrapper* Object1, int PartId0, int Index0, const btCollisionObjectWrapper* Object2, int PartId1, int Index1)
			{
				if (!Callback || !Object1 || !Object1->getCollisionObject() || !Object2 || !Object2->getCollisionObject())
					return 0;

				ShapeContact Contact;
				Contact.LocalPoint1 = Vector3(Point.m_localPointA.getX(), Point.m_localPointA.getY(), Point.m_localPointA.getZ());
				Contact.LocalPoint2 = Vector3(Point.m_localPointB.getX(), Point.m_localPointB.getY(), Point.m_localPointB.getZ());
				Contact.PositionWorld1 = Vector3(Point.getPositionWorldOnA().getX(), Point.getPositionWorldOnA().getY(), Point.getPositionWorldOnA().getZ());
				Contact.PositionWorld2 = Vector3(Point.getPositionWorldOnB().getX(), Point.getPositionWorldOnB().getY(), Point.getPositionWorldOnB().getZ());
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
			int (* Callback)(RayContact*, const CollisionBody&) = nullptr;

		public:
			btScalar addSingleResult(btCollisionWorld::LocalRayResult& RayResult, bool NormalInWorldSpace)
			{
				if (!Callback || !RayResult.m_collisionObject)
					return 0;

				RayContact Contact;
				Contact.HitNormalLocal = V3Bt(RayResult.m_hitNormalLocal);
				Contact.NormalInWorldSpace = NormalInWorldSpace;
				Contact.HitFraction = RayResult.m_hitFraction;
				Contact.ClosestHitFraction = m_closestHitFraction;

				btCollisionObject* Body1 = (btCollisionObject*)RayResult.m_collisionObject;
				return (btScalar)Callback(&Contact, CollisionBody(Body1));
			}
		};

		Vector2::Vector2()
		{
			X = 0;
			Y = 0;
		}
		Vector2::Vector2(float x, float y)
		{
			X = x;
			Y = y;
		}
		Vector2::Vector2(float xy)
		{
			X = xy;
			Y = xy;
		}
		Vector2::Vector2(const Vector3& Value)
		{
			X = Value.X;
			Y = Value.Y;
		}
		Vector2::Vector2(const Vector4& Value)
		{
			X = Value.X;
			Y = Value.Y;
		}
		float Vector2::Hypotenuse() const
		{
			return Mathf::Sqrt(X * X + Y * Y);
		}
		float Vector2::LookAt(const Vector2& At) const
		{
			return Mathf::Atan2(At.X - X, At.Y - Y);
		}
		float Vector2::Distance(const Vector2& Point) const
		{
			float x = X - Point.X;
			float y = Y - Point.Y;

			return Mathf::Sqrt(x * x + y * y);
		}
		float Vector2::Length() const
		{
			return Mathf::Sqrt(X * X + Y * Y);
		}
		float Vector2::ModLength() const
		{
			return X + Y;
		}
		float Vector2::DotProduct(const Vector2& B) const
		{
			return X * B.X + Y * B.Y;
		}
		float Vector2::Cross(const Vector2& Vector1) const
		{
			return X * Vector1.Y - Y * Vector1.X;
		}
		Vector2 Vector2::Direction(float Rotation) const
		{
			return Vector2(cos(-Rotation), sin(-Rotation));
		}
		Vector2 Vector2::SaturateRotation() const
		{
			float Ax = Mathf::SaturateAngle(X);
			float Ay = Mathf::SaturateAngle(Y);

			return Vector2(Ax, Ay);
		}
		Vector2 Vector2::Invert() const
		{
			return Vector2(-X, -Y);
		}
		Vector2 Vector2::InvertX() const
		{
			return Vector2(-X, Y);
		}
		Vector2 Vector2::InvertY() const
		{
			return Vector2(X, -Y);
		}
		Vector2 Vector2::Normalize() const
		{
			float Factor = 1.0f / Length();

			return Vector2(X * Factor, Y * Factor);
		}
		Vector2 Vector2::NormalizeSafe() const
		{
			float LengthZero = Length();
			float Factor = 1.0f / LengthZero;

			if (LengthZero == 0.0f)
				Factor = 0.0f;

			return Vector2(X * Factor, Y * Factor);
		}
		Vector2 Vector2::SetX(float Xf) const
		{
			return Vector2(Xf, Y);
		}
		Vector2 Vector2::SetY(float Yf) const
		{
			return Vector2(X, Yf);
		}
		Vector2 Vector2::Mul(float x, float y) const
		{
			return Vector2(X * x, Y * y);
		}
		Vector2 Vector2::Mul(const Vector2& Value) const
		{
			return *this * Value;
		}
		Vector2 Vector2::Add(const Vector2& Value) const
		{
			return *this + Value;
		}
		Vector2 Vector2::Div(const Vector2& Value) const
		{
			return *this / Value;
		}
		Vector2 Vector2::Lerp(const Vector2& B, float DeltaTime) const
		{
			return *this + (B - *this) * DeltaTime;
		}
		Vector2 Vector2::SphericalLerp(const Vector2& B, float DeltaTime) const
		{
			return Quaternion(Vector3()).SphericalLerp(B.XYZ(), DeltaTime).GetEuler().XY();
		}
		Vector2 Vector2::AngularLerp(const Vector2& B, float DeltaTime) const
		{
			float Ax = Mathf::AngluarLerp(X, B.X, DeltaTime);
			float Ay = Mathf::AngluarLerp(Y, B.Y, DeltaTime);

			return Vector2(Ax, Ay);
		}
		Vector2 Vector2::Abs() const
		{
			return Vector2(X < 0 ? -X : X, Y < 0 ? -Y : Y);
		}
		Vector2 Vector2::Transform(const Matrix4x4& Matrix) const
		{
			return Vector2(X * Matrix.Row[0] + Y * Matrix.Row[4], X * Matrix.Row[1] + Y * Matrix.Row[5]);
		}
		void Vector2::Set(const Vector2& Value)
		{
			X = Value.X;
			Y = Value.Y;
		}
		void Vector2::Get2(float* In) const
		{
			In[0] = X;
			In[1] = Y;
		}
		Vector2& Vector2::operator -=(const Vector2& V)
		{
			X -= V.X;
			Y -= V.Y;
			return *this;
		}
		Vector2& Vector2::operator -=(float V)
		{
			X -= V;
			Y -= V;
			return *this;
		}
		Vector2& Vector2::operator *=(const Vector2& V)
		{
			X *= V.X;
			Y *= V.Y;
			return *this;
		}
		Vector2& Vector2::operator *=(float V)
		{
			X *= V;
			Y *= V;
			return *this;
		}
		Vector2& Vector2::operator /=(const Vector2& V)
		{
			X /= V.X;
			Y /= V.Y;
			return *this;
		}
		Vector2& Vector2::operator /=(float V)
		{
			X /= V;
			Y /= V;
			return *this;
		}
		Vector2& Vector2::operator +=(const Vector2& V)
		{
			X += V.X;
			Y += V.Y;
			return *this;
		}
		Vector2& Vector2::operator +=(float V)
		{
			X += V;
			Y += V;
			return *this;
		}
		Vector2 Vector2::operator *(const Vector2& V) const
		{
			return Vector2(X * V.X, Y * V.Y);
		}
		Vector2 Vector2::operator *(float V) const
		{
			return Vector2(X * V, Y * V);
		}
		Vector2 Vector2::operator /(const Vector2& V) const
		{
			return Vector2(X / V.X, Y / V.Y);
		}
		Vector2 Vector2::operator /(float V) const
		{
			return Vector2(X / V, Y / V);
		}
		Vector2 Vector2::operator +(const Vector2& V) const
		{
			return Vector2(X + V.X, Y + V.Y);
		}
		Vector2 Vector2::operator +(float V) const
		{
			return Vector2(X + V, Y + V);
		}
		Vector2 Vector2::operator -(const Vector2& V) const
		{
			return Vector2(X - V.X, Y - V.Y);
		}
		Vector2 Vector2::operator -(float V) const
		{
			return Vector2(X - V, Y - V);
		}
		Vector2 Vector2::operator -() const
		{
			return Invert();
		}
		void Vector2::ToInt32(unsigned int& Value) const
		{
			unsigned char Bytes[4];
			Bytes[0] = static_cast<unsigned char>(X * 255.0f + 0.5f);
			Bytes[1] = static_cast<unsigned char>(Y * 255.0f + 0.5f);
			Bytes[2] = Bytes[3] = 0;

			Value = (unsigned int)(unsigned long long)Bytes;
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
			return X < R.X && Y < R.Y;
		}
		bool Vector2::operator >(const Vector2& R) const
		{
			return X > R.X && Y > R.Y;
		}
		float& Vector2::operator [](int Axis)
		{
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;

			return X;
		}
		float Vector2::operator [](int Axis) const
		{
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;

			return X;
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
		Vector2 Vector2::Random()
		{
			return Vector2(Mathf::RandomMag(), Mathf::RandomMag());
		}
		Vector2 Vector2::RandomAbs()
		{
			return Vector2(Mathf::Random(), Mathf::Random());
		}
		Vector2 Vector2::Radians() const
		{
			return (*this) * Mathf::Deg2Rad();
		}
		Vector2 Vector2::Degrees() const
		{
			return (*this) * Mathf::Rad2Deg();
		}

		Vector3::Vector3()
		{
			X = 0;
			Y = 0;
			Z = 0;
		}
		Vector3::Vector3(float x, float y)
		{
			X = x;
			Y = y;
			Z = 0;
		}
		Vector3::Vector3(float x, float y, float z)
		{
			X = x;
			Y = y;
			Z = z;
		}
		Vector3::Vector3(float xyzw)
		{
			X = Y = Z = xyzw;
		}
		Vector3::Vector3(const Vector2& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = 0;
		}
		Vector3::Vector3(const Vector4& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
		}
		float Vector3::Length() const
		{
			return Mathf::Sqrt(X * X + Y * Y + Z * Z);
		}
		float Vector3::ModLength() const
		{
			return X + Y + Z;
		}
		float Vector3::DotProduct(const Vector3& B) const
		{
			return X * B.X + Y * B.Y + Z * B.Z;
		}
		float Vector3::Distance(const Vector3& Point) const
		{
			float x = X - Point.X;
			float y = Y - Point.Y;
			float z = Z - Point.Z;

			return Mathf::Sqrt(x * x + y * y + z * z);
		}
		float Vector3::Hypotenuse() const
		{
			float R = Mathf::Sqrt(X * X + Z * Z);
			return Mathf::Sqrt(R * R + Y * Y);
		}
		float Vector3::LookAtXY(const Vector3& At) const
		{
			return Mathf::Atan2(At.X - X, At.Y - Y);
		}
		float Vector3::LookAtXZ(const Vector3& At) const
		{
			return Mathf::Atan2(At.X - X, At.Z - Z);
		}
		Vector3 Vector3::Transform(const Matrix4x4& Matrix) const
		{
			return Vector3(X * Matrix.Row[0] + Y * Matrix.Row[4] + Z * Matrix.Row[8], X * Matrix.Row[1] + Y * Matrix.Row[5] + Z * Matrix.Row[9], X * Matrix.Row[2] + Y * Matrix.Row[6] + Z * Matrix.Row[10]);
		}
		Vector3 Vector3::DepthDirection() const
		{
			float CosX = cos(X);
			return Vector3(sin(Y) * CosX, -sin(X), cos(Y) * CosX);
		}
		Vector3 Vector3::HorizontalDirection() const
		{
			return Vector3(-cos(Y), 0, sin(Y));
		}
		Vector3 Vector3::Invert() const
		{
			return Vector3(-X, -Y, -Z);
		}
		Vector3 Vector3::InvertX() const
		{
			return Vector3(-X, Y, Z);
		}
		Vector3 Vector3::InvertY() const
		{
			return Vector3(X, -Y, Z);
		}
		Vector3 Vector3::InvertZ() const
		{
			return Vector3(X, Y, -Z);
		}
		Vector3 Vector3::Cross(const Vector3& Vector1) const
		{
			return Vector3(Y * Vector1.Z - Z * Vector1.Y, Z * Vector1.X - X * Vector1.Z, X * Vector1.Y - Y * Vector1.X);
		}
		Vector3 Vector3::Normalize() const
		{
			float Factor = 1.0f / Length();

			return Vector3(X * Factor, Y * Factor, Z * Factor);
		}
		Vector3 Vector3::NormalizeSafe() const
		{
			float LengthZero = Length();
			float Factor = 1.0f / LengthZero;

			if (LengthZero == 0.0f)
				Factor = 0.0f;

			return Vector3(X * Factor, Y * Factor, Z * Factor);
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
		Vector3 Vector3::Mul(const Vector2& XY, float z) const
		{
			return Vector3(X * XY.X, Y * XY.Y, Z * z);
		}
		Vector3 Vector3::Mul(const Vector3& Value) const
		{
			return *this * Value;
		}
		Vector3 Vector3::Add(const Vector3& Value) const
		{
			return *this + Value;
		}
		Vector3 Vector3::Div(const Vector3& Value) const
		{
			return *this / Value;
		}
		Vector3 Vector3::Lerp(const Vector3& B, float DeltaTime) const
		{
			return *this + (B - *this) * DeltaTime;
		}
		Vector3 Vector3::SphericalLerp(const Vector3& B, float DeltaTime) const
		{
			return Quaternion(*this).SphericalLerp(B, DeltaTime).GetEuler();
		}
		Vector3 Vector3::AngularLerp(const Vector3& B, float DeltaTime) const
		{
			float Ax = Mathf::AngluarLerp(X, B.X, DeltaTime);
			float Ay = Mathf::AngluarLerp(Y, B.Y, DeltaTime);
			float Az = Mathf::AngluarLerp(Z, B.Z, DeltaTime);

			return Vector3(Ax, Ay, Az);
		}
		Vector3 Vector3::SaturateRotation() const
		{
			float Ax = Mathf::SaturateAngle(X);
			float Ay = Mathf::SaturateAngle(Y);
			float Az = Mathf::SaturateAngle(Z);

			return Vector3(Ax, Ay, Az);
		}
		Vector3 Vector3::Abs() const
		{
			return Vector3(X < 0 ? -X : X, Y < 0 ? -Y : Y, Z < 0 ? -Z : Z);
		}
		void Vector3::Set(const Vector3& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
		}
		void Vector3::Get2(float* In) const
		{
			In[0] = X;
			In[1] = Y;
		}
		void Vector3::Get3(float* In) const
		{
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
		}
		Vector3& Vector3::operator -=(const Vector3& V)
		{
			X -= V.X;
			Y -= V.Y;
			Z -= V.Z;
			return *this;
		}
		Vector3& Vector3::operator -=(float V)
		{
			X -= V;
			Y -= V;
			Z -= V;
			return *this;
		}
		Vector3& Vector3::operator *=(const Vector3& V)
		{
			X *= V.X;
			Y *= V.Y;
			Z *= V.Z;
			return *this;
		}
		Vector3& Vector3::operator *=(float V)
		{
			X *= V;
			Y *= V;
			Z *= V;
			return *this;
		}
		Vector3& Vector3::operator /=(const Vector3& V)
		{
			X /= V.X;
			Y /= V.Y;
			Z /= V.Z;
			return *this;
		}
		Vector3& Vector3::operator /=(float V)
		{
			X /= V;
			Y /= V;
			Z /= V;
			return *this;
		}
		Vector3& Vector3::operator +=(const Vector3& V)
		{
			X += V.X;
			Y += V.Y;
			Z += V.Z;
			return *this;
		}
		Vector3& Vector3::operator +=(float V)
		{
			X += V;
			Y += V;
			Z += V;
			return *this;
		}
		Vector3 Vector3::operator *(const Vector3& V) const
		{
			return Vector3(X * V.X, Y * V.Y, Z * V.Z);
		}
		Vector3 Vector3::operator *(float V) const
		{
			return Vector3(X * V, Y * V, Z * V);
		}
		Vector3 Vector3::operator /(const Vector3& V) const
		{
			return Vector3(X / V.X, Y / V.Y, Z / V.Z);
		}
		Vector3 Vector3::operator /(float V) const
		{
			return Vector3(X / V, Y / V, Z / V);
		}
		Vector3 Vector3::operator +(const Vector3& V) const
		{
			return Vector3(X + V.X, Y + V.Y, Z + V.Z);
		}
		Vector3 Vector3::operator +(float V) const
		{
			return Vector3(X + V, Y + V, Z + V);
		}
		Vector3 Vector3::operator -(const Vector3& V) const
		{
			return Vector3(X - V.X, Y - V.Y, Z - V.Z);
		}
		Vector3 Vector3::operator -(float V) const
		{
			return Vector3(X - V, Y - V, Z - V);
		}
		Vector3 Vector3::operator -() const
		{
			return Invert();
		}
		void Vector3::ToInt32(unsigned int& Value) const
		{
			unsigned char Bytes[4];
			Bytes[0] = static_cast<unsigned char>(X * 255.0f + 0.5f);
			Bytes[1] = static_cast<unsigned char>(Y * 255.0f + 0.5f);
			Bytes[2] = static_cast<unsigned char>(Z * 255.0f + 0.5f);
			Bytes[3] = 0;

			Value = (unsigned int)(unsigned long long)Bytes;
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
			return X < R.X && Y < R.Y && Z < R.Z;
		}
		bool Vector3::operator >(const Vector3& R) const
		{
			return X > R.X && Y > R.Y && Z > R.Z;
		}
		float& Vector3::operator [](int Axis)
		{
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;
			else if (Axis == 2)
				return Z;

			return X;
		}
		float Vector3::operator [](int Axis) const
		{
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;
			else if (Axis == 2)
				return Z;

			return X;
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
		Vector3 Vector3::Random()
		{
			return Vector3(Mathf::RandomMag(), Mathf::RandomMag(), Mathf::RandomMag());
		}
		Vector3 Vector3::Radians() const
		{
			return (*this) * Mathf::Deg2Rad();
		}
		Vector3 Vector3::RandomAbs()
		{
			return Vector3(Mathf::Random(), Mathf::Random(), Mathf::Random());
		}
		Vector3 Vector3::Degrees() const
		{
			return (*this) * Mathf::Rad2Deg();
		}
		void Vector3::ToBtVector3(const Vector3& In, btVector3* Out)
		{
			if (Out != nullptr)
			{
				Out->setX(In.X);
				Out->setY(In.Y);
				Out->setZ(In.Z);
			}
		}
		void Vector3::FromBtVector3(const btVector3& In, Vector3* Out)
		{
			if (Out != nullptr)
			{
				Out->X = In.getX();
				Out->Y = In.getY();
				Out->Z = In.getZ();
			}
		}

		Vector4::Vector4()
		{
			X = 0;
			Y = 0;
			Z = 0;
			W = 0;
		}
		Vector4::Vector4(float x, float y)
		{
			X = x;
			Y = y;
			Z = 0;
			W = 0;
		}
		Vector4::Vector4(float x, float y, float z)
		{
			X = x;
			Y = y;
			Z = z;
			W = 0;
		}
		Vector4::Vector4(float x, float y, float z, float w)
		{
			X = x;
			Y = y;
			Z = z;
			W = w;
		}
		Vector4::Vector4(float xyzw)
		{
			X = Y = Z = W = xyzw;
		}
		Vector4::Vector4(const Vector2& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = 0;
			W = 0;
		}
		Vector4::Vector4(const Vector3& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
			W = 0;
		}
		float Vector4::Length() const
		{
			return Mathf::Sqrt(X * X + Y * Y + Z * Z + W * W);
		}
		float Vector4::ModLength() const
		{
			return X + Y + Z + W;
		}
		float Vector4::DotProduct(const Vector4& B) const
		{
			return X * B.X + Y * B.Y + Z * B.Z + W * B.W;
		}
		float Vector4::Distance(const Vector4& Point) const
		{
			float x = X - Point.X;
			float y = Y - Point.Y;
			float z = Z - Point.Z;
			float w = W - Point.W;

			return Mathf::Sqrt(x * x + y * y + z * z + w * w);
		}
		Vector4 Vector4::SaturateRotation() const
		{
			float Ax = Mathf::SaturateAngle(X);
			float Ay = Mathf::SaturateAngle(Y);
			float Az = Mathf::SaturateAngle(Z);
			float Aw = Mathf::SaturateAngle(W);

			return Vector4(Ax, Ay, Az, Aw);
		}
		Vector4 Vector4::Lerp(const Vector4& B, float DeltaTime) const
		{
			return *this + (B - *this) * DeltaTime;
		}
		Vector4 Vector4::SphericalLerp(const Vector4& B, float DeltaTime) const
		{
			Vector3 Lerp = Quaternion(Vector3()).SphericalLerp(B.XYZ(), DeltaTime).GetEuler();
			return Vector4(Lerp.X, Lerp.Y, Lerp.Z, W + (B.W - W) * DeltaTime);
		}
		Vector4 Vector4::AngularLerp(const Vector4& B, float DeltaTime) const
		{
			float Ax = Mathf::AngluarLerp(X, B.X, DeltaTime);
			float Ay = Mathf::AngluarLerp(Y, B.Y, DeltaTime);
			float Az = Mathf::AngluarLerp(Z, B.Z, DeltaTime);
			float Aw = Mathf::AngluarLerp(W, B.W, DeltaTime);

			return Vector4(Ax, Ay, Az, Aw);
		}
		Vector4 Vector4::Invert() const
		{
			return Vector4(-X, -Y, -Z, -W);
		}
		Vector4 Vector4::InvertX() const
		{
			return Vector4(-X, Y, Z, W);
		}
		Vector4 Vector4::InvertY() const
		{
			return Vector4(X, -Y, Z, W);
		}
		Vector4 Vector4::InvertZ() const
		{
			return Vector4(X, Y, -Z, W);
		}
		Vector4 Vector4::InvertW() const
		{
			return Vector4(X, Y, Z, -W);
		}
		Vector4 Vector4::Cross(const Vector4& Vector1) const
		{
			return Vector4(Y * Vector1.Z - Z * Vector1.Y, Z * Vector1.X - X * Vector1.Z, X * Vector1.Y - Y * Vector1.X);
		}
		Vector4 Vector4::Normalize() const
		{
			float Factor = 1.0f / Length();

			return Vector4(X * Factor, Y * Factor, Z * Factor, W * Factor);
		}
		Vector4 Vector4::NormalizeSafe() const
		{
			float LengthZero = Length();
			float Factor = 1.0f / LengthZero;

			if (LengthZero == 0.0f)
				Factor = 0.0f;

			return Vector4(X * Factor, Y * Factor, Z * Factor, W * Factor);
		}
		Vector4 Vector4::Transform(const Matrix4x4& Matrix) const
		{
			return Vector4(
				X * Matrix.Row[0] + Y * Matrix.Row[4] + Z * Matrix.Row[8] + W * Matrix.Row[12],
				X * Matrix.Row[1] + Y * Matrix.Row[5] + Z * Matrix.Row[9] + W * Matrix.Row[13],
				X * Matrix.Row[2] + Y * Matrix.Row[6] + Z * Matrix.Row[10] + W * Matrix.Row[14],
				X * Matrix.Row[3] + Y * Matrix.Row[7] + Z * Matrix.Row[11] + W * Matrix.Row[15]);
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
		Vector4 Vector4::Mul(const Vector2& XY, float z, float w) const
		{
			return Vector4(X * XY.X, Y * XY.Y, Z * z, W * w);
		}
		Vector4 Vector4::Mul(const Vector3& XYZ, float w) const
		{
			return Vector4(X * XYZ.X, Y * XYZ.Y, Z * XYZ.Z, W * w);
		}
		Vector4 Vector4::Mul(const Vector4& Value) const
		{
			return *this * Value;
		}
		Vector4 Vector4::Add(const Vector4& Value) const
		{
			return *this + Value;
		}
		Vector4 Vector4::Div(const Vector4& Value) const
		{
			return *this / Value;
		}
		Vector4 Vector4::Abs() const
		{
			return Vector4(X < 0 ? -X : X, Y < 0 ? -Y : Y, Z < 0 ? -Z : Z, W < 0 ? -W : W);
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
			In[0] = X;
			In[1] = Y;
		}
		void Vector4::Get3(float* In) const
		{
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
		}
		void Vector4::Get4(float* In) const
		{
			In[0] = X;
			In[1] = Y;
			In[2] = Z;
			In[3] = W;
		}
		Vector4& Vector4::operator -=(const Vector4& V)
		{
			X -= V.X;
			Y -= V.Y;
			Z -= V.Z;
			W -= V.W;
			return *this;
		}
		Vector4& Vector4::operator -=(float V)
		{
			X -= V;
			Y -= V;
			Z -= V;
			W -= V;
			return *this;
		}
		Vector4& Vector4::operator *=(const Matrix4x4& V)
		{
			Set(Transform(V));
			return *this;
		}
		Vector4& Vector4::operator *=(const Vector4& V)
		{
			X *= V.X;
			Y *= V.Y;
			Z *= V.Z;
			W *= V.W;
			return *this;
		}
		Vector4& Vector4::operator *=(float V)
		{
			X *= V;
			Y *= V;
			Z *= V;
			W *= V;
			return *this;
		}
		Vector4& Vector4::operator /=(const Vector4& V)
		{
			X /= V.X;
			Y /= V.Y;
			Z /= V.Z;
			W /= V.W;
			return *this;
		}
		Vector4& Vector4::operator /=(float V)
		{
			X /= V;
			Y /= V;
			Z /= V;
			W /= V;
			return *this;
		}
		Vector4& Vector4::operator +=(const Vector4& V)
		{
			X += V.X;
			Y += V.Y;
			Z += V.Z;
			W += V.W;
			return *this;
		}
		Vector4& Vector4::operator +=(float V)
		{
			X += V;
			Y += V;
			Z += V;
			W += V;
			return *this;
		}
		Vector4 Vector4::operator *(const Matrix4x4& V) const
		{
			return Transform(V);
		}
		Vector4 Vector4::operator *(const Vector4& V) const
		{
			return Vector4(X * V.X, Y * V.Y, Z * V.Z, W * V.W);
		}
		Vector4 Vector4::operator *(float V) const
		{
			return Vector4(X * V, Y * V, Z * V, W * V);
		}
		Vector4 Vector4::operator /(const Vector4& V) const
		{
			return Vector4(X / V.X, Y / V.Y, Z / V.Z, W / V.W);
		}
		Vector4 Vector4::operator /(float V) const
		{
			return Vector4(X / V, Y / V, Z / V, W / V);
		}
		Vector4 Vector4::operator +(const Vector4& V) const
		{
			return Vector4(X + V.X, Y + V.Y, Z + V.Z, W + V.W);
		}
		Vector4 Vector4::operator +(float V) const
		{
			return Vector4(X + V, Y + V, Z + V, W + V);
		}
		Vector4 Vector4::operator -(const Vector4& V) const
		{
			return Vector4(X - V.X, Y - V.Y, Z - V.Z, W - V.W);
		}
		Vector4 Vector4::operator -(float V) const
		{
			return Vector4(X - V, Y - V, Z - V, W - V);
		}
		Vector4 Vector4::operator -() const
		{
			return Invert();
		}
		void Vector4::ToInt32(unsigned int& Value) const
		{
			unsigned char Bytes[4];
			Bytes[0] = static_cast<unsigned char>(X * 255.0f + 0.5f);
			Bytes[1] = static_cast<unsigned char>(Y * 255.0f + 0.5f);
			Bytes[2] = static_cast<unsigned char>(Z * 255.0f + 0.5f);
			Bytes[3] = static_cast<unsigned char>(W * 255.0f + 0.5f);

			Value = (unsigned int)(unsigned long long)Bytes;
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
			return X < R.X && Y < R.Y && Z < R.Z && W < R.W;
		}
		bool Vector4::operator >(const Vector4& R) const
		{
			return X > R.X && Y > R.Y && Z > R.Z && W > R.W;
		}
		float& Vector4::operator [](int Axis)
		{
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;
			else if (Axis == 2)
				return Z;
			else if (Axis == 3)
				return W;

			return X;
		}
		float Vector4::operator [](int Axis) const
		{
			if (Axis == 0)
				return X;
			else if (Axis == 1)
				return Y;
			else if (Axis == 2)
				return Z;
			else if (Axis == 3)
				return W;

			return X;
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
		Vector4 Vector4::Random()
		{
			return Vector4(Mathf::RandomMag(), Mathf::RandomMag(), Mathf::RandomMag(), Mathf::RandomMag());
		}
		Vector4 Vector4::RandomAbs()
		{
			return Vector4(Mathf::Random(), Mathf::Random(), Mathf::Random(), Mathf::Random());
		}
		Vector4 Vector4::Radians() const
		{
			return (*this) * Mathf::Deg2Rad();
		}
		Vector4 Vector4::Degrees() const
		{
			return (*this) * Mathf::Rad2Deg();
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
			float D = Normal.DotProduct(Direction);
			if (Mathf::Abs(D) < std::numeric_limits<float>::epsilon())
				return false;

			float N = Normal.DotProduct(Origin) + Diameter;
			float T = -(N / D);
			return T >= 0;
		}
		bool Ray::IntersectsSphere(const Vector3& Position, float Radius, bool DiscardInside) const
		{
			Vector3 R = Origin - Position;
			float L = R.Length();

			if (L * L <= Radius * Radius && DiscardInside)
				return true;

			float A = Direction.DotProduct(Direction);
			float B = 2 * R.DotProduct(Direction);
			float C = R.DotProduct(R) - Radius * Radius;

			float D = (B * B) - (4 * A * C);
			if (D < 0)
				return false;

			float T = (-B - Mathf::Sqrt(D)) / (2 * A);
			if (T < 0)
				T = (-B + Mathf::Sqrt(D)) / (2 * A);

			return true;
		}
		bool Ray::IntersectsAABBAt(const Vector3& Min, const Vector3& Max) const
		{
			Vector3 HitPoint; float T;
			if (Origin > Min && Origin < Max)
				return true;

			if (Origin.X <= Min.X && Direction.X > 0)
			{
				T = (Min.X - Origin.X) / Direction.X;
				HitPoint = Origin + Direction * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (Origin.X >= Max.X && Direction.X < 0)
			{
				T = (Max.X - Origin.X) / Direction.X;
				HitPoint = Origin + Direction * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (Origin.Y <= Min.Y && Direction.Y > 0)
			{
				T = (Min.Y - Origin.Y) / Direction.Y;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (Origin.Y >= Max.Y && Direction.Y < 0)
			{
				T = (Max.Y - Origin.Y) / Direction.Y;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (Origin.Z <= Min.Z && Direction.Z > 0)
			{
				T = (Min.Z - Origin.Z) / Direction.Z;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
					return true;
			}

			if (Origin.Z >= Max.Z && Direction.Z < 0)
			{
				T = (Max.Z - Origin.Z) / Direction.Z;
				HitPoint = Origin + Direction * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
					return true;
			}

			return false;
		}
		bool Ray::IntersectsAABB(const Vector3& Position, const Vector3& Scale) const
		{
			Vector3 Min = Position - Scale;
			Vector3 Max = Position + Scale;

			return IntersectsAABBAt(Min, Max);
		}
		bool Ray::IntersectsOBB(const Matrix4x4& World) const
		{
			Matrix4x4 Offset = World.Invert();
			Vector3 Min = -1.0f, Max = 1.0f;
			Vector3 O = (Vector4(Origin.X, Origin.Y, Origin.Z, 1.0f)  * Offset).XYZ();
			if (O > Min && O < Max)
				return true;

			Vector3 D = (Direction.XYZW() * Offset).NormalizeSafe().XYZ();
			Vector3 HitPoint; float T;

			if (O.X <= Min.X && D.X > 0)
			{
				T = (Min.X - O.X) / D.X;
				HitPoint = O + D * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (O.X >= Max.X && D.X < 0)
			{
				T = (Max.X - O.X) / D.X;
				HitPoint = O + D * T;

				if (HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (O.Y <= Min.Y && D.Y > 0)
			{
				T = (Min.Y - O.Y) / D.Y;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (O.Y >= Max.Y && D.Y < 0)
			{
				T = (Max.Y - O.Y) / D.Y;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Z >= Min.Z && HitPoint.Z <= Max.Z)
					return true;
			}

			if (O.Z <= Min.Z && D.Z > 0)
			{
				T = (Min.Z - O.Z) / D.Z;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
					return true;
			}

			if (O.Z >= Max.Z && D.Z < 0)
			{
				T = (Max.Z - O.Z) / D.Z;
				HitPoint = O + D * T;

				if (HitPoint.X >= Min.X && HitPoint.X <= Max.X && HitPoint.Y >= Min.Y && HitPoint.Y <= Max.Y)
					return true;
			}

			return false;
		}

		Matrix4x4::Matrix4x4()
		{
			Row[0] = 1;
			Row[1] = 0;
			Row[2] = 0;
			Row[3] = 0;
			Row[4] = 0;
			Row[5] = 1;
			Row[6] = 0;
			Row[7] = 0;
			Row[8] = 0;
			Row[9] = 0;
			Row[10] = 1;
			Row[11] = 0;
			Row[12] = 0;
			Row[13] = 0;
			Row[14] = 0;
			Row[15] = 1;
		}
		Matrix4x4::Matrix4x4(btTransform* In)
		{
			if (!In)
				return;

			btMatrix3x3 Offset = In->getBasis();
			Row[0] = Offset[0][0];
			Row[1] = Offset[1][0];
			Row[2] = Offset[2][0];
			Row[4] = Offset[0][1];
			Row[5] = Offset[1][1];
			Row[6] = Offset[2][1];
			Row[8] = Offset[0][2];
			Row[9] = Offset[1][2];
			Row[10] = Offset[2][2];
		}
		Matrix4x4::Matrix4x4(float Array[16])
		{
			Row[0] = Array[0];
			Row[1] = Array[1];
			Row[2] = Array[2];
			Row[3] = Array[3];

			Row[4] = Array[4];
			Row[5] = Array[5];
			Row[6] = Array[6];
			Row[7] = Array[7];

			Row[8] = Array[8];
			Row[9] = Array[9];
			Row[10] = Array[10];
			Row[11] = Array[11];

			Row[12] = Array[12];
			Row[13] = Array[13];
			Row[14] = Array[14];
			Row[15] = Array[15];
		}
		Matrix4x4::Matrix4x4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3)
		{
			Row[0] = row0.X;
			Row[1] = row0.Y;
			Row[2] = row0.Z;
			Row[3] = row0.W;
			Row[4] = row1.X;
			Row[5] = row1.Y;
			Row[6] = row1.Z;
			Row[7] = row1.W;
			Row[8] = row2.X;
			Row[9] = row2.Y;
			Row[10] = row2.Z;
			Row[11] = row2.W;
			Row[12] = row3.X;
			Row[13] = row3.Y;
			Row[14] = row3.Z;
			Row[15] = row3.W;
		}
		Matrix4x4::Matrix4x4(float row00, float row01, float row02, float row03, float row10, float row11, float row12, float row13, float row20, float row21, float row22, float row23, float row30, float row31, float row32, float row33)
		{
			Row[0] = row00;
			Row[1] = row01;
			Row[2] = row02;
			Row[3] = row03;
			Row[4] = row10;
			Row[5] = row11;
			Row[6] = row12;
			Row[7] = row13;
			Row[8] = row20;
			Row[9] = row21;
			Row[10] = row22;
			Row[11] = row23;
			Row[12] = row30;
			Row[13] = row31;
			Row[14] = row32;
			Row[15] = row33;
		}
		float& Matrix4x4::operator [](int Index)
		{
			return Row[Index];
		}
		float Matrix4x4::operator [](int Index) const
		{
			return Row[Index];
		}
		bool Matrix4x4::operator ==(const Matrix4x4& Equal) const
		{
			for (int i = 0; i < 16; i++)
			{
				if (Row[i] != Equal.Row[i])
					return false;
			}

			return true;
		}
		bool Matrix4x4::operator !=(const Matrix4x4& Equal) const
		{
			for (int i = 0; i < 16; i++)
			{
				if (Row[i] != Equal.Row[i])
					return true;
			}

			return false;
		}
		Matrix4x4 Matrix4x4::operator *(const Matrix4x4& V) const
		{
			return this->Mul(V);
		}
		Matrix4x4 Matrix4x4::operator *(const Vector4& V) const
		{
			return this->Mul(V);
		}
		Matrix4x4& Matrix4x4::operator =(const Matrix4x4& V)
		{
			Row[0] = V.Row[0];
			Row[1] = V.Row[1];
			Row[2] = V.Row[2];
			Row[3] = V.Row[3];
			Row[4] = V.Row[4];
			Row[5] = V.Row[5];
			Row[6] = V.Row[6];
			Row[7] = V.Row[7];
			Row[8] = V.Row[8];
			Row[9] = V.Row[9];
			Row[10] = V.Row[10];
			Row[11] = V.Row[11];
			Row[12] = V.Row[12];
			Row[13] = V.Row[13];
			Row[14] = V.Row[14];
			Row[15] = V.Row[15];

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
			return Vector3(Row[4], Row[5], Row[6]);
		}
		Vector3 Matrix4x4::Right() const
		{
			return Vector3(Row[0], Row[1], Row[2]);
		}
		Vector3 Matrix4x4::Forward() const
		{
			return Vector3(Row[8], Row[9], Row[10]);
		}
		Matrix4x4 Matrix4x4::Invert() const
		{
			Matrix4x4 Result = Identity();

			Result.Row[0] = Row[5] * Row[10] * Row[15] - Row[5] * Row[11] * Row[14] - Row[9] * Row[6] * Row[15] + Row[9] * Row[7] * Row[14] + Row[13] * Row[6] * Row[11] - Row[13] * Row[7] * Row[10];

			Result.Row[4] = -Row[4] * Row[10] * Row[15] + Row[4] * Row[11] * Row[14] + Row[8] * Row[6] * Row[15] - Row[8] * Row[7] * Row[14] - Row[12] * Row[6] * Row[11] + Row[12] * Row[7] * Row[10];

			Result.Row[8] = Row[4] * Row[9] * Row[15] - Row[4] * Row[11] * Row[13] - Row[8] * Row[5] * Row[15] + Row[8] * Row[7] * Row[13] + Row[12] * Row[5] * Row[11] - Row[12] * Row[7] * Row[9];

			Result.Row[12] = -Row[4] * Row[9] * Row[14] + Row[4] * Row[10] * Row[13] + Row[8] * Row[5] * Row[14] - Row[8] * Row[6] * Row[13] - Row[12] * Row[5] * Row[10] + Row[12] * Row[6] * Row[9];

			Result.Row[1] = -Row[1] * Row[10] * Row[15] + Row[1] * Row[11] * Row[14] + Row[9] * Row[2] * Row[15] - Row[9] * Row[3] * Row[14] - Row[13] * Row[2] * Row[11] + Row[13] * Row[3] * Row[10];

			Result.Row[5] = Row[0] * Row[10] * Row[15] - Row[0] * Row[11] * Row[14] - Row[8] * Row[2] * Row[15] + Row[8] * Row[3] * Row[14] + Row[12] * Row[2] * Row[11] - Row[12] * Row[3] * Row[10];

			Result.Row[9] = -Row[0] * Row[9] * Row[15] + Row[0] * Row[11] * Row[13] + Row[8] * Row[1] * Row[15] - Row[8] * Row[3] * Row[13] - Row[12] * Row[1] * Row[11] + Row[12] * Row[3] * Row[9];

			Result.Row[13] = Row[0] * Row[9] * Row[14] - Row[0] * Row[10] * Row[13] - Row[8] * Row[1] * Row[14] + Row[8] * Row[2] * Row[13] + Row[12] * Row[1] * Row[10] - Row[12] * Row[2] * Row[9];

			Result.Row[2] = Row[1] * Row[6] * Row[15] - Row[1] * Row[7] * Row[14] - Row[5] * Row[2] * Row[15] + Row[5] * Row[3] * Row[14] + Row[13] * Row[2] * Row[7] - Row[13] * Row[3] * Row[6];

			Result.Row[6] = -Row[0] * Row[6] * Row[15] + Row[0] * Row[7] * Row[14] + Row[4] * Row[2] * Row[15] - Row[4] * Row[3] * Row[14] - Row[12] * Row[2] * Row[7] + Row[12] * Row[3] * Row[6];

			Result.Row[10] = Row[0] * Row[5] * Row[15] - Row[0] * Row[7] * Row[13] - Row[4] * Row[1] * Row[15] + Row[4] * Row[3] * Row[13] + Row[12] * Row[1] * Row[7] - Row[12] * Row[3] * Row[5];

			Result.Row[14] = -Row[0] * Row[5] * Row[14] + Row[0] * Row[6] * Row[13] + Row[4] * Row[1] * Row[14] - Row[4] * Row[2] * Row[13] - Row[12] * Row[1] * Row[6] + Row[12] * Row[2] * Row[5];

			Result.Row[3] = -Row[1] * Row[6] * Row[11] + Row[1] * Row[7] * Row[10] + Row[5] * Row[2] * Row[11] - Row[5] * Row[3] * Row[10] - Row[9] * Row[2] * Row[7] + Row[9] * Row[3] * Row[6];

			Result.Row[7] = Row[0] * Row[6] * Row[11] - Row[0] * Row[7] * Row[10] - Row[4] * Row[2] * Row[11] + Row[4] * Row[3] * Row[10] + Row[8] * Row[2] * Row[7] - Row[8] * Row[3] * Row[6];

			Result.Row[11] = -Row[0] * Row[5] * Row[11] + Row[0] * Row[7] * Row[9] + Row[4] * Row[1] * Row[11] - Row[4] * Row[3] * Row[9] - Row[8] * Row[1] * Row[7] + Row[8] * Row[3] * Row[5];

			Result.Row[15] = Row[0] * Row[5] * Row[10] - Row[0] * Row[6] * Row[9] - Row[4] * Row[1] * Row[10] + Row[4] * Row[2] * Row[9] + Row[8] * Row[1] * Row[6] - Row[8] * Row[2] * Row[5];

			float Determinant = Row[0] * Result.Row[0] + Row[1] * Result.Row[4] + Row[2] * Result.Row[8] + Row[3] * Result.Row[12];

			if (Determinant == 0)
				return Identity();

			Determinant = 1.0f / Determinant;

			for (int i = 0; i < 16; i++)
				Result.Row[i] *= Determinant;

			return Result;
		}
		Matrix4x4 Matrix4x4::Transpose() const
		{
			return Matrix4x4(Vector4(Row[0], Row[4], Row[8], Row[12]), Vector4(Row[1], Row[5], Row[9], Row[13]), Vector4(Row[2], Row[6], Row[10], Row[14]), Vector4(Row[3], Row[7], Row[11], Row[15]));
		}
		Vector3 Matrix4x4::Rotation() const
		{
			Vector3 Result = 0;
			Result.X = atan2(-Row[6], Row[10]);
			Result.Y = atan2(Row[2], sqrt(Row[0] * Row[0] + Row[1] * Row[1]));
			Result.Z = atan2(cos(Result.X) * Row[4] + sin(Result.X) * Row[8], cos(Result.X) * Row[5] + sin(Result.X) * Row[9]);
			return -Result;
		}
		Vector3 Matrix4x4::Position() const
		{
			return Vector3(Row[12], Row[13], -Row[14]);
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

			return Result;
		}
		Matrix4x4 Matrix4x4::Mul(const Vector4& V) const
		{
			Matrix4x4 Result;

			Result.Row[0] = (Row[0] * V.X) + (Row[1] * V.Y) + (Row[2] * V.Z) + (Row[3] * V.W);
			Result.Row[1] = (Row[0] * V.X) + (Row[1] * V.Y) + (Row[2] * V.Z) + (Row[3] * V.W);
			Result.Row[2] = (Row[0] * V.X) + (Row[1] * V.Y) + (Row[2] * V.Z) + (Row[3] * V.W);
			Result.Row[3] = (Row[0] * V.X) + (Row[1] * V.Y) + (Row[2] * V.Z) + (Row[3] * V.W);

			Result.Row[4] = (Row[4] * V.X) + (Row[5] * V.Y) + (Row[6] * V.Z) + (Row[7] * V.W);
			Result.Row[5] = (Row[4] * V.X) + (Row[5] * V.Y) + (Row[6] * V.Z) + (Row[7] * V.W);
			Result.Row[6] = (Row[4] * V.X) + (Row[5] * V.Y) + (Row[6] * V.Z) + (Row[7] * V.W);
			Result.Row[7] = (Row[4] * V.X) + (Row[5] * V.Y) + (Row[6] * V.Z) + (Row[7] * V.W);

			Result.Row[8] = (Row[8] * V.X) + (Row[9] * V.Y) + (Row[10] * V.Z) + (Row[11] * V.W);
			Result.Row[9] = (Row[8] * V.X) + (Row[9] * V.Y) + (Row[10] * V.Z) + (Row[11] * V.W);
			Result.Row[10] = (Row[8] * V.X) + (Row[9] * V.Y) + (Row[10] * V.Z) + (Row[11] * V.W);
			Result.Row[11] = (Row[8] * V.X) + (Row[9] * V.Y) + (Row[10] * V.Z) + (Row[11] * V.W);

			Result.Row[12] = (Row[12] * V.X) + (Row[13] * V.Y) + (Row[14] * V.Z) + (Row[15] * V.W);
			Result.Row[13] = (Row[12] * V.X) + (Row[13] * V.Y) + (Row[14] * V.Z) + (Row[15] * V.W);
			Result.Row[14] = (Row[12] * V.X) + (Row[13] * V.Y) + (Row[14] * V.Z) + (Row[15] * V.W);
			Result.Row[15] = (Row[12] * V.X) + (Row[13] * V.Y) + (Row[14] * V.Z) + (Row[15] * V.W);

			return Result;
		}
		Vector2 Matrix4x4::XY() const
		{
			return Vector2(Row[0] + Row[4] + Row[8] + Row[12], Row[1] + Row[5] + Row[9] + Row[13]);
		}
		Vector3 Matrix4x4::XYZ() const
		{
			return Vector3(Row[0] + Row[4] + Row[8] + Row[12], Row[1] + Row[5] + Row[9] + Row[13], Row[2] + Row[6] + Row[10] + Row[14]);
		}
		Vector4 Matrix4x4::XYZW() const
		{
			return Vector4(Row[0] + Row[4] + Row[8] + Row[12], Row[1] + Row[5] + Row[9] + Row[13], Row[2] + Row[6] + Row[10] + Row[14], Row[3] + Row[7] + Row[11] + Row[15]);
		}
		void Matrix4x4::Identify()
		{
			Row[0] = 1;
			Row[1] = 0;
			Row[2] = 0;
			Row[3] = 0;
			Row[4] = 0;
			Row[5] = 1;
			Row[6] = 0;
			Row[7] = 0;
			Row[8] = 0;
			Row[9] = 0;
			Row[10] = 1;
			Row[11] = 0;
			Row[12] = 0;
			Row[13] = 0;
			Row[14] = 0;
			Row[15] = 1;
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
			Value.Row[14] = -Position.Z;
			Value.Row[0] = Scale.X;
			Value.Row[5] = Scale.Y;
			Value.Row[10] = Scale.Z;

			return Value;
		}
		Matrix4x4 Matrix4x4::CreateRotationX(float Rotation)
		{
			Matrix4x4 X = Identity();

			float Cos = Mathf::Cos(Rotation);
			float Sin = Mathf::Sin(Rotation);

			X.Row[5] = Cos;
			X.Row[6] = Sin;
			X.Row[9] = -Sin;
			X.Row[10] = Cos;

			return X;
		}
		Matrix4x4 Matrix4x4::CreateRotationY(float Rotation)
		{
			Matrix4x4 Y = Identity();

			float Cos = Mathf::Cos(Rotation);
			float Sin = Mathf::Sin(Rotation);

			Y.Row[0] = Cos;
			Y.Row[2] = -Sin;
			Y.Row[8] = Sin;
			Y.Row[10] = Cos;

			return Y;
		}
		Matrix4x4 Matrix4x4::CreateRotationZ(float Rotation)
		{
			Matrix4x4 Z = Identity();

			float Cos = Mathf::Cos(Rotation);
			float Sin = Mathf::Sin(Rotation);

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
		Matrix4x4 Matrix4x4::CreateCameraRotation(const Vector3& Rotation)
		{
			return Matrix4x4::CreateRotationZ(Rotation.Z) * Matrix4x4::CreateRotationY(Rotation.Y) * Matrix4x4::CreateRotationX(Rotation.X);
		}
		Matrix4x4 Matrix4x4::CreateScale(const Vector3& Scale)
		{
			Matrix4x4 Result = Identity();
			Result.Row[0] = Scale.X;
			Result.Row[5] = Scale.Y;
			Result.Row[10] = Scale.Z;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& Position)
		{
			Matrix4x4 Result = Identity();
			Result.Row[12] = Position.X;
			Result.Row[13] = Position.Y;
			Result.Row[14] = -Position.Z;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreatePerspectiveRad(float FieldOfView, float AspectRatio, float NearClip, float FarClip)
		{
			float Y = Mathf::Cotan(FieldOfView / 2.0f);
			float X = Y / AspectRatio;

			return Matrix4x4(Vector4(X, 0, 0, 0), Vector4(0, Y, 0, 0), Vector4(0, 0, FarClip / (FarClip - NearClip), 1), Vector4(0, 0, -NearClip * FarClip / (FarClip - NearClip), 0));
		}
		Matrix4x4 Matrix4x4::CreatePerspective(float FieldOfView, float AspectRatio, float NearClip, float FarClip)
		{
			float Y = Mathf::Cotan(Mathf::Deg2Rad() * FieldOfView / 2.0f);
			float X = Y / AspectRatio;

			return Matrix4x4(Vector4(X, 0, 0, 0), Vector4(0, Y, 0, 0), Vector4(0, 0, FarClip / (FarClip - NearClip), 1), Vector4(0, 0, -NearClip * FarClip / (FarClip - NearClip), 0));
		}
		Matrix4x4 Matrix4x4::CreateOrthographic(float width, float height, float NearClip, float FarClip)
		{
			return Matrix4x4(Vector4(2 / width, 0, 0, 0), Vector4(0, 2 / height, 0, 0), Vector4(0, 0, 1 / (FarClip - NearClip), 0), Vector4(0, 0, NearClip / (NearClip - FarClip), 1));
		}
		Matrix4x4 Matrix4x4::CreateOrthographic(float Left, float Right, float Bottom, float Top, float NearClip, float FarClip)
		{
			float Width = Right - Left;
			float Height = Top - Bottom;
			float Depth = FarClip - NearClip;

			return Matrix4x4(Vector4(2 / Width, 0, 0, -(Right + Left) / Width), Vector4(0, 2 / Height, 0, -(Top + Bottom) / Height), Vector4(0, 0, -2 / Depth, -(FarClip + NearClip) / Depth), Vector4(0, 0, 0, 1));
		}
		Matrix4x4 Matrix4x4::Create(const Vector3& Position, const Vector3& Scale, const Vector3& Rotation)
		{
			return Matrix4x4::CreateScale(Scale) * Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		Matrix4x4 Matrix4x4::Create(const Vector3& Position, const Vector3& Rotation)
		{
			return Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		Matrix4x4 Matrix4x4::CreateCamera(const Vector3& Position, const Vector3& Rotation)
		{
			return Matrix4x4::CreateTranslation(Position.InvertZ()) * Matrix4x4::CreateCameraRotation(Rotation);
		}
		Matrix4x4 Matrix4x4::CreateLookAt(const Vector3& Position, const Vector3& Target, const Vector3& Up)
		{
			Vector3 Z = (Target - Position).Normalize();
			Vector3 X = Up.Cross(Z).Normalize();
			Vector3 Y = Z.Cross(X);

			Matrix4x4 Result = Matrix4x4::Identity();
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
			Result.Row[12] = -X.DotProduct(Position);
			Result.Row[13] = -Y.DotProduct(Position);
			Result.Row[14] = -Z.DotProduct(Position);
			Result.Row[15] = 1;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreateLineLightLookAt(const Vector3& Position, const Vector3& Camera)
		{
			Vector3 APosition = (Position + Camera).InvertZ();
			Vector3 Z = (Position * Vector3(-1, -1, 1)).Normalize();
			Vector3 X = Vector3(0, 1).Cross(Z).Normalize();
			Vector3 Y = Z.Cross(X);

			Matrix4x4 Result = Matrix4x4::Identity();
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
			Result.Row[12] = -X.DotProduct(APosition);
			Result.Row[13] = -Y.DotProduct(APosition);
			Result.Row[14] = -Z.DotProduct(APosition);
			Result.Row[15] = 1;

			return Result;
		}
		Matrix4x4 Matrix4x4::CreateOrigin(const Vector3& Position, const Vector3& Rotation)
		{
			return CreateTranslation(Position) * CreateRotation(Rotation) * CreateTranslation(-Position);
		}
		Matrix4x4 Matrix4x4::CreateRotation(const Vector3& Forward, const Vector3& Up, const Vector3& Right)
		{
			Matrix4x4 Rotation;
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
		Matrix4x4 Matrix4x4::CreateCubeMapLookAt(int Face, const Vector3& Position)
		{
			if (Face == 0)
				return Matrix4x4::CreateLookAt(Position, Position + Vector3(1, 0, 0), Vector3::Up());

			if (Face == 1)
				return Matrix4x4::CreateLookAt(Position, Position - Vector3(1, 0, 0), Vector3::Up());

			if (Face == 2)
				return Matrix4x4::CreateLookAt(Position, Position + Vector3(0, 1, 0), Vector3::Backward());

			if (Face == 3)
				return Matrix4x4::CreateLookAt(Position, Position - Vector3(0, 1, 0), Vector3::Forward());

			if (Face == 4)
				return Matrix4x4::CreateLookAt(Position, Position + Vector3(0, 0, 1), Vector3::Up());

			if (Face == 5)
				return Matrix4x4::CreateLookAt(Position, Position - Vector3(0, 0, 1), Vector3::Up());

			return Matrix4x4::Identity();
		}

		Quaternion::Quaternion()
		{
			X = 0;
			Y = 0;
			Z = 0;
			W = 0;
		}
		Quaternion::Quaternion(float x, float y, float z, float w)
		{
			X = x;
			Y = y;
			Z = z;
			W = w;
		}
		Quaternion::Quaternion(const Quaternion& In)
		{
			X = In.X;
			Y = In.Y;
			Z = In.Z;
			W = In.W;
		}
		Quaternion::Quaternion(const Vector3& Axis, float Angle)
		{
			Set(Axis, Angle);
		}
		Quaternion::Quaternion(const Vector3& GetEuler)
		{
			Set(GetEuler);
		}
		Quaternion::Quaternion(const Matrix4x4& Value)
		{
			Set(Value);
		}
		void Quaternion::Set(const Vector3& Axis, float Angle)
		{
			float Sin = std::sin(Angle / 2);

			X = Axis.X * Sin;
			Y = Axis.Y * Sin;
			Z = Axis.Z * Sin;
			W = std::cos(Angle / 2);
		}
		void Quaternion::Set(const Vector3& GetEuler)
		{
			float SinX = std::sin(GetEuler.X / 2);
			float CosX = std::cos(GetEuler.X / 2);
			float SinY = std::sin(GetEuler.Y / 2);
			float CosY = std::cos(GetEuler.Y / 2);
			float SinZ = std::sin(GetEuler.Z / 2);
			float CosZ = std::cos(GetEuler.Z / 2);

			W = CosX * CosY;
			X = SinX * CosY;
			Y = CosX * SinY;
			Z = SinX * SinY;

			float FinalW = W * CosZ - Z * SinZ;
			float FinalX = X * CosZ + Y * SinZ;
			float FinalY = Y * CosZ - X * SinZ;
			float FinalZ = Z * CosZ + W * SinZ;

			X = FinalX;
			Y = FinalY;
			Z = FinalZ;
			W = FinalW;
		}
		void Quaternion::Set(const Matrix4x4& Value)
		{
			float trace = Value[0] + Value[5] + Value[9];

			if (trace > 0)
			{
				float s = 0.5f / std::sqrt(trace + 1.0f);
				X = 0.25f / s;
				Y = (Value[6] - Value[8]) * s;
				Z = (Value[7] - Value[2]) * s;
				W = (Value[1] - Value[4]) * s;
			}
			else
			{
				if (Value[0] > Value[5] && Value[0] > Value[9])
				{
					float s = 2.0f * std::sqrt(1.0f + Value[0] - Value[5] - Value[9]);
					W = (Value[6] - Value[8]) / s;
					X = 0.25f * s;
					Y = (Value[4] + Value[1]) / s;
					Z = (Value[7] + Value[2]) / s;
				}
				else if (Value[5] > Value[9])
				{
					float s = 2.0f * std::sqrt(1.0f + Value[5] - Value[0] - Value[9]);
					W = (Value[7] - Value[2]) / s;
					X = (Value[4] + Value[1]) / s;
					Y = 0.25f * s;
					Z = (Value[8] + Value[6]) / s;
				}
				else
				{
					float s = 2.0f * std::sqrt(1.0f + Value[9] - Value[0] - Value[5]);
					W = (Value[1] - Value[4]) / s;
					X = (Value[7] + Value[2]) / s;
					Y = (Value[6] + Value[8]) / s;
					Z = 0.25f * s;
				}
			}

			float length = std::sqrt(X * X + Y * Y + Z * Z + W * W);
			X /= length;
			Y /= length;
			Z /= length;
			W /= length;
		}
		void Quaternion::Set(const Quaternion& Value)
		{
			X = Value.X;
			Y = Value.Y;
			Z = Value.Z;
			W = Value.W;
		}
		Quaternion Quaternion::operator *(float r) const
		{
			return Quaternion(X * r, Y * r, Z * r, W * r);
		}
		Quaternion Quaternion::operator *(const Vector3& r) const
		{
			float w_ = -X * r.X - Y * r.Y - Z * r.Z;
			float x_ = W * r.X + Y * r.Z - Z * r.Y;
			float y_ = W * r.Y + Z * r.X - X * r.Z;
			float z_ = W * r.Z + X * r.Y - Y * r.X;

			return Quaternion(x_, y_, z_, w_);
		}
		Quaternion Quaternion::operator *(const Quaternion& r) const
		{
			float w_ = W * r.W - X * r.X - Y * r.Y - Z * r.Z;
			float x_ = X * r.W + W * r.X + Y * r.Z - Z * r.Y;
			float y_ = Y * r.W + W * r.Y + Z * r.X - X * r.Z;
			float z_ = Z * r.W + W * r.Z + X * r.Y - Y * r.X;

			return Quaternion(x_, y_, z_, w_);
		}
		Quaternion Quaternion::operator -(const Quaternion& r) const
		{
			return Quaternion(X - r.X, Y - r.Y, Z - r.Z, W - r.W);
		}
		Quaternion Quaternion::operator +(const Quaternion& r) const
		{
			return Quaternion(X + r.X, Y + r.Y, Z + r.Z, W + r.W);
		}
		Quaternion& Quaternion::operator =(const Quaternion& r)
		{
			this->X = r.X;
			this->Y = r.Y;
			this->Z = r.Z;
			this->W = r.W;
			return *this;
		}
		Quaternion Quaternion::Normalize() const
		{
			float length = Length();

			return Quaternion(X / length, Y / length, Z / length, W / length);
		}
		Quaternion Quaternion::Conjugate() const
		{
			return Quaternion(-X, -Y, -Z, W);
		}
		Quaternion Quaternion::Mul(float r) const
		{
			return Quaternion(X * r, Y * r, Z * r, W * r);
		}
		Quaternion Quaternion::Mul(const Quaternion& r) const
		{
			float w_ = W * r.W - X * r.X - Y * r.Y - Z * r.Z;
			float x_ = X * r.W + W * r.X + Y * r.Z - Z * r.Y;
			float y_ = Y * r.W + W * r.Y + Z * r.X - X * r.Z;
			float z_ = Z * r.W + W * r.Z + X * r.Y - Y * r.X;

			return Quaternion(x_, y_, z_, w_);
		}
		Quaternion Quaternion::Mul(const Vector3& r) const
		{
			float w_ = -X * r.X - Y * r.Y - Z * r.Z;
			float x_ = W * r.X + Y * r.Z - Z * r.Y;
			float y_ = W * r.Y + Z * r.X - X * r.Z;
			float z_ = W * r.Z + X * r.Y - Y * r.X;

			return Quaternion(x_, y_, z_, w_);
		}
		Quaternion Quaternion::Sub(const Quaternion& r) const
		{
			return Quaternion(X - r.X, Y - r.Y, Z - r.Z, W - r.W);
		}
		Quaternion Quaternion::Add(const Quaternion& r) const
		{
			return Quaternion(X + r.X, Y + r.Y, Z + r.Z, W + r.W);
		}
		Quaternion Quaternion::Lerp(const Quaternion& B, float DeltaTime) const
		{
			Quaternion Correction = B;

			if (DotProduct(B) < 0)
				Correction = Quaternion(-B.X, -B.Y, -B.Z, -B.W);

			return (Correction - *this) * DeltaTime + Normalize();
		}
		Quaternion Quaternion::SphericalLerp(const Quaternion& B, float DeltaTime) const
		{
			Quaternion Correction = B;
			float Cos = DotProduct(B);

			if (Cos < 0)
			{
				Correction = Quaternion(-B.X, -B.Y, -B.Z, -B.W);
				Cos = -Cos;
			}

			if (std::abs(Cos) >= 1.0f - 1e3f)
				return Lerp(Correction, DeltaTime);

			float Sin = std::sqrt(1.0f - Cos * Cos);
			float Angle = std::atan2(Sin, Cos);
			float InvertedSin = 1.0f / Sin;

			float Source = std::sin(Angle - DeltaTime * Angle) * InvertedSin;
			float Destination = std::sin(DeltaTime * Angle) * InvertedSin;

			return Mul(Source).Add(Correction.Mul(Destination));
		}
		Quaternion Quaternion::CreateEulerRotation(const Vector3& GetEuler)
		{
			float SinX = std::sin(GetEuler.X / 2);
			float CosX = std::cos(GetEuler.X / 2);
			float SinY = std::sin(GetEuler.Y / 2);
			float CosY = std::cos(GetEuler.Y / 2);
			float SinZ = std::sin(GetEuler.Z / 2);
			float CosZ = std::cos(GetEuler.Z / 2);

			Quaternion Q;
			Q.W = CosX * CosY;
			Q.X = SinX * CosY;
			Q.Y = CosX * SinY;
			Q.Z = SinX * SinY;

			float FinalW = Q.W * CosZ - Q.Z * SinZ;
			float FinalX = Q.X * CosZ + Q.Y * SinZ;
			float FinalY = Q.Y * CosZ - Q.X * SinZ;
			float FinalZ = Q.Z * CosZ + Q.W * SinZ;

			Q.X = FinalX;
			Q.Y = FinalY;
			Q.Z = FinalZ;
			Q.W = FinalW;
			return Q;
		}
		Quaternion Quaternion::CreateRotation(const Matrix4x4& Transform)
		{
			Quaternion Q;
			float trace = Transform[0] + Transform[5] + Transform[9];

			if (trace > 0)
			{
				float s = 0.5f / std::sqrt(trace + 1.0f);
				Q.X = 0.25f / s;
				Q.Y = (Transform[6] - Transform[8]) * s;
				Q.Z = (Transform[7] - Transform[2]) * s;
				Q.W = (Transform[1] - Transform[4]) * s;
			}
			else
			{
				if (Transform[0] > Transform[5] && Transform[0] > Transform[9])
				{
					float s = 2.0f * std::sqrt(1.0f + Transform[0] - Transform[5] - Transform[9]);
					Q.W = (Transform[6] - Transform[8]) / s;
					Q.X = 0.25f * s;
					Q.Y = (Transform[4] + Transform[1]) / s;
					Q.Z = (Transform[7] + Transform[2]) / s;
				}
				else if (Transform[5] > Transform[9])
				{
					float s = 2.0f * std::sqrt(1.0f + Transform[5] - Transform[0] - Transform[9]);
					Q.W = (Transform[7] - Transform[2]) / s;
					Q.X = (Transform[4] + Transform[1]) / s;
					Q.Y = 0.25f * s;
					Q.Z = (Transform[8] + Transform[6]) / s;
				}
				else
				{
					float s = 2.0f * std::sqrt(1.0f + Transform[9] - Transform[0] - Transform[5]);
					Q.W = (Transform[1] - Transform[4]) / s;
					Q.X = (Transform[7] + Transform[2]) / s;
					Q.Y = (Transform[6] + Transform[8]) / s;
					Q.Z = 0.25f * s;
				}
			}

			float length = std::sqrt(Q.X * Q.X + Q.Y * Q.Y + Q.Z * Q.Z + Q.W * Q.W);
			Q.X /= length;
			Q.Y /= length;
			Q.Z /= length;
			Q.W /= length;

			return Q;
		}
		Matrix4x4 Quaternion::GetMatrix() const
		{
			Vector3 Forward = Vector3(2.0f * (X * Z - W * Y), 2.0f * (Y * Z + W * X), 1.0f - 2.0f * (X * X + Y * Y));
			Vector3 Up = Vector3(2.0f * (X * Y + W * Z), 1.0f - 2.0f * (X * X + Z * Z), 2.0f * (Y * Z - W * X));
			Vector3 Right = Vector3(1.0f - 2.0f * (Y * Y + Z * Z), 2.0f * (X * Y - W * Z), 2.0f * (X * Z + W * Y));

			return Matrix4x4::CreateRotation(Forward, Up, Right);
		}
		Vector3 Quaternion::GetEuler() const
		{
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
		}
		float Quaternion::DotProduct(const Quaternion& r) const
		{
			return X * r.X + Y * r.Y + Z * r.Z + W * r.W;
		}
		float Quaternion::Length() const
		{
			return std::sqrt(X * X + Y * Y + Z * Z + W * W);
		}

		RandomVector2::RandomVector2()
		{
			Min = 0;
			Max = 1;
			Intensity = false;
			Accuracy = 1;
		}
		RandomVector2::RandomVector2(Vector2 MinV, Vector2 MaxV, bool IntensityV, float AccuracyV)
		{
			Min = MinV;
			Max = MaxV;
			Intensity = IntensityV;
			Accuracy = AccuracyV;
		}
		Vector2 RandomVector2::Generate()
		{
			return Vector2(Mathf::Random(Min.X * Accuracy, Max.X * Accuracy) / Accuracy, Mathf::Random(Min.Y * Accuracy, Max.Y * Accuracy) / Accuracy) * (Intensity ? Mathf::Random() : 1);
		}

		RandomVector3::RandomVector3()
		{
			Min = 0;
			Max = 1;
			Intensity = false;
			Accuracy = 1;
		}
		RandomVector3::RandomVector3(Vector3 MinV, Vector3 MaxV, bool IntensityV, float AccuracyV)
		{
			Min = MinV;
			Max = MaxV;
			Intensity = IntensityV;
			Accuracy = AccuracyV;
		}
		Vector3 RandomVector3::Generate()
		{
			return Vector3(Mathf::Random(Min.X * Accuracy, Max.X * Accuracy) / Accuracy, Mathf::Random(Min.Y * Accuracy, Max.Y * Accuracy) / Accuracy, Mathf::Random(Min.Z * Accuracy, Max.Z * Accuracy) / Accuracy) * (Intensity ? Mathf::Random() : 1);
		}

		RandomVector4::RandomVector4()
		{
			Min = 0;
			Max = 1;
			Intensity = false;
			Accuracy = 1;
		}
		RandomVector4::RandomVector4(Vector4 MinV, Vector4 MaxV, bool IntensityV, float AccuracyV)
		{
			Min = MinV;
			Max = MaxV;
			Intensity = IntensityV;
			Accuracy = AccuracyV;
		}
		Vector4 RandomVector4::Generate()
		{
			return Vector4(Mathf::Random(Min.X * Accuracy, Max.X * Accuracy) / Accuracy, Mathf::Random(Min.Y * Accuracy, Max.Y * Accuracy) / Accuracy, Mathf::Random(Min.Z * Accuracy, Max.Z * Accuracy) / Accuracy, Mathf::Random(Min.W * Accuracy, Max.W * Accuracy) / Accuracy) * (Intensity ? Mathf::Random() : 1);
		}

		RandomFloat::RandomFloat()
		{
			Min = 0;
			Max = 1;
			Intensity = false;
			Accuracy = 1;
		}
		RandomFloat::RandomFloat(float MinV, float MaxV, bool IntensityV, float AccuracyV)
		{
			Min = MinV;
			Max = MaxV;
			Intensity = IntensityV;
			Accuracy = AccuracyV;
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
		std::string Hybi10Request::GetTextType()
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
		Hybi10_Opcode Hybi10Request::GetEnumType()
		{
			if (Type == 0 || Type == 1 || Type == 2)
				return Hybi10_Opcode_Text;

			if (Type == 8)
				return Hybi10_Opcode_Close;

			if (Type == 9)
				return Hybi10_Opcode_Ping;

			if (Type == 10)
				return Hybi10_Opcode_Pong;

			return Hybi10_Opcode_Invalid;
		}

		void RegexResult::ResetNextMatch()
		{
			NextMatch = -1;
		}
		bool RegexResult::HasMatch()
		{
			return !Matches.empty();
		}
		bool RegexResult::IsMatch(std::vector<RegexMatch>::iterator It)
		{
			return It != Matches.end();
		}
		int64_t RegexResult::Start()
		{
			auto Match = GetMatch(NextMatch);
			return (Match != Matches.end() ? Match->Start : -1);
		}
		int64_t RegexResult::End()
		{
			auto Match = GetMatch(NextMatch);
			return (Match != Matches.end() ? Match->End : -1);
		}
		int64_t RegexResult::Length()
		{
			auto Match = GetMatch(NextMatch);
			return (Match != Matches.end() ? Match->Length : -1);
		}
		int64_t RegexResult::GetSteps()
		{
			return Steps;
		}
		int64_t RegexResult::GetMatchesCount()
		{
			return Matches.size();
		}
		RegexState RegexResult::GetState()
		{
			return State;
		}
		std::vector<RegexMatch>::iterator RegexResult::GetMatch(int64_t Id)
		{
			return (Id >= 0 && Id < (int64_t)Matches.size() ? Matches.begin() + Id : Matches.end());
		}
		std::vector<RegexMatch>::iterator RegexResult::GetNextMatch()
		{
			return GetMatch(NextMatch++);
		}
		const char* RegexResult::Pointer()
		{
			auto Match = GetMatch(NextMatch);
			return (Match != Matches.end() ? Match->Pointer : nullptr);
		}

		void Regex::Setup(RegexResult* Info)
		{
			int64_t i, j;
			RegexBranch tmp;
			for (i = 0; i < (int64_t)Info->Branches.size(); i++)
			{
				for (j = i + 1; j < (int64_t)Info->Branches.size(); j++)
				{
					if (Info->Branches[i].BracketIndex > Info->Branches[j].BracketIndex)
					{
						tmp = Info->Branches[i];
						Info->Branches[i] = Info->Branches[j];
						Info->Branches[j] = tmp;
					}
				}
			}

			for (i = j = 0; i < (int64_t)Info->Brackets.size(); i++)
			{
				Info->Brackets[i].BranchesCount = 0;
				Info->Brackets[i].Branches = j;
				while (j < (int64_t)Info->Branches.size() && Info->Branches[j].BracketIndex == i)
				{
					Info->Brackets[i].BranchesCount++;
					j++;
				}
			}
		}
		bool Regex::Match(RegExp* Value, RegexResult* Result, const std::string& Buffer)
		{
			return Match(Value, Result, Buffer.c_str(), Buffer.size());
		}
		bool Regex::Match(RegExp* Value, RegexResult* Result, const char* Buffer, int64_t Length)
		{
			if (!Value || !Buffer || !Length)
				return false;

			RegexResult R;
			R.Expression = Value;
			R.Matches.reserve(8);
			R.Brackets.reserve(8);
			R.Branches.reserve(8);

			int64_t Code = Parse(Value->Regex.c_str(), (int64_t)Value->Regex.size(), Buffer, Length, &R);
			if (Code > 0)
			{
				for (auto It = R.Matches.begin(); It != R.Matches.end(); It++)
				{
					It->Start = It->Pointer - Buffer;
					It->End = It->Start + It->Length;
				}

				R.State = RegexState_Match_Found;
			}
			else
				R.State = (RegexState)Code;

			if (Result != nullptr)
				*Result = R;

			return Code > 0;
		}
		bool Regex::MatchAll(RegExp* Value, RegexResult* Result, const std::string& Buffer)
		{
			return MatchAll(Value, Result, Buffer.c_str(), Buffer.size());
		}
		bool Regex::MatchAll(RegExp* Value, RegexResult* Result, const char* Buffer, int64_t Length)
		{
			if (!Value || !Buffer || !Length)
				return false;

			RegexResult R;
			R.Expression = Value;
			R.Matches.reserve(16);
			R.Brackets.reserve(16);
			R.Branches.reserve(16);

			int64_t Code = 0, Offset = 0, Steps = 0;
			std::vector<RegexMatch> Matches;

			while ((Code = Parse(Value->Regex.c_str(), (int64_t)Value->Regex.size(), Buffer + Offset, Length - Offset, &R)) >= 0)
			{
				for (auto It = R.Matches.begin(); It != R.Matches.end(); It++)
				{
					RegexMatch Match = *It;
					Match.Start = It->Pointer - Buffer;
					Match.End = It->Start + It->Length;
					Matches.push_back(Match);
				}

				R.Matches.clear();
				R.Brackets.clear();
				R.Branches.clear();
				Offset += Code;
				Steps++;
			}

			R.State = (Steps > 0 ? RegexState_Match_Found : (RegexState)Code);
			R.Matches = Matches;
			if (Result != nullptr)
				*Result = R;

			return R.Steps > 0;
		}
		int64_t Regex::Meta(const unsigned char* Buffer)
		{
			static const char* metacharacters = "^$().[]*+?|\\Ssdbfnrtv";
			return strchr(metacharacters, *Buffer) != nullptr;
		}
		int64_t Regex::OpLength(const char* Value)
		{
			return Value[0] == '\\' && Value[1] == 'x' ? 4 : Value[0] == '\\' ? 2 : 1;
		}
		int64_t Regex::SetLength(const char* Value, int64_t ValueLength)
		{
			int64_t Length = 0;
			while (Length < ValueLength && Value[Length] != ']')
				Length += OpLength(Value + Length);

			return Length <= ValueLength ? Length + 1 : -1;
		}
		int64_t Regex::GetOpLength(const char* Value, int64_t ValueLength)
		{
			return Value[0] == '[' ? SetLength(Value + 1, ValueLength - 1) + 1 : OpLength(Value);
		}
		int64_t Regex::Quantifier(const char* Value)
		{
			return Value[0] == '*' || Value[0] == '+' || Value[0] == '?';
		}
		int64_t Regex::ToInt(int64_t x)
		{
			return (int64_t)(isdigit((int)x) ? (int)x - '0' : (int)x - 'W');
		}
		int64_t Regex::HexToInt(const unsigned char* Buffer)
		{
			return (ToInt(tolower(Buffer[0])) << 4) | ToInt(tolower(Buffer[1]));
		}
		int64_t Regex::MatchOp(const unsigned char* Value, const unsigned char* Buffer, RegexResult* Info)
		{
			int64_t result = 0;
			switch (*Value)
			{
				case '\\':
					switch (Value[1])
					{
						case 'S':
							REGEX_FAIL(isspace(*Buffer), RegexState_No_Match);
							result++;
							break;
						case 's':
							REGEX_FAIL(!isspace(*Buffer), RegexState_No_Match);
							result++;
							break;
						case 'd':
							REGEX_FAIL(!isdigit(*Buffer), RegexState_No_Match);
							result++;
							break;
						case 'b':
							REGEX_FAIL(*Buffer != '\b', RegexState_No_Match);
							result++;
							break;
						case 'f':
							REGEX_FAIL(*Buffer != '\f', RegexState_No_Match);
							result++;
							break;
						case 'n':
							REGEX_FAIL(*Buffer != '\n', RegexState_No_Match);
							result++;
							break;
						case 'r':
							REGEX_FAIL(*Buffer != '\r', RegexState_No_Match);
							result++;
							break;
						case 't':
							REGEX_FAIL(*Buffer != '\t', RegexState_No_Match);
							result++;
							break;
						case 'v':
							REGEX_FAIL(*Buffer != '\v', RegexState_No_Match);
							result++;
							break;
						case 'x':
							REGEX_FAIL((unsigned char)HexToInt(Value + 2) != *Buffer, RegexState_No_Match);
							result++;
							break;
						default:
							REGEX_FAIL(Value[1] != Buffer[0], RegexState_No_Match);
							result++;
							break;
					}
					break;
				case '|':
					REGEX_FAIL(1, RegexState_Internal_Error);
					break;
				case '$':
					REGEX_FAIL(1, RegexState_No_Match);
					break;
				case '.':
					result++;
					break;
				default:
					if (Info->Expression->Flags & RegexFlags_IgnoreCase)
					{
						REGEX_FAIL(tolower(*Value) != tolower(*Buffer), RegexState_No_Match);
					}
					else
					{
						REGEX_FAIL(*Value != *Buffer, RegexState_No_Match);
					}
					result++;
					break;
			}

			return result;
		}
		int64_t Regex::MatchSet(const char* Value, int64_t ValueLength, const char* Buffer, RegexResult* Info)
		{
			int64_t Length = 0, result = -1, invert = Value[0] == '^';
			if (invert)
				Value++, ValueLength--;

			while (Length <= ValueLength && Value[Length] != ']' && result <= 0)
			{
				if (Value[Length] != '-' && Value[Length + 1] == '-' && Value[Length + 2] != ']' && Value[Length + 2] != '\0')
				{
					result = (Info->Expression->Flags & RegexFlags_IgnoreCase) ? tolower(*Buffer) >= tolower(Value[Length]) && tolower(*Buffer) <= tolower(Value[Length + 2]) : *Buffer >= Value[Length] && *Buffer <= Value[Length + 2];
					Length += 3;
				}
				else
				{
					result = MatchOp((const unsigned char*)Value + Length, (const unsigned char*)Buffer, Info);
					Length += OpLength(Value + Length);
				}
			}

			return (!invert && result > 0) || (invert && result <= 0) ? 1 : -1;
		}
		int64_t Regex::ParseInner(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case)
		{
			int64_t i, j, n, step;
			for (i = j = 0; i < ValueLength && j <= BufferLength; i += step)
			{
				step = Value[i] == '(' ? Info->Brackets[Case + 1].Length + 2 : GetOpLength(Value + i, ValueLength - i);
				REGEX_FAIL(Quantifier(&Value[i]), RegexState_Unexpected_Quantifier);
				REGEX_FAIL(step <= 0, RegexState_Invalid_Character_Set);
				Info->Steps++;

				if (i + step < ValueLength && Quantifier(Value + i + step))
				{
					if (Value[i + step] == '?')
					{
						int64_t result = ParseInner(Value + i, step, Buffer + j, BufferLength - j, Info, Case);
						j += result > 0 ? result : 0;
						i++;
					}
					else if (Value[i + step] == '+' || Value[i + step] == '*')
					{
						int64_t j2 = j, nj = j, n1, n2 = -1, ni, non_greedy = 0;
						ni = i + step + 1;
						if (ni < ValueLength && Value[ni] == '?')
						{
							non_greedy = 1;
							ni++;
						}

						do
						{
							if ((n1 = ParseInner(Value + i, step, Buffer + j2, BufferLength - j2, Info, Case)) > 0)
								j2 += n1;

							if (Value[i + step] == '+' && n1 < 0)
								break;

							if (ni >= ValueLength)
								nj = j2;
							else if ((n2 = ParseInner(Value + ni, ValueLength - ni, Buffer + j2, BufferLength - j2, Info, Case)) >= 0)
								nj = j2 + n2;

							if (nj > j && non_greedy)
								break;
						}while (n1 > 0);

						if (n1 < 0 && n2 < 0 && Value[i + step] == '*' && (n2 = ParseInner(Value + ni, ValueLength - ni, Buffer + j, BufferLength - j, Info, Case)) > 0)
							nj = j + n2;

						REGEX_FAIL(Value[i + step] == '+' && nj == j, RegexState_No_Match);
						REGEX_FAIL(nj == j && ni < ValueLength && n2 < 0, RegexState_No_Match);
						return nj;
					}

					continue;
				}

				if (Value[i] == '[')
				{
					n = MatchSet(Value + i + 1, ValueLength - (i + 2), Buffer + j, Info);
					REGEX_FAIL(n <= 0, RegexState_No_Match);
					j += n;
				}
				else if (Value[i] == '(')
				{
					n = RegexState_No_Match;
					Case++;
					REGEX_FAIL(Case >= (int64_t)Info->Brackets.size(), RegexState_Internal_Error);
					if (ValueLength - (i + step) > 0)
					{
						int64_t j2;
						for (j2 = 0; j2 <= BufferLength - j; j2++)
						{
							if ((n = ParseDOH(Buffer + j, BufferLength - (j + j2), Info, Case)) >= 0 && ParseInner(Value + i + step, ValueLength - (i + step), Buffer + j + n, BufferLength - (j + n), Info, Case) >= 0)
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
							Info->Matches.push_back(RegexMatch());
							Match = &Info->Matches.at(Info->Matches.size() - 1);
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
					REGEX_FAIL(j != 0, RegexState_No_Match);
				}
				else if (Value[i] == '$')
				{
					REGEX_FAIL(j != BufferLength, RegexState_No_Match);
				}
				else
				{
					REGEX_FAIL(j >= BufferLength, RegexState_No_Match);
					n = MatchOp((const unsigned char*)(Value + i), (const unsigned char*)(Buffer + j), Info);
					REGEX_FAIL(n <= 0, n);
					j += n;
				}
			}

			return j;
		}
		int64_t Regex::ParseDOH(const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case)
		{
			const RegexBracket* b = &Info->Brackets[Case];
			int64_t i = 0, Length, result;
			const char* p;

			do
			{
				p = i == 0 ? b->Pointer : Info->Branches[b->Branches + i - 1].Pointer + 1;
				Length = b->BranchesCount == 0 ? b->Length : i == b->BranchesCount ? (int64_t)(b->Pointer + b->Length - p) : (int64_t)(Info->Branches[b->Branches + i].Pointer - p);
				result = ParseInner(p, Length, Buffer, BufferLength, Info, Case);
			}while (result <= 0 && i++ < b->BranchesCount);

			return result;
		}
		int64_t Regex::ParseOuter(const char* Buffer, int64_t BufferLength, RegexResult* Info)
		{
			int64_t is_anchored = Info->Brackets[0].Pointer[0] == '^', i, result = -1;
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

			if (Info->Matches.empty() && result >= 0 && i >= 0)
			{
				RegexMatch Match;
				Match.Steps = Info->Steps;
				if (result == Info->Expression->Regex.size())
				{
					Match.Length = result;
					Match.Pointer = Buffer;
				}
				else
				{
					Match.Length = result - Info->Expression->Regex.size();
					Match.Pointer = Buffer + Match.Length;
				}

				Info->Matches.push_back(Match);
			}

			return result;
		}
		int64_t Regex::Parse(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info)
		{
			RegexBracket Bracket;
			Bracket.Pointer = Value;
			Bracket.Length = ValueLength;
			Info->Brackets.push_back(Bracket);

			int64_t i, step, depth = 0;
			for (i = 0; i < ValueLength; i += step)
			{
				step = GetOpLength(Value + i, ValueLength - i);
				if (Value[i] == '|')
				{
					RegexBranch Branch;
					Branch.BracketIndex = (Info->Brackets.back().Length == -1 ? Info->Brackets.size() - 1 : depth);
					Branch.Pointer = &Value[i];
					Info->Branches.push_back(Branch);
				}
				else if (Value[i] == '\\')
				{
					REGEX_FAIL(i >= ValueLength - 1, RegexState_Invalid_Metacharacter);
					if (Value[i + 1] == 'x')
					{
						REGEX_FAIL(Value[i + 1] == 'x' && i >= ValueLength - 3, RegexState_Invalid_Metacharacter);
						REGEX_FAIL(Value[i + 1] == 'x' && !(isxdigit(Value[i + 2]) && isxdigit(Value[i + 3])), RegexState_Invalid_Metacharacter);
					}
					else
					{
						REGEX_FAIL(!Meta((const unsigned char*)Value + i + 1), RegexState_Invalid_Metacharacter);
					}
				}
				else if (Value[i] == '(')
				{
					depth++;
					Bracket.Pointer = Value + i + 1;
					Bracket.Length = -1;
					Info->Brackets.push_back(Bracket);
					REGEX_FAIL(Info->Matches.size() > 0 && Info->Brackets.size() - 1 > Info->Matches.size(), RegexState_Sumatch_Array_Too_Small);
				}
				else if (Value[i] == ')')
				{
					int64_t ind = (Info->Brackets[Info->Brackets.size() - 1].Length == -1 ? Info->Brackets.size() - 1 : depth);
					Info->Brackets[ind].Length = (int64_t)(&Value[i] - Info->Brackets[ind].Pointer);
					depth--;
					REGEX_FAIL(depth < 0, RegexState_Unbalanced_Brackets);
					REGEX_FAIL(i > 0 && Value[i - 1] == '(', RegexState_No_Match);
				}
			}

			REGEX_FAIL(depth != 0, RegexState_Unbalanced_Brackets);
			Setup(Info);

			return ParseOuter(Buffer, BufferLength, Info);
		}
		RegExp Regex::Create(const std::string& Regexp, RegexFlags Flags, int64_t MaxMatches, int64_t MaxBranches, int64_t MaxBrackets)
		{
			RegExp Value;
			Value.Regex = Regexp;
			Value.Flags = Flags;
			Value.MaxBrackets = (MaxBrackets >= 1 ? MaxBrackets : 128);
			Value.MaxBranches = (MaxBranches >= 1 ? MaxBranches : 128);
			Value.MaxMatches = (MaxMatches >= 1 ? MaxMatches : 128);

			return Value;
		}
		const char* Regex::Syntax()
		{
			return "\"^\" - Match beginning of a buffer\n"
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

		CollisionBody::CollisionBody(btCollisionObject* Object)
		{
			btRigidBody* RigidObject = btRigidBody::upcast(Object);
			if (RigidObject != nullptr)
				Rigid = (RigidBody*)RigidObject->getUserPointer();

			btSoftBody* SoftObject = btSoftBody::upcast(Object);
			if (SoftObject != nullptr)
				Soft = (SoftBody*)SoftObject->getUserPointer();
		}

		MD5Hasher::MD5Hasher()
		{
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
			for (unsigned int i = 0, j = 0; j < Length; i++, j += 4)
				Output[i] = ((UInt4)Input[j]) | (((UInt4)Input[j + 1]) << 8) | (((UInt4)Input[j + 2]) << 16) | (((UInt4)Input[j + 3]) << 24);
		}
		void MD5Hasher::Encode(UInt1* Output, const UInt4* Input, unsigned int Length)
		{
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
			memset(X, 0, sizeof(X));
		}
		void MD5Hasher::Update(const std::string& Input, unsigned int BlockSize)
		{
			Update(Input.c_str(), (unsigned int)Input.size(), BlockSize);
		}
		void MD5Hasher::Update(const unsigned char* Input, unsigned int Length, unsigned int BlockSize)
		{
			unsigned int Index = Count[0] / 8 % BlockSize;
			if ((Count[0] += (Length << 3)) < (Length << 3))
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
			if (!Finalized)
				return nullptr;

			char* Output = (char*)malloc(sizeof(char) * 33);
			memset((void*)Output, 0, 33);

			for (int i = 0; i < 16; i++)
				sprintf(Output + i * 2, "%02x", Digest[i]);
			Output[32] = '\0';

			return Output;
		}
		unsigned char* MD5Hasher::RawDigest() const
		{
			if (!Finalized)
				return nullptr;

			UInt1* Output = (UInt1*)malloc(sizeof(UInt1) * 17);
			memcpy(Output, Digest, 17);
			Output[16] = '\0';

			return Output;
		}
		std::string MD5Hasher::ToHex() const
		{
			if (!Finalized)
				return std::string();

			char Data[33];
			for (int i = 0; i < 16; i++)
				sprintf(Data + i * 2, "%02x", Digest[i]);
			Data[32] = 0;

			return Data;
		}
		std::string MD5Hasher::ToRaw() const
		{
			if (!Finalized)
				return std::string();

			UInt1 Data[17];
			memcpy(Data, Digest, 17);
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

		float MathCommon::IsCubeInFrustum(const Matrix4x4& WVP, float Radius)
		{
			float Plane[4];
			Plane[0] = WVP.Row[3] + WVP.Row[0];
			Plane[1] = WVP.Row[7] + WVP.Row[4];
			Plane[2] = WVP.Row[11] + WVP.Row[8];
			Plane[3] = WVP.Row[15] + WVP.Row[12];

			Plane[3] /= sqrtf(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= -Radius)
				return Plane[3];

			Plane[0] = WVP.Row[3] - WVP.Row[0];
			Plane[1] = WVP.Row[7] - WVP.Row[4];
			Plane[2] = WVP.Row[11] - WVP.Row[8];
			Plane[3] = WVP.Row[15] - WVP.Row[12];

			Plane[3] /= sqrtf(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= -Radius)
				return Plane[3];

			Plane[0] = WVP.Row[3] + WVP.Row[1];
			Plane[1] = WVP.Row[7] + WVP.Row[5];
			Plane[2] = WVP.Row[11] + WVP.Row[9];
			Plane[3] = WVP.Row[15] + WVP.Row[13];

			Plane[3] /= sqrtf(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= -Radius)
				return Plane[3];

			Plane[0] = WVP.Row[3] - WVP.Row[1];
			Plane[1] = WVP.Row[7] - WVP.Row[5];
			Plane[2] = WVP.Row[11] - WVP.Row[9];
			Plane[3] = WVP.Row[15] - WVP.Row[13];

			Plane[3] /= sqrtf(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= -Radius)
				return Plane[3];

			Plane[0] = WVP.Row[3] + WVP.Row[2];
			Plane[1] = WVP.Row[7] + WVP.Row[6];
			Plane[2] = WVP.Row[11] + WVP.Row[10];
			Plane[3] = WVP.Row[15] + WVP.Row[14];

			Plane[3] /= sqrtf(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= -Radius)
				return Plane[3];

			Plane[0] = WVP.Row[3] - WVP.Row[2];
			Plane[1] = WVP.Row[7] - WVP.Row[6];
			Plane[2] = WVP.Row[11] - WVP.Row[10];
			Plane[3] = WVP.Row[15] - WVP.Row[14];

			Plane[3] /= sqrtf(Plane[0] * Plane[0] + Plane[1] * Plane[1] + Plane[2] * Plane[2]);
			if (Plane[3] <= -Radius)
				return Plane[3];

			return -1;
		}
		bool MathCommon::HasSphereIntersected(const Vector3& PositionR0, float RadiusR0, const Vector3& PositionR1, float RadiusR1)
		{
			if (PositionR0.Distance(PositionR1) < RadiusR0 + RadiusR1)
				return true;

			return false;
		}
		bool MathCommon::HasLineIntersected(float Distance0, float Distance1, const Vector3& Point0, const Vector3& Point1, Vector3& Hit)
		{
			if ((Distance0 * Distance1) >= 0)
				return false;

			if (Distance0 == Distance1)
				return false;

			Hit = Point0 + (Point1 - Point0) * (-Distance0 / (Distance1 - Distance0));
			return true;
		}
		bool MathCommon::HasLineIntersectedCube(const Vector3& Min, const Vector3& Max, const Vector3& Start, const Vector3& End)
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
		bool MathCommon::HasPointIntersectedCube(const Vector3& LastHit, const Vector3& Min, const Vector3& Max, int Axis)
		{
			if (Axis == 1 && LastHit.Z > Min.Z && LastHit.Z < Max.Z && LastHit.Y > Min.Y && LastHit.Y < Max.Y)
				return true;

			if (Axis == 2 && LastHit.Z > Min.Z && LastHit.Z < Max.Z && LastHit.X > Min.X && LastHit.X < Max.X)
				return true;

			if (Axis == 3 && LastHit.X > Min.X && LastHit.X < Max.X && LastHit.Y > Min.Y && LastHit.Y < Max.Y)
				return true;

			return false;
		}
		bool MathCommon::HasPointIntersectedCube(const Vector3& Position, const Vector3& Scale, const Vector3& P0)
		{
			return (P0.X) <= (Position.X + Scale.X) && (Position.X - Scale.X) <= (P0.X) && (P0.Y) <= (Position.Y + Scale.Y) && (Position.Y - Scale.Y) <= (P0.Y) && (P0.Z) <= (Position.Z + Scale.Z) && (Position.Z - Scale.Z) <= (P0.Z);
		}
		bool MathCommon::HasPointIntersectedRectangle(const Vector3& Position, const Vector3& Scale, const Vector3& P0)
		{
			return P0.X >= Position.X - Scale.X && P0.X < Position.X + Scale.X && P0.Y >= Position.Y - Scale.Y && P0.Y < Position.Y + Scale.Y;
		}
		bool MathCommon::HasSBIntersected(Transform* R0, Transform* R1)
		{
			if (!HasSphereIntersected(R0->Position, R0->Scale.Hypotenuse(), R1->Position, R1->Scale.Hypotenuse()))
				return false;

			return true;
		}
		bool MathCommon::HasOBBIntersected(Transform* R0, Transform* R1)
		{
			if (R1->Rotation + R0->Rotation == 0)
				return HasAABBIntersected(R0, R1);

			Matrix4x4 Temp0 = Matrix4x4::Create(R0->Position - R1->Position, R0->Scale, R0->Rotation) * Matrix4x4::CreateRotation(R1->Rotation).Invert();
			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(-1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(-1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(0, 1, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(0, -1, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(1, 0, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 0, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R1->Scale, R1->Scale, Vector4(0, 0, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(0, 0, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			Temp0 = Matrix4x4::Create(R1->Position - R0->Position, R1->Scale, R1->Rotation) * Matrix4x4::CreateRotation(R0->Rotation).Invert();
			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(-1, -1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, 1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(-1, 1, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(1, -1, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(0, 1, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(0, -1, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(1, 0, 0, 1).Transform(Temp0.Row).XYZ(), Vector4(-1, 0, 0, 1).Transform(Temp0.Row).XYZ()))
				return true;

			if (HasLineIntersectedCube(-R0->Scale, R0->Scale, Vector4(0, 0, 1, 1).Transform(Temp0.Row).XYZ(), Vector4(0, 0, -1, 1).Transform(Temp0.Row).XYZ()))
				return true;

			return false;
		}
		bool MathCommon::HasAABBIntersected(Transform* R0, Transform* R1)
		{
			return (R0->Position.X - R0->Scale.X) <= (R1->Position.X + R1->Scale.X) && (R1->Position.X - R1->Scale.X) <= (R0->Position.X + R0->Scale.X) && (R0->Position.Y - R0->Scale.Y) <= (R1->Position.Y + R1->Scale.Y) && (R1->Position.Y - R1->Scale.Y) <= (R0->Position.Y + R0->Scale.Y) && (R0->Position.Z - R0->Scale.Z) <= (R1->Position.Z + R1->Scale.Z) && (R1->Position.Z - R1->Scale.Z) <= (R0->Position.Z + R0->Scale.Z);
		}
		bool MathCommon::HexToString(void* Data, uint64_t Length, char* Buffer, uint64_t BufferLength)
		{
			if (!Data || !Length || !Buffer || !BufferLength)
				return false;

			if (BufferLength < (3 * Length))
				return false;

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
		bool MathCommon::Hex(char c, int& v)
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
		bool MathCommon::HexToDecimal(const std::string& s, uint64_t i, uint64_t cnt, int& Value)
		{
			if (i >= s.size())
				return false;

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
		void MathCommon::ComputeJointOrientation(Compute::Joint* Value, bool LeftHanded)
		{
			if (Value != nullptr)
			{
				ComputeMatrixOrientation(&Value->BindShape, LeftHanded);
				ComputeMatrixOrientation(&Value->Transform, LeftHanded);
				for (auto&& Child : Value->Childs)
					ComputeJointOrientation(&Child, LeftHanded);
			}
		}
		void MathCommon::ComputeMatrixOrientation(Compute::Matrix4x4* Matrix, bool LeftHanded)
		{
			Compute::Matrix4x4 Coord(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);

			if (!LeftHanded)
				Coord = Coord.Invert();

			if (Matrix != nullptr)
				*Matrix = *Matrix * Coord;
		}
		void MathCommon::ComputePositionOrientation(Compute::Vector3* Position, bool LeftHanded)
		{
			Compute::Matrix4x4 Coord(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);

			if (!LeftHanded)
				Coord = Coord.Invert();

			if (Position != nullptr)
				*Position = (Coord * Compute::Matrix4x4::CreateTranslation(*Position)).Position();
		}
		void MathCommon::ComputeIndexWindingOrderFlip(std::vector<int>& Indices)
		{
			std::reverse(Indices.begin(), Indices.end());
		}
		void MathCommon::ComputeVertexOrientation(std::vector<Vertex>& Vertices, bool LeftHanded)
		{
			Compute::Matrix4x4 Coord(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);

			if (!LeftHanded)
				Coord = Coord.Invert();

			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
			{
				Compute::Vector3 Position(It->PositionX, It->PositionY, It->PositionZ);
				Position = (Coord * Compute::Matrix4x4::CreateTranslation(Position)).Position();

				It->PositionX = Position.X;
				It->PositionY = Position.Y;
				It->PositionZ = Position.Z;
			}
		}
		void MathCommon::ComputeInfluenceOrientation(std::vector<SkinVertex>& Vertices, bool LeftHanded)
		{
			Compute::Matrix4x4 Coord(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);

			if (!LeftHanded)
				Coord = Coord.Invert();

			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
			{
				Compute::Vector3 Position(It->PositionX, It->PositionY, It->PositionZ);
				Position = (Coord * Compute::Matrix4x4::CreateTranslation(Position)).Position();

				It->PositionX = Position.X;
				It->PositionY = Position.Y;
				It->PositionZ = Position.Z;
			}
		}
		void MathCommon::ComputeInfluenceNormals(std::vector<SkinVertex>& Vertices)
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
		void MathCommon::ComputeInfluenceNormalsArray(SkinVertex* Vertices, uint64_t Count)
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
		void MathCommon::ComputeInfluenceTangentBitangent(SkinVertex V1, SkinVertex V2, SkinVertex V3, Vector3& Tangent, Vector3& Bitangent, Vector3& Normal)
		{
			Vector3 Face1 = Vector3(V2.PositionX - V1.PositionX, V2.PositionY - V1.PositionY, V2.PositionZ - V1.PositionZ);
			Vector3 Face2 = Vector3(V3.PositionX - V1.PositionX, V3.PositionY - V1.PositionY, V3.PositionZ - V1.PositionZ);
			Vector2 Coord1 = Vector2(V2.TexCoordX - V1.TexCoordX, V3.TexCoordX - V1.TexCoordX);
			Vector2 Coord2 = Vector2(V2.TexCoordY - V1.TexCoordY, V3.TexCoordY - V1.TexCoordY);

			float Ray = 1.0f / (Coord1.X * Coord2.Y - Coord1.Y * Coord2.X);

			Tangent.X = (Coord1.Y * Face1.X - Coord1.X * Face2.X) * Ray;
			Tangent.Y = (Coord1.Y * Face1.Y - Coord1.X * Face2.Y) * Ray;
			Tangent.Z = (Coord1.Y * Face1.Z - Coord1.X * Face2.Z) * Ray;
			Tangent = Tangent.NormalizeSafe();

			Bitangent.X = (Coord2.X * Face2.X - Coord2.Y * Face1.X) * Ray;
			Bitangent.Y = (Coord2.X * Face2.Y - Coord2.Y * Face1.Y) * Ray;
			Bitangent.Z = (Coord2.X * Face2.Z - Coord2.Y * Face1.Z) * Ray;
			Bitangent = Bitangent.NormalizeSafe();

			Normal.X = (Tangent.Y * Bitangent.Z) - (Tangent.Z * Bitangent.Y);
			Normal.Y = (Tangent.Z * Bitangent.X) - (Tangent.X * Bitangent.Z);
			Normal.Z = (Tangent.X * Bitangent.Y) - (Tangent.Y * Bitangent.X);
			Normal = -Normal.NormalizeSafe();
		}
		void MathCommon::ComputeInfluenceTangentBitangent(SkinVertex V1, SkinVertex V2, SkinVertex V3, Vector3& Tangent, Vector3& Bitangent)
		{
			Vector3 Face1 = Vector3(V2.PositionX - V1.PositionX, V2.PositionY - V1.PositionY, V2.PositionZ - V1.PositionZ);
			Vector3 Face2 = Vector3(V3.PositionX - V1.PositionX, V3.PositionY - V1.PositionY, V3.PositionZ - V1.PositionZ);
			Vector2 Coord1 = Vector2(V2.TexCoordX - V1.TexCoordX, V3.TexCoordX - V1.TexCoordX);
			Vector2 Coord2 = Vector2(V2.TexCoordY - V1.TexCoordY, V3.TexCoordY - V1.TexCoordY);

			float Ray = 1.0f / (Coord1.X * Coord2.Y - Coord1.Y * Coord2.X);

			Tangent.X = (Coord1.Y * Face1.X - Coord1.X * Face2.X) * Ray;
			Tangent.Y = (Coord1.Y * Face1.Y - Coord1.X * Face2.Y) * Ray;
			Tangent.Z = (Coord1.Y * Face1.Z - Coord1.X * Face2.Z) * Ray;
			Tangent = Tangent.NormalizeSafe();

			Bitangent.X = (Coord2.X * Face2.X - Coord2.Y * Face1.X) * Ray;
			Bitangent.Y = (Coord2.X * Face2.Y - Coord2.Y * Face1.Y) * Ray;
			Bitangent.Z = (Coord2.X * Face2.Z - Coord2.Y * Face1.Z) * Ray;
			Bitangent = Bitangent.NormalizeSafe();
		}
		void MathCommon::Randomize()
		{
			srand((unsigned int)time(nullptr));
#ifdef THAWK_HAS_OPENSSL
			RAND_poll();
#endif
		}
		void MathCommon::ConfigurateUnsafe(Transform* In, Matrix4x4* LocalTransform, Vector3* LocalPosition, Vector3* LocalRotation, Vector3* LocalScale)
		{
			if (!In)
				return;

			if (In->LocalTransform)
				delete In->LocalTransform;
			In->LocalTransform = LocalTransform;

			if (In->LocalPosition)
				delete In->LocalPosition;
			In->LocalPosition = LocalPosition;

			if (In->LocalRotation)
				delete In->LocalRotation;
			In->LocalRotation = LocalRotation;

			if (In->LocalScale)
				delete In->LocalScale;
			In->LocalScale = LocalScale;
		}
		void MathCommon::SetRootUnsafe(Transform* In, Transform* Root)
		{
			In->Root = Root;

			if (Root != nullptr)
				Root->AddChild(In);
		}
		void MathCommon::Sha1CollapseBufferBlock(unsigned int* Buffer)
		{
			for (int i = 16; --i >= 0;)
				Buffer[i] = 0;
		}
		void MathCommon::Sha1ComputeHashBlock(unsigned int* Result, unsigned int* W)
		{
			unsigned int A = Result[0];
			unsigned int B = Result[1];
			unsigned int C = Result[2];
			unsigned int D = Result[3];
			unsigned int E = Result[4];
			int R = 0;

#define Sha1Roll(A1, A2) ((A1 << A2) | (A1 >> (32 - A2)))
#define Sha1Make(F, V) {const unsigned int T=Sha1Roll(A,5)+(F)+E+V+W[R];E=D;D=C;C=Sha1Roll(B,30);B=A;A=T;R++;}

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
		void MathCommon::Sha1Compute(const void* Value, const int Length, unsigned char* Hash20)
		{
			unsigned int Result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
			const unsigned char* ValueCUC = (const unsigned char*)Value;
			unsigned int W[80];

			const int EndOfFullBlocks = Length - 64;
			int EndCurrentBlock, CurrentBlock = 0;

			while (CurrentBlock <= EndOfFullBlocks)
			{
				EndCurrentBlock = CurrentBlock + 64;
				for (int i = 0; CurrentBlock < EndCurrentBlock; CurrentBlock += 4)
				{
					W[i++] = (unsigned int)ValueCUC[CurrentBlock + 3] | (((unsigned int)ValueCUC[CurrentBlock + 2]) << 8) | (((unsigned int)ValueCUC[CurrentBlock + 1]) << 16) | (((unsigned int)ValueCUC[CurrentBlock]) << 24);
				}
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
		void MathCommon::Sha1Hash20ToHex(const unsigned char* Hash20, char* HexString)
		{
			const char Hex[] = { "0123456789abcdef" };
			for (int i = 20; --i >= 0;)
			{
				HexString[i << 1] = Hex[(Hash20[i] >> 4) & 0xf];
				HexString[(i << 1) + 1] = Hex[Hash20[i] & 0xf];
			}

			HexString[40] = 0;
		}
		bool MathCommon::IsBase64(unsigned char Value)
		{
			return (isalnum(Value) || (Value == '+') || (Value == '/'));
		}
		unsigned char MathCommon::RandomUC()
		{
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
			return Alphabet[rand() % (sizeof(Alphabet) - 1)];
		}
		int64_t MathCommon::RandomNumber(int64_t Min, int64_t Max)
		{
			int64_t Raw = 0;
#ifdef THAWK_HAS_OPENSSL
			RAND_bytes((unsigned char*)&Raw, sizeof(int64_t));
#else
			Raw = (int64_t)rand();
#endif
			return Min + (Raw % static_cast<int64_t>(Max - Min + 1));
		}
		uint64_t MathCommon::Utf8(int code, char* Buffer)
		{
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
		std::string MathCommon::Base10ToBaseN(uint64_t Value, unsigned int BaseLessThan65)
		{
			static const char* Base62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
			static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			if (Value == 0)
				return "0";

			if (BaseLessThan65 > 64 || BaseLessThan65 < 1)
				return "";

			const char* Base = (BaseLessThan65 == 64 ? Base64 : Base62);
			std::string Output;

			while (Value > 0)
			{
				Output.insert(0, 1, Base[Value % BaseLessThan65]);
				Value /= BaseLessThan65;
			}

			return Output;
		}
		std::string MathCommon::Encrypt(const std::string& Text, int Offset)
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
		std::string MathCommon::Decrypt(const std::string& Text, int Offset)
		{
			std::string Result;
			for (uint64_t i = 0; i < (uint64_t)Text.size(); i++)
			{
				if (Text[i] != 0)
					Result += char(Text[i] - Offset);
				else
					Result += " ";
			}

			return Result;
		}
		std::string MathCommon::BinToHex(const char* Value, size_t Size)
		{
			const char Hex[] = "0123456789abcdef";
			std::string Output;

			if (!Value)
				return Output;

			for (size_t i = 0; i < Size; i++)
			{
				unsigned char C = static_cast<unsigned char>(Value[i]);
				Output += Hex[C >> 4];
				Output += Hex[C & 0xf];
			}

			return Output;
		}
		std::string MathCommon::RandomBytes(uint64_t Length)
		{
#ifdef THAWK_HAS_OPENSSL
			unsigned char* Buffer = (unsigned char*)malloc(sizeof(unsigned char) * Length);
			RAND_bytes(Buffer, (int)Length);

			std::string Output((const char*)Buffer, Length);
			free(Buffer);

			return Output;
#else
			return "";
#endif
		}
		std::string MathCommon::MD5Hash(const std::string& Value)
		{
			MD5Hasher Hasher;
			Hasher.Update(Value);
			Hasher.Finalize();

			return Hasher.ToHex();
		}
		std::string MathCommon::Sha256Encode(const char* Value, const char* Key, const char* IV)
		{
#ifdef THAWK_HAS_OPENSSL
			EVP_CIPHER_CTX* Context;
			if (!Value || !Key || !IV || !(Context = EVP_CIPHER_CTX_new()))
				return "";

			if (1 != EVP_EncryptInit_ex(Context, EVP_aes_256_cbc(), nullptr, (const unsigned char*)Key, (const unsigned char*)IV))
			{
				EVP_CIPHER_CTX_free(Context);
				return "";
			}

			int Size1 = (int)strlen(Value), Size2 = 0;
			unsigned char* Buffer = new unsigned char[Size1 + 2048];

			if (1 != EVP_EncryptUpdate(Context, Buffer, &Size2, (const unsigned char*)Value, Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			if (1 != EVP_EncryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			std::string Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			delete[] Buffer;

			return Output;
#else
			return Value;
#endif
		}
		std::string MathCommon::Sha256Decode(const char* Value, const char* Key, const char* IV)
		{
#ifdef THAWK_HAS_OPENSSL
			EVP_CIPHER_CTX* Context;
			if (!Value || !Key || !IV || !(Context = EVP_CIPHER_CTX_new()))
				return "";

			if (1 != EVP_DecryptInit_ex(Context, EVP_aes_256_cbc(), nullptr, (const unsigned char*)Key, (const unsigned char*)IV))
			{
				EVP_CIPHER_CTX_free(Context);
				return "";
			}

			int Size1 = (int)strlen(Value), Size2 = 0;
			unsigned char* Buffer = new unsigned char[Size1 + 2048];

			if (1 != EVP_DecryptUpdate(Context, Buffer, &Size2, (const unsigned char*)Value, Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			if (1 != EVP_DecryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			std::string Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			delete[] Buffer;

			return Output;
		}
		std::string MathCommon::Aes256Encode(const std::string& Value, const char* Key, const char* IV)
		{
			EVP_CIPHER_CTX* Context;
			if (Value.empty() || !Key || !IV || !(Context = EVP_CIPHER_CTX_new()))
				return "";

			if (1 != EVP_EncryptInit_ex(Context, EVP_aes_256_cbc(), nullptr, (const unsigned char*)Key, (const unsigned char*)IV))
			{
				EVP_CIPHER_CTX_free(Context);
				return "";
			}

			int Size1 = (int)Value.size(), Size2 = 0;
			unsigned char* Buffer = new unsigned char[Size1 + 2048];

			if (1 != EVP_EncryptUpdate(Context, Buffer, &Size2, (const unsigned char*)Value.c_str(), Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			if (1 != EVP_EncryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			std::string Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			delete[] Buffer;

			return Output;
#else
			return Value;
#endif
		}
		std::string MathCommon::Aes256Decode(const std::string& Value, const char* Key, const char* IV)
		{
#ifdef THAWK_HAS_OPENSSL
			EVP_CIPHER_CTX* Context;
			if (Value.empty() || !Key || !IV || !(Context = EVP_CIPHER_CTX_new()))
				return "";

			if (1 != EVP_DecryptInit_ex(Context, EVP_aes_256_cbc(), nullptr, (const unsigned char*)Key, (const unsigned char*)IV))
			{
				EVP_CIPHER_CTX_free(Context);
				return "";
			}

			int Size1 = (int)Value.size(), Size2 = 0;
			unsigned char* Buffer = new unsigned char[Size1 + 2048];

			if (1 != EVP_DecryptUpdate(Context, Buffer, &Size2, (const unsigned char*)Value.c_str(), Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			if (1 != EVP_DecryptFinal_ex(Context, Buffer + Size2, &Size1))
			{
				EVP_CIPHER_CTX_free(Context);
				delete[] Buffer;
				return "";
			}

			std::string Output((const char*)Buffer, Size1 + Size2);
			EVP_CIPHER_CTX_free(Context);
			delete[] Buffer;

			return Output;
#else
			return Value;
#endif
		}
		std::string MathCommon::Base64Encode(const unsigned char* Value, uint64_t Length)
		{
			if (!Value)
				return "";

			std::string Encoded;
			unsigned char Row3[3];
			unsigned char Row4[4];
			int Offset = 0, Step = 0;

			std::string Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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
					Encoded += Base64[Row4[Offset]];

				Offset = 0;
			}

			if (!Offset)
				return Encoded;

			for (Step = Offset; Step < 3; Step++)
				Row3[Step] = '\0';

			Row4[0] = (Row3[0] & 0xfc) >> 2;
			Row4[1] = ((Row3[0] & 0x03) << 4) + ((Row3[1] & 0xf0) >> 4);
			Row4[2] = ((Row3[1] & 0x0f) << 2) + ((Row3[2] & 0xc0) >> 6);
			Row4[3] = Row3[2] & 0x3f;

			for (Step = 0; (Step < Offset + 1); Step++)
				Encoded += Base64[Row4[Step]];

			while (Offset++ < 3)
				Encoded += '=';

			return Encoded;

		}
		std::string MathCommon::Base64Encode(const std::string& Text)
		{
			std::string Encoded;
			unsigned char Row3[3];
			unsigned char Row4[4];
			const char* Value = Text.c_str();
			unsigned int Length = (unsigned int)Text.size();
			int Offset = 0, Step = 0;

			std::string Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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
					Encoded += Base64[Row4[Offset]];

				Offset = 0;
			}

			if (!Offset)
				return Encoded;

			for (Step = Offset; Step < 3; Step++)
				Row3[Step] = '\0';

			Row4[0] = (Row3[0] & 0xfc) >> 2;
			Row4[1] = ((Row3[0] & 0x03) << 4) + ((Row3[1] & 0xf0) >> 4);
			Row4[2] = ((Row3[1] & 0x0f) << 2) + ((Row3[2] & 0xc0) >> 6);
			Row4[3] = Row3[2] & 0x3f;

			for (Step = 0; (Step < Offset + 1); Step++)
				Encoded += Base64[Row4[Step]];

			while (Offset++ < 3)
				Encoded += '=';

			return Encoded;
		}
		std::string MathCommon::Base64Decode(const std::string& Value)
		{
			int Length = (int)Value.size();
			int Offset = 0, Step = 0;
			int Focus = 0;

			std::string Decoded;
			unsigned char Row4[4];
			unsigned char Row3[3];

			std::string Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			Rest::Stroke Base(&Base64);

			while (Length-- && (Value[Focus] != '=') && IsBase64(Value[Focus]))
			{
				Row4[Offset++] = Value[Focus];
				Focus++;
				if (Offset != 4)
					continue;

				for (Offset = 0; Offset < 4; Offset++)
					Row4[Offset] = (unsigned char)Base.Find(Row4[Offset]).Start;

				Row3[0] = (Row4[0] << 2) + ((Row4[1] & 0x30) >> 4);
				Row3[1] = ((Row4[1] & 0xf) << 4) + ((Row4[2] & 0x3c) >> 2);
				Row3[2] = ((Row4[2] & 0x3) << 6) + Row4[3];

				for (Offset = 0; (Offset < 3); Offset++)
					Decoded += Row3[Offset];

				Offset = 0;
			}

			if (!Offset)
				return Decoded;

			for (Step = Offset; Step < 4; Step++)
				Row4[Step] = 0;

			for (Step = 0; Step < 4; Step++)
				Row4[Step] = (unsigned char)Base.Find(Row4[Step]).Start;

			Row3[0] = (Row4[0] << 2) + ((Row4[1] & 0x30) >> 4);
			Row3[1] = ((Row4[1] & 0xf) << 4) + ((Row4[2] & 0x3c) >> 2);
			Row3[2] = ((Row4[2] & 0x3) << 6) + Row4[3];

			for (Step = 0; (Step < Offset - 1); Step++)
				Decoded += Row3[Step];

			return Decoded;
		}
		std::string MathCommon::URIEncode(const std::string& Text)
		{
			return URIEncode(Text.c_str(), Text.size());
		}
		std::string MathCommon::URIEncode(const char* Text, uint64_t Length)
		{
			if (!Text || !Length)
				return "";

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
		std::string MathCommon::URIDecode(const std::string& Text)
		{
			return URIDecode(Text.c_str(), Text.size());
		}
		std::string MathCommon::URIDecode(const char* Text, uint64_t Length)
		{
			if (!Text)
				return "";

			std::string Value;
			if (!Length)
				return Value;

			int i = 0;
			while (i < Length)
			{
				if (i < Length - 2 && Text[i] == '%' && isxdigit(*(const unsigned char*)(Text + i + 1)) && isxdigit(*(const unsigned char*)(Text + i + 2)))
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
		std::string MathCommon::Hybi10Encode(Hybi10Request Request, bool Masked)
		{
			std::string Stream;
			size_t Length = (size_t)Request.Payload.size();
			unsigned char Head = 0;

			switch (Request.GetEnumType())
			{
				case Hybi10_Opcode_Text:
					Head = 129;
					break;
				case Hybi10_Opcode_Close:
					Head = 136;
					break;
				case Hybi10_Opcode_Ping:
					Head = 137;
					break;
				case Hybi10_Opcode_Pong:
					Head = 138;
					break;
				default:
					break;
			}

			Stream += (char)Head;
			if (Length > 65535)
			{
				unsigned long Offset = (unsigned long)Length;
				Stream += (char)(unsigned char)(Masked ? 255 : 127);

				for (int i = 7; i > 0; i--)
				{
					unsigned char Block = 0;
					for (int j = 0; j < 8; j++)
					{
						unsigned char Shift = 0x01 << j;
						Shift = Shift << (8 * i);

						Block += (unsigned char)pow(2, j) * (Offset & Shift);
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
		std::string MathCommon::DecimalToHex(uint64_t n)
		{
			const char* Set = "0123456789abcdef";
			std::string Result;

			do
			{
				Result = Set[n & 15] + Result;
				n >>= 4;
			}while (n > 0);

			return Result;
		}
		Hybi10Request MathCommon::Hybi10Decode(const std::string& Value)
		{
			Hybi10PayloadHeader* Payload = (Hybi10PayloadHeader*)Value.substr(0, 2).c_str();
			Hybi10Request Decoded;
			Decoded.Type = Payload->Opcode;

			if (Decoded.GetEnumType() == Hybi10_Opcode_Invalid)
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
		Ray MathCommon::CreateCursorRay(const Vector3& Origin, const Vector2& Cursor, const Vector2& Screen, const Matrix4x4& InvProjection, const Matrix4x4& InvView)
		{
			Vector4 Eye = Vector4((2.0f * Cursor.X) / Screen.X - 1.0f, 1.0f - (2.0f * Cursor.Y) / Screen.Y, 1.0f, 1.0f) * InvProjection;

			Eye = (Vector4(Eye.X, Eye.Y, 1.0f, 0.0f) * InvView).NormalizeSafe();
			return Ray(Origin.InvertZ(), Vector3(Eye.X, Eye.Y, Eye.Z));
		}
		bool MathCommon::CursorRayTest(const Ray& Cursor, const Vector3& Position, const Vector3& Scale)
		{
			return Cursor.IntersectsAABB(Position.InvertZ(), Scale);
		}
		bool MathCommon::CursorRayTest(const Ray& Cursor, const Matrix4x4& World)
		{
			return Cursor.IntersectsOBB(World);
		}
		uint64_t MathCommon::CRC32(const std::string& Data)
		{
			return Rest::OS::CheckSum(Data);
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
			for (auto It = Defines.begin(); It != Defines.end(); It++)
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
		}
		bool Preprocessor::IsDefined(const char* Name) const
		{
			if (!Name)
				return false;

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

			Rest::Stroke Buffer(&Data);
			if (Features.Conditions && !ProcessBlockDirective(Buffer))
				return ReturnResult(false, Nesting);

			if (Features.Includes)
			{
				if (!Path.empty())
					Sets.push_back(Path);

				if (!ProcessIncludeDirective(Path, Buffer))
					return ReturnResult(false, Nesting);
			}

			if (Features.Pragmas && !ProcessPragmaDirective(Buffer))
				return ReturnResult(false, Nesting);

			uint64_t Offset;
			if (Features.Defines && !ProcessDefineDirective(Buffer, 0, Offset, true))
				return ReturnResult(false, Nesting);

			Buffer.Trim();
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
				Sets.clear();

			return Result;
		}
		bool Preprocessor::ProcessIncludeDirective(const std::string& Path, Rest::Stroke& Buffer)
		{
			if (Buffer.Empty())
				return true;

			Rest::Stroke::Settle Result;
			Result.Start = Result.End = 0;
			Result.Found = false;

			std::string Dir = Path.empty() ? Rest::OS::GetDirectory() : Rest::OS::FileDirectory(Path);
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
					THAWK_ERROR("%s: cannot process include directive", Path.c_str());
					return false;
				}

				Rest::Stroke Section(Buffer.Get() + Start, End - Start);
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
						THAWK_ERROR("%s: cannot find \"%s\"", Path.c_str(), Section.Get());
						return false;
					}

					if (!Output.empty() && !Process(File.Module, Output))
						return false;
				}

				Buffer.ReplacePart(Result.Start, End, Output);
				Result.End = Result.Start + 1;
			}
		}
		bool Preprocessor::ProcessPragmaDirective(Rest::Stroke& Buffer)
		{
			if (Buffer.Empty())
				return true;

			uint64_t Offset = 0;
			while (true)
			{
				uint64_t Base, Start, End;
				int R = FindDirective(Buffer, "#pragma", &Offset, &Base, &Start, &End);
				if (R < 0)
				{
					THAWK_ERROR("cannot process pragma directive");
					return false;
				}
				else if (R == 0)
					return true;

				Rest::Stroke Value(Buffer.Get() + Start, End - Start);
				if (Pragma && !Pragma(this, Value.Trim().Replace("  ", " ").R()))
				{
					THAWK_ERROR("cannot process pragma \"%s\" directive", Value.Get());
					return false;
				}

				Buffer.ReplacePart(Base, End, "");
			}
		}
		bool Preprocessor::ProcessBlockDirective(Rest::Stroke& Buffer)
		{
			if (Buffer.Empty())
				return true;

			uint64_t Offset = 0;
			while (true)
			{
				int R = FindBlockDirective(Buffer, Offset, false);
				if (R < 0)
				{
					THAWK_ERROR("cannot process ifdef/endif directive");
					return false;
				}
				else if (R == 0)
					return true;
			}
		}
		bool Preprocessor::ProcessDefineDirective(Rest::Stroke& Buffer, uint64_t Base, uint64_t& Offset, bool Endless)
		{
			if (Buffer.Empty())
				return true;

			uint64_t Size = 0;
			while (Endless || Base < Offset)
			{
				int R = FindDefineDirective(Buffer, Base, &Size);
				if (R < 0)
				{
					THAWK_ERROR("cannot process define directive");
					return false;
				}
				else if (R == 0)
					return true;

				Offset -= Size;
			}

			return true;
		}
		int Preprocessor::FindDefineDirective(Rest::Stroke& Buffer, uint64_t& Offset, uint64_t* Size)
		{
			uint64_t Base, Start, End;
			Offset--;
			int R = FindDirective(Buffer, "#define", &Offset, &Base, &Start, &End);
			if (R < 0)
			{
				THAWK_ERROR("cannot process define directive");
				return -1;
			}
			else if (R == 0)
				return 0;

			Rest::Stroke Value(Buffer.Get() + Start, End - Start);
			Define(Value.Trim().Replace("  ", " ").R());
			Buffer.ReplacePart(Base, End, "");

			if (Size != nullptr)
				*Size = End - Base;

			return 1;
		}
		int Preprocessor::FindBlockDirective(Rest::Stroke& Buffer, uint64_t& Offset, bool Nested)
		{
			uint64_t B1Start = 0, B1End = 0;
			uint64_t B2Start = 0, B2End = 0;
			uint64_t Start, End, Base, Size;
			uint64_t BaseOffset = Offset;
			bool Resolved = false;

			int R = FindDirective(Buffer, "#ifdef", &Offset, nullptr, &Start, &End);
			if (R < 0)
			{
				THAWK_ERROR("cannot parse ifdef block directive");
				return -1;
			}
			else if (R == 0)
				return 0;

			Base = Offset;
			ProcessDefineDirective(Buffer, BaseOffset, Base, false);
			Size = Offset - Base;
			Start -= Size;
			End -= Size;
			Offset = Base;

			Rest::Stroke Name(Buffer.Get() + Start, End - Start);
			Name.Trim().Replace("  ", " ");
			Resolved = IsDefined(Name.Get());
			Start = Offset - 1;

			if (Name.Get()[0] == '!')
			{
				Name.Substring(1);
				Resolved = !Resolved;
			}

			ResolveDirective:
			Rest::Stroke::Settle Cond = Buffer.Find('#', Offset);
			R = FindBlockNesting(Buffer, Cond, Offset, B1Start + B1End == 0 ? Resolved : !Resolved);
			if (R < 0)
			{
				THAWK_ERROR("cannot find endif directive of %s", Name.Get());
				return -1;
			}
			else if (R == 1)
			{
				int C = FindBlockDirective(Buffer, Offset, true);
				if (C == 0)
				{
					THAWK_ERROR("cannot process nested ifdef/endif directive of %s", Name.Get());
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
				Rest::Stroke Section(Buffer.Get() + B1Start, B1End - B1Start);
				if (B2Start + B2End != 0)
					Buffer.ReplacePart(Start, B2End + 6, Section.R());
				else
					Buffer.ReplacePart(Start, B1End + 6, Section.R());
				Offset = Start + Section.Size() - 1;
			}
			else if (B2Start + B2End != 0)
			{
				Rest::Stroke Section(Buffer.Get() + B2Start, B2End - B2Start);
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
		int Preprocessor::FindBlockNesting(Rest::Stroke& Buffer, Rest::Stroke::Settle& Hash, uint64_t& Offset, bool Resolved)
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
		int Preprocessor::FindDirective(Rest::Stroke& Buffer, const char* V, uint64_t* SOffset, uint64_t* SBase, uint64_t* SStart, uint64_t* SEnd)
		{
			auto Result = Buffer.Find(V, SOffset ? *SOffset : 0);
			if (!Result.Found)
				return 0;

			if (Result.Start > 0 && (Buffer.R()[Result.Start - 1] != '\n' && Buffer.R()[Result.Start - 1] != '\r'))
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
		IncludeResult Preprocessor::ResolveInclude(const IncludeDesc& Desc)
		{
			std::string Base;
			if (!Desc.From.empty())
				Base.assign(Rest::OS::FileDirectory(Desc.From));
			else
				Base.assign(Rest::OS::GetDirectory());

			IncludeResult Result;
			if (!Rest::Stroke(Desc.Path).StartsOf("/."))
			{
				if (Desc.Root.empty())
				{
					Result.Module = Desc.Path;
					Result.IsSystem = true;
					Result.IsFile = false;
					return Result;
				}

				Result.Module = Rest::OS::Resolve(Desc.Path, Desc.Root);
				if (Rest::OS::FileExists(Result.Module.c_str()))
				{
					Result.IsSystem = true;
					Result.IsFile = true;
					return Result;
				}

				for (auto It : Desc.Exts)
				{
					std::string File(Result.Module);
					if (Result.Module.empty())
						File.assign(Rest::OS::Resolve(Desc.Path + It, Desc.Root));
					else
						File.append(It);

					if (!Rest::OS::FileExists(File.c_str()))
						continue;

					Result.Module = File;
					Result.IsSystem = true;
					Result.IsFile = true;
					return Result;
				}

				Result.Module = Desc.Path;
				Result.IsSystem = true;
				Result.IsFile = false;
				return Result;
			}

			Result.Module = Rest::OS::Resolve(Desc.Path, Base);
			if (Rest::OS::FileExists(Result.Module.c_str()))
			{
				Result.IsSystem = false;
				Result.IsFile = true;
				return Result;
			}

			for (auto It : Desc.Exts)
			{
				std::string File(Result.Module);
				if (Result.Module.empty())
					File.assign(Rest::OS::Resolve(Desc.Path + It, Desc.Root));
				else
					File.append(It);

				if (!Rest::OS::FileExists(File.c_str()))
					continue;

				Result.Module = File;
				Result.IsSystem = false;
				Result.IsFile = true;
				return Result;
			}

			Result.Module.clear();
			Result.IsFile = false;
			Result.IsSystem = false;
			return Result;
		}

		Transform::Transform()
		{
			Root = nullptr;
			UserPointer = nullptr;
			LocalTransform = nullptr;
			LocalPosition = nullptr;
			LocalRotation = nullptr;
			LocalScale = nullptr;
			Position = 0.0f;
			Rotation = 0.0f;
			Childs = nullptr;
			Scale = 1;
			ConstantScale = false;
		}
		Transform::~Transform()
		{
			SetRoot(nullptr);
			RemoveChilds();
			UserPointer = nullptr;

			if (LocalTransform != nullptr)
			{
				delete LocalTransform;
				LocalTransform = nullptr;
			}

			if (LocalPosition != nullptr)
			{
				delete LocalPosition;
				LocalPosition = nullptr;
			}

			if (LocalRotation != nullptr)
			{
				delete LocalRotation;
				LocalRotation = nullptr;
			}

			if (LocalScale != nullptr)
			{
				delete LocalScale;
				LocalScale = nullptr;
			}
		}
		void Transform::Copy(Transform* Target)
		{
			if (!Target->Root)
			{
				if (LocalTransform != nullptr)
				{
					delete LocalTransform;
					LocalTransform = nullptr;
				}

				if (LocalPosition != nullptr)
				{
					delete LocalPosition;
					LocalPosition = nullptr;
				}

				if (LocalRotation != nullptr)
				{
					delete LocalRotation;
					LocalRotation = nullptr;
				}

				if (LocalScale != nullptr)
				{
					delete LocalScale;
					LocalScale = nullptr;
				}
				Root = nullptr;
			}
			else
			{
				LocalTransform = new Matrix4x4(*Target->LocalTransform);
				LocalPosition = new Vector3(*Target->LocalPosition);
				LocalRotation = new Vector3(*Target->LocalRotation);
				LocalScale = new Vector3(*Target->LocalScale);
				Root = Target->Root;
			}

			if (Childs)
			{
				delete Childs;
				Childs = nullptr;
			}

			if (Target->Childs)
				Childs = new std::vector<Transform*>(*Target->Childs);

			UserPointer = Target->UserPointer;
			Position = Target->Position;
			Rotation = Target->Rotation;
			Scale = Target->Scale;
			ConstantScale = Target->ConstantScale;
		}
		void Transform::Synchronize()
		{
			if (!Root)
				return;

			*LocalTransform = Matrix4x4::Create(*LocalPosition, *LocalRotation) * Root->GetWorldUnscaled();
			Position = LocalTransform->Position();
			Rotation = LocalTransform->Rotation();
			Scale = (ConstantScale ? *LocalScale : *LocalScale * Root->Scale);
		}
		void Transform::AddChild(Transform* Child)
		{
			if (Childs != nullptr)
			{
				for (auto It = Childs->begin(); It != Childs->end(); It++)
				{
					if (*It == Child)
						return;
				}
			}
			else
				Childs = new std::vector<Transform*>();

			Childs->push_back(Child);
		}
		void Transform::RemoveChild(Transform* Child)
		{
			if (!Childs)
				return;

			if (Child->Root == this)
				Child->SetRoot(nullptr);

			if (Childs->empty())
			{
				delete Childs;
				Childs = nullptr;
			}
		}
		void Transform::RemoveChilds()
		{
			if (!Childs)
				return;

			std::vector<Transform*> Array = *Childs;
			for (auto& Child : Array)
			{
				if (Child->Root == this)
					Child->SetRoot(nullptr);
			}

			delete Childs;
			Childs = nullptr;
		}
		void Transform::Localize(Vector3* _Position, Vector3* _Scale, Vector3* _Rotation)
		{
			if (!Root)
				return;

			Matrix4x4 Result = Matrix4x4::Create(_Position ? *_Position : 0, _Rotation ? *_Rotation : 0) * Root->GetWorldUnscaled().Invert();
			if (_Position != nullptr)
				*_Position = Result.Position();

			if (_Rotation != nullptr)
				*_Rotation = Result.Rotation();
			
			if (_Scale != nullptr)
				*_Scale = (ConstantScale ? *_Scale : *_Scale / Root->Scale);
		}
		void Transform::Globalize(Vector3* _Position, Vector3* _Scale, Vector3* _Rotation)
		{
			if (!Root)
				return;

			Matrix4x4 Result = Matrix4x4::Create(_Position ? *_Position : 0, _Rotation ? *_Rotation : 0) * Root->GetWorldUnscaled();
			if (_Position != nullptr)
				*_Position = Result.Position();

			if (_Rotation != nullptr)
				*_Rotation = Result.Rotation();
			
			if (_Scale != nullptr)
				*_Scale = (ConstantScale ? *_Scale : *_Scale * Root->Scale);
		}
		void Transform::SetTransform(TransformSpace Space, const Vector3& _Position, const Vector3& _Scale, const Vector3& _Rotation)
		{
			if (Root != nullptr && Space == TransformSpace_Global)
			{
				Vector3 P = _Position, R = _Rotation, S = _Scale;
				Localize(&P, &S, &R);

				*GetLocalPosition() = P;
				*GetLocalRotation() = R;
				*GetLocalScale() = S;
				Synchronize();
			}
			else
			{
				*GetLocalPosition() = _Position;
				*GetLocalRotation() = _Rotation;
				*GetLocalScale() = _Scale;
			}
		}
		void Transform::SetRoot(Transform* Parent)
		{
			if (!CanRootBeApplied(Parent))
				return;

			if (Root != nullptr)
			{
				GetRootBasis(&Position, &Scale, &Rotation);
				if (LocalTransform != nullptr)
				{
					delete LocalTransform;
					LocalTransform = nullptr;
				}

				if (LocalPosition != nullptr)
				{
					delete LocalPosition;
					LocalPosition = nullptr;
				}

				if (LocalRotation != nullptr)
				{
					delete LocalRotation;
					LocalRotation = nullptr;
				}

				if (LocalScale != nullptr)
				{
					delete LocalScale;
					LocalScale = nullptr;
				}

				if (Root->Childs != nullptr)
				{
					for (auto It = Root->Childs->begin(); It != Root->Childs->end(); It++)
					{
						if ((*It) == this)
						{
							Root->Childs->erase(It);
							break;
						}
					}

					if (Root->Childs->empty())
					{
						delete Root->Childs;
						Root->Childs = nullptr;
					}
				}
			}

			Root = Parent;
			if (!Root)
				return;

			Root->AddChild(this);
			LocalTransform = new Matrix4x4(Matrix4x4::Create(Position, Rotation) * Root->GetWorldUnscaled().Invert());
			LocalPosition = new Vector3(LocalTransform->Position());
			LocalRotation = new Vector3(LocalTransform->Rotation());
			LocalScale = new Vector3(ConstantScale ? Scale : Scale / Root->Scale);
		}
		void Transform::GetRootBasis(Vector3* _Position, Vector3* _Scale, Vector3* _Rotation)
		{
			if (!Root)
			{
				Matrix4x4 Result = Matrix4x4::Create(*LocalPosition, *LocalRotation) * Root->GetWorldUnscaled();
				if (_Position != nullptr)
					*_Position = Result.Position();

				if (_Rotation != nullptr)
					*_Rotation = Result.Rotation();

				if (_Scale != nullptr)
					*_Scale = (ConstantScale ? *LocalScale : *LocalScale * Root->Scale);
			}
			else
			{
				if (_Position != nullptr)
					*_Position = Position;

				if (_Rotation != nullptr)
					*_Rotation = Rotation;

				if (_Scale != nullptr)
					*_Scale = Scale;
			}
		}
		bool Transform::HasRoot(Transform* Target)
		{
			Compute::Transform* UpperRoot = Root;
			while (true)
			{
				if (!UpperRoot)
					return false;

				if (UpperRoot == Target)
					return true;

				UpperRoot = UpperRoot->GetRoot();
			}

			return false;
		}
		bool Transform::HasChild(Transform* Target)
		{
			if (!Childs)
				return false;

			for (auto It = Childs->begin(); It != Childs->end(); It++)
			{
				if (*It == Target)
					return true;

				if ((*It)->HasChild(Target))
					return true;
			}

			return false;
		}
		std::vector<Transform*>* Transform::GetChilds()
		{
			return Childs;
		}
		Transform* Transform::GetChild(uint64_t Child)
		{
			if (!Childs)
				return nullptr;

			if (Child < 0 || Child >= Childs->size())
				return nullptr;

			return (*Childs)[Child];
		}
		Vector3* Transform::GetLocalPosition()
		{
			if (!Root)
				return &Position;

			return LocalPosition;
		}
		Vector3* Transform::GetLocalRotation()
		{
			if (!Root)
				return &Rotation;

			return LocalRotation;
		}
		Vector3* Transform::GetLocalScale()
		{
			if (!Root)
				return &Scale;

			return LocalScale;
		}
		Vector3 Transform::Up()
		{
			if (Root)
				return LocalTransform->Up();

			return Matrix4x4::Create(Position, Scale, Rotation).Up();
		}
		Vector3 Transform::Right()
		{
			if (Root)
				return LocalTransform->Right();

			return Matrix4x4::Create(Position, Scale, Rotation).Right();
		}
		Vector3 Transform::Forward()
		{
			if (Root)
				return LocalTransform->Forward();

			return Matrix4x4::Create(Position, Scale, Rotation).Forward();
		}
		Vector3 Transform::Direction(const Vector3& Direction)
		{
			return Matrix4x4::CreateLookAt(0, Direction, Vector3::Up()).Rotation();
		}
		Transform* Transform::GetRoot()
		{
			return Root;
		}
		Transform* Transform::GetUpperRoot()
		{
			Compute::Transform* UpperRoot = Root;

			while (true)
			{
				if (!UpperRoot)
					return nullptr;

				if (!UpperRoot->GetRoot())
					return UpperRoot;

				UpperRoot = UpperRoot->GetRoot();
			}

			return nullptr;
		}
		Matrix4x4 Transform::GetWorld()
		{
			if (Root)
				return Matrix4x4::CreateScale(Scale) * *LocalTransform;

			return Matrix4x4::Create(Position, Scale, Rotation);
		}
		Matrix4x4 Transform::GetWorld(const Vector3& Rescale)
		{
			if (Root)
				return Matrix4x4::CreateScale(Scale * Rescale) * *LocalTransform;

			return Matrix4x4::Create(Position, Scale * Rescale, Rotation);
		}
		Matrix4x4 Transform::GetWorldUnscaled()
		{
			if (Root)
				return *LocalTransform;

			return Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		Matrix4x4 Transform::GetWorldUnscaled(const Vector3& Rescale)
		{
			if (Root)
				return Matrix4x4::CreateScale(Rescale) * *LocalTransform;

			return Matrix4x4::CreateScale(Rescale) * Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		Matrix4x4 Transform::GetLocal()
		{
			if (Root)
				return Matrix4x4::Create(*LocalPosition, Scale, *LocalRotation);

			return Matrix4x4::Create(Position, Scale, Rotation);
		}
		Matrix4x4 Transform::GetLocal(const Vector3& Rescale)
		{
			if (Root)
				return Matrix4x4::Create(*LocalPosition, Scale * Rescale, *LocalRotation);

			return Matrix4x4::Create(Position, Scale * Rescale, Rotation);
		}
		Matrix4x4 Transform::GetLocalUnscaled()
		{
			if (Root)
				return Matrix4x4::CreateRotation(*LocalRotation) * Matrix4x4::CreateTranslation(*LocalPosition);

			return Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		Matrix4x4 Transform::GetLocalUnscaled(const Vector3& Rescale)
		{
			if (Root)
				return Matrix4x4::CreateScale(Rescale) * Matrix4x4::CreateRotation(*LocalRotation) * Matrix4x4::CreateTranslation(*LocalPosition);

			return Matrix4x4::CreateScale(Rescale) * Matrix4x4::CreateRotation(Rotation) * Matrix4x4::CreateTranslation(Position);
		}
		uint64_t Transform::GetChildCount()
		{
			if (!Childs)
				return 0;

			return Childs->size();
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

		RigidBody::RigidBody(Simulator* Refer, const Desc& I) : Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
			if (!Initial.Shape || !Engine)
				return;

			Initial.Shape = Engine->ReuseShape(Initial.Shape);
			if (!Initial.Shape)
			{
				Initial.Shape = Engine->TryCloneShape(I.Shape);
				if (!Initial.Shape)
					return;
			}

			btVector3 LocalInertia(0, 0, 0);
			Initial.Shape->setLocalScaling(BtV3(Initial.Scale));
			if (Initial.Mass > 0)
				Initial.Shape->calculateLocalInertia(Initial.Mass, LocalInertia);

			btQuaternion Rotation;
			Rotation.setEulerZYX(Initial.Rotation.Z, Initial.Rotation.Y, Initial.Rotation.X);

			btTransform BtTransform(Rotation, btVector3(Initial.Position.X, Initial.Position.Y, Initial.Position.Z));
			btRigidBody::btRigidBodyConstructionInfo Info(Initial.Mass, new btDefaultMotionState(BtTransform), Initial.Shape, LocalInertia);
			Instance = new btRigidBody(Info);
			Instance->setUserPointer(this);
			Instance->setGravity(Engine->GetWorld()->getGravity());

			if (Initial.Anticipation > 0 && Initial.Mass > 0)
			{
				Instance->setCcdMotionThreshold(Initial.Anticipation);
				Instance->setCcdSweptSphereRadius(Initial.Scale.Length() / 15.0f);
			}

			if (Instance->getWorldArrayIndex() == -1)
				Engine->GetWorld()->addRigidBody(Instance);
		}
		RigidBody::~RigidBody()
		{
			if (!Instance)
				return;

			if (Instance->getMotionState())
			{
				btMotionState* Object = Instance->getMotionState();
				delete Object;
				Instance->setMotionState(nullptr);
			}

			Instance->setCollisionShape(nullptr);
			Instance->setUserPointer(nullptr);
			if (Instance->getWorldArrayIndex() >= 0)
				Engine->GetWorld()->removeRigidBody(Instance);

			if (Initial.Shape)
				Engine->FreeShape(&Initial.Shape);

			delete Instance;
		}
		RigidBody* RigidBody::Copy()
		{
			if (!Instance)
				return nullptr;

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
			if (Instance)
				Instance->applyCentralImpulse(BtV3(Velocity));
		}
		void RigidBody::Push(const Vector3& Velocity, const Vector3& Torque)
		{
			if (Instance)
			{
				Instance->applyCentralImpulse(BtV3(Velocity));
				Instance->applyTorqueImpulse(BtV3(Torque));
			}
		}
		void RigidBody::Push(const Vector3& Velocity, const Vector3& Torque, const Vector3& Center)
		{
			if (Instance)
			{
				Instance->applyImpulse(BtV3(Velocity), BtV3(Center));
				Instance->applyTorqueImpulse(BtV3(Torque));
			}
		}
		void RigidBody::PushKinematic(const Vector3& Velocity)
		{
			if (Instance)
			{
				btTransform Offset;
				Instance->getMotionState()->getWorldTransform(Offset);

				btVector3 Origin = Offset.getOrigin();
				Origin.setX(Origin.getX() + Velocity.X);
				Origin.setY(Origin.getY() + Velocity.Y);
				Origin.setZ(Origin.getZ() + Velocity.Z);

				Offset.setOrigin(Origin);
				Instance->getMotionState()->setWorldTransform(Offset);
			}
		}
		void RigidBody::PushKinematic(const Vector3& Velocity, const Vector3& Torque)
		{
			if (Instance)
			{
				btTransform Offset;
				Instance->getMotionState()->getWorldTransform(Offset);

				Vector3 Rotation = Matrix4x4(&Offset).Rotation();
				Offset.getBasis().setEulerZYX(Rotation.Z + Torque.Z, Rotation.Y + Torque.Y, Rotation.X + Torque.X);

				btVector3 Origin = Offset.getOrigin();
				Origin.setX(Origin.getX() + Velocity.X);
				Origin.setY(Origin.getY() + Velocity.Y);
				Origin.setZ(Origin.getZ() + Velocity.Z);

				Offset.setOrigin(Origin);
				Instance->getMotionState()->setWorldTransform(Offset);
			}
		}
		void RigidBody::Synchronize(Transform* Transform, bool Kinematic)
		{
			if (!Instance)
				return;

			btTransform& Base = Instance->getWorldTransform();
			if (!Kinematic)
			{
				btVector3 Position = Base.getOrigin();
				btVector3 Scale = Instance->getCollisionShape()->getLocalScaling();
				btScalar X, Y, Z;

				Base.getRotation().getEulerZYX(Z, Y, X);
				Transform->SetTransform(TransformSpace_Global, V3Bt(Position), V3Bt(Scale), Vector3(-X, -Y, Z));
			}
			else
			{
				Base.setOrigin(btVector3(Transform->Position.X, Transform->Position.Y, Transform->Position.Z));
				Base.getBasis().setEulerZYX(Transform->Rotation.X, Transform->Rotation.Y, Transform->Rotation.Z);
				Instance->getCollisionShape()->setLocalScaling(BtV3(Transform->Scale));
			}
		}
		void RigidBody::SetActivity(bool Active)
		{
			if (!Instance || GetActivationState() == MotionState_Disable_Deactivation)
				return;

			if (Active)
			{
				Instance->forceActivationState(MotionState_Active);
				Instance->activate(true);
			}
			else
				Instance->forceActivationState(MotionState_Deactivation_Needed);
		}
		void RigidBody::SetAsGhost()
		{
			if (Instance)
				Instance->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
		}
		void RigidBody::SetAsNormal()
		{
			if (Instance)
				Instance->setCollisionFlags(0);
		}
		void RigidBody::SetSelfPointer()
		{
			if (Instance)
				Instance->setUserPointer(this);
		}
		void RigidBody::SetWorldTransform(btTransform* Value)
		{
			if (Instance && Value)
				Instance->setWorldTransform(*Value);
		}
		void RigidBody::SetActivationState(MotionState Value)
		{
			if (Instance)
				Instance->forceActivationState((int)Value);
		}
		void RigidBody::SetAngularDamping(float Value)
		{
			if (Instance)
				Instance->setDamping(Instance->getLinearDamping(), Value);
		}
		void RigidBody::SetAngularSleepingThreshold(float Value)
		{
			if (Instance)
				Instance->setSleepingThresholds(Instance->getLinearSleepingThreshold(), Value);
		}
		void RigidBody::SetFriction(float Value)
		{
			if (Instance)
				Instance->setFriction(Value);
		}
		void RigidBody::SetRestitution(float Value)
		{
			if (Instance)
				Instance->setRestitution(Value);
		}
		void RigidBody::SetSpinningFriction(float Value)
		{
			if (Instance)
				Instance->setSpinningFriction(Value);
		}
		void RigidBody::SetContactStiffness(float Value)
		{
			if (Instance)
				Instance->setContactStiffnessAndDamping(Value, Instance->getContactDamping());
		}
		void RigidBody::SetContactDamping(float Value)
		{
			if (Instance)
				Instance->setContactStiffnessAndDamping(Instance->getContactStiffness(), Value);
		}
		void RigidBody::SetHitFraction(float Value)
		{
			if (Instance)
				Instance->setHitFraction(Value);
		}
		void RigidBody::SetLinearDamping(float Value)
		{
			if (Instance)
				Instance->setDamping(Value, Instance->getAngularDamping());
		}
		void RigidBody::SetLinearSleepingThreshold(float Value)
		{
			if (Instance)
				Instance->setSleepingThresholds(Value, Instance->getAngularSleepingThreshold());
		}
		void RigidBody::SetCcdMotionThreshold(float Value)
		{
			if (Instance)
				Instance->setCcdMotionThreshold(Value);
		}
		void RigidBody::SetCcdSweptSphereRadius(float Value)
		{
			if (Instance)
				Instance->setCcdSweptSphereRadius(Value);
		}
		void RigidBody::SetContactProcessingThreshold(float Value)
		{
			if (Instance)
				Instance->setContactProcessingThreshold(Value);
		}
		void RigidBody::SetDeactivationTime(float Value)
		{
			if (Instance)
				Instance->setDeactivationTime(Value);
		}
		void RigidBody::SetRollingFriction(float Value)
		{
			if (Instance)
				Instance->setRollingFriction(Value);
		}
		void RigidBody::SetAngularFactor(const Vector3& Value)
		{
			if (Instance)
				Instance->setAngularFactor(BtV3(Value));
		}
		void RigidBody::SetAnisotropicFriction(const Vector3& Value)
		{
			if (Instance)
				Instance->setAnisotropicFriction(BtV3(Value));
		}
		void RigidBody::SetGravity(const Vector3& Value)
		{
			if (Instance)
				Instance->setGravity(BtV3(Value));
		}
		void RigidBody::SetLinearFactor(const Vector3& Value)
		{
			if (Instance)
				Instance->setLinearFactor(BtV3(Value));
		}
		void RigidBody::SetLinearVelocity(const Vector3& Value)
		{
			if (Instance)
				Instance->setLinearVelocity(BtV3(Value));
		}
		void RigidBody::SetAngularVelocity(const Vector3& Value)
		{
			if (Instance)
				Instance->setAngularVelocity(BtV3(Value));
		}
		void RigidBody::SetCollisionShape(btCollisionShape* Shape, Transform* Transform)
		{
			if (!Instance)
				return;

			btCollisionShape* Collision = Instance->getCollisionShape();
			if (Collision != nullptr)
				delete Collision;

			Instance->setCollisionShape(Shape);
			if (Transform)
				Synchronize(Transform, true);
		}
		void RigidBody::SetMass(float Mass)
		{
			if (!Instance || !Engine)
				return;

			btVector3 Inertia = Engine->GetWorld()->getGravity();
			if (Instance->getWorldArrayIndex() >= 0)
				Engine->GetWorld()->removeRigidBody(Instance);

			Instance->setGravity(Inertia);
			Instance->getCollisionShape()->calculateLocalInertia(Mass, Inertia);
			Instance->setMassProps(Mass, Inertia);
			if (Instance->getWorldArrayIndex() == -1)
				Engine->GetWorld()->addRigidBody(Instance);

			SetActivity(true);
		}
		void RigidBody::SetCollisionFlags(uint64_t Flags)
		{
			if (Instance)
				Instance->setCollisionFlags((int)Flags);
		}
		MotionState RigidBody::GetActivationState()
		{
			if (!Instance)
				return MotionState_Active;

			return (MotionState)Instance->getActivationState();
		}
		Shape RigidBody::GetCollisionShapeType()
		{
			if (!Instance || !Instance->getCollisionShape())
				return Shape_Invalid;

			return (Shape)Instance->getCollisionShape()->getShapeType();
		}
		Vector3 RigidBody::GetAngularFactor()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getAngularFactor();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetAnisotropicFriction()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getAnisotropicFriction();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetGravity()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getGravity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetLinearFactor()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getLinearFactor();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetLinearVelocity()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getLinearVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetAngularVelocity()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getAngularVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetScale()
		{
			if (!Instance || !Instance->getCollisionShape())
				return Vector3(1, 1, 1);

			btVector3 Value = Instance->getCollisionShape()->getLocalScaling();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetPosition()
		{
			btVector3 Value = Instance->getWorldTransform().getOrigin();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 RigidBody::GetRotation()
		{
			btScalar X, Y, Z;
			Instance->getWorldTransform().getBasis().getEulerZYX(Z, Y, X);

			return Vector3(-X, -Y, Z);
		}
		btTransform* RigidBody::GetWorldTransform()
		{
			return &Instance->getWorldTransform();
		}
		btCollisionShape* RigidBody::GetCollisionShape()
		{
			if (!Instance)
				return nullptr;

			return Instance->getCollisionShape();
		}
		btRigidBody* RigidBody::Bullet()
		{
			return Instance;
		}
		bool RigidBody::IsGhost()
		{
			if (!Instance)
				return false;

			return (Instance->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
		}
		bool RigidBody::IsActive()
		{
			if (!Instance)
				return false;

			return Instance->isActive();
		}
		bool RigidBody::IsStatic()
		{
			if (!Instance)
				return false;

			return Instance->isStaticObject();
		}
		bool RigidBody::IsColliding()
		{
			if (!Instance)
				return false;

			return Instance->hasContactResponse();
		}
		float RigidBody::GetSpinningFriction()
		{
			if (!Instance)
				return 0;

			return Instance->getSpinningFriction();
		}
		float RigidBody::GetContactStiffness()
		{
			if (!Instance)
				return 0;

			return Instance->getContactStiffness();
		}
		float RigidBody::GetContactDamping()
		{
			if (!Instance)
				return 0;

			return Instance->getContactDamping();
		}
		float RigidBody::GetAngularDamping()
		{
			if (!Instance)
				return 0;

			return Instance->getAngularDamping();
		}
		float RigidBody::GetAngularSleepingThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getAngularSleepingThreshold();
		}
		float RigidBody::GetFriction()
		{
			if (!Instance)
				return 0;

			return Instance->getFriction();
		}
		float RigidBody::GetRestitution()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitution();
		}
		float RigidBody::GetHitFraction()
		{
			if (!Instance)
				return 0;

			return Instance->getHitFraction();
		}
		float RigidBody::GetLinearDamping()
		{
			if (!Instance)
				return 0;

			return Instance->getLinearDamping();
		}
		float RigidBody::GetLinearSleepingThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getLinearSleepingThreshold();
		}
		float RigidBody::GetCcdMotionThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getCcdMotionThreshold();
		}
		float RigidBody::GetCcdSweptSphereRadius()
		{
			if (!Instance)
				return 0;

			return Instance->getCcdSweptSphereRadius();
		}
		float RigidBody::GetContactProcessingThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getContactProcessingThreshold();
		}
		float RigidBody::GetDeactivationTime()
		{
			if (!Instance)
				return 0;

			return Instance->getDeactivationTime();
		}
		float RigidBody::GetRollingFriction()
		{
			if (!Instance)
				return 0;

			return Instance->getRollingFriction();
		}
		float RigidBody::GetMass()
		{
			if (Instance && Instance->getInvMass() != 0)
				return 1.0f / Instance->getInvMass();

			return 0;
		}
		uint64_t RigidBody::GetCollisionFlags()
		{
			if (!Instance)
				return 0;

			return Instance->getCollisionFlags();
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
			if (!From)
				return nullptr;

			return (RigidBody*)From->getUserPointer();
		}

		SoftBody::SoftBody(Simulator* Refer, const Desc& I) : Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
			if (!Engine || !Engine->HasSoftBodySupport())
				return;

			btQuaternion Rotation;
			Rotation.setEulerZYX(Initial.Rotation.Z, Initial.Rotation.Y, Initial.Rotation.X);

			btTransform BtTransform(Rotation, btVector3(Initial.Position.X, Initial.Position.Y, -Initial.Position.Z));
			btSoftRigidDynamicsWorld* World = (btSoftRigidDynamicsWorld*)Engine->GetWorld();
			btSoftBodyWorldInfo& Info = World->getWorldInfo();

			if (Initial.Shape.Convex.Enabled && Initial.Shape.Convex.Hull)
			{
				UnmanagedShape* Hull = Initial.Shape.Convex.Hull;
				std::vector<btScalar> Vertices;

				Vertices.resize(Hull->Vertices.size() * 3);
				for (size_t i = 0; i < Hull->Vertices.size() * 3; i += 3)
				{
					Vertex& V = Hull->Vertices[i / 3];
					Vertices[i + 0] = V.PositionX;
					Vertices[i + 1] = V.PositionY;
					Vertices[i + 2] = V.PositionZ;
				}

				Instance = btSoftBodyHelpers::CreateFromTriMesh(Info, Vertices.data(), Hull->Indices.data(), (int)Hull->Indices.size() / 3);
			}
			else if (Initial.Shape.Ellipsoid.Enabled)
			{
				Instance = btSoftBodyHelpers::CreateEllipsoid(Info, BtV3(Initial.Shape.Ellipsoid.Center), BtV3(Initial.Shape.Ellipsoid.Radius), Initial.Shape.Ellipsoid.Count);
			}
			else if (Initial.Shape.Rope.Enabled)
			{
				int FixedAnchors = 0;
				if (Initial.Shape.Rope.StartFixed)
					FixedAnchors |= 1;

				if (Initial.Shape.Rope.EndFixed)
					FixedAnchors |= 2;

				Instance = btSoftBodyHelpers::CreateRope(Info, BtV3(Initial.Shape.Rope.Start), BtV3(Initial.Shape.Rope.End), Initial.Shape.Rope.Count, FixedAnchors);
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

				Instance = btSoftBodyHelpers::CreatePatch(Info, BtV3(Initial.Shape.Patch.Corner00), BtV3(Initial.Shape.Patch.Corner10), BtV3(Initial.Shape.Patch.Corner01), BtV3(Initial.Shape.Patch.Corner11), Initial.Shape.Patch.CountX, Initial.Shape.Patch.CountY, FixedCorners, Initial.Shape.Patch.GenerateDiagonals);
			}

			Instance->setUserPointer(this);
			if (Initial.Anticipation > 0)
			{
				Instance->setCcdMotionThreshold(Initial.Anticipation);
				Instance->setCcdSweptSphereRadius(Initial.Scale.Length() / 15.0f);
			}

			SetConfig(Initial.Config);
			Instance->transform(BtTransform);
			Instance->setPose(true, true);
			if (Instance->getWorldArrayIndex() == -1)
				World->addSoftBody(Instance);
		}
		SoftBody::~SoftBody()
		{
			if (!Instance)
				return;

			btSoftRigidDynamicsWorld* World = (btSoftRigidDynamicsWorld*)Engine->GetWorld();
			if (Instance->getWorldArrayIndex() >= 0)
				World->removeSoftBody(Instance);

			Instance->setUserPointer(nullptr);
			delete Instance;
		}
		SoftBody* SoftBody::Copy()
		{
			if (!Instance)
				return nullptr;

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
			if (Instance)
				Instance->activate(Force);
		}
		void SoftBody::Synchronize(Transform* Transform, bool Kinematic)
		{
			if (!Instance)
				return;

			Center.Set(0);
			for (int i = 0; i < Instance->m_nodes.size(); i++)
			{
				auto& Node = Instance->m_nodes[i];
				Center.X += Node.m_x.x();
				Center.Y += Node.m_x.y();
				Center.Z += Node.m_x.z();
			}

			Center /= (float)Instance->m_nodes.size();
			Center = Center.InvertZ();

			if (!Kinematic)
			{
				btScalar X, Y, Z;
				Instance->getWorldTransform().getRotation().getEulerZYX(Z, Y, X);
				Transform->SetTransform(TransformSpace_Global, Center, 1.0f, Vector3(-X, -Y, Z));
			}
			else
			{
				Vector3 Position = Transform->Position.InvertZ() - Center.InvertZ();
				if (Position.Length() > 0.005f)
					Instance->translate(BtV3(Position));
			}
		}
		void SoftBody::Reindex(std::vector<int>* Indices)
		{
			if (!Instance || !Indices)
				return;

			std::map<btSoftBody::Node*, int> Nodes;
			for (int i = 0; i < Instance->m_nodes.size(); i++)
				Nodes.insert(std::make_pair(&Instance->m_nodes[i], i));

			for (int i = 0; i < Instance->m_faces.size(); i++)
			{
				btSoftBody::Face& Face = Instance->m_faces[i];
				for (int j = 0; j < 3; j++)
				{
					auto It = Nodes.find(Face.m_n[j]);
					if (It != Nodes.end())
						Indices->push_back(It->second);
				}
			}
		}
		void SoftBody::Retrieve(std::vector<Vertex>* Vertices)
		{
			if (!Instance || !Vertices)
				return;

			size_t Size = (size_t)Instance->m_nodes.size();
			if (Vertices->size() != Size)
				Vertices->resize(Size);

			for (size_t i = 0; i < Size; i++)
			{
				auto* Node = &Instance->m_nodes[i];
				Vertex& Position = Vertices->at(i);
				Position.PositionX = Node->m_x.x();
				Position.PositionY = Node->m_x.y();
				Position.PositionZ = Node->m_x.z();
				Position.NormalX = Node->m_n.x();
				Position.NormalY = Node->m_n.y();
				Position.NormalZ = Node->m_n.z();
			}
		}
		void SoftBody::Update(std::vector<Vertex>* Vertices)
		{
			if (!Instance || !Vertices)
				return;

			if (Vertices->size() != Instance->m_nodes.size())
				return;

			for (int i = 0; i < Instance->m_nodes.size(); i++)
			{
				auto* Node = &Instance->m_nodes[i];
				Vertex& Position = Vertices->at(i);
				Position.PositionX = Node->m_x.x();
				Position.PositionY = Node->m_x.y();
				Position.PositionZ = Node->m_x.z();
				Position.NormalX = Node->m_n.x();
				Position.NormalY = Node->m_n.y();
				Position.NormalZ = Node->m_n.z();
			}
		}
		void SoftBody::SetContactStiffnessAndDamping(float Stiffness, float Damping)
		{
			if (Instance)
				Instance->setContactStiffnessAndDamping(Stiffness, Damping);
		}
		void SoftBody::AddAnchor(int Node, RigidBody* Body, bool DisableCollisionBetweenLinkedBodies, float Influence)
		{
			if (Instance && Body)
				Instance->appendAnchor(Node, Body->Bullet(), DisableCollisionBetweenLinkedBodies, Influence);
		}
		void SoftBody::AddAnchor(int Node, RigidBody* Body, const Vector3& LocalPivot, bool DisableCollisionBetweenLinkedBodies, float Influence)
		{
			if (Instance && Body)
				Instance->appendAnchor(Node, Body->Bullet(), BtV3(LocalPivot), DisableCollisionBetweenLinkedBodies, Influence);
		}
		void SoftBody::AddForce(const Vector3& Force)
		{
			if (Instance)
				Instance->addForce(BtV3(Force));
		}
		void SoftBody::AddForce(const Vector3& Force, int Node)
		{
			if (Instance)
				Instance->addForce(BtV3(Force), Node);
		}
		void SoftBody::AddAeroForceToNode(const Vector3& WindVelocity, int NodeIndex)
		{
			if (Instance)
				Instance->addAeroForceToNode(BtV3(WindVelocity), NodeIndex);
		}
		void SoftBody::AddAeroForceToFace(const Vector3& WindVelocity, int FaceIndex)
		{
			if (Instance)
				Instance->addAeroForceToFace(BtV3(WindVelocity), FaceIndex);
		}
		void SoftBody::AddVelocity(const Vector3& Velocity)
		{
			if (Instance)
				Instance->addVelocity(BtV3(Velocity));
		}
		void SoftBody::SetVelocity(const Vector3& Velocity)
		{
			if (Instance)
				Instance->setVelocity(BtV3(Velocity));
		}
		void SoftBody::AddVelocity(const Vector3& Velocity, int Node)
		{
			if (Instance)
				Instance->addVelocity(BtV3(Velocity), Node);
		}
		void SoftBody::SetMass(int Node, float Mass)
		{
			if (Instance)
				Instance->setMass(Node, Mass);
		}
		void SoftBody::SetTotalMass(float Mass, bool FromFaces)
		{
			if (Instance)
				Instance->setTotalMass(Mass, FromFaces);
		}
		void SoftBody::SetTotalDensity(float Density)
		{
			if (Instance)
				Instance->setTotalDensity(Density);
		}
		void SoftBody::SetVolumeMass(float Mass)
		{
			if (Instance)
				Instance->setVolumeMass(Mass);
		}
		void SoftBody::SetVolumeDensity(float Density)
		{
			if (Instance)
				Instance->setVolumeDensity(Density);
		}
		void SoftBody::Translate(const Vector3& Position)
		{
			if (Instance)
				Instance->translate(btVector3(Position.X, Position.Y, -Position.Z));
		}
		void SoftBody::Rotate(const Vector3& Rotation)
		{
			if (Instance)
			{
				btQuaternion Value;
				Value.setEulerZYX(Rotation.X, Rotation.Y, Rotation.Z);
				Instance->rotate(Value);
			}
		}
		void SoftBody::Scale(const Vector3& Scale)
		{
			if (Instance)
				Instance->scale(BtV3(Scale));
		}
		void SoftBody::SetRestLengthScale(float RestLength)
		{
			if (Instance)
				Instance->setRestLengthScale(RestLength);
		}
		void SoftBody::SetPose(bool Volume, bool Frame)
		{
			if (Instance)
				Instance->setPose(Volume, Frame);
		}
		float SoftBody::GetMass(int Node) const
		{
			if (!Instance)
				return 0;

			return Instance->getMass(Node);
		}
		float SoftBody::GetTotalMass() const
		{
			if (!Instance)
				return 0;

			return Instance->getTotalMass();
		}
		float SoftBody::GetRestLengthScale()
		{
			if (!Instance)
				return 0;

			return Instance->getRestLengthScale();
		}
		float SoftBody::GetVolume() const
		{
			if (!Instance)
				return 0;

			return Instance->getVolume();
		}
		int SoftBody::GenerateBendingConstraints(int Distance)
		{
			if (!Instance)
				return 0;

			return Instance->generateBendingConstraints(Distance);
		}
		void SoftBody::RandomizeConstraints()
		{
			if (Instance)
				Instance->randomizeConstraints();
		}
		bool SoftBody::CutLink(int Node0, int Node1, float Position)
		{
			if (!Instance)
				return false;

			return Instance->cutLink(Node0, Node1, Position);
		}
		bool SoftBody::RayTest(const Vector3& From, const Vector3& To, RayCast& Result)
		{
			if (!Instance)
				return false;

			btSoftBody::sRayCast Cast;
			bool R = Instance->rayTest(BtV3(From), BtV3(To), Cast);
			Result.Body = Get(Cast.body);
			Result.Feature = (SoftFeature)Cast.feature;
			Result.Index = Cast.index;
			Result.Fraction = Cast.fraction;

			return R;
		}
		void SoftBody::SetWindVelocity(const Vector3& Velocity)
		{
			if (Instance)
				Instance->setWindVelocity(BtV3(Velocity));
		}
		Vector3 SoftBody::GetWindVelocity()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getWindVelocity();
			return V3Bt(Value);
		}
		void SoftBody::GetAabb(Vector3& Min, Vector3& Max) const
		{
			if (!Instance)
				return;

			btVector3 BMin, BMax;
			Instance->getAabb(BMin, BMax);
			Min = V3Bt(BMin);
			Max = V3Bt(BMax);
		}
		void SoftBody::IndicesToPointers(const int* Map)
		{
			if (Instance)
				Instance->indicesToPointers(Map);
		}
		void SoftBody::SetSpinningFriction(float Value)
		{
			if (Instance)
				Instance->setSpinningFriction(Value);
		}
		Vector3 SoftBody::GetLinearVelocity()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getInterpolationLinearVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 SoftBody::GetAngularVelocity()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getInterpolationAngularVelocity();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 SoftBody::GetCenterPosition()
		{
			return Center;
		}
		void SoftBody::SetActivity(bool Active)
		{
			if (!Instance || GetActivationState() == MotionState_Disable_Deactivation)
				return;

			if (Active)
				Instance->forceActivationState(MotionState_Active);
			else
				Instance->forceActivationState(MotionState_Deactivation_Needed);
		}
		void SoftBody::SetAsGhost()
		{
			if (Instance)
				Instance->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
		}
		void SoftBody::SetAsNormal()
		{
			if (Instance)
				Instance->setCollisionFlags(0);
		}
		void SoftBody::SetSelfPointer()
		{
			if (Instance)
				Instance->setUserPointer(this);
		}
		void SoftBody::SetWorldTransform(btTransform* Value)
		{
			if (Instance && Value)
				Instance->setWorldTransform(*Value);
		}
		void SoftBody::SetActivationState(MotionState Value)
		{
			if (Instance)
				Instance->forceActivationState((int)Value);
		}
		void SoftBody::SetFriction(float Value)
		{
			if (Instance)
				Instance->setFriction(Value);
		}
		void SoftBody::SetRestitution(float Value)
		{
			if (Instance)
				Instance->setRestitution(Value);
		}
		void SoftBody::SetContactStiffness(float Value)
		{
			if (Instance)
				Instance->setContactStiffnessAndDamping(Value, Instance->getContactDamping());
		}
		void SoftBody::SetContactDamping(float Value)
		{
			if (Instance)
				Instance->setContactStiffnessAndDamping(Instance->getContactStiffness(), Value);
		}
		void SoftBody::SetHitFraction(float Value)
		{
			if (Instance)
				Instance->setHitFraction(Value);
		}
		void SoftBody::SetCcdMotionThreshold(float Value)
		{
			if (Instance)
				Instance->setCcdMotionThreshold(Value);
		}
		void SoftBody::SetCcdSweptSphereRadius(float Value)
		{
			if (Instance)
				Instance->setCcdSweptSphereRadius(Value);
		}
		void SoftBody::SetContactProcessingThreshold(float Value)
		{
			if (Instance)
				Instance->setContactProcessingThreshold(Value);
		}
		void SoftBody::SetDeactivationTime(float Value)
		{
			if (Instance)
				Instance->setDeactivationTime(Value);
		}
		void SoftBody::SetRollingFriction(float Value)
		{
			if (Instance)
				Instance->setRollingFriction(Value);
		}
		void SoftBody::SetAnisotropicFriction(const Vector3& Value)
		{
			if (Instance)
				Instance->setAnisotropicFriction(BtV3(Value));
		}
		void SoftBody::SetConfig(const Desc::SConfig& Conf)
		{
			if (!Instance)
				return;

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
		}
		MotionState SoftBody::GetActivationState()
		{
			if (!Instance)
				return MotionState_Active;

			return (MotionState)Instance->getActivationState();
		}
		Shape SoftBody::GetCollisionShapeType()
		{
			if (!Instance || !Initial.Shape.Convex.Enabled)
				return Shape_Invalid;

			return Shape_Convex_Hull;
		}
		Vector3 SoftBody::GetAnisotropicFriction()
		{
			if (!Instance)
				return 0;

			btVector3 Value = Instance->getAnisotropicFriction();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 SoftBody::GetScale()
		{
			if (!Instance || !Instance->getCollisionShape())
				return Vector3(1, 1, 1);

			btVector3 Value = Instance->getCollisionShape()->getLocalScaling();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 SoftBody::GetPosition()
		{
			btVector3 Value = Instance->getWorldTransform().getOrigin();
			return Vector3(Value.getX(), Value.getY(), Value.getZ());
		}
		Vector3 SoftBody::GetRotation()
		{
			btScalar X, Y, Z;
			Instance->getWorldTransform().getBasis().getEulerZYX(Z, Y, X);

			return Vector3(-X, -Y, Z);
		}
		btTransform* SoftBody::GetWorldTransform()
		{
			return &Instance->getWorldTransform();
		}
		btSoftBody* SoftBody::Bullet()
		{
			return Instance;
		}
		bool SoftBody::IsGhost()
		{
			if (!Instance)
				return false;

			return (Instance->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
		}
		bool SoftBody::IsActive()
		{
			if (!Instance)
				return false;

			return Instance->isActive();
		}
		bool SoftBody::IsStatic()
		{
			if (!Instance)
				return false;

			return Instance->isStaticObject();
		}
		bool SoftBody::IsColliding()
		{
			if (!Instance)
				return false;

			return Instance->hasContactResponse();
		}
		float SoftBody::GetSpinningFriction()
		{
			if (!Instance)
				return 0;

			return Instance->getSpinningFriction();
		}
		float SoftBody::GetContactStiffness()
		{
			if (!Instance)
				return 0;

			return Instance->getContactStiffness();
		}
		float SoftBody::GetContactDamping()
		{
			if (!Instance)
				return 0;

			return Instance->getContactDamping();
		}
		float SoftBody::GetFriction()
		{
			if (!Instance)
				return 0;

			return Instance->getFriction();
		}
		float SoftBody::GetRestitution()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitution();
		}
		float SoftBody::GetHitFraction()
		{
			if (!Instance)
				return 0;

			return Instance->getHitFraction();
		}
		float SoftBody::GetCcdMotionThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getCcdMotionThreshold();
		}
		float SoftBody::GetCcdSweptSphereRadius()
		{
			if (!Instance)
				return 0;

			return Instance->getCcdSweptSphereRadius();
		}
		float SoftBody::GetContactProcessingThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getContactProcessingThreshold();
		}
		float SoftBody::GetDeactivationTime()
		{
			if (!Instance)
				return 0;

			return Instance->getDeactivationTime();
		}
		float SoftBody::GetRollingFriction()
		{
			if (!Instance)
				return 0;

			return Instance->getRollingFriction();
		}
		uint64_t SoftBody::GetCollisionFlags()
		{
			if (!Instance)
				return 0;

			return Instance->getCollisionFlags();
		}
		uint64_t SoftBody::GetVerticesCount()
		{
			if (!Instance)
				return 0;

			return Instance->m_nodes.size();
		}
		SoftBody::Desc& SoftBody::GetInitialState()
		{
			return Initial;
		}
		Simulator* SoftBody::GetSimulator()
		{
			return Engine;
		}
		SoftBody* SoftBody::Get(btSoftBody* From)
		{
			if (!From)
				return nullptr;

			return (SoftBody*)From->getUserPointer();
		}

		SliderConstraint::SliderConstraint(Simulator* Refer, const Desc& I) : First(nullptr), Second(nullptr), Instance(nullptr), Engine(Refer), Initial(I), UserPointer(nullptr)
		{
			if (!I.Target1 || !I.Target2 || !Engine)
				return;

			First = I.Target1->Bullet();
			Second = I.Target2->Bullet();
			Instance = new btSliderConstraint(*First, *Second, btTransform::getIdentity(), btTransform::getIdentity(), I.UseLinearPower);
			Instance->setUpperLinLimit(20);
			Instance->setLowerLinLimit(10);

			Engine->AddSliderConstraint(this);
		}
		SliderConstraint::~SliderConstraint()
		{
			Engine->RemoveSliderConstraint(this);
			delete Instance;
		}
		SliderConstraint* SliderConstraint::Copy()
		{
			if (!Instance)
				return nullptr;

			SliderConstraint* Target = new SliderConstraint(Engine, Initial);
			Target->SetAngularMotorVelocity(GetAngularMotorVelocity());
			Target->SetLinearMotorVelocity(GetLinearMotorVelocity());
			Target->SetUpperLinearLimit(GetUpperLinearLimit());
			Target->SetLowerLinearLimit(GetLowerLinearLimit());
			Target->SetBreakingImpulseThreshold(GetBreakingImpulseThreshold());
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
			Target->SetEnabled(IsEnabled());

			return Target;
		}
		void SliderConstraint::SetAngularMotorVelocity(float Value)
		{
			if (Instance)
				Instance->setTargetAngMotorVelocity(Value);
		}
		void SliderConstraint::SetLinearMotorVelocity(float Value)
		{
			if (Instance)
				Instance->setTargetAngMotorVelocity(Value);
		}
		void SliderConstraint::SetUpperLinearLimit(float Value)
		{
			if (Instance)
				Instance->setUpperLinLimit(Value);
		}
		void SliderConstraint::SetLowerLinearLimit(float Value)
		{
			if (Instance)
				Instance->setLowerLinLimit(Value);
		}
		void SliderConstraint::SetBreakingImpulseThreshold(float Value)
		{
			if (Instance)
				Instance->setBreakingImpulseThreshold(Value);
		}
		void SliderConstraint::SetAngularDampingDirection(float Value)
		{
			if (Instance)
				Instance->setDampingDirAng(Value);
		}
		void SliderConstraint::SetLinearDampingDirection(float Value)
		{
			if (Instance)
				Instance->setDampingDirLin(Value);
		}
		void SliderConstraint::SetAngularDampingLimit(float Value)
		{
			if (Instance)
				Instance->setDampingLimAng(Value);
		}
		void SliderConstraint::SetLinearDampingLimit(float Value)
		{
			if (Instance)
				Instance->setDampingLimLin(Value);
		}
		void SliderConstraint::SetAngularDampingOrtho(float Value)
		{
			if (Instance)
				Instance->setDampingOrthoAng(Value);
		}
		void SliderConstraint::SetLinearDampingOrtho(float Value)
		{
			if (Instance)
				Instance->setDampingOrthoLin(Value);
		}
		void SliderConstraint::SetUpperAngularLimit(float Value)
		{
			if (Instance)
				Instance->setUpperAngLimit(Value);
		}
		void SliderConstraint::SetLowerAngularLimit(float Value)
		{
			if (Instance)
				Instance->setLowerAngLimit(Value);
		}
		void SliderConstraint::SetMaxAngularMotorForce(float Value)
		{
			if (Instance)
				Instance->setMaxAngMotorForce(Value);
		}
		void SliderConstraint::SetMaxLinearMotorForce(float Value)
		{
			if (Instance)
				Instance->setMaxLinMotorForce(Value);
		}
		void SliderConstraint::SetAngularRestitutionDirection(float Value)
		{
			if (Instance)
				Instance->setRestitutionDirAng(Value);
		}
		void SliderConstraint::SetLinearRestitutionDirection(float Value)
		{
			if (Instance)
				Instance->setRestitutionDirLin(Value);
		}
		void SliderConstraint::SetAngularRestitutionLimit(float Value)
		{
			if (Instance)
				Instance->setRestitutionLimAng(Value);
		}
		void SliderConstraint::SetLinearRestitutionLimit(float Value)
		{
			if (Instance)
				Instance->setRestitutionLimLin(Value);
		}
		void SliderConstraint::SetAngularRestitutionOrtho(float Value)
		{
			if (Instance)
				Instance->setRestitutionOrthoAng(Value);
		}
		void SliderConstraint::SetLinearRestitutionOrtho(float Value)
		{
			if (Instance)
				Instance->setRestitutionOrthoLin(Value);
		}
		void SliderConstraint::SetAngularSoftnessDirection(float Value)
		{
			if (Instance)
				Instance->setSoftnessDirAng(Value);
		}
		void SliderConstraint::SetLinearSoftnessDirection(float Value)
		{
			if (Instance)
				Instance->setSoftnessDirLin(Value);
		}
		void SliderConstraint::SetAngularSoftnessLimit(float Value)
		{
			if (Instance)
				Instance->setSoftnessLimAng(Value);
		}
		void SliderConstraint::SetLinearSoftnessLimit(float Value)
		{
			if (Instance)
				Instance->setSoftnessLimLin(Value);
		}
		void SliderConstraint::SetAngularSoftnessOrtho(float Value)
		{
			if (Instance)
				Instance->setSoftnessOrthoAng(Value);
		}
		void SliderConstraint::SetLinearSoftnessOrtho(float Value)
		{
			if (Instance)
				Instance->setSoftnessOrthoLin(Value);
		}
		void SliderConstraint::SetPoweredAngularMotor(bool Value)
		{
			if (Instance)
				Instance->setPoweredAngMotor(Value);
		}
		void SliderConstraint::SetPoweredLinearMotor(bool Value)
		{
			if (Instance)
				Instance->setPoweredLinMotor(Value);
		}
		void SliderConstraint::SetEnabled(bool Value)
		{
			if (Instance)
				Instance->setEnabled(Value);
		}
		btSliderConstraint* SliderConstraint::Bullet()
		{
			return Instance;
		}
		btRigidBody* SliderConstraint::GetFirst()
		{
			return First;
		}
		btRigidBody* SliderConstraint::GetSecond()
		{
			return Second;
		}
		float SliderConstraint::GetAngularMotorVelocity()
		{
			if (!Instance)
				return 0;

			return Instance->getTargetAngMotorVelocity();
		}
		float SliderConstraint::GetLinearMotorVelocity()
		{
			if (!Instance)
				return 0;

			return Instance->getTargetLinMotorVelocity();
		}
		float SliderConstraint::GetUpperLinearLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getUpperLinLimit();
		}
		float SliderConstraint::GetLowerLinearLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getLowerLinLimit();
		}
		float SliderConstraint::GetBreakingImpulseThreshold()
		{
			if (!Instance)
				return 0;

			return Instance->getBreakingImpulseThreshold();
		}
		float SliderConstraint::GetAngularDampingDirection()
		{
			if (!Instance)
				return 0;

			return Instance->getDampingDirAng();
		}
		float SliderConstraint::GetLinearDampingDirection()
		{
			if (!Instance)
				return 0;

			return Instance->getDampingDirLin();
		}
		float SliderConstraint::GetAngularDampingLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getDampingLimAng();
		}
		float SliderConstraint::GetLinearDampingLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getDampingLimLin();
		}
		float SliderConstraint::GetAngularDampingOrtho()
		{
			if (!Instance)
				return 0;

			return Instance->getDampingOrthoAng();
		}
		float SliderConstraint::GetLinearDampingOrtho()
		{
			if (!Instance)
				return 0;

			return Instance->getDampingOrthoLin();
		}
		float SliderConstraint::GetUpperAngularLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getUpperAngLimit();
		}
		float SliderConstraint::GetLowerAngularLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getLowerAngLimit();
		}
		float SliderConstraint::GetMaxAngularMotorForce()
		{
			if (!Instance)
				return 0;

			return Instance->getMaxAngMotorForce();
		}
		float SliderConstraint::GetMaxLinearMotorForce()
		{
			if (!Instance)
				return 0;

			return Instance->getMaxLinMotorForce();
		}
		float SliderConstraint::GetAngularRestitutionDirection()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitutionDirAng();
		}
		float SliderConstraint::GetLinearRestitutionDirection()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitutionDirLin();
		}
		float SliderConstraint::GetAngularRestitutionLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitutionLimAng();
		}
		float SliderConstraint::GetLinearRestitutionLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitutionLimLin();
		}
		float SliderConstraint::GetAngularRestitutionOrtho()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitutionOrthoAng();
		}
		float SliderConstraint::GetLinearRestitutionOrtho()
		{
			if (!Instance)
				return 0;

			return Instance->getRestitutionOrthoLin();
		}
		float SliderConstraint::GetAngularSoftnessDirection()
		{
			if (!Instance)
				return 0;

			return Instance->getSoftnessDirAng();
		}
		float SliderConstraint::GetLinearSoftnessDirection()
		{
			if (!Instance)
				return 0;

			return Instance->getSoftnessDirLin();
		}
		float SliderConstraint::GetAngularSoftnessLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getSoftnessLimAng();
		}
		float SliderConstraint::GetLinearSoftnessLimit()
		{
			if (!Instance)
				return 0;

			return Instance->getSoftnessLimLin();
		}
		float SliderConstraint::GetAngularSoftnessOrtho()
		{
			if (!Instance)
				return 0;

			return Instance->getSoftnessOrthoAng();
		}
		float SliderConstraint::GetLinearSoftnessOrtho()
		{
			if (!Instance)
				return 0;

			return Instance->getSoftnessOrthoLin();
		}
		bool SliderConstraint::GetPoweredAngularMotor()
		{
			if (!Instance)
				return 0;

			return Instance->getPoweredAngMotor();
		}
		bool SliderConstraint::GetPoweredLinearMotor()
		{
			if (!Instance)
				return 0;

			return Instance->getPoweredLinMotor();
		}
		bool SliderConstraint::IsConnected()
		{
			if (!First || !Second)
				return false;

			bool CollisionState = false;
			for (int i = 0; i < First->getNumConstraintRefs(); i++)
			{
				if (First->getConstraintRef(i) == Instance)
				{
					CollisionState = true;
					break;
				}
			}

			if (!CollisionState)
				return false;

			for (int i = 0; i < Second->getNumConstraintRefs(); i++)
			{
				if (Second->getConstraintRef(i) == Instance)
					return true;
			}

			return false;
		}
		bool SliderConstraint::IsEnabled()
		{
			if (!Instance)
				return 0;

			return Instance->isEnabled();
		}
		SliderConstraint::Desc& SliderConstraint::GetInitialState()
		{
			return Initial;
		}
		Simulator* SliderConstraint::GetSimulator()
		{
			return Engine;
		}

		Simulator::Simulator(const Desc& I) : SoftSolver(nullptr), TimeSpeed(1), Interpolate(1), Active(true)
		{
			Broadphase = new btDbvtBroadphase();
			Solver = new btSequentialImpulseConstraintSolver();

			if (I.EnableSoftBody)
			{
				SoftSolver = new btDefaultSoftBodySolver();
				Collision = new btSoftBodyRigidBodyCollisionConfiguration();
				Dispatcher = new btCollisionDispatcher(Collision);
				World = new btSoftRigidDynamicsWorld(Dispatcher, Broadphase, Solver, Collision, SoftSolver);

				btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
				btSoftBodyWorldInfo& Info = SoftWorld->getWorldInfo();
				Info.m_gravity = BtV3(I.Gravity);
				Info.water_normal = BtV3(I.WaterNormal);
				Info.water_density = I.WaterDensity;
				Info.water_offset = I.WaterOffset;
				Info.air_density = I.AirDensity;
				Info.m_maxDisplacement = I.MaxDisplacement;
			}
			else
			{
				Collision = new btDefaultCollisionConfiguration();
				Dispatcher = new btCollisionDispatcher(Collision);
				World = new btDiscreteDynamicsWorld(Dispatcher, Broadphase, Solver, Collision);
			}

			World->setWorldUserInfo(this);
			World->setGravity(BtV3(I.Gravity));
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
		}
		Simulator::~Simulator()
		{
			RemoveAll();

			for (auto It = Shapes.begin(); It != Shapes.end(); It++)
			{
				btCollisionShape* Object = (btCollisionShape*)It->first;
				delete Object;
			}

			delete Dispatcher;
			delete Collision;
			delete Solver;
			delete Broadphase;
			delete SoftSolver;
			delete World;
		}
		void Simulator::SetGravity(const Vector3& Gravity)
		{
			World->setGravity(BtV3(Gravity));
		}
		void Simulator::SetLinearImpulse(const Vector3& Impulse, bool RandomFactor)
		{
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(BtV3(Velocity));
			}
		}
		void Simulator::SetLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
			if (Start >= 0 && Start < World->getNumCollisionObjects() && End >= 0 && End < World->getNumCollisionObjects())
			{
				for (int i = Start; i < End; i++)
				{
					Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
					btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(BtV3(Velocity));
				}
			}
		}
		void Simulator::SetAngularImpulse(const Vector3& Impulse, bool RandomFactor)
		{
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(BtV3(Velocity));
			}
		}
		void Simulator::SetAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
			if (Start >= 0 && Start < World->getNumCollisionObjects() && End >= 0 && End < World->getNumCollisionObjects())
			{
				for (int i = Start; i < End; i++)
				{
					Vector3 Velocity = Impulse * (RandomFactor ? Vector3::Random() : 1);
					btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(BtV3(Velocity));
				}
			}
		}
		void Simulator::SetOnCollisionEnter(ContactStartedCallback Callback)
		{
			gContactStartedCallback = Callback;
		}
		void Simulator::SetOnCollisionExit(ContactEndedCallback Callback)
		{
			gContactEndedCallback = Callback;
		}
		void Simulator::CreateLinearImpulse(const Vector3& Impulse, bool RandomFactor)
		{
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btRigidBody* Body = btRigidBody::upcast(World->getCollisionObjectArray()[i]);
				btVector3 Velocity = Body->getLinearVelocity();
				Velocity.setX(Velocity.getX() + Impulse.X * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setY(Velocity.getY() + Impulse.Y * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setZ(Velocity.getZ() + Impulse.Z * (RandomFactor ? Mathf::Random() : 1));
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setLinearVelocity(Velocity);
			}
		}
		void Simulator::CreateLinearImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
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
		}
		void Simulator::CreateAngularImpulse(const Vector3& Impulse, bool RandomFactor)
		{
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btRigidBody* Body = btRigidBody::upcast(World->getCollisionObjectArray()[i]);
				btVector3 Velocity = Body->getAngularVelocity();
				Velocity.setX(Velocity.getX() + Impulse.X * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setY(Velocity.getY() + Impulse.Y * (RandomFactor ? Mathf::Random() : 1));
				Velocity.setZ(Velocity.getZ() + Impulse.Z * (RandomFactor ? Mathf::Random() : 1));
				btRigidBody::upcast(World->getCollisionObjectArray()[i])->setAngularVelocity(Velocity);
			}
		}
		void Simulator::CreateAngularImpulse(const Vector3& Impulse, int Start, int End, bool RandomFactor)
		{
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
		}
		void Simulator::AddSoftBody(SoftBody* Body)
		{
			btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
			if (Body != nullptr && Body->Instance != nullptr && HasSoftBodySupport() && Body->Instance->getWorldArrayIndex() == -1)
				SoftWorld->addSoftBody(Body->Instance);
		}
		void Simulator::RemoveSoftBody(SoftBody* Body)
		{
			btSoftRigidDynamicsWorld* SoftWorld = (btSoftRigidDynamicsWorld*)World;
			if (Body != nullptr && Body->Instance != nullptr && HasSoftBodySupport() && Body->Instance->getWorldArrayIndex() >= 0)
				SoftWorld->removeSoftBody(Body->Instance);
		}
		void Simulator::AddRigidBody(RigidBody* Body)
		{
			if (Body != nullptr && Body->Instance != nullptr && Body->Instance->getWorldArrayIndex() == -1)
				World->addRigidBody(Body->Instance);
		}
		void Simulator::RemoveRigidBody(RigidBody* Body)
		{
			if (Body != nullptr && Body->Instance != nullptr && Body->Instance->getWorldArrayIndex() >= 0)
				World->removeRigidBody(Body->Instance);
		}
		void Simulator::AddSliderConstraint(SliderConstraint* Constraint)
		{
			if (Constraint != nullptr && Constraint->Instance != nullptr)
				World->addConstraint(Constraint->Instance, !Constraint->Initial.UseCollisions);
		}
		void Simulator::RemoveSliderConstraint(SliderConstraint* Constraint)
		{
			if (Constraint != nullptr && Constraint->Instance != nullptr)
				World->removeConstraint(Constraint->Instance);
		}
		void Simulator::RemoveAll()
		{
			for (int i = 0; i < World->getNumCollisionObjects(); i++)
			{
				btCollisionObject* Object = World->getCollisionObjectArray()[i];
				btRigidBody* Body = btRigidBody::upcast(Object);
				if (Body != nullptr)
				{
					delete Body->getMotionState();
					Body->setMotionState(nullptr);

					delete Body->getCollisionShape();
					Body->setCollisionShape(nullptr);
				}

				World->removeCollisionObject(Object);
				delete Object;
			}
		}
		void Simulator::Simulate(float TimeStep)
		{
			if (!Active || TimeSpeed <= 0.0f)
				return;

			World->stepSimulation(TimeStep * TimeSpeed, Interpolate, TimeSpeed / 60.0f);
		}
		void Simulator::FindContacts(RigidBody* Body, int(* Callback)(ShapeContact*, const CollisionBody&, const CollisionBody&))
		{
			if (!Callback || !Body)
				return;

			FindContactsHandler Handler;
			Handler.Callback = Callback;
			World->contactTest(Body->Bullet(), Handler);
		}
		bool Simulator::FindRayContacts(const Vector3& Start, const Vector3& End, int(* Callback)(RayContact*, const CollisionBody&))
		{
			if (!Callback)
				return false;

			FindRayContactsHandler Handler;
			Handler.Callback = Callback;

			World->rayTest(btVector3(Start.X, Start.Y, Start.Z), btVector3(End.X, End.Y, End.Z), Handler);
			return Handler.m_collisionObject != nullptr;
		}
		RigidBody* Simulator::CreateRigidBody(const RigidBody::Desc& I, Transform* Transform)
		{
			if (!Transform)
				return CreateRigidBody(I);

			RigidBody::Desc F(I);
			F.Position = Transform->Position;
			F.Rotation = Transform->Rotation;
			F.Scale = Transform->Scale;
			return CreateRigidBody(F);
		}
		RigidBody* Simulator::CreateRigidBody(const RigidBody::Desc& I)
		{
			return new RigidBody(this, I);
		}
		SoftBody* Simulator::CreateSoftBody(const SoftBody::Desc& I, Transform* Transform)
		{
			if (!Transform)
				return CreateSoftBody(I);

			SoftBody::Desc F(I);
			F.Position = Transform->Position;
			F.Rotation = Transform->Rotation;
			F.Scale = Transform->Scale;
			return CreateSoftBody(F);
		}
		SoftBody* Simulator::CreateSoftBody(const SoftBody::Desc& I)
		{
			if (!HasSoftBodySupport())
				return nullptr;

			return new SoftBody(this, I);
		}
		SliderConstraint* Simulator::CreateSliderConstraint(const SliderConstraint::Desc& I)
		{
			return new SliderConstraint(this, I);
		}
		btCollisionShape* Simulator::CreateCube(const Vector3& Scale)
		{
			btCollisionShape* Shape = new btBoxShape(BtV3(Scale));
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateSphere(float Radius)
		{
			btCollisionShape* Shape = new btSphereShape(Radius);
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateCapsule(float Radius, float Height)
		{
			btCollisionShape* Shape = new btCapsuleShape(Radius, Height);
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateCone(float Radius, float Height)
		{
			btCollisionShape* Shape = new btConeShape(Radius, Height);
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateCylinder(const Vector3& Scale)
		{
			btCollisionShape* Shape = new btCylinderShape(BtV3(Scale));
			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<SkinVertex>& Vertices)
		{
			btConvexHullShape* Shape = new btConvexHullShape();
			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vertex>& Vertices)
		{
			btConvexHullShape* Shape = new btConvexHullShape();
			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vector2>& Vertices)
		{
			btConvexHullShape* Shape = new btConvexHullShape();
			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
				Shape->addPoint(btVector3(It->X, It->Y, 0), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vector3>& Vertices)
		{
			btConvexHullShape* Shape = new btConvexHullShape();
			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
				Shape->addPoint(btVector3(It->X, It->Y, It->Z), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateConvexHull(std::vector<Vector4>& Vertices)
		{
			btConvexHullShape* Shape = new btConvexHullShape();
			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
				Shape->addPoint(btVector3(It->X, It->Y, It->Z), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			Safe.lock();
			Shapes[Shape] = 1;
			Safe.unlock();

			return Shape;
		}
		btCollisionShape* Simulator::CreateConvexHull(btCollisionShape* From)
		{
			if (!From || From->getShapeType() != Shape_Convex_Hull)
				return nullptr;

			btConvexHullShape* Hull = new btConvexHullShape();
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
		}
		btCollisionShape* Simulator::CreateShape(Shape Wanted)
		{
			switch (Wanted)
			{
				case Shape::Shape_Box:
					return CreateCube();
				case Shape::Shape_Sphere:
					return CreateSphere();
				case Shape::Shape_Capsule:
					return CreateCapsule();
				case Shape::Shape_Cone:
					return CreateCone();
				case Shape::Shape_Cylinder:
					return CreateCylinder();
				default:
					return nullptr;
			}
		}
		btCollisionShape* Simulator::TryCloneShape(btCollisionShape* Value)
		{
			if (!Value)
				return nullptr;

			Shape Type = (Shape)Value->getShapeType();
			if (Type == Shape_Box)
			{
				btBoxShape* Box = (btBoxShape*)Value;
				btVector3 Size = Box->getHalfExtentsWithMargin() / Box->getLocalScaling();
				return CreateCube(V3Bt(Size));
			}
			else if (Type == Shape_Sphere)
			{
				btSphereShape* Sphere = (btSphereShape*)Value;
				return CreateSphere(Sphere->getRadius());
			}
			else if (Type == Shape_Capsule)
			{
				btCapsuleShape* Capsule = (btCapsuleShape*)Value;
				return CreateCapsule(Capsule->getRadius(), Capsule->getHalfHeight() * 2.0f);
			}
			else if (Type == Shape_Cone)
			{
				btConeShape* Cone = (btConeShape*)Value;
				return CreateCone(Cone->getRadius(), Cone->getHeight());
			}
			else if (Type == Shape_Cylinder)
			{
				btCylinderShape* Cylinder = (btCylinderShape*)Value;
				btVector3 Size = Cylinder->getHalfExtentsWithMargin() / Cylinder->getLocalScaling();
				return CreateCylinder(V3Bt(Size));
			}
			else if (Type == Shape_Convex_Hull)
				return CreateConvexHull(Value);

			return nullptr;
		}
		btCollisionShape* Simulator::ReuseShape(btCollisionShape* Value)
		{
			if (!Value)
				return nullptr;

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
		}
		void Simulator::FreeShape(btCollisionShape** Value)
		{
			if (!Value || !*Value)
				return;

			Safe.lock();
			auto It = Shapes.find(*Value);
			if (It != Shapes.end())
			{
				*Value = nullptr;
				if (It->second-- <= 1)
				{
					delete (btCollisionShape*)It->first;
					Shapes.erase(It);
				}
			}
			Safe.unlock();
		}
		std::vector<Vector3> Simulator::GetShapeVertices(btCollisionShape* Value)
		{
			if (!Value)
				return std::vector<Vector3>();

			auto Type = (Shape)Value->getShapeType();
			if (Type != Shape_Convex_Hull)
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
		}
		uint64_t Simulator::GetShapeVerticesCount(btCollisionShape* Value)
		{
			if (!Value)
				return 0;

			auto Type = (Compute::Shape)Value->getShapeType();
			if (Type != Shape_Convex_Hull)
				return 0;

			btConvexHullShape* Hull = (btConvexHullShape*)Value;
			return Hull->getNumPoints();
		}
		float Simulator::GetMaxDisplacement()
		{
			if (!SoftSolver || !World)
				return 1000;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().m_maxDisplacement;
		}
		float Simulator::GetAirDensity()
		{
			if (!SoftSolver || !World)
				return 1.2f;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().air_density;
		}
		float Simulator::GetWaterOffset()
		{
			if (!SoftSolver || !World)
				return 0;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_offset;
		}
		float Simulator::GetWaterDensity()
		{
			if (!SoftSolver || !World)
				return 0;

			return ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_density;
		}
		Vector3 Simulator::GetWaterNormal()
		{
			if (!SoftSolver || !World)
				return 0;

			btVector3 Value = ((btSoftRigidDynamicsWorld*)World)->getWorldInfo().water_normal;
			return V3Bt(Value);
		}
		Vector3 Simulator::GetGravity()
		{
			if (!World)
				return 0;

			btVector3 Value = World->getGravity();
			return V3Bt(Value);
		}
		ContactStartedCallback Simulator::GetOnCollisionEnter()
		{
			return gContactStartedCallback;
		}
		ContactEndedCallback Simulator::GetOnCollisionExit()
		{
			return gContactEndedCallback;
		}
		btCollisionConfiguration* Simulator::GetCollision()
		{
			return Collision;
		}
		btBroadphaseInterface* Simulator::GetBroadphase()
		{
			return Broadphase;
		}
		btConstraintSolver* Simulator::GetSolver()
		{
			return Solver;
		}
		btDiscreteDynamicsWorld* Simulator::GetWorld()
		{
			return World;
		}
		btCollisionDispatcher* Simulator::GetDispatcher()
		{
			return Dispatcher;
		}
		btSoftBodySolver* Simulator::GetSoftSolver()
		{
			return SoftSolver;
		}
		bool Simulator::HasSoftBodySupport()
		{
			return SoftSolver != nullptr;
		}
		int Simulator::GetContactManifoldCount()
		{
			if (!Dispatcher)
				return 0;

			return Dispatcher->getNumManifolds();
		}
		void Simulator::FreeUnmanagedShape(btCollisionShape* Shape)
		{
			if (Shape != nullptr)
				delete Shape;
		}
		Simulator* Simulator::Get(btDiscreteDynamicsWorld* From)
		{
			if (!From)
				return nullptr;

			return (Simulator*)From->getWorldUserInfo();
		}
		btCollisionShape* Simulator::CreateUnmanagedShape(std::vector<Vertex>& Vertices)
		{
			btConvexHullShape* Shape = new btConvexHullShape();
			for (auto It = Vertices.begin(); It != Vertices.end(); It++)
				Shape->addPoint(btVector3(It->PositionX, It->PositionY, It->PositionZ), false);

			Shape->recalcLocalAabb();
			Shape->optimizeConvexHull();
			Shape->setMargin(0);

			return Shape;
		}
		btCollisionShape* Simulator::CreateUnmanagedShape(btCollisionShape* From)
		{
			if (!From || From->getShapeType() != Shape_Convex_Hull)
				return nullptr;

			btConvexHullShape* Hull = new btConvexHullShape();
			btConvexHullShape* Base = (btConvexHullShape*)From;

			for (size_t i = 0; i < Base->getNumPoints(); i++)
				Hull->addPoint(*(Base->getUnscaledPoints() + i), false);

			Hull->recalcLocalAabb();
			Hull->optimizeConvexHull();
			Hull->setMargin(0);

			return Hull;
		}
	}
}
