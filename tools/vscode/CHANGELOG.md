# Zmiany

## 1.2.0
- Polecenie „Zbuduj plik programu (.hpprgm)”: plik gotowy do przeciągnięcia na kalkulator w HP Connectivity Kit.
- Polecenie „Otwórz plik programu (.hpprgm)…” (także z menu kontekstowego Eksploratora): wyciąga kod źródłowy do edytora.
- Narzędzie MCP `ppl_build` dla agentów AI.
- Walidator zna wszystkie ~1170 nazw z pomocy HP (koniec fałszywych ostrzeżeń „nieznana nazwa” np. dla NORMALD_CDF, POLYROOT, erf).
- Nowe reguły zmierzone na emulatorze: brak `;` po `END` funkcji to błąd, najwyżej 8 zmiennych w jednym `LOCAL`, zakaz `F()(2)`.
- Symulator bliższy kalkulatorowi (test zgodności z wynikami HP Prime Virtual Calculator 2.4).

## 1.1.0
- Polecenie „Uruchom program (symulator)” (F5): wynik, wyjście PRINT/MSGBOX, zrzut ekranu, zaznaczenie linii błędu.
- Narzędzie MCP `ppl_run` dla agentów AI (wynik + zrzut ekranu).
- Baza komend wg instrukcji HP wyd. 3 (Finance, Explorer, Graph 3D).

## 1.0.0
- Pierwsza wersja: kolorowanie, walidacja na żywo (LSP), podpowiedzi, sygnatury, opisy, formatowanie, konspekt, serwer MCP dla agentów AI, konfiguracja Copilot / Claude Code / Continue / Gemini.
