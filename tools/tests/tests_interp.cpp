// Interpreter tests: "golden results" of programs taken directly from the tutorial files
// (expected values come from the tutorial, E. Shore's tutorial and the HP User Guide).
#include "testing.h"

#include "interpreter.h"
#include "scripthost.h"
#include "utf8.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

using namespace ppl;

namespace {

// All ``` code blocks of the tutorial.
const std::vector<std::string> &tutorialBlocks()
{
    static std::vector<std::string> blocks = [] {
        std::vector<std::string> out;
        namespace fs = std::filesystem;
        fs::path root = fs::path(PPL_REPO_ROOT);
        for (const char *dir : {"rozdzialy", "programy"}) {
            for (const auto &e : fs::directory_iterator(root / dir)) {
                std::ifstream f(e.path(), std::ios::binary);
                std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                if (e.path().extension() == ".txt") {
                    out.push_back(text);
                    continue;
                }
                std::istringstream in(text);
                std::string line, cur;
                bool inside = false;
                while (std::getline(in, line)) {
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();
                    size_t fs0 = line.find_first_not_of(' ');
                    if (fs0 != std::string::npos && line.compare(fs0, 3, "```") == 0) {
                        if (inside)
                            out.push_back(cur);
                        inside = !inside;
                        cur.clear();
                        continue;
                    }
                    if (inside)
                        cur += line + "\n";
                }
            }
        }
        return out;
    }();
    return blocks;
}

// First tutorial block that defines "EXPORT NAME(".
std::string program(const std::string &name)
{
    for (const auto &b : tutorialBlocks())
        if (b.find("EXPORT " + name + "(") != std::string::npos && b.find("...") == std::string::npos)
            return b;
    return {};
}

struct Run {
    std::string result;
    std::vector<std::string> output;
    std::string error;
    Graphics graphics;
};

Run runProgram(const std::string &source, const std::string &call, std::vector<std::string> inputs = {},
               std::vector<int> keys = {}, std::vector<std::string> extraPrograms = {})
{
    Run r;
    ScriptedHost host;
    for (auto &i : inputs)
        host.addInput(i);
    for (int k : keys)
        host.addKey(k);
    Interpreter in(host);
    host.interpreter = &in;
    in.setMaxSteps(3000000);
    in.setSeed(7);
    try {
        in.loadProgram(toU32(source), U"TEST");
        for (auto &p : extraPrograms)
            in.loadProgram(toU32(p), U"LIB");
        Value v = in.run(toU32(call));
        r.result = toUtf8(in.format(v));
    } catch (const ParseError &e) {
        r.error = "parse: " + e.message + " @" + std::to_string(e.line);
    } catch (const RuntimeError &e) {
        r.error = e.what();
    } catch (const KillSignal &) {
        r.error = "KILL";
    }
    r.output = host.output;
    r.graphics = in.graphics();
    return r;
}

std::string joined(const std::vector<std::string> &v)
{
    std::string s;
    for (const auto &x : v)
        s += x + " | ";
    return s;
}

bool contains(const std::vector<std::string> &v, const std::string &s)
{
    for (const auto &x : v)
        if (x == s)
            return true;
    return false;
}

double num(const std::string &s) { return std::strtod(s.c_str(), nullptr); }

#define EXPECT_RESULT(run, expected)                                                                                   \
    CHECK_MSG((run).error.empty() && (run).result == (expected), "wynik=" + (run).result + " błąd=" + (run).error)

} // namespace

TEST(interp_sqin_mopmt_doestax)
{
    std::string sqin = program("SQIN");
    CHECK(!sqin.empty());
    EXPECT_RESULT(runProgram(sqin, "SQIN(5)"), "0.04");
    EXPECT_RESULT(runProgram(sqin, "SQIN(36)"), "7.71604938272E-4");
    std::string mopmt = program("MOPMT");
    EXPECT_RESULT(runProgram(mopmt, "MOPMT(4000, 9.5, 30)"), "\"Payment =150.317437565\"");
    EXPECT_RESULT(runProgram(mopmt, "MOPMT(370000, 3.5, 360)"), "\"Payment =1661.46534383\"");
    std::string tax = program("DOESTAX");
    EXPECT_RESULT(runProgram(tax, "DOESTAX(8)"), "8");
    EXPECT_RESULT(runProgram(tax, "DOESTAX(12)"), "13.02");
}

