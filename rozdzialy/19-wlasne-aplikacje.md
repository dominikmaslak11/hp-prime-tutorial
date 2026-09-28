# Rozdział 19. Tworzenie własnych aplikacji (app programs)

[← Poprzedni](18-sterowanie-aplikacjami.md) · [Spis treści](../README.md) · [Następny →](20-klawiatura-uzytkownika.md)

Aplikacja HP Prime to zestaw widoków, programu, notatki i danych (*UG s. 576*). Możesz utworzyć własną aplikację na bazie aplikacji HP i przeprogramować jej zachowanie. Źródło: *UG s. 576–582, 614–615*, *UG rozdz. 6 „Creating an app”*.

---

## 19.1. Program aplikacji

- Każda aplikacja ma **jeden** związany z nią program. Gdy aplikacja jest aktywna, jej program jest **pierwszą pozycją** w katalogu programów (*UG s. 577*).
- Nowa aplikacja **dziedziczy** wszystkie cechy aplikacji bazowej: widoki, zmienne i funkcje.
- Program aplikacji wysyła się razem z aplikacją.

## 19.2. Funkcje o specjalnych nazwach (*UG s. 576*)

Jeśli program aplikacji zawiera funkcję o jednej z tych nazw, zostanie ona wywołana **zamiast** standardowej akcji klawisza:

| Nazwa funkcji | Kiedy jest wywoływana |
|---|---|
| `Symb` | [Symb] (Symbolic view) |
| `SymbSetup` | [Shift][Symb] |
| `Plot` | [Plot] |
| `PlotSetup` | [Shift][Plot] |
| `Num` | [Num] |
| `NumSetup` | [Shift][Num] |
| `Info` | [Shift][Apps] (Info) |
| `START` | przy uruchomieniu aplikacji z biblioteki lub przez `STARTAPP` |
| `RESET` | przy resecie aplikacji |

## 19.3. VIEW — własne pozycje menu View (*UG s. 577, 589, 604*)

```
VIEW "Tekst w menu", NazwaFunkcji()
BEGIN
  ...
END;
```

Linia `VIEW` przed definicją funkcji dodaje pozycję do menu [View] aplikacji. Jeśli program ma choć jedną taką pozycję, standardowa lista widoków zostaje **zastąpiona** Twoją. Funkcje wywoływane z menu View **nie przyjmują parametrów**. Dane przekazuje się przez zmienne eksportowane (*UG s. 581*).

## 19.4. Procedura tworzenia aplikacji (*UG s. 577*)

1. Wybierz aplikację HP, którą chcesz dostosować.
2. W bibliotece aplikacji ([Apps]) zaznacz ją, naciśnij **(Save)** i nadaj nową nazwę.
3. Skonfiguruj nową aplikację (osie, tryb kątów…).
4. Otwórz katalog programów, wybierz program nowej aplikacji i naciśnij (Edit).
5. Napisz funkcje, stosując nazwy specjalne z sekcji 19.2.
6. Dodaj `VIEW`, żeby zmienić menu View.
7. Nowe zmienne globalne **eksportuj z osobnego programu użytkownika**, wywoływanego z funkcji `START` aplikacji. Dzięki temu nie stracą wartości (*UG s. 577*).
8. Przetestuj i uruchom w debugerze.

Aplikacje można łączyć: program jednej aplikacji może uruchamiać drugą (`STARTAPP`) i z niej wracać (*UG s. 577*).

## 19.5. Przykład z instrukcji: DiceSimulation (*UG s. 577–582*)

Aplikacja na bazie **Statistics 1Var** symuluje rzuty dwiema kostkami o zadanej liczbie ścian i pokazuje histogram sum.

### Krok 1: utworzenie aplikacji

1. [Apps], zaznacz **Statistics 1Var** (nie otwieraj).
2. (Save), nazwa `DiceSimulation`, (OK) dwa razy.
3. Otwórz aplikację i w [Shift][Apps] (Info) wpisz notatkę z instrukcją obsługi. Zostanie wyświetlona przez `STARTVIEW(6)`.

### Krok 2: program pomocniczy DICESIMVARS

Osobny program (w katalogu programów: (New), nazwa `DICESIMVARS`):

```
EXPORT ROLLS, SIDES;

EXPORT DICESIMVARS()
BEGIN
  ROLLS := 10;
  SIDES := 6;
END;
```

