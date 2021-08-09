#include "compute-lib.h"
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

namespace Tomahawk
{
	namespace Script
	{
		Compute::Vector2& Vector2MulEq1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A *= B;
		}
		Compute::Vector2& Vector2MulEq2(Compute::Vector2& A, float B)
		{
			return A *= B;
		}
		Compute::Vector2& Vector2DivEq1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A /= B;
		}
		Compute::Vector2& Vector2DivEq2(Compute::Vector2& A, float B)
		{
			return A /= B;
		}
		Compute::Vector2& Vector2AddEq1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A += B;
		}
		Compute::Vector2& Vector2AddEq2(Compute::Vector2& A, float B)
		{
			return A += B;
		}
		Compute::Vector2& Vector2SubEq1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A -= B;
		}
		Compute::Vector2& Vector2SubEq2(Compute::Vector2& A, float B)
		{
			return A -= B;
		}
		Compute::Vector2 Vector2Mul1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A * B;
		}
		Compute::Vector2 Vector2Mul2(Compute::Vector2& A, float B)
		{
			return A * B;
		}
		Compute::Vector2 Vector2Div1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A / B;
		}
		Compute::Vector2 Vector2Div2(Compute::Vector2& A, float B)
		{
			return A / B;
		}
		Compute::Vector2 Vector2Add1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A + B;
		}
		Compute::Vector2 Vector2Add2(Compute::Vector2& A, float B)
		{
			return A + B;
		}
		Compute::Vector2 Vector2Sub1(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A - B;
		}
		Compute::Vector2 Vector2Sub2(Compute::Vector2& A, float B)
		{
			return A - B;
		}
		Compute::Vector2 Vector2Neg(Compute::Vector2& A)
		{
			return -A;
		}
		bool Vector2Eq(Compute::Vector2& A, const Compute::Vector2& B)
		{
			return A == B;
		}
		int Vector2Cmp(Compute::Vector2& A, const Compute::Vector2& B)
		{
			if (A == B)
				return 0;

			return A > B ? 1 : -1;
		}

		Compute::Vector3& Vector3MulEq1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A *= B;
		}
		Compute::Vector3& Vector3MulEq2(Compute::Vector3& A, float B)
		{
			return A *= B;
		}
		Compute::Vector3& Vector3DivEq1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A /= B;
		}
		Compute::Vector3& Vector3DivEq2(Compute::Vector3& A, float B)
		{
			return A /= B;
		}
		Compute::Vector3& Vector3AddEq1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A += B;
		}
		Compute::Vector3& Vector3AddEq2(Compute::Vector3& A, float B)
		{
			return A += B;
		}
		Compute::Vector3& Vector3SubEq1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A -= B;
		}
		Compute::Vector3& Vector3SubEq2(Compute::Vector3& A, float B)
		{
			return A -= B;
		}
		Compute::Vector3 Vector3Mul1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A * B;
		}
		Compute::Vector3 Vector3Mul2(Compute::Vector3& A, float B)
		{
			return A * B;
		}
		Compute::Vector3 Vector3Div1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A / B;
		}
		Compute::Vector3 Vector3Div2(Compute::Vector3& A, float B)
		{
			return A / B;
		}
		Compute::Vector3 Vector3Add1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A + B;
		}
		Compute::Vector3 Vector3Add2(Compute::Vector3& A, float B)
		{
			return A + B;
		}
		Compute::Vector3 Vector3Sub1(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A - B;
		}
		Compute::Vector3 Vector3Sub2(Compute::Vector3& A, float B)
		{
			return A - B;
		}
		Compute::Vector3 Vector3Neg(Compute::Vector3& A)
		{
			return -A;
		}
		bool Vector3Eq(Compute::Vector3& A, const Compute::Vector3& B)
		{
			return A == B;
		}
		int Vector3Cmp(Compute::Vector3& A, const Compute::Vector3& B)
		{
			if (A == B)
				return 0;

			return A > B ? 1 : -1;
		}

		Compute::Vector4& Vector4MulEq1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A *= B;
		}
		Compute::Vector4& Vector4MulEq2(Compute::Vector4& A, float B)
		{
			return A *= B;
		}
		Compute::Vector4& Vector4DivEq1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A /= B;
		}
		Compute::Vector4& Vector4DivEq2(Compute::Vector4& A, float B)
		{
			return A /= B;
		}
		Compute::Vector4& Vector4AddEq1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A += B;
		}
		Compute::Vector4& Vector4AddEq2(Compute::Vector4& A, float B)
		{
			return A += B;
		}
		Compute::Vector4& Vector4SubEq1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A -= B;
		}
		Compute::Vector4& Vector4SubEq2(Compute::Vector4& A, float B)
		{
			return A -= B;
		}
		Compute::Vector4 Vector4Mul1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A * B;
		}
		Compute::Vector4 Vector4Mul2(Compute::Vector4& A, float B)
		{
			return A * B;
		}
		Compute::Vector4 Vector4Div1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A / B;
		}
		Compute::Vector4 Vector4Div2(Compute::Vector4& A, float B)
		{
			return A / B;
		}
		Compute::Vector4 Vector4Add1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A + B;
		}
		Compute::Vector4 Vector4Add2(Compute::Vector4& A, float B)
		{
			return A + B;
		}
		Compute::Vector4 Vector4Sub1(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A - B;
		}
		Compute::Vector4 Vector4Sub2(Compute::Vector4& A, float B)
		{
			return A - B;
		}
		Compute::Vector4 Vector4Neg(Compute::Vector4& A)
		{
			return -A;
		}
		bool Vector4Eq(Compute::Vector4& A, const Compute::Vector4& B)
		{
			return A == B;
		}
		int Vector4Cmp(Compute::Vector4& A, const Compute::Vector4& B)
		{
			if (A == B)
				return 0;

			return A > B ? 1 : -1;
		}

		bool CURegisterVector2(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CU");
			VMTypeClass VVector2 = Register.SetPod<Compute::Vector2>("Vector2");
			VVector2.SetConstructor<Compute::Vector2>("void f()");
			VVector2.SetConstructor<Compute::Vector2, float, float>("void f(float, float)");
			VVector2.SetConstructor<Compute::Vector2, float>("void f(float)");
			VVector2.SetProperty<Compute::Vector2>("float X", &Compute::Vector2::X);
			VVector2.SetProperty<Compute::Vector2>("float Y", &Compute::Vector2::Y);
			VVector2.SetMethod("float Length() const", &Compute::Vector2::Length);
			VVector2.SetMethod("float Sum() const", &Compute::Vector2::Sum);
			VVector2.SetMethod("float Dot(const Vector2 &in) const", &Compute::Vector2::Dot);
			VVector2.SetMethod("float Distance(const Vector2 &in) const", &Compute::Vector2::Distance);
			VVector2.SetMethod("float Hypotenuse() const", &Compute::Vector2::Hypotenuse);
			VVector2.SetMethod("float LookAt(const Vector2 &in) const", &Compute::Vector2::LookAt);
			VVector2.SetMethod("float Cross(const Vector2 &in) const", &Compute::Vector2::Cross);
			VVector2.SetMethod("Vector2 Direction(float) const", &Compute::Vector2::Direction);
			VVector2.SetMethod("Vector2 Inv() const", &Compute::Vector2::Inv);
			VVector2.SetMethod("Vector2 InvX() const", &Compute::Vector2::InvX);
			VVector2.SetMethod("Vector2 InvY() const", &Compute::Vector2::InvY);
			VVector2.SetMethod("Vector2 Normalize() const", &Compute::Vector2::Normalize);
			VVector2.SetMethod("Vector2 sNormalize() const", &Compute::Vector2::sNormalize);
			VVector2.SetMethod("Vector2 Lerp(const Vector2 &in, float) const", &Compute::Vector2::Lerp);
			VVector2.SetMethod("Vector2 sLerp(const Vector2 &in, float) const", &Compute::Vector2::sLerp);
			VVector2.SetMethod("Vector2 aLerp(const Vector2 &in, float) const", &Compute::Vector2::aLerp);
			VVector2.SetMethod("Vector2 rLerp() const", &Compute::Vector2::rLerp);
			VVector2.SetMethod("Vector2 Abs() const", &Compute::Vector2::Abs);
			VVector2.SetMethod("Vector2 Radians() const", &Compute::Vector2::Radians);
			VVector2.SetMethod("Vector2 Degrees() const", &Compute::Vector2::Degrees);
			VVector2.SetMethod("Vector2 XY() const", &Compute::Vector2::XY);
			VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float>("Vector2 Mul(float) const", &Compute::Vector2::Mul);
			VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float, float>("Vector2 Mul(float, float) const", &Compute::Vector2::Mul);
			VVector2.SetMethod<Compute::Vector2, Compute::Vector2, const Compute::Vector2&>("Vector2 Mul(const Vector2 &in) const", &Compute::Vector2::Mul);
			VVector2.SetMethod("Vector2 Div(const Vector2 &in) const", &Compute::Vector2::Div);
			VVector2.SetMethod("Vector2 SetX(float) const", &Compute::Vector2::SetX);
			VVector2.SetMethod("Vector2 SetY(float) const", &Compute::Vector2::SetY);
			VVector2.SetMethod("void Set(const Vector2 &in) const", &Compute::Vector2::Set);
			VVector2.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Vector2&", "const Vector2 &in", &Vector2MulEq1);
			VVector2.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Vector2&", "float", &Vector2MulEq2);
			VVector2.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Vector2&", "const Vector2 &in", &Vector2DivEq1);
			VVector2.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Vector2&", "float", &Vector2DivEq2);
			VVector2.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Vector2&", "const Vector2 &in", &Vector2AddEq1);
			VVector2.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Vector2&", "float", &Vector2AddEq2);
			VVector2.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Vector2&", "const Vector2 &in", &Vector2SubEq1);
			VVector2.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Vector2&", "float", &Vector2SubEq2);
			VVector2.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Vector2", "const Vector2 &in", &Vector2Mul1);
			VVector2.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Vector2", "float", &Vector2Mul2);
			VVector2.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Vector2", "const Vector2 &in", &Vector2Div1);
			VVector2.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Vector2", "float", &Vector2Div2);
			VVector2.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Vector2", "const Vector2 &in", &Vector2Add1);
			VVector2.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Vector2", "float", &Vector2Add2);
			VVector2.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Vector2", "const Vector2 &in", &Vector2Sub1);
			VVector2.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Vector2", "float", &Vector2Sub2);
			VVector2.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "Vector2", "", &Vector2Neg);
			VVector2.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const Vector2 &in", &Vector2Eq);
			VVector2.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const Vector2 &in", &Vector2Cmp);
			Engine->EndNamespace();
			Engine->BeginNamespace("CU::Vector2");
			Register.SetFunction("Vector2 Random()", &Compute::Vector2::Random);
			Register.SetFunction("Vector2 RandomAbs()", &Compute::Vector2::RandomAbs);
			Register.SetFunction("Vector2 One()", &Compute::Vector2::One);
			Register.SetFunction("Vector2 Zero()", &Compute::Vector2::Zero);
			Register.SetFunction("Vector2 Up()", &Compute::Vector2::Up);
			Register.SetFunction("Vector2 Down()", &Compute::Vector2::Down);
			Register.SetFunction("Vector2 Left()", &Compute::Vector2::Left);
			Register.SetFunction("Vector2 Right()", &Compute::Vector2::Right);
			Engine->EndNamespace();
			
			return true;
		}
		bool CURegisterVector3(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CU");
			VMTypeClass VVector3 = Register.SetPod<Compute::Vector3>("Vector3");
			VVector3.SetConstructor<Compute::Vector3>("void f()");
			VVector3.SetConstructor<Compute::Vector3, float, float>("void f(float, float)");
			VVector3.SetConstructor<Compute::Vector3, float, float, float>("void f(float, float, float)");
			VVector3.SetConstructor<Compute::Vector3, float>("void f(float)");
			VVector3.SetConstructor<Compute::Vector3, const Compute::Vector2&>("void f(const Vector2 &in)");
			VVector3.SetProperty<Compute::Vector3>("float X", &Compute::Vector3::X);
			VVector3.SetProperty<Compute::Vector3>("float Y", &Compute::Vector3::Y);
			VVector3.SetProperty<Compute::Vector3>("float Z", &Compute::Vector3::Z);
			VVector3.SetMethod("float Length() const", &Compute::Vector3::Length);
			VVector3.SetMethod("float Sum() const", &Compute::Vector3::Sum);
			VVector3.SetMethod("float Dot(const Vector3 &in) const", &Compute::Vector3::Dot);
			VVector3.SetMethod("float Distance(const Vector3 &in) const", &Compute::Vector3::Distance);
			VVector3.SetMethod("float Hypotenuse() const", &Compute::Vector3::Hypotenuse);
			VVector3.SetMethod("float LookAtXY(const Vector3 &in) const", &Compute::Vector3::LookAtXY);
			VVector3.SetMethod("float LookAtXZ(const Vector3 &in) const", &Compute::Vector3::LookAtXZ);
			VVector3.SetMethod("float Cross(const Vector3 &in) const", &Compute::Vector3::Cross);
			VVector3.SetMethod("Vector3 hDirection() const", &Compute::Vector3::hDirection);
			VVector3.SetMethod("Vector3 dDirection() const", &Compute::Vector3::dDirection);
			VVector3.SetMethod("Vector3 Inv() const", &Compute::Vector3::Inv);
			VVector3.SetMethod("Vector3 InvX() const", &Compute::Vector3::InvX);
			VVector3.SetMethod("Vector3 InvY() const", &Compute::Vector3::InvY);
			VVector3.SetMethod("Vector3 InvZ() const", &Compute::Vector3::InvZ);
			VVector3.SetMethod("Vector3 Normalize() const", &Compute::Vector3::Normalize);
			VVector3.SetMethod("Vector3 sNormalize() const", &Compute::Vector3::sNormalize);
			VVector3.SetMethod("Vector3 Lerp(const Vector3 &in, float) const", &Compute::Vector3::Lerp);
			VVector3.SetMethod("Vector3 sLerp(const Vector3 &in, float) const", &Compute::Vector3::sLerp);
			VVector3.SetMethod("Vector3 aLerp(const Vector3 &in, float) const", &Compute::Vector3::aLerp);
			VVector3.SetMethod("Vector3 rLerp() const", &Compute::Vector3::rLerp);
			VVector3.SetMethod("Vector3 Abs() const", &Compute::Vector3::Abs);
			VVector3.SetMethod("Vector3 Radians() const", &Compute::Vector3::Radians);
			VVector3.SetMethod("Vector3 Degrees() const", &Compute::Vector3::Degrees);
			VVector3.SetMethod("Vector2 XY() const", &Compute::Vector3::XY);
			VVector3.SetMethod("Vector3 XYZ() const", &Compute::Vector3::XYZ);
			VVector3.SetMethod<Compute::Vector3, Compute::Vector3, float>("Vector3 Mul(float) const", &Compute::Vector3::Mul);
			VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector2&, float>("Vector3 Mul(const Vector2 &in, float) const", &Compute::Vector3::Mul);
			VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector3&>("Vector3 Mul(const Vector3 &in) const", &Compute::Vector3::Mul);
			VVector3.SetMethod("Vector3 Div(const Vector3 &in) const", &Compute::Vector3::Div);
			VVector3.SetMethod("Vector3 SetX(float) const", &Compute::Vector3::SetX);
			VVector3.SetMethod("Vector3 SetY(float) const", &Compute::Vector3::SetY);
			VVector3.SetMethod("Vector3 SetZ(float) const", &Compute::Vector3::SetZ);
			VVector3.SetMethod("void Set(const Vector3 &in) const", &Compute::Vector3::Set);
			VVector3.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Vector3&", "const Vector3 &in", &Vector3MulEq1);
			VVector3.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Vector3&", "float", &Vector3MulEq2);
			VVector3.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Vector3&", "const Vector3 &in", &Vector3DivEq1);
			VVector3.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Vector3&", "float", &Vector3DivEq2);
			VVector3.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Vector3&", "const Vector3 &in", &Vector3AddEq1);
			VVector3.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Vector3&", "float", &Vector3AddEq2);
			VVector3.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Vector3&", "const Vector3 &in", &Vector3SubEq1);
			VVector3.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Vector3&", "float", &Vector3SubEq2);
			VVector3.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Vector3", "const Vector3 &in", &Vector3Mul1);
			VVector3.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Vector3", "float", &Vector3Mul2);
			VVector3.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Vector3", "const Vector3 &in", &Vector3Div1);
			VVector3.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Vector3", "float", &Vector3Div2);
			VVector3.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Vector3", "const Vector3 &in", &Vector3Add1);
			VVector3.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Vector3", "float", &Vector3Add2);
			VVector3.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Vector3", "const Vector3 &in", &Vector3Sub1);
			VVector3.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Vector3", "float", &Vector3Sub2);
			VVector3.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "Vector3", "", &Vector3Neg);
			VVector3.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const Vector3 &in", &Vector3Eq);
			VVector3.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const Vector3 &in", &Vector3Cmp);
			Engine->EndNamespace();
			Engine->BeginNamespace("CU::Vector3");
			Register.SetFunction("Vector3 Random()", &Compute::Vector3::Random);
			Register.SetFunction("Vector3 RandomAbs()", &Compute::Vector3::RandomAbs);
			Register.SetFunction("Vector3 One()", &Compute::Vector3::One);
			Register.SetFunction("Vector3 Zero()", &Compute::Vector3::Zero);
			Register.SetFunction("Vector3 Up()", &Compute::Vector3::Up);
			Register.SetFunction("Vector3 Down()", &Compute::Vector3::Down);
			Register.SetFunction("Vector3 Left()", &Compute::Vector3::Left);
			Register.SetFunction("Vector3 Right()", &Compute::Vector3::Right);
			Register.SetFunction("Vector3 Forward()", &Compute::Vector3::Forward);
			Register.SetFunction("Vector3 Backward()", &Compute::Vector3::Backward);
			Engine->EndNamespace();

			return true;
		}
		bool CURegisterVector4(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CU");
			VMTypeClass VVector4 = Register.SetPod<Compute::Vector4>("Vector4");
			VVector4.SetConstructor<Compute::Vector4>("void f()");
			VVector4.SetConstructor<Compute::Vector4, float, float>("void f(float, float)");
			VVector4.SetConstructor<Compute::Vector4, float, float, float>("void f(float, float, float)");
			VVector4.SetConstructor<Compute::Vector4, float, float, float, float>("void f(float, float, float, float)");
			VVector4.SetConstructor<Compute::Vector4, float>("void f(float)");
			VVector4.SetConstructor<Compute::Vector4, const Compute::Vector2&>("void f(const Vector2 &in)");
			VVector4.SetConstructor<Compute::Vector4, const Compute::Vector3&>("void f(const Vector3 &in)");
			VVector4.SetProperty<Compute::Vector4>("float X", &Compute::Vector4::X);
			VVector4.SetProperty<Compute::Vector4>("float Y", &Compute::Vector4::Y);
			VVector4.SetProperty<Compute::Vector4>("float Z", &Compute::Vector4::Z);
			VVector4.SetProperty<Compute::Vector4>("float W", &Compute::Vector4::W);
			VVector4.SetMethod("float Length() const", &Compute::Vector4::Length);
			VVector4.SetMethod("float Sum() const", &Compute::Vector4::Sum);
			VVector4.SetMethod("float Dot(const Vector4 &in) const", &Compute::Vector4::Dot);
			VVector4.SetMethod("float Distance(const Vector4 &in) const", &Compute::Vector4::Distance);
			VVector4.SetMethod("float Cross(const Vector4 &in) const", &Compute::Vector4::Cross);
			VVector4.SetMethod("Vector4 Inv() const", &Compute::Vector4::Inv);
			VVector4.SetMethod("Vector4 InvX() const", &Compute::Vector4::InvX);
			VVector4.SetMethod("Vector4 InvY() const", &Compute::Vector4::InvY);
			VVector4.SetMethod("Vector4 InvZ() const", &Compute::Vector4::InvZ);
			VVector4.SetMethod("Vector4 InvW() const", &Compute::Vector4::InvW);
			VVector4.SetMethod("Vector4 Normalize() const", &Compute::Vector4::Normalize);
			VVector4.SetMethod("Vector4 sNormalize() const", &Compute::Vector4::sNormalize);
			VVector4.SetMethod("Vector4 Lerp(const Vector4 &in, float) const", &Compute::Vector4::Lerp);
			VVector4.SetMethod("Vector4 sLerp(const Vector4 &in, float) const", &Compute::Vector4::sLerp);
			VVector4.SetMethod("Vector4 aLerp(const Vector4 &in, float) const", &Compute::Vector4::aLerp);
			VVector4.SetMethod("Vector4 rLerp() const", &Compute::Vector4::rLerp);
			VVector4.SetMethod("Vector4 Abs() const", &Compute::Vector4::Abs);
			VVector4.SetMethod("Vector4 Radians() const", &Compute::Vector4::Radians);
			VVector4.SetMethod("Vector4 Degrees() const", &Compute::Vector4::Degrees);
			VVector4.SetMethod("Vector2 XY() const", &Compute::Vector4::XY);
			VVector4.SetMethod("Vector3 XYZ() const", &Compute::Vector4::XYZ);
			VVector4.SetMethod("Vector4 XYZW() const", &Compute::Vector4::XYZW);
			VVector4.SetMethod<Compute::Vector4, Compute::Vector4, float>("Vector4 Mul(float) const", &Compute::Vector4::Mul);
			VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector2&, float, float>("Vector4 Mul(const Vector2 &in, float, float) const", &Compute::Vector4::Mul);
			VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector3&, float>("Vector4 Mul(const Vector3 &in, float) const", &Compute::Vector4::Mul);
			VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector4&>("Vector4 Mul(const Vector4 &in) const", &Compute::Vector4::Mul);
			VVector4.SetMethod("Vector4 Div(const Vector4 &in) const", &Compute::Vector4::Div);
			VVector4.SetMethod("Vector4 SetX(float) const", &Compute::Vector4::SetX);
			VVector4.SetMethod("Vector4 SetY(float) const", &Compute::Vector4::SetY);
			VVector4.SetMethod("Vector4 SetZ(float) const", &Compute::Vector4::SetZ);
			VVector4.SetMethod("Vector4 SetW(float) const", &Compute::Vector4::SetW);
			VVector4.SetMethod("void Set(const Vector4 &in) const", &Compute::Vector4::Set);
			VVector4.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Vector4&", "const Vector4 &in", &Vector4MulEq1);
			VVector4.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Vector4&", "float", &Vector4MulEq2);
			VVector4.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Vector4&", "const Vector4 &in", &Vector4DivEq1);
			VVector4.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Vector4&", "float", &Vector4DivEq2);
			VVector4.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Vector4&", "const Vector4 &in", &Vector4AddEq1);
			VVector4.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Vector4&", "float", &Vector4AddEq2);
			VVector4.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Vector4&", "const Vector4 &in", &Vector4SubEq1);
			VVector4.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Vector4&", "float", &Vector4SubEq2);
			VVector4.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Vector4", "const Vector4 &in", &Vector4Mul1);
			VVector4.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Vector4", "float", &Vector4Mul2);
			VVector4.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Vector4", "const Vector4 &in", &Vector4Div1);
			VVector4.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Vector4", "float", &Vector4Div2);
			VVector4.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Vector4", "const Vector4 &in", &Vector4Add1);
			VVector4.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Vector4", "float", &Vector4Add2);
			VVector4.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Vector4", "const Vector4 &in", &Vector4Sub1);
			VVector4.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Vector4", "float", &Vector4Sub2);
			VVector4.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "Vector4", "", &Vector4Neg);
			VVector4.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const Vector4 &in", &Vector4Eq);
			VVector4.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const Vector4 &in", &Vector4Cmp);
			Engine->EndNamespace();
			Engine->BeginNamespace("CU::Vector4");
			Register.SetFunction("Vector4 Random()", &Compute::Vector4::Random);
			Register.SetFunction("Vector4 RandomAbs()", &Compute::Vector4::RandomAbs);
			Register.SetFunction("Vector4 One()", &Compute::Vector4::One);
			Register.SetFunction("Vector4 Zero()", &Compute::Vector4::Zero);
			Register.SetFunction("Vector4 Up()", &Compute::Vector4::Up);
			Register.SetFunction("Vector4 Down()", &Compute::Vector4::Down);
			Register.SetFunction("Vector4 Left()", &Compute::Vector4::Left);
			Register.SetFunction("Vector4 Right()", &Compute::Vector4::Right);
			Register.SetFunction("Vector4 Forward()", &Compute::Vector4::Forward);
			Register.SetFunction("Vector4 Backward()", &Compute::Vector4::Backward);
			Register.SetFunction("Vector4 UnitW()", &Compute::Vector4::UnitW);
			Engine->EndNamespace();

			return true;
		}
	}
}
