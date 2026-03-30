#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <chrono>

namespace extractor::asio_utils {

using asio::local::stream_protocol;


// READ WITH TIMEOUT

template<typename MutableBuffer>
bool read_full(stream_protocol::socket& socket,
               const MutableBuffer& buffer,
               int timeoutMs)
{
    asio::steady_timer timer(socket.get_executor());
    bool timedOut = false;

    timer.expires_after(std::chrono::milliseconds(timeoutMs));

    timer.async_wait([&](const asio::error_code& ec) {
        if (!ec) {
            timedOut = true;
            socket.cancel();
        }
    });

    asio::error_code ec;   
    asio::read(socket, buffer, ec);   

    timer.cancel();

    return !ec && !timedOut;
}

// WRITE WITH TIMEOUT

template<typename ConstBuffer>
bool write_full(stream_protocol::socket& socket,
                const ConstBuffer& buffer,
                int timeoutMs)
{
    asio::steady_timer timer(socket.get_executor());
    bool timedOut = false;

    timer.expires_after(std::chrono::milliseconds(timeoutMs));

    timer.async_wait([&](const asio::error_code& ec) {
        if (!ec) {
            timedOut = true;
            socket.cancel();
        }
    });

    asio::error_code ec;    
    asio::write(socket, buffer, ec);   

    timer.cancel();

    return !ec && !timedOut;
}

} // namespace extractor::asio_utils