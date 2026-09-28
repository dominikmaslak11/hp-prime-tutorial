# Rozdział 23. Mapa całej instrukcji HP z perspektywy programisty

[← Poprzedni](22-debugowanie.md) · [Spis treści](../README.md) · [Następny →](24-projekty.md)

Instrukcja *HP Prime Graphing Calculator User Guide*, **wydanie 3 (grudzień 2017, nr 813269-003)**, ma 32 rozdziały i ponad 700 stron. To najnowsze pełne wydanie opublikowane przez HP. Opisuje firmware z tego samego okresu co tutorial Shore'a (13441). Ten rozdział przechodzi przez **każdy** rozdział instrukcji i odpowiada na trzy pytania:
1. O czym jest rozdział?
2. Co z niego przyda się w programach?
3. Gdzie w tym kursie jest to omówione?

> **Zmiany względem wydania 2 (2016):** nowy rozdział 9 *Graph 3D*, więc numery kolejnych rozdziałów przesunęły się o 1. Aplikacja *Finance* została mocno rozbudowana (konwersja odsetek, daty, przepływy pieniężne, amortyzacja, próg rentowności, marże, obligacje, opcje). Trzy aplikacje Explorer połączono w jedną aplikację *Explorer*. Doszły też zmienne obrazu tła wykresu (`Image*`). Komendy programowania (menu Tmplt i Cmds) się nie zmieniły.

---

## Rozdz. 1 — Preface (s. 1)

Konwencje instrukcji: nazwy klawiszy, przycisków i menu. **W programach:** nic bezpośrednio.

## Rozdz. 2 — Getting started (s. 2–37)

Włączanie i wyłączanie, widoki Home i CAS, ekran, gesty dotykowe, klawiatura (Shift, ALPHA, szablony matematyczne, ułamki, zapis sześćdziesiątkowy, EEX), menu i Toolbox, formularze, **ustawienia Home**, kolejność działań, mnożenie jawne i domyślne, historia i `Ans`, schowek, liczby zespolone, udostępnianie danych, Memory Manager, kopie zapasowe, pomoc online.

**W programach:**
- ustawienia Home mają swoje zmienne: `HAngle`, `HFormat`, `HDigits`, `HComplex`, `Entry`, `Base`, `Bits`, `Signed`, `Language`, `Date`, `Time`. Zob. [rozdz. 21](21-zmienne-systemowe.md) i [dodatek C](../dodatki/C-tabele.md);
- `Ans`: ostatni wynik;
- mnożenie domyślne (np. `2X`) jest wygodne w Home, ale w programach **zawsze pisz `*`**;
- klawisz [Help] działa także w edytorze programów i przy komendach w menu.

## Rozdz. 3 — Reverse Polish Notation (RPN) (s. 38–46)

Tryb RPN: stos zamiast nawiasów, komendy stosu `PICK`, `ROLL`, `SWAP`, `DROPN`, `DUPN`, `Echo`, `→LIST`.

