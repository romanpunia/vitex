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
			Compute::Matrix4x4 ToMatrix(const aiMatrix4x4& Root)
			{
				return Compute::Matrix4x4(Root.a1, Root.a2, Root.a3, Root.a4, Root.b1, Root.b2, Root.b3, Root.b4, Root.c1, Root.c2, Root.c3, Root.c4, Root.d1, Root.d2, Root.d3, Root.d4);
			}
#endif
			Asset::Asset(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* Asset::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
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

			Material::Material(ContentManager* Manager) : Processor(Manager)
			{
			}
			void Material::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Engine::Material*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* Material::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Engine::Material*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* Material::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
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
					});
				}

				if (Series::Unpack(Data->Get("normal-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetNormalMap(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("metallic-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetMetallicMap(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("roughness-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetRoughnessMap(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("height-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetHeightMap(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("occlusion-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetOcclusionMap(NewTexture);
					});
				}

				if (Series::Unpack(Data->Get("emission-map"), &Path) && !Path.empty())
				{
					Content->LoadAsync<Graphics::Texture2D>(Path).Await([Object](Graphics::Texture2D* NewTexture)
					{
						Object->SetEmissionMap(NewTexture);
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
				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				return Object;
			}
			bool Material::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, false, "stream should be set");
				ED_ASSERT(Instance != nullptr, false, "instance should be set");

				Engine::Material* Object = (Engine::Material*)Instance;
				Core::Schema* Data = Core::Var::Set::Object();
				Data->Key = "material";

				AssetCache* Asset = Content->Find<Graphics::Texture2D>(Object->GetDiffuseMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("diffuse-map"), Asset->Path);

				Asset = Content->Find<Graphics::Texture2D>(Object->GetNormalMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("normal-map"), Asset->Path);

				Asset = Content->Find<Graphics::Texture2D>(Object->GetMetallicMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("metallic-map"), Asset->Path);

				Asset = Content->Find<Graphics::Texture2D>(Object->GetRoughnessMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("roughness-map"), Asset->Path);

				Asset = Content->Find<Graphics::Texture2D>(Object->GetHeightMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("height-map"), Asset->Path);

				Asset = Content->Find<Graphics::Texture2D>(Object->GetOcclusionMap());
				if (Asset != nullptr)
					Series::Pack(Data->Set("occlusion-map"), Asset->Path);

				Asset = Content->Find<Graphics::Texture2D>(Object->GetEmissionMap());
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

			SceneGraph::SceneGraph(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* SceneGraph::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				Engine::SceneGraph::Desc I = Engine::SceneGraph::Desc::Get(Application::Get());
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				ED_ASSERT(I.Shared.Device != nullptr, nullptr, "graphics device should be set");

				Core::Schema* Schema = Content->Load<Core::Schema>(Stream->GetSource());
				if (!Schema)
					return nullptr;

				Core::Schema* Metadata = Schema->Find("metadata");
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

				Core::Schema* Materials = Schema->Find("materials");
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

				Core::Schema* Entities = Schema->Find("entities");
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
									ED_WARN("[engine] component with id %llu cannot be created", Id);
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

				ED_RELEASE(Schema);
				Object->Snapshot = nullptr;
				Object->Actualize();

				return Object;
			}
			bool SceneGraph::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
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

				Core::Schema* Schema = Core::Var::Set::Object();
				Schema->Key = "scene";

				auto& Conf = Object->GetConf();
				Core::Schema* Metadata = Schema->Set("metadata");
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

				Core::Schema* Materials = Schema->Set("materials", Core::Var::Array());
				for (size_t i = 0; i < Object->GetMaterialsCount(); i++)
				{
					Engine::Material* Material = Object->GetMaterial(i);
					if (!Material)
						continue;

					std::string Path;
					AssetCache* Asset = Content->Find<Engine::Material>(Material);
					if (!Asset)
						Path.assign("./materials/" + Material->GetName() + "_" + Compute::Codec::HexEncode(Compute::Crypto::RandomBytes(6)));
					else
						Path.assign(Asset->Path);

					if (!Core::Parser(&Path).EndsWith(Ext))
						Path.append(Ext);

					if (Content->Save<Engine::Material>(Path, Material, Args))
					{
						Core::Schema* Where = Materials->Set("material");
						Series::Pack(Where, Material->Slot);
						Series::Pack(Where, Path);
					}
				}

				Core::Schema* Entities = Schema->Set("entities", Core::Var::Array());
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
				Content->Save<Core::Schema>(Stream->GetSource(), Schema, Args);
				ED_RELEASE(Schema);

				return true;
			}

			AudioClip::AudioClip(ContentManager* Manager) : Processor(Manager)
			{
			}
			AudioClip::~AudioClip()
			{
			}
			void AudioClip::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Audio::AudioClip*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* AudioClip::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "asset resource should be set");

				((Audio::AudioClip*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* AudioClip::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				if (Core::Parser(&Stream->GetSource()).EndsWith(".wav"))
					return DeserializeWAVE(Stream, Offset, Args);
				else if (Core::Parser(&Stream->GetSource()).EndsWith(".ogg"))
					return DeserializeOGG(Stream, Offset, Args);

				return nullptr;
			}
			void* AudioClip::DeserializeWAVE(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
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

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				return Object;
#else
				return nullptr;
#endif
			}
			void* AudioClip::DeserializeOGG(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
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

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				return Object;
			}

			Texture2D::Texture2D(ContentManager* Manager) : Processor(Manager)
			{
			}
			Texture2D::~Texture2D()
			{
			}
			void Texture2D::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Graphics::Texture2D*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* Texture2D::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Graphics::Texture2D*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* Texture2D::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::vector<char> Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Size)
				{
					Data.reserve(Data.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Data.push_back(Buffer[i]);
				});

				if (ED_LEFT_HANDED)
					stbi_set_flip_vertically_on_load(0);
				else
					stbi_set_flip_vertically_on_load(1);

				int Width, Height, Channels;
				unsigned char* Resource = stbi_load_from_memory((const unsigned char*)Data.data(), (int)Data.size(), &Width, &Height, &Channels, STBI_rgb_alpha);
				if (!Resource)
					return nullptr;

				auto* Device = Content->GetDevice();
				Graphics::Texture2D::Desc F = Graphics::Texture2D::Desc();
				F.Data = (void*)Resource;
				F.Width = (unsigned int)Width;
				F.Height = (unsigned int)Height;
				F.RowPitch = (Width * 32 + 7) / 8;
				F.DepthPitch = F.RowPitch * Height;
				F.MipLevels = Device->GetMipLevel(F.Width, F.Height);

				Device->Lock();
				Graphics::Texture2D* Object = Device->CreateTexture2D(F);
				Device->Unlock();

				stbi_image_free(Resource);
				if (!Object)
					return nullptr;

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				return (void*)Object;
			}

			Shader::Shader(ContentManager* Manager) : Processor(Manager)
			{
			}
			Shader::~Shader()
			{
			}
			void Shader::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Graphics::Shader*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* Shader::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Graphics::Shader*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* Shader::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
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
				Device->Unlock();
				Graphics::Shader* Object = Device->CreateShader(I);
				Device->Lock();

				if (!Object)
					return nullptr;

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				return Object;
			}

			Model::Model(ContentManager* Manager) : Processor(Manager)
			{
			}
			Model::~Model()
			{
			}
			void Model::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Graphics::Model*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* Model::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Graphics::Model*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* Model::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				auto* Schema = Content->Load<Core::Schema>(Stream->GetSource());
				if (!Schema)
					return nullptr;

				auto Object = new Graphics::Model();
				Series::Unpack(Schema->Find("root"), &Object->Root);
				Series::Unpack(Schema->Find("max"), &Object->Max);
				Series::Unpack(Schema->Find("min"), &Object->Min);

				std::vector<Core::Schema*> Meshes = Schema->FetchCollection("meshes.mesh");
				for (auto&& Mesh : Meshes)
				{
					Graphics::MeshBuffer::Desc F;
					F.AccessFlags = Options.AccessFlags;
					F.Usage = Options.Usage;

					if (!Series::Unpack(Mesh->Find("indices"), &F.Indices))
					{
						ED_RELEASE(Schema);
						return nullptr;
					}

					if (!Series::Unpack(Mesh->Find("vertices"), &F.Elements))
					{
						ED_RELEASE(Schema);
						return nullptr;
					}

					auto* Device = Content->GetDevice();
					Compute::Geometric::TexCoordRhToLh(F.Elements);

					Device->Lock();
					Object->Meshes.push_back(Device->CreateMeshBuffer(F));
					Device->Unlock();

					auto* Sub = Object->Meshes.back();
					Series::Unpack(Mesh->Find("name"), &Sub->Name);
					Series::Unpack(Mesh->Find("world"), &Sub->World);
				}

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();
				ED_RELEASE(Schema);

				return (void*)Object;
			}
			Core::Schema* Model::Import(const std::string& Path, uint64_t Opts)
			{
#ifdef ED_HAS_ASSIMP
				Assimp::Importer Importer;

				auto* Scene = Importer.ReadFile(Path, (unsigned int)Opts);
				if (!Scene)
				{
					ED_ERR("[engine] cannot import mesh\n\t%s", Importer.GetErrorString());
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

				auto* Schema = Core::Var::Set::Object();
				Schema->Key = "model";

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

				Series::Pack(Schema->Set("options"), Opts);
				Series::Pack(Schema->Set("root"), ToMatrix(Scene->mRootNode->mTransformation.Inverse()).Transpose());
				Series::Pack(Schema->Set("max"), Compute::Vector4(Info.PX, Info.PY, Info.PZ, Max));
				Series::Pack(Schema->Set("min"), Compute::Vector4(Info.NX, Info.NY, Info.NZ, Min));
				Series::Pack(Schema->Set("joints", Core::Var::Array()), Joints);

				Core::Schema* Meshes = Schema->Set("meshes", Core::Var::Array());
				for (auto&& It : Info.Meshes)
				{
					Core::Schema* Mesh = Meshes->Set("mesh");
					Series::Pack(Mesh->Set("name"), It.Name);
					Series::Pack(Mesh->Set("world"), It.World);
					Series::Pack(Mesh->Set("vertices"), It.Vertices);
					Series::Pack(Mesh->Set("indices"), It.Indices);
				}

				return Schema;
#else
				return nullptr;
#endif
			}
			void Model::ProcessNode(void* Scene_, void* Node_, MeshInfo* Info, const Compute::Matrix4x4& Global)
			{
#ifdef ED_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;
				Compute::Matrix4x4 World = ToMatrix(Node->mTransformation).Transpose() * Global;

				for (unsigned int n = 0; n < Node->mNumMeshes; n++)
					ProcessMesh(Scene, Scene->mMeshes[Node->mMeshes[n]], Info, World);

				for (unsigned int n = 0; n < Node->mNumChildren; n++)
					ProcessNode(Scene, Node->mChildren[n], Info, World);
#endif
			}
			void Model::ProcessMesh(void*, void* Mesh_, MeshInfo* Info, const Compute::Matrix4x4& Global)
			{
#ifdef ED_HAS_ASSIMP
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
					Blob.Name = Compute::Crypto::Hash(Compute::Digests::MD5(), Compute::Crypto::RandomBytes(8)).substr(0, 8);

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
						Info->Joints.emplace_back(std::make_pair(Index, Element));
					}
					else
						Index = It->first;

					for (unsigned int w = 0; w < Joint->mNumWeights; w++)
					{
						auto& Element = Blob.Vertices[Joint->mWeights[w].mVertexId];
						if (Element.JointIndex0 == -1.0f)
						{
							Element.JointIndex0 = (float)Index;
							Element.JointBias0 = Joint->mWeights[w].mWeight;
						}
						else if (Element.JointIndex1 == -1.0f)
						{
							Element.JointIndex1 = (float)Index;
							Element.JointBias1 = Joint->mWeights[w].mWeight;
						}
						else if (Element.JointIndex2 == -1.0f)
						{
							Element.JointIndex2 = (float)Index;
							Element.JointBias2 = Joint->mWeights[w].mWeight;
						}
						else if (Element.JointIndex3 == -1.0f)
						{
							Element.JointIndex3 = (float)Index;
							Element.JointBias3 = Joint->mWeights[w].mWeight;
						}
					}
				}

				Info->Meshes.emplace_back(Blob);
