# Rozdział 6. Pętle: FOR, WHILE, REPEAT, BREAK, CONTINUE

[← Poprzedni](05-warunki.md) · [Spis treści](../README.md) · [Następny →](07-wejscie.md)

Pętle znajdziesz w edytorze pod **(Tmplt) › Loop**: FOR, FOR STEP, FOR DOWN, FOR STEP DOWN, WHILE, REPEAT, BREAK, CONTINUE.

| Pętla | Kiedy używać | Sprawdzanie warunku |
|---|---|---|
| `FOR` | znana liczba powtórzeń | przed każdym obiegiem |
| `WHILE` | powtarzaj, **dopóki** warunek jest prawdziwy | przed obiegiem, więc pętla może nie wykonać się ani razu |
| `REPEAT` | powtarzaj, **aż** warunek stanie się prawdziwy | po obiegu, więc pętla wykona się co najmniej raz |

---

## 6.1. FOR (*UG s. 628, ES #2*)

```
FOR zmienna FROM start TO koniec DO
  polecenia;
END;
```

Zmienna przyjmuje wartość `start`. Dopóki jest ≤ `koniec`, wykonują się polecenia, a zmienna rośnie o 1.

Program DISPCUBES (*ES #2*) wypisuje sześciany liczb od 1 do N:

```
EXPORT DISPCUBES(N)
BEGIN
  LOCAL I;
  PRINT();                // czyść terminal
  FOR I FROM 1 TO N DO
    PRINT(I^3);
  END;
END;
```

`DISPCUBES(5)` wypisze: 1, 8, 27, 64, 125.

Program SUMDIV (*ES #2*) oblicza sumę dzielników liczby N:

```
EXPORT SUMDIV(N)
BEGIN
  LOCAL S:=0, K, mdiv, ldiv;
  mdiv := CAS.idivis(N);      // ciąg wszystkich dzielników (funkcja CAS)
  ldiv := DIM(mdiv);          // {długość}
  FOR K FROM 1 TO ldiv(1) DO
    S := S + mdiv(K);
  END;
  RETURN S;
END;
```

`SUMDIV(12)` → 28, `SUMDIV(24)` → 60, `SUMDIV(85)` → 108.

Nowe elementy w tym programie:
- `CAS.idivis(n)`: funkcja CAS wywołana z programu. Funkcje CAS poprzedza się prefiksem `CAS.` (*ES #2*).
- `DIM`: dla ciągu lub wektora zwraca listę `{długość}`, dla tekstu liczbę znaków, dla macierzy `{wiersze, kolumny}`. Długość **listy** podaje `SIZE`.

Program MAXFACTORS (*UG s. 629*) szuka liczby z przedziału 2…N o największej liczbie dzielników:

```
EXPORT MAXFACTORS(N)
BEGIN
  LOCAL cur, max, k, result;
  max := 1; result := 1;
  FOR k FROM 2 TO N DO
    cur := SIZE(CAS.idivis(k));
    IF cur(1) > max THEN
      max := cur(1);
      result := k;
    END;
  END;
  MSGBOX("Max of "+max+" factors for "+result);
END;
```

Uruchom `MAXFACTORS(100)`.

## 6.2. FOR ze STEP (*UG s. 629, ES #2*)

```
FOR zmienna FROM start TO koniec STEP krok DO
  polecenia;
END;
```

Program PRINTEVENS (*ES #2*) wypisuje liczby parzyste z przedziału [A, B]:

```
EXPORT PRINTEVENS(A,B)
BEGIN
  LOCAL I;
  PRINT();
  A := CEILING(A);                       // do najbliższej liczby całkowitej w górę
  A := when(FP(A/2)==0, A, A+1);         // do najbliższej parzystej
  FOR I FROM A TO B STEP 2 DO
    PRINT(I);
  END;
END;
```

`PRINTEVENS(3,10)` wypisze: 4, 6, 8, 10.

Krok nie musi być liczbą całkowitą. Program DRAWPATTERN (*UG s. 630*) przechodzi po wszystkich pikselach ekranu we współrzędnych kartezjańskich:

```
EXPORT DRAWPATTERN()
BEGIN
  LOCAL xincr, yincr, color;
  STARTAPP("Function");
  RECT();
  xincr := (Xmax - Xmin)/318;
  yincr := (Ymax - Ymin)/218;
  FOR X FROM Xmin TO Xmax STEP xincr DO
    FOR Y FROM Ymin TO Ymax STEP yincr DO
      color := RGB(X^3 MOD 255, Y^3 MOD 255, TAN(0.1*(X^3+Y^3)) MOD 255);
      PIXON(X, Y, color);
    END;
  END;
  WAIT;
END;
```

To także przykład **pętli zagnieżdżonych**: wewnętrzna pętla wykonuje się w całości przy każdym obiegu zewnętrznej.

## 6.3. FOR DOWNTO (*UG s. 630*)

Pętla liczy w dół:

```
FOR zmienna FROM start DOWNTO koniec DO
  polecenia;
END;

FOR zmienna FROM start DOWNTO koniec STEP krok DO   // krok dodatni, odejmowany
  polecenia;
END;
```

```
EXPORT ODLICZ()
BEGIN
  LOCAL i;
  FOR i FROM 10 DOWNTO 1 DO
    RECT_P(140,90,200,130);            // wyczyść pole (biały prostokąt)
    TEXTOUT_P(i, 150, 100, 7);         // czcionka 7 = 22 pt (firmware 13441+)
    WAIT(1);                           // pauza 1 s
  END;
  MSGBOX("Start!");
END;
```

## 6.4. WHILE (*UG s. 631, ES #3*)

```
WHILE warunek DO
  polecenia;
END;
```

Program ISPERFECT (*UG s. 631*) sprawdza, czy n jest liczbą doskonałą, czyli równą sumie swoich dzielników właściwych (np. 6 = 1+2+3):

```
EXPORT ISPERFECT(n)
BEGIN
  LOCAL d, sum;
  d := 2;
  sum := 1;
  WHILE sum <= n AND d < n DO
    IF irem(n,d)==0 THEN
      sum := sum+d;
    END;
    d := d+1;
  END;
  RETURN sum==n;
END;
```

Drugi program wywołuje poprzedni. `ISPERFECT` ma `EXPORT`, więc jest widoczny dla innych programów:

```
EXPORT PERFECTNUMS()
BEGIN
  LOCAL k;
  FOR k FROM 2 TO 1000 DO
    IF ISPERFECT(k) THEN
      MSGBOX(k+" is perfect, press OK");
    END;
  END;
END;
```

Program TARGET (*ES #3*) to gra w zgadywanie liczby 1–20:

```
EXPORT TARGET()
BEGIN
  LOCAL C:=0, N:=RANDINT(1,20), G:=-1;
  WHILE G<>N DO
    C := C+1;
    INPUT(G,"Guess?","GUESS:","1 - 20");
    IF G==0 THEN          // 0 kończy grę
      KILL;
    END;
    IF G < N THEN MSGBOX("Higher"); END;
    IF G > N THEN MSGBOX("Lower");  END;
  END;
  MSGBOX("Correct! Score: "+C);
END;
```

`KILL` natychmiast przerywa cały program (*ES #3*). Szczegóły w [rozdziale 7](07-wejscie.md).

## 6.5. REPEAT … UNTIL (*UG s. 631, ES #3*)

```
REPEAT
  polecenia;
UNTIL warunek;
```

Program ULAM (*ES #3*) sprawdza hipotezę Collatza: jeśli n jest parzyste, podziel je przez 2, a jeśli nieparzyste, weź 3n+1. Program liczy kroki potrzebne, żeby dojść do 1:

```
EXPORT ULAM(N)
BEGIN
  LOCAL C:=1, L0:={N};
  REPEAT
    IF FP(N/2)==0 THEN
      N := N/2;
    ELSE
      N := 3*N+1;
    END;
    C := C+1;
    L0 := CONCAT(L0,{N});     // dopisz N na końcu listy
  UNTIL N==1;
  MSGBOX("NO. OF STEPS="+C);
  RETURN L0;
END;
```

- `ULAM(5)`: komunikat „NO. OF STEPS=6”, lista `{5,16,8,4,2,1}`.
- `ULAM(22)`: komunikat „NO. OF STEPS=16”.

Typowe zastosowanie `REPEAT`: **walidacja danych**. Pytaj, dopóki użytkownik nie poda poprawnej wartości (*UG s. 631*):

```
EXPORT SIDES;
EXPORT GETSIDES()
BEGIN
  REPEAT
    INPUT(SIDES,"Die Sides","N = ","Enter num sides",2);
  UNTIL SIDES>0;
END;
```

## 6.6. BREAK i CONTINUE (*UG s. 632*)

- `BREAK` przerywa bieżącą pętlę. Program kontynuuje od pierwszego polecenia po niej.
- `BREAK(n)` przerywa **n poziomów** zagnieżdżonych pętli.
- `CONTINUE` przechodzi od razu do następnego obiegu pętli.

```
EXPORT PIERWSZA_WIEKSZA(n)
BEGIN
  // najmniejsza liczba pierwsza większa od n
  LOCAL k := n;
  WHILE 1 DO               // pętla "nieskończona"
    k := k+1;
    IF CAS.isprime(k) THEN BREAK; END;   // funkcja CAS, stąd prefiks CAS.
  END;
  RETURN k;
END;
```

```
EXPORT NIEPARZYSTE(n)
BEGIN
  LOCAL i, s:=0;
  FOR i FROM 1 TO n DO
    IF i MOD 2 == 0 THEN CONTINUE; END;   // pomiń parzyste
    s := s+i;
  END;
  RETURN s;                 // suma nieparzystych 1..n
END;
```

## 6.7. Pętla nieskończona i przerywanie programu

`WHILE 1 DO … END;` albo `REPEAT … UNTIL 0;` to pętle nieskończone. Przydają się w grach i animacjach, ale wtedy wyjście z pętli musi obsłużyć `BREAK` albo `KILL` (np. po naciśnięciu klawisza, zob. [rozdział 17](17-klawiatura-dotyk-czas.md)).

**Awaryjne przerwanie** działającego programu: klawisz **[On]**. Jeśli to nie działa, przytrzymaj [On]. Pamiętaj o tym przy testowaniu pętli.

Program PISERIES (*UG s. 643*) działa, dopóki użytkownik go nie przerwie. Liczy kolejne przybliżenia π z szeregu Leibniza:

```
EXPORT PISERIES()
BEGIN
  LOCAL sign;
  K := 2;
  A := 4;
  sign := -1;
  RECT();
  TEXTOUT_P("N=",0,0);
  TEXTOUT_P("PI APPROX=",0,30);
  REPEAT
    A := A+sign*4/(2*K-1);
    TEXTOUT_P(K,35,0,2,#FFFFFF,100,#333399);
    TEXTOUT_P(A,90,30,2,#000000,100,#99CC33);
    sign := sign*-1;
    K := K+1;
  UNTIL 0;
END;
```

## 6.8. Pułapki

- **Brakujące `END`** to najczęstszy błąd. Każda pętla, każde `IF` i każda funkcja ma swoje `END;`. Wcinaj kod i używaj (Check) (*ES #3*).
- **Zmiana zmiennej sterującej** wewnątrz `FOR` zmienia przebieg pętli. Unikaj tego.
- **Warunek, który nigdy nie stanie się prawdziwy** w `REPEAT`, oznacza pętlę nieskończoną.
- **Liczby ułamkowe w STEP**: przez błędy zaokrągleń ostatni obieg może się nie wykonać (np. `FOR x FROM 0 TO 1 STEP 0.1`). Bezpieczniej liczyć po liczbach całkowitych i przeliczać: `x := i/10`.

---

## Sprawdź się

1. `SILNIA(n)`: oblicz n! pętlą `FOR`.
2. `NWD(a,b)`: algorytm Euklidesa w pętli `WHILE`.
3. `PIERWSZE(n)`: wypisz w terminalu wszystkie liczby pierwsze ≤ n. Nie używaj `isprime`, tylko sprawdzaj dzielniki do √k i przerywaj pętlę `BREAK`.
4. `FIB(n)`: zwróć listę pierwszych n wyrazów ciągu Fibonacciego.

[Rozwiązania →](25-cwiczenia.md#rozdział-6)
