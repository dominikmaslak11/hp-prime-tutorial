# Rozdział 24. Projekty końcowe

[← Poprzedni](23-mapa-instrukcji.md) · [Spis treści](../README.md) · [Następny →](25-cwiczenia.md)

Cztery większe programy łączą materiał z całego kursu. Przy każdym jest lista wykorzystanych technik i pomysły na rozbudowę. Kody źródłowe znajdziesz też w katalogu [`programy/`](../programy/).

> Wskazówka: najpierw wpisz program i uruchom go. Dopiero potem czytaj kod linia po linii, szczególnie komentarze. Na koniec spróbuj samodzielnie dodać jedną z proponowanych rozbudów.

---

## Projekt 1: Quiz z tabliczki mnożenia (rekord w zmiennej eksportowanej)

**Techniki:** `EXPORT` zmiennej, `INPUT` z obsługą Cancel, `RANDINT`, `TICKS`, `MSGBOX`, `STRING`, pętla `FOR` z `BREAK`.

```
EXPORT QREKORD := 0;              // najlepszy wynik (pamiętany między uruchomieniami)

EXPORT QUIZ()
BEGIN
  LOCAL i, a, b, odp, dobre := 0, n := 10, t0, czas, pkt;
  MSGBOX("Quiz: "+n+" pytań. Liczy się poprawność i czas.");
  t0 := TICKS;
  FOR i FROM 1 TO n DO
    a := RANDINT(2,9);
    b := RANDINT(2,9);
    odp := 0;
    IF NOT INPUT(odp, "Pytanie "+i+"/"+n, a+" × "+b+" =", "Wpisz wynik") THEN
      BREAK;                                   // Cancel kończy quiz
    END;
    IF odp == a*b THEN
      dobre := dobre+1;
    ELSE
      MSGBOX("Źle! "+a+"×"+b+" = "+(a*b));
    END;
  END;
  czas := (TICKS-t0)/1000;
  pkt  := ROUND(dobre*100 - czas, 0);          // punkty: 100 za odpowiedź, minus sekundy
  IF pkt > QREKORD THEN
    QREKORD := pkt;
    MSGBOX("NOWY REKORD! "+pkt+" pkt");
  END;
  MSGBOX("Dobrze: "+dobre+"/"+n+"\nCzas: "+STRING(czas,2,1)+" s\nPunkty: "+pkt+"\nRekord: "+QREKORD);
  RETURN pkt;
END;
```

**Rozbudowa:** poziomy trudności (`CHOOSE`), dzielenie, zapis 5 najlepszych wyników w liście, przechowywanie rekordu w `AVars` własnej aplikacji.

---

## Projekt 2: Sito Eratostenesa z wizualizacją

**Techniki:** listy jako tablice, `MAKELIST`, pętle zagnieżdżone, `STEP`, grafika pikselowa (`RECT_P`, `TEXTOUT_P`), pomiar czasu, `FREEZE`.

Liczby od 1 do 400 są pokazane jako siatka 20×20. Wykreślane liczby zmieniają kolor na szary, a liczby pierwsze zostają zielone.

```
KOMORKA();

EXPORT SITO()
BEGIN
  LOCAL n := 400, p, k, t0, lp := {}, jest;
  jest := MAKELIST(1, X, 1, n, 1);   // 1 = kandydat na liczbę pierwszą
  jest(1) := 0;                      // 1 nie jest pierwsza

  RECT(#FFFFFFh);
  FOR k FROM 1 TO n DO KOMORKA(k, #D0E8FFh); END;   // wszystkie jasnoniebieskie
  t0 := TICKS;

  FOR p FROM 2 TO IP(√n) DO
    IF jest(p) THEN
      KOMORKA(p, #00C060h);                        // p jest pierwsza
      FOR k FROM p*p TO n STEP p DO                // wykreśl wielokrotności
        IF jest(k) THEN
          jest(k) := 0;
          KOMORKA(k, #A0A0A0h);
        END;
      END;
    END;
  END;

  FOR k FROM 2 TO n DO                             // pokoloruj pozostałe pierwsze
    IF jest(k) THEN
      KOMORKA(k, #00C060h);
      lp := CONCAT(lp, {k});
    END;
  END;
  TEXTOUT_P("Pierwszych: "+SIZE(lp)+"   czas: "+(TICKS-t0)+" ms", 4, 224, 2);
  FREEZE;
  RETURN lp;
END;

// rysuje komórkę liczby k (1..400) w siatce 20x20; komórka 11x11 px
KOMORKA(k, kolor)
BEGIN
  LOCAL w, c, x, y;
  w := IP((k-1)/20);           // wiersz 0..19
  c := (k-1) MOD 20;           // kolumna 0..19
  x := 50 + c*11;
  y := 1 + w*11;
  RECT_P(x, y, x+9, y+9, kolor, kolor);
END;
```

Na końcu program zwraca listę 78 liczb pierwszych ≤ 400.

