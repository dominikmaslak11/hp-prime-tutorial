# Rozdział 14. Grafika — podstawy: ekran, współrzędne, kolory, tekst

[← Poprzedni](13-cas-i-analiza.md) · [Spis treści](../README.md) · [Następny →](15-grafika-rysowanie.md)

Źródła: *ES #7*, *UG s. 592–600*. Komendy graficzne są w **(Cmds) › Drawing**: 6. Pixels dla wersji pikselowych, 7. Cartesian dla kartezjańskich.

---

## 14.1. Zmienne graficzne G0–G9 (GROB)

- HP Prime ma 10 zmiennych graficznych (**GROB**, *graphic object*): `G0`–`G9` (*UG s. 592*).
- **`G0` to zawsze bieżący ekran.** Rysowanie w `G0` jest od razu widoczne.
- `G1`–`G9` to bufory w pamięci, niewidoczne, dopóki nie skopiujesz ich na ekran. Przydają się do animacji ([rozdział 16](16-grafika-zaawansowana.md)). Są czyszczone po wyłączeniu kalkulatora.
- Prawie każda komenda graficzna przyjmuje GROB jako **pierwszy, opcjonalny** argument. Domyślnie jest to `G0`.

## 14.2. Dwa układy współrzędnych (*ES #7*)

Każda komenda ma dwie wersje:
- **kartezjańską**, np. `LINE`, `RECT`, `TEXTOUT`,
- **pikselową** z przyrostkiem `_P`, np. `LINE_P`, `RECT_P`, `TEXTOUT_P`.

| | Kartezjański | Pikselowy (`_P`) |
|---|---|---|
| początek | zależy od `Xmin, Xmax, Ymin, Ymax` bieżącej aplikacji | (0,0) w **lewym górnym** rogu |
| oś x | w prawo | w prawo |
| oś y | **w górę** | **w dół** |
| zakres | dowolny, zależny od okna wykresu | ekran 320 × 240: x = 0…319, y = 0…239 |
| zastosowanie | wykresy matematyczne | interfejsy, gry, tekst |

