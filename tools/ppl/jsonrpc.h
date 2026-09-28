#pragma once

#include "json.h"

#include <optional>
#include <string>

namespace ppl::rpc {

// Transport framing: LSP uses "Content-Length" headers, MCP (stdio) uses one JSON message per line.
enum class Framing { ContentLength, NewlineDelimited };

void setupStdio();

// Returns std::nullopt at end of input. Malformed JSON yields a Json with type Null and sets *parseError.
std::optional<Json> readMessage(Framing framing, bool *parseError);
void writeMessage(Framing framing, const Json &message);

Json makeResponse(const Json &id, Json result);
Json makeError(const Json &id, int code, const std::string &message);
Json makeNotification(const std::string &method, Json params);

} // namespace ppl::rpc
