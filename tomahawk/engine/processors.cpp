#include "processors.h"
#include "components.h"
#include "renderers.h"
#include "../network/http.h"
#include <stb_vorbis.h>
#include <sstream>
#ifdef THAWK_HAS_OPENAL
#include <AL/al.h>
#endif
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#endif
#ifdef THAWK_HAS_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/matrix4x4.h>
#include <assimp/cimport.h>
#endif
extern "C"
{
#include <stb_image.h>
#include <stb_vorbis.h>
}

namespace Tomahawk
{
	namespace Engine
	{
		namespace FileProcessors
		{
#ifdef THAWK_HAS_ASSIMP
			Compute::Matrix4x4 ToMatrix(const aiMatrix4x4& Root)
			{
				return Compute::Matrix4x4(Root.a1, Root.a2, Root.a3, Root.a4, Root.b1, Root.b2, Root.b3, Root.b4, Root.c1, Root.c2, Root.c3, Root.c4, Root.d1, Root.d2, Root.d3, Root.d4);
			}
#endif
			AssetFileProcessor::AssetFileProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			void* AssetFileProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				char* Binary = (char*)malloc(sizeof(char) * Length);
				if (Stream->Read(Binary, Length) != Length)
				{
					THAWK_ERROR("cannot read %llu bytes from audio clip file", Length);
					free(Binary);
					return nullptr;
				}

				return new AssetFile(Binary, (size_t)Length);
			}

			SceneGraphProcessor::SceneGraphProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			void* SceneGraphProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				SceneGraph::Desc I = SceneGraph::Desc();
				I.Device = Content->GetDevice();

				Application* App = Application::Get();
				if (App != nullptr)
				{
					I.Queue = App->Queue;
					I.Cache = App->Shaders;
				}

				std::string Environment = Content->GetEnvironment();
				Rest::Document* Document = Content->Load<Rest::Document>(Stream->Filename(), nullptr);
				if (!Document)
					return nullptr;

				Rest::Document* Metadata = Document->Find("metadata");
				if (Metadata != nullptr)
				{
					Rest::Document* Simulator = Metadata->Find("simulator");
					if (Simulator != nullptr)
					{
						NMake::Unpack(Simulator->Find("enable-soft-body"), &I.Simulator.EnableSoftBody);
						NMake::Unpack(Simulator->Find("max-displacement"), &I.Simulator.MaxDisplacement);
						NMake::Unpack(Simulator->Find("air-density"), &I.Simulator.AirDensity);
						NMake::Unpack(Simulator->Find("water-offset"), &I.Simulator.WaterOffset);
						NMake::Unpack(Simulator->Find("water-density"), &I.Simulator.WaterDensity);
						NMake::Unpack(Simulator->Find("water-normal"), &I.Simulator.WaterNormal);
						NMake::Unpack(Simulator->Find("gravity"), &I.Simulator.Gravity);
					}

					NMake::Unpack(Metadata->Find("components"), &I.ComponentCount);
					NMake::Unpack(Metadata->Find("entities"), &I.EntityCount);
					NMake::Unpack(Metadata->Find("render-quality"), &I.RenderQuality);
				}

				SceneGraph* Object = new SceneGraph(I);

				Rest::Document* Materials = Document->Find("materials", true);
				if (Materials != nullptr)
				{
					std::vector<Rest::Document*> Collection = Materials->FindCollection("material");
					for (auto& It : Collection)
					{
						std::string Name;
						NMake::Unpack(It->Find("name"), &Name);

						Engine::Material Value;
						NMake::Unpack(It, &Value);

						Object->AddMaterial(Name, Value);
					}
				}

				Rest::Document* Entities = Document->Find("entities", true);
				if (Entities != nullptr)
				{
					std::vector<Rest::Document*> Collection = Entities->FindCollection("entity");
					for (auto& It : Collection)
					{
						Entity* Entity = new Engine::Entity(Object);
						Object->AddEntity(Entity);

						NMake::Unpack(It->Find("name"), &Entity->Name);
						NMake::Unpack(It->Find("tag"), &Entity->Tag);

						Rest::Document* Transform = It->Find("transform");
						if (Transform != nullptr)
						{
							NMake::Unpack(Transform->Find("position"), &Entity->Transform->Position);
							NMake::Unpack(Transform->Find("rotation"), &Entity->Transform->Rotation);
							NMake::Unpack(Transform->Find("scale"), &Entity->Transform->Scale);
							NMake::Unpack(Transform->Find("constant-scale"), &Entity->Transform->ConstantScale);
						}

						Rest::Document* Parent = It->Find("parent");
						if (Parent != nullptr)
						{
							Compute::Vector3* Position = new Compute::Vector3();
							Compute::Vector3* Rotation = new Compute::Vector3();
							Compute::Vector3* Scale = new Compute::Vector3();
							Compute::Matrix4x4* World = new Compute::Matrix4x4();

							NMake::Unpack(Parent->Find("id"), &Entity->Id);
							NMake::Unpack(Parent->Find("position"), Position);
							NMake::Unpack(Parent->Find("rotation"), Rotation);
							NMake::Unpack(Parent->Find("scale"), Scale);
							NMake::Unpack(Parent->Find("world"), World);

							Compute::MathCommon::ConfigurateUnsafe(Entity->Transform, World, Position, Rotation, Scale);
						}

						Rest::Document* Components = It->Find("components");
						if (Components != nullptr)
						{
							std::vector<Rest::Document*> Elements = It->FindCollection("component");
							for (auto& Element : Elements)
							{
								uint64_t Id;
								if (!NMake::Unpack(Element->Find("id"), &Id))
									continue;

								Component* Target = Rest::Composer::Create<Component>(Id, Entity);
								if (!Entity->AddComponent(Target))
								{
									THAWK_WARN("component with id %llu cannot be created", Id);
									continue;
								}

								bool Active = true;
								if (NMake::Unpack(Element->Find("active"), &Active))
									Target->SetActive(Active);

								Rest::Document* Meta = Element->Find("metadata", "");
								if (!Meta)
									Meta = Element->SetDocument("metadata");

								Target->Deserialize(Content, Meta);
							}
						}
					}
				}

				for (int64_t i = 0; i < (int64_t)Object->GetEntityCount(); i++)
				{
					Entity* Entity = Object->GetEntity(i);
					int64_t Index = Entity->Id;
					Entity->Id = i;

					if (Index >= 0 && Index < (int64_t)Object->GetEntityCount() && Index != i)
						Compute::MathCommon::SetRootUnsafe(Entity->Transform, Object->GetEntity(Index)->Transform);
				}

				Object->Denotify();
				Object->Redistribute();
				delete Document;

