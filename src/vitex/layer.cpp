#include "layer.h"
#include "layer/processors.h"
#include "network/http.h"
#include "network/mdb.h"
#include "network/pdb.h"
#include "vitex.h"
#include <sstream>
#define CONTENT_BLOCKED_WAIT_MS 50
#define CONTENT_ARCHIVE_HEADER_MAGIC "2a20c37fc5ea31f2057d1bc3aa2ad7b8"
#define CONTENT_ARCHIVE_HASH_MAGIC "a6ef0892ed9fc12442a3ed68b0266b5a"
#define CONTENT_ARCHIVE_MAX_FILES 1024 * 1024 * 16
#define CONTENT_ARCHIVE_MAX_PATH 1024
#define CONTENT_ARCHIVE_MAX_SIZE 1024llu * 1024 * 1024 * 4

namespace Vitex
{
	namespace Layer
	{
		AssetFile::AssetFile(char* SrcBuffer, size_t SrcSize) noexcept : Buffer(SrcBuffer), Length(SrcSize)
		{
		}
		AssetFile::~AssetFile() noexcept
		{
			if (Buffer != nullptr)
				Core::Memory::Deallocate(Buffer);
		}
		char* AssetFile::GetBuffer()
		{
			return Buffer;
		}
		size_t AssetFile::Size()
		{
			return Length;
		}

		ContentException::ContentException() : Core::SystemException()
		{
		}
		ContentException::ContentException(const std::string_view& Message) : Core::SystemException(Message)
		{
		}
		const char* ContentException::type() const noexcept
		{
			return "content_error";
		}

