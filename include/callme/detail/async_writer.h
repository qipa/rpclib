#pragma once

#ifndef ASYNC_WRITER_H_HQIRH28I
#define ASYNC_WRITER_H_HQIRH28I

#include "asio.hpp"
#include "msgpack.hpp"
#include <deque>

namespace callme {
namespace detail {

//! \brief Common logic for classes that have a write queue with async writing.
class async_writer {
public:
    async_writer(asio::io_service *io, asio::ip::tcp::socket socket)
        : socket_(std::move(socket)), write_strand_(*io) {}

    void do_write() {
        auto &item = write_queue_.front();
        // the data in item remains valid until the handler is called
        // since it will still be in the queue physically until then.
        asio::async_write(socket_, asio::buffer(item.data(), item.size()),
                          write_strand_.wrap([this](std::error_code ec,
                                                    std::size_t transferred) {
                              write_queue_.pop_front();
                              if (write_queue_.size() > 0) {
                                  do_write();
                              }
                          }));
    }

    void write(msgpack::sbuffer &&data) {
        write_queue_.push_back(std::move(data));
        if (write_queue_.size() > 1) {
            return; // there is an ongoing write chain so don't start another
        }

        do_write();
    }

    asio::ip::tcp::socket socket_;
    asio::strand write_strand_;

private:
    std::deque<msgpack::sbuffer> write_queue_;
};

} /* detail */
} /* callme  */

#endif /* end of include guard: ASYNC_WRITER_H_HQIRH28I */
