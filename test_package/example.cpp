// Test that common_system can be included and used
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/patterns/event_bus.h>
#include <kcenon/common/utils/object_pool.h>
#include <kcenon/common/utils/circular_buffer.h>
#include <iostream>

using namespace kcenon::common;

int main() {
    // Test Result<T>
    auto result = Result<int>::ok(42);
    if (result.is_ok()) {
        std::cout << "Result: " << result.value() << std::endl;
    }

    // Test Result chaining
    auto chained = result
        .map([](int x) { return x * 2; })
        .and_then([](int x) -> Result<std::string> {
            return Result<std::string>::ok("Value: " + std::to_string(x));
        });

    if (chained.is_ok()) {
        std::cout << "Chained: " << chained.value() << std::endl;
    }

    // Test EventBus
    simple_event_bus bus;
    struct TestEvent { int value; };

    int received = 0;
    auto sub_id = bus.subscribe<TestEvent>([&received](const TestEvent& e) {
        received = e.value;
    });

    bus.publish(TestEvent{123});
    bus.unsubscribe(sub_id);

    std::cout << "EventBus received: " << received << std::endl;

    // Test ObjectPool
    struct TestObject { int data; };
    utils::ObjectPool<TestObject> pool(4);

    bool reused = false;
    auto obj = pool.acquire(&reused);
    obj->data = 99;
    std::cout << "ObjectPool object data: " << obj->data << std::endl;

    // Test CircularBuffer
    utils::CircularBuffer<int, 10> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    auto val = buffer.pop();
    if (val.has_value()) {
        std::cout << "CircularBuffer popped: " << val.value() << std::endl;
    }

    std::cout << "common_system package test passed!" << std::endl;
    return 0;
}
