# Rozdział 8. Funkcje, podprogramy i rekurencja

[← Poprzedni](07-wejscie.md) · [Spis treści](../README.md) · [Następny →](09-lancuchy.md)

W tym rozdziale:
- funkcje eksportowane i prywatne,
- deklaracje zapowiadające (dlaczego podprogram trzeba „zapowiedzieć”),
- przekazywanie parametrów i zwracanie wielu wyników,
- wywoływanie funkcji z innych programów,
- rekurencja.

---

## 8.1. Funkcja eksportowana a prywatna (*UG s. 569–571, 588*)

| | Z `EXPORT` | Bez `EXPORT` |
|---|---|---|
| widoczna w Home | tak | nie |
| widoczna w [Toolbox] (User) | tak | nie |
| widoczna w innych programach | tak | nie |
| widoczna w swoim pliku | tak | tak (po deklaracji) |

Funkcja **bez** `EXPORT` to podprogram „prywatny” pliku. Nie zaśmieca menu User i nie koliduje z nazwami w innych programach.

## 8.2. Struktura z podprogramem (*ES #6*)

```
SUB1();                  // 1. deklaracja zapowiadająca

EXPORT MAIN()            // 2. program główny
BEGIN
  ... SUB1() ...
END;

SUB1()                   // 3. definicja podprogramu
BEGIN
  ...
END;
```

**Dlaczego deklaracja?** Kompilator czyta plik od góry do dołu. Gdy w `MAIN` natrafi na `SUB1`, musi już wiedzieć, że to funkcja. Deklaracja `SUB1();` na początku pliku to informacja, że funkcja zostanie zdefiniowana niżej (*UG s. 570*). Zamiast deklaracji możesz umieścić definicję podprogramu **nad** funkcją, która go wywołuje.

> W deklaracji zapowiadającej nie trzeba podawać parametrów: `SUB1();` wystarczy, nawet jeśli `SUB1` ma dwa parametry.

