#include <boost/beast/http/verb.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <chrono>
#include <string_view>

#include "logger.h"

using namespace std::literals;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

namespace http_logger {

void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    boost::json::value log = {
        {"timestamp", to_iso_extended_string(*ts)},
        {"data", *rec[log_data]},
        {"message", *rec[expr::smessage]}
    };

    strm << boost::json::serialize(log);
}

void LogServerStart(unsigned int port, std::string_view address) {
    boost::json::value data = {
        {"port", port},
        {"address", address}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "server started";
}

void LogServerEnd(unsigned int return_code, std::string_view exeption_text) {
    if (return_code == 0) {
        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, boost::json::value{{"code", return_code}}) << "server exited";
    } else {
        boost::json::value data = {
            {"code", return_code},
            {"exception", exeption_text}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "server exited";
    }
}

void LogServerError(unsigned int error_code, std::string_view error_message, std::string_view where) {
    boost::json::value data = {
        {"code", error_code},
        {"text", error_message},
        {"where", where}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(log_data, data) << "error";
}

} // namespace http_logger
