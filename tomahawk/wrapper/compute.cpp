#include "compute.h"
#include "../compute.h"

namespace Tomahawk
{
	namespace Wrapper
	{
		namespace Compute
		{
			void Enable(Script::VMManager* Manager)
			{
				Manager->Namespace("Compute", [](Script::VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Compute
							enum Hybi10_Opcode;
							enum Shape;
							enum MotionState;
							enum RegexState;
							enum RegexFlags;
							value Vertex;
							value InfluenceVertex;
							value ShapeVertex;
							value ElementVertex;
							value Vector2;
							value Vector3;
							value Vector4;
							value Matrix4x4;
							value Quaternion;
							value AnimatorKey;
							ref SkinAnimatorClip;
							ref KeyAnimatorClip;
							ref Joint;
							value RandomVector2;
							value RandomVector3;
							value RandomVector4;
							value RandomFloat;
							value Hybi10PayloadHeader;
							value Hybi10Request;
							value ShapeContact;
							value RayContact;
							ref RegexBracket;
							ref RegexBranch;
							ref RegexMatch;
							value RegExp;
							ref RegexResult;
							ref MD5Hasher;
							namespace Math<double>;
							namespace MathCommon;
							namespace Regex;
							ref CollisionShape;
							ref Transform;
							ref RigidBody;
							ref SliderConstraint;
							ref Simulator;
					*/


					return 0;
				});
			}
		}
	}
}