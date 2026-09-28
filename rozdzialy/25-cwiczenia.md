# Rozdział 25. Ćwiczenia — rozwiązania

[← Poprzedni](24-projekty.md) · [Spis treści](../README.md)

Rozwiązania zadań „Sprawdź się” z końca każdego rozdziału. Najpierw spróbuj rozwiązać zadanie sam. Twoje rozwiązanie może wyglądać inaczej i nadal być poprawne.

---

## Rozdział 1

```
EXPORT CUBE(X)
BEGIN
  RETURN X^3;
END;
```

```
EXPORT CIRC(R)
BEGIN
  RETURN {2*π*R, π*R^2};
END;
```

Plik `GEOM` (usuń automatyczny szablon):

```
EXPORT SQAREA(a)
BEGIN
  RETURN a^2;
END;

EXPORT RECTAREA(a,b)
BEGIN
  RETURN a*b;
END;

EXPORT TRIAREA(a,h)
BEGIN
  RETURN a*h/2;
END;
```

W [Toolbox] (User) pojawi się pozycja `GEOM >` z trzema funkcjami.

## Rozdział 2

1. Program zwraca **6**. Globalne `B` nadal ma wartość **10**, bo `LOCAL B` przesłania zmienną globalną.
2. Licznik:
   ```
   EXPORT ILE := 0;
   EXPORT LICZNIK()
   BEGIN
     ILE := ILE + 1;
     RETURN ILE;
   END;
   ```
3. `L1` to zmienna systemowa, która przechowuje wyłącznie **listy**. Zapisanie w niej liczby jest niedozwolone. Poprawnie: `L1 := {5};`.

## Rozdział 3

1. `7 MOD 3` → 1; `IP(-7/2)` → -3; `FLOOR(-7/2)` → -4; `"A"+1+2` → `"A12"` (działania od lewej: najpierw tekst + 1); `1+2+"A"` → `"3A"`.
2. Rok przestępny:
   ```
   EXPORT PRZESTEPNY(r)
   BEGIN
     RETURN ((r MOD 4 == 0) AND (r MOD 100 <> 0)) OR (r MOD 400 == 0);
   END;
   ```
3. Suma cyfr:
   ```
   EXPORT CYFRY(n)
   BEGIN
     LOCAL s := 0;
     WHILE n > 0 DO
       s := s + (n MOD 10);
       n := IP(n/10);
     END;
     RETURN s;
   END;
   ```

## Rozdział 4

```
EXPORT TABLICZKA(n)
BEGIN
  LOCAL i;
  PRINT();
  FOR i FROM 1 TO 10 DO
    PRINT(n+" x "+i+" = "+(n*i));
  END;
END;
```

Zwróć uwagę na nawias `(n*i)`. Bez niego `+` najpierw dokleiłby `n` do tekstu, a potem próbował pomnożyć tekst przez `i`.

```
EXPORT QROOTS2(A,B,C)
BEGIN
  LOCAL D, r1, r2, stary := HComplex;
  PRINT();
  HComplex := 1;
  D := B^2-4*A*C;
  IF D>=0 THEN PRINT("Roots are real."); ELSE PRINT("Roots are complex."); END;
  r1 := (-B+√D)/(2*A);
  r2 := (-B-√D)/(2*A);
  PRINT(r1); PRINT(r2);
  HComplex := stary;
  RETURN {r1, r2};
END;
```

```
EXPORT LOSUJ()
BEGIN
  IF MSGBOX("Wylosować liczbę 1-100?", 1) THEN
    MSGBOX("Wylosowano: " + RANDINT(1,100));
  END;
END;
```

## Rozdział 5

```
EXPORT TROJKAT(a,b,c)
BEGIN
  CASE
    IF a+b<=c OR a+c<=b OR b+c<=a THEN RETURN "nie istnieje"; END;
    IF a==b AND b==c THEN RETURN "równoboczny"; END;
    IF a==b OR b==c OR a==c THEN RETURN "równoramienny"; END;
    DEFAULT RETURN "różnoboczny";
  END;
END;
```