				return Object;
			}
			bool SceneGraphProcessor::Serialize(Rest::FileStream* Stream, void* Instance, ContentArgs* Args)
			{
				SceneGraph* Object = (SceneGraph*)Instance;
				Object->Denotify();
				Object->Redistribute();
				Object->Reindex();

				Rest::Document* Document = new Rest::Document();
				Document->Name = "scene";

				Rest::Document* Metadata = Document->SetDocument("metadata");
				NMake::Pack(Metadata->SetDocument("components"), Object->GetConf().ComponentCount);
				NMake::Pack(Metadata->SetDocument("entities"), Object->GetConf().EntityCount);
				NMake::Pack(Metadata->SetDocument("render-quality"), Object->GetConf().RenderQuality);

				Rest::Document* Simulator = Metadata->SetDocument("simulator");
				NMake::Pack(Simulator->SetDocument("enable-soft-body"), Object->GetSimulator()->HasSoftBodySupport());
				NMake::Pack(Simulator->SetDocument("max-displacement"), Object->GetSimulator()->GetMaxDisplacement());
				NMake::Pack(Simulator->SetDocument("air-density"), Object->GetSimulator()->GetAirDensity());
				NMake::Pack(Simulator->SetDocument("water-offset"), Object->GetSimulator()->GetWaterOffset());
				NMake::Pack(Simulator->SetDocument("water-density"), Object->GetSimulator()->GetWaterDensity());
				NMake::Pack(Simulator->SetDocument("water-normal"), Object->GetSimulator()->GetWaterNormal());
				NMake::Pack(Simulator->SetDocument("gravity"), Object->GetSimulator()->GetGravity());

				Rest::Document* Materials = Document->SetArray("materials");
				for (uint64_t i = 0; i < Object->GetMaterialCount(); i++)
				{
					Rest::Document* Material = Materials->SetDocument("material");
					NMake::Pack(Material->SetDocument("name"), Object->GetMaterialName(i));

					Engine::Material* Ref = Object->GetMaterialById(i);
					if (Ref != nullptr)
						NMake::Pack(Material, *Ref);
				}

				Rest::Document* Entities = Document->SetArray("entities");
				for (uint64_t i = 0; i < Object->GetEntityCount(); i++)
				{
					Entity* Ref = Object->GetEntity(i);

					Rest::Document* Entity = Entities->SetDocument("entity");
					NMake::Pack(Entity->SetDocument("name"), Ref->Name);
					NMake::Pack(Entity->SetDocument("tag"), Ref->Tag);

					Rest::Document* Transform = Entity->SetDocument("transform");
					NMake::Pack(Transform->SetDocument("position"), Ref->Transform->Position);
					NMake::Pack(Transform->SetDocument("rotation"), Ref->Transform->Rotation);
					NMake::Pack(Transform->SetDocument("scale"), Ref->Transform->Scale);
					NMake::Pack(Transform->SetDocument("constant-scale"), Ref->Transform->ConstantScale);

					if (Ref->Transform->GetRoot() != nullptr)
					{
						Rest::Document* Parent = Entity->SetDocument("parent");
						if (Ref->Transform->GetRoot()->UserPointer)
							NMake::Pack(Parent->SetDocument("id"), ((Engine::Entity*)Ref->Transform->GetRoot()->UserPointer)->Id);

						NMake::Pack(Parent->SetDocument("position"), *Ref->Transform->GetLocalPosition());
						NMake::Pack(Parent->SetDocument("rotation"), *Ref->Transform->GetLocalRotation());
						NMake::Pack(Parent->SetDocument("scale"), *Ref->Transform->GetLocalScale());
						NMake::Pack(Parent->SetDocument("world"), Ref->Transform->GetWorld());
					}

					if (Ref->GetComponentCount() <= 0)
						continue;

					Rest::Document* Components = Entity->SetArray("components");
					for (auto It = Ref->First(); It != Ref->Last(); It++)
					{
						Rest::Document* Component = Components->SetDocument("component");
						NMake::Pack(Component->SetDocument("id"), It->second->Id());
						NMake::Pack(Component->SetDocument("active"), It->second->IsActive());
						It->second->Serialize(Content, Component->SetDocument("metadata"));
					}
				}

				Content->Save<Rest::Document>(Stream->Filename(), Document, Args->Args);
				delete Document;

				return true;
			}

			AudioClipProcessor::AudioClipProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			AudioClipProcessor::~AudioClipProcessor()
			{
			}
			void AudioClipProcessor::Free(AssetResource* Asset)
			{
				if (Asset->Resource != nullptr)
					delete ((Audio::AudioClip*)Asset->Resource);
			}
			void* AudioClipProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
			{
				return Asset->Resource;
			}
			void* AudioClipProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				if (Rest::Stroke(&Stream->Filename()).EndsWith(".wav"))
					return DeserializeWAVE(Stream, Length, Offset, Args);
				else if (Rest::Stroke(&Stream->Filename()).EndsWith(".ogg"))
					return DeserializeOGG(Stream, Length, Offset, Args);

				return nullptr;
			}
			void* AudioClipProcessor::DeserializeWAVE(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
#ifdef THAWK_HAS_SDL2
				void* Binary = malloc(sizeof(char) * Length);
				if (Stream->Read((char*)Binary, Length) != Length)
				{
					THAWK_ERROR("cannot read %llu bytes from audio clip file", Length);
					free(Binary);
					return nullptr;
				}

				SDL_RWops* WavData = SDL_RWFromMem(Binary, (int)Length);
				SDL_AudioSpec WavInfo;
				Uint8* WavSamples;
				Uint32 WavCount;

				if (!SDL_LoadWAV_RW(WavData, 1, &WavInfo, &WavSamples, &WavCount))
				{
					free(Binary);
					return nullptr;
				}

				int Format = 0;
#ifdef THAWK_HAS_OPENAL
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
						free(Binary);
						return nullptr;
				}
#endif
				Audio::AudioClip* Object = new Audio::AudioClip(1, Format);
				Audio::AudioContext::SetBufferData(Object->GetBuffer(), (int)Format, (const void*)WavSamples, (int)WavCount, (int)WavInfo.freq);
				SDL_FreeWAV(WavSamples);
				free(Binary);

				Content->Cache(this, Stream->Filename(), Object);
				return Object;
#else
				return nullptr;
