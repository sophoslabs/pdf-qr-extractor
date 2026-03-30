#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <chrono>

namespace extractor::asio_utils {

using asio::local::stream_protocol;

// ==========================
// READ WITH TIMEOUT (ASYNC)
// ==========================

template<typename MutableBuffer>
bool read_full(asio::io_context& io,
               stream_protocol::socket& socket,
               const MutableBuffer& buffer,
               int timeoutMs)
{
    asio::steady_timer timer(io);

    asio::error_code ec = asio::error::would_block;
    bool done = false;

    // Start async read
    asio::async_read(socket, buffer,
        [&](const asio::error_code& error, std::size_t) {
            ec = error;
            done = true;
            timer.cancel();
        });

    // Start timer
    timer.expires_after(std::chrono::milliseconds(timeoutMs));
    timer.async_wait([&](const asio::error_code& error) {
        if (!error && !done) {
            socket.cancel();
        }
    });

    // Drive event loop
    while (!done && ec == asio::error::would_block) {
        io.run_one();
    }

    return !ec;
}

// ==========================
// WRITE WITH TIMEOUT (ASYNC)
// ==========================

template<typename ConstBuffer>
bool write_full(asio::io_context& io,
                stream_protocol::socket& socket,
                const ConstBuffer& buffer,
                int timeoutMs)
{
    asio::steady_timer timer(io);

    asio::error_code ec = asio::error::would_block;
    bool done = false;

    // Start async write
    asio::async_write(socket, buffer,
        [&](const asio::error_code& error, std::size_t) {
            ec = error;
            done = true;
            timer.cancel();
        });

    // Start timer
    timer.expires_after(std::chrono::milliseconds(timeoutMs));
    timer.async_wait([&](const asio::error_code& error) {
        if (!error && !done) {
            socket.cancel();
        }
    });

    // Drive event loop
    while (!done && ec == asio::error::would_block) {
        io.run_one();
    }

    return !ec;
}

} // namespace extractor::asio_utils
