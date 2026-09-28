#include "graphics.h"

#include <algorithm>
#include <cmath>

namespace ppl {

namespace {

// 5×8 bitmap font (columns, bit 0 = top row), ASCII 32..126.
const uint8_t kFont[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x56, 0x20, 0x50}, {0x00, 0x08, 0x07, 0x03, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x2A, 0x1C, 0x7F, 0x1C, 0x2A}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x80, 0x70, 0x30, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x00, 0x60, 0x60, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x72, 0x49, 0x49, 0x49, 0x46}, {0x21, 0x41, 0x49, 0x4D, 0x33}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x31}, {0x41, 0x21, 0x11, 0x09, 0x07},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x46, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x00, 0x14, 0x00, 0x00},
    {0x00, 0x40, 0x34, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x59, 0x09, 0x06}, {0x3E, 0x41, 0x5D, 0x59, 0x4E},
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x73}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x26, 0x49, 0x49, 0x49, 0x32}, {0x03, 0x01, 0x7F, 0x01, 0x03}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x59, 0x49, 0x4D, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x41},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x41, 0x7F}, {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x03, 0x07, 0x08, 0x00}, {0x20, 0x54, 0x54, 0x78, 0x40},
    {0x7F, 0x28, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x28}, {0x38, 0x44, 0x44, 0x28, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18}, {0x00, 0x08, 0x7E, 0x09, 0x02}, {0x18, 0xA4, 0xA4, 0x9C, 0x78},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x40, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x78, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0xFC, 0x18, 0x24, 0x24, 0x18},
    {0x18, 0x24, 0x24, 0x18, 0xFC}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x24},
    {0x04, 0x04, 0x3F, 0x44, 0x24}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x4C, 0x90, 0x90, 0x90, 0x7C},
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x77, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00}, {0x02, 0x01, 0x02, 0x04, 0x02}};

char32_t fold(char32_t c)
{
    // Polish letters and common symbols → closest ASCII glyph
    switch (c) {
    case U'ą': return U'a'; case U'ć': return U'c'; case U'ę': return U'e'; case U'ł': return U'l';
    case U'ń': return U'n'; case U'ó': return U'o'; case U'ś': return U's'; case U'ź': case U'ż': return U'z';
    case U'Ą': return U'A'; case U'Ć': return U'C'; case U'Ę': return U'E'; case U'Ł': return U'L';
    case U'Ń': return U'N'; case U'Ó': return U'O'; case U'Ś': return U'S'; case U'Ź': case U'Ż': return U'Z';
    case U'π': return U'p'; case U'θ': return U'0'; case U'×': return U'x'; case U'÷': return U'/';
    case U'−': return U'-'; case U'≠': return U'#'; case U'≤': return U'<'; case U'≥': return U'>';
    case U'▶': return U'>'; case U'→': return U'>'; case U'°': return U'o'; case U'√': return U'V';
    default: return c;
    }
}

int fontScale(int font)
{
    // 0 = system (12 pt), 1..7 = 10..22 pt; the 5×8 glyph is scaled to approximate the height.
    int pt = font <= 0 ? 12 : 8 + 2 * std::clamp(font, 1, 7);
    return std::max(1, static_cast<int>(std::lround(pt / 9.0)));
}

uint32_t blend(uint32_t dst, uint32_t src, int alpha)
{
    if (alpha >= 255)
        return src;
    if (alpha <= 0)
        return dst;
    auto ch = [&](int shift) {
        int d = (dst >> shift) & 0xFF, s = (src >> shift) & 0xFF;
        return static_cast<uint32_t>((s * alpha + d * (255 - alpha)) / 255) << shift;
    };
    return ch(16) | ch(8) | ch(0);
}

uint32_t crcTable[256];
bool crcInit = false;
uint32_t crc32(const std::string &data, uint32_t crc = 0)
{
    if (!crcInit) {
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k)
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            crcTable[n] = c;
        }
        crcInit = true;
    }
    crc = ~crc;
    for (unsigned char b : data)
        crc = crcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