TEST(interp_loops_and_lists)
{
    std::string sumdiv = program("SUMDIV");
    EXPECT_RESULT(runProgram(sumdiv, "SUMDIV(12)"), "28");
    EXPECT_RESULT(runProgram(sumdiv, "SUMDIV(24)"), "60");
    EXPECT_RESULT(runProgram(sumdiv, "SUMDIV(85)"), "108");
    Run u = runProgram(program("ULAM"), "ULAM(5)");
    EXPECT_RESULT(u, "{5,16,8,4,2,1}");
    CHECK_MSG(contains(u.output, "[MSGBOX] NO. OF STEPS=6"), joined(u.output));
    Run u22 = runProgram(program("ULAM"), "ULAM(22)");
    CHECK_MSG(contains(u22.output, "[MSGBOX] NO. OF STEPS=16"), joined(u22.output));
    Run cubes = runProgram(program("DISPCUBES"), "DISPCUBES(5)");
    CHECK_MSG(cubes.output == std::vector<std::string>({"1", "8", "27", "64", "125"}), joined(cubes.output));
    Run evens = runProgram(program("PRINTEVENS"), "PRINTEVENS(3,10)");
    CHECK_MSG(evens.output == std::vector<std::string>({"4", "6", "8", "10"}), joined(evens.output));
    Run mf = runProgram(program("MAXFACTORS"), "MAXFACTORS(100)");
    CHECK_MSG(contains(mf.output, "[MSGBOX] Max of 12 factors for 60"), joined(mf.output) + mf.error);
    std::string perfect = program("ISPERFECT");
    EXPECT_RESULT(runProgram(perfect, "ISPERFECT(28)"), "1");
    EXPECT_RESULT(runProgram(perfect, "ISPERFECT(12)"), "0");
}

TEST(interp_complex_roots)
{
    Run q = runProgram(program("QROOTS"), "QROOTS(1,5,8)");
    CHECK_MSG(q.error.empty() && q.output.size() == 3 && q.output[1] == "-2.5+1.32287565553*i" && q.output[2] == "-2.5-1.32287565553*i",
              joined(q.output) + q.error);
    Run r = runProgram(program("QROOTS"), "QROOTS(2,-4,-8)");
    CHECK_MSG(r.output.size() == 3 && r.output[1] == "3.2360679775" && r.output[2] == "-1.2360679775", joined(r.output) + r.error);
}

TEST(interp_subroutines_and_recursion)
{
    std::string sub = program("SUBEXAM");
    EXPECT_RESULT(runProgram(sub, "SUBEXAM(-4, 1)"), "21998.918189");
    EXPECT_RESULT(runProgram(sub, "SUBEXAM(2,3)"), "86283.2797974");
    EXPECT_RESULT(runProgram(sub, "SUBEXAM(-5,-6)"), "30.648061288");
    EXPECT_RESULT(runProgram(sub, "SUBEXAM(2,-3)"), "21810.6046664");
    EXPECT_RESULT(runProgram(program("SILNIAR"), "SILNIAR(10)"), "3628800");
    Run h = runProgram(program("WIEZA"), "WIEZA(3)");
    CHECK_MSG(h.output.size() == 7 && h.output[0] == "krążek 1: A -> C", joined(h.output) + h.error);
    std::string rollmany;
    for (const auto &b : tutorialBlocks())
        if (b.find("EXPORT ROLLMANY(") != std::string::npos && b.find("RETURN results;") != std::string::npos)
            rollmany = b;
    Run roll = runProgram(rollmany, "ΣLIST(ROLLMANY(100,6))");
    EXPECT_RESULT(roll, "100");
}

