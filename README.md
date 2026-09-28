# Programowanie HP Prime — kompletny kurs HP PPL po polsku

Obszerny, praktyczny kurs programowania kalkulatora graficznego **HP Prime** w jego
natywnym języku **HP PPL** (HP Prime Programming Language). Kurs prowadzi od
pierwszego programu, przez wszystkie konstrukcje języka, grafikę, obsługę
klawiatury i ekranu dotykowego, aż po tworzenie własnych aplikacji (apps)
i przeprogramowywanie klawiszy.

Kurs powstał na podstawie dwóch dokumentów:

| Źródło | Co zawiera |
|---|---|
| **HP Prime Graphing Calculator — User Guide**, wyd. 2 (09.2016), nr 813269-002 (plik `c05332710.pdf`) | Oficjalna instrukcja HP, 31 rozdziałów. Rozdział 28 „Programming in HP PPL” to pełny opis języka i komend. |
| **HP Prime Programming Tutorial**, Edward Shore, rev. 2 (2018, firmware 13441) (plik `hpprime-prog-tutorial.pdf`) | 9 lekcji z przykładowymi programami (LOCAL, RETURN, pętle, INPUT, CHOOSE, CASE, aplikacje, grafika, analiza). |

Wszystkie przykłady z obu źródeł zostały przepisane, skomentowane po polsku
i uzupełnione o własne programy, ćwiczenia i projekty. Przy każdym temacie
podaję numer strony instrukcji HP (np. *UG s. 583*), żeby łatwo było sprawdzić
oryginał.

---

## Spis treści

### Część I — Start
1. [Środowisko: kalkulator oczami programisty, katalog programów, edytor, pierwszy program](rozdzialy/01-srodowisko.md)
2. [Składnia języka i zmienne (LOCAL, EXPORT, zmienne systemowe)](rozdzialy/02-skladnia-i-zmienne.md)
3. [Typy danych i operatory](rozdzialy/03-typy-danych-i-operatory.md)
4. [Wyświetlanie wyników: RETURN, MSGBOX, PRINT](rozdzialy/04-wyjscie.md)

### Część II — Sterowanie przebiegiem programu
5. [Instrukcje warunkowe: IF, when, CASE, IFERR](rozdzialy/05-warunki.md)
6. [Pętle: FOR, WHILE, REPEAT, BREAK, CONTINUE](rozdzialy/06-petle.md)
7. [Pobieranie danych: INPUT, CHOOSE, EDITLIST, EDITMAT](rozdzialy/07-wejscie.md)
8. [Funkcje, podprogramy i rekurencja](rozdzialy/08-funkcje.md)

### Część III — Dane
9. [Łańcuchy znaków](rozdzialy/09-lancuchy.md)
10. [Listy](rozdzialy/10-listy.md)
11. [Macierze i wektory](rozdzialy/11-macierze.md)
12. [Liczby całkowite, systemy liczbowe i operacje bitowe](rozdzialy/12-liczby-calkowite.md)
13. [CAS i analiza matematyczna w programach](rozdzialy/13-cas-i-analiza.md)

### Część IV — Grafika i interakcja
14. [Grafika — podstawy: ekran, współrzędne, kolory, tekst](rozdzialy/14-grafika-podstawy.md)
15. [Grafika — rysowanie: linie, prostokąty, wielokąty, łuki, trójkąty](rozdzialy/15-grafika-rysowanie.md)
16. [Grafika zaawansowana: bufory GROB, BLIT, animacja, menu](rozdzialy/16-grafika-zaawansowana.md)
17. [Klawiatura, dotyk i czas: GETKEY, ISKEYDOWN, MOUSE, TICKS](rozdzialy/17-klawiatura-dotyk-czas.md)

### Część V — Kalkulator jako platforma
18. [Sterowanie aplikacjami HP: STARTAPP, STARTVIEW, zmienne aplikacji](rozdzialy/18-sterowanie-aplikacjami.md)
19. [Tworzenie własnych aplikacji (app programs)](rozdzialy/19-wlasne-aplikacje.md)
20. [Klawiatura użytkownika — przeprogramowanie klawiszy](rozdzialy/20-klawiatura-uzytkownika.md)
21. [Zmienne systemowe, notatki i programy jako dane](rozdzialy/21-zmienne-systemowe.md)
22. [Debugowanie, obsługa błędów i dobre praktyki](rozdzialy/22-debugowanie.md)