		void Series::Pack(Core::Schema* V, bool Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("b", Core::Var::Boolean(Value));
		}
		void Series::Pack(Core::Schema* V, int32_t Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, int64_t Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, uint32_t Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, uint64_t Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("i", Core::Var::Integer(Value));
		}
		void Series::Pack(Core::Schema* V, float Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("n", Core::Var::Number(Value));
		}
		void Series::Pack(Core::Schema* V, double Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("n", Core::Var::Number(Value));
		}
		void Series::Pack(Core::Schema* V, const std::string_view& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			V->SetAttribute("s", Core::Var::String(Value));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<bool>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("b-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<int32_t>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<int64_t>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<uint32_t>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<uint64_t>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("i-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<float>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<double>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << It << " ";

			V->Set("n-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::Vector<Core::String>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::Schema* Array = V->Set("s-array", Core::Var::Array());
			for (auto&& It : Value)
				Array->Set("s", Core::Var::String(It));

			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		void Series::Pack(Core::Schema* V, const Core::UnorderedMap<size_t, size_t>& Value)
		{
			VI_ASSERT(V != nullptr, "schema should be set");
			Core::StringStream Stream;
			for (auto&& It : Value)
				Stream << (uint64_t)It.first << " " << (uint64_t)It.second << " ";

			V->Set("gl-array", Core::Var::String(Stream.str().substr(0, Stream.str().size() - 1)));
			V->Set("size", Core::Var::Integer((int64_t)Value.size()));
		}
		bool Series::Unpack(Core::Schema* V, bool* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = V->GetAttributeVar("b").GetBoolean();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, int32_t* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = (int)V->GetAttributeVar("i").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, int64_t* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = V->GetAttributeVar("i").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, uint32_t* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = (uint32_t)V->GetAttributeVar("i").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, uint64_t* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = (uint64_t)V->GetAttributeVar("i").GetInteger();
			return true;
		}
		bool Series::UnpackA(Core::Schema* V, size_t* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = (size_t)V->GetAttributeVar("i").GetInteger();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, float* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = (float)V->GetAttributeVar("n").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, double* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = (int)V->GetAttributeVar("n").GetNumber();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::String* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			*O = V->GetAttributeVar("s").GetBlob();
			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<bool>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("b-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				bool Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<int32_t>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				int Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<int64_t>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				int64_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<uint32_t>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				uint32_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<uint64_t>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("i-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				uint64_t Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<float>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("n-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				float Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<double>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("n-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->resize((size_t)Size);

			for (auto It = O->begin(); It != O->end(); ++It)
			{
				double Item;
				Stream >> Item;
				*It = Item;
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::Vector<Core::String>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::Schema* Array = V->Get("s-array");
			if (!Array)
				return false;

			for (auto&& It : Array->GetChilds())
			{
				if (It->Key == "s" && It->Value.GetType() == Core::VarType::String)
					O->push_back(It->Value.GetBlob());
			}

			return true;
		}
		bool Series::Unpack(Core::Schema* V, Core::UnorderedMap<size_t, size_t>* O)
		{
			VI_ASSERT(O != nullptr, "output should be set");
			if (!V)
				return false;

			Core::String Array(V->GetVar("gl-array").GetBlob());
			int64_t Size = V->GetVar("size").GetInteger();
			if (Array.empty() || !Size)
				return false;

			Core::StringStream Stream(Array);
			O->reserve((size_t)Size);

			for (size_t i = 0; i < (size_t)Size; i++)
			{
				uint64_t GlobalIndex = 0;
				uint64_t LocalIndex = 0;
				Stream >> GlobalIndex;
				Stream >> LocalIndex;
				(*O)[GlobalIndex] = LocalIndex;
			}

			return true;
		}

		Core::Promise<void> Parallel::Enqueue(Core::TaskCallback&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			return Core::Cotask<void>(std::move(Callback));
		}
		Core::Vector<Core::Promise<void>> Parallel::EnqueueAll(Core::Vector<Core::TaskCallback>&& Callbacks)
		{
			VI_ASSERT(!Callbacks.empty(), "callbacks should not be empty");
			Core::Vector<Core::Promise<void>> Result;
			Result.reserve(Callbacks.size());

			for (auto& Callback : Callbacks)
				Result.emplace_back(Enqueue(std::move(Callback)));

			return Result;
		}
		void Parallel::Wait(Core::Promise<void>&& Value)
		{
			Value.Wait();
		}
		void Parallel::WailAll(Core::Vector<Core::Promise<void>>&& Values)
		{
			for (auto& Value : Values)
				Value.Wait();
		}
		size_t Parallel::GetThreadIndex()
		{
			return Core::Schedule::Get()->GetThreadLocalIndex();
		}
		size_t Parallel::GetThreads()
		{
			return Core::Schedule::Get()->GetThreads(Core::Difficulty::Sync);
		}

		Processor::Processor(ContentManager* NewContent) noexcept : Content(NewContent)
		{
		}
		Processor::~Processor() noexcept
		{
		}
		void Processor::Free(AssetCache* Asset)
		{
		}
		ExpectsContent<void*> Processor::Duplicate(AssetCache* Asset, const Core::VariantArgs& Args)
		{
			return ContentException("not implemented");
		}
		ExpectsContent<void*> Processor::Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args)
		{
			return ContentException("not implemented");
		}
		ExpectsContent<void> Processor::Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args)
		{
			return ContentException("not implemented");
		}
		ContentManager* Processor::GetContent() const
		{
			return Content;
		}

		ContentManager::ContentManager() noexcept : Queue(0)
		{
			auto Directory = Core::OS::Directory::GetWorking();
			if (!Directory)
				return;

			Directory = Core::OS::Path::ResolveDirectory(Directory->c_str());
			if (!Directory)
				return;

			Base = std::move(*Directory);
			SetEnvironment(Base);
		}
		ContentManager::~ContentManager() noexcept
		{
			ClearCache();
			ClearArchives();
			ClearStreams();
			ClearProcessors();
		}
		void ContentManager::ClearCache()
		{
			VI_TRACE("[content] clear cache on 0x%" PRIXPTR, (void*)this);
			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto& Entries : Assets)
			{
				for (auto& Entry : Entries.second)
				{
					if (!Entry.second)
						continue;

					Unique.Negate();
					if (Entry.first != nullptr)
						Entry.first->Free(Entry.second);
					Core::Memory::Delete(Entry.second);
					Unique.Negate();
				}
			}
			Assets.clear();
		}
		void ContentManager::ClearArchives()
		{
			VI_TRACE("[content] clear archives on 0x%" PRIXPTR, (void*)this);
			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto It = Archives.begin(); It != Archives.end(); ++It)
				Core::Memory::Delete(It->second);
			Archives.clear();
		}
		void ContentManager::ClearStreams()
		{
			VI_TRACE("[content] clear streams on 0x%" PRIXPTR, (void*)this);
			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto It = Streams.begin(); It != Streams.end(); ++It)
				It->first->Release();
			Streams.clear();
		}
		void ContentManager::ClearProcessors()
		{
			VI_TRACE("[content] clear processors on 0x%" PRIXPTR, (void*)this);
			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto It = Processors.begin(); It != Processors.end(); ++It)
				Core::Memory::Release(It->second);
			Processors.clear();
		}
		void ContentManager::ClearPath(const std::string_view& Path)
		{
			VI_TRACE("[content] clear path %.*s on 0x%" PRIXPTR, (int)Path.size(), Path.data(), (void*)this);
			auto File = Core::OS::Path::Resolve(Path, Environment, true);
			Core::UMutex<std::mutex> Unique(Exclusive);
			if (File)
			{
				auto It = Assets.find(*File);
				if (It != Assets.end())
					Assets.erase(It);
			}

			auto It = Assets.find(Core::KeyLookupCast(Path));
			if (It != Assets.end())
				Assets.erase(It);
		}
		void ContentManager::SetEnvironment(const std::string_view& Path)
		{
			auto Target = Core::OS::Path::ResolveDirectory(Path);
			if (!Target)
				return;

			Core::String NewPath = *Target;
			Core::Stringify::Replace(NewPath, '\\', '/');
			Core::OS::Directory::SetWorking(NewPath.c_str());
			Core::UMutex<std::mutex> Unique(Exclusive);
			Environment = NewPath;
		}
		ExpectsContent<void> ContentManager::ImportArchive(const std::string_view& Path, bool ValidateChecksum)
		{
			auto TargetPath = Core::OS::Path::Resolve(Path, Environment, true);
			if (!TargetPath)
				return ContentException("archive was not found: " + Core::String(Path));

			Core::UPtr<Core::Stream> Stream = Core::OS::File::Open(*TargetPath, Core::FileMode::Binary_Read_Only).Or(nullptr);
			if (!Stream)
				return ContentException("cannot open archive: " + Core::String(Path));

			uint8_t Header[32] = { 0 }; size_t HeaderSize = sizeof(Header);
			if (Stream->Read(Header, HeaderSize).Or(0) != HeaderSize || memcmp(Header, CONTENT_ARCHIVE_HEADER_MAGIC, HeaderSize) != 0)
				return ContentException("invalid archive header: " + Core::String(Path));

			uint64_t ContentElements = 0;
			if (Stream->Read((uint8_t*)&ContentElements, sizeof(uint64_t)).Or(0) != sizeof(uint64_t))
				return ContentException("invalid archive size: " + Core::String(Path) + " (size = " + Core::ToString(ContentElements) + ")");

			ContentElements = Core::OS::CPU::ToEndianness<uint64_t>(Core::OS::CPU::Endian::Little, ContentElements);
			if (!ContentElements || ContentElements > CONTENT_ARCHIVE_MAX_FILES)
				return ContentException("invalid archive size: " + Core::String(Path) + " (size = " + Core::ToString(ContentElements) + ")");

			uint64_t ContentOffset = 0;
			Core::Vector<Core::UPtr<AssetArchive>> ContentFiles;
			ContentFiles.reserve((size_t)ContentElements);
			for (uint64_t i = 0; i < ContentElements; i++)
			{
				uint64_t ContentLength = 0;
				if (Stream->Read((uint8_t*)&ContentLength, sizeof(uint64_t)).Or(0) != sizeof(uint64_t))
					return ContentException("invalid archive chunk length: " + Core::String(Path) + " (chunk = " + Core::ToString(i) + ")");

				ContentLength = Core::OS::CPU::ToEndianness<uint64_t>(Core::OS::CPU::Endian::Little, ContentLength);
				if (ContentLength > CONTENT_ARCHIVE_MAX_SIZE)
					return ContentException("invalid archive chunk length: " + Core::String(Path) + " (chunk = " + Core::ToString(i) + ")");

				uint64_t PathLength = 0;
				if (Stream->Read((uint8_t*)&PathLength, sizeof(uint64_t)).Or(0) != sizeof(uint64_t))
					return ContentException("invalid archive chunk path size: " + Core::String(Path) + " (chunk = " + Core::ToString(i) + ")");

				PathLength = Core::OS::CPU::ToEndianness<uint64_t>(Core::OS::CPU::Endian::Little, PathLength);
				if (PathLength > CONTENT_ARCHIVE_MAX_PATH)
					return ContentException("invalid archive chunk path size: " + Core::String(Path) + " (chunk = " + Core::ToString(i) + ")");

				Core::String PathValue = Core::String((size_t)PathLength, '\0');
				if (Stream->Read((uint8_t*)PathValue.c_str(), sizeof(char) * (size_t)PathLength).Or(0) != (size_t)PathLength)
					return ContentException("invalid archive chunk path data: " + Core::String(Path) + " (chunk = " + Core::ToString(i) + ")");
				
				AssetArchive* Archive = Core::Memory::New<AssetArchive>();
				Archive->Path = std::move(PathValue);
				Archive->Offset = ContentOffset;
				Archive->Length = ContentLength;
				Archive->Stream = *Stream;
				ContentFiles.push_back(Archive);
				ContentOffset += ContentLength;
			}

			size_t HeadersOffset = Stream->Tell().Or(0);
			if (ValidateChecksum)
			{
				size_t CalculatedChunk = 0;
				Core::String CalculatedHash = CONTENT_ARCHIVE_HASH_MAGIC;
				for (auto& Item : ContentFiles)
				{
					size_t DataLength = Item->Length;
					while (DataLength > 0)
					{
						uint8_t DataBuffer[Core::CHUNK_SIZE];
						size_t DataSize = std::min<size_t>(sizeof(DataBuffer), DataLength);
						size_t ValueSize = Stream->Read(DataBuffer, DataSize).Or(0);
						DataLength -= ValueSize;
						if (!ValueSize)
							break;

						CalculatedHash.append((char*)DataBuffer, ValueSize);
						CalculatedHash = *Compute::Crypto::HashRaw(Compute::Digests::SHA256(), CalculatedHash);
						if (ValueSize < DataSize)
							break;
					}

					if (DataLength > 0)
						return ContentException("invalid archive chunk content data: " + Core::String(Path) + " (chunk = " + Core::ToString(CalculatedChunk) + ")");

					++CalculatedChunk;
				}

				uint8_t RequestedHash[64] = { 0 };
				CalculatedHash = Compute::Codec::HexEncode(CalculatedHash);
				if (Stream->Read(RequestedHash, sizeof(RequestedHash)).Or(0) != sizeof(RequestedHash) || CalculatedHash.size() != sizeof(RequestedHash) || memcmp(RequestedHash, CalculatedHash.c_str(), sizeof(RequestedHash)) != 0)
					return ContentException("invalid archive checksum: " + Core::String(Path) + " (calculated = " + CalculatedHash + ")");
				Stream->Seek(Core::FileSeek::Begin, (int64_t)HeadersOffset);
			}

			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto& Item : ContentFiles)
			{
				auto It = Archives.find(Item->Path);
				if (It != Archives.end())
				{
					Core::Memory::Delete(It->second);
					It->second = Item.Reset();
				}
				else
					Archives[Item->Path] = Item.Reset();
			}
			Streams[Stream.Reset()] = HeadersOffset;
			return Core::Expectation::Met;
		}
		ExpectsContent<void> ContentManager::ExportArchive(const std::string_view& Path, const std::string_view& PhysicalDirectory, const std::string_view& VirtualDirectory)
		{
			VI_ASSERT(!Path.empty() && !PhysicalDirectory.empty(), "path and directory should not be empty");
			auto TargetPath = Core::OS::Path::Resolve(Path, Environment, true);
			if (!TargetPath)
				return ContentException("cannot resolve archive path: " + Core::String(Path));

			auto PhysicalVolume = Core::OS::Path::Resolve(PhysicalDirectory, Environment, true);
			if (!PhysicalVolume)
				return ContentException("cannot resolve archive target path: " + Core::String(PhysicalDirectory));

			size_t PathIndex = TargetPath->find(*PhysicalVolume);
			if (PathIndex != std::string::npos)
			{
				auto Copy = TargetPath->substr(PathIndex + PhysicalVolume->size());
				if (Copy.empty() || Copy.front() != '.' || Copy.find_first_of("\\/") != std::string::npos)
					return ContentException("export path overlaps physical directory: " + *TargetPath);
			}

			Core::String VirtualVolume = Core::String(VirtualDirectory);
			Core::Stringify::Replace(VirtualVolume, '\\', '/');
			while (!VirtualVolume.empty() && VirtualVolume.front() == '/')
				VirtualVolume.erase(VirtualVolume.begin());

			Core::UPtr<Core::Stream> Stream = Core::OS::File::Open(*TargetPath, Core::FileMode::Binary_Write_Only).Or(nullptr);
			if (!Stream)
				return ContentException("cannot open archive: " + Core::String(Path));

			uint8_t Header[] = CONTENT_ARCHIVE_HEADER_MAGIC;
			if (Stream->Write(Header, sizeof(Header) - 1).Or(0) != sizeof(Header) - 1)
				return ContentException("cannot write header: " + *PhysicalVolume);

			Core::UPtr<Core::FileTree> Scanner = new Core::FileTree(*PhysicalVolume);
			size_t ContentCount = Scanner->GetFiles();
			uint64_t ContentElements = Core::OS::CPU::ToEndianness<uint64_t>(Core::OS::CPU::Endian::Little, (uint64_t)ContentCount);
			if (Stream->Write((uint8_t*)&ContentElements, sizeof(uint64_t)).Or(0) != sizeof(uint64_t) || ContentElements > CONTENT_ARCHIVE_MAX_FILES)
				return ContentException("too many files: " + *PhysicalVolume);

			size_t CalculatedChunk = 0;
			Core::String CalculatedHash = CONTENT_ARCHIVE_HASH_MAGIC;
			ExpectsContent<void> ContentError = Core::Expectation::Met;
			Core::Vector<std::pair<Core::UPtr<Core::Stream>, size_t>> ContentFiles;
			Core::Stringify::Replace(*PhysicalVolume, '\\', '/');
			ContentFiles.reserve(ContentCount);	
			Scanner->Loop([&Stream, &ContentFiles, &ContentError, &PhysicalVolume, &VirtualVolume](const Core::FileTree* Tree) mutable
			{
				Core::Stringify::Replace(((Core::FileTree*)Tree)->Path, '\\', '/');
				for (auto& PhysicalPath : Tree->Files)
				{
					Core::String VirtualPath = Tree->Path + '/' + PhysicalPath;
					Core::UPtr<Core::Stream> File = Core::OS::File::Open(VirtualPath, Core::FileMode::Binary_Read_Only).Or(nullptr);
					if (!File)
					{
						ContentError = ContentException("cannot open content path: " + PhysicalPath);
						return false;
					}

					size_t ContentSize = File->Size().Or(0);
					uint64_t ContentLength = Core::OS::CPU::ToEndianness<uint64_t>(Core::OS::CPU::Endian::Little, (uint64_t)ContentSize);
					if (ContentLength > CONTENT_ARCHIVE_MAX_SIZE || Stream->Write((uint8_t*)&ContentLength, sizeof(uint64_t)).Or(0) != sizeof(uint64_t))
					{
						ContentError = ContentException("cannot write content length: " + PhysicalPath);
						return false;
					}

					Core::Stringify::Replace(VirtualPath, *PhysicalVolume, VirtualVolume);
					Core::Stringify::Replace(VirtualPath, "//", "/");
					while (VirtualVolume.empty() && !VirtualPath.empty() && VirtualPath.front() == '/')
						VirtualPath.erase(VirtualPath.begin());

					uint64_t PathLength = Core::OS::CPU::ToEndianness<uint64_t>(Core::OS::CPU::Endian::Little, (uint64_t)VirtualPath.size());
					if (PathLength > CONTENT_ARCHIVE_MAX_PATH || Stream->Write((uint8_t*)&PathLength, sizeof(uint64_t)).Or(0) != sizeof(uint64_t))
					{
						ContentError = ContentException("cannot write content path length: " + PhysicalPath);
						return false;
					}
					else if (!VirtualPath.empty() && Stream->Write((uint8_t*)VirtualPath.c_str(), sizeof(char) * VirtualPath.size()).Or(0) != VirtualPath.size())
					{
						ContentError = ContentException("cannot write content path data: " + PhysicalPath);
						return false;
					}

					ContentFiles.push_back(std::make_pair(*File, ContentSize));
				}
				return true;
			});

			if (!ContentError)
				return ContentError;

			for (auto& Item : ContentFiles)
			{
				size_t DataLength = Item.second;
				while (DataLength > 0)
				{
					uint8_t DataBuffer[Core::CHUNK_SIZE];
					size_t DataSize = std::min<size_t>(sizeof(DataBuffer), DataLength);
					size_t ValueSize = Item.first->Read(DataBuffer, DataSize).Or(0);
					DataLength -= ValueSize;
					if (!ValueSize)
						break;

					if (Stream->Write(DataBuffer, ValueSize).Or(0) != ValueSize)
						return ContentException("cannot write content data: " + Core::String(Item.first->VirtualName()) + " (chunk = " + Core::ToString(CalculatedChunk) + ")");

					CalculatedHash.append((char*)DataBuffer, ValueSize);
					CalculatedHash = *Compute::Crypto::HashRaw(Compute::Digests::SHA256(), CalculatedHash);
					if (ValueSize < DataSize)
						break;
				}

				if (DataLength > 0)
					return ContentException("cannot read content data: " + Core::String(Item.first->VirtualName()) + " (chunk = " + Core::ToString(CalculatedChunk) + ")");

				++CalculatedChunk;
			}

			CalculatedHash = Compute::Codec::HexEncode(CalculatedHash);
			if (Stream->Write((uint8_t*)CalculatedHash.data(), CalculatedHash.size()).Or(0) != CalculatedHash.size())
				return ContentException("cannot write archive checksum: " + Core::String(Path) + " (calculated = " + CalculatedHash + ")");

			return Core::Expectation::Met;
		}
		ExpectsContent<void*> ContentManager::LoadFromArchive(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Map)
		{
			Core::String File(Path);
			Core::Stringify::Replace(File, '\\', '/');
			Core::Stringify::Replace(File, "./", "");

			Core::UMutex<std::mutex> Unique(Exclusive);
			auto Archive = Archives.find(File);
			if (Archive == Archives.end() || !Archive->second || !Archive->second->Stream)
			{
				VI_TRACE("[content] archive was not found: %.*s", (int)Path.size(), Path.data());
				return ContentException("archive was not found: " + Core::String(Path));
			}

			Unique.Negate();
			{
				AssetCache* Asset = FindCache(Processor, File);
				if (Asset != nullptr)
				{
					VI_TRACE("[content] load archived %.*s: cached", (int)Path.size(), Path.data());
					return Processor->Duplicate(Asset, Map);
				}
			}
			Unique.Negate();

			auto It = Streams.find(Archive->second->Stream);
			if (It == Streams.end())
				return ContentException("archived content does not contain: " + Core::String(Path));

			auto* Stream = Archive->second->Stream;
			Stream->SetVirtualName(File);
			Stream->SetVirtualSize(Archive->second->Length);
			Stream->Seek(Core::FileSeek::Begin, It->second + Archive->second->Offset);
			Unique.Negate();

			VI_TRACE("[content] load archived: %.*s", (int)Path.size(), Path.data());
			return Processor->Deserialize(Stream, It->second + Archive->second->Offset, Map);
		}
		ExpectsContent<void*> ContentManager::Load(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Map)
		{
			if (Path.empty())
			{
				VI_TRACE("[content] load from archive: no path provided");
				return ContentException("content path is empty");
			}

			if (!Processor)
				return ContentException("content processor was not found: " + Core::String(Path));

			auto Object = LoadFromArchive(Processor, Path, Map);
			if (Object && *Object != nullptr)
			{
				VI_TRACE("[content] load from archive %.*s: OK", (int)Path.size(), Path.data());
				return Object;
			}

			Core::String File = Core::String(Path);
			if (!Core::OS::Path::IsRemote(File.c_str()))
			{
				auto Subfile = Core::OS::Path::Resolve(File, Environment, true);
				if (Subfile && Core::OS::File::IsExists(Subfile->c_str()))
				{
					File = *Subfile;
					auto Subtarget = Environment + File;
					auto Subpath = Core::OS::Path::Resolve(Subtarget.c_str());
					if (Subpath && Core::OS::File::IsExists(Subpath->c_str()))
						File = *Subpath;
				}

				if (File.empty())
					return ContentException("content was not found: " + Core::String(Path));
			}

			AssetCache* Asset = FindCache(Processor, File);
			if (Asset != nullptr)
			{
				VI_TRACE("[content] load from archive %.*s: cached", (int)Path.size(), Path.data());
				return Processor->Duplicate(Asset, Map);
			}

			Core::UPtr<Core::Stream> Stream = Core::OS::File::Open(File, Core::FileMode::Binary_Read_Only).Or(nullptr);
			if (!Stream)
			{
				VI_TRACE("[content] load from archive %.*s: non-existant", (int)Path.size(), Path.data());
				return ContentException("content was not found: " + Core::String(Path));
			}

			Object = Processor->Deserialize(*Stream, 0, Map);
			VI_TRACE("[content] load from archive %.*s: %s", (int)Path.size(), Path.data(), Object ? "OK" : Object.Error().what());
			return Object;
		}
		ExpectsContent<void> ContentManager::Save(Processor* Processor, const std::string_view& Path, void* Object, const Core::VariantArgs& Map)
		{
			VI_ASSERT(Object != nullptr, "object should be set");
			if (Path.empty())
			{
				VI_TRACE("[content] save forward: no path provided");
				return ContentException("content path is empty");
			}

			if (!Processor)
				return ContentException("content processor was not found: " + Core::String(Path));

			Core::String Directory = Core::OS::Path::GetDirectory(Path);
			auto File = Core::OS::Path::Resolve(Directory, Environment, true);
			if (!File)
				return ContentException("cannot resolve saving path: " + Core::String(Path));

			Core::String Target = *File;
			Target.append(Path.substr(Directory.size()));

			if (!Target.empty())
				Core::OS::Directory::Patch(Core::OS::Path::GetDirectory(Target.c_str()));
			else
				Core::OS::Directory::Patch(Core::OS::Path::GetDirectory(Path));

			Core::UPtr<Core::Stream> Stream = Core::OS::File::Open(Target, Core::FileMode::Binary_Write_Only).Or(nullptr);
			if (!Stream)
			{
				Stream = Core::OS::File::Open(Path, Core::FileMode::Binary_Write_Only).Or(nullptr);
				if (!Stream)
					return ContentException("cannot open saving stream: " + Core::String(Path) + " or " + Target);
			}

			auto Result = Processor->Serialize(*Stream, Object, Map);
			VI_TRACE("[content] save forward %.*s: %s", (int)Path.size(), Path.data(), Result ? "OK" : Result.Error().what());
			return Result;
		}
		ExpectsPromiseContent<void*> ContentManager::LoadDeferred(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys)
		{
			Enqueue();
			Core::String TargetPath = Core::String(Path);
			return Core::Cotask<ExpectsContent<void*>>([this, Processor, TargetPath, Keys]()
			{
				auto Result = Load(Processor, TargetPath, Keys);
				Dequeue();
				return Result;
			});
		}
		ExpectsPromiseContent<void> ContentManager::SaveDeferred(Processor* Processor, const std::string_view& Path, void* Object, const Core::VariantArgs& Keys)
		{
			Enqueue();
			Core::String TargetPath = Core::String(Path);
			return Core::Cotask<ExpectsContent<void>>([this, Processor, TargetPath, Object, Keys]()
			{
				auto Result = Save(Processor, TargetPath, Object, Keys);
				Dequeue();
				return Result;
			});
		}
		Processor* ContentManager::AddProcessor(Processor* Value, uint64_t Id)
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Processors.find(Id);
			if (It != Processors.end())
			{
				Core::Memory::Release(It->second);
				It->second = Value;
			}
			else
				Processors[Id] = Value;