```
EXPORT ABSW(x)
BEGIN
  RETURN when(x<0, -x, x);
END;
```

```
EXPORT ODWR(x)
BEGIN
  IF x==0 THEN
    MSGBOX("Dzielenie przez zero!");
    RETURN 0;
  END;
  RETURN 1/x;
END;

EXPORT ODWR2(x)
BEGIN
  LOCAL w;
  IFERR
    w := 1/x;
  THEN
    MSGBOX("Błąd obliczeń!");
    w := 0;
  END;
  RETURN w;
END;
```

Uwaga: w niektórych ustawieniach `1/0` daje ∞ zamiast błędu i `IFERR` go nie przechwyci. Wersja z `IF` jest pewniejsza.

## Rozdział 6

```
EXPORT SILNIA(n)
BEGIN
  LOCAL i, w := 1;
  FOR i FROM 2 TO n DO w := w*i; END;
  RETURN w;
END;
```

```
EXPORT NWD(a,b)
BEGIN
  LOCAL t;
  WHILE b <> 0 DO
    t := a MOD b;
    a := b;
    b := t;
  END;
  RETURN a;
END;
```

```
EXPORT PIERWSZE(n)
BEGIN
  LOCAL k, d, pierwsza;
  PRINT();
  FOR k FROM 2 TO n DO
    pierwsza := 1;
    FOR d FROM 2 TO IP(√k) DO
      IF k MOD d == 0 THEN
        pierwsza := 0;
        BREAK;
      END;
    END;
    IF pierwsza THEN PRINT(k); END;
  END;
END;
```

Dla k = 2 i k = 3 wewnętrzna pętla nie wykona się ani razu (bo `IP(√k)` = 1 < 2), więc liczby te zostaną poprawnie uznane za pierwsze.

```
EXPORT FIB(n)
BEGIN
  LOCAL f := {1,1}, i;
  IF n<=2 THEN RETURN f({1,n}); END;
  FOR i FROM 3 TO n DO
    f := CONCAT(f, {f(i-1)+f(i-2)});
  END;
  RETURN f;
END;
```

## Rozdział 7

```
EXPORT BMI()
BEGIN
  LOCAL m, h, b, opis;
  IF NOT INPUT({m,h}, "BMI", {"Masa [kg]:","Wzrost [m]:"}) THEN
    RETURN "anulowano";
  END;
  b := m/h^2;
  CASE
    IF b<18.5 THEN opis := "niedowaga"; END;
    IF b<25   THEN opis := "norma"; END;
    IF b<30   THEN opis := "nadwaga"; END;
    DEFAULT opis := "otyłość";
  END;
  MSGBOX("BMI = "+ROUND(b,1)+" ("+opis+")");
  RETURN b;
END;
```

```
EXPORT TEMPERATURA()
BEGIN
  LOCAL w, t, r;
  IF NOT CHOOSE(w, "Przelicz", "°C → °F", "°F → °C", "°C → K") THEN RETURN "anulowano"; END;
  IF NOT INPUT(t, "Temperatura", "t =") THEN RETURN "anulowano"; END;
  CASE
    IF w==1 THEN r := t*9/5+32; END;
    IF w==2 THEN r := (t-32)*5/9; END;
    IF w==3 THEN r := t+273.15; END;
  END;
  MSGBOX("Wynik: "+r);
  RETURN r;
END;
```

```
EXPORT KANAPKA()
BEGIN
  LOCAL ma:=0, se:=0, sz:=0;
  IF NOT INPUT({{ma,1},{se,1},{sz,1}}, "Kanapka", {"Masło","Ser","Szynka"}) THEN
    RETURN "anulowano";
  END;
  RETURN 3 + 1.5*(ma+se+sz);
END;
```

## Rozdział 8

