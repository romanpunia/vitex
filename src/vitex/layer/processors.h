#ifndef VI_LAYER_PROCESSORS_H
#define VI_LAYER_PROCESSORS_H
#include "../layer.h"

namespace Vitex
{
	namespace Layer
	{
		namespace Processors
		{
			class VI_OUT AssetProcessor final : public Processor
			{
			public:
				AssetProcessor(ContentManager * Manager);
				ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Args) override;
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
		}
	}
}
#endif