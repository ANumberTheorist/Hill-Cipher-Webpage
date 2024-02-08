#pragma once
#ifndef MATH_NERD_FILE_HANDLER__H
#define MATH_NERD_FILE_HANDLER__H
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

// The next two functions are modified from Konrad Rudolph's code on StackOverflow:
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

auto gen_string(std::size_t len = 8) -> std::string
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
        std::string directory_;
        std::string html_name_;
        std::string css_name_;
        std::string js_name_;

        auto strip_exe_name(std::string path) -> std::string
        {
            #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
                path.erase(path.find_last_of('\\') + 1);
            #elif __linux__ || __APPLE__ || __APPLE_CC__ || __unix__
                path.erase(path.find_last_of('/') + 1);
            #endif
            return path;
        }

        auto replace_string(std::string &str, std::string const &from, std::string const &to) -> void
        {
            auto start = str.find(from);
            str.replace(start, from.length(), to);
        }

        auto file_name(std::string const &directory, char const *ext) -> std::string
        {
            std::string name;
            do
            {
                name = gen_string() + ext;
            } while( std::filesystem::exists(directory + name) );

            return name;
        }

        auto make_file(std::string const &name, std::string const &content) -> void
        {
            std::ofstream out(name);
            out << content;
        }

    public:
        file_handler(char const *directory) :
            directory_{ strip_exe_name(directory) },
            html_name_{ file_name(directory_, ".html") },
            css_name_{ file_name(directory_, ".css") },
            js_name_{ file_name(directory_, ".js") }
        {
            {
                // Start HTML file. 
                std::string html_content
                {
                    #include "html.txt"
                };

                // Start CSS file.
                std::string css_content
                {
                    #include "css.txt"
                };

                // Put CSS file name into HTML content.
                replace_string(html_content, "$css_name$", css_name_);

                // Create CSS file.
                css_name_ = directory_ + css_name_;
                make_file(css_name_, css_content);

                // Start JS file.
                std::string js_content
                {
                    #include "js.txt"
                };

                // Put JS file name into HTML content.
                replace_string(html_content, "$js_name$", js_name_);

                // Create JS file.
                js_name_ = directory_ + js_name_;
                make_file(js_name_, js_content);

                // Create HTML file.
                html_name_ = directory_ + html_name_;
                make_file(html_name_, html_content);
            }

            #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
                ShellExecuteA(0, 0, html_name_.c_str(), 0, 0, SW_SHOW);
            #elif __linux__
                std::string cmd{ "xdg-open ./" + html_name_ };

                if( std::system(cmd.c_str()) != 0 )
                {
                    std::cout << "Unable to open browser using xdg-open command.\n";
                }
            #elif  __APPLE__ || __APPLE_CC__ || __unix__
                std::string cmd{ "open ./" + html_name_ };

                if( std::system(cmd.c_str()) != 0 )
                {
                    std::cout << "Unable to open browser using open command.\n";
                }
            #endif
        }


        ~file_handler()
        {
            std::filesystem::remove(html_name_);
            std::filesystem::remove(css_name_);
            std::filesystem::remove(js_name_);
        }
};
#endif // MATH_NERD_FILE_HANDLER__H
