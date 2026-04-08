// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file utility_containers_example.cpp
/// @example utility_containers_example.cpp
/// @brief Demonstrates CircularBuffer and ObjectPool utility containers.
///
/// Shows mutex-protected circular buffer, SPSC lock-free buffer for
/// producer-consumer scenarios, and object pool with automatic recycling.
///
/// @see kcenon::common::CircularBuffer
/// @see kcenon::common::SPSCCircularBuffer
/// @see kcenon::common::ObjectPool

#include <kcenon/common/utils/circular_buffer.h>
#include <kcenon/common/utils/object_pool.h>

#include <iostream>
#include <string>

using namespace kcenon::common;

// A sample resource for the object pool
struct Connection
{
	int id;
	std::string endpoint;

	Connection() : id(0), endpoint("default") {}
	Connection(int i, const std::string& ep) : id(i), endpoint(ep) {}
};

int main()
{
	std::cout << "=== Utility Containers Example ===\n\n";

	// --- Circular Buffer ---
	std::cout << "1. CircularBuffer (mutex-protected, capacity=5):\n";
	{
		CircularBuffer<int, 5> buffer;

		// Push elements
		for (int i = 1; i <= 5; ++i)
		{
			buffer.push(i);
		}
		std::cout << "   Pushed 1-5. Size=" << buffer.size() << ", Full=" << buffer.full() << "\n";

		// Overwrite oldest when full
		buffer.push(99, true); // overwrite=true
		std::cout << "   Pushed 99 with overwrite. Size=" << buffer.size() << "\n";

		// Pop elements
		std::cout << "   Popping: ";
		while (auto val = buffer.pop())
		{
			std::cout << *val << " ";
		}
		std::cout << "\n   Empty=" << buffer.empty() << "\n";
	}

	// --- SPSC Lock-Free Buffer ---
	std::cout << "\n2. SPSCCircularBuffer (lock-free, capacity=4):\n";
	{
		SPSCCircularBuffer<std::string, 4> spsc;

		spsc.push("alpha");
		spsc.push("beta");
		spsc.push("gamma");
		std::cout << "   Pushed 3 strings. Size=" << spsc.size() << "\n";

		// Pop one
		if (auto val = spsc.pop())
		{
			std::cout << "   Popped: '" << *val << "'\n";
		}
		std::cout << "   Remaining size=" << spsc.size() << "\n";
	}

	// --- Object Pool ---
	std::cout << "\n3. ObjectPool (growth=4):\n";
	{
		ObjectPool<Connection> pool(4);

		// Pre-allocate
		pool.reserve(8);
		std::cout << "   Reserved 8 slots. Available=" << pool.available() << "\n";

		// Acquire objects
		bool reused = false;
		auto conn1 = pool.acquire(&reused, 1, "db-primary");
		std::cout << "   Acquired conn1: id=" << conn1->id << ", endpoint=" << conn1->endpoint
				  << ", reused=" << (reused ? "true" : "false") << "\n";

		auto conn2 = pool.acquire(&reused, 2, "db-replica");
		std::cout << "   Acquired conn2: id=" << conn2->id
				  << ", available=" << pool.available() << "\n";

		// Release conn1 back to pool (via unique_ptr destructor)
		std::cout << "   Releasing conn1...\n";
		conn1.reset();
		std::cout << "   Available after release=" << pool.available() << "\n";

		// Re-acquire — should reuse
		auto conn3 = pool.acquire(&reused, 3, "db-analytics");
		std::cout << "   Acquired conn3: reused=" << (reused ? "true" : "false") << "\n";

		// Clear the pool
		conn2.reset();
		conn3.reset();
		pool.clear();
		std::cout << "   Pool cleared. Available=" << pool.available() << "\n";
	}

	std::cout << "\nDone.\n";
	return 0;
}
