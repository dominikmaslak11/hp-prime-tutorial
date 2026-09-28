# Rozdział 17. Klawiatura, dotyk i czas: GETKEY, ISKEYDOWN, MOUSE, TICKS

[← Poprzedni](16-grafika-zaawansowana.md) · [Spis treści](../README.md) · [Następny →](18-sterowanie-aplikacjami.md)

Źródła: *ES #3*, *UG s. 607–612*.

---

## 17.1. GETKEY (*UG s. 607, ES #3*)

```
k := GETKEY;
```

- Zwraca kod **pierwszego klawisza z bufora klawiatury** albo **-1**, jeśli od ostatniego wywołania nie naciśnięto żadnego klawisza.
- **Nie czeka** na naciśnięcie. Oczekiwanie trzeba zrobić samodzielnie w pętli.
- Kody mają wartości od 0 do 50 i są numerowane od lewego górnego klawisza do prawego dolnego.

### Kody klawiszy

| Klawisz | Kod | Klawisz | Kod | Klawisz | Kod | Klawisz | Kod |
|---|---|---|---|---|---|---|---|
| Apps | 0 | Symb | 1 | ▲ | 2 | Help | 3 |
| Esc | 4 | Home | 5 | Plot | 6 | ◀ | 7 |
| ▶ | 8 | View | 9 | CAS | 10 | Num | 11 |
| ▼ | 12 | Menu | 13 | Vars | 14 | Toolbox | 15 |
| Template | 16 | x,t,θ,n | 17 | a b/c | 18 | Backspace (Del) | 19 |
| xʸ | 20 | SIN | 21 | COS | 22 | TAN | 23 |
| LN | 24 | LOG | 25 | x² | 26 | +/- | 27 |
| ( ) | 28 | , | 29 | Enter | 30 | EEX | 31 |
| 7 | 32 | 8 | 33 | 9 | 34 | ÷ | 35 |
| ALPHA | 36 | 4 | 37 | 5 | 38 | 6 | 39 |
| × | 40 | Shift | 41 | 1 | 42 | 2 | 43 |
| 3 | 44 | − | 45 | On | 46 | 0 | 47 |
| . | 48 | spacja | 49 | + | 50 | | |

Pełna tabela z układem klawiatury jest w [dodatku B](../dodatki/B-kody-klawiszy.md).

