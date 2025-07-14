#include "processors.h"
#include "../network/http.h"

namespace vitex
{
	namespace layer
	{
		namespace processors
		{
			asset_processor::asset_processor(content_manager* manager) : processor(manager)
			{
			}
			expects_content<void*> asset_processor::deserialize(core::stream* stream, size_t offset, const core::variant_args& args)
			{
				VI_ASSERT(stream != nullptr, "stream should be set");

				core::vector<char> temp;
				stream->read_all([&temp](uint8_t* buffer, size_t size)
				{
					temp.reserve(temp.size() + size);
					for (size_t i = 0; i < size; i++)
						temp.push_back(buffer[i]);
				});

				char* data = core::memory::allocate<char>(sizeof(char) * temp.size());
				memcpy(data, temp.data(), sizeof(char) * temp.size());
				return new asset_file(data, temp.size());
			}

			schema_processor::schema_processor(content_manager* manager) : processor(manager)
			{
			}
			expects_content<void*> schema_processor::deserialize(core::stream* stream, size_t offset, const core::variant_args& args)
			{
				VI_ASSERT(stream != nullptr, "stream should be set");
				auto object = core::schema::convert_from_jsonb([stream](uint8_t* buffer, size_t size) { return size > 0 ? stream->read(buffer, size).or_else(0) == size : true; });
				if (object)
					return *object;

				core::string data;
				stream->seek(core::file_seek::begin, offset);
				stream->read_all([&data](uint8_t* buffer, size_t size) { data.append((char*)buffer, size); });

				object = core::schema::convert_from_json(data);
				if (object)
					return *object;

				object = core::schema::convert_from_xml(data);
				if (!object)
					return content_exception(std::move(object.error().message()));

				return *object;
			}
			expects_content<void> schema_processor::serialize(core::stream* stream, void* instance, const core::variant_args& args)
			{
				auto type = args.find("type");
				VI_ASSERT(type != args.end(), "type argument should be set");
				VI_ASSERT(stream != nullptr, "stream should be set");
				VI_ASSERT(instance != nullptr, "instance should be set");

				auto schema = (core::schema*)instance;
				if (type->second == core::var::string("XML"))
				{
					core::string offset;
					core::schema::convert_to_xml(schema, [stream, &offset](core::var_form pretty, const std::string_view& buffer)
					{
						if (!buffer.empty())
							stream->write((uint8_t*)buffer.data(), buffer.size());

						switch (pretty)
						{
							case vitex::core::var_form::tab_decrease:
								offset.erase(offset.end() - 1);
								break;
							case vitex::core::var_form::tab_increase:
								offset.append(1, '\t');
								break;
							case vitex::core::var_form::write_space:
								stream->write((uint8_t*)" ", 1);
								break;
							case vitex::core::var_form::write_line:
								stream->write((uint8_t*)"\n", 1);
								break;
							case vitex::core::var_form::write_tab:
								stream->write((uint8_t*)offset.c_str(), offset.size());
								break;
							default:
								break;
						}
					});
					return core::expectation::met;
				}
				else if (type->second == core::var::string("JSON"))
				{
					core::string offset;
					core::schema::convert_to_json(schema, [stream, &offset](core::var_form pretty, const std::string_view& buffer)
					{
						if (!buffer.empty())
							stream->write((uint8_t*)buffer.data(), buffer.size());

						switch (pretty)
						{
							case vitex::core::var_form::tab_decrease:
								offset.erase(offset.end() - 1);
								break;
							case vitex::core::var_form::tab_increase:
								offset.append(1, '\t');
								break;
							case vitex::core::var_form::write_space:
								stream->write((uint8_t*)" ", 1);
								break;
							case vitex::core::var_form::write_line:
								stream->write((uint8_t*)"\n", 1);
								break;
							case vitex::core::var_form::write_tab:
								stream->write((uint8_t*)offset.c_str(), offset.size());
								break;
							default:
								break;
						}
					});
					return core::expectation::met;
				}
				else if (type->second == core::var::string("JSONB"))
				{
					core::schema::convert_to_jsonb(schema, [stream](core::var_form, const std::string_view& buffer)
					{
						if (!buffer.empty())
							stream->write((uint8_t*)buffer.data(), buffer.size());
					});
					return core::expectation::met;
				}

				return content_exception("save schema: unsupported type");
			}

