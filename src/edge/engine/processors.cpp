#include "processors.h"
#include "components.h"
#include "renderers.h"
#include "../network/http.h"
#include <stb_vorbis.h>
#include <sstream>
#ifdef ED_HAS_OPENAL
#ifdef ED_AL_AT_OPENAL
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif
#endif
#ifdef ED_HAS_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#endif
#ifdef ED_HAS_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h> 
#endif
extern "C"
{
#include <stb_image.h>
#include <stb_vorbis.h>
}

namespace Edge
{
	namespace Engine
	{
		namespace Processors
		{
#ifdef ED_HAS_ASSIMP
			Compute::Matrix4x4 FromAssimpMatrix(const aiMatrix4x4& Root)
			{
				return Compute::Matrix4x4(
					Root.a1, Root.a2, Root.a3, Root.a4,
					Root.b1, Root.b2, Root.b3, Root.b4,
					Root.c1, Root.c2, Root.c3, Root.c4,
					Root.d1, Root.d2, Root.d3, Root.d4).Transpose();
			}
			std::string GetMeshName(const std::string& Name, ModelInfo* Info)
			{
				if (Name.empty())
					return Compute::Crypto::Hash(Compute::Digests::MD5(), Compute::Crypto::RandomBytes(8)).substr(0, 8);
				
				std::string Result = Name;
				for (auto&& Data : Info->Meshes)
				{
					if (Data.Name == Result)
						Result += '_';
				}

				return Result;
			}
			std::string GetJointName(const std::string& BaseName, ModelInfo* Info, MeshBlob* Blob)
			{
				if (!BaseName.empty())
					return BaseName;

				std::string Name = BaseName + '?';
				while (Info->JointOffsets.find(Name) != Info->JointOffsets.end())
					Name += '?';

				return Name;
			}
			bool GetKeyFromTime(std::vector<Compute::AnimatorKey>& Keys, float Time, Compute::AnimatorKey& Result)
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
				std::string Name = Node->mName.C_Str();
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
					auto& Next = Top->Childs.emplace_back();
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
					MeshBlob& Blob = Info->Meshes.emplace_back();
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
			void FillSceneTimeline(const std::unordered_set<float>& Timings, std::vector<float>& Timeline)
			{
				Timeline.reserve(Timings.size());
				for (auto& Time : Timings)
					Timeline.push_back(Time);

				ED_SORT(Timeline.begin(), Timeline.end(), [](float A, float B)
				{
					return A < B;
				});
			}
			void FillSceneKeys(ModelChannel& Info, std::vector<Compute::AnimatorKey>& Keys)
			{
				std::unordered_set<float> Timings;
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

				std::vector<float> Timeline;
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
			void FillSceneClip(Compute::SkinAnimatorClip& Clip, std::unordered_map<std::string, MeshBone>& Indices, std::unordered_map<std::string, std::vector<Compute::AnimatorKey>>& Channels)
			{
				std::unordered_set<float> Timings;
				for (auto& Channel : Channels)
				{
					Timings.reserve(Channel.second.size());
					for (auto& Key : Channel.second)
						Timings.insert(Key.Time);
				}

				std::vector<float> Timeline;
				FillSceneTimeline(Timings, Timeline);

				for (auto& Time : Timeline)
				{
					auto& Key = Clip.Keys.emplace_back();
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
			void FillSceneJointIndices(const aiScene* Scene, aiNode* Node, std::unordered_map<std::string, MeshBone>& Indices, size_t& Index)
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
			bool FillSceneJointDefaults(aiNode* Node, std::unordered_map<std::string, MeshBone>& Indices, size_t& Index, bool InSkeleton)
			{
				std::string Name = Node->mName.C_Str();
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
			void FillSceneAnimations(std::vector<Compute::SkinAnimatorClip>* Info, const aiScene* Scene)
			{
				std::unordered_map<std::string, MeshBone> Indices; size_t Index = 0;
				FillSceneJointIndices(Scene, Scene->mRootNode, Indices, Index);
				FillSceneJointDefaults(Scene->mRootNode, Indices, Index, false);

				Info->reserve((size_t)Scene->mNumAnimations);
				for (unsigned int i = 0; i < Scene->mNumAnimations; i++)
				{
					aiAnimation* Animation = Scene->mAnimations[i];
					auto& Clip = Info->emplace_back();
					Clip.Name = Animation->mName.C_Str();
					Clip.Duration = (float)Animation->mDuration;
					Clip.Rate = Compute::Mathf::Max(0.01f, (float)Animation->mTicksPerSecond);

					std::unordered_map<std::string, std::vector<Compute::AnimatorKey>> Channels;
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
			std::vector<Compute::Vertex> SkinVerticesToVertices(const std::vector<Compute::SkinVertex>& Data)
			{
				std::vector<Compute::Vertex> Result;
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
			T* ProcessRendererJob(Graphics::GraphicsDevice* Device, std::function<T*(Graphics::GraphicsDevice*)>&& Callback)
			{
				Parallel::Context<T*> Promise; std::future<T*> Future = Promise.Get();
				Graphics::RenderThreadCallback Job = [Promise = std::move(Promise), Callback = std::move(Callback)](Graphics::GraphicsDevice* Device) mutable
				{
					Promise.Value.set_value(Callback(Device));
				};

				auto* App = Application::Get();
				if (!App || App->GetState() != ApplicationState::Active || Device != App->Renderer)
					Device->Lockup(std::move(Job));
				else
					Device->Enqueue(std::move(Job));

				return Future.get();
			}

			AssetProcessor::AssetProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* AssetProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");

				std::vector<char> Temp;
				Stream->ReadAll([&Temp](char* Buffer, size_t Size)
				{
					Temp.reserve(Temp.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Temp.push_back(Buffer[i]);
				});

				char* Data = ED_MALLOC(char, sizeof(char) * Temp.size());
				memcpy(Data, Temp.data(), sizeof(char) * Temp.size());
				return new AssetFile(Data, Temp.size());
			}

			MaterialProcessor::MaterialProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			void MaterialProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Engine::Material*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* MaterialProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Engine::Material*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* MaterialProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				Core::Schema* Data = Content->Load<Core::Schema>(Stream->GetSource());
				std::string Path;

				if (!Data)
					return nullptr;

				Engine::Material* Object = new Engine::Material(nullptr);
				if (Series::Unpack(Data->Get("diffuse-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetDiffuseMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("normal-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetNormalMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("metallic-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetMetallicMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("roughness-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetRoughnessMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("height-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetHeightMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("occlusion-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetOcclusionMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("emission-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetEmissionMap(NewTexture);
						ED_RELEASE(NewTexture);
					});
				}

				std::string Name;
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
				ED_RELEASE(Data);

				auto* Existing = (Engine::Material*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			bool MaterialProcessor::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, false, "stream should be set");
				ED_ASSERT(Instance != nullptr, false, "instance should be set");

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

				if (!Content->Save<Core::Schema>(Stream->GetSource(), Data, Args))
				{
					ED_RELEASE(Data);
					return false;
				}

				ED_RELEASE(Data);
				return true;
			}

			SceneGraphProcessor::SceneGraphProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* SceneGraphProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				Engine::SceneGraph::Desc I = Engine::SceneGraph::Desc::Get(Application::Get());
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				ED_ASSERT(I.Shared.Device != nullptr, nullptr, "graphics device should be set");

				Core::Schema* Blob = Content->Load<Core::Schema>(Stream->GetSource());
				if (!Blob)
					return nullptr;

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
					std::vector<Core::Schema*> Collection = Materials->FindCollection("material");
					for (auto& It : Collection)
					{
						std::string Path;
						if (!Series::Unpack(It, &Path) || Path.empty())
							continue;

						Engine::Material* Value = Content->Load<Engine::Material>(Path);
						if (Value != nullptr)
						{
							Series::Unpack(It, &Value->Slot);
							Object->AddMaterial(Value);
						}
					}
				}

				Core::Schema* Entities = Blob->Find("entities");
				if (Entities != nullptr)
				{
					std::vector<Core::Schema*> Collection = Entities->FindCollection("entity");
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

						std::string Name;
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
							Compute::Transform::Spacing* Space = ED_NEW(Compute::Transform::Spacing);
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
							std::vector<Core::Schema*> Elements = Components->FindCollection("component");
							for (auto& Element : Elements)
							{
								uint64_t Id;
								if (!Series::Unpack(Element->Find("id"), &Id))
									continue;

								Component* Target = Core::Composer::Create<Component>(Id, Entity);
								if (!Entity->AddComponent(Target))
								{
									ED_WARN("[engine] component with id %" PRIu64 " cannot be created", Id);
									continue;
								}

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

				ED_RELEASE(Blob);
				Object->Snapshot = nullptr;
				Object->Actualize();

				return Object;
			}
			bool SceneGraphProcessor::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, false, "stream should be set");
				ED_ASSERT(Instance != nullptr, false, "instance should be set");

				const char* Ext = Core::OS::Path::GetExtension(Stream->GetSource().c_str());
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
					if (!Material)
						continue;

					std::string Path;
					AssetCache* Asset = Content->FindCache<Engine::Material>(Material);
					if (!Asset)
						Path.assign("./materials/" + Material->GetName() + "_" + Compute::Codec::HexEncode(Compute::Crypto::RandomBytes(6)));
					else
						Path.assign(Asset->Path);

					if (!Core::Stringify(&Path).EndsWith(Ext))
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
				Content->Save<Core::Schema>(Stream->GetSource(), Blob, Args);
				ED_RELEASE(Blob);

				return true;
			}

			AudioClipProcessor::AudioClipProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			AudioClipProcessor::~AudioClipProcessor()
			{
			}
			void AudioClipProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Audio::AudioClip*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* AudioClipProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "asset resource should be set");

				((Audio::AudioClip*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* AudioClipProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				if (Core::Stringify(&Stream->GetSource()).EndsWith(".wav"))
					return DeserializeWAVE(Stream, Offset, Args);
				else if (Core::Stringify(&Stream->GetSource()).EndsWith(".ogg"))
					return DeserializeOGG(Stream, Offset, Args);

				return nullptr;
			}
			void* AudioClipProcessor::DeserializeWAVE(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
#ifdef ED_HAS_SDL2
				std::vector<char> Data;
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
					return nullptr;

				int Format = 0;
#ifdef ED_HAS_OPENAL
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
						return nullptr;
				}
#endif
				Audio::AudioClip* Object = new Audio::AudioClip(1, Format);
				Audio::AudioContext::SetBufferData(Object->GetBuffer(), (int)Format, (const void*)WavSamples, (int)WavCount, (int)WavInfo.freq);
				SDL_FreeWAV(WavSamples);

				auto* Existing = (Audio::AudioClip*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
#else
				return nullptr;
#endif
			}
			void* AudioClipProcessor::DeserializeOGG(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::vector<char> Data;
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
				{
					ED_ERR("[engine] cannot interpret OGG stream");
					return nullptr;
				}

				int Format = 0;
#ifdef ED_HAS_OPENAL
				if (Channels == 2)
					Format = AL_FORMAT_STEREO16;
				else
					Format = AL_FORMAT_MONO16;
#endif
				Audio::AudioClip* Object = new Audio::AudioClip(1, Format);
				Audio::AudioContext::SetBufferData(Object->GetBuffer(), (int)Format, (const void*)Buffer, Samples * sizeof(short) * Channels, (int)SampleRate);
				ED_FREE(Buffer);

				auto* Existing = (Audio::AudioClip*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}

			Texture2DProcessor::Texture2DProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			Texture2DProcessor::~Texture2DProcessor()
			{
			}
			void Texture2DProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Graphics::Texture2D*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* Texture2DProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Graphics::Texture2D*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* Texture2DProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				int Width, Height, Channels;
				unsigned char* Resource = stbi_load_from_memory((const unsigned char*)Data.data(), (int)Data.size(), &Width, &Height, &Channels, STBI_rgb_alpha);
				if (!Resource)
					return nullptr;

				auto* Device = Content->GetDevice();
				Graphics::Texture2D::Desc I = Graphics::Texture2D::Desc();
				I.Data = (void*)Resource;
				I.Width = (unsigned int)Width;
				I.Height = (unsigned int)Height;
				I.RowPitch = Device->GetRowPitch(I.Width);
				I.DepthPitch = Device->GetDepthPitch(I.RowPitch, I.Height);
				I.MipLevels = Device->GetMipLevel(I.Width, I.Height);

				Graphics::Texture2D* Object = ProcessRendererJob<Graphics::Texture2D>(Device, [&I](Graphics::GraphicsDevice* Device)
				{
					return Device->CreateTexture2D(I);
				});

				stbi_image_free(Resource);
				if (!Object)
					return nullptr;

				auto* Existing = (Graphics::Texture2D*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}

			ShaderProcessor::ShaderProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ShaderProcessor::~ShaderProcessor()
			{
			}
			void ShaderProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Graphics::Shader*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* ShaderProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Graphics::Shader*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* ShaderProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::string Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.append(Buffer, Size);
				});

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Filename = Stream->GetSource();
				I.Data = Data;

				Graphics::GraphicsDevice* Device = Content->GetDevice();
				Graphics::Shader* Object = ProcessRendererJob<Graphics::Shader>(Device, [&I](Graphics::GraphicsDevice* Device)
				{
					return Device->CreateShader(I);
				});

				if (!Object)
					return nullptr;

				auto* Existing = (Graphics::Shader*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}

			ModelProcessor::ModelProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ModelProcessor::~ModelProcessor()
			{
			}
			void ModelProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Model*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* ModelProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Model*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* ModelProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				Model* Object = nullptr;
				std::string& Path = Stream->GetSource();
				Core::Stringify Location(&Path);

				if (Location.EndsWith(".xml") || Location.EndsWith(".json") || Location.EndsWith(".jsonb") || Location.EndsWith(".xml.gz") || Location.EndsWith(".json.gz") || Location.EndsWith(".jsonb.gz"))
				{
					Core::Schema* Data = Content->Load<Core::Schema>(Path);
					if (!Data)
						return nullptr;

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
								ED_RELEASE(Data);
								return nullptr;
							}

							if (!Series::Unpack(Mesh->Get("vertices"), &I.Elements))
							{
								ED_RELEASE(Data);
								return nullptr;
							}

							auto* Next = Object->Meshes.emplace_back(ProcessRendererJob<Graphics::MeshBuffer>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device)
							{
								return Device->CreateMeshBuffer(I);
							}));

							ED_ASSERT(Next != nullptr, Object, "mesh should be initializable");
							Series::Unpack(Mesh->Get("name"), &Next->Name);
							Series::Unpack(Mesh->Get("transform"), &Next->Transform);
						}
					}
					ED_RELEASE(Data);
				}
				else
				{
					ModelInfo Data = ImportForImmediateUse(Stream);
					if (Data.Meshes.empty())
						return nullptr;

					Object = new Model();
					Object->Meshes.reserve(Data.Meshes.size());
					Object->Min = Data.Min;
					Object->Max = Data.Max;

					for (auto& Mesh : Data.Meshes)
					{
						Graphics::MeshBuffer::Desc I;
						I.AccessFlags = Options.AccessFlags;
						I.Usage = Options.Usage;
						I.Indices = std::move(Mesh.Indices);
						I.Elements = SkinVerticesToVertices(Mesh.Vertices);

						auto* Next = Object->Meshes.emplace_back(ProcessRendererJob<Graphics::MeshBuffer>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device)
						{
							return Device->CreateMeshBuffer(I);
						}));

						ED_ASSERT(Next != nullptr, Object, "mesh should be initializable");
						Next->Name = Mesh.Name;
						Next->Transform = Mesh.Transform;
					}
				}