			return Value;
		}
		Processor* ContentManager::GetProcessor(uint64_t Id)
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Processors.find(Id);
			if (It != Processors.end())
				return It->second;

			return nullptr;
		}
		bool ContentManager::RemoveProcessor(uint64_t Id)
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Processors.find(Id);
			if (It == Processors.end())
				return false;

			Core::Memory::Release(It->second);
			Processors.erase(It);
			return true;
		}
		void* ContentManager::TryToCache(Processor* Root, const std::string_view& Path, void* Resource)
		{
			VI_TRACE("[content] save 0x%" PRIXPTR " to cache", Resource);
			Core::String Target = Core::String(Path);
			Core::Stringify::Replace(Target, '\\', '/');
			Core::Stringify::Replace(Target, Environment, "./");		
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto& Entries = Assets[Target];
			auto& Entry = Entries[Root];
			if (Entry != nullptr)
				return Entry->Resource;

			AssetCache* Asset = Core::Memory::New<AssetCache>();
			Asset->Path = Target;
			Asset->Resource = Resource;
			Entry = Asset;
			return nullptr;
		}
		bool ContentManager::IsBusy()
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			return Queue > 0;
		}
		void ContentManager::Enqueue()
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			++Queue;
		}
		void ContentManager::Dequeue()
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			--Queue;
		}
		const Core::UnorderedMap<uint64_t, Processor*>& ContentManager::GetProcessors() const
		{
			return Processors;
		}
		AssetCache* ContentManager::FindCache(Processor* Target, const std::string_view& Path)
		{
			if (Path.empty())
				return nullptr;

			Core::String RelPath = Core::String(Path);
			Core::Stringify::Replace(RelPath, '\\', '/');
			Core::Stringify::Replace(RelPath, Environment, "./");
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Assets.find(RelPath);
			if (It != Assets.end())
			{
				auto KIt = It->second.find(Target);
				if (KIt != It->second.end())
					return KIt->second;
			}

			return nullptr;
		}
		AssetCache* ContentManager::FindCache(Processor* Target, void* Resource)
		{
			if (!Resource)
				return nullptr;

			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto& It : Assets)
			{
				auto KIt = It.second.find(Target);
				if (KIt == It.second.end())
					continue;

				if (KIt->second && KIt->second->Resource == Resource)
					return KIt->second;
			}

			return nullptr;
		}
		const Core::String& ContentManager::GetEnvironment() const
		{
			return Environment;
		}

		AppData::AppData(ContentManager* Manager, const std::string_view& NewPath) noexcept : Content(Manager), Data(nullptr)
		{
			VI_ASSERT(Manager != nullptr, "content manager should be set");
			Migrate(NewPath);
		}
		AppData::~AppData() noexcept
		{
			Core::Memory::Release(Data);
		}
		void AppData::Migrate(const std::string_view& Next)
		{
			VI_ASSERT(!Next.empty(), "path should not be empty");
			VI_TRACE("[appd] migrate %s to %.*s", Path.c_str(), (int)Next.size(), Next.data());

			Core::UMutex<std::mutex> Unique(Exclusive);
			if (Data != nullptr)
			{
				if (!Path.empty())
					Core::OS::File::Remove(Path.c_str());
				WriteAppData(Next);
			}
			else
				ReadAppData(Next);
			Path = Next;
		}
		void AppData::SetKey(const std::string_view& Name, Core::Schema* Value)
		{
			VI_TRACE("[appd] apply %.*s = %s", (int)Name.size(), Name.data(), Value ? Core::Schema::ToJSON(Value).c_str() : "NULL");
			Core::UMutex<std::mutex> Unique(Exclusive);
			if (!Data)
				Data = Core::Var::Set::Object();
			Data->Set(Name, Value);
			WriteAppData(Path);
		}
		void AppData::SetText(const std::string_view& Name, const std::string_view& Value)
		{
			SetKey(Name, Core::Var::Set::String(Value));
		}
		Core::Schema* AppData::GetKey(const std::string_view& Name)
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			if (!ReadAppData(Path))
				return nullptr;

			Core::Schema* Result = Data->Get(Name);
			if (Result != nullptr)
				Result = Result->Copy();
			return Result;
		}
		Core::String AppData::GetText(const std::string_view& Name)
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			if (!ReadAppData(Path))
				return Core::String();

			return Data->GetVar(Name).GetBlob();
		}
		bool AppData::Has(const std::string_view& Name)
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			if (!ReadAppData(Path))
				return false;

			return Data->Has(Name);
		}
		bool AppData::ReadAppData(const std::string_view& Next)
		{
			if (Data != nullptr)
				return true;

			if (Next.empty())
				return false;

			Data = Content->Load<Core::Schema>(Next).Or(nullptr);
			return Data != nullptr;
		}
		bool AppData::WriteAppData(const std::string_view& Next)
		{
			if (Next.empty() || !Data)
				return false;

			Core::String Type = "JSONB";
			auto TypeId = Core::OS::Path::GetExtension(Next);
			if (!TypeId.empty())
			{
				Type.assign(TypeId);
				Core::Stringify::ToUpper(Type);
				if (Type != "JSON" && Type != "JSONB" && Type != "XML")
					Type = "JSONB";
			}

			Core::VariantArgs Args;
			Args["type"] = Core::Var::String(Type);

			return !!Content->Save<Core::Schema>(Next, Data, Args);
		}
		Core::Schema* AppData::GetSnapshot() const
		{
			return Data;
		}

		Application::Application(Desc* I) noexcept : Control(I ? *I : Desc())
		{
			VI_ASSERT(I != nullptr, "desc should be set");
			State = ApplicationState::Staging;
		}
		Application::~Application() noexcept
		{
			Core::Memory::Release(VM);
			Core::Memory::Release(Content);
			Core::Memory::Release(Clock);
			Application::UnlinkInstance(this);
			Vitex::Runtime::CleanupInstances();
		}
		Core::Promise<void> Application::Startup()
		{
			return Core::Promise<void>::Null();
		}
		Core::Promise<void> Application::Shutdown()
		{
			return Core::Promise<void>::Null();
		}
		void Application::ScriptHook()
		{
		}
		void Application::Composition()
		{
		}
		void Application::Dispatch(Core::Timer* Time)
		{
		}
		void Application::Publish(Core::Timer* Time)
		{
		}
		void Application::Initialize()
		{
		}
		void Application::LoopTrigger()
		{
			VI_MEASURE(Core::Timings::Infinite);
			Core::Schedule::Desc& Policy = Control.Scheduler;
			Core::Schedule* Queue = Core::Schedule::Get();
			if (Policy.Parallel)
			{
				while (State == ApplicationState::Active)
				{
					Clock->Begin();
					Dispatch(Clock);

					Clock->Finish();
					Publish(Clock);
				}

				while (Content && Content->IsBusy())
					std::this_thread::sleep_for(std::chrono::milliseconds(CONTENT_BLOCKED_WAIT_MS));
			}
			else
			{
				while (State == ApplicationState::Active)
				{
					Queue->Dispatch();
					Clock->Begin();
					Dispatch(Clock);

					Clock->Finish();
					Publish(Clock);
				}
			}
		}
		int Application::Start()
		{
			Composition();
			if (Control.Usage & (size_t)USE_PROCESSING)
			{
				if (!Content)
					Content = new ContentManager();

				Content->AddProcessor<Processors::AssetProcessor, AssetFile>();
				Content->AddProcessor<Processors::SchemaProcessor, Core::Schema>();
				Content->AddProcessor<Processors::ServerProcessor, Network::HTTP::Server>();
				if (Control.Environment.empty())
				{
					auto Directory = Core::OS::Directory::GetWorking();
					if (Directory)
						Content->SetEnvironment(*Directory + Control.Directory);
				}
				else
					Content->SetEnvironment(Control.Environment + Control.Directory);
				
				if (!Control.Preferences.empty() && !Database)
				{
					auto Path = Core::OS::Path::Resolve(Control.Preferences, Content->GetEnvironment(), true);
					Database = new AppData(Content, Path ? *Path : Control.Preferences);
				}
			}

			if (Control.Usage & (size_t)USE_SCRIPTING && !VM)
				VM = new Scripting::VirtualMachine();

			Clock = new Core::Timer();
			Clock->SetFixedFrames(Control.Refreshrate.Stable);
			Clock->SetMaxFrames(Control.Refreshrate.Limit);

			if (Control.Usage & (size_t)USE_NETWORKING)
			{
				if (Network::Multiplexer::HasInstance())
					Network::Multiplexer::Get()->Rescale(Control.PollingTimeout, Control.PollingEvents);
				else
					new Network::Multiplexer(Control.PollingTimeout, Control.PollingEvents);
			}

			if (Control.Usage & (size_t)USE_SCRIPTING)
				ScriptHook();

			Initialize();
			if (State == ApplicationState::Terminated)
				return ExitCode != 0 ? ExitCode : EXIT_JUMP + 6;

			State = ApplicationState::Active;
			Core::Schedule::Desc& Policy = Control.Scheduler;
			Policy.Initialize = [this](Core::TaskCallback&& Callback) { this->Startup().When(std::move(Callback)); };
			Policy.Ping = Control.Daemon ? std::bind(&Application::Status, this) : (Core::ActivityCallback)nullptr;

			if (Control.Threads > 0)
			{
				Core::Schedule::Desc Launch = Core::Schedule::Desc(Control.Threads);
				memcpy(Policy.Threads, Launch.Threads, sizeof(Policy.Threads));
			}

			auto* Queue = Core::Schedule::Get();
			Queue->Start(Policy);
			Clock->Reset();
			LoopTrigger();
			Shutdown().Wait();
			Queue->Stop();

			ExitCode = (State == ApplicationState::Restart ? EXIT_RESTART : ExitCode);
			State = ApplicationState::Terminated;
			return ExitCode;
		}
		void Application::Stop(int Code)
		{
			Core::Schedule* Queue = Core::Schedule::Get();
			State = ApplicationState::Terminated;
			ExitCode = Code;
			Queue->Wakeup();
		}
		void Application::Restart()
		{
			Core::Schedule* Queue = Core::Schedule::Get();
			State = ApplicationState::Restart;
			Queue->Wakeup();
		}
		ApplicationState Application::GetState() const
		{
			return State;
		}
		bool Application::Status(Application* App)
		{
			return App->State == ApplicationState::Active;
		}
	}
}
