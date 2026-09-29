# Narzędzia programisty HP PPL

Zestaw narzędzi do pisania programów na kalkulator **HP Prime** w języku **HP PPL**:

| Składnik | Co daje |
|---|---|
| **`ppl.exe`** | walidator, formatter, opis komend, serwer języka (LSP) i serwer MCP dla agentów AI; jeden plik bez zależności |
| **Rozszerzenie VS Code** (`vscode/`) | kolorowanie, błędy na żywo, podpowiedzi, sygnatury, opisy, formatowanie, konspekt, integracja z asystentami AI |
| **Pakiet Notepad++** (`notepadpp/`) | kolorowanie UDL, autouzupełnianie z podpowiedzią parametrów, sprawdzanie błędów przez NppExec |

Plan i decyzje projektowe: [PLAN.md](PLAN.md).

---

## Szybki start

### VS Code

1. Zbuduj paczkę (sekcja *Budowanie*) albo użyj gotowej `tools\dist\hp-prime-ppl-1.1.0.vsix`.
2. Zainstaluj: *Rozszerzenia › ⋯ › Zainstaluj z pliku VSIX…* albo w terminalu:
   ```
   code --install-extension tools\dist\hp-prime-ppl-1.1.0.vsix
   ```
3. Otwórz albo utwórz plik `*.hpppl` (*Ctrl+Shift+P › HP PPL: Nowy program*).

Szczegóły funkcji: [vscode/README.md](vscode/README.md).

### Notepad++

```powershell
powershell -ExecutionPolicy Bypass -File tools\notepadpp\install-notepadpp.ps1
```

Szczegóły, w tym konfiguracja NppExec: [notepadpp/README.md](notepadpp/README.md).

### Linia poleceń

```
ppl check PROGRAM.hpppl            sprawdza program (kod wyjścia 1 przy błędach)
ppl check --json *.hpppl           wynik w JSON
ppl format --write PROGRAM.hpppl   poprawia wcięcia
ppl format --symbols PROGRAM.hpppl zamienia <> <= >= na ≠ ≤ ≥
ppl help TEXTOUT_P                 opis komendy
ppl search klawisz                 wyszukiwanie komend
ppl guide                          przewodnik po PPL (Markdown)
ppl run PROGRAM.hpppl "SUMDIV(12)" uruchamia program w symulatorze
ppl build PROGRAM.hpppl            buduje PROGRAM.hpprgm dla Connectivity Kit
ppl extract PROGRAM.hpprgm         wypisuje kod źródłowy z pliku programu
```

Format błędów to `plik:linia:kolumna: error: komunikat [kod]`. Rozumie go większość edytorów i systemów CI.

---

## Uruchamianie programów na komputerze (symulator)

`ppl run` wykonuje program PPL na komputerze. W VS Code to samo robi polecenie **HP PPL: Uruchom program (symulator)** (F5): pyta o wywołanie i dane, pokazuje wynik oraz zrzut ekranu, a przy błędzie wykonania zaznacza linię w edytorze.

```
ppl run SNAKE.hpppl --keys 8,8,12,4 --screen ekran.png   klawisze dla GETKEY + zrzut ekranu (PNG)
ppl run AREAC.hpppl --input "2;2.5;1.5"                   odpowiedzi dla CHOOSE i INPUT, po kolei
ppl run KASUJ.hpppl --answers cancel                      odpowiedź dla MSGBOX(…, 1)
ppl run PROG.hpppl "F(3)" --json                          wynik w JSON (dla skryptów)
```

