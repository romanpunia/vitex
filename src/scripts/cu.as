namespace CU
{
	class Vector2
	{
		float X;
		float Y;

		float Length() const;
		float Sum() const;
		float Dot(const CU::Vector2&in) const;
		float Distance(const CU::Vector2&in) const;
		float Hypotenuse() const;
		float LookAt(const CU::Vector2&in) const;
		float Cross(const CU::Vector2&in) const;
		CU::Vector2 Direction(float) const;
		CU::Vector2 Inv() const;
		CU::Vector2 InvX() const;
		CU::Vector2 InvY() const;
		CU::Vector2 Normalize() const;
		CU::Vector2 sNormalize() const;
		CU::Vector2 Lerp(const CU::Vector2&in, float) const;
		CU::Vector2 sLerp(const CU::Vector2&in, float) const;
		CU::Vector2 aLerp(const CU::Vector2&in, float) const;
		CU::Vector2 rLerp() const;
		CU::Vector2 Abs() const;
		CU::Vector2 Radians() const;
		CU::Vector2 Degrees() const;
		CU::Vector2 XY() const;
		CU::Vector2 Mul(float) const;
		CU::Vector2 Mul(float, float) const;
		CU::Vector2 Mul(const CU::Vector2&in) const;
		CU::Vector2 Div(const CU::Vector2&in) const;
		CU::Vector2 SetX(float) const;
		CU::Vector2 SetY(float) const;
		void Set(const CU::Vector2&in) const;
		CU::Vector2& opMulAssign(const CU::Vector2&in);
		CU::Vector2& opMulAssign(float);
		CU::Vector2& opDivAssign(const CU::Vector2&in);
		CU::Vector2& opDivAssign(float);
		CU::Vector2& opAddAssign(const CU::Vector2&in);
		CU::Vector2& opAddAssign(float);
		CU::Vector2& opSubAssign(const CU::Vector2&in);
		CU::Vector2& opSubAssign(float);
		CU::Vector2 opMul(const CU::Vector2&in) const;
		CU::Vector2 opMul(float) const;
		CU::Vector2 opDiv(const CU::Vector2&in) const;
		CU::Vector2 opDiv(float) const;
		CU::Vector2 opAdd(const CU::Vector2&in) const;
		CU::Vector2 opAdd(float) const;
		CU::Vector2 opSub(const CU::Vector2&in) const;
		CU::Vector2 opSub(float) const;
		CU::Vector2 opNeg() const;
		bool opEquals(const CU::Vector2&in) const;
		int opCmp(const CU::Vector2&in) const;
	}

	namespace Vector2
	{
		CU::Vector2 Random();
		CU::Vector2 RandomAbs();
		CU::Vector2 One();
		CU::Vector2 Zero();
		CU::Vector2 Up();
		CU::Vector2 Down();
		CU::Vector2 Left();
		CU::Vector2 Right();
	}

	class Vector3
	{
		float X;
		float Y;
		float Z;

		float Length() const;
		float Sum() const;
		float Dot(const CU::Vector3&in) const;
		float Distance(const CU::Vector3&in) const;
		float Hypotenuse() const;
		float LookAtXY(const CU::Vector3&in) const;
		float LookAtXZ(const CU::Vector3&in) const;
		float Cross(const CU::Vector3&in) const;
		CU::Vector3 hDirection() const;
		CU::Vector3 dDirection() const;
		CU::Vector3 Inv() const;
		CU::Vector3 InvX() const;
		CU::Vector3 InvY() const;
		CU::Vector3 InvZ() const;
		CU::Vector3 Normalize() const;
		CU::Vector3 sNormalize() const;
		CU::Vector3 Lerp(const CU::Vector3&in, float) const;
		CU::Vector3 sLerp(const CU::Vector3&in, float) const;
		CU::Vector3 aLerp(const CU::Vector3&in, float) const;
		CU::Vector3 rLerp() const;
		CU::Vector3 Abs() const;
		CU::Vector3 Radians() const;
		CU::Vector3 Degrees() const;
		CU::Vector2 XY() const;
		CU::Vector3 XYZ() const;
		CU::Vector3 Mul(float) const;
		CU::Vector3 Mul(const CU::Vector2&in, float) const;
		CU::Vector3 Mul(const CU::Vector3&in) const;
		CU::Vector3 Div(const CU::Vector3&in) const;
		CU::Vector3 SetX(float) const;
		CU::Vector3 SetY(float) const;
		CU::Vector3 SetZ(float) const;
		void Set(const CU::Vector3&in) const;
		CU::Vector3& opMulAssign(const CU::Vector3&in);
		CU::Vector3& opMulAssign(float);
		CU::Vector3& opDivAssign(const CU::Vector3&in);
		CU::Vector3& opDivAssign(float);
		CU::Vector3& opAddAssign(const CU::Vector3&in);
		CU::Vector3& opAddAssign(float);
		CU::Vector3& opSubAssign(const CU::Vector3&in);
		CU::Vector3& opSubAssign(float);
		CU::Vector3 opMul(const CU::Vector3&in) const;
		CU::Vector3 opMul(float) const;
		CU::Vector3 opDiv(const CU::Vector3&in) const;
		CU::Vector3 opDiv(float) const;
		CU::Vector3 opAdd(const CU::Vector3&in) const;
		CU::Vector3 opAdd(float) const;
		CU::Vector3 opSub(const CU::Vector3&in) const;
		CU::Vector3 opSub(float) const;
		CU::Vector3 opNeg() const;
		bool opEquals(const CU::Vector3&in) const;
		int opCmp(const CU::Vector3&in) const;
	}

	namespace Vector3
	{
		CU::Vector3 Random();
		CU::Vector3 RandomAbs();
		CU::Vector3 One();
		CU::Vector3 Zero();
		CU::Vector3 Up();
		CU::Vector3 Down();
		CU::Vector3 Left();
		CU::Vector3 Right();
		CU::Vector3 Forward();
		CU::Vector3 Backward();
	}

	class Vector4
	{
		float X;
		float Y;
		float Z;
		float W;

		float Length() const;
		float Sum() const;
		float Dot(const CU::Vector4&in) const;
		float Distance(const CU::Vector4&in) const;
		float Cross(const CU::Vector4&in) const;
		CU::Vector4 Inv() const;
		CU::Vector4 InvX() const;
		CU::Vector4 InvY() const;
		CU::Vector4 InvZ() const;
		CU::Vector4 InvW() const;
		CU::Vector4 Normalize() const;
		CU::Vector4 sNormalize() const;
		CU::Vector4 Lerp(const CU::Vector4&in, float) const;
		CU::Vector4 sLerp(const CU::Vector4&in, float) const;
		CU::Vector4 aLerp(const CU::Vector4&in, float) const;
		CU::Vector4 rLerp() const;
		CU::Vector4 Abs() const;
		CU::Vector4 Radians() const;
		CU::Vector4 Degrees() const;
		CU::Vector2 XY() const;
		CU::Vector3 XYZ() const;
		CU::Vector4 XYZW() const;
		CU::Vector4 Mul(float) const;
		CU::Vector4 Mul(const CU::Vector2&in, float, float) const;
		CU::Vector4 Mul(const CU::Vector3&in, float) const;
		CU::Vector4 Mul(const CU::Vector4&in) const;
		CU::Vector4 Div(const CU::Vector4&in) const;
		CU::Vector4 SetX(float) const;
		CU::Vector4 SetY(float) const;
		CU::Vector4 SetZ(float) const;
		CU::Vector4 SetW(float) const;
		void Set(const CU::Vector4&in) const;
		CU::Vector4& opMulAssign(const CU::Vector4&in);
		CU::Vector4& opMulAssign(float);
		CU::Vector4& opDivAssign(const CU::Vector4&in);
		CU::Vector4& opDivAssign(float);
		CU::Vector4& opAddAssign(const CU::Vector4&in);
		CU::Vector4& opAddAssign(float);
		CU::Vector4& opSubAssign(const CU::Vector4&in);
		CU::Vector4& opSubAssign(float);
		CU::Vector4 opMul(const CU::Vector4&in) const;
		CU::Vector4 opMul(float) const;
		CU::Vector4 opDiv(const CU::Vector4&in) const;
		CU::Vector4 opDiv(float) const;
		CU::Vector4 opAdd(const CU::Vector4&in) const;
		CU::Vector4 opAdd(float) const;
		CU::Vector4 opSub(const CU::Vector4&in) const;
		CU::Vector4 opSub(float) const;
		CU::Vector4 opNeg() const;
		bool opEquals(const CU::Vector4&in) const;
		int opCmp(const CU::Vector4&in) const;
	}

	namespace Vector4
	{
		CU::Vector4 Random();
		CU::Vector4 RandomAbs();
		CU::Vector4 One();
		CU::Vector4 Zero();
		CU::Vector4 Up();
		CU::Vector4 Down();
		CU::Vector4 Left();
		CU::Vector4 Right();
		CU::Vector4 Forward();
		CU::Vector4 Backward();
		CU::Vector4 UnitW();
	}

	class Vertex
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
	}

	class Rectangle
	{
		int64 Left;
		int64 Top;
		int64 Right;
		int64 Bottom;
	}

	class SkinVertex
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
	}

	class ShapeVertex
	{
		float PositionX;
		float PositionY;
		float PositionZ;
		float TexCoordX;
		float TexCoordY;
	}

	class ElementVertex
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
	}
}