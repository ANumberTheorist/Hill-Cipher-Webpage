#include <bitset>
#include <sstream>
#include <string>
#include <thread>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

#define MATH_NERD_OMP
#include <math_nerd/hill_cipher.h>

#include "file_handler.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace mod = math_nerd::int_mod;
namespace matrix = math_nerd::matrix_t;
namespace hc = math_nerd::hill_cipher;

using tcp = net::ip::tcp;

auto operator<<(std::ostream &os, hc::hill_key key)->std::ostream &;

auto do_session(tcp::socket socket) -> void
try
{
    websocket::stream<tcp::socket> ws{ std::move(socket) };

    ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type &res)
        {
            res.set(http::field::server,
                "Hill Cipher");
        }));

    ws.accept();

    auto key_size{ 5 };
    std::unique_ptr<hc::hill_key> key{ new hc::hill_key{ key_size } };

    while( true )
    {
        beast::flat_buffer input;

        ws.read(input);

        auto cmd{ beast::buffers_to_string(input.data()) };

        std::bitset<3> cmd_type{ 0x0 };

        enum command
        {
            generate_key,
            encrypt,
            decrypt
        };

        std::string output;
        output.resize(100);
        output = "Error.";

        switch( cmd[0] )
        {
            case 'g':
            {
                if( cmd[1] >= '0' && cmd[1] <= '9' )
                {
                    key_size = std::max(cmd[1] - '0', 2);
                    cmd_type[command::generate_key] = true;
                }
            }
            break;

            case 'e':
            {
                cmd_type[command::encrypt] = true;
                cmd.erase(0, 1);
            }
            break;

            case 'd':
            {
                cmd_type[command::decrypt] = true;
                cmd.erase(0, 1);
            }
            break;

            default:
            {
                // Empty.
            }
        }

        if( cmd_type[command::generate_key] )
        {
            if( key_size != key->row_count() )
            {
                key.reset(new hc::hill_key{ key_size });
            }

            output = "g" + std::to_string(key_size);

            // Create our (not cryptographically secure) PRNG.
            std::random_device device;
            std::mt19937 rng(device());

            // Key distribution.
            std::uniform_int_distribution<int> key_dist(0, 96);

            // Index distribution for fixing invalid keys.
            std::uniform_int_distribution<int> idx_dist(0, key_size - 1);

            for( auto i{ 0 }; i < key_size; ++i )
            {
                for( auto j{ 0 }; j < key_size; ++j )
                {
                    // Create (not cryptographically secure) random elements.
                    (*key)[i][j] = key_dist(rng);
                }
            }

            while( not hc::is_valid_key((*key)) )
            {   // Randomly touch parts of the key to attempt to fix.
                (*key)[idx_dist(rng)][idx_dist(rng)] += key_dist(rng);
            }

            std::stringstream os;

            os << (*key);

            output += os.str();
        }
        else if( cmd_type[command::encrypt] )
        {
            output = "e" + hc::encrypt((*key), cmd);
        }
        else if( cmd_type[command::decrypt] )
        {
            output = "d" + hc::decrypt((*key), cmd);
        }

        beast::multi_buffer buffer;
        auto msg = net::buffer_copy(buffer.prepare(output.size()), net::buffer(output));
        buffer.commit(msg);

        ws.text(ws.got_text());
        ws.write(buffer.data());
    }
}
catch( ... )
{
    throw;
}

auto main(int, char **argv) -> int
try
{
    auto const address{ net::ip::make_address("127.0.0.1") };
    auto const port{ static_cast<std::uint16_t>(31337) };

    file_handler file{ argv[0] };

    net::io_context ioc{ 1 };

    tcp::acceptor acceptor{ ioc, {address, port} };

    tcp::socket socket{ ioc };

    acceptor.accept(socket);

    do_session(std::move(socket));

    return EXIT_SUCCESS;
}
catch( std::exception const &e )
{
    if( std::string(e.what()) == "The WebSocket stream was gracefully closed at both endpoints" )
    {
        return EXIT_SUCCESS;
    }

    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

auto operator<<(std::ostream &os, hc::hill_key key) -> std::ostream &
{
    // Starting bracket for key.
    os << "[";

    for( auto i{ 0 }; i < key.row_count(); ++i )
    {
        for( auto j{ 0 }; j < key.column_count(); ++j )
        {
            // Print element.
            os << key[i][j];

            if( j != key.column_count() - 1 )
            {
                // Delimit with comma.
                os << ",";
            }
        }

        if( i != key.row_count() - 1 )
        {   // Delimit row with semicolon.
            os << ";";
        }
    }

    // Ending bracket for key.
    os << "]";

    return os;
}