**W programach:** uruchamianie programu z argumentami w RPN to `NAZWA(n)`, gdzie n to liczba argumentów pobieranych ze stosu ([rozdz. 1](01-srodowisko.md#tryby-wprowadzania)). Programy PPL nie operują na stosie RPN, bo używają zmiennych.

## Rozdz. 4 — Computer algebra system (CAS) (s. 47–53)

Obliczenia dokładne i symboliczne, ustawienia CAS, nazwy opisowe i komendy w menu, przenoszenie wyrażeń między Home i CAS.

**W programach:** prefiks `CAS.`, funkcje całkowitoliczbowe (`idivis`, `isprime`, `ifactor`), programy `#cas`. Zob. [rozdz. 13](13-cas-i-analiza.md).

## Rozdz. 5 — Exam Mode (s. 54–61)

Tryb egzaminacyjny: konfiguracje blokujące wybrane funkcje (CAS, aplikacje, notatki, **programy**…), włączanie przez kabel lub z menu, dioda sygnalizująca tryb.

**W programach:** w trybie egzaminacyjnym programy i notatki użytkownika mogą być zablokowane lub niedostępne, zależnie od konfiguracji. Nie licz na swoje programy na egzaminie, jeśli nauczyciel włączy tryb egzaminacyjny.

## Rozdz. 6 — Introduction to HP apps (s. 62–108)

Biblioteka aplikacji, widoki (Symbolic, Plot, Numeric i ich Setup), wspólne operacje: definicje, kolory wykresów, zoom, trace, tabele, łączenie widoków, notatka aplikacji, **tworzenie własnej aplikacji**, funkcje i zmienne aplikacji, kwalifikowanie nazw.

**W programach:** podstawa [rozdz. 18](18-sterowanie-aplikacjami.md) (sterowanie aplikacjami) i [rozdz. 19](19-wlasne-aplikacje.md) (własne aplikacje). Numery widoków w `STARTVIEW` odpowiadają widokom opisanym w tym rozdziale. Nowość w wyd. 3: **obraz tła wykresu**, sterowany zmiennymi `ImageName`, `ImageDisplay`, `ImageOpacity`, `ImageXmin`…`ImageYmax`.

## Rozdz. 7 — Function app (s. 109–133)

Wykresy y = f(x): definiowanie, trace, tabela, analiza (miejsce zerowe, przecięcie, nachylenie, pole, ekstremum, styczna), funkcje definiowane przez pochodne i całki.

**W programach:** `F0`–`F9`, `ROOT`, `EXTREMUM`, `ISECT`, `SLOPE`, `AREA`, zmienne wyników `Root`, `Extremum`, `Isect`, `Slope`, `SignedArea`. Zob. [rozdz. 13.4](13-cas-i-analiza.md#134-rozwiązywanie-równań-numerycznie) i [18](18-sterowanie-aplikacjami.md).

> W wyd. 3 opis funkcji `ROOT`, `EXTREMUM`, `ISECT`, `SLOPE` i `AREA` wypadł z rozdziału o funkcjach (menu App), ale funkcje nadal istnieją. Wyd. 3 wspomina je w rozdziale o zmiennych. Składnię opisuje wyd. 2 i [rozdz. 13.4](13-cas-i-analiza.md#134-rozwiązywanie-równań-numerycznie) kursu.

## Rozdz. 8 — Advanced Graphing app (s. 134–148)

Wykresy dowolnych zdań otwartych w X i Y (równania, nierówności), Plot Gallery.

**W programach:** `V0`–`V9` jako teksty, np. `V1:="X^2+Y^2<=4"`. Przykład: CONIC ([rozdz. 18.9](18-sterowanie-aplikacjami.md#189-program-conic-es-5)).

## Rozdz. 9 — Graph 3D app (s. 149–159) *(nowy w wyd. 3)*

Wykresy powierzchni z = f(x, y): definiowanie, obracanie bryły, zoom, tabela wartości.

**W programach:**
- funkcje `FZ0`–`FZ9` przypisujesz jako tekst: `FZ1 := "SIN(X)*COS(Y)"; CHECK(1);` po `STARTAPP("Graph 3D")`,
- zakres osi: `Xmin`…`Ymax` oraz `Zmin`, `Zmax`, `Ztick`, `Zzoom`,
- wygląd: `BoxAxes`, `BoxDots`, `BoxFrame`, `BoxLines`, `BoxScale`, `BoxSides`, `KeyAxes`, gęstość siatki `Surface`,
- orientacja: `PoseTurn`, `PoseXaxis`, `PoseYaxis`, `PoseZaxis`,
- kolor funkcji `FZ#(COLOR)` to lista dwóch kolorów (górna i dolna strona powierzchni).

## Rozdz. 10 — Geometry (s. 160–221)

Geometria dynamiczna: punkty, proste, okręgi, przekształcenia, pomiary, widok Symbolic z komendami tworzącymi obiekty, ogromny zestaw funkcji geometrycznych (`point`, `line`, `circle`, `distance`, `area`, `midpoint`, `barycenter`, `convexhull`…).

**W programach:** funkcje geometryczne są w katalogu i można ich używać w CAS. Zmienne `PixSize`, `ScrollText`. Obiekty geometryczne tworzy się komendami, więc program może budować konstrukcje. To temat zaawansowany i niszowy. Rozdział 10 instrukcji jest tu pełną dokumentacją.

## Rozdz. 11 — Spreadsheet (s. 222–237)

Arkusz kalkulacyjny: komórki, formuły, odwołania (`A1`, `$A$1`), nazwy, formatowanie, funkcje arkusza (`SUM`, `AVERAGE`, `AMORT`, `STAT1`, `STAT2`, `REGRS`, `PredX`, `PredY`, testy statystyczne).

**W programach:** komórki arkusza są zmiennymi aplikacji Spreadsheet, np. `A1`, `B2:C5`. Można je czytać i zapisywać z programu po uruchomieniu aplikacji. Zob. [rozdz. 18.8](18-sterowanie-aplikacjami.md#188-funkcje-aplikacji-ug-rozdz-23-app-menu).

## Rozdz. 12 — Statistics 1Var (s. 238–255)

Statystyka jednej zmiennej: kolumny `D0`–`D9`, analizy `H1`–`H5`, histogramy, wykresy pudełkowe, statystyki opisowe.

**W programach:** `SetSample`, `SetFreq`, `Do1VStats`, zmienne wyników (`MeanX`, `MedVal`, `σX`…), `H1Type`. Zob. [rozdz. 18.11](18-sterowanie-aplikacjami.md#1811-statystyka-z-programu) i przykład DiceSimulation w [rozdz. 19](19-wlasne-aplikacje.md).

## Rozdz. 13 — Statistics 2Var (s. 256–271)

Dwie zmienne: kolumny `C0`–`C9`, analizy `S1`–`S5`, dopasowania (liniowe, wykładnicze, potęgowe, logistyczne…), korelacja, predykcja.

**W programach:** `SetIndep`, `SetDepend`, `Do2VStats`, `PredX`, `PredY`, `Resid`, zmienne `Corr`, `CoefDet`…

## Rozdz. 14 — Inference (s. 272–303)

Testy hipotez i przedziały ufności (Z, T, proporcje, χ², regresja liniowa).

**W programach:** funkcje `HypZ1mean`, `ConfT1mean`, `Chi2GOF`, `LinRegrTTest` i inne, `DoInference`, zmienne `Result`, `TestScore`, `Prob`, `CritVal1`…

## Rozdz. 15 — Solve app (s. 304–311)

Rozwiązywanie równań i układów numerycznie (`E0`–`E9`).

**W programach:** `SOLVE(En, zmienna, start)`. Zob. [rozdz. 18.12](18-sterowanie-aplikacjami.md#1812-aplikacja-solve-z-programu).

## Rozdz. 16 — Linear Solver app (s. 312–314)

Układy równań liniowych 2×2 i 3×3.

**W programach:** `Solve2x2`, `Solve3x3`, `LinSolve`, zmienna `LSystem`. Alternatywa: `A^-1*b` ([rozdz. 11](11-macierze.md)).

## Rozdz. 17 — Parametric app (s. 315–319)

Krzywe parametryczne x(t), y(t).

**W programach:** `X0`–`X9`, `Y0`–`Y9`, `Tmin`, `Tmax`, `Tstep`. Przykład: PROJ13 ([rozdz. 18.10](18-sterowanie-aplikacjami.md#1810-program-proj13--rzut-ukośny-es-5)).

## Rozdz. 18 — Polar app (s. 320–324)

Krzywe we współrzędnych biegunowych r(θ).

**W programach:** `R0`–`R9`, `θmin`, `θmax`, `θstep`.

## Rozdz. 19 — Sequence app (s. 325–333)

Ciągi rekurencyjne i jawne U(N), wykresy schodkowe i pajęczynowe.

**W programach:** `U0`–`U9`, `Nmin`, `Nmax`, `SeqPlot`.

## Rozdz. 20 — Finance app (s. 334–365) *(mocno rozbudowany w wyd. 3)*

Wartość pieniądza w czasie (TVM), amortyzacja, a w wyd. 3 dodatkowo: konwersja stóp procentowych, obliczenia na datach, przepływy pieniężne (IRR, MIRR, NPV…), amortyzacja środków trwałych (Depreciation), próg rentowności, marże i narzuty, zmiana procentowa, obligacje, opcje Blacka–Scholesa.

**W programach (wyd. 3):**

| Grupa | Funkcje |
|---|---|
| TVM | `TvmPMT`, `TvmPV`, `TvmFV`, `TvmIPYR`, `TvmNbPmt` (oraz starsze `CalcPMT`, `CalcPV`… z tymi samymi argumentami: `(NbPmt, IPYR, PV, FV, [PPYR], [CPYR], [BEG])`), `DoFinance(zmienna)` |
| Odsetki | `IntConvNom`, `IntConvEff`, `IntConvCPYR` |
| Daty | `DateDays(data1, data2, [kal360])` (daty w formacie RRRR.MMDD) |
| Przepływy | `CashFlowIRR`, `CashFlowMIRR`, `CashFlowFMRR`, `CashFlowNPV`, `CashFlowNFV`, `CashFlowNUS`, `CashFlowPB`, `CashFlowTotal` |
| Amortyzacja | `Depreciate(metoda, koszt, wartość_końcowa, okres, [pierwszy], [współczynnik])` |
| Próg rentowności | `BrkEvFixed`, `BrkEvQuant`, `BrkEvCost`, `BrkEvPrice`, `BrkEvProfit` |
| Marże i zmiany | `ChangePrice`, `ChangeCost`, `PercentMargin`, `PercentMarkup`, `ChangeOld`, `ChangeNew`, `PercentTotal`, `PercentChange` |
| Obligacje i opcje | `BondPrice`, `BondYield`, `BlackScholes` |

Przykład z instrukcji: `CalcPMT(360, 6.5, 150000, -2.25)` → -948.10, czyli rata kredytu 150 000 na 30 lat przy 6,5%. Porównaj z własnym programem MOPMT z [rozdz. 1](01-srodowisko.md#17-drugi-program-mopmt--rata-kredytu-es-1).

## Rozdz. 21 — Triangle Solver app (s. 366–370)

Rozwiązywanie trójkątów (SSS, SAS, ASA, AAS, SSA).

**W programach:** `SideA`–`SideC`, `AngleA`–`AngleC`, `TriType`, funkcje `SSS`, `SAS`, `ASA`, `AAS`, `SSA`, `DoSolve`.

## Rozdz. 22 — Explorer app (s. 371–378)

W wyd. 3 **jedna aplikacja Explorer** zastępuje trzy osobne (Linear, Quadratic i Trig Explorer). Doszły funkcje sześcienne, wykładnicze i logarytmiczne.

**W programach:** `LinearSlope(x1,y1,x2,y2)`, `LinearYIntercept(x,y,m)`, `QuadSolve(a,b,c)`, `QuadDelta(a,b,c)`. W starszym firmware te funkcje nazywały się `SolveForSlope`, `SolveForYIntercept`, `SOLVE` i `DELTA`.

## Rozdz. 23 — Functions and commands (s. 379–509)

Największy rozdział: funkcje klawiatury, menu **Math** (liczby, arytmetyka, trygonometria, hiperboliczne, prawdopodobieństwo, rozkłady, macierze, listy, specjalne), menu **CAS** (algebra, analiza, rozwiązywanie, przekształcenia, całkowite, wielomiany, grafy, geometria), menu **App** z funkcjami wszystkich aplikacji, katalog **Ctlg** (setki komend alfabetycznie) i tworzenie własnych funkcji użytkownika.

**W programach:** to słownik wszystkiego, czego możesz użyć. Większość funkcji Math działa w PPL bezpośrednio, a funkcje CAS wymagają `CAS.`. Najważniejsze są w [rozdz. 3](03-typy-danych-i-operatory.md) i [dodatku A](../dodatki/A-sciaga.md). Sposób nauki: gdy potrzebujesz funkcji, szukaj jej w [Toolbox] i czytaj [Help].

## Rozdz. 24 — Variables (s. 510–533)

Zmienne Home, aplikacji, CAS i użytkownika. Menu Vars, kwalifikowanie, pełne listy zmiennych każdej aplikacji (w wyd. 3 także Graph 3D i nowych trybów Finance).

**W programach:** kluczowe. Zob. [rozdz. 2](02-skladnia-i-zmienne.md), [18](18-sterowanie-aplikacjami.md) i [21](21-zmienne-systemowe.md).

## Rozdz. 25 — Units and constants (s. 534–544)

Jednostki (kategorie, przedrostki, obliczenia), narzędzia `CONVERT`, `MKSA`, `UFACTOR`, `USIMPLIFY`, stałe fizyczne.

**W programach:** liczby z jednostkami (`5_m`), `TYPE` = 9. Zob. [rozdz. 3.1](03-typy-danych-i-operatory.md#jednostki) i [21.6](21-zmienne-systemowe.md#216-stałe-fizyczne-i-jednostki-ug-rozdz-25).

## Rozdz. 26 — Lists (s. 545–559)

Katalog i edytor list, listy w Home, odwołania, funkcje listowe, statystyka list.

**W programach:** kluczowe. Zob. [rozdz. 10](10-listy.md).

## Rozdz. 27 — Matrices (s. 560–587)

Katalog i edytor macierzy, arytmetyka, układy równań, funkcje macierzowe (tworzenie, normy, rozkłady, wektory).

**W programach:** zob. [rozdz. 11](11-macierze.md).

## Rozdz. 28 — Notes and Info (s. 588–595)

Katalog notatek, edytor z formatowaniem, notatki aplikacji (Info), import notatek.

**W programach:** `Notes`, `ANote`. Zob. [rozdz. 21.4](21-zmienne-systemowe.md#214-notes--notatki-z-poziomu-programu-ug-s-694).

## Rozdz. 29 — Programming in HP PPL (s. 596–695)

Cały rozdział jest omówiony w [rozdziałach 1–22](../README.md) tego kursu:

| Temat instrukcji | Rozdział kursu |
|---|---|
| Program Catalog, Program Editor, run, debug, copy, delete, share | [1](01-srodowisko.md), [22](22-debugowanie.md) |
| Variables and visibility, qualifying names | [2](02-skladnia-i-zmienne.md) |
| Functions, arguments, ROLLDIE/ROLLMANY | [8](08-funkcje.md) |
| User keyboard | [20](20-klawiatura-uzytkownika.md) |
| App programs, VIEW, DiceSimulation | [19](19-wlasne-aplikacje.md) |
| Tmplt: Block, Branch, Loop, Variable, Function | [4](04-wyjscie.md), [5](05-warunki.md), [6](06-petle.md), [2](02-skladnia-i-zmienne.md), [8](08-funkcje.md) |
| Cmds: Strings | [9](09-lancuchy.md) |
| Cmds: Drawing, Pixels and Cartesian | [14](14-grafika-podstawy.md), [15](15-grafika-rysowanie.md), [16](16-grafika-zaawansowana.md) |
| Cmds: Matrix | [11](11-macierze.md) |
| Cmds: App Functions | [18](18-sterowanie-aplikacjami.md) |
| Cmds: Integer | [12](12-liczby-calkowite.md) |
| Cmds: I/O | [4](04-wyjscie.md), [7](07-wejscie.md), [17](17-klawiatura-dotyk-czas.md) |
| Cmds: More (%CHANGE, CAS, EVALLIST, EXECON, HMS, ITERATE, TICKS, TEVAL, TYPE) | [3](03-typy-danych-i-operatory.md), [10](10-listy.md), [13](13-cas-i-analiza.md), [17](17-klawiatura-dotyk-czas.md) |
| Variables and programs, App variables | [18](18-sterowanie-aplikacjami.md), [21](21-zmienne-systemowe.md) |

## Rozdz. 30 — Basic integer arithmetic (s. 696–701)

Systemy liczbowe, rozmiar słowa, arytmetyka mieszana, edytor liczb całkowitych, funkcje systemów.

**W programach:** zob. [rozdz. 12](12-liczby-calkowite.md).

## Rozdz. 31 — Appendix A: Glossary (s. 702–703)

Słownik pojęć: app, catalog, expression, function, Home, library, list, matrix, note, program, variable, vector, views.

## Rozdz. 32 — Appendix B: Troubleshooting (s. 704–706)

Reset, ładowanie, parametry baterii, komunikaty błędów. Zob. [rozdz. 22.10–22.11](22-debugowanie.md#2210-komunikaty-błędów-ug-s-704705).