```
OBLICZBMI();

EXPORT BMI2()
BEGIN
  LOCAL m, h;
  IF NOT INPUT({m,h}, "BMI", {"Masa [kg]:","Wzrost [m]:"}) THEN RETURN "anulowano"; END;
  MSGBOX("BMI = "+ROUND(OBLICZBMI(m,h),1));
  RETURN OBLICZBMI(m,h);
END;

OBLICZBMI(m,h)
BEGIN
  RETURN m/h^2;
END;
```

```
EXPORT POTEGA(a,n)
BEGIN
  LOCAL p;
  IF n==0 THEN RETURN 1; END;
  IF n MOD 2 == 0 THEN
    p := POTEGA(a, n/2);
    RETURN p*p;
  END;
  RETURN a*POTEGA(a, n-1);
END;
```

Biblioteka `STATLIB`:

```
EXPORT SREDNIA(l)
BEGIN
  RETURN ΣLIST(l)/SIZE(l);
END;

EXPORT MEDIANA(l)
BEGIN
  LOCAL s := SORT(l), n := SIZE(l);
  IF n MOD 2 == 1 THEN
    RETURN s((n+1)/2);
  END;
  RETURN (s(n/2)+s(n/2+1))/2;
END;

EXPORT ODCH(l)
BEGIN
  LOCAL m := SREDNIA(l);
  RETURN √(ΣLIST((l-m)^2)/SIZE(l));
END;
```

Program korzystający z biblioteki (osobny plik):

```
EXPORT TEST_STAT()
BEGIN
  LOCAL d := {2,4,4,4,5,5,7,9};
  RETURN {SREDNIA(d), MEDIANA(d), ODCH(d)};   // {5, 4.5, 2}
END;
```

## Rozdział 9

```
EXPORT PALINDROM(s)
BEGIN
  LOCAL i, t := "", c;
  s := LOWER(s);
  FOR i FROM 1 TO DIM(s) DO          // usuń spacje
    c := MID(s,i,1);
    IF c <> " " THEN t := t + c; END;
  END;
  FOR i FROM 1 TO IP(DIM(t)/2) DO
    IF MID(t,i,1) <> MID(t,DIM(t)-i+1,1) THEN RETURN 0; END;
  END;
  RETURN 1;
END;
```

```
EXPORT ZAMIEN(s, a, b)
BEGIN
  LOCAL w := "", p;
  p := INSTRING(s, a);
  WHILE p > 0 DO
    w := w + when(p>1, LEFT(s, p-1), "") + b;
    s := MID(s, p+DIM(a));
    p := INSTRING(s, a);
  END;
  RETURN w + s;
END;
```

`when(p>1, …, "")` zabezpiecza przypadek, gdy wzorzec stoi na samym początku tekstu. Opis `LEFT` w instrukcji jest dla n = 0 niejednoznaczny, więc nie polegamy na `LEFT(s,0)`.

```
EXPORT BIN(n)
BEGIN
  LOCAL w := "";
  IF n==0 THEN RETURN "0"; END;
  WHILE n > 0 DO
    w := STRING(n MOD 2) + w;
    n := IP(n/2);
  END;
  RETURN w;
END;
```

## Rozdział 10

```
EXPORT USUN_DUPL(l)
BEGIN
  LOCAL w := {}, i;
  FOR i FROM 1 TO SIZE(l) DO
    IF POS(w, l(i)) == 0 THEN w := CONCAT(w, {l(i)}); END;
  END;
  RETURN w;
END;
```

```
EXPORT HISTO(l)
BEGIN
  LOCAL h := MAKELIST(0,X,1,6,1), i;
  FOR i FROM 1 TO SIZE(l) DO
    h(l(i)) := h(l(i)) + 1;
  END;
  RETURN h;
END;
```

Suma kwadratów bez pętli: `ΣLIST(MAKELIST(X^2,X,1,100,1))` → 338350. Można też użyć szablonu `Σ(N^2,N,1,100)`.