			server_processor::server_processor(content_manager* manager) : processor(manager)
			{
			}
			expects_content<void*> server_processor::deserialize(core::stream* stream, size_t offset, const core::variant_args& args)
			{
				VI_ASSERT(stream != nullptr, "stream should be set");
				auto blob_status = content->load<core::schema>(stream->virtual_name());
				if (!blob_status)
					return blob_status.error();

				auto net_addresses = network::utils::get_host_ip_addresses();
				core::string base_directory = core::os::path::get_directory(stream->virtual_name());
				core::uptr<network::http::map_router> router = new network::http::map_router();
				core::uptr<network::http::server> object = new network::http::server();
				core::uptr<core::schema> blob = *blob_status;
				if (callback)
					callback((void*)*object, *blob);

				core::vector<core::schema*> certificates = blob->find_collection("certificate", true);
				for (auto&& it : certificates)
				{
					core::string name;
					if (series::unpack(it, &name))
						core::stringify::eval_envs(name, base_directory, net_addresses);
					else
						name = "*";

					network::socket_certificate* cert = &router->certificates[name];
					series::unpack(it->find("ciphers"), &cert->ciphers);
					series::unpack(it->find("verify-peers"), &cert->verify_peers);
					series::unpack(it->find("pkey"), &cert->blob.private_key);
					series::unpack(it->find("cert"), &cert->blob.certificate);
					if (series::unpack(it->find("options"), &name))
					{
						if (name.find("no_ssl_v2") != std::string::npos)
							cert->options = (network::secure_layer_options)((size_t)cert->options & (size_t)network::secure_layer_options::no_sslv2);
						if (name.find("no_ssl_v3") != std::string::npos)
							cert->options = (network::secure_layer_options)((size_t)cert->options & (size_t)network::secure_layer_options::no_sslv3);
						if (name.find("no_tls_v1") != std::string::npos)
							cert->options = (network::secure_layer_options)((size_t)cert->options & (size_t)network::secure_layer_options::no_tlsv1);
						if (name.find("no_tls_v1_1") != std::string::npos)
							cert->options = (network::secure_layer_options)((size_t)cert->options & (size_t)network::secure_layer_options::no_tlsv11);
						if (name.find("no_tls_v1_2") != std::string::npos)
							cert->options = (network::secure_layer_options)((size_t)cert->options & (size_t)network::secure_layer_options::no_tlsv12);
						if (name.find("no_tls_v1_3") != std::string::npos)
							cert->options = (network::secure_layer_options)((size_t)cert->options & (size_t)network::secure_layer_options::no_tlsv13);
					}

					core::stringify::eval_envs(cert->blob.private_key, base_directory, net_addresses);
					if (!cert->blob.private_key.empty())
					{
						auto data = core::os::file::read_as_string(cert->blob.private_key);
						if (!data)
							return content_exception(core::stringify::text("import invalid server private key: %s", data.error().message().c_str()));

						cert->blob.private_key = *data;
					}

					core::stringify::eval_envs(cert->blob.certificate, base_directory, net_addresses);
					if (!cert->blob.certificate.empty())
					{
						auto data = core::os::file::read_as_string(cert->blob.certificate);
						if (!data)
							return content_exception(core::stringify::text("import invalid server certificate: %s", data.error().message().c_str()));

						cert->blob.certificate = *data;
					}
				}

				core::vector<core::schema*> listeners = blob->find_collection("listen", true);
				for (auto&& it : listeners)
				{
					core::string pattern, hostname, service;
					uint32_t port = 0; bool secure = false;
					series::unpack(it, &pattern);
					series::unpack(it->find("hostname"), &hostname);
					series::unpack(it->find("service"), &service);
					series::unpack(it->find("port"), &port);
					series::unpack(it->find("secure"), &secure);
					core::stringify::eval_envs(pattern, base_directory, net_addresses);
					core::stringify::eval_envs(hostname, base_directory, net_addresses);

					auto status = router->listen(pattern.empty() ? "*" : pattern, hostname.empty() ? "0.0.0.0" : hostname, service.empty() ? (port > 0 ? core::to_string(port) : "80") : service, secure);
					if (!status)
						return content_exception(core::stringify::text("bind %s:%i listener from %s: %s", hostname.c_str(), (int32_t)port, pattern.c_str(), status.error().message().c_str()));
				}

				core::schema* network = blob->find("network");
				if (network != nullptr)
				{
					series::unpack(network->find("keep-alive"), &router->keep_alive_max_count);
					series::unpack_a(network->find("payload-max-length"), &router->max_heap_buffer);
					series::unpack_a(network->find("payload-max-length"), &router->max_net_buffer);
					series::unpack_a(network->find("backlog-queue"), &router->backlog_queue);
					series::unpack_a(network->find("socket-timeout"), &router->socket_timeout);
					series::unpack(network->find("graceful-time-wait"), &router->graceful_time_wait);
					series::unpack_a(network->find("max-connections"), &router->max_connections);
					series::unpack(network->find("enable-no-delay"), &router->enable_no_delay);
					series::unpack_a(network->find("max-uploadable-resources"), &router->max_uploadable_resources);
					series::unpack(network->find("temporary-directory"), &router->temporary_directory);
					series::unpack(network->fetch("session.cookie.name"), &router->session.cookie.name);
					series::unpack(network->fetch("session.cookie.domain"), &router->session.cookie.domain);
					series::unpack(network->fetch("session.cookie.path"), &router->session.cookie.path);
					series::unpack(network->fetch("session.cookie.same-site"), &router->session.cookie.same_site);
					series::unpack(network->fetch("session.cookie.expires"), &router->session.cookie.expires);
					series::unpack(network->fetch("session.cookie.secure"), &router->session.cookie.secure);
					series::unpack(network->fetch("session.cookie.http-only"), &router->session.cookie.http_only);
					series::unpack(network->fetch("session.directory"), &router->session.directory);
					series::unpack(network->fetch("session.expires"), &router->session.expires);
					core::stringify::eval_envs(router->session.directory, base_directory, net_addresses);
					core::stringify::eval_envs(router->temporary_directory, base_directory, net_addresses);

					core::unordered_map<core::string, network::http::router_entry*> aliases;
					core::vector<core::schema*> groups = network->find_collection("group", true);
					for (auto&& subgroup : groups)
					{
						core::string match;
						core::schema* match_data = subgroup->get_attribute("match");
						if (match_data != nullptr && match_data->value.get_type() == core::var_type::string)
							match = match_data->value.get_blob();

						network::http::route_mode mode = network::http::route_mode::start;
						core::schema* fMode = subgroup->get_attribute("mode");
						if (fMode != nullptr && fMode->value.get_type() == core::var_type::string)
						{
							if (fMode->value.is_string("start"))
								mode = network::http::route_mode::start;
							else if (fMode->value.is_string("sub"))
								mode = network::http::route_mode::match;
							else if (fMode->value.is_string("end"))
								mode = network::http::route_mode::end;
						}

						network::http::router_group* group = router->group(match, mode);
						core::vector<core::schema*> routes = subgroup->find_collection("route", true);
						for (auto&& base : routes)
						{
							network::http::router_entry* route = nullptr;
							core::schema* from = base->get_attribute("from");
							core::schema* destination = base->get_attribute("for");
							core::string source_location = "*";
							series::unpack(base, &source_location);

							if (from != nullptr && from->value.get_type() == core::var_type::string)
							{
								auto subalias = aliases.find(from->value.get_blob());
								if (subalias != aliases.end())
									route = router->route(source_location, group, subalias->second);
								else
									route = router->route(match, mode, source_location, true);
							}
							else if (destination != nullptr && destination->value.get_type() == core::var_type::string && source_location.empty())
								route = router->route(match, mode, "..." + destination->value.get_blob() + "...", true);
							else
								route = router->route(match, mode, source_location, true);

							core::schema* level = base->get_attribute("level");
							if (level != nullptr)
								route->level = (size_t)level->value.get_integer();

							core::string tune;
							if (series::unpack(base->fetch("compression.tune"), &tune))
							{
								if (!strcmp(tune.c_str(), "filtered"))
									route->compression.tune = network::http::compression_tune::filtered;
								else if (!strcmp(tune.c_str(), "huffman"))
									route->compression.tune = network::http::compression_tune::huffman;
								else if (!strcmp(tune.c_str(), "rle"))
									route->compression.tune = network::http::compression_tune::rle;
								else if (!strcmp(tune.c_str(), "fixed"))
									route->compression.tune = network::http::compression_tune::fixed;
								else
									route->compression.tune = network::http::compression_tune::default_tune;
							}

							if (series::unpack(base->fetch("compression.quality-level"), &route->compression.quality_level))
								route->compression.quality_level = compute::math32::clamp(route->compression.quality_level, 0, 9);

							if (series::unpack(base->fetch("compression.memory-level"), &route->compression.memory_level))
								route->compression.memory_level = compute::math32::clamp(route->compression.memory_level, 1, 9);

							series::unpack(base->find("alias"), &route->alias);
							series::unpack(base->fetch("auth.type"), &route->auth.type);
							series::unpack(base->fetch("auth.realm"), &route->auth.realm);
							series::unpack_a(base->fetch("compression.min-length"), &route->compression.min_length);
							series::unpack(base->fetch("compression.enabled"), &route->compression.enabled);
							series::unpack(base->find("char-set"), &route->char_set);
							series::unpack(base->find("access-control-allow-origin"), &route->access_control_allow_origin);
							series::unpack(base->find("redirect"), &route->redirect);
							series::unpack_a(base->find("web-socket-timeout"), &route->web_socket_timeout);
							series::unpack_a(base->find("static-file-max-age"), &route->static_file_max_age);
							series::unpack(base->find("allow-directory-listing"), &route->allow_directory_listing);
							series::unpack(base->find("allow-web-socket"), &route->allow_web_socket);
							series::unpack(base->find("allow-send-file"), &route->allow_send_file);
							series::unpack(base->find("proxy-ip-address"), &route->proxy_ip_address);
							if (series::unpack(base->find("files-directory"), &route->files_directory))
								core::stringify::eval_envs(route->files_directory, base_directory, net_addresses);

							core::vector<core::schema*> auth_methods = base->fetch_collection("auth.methods.method");
							core::vector<core::schema*> compression_files = base->fetch_collection("compression.files.file");
							core::vector<core::schema*> hidden_files = base->fetch_collection("hidden-files.hide");
							core::vector<core::schema*> index_files = base->fetch_collection("index-files.index");
							core::vector<core::schema*> try_files = base->fetch_collection("try-files.fallback");
							core::vector<core::schema*> error_files = base->fetch_collection("error-files.error");
							core::vector<core::schema*> mime_types = base->fetch_collection("mime-types.file");
							core::vector<core::schema*> disallowed_methods = base->fetch_collection("disallowed-methods.method");
							if (base->fetch("auth.methods.[clear]") != nullptr)
								route->auth.methods.clear();
							if (base->fetch("compression.files.[clear]") != nullptr)
								route->compression.files.clear();
							if (base->fetch("hidden-files.[clear]") != nullptr)
								route->hidden_files.clear();
							if (base->fetch("index-files.[clear]") != nullptr)
								route->index_files.clear();
							if (base->fetch("try-files.[clear]") != nullptr)
								route->try_files.clear();
							if (base->fetch("error-files.[clear]") != nullptr)
								route->error_files.clear();
							if (base->fetch("mime-types.[clear]") != nullptr)
								route->mime_types.clear();
							if (base->fetch("disallowed-methods.[clear]") != nullptr)
								route->disallowed_methods.clear();

							for (auto& method : auth_methods)
							{
								core::string value;
								if (series::unpack(method, &value))
									route->auth.methods.push_back(value);
							}

							for (auto& file : compression_files)
							{
								core::string pattern;
								if (series::unpack(file, &pattern))
									route->compression.files.emplace_back(pattern, true);
							}

							for (auto& file : hidden_files)
							{
								core::string pattern;
								if (series::unpack(file, &pattern))
									route->hidden_files.emplace_back(pattern, true);
							}

							for (auto& file : index_files)
							{
								core::string pattern;
								if (series::unpack(file, &pattern))
								{
									if (!file->get_attribute("use"))
										core::stringify::eval_envs(pattern, base_directory, net_addresses);

									route->index_files.push_back(pattern);
								}
							}

							for (auto& file : try_files)
							{
								core::string pattern;
								if (series::unpack(file, &pattern))
								{
									if (!file->get_attribute("use"))
										core::stringify::eval_envs(pattern, base_directory, net_addresses);

									route->try_files.push_back(pattern);
								}
							}

							for (auto& file : error_files)
							{
								network::http::error_file source;
								series::unpack(file->find("status"), &source.status_code);
								if (series::unpack(file->find("file"), &source.pattern))
									core::stringify::eval_envs(source.pattern, base_directory, net_addresses);
								route->error_files.push_back(source);
							}

							for (auto& type : mime_types)
							{
								network::http::mime_type pattern;
								series::unpack(type->find("ext"), &pattern.extension);
								series::unpack(type->find("type"), &pattern.type);
								route->mime_types.push_back(pattern);
							}

							for (auto& method : disallowed_methods)
							{
								core::string value;
								if (series::unpack(method, &value))
									route->disallowed_methods.push_back(value);
							}

							if (!destination || destination->value.get_type() != core::var_type::string)
								continue;

							core::string alias = destination->value.get_blob();
							auto subalias = aliases.find(alias);
							if (subalias == aliases.end())
								aliases[alias] = route;
						}
					}

					for (auto& item : aliases)
						router->remove(item.second);
				}

				auto configure = args.find("configure");
				if (configure != args.end() && configure->second.get_boolean())
				{
					auto status = object->configure(router.reset());
					if (!status)
						return content_exception(std::move(status.error().message()));
				}
				else
					object->set_router(router.reset());

				return object.reset();
			}
		}
	}
}
