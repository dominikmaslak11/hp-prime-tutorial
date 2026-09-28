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

## Później (poza zakresem pierwszej wersji)
- eksport/import binarnych plików `.hpprgm` (Connectivity Kit),
- uruchamianie emulatora HP Prime z edytora,
- własne IDE w Qt korzystające z tego samego rdzenia.