#endif
			}
			void Model::ProcessHeirarchy(void* Scene_, void* Node_, MeshInfo* Info, Compute::Joint* Parent)
			{
#ifdef ED_HAS_ASSIMP
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
			std::vector<std::pair<int64_t, Compute::Joint>>::iterator Model::FindJoint(std::vector<std::pair<int64_t, Compute::Joint>>& Joints, const std::string& Name)
			{
				for (auto It = Joints.begin(); It != Joints.end(); ++It)
				{
					if (It->second.Name == Name)
						return It;
				}

				return Joints.end();
			}

			SkinModel::SkinModel(ContentManager* Manager) : Processor(Manager)
			{
			}
			SkinModel::~SkinModel()
			{
			}
			void SkinModel::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_RELEASE((Graphics::SkinModel*)Asset->Resource);
				Asset->Resource = nullptr;
			}
			void* SkinModel::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Graphics::SkinModel*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* SkinModel::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				auto* Schema = Content->Load<Core::Schema>(Stream->GetSource());
				if (!Schema)
					return nullptr;

				auto Object = new Graphics::SkinModel();
				Series::Unpack(Schema->Find("root"), &Object->Root);
				Series::Unpack(Schema->Find("max"), &Object->Max);
				Series::Unpack(Schema->Find("min"), &Object->Min);
				Series::Unpack(Schema->Find("joints"), &Object->Joints);

				std::vector<Core::Schema*> Meshes = Schema->FetchCollection("meshes.mesh");
				for (auto&& Mesh : Meshes)
				{
					Graphics::SkinMeshBuffer::Desc F;
					F.AccessFlags = Options.AccessFlags;
					F.Usage = Options.Usage;

					if (!Series::Unpack(Mesh->Find("indices"), &F.Indices))
					{
						ED_RELEASE(Schema);
						return nullptr;
					}

					if (!Series::Unpack(Mesh->Find("vertices"), &F.Elements))
					{
						ED_RELEASE(Schema);
						return nullptr;
					}

					auto* Device = Content->GetDevice();
					Compute::Geometric::TexCoordRhToLh(F.Elements);

					Device->Lock();
					Object->Meshes.push_back(Device->CreateSkinMeshBuffer(F));
					Device->Unlock();

					auto* Sub = Object->Meshes.back();
					Series::Unpack(Mesh->Find("name"), &Sub->Name);
					Series::Unpack(Mesh->Find("world"), &Sub->World);
				}

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				ED_RELEASE(Schema);
				return (void*)Object;
			}
			Core::Schema* SkinModel::ImportAnimation(const std::string& Path, uint64_t Opts)
			{
#ifdef ED_HAS_ASSIMP
				Assimp::Importer Importer;

				auto* Scene = Importer.ReadFile(Path, (unsigned int)Opts);
				if (!Scene)
				{
					ED_ERR("[engine] cannot import mesh animation because %s", Importer.GetErrorString());
					return nullptr;
				}

				std::unordered_map<std::string, MeshNode> Joints;
				int64_t Index = 0;
				ProcessNode((void*)Scene, (void*)Scene->mRootNode, &Joints, Index);
				ProcessHeirarchy((void*)Scene, (void*)Scene->mRootNode, &Joints);

				std::vector<Compute::SkinAnimatorClip> Clips;
				Clips.resize((size_t)Scene->mNumAnimations);

				for (size_t i = 0; i < (size_t)Scene->mNumAnimations; i++)
				{
					aiAnimation* Animation = Scene->mAnimations[i];
					Compute::SkinAnimatorClip* Clip = &Clips[i];
					Clip->Name = Animation->mName.C_Str();
					Clip->Duration = (float)Animation->mDuration;

					if (Animation->mTicksPerSecond > 0.0f)
						Clip->Rate = (float)Animation->mTicksPerSecond;

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

						for (size_t k = 0; k < (size_t)Channel->mNumPositionKeys; k++)
						{
							auto& Keys = Clip->Keys[k].Pose;
							ProcessKeys(&Keys, &Joints);

							aiVector3D& V = Channel->mPositionKeys[k].mValue;
							Keys[(size_t)It->second.Index].Position.X = V.x;
							Keys[(size_t)It->second.Index].Position.Y = V.y;
							Keys[(size_t)It->second.Index].Position.Z = V.z;
						}

						for (size_t k = 0; k < (size_t)Channel->mNumRotationKeys; k++)
						{
							auto& Keys = Clip->Keys[k].Pose;
							ProcessKeys(&Keys, &Joints);

							aiQuaternion Q1 = Channel->mRotationKeys[k].mValue;
							Compute::Quaternion Q2(Q1.x, Q1.y, Q1.z, Q1.w);

							Keys[(size_t)It->second.Index].Rotation = Q2.GetEuler().rLerp();
						}

						for (size_t k = 0; k < (size_t)Channel->mNumScalingKeys; k++)
						{
							auto& Keys = Clip->Keys[k].Pose;
							ProcessKeys(&Keys, &Joints);

							aiVector3D& V = Channel->mScalingKeys[k].mValue;
							Keys[(size_t)It->second.Index].Scale.X = V.x;
							Keys[(size_t)It->second.Index].Scale.Y = V.y;
							Keys[(size_t)It->second.Index].Scale.Z = V.z;
						}
					}
				}

				auto* Schema = Core::Var::Set::Object();
				Schema->Key = "animation";

				Series::Pack(Schema, Clips);
				return Schema;
