/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#pragma once

#include "runtime.hpp"
#include "com_ptr.hpp"
#include <d3d12.h>
#include <dxgi1_5.h>

namespace reshade::d3d12
{
	class runtime_d3d12 : public runtime
	{
	public:
		runtime_d3d12(ID3D12Device *device, ID3D12CommandQueue *queue, IDXGISwapChain3 *swapchain);
		~runtime_d3d12();

		bool on_init(const DXGI_SWAP_CHAIN_DESC &desc);
		void on_reset();
		void on_present();

		void capture_screenshot(uint8_t *buffer) const override;

	private:
		bool init_texture(texture &info) override;
		void upload_texture(texture &texture, const uint8_t *pixels) override;
		bool update_texture_reference(texture &texture);

		bool compile_effect(effect_data &effect) override;
		void unload_effects() override;

		bool init_technique(technique &technique, const struct d3d12_technique_data &impl_init, const std::unordered_map<std::string, com_ptr<ID3DBlob>> &entry_points);

		void render_technique(technique &technique) override;

#if RESHADE_GUI
		bool init_imgui_resources();
		void render_imgui_draw_data(ImDrawData *data) override;
#endif
		void execute_command_list(const com_ptr<ID3D12GraphicsCommandList> &list)
		{
			ID3D12CommandList *const cmd_lists[] = { list.get() };
			_commandqueue->ExecuteCommandLists(ARRAYSIZE(cmd_lists), cmd_lists);
		}

		com_ptr<ID3D12Device> _device;
		com_ptr<ID3D12CommandQueue> _commandqueue;
		com_ptr<IDXGISwapChain3> _swapchain;
		HMODULE _d3d_compiler = nullptr;

		DXGI_FORMAT _backbuffer_format = DXGI_FORMAT_UNKNOWN;
		com_ptr<ID3D12CommandAllocator> _cmd_alloc;

		D3D12_CPU_DESCRIPTOR_HANDLE _default_depthstencil;

		int _imgui_index_buffer_size[3] = {};
		com_ptr<ID3D12Resource> _imgui_index_buffer[3];
		int _imgui_vertex_buffer_size[3] = {};
		com_ptr<ID3D12Resource> _imgui_vertex_buffer[3];
		com_ptr<ID3D12PipelineState> _imgui_pipeline;
		com_ptr<ID3D12RootSignature> _imgui_signature;
		com_ptr<ID3D12GraphicsCommandList> _imgui_cmd_list;
	};
}