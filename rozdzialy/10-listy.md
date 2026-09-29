# Rozdział 10. Listy

[← Poprzedni](09-lancuchy.md) · [Spis treści](../README.md) · [Następny →](11-macierze.md)

Lista to uporządkowany zbiór dowolnych obiektów w nawiasach klamrowych: `{1, 2.5, "tekst", {3,4}, [1,2]}`. Listy są w PPL podstawową strukturą danych. Służą jako tablice, stosy, rekordy i tablice wyników. Opis w instrukcji: rozdział 25 (*UG s. 545–559*).

---

## 10.1. Tworzenie i zmienne listowe

```
L1 := {25,147,8};        // globalna lista systemowa (L0–L9)
LOCAL lst := {};         // pusta lista lokalna
lst := {1,2,3};
```

- Zmienne systemowe `L0`–`L9` przechowują wyłącznie listy.
- Listy możesz edytować ręcznie w **katalogu list** (otwierany skrótem List na klawiaturze albo z programu przez `STARTVIEW(-6)`) i w **edytorze list**. Z programu otwiera go `EDITLIST(L1)`.

## 10.2. Odwołania do elementów (*UG s. 552*)

```
L6 := {3,4,5,6};
L6(2)              // 4
L6(2) := 148;      // zapis elementu
```

Odwołania zagnieżdżone i podlisty (przykład z instrukcji). Niech `L1 := {5, "abcde", {1,2,3,4,5}, 11}`:

| Wyrażenie | Wynik | Wyjaśnienie |
|---|---|---|
| `L1(1)` | 5 | pierwszy element |
| `L1(2)` | `"abcde"` | drugi element |
| `L1(2,4)` | 100 | 4. znak tekstu `"abcde"` jako kod ASCII (`d`) |
| `L1(3,2)` | 2 | 2. element podlisty |
| `L1({2,4})` | `{"abcde",{1,2,3,4,5},11}` | podlista od elementu 2 do 4 |

Indeks `0` przy odczycie daje **ostatni** element: `{10,20,30}(0)` → 30 (zmierzone na emulatorze 2.4 przez projekt hp-prime-kit). Instrukcja tego nie opisuje, więc w programach lepiej pisz jawnie `lst(SIZE(lst))`.

### Dopisywanie elementu na końcu

```
lst := CONCAT(lst, {x});         // uniwersalnie
lst(SIZE(lst)+1) := x;           // zapis na pozycji o 1 dalej niż koniec wydłuża listę
```

## 10.3. Arytmetyka na listach (*UG s. 553*)

Operatory działają **element po elemencie**:

```
5*{1,2,3}            // {5,10,15}
{1,2,3}+{10,20,30}   // {11,22,33} (listy muszą mieć tę samą długość)
{1,2,3}^2            // {1,4,9}
SIN({0, π/2})        // {0, 1} w radianach
```

Wiele obliczeń można więc zrobić **bez pętli**, co jest i krótsze, i szybsze.

## 10.4. Funkcje listowe (*UG s. 553–556*)

Są w [Toolbox] (Math) › 6 List. Domyślnie menu pokazuje nazwy opisowe („Concatenate”). Nazwy komend (`CONCAT`) zobaczysz po odznaczeniu opcji *Menu Display* na 2. stronie ustawień Home.

| Funkcja | Działanie | Przykład → wynik |
|---|---|---|
| `SIZE(l)` | liczba elementów (dla macierzy wymiary) | `SIZE({1,2,3})` → 3; `SIZE([[1,2,3],[4,5,6]])` → `{2,3}` |
| `MAKELIST(wyr,zm,od,do,krok)` | tworzy listę z wyrażenia | `MAKELIST(X^2,X,23,27,1)` → `{529,576,625,676,729}` |
| `SORT(l)` | sortuje rosnąco | `SORT({2,5,3})` → `{2,3,5}`. Instrukcja nie opisuje drugiego argumentu, ale emulator 2.4 go przyjmuje: `SORT({"foo","bar","bra"},2)` → `{"bar","foo","bra"}` (według 2. znaku) |
| `REVERSE(l)` | odwraca kolejność | `REVERSE({1,2,3})` → `{3,2,1}` |
| `CONCAT(l1,l2)` | łączy listy | `CONCAT({1,2,3},{4})` → `{1,2,3,4}` |
| `POS(l,el)` | pozycja pierwszego wystąpienia (0 gdy brak) | `POS({3,7,12,19},12)` → 3 |
| `ΔLIST(l)` | różnice kolejnych elementów | `ΔLIST({3,5,8,12,17,23})` → `{2,3,4,5,6}` |
| `ΣLIST(l)` | suma elementów | `ΣLIST({2,3,4})` → 9 |
| `ΠLIST(l)` | iloczyn elementów | `ΠLIST({2,3,4})` → 24 |
| `DIFFERENCE(l1,l2)` | elementy, które nie są wspólne | `DIFFERENCE({1,2,3,4},{1,3,5,7})` → `{2,4,5,7}` |
| `INTERSECT(l1,l2)` | elementy wspólne | `INTERSECT({1,2,3,4},{1,3,5,7})` → `{1,3}` |
| `MAX(l)`, `MIN(l)` | największy / najmniejszy element; dla dwóch list element po elemencie | `MAX({1,8,2},{2,4,6})` → `{2,8,6}` |
| `EVALLIST(l)` | oblicza każdy element listy | |
| `EXECON("wyr",l1[,l2])` | przekształca listę według wyrażenia z `&` | zob. 10.5 |

