/*
 * Copyright (C) 2014 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "runtime.hpp"
#include <dxgi1_5.h>

namespace reshade::d3d12
{
	class device_impl;
	class command_queue_impl;

	class swapchain_impl : public api::api_object_impl<IDXGISwapChain3 *, runtime>
	{
	public:
		swapchain_impl(device_impl *device, command_queue_impl *queue, IDXGISwapChain3 *swapchain);
		~swapchain_impl();

		api::resource get_back_buffer(uint32_t index) final;

		uint32_t get_back_buffer_count() const final;
		uint32_t get_current_back_buffer_index() const final;

		bool on_init();
		void on_reset();

		void on_present();
		bool on_present(ID3D12Resource *source, HWND hwnd);

	private:
		UINT _swap_index = 0;
		std::vector<com_ptr<ID3D12Resource>> _backbuffers;
	};
}