				auto* Existing = (Model*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			Core::Schema* ModelProcessor::Import(Core::Stream* Stream, uint64_t Opts)
			{
				ModelInfo Info = ImportForImmediateUse(Stream, Opts);
				if (Info.Meshes.empty() && Info.JointOffsets.empty())
					return nullptr;

				auto* Blob = Core::Var::Set::Object();
				Blob->Key = "model";

				Series::Pack(Blob->Set("options"), Opts);
				Series::Pack(Blob->Set("inv-transform"), Info.Transform);
				Series::Pack(Blob->Set("min"), Info.Min.XYZW().SetW(Info.Low));
				Series::Pack(Blob->Set("max"), Info.Max.XYZW().SetW(Info.High));
				Series::Pack(Blob->Set("skeleton"), Info.Skeleton);

				Core::Schema* Meshes = Blob->Set("meshes", Core::Var::Array());
				for (auto&& It : Info.Meshes)
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
			ModelInfo ModelProcessor::ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts)
			{
				ModelInfo Info;
#ifdef ED_HAS_ASSIMP
				std::vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				Assimp::Importer Importer;
				auto* Scene = Importer.ReadFileFromMemory(Data.data(), Data.size(), (unsigned int)Opts, Core::OS::Path::GetExtension(Stream->GetSource().c_str()));
				if (!Scene)
				{
					ED_ERR("[engine] cannot import mesh\n\t%s", Importer.GetErrorString());
					return Info;
				}

				FillSceneGeometries(&Info, Scene, Scene->mRootNode, Scene->mRootNode->mTransformation);
				FillSceneSkeletons(&Info, Scene);
#endif
				return Info;
			}
			
			SkinModelProcessor::SkinModelProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			SkinModelProcessor::~SkinModelProcessor()
			{
			}
			void SkinModelProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((SkinModel*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* SkinModelProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((SkinModel*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* SkinModelProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				SkinModel* Object = nullptr;
				std::string& Path = Stream->GetSource();
				Core::Stringify Location(&Path);

				if (Location.EndsWith(".xml") || Location.EndsWith(".json") || Location.EndsWith(".jsonb") || Location.EndsWith(".xml.gz") || Location.EndsWith(".json.gz") || Location.EndsWith(".jsonb.gz"))
				{
					Core::Schema* Data = Content->Load<Core::Schema>(Path);
					if (!Data)
						return nullptr;

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
								ED_RELEASE(Data);
								return nullptr;
							}

							if (!Series::Unpack(Mesh->Get("vertices"), &I.Elements))
							{
								ED_RELEASE(Data);
								return nullptr;
							}

							auto* Next = Object->Meshes.emplace_back(ProcessRendererJob<Graphics::SkinMeshBuffer>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device)
							{
								return Device->CreateSkinMeshBuffer(I);
							}));

							ED_ASSERT(Next != nullptr, Object, "mesh should be initializable");
							Series::Unpack(Mesh->Get("name"), &Next->Name);
							Series::Unpack(Mesh->Get("transform"), &Next->Transform);
							Series::Unpack(Mesh->Get("joints"), &Next->Joints);
						}
					}
					ED_RELEASE(Data);
				}
				else
				{
					ModelInfo Data = ModelProcessor::ImportForImmediateUse(Stream);
					if (Data.Meshes.empty())
						return nullptr;

					Object = new SkinModel();
					Object->Meshes.reserve(Data.Meshes.size());
					Object->InvTransform = Data.Transform;
					Object->Min = Data.Min;
					Object->Max = Data.Max;
					Object->Skeleton = std::move(Data.Skeleton);

					for (auto& Mesh : Data.Meshes)
					{
						Graphics::SkinMeshBuffer::Desc I;
						I.AccessFlags = Options.AccessFlags;
						I.Usage = Options.Usage;
						I.Indices = std::move(Mesh.Indices);
						I.Elements = std::move(Mesh.Vertices);

						auto* Next = Object->Meshes.emplace_back(ProcessRendererJob<Graphics::SkinMeshBuffer>(Content->GetDevice(), [&I](Graphics::GraphicsDevice* Device)
						{
							return Device->CreateSkinMeshBuffer(I);
						}));

						ED_ASSERT(Next != nullptr, Object, "mesh should be initializable");
						Next->Name = Mesh.Name;
						Next->Transform = Mesh.Transform;
						Next->Joints = std::move(Mesh.JointIndices);
					}
				}