#else
				return nullptr;
#endif
			}
			void SkinModel::ProcessNode(void* Scene_, void* Node_, std::unordered_map<std::string, MeshNode>* Joints, int64_t& Id)
			{
#ifdef ED_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;

				for (unsigned int n = 0; n < Node->mNumMeshes; n++)
				{
					auto* Mesh = Scene->mMeshes[Node->mMeshes[n]];
					for (unsigned int j = 0; j < Mesh->mNumBones; j++)
					{
						auto& Joint = Mesh->mBones[j];
						auto It = Joints->find(Joint->mName.C_Str());
						if (It == Joints->end())
						{
							MeshNode Element;
							Element.Index = Id++;
							Joints->insert(std::make_pair(Joint->mName.C_Str(), Element));
						}
					}
				}

				for (unsigned int n = 0; n < Node->mNumChildren; n++)
					ProcessNode(Scene, Node->mChildren[n], Joints, Id);
#endif
			}
			void SkinModel::ProcessHeirarchy(void* Scene_, void* Node_, std::unordered_map<std::string, MeshNode>* Joints)
			{
#ifdef ED_HAS_ASSIMP
				auto* Scene = (aiScene*)Scene_;
				auto* Node = (aiNode*)Node_;
				auto It = Joints->find(Node->mName.C_Str());

				if (It != Joints->end())
					It->second.Transform = ToMatrix(Node->mTransformation).Transpose();

				for (int64_t i = 0; i < Node->mNumChildren; i++)
					ProcessHeirarchy(Scene, Node->mChildren[i], Joints);
#endif
			}
			void SkinModel::ProcessKeys(std::vector<Compute::AnimatorKey>* Keys, std::unordered_map<std::string, MeshNode>* Joints)
			{
				if (Keys->size() < Joints->size())
				{
					Keys->resize(Joints->size());
					for (auto It = Joints->begin(); It != Joints->end(); ++It)
					{
						auto* Key = &Keys->at((size_t)It->second.Index);
						Key->Position = It->second.Transform.Position();
						Key->Rotation = It->second.Transform.Rotation();
						Key->Scale = It->second.Transform.Scale();
					}
				}
			}

			Schema::Schema(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* Schema::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
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
			bool Schema::Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Args)
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

			Server::Server(ContentManager* Manager) : Processor(Manager)
			{
			}
			void* Server::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				std::string N = Network::Multiplexer::GetLocalAddress();
				std::string D = Core::OS::Path::GetDirectory(Stream->GetSource().c_str());
				auto* Schema = Content->Load<Core::Schema>(Stream->GetSource());
				auto* Router = new Network::HTTP::MapRouter();
				auto* Object = new Network::HTTP::Server();

				Application* App = Application::Get();
				if (App != nullptr)
					Router->VM = App->VM;

				if (Schema == nullptr)
				{
					ED_RELEASE(Router);
					return (void*)Object;
				}
				else if (Callback)
					Callback((void*)Object, Schema);

				Core::Schema* Config = Schema->Find("netstat");
				if (Config != nullptr)
				{
					if (Series::Unpack(Config->Fetch("module-root"), &Router->ModuleRoot))
						Core::Parser(&Router->ModuleRoot).Eval(N, D);

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

				std::vector<Core::Schema*> Certificates = Schema->FindCollection("certificate", true);
				for (auto&& It : Certificates)
				{
					std::string Name;
					if (!Series::Unpack(It, &Name))
						Name = "*";

					Network::SocketCertificate* Cert = &Router->Certificates[Core::Parser(&Name).Eval(N, D).R()];
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

					Core::Parser(&Cert->Key).Eval(N, D).R();
					Core::Parser(&Cert->Chain).Eval(N, D).R();
				}

				std::vector<Core::Schema*> Listeners = Schema->FindCollection("listen", true);
				for (auto&& It : Listeners)
				{
					std::string Name;
					if (!Series::Unpack(It, &Name))
						Name = "*";

					Network::RemoteHost* Host = &Router->Listeners[Core::Parser(&Name).Eval(N, D).R()];
					if (!Series::Unpack(It->Find("hostname"), &Host->Hostname))
						Host->Hostname = "0.0.0.0";

					Core::Parser(&Host->Hostname).Eval(N, D).R();
					if (!Series::Unpack(It->Find("port"), &Host->Port))
						Host->Port = 80;

					if (!Series::Unpack(It->Find("secure"), &Host->Secure))
						Host->Secure = false;
				}

				std::vector<Core::Schema*> Sites = Schema->FindCollection("site", true);
				for (auto&& It : Sites)
				{
					std::string Name = "*";
					Series::Unpack(It, &Name);

					Network::HTTP::SiteEntry* Site = Router->Site(Core::Parser(&Name).Eval(N, D).Get());
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
						Core::Parser(&Site->Gateway.Session.DocumentRoot).Eval(N, D);

					if (!Series::Unpack(It->Fetch("gateway.session.expires"), &Site->Gateway.Session.Expires))
						Site->Gateway.Session.Expires = 604800;

					if (!Series::Unpack(It->Fetch("gateway.verify"), &Site->Gateway.Verify))
						Site->Gateway.Verify = false;

					if (!Series::Unpack(It->Fetch("gateway.enabled"), &Site->Gateway.Enabled))
						Site->Gateway.Enabled = false;

					if (!Series::Unpack(It->Find("max-resources"), &Site->MaxResources))
						Site->MaxResources = 5;

                    Series::Unpack(It->Find("resource-root"), &Site->ResourceRoot);
                    Core::Parser(&Site->ResourceRoot).Eval(N, D);

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
										Core::Parser(&Pattern).Eval(N, D);

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
										Core::Parser(&Pattern).Eval(N, D);

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
									Core::Parser(&Source.Pattern).Eval(N, D);

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
								Core::Parser(&Route->DocumentRoot).Eval(N, D);

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

				ED_RELEASE(Schema);
				return (void*)Object;
			}

			HullShape::HullShape(ContentManager* Manager) : Processor(Manager)
			{
			}
			HullShape::~HullShape()
			{
			}
			void HullShape::Free(AssetCache* Asset)
			{
				ED_ASSERT_V(Asset != nullptr, "asset should be set");
				ED_ASSERT_V(Asset->Resource != nullptr, "instance should be set");

				Compute::HullShape* Shape = (Compute::HullShape*)Asset->Resource;
				Compute::Simulator::FreeHullShape(Shape->Shape);
				ED_RELEASE(Shape);
			}
			void* HullShape::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Asset != nullptr, nullptr, "asset should be set");
				ED_ASSERT(Asset->Resource != nullptr, nullptr, "instance should be set");

				((Compute::HullShape*)Asset->Resource)->AddRef();
				return Asset->Resource;
			}
			void* HullShape::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				ED_ASSERT(Stream != nullptr, nullptr, "stream should be set");
				auto* Schema = Content->Load<Core::Schema>(Stream->GetSource());
				if (!Schema)
					return nullptr;

				Compute::HullShape* Object = new Compute::HullShape();
				std::vector<Core::Schema*> Meshes = Schema->FetchCollection("meshes.mesh");
				for (auto&& Mesh : Meshes)
				{
					if (!Series::Unpack(Mesh->Find("indices"), &Object->Indices))
					{
						ED_RELEASE(Schema);
						ED_RELEASE(Object);
						return nullptr;
					}

					if (!Series::Unpack(Mesh->Find("vertices"), &Object->Vertices))
					{
						ED_RELEASE(Schema);
						ED_RELEASE(Object);
						return nullptr;
					}
				}

				Object->Shape = Compute::Simulator::CreateHullShape(Object->Vertices);
				ED_RELEASE(Schema);

				if (!Object->Shape)
				{
					ED_RELEASE(Object);
					return nullptr;
				}

				Content->Cache(this, Stream->GetSource(), Object);
				Object->AddRef();

				return (void*)Object;
			}
		}
	}
}
