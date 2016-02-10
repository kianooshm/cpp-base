/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "cpp-base/http/request_handler.h"

using std::string;

namespace cpp_base {

void RequestHandler::AddPath(
        const string& path,
        std::function<bool(const HttpRequest &, HttpReply *)> handler) {
    CHECK(!path.empty());
    Handler h;
    h.path = path;
    h.callback = handler;
    handlers_.push_back(h);
}

void RequestHandler::HandleRequest(const HttpRequest &req, HttpReply *reply) {
    VLOG(1) << "Received request for \"" << req.uri.Assemble() << "\"";
    for (size_t i = 0; i < req.headers().size(); ++i) {
        VLOG(2) << "\t\"" << req.headers()[i].name
                << "\": \"" << req.headers()[i].value << "\"";
    }

    // Find a callback for the requested path
    for (size_t i = 0; i < handlers_.size(); ++i) {
        const string& path = handlers_[i].path;
        if (EndsWith(path, "$")) {
            // Require full match
            if (req.uri.path != path.substr(0, path.size() - 1))
                continue;
        } else {
            // Just substring match
            if (!StartsWith(req.uri.path, handlers_[i].path))
                continue;
        }

        // Found the handler. Run the callback.
        reply->SetStatus(HttpReply::OK);
        try {
            if (!handlers_[i].callback(req, reply)) {
                LOG(WARNING) << "Callback for " << req.uri.path
                             << " returned false";
                reply->StockReply(HttpReply::INTERNAL_SERVER_ERROR);
            }
        } catch (std::exception &e) {
            LOG(ERROR) << "Callback for " << req.uri.path
                       << " threw an exception: " << e.what();
            reply->StockReply(HttpReply::INTERNAL_SERVER_ERROR);
        }
        return;
    }

    LOG(INFO) << "Could not find handler for path \""
              << req.uri.path << "\", returning 404";
    reply->StockReply(HttpReply::NOT_FOUND);
}

}  // namespace cpp_base
