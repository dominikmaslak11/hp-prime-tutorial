#include "jsonrpc.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace ppl::rpc {

void setupStdio()
{
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
}

namespace {

bool readLine(std::string &line)
{
    line.clear();
    int c;
    while ((c = std::fgetc(stdin)) != EOF) {
        if (c == '\n')
            return true;
        line.push_back(static_cast<char>(c));
    }
    return !line.empty();
}

int strncasecmp_portable(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        int ca = static_cast<unsigned char>(a[i]);
        int cb = static_cast<unsigned char>(b[i]);
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb || ca == 0)
            return ca - cb;
    }
    return 0;
}

} // namespace

std::optional<Json> readMessage(Framing framing, bool *parseError)
{
    if (parseError)
        *parseError = false;
    std::string line;
    if (framing == Framing::NewlineDelimited) {
        while (true) {
            if (!readLine(line))
                return std::nullopt;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.find_first_not_of(" \t") == std::string::npos)
                continue;
            std::string e;
            Json j = Json::parse(line, &e);
            if (!e.empty() && parseError)
                *parseError = true;
            return j;
        }
    }
    size_t length = 0;
    bool haveLength = false;
    while (true) {
        if (!readLine(line))
            return std::nullopt;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty()) {
            if (haveLength)
                break;
            continue;
        }
        const char *key = "Content-Length:";
        if (line.size() > std::strlen(key) && strncasecmp_portable(line.c_str(), key, std::strlen(key)) == 0) {
            length = static_cast<size_t>(std::strtoul(line.c_str() + std::strlen(key), nullptr, 10));
            haveLength = true;
        }
    }
    std::string body(length, '\0');
    size_t got = std::fread(body.data(), 1, length, stdin);
    if (got != length)
        return std::nullopt;
    std::string e;
    Json j = Json::parse(body, &e);
    if (!e.empty() && parseError)
        *parseError = true;
    return j;
}

void writeMessage(Framing framing, const Json &message)
{
    std::string body = message.dump();
    if (framing == Framing::ContentLength) {
        std::string header = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
        std::fwrite(header.data(), 1, header.size(), stdout);
        std::fwrite(body.data(), 1, body.size(), stdout);
    } else {
        body.push_back('\n');
        std::fwrite(body.data(), 1, body.size(), stdout);
    }
    std::fflush(stdout);
}

Json makeResponse(const Json &id, Json result)
{
    return Json::object({{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}});
}

Json makeError(const Json &id, int code, const std::string &message)
{
    return Json::object(
        {{"jsonrpc", "2.0"}, {"id", id}, {"error", Json::object({{"code", code}, {"message", message}})}});
}

Json makeNotification(const std::string &method, Json params)
{
    return Json::object({{"jsonrpc", "2.0"}, {"method", method}, {"params", std::move(params)}});
}

} // namespace ppl::rpc