## Rozdział 11

```
EXPORT CRAMER2(a,b,c,d,e,f)
BEGIN
  LOCAL W := DET([[a,b],[c,d]]);
  IF W==0 THEN RETURN "brak jednoznacznego rozwiązania"; END;
  RETURN {DET([[e,b],[f,d]])/W, DET([[a,e],[c,f]])/W};
END;
```

Sprawdzenie: `[[a,b],[c,d]]^-1*[[e],[f]]`.

```
EXPORT OBROT(p, kat)
BEGIN
  LOCAL stary := HAngle, R, w;
  HAngle := 1;
  R := [[COS(kat), -SIN(kat)], [SIN(kat), COS(kat)]];
  w := R*TRN([p]);                 // wektor kolumnowy
  HAngle := stary;
  RETURN [w(1,1), w(2,1)];
END;
```

```
EXPORT ZERUJ_UJEMNE(m)
BEGIN
  LOCAL d := SIZE(m), i, j;
  FOR i FROM 1 TO d(1) DO
    FOR j FROM 1 TO d(2) DO
      IF m(i,j) < 0 THEN m(i,j) := 0; END;
    END;
  END;
  RETURN m;
END;
```

## Rozdział 12

```
EXPORT RGBROZ(k)
BEGIN
  RETURN {BITAND(BITSR(k,16),255), BITAND(BITSR(k,8),255), BITAND(k,255)};
END;
```

```
EXPORT PARZYSTOSC(n)
BEGIN
  LOCAL p := 0;
  WHILE n > 0 DO
    p := BITXOR(p, BITAND(n,1));
    n := BITSR(n,1);
  END;
  RETURN p;
END;
```

3. `#FFh + #1b` → `#100h` (255+1 = 256, wynik w systemie pierwszego argumentu); `#7o * #10b` → `#16o` (7·2 = 14 = 16₈); `BITXOR(#F0h,#FFh)` → `#Fh`.

## Rozdział 13

```
EXPORT TRAPEZY(fs,a,b,n)
BEGIN
  LOCAL h := (b-a)/n, s := 0, i;
  FOR i FROM 0 TO n DO
    X := a + i*h;
    s := s + when(i==0 OR i==n, 0.5, 1)*EXPR(fs);
  END;
  RETURN s*h;
END;
```

`TRAPEZY("SIN(X)",1,3,100)` ≈ 1.5302 (∫ daje 1.53029480247).

```
EXPORT STYCZNA(fs, x0)
BEGIN
  LOCAL a, y0;
  X := x0;
  y0 := EXPR(fs);
  a := EXPR("∂("+fs+",X="+x0+")");
  RETURN {a, y0 - a*x0};
END;
```

Porównanie czasów:

```
EXPORT CZASY()
BEGIN
  LOCAL t1, t2;
  t1 := TEVAL(Σ(N^2,N,1,1000));
  t2 := TEVAL(ΣLIST(MAKELIST(X^2,X,1,1000,1)));
  RETURN {t1, t2};
END;
```

(Wersję z pętlą `FOR` zmierz przez `TICKS` przed pętlą i po niej, jak w [rozdziale 17](17-klawiatura-dotyk-czas.md).)

## Rozdział 14

```
EXPORT FLAGA()
BEGIN
  RECT_P(0,0,319,119,#FFFFFFh,#FFFFFFh);
  RECT_P(0,120,319,239,#DC143Ch,#DC143Ch);
  FREEZE;
END;
```

```
EXPORT DATA_EKRAN()
BEGIN
  RECT();
  TEXTOUT_P(STRING(Date), 70, 100, 7, #000000h, 200, #FFFF00h);
  FREEZE;
END;
```

W SNOWFLAKE zamień `RECT();` na `RECT(#000080h);`, a w pętli użyj `TEXTOUT_P("*",X,Y,RANDINT(1,7),Z);`.

## Rozdział 15

