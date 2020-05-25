#ifndef THAWK_ENGINE_PROCESSORS_H
#define THAWK_ENGINE_PROCESSORS_H

#include "../engine.h"

namespace Tomahawk
{
    namespace Engine
    {
        namespace FileProcessors
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

            struct THAWK_OUT MeshBlob
            {
                std::vector<Compute::InfluenceVertex> Vertices;
                std::vector<int> Indices;
                std::string Name;
                Compute::Matrix4x4 World;
            };

            struct THAWK_OUT MeshInfo
            {
                std::vector<std::pair<Int64, Compute::Joint>> Joints;
                std::vector<MeshBlob> Meshes;
                float PX = 0, PY = 0, PZ = 0;
                float NX = 0, NY = 0, NZ = 0;
                unsigned int Weights = 0;
                bool Flip, Invert;
            };

            struct THAWK_OUT MeshNode
            {
                Compute::Matrix4x4 Transform;
                Int64 Index;
            };

            class THAWK_OUT SceneGraphProcessor : public FileProcessor
            {
            public:
                void (* OnComponentCreation)(Entity*, Component**, UInt64);
                void (* OnRendererCreation)(Renderer**, UInt64);

            public:
                SceneGraphProcessor(ContentManager* Manager);
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
                bool Save(Rest::FileStream* Stream, void* Object, ContentArgs* Args) override;
            };

            class THAWK_OUT FontProcessor : public FileProcessor
            {
            public:
                FontProcessor(ContentManager* Manager);
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
            };

            class THAWK_OUT AudioClipProcessor : public FileProcessor
            {
            public:
                AudioClipProcessor(ContentManager* Manager);
                virtual ~AudioClipProcessor() override;
                void Free(AssetResource* Asset) override;
                void* Duplicate(AssetResource* Asset, ContentArgs* Args) override;
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
				void* LoadWAVE(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args);
				void* LoadOGG(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args);
            };

            class THAWK_OUT Texture2DProcessor : public FileProcessor
            {
            public:
                Texture2DProcessor(ContentManager* Manager);
                virtual ~Texture2DProcessor() override;
                void Free(AssetResource* Asset) override;
                void* Duplicate(AssetResource* Asset, ContentArgs* Args) override;
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
            };

            class THAWK_OUT ShaderProcessor : public FileProcessor
            {
            public:
                ShaderProcessor(ContentManager* Manager);
                virtual ~ShaderProcessor() override;
                void Free(AssetResource* Asset) override;
                void* Duplicate(AssetResource* Asset, ContentArgs* Args) override;
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
            };

            class THAWK_OUT ModelProcessor : public FileProcessor
            {
            public:
                Graphics::Mesh::Desc Options;

            public:
                ModelProcessor(ContentManager* Manager);
                virtual ~ModelProcessor() override;
                void Free(AssetResource* Asset) override;
                void* Duplicate(AssetResource* Asset, ContentArgs* Args) override;
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;

            public:
                static Rest::Document* Import(const std::string& Path, unsigned int Opts = MeshOpt_CalcTangentSpace | MeshOpt_GenSmoothNormals | MeshOpt_JoinIdenticalVertices | MeshOpt_ImproveCacheLocality | MeshOpt_LimitBoneWeights | MeshOpt_RemoveRedundantMaterials | MeshOpt_SplitLargeMeshes | MeshOpt_Triangulate | MeshOpt_GenUVCoords | MeshOpt_SortByPType | MeshOpt_RemoveDegenerates | MeshOpt_RemoveInvalidData | MeshOpt_RemoveInstances | MeshOpt_ValidateDataStructure | MeshOpt_OptimizeMeshes | MeshOpt_TransformUVCoords | 0);

            private:
                static void ProcessNode(void* Scene, void* Node, MeshInfo* Info, const Compute::Matrix4x4& Global);
                static void ProcessMesh(void* Scene, void* Mesh, MeshInfo* Info, const Compute::Matrix4x4& Global);
                static void ProcessHeirarchy(void* Scene, void* Node, MeshInfo* Info, Compute::Joint* Parent);
                static std::vector<std::pair<Int64, Compute::Joint>>::iterator FindJoint(std::vector<std::pair<Int64, Compute::Joint>>& Joints, const std::string& Name);
            };

            class THAWK_OUT SkinnedModelProcessor : public FileProcessor
            {
            public:
                Graphics::SkinnedMesh::Desc Options;

            public:
                SkinnedModelProcessor(ContentManager* Manager);
                virtual ~SkinnedModelProcessor() override;
                void Free(AssetResource* Asset) override;
                void* Duplicate(AssetResource* Asset, ContentArgs* Args) override;
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;

            public:
                static std::vector<Compute::SkinAnimatorClip> ImportAnimation(const std::string& Path, unsigned int Opts = MeshOpt_CalcTangentSpace | MeshOpt_GenSmoothNormals | MeshOpt_JoinIdenticalVertices | MeshOpt_ImproveCacheLocality | MeshOpt_LimitBoneWeights | MeshOpt_RemoveRedundantMaterials | MeshOpt_SplitLargeMeshes | MeshOpt_Triangulate | MeshOpt_GenUVCoords | MeshOpt_SortByPType | MeshOpt_RemoveDegenerates | MeshOpt_RemoveInvalidData | MeshOpt_RemoveInstances | MeshOpt_ValidateDataStructure | MeshOpt_OptimizeMeshes | MeshOpt_TransformUVCoords | 0);

            private:
                static void ProcessNode(void* Scene, void* Node, std::unordered_map<std::string, MeshNode>* Joints, Int64& Index);
                static void ProcessHeirarchy(void* Scene, void* Node, std::unordered_map<std::string, MeshNode>* Joints);
                static void ProcessKeys(std::vector<Compute::AnimatorKey>* Keys, std::unordered_map<std::string, MeshNode>* Joints);
            };

            class THAWK_OUT DocumentProcessor : public FileProcessor
            {
            public:
                DocumentProcessor(ContentManager* Manager);
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
                bool Save(Rest::FileStream* Stream, void* Object, ContentArgs* Args) override;
            };

            class THAWK_OUT ServerProcessor : public FileProcessor
            {
            public:
                std::function<void(void*, Rest::Document*)> Callback;

            public:
                ServerProcessor(ContentManager* Manager);
                void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Args) override;
            };
        }
    }
}
#endif