#include "processors.h"
#include "components.h"
#include "renderers.h"
#include "../network/http.h"
#ifdef VI_OPENAL
#ifdef VI_AL_AT_OPENAL
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif
#endif
#ifdef VI_SDL2
#include "../internal/sdl2-cross.hpp"
#endif
#ifdef VI_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h> 
#endif
#ifdef VI_STB
extern "C"
{
#define STB_VORBIS_HEADER_ONLY
#include <stb_image.h>
#include <stb_vorbis.c>
}
#endif

namespace Vitex
{
	namespace Engine
	{
		namespace Processors
		{
#ifdef VI_ASSIMP
			Compute::Matrix4x4 FromAssimpMatrix(const aiMatrix4x4& Root)
			{
				return Compute::Matrix4x4(
					Root.a1, Root.a2, Root.a3, Root.a4,
					Root.b1, Root.b2, Root.b3, Root.b4,
					Root.c1, Root.c2, Root.c3, Root.c4,
					Root.d1, Root.d2, Root.d3, Root.d4).Transpose();
			}
			Core::String GetMeshName(const Core::String& Name, ModelInfo* Info)
			{
				if (Name.empty())
				{
					auto Random = Compute::Crypto::RandomBytes(8);
					if (!Random)
						return "NULL";

					auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), *Random);
					if (!Hash)
						return "NULL";

					return Hash->substr(0, 8);
				}

				Core::String Result = Name;
				for (auto&& Data : Info->Meshes)
				{
					if (Data.Name == Result)
						Result += '_';
				}

