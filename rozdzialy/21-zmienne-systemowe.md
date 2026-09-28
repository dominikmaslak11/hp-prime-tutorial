# Rozdział 21. Zmienne systemowe, notatki i programy jako dane

[← Poprzedni](20-klawiatura-uzytkownika.md) · [Spis treści](../README.md) · [Następny →](22-debugowanie.md)

Źródła: *UG s. 658–695* i rozdział 23 „Variables” (*UG s. 510–533*).

---

## 21.1. Menu Vars (*UG rozdz. 24*)

Klawisz [Vars] otwiera cztery kategorie zmiennych:

| Przycisk | Zawartość |
|---|---|
| (Home) | zmienne Home: Real (A–Z), Complex (Z0–Z9), List (L0–L9), Matrix (M0–M9), Graphics (G0–G9), Settings (HAngle…), System |
| (CAS) | zmienne CAS |
| (App) | zmienne aplikacji, pogrupowane według aplikacji i widoków (Symbolic, Plot, Numeric, Results…) |
| (User) | Twoje zmienne i zmienne eksportowane przez programy (pogrupowane według programów) |

W programie wygodnie jest wstawiać nazwy zmiennych z [Vars], bo unikasz literówek. W przełączniku u dołu menu możesz wybrać, czy wstawić **nazwę** zmiennej, czy jej **wartość**.

## 21.2. Zmienne ustawień Home (*UG s. 691–694*)

| Zmienna | Znaczenie | Wartości |
|---|---|---|
| `Ans` | ostatni wynik; `Ans(n)` to n-ty wynik z historii | |
| `HAngle` | tryb kątów | 0 rad, 1 stopnie, 2 grady |
| `HFormat` | format liczb | 0 Standard, 1 Fixed, 2 Sci, 3 Eng |
| `HDigits` | liczba cyfr | 0 < n < 11 |
| `HComplex` | wyniki zespolone | 0 wył., 1 wł. |
| `Date` | data | RRRR.MMDD (zapis też ustawia datę) |
| `Time` | czas | format sześćdziesiątkowy |
| `Language` | język | 1 angielski, 2 chiński, 3 francuski, 4 niemiecki, 5 hiszpański, 6 holenderski, 7 portugalski |
| `Entry` | tryb wprowadzania | 0 Textbook, 1 Algebraic, 2 RPN |
| `Base` | system liczb całkowitych | 0 bin, 1 okt, 2 dec, 3 hex |
| `Bits` | rozmiar słowa | 1–64 |
| `Signed` | liczby ze znakiem | 0 nie, 1 tak |
| `TOff` | czas do autowyłączenia (ms) | domyślnie 300000 (`#493E0h`); zakres `#1388h`–`#3FFFFFFFh` |

Przykład: program, który zapisuje wszystkie ustawienia i je przywraca.

```
zapis;

EXPORT ZAPISZ_UST()
BEGIN
  zapis := {HAngle, HFormat, HDigits, HComplex};
END;

EXPORT PRZYWROC_UST()
BEGIN
  HAngle   := zapis(1);
  HFormat  := zapis(2);
  HDigits  := zapis(3);
  HComplex := zapis(4);
END;
```

Pomysł: przedłuż czas autowyłączenia na czas długich obliczeń (`TOff := 1800000;`, czyli 30 min), a na koniec przywróć starą wartość.

## 21.3. HVars i DelHVars — zmienne użytkownika Home (*UG s. 693–694*)

| Zapis | Działanie |
|---|---|
| `HVars` | lista nazw wszystkich zmiennych użytkownika w Home |
| `HVars(n)` / `HVars("nazwa")` | wartość zmiennej |
| `HVars("nazwa") := wartość` | zapis; **tworzy zmienną**, jeśli nie istnieje |
| `HVars(n lub "nazwa", 2)` | lista nazw parametrów, jeśli zmienna jest funkcją użytkownika (w przeciwnym razie 0) |
| `HVars("nazwa", 2) := {"p1", …}` | ustawia nazwy parametrów funkcji |
| `DelHVars(n lub "nazwa")` | usuwa zmienną |

`HVars` pozwala programowi tworzyć zmienne o nazwach znanych dopiero w czasie działania:

```
EXPORT UTWORZ_ZMIENNE(n)
BEGIN
  LOCAL i;
  FOR i FROM 1 TO n DO
    HVars("wynik"+i) := i^2;     // tworzy wynik1, wynik2, ...
  END;
  RETURN HVars;                  // lista nazw
END;
```