				auto* Existing = (SkinModel*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}

			SkinAnimationProcessor::SkinAnimationProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			SkinAnimationProcessor::~SkinAnimationProcessor()
			{
			}
			void SkinAnimationProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Engine::SkinAnimation*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* SkinAnimationProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Engine::SkinAnimation*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* SkinAnimationProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::vector<Compute::SkinAnimatorClip> Clips;
				std::string& Path = Stream->GetSource();
				Core::Stringify Location(&Path);

				if (Location.EndsWith(".xml") || Location.EndsWith(".json") || Location.EndsWith(".jsonb") || Location.EndsWith(".xml.gz") || Location.EndsWith(".json.gz") || Location.EndsWith(".jsonb.gz"))
				{
					Core::Schema* Data = Content->Load<Core::Schema>(Path);
					if (!Data)
						return nullptr;

					Clips.reserve(Data->Size());
					for (auto& Item : Data->GetChilds())
					{
						auto& Clip = Clips.emplace_back();
						Series::Unpack(Item->Get("name"), &Clip.Name);
						Series::Unpack(Item->Get("duration"), &Clip.Duration);
						Series::Unpack(Item->Get("rate"), &Clip.Rate);

						auto* Keys = Item->Get("keys");
						if (Keys != nullptr)
						{
							Clip.Keys.reserve(Keys->Size());
							for (auto& Key : Keys->GetChilds())
							{
								auto& Pose = Clip.Keys.emplace_back();
								Series::Unpack(Key, &Pose.Time);

								size_t ArrayOffset = 0;
								for (auto& Orientation : Key->GetChilds())
								{
									if (!ArrayOffset++)
										continue;

									auto& Value = Pose.Pose.emplace_back();
									Series::Unpack(Orientation->Get("position"), &Value.Position);
									Series::Unpack(Orientation->Get("scale"), &Value.Scale);
									Series::Unpack(Orientation->Get("rotation"), &Value.Rotation);
									Series::Unpack(Orientation->Get("time"), &Value.Time);
								}
							}
						}
					}
					ED_RELEASE(Data);
				}
				else
					Clips = ImportForImmediateUse(Stream);