void put32(std::string &s, uint32_t v)
{
    s.push_back(static_cast<char>(v >> 24));
    s.push_back(static_cast<char>(v >> 16));
    s.push_back(static_cast<char>(v >> 8));
    s.push_back(static_cast<char>(v));
}

} // namespace

Graphics::Graphics() { reset(); }

void Graphics::reset()
{
    for (auto &g : m_grobs)
        g = Grob();
    dimension(0, ScreenWidth, ScreenHeight, 0xFFFFFF);
}

Grob &Graphics::grob(int index)
{
    return m_grobs[static_cast<size_t>(std::clamp(index, 0, 9))];
}

const Grob &Graphics::grob(int index) const
{
    return m_grobs[static_cast<size_t>(std::clamp(index, 0, 9))];
}

void Graphics::dimension(int g, int w, int h, uint32_t color)
{
    Grob &b = grob(g);
    b.width = std::clamp(w, 0, 4096);
    b.height = std::clamp(h, 0, 4096);
    b.pixels.assign(static_cast<size_t>(b.width) * b.height, color);
    touched(g);
}

void Graphics::fill(int g, uint32_t color)
{
    Grob &b = grob(g);
    std::fill(b.pixels.begin(), b.pixels.end(), color);
    touched(g);
}

void Graphics::plot(Grob &b, int x, int y, uint32_t c, int alpha)
{
    if (!inside(b, x, y))
        return;
    uint32_t &p = b.pixels[static_cast<size_t>(y) * b.width + x];
    p = blend(p, c, alpha);
}

void Graphics::hspan(Grob &b, int x1, int x2, int y, uint32_t c, int alpha)
{
    if (y < 0 || y >= b.height)
        return;
    if (x1 > x2)
        std::swap(x1, x2);
    x1 = std::max(x1, 0);
    x2 = std::min(x2, b.width - 1);
    for (int x = x1; x <= x2; ++x)
        plot(b, x, y, c, alpha);
}

void Graphics::setPixel(int g, int x, int y, uint32_t color, int alpha)
{
    plot(grob(g), x, y, color, alpha);
    touched(g);
}

uint32_t Graphics::pixel(int g, int x, int y) const
{
    const Grob &b = grob(g);
    if (!inside(b, x, y))
        return 0;
    return b.pixels[static_cast<size_t>(y) * b.width + x];
}

