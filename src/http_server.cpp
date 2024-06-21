#include "http_server.h"
#include "logger.h"

#include <boost/asio/dispatch.hpp>

#include <iostream>

namespace http_server {

using namespace std::literals;

void ReportError(beast::error_code ec, std::string_view what) {
    http_logger::LogServerError(ec.value(), ec.message(), what);
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

void SessionBase::Run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(30s);
    http::async_read(stream_, buffer_, request_,
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        return Close();
    }

    if (ec) {
        return ReportError(ec, "read"sv);
    }

    HandlerRequest(std::move(request_));
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    Read();
}

net::ip::tcp::endpoint SessionBase::GetRemoteEndpoint() const {
    return stream_.socket().remote_endpoint();
}

net::ip::tcp::endpoint SessionBase::GetLocalEndpoint() const {
    return stream_.socket().local_endpoint();
}

}  // namespace http_server
