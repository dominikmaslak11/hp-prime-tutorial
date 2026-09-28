# Plan: narzędzia programisty HP PPL

Decyzja z 28.09.2026: zamiast budować od zera własne IDE w Qt robimy **rozszerzenie do VS Code** oparte na wspólnym rdzeniu w C++. Rozszerzenie łączymy z istniejącymi agentami AI przez serwer MCP. Do tego dochodzi **pakiet dla Notepad++**.

## Dlaczego tak

- VS Code ma już dojrzały edytor, a rozszerzenia AI (GitHub Copilot, Claude Code, Continue) działają z Claude, ChatGPT, Gemini i DeepSeek, także w trybie agenta.
- Nasza wartość dodana to **wiedza o języku PPL**: walidator, baza komend i dokumentacja po polsku. Udostępniamy ją edytorom przez LSP, a agentom AI przez MCP. Dzięki temu agent sam sprawdza wygenerowany kod i nie zmyśla komend.
- Rdzeń w czystym C++ (bez Qt) daje jeden plik `ppl.exe` bez zależności. Można go użyć w VS Code, Notepad++, z linii poleceń, w CI, a w przyszłości we własnym IDE.

## Architektura

```
tools/
├── PLAN.md                  ← ten plik
├── CMakeLists.txt           ← budowa rdzenia, ppl.exe i testów (C++20)
├── core/                    ← biblioteka pplcore (czysty C++20)
│   ├── utf8, json           ← UTF-8 ↔ UTF-32, własny parser i generator JSON
│   ├── lexer                ← tokeny PPL (Unicode: π √ Σ ∂ ∫ θ ≠ ≤ ≥ ▶ →)
│   ├── analyzer             ← parser + walidacja semantyczna → diagnostyka
│   ├── formatter            ← wcięcia, konwersja ASCII ↔ symbole
│   ├── commanddb            ← baza komend (data/commands.json, wbudowana w exe)
│   └── services             ← hover, podpowiedzi, sygnatury, symbole (dla LSP/MCP)
├── ppl/                     ← jeden program ppl.exe z trybami:
│   ├── ppl check  pliki…    ← walidacja (format gcc: plik:linia:kol: error: …)
│   ├── ppl format plik      ← formatowanie
│   ├── ppl help KOMENDA     ← opis komendy
│   ├── ppl lsp              ← serwer Language Server Protocol (stdio)
│   ├── ppl mcp              ← serwer Model Context Protocol dla agentów AI (stdio)
│   └── ppl gen …            ← generowanie gramatyki VS Code i plików Notepad++ z bazy komend
├── tests/                   ← testy jednostkowe + test korpusu (wszystkie programy z kursu)
├── vscode/                  ← rozszerzenie VS Code „HP Prime PPL”
│   ├── gramatyka TextMate, konfiguracja języka, snippety
│   ├── klient LSP (vscode-languageclient) uruchamiający bin/ppl.exe lsp
│   ├── rejestracja serwera MCP dla agentów w VS Code
│   └── komendy: sprawdź, formatuj, kopiuj do schowka, dodaj instrukcje AI do projektu
└── notepadpp/               ← kolorowanie UDL, autouzupełnianie, skrypt NppExec (pplcheck)
```

## Funkcje

### Walidator (rdzeń)
- struktura bloków: BEGIN/END, IF/THEN/ELSE/END, CASE/DEFAULT, IFERR, FOR/WHILE/REPEAT; wskazanie, gdzie blok został otwarty,
- średniki, nawiasy, niezamknięte teksty, cudzysłowy typograficzne,
- funkcja użyta przed deklaracją, zła liczba argumentów (funkcje w pliku i komendy wbudowane),
- `=` zamiast `==` w warunku, `=` zamiast `:=` w `LOCAL`,
- BREAK/CONTINUE poza pętlą, EXPORT wewnątrz funkcji,
- zapis złego typu do zmiennej systemowej (`L1:=5`, `A:={1}`, `G1:=…`),
- nieznane nazwy (ostrzeżenie), nieużywane zmienne lokalne, nazwy klawiszy w `KEY`, nazwy aplikacji w `STARTAPP`.

### LSP (VS Code i inne edytory)
diagnostyka na żywo, podpowiedzi (komendy, słowa kluczowe, zmienne systemowe, funkcje i zmienne z pliku, zmienne lokalne), podpowiedzi sygnatury z zaznaczeniem bieżącego argumentu, opisy po najechaniu (po polsku, z odnośnikiem do rozdziału kursu), konspekt dokumentu, przejście do definicji, formatowanie.

### MCP (agenci AI)
Narzędzia: `ppl_validate`, `ppl_format`, `ppl_command_help`, `ppl_search_commands`, `ppl_language_guide`. Konfiguracja dla Claude Code, GitHub Copilot (VS Code) i Continue, które obsługują Claude, GPT, Gemini i DeepSeek.

### Notepad++
UDL z kolorowaniem PPL, autouzupełnianie z podpowiedziami parametrów (generowane z bazy komend), sprawdzanie przez NppExec + `ppl.exe check` z klikalnymi błędami.

## Etapy

1. [x] Rdzeń C++: utf8, json, lexer, analyzer, formatter, commanddb + testy + test korpusu kursu
2. [x] `ppl.exe`: check, format, help, gen
3. [x] Serwer LSP
4. [x] Serwer MCP
5. [x] Rozszerzenie VS Code (+ paczka .vsix)
6. [x] Pakiet Notepad++
7. [x] Dokumentacja użytkownika (README w `tools/`), skrypt budowania, commit i push

---