				if (Clips.empty())
					return nullptr;

				auto* Object = new Engine::SkinAnimation(std::move(Clips));
				auto* Existing = (Engine::SkinAnimation*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
			Core::Schema* SkinAnimationProcessor::Import(Core::Stream* Stream, uint64_t Opts)
			{
				std::vector<Compute::SkinAnimatorClip> Info = ImportForImmediateUse(Stream, Opts);
				if (Info.empty())
					return nullptr;

				auto* Blob = Core::Var::Set::Array();
				Blob->Key = "animation";

				for (auto& Clip : Info)
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
			std::vector<Compute::SkinAnimatorClip> SkinAnimationProcessor::ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts)
			{
				std::vector<Compute::SkinAnimatorClip> Info;
#ifdef ED_HAS_ASSIMP
				std::vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				Assimp::Importer Importer;
				auto* Scene = Importer.ReadFileFromMemory(Data.data(), Data.size(), (unsigned int)Opts, Core::OS::Path::GetExtension(Stream->GetSource().c_str()));
				if (!Scene)
				{
					ED_ERR("[engine] cannot import mesh animation because %s", Importer.GetErrorString());
					return Info;
				}

				FillSceneAnimations(&Info, Scene);
#endif
				return Info;
			}

			SchemaProcessor::SchemaProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* SchemaProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				auto* Object = Core::Schema::ConvertFromJSONB([Stream](char* Buffer, size_t Size)
				{
					return Size > 0 ? Stream->Read(Buffer, Size) == Size : true;
				}, false);

				if (Object != nullptr)
					return Object;
				
				std::string Data;
				Stream->Seek(Core::FileSeek::Begin, Offset);
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.append(Buffer, Size);
				});

