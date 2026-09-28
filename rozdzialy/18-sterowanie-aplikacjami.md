# Rozdział 18. Sterowanie aplikacjami HP: STARTAPP, STARTVIEW, zmienne aplikacji

[← Poprzedni](17-klawiatura-dotyk-czas.md) · [Spis treści](../README.md) · [Następny →](19-wlasne-aplikacje.md)

Program może włączać aplikacje HP, definiować w nich funkcje, ustawiać okno wykresu, uruchamiać obliczenia i odczytywać wyniki. W ten sposób programy korzystają z całej mocy kalkulatora. Źródła: *ES #5*, *UG s. 647–648, 658–695*, *UG rozdz. 6–22*.

---

## 18.1. Systemowe zmienne równań (*ES #5*)

| Aplikacja | Zmienne (# = 0…9) | Zmienna niezależna |
|---|---|---|
| Function | `F#` | X |
| Polar | `R#` | θ |
| Parametric | `X#`, `Y#` | T |
| Sequence | `U#` | N (także N−1, N−2) |
| Advanced Graphing | `V#` | X, Y |
| Graph 3D | `FZ#` | X, Y |
| Solve | `E#` | dowolne zmienne |
| Statistics 1Var | `H1`–`H5` (analizy), dane w kolumnach `D0`–`D9` | |
| Statistics 2Var | `S1`–`S5` (analizy), dane w kolumnach `C0`–`C9` | |

### Definiowanie równań w programie

Wyrażenie przypisujesz jako **tekst** (*ES #5*):

```
F1 := "2*X^3";                       // f(x) = 2x³ w Function
R5 := "A*SIN(θ)";                    // r(θ) = A·sin θ w Polar
X1 := "V*COS(θ)*T";                  // Parametric
Y1 := "V*SIN(θ)*T-.5*G*T^2";
V1 := "A*X^2+B*Y^2+C*X*Y+D*X+E*Y+F=0"; // Advanced Graphing
```

Równanie zdefiniowane w ten sposób jest **niezaznaczone** (*unchecked*): nie rysuje się na wykresie i nie pojawia w tabeli, dopóki go nie zaznaczysz.

### Kolor wykresu (*ES #5*)

```
F8(COLOR) := RGB(0,0,255);     // F8 rysowana na niebiesko
F1(COLOR) := #FF8000h;
F1(COLOR)                      // odczyt koloru
```

Dotyczy `F#`, `R#`, `X#/Y#`, `U#`, `V#`, `E#`. Dla `FZ#` kolorem jest lista dwóch kolorów.

## 18.2. STARTAPP (*UG s. 647, ES #5*)

```
STARTAPP("nazwa aplikacji");
```

Uruchamia aplikację. Wykonuje przy tym jej funkcję `START` (jeśli ją ma) i otwiera domyślny widok. Nazwy aplikacji HP:

`"Function"`, `"Advanced Graphing"`, `"Graph 3D"`, `"Geometry"`, `"Spreadsheet"`, `"Statistics 1Var"`, `"Statistics 2Var"`, `"Inference"`, `"Solve"`, `"Linear Solver"`, `"Parametric"`, `"Polar"`, `"Sequence"`, `"Finance"`, `"Triangle Solver"`, `"Explorer"` (wyd. 3; w starszym firmware osobno `"Linear Explorer"`, `"Quadratic Explorer"`, `"Trig Explorer"`), a także Twoje własne aplikacje.

## 18.3. CHECK, UNCHECK, ISCHECK (*ES #5, UG rozdz. 23*)

```
CHECK(n);            // zaznacz równanie nr n (0..9) w bieżącej aplikacji
UNCHECK(n);          // odznacz
ISCHECK(n)           // 1, jeśli zaznaczone
Function.CHECK(1);   // wersja kwalifikowana (zalecana)
Solve.UNCHECK(0);
```

Zaznaczenie decyduje o tym, czy funkcja jest rysowana w Plot view i analizowana w Num view. To funkcje **konkretnej aplikacji**. Jeśli ta aplikacja nie jest bieżąca, poprzedź je nazwą aplikacji i kropką (*ES #5*).

## 18.4. STARTVIEW (*UG s. 647–648, ES #5*)

```
STARTVIEW(n [, przerysuj]);
```

| n | Widok |
|---|---|
| 0 | Symbolic (Symb) |
| 1 | Plot |
| 2 | Numeric (Num) |
| 3 | Symbolic Setup |
| 4 | Plot Setup |
| 5 | Numeric Setup |
| 6 | App Info (notatka aplikacji) |
| 7 | menu View |
| 8, 9, 10… | kolejne widoki specjalne z menu View (w Function: 8 Split Screen Plot Detail, 9 Split Screen Plot Table, 10 Autoscale, 11 Decimal, 12 Integer, 13 Trig) |
| -1 | Home |
| -2 | ustawienia Home (Modes) |
| -3 | Memory Manager |
| -4 | biblioteka aplikacji |
| -5 | katalog macierzy |
| -6 | katalog list |
| -7 | katalog programów |
| -8 | katalog notatek |

`przerysuj` różne od 0 wymusza natychmiastowe przerysowanie ekranu.

## 18.5. Zmienne widoku Plot (*UG s. 659–670*)

| Zmienna | Znaczenie | Aplikacje |
|---|---|---|
| `Xmin`, `Xmax`, `Ymin`, `Ymax` | okno wykresu | większość |
| `Xtick`, `Ytick` | odstęp podziałki | |
| `Xzoom`, `Yzoom` | współczynnik powiększenia (domyślnie 4) | |
| `Axes` | osie: 0 wł., 1 wył. | |
| `GridDots`, `GridLines` | siatka punktowa lub liniowa: 0 wł., 1 wył. | |
| `Labels` | etykiety zakresów osi | |
| `Cursor` | 0 krzyż, 1 odwrócony, 2 migający | |
| `Method` | 0 adaptacyjna, 1 odcinki, 2 punkty | Function, Solve, Parametric, Polar, Stats 2Var |
| `Recenter` | wyśrodkowanie przy powiększaniu | |
| `Tmin`, `Tmax`, `Tstep` | zakres i krok T | Parametric |
| `θmin`, `θmax`, `θstep` | zakres i krok θ | Polar |
| `Nmin`, `Nmax` | zakres N | Sequence |
| `SeqPlot` | 0 schodki, 1 pajęczyna | Sequence |
| `Hmin`, `Hmax`, `Hwidth` | histogram | Stats 1Var |
| `S1mark`–`S5mark` | znaczniki punktów | Stats 2Var |
| `PixSize`, `ScrollText` | | Geometry |

> Uwaga: w *UG* niektóre przełączniki (Axes, GridDots…) mają opis „0 = włączone, 1 = wyłączone”. Sprawdź zachowanie na swoim kalkulatorze i w razie potrzeby odwróć wartości.

Zmienne widoku Numeric: `NumStart`, `NumStep`, `NumType`, `NumZoom`, `NumIndep` (tabela własna) oraz ich warianty X/Y dla aplikacji z dwiema zmiennymi.

## 18.6. Zmienne ustawień Symbolic Setup (*UG s. 695*)

Nadpisują ustawienia Home tylko w danej aplikacji:

| Zmienna | Wartości |
|---|---|
| `AAngle` | 0 systemowy, 1 radiany, 2 stopnie, 3 grady |
| `AComplex` | 0 systemowy, 1 wł., 2 wył. |
| `AFormat` | 0 systemowy, 1 Standard, 2 Fixed, 3 Scientific, 4 Engineering |
| `ADigits` | liczba cyfr |

## 18.7. Zmienne wyników (*UG s. 695, rozdz. 23*)

Funkcje aplikacji zapisują wyniki w zmiennych, które program może odczytać:

| Aplikacja | Zmienne wyników |
|---|---|
| Function | `Root`, `Extremum`, `Isect`, `Slope`, `SignedArea` |
| Statistics 1Var | `NbItem`, `MinVal`, `Q1`, `MedVal`, `Q3`, `MaxVal`, `ΣX`, `ΣX2`, `MeanX`, `sX`, `σX`, `serrX`, `ssX` |
| Statistics 2Var | `NbItem`, `Corr`, `CoefDet`, `sCov`, `σCov`, `ΣXY`, `MeanX`, `MeanY`, `sX`, `sY`, `σX`, `σY`, `ΣX`, `ΣY`, `ΣX2`, `ΣY2`, … |
| Inference | `Result`, `TestScore`, `TestValue`, `Prob`, `CritScore`, `CritVal1`, `CritVal2`, `DF`, `Slope`, `Inter`, `Corr`, `CoefDet`, `ExpList`, `ExpMat`, `ContribList`, `ContribMat`, … |
| Finance | `NbPmt`, `IPYR`, `PV`, `PMT`, `FV`, `PPYR`, `CPYR`, `BEG`, `GSize` |
| Triangle Solver | `SideA`, `SideB`, `SideC`, `AngleA`, `AngleB`, `AngleC`, `TriType` |
| Linear Solver | `LSystem` |

## 18.8. Funkcje aplikacji (*UG rozdz. 23: App menu*)

| Aplikacja | Funkcje |
|---|---|
| Function | `AREA(Fn,[Fm,]a,b)`, `EXTREMUM(Fn,x)`, `ISECT(Fn,Fm,x)`, `ROOT(Fn,x)`, `SLOPE(Fn,x)` |
| Solve | `SOLVE(En, zmienna, start)` |
| Spreadsheet | `SUM`, `AVERAGE`, `AMORT`, `STAT1`, `STAT2`, `REGRS`, `PredY`, `PredX`, testy `HypZ1mean`… i przedziały `ConfZ1mean`… |
| Statistics 1Var | `Do1VStats(Hn)`, `SetSample(Hn,Dn)`, `SetFreq(Hn,Dn lub wartość)` |
| Statistics 2Var | `PredX`, `PredY`, `Resid`, `Do2VStats(Sn)`, `SetDepend(Sn,Cn)`, `SetIndep(Sn,Cn)` |
| Inference | `DoInference`, `HypZ1mean`, `HypZ2mean`, `HypZ1prop`, `HypZ2prop`, `HypT1mean`, `HypT2mean`, `ConfZ1mean`, …, `Chi2GOF`, `Chi2TwoWay`, `LinRegrTTest`, … |
| Finance | TVM: `TvmPMT`, `TvmPV`, `TvmFV`, `TvmIPYR`, `TvmNbPmt` (starsze `CalcPMT`â¦ z tymi samymi argumentami `(NbPmt, IPYR, PV, FV, [PPYR], [CPYR], [BEG])`), `DoFinance(zmienna)`; wyd. 3 dodaje `IntConv*`, `DateDays`, `CashFlow*`, `Depreciate`, `BrkEv*`, `Change*`, `Percent*`, `BondPrice`, `BondYield`, `BlackScholes` (zob. [rozdz. 23](23-mapa-instrukcji.md#rozdz-20--finance-app-s-334365-mocno-rozbudowany-w-wyd-3)) |
| Linear Solver | `Solve2x2`, `Solve3x3`, `LinSolve` |
| Triangle Solver | `AAS`, `ASA`, `SAS`, `SSA`, `SSS`, `DoSolve` |
| Explorer (wyd. 3) | `LinearSlope`, `LinearYIntercept`, `QuadSolve`, `QuadDelta` |
| Linear/Quadratic Explorer (starszy firmware) | `SolveForSlope`, `SolveForYIntercept`, `SOLVE`, `DELTA` |
| wszystkie | `CHECK`, `UNCHECK`, `ISCHECK` |

Szczegółową składnię każdej funkcji znajdziesz w pomocy kalkulatora: podświetl funkcję w menu i naciśnij [Help].

## 18.9. Program CONIC (*ES #5*)

Program rysuje stożkową Ax² + By² + Cxy + Dx + Ey + F = 0 w wybranym kolorze:

```
EXPORT CONIC()
BEGIN
  LOCAL cr, cg, cb, I;

  INPUT({A,B,C,D,E,F}, "Ax^2+By^2+Cxy+Dx+Ey+F", {}, {},
        {0,0,0,0,0,0}, {0,0,0,0,0,0});

  CHOOSE(I, "Choose a Color", "Red","Blue","Orange","Green");
  cr := {255,0,255,0};       // składowe R kolorów
  cg := {0,0,127,255};       // G
  cb := {0,255,0,0};         // B

  STARTAPP("Advanced Graphing");
  // jesteśmy w Advanced Graphing, więc CHECK nie wymaga prefiksu
  V1 := "A*X^2+B*Y^2+C*X*Y+D*X+E*Y+F=0";
  V1(COLOR) := RGB(cr(I),cg(I),cb(I));
  CHECK(1);
  STARTVIEW(1,1);            // Plot view
END;
```

## 18.10. Program PROJ13 — rzut ukośny (*ES #5*)

Program oblicza zasięg i wysokość rzutu oraz rysuje tor lotu w aplikacji Parametric:

x = V·cos θ·t, y = V·sin θ·t − ½·g·t²

```
EXPORT PROJ13()
BEGIN
  LOCAL M, str;
  // V, G, θ to zmienne globalne (używane we wzorach X1, Y1)
  HAngle := 1;                           // stopnie

  CHOOSE(M, "Units", "SI", "US");
  IF M==1 THEN
    str := "m";  G := 9.80665;
  ELSE
    str := "ft"; G := 32.17404;
  END;

  INPUT({V, θ}, "Data", {"V:","θ:"},
        {"Initial Velocity in "+str+"/s", "Initial Angle in Degrees"});

  X1 := "V*COS(θ)*T";
  Y1 := "V*SIN(θ)*T-.5*G*T^2";

  STARTAPP("Parametric");
  CHECK(1);
  Xmin := 0;
  Xmax := V^2/G*SIN(2*θ);                // zasięg
  Ymin := 0;
  Ymax := (V^2*SIN(θ)^2)/(2*G);          // maksymalna wysokość
  MSGBOX("Range: "+Xmax+" "+str+", Height: "+Ymax+" "+str);
  STARTVIEW(1,1);
END;
```

> Dlaczego `V`, `G`, `θ` są globalne? Wzory `X1`/`Y1` oblicza aplikacja Parametric **po zakończeniu programu**. Zmienne lokalne już wtedy nie istnieją.

Warto dopisać ustawienie `Tmax` na czas lotu: `Tmax := 2*V*SIN(θ)/G;`. Bez tego wykres może być ucięty.

## 18.11. Statystyka z programu

```
EXPORT STAT_OCENY()
BEGIN
  LOCAL dane := {4,5,3,5,2,4,4,5,3,4};
  D1 := dane;                 // dane do kolumny D1 (Statistics 1Var)
  SetSample(H1, D1);
  SetFreq(H1, 1);             // każda wartość z częstością 1
  Do1VStats(H1);
  RETURN {"średnia", MeanX, "mediana", MedVal, "odch. std", σX};
END;
```

Zmienne `D1`, `H1`, `MeanX` należą do aplikacji Statistics 1Var. Jeśli bieżąca aplikacja jest inna, wywołaj najpierw `STARTAPP("Statistics 1Var")` albo kwalifikuj nazwy nazwą aplikacji. Poprawny zapis kwalifikowanej nazwy (nazwa aplikacji zawiera spację) najłatwiej wstawić z menu [Vars] (App).

## 18.12. Aplikacja Solve z programu

Równanie zapisujesz w zmiennej `E#` (jak funkcje w `F#`) i rozwiązujesz funkcją `SOLVE`:

```
EXPORT RUCH_JEDNOST()
BEGIN
  // s = v0*t + a*t^2/2 dla s=100, v0=5, a=2; szukamy t (tu: X)
  E1 := "5*X+2*X^2/2-100";
  RETURN Solve.SOLVE(E1, X, 1);    // start od X=1; wynik ≈ 7.8078
END;
```

Przykład z instrukcji to `SOLVE(X^2-X-2, X, 3)`, który zwraca 2.

---

## Sprawdź się

1. Napisz program, który rysuje w aplikacji Function wykresy sin x, cos x i tan x w trzech kolorach, z oknem od −2π do 2π.
2. Napisz program `STYCZNA_WYKRES(f, x0)`: zapisuje f w F1, a styczną w x0 w F2, ustawia okno wokół x0 i pokazuje wykres.
3. Użyj aplikacji Statistics 2Var z programu: dopasuj prostą regresji do danych `{1,2,3,4,5}` i `{2.1,3.9,6.2,7.8,10.1}`. Zwróć współczynnik korelacji.

[Rozwiązania →](25-cwiczenia.md#rozdział-18)