# Wersja 2: uruchamianie i debugowanie programów

Decyzja z 28.09.2026 (użytkownik dał wolną rękę). Kalkulator, oficjalny emulator i Connectivity Kit nie mają interfejsu do zdalnego debugowania. Dlatego piszemy **własny interpreter PPL** w rdzeniu C++ i udostępniamy go:
- w linii poleceń (`ppl run`),
- w VS Code jako debugger zgodny z **Debug Adapter Protocol** (`ppl dap`),
- agentom AI jako narzędzie MCP `ppl_run`: agent uruchamia program i widzi wynik oraz zrzut ekranu.

Interpreter to **symulator**, a nie emulator firmware. Tam, gdzie nie odtwarza kalkulatora wiernie, mówi to wprost i nie zgaduje.

## Architektura v2

```
core/
├── ast.h / parser.cpp       drzewo składni (osobny parser; analyzer zostaje walidatorem)
├── value.h / value.cpp      wartości: liczba rzeczywista (zaokrąglanie do 12 cyfr jak w kalkulatorze),
│                             całkowita #…h/b/o/d (rozmiar słowa, system), zespolona, tekst, lista, macierz
├── interpreter.h/.cpp       wykonywanie: zasięgi LOCAL/plik/globalne, zmienne systemowe z typami,
│                             funkcje, rekurencja, BREAK/CONTINUE/RETURN/KILL, IFERR, haki dla debuggera
├── builtins_*.cpp           ~180 komend: matematyka, listy, teksty, macierze, bity, ∂ ∫ Σ numerycznie,
│                             wybrane CAS (idivis, isprime, gcd…), aplikacje (F0–F9, ROOT, SLOPE, AREA, SOLVE,
│                             statystyka 1Var), Notes/Programs/HVars/AVars
├── graphics.h/.cpp          ekran 320×240 + bufory G0–G9: linie, prostokąty, wielokąty, łuki, trójkąty
│                             (gradient), tekst (czcionka bitmapowa), BLIT ze skalowaniem, INVERT, PNG
└── host.h                   interfejs wejścia/wyjścia: PRINT, MSGBOX, INPUT, CHOOSE, klawisze, dotyk, czas
ppl/
├── run.cpp                  ppl run PLIK "WYWOŁANIE" [--input …] [--keys …] [--screen ekran.png]
└── dap.cpp                  ppl dap: serwer Debug Adapter Protocol (osobny wątek interpretera)
vscode/
├── debugger                 konfiguracja uruchamiania (F5, Ctrl+F5), pytanie o wywołanie np. SQIN(5)
├── dialogi                  MSGBOX/INPUT/CHOOSE jako okna VS Code
└── ekran                    panel z ekranem kalkulatora na żywo + klawiatura i dotyk (GETKEY, MOUSE)
```

## Funkcje debuggera
F5 / Ctrl+F5, pułapki (także warunkowe i logujące), Step Over / Into / Out, pauza, stos wywołań, zmienne (lokalne, pliku, systemowe; rozwijane listy i macierze), obserwowane wyrażenia, obliczanie w konsoli debugowania, zmiana wartości zmiennej, zatrzymanie na błędzie wykonania, PRINT do konsoli, wynik funkcji po zakończeniu.

## Poza zakresem v2 (jasny komunikat „nieobsługiwane w symulatorze”)
Symboliczny CAS (`diff`, `solve` symbolicznie…), pełne widoki aplikacji HP (wykresy, tabele), jednostki fizyczne, zaawansowane formy 3D `LINE_P`/`TRIANGLE_P`, `AFiles`, bity trybu egzaminacyjnego.

## Etapy v2

8. [x] Parser AST + wartości + interpreter + wbudowane komendy (bez grafiki) + `ppl run` + testy „złotych wyników” z kursu (SQIN, MOPMT, SUMDIV, ULAM, SUBEXAM, CALCDEMO, TERMVEL…)
9. [x] Wejście/wyjście (host), skryptowane dane w CLI, silnik graficzny + czcionka + zapis PNG; testy obrazu
10. [—] ~~Serwer DAP + debugger w VS Code (pułapki, kroki, zmienne, konsola, dialogi MSGBOX/INPUT/CHOOSE)~~ — **porzucone**, zob. niżej
11. [—] ~~Panel ekranu kalkulatora w VS Code (grafika na żywo, klawiatura → GETKEY/ISKEYDOWN, mysz → MOUSE)~~ — **porzucone**, zob. niżej
12. [x] Narzędzie MCP `ppl_run` (tekst + zrzut ekranu), polecenie VS Code „Uruchom program (symulator)” (F5), dokumentacja, testy, commit i push

### Decyzja z 28.09.2026: bez własnego debuggera w VS Code
HP Prime ma **wbudowany debugger krok po kroku** (katalog programów › Debug: Step, Skip, Vars, Cont, Exit; UG wyd. 3, s. 607). Działa on także w oficjalnym emulatorze **HP Prime Virtual Calculator** na PC. Etapy 10–11 dawałyby tylko wygodę, bez nowej możliwości, więc je porzucono. Interpreter zostaje, bo robi to, czego kalkulator i emulator nie umieją: automatyczne uruchamianie programów przez testy i agentów AI (`ppl run`, MCP `ppl_run`). W VS Code jest proste polecenie „Uruchom program (symulator)”.

## Później
- eksport/import binarnych plików `.hpprgm` (Connectivity Kit),
- własne IDE w Qt korzystające z tego samego rdzenia,
- dokładna arytmetyka dziesiętna (BCD) zamiast zaokrąglania double do 12 cyfr.
