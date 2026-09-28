# Rozdział 2. Składnia języka i zmienne

[← Poprzedni](01-srodowisko.md) · [Spis treści](../README.md) · [Następny →](03-typy-danych-i-operatory.md)

W tym rozdziale:
- budowa pliku programu,
- komentarze i średniki,
- cztery rodzaje zmiennych w HP Prime: Home, App, CAS i User,
- `LOCAL`, `EXPORT`, zmienne wspólne dla całego pliku,
- kwalifikowanie nazw (`Function.Xmin`).

---

## 2.1. Budowa pliku programu

Plik programu może zawierać dowolnie wiele **funkcji** (podprogramów) (*UG s. 596*). Każda funkcja ma:
1. **nagłówek**: nazwę i w nawiasach listę parametrów rozdzielonych przecinkami,
2. **ciało**: polecenia zamknięte w bloku `BEGIN … END;`.

Ogólny układ pliku:

```
// 1. deklaracje zapowiadające funkcje pomocnicze
POMOC();

// 2. zmienne eksportowane i zmienne wspólne dla pliku
EXPORT WYNIK;
licznik;

// 3. funkcja główna (eksportowana)
EXPORT GLOWNA(a,b)
BEGIN
  LOCAL x;
  x := POMOC(a) + b;
  WYNIK := x;
  RETURN x;
END;

// 4. definicje funkcji pomocniczych
POMOC(n)
BEGIN
  RETURN n^2;
END;
```

Szczegóły funkcji opisuje [rozdział 8](08-funkcje.md). Tutaj skupiamy się na zmiennych.

## 2.2. Średniki, wielkość liter, białe znaki