void Graphics::line(int g, double fx1, double fy1, double fx2, double fy2, uint32_t color)
{
    Grob &b = grob(g);
    int x1 = static_cast<int>(std::lround(fx1)), y1 = static_cast<int>(std::lround(fy1));
    int x2 = static_cast<int>(std::lround(fx2)), y2 = static_cast<int>(std::lround(fy2));
    // clip absurd coordinates
    auto clampc = [](int v) { return std::clamp(v, -20000, 20000); };
    x1 = clampc(x1); y1 = clampc(y1); x2 = clampc(x2); y2 = clampc(y2);
    int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    for (int guard = 0; guard < 100000; ++guard) {
        plot(b, x1, y1, color, 255);
        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
    touched(g);
}

void Graphics::rect(int g, int x1, int y1, int x2, int y2, uint32_t edge, uint32_t fillColor, bool filled)
{
    Grob &b = grob(g);
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    for (int y = std::max(y1, 0); y <= std::min(y2, b.height - 1); ++y) {
        if (y == y1 || y == y2) {
            hspan(b, x1, x2, y, edge, 255);
        } else {
            plot(b, x1, y, edge, 255);
            plot(b, x2, y, edge, 255);
            if (filled && x2 - x1 > 1)
                hspan(b, x1 + 1, x2 - 1, y, fillColor, 255);
        }
    }
    touched(g);
}

void Graphics::fillPolygon(int g, const std::vector<std::pair<double, double>> &pts, uint32_t color, int alpha)
{
    Grob &b = grob(g);
    if (pts.size() < 3)
        return;
    double minY = pts[0].second, maxY = pts[0].second;
    for (const auto &p : pts) {
        minY = std::min(minY, p.second);
        maxY = std::max(maxY, p.second);
    }
    int y0 = std::max(0, static_cast<int>(std::floor(minY)));
    int y1 = std::min(b.height - 1, static_cast<int>(std::ceil(maxY)));
    std::vector<double> xs;
    for (int y = y0; y <= y1; ++y) {
        double sy = y + 0.5;
        xs.clear();
        for (size_t i = 0; i < pts.size(); ++i) {
            auto [ax, ay] = pts[i];
            auto [bx, by] = pts[(i + 1) % pts.size()];
            if ((ay <= sy && by > sy) || (by <= sy && ay > sy))
                xs.push_back(ax + (sy - ay) * (bx - ax) / (by - ay));
        }
        std::sort(xs.begin(), xs.end());
        for (size_t k = 0; k + 1 < xs.size(); k += 2)
            hspan(b, static_cast<int>(std::ceil(xs[k] - 0.5)), static_cast<int>(std::floor(xs[k + 1] - 0.5)), y, color, alpha);
    }
    touched(g);
}

void Graphics::arc(int g, double cx, double cy, double rx, double ry, double a1, double a2, bool full, uint32_t edge,
                   uint32_t fillColor, bool filled)
{
    Grob &b = grob(g);
    rx = std::fabs(rx);
    ry = std::fabs(ry);
    if (full) {
        a1 = 0;
        a2 = 2 * M_PI;
    }
    while (a2 < a1)
        a2 += 2 * M_PI;
    int steps = std::max(16, static_cast<int>((rx + ry) * (a2 - a1)));
    std::vector<std::pair<double, double>> pts;
    for (int i = 0; i <= steps; ++i) {
        double a = a1 + (a2 - a1) * i / steps;
        pts.push_back({cx + rx * std::cos(a), cy - ry * std::sin(a)}); // screen y grows downwards
    }
    if (filled) {
        std::vector<std::pair<double, double>> poly = pts;
        if (!full)
            poly.push_back({cx, cy});
        fillPolygon(g, poly, fillColor, 255);
    }
    for (size_t i = 1; i < pts.size(); ++i)
        line(g, pts[i - 1].first, pts[i - 1].second, pts[i].first, pts[i].second, edge);
    (void)b;
    touched(g);
}

void Graphics::triangle(int g, const double x[3], const double y[3], const uint32_t c[3], bool gradient, int alpha)
{
    Grob &b = grob(g);
    double minX = std::min({x[0], x[1], x[2]}), maxX = std::max({x[0], x[1], x[2]});
    double minY = std::min({y[0], y[1], y[2]}), maxY = std::max({y[0], y[1], y[2]});
    double den = (y[1] - y[2]) * (x[0] - x[2]) + (x[2] - x[1]) * (y[0] - y[2]);
    if (den == 0)
        return;
    for (int py = std::max(0, static_cast<int>(minY)); py <= std::min(b.height - 1, static_cast<int>(maxY)); ++py) {
        for (int px = std::max(0, static_cast<int>(minX)); px <= std::min(b.width - 1, static_cast<int>(maxX)); ++px) {
            double sx = px + 0.5, sy = py + 0.5;
            double w0 = ((y[1] - y[2]) * (sx - x[2]) + (x[2] - x[1]) * (sy - y[2])) / den;
            double w1 = ((y[2] - y[0]) * (sx - x[2]) + (x[0] - x[2]) * (sy - y[2])) / den;
            double w2 = 1 - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0)
                continue;
            uint32_t col = c[0];
            if (gradient) {
                auto ch = [&](int s) {
                    double v = w0 * ((c[0] >> s) & 0xFF) + w1 * ((c[1] >> s) & 0xFF) + w2 * ((c[2] >> s) & 0xFF);
                    return static_cast<uint32_t>(std::clamp(static_cast<int>(std::lround(v)), 0, 255)) << s;
                };
                col = ch(16) | ch(8) | ch(0);
            }
            plot(b, px, py, col, alpha);
        }
    }
    touched(g);
}

