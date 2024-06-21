#pragma once

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <chrono>
#include <iostream>
#include <string_view>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace http = boost::beast::http;

namespace http_logger {

class DurationMeasure {
public:
    DurationMeasure() = default;

    auto Count() const {
        std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts_).count();
    };

private:
    std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
};

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_data, "LogData", boost::json::value)

void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

template <typename Formatter>
void InitBoostLogFilter(Formatter&& formatter) {
    logging::add_common_attributes();
    
    logging::add_console_log(
        std::cout,
        keywords::format = &formatter,
        keywords::auto_flush = true
    );
}

template<class RequestHandler>
class LogginRequestHandler {
public:

    explicit LogginRequestHandler(RequestHandler& request_handler)
        : decorated_(request_handler) {
    }

    template <typename Request, typename Send>
    void operator()(Request&& req, std::string_view client_ip, Send&& send) {
        DurationMeasure dur;
        LogRequest(client_ip, req);
        decorated_(std::forward<decltype(req)>(req), [this, send = std::forward<Send>(send),
                                                      dur = std::move(dur)] (auto&& response) {
            this->LogResponse(dur.Count(), response);
            send(response);
        });
    }

private:
    RequestHandler& decorated_;

    template <typename Request>
    void LogRequest(std::string_view client_ip, Request& request) const {
        boost::json::value data = {
            {"ip", client_ip},
            {"URI", request.target()},
            {"method", http::to_string(request.method())}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "request received";
    }

    template <typename Response>
    void LogResponse(std::uint64_t time, Response& response) const {
        auto content_type = response[http::field::content_type];
        if (content_type.empty()) {
            content_type = "null";
        }
        boost::json::value data = {
            {"response_time", time},
            {"code", response.result_int()},
            {"content_type", content_type}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "response sent";
    }
};



void LogServerStart(unsigned int port, std::string_view address);
void LogServerEnd(unsigned int return_code, std::string_view exeption_text);
void LogServerError(unsigned int error_code, std::string_view error_message, std::string_view where);
} // namespace http_logger
