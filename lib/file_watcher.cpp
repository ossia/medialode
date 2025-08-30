/* #include <llfio/llfio.hpp>
#include <iostream>
#include <system_error>

namespace llfio = LLFIO_V2_NAMESPACE;

void test_llfio()
{
    try
    {
        auto result = llfio::temp_directory();
        if (!result)
        {
            auto err = result.error();
            std::cerr << "LLFIO error: " << err.message() << std::endl;
            throw std::system_error(std::error_code(static_cast<int>(err.value()), std::generic_category()), "temp_directory() failed");
        }

        auto dir = std::move(result.value());
        std::cout << "Temp directory handle obtained successfully.\n";
    }

    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}*/
