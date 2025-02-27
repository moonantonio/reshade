/*
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "addon_manager.hpp"
#include <shared_mutex>
#include <unordered_map>
#pragma warning(push)
#pragma warning(disable: 4100 4127 4324 4703) // Disable a bunch of warnings thrown by VMA code
#include <vk_mem_alloc.h>
#pragma warning(pop)
#include <vk_layer_dispatch_table.h>

namespace reshade::vulkan
{
	template <VkObjectType type> struct object_data;

	class device_impl : public api::api_object_impl<VkDevice, api::device>
	{
		friend class command_list_impl;
		friend class command_queue_impl;

	public:
		device_impl(
			VkDevice device,
			VkPhysicalDevice physical_device,
			const VkLayerInstanceDispatchTable &instance_table, const VkLayerDispatchTable &device_table, const VkPhysicalDeviceFeatures &enabled_features,
			bool push_descriptors_ext,
			bool dynamic_rendering_ext,
			bool custom_border_color_ext,
			bool extended_dynamic_state_ext,
			bool conservative_rasterization_ext);
		~device_impl();

		api::device_api get_api() const final { return api::device_api::vulkan; }

		bool check_capability(api::device_caps capability) const final;
		bool check_format_support(api::format format, api::resource_usage usage) const final;

		bool create_sampler(const api::sampler_desc &desc, api::sampler *out_handle) final;
		void destroy_sampler(api::sampler handle) final;

		bool create_resource(const api::resource_desc &desc, const api::subresource_data *initial_data, api::resource_usage initial_state, api::resource *out_handle, HANDLE *shared_handle = nullptr) final;
		void destroy_resource(api::resource handle) final;

		api::resource_desc get_resource_desc(api::resource resource) const final;

		bool create_resource_view(api::resource resource, api::resource_usage usage_type, const api::resource_view_desc &desc, api::resource_view *out_handle) final;
		void destroy_resource_view(api::resource_view handle) final;

		api::resource get_resource_from_view(api::resource_view view) const final;
		api::resource_view_desc get_resource_view_desc(api::resource_view view) const final;

		bool map_buffer_region(api::resource resource, uint64_t offset, uint64_t size, api::map_access access, void **out_data) final;
		void unmap_buffer_region(api::resource resource) final;
		bool map_texture_region(api::resource resource, uint32_t subresource, const api::subresource_box *box, api::map_access access, api::subresource_data *out_data) final;
		void unmap_texture_region(api::resource resource, uint32_t subresource) final;

		void update_buffer_region(const void *data, api::resource resource, uint64_t offset, uint64_t size) final;
		void update_texture_region(const api::subresource_data &data, api::resource resource, uint32_t subresource, const api::subresource_box *box) final;

		bool create_pipeline(api::pipeline_layout layout, uint32_t subobject_count, const api::pipeline_subobject *subobjects, api::pipeline *out_handle) final;
		void destroy_pipeline(api::pipeline handle) final;

		bool create_pipeline_layout(uint32_t param_count, const api::pipeline_layout_param *params, api::pipeline_layout *out_handle) final;
		void destroy_pipeline_layout(api::pipeline_layout handle) final;

		bool allocate_descriptor_sets(uint32_t count, api::pipeline_layout layout, uint32_t layout_param, api::descriptor_set *out_sets) final;
		void free_descriptor_sets(uint32_t count, const api::descriptor_set *sets) final;

		void get_descriptor_pool_offset(api::descriptor_set set, uint32_t binding, uint32_t array_offset, api::descriptor_pool *out_pool, uint32_t *out_offset) const final;

		void copy_descriptor_sets(uint32_t count, const api::descriptor_set_copy *copies) final;
		void update_descriptor_sets(uint32_t count, const api::descriptor_set_update *updates) final;

		bool create_query_pool(api::query_type type, uint32_t size, api::query_pool *out_handle) final;
		void destroy_query_pool(api::query_pool handle) final;

		bool get_query_pool_results(api::query_pool pool, uint32_t first, uint32_t count, void *results, uint32_t stride) final;

		void set_resource_name(api::resource handle, const char *name) final;
		void set_resource_view_name(api::resource_view handle, const char *name) final;

		void advance_transient_descriptor_pool();

		template <VkObjectType type, typename... Args>
		void register_object(typename object_data<type>::Handle object, Args... args)
		{
			assert(object != VK_NULL_HANDLE);
			uint64_t private_data = reinterpret_cast<uint64_t>(new object_data<type>(std::forward<Args>(args)...));
			_dispatch_table.SetPrivateData(_orig, type, (uint64_t)object, _private_data_slot, private_data);
		}
		void register_object(VkObjectType type, uint64_t object, void *private_data)
		{
			_dispatch_table.SetPrivateData(_orig, type, object, _private_data_slot, reinterpret_cast<uint64_t>(private_data));
		}

		template <VkObjectType type>
		void unregister_object(typename object_data<type>::Handle object)
		{
			if (object == VK_NULL_HANDLE)
				return;

			uint64_t private_data = 0;
			_dispatch_table.GetPrivateData(_orig, type, (uint64_t)object, _private_data_slot, &private_data);
			delete reinterpret_cast<object_data<type> *>(private_data);
			_dispatch_table.SetPrivateData(_orig, type, (uint64_t)object, _private_data_slot, 0);
		}
		void unregister_object(VkObjectType type, uint64_t object)
		{
			_dispatch_table.SetPrivateData(_orig, type, object, _private_data_slot, 0);
		}

		template <VkObjectType type>
		__forceinline object_data<type> *get_private_data_for_object(typename object_data<type>::Handle object) const
		{
			assert(object != VK_NULL_HANDLE);
			uint64_t private_data = 0;
			_dispatch_table.GetPrivateData(_orig, type, (uint64_t)object, _private_data_slot, &private_data);
			assert(private_data != 0);
			return reinterpret_cast<object_data<type> *>(private_data);
		}

		const VkPhysicalDevice _physical_device;
		const VkLayerDispatchTable _dispatch_table;
		const VkLayerInstanceDispatchTable _instance_dispatch_table;

		uint32_t _graphics_queue_family_index = std::numeric_limits<uint32_t>::max();
		std::vector<command_queue_impl *> _queues;

		const bool _push_descriptor_ext;
		const bool _dynamic_rendering_ext;
		const bool _custom_border_color_ext;
		const bool _extended_dynamic_state_ext;
		const bool _conservative_rasterization_ext;
		const VkPhysicalDeviceFeatures _enabled_features;

#ifndef NDEBUG
		mutable bool _wait_for_idle_happened = false;
#endif

	private:
		bool create_shader_module(VkShaderStageFlagBits stage, const api::shader_desc &desc, VkPipelineShaderStageCreateInfo &stage_info, VkSpecializationInfo &spec_info, std::vector<VkSpecializationMapEntry> &spec_map);

		VmaAllocator _alloc = nullptr;
		VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
		VkDescriptorPool _transient_descriptor_pool[4] = {};
		uint32_t _transient_index = 0;

		VkPrivateDataSlot _private_data_slot = VK_NULL_HANDLE;

		std::shared_mutex _mutex;
		std::unordered_map<size_t, VkRenderPassBeginInfo> _render_pass_lookup;
	};
}
