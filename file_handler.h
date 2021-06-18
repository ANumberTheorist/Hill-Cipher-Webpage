#pragma once
#ifndef MATH_NERD_HILL_CIPHER_FILE_HANDLER_H
#define MATH_NERD_HILL_CIPHER_FILE_HANDLER_H
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <random>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

// The next two functions come from Konrad Rudolph on StackOverflow:
// https://stackoverflow.com/a/444614
template <typename T = std::mt19937>
auto random_generator() -> T
{
    auto constexpr seed_bytes = sizeof(typename T::result_type) * T::state_size;
    auto constexpr seed_len = seed_bytes / sizeof(std::seed_seq::result_type);
    auto seed = std::array<std::seed_seq::result_type, seed_len>();
    auto dev = std::random_device();
    std::generate_n(begin(seed), seed_len, std::ref(dev));
    auto seed_seq = std::seed_seq(begin(seed), end(seed));
    return T{ seed_seq };
}

auto gen_string(std::size_t len = 16) -> std::string
{
    static constexpr auto chars =
        "0123456789"
        "ABCXYZ"
        "abcxyz";
    thread_local auto rng = random_generator<>();
    auto dist = std::uniform_int_distribution{ {}, std::strlen(chars) - 1 };
    auto result = std::string(len, '\0');
    std::generate_n(begin(result), len, [&]() { return chars[dist(rng)]; });
    return result;
}

class file_handler
{
private:
    std::string file_name;

public:
    file_handler() : file_name{ gen_string() }
    {
        int len = 16;
        while( std::filesystem::exists(file_name) )
        {
            file_name = gen_string(len++);

            if( len > 20 )
            {
                len = 16;
            }
        }

        file_name += ".html";
        {
            std::ofstream ofile(file_name);

            std::string file_content
            {
                #include "hill_cipher.txt"
            };

            if( std::filesystem::exists("favicon.ico") )
            {
                auto head = file_content.find("<head>\n");

                if( head != std::string::npos )
                {
                    file_content.insert(head + 7, "\n");
                    file_content.insert(head + 7, R"html(        <link rel="shortcut icon" href="favicon.ico">)html");
                }
            }

            ofile << file_content;
        }
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
        ShellExecuteA(0, 0, file_name.c_str(), 0, 0, SW_SHOW);
#elif __linux__
        std::string cmd{ "xdg-open ./" + file_name };

        if( std::system(cmd.c_str()) != 0 )
        {
            std::cout << "Unable to open browser using xdg-open command.\n";
        }
#elif  __APPLE__ || __APPLE_CC__ || __unix__
        std::string cmd{ "open ./" + file_name };

        if( std::system(cmd.c_str()) != 0 )
        {
            std::cout << "Unable to open browser using open command.\n";
        }
#endif
    }


    ~file_handler()
    {
        std::filesystem::remove(file_name);
    }
};
#endif
