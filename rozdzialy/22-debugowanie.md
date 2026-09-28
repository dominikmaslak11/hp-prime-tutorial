# Rozdział 22. Debugowanie, obsługa błędów i dobre praktyki

[← Poprzedni](21-zmienne-systemowe.md) · [Spis treści](../README.md) · [Następny →](23-mapa-instrukcji.md)

---

## 22.1. Trzy rodzaje błędów

| Rodzaj | Kiedy się pojawia | Jak go znaleźć |
|---|---|---|
| **składniowy** | przy kompilacji: brak `END`, średnika, nawiasu, literówka w komendzie | (Check) w edytorze pokazuje miejsce błędu |
| **wykonania** | w trakcie działania: zły typ, indeks poza listą, niezdefiniowana zmienna | komunikat błędu, debuger, `IFERR` |
| **logiczny** | program działa, ale wynik jest zły | testy na znanych danych, debuger, `PRINT` |

Programu z błędem składniowym **nie da się uruchomić** (*UG s. 563*).

## 22.2. Check (*ES #1, UG s. 556*)

- Naciskaj (Check) często, najlepiej po każdych kilku liniach.
- Check sprawdza wyłącznie składnię. Nie sprawdza, czy argumenty komend są poprawne (*ES #1*).
- Komunikat wskazuje miejsce, w którym kompilator się „zgubił”. Prawdziwa przyczyna często leży **wcześniej**, np. brakujące `END` kilka linii wyżej.

Najczęstsze błędy składniowe:

| Objaw | Przyczyna |
|---|---|
| błąd przy ostatnim `END;` | brakuje `END` w jednej z pętli lub instrukcji `IF` |
| błąd w linii po poleceniu | brak średnika na końcu poprzedniej linii |
| „Syntax error” przy nazwie funkcji | funkcja użyta przed deklaracją (brak `NAZWA();` na górze pliku) |
| błąd przy `=` | użyto `=` zamiast `:=` lub `==` |
| błąd przy nazwie zmiennej | nazwa zarezerwowana (`M1` jako liczba, `L1` jako liczba) |
| nieoczekiwany znak | cudzysłów typograficzny `„ ”` zamiast `" "` (częste przy kopiowaniu z internetu do Connectivity Kit) |

## 22.3. Debuger (*UG s. 563–564*)

Uruchomienie: w katalogu programów zaznacz program i naciśnij **(Debug)**. Jeśli plik ma kilka funkcji z `EXPORT`, wybierz jedną z listy.

Na ekranie debugera widać:
- u góry nazwę programu lub funkcji,
- pod nią **bieżącą linię**,
- w głównej części **wartości zmiennych**.

| Przycisk | Działanie |
|---|---|
| (Skip) | przeskakuje do następnej linii lub bloku bez wykonywania |
| (Step) | wykonuje bieżącą linię |
| (Vars) | dodaje zmienną do obserwowanych |
| (Exit) | zamyka debuger |
| (Cont) | kontynuuje bez debugowania |

Okna dialogowe (`MSGBOX`, `INPUT`) wyświetlają się normalnie i trzeba je obsłużyć. `KILL` w debugerze przerywa pracę krok po kroku (*UG s. 583*).

Przykład z instrukcji: MYPROGRAM z pętlą `FOR N FROM 1 TO 3 DO MSGBOX(N); END;`. Wykonuj (Step) i obserwuj, jak zmienia się N.

## 22.4. Debugowanie przez PRINT

Najprostsza i często najszybsza metoda to wypisywanie wartości w kluczowych miejscach:

```
PRINT("i="+i+" suma="+s);
```

Po zakończeniu testów usuń te linie albo zamień je na komentarze `//`. Możesz też użyć przełącznika:

```
dbg := 1;                         // zmienna pliku: 1 = debug włączony

DEBUG(t)                          // wywołuj: DEBUG("i="+i);
BEGIN
  IF dbg THEN PRINT(t); END;
END;
```

## 22.5. Obsługa błędów w czasie działania — IFERR

Pełny opis jest w [rozdziale 5.5](05-warunki.md#55-iferr--przechwytywanie-błędów-ug-s-584). Stosuj `IFERR` tam, gdzie błąd może wynikać z danych, na które nie masz wpływu: wpisów użytkownika, `EXPR` na tekście, funkcji numerycznych bez rozwiązania.

```
EXPORT OBLICZ_WZOR()
BEGIN
  LOCAL s := "", w;
  IF NOT INPUT({{s,[2]}}, "Wzór", {"f ="}, {"np. 2*3+SIN(1)"}) THEN RETURN "anulowano"; END;
  IFERR
    w := EXPR(s);
  THEN
    MSGBOX("Niepoprawny wzór: " + s);
    RETURN "błąd";
  END;
  RETURN w;
END;
```

## 22.6. Walidacja danych wejściowych

```
EXPORT PIERW(x)
BEGIN
  IF TYPE(x) <> 0 THEN RETURN "Podaj liczbę rzeczywistą"; END;
  IF x < 0 THEN RETURN "Liczba ujemna!"; END;
  RETURN √x;
END;
```

W `INPUT` walidację robi się w pętli `REPEAT … UNTIL poprawne;` (wzorzec GETSIDES z [rozdziału 6](06-petle.md)) albo przez ograniczenie typu `[0]` w definicji pola.

## 22.7. #pragma (*UG s. 558*)

```
#pragma mode( separator(.,;) integer(h32) )
```

- Wstawia się przez [Shift][Menu] › *Insert pragma* w edytorze.
- Wymusza kompilację programu z określonymi separatorami, np. kropka dziesiętna i średnik jako separator argumentów, oraz z określonym typem liczb całkowitych (np. `h32` to hex, 32 bity).
- Przydaje się, gdy program napisany przy jednych ustawieniach regionalnych (kropka dziesiętna) ma działać na kalkulatorze z innymi (przecinek dziesiętny). **Dodawaj pragmę do programów, które udostępniasz innym.**

## 22.8. Wydajność

1. **Zmienne lokalne** zamiast globalnych w pętlach.
2. **Operacje na całych listach** (`5*lst`, `ΣLIST`, `MAKELIST`) zamiast pętli, gdy to możliwe.
3. **Unikaj wielokrotnego `CONCAT` w długich pętlach.** Jeśli znasz rozmiar, utwórz listę od razu (`MAKELIST(0,X,1,n,1)`) i wpisuj wartości przez indeks.
4. **Grafika:** rysuj w buforze (`G1`) i kopiuj jednym `BLIT_P`. Unikaj tysięcy `PIXON_P`, jeśli można narysować linię lub prostokąt.
5. **Mierz czas**, zamiast zgadywać: `TICKS` i `TEVAL` ([rozdział 17](17-klawiatura-dotyk-czas.md)).
6. Obliczeń symbolicznych CAS w pętlach numerycznych unikaj. Są znacznie wolniejsze.

## 22.9. Styl i dobre praktyki

1. **Wcięcia**: 2 spacje na każdy poziom zagnieżdżenia.
2. **Nazwy**: opisowe; zmienne lokalne małymi literami (*UG s. 611*), funkcje eksportowane WIELKIMI.
3. **Komentarz na początku pliku**: co robi program, parametry, przykład użycia.
4. **Nie zmieniaj ustawień użytkownika na stałe.** Zapamiętaj je (`HAngle`, `HComplex`) i przywróć.
5. **Nie nadpisuj zmiennych użytkownika** (`L1`, `A`, `M1`) bez potrzeby. Zwracaj wyniki przez `RETURN`.
6. **Obsługuj Cancel** w każdym `INPUT` i `CHOOSE`.
7. **Oddziel logikę od interfejsu**: funkcja obliczeniowa i osobna funkcja z dialogami.
8. **Testuj przypadki brzegowe**: 0, liczby ujemne, pusta lista, bardzo duże wartości.
9. **Kopia zapasowa** przed większymi zmianami: (More) › Save w katalogu programów tworzy kopię programu.

## 22.10. Komunikaty błędów (*UG s. 647–648*)

| Komunikat | Znaczenie |
|---|---|
| Bad argument type | zły typ argumentu dla tej operacji |
| Insufficient memory | brak pamięci; usuń aplikacje, macierze, listy, notatki lub programy |
| Insufficient statistics data | za mało danych (Stats 2Var: dwie kolumny, po co najmniej 4 liczby) |
| Invalid dimension | złe wymiary tablicy (np. mnożenie macierzy o niezgodnych wymiarach) |
| Statistics data size not equal | kolumny danych mają różne długości |
| Syntax error | złe argumenty, zła kolejność lub złe separatory (nawiasy, przecinki, średniki) |
| No functions checked | brak zaznaczonej funkcji w Symbolic view przed otwarciem Plot view (w programie: zapomniane `CHECK`) |
| Receive error | błąd odbioru danych z innego kalkulatora |
| Undefined name | zmienna globalna o tej nazwie nie istnieje (literówka albo brak `LOCAL` lub `EXPORT`) |
| Out of memory | krytyczny brak pamięci |
| Two decimal separators input | liczba z dwoma przecinkami dziesiętnymi |
| X/0 | dzielenie przez zero |
| 0/0 | wynik nieoznaczony |
| LN(0) | logarytm z zera |
| Inconsistent units | niezgodne jednostki (np. dodawanie długości do masy) |

## 22.11. Gdy kalkulator się zawiesi (*UG s. 647*)

- Najpierw spróbuj przerwać program klawiszem **[On]**.
- Jeśli kalkulator nie reaguje, zrób **reset**: odwróć kalkulator i włóż spinacz do otworu *Reset* nad pokrywą baterii. Kalkulator uruchomi się ponownie w widoku Home. Reset **nie usuwa** zapisanych danych (zmiennych, aplikacji, programów).
- Jeśli kalkulator się nie włącza: ładuj go co najmniej godzinę, włącz, a w razie potrzeby zrób reset.
- Pełne czyszczenie pamięci usuwa wszystko. Przed nim zrób kopię w Connectivity Kit.

---

## Sprawdź się

W programie poniżej jest 6 błędów. Znajdź je.

```
// Ten program celowo zawiera błędy.
EXPORT SREDNIA(lst)
BEGIN
  LOCAL i, s = 0
  FOR i FROM 1 TO SIZE(lst)
    s := s + lst(i);
  RETURN s/SIZE(lst);
END
```

[Rozwiązania →](25-cwiczenia.md#rozdział-22)
