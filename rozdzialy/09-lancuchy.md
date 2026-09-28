# Rozdział 9. Łańcuchy znaków (stringi)

[← Poprzedni](08-funkcje.md) · [Spis treści](../README.md) · [Następny →](10-listy.md)

Komendy tekstowe są w **(Cmds) › Strings** (*UG s. 589–592*).

---

## 9.1. Zapis i znaki specjalne

- Tekst zapisuje się w cudzysłowach: `"HP Prime"`.
- Cudzysłów wewnątrz tekstu zapisuje się podwójnie: `"Mówi ""hej"""`.
- `\n` to nowa linia, a `\\` to jeden ukośnik wsteczny (*UG s. 589*).
- Znaki numeruje się **od 1**: pierwszy znak ma indeks 1.
- Pojedynczy znak odczytasz jak element listy: `s(1)` zwraca **kod** znaku. Zob. też `MID`.

## 9.2. Łączenie i konwersja

```
"Ala"+" ma "+"kota"      // "Ala ma kota"
"x="+5                   // "x=5"
STRING(1/3)              // "0.333333333333"
EXPR("2+3")              // 5, oblicza wyrażenie zapisane w tekście
```

`EXPR` zamienia tekst na wynik obliczenia. Przydaje się, gdy użytkownik wpisuje wzór jako tekst, na przykład w polu `INPUT` z typem `[2]`.

## 9.3. Komendy z instrukcji

| Komenda | Działanie | Przykład → wynik |
|---|---|---|
| `DIM(s)` | liczba znaków | `DIM("12345")` → 5; `DIM("""")` → 1; `DIM("\n")` → 1 |
| `LEFT(s,n)` | pierwsze n znaków | `LEFT("MOMOGUMBO",3)` → `"MOM"` |
| `RIGHT(s,n)` | ostatnie n znaków | `RIGHT("MOMOGUMBO",5)` → `"GUMBO"` |
| `MID(s,poz[,n])` | n znaków od pozycji poz (bez n: do końca) | `MID("MOMOGUMBO",3,5)` → `"MOGUM"`; `MID("PUDGE",4)` → `"GE"` |
| `INSTRING(s1,s2)` | pozycja pierwszego wystąpienia s2 w s1, 0 gdy brak | `INSTRING("banana","na")` → 3; `INSTRING("ab","abc")` → 0 |
| `UPPER(s)` / `LOWER(s)` | wielkie / małe litery | `UPPER("abc")` → `"ABC"` |
| `ASC(s)` | wektor kodów znaków | `ASC("AB")` → `[65,66]` |
| `CHAR(k)` / `CHAR(wektor)` | znak(i) o danym kodzie | `CHAR(65)` → `"A"`; `CHAR([82,77,72])` → `"RMH"` |
| `ROTATE(s,n)` | przesunięcie cykliczne (n>0 w lewo, n<0 w prawo) | `ROTATE("12345",2)` → `"34512"`; `ROTATE("12345",-1)` → `"51234"` |
| `REPLACE(s,start,s2)` | nadpisuje fragment od pozycji start | `REPLACE("123456",2,"GRM")` → `"1GRM56"` |
| `STRINGFROMID(n)` | wbudowany napis systemowy o numerze n | `STRINGFROMID(56)` → `"Complex"` |
| `STRING(wyr, ...)` | wynik jako tekst z formatowaniem | zob. 9.4 |

Szczegóły zachowania na krańcach (*UG s. 591–592*):
- `LEFT(s,n)`: gdy n ≥ DIM(s) lub n < 0, zwraca cały tekst. Opis przypadku n = 0 jest w instrukcji niejasny, więc go unikaj.
- `RIGHT(s,n)`: gdy n ≤ 0, zwraca tekst pusty; gdy n > DIM(s), zwraca cały tekst.
- `ROTATE`: gdy |n| > DIM(s), zwraca tekst bez zmian.

> Instrukcja podaje też przykład `REPLACE("12345","3","99")` → `"12995"`. Poprawnym i pewnym zapisem pozycji startowej jest liczba, jak w przykładzie z tabeli.

## 9.4. STRING — formatowanie liczb (*UG s. 590–591*)

