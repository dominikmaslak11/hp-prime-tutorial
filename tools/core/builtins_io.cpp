// Built-in commands: input/output, keyboard, touch, time and graphics.
#include "builtins.h"
#include "interpreter.h"
#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace ppl {

namespace {

std::string u8(const std::u32string &s) { return toUtf8(s); }

uint32_t colorOf(const Value &v)
{
    if (v.type == Value::Type::Integer)
        return static_cast<uint32_t>(v.ival) & 0xFFFFFF;
    if (v.isNumber())
        return static_cast<uint32_t>(static_cast<int64_t>(v.toReal())) & 0xFFFFFF;
    throw RuntimeError("Kolor musi być liczbą, np. RGB(255,0,0) albo #FF0000h.");
}

// Argument cursor for graphics commands: optional leading G, Cartesian → pixel conversion.
struct GArgs {
    CallArgs &a;
    bool cartesian;
    int g = 0;
    size_t i = 0;

    GArgs(CallArgs &args, bool cart, bool grobFirst = true) : a(args), cartesian(cart)
    {
        if (grobFirst && a.size() > 0 && a.values[0].type == Value::Type::Graphic) {
            g = a.values[0].grob;
            i = 1;
        }
    }
    size_t left() const { return a.size() - i; }
    bool present() const { return i < a.size() && a.exprs[i] != nullptr; }
    const Value &next() { return a.values.at(i++); }
    double num() { return next().toReal(u8(a.name).c_str()); }
    double x(double v) const
    {
        if (!cartesian)
            return v;
        double xmin = a.in.getVariable(U"Xmin").toReal(), xmax = a.in.getVariable(U"Xmax").toReal();
        return (v - xmin) / (xmax - xmin) * 318.0;
    }
    double y(double v) const
    {
        if (!cartesian)
            return v;
        double ymin = a.in.getVariable(U"Ymin").toReal(), ymax = a.in.getVariable(U"Ymax").toReal();
        return (ymax - v) / (ymax - ymin) * 218.0;
    }
    int px() { return static_cast<int>(std::lround(x(num()))); }
    int py() { return static_cast<int>(std::lround(y(num()))); }
};

std::pair<double, double> pointOf(const Value &v)
{
    if (v.type == Value::Type::Complex || v.type == Value::Type::Real)
        return {v.toComplex().real(), v.toComplex().imag()};
    if (v.isList() && v.items().size() >= 2)
        return {v.items()[0].toReal(), v.items()[1].toReal()};
    if (v.isMatrix() && v.mat->data.size() >= 2)
        return {v.mat->data[0].toReal(), v.mat->data[1].toReal()};
    throw RuntimeError("Punkt musi mieć postać (x,y) albo {x,y}.");
}

void registerGraphicsPair(BuiltinTable &t, const std::u32string &name, int minA, int maxA,
                          std::function<Value(CallArgs &, bool cartesian)> f)
{
    // drawing commands answer 1 on the calculator (measured on the emulator)
    bool answersOne = name != U"GETPIX" && name != U"GROBW" && name != U"GROBH";
    auto wrap = [f, answersOne](bool cart) {
        return [f, answersOne, cart](CallArgs &a) {
            Value v = f(a, cart);
            return answersOne && v.isReal() && v.re == 0 ? Value::real(1) : v;
        };
    };
    t[name] = Builtin{minA, maxA, false, wrap(true)};
    t[name + U"_P"] = Builtin{minA, maxA, false, wrap(false)};
}

} // namespace

