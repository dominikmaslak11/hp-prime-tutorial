# Rozdział 16. Grafika zaawansowana: bufory GROB, BLIT, animacja, menu, 3D

[← Poprzedni](15-grafika-rysowanie.md) · [Spis treści](../README.md) · [Następny →](17-klawiatura-dotyk-czas.md)

Źródła: *UG s. 592–601*.

---

## 16.1. Tworzenie bufora: DIMGROB_P (*UG s. 594*)

```
DIMGROB_P(G, szerokość, wysokość [, kolor]);
DIMGROB_P(G, lista_danych);
```

- Ustawia rozmiar GROB-a `G` (`G1`–`G9`) i wypełnia go kolorem.
- Druga forma wypełnia obraz danymi z listy liczb całkowitych. Każda liczba, czytana w systemie szesnastkowym, koduje piksele po 16 bitów w formacie **A1R5G5B5** (1 bit alfa, po 5 bitów R, G, B). Tak można zapisać w programie małe obrazki (sprite'y).

```
DIMGROB_P(G1, 320, 240, #FFFFFFh);     // bufor wielkości ekranu, biały
DIMGROB_P(G2, 16, 16, #FF0000h);       // czerwony kwadrat 16x16 (np. sprite)
```

## 16.2. Kopiowanie obrazów: BLIT_P (*UG s. 594*)

```
BLIT_P([cel, dx1, dy1, dx2, dy2], źródło [, sx1, sy1, sx2, sy2, kolor_pomijany, alfa]);
```

- Kopiuje prostokąt ze **źródła** do **celu**. Cel domyślnie to `G0`.
- Obszar źródła obejmuje punkt `(sx1,sy1)`, ale **nie** obejmuje `(sx2,sy2)`.
- Jeśli nie podasz `dx2,dy2`, obszar docelowy ma rozmiar źródła, czyli nie ma skalowania. Jeśli je podasz, obraz zostanie **przeskalowany** do tego prostokąta.
- `kolor_pomijany`: piksele w tym kolorze nie są kopiowane. Tak robi się sprite'y z przezroczystym tłem.
- `alfa` od 0 (przezroczysty) do 255 (nieprzezroczysty) to przezroczystość całego źródła.
- Jeśli używasz jednocześnie `kolor_pomijany` i `alfa`, podaj też współrzędne źródła. Kalkulator odróżni wtedy argumenty (*UG s. 594*).
- Nie używaj tego samego GROB-a jako źródła i celu, gdy obszary na siebie nachodzą.

```
BLIT_P(G1);                          // cały G1 na ekran (G0), od (0,0)
BLIT_P(G0, 100, 50, G2);             // G2 wklejony w punkt (100,50)
BLIT_P(G0, 0, 0, 320, 240, G2);      // G2 rozciągnięty na cały ekran
BLIT_P(G0, x, y, G2, 0, 0, 16, 16, #FFFFFFh);   // sprite bez białego tła
```

## 16.3. Wycinanie fragmentu: SUBGROB_P (*UG s. 598*)

```
SUBGROB_P(źródło [, x1, y1, x2, y2], cel);
```

Kopiuje fragment źródła do GROB-a `cel`, który **nie może być `G0`**. Przykład: `SUBGROB(G1, G4)` kopiuje cały `G1` do `G4`. Zastosowanie: zapamiętanie tła pod obiektem, żeby móc je potem odtworzyć.

## 16.4. Animacja bez migotania — podwójne buforowanie

Jeśli czyścisz i rysujesz bezpośrednio w `G0`, obraz migocze, bo użytkownik widzi każdy etap rysowania. Rozwiązanie: **rysuj całą klatkę w buforze `G1`, a potem jednym `BLIT_P` przenieś ją na ekran.**

Piłka odbijająca się od ścian (kończy się dowolnym klawiszem):

```
EXPORT PILKA()
BEGIN
  LOCAL x:=40, y:=40, vx:=3, vy:=2, r:=10, k;
  DIMGROB_P(G1, 320, 240);
  REPEAT
    // --- rysowanie klatki w buforze G1 ---
    RECT_P(G1, 0, 0, 319, 239, #102040h, #102040h);         // tło
    ARC_P(G1, x, y, r, {#FFFFFFh, #FF8000h});                // piłka
    TEXTOUT_P("Dowolny klawisz = koniec", G1, 60, 222, 1, #C0C0C0h);
    BLIT_P(G0, G1);                                          // klatka na ekran

    // --- fizyka ---
    x := x+vx;  y := y+vy;
    IF x-r<0 OR x+r>319 THEN vx := -vx; END;
    IF y-r<0 OR y+r>219 THEN vy := -vy; END;

    WAIT(0.02);                                              // ~50 klatek/s
    k := GETKEY;
  UNTIL k <> -1;
END;
```

Schemat każdej animacji:
1. wyczyść bufor,
2. narysuj w nim wszystkie obiekty,
3. skopiuj bufor na ekran (`BLIT_P`),
4. zaktualizuj stan (pozycje, prędkości),
5. obsłuż wejście (klawisze, dotyk),
6. poczekaj chwilę i wróć do punktu 1.

Tempo animacji można uzależnić od czasu (`TICKS`, [rozdział 17](17-klawiatura-dotyk-czas.md)), a nie od `WAIT`. Wtedy prędkość gry nie zależy od tego, jak długo trwa rysowanie klatki.

## 16.5. Menu dotykowe: DRAWMENU (*UG s. 593*)

```
DRAWMENU("etykieta1", "etykieta2", ..., "etykieta6");
DRAWMENU({"etykieta1", ..., "etykieta6"});
```

Rysuje na dole ekranu pasek sześciu przycisków w stylu systemowym. Puste teksty `""` dają puste przyciski. Przykład z instrukcji: `DRAWMENU("ABC", "", "DEF")`.

`DRAWMENU` tylko **rysuje** przyciski. Dotknięcia trzeba obsłużyć samodzielnie komendą `MOUSE` ([rozdział 17](17-klawiatura-dotyk-czas.md)). Każdy przycisk ma szerokość około 53 pikseli (320/6), a pasek zajmuje y ≈ 220–239. Numer dotkniętego przycisku to `IP(x/53.33)+1`.

## 16.6. Grafika 3D: zaawansowane LINE_P i TRIANGLE_P (*UG s. 595–601*)

Zaawansowane formy komend rysują **wiele** linii lub trójkątów w jednym wywołaniu, z obrotem, przesunięciem i rzutem perspektywicznym:

```
LINE_P([G], punkty, linie, macierz_obrotu
       | {macierz_obrotu lub -1, ["N"], [{oko_x, oko_y, oko_z} lub -1],
          [{xmin, xmax, ymin, ymax, zmin, zmax}]}, [zstring]);

TRIANGLE_P([G], punkty, trójkąty, (to samo co wyżej), [zstring]);
```

| Element | Opis |
|---|---|
| `punkty` | lista lub macierz punktów; każdy punkt ma postać np. `{x,y,z,kolor}` lub `[x,y,z,kolor]` |
| `linie` | lista `{p1, p2, [kolor], [alfa]}`, gdzie p1 i p2 to **indeksy** punktów; jeśli podajesz alfę bez koloru, jako kolor wpisz -1; zapis `{kolor, [alfa], linia1, linia2, ...}` nadaje jeden kolor wszystkim liniom |
| `trójkąty` | analogicznie `{p1, p2, p3, [kolor], [alfa]}` |
| `macierz_obrotu` | macierz od 2×2 do 3×4 (obrót i przesunięcie) |
| `{oko_x, oko_y, oko_z}` | pozycja obserwatora do rzutu perspektywicznego: x' = oko_z/z·x − oko_x, y' = oko_z/z·y − oko_y |
| `{xmin…zmax}` | obcinanie 3D |
| `"N"` | normalizuje z do przedziału 0–255 po obrocie |
| `zstring` | bufor głębokości (z-buffer); pusty bufor tworzy `TRIANGLE_P([G])` |

Obie komendy zwracają tekst z przekształconymi punktami. Można go przekazać zamiast listy punktów w kolejnych wywołaniach, bez ponownej transformacji. Przyspiesza to rysowanie tej samej bryły liniami i trójkątami.

Przykład: obracający się sześcian (szkielet).

```
EXPORT KOSTKA3D()
BEGIN
  LOCAL pts, lin, a:=0, c, s, R;
  pts := {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
          {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
  lin := {#FFFFFFh, {1,2},{2,3},{3,4},{4,1},
                    {5,6},{6,7},{7,8},{8,5},
                    {1,5},{2,6},{3,7},{4,8}};
  HAngle := 0;
  DIMGROB_P(G1,320,240);
  REPEAT
    c := COS(a); s := SIN(a);
    // obrót wokół osi Y i X; przesunięcie o 6 wzdłuż z (od obserwatora)
    R := [[c,0,s,0],[s*s,c,-c*s,0],[-c*s,s,c*c,6]];
    RECT_P(G1,0,0,319,239,#000030h,#000030h);
    LINE_P(G1, pts, lin, {R, -1, {-160,-120,300}});
    BLIT_P(G0,G1);
    a := a+0.05;
  UNTIL GETKEY <> -1;
END;
```

> Parametry kamery (`{-160,-120,300}`) dobierz doświadczalnie: pierwsze dwie liczby przesuwają obraz na środek ekranu, a trzecia działa jak ogniskowa. Jeśli na Twoim firmware obraz jest przesunięty albo odwrócony, zmień znaki tych wartości.

## 16.7. Grafika w plikach aplikacji

Aplikacja może mieć dołączone pliki (`AFiles`, [rozdział 19](19-wlasne-aplikacje.md)), np. obrazy PNG. Po wczytaniu obrazu do GROB-a możesz rysować go przez `BLIT_P`. Plik `icon.png` dołączony do aplikacji staje się jej ikoną w bibliotece aplikacji (*UG s. 614*).

---

## Sprawdź się

1. Rozbuduj PILKA o pięć piłek naraz. Pozycje i prędkości trzymaj w listach.
2. Narysuj w `G2` sprite 16×16 (np. buźkę) i przesuwaj go po ekranie z przezroczystym tłem (`kolor_pomijany`).
3. Zrób menu `DRAWMENU("Koło","Kwadrat","","","","Wyjdź")`, które rysuje figurę w zależności od dotkniętego przycisku (użyj `MOUSE` z rozdziału 17).

[Rozwiązania →](25-cwiczenia.md#rozdział-16)
