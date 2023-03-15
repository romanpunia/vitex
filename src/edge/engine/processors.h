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

			constexpr inline MeshOpt operator |(MeshOpt A, MeshOpt B)
			{
				return static_cast<MeshOpt>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
			}

			enum class MeshPreset : uint64_t
			{
				Default = (uint64_t)(MeshOpt::FlipWindingOrder | MeshOpt::Triangulate | MeshOpt::CalcTangentSpace | MeshOpt::ValidateDataStructure)
			};

			struct MeshBone
			{
				size_t Index;
				Compute::AnimatorKey Default;
			};

			struct MeshJoint
			{
				Compute::Matrix4x4 Local;
				size_t Index;
				bool Linking;
			};

			struct MeshBlob
			{
				std::unordered_map<size_t, size_t> JointIndices;
				std::vector<Compute::SkinVertex> Vertices;
				std::vector<int32_t> Indices;
				std::string Name;
				Compute::Matrix4x4 Transform;
				size_t LocalIndex = 0;
			};

			struct ModelInfo
			{
				std::unordered_map<std::string, MeshJoint> JointOffsets;
				std::vector<MeshBlob> Meshes;
				Compute::Matrix4x4 Transform;
				Compute::Joint Skeleton;
				Compute::Vector3 Min, Max;
				float Low = 0.0f, High = 0.0f;
				size_t GlobalIndex = 0;
			};

			struct ModelChannel
			{
				std::unordered_map<float, Compute::Vector3> Positions;
				std::unordered_map<float, Compute::Vector3> Scales;
				std::unordered_map<float, Compute::Quaternion> Rotations;
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
				~AudioClip() override;
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
				~Texture2D() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class ED_OUT Shader final : public Processor
			{
			public:
				Shader(ContentManager * Manager);
				~Shader() override;
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
				~Model() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;

			public:
				static Core::Unique<Core::Schema> Import(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
				static ModelInfo ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
			};

			class ED_OUT SkinModel final : public Processor
			{
			public:
				Graphics::SkinMeshBuffer::Desc Options;

			public:
				SkinModel(ContentManager* Manager);
				~SkinModel() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class ED_OUT SkinAnimation final : public Processor
			{
			public:
				SkinAnimation(ContentManager* Manager);
				~SkinAnimation() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;

			public:
				static Core::Schema* Import(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
				static std::vector<Compute::SkinAnimatorClip> ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
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
				~HullShape() override;
				void Free(AssetCache* Asset) override;
				Core::Unique<void> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				Core::Unique<void> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};
		}
	}
}
#endif