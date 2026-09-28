# HP Prime PPL dla VS Code

Rozszerzenie do programowania kalkulatora **HP Prime** w języku **HP PPL**, z opisami i komunikatami po polsku.

## Funkcje

- **Kolorowanie składni**: słowa kluczowe, ponad 250 komend, zmienne systemowe, liczby `#FFh`, symbole `≠ ≤ ≥ ▶ π √ Σ ∂ ∫`.
- **Sprawdzanie błędów na żywo** (podkreślenia i panel *Problemy*):
  - niezamknięte bloki (z informacją, w której linii blok został otwarty), brakujące średniki, nawiasy i cudzysłowy,
  - zła liczba argumentów komend i funkcji, funkcja użyta przed deklaracją,
  - `=` zamiast `==` w warunku, `=` zamiast `:=` w `LOCAL`, `BREAK` poza pętlą,
  - zły typ zapisywany do zmiennej systemowej (`L1 := 5`), pętla po stałej `i` bez `LOCAL i`,
  - nieznane nazwy, nieużywane zmienne lokalne, nieznane nazwy klawiszy w `KEY`.
- **Podpowiedzi** (Ctrl+Spacja): komendy z opisem, zmienne lokalne, funkcje z pliku, zmienne systemowe, gotowe szablony (FOR, IF, CASE, INPUT z obsługą Cancel, pętla animacji…).
- **Podpowiedź parametrów** podczas pisania `KOMENDA(`, z zaznaczeniem bieżącego argumentu.
- **Opis po najechaniu myszą** na komendę, z przykładem i odnośnikiem do rozdziału kursu.
- **Konspekt** pliku (funkcje, zmienne) i **przejście do definicji** (F12).
- **Formatowanie** (Shift+Alt+F): wcięcia bloków i wielkie litery słów kluczowych.
- Polecenia (Ctrl+Shift+P → „HP PPL”):
  - *Kopiuj program do schowka* (Ctrl+Alt+C), do wklejenia w HP Connectivity Kit,
  - *Opis komendy…* (Ctrl+F1): wyszukiwarka wszystkich komend,
  - *Zamień operatory* ASCII ↔ symbole kalkulatora,
  - *Nowy program*, *Przewodnik po języku PPL*.

## Asystenci AI

Rozszerzenie rejestruje w VS Code **serwer MCP „HP Prime PPL”**. Agenci AI w VS Code (np. GitHub Copilot w trybie Agent, z modelami Claude, GPT lub Gemini) dostają narzędzia:

| Narzędzie | Działanie |
|---|---|
| `ppl_validate` | sprawdza kod PPL i zwraca błędy z numerami linii |
| `ppl_check_file` | sprawdza plik na dysku |
| `ppl_format` | formatuje kod |
| `ppl_command_help` | opis komendy (po polsku) |
| `ppl_search_commands` | wyszukiwanie komend |
| `ppl_language_guide` | przewodnik po języku |

Dzięki temu agent sam sprawdza wygenerowany program i poprawia błędy, zanim pokaże Ci wynik.

Polecenie **HP PPL: Skonfiguruj asystentów AI dla tego projektu** zapisuje w otwartym folderze instrukcje i konfigurację MCP dla:
- **GitHub Copilot**: `.github/copilot-instructions.md`,
- **Claude Code**: `CLAUDE.md` i `.mcp.json`,
- **Continue** (obsługuje Claude, ChatGPT, Gemini, DeepSeek i modele lokalne): `.continue/rules/hp-ppl.md` i `.continue/mcpServers/hp-prime-ppl.yaml`,
- **Gemini CLI i innych agentów**: `GEMINI.md`, `AGENTS.md`.

## Pliki

Rozszerzenie obsługuje pliki `.hpppl`, `.ppl` i `.hpprgm.txt`. Dowolny inny plik przełączysz na HP PPL w prawym dolnym rogu okna (wybór języka).

## Ustawienia

| Ustawienie | Opis |
|---|---|
| `hpppl.serverPath` | własna ścieżka do `ppl.exe` (domyślnie dołączony) |
| `hpppl.copy.useCalculatorSymbols` | przy kopiowaniu zamieniaj `<>` na `≠` itd. |
| `hpppl.mcp.enabled` | udostępniaj narzędzia agentom AI |

Kod źródłowy i kurs HP PPL: https://github.com/dominikmaslak11/hp-prime-tutorial