#endif
			}
			void* AudioClipProcessor::DeserializeOGG(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				void* Binary = malloc(sizeof(char) * Length);
				if (Stream->Read((char*)Binary, Length) != Length)
				{
					THAWK_ERROR("cannot read %llu bytes from audio clip file", Length);
					free(Binary);
					return nullptr;
				}

				short* Buffer;
				int Channels, SampleRate;
				int Samples = stb_vorbis_decode_memory((const unsigned char*)Binary, (int)Length, &Channels, &SampleRate, &Buffer);
				if (Samples <= 0)
				{
					THAWK_ERROR("cannot interpret OGG stream");
					free(Binary);
					return nullptr;
				}

				int Format = 0;
#ifdef THAWK_HAS_OPENAL
				if (Channels == 2)
					Format = AL_FORMAT_STEREO16;
				else
					Format = AL_FORMAT_MONO16;
#endif
				Audio::AudioClip* Object = new Audio::AudioClip(1, Format);
				Audio::AudioContext::SetBufferData(Object->GetBuffer(), (int)Format, (const void*)Buffer, Samples * sizeof(short) * Channels, (int)SampleRate);
				free(Buffer);
				free(Binary);

				Content->Cache(this, Stream->Filename(), Object);
				return Object;
			}

			Texture2DProcessor::Texture2DProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			Texture2DProcessor::~Texture2DProcessor()
			{
			}
			void Texture2DProcessor::Free(AssetResource* Asset)
			{
				if (Asset->Resource != nullptr)
					delete ((Graphics::Texture2D*)Asset->Resource);
			}
			void* Texture2DProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
			{
				return Asset->Resource;
			}
			void* Texture2DProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				unsigned char* Binary = (unsigned char*)malloc(sizeof(unsigned char) * Length);
				if (Stream->Read((char*)Binary, Length) != Length)
				{
					THAWK_ERROR("cannot read %llu bytes from texture 2d file", Length);
					free(Binary);
					return nullptr;
				}

				int Width, Height, Channels;
				unsigned char* Resource = stbi_load_from_memory(Binary, (int)Length, &Width, &Height, &Channels, STBI_rgb_alpha);
				if (!Resource)
				{
					free(Binary);
					return nullptr;
				}

				Graphics::Texture2D::Desc F = Graphics::Texture2D::Desc();
				F.Data = (void*)Resource;
				F.Width = (unsigned int)Width;
				F.Height = (unsigned int)Height;
				F.RowPitch = (Width * 32 + 7) / 8;
				F.DepthPitch = F.RowPitch * Height;
				F.MipLevels = Content->GetDevice()->GetMipLevel(F.Width, F.Height);

				Content->GetDevice()->Lock();
				Graphics::Texture2D* Object = Content->GetDevice()->CreateTexture2D(F);
				Content->GetDevice()->Unlock();

				stbi_image_free(Resource);
				free(Binary);

				if (!Object)
					return nullptr;

				Content->Cache(this, Stream->Filename(), Object);
				return (void*)Object;
			}

			ShaderProcessor::ShaderProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			ShaderProcessor::~ShaderProcessor()
			{
			}
			void ShaderProcessor::Free(AssetResource* Asset)
			{
				if (Asset->Resource != nullptr)
					delete ((Graphics::Shader*)Asset->Resource);
			}
			void* ShaderProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
			{
				return Asset->Resource;
			}
			void* ShaderProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				if (!Args)
				{
					THAWK_ERROR("shader processor args: req pointer Layout and req integer LayoutSize");
					return nullptr;
				}

				Graphics::InputLayout* Layout = nullptr;
				if (Args->Get("Layout", ContentType_Pointer))
					Layout = (Graphics::InputLayout*)Args->Get("Layout", ContentType_Pointer)->Pointer;

				int64_t LayoutSize = 0;
				if (Args->Get("LayoutSize", ContentType_Integer))
					LayoutSize = Args->Get("LayoutSize", ContentType_Integer)->Integer;

				if (!Layout || !LayoutSize)
				{
					THAWK_ERROR("shader processor args expected: req pointer Layout and req integer LayoutSize");
					return nullptr;
				}

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (Layout && LayoutSize != -1)
				{
					I.Layout = Layout;
					I.LayoutSize = (int)LayoutSize;
				}

				char* Code = new char[(unsigned int)Length];
				Stream->Read(Code, Length);
				I.Filename = Stream->Filename();
				I.Data = Code;

				Content->GetDevice()->Unlock();
				Graphics::Shader* Object = Content->GetDevice()->CreateShader(I);
				Content->GetDevice()->Lock();

				delete[] Code;

				if (!Object)
					return nullptr;

				Content->Cache(this, Stream->Filename(), Object);
				return Object;
			}

			ModelProcessor::ModelProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			ModelProcessor::~ModelProcessor()
			{
			}
			void ModelProcessor::Free(AssetResource* Asset)
			{
				if (Asset->Resource != nullptr)
					delete ((Graphics::MeshBuffer*)Asset->Resource);
			}
			void* ModelProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
			{
				return Asset->Resource;
			}
			void* ModelProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				auto* Document = Content->Load<Rest::Document>(Stream->Filename(), nullptr);
				if (!Document)
					return nullptr;

				auto Object = new Graphics::Model();
				NMake::Unpack(Document->Find("root"), &Object->Root);
				NMake::Unpack(Document->Find("max"), &Object->Max);
				NMake::Unpack(Document->Find("min"), &Object->Min);

				std::vector<Rest::Document*> Meshes = Document->FindCollectionPath("meshes.mesh");
				for (auto&& Mesh : Meshes)
				{
					Graphics::MeshBuffer::Desc F;
					F.AccessFlags = Options.AccessFlags;
					F.Usage = Options.Usage;

					if (!NMake::Unpack(Mesh->Find("indices"), &F.Indices))
					{
						delete Document;
						return nullptr;
					}

					if (!NMake::Unpack(Mesh->Find("vertices"), &F.Elements))
					{
						delete Document;
						return nullptr;
					}

					if (Content->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
					{
						Compute::MathCommon::ComputeIndexWindingOrderFlip(F.Indices);
						Compute::MathCommon::ComputeVertexOrientation(F.Elements, true);
					}

					Content->GetDevice()->Lock();
					Object->Meshes.push_back(Content->GetDevice()->CreateMeshBuffer(F));
					Content->GetDevice()->Unlock();

					NMake::Unpack(Mesh->Find("name"), &Object->Meshes.back()->Name);
					NMake::Unpack(Mesh->Find("world"), &Object->Meshes.back()->World);

					if (Content->GetDevice()->GetBackend() == Graphics::RenderBackend_D3D11)
						Compute::MathCommon::ComputeMatrixOrientation(&Object->Meshes.back()->World, true);
				}

				Content->Cache(this, Stream->Filename(), Object);
				delete Document;

				return (void*)Object;
			}
			Rest::Document* ModelProcessor::Import(const std::string& Path, unsigned int Opts)
			{
#ifdef THAWK_HAS_ASSIMP
				Assimp::Importer Importer;

				auto* Scene = Importer.ReadFile(Path, Opts);
				if (!Scene)
				{
					THAWK_ERROR("cannot import mesh because %s", Importer.GetErrorString());
					return nullptr;
				}

				MeshInfo Info;
				ProcessNode((void*)Scene, (void*)Scene->mRootNode, &Info, Compute::Matrix4x4::Identity());
				ProcessHeirarchy((void*)Scene, (void*)Scene->mRootNode, &Info, nullptr);

				std::vector<Compute::Joint> Joints;
				for (auto&& It : Info.Joints)
				{
					if (It.first == -1)
						Joints.push_back(It.second);
				}

				auto* Document = new Rest::Document();
				Document->Name = "model";

				float Min = 0, Max = 0;
				if (Info.NX < Min)
					Min = Info.NX;

				if (Info.NY < Min)
					Min = Info.NY;

				if (Info.NZ < Min)
					Min = Info.NZ;

				if (Info.PX > Max)
					Max = Info.PX;

				if (Info.PY > Max)
					Max = Info.PY;

				if (Info.PZ > Max)
					Max = Info.PZ;

				NMake::Pack(Document->SetDocument("options"), Opts);
				NMake::Pack(Document->SetDocument("root"), ToMatrix(Scene->mRootNode->mTransformation.Inverse()).Transpose());
				NMake::Pack(Document->SetDocument("max"), Compute::Vector4(Info.PX, Info.PY, Info.PZ, Max));
				NMake::Pack(Document->SetDocument("min"), Compute::Vector4(Info.NX, Info.NY, Info.NZ, Min));
				NMake::Pack(Document->SetArray("joints"), Joints);

				Rest::Document* Meshes = Document->SetArray("meshes");
				for (auto&& It : Info.Meshes)
				{
					Rest::Document* Mesh = Meshes->SetDocument("mesh");
					NMake::Pack(Mesh->SetDocument("name"), It.Name);
					NMake::Pack(Mesh->SetDocument("world"), It.World);
					NMake::Pack(Mesh->SetDocument("vertices"), It.Vertices);
					NMake::Pack(Mesh->SetDocument("indices"), It.Indices);
				}

				return Document;
#else
				return nullptr;
#endif
			}
			void ModelProcessor::ProcessNode(void* Scene_, void* Node_, MeshInfo* Info, const Compute::Matrix4x4& Global)
			{
#ifdef THAWK_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;
				Compute::Matrix4x4 World = ToMatrix(Node->mTransformation).Transpose() * Global;

				for (unsigned int n = 0; n < Node->mNumMeshes; n++)
					ProcessMesh(Scene, Scene->mMeshes[Node->mMeshes[n]], Info, World);

				for (unsigned int n = 0; n < Node->mNumChildren; n++)
					ProcessNode(Scene, Node->mChildren[n], Info, World);
#endif
			}
			void ModelProcessor::ProcessMesh(void* Scene_, void* Mesh_, MeshInfo* Info, const Compute::Matrix4x4& Global)
			{
#ifdef THAWK_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Mesh = (aiMesh*)Mesh_;

				MeshBlob Blob;
				Blob.Name = Mesh->mName.C_Str();
				Blob.World = Global;
				if (!Blob.Name.empty())
				{
					for (auto&& MeshData : Info->Meshes)
					{
						if (MeshData.Name == Blob.Name)
							Blob.Name += '_';
					}
				}
				else
					Blob.Name = Compute::MathCommon::MD5Hash(Compute::MathCommon::RandomBytes(8)).substr(0, 8);

				for (unsigned int v = 0; v < Mesh->mNumVertices; v++)
				{
					Compute::SkinVertex Element = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 };

					auto& Vertex = Mesh->mVertices[v];
					Element.PositionX = Vertex.x;
					Element.PositionY = Vertex.y;
					Element.PositionZ = Vertex.z;

					if (Element.PositionX > Info->PX)
						Info->PX = Element.PositionX;
					else if (Element.PositionX < Info->NX)
						Info->NX = Element.PositionX;

					if (Element.PositionY > Info->PY)
						Info->PY = Element.PositionY;
					else if (Element.PositionY < Info->NY)
						Info->NY = Element.PositionY;

					if (Element.PositionZ > Info->PZ)
						Info->PZ = Element.PositionZ;
					else if (Element.PositionZ < Info->NZ)
						Info->NZ = Element.PositionZ;

					if (Mesh->HasNormals())
					{
						auto& Normal = Mesh->mNormals[v];
						Element.NormalX = Normal.x;
						Element.NormalY = Normal.y;
						Element.NormalZ = Normal.z;
					}

					if (Mesh->HasTextureCoords(0))
					{
						auto& TexCoord = Mesh->mTextureCoords[0][v];
						Element.TexCoordX = TexCoord.x;
						Element.TexCoordY = -TexCoord.y;
					}

					if (Mesh->HasTangentsAndBitangents())
					{
						auto& Tangent = Mesh->mTangents[v];
						Element.TangentX = Tangent.x;
						Element.TangentY = Tangent.y;
						Element.TangentZ = Tangent.z;

						auto& Bitangent = Mesh->mBitangents[v];
						Element.BitangentX = Bitangent.x;
						Element.BitangentY = Bitangent.y;
						Element.BitangentZ = Bitangent.z;
					}

					Blob.Vertices.push_back(Element);
				}

				for (unsigned int f = 0; f < Mesh->mNumFaces; f++)
				{
					auto* Face = &Mesh->mFaces[f];
					for (unsigned int i = 0; i < Face->mNumIndices; i++)
						Blob.Indices.push_back(Face->mIndices[i]);
				}

				for (unsigned int j = 0; j < Mesh->mNumBones; j++)
				{
					auto& Joint = Mesh->mBones[j];
					int64_t Index = 0;

					auto It = FindJoint(Info->Joints, Joint->mName.C_Str());
					if (It == Info->Joints.end())
					{
						Compute::Joint Element;
						Element.Name = Joint->mName.C_Str();
						Element.BindShape = ToMatrix(Joint->mOffsetMatrix).Transpose();
						Element.Index = Info->Weights;
						Index = Info->Weights;
						Info->Weights++;
						Info->Joints.push_back(std::make_pair(Index, Element));
					}
					else
						Index = It->first;

					for (unsigned int w = 0; w < Joint->mNumWeights; w++)
					{
						auto& Element = Blob.Vertices[Joint->mWeights[w].mVertexId];
						if (Element.JointIndex0 == -1)
						{
							Element.JointIndex0 = (float)Index;
							Element.JointBias0 = Joint->mWeights[w].mWeight;
						}
						else if (Element.JointIndex1 == -1)
						{
							Element.JointIndex1 = (float)Index;
							Element.JointBias1 = Joint->mWeights[w].mWeight;
						}
						else if (Element.JointIndex2 == -1)
						{
							Element.JointIndex2 = (float)Index;
							Element.JointBias2 = Joint->mWeights[w].mWeight;
						}
						else if (Element.JointIndex3 == -1)
						{
							Element.JointIndex3 = (float)Index;
							Element.JointBias3 = Joint->mWeights[w].mWeight;
						}
					}
				}

				Info->Meshes.emplace_back(Blob);