### MAKELIST dokładniej

```
MAKELIST(wyrażenie, zmienna, początek, koniec, krok)
```

```
MAKELIST(0,X,1,10,1)        // 10 zer
MAKELIST(X,X,1,10,1)        // {1,2,...,10}
MAKELIST(2*X-1,X,1,5,1)     // {1,3,5,7,9}
MAKELIST(RANDINT(1,6),X,1,20,1)   // 20 rzutów kostką
```

## 10.5. EXECON (*UG s. 655–656*)

`EXECON` tworzy nową listę, stosując wyrażenie z symbolami `&` do elementów jednej lub kilku list:

| Wywołanie | Wynik | Znaczenie |
|---|---|---|
| `EXECON("&1+1",{1,2,3})` | `{2,3,4}` | każdy element + 1 |
| `EXECON("&2-&1",{1,4,3,5})` | `{3,-1,2}` | różnica kolejnych par (&1 bieżący, &2 następny) |
| `EXECON("&1+&2",{1,2,3},{4,5,6})` | `{5,7,9}` | przy dwóch listach: &1 z pierwszej, &2 z drugiej |
| `EXECON("&23+&1",{1,5,16},{4,5,6,7})` | `{7,12}` | &23 to 2. lista, od 3. elementu |

Przy jednej liście liczba po `&` to przesunięcie 1–9. Przy wielu listach pierwsza cyfra oznacza numer listy, a druga pozycję.

## 10.6. Statystyka z list (*UG s. 556–559*)

Średnią, medianę, minimum i maksimum najprościej policzysz przez aplikację Statistics 1Var:

1. W Home: `{88,90,89,65,70,89} ▶ L1`, potem `L1 ▶ D1` (kolumna danych D1).
2. Uruchom aplikację Statistics 1Var: w Symbolic view `H1` domyślnie używa `D1`.
3. W Numeric view naciśnij (Stats).

W programie to samo zrobisz poleceniami (zob. [rozdział 18](18-sterowanie-aplikacjami.md)):

```
D1 := L1;
SetSample(H1, D1);
Do1VStats(H1);
RETURN {MeanX, MedVal, MinVal, MaxVal};
```

Możesz też liczyć wprost na listach:

```
srednia := ΣLIST(lst)/SIZE(lst);
```

## 10.7. Typowe wzorce z listami

**Lista jako tablica danych równoległych** (EARTHCOORD z [rozdziału 7](07-wejscie.md)): kilka list o tej samej długości, w których ten sam indeks opisuje ten sam obiekt.

**Lista jako tablica przeglądowa** (TERMVEL): `L0(C)` wybiera stałą zależną od wyboru użytkownika. Nie trzeba wtedy pisać `IF`.

**Lista list jako rekord / tabela:**

```
EXPORT KSIAZKA()
BEGIN
  LOCAL baza := {{"Kowalski",32,"Kraków"},{"Nowak",25,"Gdańsk"}};
  LOCAL i;
  PRINT();
  FOR i FROM 1 TO SIZE(baza) DO
    PRINT(baza(i,1)+", "+baza(i,2)+" lat, "+baza(i,3));
  END;
END;
```

**Licznik częstości** (ROLLMANY z [rozdziału 8](08-funkcje.md)): `wyniki(k) := wyniki(k)+1`.

**Filtrowanie:**

```
EXPORT PARZYSTE(lst)
BEGIN
  LOCAL i, w := {};
  FOR i FROM 1 TO SIZE(lst) DO
    IF lst(i) MOD 2 == 0 THEN
      w := CONCAT(w, {lst(i)});
    END;
  END;
  RETURN w;
END;
```

**Sortowanie własnym algorytmem** (ćwiczenie, bo `SORT` już istnieje): sortowanie bąbelkowe.

```
EXPORT BABELKI(l)
BEGIN
  LOCAL i, j, t, n := SIZE(l);
  FOR i FROM 1 TO n-1 DO
    FOR j FROM 1 TO n-i DO
      IF l(j) > l(j+1) THEN
        t := l(j); l(j) := l(j+1); l(j+1) := t;
      END;
    END;
  END;
  RETURN l;
END;
```

---

## Sprawdź się

1. `USUN_DUPL(l)`: zwróć listę bez powtórzeń, z zachowaniem kolejności. Użyj `POS`.
2. `HISTO(l)`: dla listy ocen 1–6 zwróć listę 6 liczb, czyli ile razy wystąpiła każda ocena.
3. Jednym wyrażeniem, bez pętli, policz sumę kwadratów liczb od 1 do 100.

[Rozwiązania →](25-cwiczenia.md#rozdział-10)
