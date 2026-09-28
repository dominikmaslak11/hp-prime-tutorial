# Rozdział 5. Instrukcje warunkowe: IF, when, CASE, IFERR

[← Poprzedni](04-wyjscie.md) · [Spis treści](../README.md) · [Następny →](06-petle.md)

Wszystkie konstrukcje z tego rozdziału znajdziesz w edytorze pod **(Tmplt) › Branch**:
1. IF THEN, 2. IF THEN ELSE, 3. CASE, 4. IFERR, 5. IFERR ELSE.

W HP PPL warunek jest **prawdziwy**, gdy ma wartość różną od zera, i **fałszywy**, gdy jest równy 0 (*UG s. 627*).

---

## 5.1. IF THEN (*UG s. 627*)

```
IF warunek THEN
  polecenia;
END;
```

Polecenia wykonają się tylko wtedy, gdy warunek jest prawdziwy.

## 5.2. IF THEN ELSE (*UG s. 627, ES #2*)

```
IF warunek THEN
  polecenia gdy prawda;
ELSE
  polecenia gdy fałsz;
END;
```

Przykład: QROOTS z [rozdziału 4](04-wyjscie.md#43-print-i-terminal-ug-s-654-es-2).

### Zagnieżdżanie i „else if”

PPL nie ma słowa `ELSEIF`. Kolejne warunki zagnieżdża się w gałęzi `ELSE`. Każde `IF` wymaga własnego `END`:

```
EXPORT OCENA(p)
BEGIN
  IF p>=90 THEN
    RETURN "bardzo dobry";
  ELSE
    IF p>=75 THEN
      RETURN "dobry";
    ELSE
      IF p>=50 THEN
        RETURN "dostateczny";
      ELSE
        RETURN "niedostateczny";
      END;
    END;
  END;
END;
```

Przy wielu warunkach czytelniej jest użyć `CASE` (sekcja 5.4).

### IF na listach (*UG s. 628*)

Jeśli warunek jest listą, `IF` działa na każdym elemencie osobno. Obie gałęzie muszą wtedy zwrócić pojedynczy obiekt albo listy tej samej długości co lista warunków. Dla każdego elementu wybierana jest wartość z gałęzi THEN albo ELSE. To zaawansowana możliwość, w praktyce rzadko używana.

## 5.3. `when` — warunek jako wyrażenie (*ES #2*)

Gdy obie gałęzie to pojedyncze wartości, wygodniej użyć funkcji `when` (pisanej **małymi literami**, dostępnej w katalogu [Toolbox] (Catlg)):

```
when(warunek, wartość_gdy_prawda, wartość_gdy_fałsz)
```

`when` jest funkcją, więc można jej użyć wewnątrz wyrażenia.

Program DOESTAX (*ES #2*): podatek 8,5% nalicza się, gdy cena przekracza 10 $.

```
EXPORT DOESTAX(N)
BEGIN
  N := when(N>10, N*1.085, N);
  RETURN N;
END;
```

`DOESTAX(8)` → 8, `DOESTAX(12)` → 13.02.

Inny przykład: `A := when(FP(A/2)==0, A, A+1);` zamienia nieparzyste A na najbliższą większą liczbę parzystą.

## 5.4. CASE — wybór spośród wielu przypadków (*UG s. 628, ES #4*)

```
CASE
  IF test1 THEN polecenia1; END;
  IF test2 THEN polecenia2; END;
  ...
  DEFAULT polecenia_domyślne;
END;
```

- Warunki są sprawdzane po kolei. Wykonuje się **tylko pierwsza** gałąź z prawdziwym warunkiem, po czym `CASE` się kończy.
- `DEFAULT` jest opcjonalne i wykonuje się, gdy żaden warunek nie jest prawdziwy.
- Maksymalnie **127 gałęzi** (*UG s. 628*).
- Każde `IF … THEN …` wewnątrz `CASE` kończy się własnym `END;`, a cały `CASE` jeszcze jednym `END;`.

Przykład z instrukcji:

```
CASE
  IF A<0 THEN RETURN "negative"; END;
  IF 0<=A AND A<=1 THEN RETURN "small"; END;
  DEFAULT RETURN "large";
END;
```

Program AREAC (*ES #4*) oblicza pole koła, pierścienia lub wycinka koła:

```
EXPORT AREAC()
BEGIN
  LOCAL C,R,S,θ,A;
  CHOOSE(C,"Areas","1. Circle","2. Ring","3. Sector");
  INPUT(R,"Input Radius","R =");

  CASE
    IF C==1 THEN
      A := π*R^2;
    END;

    IF C==2 THEN
      INPUT(S,"Small Radius","r =");
      A := π*(R^2-S^2);
    END;

    IF C==3 THEN
      INPUT(θ,"Angle","θ =");
      // w trybie stopni zamień kąt na radiany
      IF HAngle==1 THEN
        θ := θ*π/180;
      END;
      A := θ*R^2/2;
    END;
  END;

  MSGBOX("Area is "+A);
  RETURN A;
END;
```

Wyniki dla R = 2,5, r = 1,5, θ = π/4 (lub 45°): koło 19.6349540849, pierścień 12.5663706144, wycinek 2.45436926062.

## 5.5. IFERR — przechwytywanie błędów (*UG s. 628*)

```
IFERR
  polecenia_mogące_wywołać_błąd;
THEN
  polecenia_gdy_wystąpił_błąd;
END;

IFERR
  polecenia;
THEN
  gdy_błąd;
ELSE
  gdy_bez_błędu;
END;
```

- Jeśli w bloku `IFERR` wystąpi błąd (dzielenie przez zero, zły argument, zły typ), program **nie zatrzyma się z komunikatem**, tylko przejdzie do gałęzi `THEN`.
- Według instrukcji numer błędu trafia do zmiennej `Ans` i można go użyć w gałęzi `THEN` (*UG s. 628*). Nowsze firmware udostępniają też tekst błędu. Sprawdź to na swoim egzemplarzu w debugerze.

Przykład: bezpieczne dzielenie.

```
EXPORT BEZPDZIEL(a,b)
BEGIN
  LOCAL w;
  IFERR
    w := a/b;
  THEN
    MSGBOX("Błąd obliczeń!");
    RETURN "brak wyniku";
  ELSE
    RETURN w;
  END;
END;
```

> Uwaga: `1/0` w Home może zwrócić ∞ zamiast błędu (zależy od ustawień i trybu). `IFERR` łapie przypadki, które naprawdę generują błąd, np. `ASIN(2)` przy wyłączonym `HComplex` albo zły typ argumentu.

Zastosowanie praktyczne: sprawdzanie danych od użytkownika i funkcje, które mogą zawieść (np. `ROOT` bez rozwiązania).

## 5.6. Typowe błędy

| Błąd | Poprawnie |
|---|---|
| `IF x=5 THEN` | `IF x==5 THEN` |
| brak `END;` po `IF` | każde `IF` ma swoje `END;` |
| `IF a<b<c THEN` | `IF a<b AND b<c THEN` |
| średnik po `THEN` (`THEN;`) | zbędny, choć zwykle nieszkodliwy; nie pisz go |
| `ELSE IF … END;` z jednym `END` | zagnieżdżone `IF` wymaga dodatkowego `END;` |

---

## Sprawdź się

1. Napisz `TROJKAT(a,b,c)`, który zwraca „nie istnieje”, „równoboczny”, „równoramienny” lub „różnoboczny”. Użyj `CASE`.
2. Napisz `ABSW(x)` bez funkcji `ABS`, używając `when`.
3. Napisz `ODWR(x)`, który zwraca 1/x, a przy x = 0 wyświetla komunikat i zwraca 0. Zrób dwie wersje: z `IF` i z `IFERR`.

[Rozwiązania →](25-cwiczenia.md#rozdział-5)