				return Result;
			}
			Core::String GetJointName(const Core::String& BaseName, ModelInfo* Info, MeshBlob* Blob)
			{
				if (!BaseName.empty())
					return BaseName;

				Core::String Name = BaseName + '?';
				while (Info->JointOffsets.find(Name) != Info->JointOffsets.end())
					Name += '?';

				return Name;
			}
			bool GetKeyFromTime(Core::Vector<Compute::AnimatorKey>& Keys, float Time, Compute::AnimatorKey& Result)
			{
				for (auto& Key : Keys)
				{
					if (Key.Time == Time)
					{
						Result = Key;
						return true;
					}
					
					if (Key.Time < Time)
						Result = Key;
				}

				return false;
			}
			void UpdateSceneBounds(ModelInfo* Info)
			{
				if (Info->Min.X < Info->Low)
					Info->Low = Info->Min.X;

				if (Info->Min.Y < Info->Low)
					Info->Low = Info->Min.Y;

				if (Info->Min.Z < Info->Low)
					Info->Low = Info->Min.Z;

				if (Info->Max.X > Info->High)
					Info->High = Info->Max.X;

				if (Info->Max.Y > Info->High)
					Info->High = Info->Max.Y;

				if (Info->Max.Z > Info->High)
					Info->High = Info->Max.Z;
			}
			void UpdateSceneBounds(ModelInfo* Info, const Compute::SkinVertex& Element)
			{
				if (Element.PositionX > Info->Max.X)
					Info->Max.X = Element.PositionX;
				else if (Element.PositionX < Info->Min.X)
					Info->Min.X = Element.PositionX;

				if (Element.PositionY > Info->Max.Y)
					Info->Max.Y = Element.PositionY;
				else if (Element.PositionY < Info->Min.Y)
					Info->Min.Y = Element.PositionY;

				if (Element.PositionZ > Info->Max.Z)
					Info->Max.Z = Element.PositionZ;
				else if (Element.PositionZ < Info->Min.Z)
					Info->Min.Z = Element.PositionZ;
			}
			bool FillSceneSkeleton(ModelInfo* Info, aiNode* Node, Compute::Joint* Top)
			{
				Core::String Name = Node->mName.C_Str();
				auto It = Info->JointOffsets.find(Name);
				if (It == Info->JointOffsets.end())
				{
					if (Top != nullptr)
					{
						auto& Global = Info->JointOffsets[Name];
						Global.Index = Info->GlobalIndex++;
						Global.Linking = true;

						It = Info->JointOffsets.find(Name);
						goto AddLinkingJoint;
					}

					for (unsigned int i = 0; i < Node->mNumChildren; i++)
					{
						auto& Next = Node->mChildren[i];
						if (FillSceneSkeleton(Info, Next, Top))
							return true;
					}

					return false;
				}

				if (Top != nullptr)
				{
				AddLinkingJoint:
					Top->Childs.emplace_back();
					auto& Next = Top->Childs.back();
					Top = &Next;
				}
				else
					Top = &Info->Skeleton;

				Top->Global = FromAssimpMatrix(Node->mTransformation);
				Top->Local = It->second.Local;
				Top->Index = It->second.Index;
				Top->Name = Name;

				for (unsigned int i = 0; i < Node->mNumChildren; i++)
				{
					auto& Next = Node->mChildren[i];
					FillSceneSkeleton(Info, Next, Top);
				}

				return true;
			}
			void FillSceneGeometry(ModelInfo* Info, MeshBlob* Blob, aiMesh* Mesh)
			{
				Blob->Vertices.reserve((size_t)Mesh->mNumVertices);
				for (unsigned int v = 0; v < Mesh->mNumVertices; v++)
				{
					auto& Vertex = Mesh->mVertices[v];
					Compute::SkinVertex Next = { Vertex.x, Vertex.y, Vertex.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 };
					UpdateSceneBounds(Info, Next);

					if (Mesh->HasNormals())
					{
						auto& Normal = Mesh->mNormals[v];
						Next.NormalX = Normal.x;
						Next.NormalY = Normal.y;
						Next.NormalZ = Normal.z;
					}

					if (Mesh->HasTextureCoords(0))
					{
						auto& TexCoord = Mesh->mTextureCoords[0][v];
						Next.TexCoordX = TexCoord.x;
						Next.TexCoordY = -TexCoord.y;
					}

					if (Mesh->HasTangentsAndBitangents())
					{
						auto& Tangent = Mesh->mTangents[v];
						Next.TangentX = Tangent.x;
						Next.TangentY = Tangent.y;
						Next.TangentZ = Tangent.z;

						auto& Bitangent = Mesh->mBitangents[v];
						Next.BitangentX = Bitangent.x;
						Next.BitangentY = Bitangent.y;
						Next.BitangentZ = Bitangent.z;
					}

					Blob->Vertices.push_back(Next);
				}

				for (unsigned int f = 0; f < Mesh->mNumFaces; f++)
				{
					auto* Face = &Mesh->mFaces[f];
					Blob->Indices.reserve(Blob->Indices.size() + (size_t)Face->mNumIndices);
					for (unsigned int i = 0; i < Face->mNumIndices; i++)
						Blob->Indices.push_back(Face->mIndices[i]);
				}
			}
			void FillSceneJoints(ModelInfo* Info, MeshBlob* Blob, aiMesh* Mesh)
			{
				for (unsigned int i = 0; i < Mesh->mNumBones; i++)
				{
					auto& Bone = Mesh->mBones[i];
					auto Name = GetJointName(Bone->mName.C_Str(), Info, Blob);

					auto Global = Info->JointOffsets.find(Name);
					if (Global == Info->JointOffsets.end())
					{
						auto& Next = Info->JointOffsets[Name];
						Next.Local = FromAssimpMatrix(Bone->mOffsetMatrix);
						Next.Index = Info->GlobalIndex++;
						Next.Linking = false;
						Global = Info->JointOffsets.find(Name);
					}

					auto& Local = Blob->JointIndices[Global->second.Index];
					Local = Blob->LocalIndex++;

					for (unsigned int j = 0; j < Bone->mNumWeights; j++)
					{
						auto& Weight = Bone->mWeights[j];
						auto& Vertex = Blob->Vertices[Weight.mVertexId];
						if (Vertex.JointIndex0 == -1.0f)
						{
							Vertex.JointIndex0 = (float)Local;
							Vertex.JointBias0 = Weight.mWeight;
						}
						else if (Vertex.JointIndex1 == -1.0f)
						{
							Vertex.JointIndex1 = (float)Local;
							Vertex.JointBias1 = Weight.mWeight;
						}
						else if (Vertex.JointIndex2 == -1.0f)
						{
							Vertex.JointIndex2 = (float)Local;
							Vertex.JointBias2 = Weight.mWeight;
						}
						else if (Vertex.JointIndex3 == -1.0f)
						{
							Vertex.JointIndex3 = (float)Local;
							Vertex.JointBias3 = Weight.mWeight;
						}
					}
				}
			}
			void FillSceneGeometries(ModelInfo* Info, const aiScene* Scene, aiNode* Node, const aiMatrix4x4& ParentTransform)
			{
				aiMatrix4x4 Transform = ParentTransform * Node->mTransformation;
				Info->Meshes.reserve(Info->Meshes.size() + (size_t)Node->mNumMeshes);
				if (Node == Scene->mRootNode)
					Info->Transform = FromAssimpMatrix(Scene->mRootNode->mTransformation).Inv();

				for (unsigned int n = 0; n < Node->mNumMeshes; n++)
				{
					Info->Meshes.emplace_back();
					MeshBlob& Blob = Info->Meshes.back();
					auto& Geometry = Scene->mMeshes[Node->mMeshes[n]];
					Blob.Name = GetMeshName(Geometry->mName.C_Str(), Info);
					Blob.Transform = FromAssimpMatrix(ParentTransform);

					FillSceneGeometry(Info, &Blob, Geometry);
					FillSceneJoints(Info, &Blob, Geometry);
					UpdateSceneBounds(Info);
				}

				for (unsigned int n = 0; n < Node->mNumChildren; n++)
				{
					auto& Next = Node->mChildren[n];
					FillSceneGeometries(Info, Scene, Next, ParentTransform);
				}
			}
			void FillSceneSkeletons(ModelInfo* Info, const aiScene* Scene)
			{
				FillSceneSkeleton(Info, Scene->mRootNode, nullptr);
				for (auto& Blob : Info->Meshes)
				{
					for (auto& Vertex : Blob.Vertices)
					{
						float Weight = 0.0f;
						if (Vertex.JointBias0 > 0.0f)
							Weight += Vertex.JointBias0;
						if (Vertex.JointBias1 > 0.0f)
							Weight += Vertex.JointBias1;
						if (Vertex.JointBias2 > 0.0f)
							Weight += Vertex.JointBias2;
						if (Vertex.JointBias3 > 0.0f)
							Weight += Vertex.JointBias3;

						if (!Weight)
							continue;

						if (Vertex.JointBias0 > 0.0f)
							Vertex.JointBias0 /= Weight;
						if (Vertex.JointBias1 > 0.0f)
							Vertex.JointBias1 /= Weight;
						if (Vertex.JointBias2 > 0.0f)
							Vertex.JointBias2 /= Weight;
						if (Vertex.JointBias3 > 0.0f)
							Vertex.JointBias3 /= Weight;
					}
				}
			}
			void FillSceneChannel(aiNodeAnim* Channel, ModelChannel& Target)
			{
				Target.Positions.reserve((size_t)Channel->mNumPositionKeys);
				for (unsigned int k = 0; k < Channel->mNumPositionKeys; k++)
				{
					aiVectorKey& Key = Channel->mPositionKeys[k];
					Target.Positions[Key.mTime] = Compute::Vector3(Key.mValue.x, Key.mValue.y, Key.mValue.z);
				}

				Target.Scales.reserve((size_t)Channel->mNumScalingKeys);
				for (unsigned int k = 0; k < Channel->mNumScalingKeys; k++)
				{
					aiVectorKey& Key = Channel->mScalingKeys[k];
					Target.Scales[Key.mTime] = Compute::Vector3(Key.mValue.x, Key.mValue.y, Key.mValue.z);
				}

				Target.Rotations.reserve((size_t)Channel->mNumRotationKeys);
				for (unsigned int k = 0; k < Channel->mNumRotationKeys; k++)
				{
					aiQuatKey& Key = Channel->mRotationKeys[k];
					Target.Rotations[Key.mTime] = Compute::Quaternion(Key.mValue.x, Key.mValue.y, Key.mValue.z, Key.mValue.w);
				}
			}
			void FillSceneTimeline(const Core::UnorderedSet<float>& Timings, Core::Vector<float>& Timeline)
			{
				Timeline.reserve(Timings.size());
				for (auto& Time : Timings)
					Timeline.push_back(Time);

				VI_SORT(Timeline.begin(), Timeline.end(), [](float A, float B)
				{
					return A < B;
				});
			}
			void FillSceneKeys(ModelChannel& Info, Core::Vector<Compute::AnimatorKey>& Keys)
			{
				Core::UnorderedSet<float> Timings;
				Timings.reserve(Keys.size());

				float FirstPosition = std::numeric_limits<float>::max();
				for (auto& Item : Info.Positions)
				{
					Timings.insert(Item.first);
					if (Item.first < FirstPosition)
						FirstPosition = Item.first;
				}

				float FirstScale = std::numeric_limits<float>::max();
				for (auto& Item : Info.Scales)
				{
					Timings.insert(Item.first);
					if (Item.first < FirstScale)
						FirstScale = Item.first;
				}

				float FirstRotation = std::numeric_limits<float>::max();
				for (auto& Item : Info.Rotations)
				{
					Timings.insert(Item.first);
					if (Item.first < FirstRotation)
						FirstRotation = Item.first;
				}

				Core::Vector<float> Timeline;
				Compute::Vector3 LastPosition = (Info.Positions.empty() ? Compute::Vector3::Zero() : Info.Positions[FirstPosition]);
				Compute::Vector3 LastScale = (Info.Scales.empty() ? Compute::Vector3::One() : Info.Scales[FirstScale]);
				Compute::Quaternion LastRotation = (Info.Rotations.empty() ? Compute::Quaternion() : Info.Rotations[FirstRotation]);
				FillSceneTimeline(Timings, Timeline);
				Keys.resize(Timings.size());

				size_t Index = 0;
				for (auto& Time : Timeline)
				{
					auto& Target = Keys[Index++];
					Target.Position = LastPosition;
					Target.Scale = LastScale;
					Target.Rotation = LastRotation;
					Target.Time = Time;

					auto Position = Info.Positions.find(Time);
					if (Position != Info.Positions.end())
					{
						Target.Position = Position->second;
						LastPosition = Target.Position;
					}

					auto Scale = Info.Scales.find(Time);
					if (Scale != Info.Scales.end())
					{
						Target.Scale = Scale->second;
						LastScale = Target.Scale;
					}

					auto Rotation = Info.Rotations.find(Time);
					if (Rotation != Info.Rotations.end())
					{
						Target.Rotation = Rotation->second;
						LastRotation = Target.Rotation;
					}
				}
			}
			void FillSceneClip(Compute::SkinAnimatorClip& Clip, Core::UnorderedMap<Core::String, MeshBone>& Indices, Core::UnorderedMap<Core::String, Core::Vector<Compute::AnimatorKey>>& Channels)
			{
				Core::UnorderedSet<float> Timings;
				for (auto& Channel : Channels)
				{
					Timings.reserve(Channel.second.size());
					for (auto& Key : Channel.second)
						Timings.insert(Key.Time);
				}

				Core::Vector<float> Timeline;
				FillSceneTimeline(Timings, Timeline);

				for (auto& Time : Timeline)
				{
					Clip.Keys.emplace_back();
					auto& Key = Clip.Keys.back();
					Key.Pose.resize(Indices.size());
					Key.Time = Time;

					for (auto& Index : Indices)
					{
						auto& Pose = Key.Pose[Index.second.Index];
						Pose.Position = Index.second.Default.Position;
						Pose.Scale = Index.second.Default.Scale;
						Pose.Rotation = Index.second.Default.Rotation;
						Pose.Time = Time;
					}

					for (auto& Channel : Channels)
					{
						auto Index = Indices.find(Channel.first);
						if (Index == Indices.end())
							continue;

						auto& Next = Key.Pose[Index->second.Index];
						if (GetKeyFromTime(Channel.second, Time, Next))
							Next.Time = Time;
					}
				}
			}
			void FillSceneJointIndices(const aiScene* Scene, aiNode* Node, Core::UnorderedMap<Core::String, MeshBone>& Indices, size_t& Index)
			{
				for (unsigned int n = 0; n < Node->mNumMeshes; n++)
				{
					auto& Mesh = Scene->mMeshes[Node->mMeshes[n]];
					for (unsigned int i = 0; i < Mesh->mNumBones; i++)
					{
						auto& Bone = Mesh->mBones[i];
						auto Joint = Indices.find(Bone->mName.C_Str());
						if (Joint == Indices.end())
							Indices[Bone->mName.C_Str()].Index = Index++;
					}
				}

				for (unsigned int n = 0; n < Node->mNumChildren; n++)
				{
					auto& Next = Node->mChildren[n];
					FillSceneJointIndices(Scene, Next, Indices, Index);
				}
			}
			bool FillSceneJointDefaults(aiNode* Node, Core::UnorderedMap<Core::String, MeshBone>& Indices, size_t& Index, bool InSkeleton)
			{
				Core::String Name = Node->mName.C_Str();
				auto It = Indices.find(Name);
				if (It == Indices.end())
				{
					if (InSkeleton)
					{
						auto& Joint = Indices[Name];
						Joint.Index = Index++;
						It = Indices.find(Name);
						goto AddLinkingJoint;
					}

					for (unsigned int i = 0; i < Node->mNumChildren; i++)
					{
						auto& Next = Node->mChildren[i];
						if (FillSceneJointDefaults(Next, Indices, Index, InSkeleton))
							return true;
					}

					return false;
				}

			AddLinkingJoint:
				auto Offset = FromAssimpMatrix(Node->mTransformation);
				It->second.Default.Position = Offset.Position();
				It->second.Default.Scale = Offset.Scale();
				It->second.Default.Rotation = Offset.RotationQuaternion();

				for (unsigned int i = 0; i < Node->mNumChildren; i++)
				{
					auto& Next = Node->mChildren[i];
					FillSceneJointDefaults(Next, Indices, Index, true);
				}

				return true;
			}
			void FillSceneAnimations(Core::Vector<Compute::SkinAnimatorClip>* Info, const aiScene* Scene)
			{
				Core::UnorderedMap<Core::String, MeshBone> Indices; size_t Index = 0;
				FillSceneJointIndices(Scene, Scene->mRootNode, Indices, Index);
				FillSceneJointDefaults(Scene->mRootNode, Indices, Index, false);

				Info->reserve((size_t)Scene->mNumAnimations);
				for (unsigned int i = 0; i < Scene->mNumAnimations; i++)
				{
					aiAnimation* Animation = Scene->mAnimations[i];
					Info->emplace_back();

					auto& Clip = Info->back();
					Clip.Name = Animation->mName.C_Str();
					Clip.Duration = (float)Animation->mDuration;
					Clip.Rate = Compute::Mathf::Max(0.01f, (float)Animation->mTicksPerSecond);

					Core::UnorderedMap<Core::String, Core::Vector<Compute::AnimatorKey>> Channels;
					for (unsigned int j = 0; j < Animation->mNumChannels; j++)
					{
						auto& Channel = Animation->mChannels[j];
						auto& Frames = Channels[Channel->mNodeName.C_Str()];

						ModelChannel Target;
						FillSceneChannel(Channel, Target);
						FillSceneKeys(Target, Frames);
					}
					FillSceneClip(Clip, Indices, Channels);
				}
			}
#endif
			Core::Vector<Compute::Vertex> SkinVerticesToVertices(const Core::Vector<Compute::SkinVertex>& Data)
			{
				Core::Vector<Compute::Vertex> Result;
				Result.resize(Data.size());

				for (size_t i = 0; i < Data.size(); i++)
				{
					auto& From = Data[i];
					auto& To = Result[i];
					To.PositionX = From.PositionX;
					To.PositionY = From.PositionY;
					To.PositionZ = From.PositionZ;
					To.TexCoordX = From.TexCoordX;
					To.TexCoordY = From.TexCoordY;
					To.NormalX = From.NormalX;
					To.NormalY = From.NormalY;
					To.NormalZ = From.NormalZ;
					To.TangentX = From.TangentX;
					To.TangentY = From.TangentY;
					To.TangentZ = From.TangentZ;
					To.BitangentX = From.BitangentX;
					To.BitangentY = From.BitangentY;
					To.BitangentZ = From.BitangentZ;
				}

				return Result;
			}
			template <typename T>
			T ProcessRendererJob(Graphics::GraphicsDevice* Device, std::function<T(Graphics::GraphicsDevice*)>&& Callback)
			{
				Core::Promise<T> Future;
				Graphics::RenderThreadCallback Job = [Future, Callback = std::move(Callback)](Graphics::GraphicsDevice* Device) mutable
				{
					Future.Set(Callback(Device));
				};

				auto* App = Application::Get();
				if (!App || App->GetState() != ApplicationState::Active || Device != App->Renderer)
					Device->Lockup(std::move(Job));
				else
					Device->Enqueue(std::move(Job));

				return Future.Get();
			}