Potrzebny jest też program `ROLLDIE` z [rozdziału 8](08-funkcje.md#krok-1-osobny-program-rolldie).

### Krok 3: program aplikacji DiceSimulation

```
DICESIMVARS();
ROLLDIE();

EXPORT SIDES, ROLLS;

EXPORT DiceSimulation()
BEGIN
END;

VIEW "Start", START()
BEGIN
  D1 := {};
  D2 := {};
  SetSample(H1, D1);
  SetFreq(H1, D2);
  H1Type := 1;              // typ wykresu H1: histogram
  STARTVIEW(6,1);           // pokaż notatkę (Info)
END;

VIEW "Roll Dice", ROLLMANY()
BEGIN
  LOCAL k, roll;
  D1 := MAKELIST(X+1, X, 1, 2*SIDES-1, 1);   // możliwe sumy 2..2*SIDES
  D2 := MAKELIST(0,   X, 1, 2*SIDES-1, 1);   // częstości
  FOR k FROM 1 TO ROLLS DO
    roll := ROLLDIE(SIDES) + ROLLDIE(SIDES);
    D2(roll-1) := D2(roll-1) + 1;
  END;
  Xmin := -0.1;
  Xmax := MAX(D1)+1;
  Ymin := -0.1;
  Ymax := MAX(D2)+1;
  STARTVIEW(1,1);
END;

VIEW "Set Sides", SETSIDES()
BEGIN
  REPEAT
    INPUT(SIDES, "Die Sides", "N=", "Enter# of sides", 2);
    SIDES := FLOOR(SIDES);
    IF SIDES<4 THEN MSGBOX("# of sides must be >= 4"); END;
  UNTIL SIDES >= 4;
  STARTVIEW(7,1);           // wróć do menu View
END;

VIEW "Set Rolls", SETROLLS()
BEGIN
  REPEAT
    INPUT(ROLLS, "Num of rolls", "N=", "Enter# of rolls", 25);
    ROLLS := FLOOR(ROLLS);
    IF ROLLS<1 THEN MSGBOX("You must enter a num >=1"); END;
  UNTIL ROLLS >= 1;
  STARTVIEW(7,1);
END;

PLOT()
BEGIN
  Xmin := -0.1;
  Xmax := MAX(D1)+1;
  Ymin := -0.1;
  Ymax := MAX(D2)+1;
  STARTVIEW(1,1);
END;

Symb()
BEGIN
  SetSample(H1, D1);
  SetFreq(H1, D2);
  H1Type := 1;
  STARTVIEW(0,1);
END;
```

> W instrukcji warunek w SETSIDES to `IF SIDES<2`, a komunikat mówi „>= 4”. W wersji powyżej warunek zgadza się z komunikatem.

### Krok 4: użytkowanie

1. [Apps] › DiceSimulation. Wyświetli się notatka z instrukcją.
2. [View]: pozycje Start, Roll Dice, Set Sides, Set Rolls.
3. Set Rolls: 100. Set Sides: 6. Roll Dice: histogram sum.
4. [Num] pokazuje dane, [Plot] wraca do histogramu.

### Czego uczy ten przykład

- `VIEW` buduje menu aplikacji.
- `START` inicjuje dane.
- Zmienne eksportowane przekazują dane między widokami.
- Przedefiniowane `Plot` i `Symb` zmieniają zachowanie klawiszy.
- Funkcje Statistics 1Var (`SetSample`, `SetFreq`, `H1Type`) sterują wykresem.

## 19.6. Zmienne dostępu do aplikacji (*UG s. 614–615*)

| Zmienna | Działanie |
|---|---|
| `AFiles` | lista plików aplikacji; `AFiles("nazwa")` odczytuje plik, `AFiles("nazwa") := obiekt` zapisuje |
| `AFilesB` | pliki binarne: `AFilesB("nazwa")` zwraca rozmiar, `AFilesB("nazwa", poz, n)` czyta n bajtów, `AFilesB("nazwa", poz) := wartość lub lista` zapisuje |
| `DelAFiles("nazwa")` | usuwa plik aplikacji |
| `ANote` | notatka aplikacji (odczyt i zapis: `ANote := "tekst"`) |
| `AProgram` | kod źródłowy programu aplikacji jako tekst (odczyt i zapis) |
| `AVars` | lista zmiennych aplikacji; `AVars(n)` lub `AVars("nazwa")` odczytuje, `AVars("nazwa") := wartość` zapisuje lub tworzy zmienną |
| `DelAVars(n lub "nazwa")` | usuwa zmienną aplikacji |

`AVars` i `AFiles` pozwalają aplikacji **trwale przechowywać dane**, np. najlepsze wyniki gry albo ustawienia. Dane są zapisane w aplikacji i wysyłane razem z nią.

```
// zapis rekordu gry w aplikacji
AVars("Rekord") := 1234;
// po utworzeniu zmiennej można ją czytać po nazwie
IF wynik > Rekord THEN Rekord := wynik; END;
```

Plik `icon.png` dołączony do aplikacji staje się jej ikoną w bibliotece aplikacji (*UG s. 614*).

## 19.7. Szablon własnej aplikacji

```
// ===== program aplikacji "MojaApka" (np. na bazie Function) =====
EXPORT MojaApka()
BEGIN
END;

START()
BEGIN
  // inicjalizacja: ustawienia, dane domyślne
  AAngle := 2;                 // stopnie w tej aplikacji
  STARTVIEW(7,1);              // otwórz menu View
END;

VIEW "Oblicz", OBLICZ()
BEGIN
  LOCAL a;
  IF INPUT(a, "Dane", "a =") THEN
    MSGBOX("Wynik: " + a^2);
  END;
  STARTVIEW(7,1);
END;

VIEW "Rysuj", RYSUJ()
BEGIN
  F1 := "SIN(X)";
  CHECK(1);
  STARTVIEW(1,1);
END;

VIEW "O programie", OPIS()
BEGIN
  MSGBOX("MojaApka v1.0");
  STARTVIEW(7,1);
END;

Plot()
BEGIN
  // [Plot] najpierw ustawia okno, potem pokazuje wykres
  Xmin := -360; Xmax := 360; Ymin := -1.5; Ymax := 1.5;
  STARTVIEW(1,1);
END;
```

---

## Sprawdź się

1. Utwórz aplikację „Kalkulator Ohma” (na bazie Solve) z pozycjami View: „Oblicz U”, „Oblicz I”, „Oblicz R”.
2. Rozbuduj DiceSimulation o pozycję View „Statystyki”, która po symulacji pokazuje średnią sumę oczek (`Do1VStats`).
3. Zapisz w `AVars` licznik uruchomień aplikacji i wyświetlaj go w `START`.

[Rozwiązania →](25-cwiczenia.md#rozdział-19)
