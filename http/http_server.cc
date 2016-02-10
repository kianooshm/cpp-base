/*
 *    -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "cpp-base/http/http_server.h"

#include <future>

#include "cpp-base/http/request_handler.h"
#include "cpp-base/http/uri.h"
#include "cpp-base/management/exportee.h"
#include "cpp-base/management/global_exporter.h"
#include "cpp-base/string/join.h"
#include "cpp-base/string/split.h"
#include "cpp-base/util/clock.h"

using std::string;
using std::vector;

namespace cpp_base {

static const char* kRFC112Format = "%a, %d %b %Y %H:%M:%S %Z";

HttpServer::HttpServer(const string &address, const uint16_t port)
    : address_(Socket::Address(address, port)),
        request_handler_(new RequestHandler()),
        shutdown_(false) {
    address_.port = port;

    // Default handlers: varz and configz for now.
    request_handler_->AddPath(
            "/varz$",
            std::bind(&HttpServer::HandleVarz, this,
                      std::placeholders::_1, std::placeholders::_2));
    request_handler_->AddPath(
            "/configz$",
            std::bind(&HttpServer::HandleConfigz, this,
                      std::placeholders::_1, std::placeholders::_2));

    Listen();
}

HttpServer::~HttpServer() {
    Stop();
}

void HttpServer::Listen() {
    listen_socket_.Listen(address_);
    listen_thread_.reset(new std::thread(&HttpServer::Start, this));
}

void HttpServer::Start() {
#if 0
    // Block all signals
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif
    LOG(INFO) << "HttpServer listening on " << address_.ToString();

    vector<std::thread> calls;
    while (!shutdown_) {
        Socket* client_socket = listen_socket_.Accept(100);
        if (client_socket == nullptr)
            continue;
        VLOG(1) << "Accepted new connection from "
                << client_socket->remote().ToString();
        try {
            // Handle the client in a separate thread and come back to accept
            // others. We need to store the std::thread object somewhere (or
            // it goes out of scope and gets destroyed).
            calls.push_back(std::thread(
                    &HttpServer::HandleClient, this, client_socket));
        } catch (std::exception &e) {
            LOG(WARNING) << e.what();
        }
    }

    LOG(INFO) << "HTTP server waiting for " << calls.size() << " threads to stop ...";
    for (std::thread& th : calls)
        th.join();
}

void HttpServer::Stop() {
    LOG(INFO) << "Stopping HttpServer";
    shutdown_ = true;
    if (listen_thread_.get())
        listen_thread_->join();
}

void HttpServer::HandleClient(Socket* _client_socket) {
    std::unique_ptr<Socket> client_socket(_client_socket);
    bool close_connection = false;
    while (!close_connection && !shutdown_) {
        const int READ_DEADLINE_MS = 3000;
        HttpRequest request;
        request.source = client_socket->remote();
        // Do 100 'read' tries to get the full http request line. Usually we'd
        // be done after the first try.
        for (int i = 0; i < 100; ++i) {
            // Read 1 line.
            string line;
            try {
                client_socket->read_buffer()->ConsumeLine(&line);
            } catch (std::out_of_range& o) {
                // Not enough data is read from the socket. Read more:
                try {
                    if (client_socket->Read(READ_DEADLINE_MS) == 0) {
                        return;    // Socket closed
                    }
                    continue;
                } catch (std::exception& e) {
                    LOG(ERROR) << "Error reading from socket: " << e.what();
                    return;
                }
            }

            // Successfully read a line.
            try {
                request.set_method(ConsumeFirstWord(&line));
                request.uri = Uri(ConsumeFirstWord(&line));
                request.set_http_version(ConsumeFirstWord(&line));
                if (request.http_version() == "HTTP/0.0") {
                    LOG(INFO) << "Bad http version: " << line;
                    return;
                }
                request.ReadAndParseHeaders(client_socket.get(), READ_DEADLINE_MS);
            } catch (std::exception &e) {
                LOG(WARNING) << "Invalid HTTP request: " << e.what();
                HttpReply reply;
                reply.StockReply(HttpReply::BAD_REQUEST);
                reply.Write(client_socket.get());
                return;
            }
            break;
        }

        uint64_t content_len = request.GetContentLength();
        if (content_len) {
            // There has been some POST data
            while (client_socket->read_buffer()->size() < content_len) {
                try {
                    if (client_socket->Read(READ_DEADLINE_MS) == 0) {
                        return;    // Socket closed
                    }
                } catch (std::exception& e) {
                    LOG(ERROR) << "Error reading from socket: " << e.what();
                    return;
                }
            }
        } else if (request.method() == "POST") {
            if (request.headers().GetHeader("Connection") != "close") {
                LOG(WARNING) << "POST request with no Content-Length and "
                                "Connection != close";
                close_connection = true;
            }
            while (client_socket->Read(READ_DEADLINE_MS) > 0) {}
        }

        if (client_socket->read_buffer()->size())
            request.mutable_body()->CopyFrom(*client_socket->read_buffer());

        HttpReply reply;
        reply.set_http_version(request.http_version());

        if (request.http_version() == "HTTP/1.1" ||
                request.headers().GetHeader(
                    "Accept-Encoding").find("chunked") != string::npos) {
            reply.set_chunked_encoding(true);
        }

        request_handler_->HandleRequest(request, &reply);

        try {
            double now = Clock::GlobalRealClock()->Now();
            // Set default required headers in the reply
            if (reply.chunked_encoding()) {
                reply.mutable_headers()->RemoveHeader("Content-Length");
                reply.mutable_headers()->AddHeader("Transfer-Encoding",
                                                   "chunked");
            } else {
                if (!reply.headers().HeaderExists("Content-Length"))
                    reply.SetContentLength(reply.body().size());
                reply.mutable_headers()->RemoveHeader("Transfer-Encoding");
            }
            if (!reply.headers().HeaderExists("Content-Type"))
                reply.SetContentType("text/html; charset=UTF-8");
            if (!reply.headers().HeaderExists("Date"))
                reply.mutable_headers()->AddHeader(
                        "Date", Clock::GmTime(now, kRFC112Format));
            if (!reply.headers().HeaderExists("Last-Modified"))
                reply.mutable_headers()->AddHeader(
                        "Last-Modified", Clock::GmTime(now, kRFC112Format));
            if (!reply.headers().HeaderExists("Server"))
                reply.mutable_headers()->AddHeader(
                        "Server", "OpenInstrument/1.0");
            if (!reply.headers().HeaderExists("X-Frame-Options"))
                reply.mutable_headers()->AddHeader(
                        "X-Frame-Options", "SAMEORIGIN");
            if (!reply.headers().HeaderExists("X-XSS-Protection"))
                reply.mutable_headers()->AddHeader(
                        "X-XSS-Protection", "1; mode=block");
            if (!reply.headers().HeaderExists("X-Frame-Options"))
                reply.mutable_headers()->AddHeader(
                        "X-Frame-Options", "deny");
            if (!close_connection && request.http_version() >= "HTTP/1.1" &&
                    request.headers().GetHeader(
                        "Connection").find("close") == string::npos) {
                reply.mutable_headers()->SetHeader("Connection", "keep-alive");
            } else {
                close_connection = true;
                reply.mutable_headers()->SetHeader("Connection", "close");
            }
        } catch (std::exception &e) {
            LOG(WARNING) << e.what();
            // Ignore errors
        }
        try {
            reply.Write(client_socket.get());
            client_socket->Flush();
            // if (reply.IsSuccess())
            //     ++stats_["/request-success"];
            // else
            //     ++stats_["/request-failure"];
        } catch (std::exception &e) {
            LOG(WARNING) << e.what();
            // ++stats_["/request-failure"];
        }
    }
}

bool HttpServer::HandleVarz(const HttpRequest &request, HttpReply* reply) {
    string result;
    GlobalExporter* g_exporter = GlobalExporter::Instance();
    const vector<Uri::RequestParam>& params = request.uri.params;

    // Expected request formats:
    //     /varz                -> show all exported variables
    //     /varz?var=xyz        -> show only the given exported var
    //     /varz?resetvar=xyz   -> reset the given exported var, if resetable
    //     /varz?resetallvars=1 -> reset all resetable exported vars
    if (params.size() == 0) {
        // "/varz"
        result = g_exporter->RenderAllStats();
    } else if (params.size() == 1) {
        if (params[0].key == "var") {
            // "/varz?var=xyz"
            result = g_exporter->RenderStat(params[0].value);
        } else if (params[0].key == "resetvar") {
            // "/varz?resetvar=xyz"
            result = g_exporter->ResetStat(params[0].value);
        } else if (params[0].key == "resetallvars") {
            // "/varz?resetallvars=1"
            result = (params[0].value == "1") ?
                        g_exporter->ResetAllStats() :
                        "To reset all variables, pass resetallvars=1";
        } else {
            result = StrCat("Bad request: invalid parameter ", params[0].value);
        }
    } else {  // params.size() > 1
        result = "Bad request: too many URL parameters with /varz";
    }

    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("text/plain");
    reply->mutable_body()->CopyFrom(result);
    return true;
}

bool HttpServer::HandleConfigz(const HttpRequest &request, HttpReply* reply) {
    string result;
    GlobalExporter* g_exporter = GlobalExporter::Instance();
    const vector<Uri::RequestParam>& params = request.uri.params;

    // Expected request formats:
    //     /configz                     -> show all config params and values
    //     /configz?param=xyz           -> show only the given config param
    //     /configz?param=xyz&value=aaa -> set the value of a config param
    if (params.size() == 0) {
        // "/configz"
        result = g_exporter->RenderAllConfigs();
    } else if (params.size() == 1) {
        // "/configz?param=xyz"
        result = (params[0].key == "param") ?
                    g_exporter->RenderConfig(params[0].value) :
                    StrCat("Bad request: invalid parameter ", params[0].value);
    } else if (params.size() == 2) {
        // "/configz?param=xyz&value=aaa"
        // The two query parameters ("param" and "value") might be re-ordered.
        if (params[0].key == "param" && params[1].key == "value") {
            // "param=xyz&value=aaa"
            result = g_exporter->SetConfig(params[0].value, params[1].value);
        } else if (params[0].key == "value" && params[1].key == "param") {
            // "value=aaa&param=xyz"
            result = g_exporter->SetConfig(params[1].value, params[0].value);
        } else {
            result = StrCat("Bad request: invalid parameters: ",
                            params[0].value, ", ", params[1].value);
        }
    } else {  // params.size() >= 3
        result = "Bad request: too many URL parameters with /varz";
    }

    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("text/plain");
    reply->mutable_body()->CopyFrom(result);
    return true;
}

}  // namespace cpp_base