#endif
			}
			void ModelProcessor::ProcessHeirarchy(void* Scene_, void* Node_, MeshInfo* Info, Compute::Joint* Parent)
			{
#ifdef THAWK_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;
				auto It = FindJoint(Info->Joints, Node->mName.C_Str());

				if (It != Info->Joints.end())
				{
					It->second.Transform = ToMatrix(Node->mTransformation).Transpose();
					It->first = -1;
				}

				for (int64_t i = 0; i < Node->mNumChildren; i++)
					ProcessHeirarchy(Scene, Node->mChildren[i], Info, It == Info->Joints.end() ? Parent : &It->second);

				if (Parent != nullptr && It != Info->Joints.end())
				{
					Parent->Childs.push_back(It->second);
					It->first = Parent->Index;
				}
#endif
			}
			std::vector<std::pair<int64_t, Compute::Joint>>::iterator ModelProcessor::FindJoint(std::vector<std::pair<int64_t, Compute::Joint>>& Joints, const std::string& Name)
			{
				for (auto It = Joints.begin(); It != Joints.end(); It++)
				{
					if (It->second.Name == Name)
						return It;
				}

				return Joints.end();
			}

			SkinModelProcessor::SkinModelProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			SkinModelProcessor::~SkinModelProcessor()
			{
			}
			void SkinModelProcessor::Free(AssetResource* Asset)
			{
				if (Asset->Resource != nullptr)
					delete ((Graphics::SkinMeshBuffer*)Asset->Resource);
			}
			void* SkinModelProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
			{
				return Asset->Resource;
			}
			void* SkinModelProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				auto* Document = Content->Load<Rest::Document>(Stream->Filename(), nullptr);
				if (!Document)
					return nullptr;

				auto Object = new Graphics::SkinModel();
				NMake::Unpack(Document->Find("root"), &Object->Root);
				NMake::Unpack(Document->Find("max"), &Object->Max);
				NMake::Unpack(Document->Find("min"), &Object->Min);
				NMake::Unpack(Document->Find("joints"), &Object->Joints);

				std::vector<Rest::Document*> Meshes = Document->FindCollectionPath("meshes.mesh");
				for (auto&& Mesh : Meshes)
				{
					Graphics::SkinMeshBuffer::Desc F;
					F.AccessFlags = Options.AccessFlags;
					F.Usage = Options.Usage;

					if (!NMake::Unpack(Mesh->Find("indices"), &F.Indices))
					{
						delete Document;
						return nullptr;
					}

					if (!NMake::Unpack(Mesh->Find("vertices"), &F.Elements))
					{
						delete Document;
						return nullptr;
					}

					Content->GetDevice()->Lock();
					Object->Meshes.push_back(Content->GetDevice()->CreateSkinMeshBuffer(F));
					Content->GetDevice()->Unlock();

					NMake::Unpack(Mesh->Find("name"), &Object->Meshes.back()->Name);
					NMake::Unpack(Mesh->Find("world"), &Object->Meshes.back()->World);
				}

				Content->Cache(this, Stream->Filename(), Object);
				delete Document;

				return (void*)Object;
			}
			Rest::Document* SkinModelProcessor::ImportAnimation(const std::string& Path, unsigned int Opts)
			{
#ifdef THAWK_HAS_ASSIMP
				Assimp::Importer Importer;

				auto* Scene = Importer.ReadFile(Path, Opts);
				if (!Scene)
				{
					THAWK_ERROR("cannot import mesh animation because %s", Importer.GetErrorString());
					return nullptr;
				}

				std::unordered_map<std::string, MeshNode> Joints;
				int64_t Index = 0;
				ProcessNode((void*)Scene, (void*)Scene->mRootNode, &Joints, Index);
				ProcessHeirarchy((void*)Scene, (void*)Scene->mRootNode, &Joints);

				std::vector<Compute::SkinAnimatorClip> Clips;
				Clips.resize((size_t)Scene->mNumAnimations);

				for (int64_t i = 0; i < Scene->mNumAnimations; i++)
				{
					aiAnimation* Animation = Scene->mAnimations[i];
					Compute::SkinAnimatorClip* Clip = &Clips[i];
					Clip->Name = Animation->mName.C_Str();

					for (int64_t j = 0; j < Animation->mNumChannels; j++)
					{
						auto* Channel = Animation->mChannels[j];
						auto It = Joints.find(Channel->mNodeName.C_Str());
						if (It == Joints.end())
							continue;

						if (Clip->Keys.size() < Channel->mNumPositionKeys)
							Clip->Keys.resize(Channel->mNumPositionKeys);

						if (Clip->Keys.size() < Channel->mNumRotationKeys)
							Clip->Keys.resize(Channel->mNumRotationKeys);

						if (Clip->Keys.size() < Channel->mNumScalingKeys)
							Clip->Keys.resize(Channel->mNumScalingKeys);

						for (int64_t k = 0; k < Channel->mNumPositionKeys; k++)
						{
							auto& Keys = Clip->Keys[k];
							ProcessKeys(&Keys, &Joints);

							Keys[It->second.Index].Position.X = Channel->mPositionKeys[k].mValue.x;
							Keys[It->second.Index].Position.Y = Channel->mPositionKeys[k].mValue.y;
							Keys[It->second.Index].Position.Z = Channel->mPositionKeys[k].mValue.z;
						}

						for (int64_t k = 0; k < Channel->mNumRotationKeys; k++)
						{
							auto& Keys = Clip->Keys[k];
							ProcessKeys(&Keys, &Joints);

							Keys[It->second.Index].Rotation.X = Channel->mRotationKeys[k].mValue.x;
							Keys[It->second.Index].Rotation.Y = Channel->mRotationKeys[k].mValue.y;
							Keys[It->second.Index].Rotation.Z = Channel->mPositionKeys[k].mValue.z;
							Keys[It->second.Index].Rotation = Keys[It->second.Index].Rotation.SaturateRotation();
						}

						for (int64_t k = 0; k < Channel->mNumScalingKeys; k++)
						{
							auto& Keys = Clip->Keys[k];
							ProcessKeys(&Keys, &Joints);

							Keys[It->second.Index].Scale.X = Channel->mScalingKeys[k].mValue.x;
							Keys[It->second.Index].Scale.Y = Channel->mScalingKeys[k].mValue.y;
							Keys[It->second.Index].Scale.Z = Channel->mScalingKeys[k].mValue.z;
						}
					}
				}

				auto* Document = new Rest::Document();
				Document->Name = "animation";

				NMake::Pack(Document, Clips);
				return Document;
#else
				return nullptr
#endif
			}
			void SkinModelProcessor::ProcessNode(void* Scene_, void* Node_, std::unordered_map<std::string, MeshNode>* Joints, int64_t& Id)
			{
#ifdef THAWK_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;

				for (unsigned int n = 0; n < Node->mNumMeshes; n++)
				{
					auto* Mesh = Scene->mMeshes[Node->mMeshes[n]];
					for (unsigned int j = 0; j < Mesh->mNumBones; j++)
					{
						auto& Joint = Mesh->mBones[j];
						int64_t Index = 0;

						auto It = Joints->find(Joint->mName.C_Str());
						if (It == Joints->end())
						{
							MeshNode Element;
							Element.Index = Id;
							Index = Id;
							Id++;
							Joints->insert(std::make_pair(Joint->mName.C_Str(), Element));
						}
						else
							Index = It->second.Index;
					}
				}

				for (unsigned int n = 0; n < Node->mNumChildren; n++)
					ProcessNode(Scene, Node->mChildren[n], Joints, Id);
#endif
			}
			void SkinModelProcessor::ProcessHeirarchy(void* Scene_, void* Node_, std::unordered_map<std::string, MeshNode>* Joints)
			{
#ifdef THAWK_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;
				auto It = Joints->find(Node->mName.C_Str());

				if (It != Joints->end())
					It->second.Transform = ToMatrix(Node->mTransformation).Transpose();

				for (int64_t i = 0; i < Node->mNumChildren; i++)
					ProcessHeirarchy(Scene, Node->mChildren[i], Joints);
#endif
			}
			void SkinModelProcessor::ProcessKeys(std::vector<Compute::AnimatorKey>* Keys, std::unordered_map<std::string, MeshNode>* Joints)
			{
				if (Keys->size() < Joints->size())
				{
					Keys->resize(Joints->size());
					for (auto It = Joints->begin(); It != Joints->end(); It++)
					{
						auto* Key = &Keys->at(It->second.Index);
						Key->Position = It->second.Transform.Position();
						Key->Rotation = It->second.Transform.Rotation();
						Key->Scale = It->second.Transform.Scale();
						Key->PlayingSpeed = 1.0f;
					}
				}
			}

			DocumentProcessor::DocumentProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			void* DocumentProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				Rest::NReadCallback Callback = [Stream](char* Buffer, int64_t Size)
				{
					if (!Buffer || !Size)
						return true;

					return Stream->Read(Buffer, Size) == Size;
				};

				auto* Object = Rest::Document::ReadBIN(Callback);
				if (Object != nullptr)
					return Object;

				Stream->Seek(Rest::FileSeek_Begin, Offset);
				Object = Rest::Document::ReadJSON(Length, Callback);
				if (Object != nullptr)
					return Object;

				Stream->Seek(Rest::FileSeek_Begin, Offset);
				return Rest::Document::ReadXML(Length, Callback);
			}
			bool DocumentProcessor::Serialize(Rest::FileStream* Stream, void* Instance, ContentArgs* Args)
			{
				auto Document = (Rest::Document*)Instance;
				bool Result = true;
				std::string Offset;

				if (Args->Is("XML", ContentKey(true)))
				{
					Rest::Document::WriteXML(Document, [Stream, &Offset](Rest::DocumentPretty Pretty, const char* Buffer, int64_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);

						switch (Pretty)
						{
							case Tomahawk::Rest::DocumentPretty_Tab_Decrease:
								Offset.assign(Offset.substr(0, Offset.size() - 1));
								break;
							case Tomahawk::Rest::DocumentPretty_Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Tomahawk::Rest::DocumentPretty_Write_Space:
								Stream->Write(" ", 1);
								break;
							case Tomahawk::Rest::DocumentPretty_Write_Line:
								Stream->Write("\n", 1);
								break;
							case Tomahawk::Rest::DocumentPretty_Write_Tab:
								Stream->Write(Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
				}
				else if (Args->Is("JSON", ContentKey(true)))
				{
					Rest::Document::WriteJSON(Document, [Stream, &Offset](Rest::DocumentPretty Pretty, const char* Buffer, int64_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);

						switch (Pretty)
						{
							case Tomahawk::Rest::DocumentPretty_Tab_Decrease:
								Offset.assign(Offset.substr(0, Offset.size() - 1));
								break;
							case Tomahawk::Rest::DocumentPretty_Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Tomahawk::Rest::DocumentPretty_Write_Space:
								Stream->Write(" ", 1);
								break;
							case Tomahawk::Rest::DocumentPretty_Write_Line:
								Stream->Write("\n", 1);
								break;
							case Tomahawk::Rest::DocumentPretty_Write_Tab:
								Stream->Write(Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
				}
				else if (Args->Is("BIN", ContentKey(true)))
				{
					Rest::Document::WriteBIN(Document, [Stream](Rest::DocumentPretty, const char* Buffer, int64_t Length)
					{
						if (Buffer != nullptr && Length > 0)
							Stream->Write(Buffer, Length);
					});
				}
				else
					Result = false;

				return Result;
			}

			ServerProcessor::ServerProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			void* ServerProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				std::string N = Network::Socket::LocalIpAddress();
				std::string D = Rest::OS::FileDirectory(Stream->Filename());

				auto* Document = Content->Load<Rest::Document>(Stream->Filename(), nullptr);
				auto* Object = new Network::HTTP::Server();
				auto* Router = new Network::HTTP::MapRouter();

				if (Document == nullptr)
				{
					delete Router;
					return (void*)Object;
				}
				else if (Callback)
					Callback((void*)Object, Document);

				if (!NMake::Unpack(Document->Find("keep-alive"), &Router->KeepAliveMaxCount))
					Router->KeepAliveMaxCount = 10;

				if (!NMake::Unpack(Document->Find("payload-max-length"), &Router->PayloadMaxLength))
					Router->PayloadMaxLength = std::numeric_limits<uint64_t>::max();

				if (!NMake::Unpack(Document->Find("max-events"), &Router->MaxEvents))
					Router->MaxEvents = 256;

				if (!NMake::Unpack(Document->Find("backlog-queue"), &Router->BacklogQueue))
					Router->BacklogQueue = 20;

				if (!NMake::Unpack(Document->Find("socket-timeout"), &Router->SocketTimeout))
					Router->SocketTimeout = 5000;

				if (!NMake::Unpack(Document->Find("master-timeout"), &Router->MasterTimeout))
					Router->MasterTimeout = 200;

				if (!NMake::Unpack(Document->Find("close-timeout"), &Router->CloseTimeout))
					Router->CloseTimeout = 500;

				if (!NMake::Unpack(Document->Find("max-connections"), &Router->MaxConnections))
					Router->MaxConnections = 0;

				if (!NMake::Unpack(Document->Find("enable-no-delay"), &Router->EnableNoDelay))
					Router->EnableNoDelay = false;

				std::vector<Rest::Document*> Certificates = Document->FindCollection("certificate", true);
				for (auto&& It : Certificates)
				{
					std::string Name;
					if (!NMake::Unpack(It, &Name))
						Name = "*";

					Network::SocketCertificate* Cert = &Router->Certificates[Rest::Stroke(&Name).Path(N, D).R()];
					if (Cert == nullptr)
						continue;

					if (NMake::Unpack(It->Find("protocol"), &Name))
					{
						if (!strcmp(Name.c_str(), "SSL_V2"))
							Cert->Protocol = Network::Secure_SSL_V2;
						else if (!strcmp(Name.c_str(), "SSL_V3"))
							Cert->Protocol = Network::Secure_SSL_V3;
						else if (!strcmp(Name.c_str(), "TLS_V1"))
							Cert->Protocol = Network::Secure_TLS_V1;
						else if (!strcmp(Name.c_str(), "TLS_V1_1"))
							Cert->Protocol = Network::Secure_TLS_V1_1;
						else
							Cert->Protocol = Network::Secure_Any;
					}

					if (!NMake::Unpack(It->Find("ciphers"), &Cert->Ciphers))
						Cert->Ciphers = "ALL";

					if (!NMake::Unpack(It->Find("verify-peers"), &Cert->VerifyPeers))
						Cert->VerifyPeers = true;

					if (!NMake::Unpack(It->Find("depth"), &Cert->Depth))
						Cert->Depth = 9;

					if (!NMake::Unpack(It->Find("key"), &Cert->Key))
						Cert->Key.clear();

					if (!NMake::Unpack(It->Find("chain"), &Cert->Chain))
						Cert->Chain.clear();

					Rest::Stroke(&Cert->Key).Path(N, D).R();
					Rest::Stroke(&Cert->Chain).Path(N, D).R();
				}

				std::vector<Rest::Document*> Listeners = Document->FindCollection("listen", true);
				for (auto&& It : Listeners)
				{
					std::string Name;
					if (!NMake::Unpack(It, &Name))
						Name = "*";

					Network::Host* Host = &Router->Listeners[Rest::Stroke(&Name).Path(N, D).R()];
					if (Host == nullptr)
						continue;

					if (!NMake::Unpack(It->Find("hostname"), &Host->Hostname))
						Host->Hostname = N;

					if (!NMake::Unpack(It->Find("port"), &Host->Port))
						Host->Port = 80;

					if (!NMake::Unpack(It->Find("secure"), &Host->Secure))
						Host->Secure = false;
				}

				std::vector<Rest::Document*> Sites = Document->FindCollection("site", true);
				for (auto&& It : Sites)
				{
					std::string Name;
					if (!NMake::Unpack(It, &Name))
						Name = "*";

					Network::HTTP::SiteEntry* Site = Router->Site(Rest::Stroke(&Name).Path(N, D).Get());
					if (Site == nullptr)
						continue;

					if (!NMake::Unpack(It->FindPath("gateway.session.name"), &Site->Gateway.Session.Name))
						Site->Gateway.Session.Name = "SessionId";

					if (!NMake::Unpack(It->FindPath("gateway.session.document-root"), &Site->Gateway.Session.DocumentRoot))
						Site->Gateway.Session.DocumentRoot.clear();

					if (!NMake::Unpack(It->FindPath("gateway.session.domain"), &Site->Gateway.Session.Domain))
						Site->Gateway.Session.Domain.clear();

					if (!NMake::Unpack(It->FindPath("gateway.session.path"), &Site->Gateway.Session.Path))
						Site->Gateway.Session.Path = "/";

					if (!NMake::Unpack(It->FindPath("gateway.session.path"), &Site->Gateway.Session.Expires))
						Site->Gateway.Session.Expires = 604800;

					if (!NMake::Unpack(It->FindPath("gateway.session.cookie-expires"), &Site->Gateway.Session.CookieExpires))
						Site->Gateway.Session.CookieExpires = 31536000;

					if (!NMake::Unpack(It->FindPath("gateway.module-root"), &Site->Gateway.ModuleRoot))
						Site->Gateway.ModuleRoot.clear();

					if (!NMake::Unpack(It->FindPath("gateway.enabled"), &Site->Gateway.Enabled))
						Site->Gateway.Enabled = false;

					if (!NMake::Unpack(It->Find("resource-root"), &Site->ResourceRoot))
						Site->ResourceRoot = "./files/";

					if (!NMake::Unpack(It->Find("max-resources"), &Site->MaxResources))
						Site->MaxResources = 5;

					Rest::Stroke(&Site->Gateway.Session.DocumentRoot).Path(N, D);
					Rest::Stroke(&Site->Gateway.ModuleRoot).Path(N, D);
					Rest::Stroke(&Site->ResourceRoot).Path(N, D);

					std::vector<Rest::Document*> Hosts = It->FindCollection("host", true);
					for (auto&& Host : Hosts)
					{
						std::string Value;
						if (NMake::Unpack(Host, &Value))
							Site->Hosts.insert(Rest::Stroke(&Value).Path(N, D).R());
					}

					std::vector<Rest::Document*> Routes = It->FindCollection("route", true);
					for (auto&& Base : Routes)
					{
						std::string BaseName;
						if (!NMake::Unpack(Base, &BaseName))
							BaseName = "*";

						Network::HTTP::RouteEntry* Route = Site->Route(BaseName.c_str());
						if (Route == nullptr)
							continue;

						std::vector<Rest::Document*> GatewayFiles = Base->FindCollectionPath("gateway.files.file");
						for (auto& File : GatewayFiles)
						{
							Network::HTTP::GatewayFile Gate;
							if (!NMake::Unpack(File->Find("core"), &Gate.Core))
								Gate.Core = false;

							std::string Pattern;
							if (NMake::Unpack(File->Find("pattern"), &Pattern))
								Gate.Value = Compute::Regex::Create(Pattern, Compute::RegexFlags_IgnoreCase);

							Route->Gateway.Files.push_back(Gate);
						}

						std::vector<Rest::Document*> GatewayMethods = Base->FindCollectionPath("gateway.methods.method");
						for (auto& Method : GatewayMethods)
						{
							std::string Value;
							if (NMake::Unpack(Method, &Value))
								Route->Gateway.Methods.push_back(Value);
						}

						std::vector<Rest::Document*> AuthUsers = Base->FindCollectionPath("auth.users.user");
						for (auto& User : AuthUsers)
						{
							Network::HTTP::Credentials Credentials;
							NMake::Unpack(User->Find("username"), &Credentials.Username);
							NMake::Unpack(User->Find("password"), &Credentials.Password);
							Route->Auth.Users.push_back(Credentials);
						}

						std::vector<Rest::Document*> AuthMethods = Base->FindCollectionPath("auth.methods.method");
						for (auto& Method : AuthMethods)
						{
							std::string Value;
							if (NMake::Unpack(Method, &Value))
								Route->Auth.Methods.push_back(Value);
						}

						std::vector<Rest::Document*> CompressionFiles = Base->FindCollectionPath("compression.files.file");
						for (auto& File : CompressionFiles)
						{
							std::string Value;
							if (NMake::Unpack(File, &Value))
								Route->Compression.Files.push_back(Compute::Regex::Create(Value, Compute::RegexFlags_IgnoreCase));
						}

						std::vector<Rest::Document*> HiddenFiles = Base->FindCollectionPath("hidden-files.hide");
						for (auto& File : HiddenFiles)
						{
							std::string Value;
							if (NMake::Unpack(File, &Value))
								Route->HiddenFiles.push_back(Compute::Regex::Create(Value, Compute::RegexFlags_IgnoreCase));
						}

						std::vector<Rest::Document*> IndexFiles = Base->FindCollectionPath("index-files.index");
						for (auto& File : IndexFiles)
						{
							std::string Value;
							if (NMake::Unpack(File, &Value))
								Route->IndexFiles.push_back(Value);
						}

						std::vector<Rest::Document*> ErrorFiles = Base->FindCollectionPath("error-files.error");
						for (auto& File : ErrorFiles)
						{
							Network::HTTP::ErrorFile Pattern;
							NMake::Unpack(File->Find("file"), &Pattern.Pattern);
							NMake::Unpack(File->Find("status"), &Pattern.StatusCode);
							Route->ErrorFiles.push_back(Pattern);
						}

						std::vector<Rest::Document*> MimeTypes = Base->FindCollectionPath("mime-types.file");
						for (auto& Type : MimeTypes)
						{
							Network::HTTP::MimeType Pattern;
							NMake::Unpack(Type->Find("ext"), &Pattern.Extension);
							NMake::Unpack(Type->Find("type"), &Pattern.Type);
							Route->MimeTypes.push_back(Pattern);
						}

						std::vector<Rest::Document*> DisallowedMethods = Base->FindCollectionPath("disallowed-methods.method");
						for (auto& Method : DisallowedMethods)
						{
							std::string Value;
							if (NMake::Unpack(Method, &Value))
								Route->DisallowedMethods.push_back(Value);
						}

						std::string Tune;
						if (NMake::Unpack(Base->FindPath("compression.tune"), &Tune))
						{
							if (!strcmp(Tune.c_str(), "Filtered"))
								Route->Compression.Tune = Network::HTTP::CompressionTune_Filtered;
							else if (!strcmp(Tune.c_str(), "Huffman"))
								Route->Compression.Tune = Network::HTTP::CompressionTune_Huffman;
							else if (!strcmp(Tune.c_str(), "Rle"))
								Route->Compression.Tune = Network::HTTP::CompressionTune_Rle;
							else if (!strcmp(Tune.c_str(), "Fixed"))
								Route->Compression.Tune = Network::HTTP::CompressionTune_Fixed;
							else
								Route->Compression.Tune = Network::HTTP::CompressionTune_Default;
						}

						if (NMake::Unpack(Base->FindPath("compression.quality-level"), &Route->Compression.QualityLevel))
							Route->Compression.QualityLevel = Compute::Math<int>::Clamp(Route->Compression.QualityLevel, 0, 9);

						if (NMake::Unpack(Base->FindPath("compression.memory-level"), &Route->Compression.MemoryLevel))
							Route->Compression.MemoryLevel = Compute::Math<int>::Clamp(Route->Compression.MemoryLevel, 1, 9);

						if (!NMake::Unpack(Base->FindPath("auth.type"), &Route->Auth.Type))
							Route->Auth.Type.clear();

						if (!NMake::Unpack(Base->FindPath("auth.realm"), &Route->Auth.Realm))
							Route->Auth.Type.clear();

						if (!NMake::Unpack(Base->FindPath("compression.min-length"), &Route->Compression.MinLength))
							Route->Compression.MinLength = 16384;

						if (!NMake::Unpack(Base->FindPath("compression.enabled"), &Route->Compression.Enabled))
							Route->Compression.Enabled = false;

						if (NMake::Unpack(Base->Find("document-root"), &Route->DocumentRoot))
							Rest::Stroke(&Route->DocumentRoot).Path(N, D);

						if (NMake::Unpack(Base->Find("default"), &Route->Default))
							Rest::Stroke(&Route->Default).Path(N, D);

						if (!NMake::Unpack(Base->Find("char-set"), &Route->CharSet))
							Route->CharSet = "utf-8";

						if (!NMake::Unpack(Base->Find("access-control-allow-origin"), &Route->AccessControlAllowOrigin))
							Route->AccessControlAllowOrigin.clear();

						if (!NMake::Unpack(Base->Find("refer"), &Route->Refer))
							Route->Refer.clear();

						if (!NMake::Unpack(Base->Find("web-socket-timeout"), &Route->WebSocketTimeout))
							Route->WebSocketTimeout = 30000;

						if (!NMake::Unpack(Base->Find("static-file-max-age"), &Route->StaticFileMaxAge))
							Route->StaticFileMaxAge = 604800;

						if (!NMake::Unpack(Base->Find("graceful-time-wait"), &Route->GracefulTimeWait))
							Route->GracefulTimeWait = 1;

						if (!NMake::Unpack(Base->Find("max-cache-length"), &Route->MaxCacheLength))
							Route->MaxCacheLength = 16384;

						if (!NMake::Unpack(Base->Find("allow-directory-listing"), &Route->AllowDirectoryListing))
							Route->AllowDirectoryListing = false;

						if (!NMake::Unpack(Base->Find("allow-web-socket"), &Route->AllowWebSocket))
							Route->AllowWebSocket = false;

						if (!NMake::Unpack(Base->Find("allow-send-file"), &Route->AllowSendFile))
							Route->AllowSendFile = false;

						if (!NMake::Unpack(Base->Find("proxy-ip-address"), &Route->ProxyIpAddress))
							Route->ProxyIpAddress.clear();
					}
				}

				Object->Configure(Router);
				delete Document;

				return (void*)Object;
			}

			ShapeProcessor::ShapeProcessor(ContentManager* Manager) : FileProcessor(Manager)
			{
			}
			ShapeProcessor::~ShapeProcessor()
			{
			}
			void ShapeProcessor::Free(AssetResource* Asset)
			{
				if (Asset->Resource != nullptr)
				{
					Compute::UnmanagedShape* Shape = (Compute::UnmanagedShape*)Asset->Resource;
					Compute::Simulator::FreeUnmanagedShape(Shape->Shape);
					delete Shape;
				}
			}
			void* ShapeProcessor::Duplicate(AssetResource* Asset, ContentArgs* Args)
			{
				return Asset->Resource;
			}
			void* ShapeProcessor::Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Args)
			{
				auto* Document = Content->Load<Rest::Document>(Stream->Filename(), nullptr);
				if (!Document)
					return nullptr;

				Compute::UnmanagedShape* Object = new Compute::UnmanagedShape();
				std::vector<Rest::Document*> Meshes = Document->FindCollectionPath("meshes.mesh");
				for (auto&& Mesh : Meshes)
				{
					if (!NMake::Unpack(Mesh->Find("indices"), &Object->Indices))
					{
						delete Document;
						delete Object;
						return nullptr;
					}

					if (!NMake::Unpack(Mesh->Find("vertices"), &Object->Vertices))
					{
						delete Document;
						delete Object;
						return nullptr;
					}
				}

				Object->Shape = Compute::Simulator::CreateUnmanagedShape(Object->Vertices);
				delete Document;

				if (!Object->Shape)
				{
					delete Object;
					return nullptr;
				}

				Content->Cache(this, Stream->Filename(), Object);
				return (void*)Object;
			}
		}
	}
}