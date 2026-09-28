# Dodatek C. Tabele: STARTVIEW, TYPE, kolory, ustawienia systemowe

[← Spis treści](../README.md)

## STARTVIEW(n) (*UG s. 647–648*)

| n | Widok | | n | Widok |
|---|---|---|---|---|
| 0 | Symbolic | | -1 | Home |
| 1 | Plot | | -2 | ustawienia Home |
| 2 | Numeric | | -3 | Memory Manager |
| 3 | Symbolic Setup | | -4 | biblioteka aplikacji |
| 4 | Plot Setup | | -5 | katalog macierzy |
| 5 | Numeric Setup | | -6 | katalog list |
| 6 | App Info | | -7 | katalog programów |
| 7 | menu View | | -8 | katalog notatek |
| 8+ | widoki specjalne z menu View (kolejno) | | | |

## TYPE(obiekt) (*UG s. 658*)

| Kod | Typ |
|---|---|
| 0 | liczba rzeczywista |
| 1 | liczba całkowita (`#`) |
| 2 | tekst |
| 3 | liczba zespolona |
| 4 | macierz / wektor |
| 5 | błąd |
| 6 | lista |
| 8 | funkcja |
| 9 | liczba z jednostką |
| 14.x | obiekt CAS (x = typ CAS) |

## Tryby STRING (*UG s. 635*)

| Kod | Format | Kod | Format |
|---|---|---|---|
| 0 | bieżący | 4 | Engineering |
| 1 | Standard | 5 | Floating |
| 2 | Fixed | 6 | Rounded |
| 3 | Scientific | +7 / +14 | ułamek właściwy / mieszany |

## Czcionki TEXTOUT (*ES #7*)

| Kod | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Rozmiar | z ustawień | 10 | 12 | 14 | 16 | 18 | 20 | 22 |

## MOUSE — typ dotyku (*UG s. 653*)

| 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|
| nowy | zakończony | przeciąganie | rozciąganie | obrót | długie przytrzymanie |

## Ustawienia Home (*UG s. 691–694*)

| Zmienna | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| `HAngle` | radiany | stopnie | grady | |
| `HFormat` | Standard | Fixed | Scientific | Engineering |
| `HComplex` | wył. | wł. | | |
| `Entry` | Textbook | Algebraic | RPN | |
| `Base` | bin | okt | dec | hex |
| `Signed` | bez znaku | ze znakiem | | |

`Language`: 1 EN, 2 ZH, 3 FR, 4 DE, 5 ES, 6 NL, 7 PT. `HDigits`: liczba cyfr. `Bits`: 1–64. `TOff`: ms do autowyłączenia.

## Ustawienia aplikacji (Symbolic Setup) (*UG s. 695*)

| Zmienna | 0 | 1 | 2 | 3 | 4 |
|---|---|---|---|---|---|
| `AAngle` | systemowy | radiany | stopnie | grady | |
| `AComplex` | systemowy | wł. | wył. | | |
| `AFormat` | systemowy | Standard | Fixed | Scientific | Engineering |

## Przełączniki Plot (*UG s. 659–670*)

| Zmienna | Wartości |
|---|---|
| `Axes`, `GridDots`, `GridLines`, `Recenter` | według instrukcji 0 = wł., 1 = wył. |
| `Labels` | według instrukcji 1 = wł. |
| `Cursor` | 0 krzyż, 1 odwrócony, 2 migający |
| `Method` | 0 adaptacyjna, 1 odcinki, 2 punkty |
| `SeqPlot` | 0 schodki, 1 pajęczyna |

## Zmienne równań aplikacji

| Aplikacja | Równania | Zmienna niezależna |
|---|---|---|
| Function | F0–F9 | X |
| Advanced Graphing | V0–V9 | X, Y |
| Graph 3D | FZ0–FZ9 | X, Y |
| Parametric | X0–X9, Y0–Y9 | T |
| Polar | R0–R9 | θ |
| Sequence | U0–U9 | N |
| Solve | E0–E9 | dowolne |
| Statistics 1Var | H1–H5; dane D0–D9 | |
| Statistics 2Var | S1–S5; dane C0–C9 | |

## Kolory — najczęściej używane

| Nazwa | Hex | Nazwa | Hex |
|---|---|---|---|
| czarny | `#000000h` | biały | `#FFFFFFh` |
| czerwony | `#FF0000h` | zielony | `#00FF00h` |
| niebieski | `#0000FFh` | żółty | `#FFFF00h` |
| cyjan | `#00FFFFh` | magenta | `#FF00FFh` |
| szary | `#808080h` | srebrny | `#C0C0C0h` |
| pomarańczowy | `#FF8000h` | fioletowy | `#800080h` |
| granatowy | `#000080h` | bordowy | `#800000h` |
| brązowy | `#905000h` | różowy | `#FFC0C0h` |
| indygo | `#400080h` | office green | `#008000h` |
| pacific blue | `#00A0C0h` | teal | `#008080h` |

Pełna paleta z tutorialu Shore'a jest w [rozdziale 14.5](../rozdzialy/14-grafika-podstawy.md#paleta-z-tutorialu-shorea-es-5).