## 21.4. Notes — notatki z poziomu programu (*UG s. 694*)

| Zapis | Działanie |
|---|---|
| `Notes` | lista nazw notatek |
| `Notes(n)` / `Notes("nazwa")` | treść notatki |
| `Notes("nazwa") := "tekst"` | tworzy lub nadpisuje notatkę |
| `Notes("nazwa") := ""` | usuwa notatkę |

Program może więc **zapisywać raport** do notatki, którą użytkownik potem przeczyta w katalogu notatek (skrót Notes na klawiaturze albo `STARTVIEW(-8)`):

```
EXPORT RAPORT_DZIEL(n)
BEGIN
  LOCAL d := CAS.idivis(n), s := "Dzielniki liczby "+n+":\n", i;
  FOR i FROM 1 TO SIZE(d) DO
    s := s + d(i) + "\n";
  END;
  Notes("Raport") := s;
  STARTVIEW(-8);                 // otwórz katalog notatek
END;
```

Notatki opisuje rozdział 27 instrukcji. Edytor notatek obsługuje formatowanie (pogrubienie, kursywa, rozmiary, kolory, listy wypunktowane) i wstawianie wyrażeń matematycznych. Notatka aplikacji (`ANote`) to specjalna notatka pokazywana w widoku Info.

## 21.5. Programs — programy jako dane (*UG s. 694*)

| Zapis | Działanie |
|---|---|
| `Programs` | lista nazw programów |
| `Programs(n)` / `Programs("nazwa")` | **kod źródłowy** programu jako tekst |
| `Programs("nazwa") := "kod"` | tworzy lub nadpisuje program |
| `Programs("nazwa") := ""` | usuwa program |

To **metaprogramowanie**: program może czytać, generować i instalować inne programy.

```
// program generujący program
EXPORT GENERUJ(n)
BEGIN
  LOCAL kod;
  kod := "EXPORT POTEGA"+n+"(x)\nBEGIN\n  RETURN x^"+n+";\nEND;";
  Programs("POTEGA"+n) := kod;
  RETURN "Utworzono POTEGA"+n;
END;
```

Po `GENERUJ(5)` w katalogu programów pojawi się `POTEGA5`, a `POTEGA5(2)` zwróci 32.

Inne zastosowania: kopia zapasowa programów w notatce (`Notes("backup") := Programs("MOJPROG")`), statystyka programów (liczba linii, długość), prosty „instalator” zestawu programów.

> **Ostrożnie:** `Programs("nazwa") := ""` usuwa program bez pytania.

## 21.6. Stałe fizyczne i jednostki (*UG rozdz. 25*)

Menu Units zawiera jednostki i stałe fizyczne, np. prędkość światła, stałą Plancka i ładunek elementarny. W programie możesz użyć stałej z jednostką i przeliczać ją funkcjami `CONVERT` i `USIMPLIFY`. Możesz też pozbyć się jednostki przez podzielenie wyniku przez jednostkę bazową:

```
LOCAL c := 299792458_(m/s);
LOCAL w := c/1_(m/s);        // 299792458 bez jednostki
```

## 21.7. Memory Manager i kopie zapasowe (*UG rozdz. 2*)

- `STARTVIEW(-3)` otwiera Memory Manager, który pokazuje zajętość pamięci przez aplikacje, programy, notatki, listy i macierze.
- Kopie zapasowe całego kalkulatora (*Backups*) tworzysz w Memory Manager albo w Connectivity Kit na PC. Przed eksperymentami z `Programs(...) :=` i `DelHVars` zrób kopię.

---

## Sprawdź się

1. Napisz `LISTA_PROGRAMOW()`, który wypisuje w terminalu nazwy wszystkich programów i liczbę znaków kodu każdego z nich.
2. Napisz `DZIENNIK(tekst)`, który dopisuje linię z datą i tekstem na końcu notatki „Dziennik” (tworzy ją, jeśli nie istnieje).
3. Napisz `SPRZATAJ(prefiks)`, który usuwa wszystkie zmienne użytkownika Home, których nazwa zaczyna się od podanego prefiksu (np. `"wynik"` z przykładu 21.3). Przed usunięciem niech pyta o potwierdzenie `MSGBOX(…,1)`.

[Rozwiązania →](25-cwiczenia.md#rozdział-21)