			AssetProcessor::AssetProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> AssetProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");

				Core::Vector<char> Temp;
				Stream->ReadAll([&Temp](char* Buffer, size_t Size)
				{
					Temp.reserve(Temp.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Temp.push_back(Buffer[i]);
				});

				char* Data = VI_MALLOC(char, sizeof(char) * Temp.size());
				memcpy(Data, Temp.data(), sizeof(char) * Temp.size());
				return new AssetFile(Data, Temp.size());
			}

			MaterialProcessor::MaterialProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> MaterialProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((Engine::Material*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> MaterialProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				auto Data = Content->Load<Core::Schema>(Stream->VirtualName());
				if (!Data)
					return Data.Error();

				Core::String Path;
				Engine::Material* Object = new Engine::Material(nullptr);
				if (Series::Unpack(Data->Get("diffuse-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetDiffuseMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				if (Series::Unpack(Data->Get("normal-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetNormalMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				if (Series::Unpack(Data->Get("metallic-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetMetallicMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				if (Series::Unpack(Data->Get("roughness-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetRoughnessMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				if (Series::Unpack(Data->Get("height-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetHeightMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				if (Series::Unpack(Data->Get("occlusion-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetOcclusionMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				if (Series::Unpack(Data->Get("emission-map"), &Path) && !Path.empty())
				{
					auto NewTexture = Content->Load<Graphics::Texture2D>(Path);
					if (!NewTexture)
					{
						VI_RELEASE(Data);
						return NewTexture.Error();
					}

					Object->SetEmissionMap(*NewTexture);
					VI_RELEASE(NewTexture);
				}

				Core::String Name;
				Series::Unpack(Data->Get("emission"), &Object->Surface.Emission);
				Series::Unpack(Data->Get("metallic"), &Object->Surface.Metallic);
				Series::Unpack(Data->Get("diffuse"), &Object->Surface.Diffuse);
				Series::Unpack(Data->Get("scatter"), &Object->Surface.Scatter);
				Series::Unpack(Data->Get("roughness"), &Object->Surface.Roughness);
				Series::Unpack(Data->Get("occlusion"), &Object->Surface.Occlusion);
				Series::Unpack(Data->Get("fresnel"), &Object->Surface.Fresnel);
				Series::Unpack(Data->Get("refraction"), &Object->Surface.Refraction);
				Series::Unpack(Data->Get("transparency"), &Object->Surface.Transparency);
				Series::Unpack(Data->Get("environment"), &Object->Surface.Environment);
				Series::Unpack(Data->Get("radius"), &Object->Surface.Radius);
				Series::Unpack(Data->Get("height"), &Object->Surface.Height);
				Series::Unpack(Data->Get("bias"), &Object->Surface.Bias);
				Series::Unpack(Data->Get("name"), &Name);
				Object->SetName(Name);
				VI_RELEASE(Data);

				auto* Existing = (Engine::Material*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			ExpectsContent<void> MaterialProcessor::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_ASSERT(Instance != nullptr, "instance should be set");

				Engine::Material* Object = (Engine::Material*)Instance;
				Core::Schema* Data = Core::Var::Set::Object();
				Data->Key = "material";

				AssetCache* Asset = Content->FindCache<Graphics::Texture2D>(Object->GetDiffuseMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("diffuse-map"), Asset->Path);

				Asset = Content->FindCache<Graphics::Texture2D>(Object->GetNormalMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("normal-map"), Asset->Path);

				Asset = Content->FindCache<Graphics::Texture2D>(Object->GetMetallicMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("metallic-map"), Asset->Path);

				Asset = Content->FindCache<Graphics::Texture2D>(Object->GetRoughnessMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("roughness-map"), Asset->Path);

				Asset = Content->FindCache<Graphics::Texture2D>(Object->GetHeightMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("height-map"), Asset->Path);

				Asset = Content->FindCache<Graphics::Texture2D>(Object->GetOcclusionMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("occlusion-map"), Asset->Path);

				Asset = Content->FindCache<Graphics::Texture2D>(Object->GetEmissionMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("emission-map"), Asset->Path);

				Series::Pack(Data->Set("emission"), Object->Surface.Emission);
				Series::Pack(Data->Set("metallic"), Object->Surface.Metallic);
				Series::Pack(Data->Set("diffuse"), Object->Surface.Diffuse);
				Series::Pack(Data->Set("scatter"), Object->Surface.Scatter);
				Series::Pack(Data->Set("roughness"), Object->Surface.Roughness);
				Series::Pack(Data->Set("occlusion"), Object->Surface.Occlusion);
				Series::Pack(Data->Set("fresnel"), Object->Surface.Fresnel);
				Series::Pack(Data->Set("refraction"), Object->Surface.Refraction);
				Series::Pack(Data->Set("transparency"), Object->Surface.Transparency);
				Series::Pack(Data->Set("environment"), Object->Surface.Environment);
				Series::Pack(Data->Set("radius"), Object->Surface.Radius);
				Series::Pack(Data->Set("height"), Object->Surface.Height);
				Series::Pack(Data->Set("bias"), Object->Surface.Bias);
				Series::Pack(Data->Set("name"), Object->GetName());

				auto Status = Content->Save<Core::Schema>(Stream->VirtualName(), Data, Args);
				VI_RELEASE(Data);
				return Status;
			}
			void MaterialProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((Engine::Material*)Asset->Resource);
				Asset->Resource = nullptr;
			}

			SceneGraphProcessor::SceneGraphProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> SceneGraphProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				Engine::SceneGraph::Desc I = Engine::SceneGraph::Desc::Get(Application::Get());
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_ASSERT(I.Shared.Device != nullptr, "graphics device should be set");

				auto Blob = Content->Load<Core::Schema>(Stream->VirtualName());
				if (!Blob)
					return Blob.Error();

				Core::Schema* Metadata = Blob->Find("metadata");
				if (Metadata != nullptr)
				{
					Core::Schema* Simulator = Metadata->Find("simulator");
					if (Simulator != nullptr)
					{
						Series::Unpack(Simulator->Find("enable-soft-body"), &I.Simulator.EnableSoftBody);
						Series::Unpack(Simulator->Find("max-displacement"), &I.Simulator.MaxDisplacement);
						Series::Unpack(Simulator->Find("air-density"), &I.Simulator.AirDensity);
						Series::Unpack(Simulator->Find("water-offset"), &I.Simulator.WaterOffset);
						Series::Unpack(Simulator->Find("water-density"), &I.Simulator.WaterDensity);
						Series::Unpack(Simulator->Find("water-normal"), &I.Simulator.WaterNormal);
						Series::Unpack(Simulator->Find("gravity"), &I.Simulator.Gravity);
					}

					Series::Unpack(Metadata->Find("materials"), &I.StartMaterials);
					Series::Unpack(Metadata->Find("entities"), &I.StartEntities);
					Series::Unpack(Metadata->Find("components"), &I.StartComponents);
					Series::Unpack(Metadata->Find("render-quality"), &I.RenderQuality);
					Series::Unpack(Metadata->Find("enable-hdr"), &I.EnableHDR);
					Series::Unpack(Metadata->Find("grow-margin"), &I.GrowMargin);
					Series::Unpack(Metadata->Find("grow-rate"), &I.GrowRate);
					Series::Unpack(Metadata->Find("max-updates"), &I.MaxUpdates);
					Series::Unpack(Metadata->Find("voxels-size"), &I.VoxelsSize);
					Series::Unpack(Metadata->Find("voxels-max"), &I.VoxelsMax);
					Series::Unpack(Metadata->Find("points-size"), &I.PointsSize);
					Series::Unpack(Metadata->Find("points-max"), &I.PointsMax);
					Series::Unpack(Metadata->Find("spots-size"), &I.SpotsSize);
					Series::Unpack(Metadata->Find("spots-max"), &I.SpotsMax);
					Series::Unpack(Metadata->Find("line-size"), &I.LinesSize);
					Series::Unpack(Metadata->Find("lines-max"), &I.LinesMax);
				}

				bool IntegrityCheck = false;
				auto EnsureIntegrity = Args.find("integrity");
				if (EnsureIntegrity != Args.end())
					IntegrityCheck = EnsureIntegrity->second.GetBoolean();

				auto HasMutations = Args.find("mutations");
				if (HasMutations != Args.end())
					I.Mutations = HasMutations->second.GetBoolean();

				Engine::SceneGraph* Object = new Engine::SceneGraph(I);
				Engine::IdxSnapshot Snapshot;
				Object->Snapshot = &Snapshot;

				auto IsActive = Args.find("active");
				if (IsActive != Args.end())
					Object->SetActive(IsActive->second.GetBoolean());

				Core::Schema* Materials = Blob->Find("materials");
				if (Materials != nullptr)
				{
					Core::Vector<Core::Schema*> Collection = Materials->FindCollection("material");
					for (auto& It : Collection)
					{
						Core::String Path;
						if (!Series::Unpack(It, &Path) || Path.empty())
							continue;

						auto Value = Content->Load<Engine::Material>(Path);
						if (Value)
						{
							Series::Unpack(It, &Value->Slot);
							Object->AddMaterial(*Value);
						}
						else if (IntegrityCheck)
						{
							VI_RELEASE(Blob);
							return Value.Error();
						}
					}
				}

				Core::Schema* Entities = Blob->Find("entities");
				if (Entities != nullptr)
				{
					Core::Vector<Core::Schema*> Collection = Entities->FindCollection("entity");
					for (auto& It : Collection)
					{
						Entity* Entity = Object->AddEntity();
						int64_t Refer = -1;

						if (Series::Unpack(It->Find("refer"), &Refer) && Refer >= 0)
						{
							Snapshot.To[Entity] = (size_t)Refer;
							Snapshot.From[(size_t)Refer] = Entity;
						}
					}

					size_t Next = 0;
					for (auto& It : Collection)
					{
						Entity* Entity = Object->GetEntity(Next++);
						if (!Entity)
							continue;

						Core::String Name;
						Series::Unpack(It->Find("name"), &Name);
						Entity->SetName(Name);

						Core::Schema* Transform = It->Find("transform");
						if (Transform != nullptr)
						{
							Compute::Transform* Offset = Entity->GetTransform();
							Compute::Transform::Spacing& Space = Offset->GetSpacing(Compute::Positioning::Global);
							bool Scaling = Offset->HasScaling();
							Series::Unpack(Transform->Find("position"), &Space.Position);
							Series::Unpack(Transform->Find("rotation"), &Space.Rotation);
							Series::Unpack(Transform->Find("scale"), &Space.Scale);
							Series::Unpack(Transform->Find("scaling"), &Scaling);
							Offset->SetScaling(Scaling);
						}

						Core::Schema* Parent = It->Find("parent");
						if (Parent != nullptr)
						{
							Compute::Transform* Root = nullptr;
							Compute::Transform::Spacing* Space = VI_NEW(Compute::Transform::Spacing);
							Series::Unpack(Parent->Find("position"), &Space->Position);
							Series::Unpack(Parent->Find("rotation"), &Space->Rotation);
							Series::Unpack(Parent->Find("scale"), &Space->Scale);
							Series::Unpack(Parent->Find("world"), &Space->Offset);

							size_t Where = 0;
							if (Series::Unpack(Parent->Find("where"), &Where))
							{
								auto It = Snapshot.From.find(Where);
								if (It != Snapshot.From.end() && It->second != Entity)
									Root = It->second->GetTransform();
							}

							Compute::Transform* Offset = Entity->GetTransform();
							Offset->SetPivot(Root, Space);
							Offset->MakeDirty();
						}

						Core::Schema* Components = It->Find("components");
						if (Components != nullptr)
						{
							Core::Vector<Core::Schema*> Elements = Components->FindCollection("component");
							for (auto& Element : Elements)
							{
								uint64_t Id;
								if (!Series::Unpack(Element->Find("id"), &Id))
									continue;

								Component* Target = Core::Composer::Create<Component>(Id, Entity);
								if (!Entity->AddComponent(Target))
									continue;

								bool Active = true;
								if (Series::Unpack(Element->Find("active"), &Active))
									Target->SetActive(Active);

								Core::Schema* Meta = Element->Find("metadata");
								if (!Meta)
									Meta = Element->Set("metadata");
								Target->Deserialize(Meta);
							}
						}
					}
				}

				VI_RELEASE(Blob);
				Object->Snapshot = nullptr;
				Object->Actualize();
				return Object;
			}
			ExpectsContent<void> SceneGraphProcessor::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_ASSERT(Instance != nullptr, "instance should be set");

				const char* Ext = Core::OS::Path::GetExtension(Stream->VirtualName().c_str());
				if (!Ext)
				{
					auto Type = Args.find("type");
					if (Type->second == Core::Var::String("XML"))
						Ext = ".xml";
					else if (Type->second == Core::Var::String("JSON"))
						Ext = ".json";
					else if (Type->second == Core::Var::String("JSONB"))
						Ext = ".jsonb";
					else
						Ext = ".xml";
				}

				Engine::SceneGraph* Object = (Engine::SceneGraph*)Instance;
				Object->Actualize();

				Engine::IdxSnapshot Snapshot;
				Object->MakeSnapshot(&Snapshot);
				Object->Snapshot = &Snapshot;

				Core::Schema* Blob = Core::Var::Set::Object();
				Blob->Key = "scene";

				auto& Conf = Object->GetConf();
				Core::Schema* Metadata = Blob->Set("metadata");
				Series::Pack(Metadata->Set("materials"), Conf.StartMaterials);
				Series::Pack(Metadata->Set("entities"), Conf.StartEntities);
				Series::Pack(Metadata->Set("components"), Conf.StartComponents);
				Series::Pack(Metadata->Set("render-quality"), Conf.RenderQuality);
				Series::Pack(Metadata->Set("enable-hdr"), Conf.EnableHDR);
				Series::Pack(Metadata->Set("grow-margin"), Conf.GrowMargin);
				Series::Pack(Metadata->Set("grow-rate"), Conf.GrowRate);
				Series::Pack(Metadata->Set("max-updates"), Conf.MaxUpdates);
				Series::Pack(Metadata->Set("voxels-size"), Conf.VoxelsSize);
				Series::Pack(Metadata->Set("voxels-max"), Conf.VoxelsMax);
				Series::Pack(Metadata->Set("points-size"), Conf.PointsSize);
				Series::Pack(Metadata->Set("points-max"), Conf.PointsMax);
				Series::Pack(Metadata->Set("spots-size"), Conf.SpotsSize);
				Series::Pack(Metadata->Set("spots-max"), Conf.SpotsMax);
				Series::Pack(Metadata->Set("line-size"), Conf.LinesSize);
				Series::Pack(Metadata->Set("lines-max"), Conf.LinesMax);

				auto* fSimulator = Object->GetSimulator();
				Core::Schema* Simulator = Metadata->Set("simulator");
				Series::Pack(Simulator->Set("enable-soft-body"), fSimulator->HasSoftBodySupport());
				Series::Pack(Simulator->Set("max-displacement"), fSimulator->GetMaxDisplacement());
				Series::Pack(Simulator->Set("air-density"), fSimulator->GetAirDensity());
				Series::Pack(Simulator->Set("water-offset"), fSimulator->GetWaterOffset());
				Series::Pack(Simulator->Set("water-density"), fSimulator->GetWaterDensity());
				Series::Pack(Simulator->Set("water-normal"), fSimulator->GetWaterNormal());
				Series::Pack(Simulator->Set("gravity"), fSimulator->GetGravity());

				Core::Schema* Materials = Blob->Set("materials", Core::Var::Array());
				for (size_t i = 0; i < Object->GetMaterialsCount(); i++)
				{
					Engine::Material* Material = Object->GetMaterial(i);
					if (!Material || Material == Object->GetInvalidMaterial())
						continue;

					Core::String Path;
					AssetCache* Asset = Content->FindCache<Engine::Material>(Material);
					if (!Asset)
						Path.assign("./materials/" + Material->GetName() + ".modified");
					else
						Path.assign(Asset->Path);

					if (!Core::Stringify::EndsWith(Path, Ext))
						Path.append(Ext);

					if (Content->Save<Engine::Material>(Path, Material, Args))
					{
						Core::Schema* Where = Materials->Set("material");
						Series::Pack(Where, Material->Slot);
						Series::Pack(Where, Path);
					}
				}

				Core::Schema* Entities = Blob->Set("entities", Core::Var::Array());
				for (size_t i = 0; i < Object->GetEntitiesCount(); i++)
				{
					Entity* Ref = Object->GetEntity(i);
					auto* Offset = Ref->GetTransform();

					Core::Schema* Entity = Entities->Set("entity");
					Series::Pack(Entity->Set("name"), Ref->GetName());
					Series::Pack(Entity->Set("refer"), i);

					Core::Schema* Transform = Entity->Set("transform");
					Series::Pack(Transform->Set("position"), Offset->GetPosition());
					Series::Pack(Transform->Set("rotation"), Offset->GetRotation());
					Series::Pack(Transform->Set("scale"), Offset->GetScale());
					Series::Pack(Transform->Set("scaling"), Offset->HasScaling());

					if (Offset->GetRoot() != nullptr)
					{
						Core::Schema* Parent = Entity->Set("parent");
						if (Offset->GetRoot()->UserData != nullptr)
						{
							auto It = Snapshot.To.find((Engine::Entity*)Offset->GetRoot());
							if (It != Snapshot.To.end())
								Series::Pack(Parent->Set("where"), It->second);
						}

						Compute::Transform::Spacing& Space = Offset->GetSpacing();
						Series::Pack(Parent->Set("position"), Space.Position);
						Series::Pack(Parent->Set("rotation"), Space.Rotation);
						Series::Pack(Parent->Set("scale"), Space.Scale);
						Series::Pack(Parent->Set("world"), Space.Offset);
					}

					if (!Ref->GetComponentsCount())
						continue;

					Core::Schema* Components = Entity->Set("components", Core::Var::Array());
					for (auto& Item : *Ref)
					{
						Core::Schema* Component = Components->Set("component");
						Series::Pack(Component->Set("id"), Item.second->GetId());
						Series::Pack(Component->Set("active"), Item.second->IsActive());
						Item.second->Serialize(Component->Set("metadata"));
					}
				}

				Object->Snapshot = nullptr;
				auto Status = Content->Save<Core::Schema>(Stream->VirtualName(), Blob, Args);
				VI_RELEASE(Blob);
				return Status;
			}

			AudioClipProcessor::AudioClipProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			AudioClipProcessor::~AudioClipProcessor()
			{
			}
			ExpectsContent<void*> AudioClipProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "asset resource should be set");

				((Audio::AudioClip*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> AudioClipProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				if (Core::Stringify::EndsWith(Stream->VirtualName(), ".wav"))
					return DeserializeWAVE(Stream, Offset, Args);
				else if (Core::Stringify::EndsWith(Stream->VirtualName(), ".ogg"))
					return DeserializeOGG(Stream, Offset, Args);

				return ContentException("deserialize audio unsupported: " + Core::String(Core::OS::Path::GetExtension(Stream->VirtualName().c_str())));
			}
			ExpectsContent<void*> AudioClipProcessor::DeserializeWAVE(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
#ifdef VI_SDL2
				Core::Vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				SDL_RWops* WavData = SDL_RWFromMem(Data.data(), (int)Data.size());
				SDL_AudioSpec WavInfo;
				Uint8* WavSamples;
				Uint32 WavCount;

				if (!SDL_LoadWAV_RW(WavData, 1, &WavInfo, &WavSamples, &WavCount))
				{
					SDL_RWclose(WavData);
					return ContentException(std::move(Graphics::VideoException().message()));
				}

				int Format = 0;
#ifdef VI_OPENAL
				switch (WavInfo.format)
				{
					case AUDIO_U8:
					case AUDIO_S8:
						Format = WavInfo.channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
						break;
					case AUDIO_U16:
					case AUDIO_S16:
						Format = WavInfo.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
						break;
					default:
						SDL_FreeWAV(WavSamples);
						SDL_RWclose(WavData);
						return ContentException("load wave audio: unsupported audio format");
				}
#endif
				Audio::AudioClip* Object = new Audio::AudioClip(1, Format);
				Audio::AudioContext::SetBufferData(Object->GetBuffer(), (int)Format, (const void*)WavSamples, (int)WavCount, (int)WavInfo.freq);
				SDL_FreeWAV(WavSamples);
				SDL_RWclose(WavData);

				auto* Existing = (Audio::AudioClip*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
#else
				return ContentException("load wave audio: unsupported");
#endif
			}
			ExpectsContent<void*> AudioClipProcessor::DeserializeOGG(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
#ifdef VI_STB
				VI_ASSERT(Stream != nullptr, "stream should be set");
				Core::Vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				short* Buffer;
				int Channels, SampleRate;
				int Samples = stb_vorbis_decode_memory((const unsigned char*)Data.data(), (int)Data.size(), &Channels, &SampleRate, &Buffer);
				if (Samples <= 0)
					return ContentException("load ogg audio: invalid file");

				int Format = 0;
#ifdef VI_OPENAL
				if (Channels == 2)
					Format = AL_FORMAT_STEREO16;
				else
					Format = AL_FORMAT_MONO16;
#endif
				Audio::AudioClip* Object = new Audio::AudioClip(1, Format);
				Audio::AudioContext::SetBufferData(Object->GetBuffer(), (int)Format, (const void*)Buffer, Samples * sizeof(short) * Channels, (int)SampleRate);
				VI_FREE(Buffer);

				auto* Existing = (Audio::AudioClip*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
#else
				return ContentException("load ogg audio: unsupported");
#endif
			}
			void AudioClipProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((Audio::AudioClip*)Asset->Resource);
				Asset->Resource = nullptr;
			}

			Texture2DProcessor::Texture2DProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			Texture2DProcessor::~Texture2DProcessor()
			{
			}
			ExpectsContent<void*> Texture2DProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((Graphics::Texture2D*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> Texture2DProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
#ifdef VI_STB
				VI_ASSERT(Stream != nullptr, "stream should be set");
				Core::Vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				int Width, Height, Channels;
				unsigned char* Resource = stbi_load_from_memory((const unsigned char*)Data.data(), (int)Data.size(), &Width, &Height, &Channels, STBI_rgb_alpha);
				if (!Resource)
					return ContentException("load texture 2d: invalid file");

				auto* Device = Content->GetDevice();
				Graphics::Texture2D::Desc I = Graphics::Texture2D::Desc();
				I.Data = (void*)Resource;
				I.Width = (unsigned int)Width;
				I.Height = (unsigned int)Height;
				I.RowPitch = Device->GetRowPitch(I.Width);
				I.DepthPitch = Device->GetDepthPitch(I.RowPitch, I.Height);
				I.MipLevels = Device->GetMipLevel(I.Width, I.Height);

				auto Object = ProcessRendererJob<Graphics::ExpectsGraphics<Graphics::Texture2D*>>(Device, [&I](Graphics::GraphicsDevice* Device) { return Device->CreateTexture2D(I); });
				stbi_image_free(Resource);
				if (!Object)
					return ContentException(std::move(Object.Error().message()));

				auto* Existing = (Graphics::Texture2D*)Content->TryToCache(this, Stream->VirtualName(), *Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return *Object;
#else
				return ContentException("load texture 2d: unsupported");
#endif
			}
			void Texture2DProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((Graphics::Texture2D*)Asset->Resource);
				Asset->Resource = nullptr;
			}

			ShaderProcessor::ShaderProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ShaderProcessor::~ShaderProcessor()
			{
			}
			ExpectsContent<void*> ShaderProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((Graphics::Shader*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> ShaderProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				Core::String Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.append(Buffer, Size);
				});

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Filename = Stream->VirtualName();
				I.Data = Data;

				Graphics::GraphicsDevice* Device = Content->GetDevice();
				auto Object = ProcessRendererJob<Graphics::ExpectsGraphics<Graphics::Shader*>>(Device, [&I](Graphics::GraphicsDevice* Device) { return Device->CreateShader(I); });
				if (!Object)
					return ContentException(std::move(Object.Error().message()));

				auto* Existing = (Graphics::Shader*)Content->TryToCache(this, Stream->VirtualName(), *Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return *Object;
			}
			void ShaderProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((Graphics::Shader*)Asset->Resource);
				Asset->Resource = nullptr;
			}

			ModelProcessor::ModelProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ModelProcessor::~ModelProcessor()
			{
			}
			ExpectsContent<void*> ModelProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((Model*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> ModelProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				Model* Object = nullptr;
				auto& Path = Stream->VirtualName();
				if (Core::Stringify::EndsWith(Path, ".xml") || Core::Stringify::EndsWith(Path, ".json") || Core::Stringify::EndsWith(Path, ".jsonb") || Core::Stringify::EndsWith(Path, ".xml.gz") || Core::Stringify::EndsWith(Path, ".json.gz") || Core::Stringify::EndsWith(Path, ".jsonb.gz"))
				{
					auto Data = Content->Load<Core::Schema>(Path);
					if (!Data)
						return Data.Error();

					Object = new Model();
					Series::Unpack(Data->Get("min"), &Object->Min);
					Series::Unpack(Data->Get("max"), &Object->Max);

					auto* Meshes = Data->Get("meshes");
					if (Meshes != nullptr)
					{
						Object->Meshes.reserve(Meshes->Size());
						for (auto& Mesh : Meshes->GetChilds())
						{
							Graphics::MeshBuffer::Desc I;
							I.AccessFlags = Options.AccessFlags;
							I.Usage = Options.Usage;

							if (!Series::Unpack(Mesh->Get("indices"), &I.Indices))
							{
								VI_RELEASE(Data);
								VI_RELEASE(Object);
								return ContentException("import model: invalid indices");
							}

							if (!Series::Unpack(Mesh->Get("vertices"), &I.Elements))
							{
								VI_RELEASE(Data);
								VI_RELEASE(Object);
								return ContentException("import model: invalid vertices");
							}

							auto NewBuffer = ProcessRendererJob<Graphics::ExpectsGraphics<Graphics::MeshBuffer*>>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device) { return Device->CreateMeshBuffer(I); });
							if (!NewBuffer)
							{
								VI_RELEASE(Data);
								VI_RELEASE(Object);
								return ContentException(std::move(NewBuffer.Error().message()));
							}

							Object->Meshes.emplace_back(*NewBuffer);
							Series::Unpack(Mesh->Get("name"), &NewBuffer->Name);
							Series::Unpack(Mesh->Get("transform"), &NewBuffer->Transform);
						}
					}
					VI_RELEASE(Data);
				}
				else
				{
					auto Data = ImportForImmediateUse(Stream);
					if (!Data)
						return Data.Error();

					Object = new Model();
					Object->Meshes.reserve(Data->Meshes.size());
					Object->Min = Data->Min;
					Object->Max = Data->Max;

					for (auto& Mesh : Data->Meshes)
					{
						Graphics::MeshBuffer::Desc I;
						I.AccessFlags = Options.AccessFlags;
						I.Usage = Options.Usage;
						I.Indices = std::move(Mesh.Indices);
						I.Elements = SkinVerticesToVertices(Mesh.Vertices);
						
						auto NewBuffer = ProcessRendererJob<Graphics::ExpectsGraphics<Graphics::MeshBuffer*>>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device) { return Device->CreateMeshBuffer(I); });
						if (!NewBuffer)
						{
							VI_RELEASE(Object);
							return ContentException(std::move(NewBuffer.Error().message()));
						}

						Object->Meshes.emplace_back(*NewBuffer);
						NewBuffer->Name = Mesh.Name;
						NewBuffer->Transform = Mesh.Transform;
					}
				}

				auto* Existing = (Model*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			ExpectsContent<Core::Schema*> ModelProcessor::Import(Core::Stream* Stream, uint64_t Opts)
			{
				auto Info = ImportForImmediateUse(Stream, Opts);
				if (!Info || Info->Meshes.empty() && Info->JointOffsets.empty())
				{
					if (!Info)
						return Info.Error();

					return ContentException("import model: no mesh data");
				}

				auto* Blob = Core::Var::Set::Object();
				Blob->Key = "model";

				Series::Pack(Blob->Set("options"), Opts);
				Series::Pack(Blob->Set("inv-transform"), Info->Transform);
				Series::Pack(Blob->Set("min"), Info->Min.XYZW().SetW(Info->Low));
				Series::Pack(Blob->Set("max"), Info->Max.XYZW().SetW(Info->High));
				Series::Pack(Blob->Set("skeleton"), Info->Skeleton);

				Core::Schema* Meshes = Blob->Set("meshes", Core::Var::Array());
				for (auto&& It : Info->Meshes)
				{
					Core::Schema* Mesh = Meshes->Set("mesh");
					Series::Pack(Mesh->Set("name"), It.Name);
					Series::Pack(Mesh->Set("transform"), It.Transform);
					Series::Pack(Mesh->Set("vertices"), It.Vertices);
					Series::Pack(Mesh->Set("indices"), It.Indices);
					Series::Pack(Mesh->Set("joints"), It.JointIndices);
				}

				return Blob;
			}
			ExpectsContent<ModelInfo> ModelProcessor::ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts)
			{
#ifdef VI_ASSIMP
				Core::Vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				Assimp::Importer Importer;
				auto* Scene = Importer.ReadFileFromMemory(Data.data(), Data.size(), (unsigned int)Opts, Core::OS::Path::GetExtension(Stream->VirtualName().c_str()));
				if (!Scene)
					return ContentException(Core::Stringify::Text("import model: %s", Importer.GetErrorString()));

				ModelInfo Info;
				FillSceneGeometries(&Info, Scene, Scene->mRootNode, Scene->mRootNode->mTransformation);
				FillSceneSkeletons(&Info, Scene);
				return Info;
#else
				return ContentException("import model: unsupported");
#endif
			}
			void ModelProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((Model*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			
			SkinModelProcessor::SkinModelProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			SkinModelProcessor::~SkinModelProcessor()
			{
			}
			ExpectsContent<void*> SkinModelProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((SkinModel*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> SkinModelProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				SkinModel* Object = nullptr;
				auto& Path = Stream->VirtualName();
				if (Core::Stringify::EndsWith(Path, ".xml") || Core::Stringify::EndsWith(Path, ".json") || Core::Stringify::EndsWith(Path, ".jsonb") || Core::Stringify::EndsWith(Path, ".xml.gz") || Core::Stringify::EndsWith(Path, ".json.gz") || Core::Stringify::EndsWith(Path, ".jsonb.gz"))
				{
					auto Data = Content->Load<Core::Schema>(Path);
					if (!Data)
						return Data.Error();

					Object = new SkinModel();
					Series::Unpack(Data->Get("inv-transform"), &Object->InvTransform);
					Series::Unpack(Data->Get("min"), &Object->Min);
					Series::Unpack(Data->Get("max"), &Object->Max);
					Series::Unpack(Data->Get("skeleton"), &Object->Skeleton);
					Object->Transform = Object->InvTransform.Inv();

					auto* Meshes = Data->Get("meshes");
					if (Meshes != nullptr)
					{
						Object->Meshes.reserve(Meshes->Size());
						for (auto& Mesh : Meshes->GetChilds())
						{
							Graphics::SkinMeshBuffer::Desc I;
							I.AccessFlags = Options.AccessFlags;
							I.Usage = Options.Usage;

							if (!Series::Unpack(Mesh->Get("indices"), &I.Indices))
							{
								VI_RELEASE(Data);
								VI_RELEASE(Object);
								return ContentException("import model: invalid indices");
							}

							if (!Series::Unpack(Mesh->Get("vertices"), &I.Elements))
							{
								VI_RELEASE(Data);
								VI_RELEASE(Object);
								return ContentException("import model: invalid vertices");
							}

							auto NewBuffer = ProcessRendererJob<Graphics::ExpectsGraphics<Graphics::SkinMeshBuffer*>>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device) { return Device->CreateSkinMeshBuffer(I); });
							if (!NewBuffer)
							{
								VI_RELEASE(Data);
								VI_RELEASE(Object);
								return ContentException(std::move(NewBuffer.Error().message()));
							}

							Object->Meshes.emplace_back(*NewBuffer);
							Series::Unpack(Mesh->Get("name"), &NewBuffer->Name);
							Series::Unpack(Mesh->Get("transform"), &NewBuffer->Transform);
							Series::Unpack(Mesh->Get("joints"), &NewBuffer->Joints);
						}
					}
					VI_RELEASE(Data);
				}
				else
				{
					auto Data = ModelProcessor::ImportForImmediateUse(Stream);
					if (!Data)
						return Data.Error();

					Object = new SkinModel();
					Object->Meshes.reserve(Data->Meshes.size());
					Object->InvTransform = Data->Transform;
					Object->Min = Data->Min;
					Object->Max = Data->Max;
					Object->Skeleton = std::move(Data->Skeleton);

					for (auto& Mesh : Data->Meshes)
					{
						Graphics::SkinMeshBuffer::Desc I;
						I.AccessFlags = Options.AccessFlags;
						I.Usage = Options.Usage;
						I.Indices = std::move(Mesh.Indices);
						I.Elements = std::move(Mesh.Vertices);

						auto NewBuffer = ProcessRendererJob<Graphics::ExpectsGraphics<Graphics::SkinMeshBuffer*>>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device) { return Device->CreateSkinMeshBuffer(I); });
						if (!NewBuffer)
						{
							VI_RELEASE(Object);
							return ContentException(std::move(NewBuffer.Error().message()));
						}

						Object->Meshes.emplace_back(*NewBuffer);
						NewBuffer->Name = Mesh.Name;
						NewBuffer->Transform = Mesh.Transform;
						NewBuffer->Joints = std::move(Mesh.JointIndices);
					}
				}

				auto* Existing = (SkinModel*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			void SkinModelProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((SkinModel*)Asset->Resource);
				Asset->Resource = nullptr;
			}

			SkinAnimationProcessor::SkinAnimationProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			SkinAnimationProcessor::~SkinAnimationProcessor()
			{
			}
			ExpectsContent<void*> SkinAnimationProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((Engine::SkinAnimation*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> SkinAnimationProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				Core::Vector<Compute::SkinAnimatorClip> Clips;
				auto& Path = Stream->VirtualName();
				if (Core::Stringify::EndsWith(Path, ".xml") || Core::Stringify::EndsWith(Path, ".json") || Core::Stringify::EndsWith(Path, ".jsonb") || Core::Stringify::EndsWith(Path, ".xml.gz") || Core::Stringify::EndsWith(Path, ".json.gz") || Core::Stringify::EndsWith(Path, ".jsonb.gz"))
				{
					auto Data = Content->Load<Core::Schema>(Path);
					if (!Data)
						return Data.Error();

					Clips.reserve(Data->Size());
					for (auto& Item : Data->GetChilds())
					{
						Clips.emplace_back();
						auto& Clip = Clips.back();
						Series::Unpack(Item->Get("name"), &Clip.Name);
						Series::Unpack(Item->Get("duration"), &Clip.Duration);
						Series::Unpack(Item->Get("rate"), &Clip.Rate);

						auto* Keys = Item->Get("keys");
						if (Keys != nullptr)
						{
							Clip.Keys.reserve(Keys->Size());
							for (auto& Key : Keys->GetChilds())
							{
								Clip.Keys.emplace_back();
								auto& Pose = Clip.Keys.back();
								Series::Unpack(Key, &Pose.Time);

								size_t ArrayOffset = 0;
								for (auto& Orientation : Key->GetChilds())
								{
									if (!ArrayOffset++)
										continue;

									Pose.Pose.emplace_back();
									auto& Value = Pose.Pose.back();
									Series::Unpack(Orientation->Get("position"), &Value.Position);
									Series::Unpack(Orientation->Get("scale"), &Value.Scale);
									Series::Unpack(Orientation->Get("rotation"), &Value.Rotation);
									Series::Unpack(Orientation->Get("time"), &Value.Time);
								}
							}
						}
					}
					VI_RELEASE(Data);
				}
				else
				{
					auto NewClips = ImportForImmediateUse(Stream);
					if (!NewClips)
						return NewClips.Error();

					Clips = std::move(*NewClips);
				}

				if (Clips.empty())
					return ContentException("load animation: no clips");

				auto* Object = new Engine::SkinAnimation(std::move(Clips));
				auto* Existing = (Engine::SkinAnimation*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			ExpectsContent<Core::Schema*> SkinAnimationProcessor::Import(Core::Stream* Stream, uint64_t Opts)
			{
				auto Info = ImportForImmediateUse(Stream, Opts);
				if (!Info)
					return Info.Error();

				auto* Blob = Core::Var::Set::Array();
				Blob->Key = "animation";

				for (auto& Clip : *Info)
				{
					auto* Item = Blob->Push(Core::Var::Set::Object());
					Series::Pack(Item->Set("name"), Clip.Name);
					Series::Pack(Item->Set("duration"), Clip.Duration);
					Series::Pack(Item->Set("rate"), Clip.Rate);

					auto* Keys = Item->Set("keys", Core::Var::Set::Array());
					Keys->Reserve(Clip.Keys.size());

					for (auto& Key : Clip.Keys)
					{
						auto* Pose = Keys->Set("pose", Core::Var::Set::Array());
						Pose->Reserve(Key.Pose.size() + 1);
						Series::Pack(Pose, Key.Time);

						for (auto& Orientation : Key.Pose)
						{
							auto* Value = Pose->Set("key", Core::Var::Set::Object());
							Series::Pack(Value->Set("position"), Orientation.Position);
							Series::Pack(Value->Set("scale"), Orientation.Scale);
							Series::Pack(Value->Set("rotation"), Orientation.Rotation);
							Series::Pack(Value->Set("time"), Orientation.Time);
						}
					}
				}

				return Blob;
			}
			ExpectsContent<Core::Vector<Compute::SkinAnimatorClip>> SkinAnimationProcessor::ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts)
			{
#ifdef VI_ASSIMP
				Core::Vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				Assimp::Importer Importer;
				auto* Scene = Importer.ReadFileFromMemory(Data.data(), Data.size(), (unsigned int)Opts, Core::OS::Path::GetExtension(Stream->VirtualName().c_str()));
				if (!Scene)
					return ContentException(Core::Stringify::Text("import animation: %s", Importer.GetErrorString()));

				Core::Vector<Compute::SkinAnimatorClip> Info;
				FillSceneAnimations(&Info, Scene);
				return Info;
#else
				return ContentException("import animation: unsupported");
#endif
			}
			void SkinAnimationProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_RELEASE((Engine::SkinAnimation*)Asset->Resource);
				Asset->Resource = nullptr;
			}

			SchemaProcessor::SchemaProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> SchemaProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				auto Object = Core::Schema::ConvertFromJSONB([Stream](char* Buffer, size_t Size) { return Size > 0 ? Stream->Read(Buffer, Size).Or(0) == Size : true; });
				if (Object)
					return *Object;
				
				Core::String Data;
				Stream->Seek(Core::FileSeek::Begin, Offset);
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.append(Buffer, Size);
				});

				Object = Core::Schema::ConvertFromJSON(Data.data(), Data.size());
				if (Object)
					return *Object;

				Object = Core::Schema::ConvertFromXML(Data.data(), Data.size());
				if (!Object)
					return ContentException(std::move(Object.Error().message()));

				return *Object;
			}
			ExpectsContent<void> SchemaProcessor::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				auto Type = Args.find("type");
				VI_ASSERT(Type != Args.end(), "type argument should be set");
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_ASSERT(Instance != nullptr, "instance should be set");

				auto Schema = (Core::Schema*)Instance;
				if (Type->second == Core::Var::String("XML"))
				{
					Core::String Offset;
					Core::Schema::ConvertToXML(Schema, [Stream, &Offset](Core::VarForm Pretty, const char* Buffer, size_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);

						switch (Pretty)
						{
							case Vitex::Core::VarForm::Tab_Decrease:
								Offset.erase(Offset.end() - 1);
								break;
							case Vitex::Core::VarForm::Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Vitex::Core::VarForm::Write_Space:
								Stream->Write(" ", 1);
								break;
							case Vitex::Core::VarForm::Write_Line:
								Stream->Write("\n", 1);
								break;
							case Vitex::Core::VarForm::Write_Tab:
								Stream->Write(Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
					return Core::Expectation::Met;
				}
				else if (Type->second == Core::Var::String("JSON"))
				{
					Core::String Offset;
					Core::Schema::ConvertToJSON(Schema, [Stream, &Offset](Core::VarForm Pretty, const char* Buffer, size_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);

						switch (Pretty)
						{
							case Vitex::Core::VarForm::Tab_Decrease:
								Offset.erase(Offset.end() - 1);
								break;
							case Vitex::Core::VarForm::Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Vitex::Core::VarForm::Write_Space:
								Stream->Write(" ", 1);
								break;
							case Vitex::Core::VarForm::Write_Line:
								Stream->Write("\n", 1);
								break;
							case Vitex::Core::VarForm::Write_Tab:
								Stream->Write(Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
					return Core::Expectation::Met;
				}
				else if (Type->second == Core::Var::String("JSONB"))
				{
					Core::Schema::ConvertToJSONB(Schema, [Stream](Core::VarForm, const char* Buffer, size_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);
					});
					return Core::Expectation::Met;
				}

				return ContentException("save schema: unsupported type");
			}

			ServerProcessor::ServerProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> ServerProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				auto Blob = Content->Load<Core::Schema>(Stream->VirtualName());
				if (!Blob)
					return Blob.Error();

				Core::String N = Network::Utils::GetLocalAddress();
				Core::String D = Core::OS::Path::GetDirectory(Stream->VirtualName().c_str());
				auto* Router = new Network::HTTP::MapRouter();
				auto* Object = new Network::HTTP::Server();
				if (Callback)
					Callback((void*)Object, *Blob);

				Core::Vector<Core::Schema*> Certificates = Blob->FindCollection("certificate", true);
				for (auto&& It : Certificates)
				{
					Core::String Name;
					if (Series::Unpack(It, &Name))
						Core::Stringify::EvalEnvs(Name, N, D);
					else
						Name = "*";

					Network::SocketCertificate* Cert = &Router->Certificates[Name];
					Series::Unpack(It->Find("ciphers"), &Cert->Ciphers);
					Series::Unpack(It->Find("verify-peers"), &Cert->VerifyPeers);
					Series::Unpack(It->Find("pkey"), &Cert->Blob.PrivateKey);
					Series::Unpack(It->Find("cert"), &Cert->Blob.Certificate);
					if (Series::Unpack(It->Find("options"), &Name))
					{
						if (Name.find("no_ssl_v2") != std::string::npos)
							Cert->Options = (Network::SecureLayerOptions)((size_t)Cert->Options & (size_t)Network::SecureLayerOptions::NoSSL_V2);
						if (Name.find("no_ssl_v3") != std::string::npos)
							Cert->Options = (Network::SecureLayerOptions)((size_t)Cert->Options & (size_t)Network::SecureLayerOptions::NoSSL_V3);
						if (Name.find("no_tls_v1") != std::string::npos)
							Cert->Options = (Network::SecureLayerOptions)((size_t)Cert->Options & (size_t)Network::SecureLayerOptions::NoTLS_V1);
						if (Name.find("no_tls_v1_1") != std::string::npos)
							Cert->Options = (Network::SecureLayerOptions)((size_t)Cert->Options & (size_t)Network::SecureLayerOptions::NoTLS_V1_1);
						if (Name.find("no_tls_v1_2") != std::string::npos)
							Cert->Options = (Network::SecureLayerOptions)((size_t)Cert->Options & (size_t)Network::SecureLayerOptions::NoTLS_V1_2);
						if (Name.find("no_tls_v1_3") != std::string::npos)
							Cert->Options = (Network::SecureLayerOptions)((size_t)Cert->Options & (size_t)Network::SecureLayerOptions::NoTLS_V1_3);
					}

					Core::Stringify::EvalEnvs(Cert->Blob.PrivateKey, N, D);
					if (!Cert->Blob.PrivateKey.empty())
					{
						auto Data = Core::OS::File::ReadAsString(Cert->Blob.PrivateKey);
						if (!Data)
						{
							VI_RELEASE(Blob);
							VI_RELEASE(Router);
							VI_RELEASE(Object);
							return ContentException(Core::Stringify::Text("import invalid server private key: %s", Data.Error().message().c_str()));
						}
						else
							Cert->Blob.PrivateKey = *Data;
					}

					Core::Stringify::EvalEnvs(Cert->Blob.Certificate, N, D);
					if (!Cert->Blob.Certificate.empty())
					{
						auto Data = Core::OS::File::ReadAsString(Cert->Blob.Certificate);
						if (!Data)
						{
							VI_RELEASE(Blob);
							VI_RELEASE(Router);
							VI_RELEASE(Object);
							return ContentException(Core::Stringify::Text("import invalid server certificate: %s", Data.Error().message().c_str()));
						}
						else
							Cert->Blob.Certificate = *Data;
					}
				}

				Core::Vector<Core::Schema*> Listeners = Blob->FindCollection("listen", true);
				for (auto&& It : Listeners)
				{
					Core::String Name;
					if (!Series::Unpack(It, &Name))
						Name = "*";

					Core::Stringify::EvalEnvs(Name, N, D);
					Network::RemoteHost* Host = &Router->Listeners[Name];
					Series::Unpack(It->Find("hostname"), &Host->Hostname);
					Series::Unpack(It->Find("port"), &Host->Port);
					Series::Unpack(It->Find("secure"), &Host->Secure);
					Core::Stringify::EvalEnvs(Host->Hostname, N, D);
					if (Host->Hostname.empty())
						Host->Hostname = "0.0.0.0";
					if (Host->Port <= 0)
						Host->Port = 80;
				}

				Core::Schema* Network = Blob->Find("network");
				if (Network != nullptr)
				{
					Series::Unpack(Network->Find("keep-alive"), &Router->KeepAliveMaxCount);
					Series::Unpack(Network->Find("payload-max-length"), &Router->MaxHeapBuffer);
					Series::Unpack(Network->Find("payload-max-length"), &Router->MaxNetBuffer);
					Series::Unpack(Network->Find("backlog-queue"), &Router->BacklogQueue);
					Series::Unpack(Network->Find("socket-timeout"), &Router->SocketTimeout);
					Series::Unpack(Network->Find("graceful-time-wait"), &Router->GracefulTimeWait);
					Series::Unpack(Network->Find("max-connections"), &Router->MaxConnections);
					Series::Unpack(Network->Find("enable-no-delay"), &Router->EnableNoDelay);
					Series::Unpack(Network->Find("max-uploadable-resources"), &Router->MaxUploadableResources);
					Series::Unpack(Network->Find("temporary-directory"), &Router->TemporaryDirectory);
					Series::Unpack(Network->Fetch("session.cookie.name"), &Router->Session.Cookie.Name);
					Series::Unpack(Network->Fetch("session.cookie.domain"), &Router->Session.Cookie.Domain);
					Series::Unpack(Network->Fetch("session.cookie.path"), &Router->Session.Cookie.Path);
					Series::Unpack(Network->Fetch("session.cookie.same-site"), &Router->Session.Cookie.SameSite);
					Series::Unpack(Network->Fetch("session.cookie.expires"), &Router->Session.Cookie.Expires);
					Series::Unpack(Network->Fetch("session.cookie.secure"), &Router->Session.Cookie.Secure);
					Series::Unpack(Network->Fetch("session.cookie.http-only"), &Router->Session.Cookie.HttpOnly);
					Series::Unpack(Network->Fetch("session.directory"), &Router->Session.Directory);
					Series::Unpack(Network->Fetch("session.expires"), &Router->Session.Expires);
					Core::Stringify::EvalEnvs(Router->Session.Directory, N, D);
					Core::Stringify::EvalEnvs(Router->TemporaryDirectory, N, D);

					Core::UnorderedMap<Core::String, Network::HTTP::RouterEntry*> Aliases;
					Core::Vector<Core::Schema*> Groups = Network->FindCollection("group", true);
					for (auto&& Subgroup : Groups)
					{
						Core::String Match;
						Core::Schema* fMatch = Subgroup->GetAttribute("match");
						if (fMatch != nullptr && fMatch->Value.GetType() == Core::VarType::String)
							Match = fMatch->Value.GetBlob();

						Network::HTTP::RouteMode Mode = Network::HTTP::RouteMode::Start;
						Core::Schema* fMode = Subgroup->GetAttribute("mode");
						if (fMode != nullptr && fMode->Value.GetType() == Core::VarType::String)
						{
							if (fMode->Value.IsString("start"))
								Mode = Network::HTTP::RouteMode::Start;
							else if (fMode->Value.IsString("sub"))
								Mode = Network::HTTP::RouteMode::Match;
							else if (fMode->Value.IsString("end"))
								Mode = Network::HTTP::RouteMode::End;
						}

						Network::HTTP::RouterGroup* Group = Router->Group(Match, Mode);
						Core::Vector<Core::Schema*> Routes = Subgroup->FindCollection("route", true);
						for (auto&& Base : Routes)
						{
							Network::HTTP::RouterEntry* Route = nullptr;
							Core::Schema* From = Base->GetAttribute("from");
							Core::Schema* For = Base->GetAttribute("for");
							Core::String SourceLocation = "*";
							Series::Unpack(Base, &SourceLocation);

							if (From != nullptr && From->Value.GetType() == Core::VarType::String)
							{
								auto Subalias = Aliases.find(From->Value.GetBlob());
								if (Subalias != Aliases.end())
									Route = Router->Route(SourceLocation, Group, Subalias->second);
								else
									Route = Router->Route(Match, Mode, SourceLocation, true);
							}
							else if (For != nullptr && For->Value.GetType() == Core::VarType::String && SourceLocation.empty())
								Route = Router->Route(Match, Mode, "..." + For->Value.GetBlob() + "...", true);
							else
								Route = Router->Route(Match, Mode, SourceLocation, true);

							Core::Schema* Level = Base->GetAttribute("level");
							if (Level != nullptr)
								Route->Level = (size_t)Level->Value.GetInteger();

							Core::String Tune;
							if (Series::Unpack(Base->Fetch("compression.tune"), &Tune))
							{
								if (!strcmp(Tune.c_str(), "Filtered"))
									Route->Compression.Tune = Network::HTTP::CompressionTune::Filtered;
								else if (!strcmp(Tune.c_str(), "Huffman"))
									Route->Compression.Tune = Network::HTTP::CompressionTune::Huffman;
								else if (!strcmp(Tune.c_str(), "Rle"))
									Route->Compression.Tune = Network::HTTP::CompressionTune::Rle;
								else if (!strcmp(Tune.c_str(), "Fixed"))
									Route->Compression.Tune = Network::HTTP::CompressionTune::Fixed;
								else
									Route->Compression.Tune = Network::HTTP::CompressionTune::Default;
							}

							if (Series::Unpack(Base->Fetch("compression.quality-level"), &Route->Compression.QualityLevel))
								Route->Compression.QualityLevel = Compute::Math32::Clamp(Route->Compression.QualityLevel, 0, 9);

							if (Series::Unpack(Base->Fetch("compression.memory-level"), &Route->Compression.MemoryLevel))
								Route->Compression.MemoryLevel = Compute::Math32::Clamp(Route->Compression.MemoryLevel, 1, 9);

							Series::Unpack(Base->Find("alias"), &Route->Alias);
							Series::Unpack(Base->Fetch("auth.type"), &Route->Auth.Type);
							Series::Unpack(Base->Fetch("auth.realm"), &Route->Auth.Realm);
							Series::Unpack(Base->Fetch("compression.min-length"), &Route->Compression.MinLength);
							Series::Unpack(Base->Fetch("compression.enabled"), &Route->Compression.Enabled);
							Series::Unpack(Base->Find("char-set"), &Route->CharSet);
							Series::Unpack(Base->Find("access-control-allow-origin"), &Route->AccessControlAllowOrigin);
							Series::Unpack(Base->Find("redirect"), &Route->Redirect);
							Series::Unpack(Base->Find("web-socket-timeout"), &Route->WebSocketTimeout);
							Series::Unpack(Base->Find("static-file-max-age"), &Route->StaticFileMaxAge);
							Series::Unpack(Base->Find("allow-directory-listing"), &Route->AllowDirectoryListing);
							Series::Unpack(Base->Find("allow-web-socket"), &Route->AllowWebSocket);
							Series::Unpack(Base->Find("allow-send-file"), &Route->AllowSendFile);
							Series::Unpack(Base->Find("proxy-ip-address"), &Route->ProxyIpAddress);
							if (Series::Unpack(Base->Find("files-directory"), &Route->FilesDirectory))
								Core::Stringify::EvalEnvs(Route->FilesDirectory, N, D);

							Core::Vector<Core::Schema*> AuthMethods = Base->FetchCollection("auth.methods.method");
							Core::Vector<Core::Schema*> CompressionFiles = Base->FetchCollection("compression.files.file");
							Core::Vector<Core::Schema*> HiddenFiles = Base->FetchCollection("hidden-files.hide");
							Core::Vector<Core::Schema*> IndexFiles = Base->FetchCollection("index-files.index");
							Core::Vector<Core::Schema*> TryFiles = Base->FetchCollection("try-files.fallback");
							Core::Vector<Core::Schema*> ErrorFiles = Base->FetchCollection("error-files.error");
							Core::Vector<Core::Schema*> MimeTypes = Base->FetchCollection("mime-types.file");
							Core::Vector<Core::Schema*> DisallowedMethods = Base->FetchCollection("disallowed-methods.method");
							if (Base->Fetch("auth.methods.[clear]") != nullptr)
								Route->Auth.Methods.clear();
							if (Base->Fetch("compression.files.[clear]") != nullptr)
								Route->Compression.Files.clear();
							if (Base->Fetch("hidden-files.[clear]") != nullptr)
								Route->HiddenFiles.clear();
							if (Base->Fetch("index-files.[clear]") != nullptr)
								Route->IndexFiles.clear();
							if (Base->Fetch("try-files.[clear]") != nullptr)
								Route->TryFiles.clear();
							if (Base->Fetch("error-files.[clear]") != nullptr)
								Route->ErrorFiles.clear();
							if (Base->Fetch("mime-types.[clear]") != nullptr)
								Route->MimeTypes.clear();
							if (Base->Fetch("disallowed-methods.[clear]") != nullptr)
								Route->DisallowedMethods.clear();

							for (auto& Method : AuthMethods)
							{
								Core::String Value;
								if (Series::Unpack(Method, &Value))
									Route->Auth.Methods.push_back(Value);
							}

							for (auto& File : CompressionFiles)
							{
								Core::String Pattern;
								if (Series::Unpack(File, &Pattern))
									Route->Compression.Files.emplace_back(Pattern, true);
							}

							for (auto& File : HiddenFiles)
							{
								Core::String Pattern;
								if (Series::Unpack(File, &Pattern))
									Route->HiddenFiles.emplace_back(Pattern, true);
							}

							for (auto& File : IndexFiles)
							{
								Core::String Pattern;
								if (Series::Unpack(File, &Pattern))
								{
									if (!File->GetAttribute("use"))
										Core::Stringify::EvalEnvs(Pattern, N, D);

									Route->IndexFiles.push_back(Pattern);
								}
							}

							for (auto& File : TryFiles)
							{
								Core::String Pattern;
								if (Series::Unpack(File, &Pattern))
								{
									if (!File->GetAttribute("use"))
										Core::Stringify::EvalEnvs(Pattern, N, D);

									Route->TryFiles.push_back(Pattern);
								}
							}

							for (auto& File : ErrorFiles)
							{
								Network::HTTP::ErrorFile Source;
								Series::Unpack(File->Find("status"), &Source.StatusCode);
								if (Series::Unpack(File->Find("file"), &Source.Pattern))
									Core::Stringify::EvalEnvs(Source.Pattern, N, D);
								Route->ErrorFiles.push_back(Source);
							}

							for (auto& Type : MimeTypes)
							{
								Network::HTTP::MimeType Pattern;
								Series::Unpack(Type->Find("ext"), &Pattern.Extension);
								Series::Unpack(Type->Find("type"), &Pattern.Type);
								Route->MimeTypes.push_back(Pattern);
							}

							for (auto& Method : DisallowedMethods)
							{
								Core::String Value;
								if (Series::Unpack(Method, &Value))
									Route->DisallowedMethods.push_back(Value);
							}

							if (!For || For->Value.GetType() != Core::VarType::String)
								continue;

							Core::String Alias = For->Value.GetBlob();
							auto Subalias = Aliases.find(Alias);
							if (Subalias == Aliases.end())
								Aliases[Alias] = Route;
						}
					}

					for (auto& Item : Aliases)
						Router->Remove(Item.second);
				}

				auto Configure = Args.find("configure");
				VI_RELEASE(Blob);

				if (Configure != Args.end() && Configure->second.GetBoolean())
				{
					auto Status = Object->Configure(Router);
					if (!Status)
					{
						VI_RELEASE(Object);
						return ContentException(std::move(Status.Error().message()));
					}

					return (void*)Object;
				}
				else
				{
					Object->SetRouter(Router);
					return (void*)Object;
				}
			}

			HullShapeProcessor::HullShapeProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			HullShapeProcessor::~HullShapeProcessor()
			{
			}
			ExpectsContent<void*> HullShapeProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");

				((Compute::HullShape*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			ExpectsContent<void*> HullShapeProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				auto Data = Content->Load<Core::Schema>(Stream->VirtualName());
				if (!Data)
					return Data.Error();

				Core::Vector<Core::Schema*> Meshes = Data->FetchCollection("meshes.mesh");
				Core::Vector<Compute::Vertex> Vertices;
				Core::Vector<int> Indices;

				for (auto&& Mesh : Meshes)
				{
					if (!Series::Unpack(Mesh->Find("indices"), &Indices))
					{
						VI_RELEASE(Data);
						return ContentException("import shape: invalid indices");
					}

					if (!Series::Unpack(Mesh->Find("vertices"), &Vertices))
					{
						VI_RELEASE(Data);
						return ContentException("import shape: invalid vertices");
					}
				}

				Compute::HullShape* Object = new Compute::HullShape(std::move(Vertices), std::move(Indices));
				VI_RELEASE(Data);

				if (!Object->GetShape())
				{
					VI_RELEASE(Object);
					return ContentException("import shape: invalid shape");
				}

				auto* Existing = (Compute::HullShape*)Content->TryToCache(this, Stream->VirtualName(), Object);
				if (Existing != nullptr)
				{
					VI_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			void HullShapeProcessor::Free(AssetCache* Asset)
			{
				VI_ASSERT(Asset != nullptr, "asset should be set");
				VI_ASSERT(Asset->Resource != nullptr, "instance should be set");
				VI_RELEASE((Compute::HullShape*)Asset->Resource);
			}
		}
	}
}