int Graphics::textWidth(const std::u32string &text, int font) const
{
    return static_cast<int>(text.size()) * 6 * fontScale(font);
}

int Graphics::text(int g, int x, int y, const std::u32string &str, int font, uint32_t color, int width,
                   uint32_t background, bool hasBackground)
{
    Grob &b = grob(g);
    int s = fontScale(font);
    int w = textWidth(str, font);
    int limit = width > 0 ? x + width : x + w;
    if (hasBackground)
        rect(g, x, y, std::min(x + w, limit) - 1, y + 9 * s - 1, background, background, true);
    int cx = x;
    for (char32_t ch : str) {
        char32_t c = fold(ch);
        if (c < 32 || c > 126)
            c = U'?';
        const uint8_t *glyph = kFont[c - 32];
        for (int col = 0; col < 5; ++col) {
            for (int row = 0; row < 8; ++row) {
                if (!(glyph[col] >> row & 1))
                    continue;
                for (int dy = 0; dy < s; ++dy)
                    for (int dx = 0; dx < s; ++dx) {
                        int px = cx + col * s + dx;
                        if (px < limit)
                            plot(b, px, y + row * s + dy, color, 255);
                    }
            }
        }
        cx += 6 * s;
        if (cx >= limit)
            break;
    }
    touched(g);
    return std::min(cx, limit);
}

void Graphics::invert(int g, int x1, int y1, int x2, int y2)
{
    Grob &b = grob(g);
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    for (int y = std::max(0, y1); y <= std::min(b.height - 1, y2); ++y)
        for (int x = std::max(0, x1); x <= std::min(b.width - 1, x2); ++x) {
            uint32_t &p = b.pixels[static_cast<size_t>(y) * b.width + x];
            p = (~p) & 0xFFFFFF;
        }
    touched(g);
}

void Graphics::blit(int dst, int dx1, int dy1, int dx2, int dy2, int src, int sx1, int sy1, int sx2, int sy2,
                    bool hasTransparent, uint32_t transparent, int alpha)
{
    const Grob s = grob(src); // copy: source may equal destination
    Grob &d = grob(dst);
    int sw = sx2 - sx1, sh = sy2 - sy1;
    int dw = dx2 - dx1, dh = dy2 - dy1;
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;
    for (int y = 0; y < dh; ++y) {
        int ty = dy1 + y;
        if (ty < 0 || ty >= d.height)
            continue;
        int syy = sy1 + y * sh / dh;
        for (int x = 0; x < dw; ++x) {
            int tx = dx1 + x;
            if (tx < 0 || tx >= d.width)
                continue;
            int sxx = sx1 + x * sw / dw;
            if (!inside(s, sxx, syy))
                continue;
            uint32_t c = s.pixels[static_cast<size_t>(syy) * s.width + sxx];
            if (hasTransparent && c == transparent)
                continue;
            plot(d, tx, ty, c, alpha);
        }
    }
    touched(dst);
}

void Graphics::copyRegion(int src, int x1, int y1, int x2, int y2, int dst)
{
    const Grob s = grob(src);
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    x1 = std::clamp(x1, 0, s.width);
    x2 = std::clamp(x2, 0, s.width);
    y1 = std::clamp(y1, 0, s.height);
    y2 = std::clamp(y2, 0, s.height);
    Grob &d = grob(dst);
    d.width = x2 - x1;
    d.height = y2 - y1;
    d.pixels.assign(static_cast<size_t>(d.width) * d.height, 0xFFFFFF);
    for (int y = 0; y < d.height; ++y)
        for (int x = 0; x < d.width; ++x)
            d.pixels[static_cast<size_t>(y) * d.width + x] = s.pixels[static_cast<size_t>(y + y1) * s.width + x + x1];
    touched(dst);
}