TEST(interp_dialogs)
{
    Run t = runProgram(program("TERMVEL"), "TERMVEL()", {"1", "1", ".05", ".0028"});
    CHECK_MSG(t.error.empty() && std::fabs(num(t.result) - 24.6640475387) < 1e-9, t.result + " " + t.error);
    std::string areac = program("AREAC");
    EXPECT_RESULT(runProgram(areac, "AREAC()", {"1", "2.5"}), "19.6349540849");
    EXPECT_RESULT(runProgram(areac, "AREAC()", {"2", "2.5", "1.5"}), "12.5663706144");
    EXPECT_RESULT(runProgram(areac, "AREAC()", {"3", "2.5", "π/4"}), "2.45436926062");
    Run tax2 = runProgram(program("DOESTAX2"), "DOESTAX2()", {"59.99", "1", "9.99", "1", "10", "0", "9"});
    CHECK_MSG(tax2.error.empty() && std::fabs(num(tax2.result) - 86.2782) < 1e-9, tax2.result + " " + tax2.error);
    Run cancel = runProgram(program("POLE_KOLA"), "POLE_KOLA()", {"cancel"});
    EXPECT_RESULT(cancel, "\"Anulowano\"");
}

TEST(interp_calculus_template)
{
    Run c = runProgram(program("CALCDEMO"), "CALCDEMO()");
    std::vector<std::string> nums;
    for (const auto &l : c.output)
        if (!l.empty() && (std::isdigit(static_cast<unsigned char>(l[0])) || l[0] == '-'))
            nums.push_back(l);
    CHECK_MSG(nums == std::vector<std::string>({"0.707106781186", "1.53029480247", "18.6666666667", "6084", "-1"}),
              joined(c.output) + c.error);
}

