#ifndef VI_ENGINE_PROCESSORS_H
#define VI_ENGINE_PROCESSORS_H
#include "../core/engine.h"

namespace Vitex
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
				Core::UnorderedMap<size_t, size_t> JointIndices;
				Core::Vector<Compute::SkinVertex> Vertices;
				Core::Vector<int32_t> Indices;
				Core::String Name;
				Compute::Matrix4x4 Transform;
				size_t LocalIndex = 0;
			};

			struct ModelInfo
			{
				Core::UnorderedMap<Core::String, MeshJoint> JointOffsets;
				Core::Vector<MeshBlob> Meshes;
				Compute::Matrix4x4 Transform;
				Compute::Joint Skeleton;
				Compute::Vector3 Min, Max;
				float Low = 0.0f, High = 0.0f;
				size_t GlobalIndex = 0;
			};

			struct ModelChannel
			{
				Core::UnorderedMap<float, Compute::Vector3> Positions;
				Core::UnorderedMap<float, Compute::Vector3> Scales;
				Core::UnorderedMap<float, Compute::Quaternion> Rotations;
			};

			class VI_OUT AssetProcessor final : public Processor
			{
			public:
				AssetProcessor(ContentManager * Manager);
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class VI_OUT MaterialProcessor final : public Processor
			{
			public:
				MaterialProcessor(ContentManager * Manager);
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				ExpectsContent<void> Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;
			};

			class VI_OUT SceneGraphProcessor final : public Processor
			{
			public:
				SceneGraphProcessor(ContentManager * Manager);
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				ExpectsContent<void> Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args) override;
			};

			class VI_OUT AudioClipProcessor final : public Processor
			{
			public:
				AudioClipProcessor(ContentManager * Manager);
				~AudioClipProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> DeserializeWAVE(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args);
				ExpectsContent<Core::Unique<void>> DeserializeOGG(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args);
				void Free(AssetCache* Asset) override;
			};

			class VI_OUT Texture2DProcessor final : public Processor
			{
			public:
				Texture2DProcessor(ContentManager * Manager);
				~Texture2DProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;
			};

			class VI_OUT ShaderProcessor final : public Processor
			{
			public:
				ShaderProcessor(ContentManager * Manager);
				~ShaderProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;
			};

			class VI_OUT ModelProcessor final : public Processor
			{
			public:
				Graphics::MeshBuffer::Desc Options;

			public:
				ModelProcessor(ContentManager* Manager);
				~ModelProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;

			public:
				static ExpectsContent<Core::Unique<Core::Schema>> Import(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
				static ExpectsContent<ModelInfo> ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
			};

			class VI_OUT SkinModelProcessor final : public Processor
			{
			public:
				Graphics::SkinMeshBuffer::Desc Options;

			public:
				SkinModelProcessor(ContentManager* Manager);
				~SkinModelProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;
			};

			class VI_OUT SkinAnimationProcessor final : public Processor
			{
			public:
				SkinAnimationProcessor(ContentManager* Manager);
				~SkinAnimationProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;

			public:
				static ExpectsContent<Core::Schema*> Import(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
				static ExpectsContent<Core::Vector<Compute::SkinAnimatorClip>> ImportForImmediateUse(Core::Stream* Stream, uint64_t Opts = (uint64_t)MeshPreset::Default);
			};

			class VI_OUT SchemaProcessor final : public Processor
			{
			public:
				SchemaProcessor(ContentManager* Manager);
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				ExpectsContent<void> Serialize(Core::Stream* Stream, void* Object, const Core::VariantArgs& Args) override;
			};

			class VI_OUT ServerProcessor final : public Processor
			{
			public:
				std::function<void(void*, Core::Schema*)> Callback;

			public:
				ServerProcessor(ContentManager* Manager);
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
			};

			class VI_OUT HullShapeProcessor final : public Processor
			{
			public:
				HullShapeProcessor(ContentManager * Manager);
				~HullShapeProcessor() override;
				ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Args) override;
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
				void Free(AssetCache* Asset) override;
			};
		}
	}
}
#endif