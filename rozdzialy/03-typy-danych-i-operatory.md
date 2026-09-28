# Rozdział 3. Typy danych i operatory

[← Poprzedni](02-skladnia-i-zmienne.md) · [Spis treści](../README.md) · [Następny →](04-wyjscie.md)

W tym rozdziale:
- jakie typy danych zna HP PPL i jak je zapisywać,
- jak sprawdzić typ funkcją `TYPE`,
- operatory arytmetyczne, porównania i logiczne,
- najczęściej używane funkcje matematyczne.

---

## 3.1. Typy danych

| Typ | Przykład zapisu | `TYPE()` zwraca |
|---|---|---|
| liczba rzeczywista | `3.14`, `-2`, `1.5E-3` | 0 |
| liczba całkowita (binarna) | `#FFh`, `#1101b`, `#17o`, `#99d` | 1 |
| łańcuch znaków (string) | `"Ala ma kota"` | 2 |
| liczba zespolona | `3+4*i`, `(3,4)` | 3 |
| macierz / wektor | `[[1,2],[3,4]]`, `[1,2,3]` | 4 |
| błąd | | 5 |
| lista | `{1, "a", {2,3}}` | 6 |
| funkcja | | 8 |
| liczba z jednostką | `5_m`, `9.81_(m/s^2)` | 9 |
| obiekt CAS | wyrażenie symboliczne | 14.x (część ułamkowa to typ CAS) |

Źródło tabeli kodów: *UG s. 613*.

`TYPE` przydaje się do sprawdzania, co przekazał użytkownik:

```
EXPORT OPISZ(x)
BEGIN
  CASE
    IF TYPE(x)==0 THEN RETURN "liczba rzeczywista"; END;
    IF TYPE(x)==2 THEN RETURN "tekst"; END;
    IF TYPE(x)==6 THEN RETURN "lista o "+SIZE(x)+" elementach"; END;
    DEFAULT RETURN "inny typ: "+TYPE(x);
  END;
END;
```

### Liczby rzeczywiste

Kalkulator liczy z dokładnością do 12 cyfr znaczących. Zapis wykładniczy wpisujesz klawiszem [EEX]: `6.022E23`.

### Liczby zespolone

