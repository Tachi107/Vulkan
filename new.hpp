#pragma once
#include <iostream>
#include <cstdint>

#if true
static std::uint32_t allocCount{};
static std::size_t totalAllocSize{};

void* operator new(std::size_t size) {
	if (size == 0) {
		size = 1;
	}
	void* p;
	while ((p = ::malloc(size)) == nullptr) {
		// If malloc fails and there is a new_handler,
		// call it to try free up memory.
		std::new_handler nh = std::get_new_handler();
		if (nh) {
			nh();
		}
		else {
			throw std::bad_alloc();
		}
	}
	++allocCount;
	totalAllocSize += size;
	std::cerr << "Allocating " << size << " bytes, " << allocCount << " total allocations, " << totalAllocSize << " bytes allocated\n";
	return p;
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
	if (size == 0) {
		size = 1;
	}
	if (static_cast<size_t>(alignment) < sizeof(void*)) {
		alignment = std::align_val_t(sizeof(void*));
	}
	void* p;
#if defined(_LIBCPP_MSVCRT_LIKE)
	while ((p = _aligned_malloc(size, static_cast<size_t>(alignment))) == nullptr)
#else
	while (::posix_memalign(&p, static_cast<size_t>(alignment), size) != 0)
#endif
	{
		// If posix_memalign fails and there is a new_handler,
		// call it to try free up memory.
		std::new_handler nh = std::get_new_handler();
		if (nh) {
			nh();
		}
		else {
#ifndef _LIBCPP_NO_EXCEPTIONS
			throw std::bad_alloc();
#else
			p = nullptr; // posix_memalign doesn't initialize 'p' on failure
			break;
#endif
		}
	}
	++allocCount;
	totalAllocSize += size;
	std::cerr << "Allocating " << size << " bytes, " << allocCount << " total allocations, " << totalAllocSize << " bytes allocated\n";
	return p;
}
#endif