Program SUBEXAM (*ES #6*) oblicza wartości:

A = 2(y−x)/φ + xy, B = φ², gdzie φ = 2e^(x+y) − e^(x−y) − e^(y−x),

i zwraca większą z nich:

```
SUB1();

EXPORT SUBEXAM(X,Y)
BEGIN
  LOCAL A, B;
  A := (2*(Y-X))/SUB1(X,Y) + X*Y;
  B := (SUB1(X,Y))^2;
  IF A>B THEN
    RETURN A;
  ELSE
    RETURN B;
  END;
END;

SUB1(X,Y)
BEGIN
  RETURN 2*e^(X+Y) - e^(X-Y) - e^(Y-X);
END;
```

| Wywołanie | Wynik |
|---|---|
| `SUBEXAM(-4,1)` | 21998.918189 |
| `SUBEXAM(2,3)` | 86283.2797974 |
| `SUBEXAM(-5,-6)` | 30.648061288 |
| `SUBEXAM(2,-3)` | 21810.6046664 |

## 8.3. Seria programów z instrukcji: ROLLDIE i ROLLMANY (*UG s. 569–571*)

### Krok 1: osobny program ROLLDIE

```
EXPORT ROLLDIE(N)
BEGIN
  RETURN 1+RANDINT(N-1);    // RANDINT(b) daje liczbę 0..b, więc wynik to 1..N
END;
```

`ROLLDIE(6)` symuluje rzut kostką sześcienną i działa **wszędzie**, gdzie można użyć liczby: w Home, w aplikacjach i w innych programach.

### Krok 2: ROLLMANY korzysta z eksportowanego ROLLDIE

```
EXPORT ROLLMANY(n,sides)
BEGIN
  LOCAL k,roll;
  // lista częstości: L2(s) = ile razy wypadła suma s
  L2 := MAKELIST(0,X,1,2*sides,1);
  FOR k FROM 1 TO n DO
    roll := ROLLDIE(sides)+ROLLDIE(sides);
    L2(roll) := L2(roll)+1;
  END;
END;
```

### Krok 3: ROLLDIE jako podprogram prywatny

```
ROLLDIE();

EXPORT ROLLMANY(n,sides)
BEGIN
  LOCAL k,roll;
  L2 := MAKELIST(0,X,1,2*sides,1);
  FOR k FROM 1 TO n DO
    roll := ROLLDIE(sides)+ROLLDIE(sides);
    L2(roll) := L2(roll)+1;
  END;
END;

ROLLDIE(n)
BEGIN
  RETURN 1+RANDINT(n-1);
END;
```

### Krok 4: zwracanie wyniku zamiast zapisu do L2

Lepsza praktyka: funkcja nie powinna po cichu zmieniać globalnych danych użytkownika, takich jak `L2`. Niech **zwraca** wynik, a wywołujący sam zdecyduje, gdzie go zapisać:

```
ROLLDIE();

EXPORT ROLLMANY(n,sides)
BEGIN
  LOCAL k,roll,results;
  results := MAKELIST(0,X,1,2*sides,1);
  FOR k FROM 1 TO n DO
    roll := ROLLDIE(sides)+ROLLDIE(sides);
    results(roll) := results(roll)+1;
  END;
  RETURN results;
END;

ROLLDIE(N)
BEGIN
  RETURN 1+RANDINT(N-1);
END;
```

W Home: `ROLLMANY(100,6) ▶ L5` zapisze wyniki 100 rzutów dwiema kostkami w `L5`.

## 8.4. Parametry — jak są przekazywane

- Parametry przekazywane są **przez wartość**. Zmiana parametru wewnątrz funkcji nie zmienia zmiennej, którą przekazał wywołujący.
- Parametry są zmiennymi lokalnymi, więc można je modyfikować (np. `A := CEILING(A);` w PRINTEVENS).
- Liczba argumentów w wywołaniu musi zgadzać się z liczbą parametrów.
- Funkcja bez parametrów: `EXPORT F()`. Wywołanie w programie `F()` albo samo `F`.

Jak „zwrócić” kilka wartości? Zwróć listę i rozpakuj ją:

```
MINMAX();

EXPORT TEST_MINMAX()
BEGIN
  LOCAL w := MINMAX({4,9,1,7});
  RETURN "min="+w(1)+" max="+w(2);
END;

MINMAX(lst)
BEGIN
  RETURN {MIN(lst), MAX(lst)};
END;
```

## 8.5. Wywoływanie funkcji z innego programu

Każdą funkcję z `EXPORT` wywołuje się po nazwie, jak funkcję wbudowaną (tak `PERFECTNUMS` wywołuje `ISPERFECT` w [rozdziale 6](06-petle.md)). Jeśli nazwa jest niejednoznaczna, kwalifikuj ją nazwą programu: `NAZWAPROGRAMU.NAZWAFUNKCJI(...)`.

Dzięki temu możesz zbudować własną **bibliotekę**: jeden plik, np. `MYLIB`, z wieloma funkcjami eksportowanymi, używanymi przez inne programy.

## 8.6. Rekurencja

Funkcja może wywoływać samą siebie. Każde wywołanie ma własne zmienne lokalne.

```
EXPORT SILNIAR(n)
BEGIN
  IF n<=1 THEN
    RETURN 1;
  END;
  RETURN n*SILNIAR(n-1);
END;
```

```
HANOI();

EXPORT WIEZA(n)
BEGIN
  PRINT();
  HANOI(n,"A","C","B");
END;

// przenieś n krążków z palika z na palik na, używając palika przez
HANOI(n,z,na,przez)
BEGIN
  IF n>0 THEN
    HANOI(n-1,z,przez,na);
    PRINT("krążek "+n+": "+z+" -> "+na);
    HANOI(n-1,przez,na,z);
  END;
END;
```

> Rekurencja zużywa pamięć na każde wywołanie. Przy dużej głębokości (tysiące poziomów) program może skończyć się błędem. Tam, gdzie łatwo zapisać pętlę, pętla jest szybsza i bezpieczniejsza.

## 8.7. Funkcje jednowierszowe w Home

Nie każda funkcja wymaga programu. W Home możesz zdefiniować funkcję użytkownika przez przypisanie z parametrem:

```
f(x) := x^2+1      // w Home (lub w CAS); pojawi się w [Vars] (User)
```

Program jest lepszy, gdy potrzebujesz pętli, warunków, dialogów lub wielu kroków.

## 8.8. Dobre praktyki projektowania funkcji

1. **Jedna funkcja robi jedną rzecz.** `ROLLDIE` tylko rzuca kostką, a `ROLLMANY` tylko zlicza wyniki.
2. **Wejście przez parametry, wyjście przez RETURN.** Nie zmieniaj globalnych zmiennych (`L2`, `A`), jeśli nie musisz.
3. **Funkcje interfejsu oddzielone od obliczeń.** Funkcja z `INPUT` i `MSGBOX` wywołuje czystą funkcję obliczeniową, którą łatwo testować w Home.
4. **Opisowe nazwy** funkcji i parametrów.
5. **Eksportuj tylko to, co ma być dostępne z zewnątrz.**

---

## Sprawdź się

1. Przebuduj program `BMI` z rozdziału 7 na dwie funkcje: prywatną `OBLICZBMI(m,h)` i eksportowaną `BMI()` z dialogami.
2. Napisz rekurencyjną funkcję `POTEGA(a,n)` (a^n dla całkowitego n ≥ 0) korzystającą z zasady: a^n = (a^(n/2))² dla parzystych n.
3. Napisz bibliotekę `STATLIB` z funkcjami `SREDNIA(l)`, `MEDIANA(l)` i `ODCH(l)` (odchylenie standardowe populacji), a potem program, który ich używa.

[Rozwiązania →](25-cwiczenia.md#rozdział-8)