TEST(interp_builtin_examples_from_manual)
{
    auto eval = [](const std::string &expr) { return runProgram("", expr); };
    EXPECT_RESULT(eval("LEFT(\"MOMOGUMBO\",3)"), "\"MOM\"");
    EXPECT_RESULT(eval("RIGHT(\"MOMOGUMBO\",5)"), "\"GUMBO\"");
    EXPECT_RESULT(eval("MID(\"MOMOGUMBO\",3,5)"), "\"MOGUM\"");
    EXPECT_RESULT(eval("MID(\"PUDGE\",4)"), "\"GE\"");
    EXPECT_RESULT(eval("INSTRING(\"banana\",\"na\")"), "3");
    EXPECT_RESULT(eval("ROTATE(\"12345\",2)"), "\"34512\"");
    EXPECT_RESULT(eval("ROTATE(\"12345\",-1)"), "\"51234\"");
    EXPECT_RESULT(eval("REPLACE(\"123456\",2,\"GRM\")"), "\"1GRM56\"");
    EXPECT_RESULT(eval("ASC(\"AB\")"), "{65,66}");
    EXPECT_RESULT(eval("CHAR([82,77,72])"), "\"RMH\"");
    EXPECT_RESULT(eval("DIM(\"12345\")"), "5");
    EXPECT_RESULT(eval("BITAND(20,13)"), "4");
    EXPECT_RESULT(eval("BITOR(9,26)"), "27");
    EXPECT_RESULT(eval("BITXOR(9,26)"), "19");
    EXPECT_RESULT(eval("BITSL(28,2)"), "112");
    EXPECT_RESULT(eval("BITSR(112,2)"), "28");
    EXPECT_RESULT(eval("#10000b+#10100b"), "#100100b");
    EXPECT_RESULT(eval("#32Ah/#5o"), "#A2h");
    EXPECT_RESULT(eval("%CHANGE(20,50)"), "150");
    EXPECT_RESULT(eval("%TOTAL(20,50)"), "250");
    EXPECT_RESULT(eval("ITERATE(X^2,X,2,3)"), "256");
    EXPECT_RESULT(eval("MAKELIST(X^2,X,23,27,1)"), "{529,576,625,676,729}");
    EXPECT_RESULT(eval("ΣLIST({2,3,4})"), "9");
    EXPECT_RESULT(eval("ΠLIST({2,3,4})"), "24");
    EXPECT_RESULT(eval("ΔLIST({3,5,8,12,17,23})"), "{2,3,4,5,6}");
    EXPECT_RESULT(eval("DIFFERENCE({1,2,3,4},{1,3,5,7})"), "{2,4,5,7}");
    EXPECT_RESULT(eval("INTERSECT({1,2,3,4},{1,3,5,7})"), "{1,3}");
    EXPECT_RESULT(eval("POS({3,7,12,19},12)"), "3");
    EXPECT_RESULT(eval("SORT({2,5,3})"), "{2,3,5}");
    EXPECT_RESULT(eval("CONCAT({1,2,3},{4})"), "{1,2,3,4}");
    EXPECT_RESULT(eval("SIZE([[1,2,3],[4,5,6]])"), "{2,3}");
    EXPECT_RESULT(eval("EXECON(\"&1+1\",{1,2,3})"), "{2,3,4}");
    EXPECT_RESULT(eval("EXECON(\"&2-&1\",{1,4,3,5})"), "{3,-1,2}");
    EXPECT_RESULT(eval("EXECON(\"&1+&2\",{1,2,3},{4,5,6})"), "{5,7,9}");
    EXPECT_RESULT(eval("5*{1,2,3}"), "{5,10,15}");
    EXPECT_RESULT(eval("MAKEMAT(I+J,3,3)"), "[[2,3,4],[3,4,5],[4,5,6]]");
    EXPECT_RESULT(eval("DET([[1,2],[3,4]])"), "-2");
    EXPECT_RESULT(eval("[[2,1,-1],[-3,-1,2],[-2,1,2]]^-1*[[8],[-11],[-3]]"), "[[2],[3],[-1]]");
    EXPECT_RESULT(eval("SOLVE(X^2-X-2,X,3)"), "2");
    EXPECT_RESULT(eval("QuadSolve(1,-3,2)"), "{1,2}");
    EXPECT_RESULT(eval("ROUND(CalcPMT(360, 6.5, 150000, -2.25),2)"), "-948.1");
    EXPECT_RESULT(eval("0.1+0.2==0.3"), "1");
    EXPECT_RESULT(eval("when(1>2,\"a\",\"b\")"), "\"b\"");
    EXPECT_RESULT(eval("CAS.idivis(12)"), "[1,2,3,4,6,12]");
    EXPECT_RESULT(eval("L1:={5,\"abcde\",{1,2,3,4,5},11}; L1(2,4)"), "100");
    EXPECT_RESULT(eval("L1:={5,\"abcde\",{1,2,3,4,5},11}; L1({2,4})"), "{\"abcde\",{1,2,3,4,5},11}");
}

TEST(interp_strings_and_tutorial_solutions)
{
    EXPECT_RESULT(runProgram(program("CEZAR"), "CEZAR(\"HP PRIME\",3)"), "\"KS SULPH\"");
    EXPECT_RESULT(runProgram(program("ODWROC"), "ODWROC(\"Prime\")"), "\"emirP\"");
    EXPECT_RESULT(runProgram(program("PALINDROM"), "PALINDROM(\"Kobyła ma mały bok\")"), "1");
    EXPECT_RESULT(runProgram(program("NWD"), "NWD(84,36)"), "12");
    EXPECT_RESULT(runProgram(program("FIB"), "FIB(8)"), "{1,1,2,3,5,8,13,21}");
    EXPECT_RESULT(runProgram(program("CYFRY"), "CYFRY(1234)"), "10");
    EXPECT_RESULT(runProgram(program("PRZESTEPNY"), "PRZESTEPNY(2000)+PRZESTEPNY(1900)*10"), "1");
    EXPECT_RESULT(runProgram(program("BIN"), "BIN(10)"), "\"1010\"");
    EXPECT_RESULT(runProgram(program("ZAMIEN"), "ZAMIEN(\"ala ma kota\",\"a\",\"o\")"), "\"olo mo koto\"");
    EXPECT_RESULT(runProgram(program("TROJKAT"), "TROJKAT(3,4,5)"), "\"różnoboczny\"");
    EXPECT_RESULT(runProgram(program("USUN_DUPL"), "USUN_DUPL({1,2,1,3,2})"), "{1,2,3}");
    EXPECT_RESULT(runProgram(program("POTEGA"), "POTEGA(2,10)"), "1024");
}

