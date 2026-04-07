#include <kcenon/common/patterns/result.h>

#include <iostream>

int main()
{
    auto result = kcenon::common::Result<int>::ok(42);
    std::cout << "common_system: result = " << result.unwrap() << std::endl;
    return result.is_ok() ? 0 : 1;
}