void registerIoBuiltins(Interpreter &, BuiltinTable &t)
{
    // ------------------------------------------------------------------ text output
    t[U"PRINT"] = {0, 1, false, [](CallArgs &a) {
                       if (a.size() == 0)
                           a.in.host().clearTerminal();
                       else
                           a.in.host().print(a.in.format(a[0], false));
                       return Value::real(0);
                   }};
    t[U"MSGBOX"] = {1, 2, false, [](CallArgs &a) {
                        bool okCancel = a.size() > 1 && a[1].truthy();
                        return Value::real(a.in.host().messageBox(a.in.format(a[0], false), okCancel) ? 1 : 0);
                    }};

    // ------------------------------------------------------------------ dialogs
    t[U"INPUT"] = {1, 6, true, [](CallArgs &a) {
                       Interpreter &in = a.in;
                       auto evalOpt = [&](size_t i) { return a.has(i) ? in.eval(*a.exprs[i]) : Value(); };
                       struct Target {
                           std::u32string var;
                           InputField field;
                       };
                       std::vector<Target> targets;
                       auto addSpec = [&](const Expr &spec) {
                           Target tg;
                           if (spec.kind == ExprKind::Ident) {
                               tg.var = spec.text;
                           } else if (spec.kind == ExprKind::List && !spec.args.empty() && spec.args[0]->kind == ExprKind::Ident) {
                               tg.var = spec.args[0]->text;
                               if (spec.args.size() > 1) {
                                   Value kind = in.eval(*spec.args[1]);
                                   if (kind.isList()) {
                                       tg.field.kind = InputField::Kind::Choose;
                                       for (const auto &c : kind.items())
                                           tg.field.choices.push_back(in.format(c, false));
                                   } else if (kind.isNumber()) {
                                       tg.field.kind = InputField::Kind::Check;
                                       tg.field.radioGroup = kind.toInt();
                                   }
                               }
                           } else {
                               throw RuntimeError("INPUT: oczekiwano nazwy zmiennej albo listy {zmienna, …}.");
                           }
                           targets.push_back(tg);
                       };
                       const Expr &first = *a.exprs[0];
                       bool multi = first.kind == ExprKind::List
                                    && !(first.args.size() >= 2 && first.args[0]->kind == ExprKind::Ident
                                         && first.args[1]->kind != ExprKind::Ident && first.args[1]->kind != ExprKind::List
                                         && false);
                       if (first.kind == ExprKind::List)
                           for (const auto &e : first.args)
                               addSpec(*e);
                       else
                           addSpec(first);
                       (void)multi;
                       InputRequest req;
                       Value title = evalOpt(1), labels = evalOpt(2), helps = evalOpt(3), inits = evalOpt(5);
                       if (title.isList() && !title.items().empty())
                           title = title.items()[0];
                       if (title.isString())
                           req.title = title.str;
                       auto pick = [&](const Value &v, size_t k) -> Value {
                           if (v.isList())
                               return k < v.items().size() ? v.items()[k] : Value();
                           return k == 0 ? v : Value();
                       };
                       for (size_t k = 0; k < targets.size(); ++k) {
                           InputField &f = targets[k].field;
                           Value lab = pick(labels, k), hel = pick(helps, k), ini = pick(inits, k);
                           f.label = lab.isString() ? lab.str : targets[k].var + U":";
                           f.help = hel.isString() ? hel.str : U"";
                           Value cur;
                           bool hasInit = inits.isList() ? k < inits.items().size() : (a.has(5) && k == 0);
                           if (hasInit)
                               cur = ini;
                           else if (Value *v = in.findVariable(targets[k].var))
                               cur = *v;
                           else
                               cur = Value::real(0);
                           if (f.kind == InputField::Kind::Edit)
                               f.text = cur.isString() ? U"\"" + cur.str + U"\"" : in.format(cur);
                           else
                               f.number = cur.isNumber() ? cur.toInt() : 0;
                           req.fields.push_back(f);
                       }
                       if (!in.host().input(req))
                           return Value::real(0);
                       for (size_t k = 0; k < targets.size(); ++k) {
                           const InputField &f = req.fields[k];
                           Value v;
                           if (f.kind == InputField::Kind::Edit) {
                               std::u32string txt = f.text;
                               if (txt.empty())
                                   continue;
                               v = in.evalText(txt);
                           } else {
                               v = Value::real(f.number);
                           }
                           Expr target;
                           target.kind = ExprKind::Ident;
                           target.text = targets[k].var;
                           in.assign(target, v);
                       }
                       return Value::real(1);
                   }};
    t[U"CHOOSE"] = {2, -1, true, [](CallArgs &a) {
                        Interpreter &in = a.in;
                        if (a.exprs[0]->kind != ExprKind::Ident)
                            throw RuntimeError("CHOOSE: pierwszy argument musi być nazwą zmiennej.");
                        std::u32string title = in.format(in.eval(*a.exprs[1]), false);
                        std::vector<std::u32string> items;
                        if (a.size() == 3) {
                            Value v = in.eval(*a.exprs[2]);
                            if (v.isList()) {
                                for (const auto &x : v.items())
                                    items.push_back(in.format(x, false));
                            } else {
                                items.push_back(in.format(v, false));
                            }
                        } else {
                            for (size_t i = 2; i < a.size(); ++i)
                                items.push_back(in.format(in.eval(*a.exprs[i]), false));
                        }
                        int choice = in.host().choose(title, items);
                        Expr target;
                        target.kind = ExprKind::Ident;
                        target.text = a.exprs[0]->text;
                        in.assign(target, Value::real(choice));
                        return Value::real(choice > 0 ? 1 : 0);
                    }};
    auto editor = [](const char *what) {
        return Builtin{1, 3, false, [what](CallArgs &a) {
                           a.in.host().notice(toU32(std::string("[") + what + "] Edytor nie jest symulowany; wartość bez zmian: ")
                                              + a.in.format(a[0]));
                           return a[0];
                       }};
    };
    t[U"EDITLIST"] = editor("EDITLIST");
    t[U"EDITMAT"] = editor("EDITMAT");

    // ------------------------------------------------------------------ keyboard, touch, time
    t[U"GETKEY"] = {0, 0, false, [](CallArgs &a) { return Value::real(a.in.host().getKey()); }};
    t[U"ISKEYDOWN"] = {1, 1, false, [](CallArgs &a) { return Value::real(a.in.host().isKeyDown(a.integer(0)) ? 1 : 0); }};
    t[U"MOUSE"] = {0, 1, false, [](CallArgs &a) {
                       TouchPoint p = a.in.host().touch();
                       ValueList first;
                       if (p.active)
                           first = {Value::real(p.x), Value::real(p.y), Value::real(p.x0), Value::real(p.y0), Value::real(p.type)};
                       if (a.size() == 1) {
                           int k = a.integer(0);
                           if (!p.active || k < 0 || k >= 5)
                               return Value::real(-1);
                           return first[k];
                       }
                       return Value::makeList({Value::makeList(first), Value::makeList()});
                   }};
    t[U"WAIT"] = {0, 1, false, [](CallArgs &a) {
                      double s = a.size() ? a.num(0) : 0;
                      if (s <= 0)
                          return Value::real(a.in.host().waitForEvent(s < 0 ? -1 : 0));
                      a.in.host().sleep(s);
                      return Value::real(0);
                  }};
    t[U"FREEZE"] = {0, 0, false, [](CallArgs &a) {
                        a.in.host().screenChanged();
                        a.in.host().waitForEvent(0);
                        return Value::real(1);
                    }};
    t[U"TICKS"] = {0, 0, false, [](CallArgs &a) { return Value::real(std::floor(a.in.host().ticks())); }};
    t[U"TEVAL"] = {1, 1, true, [](CallArgs &a) {
                       double t0 = a.in.host().ticks();
                       a.in.eval(*a.exprs[0]);
                       return Value::real((a.in.host().ticks() - t0) / 1000.0);
                   }};

    // ------------------------------------------------------------------ graphics
    t[U"RGB"] = {3, 4, false, [](CallArgs &a) {
                     int r = std::clamp(a.integer(0), 0, 255), g = std::clamp(a.integer(1), 0, 255), b = std::clamp(a.integer(2), 0, 255);
                     int64_t c = (static_cast<int64_t>(r) << 16) | (g << 8) | b;
                     if (a.size() > 3) // alpha 0..255, stored in bits 24..31
                         c |= static_cast<int64_t>(std::clamp(a.integer(3), 0, 255)) << 24;
                     return Value::integer(c, 'h');
                 }};
    registerGraphicsPair(t, U"RECT", 0, 7, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        Graphics &gr = a.in.graphics();
        const Grob &b = gr.grob(g.g);
        size_t n = g.left();
        int x1 = 0, y1 = 0, x2 = b.width - 1, y2 = b.height - 1;
        uint32_t edge = 0xFFFFFF, fill = 0xFFFFFF;
        if (n == 1) {
            edge = fill = colorOf(g.next());
        } else if (n >= 2) {
            x1 = g.px();
            y1 = g.py();
            if (n == 3) {
                edge = fill = colorOf(g.next());
            } else if (n >= 4) {
                x2 = g.px();
                y2 = g.py();
                if (n >= 5)
                    edge = fill = colorOf(g.next());
                if (n >= 6)
                    fill = colorOf(g.next());
            }
        }
        gr.rect(g.g, x1, y1, x2, y2, edge, fill, true);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"LINE", 2, 7, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        if (g.left() < 4 || (g.present() && (a.values[g.i].isList() || a.values[g.i].isMatrix() || a.values[g.i].isString())))
            a.in.unsupported(U"Zaawansowana forma " + a.name + U" (wiele linii / 3D)");
        double x1 = g.x(g.num()), y1 = g.y(g.num()), x2 = g.x(g.num()), y2 = g.y(g.num());
        uint32_t c = g.left() ? colorOf(g.next()) : 0;
        a.in.graphics().line(g.g, x1, y1, x2, y2, c);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"PIXON", 2, 4, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        int x = g.px(), y = g.py();
        uint32_t c = g.left() ? colorOf(g.next()) : 0;
        a.in.graphics().setPixel(g.g, x, y, c);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"PIXOFF", 2, 3, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        int x = g.px(), y = g.py();
        a.in.graphics().setPixel(g.g, x, y, 0xFFFFFF);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"GETPIX", 2, 3, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        int x = g.px(), y = g.py();
        return Value::integer(a.in.graphics().pixel(g.g, x, y), 'h');
    });
    registerGraphicsPair(t, U"ARC", 3, 7, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        double cx = g.x(g.num()), cy = g.y(g.num());
        const Value &r = g.next();
        double rx, ry;
        if (r.isList() && r.items().size() >= 2) {
            rx = r.items()[0].toReal();
            ry = r.items()[1].toReal();
        } else {
            rx = ry = r.toReal("ARC");
        }
        size_t n = g.left();
        bool full = true;
        double a1 = 0, a2 = 2 * M_PI;
        uint32_t edge = 0, fill = 0;
        bool filled = false;
        auto readColor = [&](const Value &c) {
            if (c.isList() && c.items().size() >= 2) {
                edge = colorOf(c.items()[0]);
                fill = colorOf(c.items()[1]);
                filled = true;
            } else {
                edge = colorOf(c);
            }
        };
        if (n == 1) {
            readColor(g.next());
        } else if (n >= 2) {
            bool hasA2 = a.has(g.i + 1);
            a1 = a.in.angleToRadians(g.num());
            double raw2 = hasA2 ? g.num() : (g.i++, 0.0);
            a2 = hasA2 ? a.in.angleToRadians(raw2) : a1 + M_PI;
            full = false;
            if (g.left())
                readColor(g.next());
        }
        a.in.graphics().arc(g.g, cx, cy, rx, ry, a1, a2, full, edge, fill, filled);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"FILLPOLY", 2, 4, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        const Value &pts = g.next();
        std::vector<std::pair<double, double>> p;
        ValueList items = pts.isList() ? pts.items() : pts.isMatrix() ? pts.mat->data : ValueList{};
        for (const auto &v : items) {
            auto [x, y] = pointOf(v);
            p.push_back({g.x(x), g.y(y)});
        }
        uint32_t c = colorOf(g.next());
        int alpha = g.left() ? std::clamp(static_cast<int>(g.num()), 0, 255) : 255;
        a.in.graphics().fillPolygon(g.g, p, c, alpha);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"TRIANGLE", 1, 14, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        double x[3], y[3];
        uint32_t c[3] = {0, 0, 0};
        bool gradient = false;
        int alpha = 255;
        if (g.left() >= 3 && a.values[g.i].isList() && a.values[g.i + 1].isList() && a.values[g.i + 2].isList()
            && a.values[g.i].items().size() <= 4 && !a.values[g.i].items().empty() && a.values[g.i].items()[0].isNumber()) {
            for (int k = 0; k < 3; ++k) {
                const auto &l = g.next().items();
                x[k] = g.x(l.at(0).toReal());
                y[k] = g.y(l.at(1).toReal());
                c[k] = l.size() > 2 ? colorOf(l[2]) : 0;
            }
            gradient = !(c[0] == c[1] && c[1] == c[2]);
        } else {
            if (g.left() < 7)
                a.in.unsupported(U"Ta forma " + a.name + U" (bufor Z / 3D)");
            for (int k = 0; k < 3; ++k) {
                x[k] = g.x(g.num());
                y[k] = g.y(g.num());
            }
            c[0] = c[1] = c[2] = colorOf(g.next());
            if (g.left() >= 2) {
                c[1] = colorOf(g.next());
                c[2] = colorOf(g.next());
                gradient = true;
            }
            if (g.left() && a.values[g.i].isNumber())
                alpha = std::clamp(static_cast<int>(g.num()), 0, 255);
        }
        a.in.graphics().triangle(g.g, x, y, c, gradient, alpha);
        return Value::real(0);
    });
    auto textout = [](CallArgs &a, bool cart) {
        std::u32string text = a.in.format(a[0], false);
        GArgs g(a, cart, false);
        g.i = 1;
        if (a.size() > 1 && a.values[1].type == Value::Type::Graphic) {
            g.g = a.values[1].grob;
            g.i = 2;
        }
        int x = g.px(), y = g.py();
        int font = g.left() ? static_cast<int>(g.num()) : 0;
        uint32_t color = g.left() ? colorOf(g.next()) : 0;
        int width = g.left() ? static_cast<int>(g.num()) : 0;
        bool hasBg = g.left() > 0;
        uint32_t bg = hasBg ? colorOf(g.next()) : 0;
        return Value::real(a.in.graphics().text(g.g, x, y, text, font, color, width, bg, hasBg));
    };
    t[U"TEXTOUT"] = {3, 8, false, [textout](CallArgs &a) { return textout(a, true); }};
    t[U"TEXTOUT_P"] = {3, 8, false, [textout](CallArgs &a) { return textout(a, false); }};
    registerGraphicsPair(t, U"INVERT", 0, 5, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        const Grob &b = a.in.graphics().grob(g.g);
        int x1 = 0, y1 = 0, x2 = b.width - 1, y2 = b.height - 1;
        if (g.left() >= 2) { x1 = g.px(); y1 = g.py(); }
        if (g.left() >= 2) { x2 = g.px(); y2 = g.py(); }
        a.in.graphics().invert(g.g, x1, y1, x2, y2);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"DIMGROB", 2, 4, [](CallArgs &a, bool) {
        if (a[0].type != Value::Type::Graphic)
            throw RuntimeError("DIMGROB: pierwszy argument musi być G1–G9.");
        if (a[1].isList())
            a.in.unsupported(U"DIMGROB z danymi obrazu (lista)");
        int w = a.integer(1), h = a.size() > 2 ? a.integer(2) : 0;
        uint32_t c = a.size() > 3 ? colorOf(a[3]) : 0xFFFFFF;
        a.in.graphics().dimension(a[0].grob, w, h, c);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"SUBGROB", 2, 6, [](CallArgs &a, bool cart) {
        GArgs g(a, cart);
        const Grob &src = a.in.graphics().grob(g.g);
        int x1 = 0, y1 = 0, x2 = src.width, y2 = src.height;
        if (g.left() >= 3) { x1 = g.px(); y1 = g.py(); }
        if (g.left() >= 3) { x2 = g.px(); y2 = g.py(); }
        const Value &dst = g.next();
        if (dst.type != Value::Type::Graphic || dst.grob == 0)
            throw RuntimeError("SUBGROB: docelowa grafika musi być G1–G9.");
        a.in.graphics().copyRegion(g.g, x1, y1, x2, y2, dst.grob);
        return Value::real(0);
    });
    registerGraphicsPair(t, U"GROBW", 0, 1, [](CallArgs &a, bool) {
        int g = a.size() && a[0].type == Value::Type::Graphic ? a[0].grob : 0;
        return Value::real(a.in.graphics().grob(g).width);
    });
    registerGraphicsPair(t, U"GROBH", 0, 1, [](CallArgs &a, bool) {
        int g = a.size() && a[0].type == Value::Type::Graphic ? a[0].grob : 0;
        return Value::real(a.in.graphics().grob(g).height);
    });
    registerGraphicsPair(t, U"BLIT", 1, 12, [](CallArgs &a, bool cart) {
        size_t i = 0;
        int dst = 0;
        if (a[0].type == Value::Type::Graphic && a.size() > 1 && a[1].type != Value::Type::Graphic && a[1].isNumber()) {
            dst = a[0].grob;
            i = 1;
        } else if (a[0].type == Value::Type::Graphic && a.size() > 1 && a[1].type == Value::Type::Graphic) {
            dst = a[0].grob;
            i = 1;
        }
        std::vector<double> dcoords;
        while (i < a.size() && a[i].type != Value::Type::Graphic)
            dcoords.push_back(a[i++].toReal("BLIT"));
        if (i >= a.size())
            throw RuntimeError("BLIT: brak grafiki źródłowej.");
        int src = a[i++].grob;
        std::vector<double> rest;
        while (i < a.size())
            rest.push_back(a[i++].toReal("BLIT"));
        Graphics &gr = a.in.graphics();
        const Grob &s = gr.grob(src);
        auto cx = [&](double v) {
            if (!cart) return static_cast<int>(std::lround(v));
            double xmin = a.in.getVariable(U"Xmin").toReal(), xmax = a.in.getVariable(U"Xmax").toReal();
            return static_cast<int>(std::lround((v - xmin) / (xmax - xmin) * 318));
        };
        auto cy = [&](double v) {
            if (!cart) return static_cast<int>(std::lround(v));
            double ymin = a.in.getVariable(U"Ymin").toReal(), ymax = a.in.getVariable(U"Ymax").toReal();
            return static_cast<int>(std::lround((ymax - v) / (ymax - ymin) * 218));
        };
        int sx1 = 0, sy1 = 0, sx2 = s.width, sy2 = s.height;
        bool hasT = false;
        uint32_t trans = 0;
        int alpha = 255;
        if (rest.size() == 1 || rest.size() == 3) {
            hasT = true;
            trans = static_cast<uint32_t>(rest.back()) & 0xFFFFFF;
            rest.pop_back();
        }
        if (rest.size() >= 2) { sx1 = cx(rest[0]); sy1 = cy(rest[1]); }
        if (rest.size() >= 4) { sx2 = cx(rest[2]); sy2 = cy(rest[3]); }
        if (rest.size() >= 5) { hasT = true; trans = static_cast<uint32_t>(rest[4]) & 0xFFFFFF; }
        if (rest.size() >= 6) alpha = std::clamp(static_cast<int>(rest[5]), 0, 255);
        int dx1 = 0, dy1 = 0;
        if (dcoords.size() >= 2) { dx1 = cx(dcoords[0]); dy1 = cy(dcoords[1]); }
        int dx2 = dx1 + (sx2 - sx1), dy2 = dy1 + (sy2 - sy1);
        if (dcoords.size() >= 4) { dx2 = cx(dcoords[2]); dy2 = cy(dcoords[3]); }
        gr.blit(dst, dx1, dy1, dx2, dy2, src, sx1, sy1, sx2, sy2, hasT, trans, alpha);
        return Value::real(0);
    });
    t[U"C→PX"] = {1, 2, false, [](CallArgs &a) {
                      double x, y;
                      if (a.size() == 2) { x = a.num(0); y = a.num(1); } else { std::tie(x, y) = pointOf(a[0]); }
                      double xmin = a.in.getVariable(U"Xmin").toReal(), xmax = a.in.getVariable(U"Xmax").toReal();
                      double ymin = a.in.getVariable(U"Ymin").toReal(), ymax = a.in.getVariable(U"Ymax").toReal();
                      return Value::makeList({Value::real(std::round((x - xmin) / (xmax - xmin) * 318)),
                                              Value::real(std::round((ymax - y) / (ymax - ymin) * 218))});
                  }};
    t[U"PX→C"] = {1, 2, false, [](CallArgs &a) {
                      double x, y;
                      if (a.size() == 2) { x = a.num(0); y = a.num(1); } else { std::tie(x, y) = pointOf(a[0]); }
                      double xmin = a.in.getVariable(U"Xmin").toReal(), xmax = a.in.getVariable(U"Xmax").toReal();
                      double ymin = a.in.getVariable(U"Ymin").toReal(), ymax = a.in.getVariable(U"Ymax").toReal();
                      return Value::makeList({Value::real(xmin + x / 318 * (xmax - xmin)), Value::real(ymax - y / 218 * (ymax - ymin))});
                  }};
    t[U"DRAWMENU"] = {0, 6, false, [](CallArgs &a) {
                          std::vector<std::u32string> labels;
                          if (a.size() == 1 && a[0].isList())
                              for (const auto &v : a[0].items())
                                  labels.push_back(a.in.format(v, false));
                          else
                              for (const auto &v : a.values)
                                  labels.push_back(a.in.format(v, false));
                          Graphics &g = a.in.graphics();
                          g.rect(0, 0, 220, 319, 239, 0x000000, 0x000000, true);
                          for (int k = 0; k < 6; ++k) {
                              int x1 = k * 320 / 6 + 1, x2 = (k + 1) * 320 / 6 - 1;
                              std::u32string lab = k < static_cast<int>(labels.size()) ? labels[k] : U"";
                              uint32_t face = lab.empty() ? 0x404040 : 0xE0E0E0;
                              g.rect(0, x1, 221, x2, 239, face, face, true);
                              int w = g.textWidth(lab, 1);
                              g.text(0, x1 + std::max(0, (x2 - x1 - w) / 2), 226, lab, 1, 0x000000, x2 - x1, 0, false);
                          }
                          return Value::real(1);
                      }};
}

} // namespace ppl