- **Każde polecenie kończy się średnikiem** `;` (*ES #1*). Kończą się nim także zamknięcia struktur: `END;`, `UNTIL warunek;`. Dotyczy to też `END;` zamykającego funkcję: bez tego średnika kalkulator nie skompiluje programu.
- **Wielkość liter ma znaczenie** w nazwach zmiennych: `MaxTemp` i `maxTemp` to dwie różne zmienne (*UG s. 611*). Nazwy komend wbudowanych kalkulator zwykle akceptuje w różnej pisowni, ale bezpieczniej pisać je wielkimi literami (`RETURN`, `FOR`). Wyjątek to kilka funkcji pisanych małymi literami, np. `when`, `irem`, `idivis`.
- Wcięcia i puste linie nie mają znaczenia dla działania programu, ale bardzo poprawiają czytelność.
- W jednej linii może być kilka poleceń: `A:=1; B:=2;`.

## 2.3. Komentarze

Komentarz zaczyna się od `//` i trwa do końca linii (*UG s. 597*):

```
EXPORT MYPROGRAM()
BEGIN
  PIXON(1,1);
  // Ta linia to tylko komentarz.
END;
```

> Uwaga: w programie SNOWFLAKE z tutorialu Shore'a komentarze są zapisane jako `\\`. To błąd druku. Poprawny zapis to `//`.

## 2.4. Przypisanie

```
A := 5;          // styl "programistyczny"
5 ▶ A;           // styl "kalkulatorowy" (Sto▶)
L1(3) := 7;      // przypisanie do elementu listy
M1(2,1) := 0;    // przypisanie do elementu macierzy
```

W HP PPL `=` **nie służy do przypisania**. W warunkach do porównania używaj `==`.

## 2.5. Cztery rodzaje zmiennych (*UG s. 658*)

HP Prime ma cztery rodzaje zmiennych. Wszystkie znajdziesz w menu [Vars].

### (1) Zmienne Home — globalne, typowane, zarezerwowane

Są zawsze dostępne, mają stały typ i nie można ich usunąć:

| Nazwy | Typ | Uwagi |
|---|---|---|
| `A`–`Z`, `θ` | liczba rzeczywista | Zapisanie w `A` listy powoduje błąd. |
| `Z0`–`Z9` | liczba zespolona | |
| `L0`–`L9` | lista | |
| `M0`–`M9` | macierz / wektor | |
| `G0`–`G9` | grafika (GROB) | `G0` to zawsze bieżący ekran. |

Zmienne Home mają tę samą wartość w Home i we wszystkich aplikacjach. Nie wolno ich używać jako nazw programów ani zapisywać w nich innego typu danych (*UG s. 611*).

### (2) Zmienne aplikacji (App) — ustawienia i wyniki aplikacji

Przykłady: `Xmin`, `Xmax`, `Root`, `F1`, `D1`, `HAngle`. Ta sama nazwa może istnieć w wielu aplikacjach (np. `Xmin` jest w Function, Polar, Parametric, Sequence i Solve), więc trzeba ją czasem **kwalifikować** (sekcja 2.9). Są opisane w [rozdziałach 18](18-sterowanie-aplikacjami.md) i [21](21-zmienne-systemowe.md).

### (3) Zmienne CAS — małe litery, bez typu

`a`–`z` (małe litery). Mogą przechowywać cokolwiek albo nie mieć żadnej wartości (wtedy są symbolem w obliczeniach symbolicznych). Zmienna CAS z przypisaną wartością jest widoczna także w Home.

### (4) Zmienne użytkownika (User) — Twoje własne

Tworzysz je:
- w Home przez przypisanie, np. `promien:=5` (kalkulator zapyta, czy utworzyć zmienną),
- w programie przez `LOCAL` (lokalne) albo `EXPORT` (globalne).

Nie mają typu: ta sama zmienna może raz przechowywać liczbę, a potem listę. Instrukcja odradza jednak takie mieszanie typów jako złą praktykę (*UG s. 611*).

Zasady nazw (*UG s. 611*):
- litery i cyfry, pierwszy znak musi być literą,
- rozróżniana jest wielkość liter,
- nazwy powinny być **opisowe**: `RADIUS` zamiast `VGFTRFG`.

## 2.6. `LOCAL` — zmienne lokalne (*UG s. 632, ES #1*)

```
LOCAL a, b, c;              // deklaracja; wartość początkowa = 0
LOCAL k := 1;               // deklaracja z wartością początkową
LOCAL s := 0, lst := {}, m; // można mieszać
```

- Zmienna lokalna istnieje tylko podczas wykonywania funkcji, w której ją zadeklarowano. Po zakończeniu funkcji znika.
- Domyślna wartość po deklaracji to **0** (*ES #1*).
- Zmienna lokalna może przechowywać dowolny typ danych.
- **Parametry funkcji są automatycznie lokalne.**
- Jedno polecenie `LOCAL` deklaruje **najwyżej 8 zmiennych** (składnia w pomocy HP: `LOCAL Var1[:=Val1, …, Var8:=Val8]`). Program z dziewięcioma nazwami w jednym `LOCAL` się nie skompiluje; potrzebujesz więcej, to dopisz drugie `LOCAL`. (Sprawdzone na emulatorze 2.4 przez projekt hp-prime-kit.)
- Zmienna lokalna **przesłania** zmienną globalną o tej samej nazwie. `LOCAL A;` w programie nie zmieni systemowej zmiennej `A`.

Przykład działania przesłaniania:

```
EXPORT TESTLOC()
BEGIN
  LOCAL A := 100;   // lokalne A przesłania systemowe A
  RETURN A;
END;
```

Wpisz w Home `A:=7`, potem `TESTLOC()` (wynik: 100), a potem `A`. Wynik to 7: systemowe `A` nie zmieniło się.

> **Dobra praktyka.** Wszystkie zmienne robocze deklaruj jako `LOCAL`. Program nie zmieni wtedy danych użytkownika, np. jego list `L1` czy zmiennej `A`. Instrukcja HP zaleca też pisanie zmiennych lokalnych **małymi literami** (*UG s. 655*), żeby łatwo odróżnić je od systemowych `A`–`Z`.

## 2.7. `EXPORT` — zmienne globalne użytkownika (*UG s. 612, 632*)

Jeśli wartość ma być dostępna po zakończeniu programu, wyeksportuj zmienną. Deklaracja `EXPORT` stoi **poza funkcją**, nad nagłówkiem:

```
EXPORT RADIUS;

EXPORT GETRADIUS()
BEGIN
  INPUT(RADIUS);
END;
```

Po uruchomieniu `GETRADIUS` zmienna `RADIUS` pojawi się w [Vars] (User) › GETRADIUS i będzie widoczna wszędzie, w tym w Home. Można ją też od razu zainicjować:

```
EXPORT ratio := 0.15;
EXPORT ROLLS, SIDES;
EXPORT a:=1, b:=2;
```

- Jeśli dwa programy eksportują zmienną o tej samej nazwie, aktywna jest ta wyeksportowana **ostatnio** (*UG s. 612*).
- Zmienne eksportowane zachowują wartość między uruchomieniami programu. Wartość może się wyzerować przy ponownej kompilacji programu (np. po edycji).

## 2.8. Zmienne wspólne dla pliku (bez EXPORT)

Zmienna zadeklarowana poza funkcjami **bez** słowa `EXPORT` jest widoczna we wszystkich funkcjach danego pliku, ale nie na zewnątrz. Przydaje się jako „pamięć” modułu, np. stan gry:

```
wynik;          // zmienna wspólna dla pliku
PUNKT();

EXPORT GRA()
BEGIN
  wynik := 0;
  PUNKT(); PUNKT(); PUNKT();
  RETURN wynik;     // 3
END;

PUNKT()
BEGIN
  wynik := wynik + 1;
END;
```

Kolejność zasięgów przy szukaniu nazwy: najpierw **lokalne** (LOCAL i parametry), potem **zmienne pliku**, potem **zmienne globalne** (eksportowane, Home, aplikacji).

## 2.9. Kwalifikowanie nazw (*UG s. 612*)

Wiele aplikacji ma zmienne o tej samej nazwie. Żeby wskazać konkretną, poprzedź nazwę zmiennej nazwą aplikacji lub programu i kropką:

```
Function.Xmin       // Xmin w aplikacji Function
Parametric.Xmin     // Xmin w aplikacji Parametric (może mieć inną wartość)
Polar.Xmin
GETRADIUS.RADIUS    // zmienna RADIUS wyeksportowana przez program GETRADIUS
```

Tak samo kwalifikuje się funkcje aplikacji (`Function.CHECK(1)`) i funkcje CAS (`CAS.idivis(12)`).

## 2.10. Zmienne systemowe, które warto znać od razu

| Zmienna | Znaczenie | Wartości |
|---|---|---|
| `HAngle` | tryb kątów w Home | 0 radiany, 1 stopnie, 2 grady |
| `HComplex` | wyniki zespolone z danych rzeczywistych | 0 wył., 1 wł. |
| `HFormat` | format liczb | 0 Standard, 1 Fixed, 2 Scientific, 3 Engineering |
| `HDigits` | liczba cyfr dla Fixed/Sci/Eng | według instrukcji 0 < n < 11 |
| `Ans` | ostatni wynik w Home | |
| `Date`, `Time` | data (RRRR.MMDD) i czas | |

Program może zmieniać te ustawienia, np. `HAngle:=1;` przełącza na stopnie. Dobry program zapamiętuje ustawienie i przywraca je na koniec:

```
EXPORT SINDEG(x)
BEGIN
  LOCAL stary := HAngle, w;
  HAngle := 1;          // stopnie
  w := SIN(x);
  HAngle := stary;      // przywróć ustawienie użytkownika
  RETURN w;
END;
```

Pełna lista: [dodatek C](../dodatki/C-tabele.md).

---

## Sprawdź się

1. Co zwróci program poniżej i jaką wartość będzie miało globalne `B` po jego wykonaniu, jeśli wcześniej w Home wykonano `B:=10`?
   ```
   EXPORT ZADANIE1()
   BEGIN
     LOCAL B := 3;
     B := B*2;
     RETURN B;
   END;
   ```
2. Napisz program `LICZNIK()`, który przy każdym uruchomieniu zwiększa wyeksportowaną zmienną `ILE` o 1 i ją zwraca.
3. Dlaczego instrukcja `L1 := 5;` spowoduje błąd?

[Rozwiązania →](25-cwiczenia.md#rozdział-2)
