/*
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <vector>
#include <cassert>

template <typename T, size_t STACK_ELEMENTS = 16>
struct temp_mem
{
	explicit temp_mem(size_t elements = STACK_ELEMENTS)
	{
		if (elements > STACK_ELEMENTS)
			p = new T[elements];
		else
			p = stack;
	}
	~temp_mem()
	{
		if (p != stack)
			delete[] p;
	}

	T &operator[](size_t element)
	{
		assert(element < STACK_ELEMENTS || p != stack);

		return p[element];
	}

	T *p, stack[STACK_ELEMENTS];
};

namespace reshade::api
{
	template <typename T, typename... api_object_base>
	class api_object_impl : public api_object_base...
	{
		static_assert(sizeof(T) <= sizeof(uint64_t));

	public:
		api_object_impl(const api_object_impl &) = delete;
		api_object_impl &operator=(const api_object_impl &) = delete;

		void get_private_data(const uint8_t guid[16], uint64_t *data) const override
		{
			for (auto it = _private_data.begin(); it != _private_data.end(); ++it)
			{
				if (std::memcmp(it->guid, guid, 16) == 0)
				{
					*data = it->data;
					return;
				}
			}

			*data = 0;
		}
		void set_private_data(const uint8_t guid[16], const uint64_t data)  override
		{
			for (auto it = _private_data.begin(); it != _private_data.end(); ++it)
			{
				if (std::memcmp(it->guid, guid, 16) == 0)
				{
					if (data != 0)
						it->data = data;
					else
						_private_data.erase(it);
					return;
				}
			}

			if (data != 0)
			{
				_private_data.push_back({ data, {
					reinterpret_cast<const uint64_t *>(guid)[0],
					reinterpret_cast<const uint64_t *>(guid)[1] } });
			}
		}

		uint64_t get_native() const override { return (uint64_t)_orig; }

		T _orig;

	protected:
		template <typename... Args>
		explicit api_object_impl(T orig, Args... args) : api_object_base(std::forward<Args>(args)...)..., _orig(orig) {}
		~api_object_impl()
		{
			// All user data should ideally have been removed before destruction, to avoid leaks
			assert(_private_data.empty());
		}

	private:
		struct private_data
		{
			uint64_t data;
			uint64_t guid[2];
		};

		std::vector<private_data> _private_data;
	};

	struct api_object;
	struct effect_runtime;
}

#if RESHADE_ADDON

namespace reshade
{
	struct addon_info
	{
		struct overlay_callback
		{
			std::string title;
			void(*callback)(api::effect_runtime *) = nullptr;
		};

		void *handle = nullptr;
#if !RESHADE_ADDON_LITE
		bool  loaded = false;
#endif
		std::string name;
		std::string description;
		std::string file;
		std::string author;
		std::string version;

		std::vector<std::pair<uint32_t, void *>> event_callbacks;
#if RESHADE_GUI
		void(*settings_overlay_callback)(api::effect_runtime *) = nullptr;
		std::vector<overlay_callback> overlay_callbacks;
#endif
	};
}

#endif