				Object = Core::Schema::ConvertFromJSON(Data.data(), Data.size(), false);
				if (!Object)
					Object = Core::Schema::ConvertFromXML(Data.data(), false);

				return Object;
			}
			bool SchemaProcessor::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				auto Type = Args.find("type");
				ED_ASSERT(Type != Args.end(), false, "type argument should be set");
				ED_ASSERT(Stream != nullptr, false, "stream should be set");
				ED_ASSERT(Instance != nullptr, false, "instance should be set");

				auto Schema = (Core::Schema*)Instance;
				std::string Offset;

				if (Type->second == Core::Var::String("XML"))
				{
					Core::Schema::ConvertToXML(Schema, [Stream, &Offset](Core::VarForm Pretty, const char* Buffer, size_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);

						switch (Pretty)
						{
							case Edge::Core::VarForm::Tab_Decrease:
								Offset.assign(Offset.substr(0, Offset.size() - 1));
								break;
							case Edge::Core::VarForm::Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Edge::Core::VarForm::Write_Space:
								Stream->Write(" ", 1);
								break;
							case Edge::Core::VarForm::Write_Line:
								Stream->Write("\n", 1);
								break;
							case Edge::Core::VarForm::Write_Tab:
								Stream->Write(Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
				}
				else if (Type->second == Core::Var::String("JSON"))
				{
					Core::Schema::ConvertToJSON(Schema, [Stream, &Offset](Core::VarForm Pretty, const char* Buffer, size_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);

						switch (Pretty)
						{
							case Edge::Core::VarForm::Tab_Decrease:
								Offset.assign(Offset.substr(0, Offset.size() - 1));
								break;
							case Edge::Core::VarForm::Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Edge::Core::VarForm::Write_Space:
								Stream->Write(" ", 1);
								break;
							case Edge::Core::VarForm::Write_Line:
								Stream->Write("\n", 1);
								break;
							case Edge::Core::VarForm::Write_Tab:
								Stream->Write(Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
				}
				else if (Type->second == Core::Var::String("JSONB"))
				{
					Core::Schema::ConvertToJSONB(Schema, [Stream](Core::VarForm, const char* Buffer, size_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);
					});
				}

				return true;
			}

			ServerProcessor::ServerProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* ServerProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::string N = Network::Multiplexer::GetLocalAddress();
				std::string D = Core::OS::Path::GetDirectory(Stream->GetSource().c_str());
				auto* Blob = Content->Load<Core::Schema>(Stream->GetSource());
				auto* Router = new Network::HTTP::MapRouter();
				auto* Object = new Network::HTTP::Server();

				Application* App = Application::Get();
				if (App != nullptr)
					Router->VM = App->VM;

				if (Blob == nullptr)
				{
					ED_RELEASE(Router);
					return (void*)Object;
				}
				else if (Callback)
					Callback((void*)Object, Blob);

				Core::Schema* Config = Blob->Find("netstat");
				if (Config != nullptr)
				{
					if (Series::Unpack(Config->Fetch("module-root"), &Router->ModuleRoot))
						Core::Stringify(&Router->ModuleRoot).Eval(N, D);

					if (!Series::Unpack(Config->Find("keep-alive"), &Router->KeepAliveMaxCount))
						Router->KeepAliveMaxCount = 50;

					if (!Series::Unpack(Config->Find("payload-max-length"), &Router->PayloadMaxLength))
						Router->PayloadMaxLength = 12582912;

					if (!Series::Unpack(Config->Find("backlog-queue"), &Router->BacklogQueue))
						Router->BacklogQueue = 20;

					if (!Series::Unpack(Config->Find("socket-timeout"), &Router->SocketTimeout))
						Router->SocketTimeout = 10000;

					if (!Series::Unpack(Config->Find("graceful-time-wait"), &Router->GracefulTimeWait))
						Router->GracefulTimeWait = -1;

					if (!Series::Unpack(Config->Find("max-connections"), &Router->MaxConnections))
						Router->MaxConnections = 0;

					if (!Series::Unpack(Config->Find("enable-no-delay"), &Router->EnableNoDelay))
						Router->EnableNoDelay = false;
				}

				std::vector<Core::Schema*> Certificates = Blob->FindCollection("certificate", true);
				for (auto&& It : Certificates)
				{
					std::string Name;
					if (!Series::Unpack(It, &Name))
						Name = "*";

					Network::SocketCertificate* Cert = &Router->Certificates[Core::Stringify(&Name).Eval(N, D).R()];
					if (Series::Unpack(It->Find("protocol"), &Name))
					{
						if (!strcmp(Name.c_str(), "SSL_V2"))
							Cert->Protocol = Network::Secure::SSL_V2;
						else if (!strcmp(Name.c_str(), "SSL_V3"))
							Cert->Protocol = Network::Secure::SSL_V3;
						else if (!strcmp(Name.c_str(), "TLS_V1"))
							Cert->Protocol = Network::Secure::TLS_V1;
						else if (!strcmp(Name.c_str(), "TLS_V1_1"))
							Cert->Protocol = Network::Secure::TLS_V1_1;
						else
							Cert->Protocol = Network::Secure::Any;
					}

					if (!Series::Unpack(It->Find("ciphers"), &Cert->Ciphers))
						Cert->Ciphers = "ALL";

					if (!Series::Unpack(It->Find("verify-peers"), &Cert->VerifyPeers))
						Cert->VerifyPeers = true;

					if (!Series::Unpack(It->Find("depth"), &Cert->Depth))
						Cert->Depth = 9;

					if (!Series::Unpack(It->Find("key"), &Cert->Key))
						Cert->Key.clear();

					if (!Series::Unpack(It->Find("chain"), &Cert->Chain))
						Cert->Chain.clear();

					Core::Stringify(&Cert->Key).Eval(N, D).R();
					Core::Stringify(&Cert->Chain).Eval(N, D).R();
				}

				std::vector<Core::Schema*> Listeners = Blob->FindCollection("listen", true);
				for (auto&& It : Listeners)
				{
					std::string Name;
					if (!Series::Unpack(It, &Name))
						Name = "*";

					Network::RemoteHost* Host = &Router->Listeners[Core::Stringify(&Name).Eval(N, D).R()];
					if (!Series::Unpack(It->Find("hostname"), &Host->Hostname))
						Host->Hostname = "0.0.0.0";

					Core::Stringify(&Host->Hostname).Eval(N, D).R();
					if (!Series::Unpack(It->Find("port"), &Host->Port))
						Host->Port = 80;

					if (!Series::Unpack(It->Find("secure"), &Host->Secure))
						Host->Secure = false;
				}

				std::vector<Core::Schema*> Sites = Blob->FindCollection("site", true);
				for (auto&& It : Sites)
				{
					std::string Name = "*";
					Series::Unpack(It, &Name);

					Network::HTTP::SiteEntry* Site = Router->Site(Core::Stringify(&Name).Eval(N, D).Get());
					if (Site == nullptr)
						continue;

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.name"), &Site->Gateway.Session.Cookie.Name))
						Site->Gateway.Session.Cookie.Name = "sid";

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.domain"), &Site->Gateway.Session.Cookie.Domain))
						Site->Gateway.Session.Cookie.Domain.clear();

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.path"), &Site->Gateway.Session.Cookie.Path))
						Site->Gateway.Session.Cookie.Path = "/";

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.same-site"), &Site->Gateway.Session.Cookie.SameSite))
						Site->Gateway.Session.Cookie.SameSite = "Strict";

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.expires"), &Site->Gateway.Session.Cookie.Expires))
						Site->Gateway.Session.Cookie.Expires = 31536000;

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.secure"), &Site->Gateway.Session.Cookie.Secure))
						Site->Gateway.Session.Cookie.Secure = false;

					if (!Series::Unpack(It->Fetch("gateway.session.cookie.http-only"), &Site->Gateway.Session.Cookie.HttpOnly))
						Site->Gateway.Session.Cookie.HttpOnly = true;

					if (Series::Unpack(It->Fetch("gateway.session.document-root"), &Site->Gateway.Session.DocumentRoot))
						Core::Stringify(&Site->Gateway.Session.DocumentRoot).Eval(N, D);

					if (!Series::Unpack(It->Fetch("gateway.session.expires"), &Site->Gateway.Session.Expires))
						Site->Gateway.Session.Expires = 604800;

					if (!Series::Unpack(It->Fetch("gateway.verify"), &Site->Gateway.Verify))
						Site->Gateway.Verify = false;

					if (!Series::Unpack(It->Fetch("gateway.enabled"), &Site->Gateway.Enabled))
						Site->Gateway.Enabled = false;

					if (!Series::Unpack(It->Find("max-resources"), &Site->MaxResources))
						Site->MaxResources = 5;

                    Series::Unpack(It->Find("resource-root"), &Site->ResourceRoot);
                    Core::Stringify(&Site->ResourceRoot).Eval(N, D);

					std::unordered_map<std::string, Network::HTTP::RouteEntry*> Aliases;
					std::vector<Core::Schema*> Groups = It->FindCollection("group", true);
					for (auto&& Subgroup : Groups)
					{
						std::string Match;
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

						Network::HTTP::RouteGroup* Group = Site->Group(Match, Mode);
						std::vector<Core::Schema*> Routes = Subgroup->FindCollection("route", true);
						for (auto&& Base : Routes)
						{
							Network::HTTP::RouteEntry* Route = nullptr;
							std::string SourceURL = "*";
							Series::Unpack(Base, &SourceURL);

							Core::Schema* From = Base->GetAttribute("from"), * For = Base->GetAttribute("for");
							if (From != nullptr && From->Value.GetType() == Core::VarType::String)
							{
								auto Subalias = Aliases.find(From->Value.GetBlob());
								if (Subalias != Aliases.end())
									Route = Site->Route(SourceURL, Group, Subalias->second);
								else
									Route = Site->Route(Match, Mode, SourceURL);
							}
							else if (For != nullptr && For->Value.GetType() == Core::VarType::String && SourceURL.empty())
								Route = Site->Route(Match, Mode, "..." + For->Value.GetBlob() + "...");
							else
								Route = Site->Route(Match, Mode, SourceURL);

							Core::Schema* Level = Base->GetAttribute("level");
							if (Level != nullptr)
								Route->Level = (size_t)Level->Value.GetInteger();

							std::vector<Core::Schema*> GatewayFiles = Base->FetchCollection("gateway.files.file");
							if (Base->Fetch("gateway.files.[clear]") != nullptr)
								Route->Gateway.Files.clear();

							for (auto& File : GatewayFiles)
							{
								std::string Pattern;
								if (Series::Unpack(File, &Pattern))
									Route->Gateway.Files.emplace_back(Pattern, true);
							}

							std::vector<Core::Schema*> GatewayMethods = Base->FetchCollection("gateway.methods.method");
							if (Base->Fetch("gateway.methods.[clear]") != nullptr)
								Route->Gateway.Methods.clear();

							for (auto& Method : GatewayMethods)
							{
								std::string Value;
								if (Series::Unpack(Method, &Value))
									Route->Gateway.Methods.push_back(Value);
							}

							std::vector<Core::Schema*> AuthMethods = Base->FetchCollection("auth.methods.method");
							if (Base->Fetch("auth.methods.[clear]") != nullptr)
								Route->Auth.Methods.clear();

							for (auto& Method : AuthMethods)
							{
								std::string Value;
								if (Series::Unpack(Method, &Value))
									Route->Auth.Methods.push_back(Value);
							}

							std::vector<Core::Schema*> CompressionFiles = Base->FetchCollection("compression.files.file");
							if (Base->Fetch("compression.files.[clear]") != nullptr)
								Route->Compression.Files.clear();

							for (auto& File : CompressionFiles)
							{
								std::string Pattern;
								if (Series::Unpack(File, &Pattern))
									Route->Compression.Files.emplace_back(Pattern, true);
							}

							std::vector<Core::Schema*> HiddenFiles = Base->FetchCollection("hidden-files.hide");
							if (Base->Fetch("hidden-files.[clear]") != nullptr)
								Route->HiddenFiles.clear();

							for (auto& File : HiddenFiles)
							{
								std::string Pattern;
								if (Series::Unpack(File, &Pattern))
									Route->HiddenFiles.emplace_back(Pattern, true);
							}

							std::vector<Core::Schema*> IndexFiles = Base->FetchCollection("index-files.index");
							if (Base->Fetch("index-files.[clear]") != nullptr)
								Route->IndexFiles.clear();

							for (auto& File : IndexFiles)
							{
								std::string Pattern;
								if (Series::Unpack(File, &Pattern))
								{
									if (!File->GetAttribute("use"))
										Core::Stringify(&Pattern).Eval(N, D);

									Route->IndexFiles.push_back(Pattern);
								}
							}

							std::vector<Core::Schema*> TryFiles = Base->FetchCollection("try-files.fallback");
							if (Base->Fetch("try-files.[clear]") != nullptr)
								Route->TryFiles.clear();

							for (auto& File : TryFiles)
							{
								std::string Pattern;
								if (Series::Unpack(File, &Pattern))
								{
									if (!File->GetAttribute("use"))
										Core::Stringify(&Pattern).Eval(N, D);

									Route->TryFiles.push_back(Pattern);
								}
							}

							std::vector<Core::Schema*> ErrorFiles = Base->FetchCollection("error-files.error");
							if (Base->Fetch("error-files.[clear]") != nullptr)
								Route->ErrorFiles.clear();

							for (auto& File : ErrorFiles)
							{
								Network::HTTP::ErrorFile Source;
								if (Series::Unpack(File->Find("file"), &Source.Pattern))
									Core::Stringify(&Source.Pattern).Eval(N, D);

								Series::Unpack(File->Find("status"), &Source.StatusCode);
								Route->ErrorFiles.push_back(Source);
							}

							std::vector<Core::Schema*> MimeTypes = Base->FetchCollection("mime-types.file");
							if (Base->Fetch("mime-types.[clear]") != nullptr)
								Route->MimeTypes.clear();

							for (auto& Type : MimeTypes)
							{
								Network::HTTP::MimeType Pattern;
								Series::Unpack(Type->Find("ext"), &Pattern.Extension);
								Series::Unpack(Type->Find("type"), &Pattern.Type);
								Route->MimeTypes.push_back(Pattern);
							}

							std::vector<Core::Schema*> DisallowedMethods = Base->FetchCollection("disallowed-methods.method");
							if (Base->Fetch("disallowed-methods.[clear]") != nullptr)
								Route->DisallowedMethods.clear();

							for (auto& Method : DisallowedMethods)
							{
								std::string Value;
								if (Series::Unpack(Method, &Value))
									Route->DisallowedMethods.push_back(Value);
							}

							std::string Tune;
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
								Route->Compression.QualityLevel = Compute::Mathi::Clamp(Route->Compression.QualityLevel, 0, 9);

							if (Series::Unpack(Base->Fetch("compression.memory-level"), &Route->Compression.MemoryLevel))
								Route->Compression.MemoryLevel = Compute::Mathi::Clamp(Route->Compression.MemoryLevel, 1, 9);

							if (Series::Unpack(Base->Find("document-root"), &Route->DocumentRoot))
								Core::Stringify(&Route->DocumentRoot).Eval(N, D);

							Series::Unpack(Base->Find("override"), &Route->Override);
							Series::Unpack(Base->Fetch("gateway.report-errors"), &Route->Gateway.ReportErrors);
							Series::Unpack(Base->Fetch("gateway.report-stack"), &Route->Gateway.ReportStack);
							Series::Unpack(Base->Fetch("auth.type"), &Route->Auth.Type);
							Series::Unpack(Base->Fetch("auth.realm"), &Route->Auth.Realm);
							Series::Unpack(Base->Fetch("compression.min-length"), &Route->Compression.MinLength);
							Series::Unpack(Base->Fetch("compression.enabled"), &Route->Compression.Enabled);
							Series::Unpack(Base->Find("char-set"), &Route->CharSet);
							Series::Unpack(Base->Find("access-control-allow-origin"), &Route->AccessControlAllowOrigin);
							Series::Unpack(Base->Find("redirect"), &Route->Redirect);
							Series::Unpack(Base->Find("web-socket-timeout"), &Route->WebSocketTimeout);
							Series::Unpack(Base->Find("static-file-max-age"), &Route->StaticFileMaxAge);
							Series::Unpack(Base->Find("max-cache-length"), &Route->MaxCacheLength);
							Series::Unpack(Base->Find("allow-directory-listing"), &Route->AllowDirectoryListing);
							Series::Unpack(Base->Find("allow-web-socket"), &Route->AllowWebSocket);
							Series::Unpack(Base->Find("allow-send-file"), &Route->AllowSendFile);
							Series::Unpack(Base->Find("proxy-ip-address"), &Route->ProxyIpAddress);

							if (!For || For->Value.GetType() != Core::VarType::String)
								continue;

							std::string Alias = For->Value.GetBlob();
							auto Subalias = Aliases.find(Alias);
							if (Subalias == Aliases.end())
								Aliases[Alias] = Route;
						}
					}

					for (auto& Item : Aliases)
						Site->Remove(Item.second);
				}

				auto Configure = Args.find("configure");
				if (Configure == Args.end() || Configure->second.GetBoolean())
					Object->Configure(Router);
				else
					Object->SetRouter(Router);

				ED_RELEASE(Blob);
				return (void*)Object;
			}

			HullShapeProcessor::HullShapeProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			HullShapeProcessor::~HullShapeProcessor()
			{
			}
			void HullShapeProcessor::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_ASSERT_V(Asset->Resource != nullptr, "instance should be set");
				ED_RELEASE((Compute::HullShape*)Asset->Resource);
			}
			void* HullShapeProcessor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Compute::HullShape*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* HullShapeProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				auto* Data = Content->Load<Core::Schema>(Stream->GetSource());
				if (!Data)
					return nullptr;

				std::vector<Core::Schema*> Meshes = Data->FetchCollection("meshes.mesh");
				std::vector<Compute::Vertex> Vertices;
				std::vector<int> Indices;

				for (auto&& Mesh : Meshes)
				{
					if (!Series::Unpack(Mesh->Find("indices"), &Indices))
					{
						ED_RELEASE(Data);
						return nullptr;
					}

					if (!Series::Unpack(Mesh->Find("vertices"), &Vertices))
					{
						ED_RELEASE(Data);
						return nullptr;
					}
				}

				Compute::HullShape* Object = new Compute::HullShape(std::move(Vertices), std::move(Indices));
				ED_RELEASE(Data);

				if (!Object->GetShape())
				{
					ED_RELEASE(Object);
					return nullptr;
				}

				auto* Existing = (Compute::HullShape*)Content->TryToCache(this, Stream->GetSource(), Object);
				if (Existing != nullptr)
				{
					ED_RELEASE(Object);
					Object = Existing;
				}

				Object->AddRef();
				return Object;
			}
		}
	}
}
