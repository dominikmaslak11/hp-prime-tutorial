#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ppl {

struct Grob {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;   // 0xRRGGBB
};

// Screen (G0, 320×240) and graphic buffers G1–G9.
class Graphics {
public:
    static constexpr int ScreenWidth = 320;
    static constexpr int ScreenHeight = 240;

    Graphics();
    void reset();

    Grob &grob(int index);
    const Grob &grob(int index) const;
    uint64_t version() const { return m_version; }   // changes whenever G0 changes

    void dimension(int g, int w, int h, uint32_t color);
    void fill(int g, uint32_t color);
    void setPixel(int g, int x, int y, uint32_t color, int alpha = 255);
    uint32_t pixel(int g, int x, int y) const;
    void line(int g, double x1, double y1, double x2, double y2, uint32_t color);
    void rect(int g, int x1, int y1, int x2, int y2, uint32_t edge, uint32_t fillColor, bool filled);
    void fillPolygon(int g, const std::vector<std::pair<double, double>> &pts, uint32_t color, int alpha);
    // Angles in radians, counter-clockwise; full = whole ellipse.
    void arc(int g, double cx, double cy, double rx, double ry, double a1, double a2, bool full, uint32_t edge,
             uint32_t fillColor, bool filled);
    void triangle(int g, const double x[3], const double y[3], const uint32_t c[3], bool gradient, int alpha);
    int text(int g, int x, int y, const std::u32string &text, int font, uint32_t color, int width, uint32_t background,
             bool hasBackground);
    int textWidth(const std::u32string &text, int font) const;
    void invert(int g, int x1, int y1, int x2, int y2);
    void blit(int dst, int dx1, int dy1, int dx2, int dy2, int src, int sx1, int sy1, int sx2, int sy2,
              bool hasTransparent, uint32_t transparent, int alpha);
    void copyRegion(int src, int x1, int y1, int x2, int y2, int dst);

    // PNG image of a graphic (scale ≥ 1, nearest neighbour).
    std::string png(int g, int scale = 1) const;
    // Run-length encoded RGB: [count:u16 little endian][r][g][b]… — compact transport for the VS Code panel.
    std::string rle(int g) const;

private:
    std::array<Grob, 10> m_grobs;
    uint64_t m_version = 1;
    void touched(int g)
    {
        if (g == 0)
            ++m_version;
    }
    bool inside(const Grob &b, int x, int y) const { return x >= 0 && y >= 0 && x < b.width && y < b.height; }
    void plot(Grob &b, int x, int y, uint32_t c, int alpha);
    void hspan(Grob &b, int x1, int x2, int y, uint32_t c, int alpha);
};

std::string base64(const std::string &data);

} // namespace ppl
