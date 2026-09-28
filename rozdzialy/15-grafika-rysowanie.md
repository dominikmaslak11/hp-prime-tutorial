# Rozdział 15. Grafika — rysowanie: piksele, linie, prostokąty, wielokąty, łuki, trójkąty

[← Poprzedni](14-grafika-podstawy.md) · [Spis treści](../README.md) · [Następny →](16-grafika-zaawansowana.md)

Źródła: *ES #8*, *UG s. 593–601*. W składni `[G]` oznacza opcjonalny GROB (domyślnie `G0`). Każda komenda ma wersję kartezjańską (bez `_P`) o tej samej składni.

---

## 15.1. Piksele: PIXON, PIXOFF, GETPIX

```
PIXON_P([G], x, y [, kolor]);    // zapal piksel (domyślnie czarny)
PIXOFF_P([G], x, y);             // ustaw piksel na biały
GETPIX_P([G], x, y);             // odczytaj kolor piksela
```

Według instrukcji kolor w `PIXON` może mieć postać `aaRRGGBB`, gdzie `aa` to kanał alfa od 0 (kolor nieprzezroczysty) do 255 (w pełni przezroczysty) (*UG s. 597*).

Przykład: wykres funkcji piksel po pikselu.

```
EXPORT WYKRES_PIX()
BEGIN
  LOCAL px, x, y, py;
  RECT();
  LINE_P(0,120,319,120,#C0C0C0h);          // oś x
  LINE_P(160,0,160,219,#C0C0C0h);          // oś y
  FOR px FROM 0 TO 319 DO
    x := (px-160)/30;                      // 30 pikseli = 1 jednostka
    y := SIN(x)*2;
    py := ROUND(120 - y*30, 0);
    IF py>=0 AND py<220 THEN
      PIXON_P(px, py, #0000FFh);
    END;
  END;
  FREEZE;
END;
```

(W trybie radianów: `HAngle:=0`.)

`GETPIX_P` pozwala wykrywać kolizje w grach: „czy w miejscu, w które się ruszam, jest ściana (czarny piksel)?”.

## 15.2. Linie: LINE_P (*ES #8, UG s. 595*)

```
LINE_P([G], x1, y1, x2, y2 [, kolor]);
```