Program KEYNO (*ES #3*) pokazuje kod każdego naciśniętego klawisza i kończy się po [Enter]:

```
EXPORT KEYNO()
BEGIN
  LOCAL K;
  PRINT();
  PRINT("Press any key to get its code.");
  PRINT("Press Enter to exit.");
  REPEAT
    K := GETKEY;
    IF K <> -1 THEN
      PRINT(K);
    END;
  UNTIL K==30;
END;
```

> Tutorial Shore'a sprawdza w tym miejscu `K≠0`, a nie `K≠-1`. Instrukcja HP podaje, że brak klawisza to **-1**. Kod 0 to klawisz [Apps].

### Oczekiwanie na klawisz

```
WAITKEY();

EXPORT TEST_WK()
BEGIN
  LOCAL k := WAITKEY();
  RETURN k;
END;

WAITKEY()
BEGIN
  LOCAL k;
  REPEAT
    k := GETKEY;
  UNTIL k <> -1;
  RETURN k;
END;
```

W nowszym firmware to samo robi `WAIT(-1)`. Zwraca kod klawisza albo listę z informacją o dotyku.

## 17.2. ISKEYDOWN (*UG s. 609*)

```
ISKEYDOWN(kod)   // 1, jeśli klawisz jest TERAZ wciśnięty; 0 w przeciwnym razie
```

Różnica między komendami:
- `GETKEY` odczytuje **zdarzenie** naciśnięcia z bufora. Każde naciśnięcie zostaje odczytane dokładnie raz.
- `ISKEYDOWN` sprawdza **stan** klawisza w danej chwili. Pozwala wykryć przytrzymanie i kilka klawiszy wciśniętych naraz, np. ruch po skosie w grze.

```
// płynny ruch kwadratu strzałkami; [Esc] kończy
EXPORT RUCH()
BEGIN
  LOCAL x:=150, y:=100;
  DIMGROB_P(G1,320,240);
  REPEAT
    IF ISKEYDOWN(7)  THEN x := MAX(0,   x-3); END;   // ◀
    IF ISKEYDOWN(8)  THEN x := MIN(300, x+3); END;   // ▶
    IF ISKEYDOWN(2)  THEN y := MAX(0,   y-3); END;   // ▲
    IF ISKEYDOWN(12) THEN y := MIN(200, y+3); END;   // ▼
    RECT_P(G1,0,0,319,239,#FFFFFFh,#FFFFFFh);
    RECT_P(G1,x,y,x+19,y+19,#0070C0h,#0070C0h);
    TEXTOUT_P("Strzałki = ruch, Esc = koniec",G1,40,222,1);
    BLIT_P(G0,G1);
    WAIT(0.02);
  UNTIL ISKEYDOWN(4);                                 // Esc
END;
```

## 17.3. MOUSE — ekran dotykowy (*UG s. 609*)

```
m := MOUSE;          // dwie listy, po jednej na każdy możliwy punkt dotyku
m := MOUSE(n);       // tylko n-ty element opisu (lub -1, gdy brak dotyku)
```

Każda lista punktu dotyku ma postać:

```
{x, y, x_początkowe, y_początkowe, typ}
```

| typ | Znaczenie |
|---|---|
| 0 | nowe dotknięcie |
| 1 | zakończone (puszczenie) |
| 2 | przeciąganie |
| 3 | rozciąganie (dwoma palcami) |
| 4 | obrót |
| 5 | długie przytrzymanie |

Gdy ekran nie jest dotykany, listy są puste. Wzorzec odczytu:

```
m := MOUSE;
IF SIZE(m(1)) > 0 THEN
  x := m(1,1);
  y := m(1,2);
  // obsłuż dotknięcie w punkcie (x,y)
END;
```

Współrzędne są **pikselowe**, w tym samym układzie co `_P`.

### Przyciski ekranowe: DRAWMENU + MOUSE

```
EXPORT MENU_DOTYK()
BEGIN
  LOCAL m, x, y, nr, dziala := 1;
  RECT();
  TEXTOUT_P("Dotknij przycisku", 90, 80, 4);
  DRAWMENU("Czerw.","Ziel.","Nieb.","","","Koniec");
  WHILE dziala DO
    m := MOUSE;
    IF SIZE(m(1)) > 0 THEN
      x := m(1,1); y := m(1,2);
      IF y >= 220 THEN                     // dotyk w pasku menu
        nr := IP(x/(320/6)) + 1;           // numer przycisku 1..6
        CASE
          IF nr==1 THEN RECT_P(0,0,319,219,#FF0000h,#FF0000h); END;
          IF nr==2 THEN RECT_P(0,0,319,219,#00C000h,#00C000h); END;
          IF nr==3 THEN RECT_P(0,0,319,219,#0000FFh,#0000FFh); END;
          IF nr==6 THEN dziala := 0; END;
        END;
        // poczekaj na puszczenie palca, żeby jeden dotyk = jedna akcja
        REPEAT m := MOUSE; UNTIL SIZE(m(1))==0;
      END;
    END;
    IF GETKEY == 4 THEN dziala := 0; END; // Esc też kończy
  END;
END;
```

### Rysowanie palcem

```
EXPORT RYSUJ()
BEGIN
  LOCAL m, px:=-1, py:=-1, x, y;
  RECT();
  TEXTOUT_P("Rysuj palcem. Esc = koniec", 60, 222, 1);
  REPEAT
    m := MOUSE;
    IF SIZE(m(1)) > 0 THEN
      x := m(1,1); y := m(1,2);
      IF px >= 0 THEN LINE_P(px,py,x,y,#000000h); END;
      px := x; py := y;
    ELSE
      px := -1;                          // palec podniesiony: nowa kreska
    END;
  UNTIL GETKEY == 4;
END;
```

## 17.4. Czas: TICKS, TEVAL, WAIT, Date, Time

| Element | Działanie |
|---|---|
| `TICKS` | wewnętrzny zegar w **milisekundach** (*UG s. 612*) |
| `TEVAL(wyr)` | czas obliczenia wyrażenia w sekundach (*UG s. 613*) |
| `WAIT(n)` | pauza n sekund (może być ułamek, np. 0.05) |
| `Date` | data jako RRRR.MMDD, np. 2026.0928 |
| `Time` | bieżąca godzina w formacie sześćdziesiątkowym |
| `TOff` | czas do automatycznego wyłączenia (ms) |

### Pomiar czasu wykonania

```
EXPORT CZAS_PETLI(n)
BEGIN
  LOCAL t0 := TICKS, i, s := 0;
  FOR i FROM 1 TO n DO
    s := s + i^2;
  END;
  RETURN {s, (TICKS - t0)/1000 + " s"};
END;
```

### Stoper

```
EXPORT STOPER()
BEGIN
  LOCAL t0, t, k;
  RECT();
  TEXTOUT_P("Enter = start/stop, Esc = wyjście", 20, 200, 2);
  REPEAT k := GETKEY; UNTIL k==30 OR k==4;      // czekaj na Enter
  IF k==4 THEN KILL; END;
  t0 := TICKS;
  REPEAT
    t := (TICKS - t0)/1000;
    TEXTOUT_P(STRING(t,2,2)+" s", 100, 90, 7, #000000h, 160, #FFFFFFh);
    k := GETKEY;
  UNTIL k==30 OR k==4;
  FREEZE;
END;
```

`TEXTOUT_P` z kolorem tła i szerokością zamalowuje poprzednią wartość. Dzięki temu można odświeżać liczbę bez czyszczenia całego ekranu.

### Gra na refleks

```
EXPORT REFLEKS()
BEGIN
  LOCAL t0, t, k;
  RECT();
  TEXTOUT_P("Czekaj na zielone...", 80, 100, 4);
  WAIT(1 + RANDOM*3);                     // losowe opóźnienie 1–4 s
  WHILE GETKEY <> -1 DO END;              // wyczyść bufor (falstarty)
  RECT(#00C000h);
  TEXTOUT_P("TERAZ! Naciśnij dowolny klawisz", 30, 100, 4, #FFFFFFh);
  t0 := TICKS;
  REPEAT k := GETKEY; UNTIL k <> -1;
  t := TICKS - t0;
  MSGBOX("Czas reakcji: " + t + " ms");
END;
```

---

## Sprawdź się

1. Napisz program, który rysuje kropkę w miejscu każdego dotknięcia ekranu, w kolorze zależnym od typu dotyku.
2. Rozbuduj `RUCH`: kwadrat nie może wjechać na czarną przeszkodę narysowaną na środku ekranu. Użyj `GETPIX_P`.
3. Napisz `TEST_CYFR()`: program losuje cyfrę 0–9 i wyświetla ją, a użytkownik musi nacisnąć klawisz tej cyfry. Kody klawiszy cyfr zapisz w liście. Po 10 próbach program podaje średni czas reakcji i liczbę błędów.

[Rozwiązania →](25-cwiczenia.md#rozdział-17)