std::string Graphics::png(int g, int scale) const
{
    const Grob &b = grob(g);
    scale = std::clamp(scale, 1, 8);
    int w = std::max(1, b.width * scale), h = std::max(1, b.height * scale);
    std::string raw;
    raw.reserve(static_cast<size_t>(h) * (w * 3 + 1));
    for (int y = 0; y < h; ++y) {
        raw.push_back(0); // filter: none
        for (int x = 0; x < w; ++x) {
            uint32_t c = b.width ? b.pixels[static_cast<size_t>(y / scale) * b.width + x / scale] : 0xFFFFFF;
            raw.push_back(static_cast<char>(c >> 16));
            raw.push_back(static_cast<char>(c >> 8));
            raw.push_back(static_cast<char>(c));
        }
    }
    // zlib stream with stored (uncompressed) deflate blocks
    std::string z = "\x78\x01";
    size_t pos = 0;
    do {
        size_t len = std::min<size_t>(65535, raw.size() - pos);
        bool last = pos + len >= raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<char>(len & 0xFF));
        z.push_back(static_cast<char>(len >> 8));
        z.push_back(static_cast<char>(~len & 0xFF));
        z.push_back(static_cast<char>((~len >> 8) & 0xFF));
        z.append(raw, pos, len);
        pos += len;
    } while (pos < raw.size());
    uint32_t a = 1, bsum = 0;
    for (unsigned char c : raw) {
        a = (a + c) % 65521;
        bsum = (bsum + a) % 65521;
    }
    put32(z, (bsum << 16) | a);

    auto chunk = [](std::string &out, const char *type, const std::string &data) {
        put32(out, static_cast<uint32_t>(data.size()));
        std::string td = std::string(type, 4) + data;
        out += td;
        put32(out, crc32(td));
    };
    std::string pngData = "\x89PNG\r\n\x1a\n";
    std::string ihdr;
    put32(ihdr, static_cast<uint32_t>(w));
    put32(ihdr, static_cast<uint32_t>(h));
    ihdr += std::string("\x08\x02\x00\x00\x00", 5); // 8-bit RGB
    chunk(pngData, "IHDR", ihdr);
    chunk(pngData, "IDAT", z);
    chunk(pngData, "IEND", "");
    return pngData;
}

std::string Graphics::rle(int g) const
{
    const Grob &b = grob(g);
    std::string out;
    size_t n = b.pixels.size();
    for (size_t i = 0; i < n;) {
        uint32_t c = b.pixels[i];
        size_t j = i + 1;
        while (j < n && b.pixels[j] == c && j - i < 65535)
            ++j;
        size_t count = j - i;
        out.push_back(static_cast<char>(count & 0xFF));
        out.push_back(static_cast<char>(count >> 8));
        out.push_back(static_cast<char>(c >> 16));
        out.push_back(static_cast<char>(c >> 8));
        out.push_back(static_cast<char>(c));
        i = j;
    }
    return out;
}

std::string base64(const std::string &data)
{
    static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        uint32_t v = (static_cast<unsigned char>(data[i]) << 16) | (static_cast<unsigned char>(data[i + 1]) << 8)
                     | static_cast<unsigned char>(data[i + 2]);
        out += t[v >> 18];
        out += t[(v >> 12) & 63];
        out += t[(v >> 6) & 63];
        out += t[v & 63];
    }
    if (i < data.size()) {
        uint32_t v = static_cast<unsigned char>(data[i]) << 16;
        if (i + 1 < data.size())
            v |= static_cast<unsigned char>(data[i + 1]) << 8;
        out += t[v >> 18];
        out += t[(v >> 12) & 63];
        out += i + 1 < data.size() ? t[(v >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

} // namespace ppl