```
STRING(wyrażenie, [tryb], [precyzja], [separator], [limit_rozmiaru])
```

**Tryb:**

| Kod | Format |
|---|---|
| 0 | bieżące ustawienie |
| 1 | Standard |
| 2 | Fixed (stała liczba miejsc po przecinku) |
| 3 | Scientific |
| 4 | Engineering |
| 5 | Floating |
| 6 | Rounded |
| +7 | ułamek właściwy (np. 2+7 = 9) |
| +14 | ułamek mieszany |

**Precyzja:** -1 oznacza bieżące ustawienie, poza tym 0–12.

**Separator:** tekst z cyframi i separatorami albo liczba (-1 domyślny, 0–10 jeden z 11 wbudowanych separatorów). Może to być też lista `{separator, "[przecinek[wykładnik[minus]]]", [DotZero]}`. `DotZero ≠ 0` wyświetla `.1` zamiast `0.1`.

**Limit rozmiaru:** szerokość w pikselach, w której ma się zmieścić wynik. Opcjonalnie lista `{limit, rozmiar_czcionki, pogrubienie, kursywa, stała_szerokość}`.

Przykłady:

```
STRING(2/3)           // "0.666666666667"
STRING(2/3, 2, 2)     // "0.67"   (Fixed, 2 miejsca)
STRING(123456, 3, 2)  // "1.23E5" (Scientific)
STRING(0.75, 9)       // "3/4"    (Standard + ułamek, zależnie od firmware)
STRING({1,2,3})       // "{1,2,3}"
STRING(M1)            // np. "[[1,2,3],[4,5,6]]"
```

## 9.5. Przetwarzanie tekstu znak po znaku

Wzorzec pętli po tekście:

```
EXPORT ODWROC(s)
BEGIN
  LOCAL i, w := "";
  FOR i FROM DIM(s) DOWNTO 1 DO
    w := w + MID(s,i,1);
  END;
  RETURN w;
END;
```

`ODWROC("kajak")` → `"kajak"`, `ODWROC("Prime")` → `"emirP"`.

Liczenie samogłosek:

```
EXPORT SAMOGLOSKI(s)
BEGIN
  LOCAL i, c, n := 0;
  s := LOWER(s);
  FOR i FROM 1 TO DIM(s) DO
    c := MID(s,i,1);
    IF INSTRING("aeiouy", c) > 0 THEN
      n := n+1;
    END;
  END;
  RETURN n;
END;
```

Szyfr Cezara na kodach ASCII:

```
EXPORT CEZAR(s,k)
BEGIN
  LOCAL i, c, w := "";
  s := UPPER(s);
  FOR i FROM 1 TO DIM(s) DO
    c := ASC(MID(s,i,1));        // wektor jednoelementowy
    c := c(1);
    IF c>=65 AND c<=90 THEN      // A..Z
      c := 65 + ((c-65+k) MOD 26);
    END;
    w := w + CHAR(c);
  END;
  RETURN w;
END;
```

`CEZAR("HP PRIME",3)` → `"KS SULPH"`. Odszyfrowanie: `CEZAR(s,-3)`.

## 9.6. Dzielenie tekstu na części

```
EXPORT PODZIEL(s, sep)
BEGIN
  LOCAL wynik := {}, p;
  p := INSTRING(s, sep);
  WHILE p > 0 DO
    wynik := CONCAT(wynik, {when(p>1, LEFT(s, p-1), "")});
    s := MID(s, p+DIM(sep));
    p := INSTRING(s, sep);
  END;
  RETURN CONCAT(wynik, {s});
END;
```

`PODZIEL("1,22,333", ",")` → `{"1","22","333"}`. Dodaj `EXPR` na każdym elemencie, żeby zamienić teksty na liczby.

---

## Sprawdź się

1. `PALINDROM(s)`: zwróć 1, gdy tekst jest palindromem. Ignoruj wielkość liter i spacje.
2. `ZAMIEN(s, a, b)`: zamień **wszystkie** wystąpienia tekstu a na b.
3. `BIN(n)`: zamień liczbę naturalną na tekst w systemie dwójkowym bez użycia `#`, dzieląc przez 2.

[Rozwiązania →](25-cwiczenia.md#rozdział-9)
