# Rozdział 11. Macierze i wektory

[← Poprzedni](10-listy.md) · [Spis treści](../README.md) · [Następny →](12-liczby-calkowite.md)

Opis w instrukcji: rozdział 26 (*UG s. 516–543*) oraz komendy macierzowe PPL (*UG s. 601–603*).

---

## 11.1. Podstawy

- **Wektor**: tablica jednowymiarowa w pojedynczych nawiasach kwadratowych: `[1,2,3]`.
- **Macierz**: tablica dwuwymiarowa: `[[1,2,3],[4,5,6]]` (2 wiersze, 3 kolumny).
- Elementy mogą być rzeczywiste lub zespolone.
- Zmienne systemowe `M0`–`M9` przechowują macierze. W zmiennej lokalnej też możesz trzymać macierz.
- Ręczna edycja: **katalog macierzy** (otwierany skrótem Matrix na klawiaturze albo z programu przez `STARTVIEW(-5)`) i **edytor macierzy**. Z programu otwiera go `EDITMAT`.
- Szablon macierzy jest też pod klawiszem [Template].

## 11.2. Odwołania (*UG rozdz. 26*)

```
M1 := [[1,2,3],[4,5,6]];
M1(2,3)            // 6: wiersz 2, kolumna 3
M1(2,3) := 0;      // zapis elementu
M1(1)              // [1,2,3]: cały wiersz jako wektor
V := [10,20,30];
V(2)               // 20
SIZE(M1)           // {2,3}
DIM(M1)            // {2,3}
```

## 11.3. Arytmetyka macierzowa

| Operacja | Zapis |
|---|---|
| mnożenie przez skalar | `3*M1` |
| suma, różnica | `M1+M2`, `M1-M2` (te same wymiary) |
| iloczyn macierzy | `M1*M2` (kolumny M1 = wiersze M2) |
| potęga | `M1^3` (macierz kwadratowa) |
| odwrotność | `M1^-1` (klawisz x⁻¹) |
| dzielenie przez macierz kwadratową | `M1/M2` = `M1*M2^-1` |
| zmiana znaku | `-M1` |
| działania element po elemencie | `M1 .* M2`, `M1 ./ M2`, `M1 .^ 2` |

### Rozwiązywanie układów równań

Układ A·x = b rozwiązujesz jako `x := A^-1*b`, przy czym b to wektor kolumnowy:

```
EXPORT UKLAD3()
BEGIN
  LOCAL A := [[2,1,-1],[-3,-1,2],[-2,1,2]];
  LOCAL b := [[8],[-11],[-3]];
  RETURN A^-1*b;      // [[2],[3],[-1]]
END;
```

Inne sposoby: `RREF` na macierzy rozszerzonej, `LSQ(A,b)` (metoda najmniejszych kwadratów), funkcje aplikacji Linear Solver (`LinSolve`, `Solve2x2`, `Solve3x3`).

## 11.4. Komendy programowe (Cmds › Matrix) (*UG s. 601–603*)

Te komendy modyfikują macierz **zapisaną w zmiennej** (`M0`–`M9` albo lokalnej):

| Komenda | Działanie |
|---|---|
| `ADDCOL(nazwa, wektor, nr)` | wstawia kolumnę przed kolumną nr |
| `ADDROW(nazwa, wektor, nr)` | wstawia wiersz przed wierszem nr |
| `DELCOL(nazwa, nr)` | usuwa kolumnę |
| `DELROW(nazwa, nr)` | usuwa wiersz |
| `SWAPCOL(nazwa, k1, k2)` | zamienia kolumny |
| `SWAPROW(nazwa, w1, w2)` | zamienia wiersze |
| `REDIM(nazwa, {w,k})` | zmienia wymiary; zachowuje dane, uzupełnia zerami |
| `REPLACE(nazwa, {w,k}, obiekt)` | wstawia podmacierz od pozycji `{w,k}` |
| `SCALE(nazwa, wartość, wiersz)` | mnoży wiersz przez wartość |
| `SCALEADD(nazwa, wartość, w1, w2)` | operacja wierszowa: w1·wartość dodane do w2 (opis w instrukcji: *UG s. 602*) |
| `SUB(nazwa, początek, koniec)` | wycina fragment listy, macierzy lub grafiki |
| `EDITMAT(nazwa [, tytuł] [, tylko_odczyt])` | edytor macierzy |