**Rozbudowa:** spowolnienie animacji (`WAIT(0.01)` po każdym wykreśleniu), wyświetlanie numerów w komórkach czcionką 1, n jako parametr z automatycznym doborem rozmiaru siatki.

---

## Projekt 3: Własny rysownik wykresów z zoomem i przesuwaniem

**Techniki:** `INPUT` z polem tekstowym `[2]`, `EXPR`, `IFERR`, przeliczanie współrzędnych matematycznych na pikselowe, `LINE_P`, `GETKEY` w pętli, zapisywanie i przywracanie `HAngle`, podprogramy.

Sterowanie: strzałki przesuwają widok, [+] i [−] zmieniają przybliżenie, [Enter] pozwala wpisać nową funkcję, [Esc] kończy.

```
RYSUJ();
CZEKAJKLAWISZ();

EXPORT PLOTER()
BEGIN
  LOCAL fs := "SIN(X)*X", xmin := -10, xmax := 10, ymin := -8, ymax := 8;
  LOCAL k, dx, dy, sx, sy, stary := HAngle;
  HAngle := 0;                                  // radiany
  IF NOT INPUT({{fs,[2]}}, "Ploter", {"f(X)="}, {"np. SIN(X)*X"}) THEN
    HAngle := stary;
    RETURN "anulowano";
  END;

  REPEAT
    RYSUJ(fs, xmin, xmax, ymin, ymax);
    k := CZEKAJKLAWISZ();
    dx := (xmax-xmin)/5;  dy := (ymax-ymin)/5;
    sx := (xmax-xmin)/4;  sy := (ymax-ymin)/4;
    CASE
      IF k==7  THEN xmin := xmin-dx; xmax := xmax-dx; END;     // ◀
      IF k==8  THEN xmin := xmin+dx; xmax := xmax+dx; END;     // ▶
      IF k==2  THEN ymin := ymin+dy; ymax := ymax+dy; END;     // ▲
      IF k==12 THEN ymin := ymin-dy; ymax := ymax-dy; END;     // ▼
      IF k==50 THEN                                            // + przybliż
        xmin := xmin+sx; xmax := xmax-sx; ymin := ymin+sy; ymax := ymax-sy;
      END;
      IF k==45 THEN                                            // − oddal
        xmin := xmin-2*sx; xmax := xmax+2*sx; ymin := ymin-2*sy; ymax := ymax+2*sy;
      END;
      IF k==30 THEN                                            // Enter: nowa funkcja
        INPUT({{fs,[2]}}, "Ploter", {"f(X)="});
      END;
    END;
  UNTIL k==4;                                                  // Esc

  HAngle := stary;
  RETURN fs;
END;

RYSUJ(fs, xmin, xmax, ymin, ymax)
BEGIN
  LOCAL px, py, y, ok, pokX, pokY, prevOK := 0, prevPY := 0;
  RECT();
  // osie (jeśli są w oknie)
  pokX := ROUND((0-xmin)/(xmax-xmin)*319, 0);
  pokY := ROUND((ymax-0)/(ymax-ymin)*219, 0);
  IF pokY>=0 AND pokY<=219 THEN LINE_P(0,pokY,319,pokY,#A0A0A0h); END;
  IF pokX>=0 AND pokX<=319 THEN LINE_P(pokX,0,pokX,219,#A0A0A0h); END;

  FOR px FROM 0 TO 319 DO
    X := xmin + px*(xmax-xmin)/319;            // globalne X używane przez EXPR
    IFERR
      y := EXPR(fs);
      ok := (TYPE(y)==0);                      // tylko wynik rzeczywisty
    THEN
      ok := 0;
    END;
    IF ok THEN
      py := ROUND((ymax-y)/(ymax-ymin)*219, 0);
      ok := (py > -1000 AND py < 1200);        // odrzuć asymptoty
    END;
    IF ok AND prevOK THEN
      LINE_P(px-1, prevPY, px, py, #0060D0h);
    END;
    prevOK := ok;
    IF ok THEN prevPY := py; END;
  END;

  RECT_P(0,220,319,239,#F0F0F0h,#F0F0F0h);
  TEXTOUT_P("f="+fs, 2, 223, 1);
  TEXTOUT_P("x:["+ROUND(xmin,2)+","+ROUND(xmax,2)+"] y:["+ROUND(ymin,2)+","+ROUND(ymax,2)+"]", 120, 223, 1);
END;

CZEKAJKLAWISZ()
BEGIN
  LOCAL k;
  REPEAT k := GETKEY; UNTIL k <> -1;
  RETURN k;
END;
```

**Rozbudowa:** kilka funkcji w różnych kolorach (lista tekstów), kursor „trace” sterowany strzałkami z odczytem (x, f(x)), przesuwanie widoku palcem (`MOUSE`, typ 2 = przeciąganie), rysowanie w buforze `G1`.

---

## Projekt 4: Gra Snake

**Techniki:** listy współrzędnych jako kolejka, `CONCAT` i podlisty `L({1,n})`, podwójne buforowanie (`DIMGROB_P`, `BLIT_P`), sterowanie w czasie rzeczywistym (`GETKEY` z opróżnianiem bufora), tempo gry przez `TICKS`, wykrywanie kolizji, `BREAK`.