Jednostkę urojoną `i` wpiszesz z klawiatury (opis nad klawiszem, dostępny przez [Shift]) albo z palety znaków. Aby funkcja rzeczywista mogła zwrócić wynik zespolony (np. `√(-4)`), ustaw `HComplex:=1` (*ES #2, UG s. 634*). Funkcje dla liczb zespolonych: `RE`, `IM`, `ABS` (moduł), `ARG`, `CONJ`.

### Liczby całkowite z `#`

Liczba poprzedzona `#` jest liczbą całkowitą w podanym systemie (`b` dwójkowy, `o` ósemkowy, `d` dziesiętny, `h` szesnastkowy). Służy do operacji bitowych i do zapisu kolorów, np. `#FF0000h`. Szczegóły w [rozdziale 12](12-liczby-calkowite.md).

### Łańcuchy, listy, macierze

Mają osobne rozdziały: [9](09-lancuchy.md), [10](10-listy.md), [11](11-macierze.md).

### Jednostki

Liczby mogą mieć jednostki (menu Units, opisane w rozdziale 24 instrukcji):

```
5_m + 20_cm           // 5.2_m
CONVERT(1_mi, 1_km)   // 1.609344_km
```

Do jednostek służą funkcje `CONVERT`, `MKSA`, `UFACTOR` i `USIMPLIFY` (*UG rozdz. 24*). W programach jednostki przydają się rzadko. Zwykle lepiej liczyć na zwykłych liczbach w ustalonym układzie jednostek.

## 3.2. Operatory arytmetyczne

| Operator | Znaczenie | Przykład |
|---|---|---|
| `+` `-` `*` `/` | działania podstawowe | `7/2` → 3.5 |
| `^` | potęga | `2^10` → 1024 |
| `MOD` | reszta z dzielenia | `17 MOD 5` → 2 |
| `-` jednoargumentowy | zmiana znaku | klawisz [+/-] |
| `√` | pierwiastek kwadratowy | `√(2)`, [Shift][x²] |
| `!` | silnia | `5!` → 120 |

Dzielenie całkowite zapisuje się jako `IP(a/b)` albo `FLOOR(a/b)`. Resztę z dzielenia dają `a MOD b` albo `irem(a,b)`.

Operator `+` z tekstem łączy teksty. Jeśli jeden z argumentów jest tekstem, drugi zostaje zamieniony na tekst:

```
"Wynik: "+42        // "Wynik: 42"
"x="+1/3            // "x=0.333333333333"
```

Operatory z kropką (`.*`, `./`, `.^`) działają na macierzach element po elemencie (zob. [rozdział 11](11-macierze.md)).

## 3.3. Operatory porównania

Porównanie zwraca **1** (prawda) albo **0** (fałsz) (*ES #2*).

| Operator | Znaczenie | Na kalkulatorze |
|---|---|---|
| `==` | równe | `==` |
| `<>` | różne | `≠` |
| `<` `>` | mniejsze / większe | |
| `<=` `>=` | mniejsze lub równe / większe lub równe | `≤` `≥` |

> **Uwaga.** Do porównania używaj `==`, nie `=`. Pojedynczy znak `=` służy do zapisu równań (np. w aplikacji Solve).

Wszystkie te symbole są w menu **[Shift] [6]**.

## 3.4. Operatory logiczne

| Operator | Znaczenie |
|---|---|
| `AND` | i |
| `OR` | lub |
| `XOR` | albo (alternatywa wykluczająca) |
| `NOT` | zaprzeczenie |

Przykład z *ES #2*: czy X jest w przedziale [1000, 1999]?

```
(X >= 1000) AND (X <= 1999)
```

Ponieważ fałsz to 0, a prawda to każda liczba różna od zera, wynik porównania można wstawić do wzoru. Tę sztuczkę wykorzystuje program DOESTAX2 z [rozdziału 7](07-wejscie.md):

```
cena*(1 + podatek*stawka)   // podatek = 0 lub 1
```

## 3.5. Kolejność działań

Od najwyższego priorytetu:
1. nawiasy `( )`,
2. funkcje (`SIN`, `√`, …), silnia `!`,
3. potęga `^` (wiązanie prawostronne: `2^3^2` = 2^9),
4. minus jednoargumentowy,
5. `*`, `/`, `MOD`,
6. `+`, `-`,
7. porównania,
8. `NOT`, potem `AND`, na końcu `OR` i `XOR`.

Kolejność operatorów logicznych i porównań łatwo pomylić, dlatego **w warunkach zawsze stawiaj nawiasy**.

Pułapka: `-2^2` daje `-4`, bo potęga ma wyższy priorytet niż minus. Jeśli chodzi Ci o kwadrat liczby -2, napisz `(-2)^2`.

## 3.6. Najważniejsze funkcje matematyczne w programach

Wszystkie są w [Toolbox] (Math) (*UG rozdz. 22*):

| Funkcja | Działanie | Przykład |
|---|---|---|
| `ABS(x)` | wartość bezwzględna | `ABS(-3)` → 3 |
| `IP(x)` | część całkowita | `IP(-3.7)` → -3 |
| `FP(x)` | część ułamkowa | `FP(3.7)` → 0.7 |
| `FLOOR(x)` | podłoga | `FLOOR(-3.7)` → -4 |
| `CEILING(x)` | sufit | `CEILING(3.2)` → 4 |
| `ROUND(x,n)` | zaokrąglenie do n miejsc | `ROUND(3.14159,2)` → 3.14 |
| `TRUNCATE(x,n)` | obcięcie do n miejsc | |
| `MAX(a,b)`, `MIN(a,b)` | maksimum, minimum | działa też na listach |
| `a MOD b` | reszta | |
| `irem(a,b)` | reszta z dzielenia całkowitego | `irem(17,5)` → 2 |
| `COMB(n,k)`, `PERM(n,k)` | kombinacje, permutacje | |
| `n!` | silnia | |
| `SIN COS TAN ASIN ACOS ATAN` | trygonometria (zależy od `HAngle`) | |
| `LN LOG EXP ALOG` | logarytmy i potęgi | `ALOG(2)` → 100 |
| `RANDOM` | losowa liczba z przedziału [0,1) | |
| `RANDINT(a,b)` | losowa liczba całkowita z [a,b] | `RANDINT(1,6)` |
| `RANDINT(n,a,b)` | lista n losowych liczb całkowitych | `RANDINT(3,0,39)` |
| `RANDSEED(v)` | ustawia ziarno generatora | powtarzalne losowania |
| `%CHANGE(x,y)` | zmiana procentowa z x na y | `%CHANGE(20,50)` → 150 |
| `%TOTAL(x,y)` | jakim procentem x jest y | `%TOTAL(20,50)` → 250 |
| `HMS→(v)` / `→HMS(v)` | zamiana między zapisem dziesiętnym a sześćdziesiątkowym | stopnie/minuty/sekundy |

Test parzystości użyty w *ES #2–3*: `FP(N/2)==0` jest prawdą dla parzystego N. To samo daje `N MOD 2 == 0`.

---

## Sprawdź się

1. Jaki wynik dadzą wyrażenia: `7 MOD 3`, `IP(-7/2)`, `FLOOR(-7/2)`, `"A"+1+2`, `1+2+"A"`?
2. Napisz funkcję `PRZESTEPNY(r)`, która zwraca 1, jeśli rok jest przestępny (podzielny przez 4 i niepodzielny przez 100, albo podzielny przez 400), a 0 w przeciwnym razie. Użyj jednego wyrażenia logicznego.
3. Napisz funkcję `CYFRY(n)`, która zwraca sumę cyfr liczby naturalnej n (podpowiedź: `MOD 10` i `IP(n/10)`).

[Rozwiązania →](25-cwiczenia.md#rozdział-3)
