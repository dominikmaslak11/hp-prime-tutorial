# Rozdział 20. Klawiatura użytkownika — przeprogramowanie klawiszy

[← Poprzedni](19-wlasne-aplikacje.md) · [Spis treści](../README.md) · [Następny →](21-zmienne-systemowe.md)

Każdemu klawiszowi, także w kombinacji z [Shift] i [ALPHA], możesz przypisać własne działanie (*UG s. 571–576*). Taki zestaw przypisań to **klawiatura użytkownika**.

---

## 20.1. Tryby użytkownika (*UG s. 571*)

| Tryb | Jak włączyć | Oznaczenie | Działanie |
|---|---|---|---|
| tymczasowy | [Shift] [Help] (User) | `1U` na pasku tytułu | tylko **następne** naciśnięcie klawisza korzysta z przypisania |
| stały | [Shift] [Help] dwa razy | `U` na pasku tytułu | przypisania działają, dopóki tryb nie zostanie wyłączony (ponowne [Shift][Help]) |

Klawisz bez przypisania zachowuje się w trybie użytkownika normalnie.

## 20.2. Program przypisujący klawisz

```
KEY K_Sin()
BEGIN
  RETURN "ALOG(";
END;
```

- `KEY` to prefiks oznaczający funkcję klawisza (*UG s. 589*).
- `K_Sin` to wewnętrzna nazwa klawisza. **Wielkość liter ma znaczenie.**
- Zwrócony **tekst** zostaje wstawiony w miejscu kursora, tak jakby użytkownik go wpisał.

Po włączeniu trybu użytkownika naciśnięcie [SIN] wpisze `ALOG(`.

Zwracany tekst może być dowolny (*UG s. 572*): nazwa funkcji systemowej lub Twojej, nazwa zmiennej, całe wyrażenie. W instrukcji jedna z kombinacji z [Shift] i [ALPHA], która normalnie wpisuje małą literę „t”, zostaje przypisana do wpisywania nachylenia F1 w punkcie 3. Nazwę klawisza najlepiej wygenerować opcją *Create user key* (sekcja 20.3):

```
KEY KSA_4()    // nazwa przykładowa: wstaw tu nazwę wygenerowaną przez "Create user key"
BEGIN
  RETURN "SLOPE(F1(X),3)";
END;
```

Ciało funkcji może też **wykonywać program**: rysować, liczyć, zmieniać ustawienia. W praktyce funkcja klawisza może zwrócić -1, co oznacza wykonanie standardowej akcji klawisza. Zachowanie to warto sprawdzić na swoim firmware.

```
// [Shift]+[x²] w trybie U przełącza tryb kątów i pokazuje komunikat
KEY KS_Sq()
BEGIN
  HAngle := when(HAngle==0, 1, 0);
  MSGBOX(when(HAngle==1, "Stopnie", "Radiany"));
  RETURN "";                   // nic nie wpisuj
END;
```

## 20.3. Najszybszy sposób: Create user key (*UG s. 572*)

W edytorze programów naciśnij **[Shift][Menu]** › **Create user key**, a potem klawisz lub kombinację klawiszy do przypisania. Kalkulator wstawi szablon z poprawną nazwą klawisza. Nie musisz jej znać na pamięć.

## 20.4. Nazwy klawiszy (*UG s. 573–576*)

Nazwa ma postać **przedrostek + nazwa klawisza**:

| Przedrostek | Kombinacja |
|---|---|
| `K_` | sam klawisz |
| `KS_` | [Shift] + klawisz |
| `KA_` | [ALPHA] + klawisz |
| `KSA_` | [Shift] + [ALPHA] + klawisz |

Nazwy klawiszy (część po przedrostku):

| | | | | | |
|---|---|---|---|---|---|
| `Apps` | `Symb` | `Up` | `Help` | `Esc` | `Home` |
| `Plot` | `Left` | `Right` | `View` | `Cas` | `Num` |
| `Down` | `Menu` | `Vars_` | `Math` (Toolbox) | `Templ` | `Xttn` |
| `Abc` | `Bksp` | `Power` | `Sin` | `Cos` | `Tan` |
| `Ln` | `Log` | `Sq` | `Neg` | `Paren` | `Comma` |
| `Enter` | `Eex` | `7` | `8` | `9` | `Div` |
| `Alpha` | `4` | `5` | `6` | `Mul` | `1` |
| `2` | `3` | `Minus` | `On` | `0` | `Dot` |
| `Space` | `Plus` | | | | |

Wyjątki według instrukcji: nie ma `KS_Help` (bo [Shift][Help] włącza tryb User) ani `KS_On`. Nie da się też przypisać działania samemu klawiszowi [Shift]. Pełną tabelę znajdziesz w [dodatku B](../dodatki/B-kody-klawiszy.md#nazwy-klawiszy-key).

> Uwaga: instrukcja zapisuje `K_Ente`, ale w kolumnach z przedrostkami `KS_Enter`, `KA_Enter`. Najpewniej to literówka, a poprawna nazwa to `K_Enter`. Użyj opcji *Create user key*, a szablon sam wpisze poprawną nazwę.

## 20.5. Organizacja

- Przypisania klawiszy możesz trzymać w jednym programie, np. `MYKEYS`, z wieloma funkcjami `KEY`.
- Przypisania są aktywne tylko w trybie użytkownika, więc nie przeszkadzają w normalnej pracy.
- Dobre zastosowania: często używane funkcje ukryte głęboko w menu (`ALOG`, `irem`, `CONVERT`), stałe fizyczne, szablony wzorów, szybkie przełączniki ustawień.

---

## Sprawdź się

1. Przypisz [ALPHA]+[SIN] do wpisywania `ASIN(`, [ALPHA]+[COS] do `ACOS(`, [ALPHA]+[TAN] do `ATAN(`.
2. Przypisz [Shift]+[9] do wstawiania stałej grawitacji `9.80665`.
3. Napisz przypisanie, które po naciśnięciu klawisza pokazuje `MSGBOX` z bieżącą godziną i nic nie wpisuje.

[Rozwiązania →](25-cwiczenia.md#rozdział-20)