```
EXPORT TARCZA()
BEGIN
  LOCAL i, k;
  RECT();
  FOR i FROM 5 DOWNTO 1 DO
    k := when(i MOD 2 == 1, #FF0000h, #FFFFFFh);
    ARC_P(160, 110, i*20, {#000000h, k});
  END;
  FREEZE;
END;
```

```
EXPORT SLUPKI()
BEGIN
  LOCAL d := {5,12,7,9,3}, kol := {#FF0000h,#00A000h,#0000FFh,#FF8000h,#8000FFh};
  LOCAL i, h, x, mx := MAX(d);
  RECT();
  LINE_P(20,200,300,200,#000000h);
  FOR i FROM 1 TO SIZE(d) DO
    h := d(i)/mx*170;
    x := 30 + (i-1)*55;
    RECT_P(x, 200-h, x+40, 200, kol(i), kol(i));
    TEXTOUT_P(d(i), x+12, 205, 2);
  END;
  FREEZE;
END;
```

```
EXPORT ZEGAR()
BEGIN
  LOCAL t := HMS→(Time), g, m, stary := HAngle;   // Time → godziny dziesiętne
  HAngle := 1;
  g := (t MOD 12)*30;          // kąt wskazówki godzinowej
  m := FP(t)*60*6;             // kąt wskazówki minutowej
  RECT();
  ARC_P(160,110,90,#000000h);
  LINE_P(160,110,160+50*SIN(g),110-50*COS(g),#000000h);
  LINE_P(160,110,160+80*SIN(m),110-80*COS(m),#0000FFh);
  HAngle := stary;
  FREEZE;
END;
```

(Jeśli na Twoim firmware `Time` zwraca już liczbę dziesiętną, pomiń `HMS→`.)

## Rozdział 16

Pięć piłek: pozycje i prędkości w listach, rysowanie w pętli `FOR i FROM 1 TO 5`. Wystarczy przerobić PILKA:

```
EXPORT PILKI()
BEGIN
  LOCAL n:=5, x, y, vx, vy, r:=8, i, kol;
  x := RANDINT(n,20,300); y := RANDINT(n,20,200);
  vx := RANDINT(n,1,4);   vy := RANDINT(n,1,4);
  kol := {#FF0000h,#00FF00h,#0080FFh,#FFFF00h,#FF00FFh};
  DIMGROB_P(G1,320,240);
  REPEAT
    RECT_P(G1,0,0,319,239,#000000h,#000000h);
    FOR i FROM 1 TO n DO
      ARC_P(G1, x(i), y(i), r, {kol(i), kol(i)});
      x(i) := x(i)+vx(i); y(i) := y(i)+vy(i);
      IF x(i)<r OR x(i)>319-r THEN vx(i) := -vx(i); END;
      IF y(i)<r OR y(i)>219-r THEN vy(i) := -vy(i); END;
    END;
    BLIT_P(G0,G1);
    WAIT(0.02);
  UNTIL GETKEY <> -1;
END;
```

Sprite z przezroczystym tłem:

```
EXPORT SPRITE()
BEGIN
  LOCAL x:=0;
  DIMGROB_P(G2,16,16,#FFFFFFh);                  // białe tło = przezroczyste
  ARC_P(G2,8,8,7,{#000000h,#FFD000h});           // buźka
  PIXON_P(G2,5,6,#000000h); PIXON_P(G2,10,6,#000000h);
  LINE_P(G2,5,11,10,11,#000000h);
  DIMGROB_P(G1,320,240);
  REPEAT
    RECT_P(G1,0,0,319,239,#40A0FFh,#40A0FFh);
    BLIT_P(G1, x, 100, G2, 0, 0, 16, 16, #FFFFFFh);   // pomiń biały kolor
    BLIT_P(G0,G1);
    x := (x+2) MOD 304;
    WAIT(0.02);
  UNTIL GETKEY <> -1;
END;
```

