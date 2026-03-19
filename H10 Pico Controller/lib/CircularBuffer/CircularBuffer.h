#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pico/mutex.h"

// Thread-safe circular (ring) buffer for the RP2040.
// Uses the hardware mutex from the Pico SDK, which is safe across both cores
// as well as interrupt contexts.
//
// Fixed capacity:
//   1024 bytes
//
// Usage:
//   CircularBuffer buf;
//   buf.push(0xAB);
//   uint8_t b;
//   if (buf.pop(b)) { /* use b */ }

class CircularBuffer
{
	static constexpr size_t kCapacity = 1024;

public:
	CircularBuffer() : _head(0), _tail(0), _count(0)
	{
		mutex_init(&_mutex);
	}

	// Not copyable or movable – mutex ownership cannot be transferred.
	CircularBuffer(const CircularBuffer&)            = delete;
	CircularBuffer& operator=(const CircularBuffer&) = delete;

	// Push an item onto the back of the buffer.
	// Returns true on success, false if the buffer is full.
	bool push(uint8_t item)
	{
		mutex_enter_blocking(&_mutex);
		bool ok = false;
		if (_count < kCapacity)
		{
			_buf[_tail] = item;
			_tail = (_tail + 1) % kCapacity;
			++_count;
			ok = true;
		}
		mutex_exit(&_mutex);
		return ok;
	}

	// Pop the front byte off the buffer into item.
	// Returns true on success, false if the buffer is empty.
	bool pop(uint8_t& item)
	{
		mutex_enter_blocking(&_mutex);
		bool ok = false;
		if (_count > 0)
		{
			item  = _buf[_head];
			_head = (_head + 1) % kCapacity;
			--_count;
			ok = true;
		}
		mutex_exit(&_mutex);
		return ok;
	}

	// Read the front item without removing it.
	// Returns true on success, false if the buffer is empty.
	bool peek(uint8_t& item) const
	{
		mutex_enter_blocking(&_mutex);
		bool ok = false;
		if (_count > 0)
		{
			item = _buf[_head];
			ok   = true;
		}
		mutex_exit(&_mutex);
		return ok;
	}

	// Number of elements currently in the buffer.
	size_t size() const
	{
		mutex_enter_blocking(&_mutex);
		size_t n = _count;
		mutex_exit(&_mutex);
		return n;
	}

	bool isEmpty() const { return size() == 0; }
	bool isFull()  const { return size() == kCapacity; }

	// Maximum number of elements the buffer can hold.
	constexpr size_t capacity() const { return kCapacity; }

	// Remove all elements.
	void clear()
	{
		mutex_enter_blocking(&_mutex);
		_head  = 0;
		_tail  = 0;
		_count = 0;
		mutex_exit(&_mutex);
	}

private:
	uint8_t        _buf[kCapacity];
	size_t         _head;
	size_t         _tail;
	size_t         _count;
	mutable mutex_t _mutex;
};
