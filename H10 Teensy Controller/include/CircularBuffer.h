// Single-producer/single-consumer circular buffer.
// Suitable for main-loop and ISR communication on single-core MCUs.
//
// Fixed capacity:
//   1024 bytes
//
// Usage:
//   CircularBuffer buf;
//   buf.push(0xAB);
//   uint8_t b;
//   if (buf.pop(b)) { /* use b */ }

#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

class CircularBuffer
{
	static constexpr size_t kCapacity = 1024;
	static constexpr size_t kStorageSize = kCapacity + 1;

public:
	CircularBuffer() : _head(0), _tail(0)
	{
	}

	// Not copyable or movable – mutex ownership cannot be transferred.
	CircularBuffer(const CircularBuffer&)            = delete;
	CircularBuffer& operator=(const CircularBuffer&) = delete;

	// Push an item onto the back of the buffer.
	// Returns true on success, false if the buffer is full.
	bool push(uint8_t item)
	{
		const size_t nextTail = nextIndex(_tail);
		if (nextTail == _head)
		{
			return false;
		}

		_buf[_tail] = item;
		_tail = nextTail;
		return true;
	}

	// Pop the front byte off the buffer into item.
	// Returns true on success, false if the buffer is empty.
	bool pop(uint8_t& item)
	{
		if (_head == _tail)
		{
			return false;
		}

		item = _buf[_head];
		_head = nextIndex(_head);
		return true;
	}

	// Read the front item without removing it.
	// Returns true on success, false if the buffer is empty.
	bool peek(uint8_t& item) const
	{
		if (_head == _tail)
		{
			return false;
		}

		item = _buf[_head];
		return true;
	}

	// Number of elements currently in the buffer.
	size_t size() const
	{
		if (_tail >= _head)
		{
			return _tail - _head;
		}

		return kStorageSize - (_head - _tail);
	}

	bool isEmpty() const { return _head == _tail; }
	bool isFull()  const { return nextIndex(_tail) == _head; }

	// Maximum number of elements the buffer can hold.
	constexpr size_t capacity() const { return kCapacity; }

	// Remove all elements.
	void clear()
	{
		_head = 0;
		_tail = 0;
	}

private:
	static constexpr size_t nextIndex(size_t index)
	{
		return (index + 1U) % kStorageSize;
	}

	volatile uint8_t _buf[kStorageSize];
	volatile size_t _head;
	volatile size_t _tail;
};
