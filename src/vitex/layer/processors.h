#ifndef VI_LAYER_PROCESSORS_H
#define VI_LAYER_PROCESSORS_H
#include "../layer.h"

namespace vitex
{
	namespace layer
	{
		namespace processors
		{
			class asset_processor final : public processor
			{
			public:
				asset_processor(content_manager* manager);
				expects_content<void*> deserialize(core::stream* stream, size_t offset, const core::variant_args& args) override;
			};

			class schema_processor final : public processor
			{
			public:
				schema_processor(content_manager* manager);
				expects_content<void*> deserialize(core::stream* stream, size_t offset, const core::variant_args& args) override;
				expects_content<void> serialize(core::stream* stream, void* object, const core::variant_args& args) override;
			};

			class server_processor final : public processor
			{
			public:
				std::function<void(void*, core::schema*)> callback;

			public:
				server_processor(content_manager* manager);
				expects_content<void*> deserialize(core::stream* stream, size_t offset, const core::variant_args& args) override;
			};
		}
	}
}
#endif