- Dane są podawane z góry, bo symulator nie jest interaktywny. `WAIT` i `TICKS` używają zegara wirtualnego, więc wyniki są powtarzalne. Pętla czekająca na klawisz, którego nie podano, kończy się komunikatem o limicie kroków.
- Programy z tego samego folderu (`*.hpppl`, `*.ppl`) są widoczne jak inne programy na kalkulatorze, więc ich funkcje z `EXPORT` można wywoływać.
- **Obsługiwane:** cały język (bloki, pętle, funkcje, rekurencja, `IFERR`, `KILL`), liczby rzeczywiste zaokrąglane do 12 cyfr (np. `0.1+0.2==0.3` daje 1, tak jak na kalkulatorze), liczby `#` z systemami i rozmiarem słowa, liczby zespolone, teksty, listy, macierze (m.in. odwrotność, wyznacznik, RREF), około 250 komend, numeryczne `∂ ∫ Σ Π` i `|`, wybrane funkcje CAS (`idivis`, `isprime`, `ifactor`, `gcd`…), grafika 320×240 z buforami G0–G9, funkcje aplikacji (F0–F9, `ROOT`, `SLOPE`, `AREA`, `SOLVE`, statystyka 1Var/2Var, TVM z Finance) oraz prosty wykres po `STARTVIEW(1)`.
- **Nieobsługiwane** (z komunikatem „nieobsługiwane w symulatorze”): symboliczny CAS (`diff`, `solve` na wyrażeniach), jednostki, widoki aplikacji inne niż prosty wykres, formy 3D `LINE_P`/`TRIANGLE_P`, `AFiles`. Czcionka tekstu na ekranie jest przybliżona (5×8 skalowana).
- **To symulator, a nie firmware HP.** Wynik warto na koniec sprawdzić na kalkulatorze. Do pracy krok po kroku służy wbudowany debugger HP Prime (katalog programów › **Debug**), dostępny też w oficjalnym emulatorze *HP Prime Virtual Calculator*.

Testy (`tools/tests`) uruchamiają w symulatorze programy z kursu i porównują wyniki z wartościami podanymi w kursie, w tutorialu Shore'a i w instrukcji HP. Są to m.in. SQIN, MOPMT, SUMDIV, ULAM, QROOTS, SUBEXAM, TERMVEL, AREAC, CALCDEMO, a także rysunki (DRAWHOUSE, DRAWPENT, DRAWARCS) i gra SNAKE.

**Przykłady z kursu.** Test `interp_tutorial_examples_match_simulator` wyszukuje w rozdziałach przykłady `wyrażenie` → wynik i `wyrażenie  // wynik`, wylicza je w symulatorze i zgłasza każdą niezgodność (obecnie 93 przykłady, 0 niezgodności).

