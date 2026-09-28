# Rozdział 12. Liczby całkowite, systemy liczbowe i operacje bitowe

[← Poprzedni](11-macierze.md) · [Spis treści](../README.md) · [Następny →](13-cas-i-analiza.md)

Opis w instrukcji: rozdział 29 „Basic integer arithmetic” (*UG s. 696–701*) oraz komendy **Cmds › Integer** (*UG s. 648–651*).

---

## 12.1. Liczby z `#` i znaczniki systemu

Liczbę całkowitą w arytmetyce binarnej poprzedza znak `#`, a system oznacza przyrostek:

| Przyrostek | System | Przykład | Wartość dziesiętna |
|---|---|---|---|
| `b` | dwójkowy | `#1101b` | 13 |
| `o` | ósemkowy | `#17o` | 15 |
| `d` | dziesiętny | `#99d` | 99 |
| `h` | szesnastkowy | `#E4h` | 228 |
| brak | system domyślny | `#1101` | zależy od ustawień |

Zasady (*UG s. 696–698*):
- **Wynik dzielenia jest obcinany do części całkowitej**: `#100b/#11b` = `#1b`.
- **Rozmiar słowa** (*wordsize*) wynosi 1–64 bity, domyślnie 32. Starsze bity, które się nie mieszczą, są odrzucane.
- **System wyniku przy mieszanych systemach** to system **pierwszego** argumentu: `#4h*#71o` → `#E4h`, a `#71o*#4h` → `#344o`.
- Jeśli jeden z argumentów nie ma `#`, wynik jest zwykłą liczbą dziesiętną.
- Domyślny system ustawiasz w ustawieniach Home (pole *Integers*). Fabrycznie jest to szesnastkowy.

Przykłady z instrukcji:

| Działanie | Wynik | Dziesiętnie |
|---|---|---|
| `#10000b+#10100b` | `#100100b` | 16+20 = 36 |
| `#71o-#10100b` | `#45o` | 57−20 = 37 |
| `#32Ah/#5o` | `#A2h` | 810/5 = 162 |

## 12.2. Konwersje

| Komenda | Działanie | Przykład |
|---|---|---|
| `B→R(#n)` | liczba z `#` na zwykłą liczbę rzeczywistą | `B→R(#1101b)` → 13 |
| `R→B(n)` | liczba rzeczywista na liczbę z `#` w systemie domyślnym | `R→B(13)` → `#Dh` (przy domyślnym hex) |
| `SETBASE(#n, c)` | pokazuje liczbę w innym systemie: c = 1 dwójkowy, 2 ósemkowy, 3 szesnastkowy | `SETBASE(#34o,1)` → `#11100b` |
| `GETBASE(#n)` | system liczby: 0 domyślny, 1 bin, 2 okt, 3 hex | |
| `SETBITS(#n, bity)` | ustala liczbę bitów reprezentacji (-63…64) | `SETBITS(#1111b,15)` → `#1111:15b` |
| `GETBITS(#n)` | liczba bitów użytych do zapisu | `GETBITS(#22122)` → 32 |

## 12.3. Operacje bitowe

| Komenda | Działanie | Przykład → wynik |
|---|---|---|
| `BITAND(a,b,...)` | iloczyn bitowy | `BITAND(20,13)` → 4 |
| `BITOR(a,b,...)` | suma bitowa | `BITOR(9,26)` → 27 |
| `BITXOR(a,b,...)` | różnica symetryczna | `BITXOR(9,26)` → 19 |
| `BITNOT(a)` | negacja (w bieżącym rozmiarze słowa) | `BITNOT(47)` → 549755813840 |
| `BITSL(a[,n])` | przesunięcie w lewo o n bitów (domyślnie 1) | `BITSL(28,2)` → 112; `BITSL(5)` → 10 |
| `BITSR(a[,n])` | przesunięcie w prawo | `BITSR(112,2)` → 28; `BITSR(10)` → 5 |

Operacje bitowe działają także na zwykłych liczbach całkowitych, bez `#`.

## 12.4. Zmienne ustawień (*UG s. 693*)

| Zmienna | Wartości |
|---|---|
| `Base` | 0 bin, 1 okt, 2 dec, 3 hex |
| `Bits` | rozmiar słowa (1–64) |
| `Signed` | 0 bez znaku, 1 ze znakiem |

## 12.5. Zastosowania w programach

### Maski bitowe i flagi

```
// flagi opcji w jednej liczbie: bit 0 = dźwięk, bit 1 = siatka, bit 2 = kolor
EXPORT FLAGI(f)
BEGIN
  PRINT();
  PRINT("Dźwięk: "+when(BITAND(f,1)<>0,"tak","nie"));
  PRINT("Siatka: "+when(BITAND(f,2)<>0,"tak","nie"));
  PRINT("Kolor:  "+when(BITAND(f,4)<>0,"tak","nie"));
END;
```

`FLAGI(5)`: dźwięk tak, siatka nie, kolor tak.

### Liczenie ustawionych bitów

```
EXPORT POPCOUNT(n)
BEGIN
  LOCAL c := 0;
  WHILE n > 0 DO
    c := c + BITAND(n,1);
    n := BITSR(n,1);
  END;
  RETURN c;
END;
```

### Kolory

Kolor to liczba całkowita w formacie `#RRGGBBh` (zob. [rozdział 14](14-grafika-podstawy.md)). Operacjami bitowymi rozłożysz go na składowe:

```
r := BITAND(BITSR(kolor,16), 255);
g := BITAND(BITSR(kolor,8), 255);
b := BITAND(kolor, 255);
```

### Edytor liczby całkowitej

W Home zaznacz wynik z `#` i otwórz edytor liczb całkowitych skrótem **Base** (dokładna kombinacja klawiszy jest pokazana na ilustracji w *UG s. 699*). Pokazuje wartość w systemach hex i dec oraz bit po bicie. Pozwala przesuwać bity, zmieniać rozmiar słowa, liczyć uzupełnienie do dwóch (Neg) i przełączać system (*UG s. 699–700*).

---

## Sprawdź się

1. Napisz `RGBROZ(k)`, który zwraca listę `{r,g,b}` dla koloru k.
2. Napisz `PARZYSTOSC(n)`, który zwraca 1, gdy liczba ustawionych bitów jest nieparzysta (bit parzystości).
3. Oblicz ręcznie, potem sprawdź na kalkulatorze: `#FFh + #1b`, `#7o * #10b`, `BITXOR(#F0h,#FFh)`.

[Rozwiązania →](25-cwiczenia.md#rozdział-12)
