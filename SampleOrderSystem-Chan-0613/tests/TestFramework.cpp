#include "TestFramework.h"

#include <iostream>

namespace Testing
{
    AssertionFailure::AssertionFailure(std::string message)
        : message_(std::move(message))
    {
    }

    const char* AssertionFailure::what() const noexcept
    {
        return message_.c_str();
    }

    TestRegistry& TestRegistry::Instance()
    {
        static TestRegistry registry;
        return registry;
    }

    void TestRegistry::Add(const std::string& suite, const std::string& name, std::function<void()> body)
    {
        entries_.push_back(Entry{suite, name, std::move(body)});
    }

    int TestRegistry::RunAll() const
    {
        int failed = 0;

        for (const auto& entry : entries_)
        {
            std::cout << "[ RUN      ] " << entry.suite << "." << entry.name << "\n";
            try
            {
                entry.body();
                std::cout << "[       OK ] " << entry.suite << "." << entry.name << "\n";
            }
            catch (const AssertionFailure& failure)
            {
                ++failed;
                std::cout << "[  FAILED  ] " << entry.suite << "." << entry.name
                          << " - " << failure.what() << "\n";
            }
            catch (const std::exception& ex)
            {
                ++failed;
                std::cout << "[  FAILED  ] " << entry.suite << "." << entry.name
                          << " - unexpected exception: " << ex.what() << "\n";
            }
        }

        const auto total = entries_.size();
        std::cout << "\n" << (total - static_cast<size_t>(failed)) << "/" << total << " tests passed";
        if (failed > 0)
        {
            std::cout << " (" << failed << " FAILED)";
        }
        std::cout << "\n";

        return failed;
    }

    AutoRegister::AutoRegister(const std::string& suite, const std::string& name, std::function<void()> body)
    {
        TestRegistry::Instance().Add(suite, name, std::move(body));
    }
}