**Test zgodności z emulatorem HP.** Projekt [hp-prime-kit](https://github.com/JordiRigau/hp-prime-kit) (licencja MIT) zmierzył na *HP Prime Virtual Calculator 2.4* odpowiedzi na ok. 1250 wywołań z dokumentacji. Test `conformance_with_virtual_calculator` porównuje z nimi symulator i walidator, a rozbieżności zapisuje w `build/conformance-report.txt`. Obecnie: 700 odpowiedzi zgodnych, 284 dotyczą funkcji nieobsługiwanych w symulatorze (CAS, geometria…), pozostałe to głównie ustawienia emulatora z chwili pomiaru (bieżąca aplikacja, zdefiniowane F1, okno wykresu). Walidator zgadza się z emulatorem w 30 z 31 przypadków „kompiluje się / nie kompiluje się”.

---

## Pliki programów `.hpprgm` (Connectivity Kit)

HP Connectivity Kit przechowuje programy w plikach `.hpprgm`. W środku jest kod źródłowy (UTF-16) i, gdy program był już na kalkulatorze, skompilowana wersja. Kalkulator odtwarza ją sam ze źródła, więc do przesłania programu wystarczy plik z samym kodem.

```
ppl build SITO.hpppl                       tworzy SITO.hpprgm (najpierw sprawdza błędy)
ppl build prog.hpppl -o GRA.hpprgm         nazwa pliku = nazwa programu na kalkulatorze
ppl extract SITO.hpprgm -o SITO.hpppl      wyciąga kod z pliku programu (także z kalkulatora)
ppl verify *.hpprgm                        sprawdza, czy pliki da się odczytać i przebudować
```

Gotowy plik przeciągnij na kalkulator (albo emulator) w oknie Connectivity Kit. W VS Code służą do tego polecenia **HP PPL: Zbuduj plik programu (.hpprgm)** i **Otwórz plik programu (.hpprgm)…**, a agent AI ma narzędzie MCP `ppl_build`.

Format pliku nie jest publicznie opisany przez HP. Implementacja opiera się na analizie z projektu hp-prime-kit i daje pliki identyczne bajt w bajt z jego narzędziem `hpprime write`, którego wyniki autorzy sprawdzili na prawdziwym kalkulatorze. Mimo to pierwszy zbudowany plik sprawdź u siebie: przeciągnij go na kalkulator i uruchom program.

---

## Asystenci AI (Claude, ChatGPT, Gemini, DeepSeek)

`ppl mcp` to serwer **Model Context Protocol**. Każdy agent AI, który obsługuje MCP, dostaje narzędzia:

| Narzędzie | Działanie |
|---|---|
| `ppl_validate` | sprawdza kod i zwraca błędy z numerami linii |
| `ppl_check_file` | sprawdza plik na dysku |
| `ppl_format` | formatuje kod |
| `ppl_command_help` | opis komendy po polsku |
| `ppl_search_commands` | wyszukiwanie komend |
| `ppl_run` | **uruchamia program w symulatorze**: wynik, wyjście PRINT/MSGBOX, błędy wykonania z numerem linii, **zrzut ekranu** (obraz PNG, który model widzi) |
| `ppl_build` | zapisuje program jako plik `.hpprgm` dla Connectivity Kit |
| `ppl_language_guide` | przewodnik po języku (zasady i lista komend) |

Modele słabo znają HP PPL i często wymyślają komendy albo mylą składnię z Pascalem lub BASIC-iem. Z tymi narzędziami agent pisze program, sprawdza składnię, uruchamia go, porównuje wynik z oczekiwanym, patrzy na zrzut ekranu, poprawia błędy i dopiero wtedy pokazuje wynik. Działa to podobnie jak asystent w Android Studio.

W przykładach poniżej `PPL` oznacza pełną ścieżkę do `ppl.exe`, np. `C:\\Users\\dominik\\AppData\\Local\\hp-ppl\\ppl.exe` (tam kopiuje go instalator Notepad++) albo `tools\build\ppl.exe`.

**Najprościej w VS Code:** polecenie *HP PPL: Skonfiguruj asystentów AI dla tego projektu* zapisuje wszystkie pliki poniżej automatycznie.

### GitHub Copilot (VS Code): modele Claude, GPT, Gemini
Rozszerzenie rejestruje serwer MCP samo (VS Code 1.101+). W oknie czatu wybierz tryb **Agent**, a w ikonie narzędzi zaznacz **HP Prime PPL**. Instrukcje dla Copilota: `.github/copilot-instructions.md` (tworzy je polecenie rozszerzenia).

### Claude Code
```
claude mcp add hp-prime-ppl --scope user -- "PPL" mcp
```
Albo plik `.mcp.json` w projekcie:
```json
{ "mcpServers": { "hp-prime-ppl": { "type": "stdio", "command": "PPL", "args": ["mcp"] } } }
```

### Continue (VS Code / JetBrains): Claude, ChatGPT, Gemini, DeepSeek, modele lokalne
Plik `.continue/mcpServers/hp-prime-ppl.yaml`:
```yaml
name: HP Prime PPL
version: 1.0.0
schema: v1
mcpServers:
  - name: hp-prime-ppl
    command: "PPL"
    args:
      - mcp
```
DeepSeek skonfigurujesz w Continue jako dostawcę modelu (`provider: deepseek`). Narzędzia MCP działają z każdym modelem, który obsługuje wywoływanie narzędzi.

### Gemini CLI
`~/.gemini/settings.json`:
```json
{ "mcpServers": { "hp-prime-ppl": { "command": "PPL", "args": ["mcp"] } } }
```

### OpenAI Codex CLI (ChatGPT)
`~/.codex/config.toml`:
```toml
[mcp_servers.hp-prime-ppl]
command = "PPL"
args = ["mcp"]
```

---

## Co sprawdza walidator

| Kod | Przykład |
|---|---|
| `missing-end` | `IF x>0 THEN …` bez `END` (komunikat podaje linię otwarcia bloku) |
| `missing-semicolon` | brak `;` między poleceniami albo po `END` kończącym funkcję |
| `too-many-locals` | więcej niż 8 zmiennych w jednym `LOCAL` (kalkulator tego nie skompiluje) |
| `call-index` | `F()(2)`: indeksowanie wyniku wywołania; zapisz wynik w zmiennej |
| `expected-keyword` | `FOR i FROM 1 TO 3` bez `DO` |
| `unclosed-paren`, `unclosed-brace` | niezamknięte nawiasy |
| `unterminated-string`, `typographic-quote` | niezamknięty tekst, cudzysłów „ ” skopiowany z Worda |
| `use-before-declaration` | wywołanie funkcji zdefiniowanej niżej bez deklaracji `NAZWA();` |
| `wrong-arg-count` | `LEFT("abc")`, `MSGBOX()`, funkcja z pliku wywołana ze złą liczbą argumentów |
| `equals-in-declaration` | `LOCAL s = 0` |
| `equals-in-condition` | `IF x = 5 THEN` (ostrzeżenie) |
| `wrong-type` | `L1 := 5`, `A := {1,2}`, `M1 := "x"` |
| `assign-to-constant` | `FOR i FROM …` bez `LOCAL i` (`i` to jednostka urojona) |
| `reserved-name` | funkcja o nazwie zmiennej systemowej (`Q1`, `F`, `M1`) |
| `break-outside-loop` | `BREAK` poza pętlą |
| `export-inside-function` | `EXPORT` w środku funkcji (zwykle brakuje `END` wyżej) |
| `unknown-name` | nazwa, której nie zna ani program, ani kalkulator (ostrzeżenie); walidator zna ~1170 nazw z pomocy HP |
| `unused-local` | zadeklarowana, ale nieużywana zmienna (ostrzeżenie) |
| `unknown-key` | zła nazwa klawisza w `KEY` |
| `implicit-multiplication` | `2X` zamiast `2*X` (ostrzeżenie) |

Walidator sprawdza składnię statycznie. Nie uruchamia programu, więc błędy wykonania, np. dzielenie przez zero, zobaczysz dopiero na kalkulatorze.

---

## Budowanie

Wymagania: MSYS2 UCRT64 (`g++`, `cmake`, `ninja`, `nodejs`) albo inny kompilator C++20, CMake 3.21+, Node.js 18+ (tylko dla VS Code).

```powershell
powershell -ExecutionPolicy Bypass -File tools\build.ps1              # wszystko
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -SkipVsix    # tylko ppl.exe + testy
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -VsCodeTests # + testy w VS Code
```

Skrypt:
1. kompiluje `ppl.exe` i testy (CMake + Ninja),
2. uruchamia testy jednostkowe oraz **test korpusu**: sprawdza wszystkie programy z rozdziałów kursu (około 170) i wymaga zera błędów,
3. generuje gramatykę VS Code i pliki Notepad++ z bazy komend,
4. pakuje rozszerzenie do `tools\dist\*.vsix`,
5. opcjonalnie uruchamia testy integracyjne w prawdziwym VS Code, w izolowanym profilu.

## Struktura

```
tools/
├── data/commands.json   baza ~260 komend i ~320 zmiennych (opisy PL, składnia, liczba argumentów, rozdział kursu)
├── core/                lekser, analizator, formatter, usługi edytora (C++20, bez zależności)
├── ppl/                 ppl.exe: CLI, LSP, MCP, generatory
├── tests/               testy jednostkowe + korpus kursu
├── vscode/              rozszerzenie VS Code (klient LSP, polecenia, rejestracja MCP, testy integracyjne)
└── notepadpp/           UDL, autouzupełnianie, instalator
```

### Dodawanie komendy

Dopisz wpis w `data/commands.json`:

```json
{"name": "NAZWA", "kind": "command", "cat": "Kategoria", "syntax": "NAZWA(a, [b])",
 "desc": "Opis.", "example": "NAZWA(1)", "ch": "03-typy-danych-i-operatory.md", "min": 1, "max": 2}
```

Potem uruchom `build.ps1`. Walidator, podpowiedzi, kolorowanie w VS Code, pliki Notepad++ i przewodnik AI zaktualizują się automatycznie.