Przykład: tabela pomiarów rozbudowywana wiersz po wierszu.

```
EXPORT POMIARY()
BEGIN
  LOCAL m := [[0,0]], t, v, n := 0;
  REPEAT
    IF NOT INPUT({t,v},"Pomiar "+(n+1),{"t=","v="}) THEN BREAK; END;
    n := n+1;
    IF n==1 THEN
      m := [[t,v]];
    ELSE
      ADDROW(m, [t,v], n);     // wstaw nowy wiersz na końcu
    END;
  UNTIL 0;
  EDITMAT(m, {"Pomiary", {}, {"t","v"}}, 1);   // podgląd tylko do odczytu
  RETURN m;
END;
```

## 11.5. Funkcje macierzowe (Math › Matrix) (*UG s. 528–540*)

| Kategoria | Funkcje |
|---|---|
| **Podstawowe** | `TRN` (transpozycja), `DET` (wyznacznik), `RREF` (postać schodkowa zredukowana) |
| **Tworzenie** | `MAKEMAT(wyr, w, k)` (wyrażenie z I, J), `IDENMAT(n)`, `randMat(n,m)`, `jordan`, `hilbert`, `isom`, `vandermonde` |
| **Normy** | `ABS` (norma Frobeniusa), `ROWNORM`, `COLNORM`, `SPECNORM`, `SPECRAD`, `COND`, `RANK`, `pivot`, `TRACE` |
| **Zaawansowane** | `EIGENVAL`, `EIGENVV`, `jordan`, `diag`, `cholesky`, `hermite`, `hessenberg`, `smith` |
| **Rozkłady** | `LQ`, `LSQ`, `LU`, `QR`, `SCHUR`, `SVD`, `SVL` |
| **Wektory** | `CROSS` (iloczyn wektorowy), `DOT` (skalarny), `l2norm`, `l1norm`, `maxnorm` |

`MAKEMAT` w zapisie z indeksami `I` (wiersz) i `J` (kolumna):

```
MAKEMAT(I+J, 3, 3)     // [[2,3,4],[3,4,5],[4,5,6]]
MAKEMAT(0, 2, 4)       // macierz zerowa 2x4
MAKEMAT(I==J, 3, 3)    // macierz jednostkowa
```

## 11.6. Pętle po macierzy

```
EXPORT SUMA_PRZEKATNEJ(m)
BEGIN
  LOCAL i, s := 0, d := SIZE(m);
  FOR i FROM 1 TO MIN(d(1), d(2)) DO
    s := s + m(i,i);
  END;
  RETURN s;           // to samo co TRACE(m)
END;
```

```
EXPORT TABLICZKA_M(n)
BEGIN
  LOCAL m := MAKEMAT(0,n,n), i, j;
  FOR i FROM 1 TO n DO
    FOR j FROM 1 TO n DO
      m(i,j) := i*j;
    END;
  END;
  EDITMAT(m, "Tabliczka mnożenia", 1);
  RETURN m;
END;
```

## 11.7. Macierze a listy

| | Lista `{}` | Macierz `[]` |
|---|---|---|
| elementy | dowolne typy, także teksty i listy | tylko liczby (rzeczywiste lub zespolone) |
| wymiar | 1, z możliwością zagnieżdżania | 1 (wektor) lub 2 |
| `*` | element po elemencie | iloczyn macierzowy |
| typowe użycie | dane, rekordy, wyniki | algebra liniowa, tabele liczb, współrzędne |

Konwersje: `list2mat(lista, kolumny)` i `mat2list(m)` (funkcje CAS dostępne w katalogu).

---

## Sprawdź się

1. `CRAMER2(a,b,c,d,e,f)`: rozwiąż układ ax+by=e, cx+dy=f wzorami Cramera przez `DET`. Sprawdź wynik przez `A^-1*b`.
2. `OBROT(p, kat)`: obróć punkt p = [x,y] o kąt `kat` (w stopniach) macierzą obrotu.
3. `ZERUJ_UJEMNE(m)`: zamień wszystkie ujemne elementy macierzy na 0.

[Rozwiązania →](25-cwiczenia.md#rozdział-11)