Zadanie 3 rozwiązuje program MENU_DOTYK z [rozdziału 17.3](17-klawiatura-dotyk-czas.md#przyciski-ekranowe-drawmenu--mouse). Zamień wypełnianie kolorem na `ARC_P` i `RECT_P`.

## Rozdział 17

```
EXPORT DOTYKI()
BEGIN
  LOCAL m, kol := {#FF0000h,#00A000h,#0000FFh,#FF8000h,#8000FFh,#000000h};
  RECT();
  REPEAT
    m := MOUSE;
    IF SIZE(m(1)) > 0 THEN
      ARC_P(m(1,1), m(1,2), 3, {kol(m(1,5)+1), kol(m(1,5)+1)});
    END;
  UNTIL GETKEY == 4;
END;
```

Zadanie 2: przed zmianą pozycji w RUCH sprawdź kolor pikseli w nowym miejscu, np. czterech narożników kwadratu. Jeśli którykolwiek jest czarny (`GETPIX_P(G1,…) == 0`), nie wykonuj ruchu. Przeszkodę rysuj w każdej klatce przed sprawdzeniem.

```
EXPORT TEST_CYFR()
BEGIN
  LOCAL kody := {47,42,43,44,37,38,39,32,33,34};   // klawisze cyfr 0..9
  LOCAL i, c, k, t0, suma := 0, bledy := 0;
  FOR i FROM 1 TO 10 DO
    c := RANDINT(0,9);
    RECT();
    TEXTOUT_P(c, 150, 90, 7);
    WHILE GETKEY <> -1 DO END;                     // wyczyść bufor
    t0 := TICKS;
    REPEAT k := GETKEY; UNTIL k <> -1;
    suma := suma + (TICKS - t0);
    IF k <> kody(c+1) THEN bledy := bledy + 1; END;
  END;
  MSGBOX("Średni czas: "+ROUND(suma/10,0)+" ms\nBłędy: "+bledy);
END;
```

## Rozdział 18

```
EXPORT TRYG()
BEGIN
  STARTAPP("Function");
  F1 := "SIN(X)"; F2 := "COS(X)"; F3 := "TAN(X)";
  F1(COLOR) := #FF0000h; F2(COLOR) := #0000FFh; F3(COLOR) := #00A000h;
  CHECK(1); CHECK(2); CHECK(3);
  AAngle := 1;                       // radiany w tej aplikacji
  Xmin := -2*π; Xmax := 2*π; Ymin := -3; Ymax := 3;
  STARTVIEW(1,1);
END;
```

```
EXPORT STYCZNA_WYKRES(fs, x0)
BEGIN
  LOCAL a, y0;
  STARTAPP("Function");
  F1 := fs;
  X := x0; y0 := EXPR(fs);
  a := EXPR("∂("+fs+",X="+x0+")");
  F2 := STRING(a)+"*(X-("+x0+"))+("+y0+")";
  CHECK(1); CHECK(2);
  Xmin := x0-5; Xmax := x0+5; Ymin := y0-5; Ymax := y0+5;
  STARTVIEW(1,1);
END;
```

```
EXPORT REGRESJA()
BEGIN
  STARTAPP("Statistics 2Var");
  C1 := {1,2,3,4,5};
  C2 := {2.1,3.9,6.2,7.8,10.1};
  SetIndep(S1, C1);
  SetDepend(S1, C2);
  CHECK(1);
  Do2VStats(S1);
  RETURN Corr;
END;
```

## Rozdział 19

Kalkulator Ohma (program aplikacji zapisanej na bazie Solve):

```
EXPORT U_, I_, R_;

VIEW "Oblicz U", OBL_U()
BEGIN
  IF INPUT({I_,R_}, "U = I·R", {"I [A]:","R [Ω]:"}) THEN
    U_ := I_*R_; MSGBOX("U = "+U_+" V");
  END;
  STARTVIEW(7,1);
END;

VIEW "Oblicz I", OBL_I()
BEGIN
  IF INPUT({U_,R_}, "I = U/R", {"U [V]:","R [Ω]:"}) THEN
    I_ := U_/R_; MSGBOX("I = "+I_+" A");
  END;
  STARTVIEW(7,1);
END;

VIEW "Oblicz R", OBL_R()
BEGIN
  IF INPUT({U_,I_}, "R = U/I", {"U [V]:","I [A]:"}) THEN
    R_ := U_/I_; MSGBOX("R = "+R_+" Ω");
  END;
  STARTVIEW(7,1);
END;
```

Zgodnie z zaleceniem instrukcji zmienne warto eksportować z osobnego programu.

Statystyki w DiceSimulation:

```
VIEW "Statystyki", STATY()
BEGIN
  Do1VStats(H1);
  MSGBOX("Średnia suma: "+MeanX+"\nMediana: "+MedVal);
  STARTVIEW(7,1);
END;
```

Licznik uruchomień w `START`:

```
START()
BEGIN
  IFERR
    AVars("Uruch") := AVars("Uruch") + 1;
  THEN
    AVars("Uruch") := 1;             // pierwsze uruchomienie: zmienna jeszcze nie istnieje
  END;
  MSGBOX("Uruchomienie nr "+AVars("Uruch"));
END;
```

## Rozdział 20

```
KEY KA_Sin()
BEGIN
  RETURN "ASIN(";
END;

KEY KA_Cos()
BEGIN
  RETURN "ACOS(";
END;

KEY KA_Tan()
BEGIN
  RETURN "ATAN(";
END;

KEY KS_9()
BEGIN
  RETURN "9.80665";
END;

KEY KSA_Help()
BEGIN
  MSGBOX("Godzina: "+Time);
  RETURN "";
END;
```

Przed użyciem sprawdź w swoim modelu, czy wybrana kombinacja klawiszy nie jest już używana przez system, np. [Shift][9]. Najpewniej jest generować nazwy przez *Create user key*.

## Rozdział 21

```
EXPORT LISTA_PROGRAMOW()
BEGIN
  LOCAL p := Programs, i;
  PRINT();
  FOR i FROM 1 TO SIZE(p) DO
    PRINT(p(i)+": "+DIM(Programs(p(i)))+" znaków");
  END;
END;
```

```
EXPORT DZIENNIK(t)
BEGIN
  LOCAL stara := "";
  IF POS(Notes, "Dziennik") > 0 THEN
    stara := Notes("Dziennik");
  END;
  Notes("Dziennik") := stara + Date + ": " + t + "\n";
END;
```

```
EXPORT SPRZATAJ(pref)
BEGIN
  LOCAL h := HVars, i, n := 0;
  IF NOT MSGBOX("Usunąć zmienne zaczynające się od "+pref+"?", 1) THEN RETURN 0; END;
  FOR i FROM 1 TO SIZE(h) DO
    IF LEFT(h(i), DIM(pref)) == pref THEN
      DelHVars(h(i));
      n := n+1;
    END;
  END;
  RETURN n;       // liczba usuniętych zmiennych
END;
```

## Rozdział 22

Błędy w programie SREDNIA:
1. `LOCAL i, s = 0`: należy użyć `:=`, nie `=`,
2. brak średnika po deklaracji `LOCAL`,
3. `FOR i FROM 1 TO SIZE(lst)`: brak słowa `DO`,
4. brak `END;` zamykającego pętlę `FOR`,
5. końcowe `END` bez średnika,
6. brak obsługi pustej listy: przy `SIZE(lst)==0` nastąpi dzielenie przez zero. To błąd logiczny, nie składniowy.

Poprawnie:

```
EXPORT SREDNIA(lst)
BEGIN
  LOCAL i, s := 0;
  IF SIZE(lst)==0 THEN RETURN "pusta lista"; END;
  FOR i FROM 1 TO SIZE(lst) DO
    s := s + lst(i);
  END;
  RETURN s/SIZE(lst);
END;
```