- Pasek menu dotykowego zajmuje dolne piksele (y ≈ 220–239). Jeśli wyświetlasz menu, rysuj powyżej (*ES #7*).
- Niektóre parametry zawsze są w pikselach, nawet w wersji kartezjańskiej. Na przykład promień w `ARC` (*ES #7, UG s. 593*).
- Konwersja między układami: `C→PX(x,y)` zamienia współrzędne kartezjańskie na piksele, a `PX→C(x,y)` odwrotnie. Obie komendy przyjmują też listę `{x,y}`. W tekście instrukcji (*UG s. 592–593*) strzałka w nazwie zgubiła się podczas składu, stąd zapis *CPX* i *PXC*. Wstawiaj je z menu (Cmds) › Drawing.

W kursie używam głównie **współrzędnych pikselowych**, bo nie zależą od ustawień aplikacji.

## 14.3. Czyszczenie ekranu — RECT

```
RECT();              // wyczyść G0 na biało
RECT(kolor);         // wypełnij cały ekran kolorem
RECT(G1);            // wyczyść bufor G1
```

`RECT()` w grafice pełni tę samą rolę co `PRINT()` w terminalu. Wywołuj go na początku każdego programu graficznego (*ES #7*).

## 14.4. Utrzymanie obrazu na ekranie — FREEZE i WAIT

Po zakończeniu programu system przerysowuje ekran bieżącej aplikacji, więc Twój rysunek natychmiast zniknie. Trzeba go zatrzymać:

| Komenda | Działanie |
|---|---|
| `FREEZE;` | Zatrzymuje ekran do naciśnięcia klawisza lub dotknięcia; nie przerysowuje go po zakończeniu programu (*UG s. 593*). W tutorialu Shore'a: [Enter] uruchamia program ponownie, [Esc] lub dotknięcie wychodzi. |
| `WAIT(n);` | Pauza n sekund. |
| `WAIT(0);` / `WAIT;` | Według tutorialu Shore'a (fw 13441) czeka na naciśnięcie klawisza. Według instrukcji z 2016 r. czeka minutę. |
| `WAIT(-1);` | W nowszym firmware czeka na klawisz lub dotyk i zwraca informację o zdarzeniu. |

Zalecany wzorzec: kończ program pętlą oczekiwania na klawisz ([rozdział 17](17-klawiatura-dotyk-czas.md)) albo komendą `FREEZE`.

## 14.5. Kolory

### RGB (*UG s. 593, ES #5*)

```
RGB(czerwony, zielony, niebieski [, alfa])     // składowe 0..255
```

Zwraca liczbę całkowitą, którą przekazujesz do komend rysujących. Według instrukcji alfa > 128 oznacza kolor przezroczysty, a na Prime nie ma mieszania kanału alfa w `RGB`. Przezroczystość obsługują natomiast osobne parametry `alpha` w `FILLPOLY`, `BLIT` i `TRIANGLE`.

```
RGB(255,0,128)     // 16711808
RECT(RGB(0,0,255));          // niebieski ekran
LINE(0,0,8,8,RGB(0,255,0));  // zielona linia
```

### Zapis szesnastkowy

Kolor można podać bezpośrednio jako `#RRGGBBh` (*ES #5*):

```
#FF0000h   // czerwony
#00FF00h   // zielony
#0000FFh   // niebieski
#000000h   // czarny  (#0 też działa)
#FFFFFFh   // biały
```

### Paleta z tutorialu Shore'a (*ES #5*)

| Kolor | R/G/B | Kolor | R/G/B |
|---|---|---|---|
| White | 255/255/255 | Pacific Blue | 0/160/192 |
| Silver | 192/192/192 | Teal | 0/128/128 |
| Gray | 128/128/128 | Cyan/Aqua | 0/255/255 |
| Black | 0/0/0 | Fir Green | 0/48/0 |
| Red | 255/0/0 | Office Green | 0/128/0 |
| Magenta | 255/0/255 | Shamrock | 0/160/96 |
| Cardinal | 208/48/32 | Apple | 80/176/64 |
| Coral | 240/128/112 | Lime Green | 0/255/0 |
| Pink | 255/192/192 | Chocolate | 64/8/8 |
| Indigo | 64/0/128 | Brown | 144/80/0 |
| Purple | 128/0/128 | Orange | 255/128/0 |
| Violet | 128/0/255 | Olive | 128/128/0 |
| Lavender | 192/192/255 | Tangerine | 255/208/0 |
| Navy Blue | 0/0/128 | Yellow | 255/255/0 |
| Blue | 0/0/255 | Khaki | 240/224/144 |
| Denim | 16/96/192 | Tan | 208/176/144 |
| Lochmara | 0/112/192 | Maroon | 128/0/0 |
| Sky Blue | 128/208/255 | Snow White | 255/240/240 |

Wartości dziesiętne na szesnastkowe: 0 → 00, 16 → 10, 32 → 20, 48 → 30, 64 → 40, 80 → 50, 96 → 60, 112 → 70, 128 → 80, 144 → 90, 160 → A0, 176 → B0, 192 → C0, 208 → D0, 224 → E0, 240 → F0, 255 → FF. Na przykład Brown 144/80/0 to `#905000h`.

## 14.6. Tekst na ekranie — TEXTOUT i TEXTOUT_P

```
TEXTOUT_P(tekst [,G], x, y [, czcionka, kolor_tekstu, szerokość, kolor_tła]);
TEXTOUT  (tekst [,G], x, y [, czcionka, kolor_tekstu, szerokość, kolor_tła]);
```

| Argument | Znaczenie |
|---|---|
| `tekst` | tekst, liczba albo wyrażenie, np. `"x="+x` |
| `G` | GROB, domyślnie `G0` |
| `x, y` | lewy górny róg tekstu |
| `czcionka` | rozmiar; **trzeba go podać, jeśli chcesz ustawić kolor** (*ES #7*) |
| `kolor_tekstu` | domyślnie czarny |
| `szerokość` | maksymalna szerokość w pikselach; dłuższy tekst zostanie obcięty |
| `kolor_tła` | jeśli podany, tło pod tekstem zostanie zamalowane (przydatne przy odświeżaniu liczb) |

Rozmiary czcionki:

| Kod | Instrukcja (2016) | Tutorial Shore'a (fw 13441+) |
|---|---|---|
| 0 | czcionka z ustawień Home | czcionka z ustawień Home |
| 1 | mała | 10 pt |
| 2 | duża | 12 pt |
| 3 | | 14 pt |
| 4 | | 16 pt |
| 5 | | 18 pt |
| 6 | | 20 pt |
| 7 | | 22 pt |

`TEXTOUT_P` **zwraca współrzędną x końca tekstu** (*UG s. 599*). Dzięki temu można wypisywać kolejne fragmenty jeden za drugim:

```
LOCAL x := TEXTOUT_P("Wynik: ", 10, 50, 3);
TEXTOUT_P(42, x, 50, 3, #FF0000h);        // czerwone "42" zaraz za napisem
```

Uproszczone wywołania (*ES #7*):

```
TEXTOUT_P("Hello", 10, 10);                  // czarny, domyślny rozmiar
TEXTOUT_P("Hello", 10, 10, 4, RGB(0,0,255)); // niebieski, 16 pt
```

## 14.7. Program SNOWFLAKE (*ES #7*)

Program rysuje N „płatków śniegu” (gwiazdek) w losowych miejscach i w losowych odcieniach niebieskiego:

```
EXPORT SNOWFLAKE(N)
BEGIN
  LOCAL X,Y,Z,I,L0;
  // niebieski, jasnoniebieski, dodger blue, cyjan
  L0 := {RGB(0,0,255),RGB(178,255,255),RGB(30,144,255),RGB(0,255,255)};
  RECT();
  FOR I FROM 1 TO N DO
    X := RANDINT(0,304);        // zostaw margines na szerokość znaku
    Y := RANDINT(0,208);
    Z := RANDINT(1,4);
    Z := L0(Z);                 // wybierz kolor z listy
    TEXTOUT_P("*",X,Y,2,Z);
  END;
  FREEZE;
END;
```

Zwróć uwagę na kolejność: najpierw losujesz pozycję i kolor, potem rysujesz (*ES #7*).

## 14.8. Wzorzec programu graficznego

```
EXPORT SZABLON_GRAF()
BEGIN
  // 1. przygotowanie
  RECT();                                   // czysty ekran
  // 2. rysowanie
  TEXTOUT_P("Mój program", 90, 10, 5, #0070C0h);
  LINE_P(0, 40, 319, 40, #808080h);
  // 3. utrzymanie obrazu
  FREEZE;
END;
```

---

## Sprawdź się

1. Narysuj „flagę Polski” na całym ekranie: górna połowa biała, dolna czerwona. Użyj dwóch wywołań `RECT_P`.
2. Wypisz w środku ekranu aktualną datę (`Date`) dużą czcionką, na żółtym tle.
3. Zmodyfikuj SNOWFLAKE tak, żeby tło było granatowe, a płatki miały losowy rozmiar czcionki (1–7).

[Rozwiązania →](25-cwiczenia.md#rozdział-14)
