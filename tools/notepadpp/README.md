# HP PPL w Notepad++

Pakiet zastępuje obecne ustawienie „Pascal/Basic”. Daje:

- **kolorowanie HP PPL** (UDL): słowa kluczowe, komendy, funkcje CAS, zmienne systemowe, stałe, liczby `#FFh`, teksty, komentarze `//`, zwijanie bloków `BEGIN…END`, `IF…END`, `FOR…END`, `REPEAT…UNTIL`,
- **autouzupełnianie** ponad 250 komend i zmiennych, z **podpowiedzią parametrów** po wpisaniu `(`,
- **sprawdzanie błędów** przez `ppl.exe` i wtyczkę NppExec, z klikalnymi komunikatami.

Pliki `HP_PPL_udl.xml` i `HP PPL.xml` są generowane z tej samej bazy komend co rozszerzenie VS Code (`ppl gen notepadpp`), więc zawsze są ze sobą zgodne.

## Instalacja

```powershell
cd tools\notepadpp
powershell -ExecutionPolicy Bypass -File install-notepadpp.ps1 -DryRun   # podgląd
powershell -ExecutionPolicy Bypass -File install-notepadpp.ps1          # instalacja
```

Skrypt kopiuje:

| Plik | Dokąd |
|---|---|
| `HP_PPL_udl.xml` | `%APPDATA%\Notepad++\userDefineLangs\` |
| `HP PPL.xml` (autouzupełnianie) | `C:\Program Files\Notepad++\autoCompletion\` (wymaga uprawnień administratora) |
| `ppl.exe` | `%LOCALAPPDATA%\hp-ppl\` |

Instalacja ręczna: *Język › Język użytkownika › Definiuj własny język… › Importuj…* i wskaż `HP_PPL_udl.xml`. Plik `HP PPL.xml` skopiuj do katalogu `autoCompletion` w folderze instalacji Notepad++.

Po instalacji uruchom ponownie Notepad++. Pliki `.hpppl` i `.ppl` otworzą się jako HP PPL. Dla innych plików wybierz *Język › HP PPL*.

### Włącz podpowiedzi

*Ustawienia › Preferencje › Autouzupełnianie*:
- ✔ *Włącz autouzupełnianie przy każdym wprowadzeniu*: **Uzupełnianie funkcji** (albo funkcji i słów),
- ✔ *Podpowiedź parametrów funkcji podczas wprowadzania*.

## Sprawdzanie błędów (NppExec)

1. Zainstaluj wtyczkę **NppExec**: *Wtyczki › Admin wtyczek › NppExec › Instaluj*.
2. *Wtyczki › NppExec › Execute NppExec Script…* (F6), wklej skrypt i zapisz go (*Save…*) jako `PPL check`:
   ```
   NPP_SAVE
   "$(SYS.LOCALAPPDATA)\hp-ppl\ppl.exe" check "$(FULL_CURRENT_PATH)"
   ```
3. Klikalne błędy: *Wtyczki › NppExec › Console Output Filters… › Highlight* i dodaj maski:
   - `%ABSFILE%:%LINE%:%CHAR%: error*`: kolor czerwony (R = FF),
   - `%ABSFILE%:%LINE%:%CHAR%: warning*`: kolor pomarańczowy (R = FF, G = 80).

   Podwójne kliknięcie błędu w konsoli przeniesie kursor do tej linii.
4. Skrót klawiszowy: *Wtyczki › NppExec › Advanced Options… › Menu item* dodaj `PPL check`, a potem przypisz klawisz (np. F7) w *Ustawienia › Mapowanie skrótów › Polecenia wtyczek*.
5. Jeśli polskie znaki w konsoli są nieczytelne: *Wtyczki › NppExec › Console Output… › UTF-8*.

Przykładowy wynik:
```
C:\prog\SNAKE.hpppl:12:5: error: Brak END zamykającego IF z linii 9. [missing-end]
C:\prog\SNAKE.hpppl:20:3: warning: Użyto '=' w warunku. Do porównania służy '=='. [equals-in-condition]
1 plik(ów): 1 błąd(ów), 1 ostrzeżeń
```

## Asystent AI w Notepad++?

Notepad++ nie ma dobrej integracji z agentami AI. Jeśli zależy Ci na asystencie (Claude, ChatGPT, Gemini, DeepSeek), użyj rozszerzenia VS Code z katalogu `tools/vscode`. Możesz też korzystać z Claude Code / Gemini CLI w terminalu razem z serwerem MCP `ppl mcp` (zob. `tools/README.md`) i nadal edytować pliki w Notepad++.