Plansza ma 32×22 pola po 10 px. Strzałki zmieniają kierunek, [Esc] kończy grę.

```
EXPORT SNAKE()
BEGIN
  LOCAL sx := {10,9,8}, sy := {7,7,7};      // segmenty węża; element 1 = głowa
  LOCAL dx := 1, dy := 0, cdx, cdy;         // kierunek ruchu
  LOCAL fx, fy, nx, ny, k, i, kol, n;
  LOCAL pkt := 0, gra := 1, t, tempo := 150;

  DIMGROB_P(G1, 320, 240);
  fx := RANDINT(0,31); fy := RANDINT(0,21);

  WHILE gra DO
    t := TICKS;

    // --- wejście: odczytaj wszystkie klawisze z bufora ---
    cdx := dx; cdy := dy;                   // kierunek z początku tego kroku
    REPEAT
      k := GETKEY;
      IF k==7  AND cdx==0 THEN dx := -1; dy := 0;  END;   // ◀
      IF k==8  AND cdx==0 THEN dx := 1;  dy := 0;  END;   // ▶
      IF k==2  AND cdy==0 THEN dx := 0;  dy := -1; END;   // ▲
      IF k==12 AND cdy==0 THEN dx := 0;  dy := 1;  END;   // ▼
      IF k==4 THEN gra := 0; END;                         // Esc
    UNTIL k == -1;
    IF NOT gra THEN BREAK; END;

    // --- ruch i kolizje ---
    nx := sx(1)+dx;  ny := sy(1)+dy;
    IF nx<0 OR nx>31 OR ny<0 OR ny>21 THEN BREAK; END;    // ściana
    kol := 0;
    FOR i FROM 1 TO SIZE(sx) DO
      IF sx(i)==nx AND sy(i)==ny THEN kol := 1; BREAK; END;
    END;
    IF kol THEN BREAK; END;                               // własny ogon

    sx := CONCAT({nx}, sx);                               // nowa głowa
    sy := CONCAT({ny}, sy);
    IF nx==fx AND ny==fy THEN
      pkt := pkt+1;                                       // zjadł: ogon zostaje (wąż rośnie)
      fx := RANDINT(0,31); fy := RANDINT(0,21);
      tempo := MAX(60, tempo-5);                          // coraz szybciej
    ELSE
      n := SIZE(sx);
      sx := sx({1, n-1});                                 // usuń ostatni segment
      sy := sy({1, n-1});
    END;

    // --- rysowanie klatki ---
    RECT_P(G1, 0, 0, 319, 239, #101010h, #101010h);
    RECT_P(G1, fx*10, fy*10, fx*10+9, fy*10+9, #FF3030h, #FF3030h);
    FOR i FROM 1 TO SIZE(sx) DO
      RECT_P(G1, sx(i)*10, sy(i)*10, sx(i)*10+9, sy(i)*10+9, #008000h, #30E030h);
    END;
    LINE_P(G1, 0, 220, 319, 220, #808080h);
    TEXTOUT_P("Punkty: "+pkt+"    Esc = koniec", G1, 4, 224, 2, #FFFFFFh);
    BLIT_P(G0, G1);

    // --- stałe tempo niezależne od czasu rysowania ---
    WHILE TICKS - t < tempo DO END;
  END;

  MSGBOX("Koniec gry! Punkty: "+pkt);
  RETURN pkt;
END;
```

Dlaczego kierunek z początku kroku (`cdx`, `cdy`)? Gdy w jednym kroku gry naciśniesz szybko dwa klawisze, np. ▲ i ◀ przy ruchu w prawo, bez tej zmiennej wąż mógłby zawrócić w miejscu i wpaść na samego siebie.

**Rozbudowa:** jedzenie nie może pojawiać się na wężu (losuj, dopóki pole nie jest wolne), rekord w `EXPORT`, przeszkody, sterowanie dotykiem (który kwadrant ekranu został dotknięty), pauza pod [Enter].

---

## Pomysły na własne projekty

| Projekt | Główne techniki |
|---|---|
| Kółko i krzyżyk (dotyk) | `MOUSE`, macierz 3×3, sprawdzanie wygranej |
| Konwerter jednostek z menu | `CHOOSE`, listy współczynników, `CONVERT` |
| Rozwiązywanie trójkątów z rysunkiem | twierdzenia sinusów i cosinusów, `LINE_P`, `TEXTOUT_P` |
| Symulator rzutu monetą z wykresem | `RANDINT`, Statistics 1Var, `STARTVIEW` |
| Gra w życie Conwaya | macierz, pętle zagnieżdżone, `BLIT_P` |
| Notatnik pomiarów laboratoryjnych | `EDITMAT`, regresja (Stats 2Var), `Notes` |
| Mastermind / zgadywanie kodu | listy, `INPUT`, `POS`, pętle |
| Zegar analogowy na żywo | `Time`, `ARC_P`, `LINE_P`, pętla z `WAIT(1)` |