### Część VI — Całość instrukcji i praktyka
23. [Mapa całej instrukcji HP z perspektywy programisty (rozdziały 1–31)](rozdzialy/23-mapa-instrukcji.md)
24. [Projekty końcowe](rozdzialy/24-projekty.md)
25. [Ćwiczenia z rozwiązaniami](rozdzialy/25-cwiczenia.md)

### Dodatki
- [A. Ściąga: wszystkie komendy PPL w jednym miejscu](dodatki/A-sciaga.md)
- [B. Kody klawiszy (GETKEY) i nazwy klawiszy (KEY)](dodatki/B-kody-klawiszy.md)
- [C. Tabele: STARTVIEW, TYPE, kolory, ustawienia systemowe](dodatki/C-tabele.md)
- [Katalog `programy/`](programy/) — kody źródłowe gotowe do wklejenia w HP Connectivity Kit

---

## Konwencje używane w kursie

| Zapis | Znaczenie | Przykład |
|---|---|---|
| **[Klawisz]** | fizyczny klawisz kalkulatora | [Shift] [1] (Program) |
| **(Przycisk)** | przycisk menu dotykowego na dole ekranu | (New), (Check), (Tmplt) |
| `KOD` | kod programu lub komenda | `RETURN X^2;` |
| *UG s. N* | strona w instrukcji HP (User Guide) | *UG s. 583* |
| *ES #N* | lekcja w tutorialu Edwarda Shore'a | *ES #3* |
| `[arg]` w składni | argument opcjonalny | `PIXON([G], x, y [,kolor])` |

Kod w kursie zapisuję w formie **ASCII**, którą da się wpisać zarówno na
kalkulatorze, jak i w programie *HP Connectivity Kit* na PC:

| Na kalkulatorze | W kursie (ASCII) | Znaczenie |
|---|---|---|
| `▶` (klawisz [Shift][EEX], Sto▶) | `:=` | przypisanie (kolejność odwrotna: `5▶A` to to samo co `A:=5`); na kalkulatorze można też pisać `:=` |
| `≠` | `<>` | różne |
| `≤` `≥` | `<=` `>=` | mniejsze/większe lub równe |
| `π` | `π` lub `PI` | liczba pi |
| `θ` | `θ` | zmienna kątowa (Polar) |

## Jak korzystać z kursu

1. Czytaj rozdziały po kolei — każdy opiera się na poprzednich.
2. **Każdy przykład wpisz samodzielnie na kalkulatorze** (albo w emulatorze *HP Prime Virtual Calculator*). Programowania uczy się palcami, nie oczami.
3. Po wpisaniu programu zawsze naciśnij **(Check)** w edytorze.
4. Na końcu każdego rozdziału są zadania „Sprawdź się”. Pełne rozwiązania znajdziesz w [rozdziale 25](rozdzialy/25-cwiczenia.md).

> **Wersja oprogramowania.** Instrukcja HP opisuje firmware z 2016 r., tutorial Shore'a firmware 13441 (2018). Nowsze wersje (np. 14181, 14596, 14730) dodają kilka funkcji, np. `WAIT(-1)` czy więcej rozmiarów czcionki w `TEXTOUT`. W takich miejscach piszę wprost, co pochodzi z instrukcji, a co z nowszego firmware.

## Źródła i prawa

- *HP Prime Graphing Calculator User Guide* © 2015–2016 HP Development Company, L.P.
- *HP Prime Programming Tutorial* © 2013, 2018 Edward Shore (blog: https://edspi31415.blogspot.com/). Autor zezwala na niekomercyjne rozpowszechnianie z podaniem autorstwa. Programy z tego tutorialu są oznaczone jako *ES*.
- Tekst tego kursu i programy własne: do nauki i użytku niekomercyjnego.