Program DRAWHOUSE (*ES #8*) rysuje domek: ściany brązowe (`#905000h`), dach bordowy (`#800000h`).

```
EXPORT DRAWHOUSE()
BEGIN
  RECT();
  // ściany
  LINE_P(20,100,20,200,#905000h);
  LINE_P(20,200,240,200,#905000h);
  LINE_P(240,200,240,100,#905000h);
  LINE_P(240,100,20,100,#905000h);
  // dach
  LINE_P(20,100,130,50,#800000h);
  LINE_P(130,50,240,100,#800000h);
  WAIT(0);
END;
```

Zaawansowana forma `LINE_P` rysuje wiele linii naraz z opcjonalną transformacją 3D. Omawia ją [rozdział 16](16-grafika-zaawansowana.md).

## 15.3. Prostokąty: RECT_P (*UG s. 597*)

```
RECT_P([G, x1, y1, x2, y2, kolor_krawędzi, kolor_wypełnienia]);
```

- `x1,y1` domyślnie oznacza lewy górny róg, a `x2,y2` prawy dolny róg GROB-a.
- Jeśli nie podasz koloru wypełnienia, będzie taki sam jak kolor krawędzi.
- **Ważna reguła (*UG s. 597*):** przy wielu argumentach opcjonalnych podane wartości trafiają do parametrów **od lewej**. Dlatego:

```
RECT_P(40,90,#000000h);                     // x1=40, y1=90, kolor krawędzi = czarny
                                            // → czarny prostokąt od (40,90) do prawego dolnego rogu
RECT_P(40,90,320,240,#000000h,#FF0000h);    // czarna ramka, czerwone wnętrze
```

Program BOX z instrukcji:

```
EXPORT BOX()
BEGIN
  RECT();
  RECT_P(40,90,320,240,#000000h,#FF0000h);
  WAIT;
END;
```

Aby narysować **samą ramkę** bez wypełnienia, narysuj cztery linie albo dwa prostokąty: większy w kolorze ramki, a w środku mniejszy w kolorze tła.

## 15.4. Wypełnione wielokąty: FILLPOLY_P (*ES #8, UG s. 594*)

```
FILLPOLY_P([G], {(x1,y1), (x2,y2), ..., (xn,yn)}, kolor [, alfa]);
```

- Punkty zapisuje się jako pary w nawiasach okrągłych. Kalkulator traktuje je jak liczby zespolone x+y·i. Można też podać wektor punktów.
- **Kolejność punktów ma znaczenie**: to kolejne wierzchołki obwodu.
- `alfa` od 0 do 255 ustawia przezroczystość.

Program DRAWPENT (*ES #8*) rysuje różowy pięciokąt na indygo:

```
EXPORT DRAWPENT()
BEGIN
  RECT(#400080h);                                   // tło indygo
  FILLPOLY_P({(80,100),(160,20),(240,100),
              (200,180),(120,180)}, #FFC0C0h);      // różowy pięciokąt
  WAIT(0);
END;
```

Przykład z instrukcji: kwadrat 80×80 w kolorze `#FF` (niebieskim) z przezroczystością 128:

```
FILLPOLY_P({(20,20),(100,20),(100,100),(20,100)}, #FFh, 128);
```

Wielokąt foremny o n bokach:

```
EXPORT NKAT(n, r)
BEGIN
  LOCAL k, pts := {}, a;
  HAngle := 0;                                  // radiany
  FOR k FROM 0 TO n-1 DO
    a := 2*π*k/n - π/2;
    pts := CONCAT(pts, {(160 + r*COS(a), 110 + r*SIN(a))});
  END;
  RECT();
  FILLPOLY_P(pts, #00A060h);
  FREEZE;
END;
```

## 15.5. Łuki, okręgi i elipsy: ARC_P (*ES #8, UG s. 593*)

**Promień jest zawsze w pikselach**, także w kartezjańskiej wersji `ARC` (*ES #8*).

| Kształt | Składnia |
|---|---|
| okrąg | `ARC_P(x, y, r, kolor)` |
| koło (wypełnione) | `ARC_P(x, y, r, {kolor_krawędzi, kolor_wypełnienia})` |
| elipsa | `ARC_P(x, y, {rx, ry}, kolor)` |
| elipsa wypełniona | `ARC_P(x, y, {rx, ry}, {kolor_krawędzi, kolor_wypełnienia})` |
| łuk od kąta a1 do a2 | `ARC_P(x, y, r, a1, a2, kolor)` |
| wycinek wypełniony | `ARC_P(x, y, r, a1, a2, {kolor_krawędzi, kolor_wypełnienia})` |

Kąty `a1` i `a2` są liczone w **bieżącym trybie kątowym** (`HAngle`). Łuk rysowany jest przeciwnie do ruchu wskazówek zegara, tak jak na okręgu jednostkowym.

Program DRAWARCS (*ES #8*):

```
EXPORT DRAWARCS()
BEGIN
  RECT();
  ARC_P(60,110,30,#008000h);            // okrąg, office green
  ARC_P(140,110,{30,50},#00A0C0h);      // elipsa, pacific blue
  HAngle := 1;                          // stopnie
  ARC_P(220,110,30,30,150,#400080h);    // łuk 30°–150°, indygo
  WAIT(0);
END;
```

Przykład z instrukcji: czerwony półokrąg we współrzędnych kartezjańskich.

```
ARC(0,0,60,0,π,RGB(255,0,0));   // środek (0,0), promień 60 px, od 0 do π (radiany)
```

## 15.6. Trójkąty: TRIANGLE_P (*UG s. 600*)

```
TRIANGLE_P([G], x1, y1, x2, y2, x3, y3, kolor [, alfa]);
TRIANGLE_P([G], x1, y1, x2, y2, x3, y3, k1, k2, k3 [, alfa]);   // gradient
TRIANGLE_P([G], {x1,y1,[k1],[z1]}, {x2,y2,[k2],[z2]}, {x3,y3,[k3],[z3]});
```

Po podaniu trzech kolorów wnętrze trójkąta zostaje wypełnione płynnym przejściem między kolorami wierzchołków:

```
EXPORT GRADIENT()
BEGIN
  RECT(#000000h);
  TRIANGLE_P(160,10, 20,210, 300,210, #FF0000h, #00FF00h, #0000FFh);
  FREEZE;
END;
```

Trójkąty to podstawa grafiki 3D na Prime (zaawansowane formy opisuje [rozdział 16](16-grafika-zaawansowana.md)).

## 15.7. Odwracanie kolorów: INVERT_P (*UG s. 595*)

```
INVERT_P([G, x1, y1, x2, y2]);
```

Odwraca kolory (negatyw) w prostokątnym obszarze. Przydaje się do podświetlania zaznaczonej pozycji menu albo do efektu „mignięcia”.

## 15.8. Wysokość i szerokość GROB-a

```
GROBW_P(G1)    // szerokość w pikselach
GROBH_P(G1)    // wysokość
```

## 15.9. Kompozycja: scena

```
EXPORT SCENA()
BEGIN
  RECT(#80D0FFh);                                      // niebo
  RECT_P(0,160,319,239,#00A060h,#00A060h);             // trawa
  ARC_P(270,40,22,{#FFD000h,#FFD000h});                // słońce
  RECT_P(60,100,160,170,#905000h,#D0B090h);            // dom
  FILLPOLY_P({(50,100),(110,55),(170,100)}, #800000h); // dach
  RECT_P(95,130,120,170,#402000h,#402000h);            // drzwi
  RECT_P(130,115,150,135,#FFFFFFh,#80D0FFh);           // okno
  TEXTOUT_P("Mój dom", 200, 190, 4, #FFFFFFh);
  FREEZE;
END;
```

---

## Sprawdź się

1. Narysuj tarczę strzelniczą: 5 koncentrycznych kół na przemian czerwonych i białych.
2. Narysuj wykres słupkowy z listy `{5,12,7,9,3}`: słupki w różnych kolorach, wysokość proporcjonalna do wartości, podpisy pod słupkami.
3. Narysuj zegar analogowy z aktualnym czasem (`Time`): tarcza (ARC_P) i dwie wskazówki (LINE_P).

[Rozwiązania →](25-cwiczenia.md#rozdział-15)
