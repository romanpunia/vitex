#ifndef ED_ENGINE_PROCESSORS_H
#define ED_ENGINE_PROCESSORS_H
#include "../core/engine.h"

namespace Edge
{
	namespace Engine
	{
		namespace Processors
		{
			enum class MeshOpt : uint64_t
			{
				CalcTangentSpace = 0x1,
				JoinIdenticalVertices = 0x2,
				MakeLeftHanded = 0x4,
				Triangulate = 0x8,
				RemoveComponent = 0x10,
				GenNormals = 0x20,
				GenSmoothNormals = 0x40,
				SplitLargeMeshes = 0x80,
				PreTransformVertices = 0x100,
				LimitBoneWeights = 0x200,
				ValidateDataStructure = 0x400,
				ImproveCacheLocality = 0x800,
				RemoveRedundantMaterials = 0x1000,
				FixInfacingNormals = 0x2000,
				SortByPType = 0x8000,
				RemoveDegenerates = 0x10000,
				RemoveInvalidData = 0x20000,
				RemoveInstances = 0x100000,
				GenUVCoords = 0x40000,
				TransformUVCoords = 0x80000,
				OptimizeMeshes = 0x200000,
				OptimizeGraph = 0x400000,
				FlipUVs = 0x800000,
				FlipWindingOrder = 0x1000000,
				SplitByBoneCount = 0x2000000,
				Debone = 0x4000000,
				GlobalScale = 0x8000000,
				EmbedTextures = 0x10000000,
				ForceGenNormals = 0x20000000,
				DropNormals = 0x40000000,
				GenBoundingBoxes = 0x80000000l
			};

			inline MeshOpt operator |(MeshOpt A, MeshOpt B)
			{
				return static_cast<MeshOpt>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
			}

			struct ED_OUT MeshBlob
			{
				std::vector<Compute::SkinVertex> Vertices;
				std::vector<int> Indices;
				std::string Name;
				Compute::Matrix4x4 World;
			};

			struct ED_OUT MeshInfo
			{
				std::vector<std::pair<int64_t, Compute::Joint>> Joints;
				std::vector<MeshBlob> Meshes;
				float PX = 0, PY = 0, PZ = 0;
				float NX = 0, NY = 0, NZ = 0;
				unsigned int Weights = 0;
				bool Flip = false, Inv = false;
			};

			struct ED_OUT MeshNode
			{
				Compute::Matrix4x4 Transform;
				int64_t Index = -1;
			};

			class ED_OUT Asset final : public Processor
			{
			public:
				Asset(ContentManager * Manager);
				void* Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class ED_OUT Material final : public Processor
			{
			public:
				Material(ContentManager * Manager);
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				bool Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args) override;
			};

			class ED_OUT SceneGraph final : public Processor
			{
			public:
				SceneGraph(ContentManager * Manager);
				void* Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				bool Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args) override;
			};

			class ED_OUT AudioClip final : public Processor
			{
			public:
				AudioClip(ContentManager * Manager);
				virtual ~AudioClip() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				Core::Unique<void> DeserializeWAVE(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args);
				Core::Unique<void> DeserializeOGG(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args);
			};

			class ED_OUT Texture2D final : public Processor
			{
			public:
				Texture2D(ContentManager * Manager);
				virtual ~Texture2D() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class ED_OUT Shader final : public Processor
			{
			public:
				Shader(ContentManager * Manager);
				virtual ~Shader() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class ED_OUT Model final : public Processor
			{
			public:
				Graphics::MeshBuffer::Desc Options;

			public:
				Model(ContentManager* Manager);
				virtual ~Model() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;

			public:
				static Core::Unique<Core::Schema> Import(const std::string& Path, uint64_t Opts = (uint64_t)(MeshOpt::CalcTangentSpace | MeshOpt::GenSmoothNormals | MeshOpt::JoinIdenticalVertices | MeshOpt::ImproveCacheLocality | MeshOpt::LimitBoneWeights | MeshOpt::RemoveRedundantMaterials | MeshOpt::SplitLargeMeshes | MeshOpt::Triangulate | MeshOpt::GenUVCoords | MeshOpt::SortByPType | MeshOpt::RemoveDegenerates | MeshOpt::RemoveInvalidData | MeshOpt::RemoveInstances | MeshOpt::ValidateDataStructure | MeshOpt::OptimizeMeshes | MeshOpt::TransformUVCoords));

			private:
				static void ProcessNode(void* Scene, void* Node, MeshInfo* Info, const Compute::Matrix4x4& Global);
				static void ProcessMesh(void* Scene, void* Mesh, MeshInfo* Info, const Compute::Matrix4x4& Global);
				static void ProcessHeirarchy(void* Scene, void* Node, MeshInfo* Info, Compute::Joint* Parent);
				static std::vector<std::pair<int64_t, Compute::Joint>>::iterator FindJoint(std::vector<std::pair<int64_t, Compute::Joint>>& Joints, const std::string& Name);
			};

			class ED_OUT SkinModel final : public Processor
			{
			public:
				Graphics::SkinMeshBuffer::Desc Options;

			public:
				SkinModel(ContentManager* Manager);
				virtual ~SkinModel() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;

			public:
				static Core::Schema* ImportAnimation(const std::string& Path, uint64_t Opts = (uint64_t)(MeshOpt::CalcTangentSpace | MeshOpt::GenSmoothNormals | MeshOpt::JoinIdenticalVertices | MeshOpt::ImproveCacheLocality | MeshOpt::LimitBoneWeights | MeshOpt::RemoveRedundantMaterials | MeshOpt::SplitLargeMeshes | MeshOpt::Triangulate | MeshOpt::GenUVCoords | MeshOpt::SortByPType | MeshOpt::RemoveDegenerates | MeshOpt::RemoveInvalidData | MeshOpt::RemoveInstances | MeshOpt::ValidateDataStructure | MeshOpt::OptimizeMeshes | MeshOpt::TransformUVCoords));

			private:
				static void ProcessNode(void* Scene, void* Node, std::unordered_map<std::string, MeshNode>* Joints, int64_t& Index);
				static void ProcessHeirarchy(void* Scene, void* Node, std::unordered_map<std::string, MeshNode>* Joints);
				static void ProcessKeys(std::vector<Compute::AnimatorKey>* Keys, std::unordered_map<std::string, MeshNode>* Joints);
			};

			class ED_OUT Schema final : public Processor
			{
			public:
				Schema(ContentManager * Manager);
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				bool Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args) override;
			};

			class ED_OUT Server final : public Processor
			{
			public:
				std::function<void(void*, Core::Schema*)> Callback;

			public:
				Server(ContentManager* Manager);
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class ED_OUT HullShape final : public Processor
			{
			public:
				HullShape(ContentManager * Manager);
				virtual ~HullShape() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};
		}
	}
}
#endif