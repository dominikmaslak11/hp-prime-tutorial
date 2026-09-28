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

1. Zbuduj paczkę (sekcja *Budowanie*) albo użyj gotowej `tools\dist\hp-prime-ppl-1.0.0.vsix`.
2. Zainstaluj: *Rozszerzenia › ⋯ › Zainstaluj z pliku VSIX…* albo w terminalu:
   ```
   code --install-extension tools\dist\hp-prime-ppl-1.0.0.vsix
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
```

Format błędów to `plik:linia:kolumna: error: komunikat [kod]`. Rozumie go większość edytorów i systemów CI.

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
| `ppl_language_guide` | przewodnik po języku (zasady i lista komend) |

Modele słabo znają HP PPL i często wymyślają komendy albo mylą składnię z Pascalem lub BASIC-iem. Z tymi narzędziami agent pisze program, sam go sprawdza, poprawia błędy i dopiero wtedy pokazuje wynik. Działa to podobnie jak asystent w Android Studio.

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
| `missing-semicolon` | brak `;` między poleceniami |
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
| `unknown-name` | nazwa, której nie zna ani program, ani kalkulator (ostrzeżenie) |
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
