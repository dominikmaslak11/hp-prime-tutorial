# Materiały zewnętrzne

## hp-prime-kit (Jordi Rigau), licencja MIT

Źródło: https://github.com/JordiRigau/hp-prime-kit (fork: https://github.com/Insoft-UK/hp-prime-kit)

Wykorzystano:
- `data/code.hpprgm`: szablon kontenera programu zapisany przez HP Connectivity Kit (plik `templates/code.hpprgm` z hp-prime-kit),
- wiedzę o formacie `.hpprgm` (`docs/topics/formats.md`, `hpkit/program.py`). Implementacja w `core/hpprgm.cpp` jest przepisana na C++ i daje pliki bajt w bajt identyczne z `hpprime write`,
- `data/hp-names.tsv`: lista wszystkich nazw PPL z pomocy HP (plik `docs/commands/names.tsv` z hp-prime-kit: nazwa, rodzaj, grupa, menu, składnia). Uzupełnia bazę poleceń o nazwy bez opisu w tutorialu, żeby walidator nie zgłaszał ich jako nieznanych,
- `tests/data/hp-prime-kit-results.tsv`: wyniki przykładów zmierzone przez autorów hp-prime-kit na HP Prime Virtual Calculator 2.4 (używane jako test zgodności symulatora).

```
MIT License

Copyright (c) 2026 Jordi Rigau

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
