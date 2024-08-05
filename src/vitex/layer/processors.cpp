#include "processors.h"
#include "../network/http.h"

namespace Vitex
{
	namespace Layer
	{
		namespace Processors
		{
			AssetProcessor::AssetProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> AssetProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");

				Core::Vector<char> Temp;
				Stream->ReadAll([&Temp](uint8_t* Buffer, size_t Size)
				{
					Temp.reserve(Temp.size() + Size);
					for (size_t i = 0; i < Size; i++)
						Temp.push_back(Buffer[i]);
				});

				char* Data = Core::Memory::Allocate<char>(sizeof(char) * Temp.size());
				memcpy(Data, Temp.data(), sizeof(char) * Temp.size());
				return new AssetFile(Data, Temp.size());
			}

			SchemaProcessor::SchemaProcessor(ContentManager* Manager) : Processor(Manager)
			{
			}
			ExpectsContent<void*> SchemaProcessor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
			{
				VI_ASSERT(Stream != nullptr, "stream should be set");
				auto Object = Core::Schema::ConvertFromJSONB([Stream](uint8_t* Buffer, size_t Size) { return Size > 0 ? Stream->Read(Buffer, Size).Or(0) == Size : true; });
				if (Object)
					return *Object;
				
				Core::String Data;
				Stream->Seek(Core::FileSeek::Begin, Offset);
				Stream->ReadAll([&Data](uint8_t* Buffer, size_t Size) { Data.append((char*)Buffer, Size); });

				Object = Core::Schema::ConvertFromJSON(Data);
				if (Object)
					return *Object;

				Object = Core::Schema::ConvertFromXML(Data);
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
					Core::Schema::ConvertToXML(Schema, [Stream, &Offset](Core::VarForm Pretty, const std::string_view& Buffer)
					{
						if (!Buffer.empty())
							Stream->Write((uint8_t*)Buffer.data(), Buffer.size());

						switch (Pretty)
						{
							case Vitex::Core::VarForm::Tab_Decrease:
								Offset.erase(Offset.end() - 1);
								break;
							case Vitex::Core::VarForm::Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Vitex::Core::VarForm::Write_Space:
								Stream->Write((uint8_t*)" ", 1);
								break;
							case Vitex::Core::VarForm::Write_Line:
								Stream->Write((uint8_t*)"\n", 1);
								break;
							case Vitex::Core::VarForm::Write_Tab:
								Stream->Write((uint8_t*)Offset.c_str(), Offset.size());
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
					Core::Schema::ConvertToJSON(Schema, [Stream, &Offset](Core::VarForm Pretty, const std::string_view& Buffer)
					{
						if (!Buffer.empty())
							Stream->Write((uint8_t*)Buffer.data(), Buffer.size());

						switch (Pretty)
						{
							case Vitex::Core::VarForm::Tab_Decrease:
								Offset.erase(Offset.end() - 1);
								break;
							case Vitex::Core::VarForm::Tab_Increase:
								Offset.append(1, '\t');
								break;
							case Vitex::Core::VarForm::Write_Space:
								Stream->Write((uint8_t*)" ", 1);
								break;
							case Vitex::Core::VarForm::Write_Line:
								Stream->Write((uint8_t*)"\n", 1);
								break;
							case Vitex::Core::VarForm::Write_Tab:
								Stream->Write((uint8_t*)Offset.c_str(), Offset.size());
								break;
							default:
								break;
						}
					});
					return Core::Expectation::Met;
				}
				else if (Type->second == Core::Var::String("JSONB"))
				{
					Core::Schema::ConvertToJSONB(Schema, [Stream](Core::VarForm, const std::string_view& Buffer)
					{
						if (!Buffer.empty())
							Stream->Write((uint8_t*)Buffer.data(), Buffer.size());
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
				auto BlobStatus = Content->Load<Core::Schema>(Stream->VirtualName());
				if (!BlobStatus)
					return BlobStatus.Error();

				auto NetAddresses = Network::Utils::GetHostIpAddresses();
				Core::String BaseDirectory = Core::OS::Path::GetDirectory(Stream->VirtualName());
				Core::UPtr<Network::HTTP::MapRouter> Router = new Network::HTTP::MapRouter();
				Core::UPtr<Network::HTTP::Server> Object = new Network::HTTP::Server();
				Core::UPtr<Core::Schema> Blob = *BlobStatus;
				if (Callback)
					Callback((void*)*Object, *Blob);

				Core::Vector<Core::Schema*> Certificates = Blob->FindCollection("certificate", true);
				for (auto&& It : Certificates)
				{
					Core::String Name;
					if (Series::Unpack(It, &Name))
						Core::Stringify::EvalEnvs(Name, BaseDirectory, NetAddresses);
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

					Core::Stringify::EvalEnvs(Cert->Blob.PrivateKey, BaseDirectory, NetAddresses);
					if (!Cert->Blob.PrivateKey.empty())
					{
						auto Data = Core::OS::File::ReadAsString(Cert->Blob.PrivateKey);
						if (!Data)
							return ContentException(Core::Stringify::Text("import invalid server private key: %s", Data.Error().message().c_str()));
						
						Cert->Blob.PrivateKey = *Data;
					}

					Core::Stringify::EvalEnvs(Cert->Blob.Certificate, BaseDirectory, NetAddresses);
					if (!Cert->Blob.Certificate.empty())
					{
						auto Data = Core::OS::File::ReadAsString(Cert->Blob.Certificate);
						if (!Data)
							return ContentException(Core::Stringify::Text("import invalid server certificate: %s", Data.Error().message().c_str()));
						
						Cert->Blob.Certificate = *Data;
					}
				}

				Core::Vector<Core::Schema*> Listeners = Blob->FindCollection("listen", true);
				for (auto&& It : Listeners)
				{
					Core::String Pattern, Hostname, Service;
					uint32_t Port = 0; bool Secure = false;
					Series::Unpack(It, &Pattern);
					Series::Unpack(It->Find("hostname"), &Hostname);
					Series::Unpack(It->Find("service"), &Service);
					Series::Unpack(It->Find("port"), &Port);
					Series::Unpack(It->Find("secure"), &Secure);
					Core::Stringify::EvalEnvs(Pattern, BaseDirectory, NetAddresses);
					Core::Stringify::EvalEnvs(Hostname, BaseDirectory, NetAddresses);

					auto Status = Router->Listen(Pattern.empty() ? "*" : Pattern, Hostname.empty() ? "0.0.0.0" : Hostname, Service.empty() ? (Port > 0 ? Core::ToString(Port) : "80") : Service, Secure);
					if (!Status)
						return ContentException(Core::Stringify::Text("bind %s:%i listener from %s: %s", Hostname.c_str(), (int32_t)Port, Pattern.c_str(), Status.Error().message().c_str()));
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
					Core::Stringify::EvalEnvs(Router->Session.Directory, BaseDirectory, NetAddresses);
					Core::Stringify::EvalEnvs(Router->TemporaryDirectory, BaseDirectory, NetAddresses);

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
								Core::Stringify::EvalEnvs(Route->FilesDirectory, BaseDirectory, NetAddresses);

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
										Core::Stringify::EvalEnvs(Pattern, BaseDirectory, NetAddresses);

									Route->IndexFiles.push_back(Pattern);
								}
							}

							for (auto& File : TryFiles)
							{
								Core::String Pattern;
								if (Series::Unpack(File, &Pattern))
								{
									if (!File->GetAttribute("use"))
										Core::Stringify::EvalEnvs(Pattern, BaseDirectory, NetAddresses);

									Route->TryFiles.push_back(Pattern);
								}
							}

							for (auto& File : ErrorFiles)
							{
								Network::HTTP::ErrorFile Source;
								Series::Unpack(File->Find("status"), &Source.StatusCode);
								if (Series::Unpack(File->Find("file"), &Source.Pattern))
									Core::Stringify::EvalEnvs(Source.Pattern, BaseDirectory, NetAddresses);
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
				if (Configure != Args.end() && Configure->second.GetBoolean())
				{
					auto Status = Object->Configure(Router.Reset());
					if (!Status)
						return ContentException(std::move(Status.Error().message()));
				}
				else
					Object->SetRouter(Router.Reset());

				return Object.Reset();
			}
		}
	}
}
