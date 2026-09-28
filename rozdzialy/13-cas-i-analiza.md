# Rozdział 13. CAS i analiza matematyczna w programach

[← Poprzedni](12-liczby-calkowite.md) · [Spis treści](../README.md) · [Następny →](14-grafika-podstawy.md)

W tym rozdziale:
- czym różni się Home od CAS,
- jak wywoływać funkcje CAS z programu PPL (`CAS.`),
- pochodne, całki, sumy i podstawianie z szablonów [Template] (*ES #9*),
- numeryczne rozwiązywanie równań w programie.

---

## 13.1. Home a CAS (*UG rozdz. 4, s. 47–53*)

| | Home | CAS |
|---|---|---|
| obliczenia | numeryczne, 12 cyfr | symboliczne, dokładne |
| `1/3+2/7` | `0.619047619047` | `13/21` |
| zmienne | wielkie litery (A–Z…) | małe litery (a–z) |
| tryb RPN | tak | nie |
| programy PPL | działają w kontekście Home | można je wywołać, ale liczą numerycznie |

Najważniejsze ustawienia CAS (ekran CAS Settings, *UG s. 50*): jednostka kąta, tryb **Exact** (dokładny lub przybliżony), **Complex** (wyniki zespolone), poziom upraszczania (None/Minimum/Maximum), **Principal** (rozwiązania główne funkcji trygonometrycznych), kolejność potęg w wielomianach. Na stronie 2 są limity rekurencji i wartość epsilon (*UG s. 51–52*).

Menu CAS ([Toolbox] (CAS)) domyślnie pokazuje nazwy opisowe. Po odznaczeniu *Menu Display* w ustawieniach Home zobaczysz nazwy komend (*UG s. 52*):

| Nazwa opisowa | Komenda |
|---|---|
| Factor List | `ifactors` |
| Complex Zeroes | `cZeros` |
| Groebner Basis | `gbasis` |
| Factor by Degree | `factor_xn` |
| Find Roots | `proot` |

Przykłady z instrukcji: `proot([2,3,-2])` daje pierwiastki wielomianu 2x²+3x−2, a `int(5*x^2-6,x,1,3)` pole pod wykresem.

## 13.2. Wywoływanie funkcji CAS z programu (*UG s. 655*)

```
CAS.funkcja(argumenty)
CAS.zmienna
```

Prefiks `CAS.` jest wymagany dla funkcji, które istnieją tylko w CAS (*ES #2*). Funkcje CAS o argumentach **liczbowych** są pewne i proste w użyciu:

| Wywołanie | Wynik |
|---|---|
| `CAS.idivis(12)` | dzielniki: 1, 2, 3, 4, 6, 12 |
| `CAS.isprime(97)` | 1 (prawda) |
| `CAS.ifactor(360)` | rozkład na czynniki pierwsze |
| `CAS.gcd(84,36)` | 12 |
| `CAS.lcm(4,6)` | 12 |
| `CAS.nextprime(100)` | 101 |

Program z instrukcji wykorzystujący `idivis` to MAXFACTORS ([rozdział 6](06-petle.md)). Z tutorialu Shore'a pochodzi SUMDIV.

### Obliczenia symboliczne z programu

Wynik funkcji CAS o argumentach symbolicznych (np. pochodna wyrażenia z `x`) jest wyrażeniem symbolicznym typu CAS, czyli `TYPE` 14.x. Przekazywanie wyrażeń symbolicznych z programu PPL do CAS bywa kapryśne i zależy od wersji firmware. Do takich zadań najlepiej napisać **program CAS**, zapisany między `#cas` a `#end`. Jest to funkcja nowszego firmware, nieopisana w instrukcji z 2016 r. Taki program działa w całości w kontekście CAS, więc małe litery oznaczają w nim zmienne symboliczne:

```
#cas
POCHODNA(f,x):=
BEGIN
  RETURN diff(f,x);
END;
#end
```

W CAS: `POCHODNA(x^3*sin(x), x)` → `3*x^2*sin(x)+x^3*cos(x)`.

> Jeśli potrzebujesz tylko **liczb** (wartość pochodnej w punkcie, całka oznaczona), użyj szablonów numerycznych z sekcji 13.3. Działają w zwykłym programie PPL bez komplikacji.

## 13.3. Analiza numeryczna z klawisza [Template] (*ES #9*)

Klawisz **[Template]** (górny rząd szarych klawiszy, trzeci od lewej) wstawia szablony matematyczne. W programie PPL (kontekst Home) liczą one **numerycznie**:

| Operacja | Składnia | Położenie w [Template] |
|---|---|---|
| pochodna w punkcie | `∂(f(zm), zm=wartość)` | rząd 1, kolumna 4 |
| całka oznaczona | `∫(f(zm), zm, a, b)` | rząd 2, kolumna 4 |
| całka podwójna | `∫(∫(f(X,Y), X, A, B), Y, C, D)` | zagnieżdżone szablony całki |
| suma | `Σ(f(zm), zm, od, do)` (krok 1, granice całkowite) | rząd 3, kolumna 3 |
| podstawienie („where”, pionowa kreska) | `wyrażenie\|zm=wartość` lub `wyrażenie\|{z1=w1, z2=w2}` | rząd 1, kolumna 3 |

Program CALCDEMO (*ES #9*):

```
EXPORT CALCDEMO()
BEGIN
  PRINT();
  HAngle := 0;                                   // radiany

  PRINT("d/dx sin x at x = π/4");
  PRINT(∂(SIN(X),X=π/4));                        // 0.707106781186

  PRINT("∫(sin x dx) from x=1 to x=3");
  PRINT(∫(SIN(X),X,1,3));                        // 1.53029480247

  PRINT("∫(∫(x^2*y dx)dy), x=[1,2], y=[0,4]");
  PRINT(∫(∫(X^2*Y,X,1,2),Y,0,4));                // 18.6666666667

  PRINT("Σ(n^3) from n=1 to n=12");
  PRINT(Σ(N^3,N,1,12));                          // 6084

  PRINT("Calculate 3*a+5 when a=-2");
  PRINT(3*A+5|A=-2);                             // -1
END;
```

## 13.4. Rozwiązywanie równań numerycznie

### Funkcje aplikacji Function (*UG s. 419–428*)

Działają na funkcjach `F0`–`F9` aplikacji Function:

| Funkcja | Działanie |
|---|---|
| `ROOT(Fn, start)` | miejsce zerowe najbliższe punktu start |
| `EXTREMUM(Fn, start)` | ekstremum najbliższe punktu start |
| `ISECT(Fn, Fm, start)` | punkt przecięcia dwóch funkcji |
| `SLOPE(Fn, x)` | nachylenie (pochodna) w punkcie x |
| `AREA(Fn, [Fm,] a, b)` | pole ze znakiem pod krzywą lub między krzywymi |

```
EXPORT MZERO()
BEGIN
  F1 := "X^3-2*X-5";                // definicja funkcji F1 jako tekst
  RETURN Function.ROOT(F1, 2);      // ≈ 2.09455148154
END;
```

### SOLVE z aplikacji Solve (*UG s. 428*)

```
SOLVE(En, zmienna, start)
```

Przykład z instrukcji: `SOLVE(X^2-X-2, X, 3)` → 2. Funkcja zwraca też kod rodzaju rozwiązania: 0 dokładne, 1 przybliżone, 2 znalezione tylko ekstremum najbliższe rozwiązaniu.

### Własna metoda: bisekcja

Własny algorytm daje pełną kontrolę nad obliczeniami. Funkcję podajemy jako tekst i obliczamy przez podstawienie `|`:

```
EXPORT BISEKCJA(fs, a, b, eps)
BEGIN
  LOCAL c, fa, fc;
  fa := EXPR(fs+"|X="+a);
  IF fa*EXPR(fs+"|X="+b) > 0 THEN
    RETURN "Brak zmiany znaku na [a,b]";
  END;
  WHILE (b-a) > eps DO
    c := (a+b)/2;
    fc := EXPR(fs+"|X="+c);
    IF fa*fc <= 0 THEN
      b := c;
    ELSE
      a := c; fa := fc;
    END;
  END;
  RETURN (a+b)/2;
END;
```

`BISEKCJA("X^3-2*X-5", 2, 3, 1E-10)` → ≈ 2.09455148154.

> Prostsza wersja podstawiania: zapisz wartość w globalnym `X` (`X:=c;`) i oblicz `EXPR(fs)`.

### Metoda Newtona z pochodną numeryczną

```
EXPORT NEWTON(fs, x0)
BEGIN
  LOCAL i, x := x0, fx, d;
  FOR i FROM 1 TO 50 DO
    X := x;
    fx := EXPR(fs);
    d := EXPR("∂("+fs+",X="+x+")");
    IF d==0 THEN RETURN "Pochodna = 0"; END;
    x := x - fx/d;
    IF ABS(fx) < 1E-12 THEN BREAK; END;
  END;
  RETURN x;
END;
```

## 13.5. Inne przydatne funkcje

| Funkcja | Działanie |
|---|---|
| `ITERATE(wyr, zm, start, n)` | n-krotne złożenie funkcji: `ITERATE(X^2,X,2,3)` → 256 |
| `TEVAL(wyr)` | czas obliczania wyrażenia w sekundach |
| `EXPR("tekst")` | oblicza tekst jako wyrażenie |
| `approx(wyr)` | przybliżenie numeryczne wyniku CAS |

---

## Sprawdź się

1. `TRAPEZY(fs,a,b,n)`: całkowanie metodą trapezów. Porównaj wynik z `∫` z [Template].
2. `STYCZNA(fs, x0)`: zwróć listę `{a, b}` współczynników stycznej y = ax+b w punkcie x0.
3. Porównaj czasy `TEVAL` dla `Σ(N^2,N,1,1000)` i pętli `FOR` sumującej te same liczby.

[Rozwiązania →](25-cwiczenia.md#rozdział-13)
