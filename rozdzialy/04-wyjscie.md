# Rozdział 4. Wyświetlanie wyników: RETURN, MSGBOX, PRINT

[← Poprzedni](03-typy-danych-i-operatory.md) · [Spis treści](../README.md) · [Następny →](05-warunki.md)

Program może przekazać wyniki na trzy podstawowe sposoby. Czwarty, grafika (`TEXTOUT_P`), jest opisany w [rozdziale 14](14-grafika-podstawy.md).

| Komenda | Gdzie trafia wynik | Czy wstrzymuje program | Typowe zastosowanie |
|---|---|---|---|
| `RETURN` | do historii Home lub do funkcji wywołującej | kończy funkcję | funkcje obliczeniowe |
| `MSGBOX` | okienko na ekranie | tak, do naciśnięcia OK | komunikaty, wynik końcowy |
| `PRINT` | terminal tekstowy | nie | wiele linii wyników, śledzenie działania |

---

## 4.1. RETURN (*UG s. 627, ES #1*)

```
RETURN wyrażenie;
```

- Zwraca wartość wyrażenia i **natychmiast kończy** funkcję. Polecenia między `RETURN` a `END` nie zostaną wykonane (*UG s. 613*).
- Może zwrócić dowolny typ: liczbę, listę, macierz, tekst.
- Aby zwrócić kilka wartości naraz, zwróć listę: `RETURN {x1, x2};`.
- Jeśli funkcja nie ma `RETURN`, program uruchomiony z Home zwraca wynik ostatniego wykonanego polecenia (*UG s. 613*).

```
EXPORT SQM1(X)
BEGIN
  RETURN X^2-1;
END;
```
`SQM1(8)` → `63`.

`RETURN` może stać w kilku miejscach funkcji, np. w różnych gałęziach `IF`:

```
EXPORT ZNAK(x)
BEGIN
  IF x>0 THEN RETURN 1; END;
  IF x<0 THEN RETURN -1; END;
  RETURN 0;
END;
```

## 4.2. MSGBOX (*UG s. 653, ES #2*)

```
MSGBOX(wyrażenie_lub_tekst [, ok_cancel]);
```

- Wyświetla okienko z komunikatem i czeka na naciśnięcie (OK) lub [Enter].
- Jeśli `ok_cancel` jest prawdą (np. 1), okienko ma przyciski (OK) i (Cancel).
- Zwraca **1**, gdy użytkownik naciśnie OK, i **0**, gdy naciśnie Cancel lub [Esc].
- Komunikat to **jeden** tekst. Kilka wartości łączysz operatorem `+`.

Program COMLOCK (*ES #2*) losuje szyfr do kłódki: trzy liczby z zakresu 0–39.

```
EXPORT COMLOCK()
BEGIN
  LOCAL L0;
  L0 := RANDINT(3,0,39);        // lista 3 losowych liczb 0..39
  MSGBOX("SECRET: "+L0(1)+","+L0(2)+","+L0(3));
END;
```

Pytanie z potwierdzeniem:

```
EXPORT KASUJ()
BEGIN
  IF MSGBOX("Wyczyścić listę L1?", 1) THEN
    L1 := {};
    MSGBOX("Wyczyszczono.");
  END;
END;
```

Przykład z instrukcji (*UG s. 653*):

```
EXPORT AREACALC()
BEGIN
  LOCAL radius;
  INPUT(radius, "Radius of Circle", "r = ", "Enter radius", 1);
  MSGBOX("The area is " + π*radius^2);
END;
```

## 4.3. PRINT i terminal (*UG s. 654, ES #2*)

```
PRINT(wyrażenie_lub_tekst);   // dopisuje linię w terminalu
PRINT();                      // czyści terminal
```

- Terminal to tekstowy ekran wyników programu. Pojawia się, gdy program coś do niego wypisze.
- Każde `PRINT` dopisuje nową linię.
- **Dobry zwyczaj:** na początku programu, który używa `PRINT`, wywołaj `PRINT();`, żeby usunąć wyniki poprzednich programów (*ES #2*).
- W terminalu po zakończeniu programu klawisze strzałek przewijają tekst, [Del] czyści terminal, a inny klawisz zamyka terminal (*UG s. 654*).
- Terminal można otworzyć w dowolnym momencie: przytrzymaj [On] i naciśnij [÷].

Program QROOTS (*ES #2*) rozwiązuje równanie kwadratowe i obsługuje pierwiastki zespolone:

```
EXPORT QROOTS(A,B,C)
BEGIN
  LOCAL D;
  PRINT();                 // czyść terminal
  HComplex := 1;           // pozwól na wyniki zespolone
  D := B^2-4*A*C;
  IF D>=0 THEN
    PRINT("Roots are real.");
  ELSE
    PRINT("Roots are complex.");
  END;
  PRINT((-B+√D)/(2*A));
  PRINT((-B-√D)/(2*A));
END;
```

| Wywołanie | Wynik w terminalu |
|---|---|
| `QROOTS(1,5,8)` | `Roots are complex.` / `-2.5+1.32287565553*i` / `-2.5-1.32287565553*i` |
| `QROOTS(2,-4,-8)` | `Roots are real.` / `3.2360679775` / `-1.2360679775` |

> **Uwaga.** Parametry `A`, `B`, `C` przesłaniają tu systemowe zmienne, bo parametry są lokalne. Program zmienia jednak `HComplex` na stałe. Lepsza wersja zapamiętałaby stare ustawienie i przywróciła je na koniec (zob. [rozdział 2.10](02-skladnia-i-zmienne.md#210-zmienne-systemowe-które-warto-znać-od-razu)).

## 4.4. Formatowanie liczb w tekście

Liczba doklejona do tekstu ma format zgodny z ustawieniami Home. Precyzyjne formatowanie daje `STRING` (pełny opis w [rozdziale 9](09-lancuchy.md)):

```
STRING(2/3, 2, 3)     // tryb Fixed, 3 miejsca po przecinku -> "0.667"
ROUND(2/3, 3)         // liczba 0.667 (to nie jest tekst)
```

Przykład:

```
PRINT("Pole = " + STRING(π*r^2, 2, 2) + " cm2");
```

## 4.5. Znaki specjalne w tekstach

- `\n` to nowa linia. Działa w `MSGBOX` i `PRINT`: `MSGBOX("Linia 1\nLinia 2");`
- `""` wewnątrz tekstu to znak cudzysłowu: `"Powiedział ""cześć"""`.
- `\\` to jeden ukośnik wsteczny.

## 4.6. Który sposób wybrać?

- **Funkcja matematyczna**, której wynik ma trafić do dalszych obliczeń: `RETURN`.
- **Program interaktywny** z jednym wynikiem końcowym: `MSGBOX` i dodatkowo `RETURN` (wynik zostanie w historii).
- **Wiele wyników, tabela, śledzenie pętli**: `PRINT`.
- **Własny wygląd ekranu, gry**: grafika ([rozdział 14](14-grafika-podstawy.md)).

---

## Sprawdź się

1. Napisz `TABLICZKA(n)`, który wypisuje w terminalu `n x 1 = …` do `n x 10 = …`.
2. Zmodyfikuj `QROOTS` tak, żeby przywracał poprzednią wartość `HComplex` i dodatkowo zwracał listę obu pierwiastków.
3. Napisz program, który pyta przez `MSGBOX(…,1)`, czy wylosować liczbę. Jeśli użytkownik potwierdzi, niech pokaże wynik `RANDINT(1,100)`.

[Rozwiązania →](25-cwiczenia.md#rozdział-4)