TEST(interp_errors_and_signals)
{
    Run typed = runProgram("EXPORT F()\nBEGIN\n  L1 := 5;\nEND;\n", "F()");
    CHECK_MSG(typed.error.find("tylko listy") != std::string::npos, typed.error);
    Run div = runProgram("EXPORT F()\nBEGIN\n  RETURN 1/0;\nEND;\n", "F()");
    CHECK_MSG(div.error.find("Dzielenie przez zero") != std::string::npos, div.error);
    Run iferr = runProgram("EXPORT F()\nBEGIN\n  IFERR RETURN 1/0; THEN RETURN \"złapano\"; END;\nEND;\n", "F()");
    EXPECT_RESULT(iferr, "\"złapano\"");
    Run kill = runProgram("EXPORT F()\nBEGIN\n  KILL;\n  RETURN 1;\nEND;\n", "F()");
    CHECK(kill.error == "KILL");
    Run loop = runProgram("EXPORT F()\nBEGIN\n  WHILE 1 DO END;\nEND;\n", "F()");
    CHECK_MSG(loop.error.find("limit") != std::string::npos, loop.error);
    Run brk = runProgram("EXPORT F()\nBEGIN\n  LOCAL i, j, n := 0;\n  FOR i FROM 1 TO 5 DO\n    FOR j FROM 1 TO 5 DO\n"
                         "      n := n+1;\n      IF j==2 THEN BREAK(2); END;\n    END;\n  END;\n  RETURN n;\nEND;\n",
                         "F()");
    EXPECT_RESULT(brk, "2");
}

TEST(interp_graphics_programs)
{
    Run house = runProgram(program("DRAWHOUSE"), "DRAWHOUSE()", {}, {30});
    CHECK_MSG(house.error.empty(), house.error);
    CHECK(house.graphics.pixel(0, 20, 150) == 0x905000);   // left wall
    CHECK(house.graphics.pixel(0, 130, 50) == 0x800000);   // roof top
    CHECK(house.graphics.pixel(0, 100, 150) == 0xFFFFFF);  // inside
    Run pent = runProgram(program("DRAWPENT"), "DRAWPENT()", {}, {30});
    CHECK(pent.graphics.pixel(0, 5, 5) == 0x400080);
    CHECK(pent.graphics.pixel(0, 160, 100) == 0xFFC0C0);
    Run arcs = runProgram(program("DRAWARCS"), "DRAWARCS()", {}, {30});
    CHECK_MSG(arcs.error.empty(), arcs.error);
    CHECK(arcs.graphics.pixel(0, 90, 110) == 0x008000);    // right edge of the circle (60+30, 110)
    Run snake = runProgram(program("SNAKE"), "SNAKE()", {}, {8, 8, 12, 4});
    CHECK_MSG(snake.error.empty() && snake.result == "0" && contains(snake.output, "[MSGBOX] Koniec gry! Punkty: 0"),
              joined(snake.output) + snake.error);
    Run sito = runProgram(program("SITO"), "SIZE(SITO())", {}, {30});
    EXPECT_RESULT(sito, "78");
}
int main() { return testing::runAll(); }
