#ifndef TH_ENGINE_PROCESSORS_H
#define TH_ENGINE_PROCESSORS_H

#include "../core/engine.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Processors
		{
			enum MeshOpt
			{
				MeshOpt_CalcTangentSpace = 0x1,
				MeshOpt_JoinIdenticalVertices = 0x2,
				MeshOpt_MakeLeftHanded = 0x4,
				MeshOpt_Triangulate = 0x8,
				MeshOpt_RemoveComponent = 0x10,
				MeshOpt_GenNormals = 0x20,
				MeshOpt_GenSmoothNormals = 0x40,
				MeshOpt_SplitLargeMeshes = 0x80,
				MeshOpt_PreTransformVertices = 0x100,
				MeshOpt_LimitBoneWeights = 0x200,
				MeshOpt_ValidateDataStructure = 0x400,
				MeshOpt_ImproveCacheLocality = 0x800,
				MeshOpt_RemoveRedundantMaterials = 0x1000,
				MeshOpt_FixInfacingNormals = 0x2000,
				MeshOpt_SortByPType = 0x8000,
				MeshOpt_RemoveDegenerates = 0x10000,
				MeshOpt_RemoveInvalidData = 0x20000,
				MeshOpt_RemoveInstances = 0x100000,
				MeshOpt_GenUVCoords = 0x40000,
				MeshOpt_TransformUVCoords = 0x80000,
				MeshOpt_OptimizeMeshes = 0x200000,
				MeshOpt_OptimizeGraph = 0x400000,
				MeshOpt_FlipUVs = 0x800000,
				MeshOpt_FlipWindingOrder = 0x1000000,
				MeshOpt_SplitByBoneCount = 0x2000000,
				MeshOpt_Debone = 0x4000000,
				MeshOpt_GlobalScale = 0x8000000,
				MeshOpt_EmbedTextures = 0x10000000,
				MeshOpt_ForceGenNormals = 0x20000000,
				MeshOpt_DropNormals = 0x40000000,
				MeshOpt_GenBoundingBoxes = 0x80000000l
			};

			struct TH_OUT MeshBlob
			{
				std::vector<Compute::SkinVertex> Vertices;
				std::vector<int> Indices;
				std::string Name;
				Compute::Matrix4x4 World;
			};

			struct TH_OUT MeshInfo
			{
				std::vector<std::pair<int64_t, Compute::Joint>> Joints;
				std::vector<MeshBlob> Meshes;
				float PX = 0, PY = 0, PZ = 0;
				float NX = 0, NY = 0, NZ = 0;
				unsigned int Weights = 0;
				bool Flip, Invert;
			};

			struct TH_OUT MeshNode
			{
				Compute::Matrix4x4 Transform;
				int64_t Index;
			};

			class TH_OUT Asset : public Processor
			{
			public:
				Asset(ContentManager* Manager);
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
			};

			class TH_OUT SceneGraph : public Processor
			{
			public:
				SceneGraph(ContentManager* Manager);
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
				bool Serialize(Rest::Stream* Stream, void* Object, const Rest::VariantArgs& Args) override;
			};

			class TH_OUT AudioClip : public Processor
			{
			public:
				AudioClip(ContentManager* Manager);
				virtual ~AudioClip() override;
				void Free(AssetCache* Asset) override;
				void* Duplicate(AssetCache* Asset, const Rest::VariantArgs& Args) override;
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
				void* DeserializeWAVE(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args);
				void* DeserializeOGG(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args);
			};

			class TH_OUT Texture2D : public Processor
			{
			public:
				Texture2D(ContentManager* Manager);
				virtual ~Texture2D() override;
				void Free(AssetCache* Asset) override;
				void* Duplicate(AssetCache* Asset, const Rest::VariantArgs& Args) override;
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
			};

			class TH_OUT Shader : public Processor
			{
			public:
				Shader(ContentManager* Manager);
				virtual ~Shader() override;
				void Free(AssetCache* Asset) override;
				void* Duplicate(AssetCache* Asset, const Rest::VariantArgs& Args) override;
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
			};

			class TH_OUT Model : public Processor
			{
			public:
				Graphics::MeshBuffer::Desc Options;

			public:
				Model(ContentManager* Manager);
				virtual ~Model() override;
				void Free(AssetCache* Asset) override;
				void* Duplicate(AssetCache* Asset, const Rest::VariantArgs& Args) override;
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;

			public:
				static Rest::Document* Import(const std::string& Path, unsigned int Opts = MeshOpt_CalcTangentSpace | MeshOpt_GenSmoothNormals | MeshOpt_JoinIdenticalVertices | MeshOpt_ImproveCacheLocality | MeshOpt_LimitBoneWeights | MeshOpt_RemoveRedundantMaterials | MeshOpt_SplitLargeMeshes | MeshOpt_Triangulate | MeshOpt_GenUVCoords | MeshOpt_SortByPType | MeshOpt_RemoveDegenerates | MeshOpt_RemoveInvalidData | MeshOpt_RemoveInstances | MeshOpt_ValidateDataStructure | MeshOpt_OptimizeMeshes | MeshOpt_TransformUVCoords | 0);

			private:
				static void ProcessNode(void* Scene, void* Node, MeshInfo* Info, const Compute::Matrix4x4& Global);
				static void ProcessMesh(void* Scene, void* Mesh, MeshInfo* Info, const Compute::Matrix4x4& Global);
				static void ProcessHeirarchy(void* Scene, void* Node, MeshInfo* Info, Compute::Joint* Parent);
				static std::vector<std::pair<int64_t, Compute::Joint>>::iterator FindJoint(std::vector<std::pair<int64_t, Compute::Joint>>& Joints, const std::string& Name);
			};

			class TH_OUT SkinModel : public Processor
			{
			public:
				Graphics::SkinMeshBuffer::Desc Options;

			public:
				SkinModel(ContentManager* Manager);
				virtual ~SkinModel() override;
				void Free(AssetCache* Asset) override;
				void* Duplicate(AssetCache* Asset, const Rest::VariantArgs& Args) override;
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;

			public:
				static Rest::Document* ImportAnimation(const std::string& Path, unsigned int Opts = MeshOpt_CalcTangentSpace | MeshOpt_GenSmoothNormals | MeshOpt_JoinIdenticalVertices | MeshOpt_ImproveCacheLocality | MeshOpt_LimitBoneWeights | MeshOpt_RemoveRedundantMaterials | MeshOpt_SplitLargeMeshes | MeshOpt_Triangulate | MeshOpt_GenUVCoords | MeshOpt_SortByPType | MeshOpt_RemoveDegenerates | MeshOpt_RemoveInvalidData | MeshOpt_RemoveInstances | MeshOpt_ValidateDataStructure | MeshOpt_OptimizeMeshes | MeshOpt_TransformUVCoords | 0);

			private:
				static void ProcessNode(void* Scene, void* Node, std::unordered_map<std::string, MeshNode>* Joints, int64_t& Index);
				static void ProcessHeirarchy(void* Scene, void* Node, std::unordered_map<std::string, MeshNode>* Joints);
				static void ProcessKeys(std::vector<Compute::AnimatorKey>* Keys, std::unordered_map<std::string, MeshNode>* Joints);
			};

			class TH_OUT Document : public Processor
			{
			public:
				Document(ContentManager* Manager);
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
				bool Serialize(Rest::Stream* Stream, void* Object, const Rest::VariantArgs& Args) override;
			};

			class TH_OUT Server : public Processor
			{
			public:
				std::function<void(void*, Rest::Document*)> Callback;

			public:
				Server(ContentManager* Manager);
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
			};

			class TH_OUT Shape : public Processor
			{
			public:
				Shape(ContentManager* Manager);
				virtual ~Shape() override;
				void Free(AssetCache* Asset) override;
				void* Duplicate(AssetCache* Asset, const Rest::VariantArgs& Args) override;
				void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Rest::VariantArgs& Args) override;
			};
		}
	}
}
#endif