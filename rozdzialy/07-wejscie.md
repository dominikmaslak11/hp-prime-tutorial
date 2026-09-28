# Rozdział 7. Pobieranie danych: INPUT, CHOOSE, EDITLIST, EDITMAT, KILL

[← Poprzedni](06-petle.md) · [Spis treści](../README.md) · [Następny →](08-funkcje.md)

Program może dostać dane na trzy sposoby:
1. **przez parametry** (`EXPORT F(a,b)`), co wymaga wpisania wywołania w Home,
2. **przez okna dialogowe** (`INPUT`, `CHOOSE`, edytory), omówione w tym rozdziale,
3. **przez klawiaturę i dotyk w czasie rzeczywistym** (`GETKEY`, `MOUSE`), omówione w [rozdziale 17](17-klawiatura-dotyk-czas.md).

Komendy z tego rozdziału są w **(Cmds) › I/O**.

---

## 7.1. INPUT — jedna zmienna (*UG s. 653, ES #3*)

```
INPUT(zmienna [, "tytuł"] [, "etykieta"] [, "pomoc"] [, wartość_reset] [, wartość_początkowa]);
```

| Argument | Znaczenie |
|---|---|
| `zmienna` | gdzie zapisać wpisaną wartość (lokalna lub globalna) |
| `"tytuł"` | nagłówek okna |
| `"etykieta"` | opis przed polem, np. `"R ="` |
| `"pomoc"` | tekst podpowiedzi na dole ekranu |
| `wartość_reset` | wartość wstawiana po naciśnięciu [Shift][Esc] (Clear) na polu |
| `wartość_początkowa` | wartość widoczna w polu na starcie |

- Wszystkie argumenty poza zmienną są opcjonalne, ale **kolejność musi zostać zachowana**. Jeśli chcesz podać późniejszy argument, a pominąć wcześniejszy, wstaw pusty tekst `""` albo pustą listę `{}` (*ES #3*).
- **Wartość zwracana:** 1, gdy użytkownik naciśnie OK, i 0 przy Cancel (*UG s. 653*). Po Cancel zmienna **nie jest aktualizowana**.

> Tutorial Shore'a podaje, że Cancel zapisuje 0 w zmiennej. W instrukcji HP stoi, że zmienna nie zmienia wartości, a zwracane jest 0. Zawsze sprawdzaj wartość zwracaną przez `INPUT`. Wtedy program działa poprawnie w obu przypadkach.

Wzorzec z obsługą przycisku Cancel:

```
EXPORT POLE_KOLA()
BEGIN
  LOCAL r;
  IF NOT INPUT(r, "Pole koła", "r =", "Podaj promień w cm", 1, 1) THEN
    RETURN "Anulowano";      // użytkownik nacisnął Cancel
  END;
  RETURN π*r^2;
END;
```

## 7.2. INPUT — wiele zmiennych (*UG s. 653, ES #3–4*)

```
INPUT({z1, z2, ...}, "tytuł", {"etyk1", "etyk2", ...}, {"pomoc1", ...},
      {reset1, ...}, {pocz1, ...});
```

Program TERMVEL (*ES #4*) oblicza prędkość graniczną spadającego ciała:

```
EXPORT TERMVEL()
BEGIN
  LOCAL L0:={9.80665,32.174},          // g: SI, jednostki angielskie
        L1:={1.225,.0765},             // gęstość powietrza
        L2:={.47,1.05,1.15,.04},       // współczynniki oporu kształtów
        C,K,M,A,T;

  CHOOSE(C,"Units","SI","English");
  CHOOSE(K,"Type of Object","Sphere","Cube","Cylinder","Tear-Shaped");
  INPUT({M,A},"Object",{"M=","A="},{"Mass","Surface Area"});

  T := √((2*M*L0(C))/(L1(C)*A*L2(K)));
  MSGBOX("Terminal Velocity="+T);
  RETURN T;
END;
```

Przykłady: kula, SI, M = 0,05 kg, A = 0,0028 m² → T ≈ 24,664 m/s. Sześcian, jednostki angielskie, M = 1,2 lb, A = 0,3403 ft² → T ≈ 53,15 ft/s.

Pomijanie argumentów pustą listą. Kolejność argumentów jest zawsze taka sama: **zmienne, tytuł, etykiety, pomoc, wartości reset, wartości początkowe**. Jeśli nie chcesz własnych etykiet ani pomocy, ale chcesz ustawić wartości reset i początkowe, wstaw puste listy:

```
INPUT({A,B,C,D,E,F},
      "Ax^2+By^2+Cxy+Dx+Ey+F",
      {},                     // etykiety: brak (zostaną nazwy zmiennych)
      {},                     // pomoc: brak
      {0,0,0,0,0,0},          // wartości reset
      {0,0,0,0,0,0});         // wartości początkowe
```

> W programie CONIC (*ES #5*) jedna pusta lista stoi na pozycji etykiet, a dwie listy zer na pozycjach pomocy i reset. Komentarz autora sugeruje jednak, że chodziło o pustą pomoc oraz wartości reset i początkowe. Wersja powyżej robi dokładnie to.

## 7.3. INPUT — pole wyboru (lista rozwijana) (*ES #3, UG s. 653*)

```
INPUT({{zmienna, {"opcja1","opcja2",...}}}, "tytuł", "etykieta", ...);
```

Zwróć uwagę na **podwójne nawiasy klamrowe**. Po wyborze zmienna zawiera **numer** wybranej opcji (1, 2, …).

Program EARTHCOORD (*ES #3*) podaje przybliżone współrzędne geograficzne wybranego miasta:

```
EXPORT EARTHCOORD()
BEGIN
  LOCAL L1,L2,L3,I;
  L1 := {34.2166666667, 41.9, -33.8599722222, 35.6895055556};   // szerokości
  L2 := {-118.25, 12.5, 151.211111111, 139.708366667};          // długości
  L3 := {"Los Angeles","Rome","Sydney","Tokyo"};

  INPUT({{I,L3}},"Lat/Long","City:");
  RETURN {→HMS(L1(I)), →HMS(L2(I))};    // stopnie, minuty, sekundy
END;
```

## 7.4. INPUT — pole wyboru tak/nie (checkbox) i przyciski radiowe (*ES #3, UG s. 653*)

```
INPUT({{zmienna, 1}}, ...)     // checkbox: zmienna = 0 lub 1
INPUT({{z1, n}, {z2, n}, ...}) // n>1: n kolejnych pól tworzy grupę radiową
```

Jeśli liczba jest większa od 1, to pole i kolejne n-1 pól tworzą grupę **radiową**: zaznaczone może być tylko jedno z nich (*UG s. 653*).

Program DOESTAX2 (*ES #3*) liczy zakupy trzech towarów z zaznaczeniem, które są opodatkowane:

```
EXPORT DOESTAX2()
BEGIN
  LOCAL item1, item2, item3;
  LOCAL ck1, ck2, ck3;
  LOCAL rate, total;

  INPUT({item1,{ck1,1},item2,{ck2,1},item3,{ck3,1},rate},
        "Check Out",
        {"Item1:","Tax1:","Item2:","Tax2:","Item3:","Tax3:","Rate:"});

  rate  := rate/100;
  total := item1*(1+ck1*rate)
         + item2*(1+ck2*rate)
         + item3*(1+ck3*rate);
  RETURN total;
END;
```

Przykład: 59,99 (opodatkowany), 9,99 (opodatkowany), 10,00 (bez podatku), stawka 9% → **86,28**.

## 7.5. INPUT — ograniczenie typu i pozycja pól (*UG s. 653*)

Każdy element listy zmiennych może być listą z dodatkowymi informacjami:

```
{zmienna, [dozwolone_typy], {pozycja}}      // pole edycyjne
{zmienna, liczba, {pozycja}}                // checkbox / radio
{zmienna, {"opcje",...}, {pozycja}}         // lista rozwijana
```

- **dozwolone_typy** to wektor kodów typów (jak w `TYPE`), np. `[0]` tylko liczby rzeczywiste, `[2]` tylko tekst, `[-1]` wszystkie typy. Jeśli dozwolony jest wyłącznie tekst, cudzysłowy nie są wyświetlane podczas edycji.
- **pozycja** to `{początek_pola_w_%_ekranu, szerokość_w_%, linia}` (linie liczone od 0). Pozycję trzeba podać **dla wszystkich pól albo dla żadnego**.
- Na stronie mieści się **7 linii**. Kolejne pola trafiają na następne strony, a tytuł może być wtedy listą tytułów stron.

Przykład formularza z tekstem, liczbą i wyborem:

```
EXPORT ANKIETA()
BEGIN
  LOCAL imie:="", wiek:=18, plec:=1, zgoda:=0;
  IF INPUT({{imie,[2],{20,70,0}},
            {wiek,[0],{20,20,1}},
            {plec,{"K","M","inna"},{20,30,2}},
            {zgoda,1,{20,10,3}}},
           "Ankieta",
           {"Imię:","Wiek:","Płeć:","Zgoda:"}) THEN
    RETURN {imie, wiek, plec, zgoda};
  END;
  RETURN "anulowano";
END;
```

## 7.6. CHOOSE — okno wyboru (*UG s. 651, ES #4*)

```
CHOOSE(zmienna, "tytuł", "opcja1", "opcja2", ..., "opcjaN");  // do 14 opcji
CHOOSE(zmienna, "tytuł", {"opcja1", "opcja2", ...});          // dowolnie wiele
```

- Zmienna dostaje numer wybranej pozycji (1, 2, …) albo **0**, gdy użytkownik anuluje.
- Funkcja zwraca wartość różną od zera przy wyborze i 0 przy anulowaniu.

Przykład z instrukcji (*UG s. 651*):

```
CHOOSE(N,"PickHero","Euler","Gauss","Newton");
IF N==1 THEN
  PRINT("You picked Euler");
ELSE
  IF N==2 THEN
    PRINT("You picked Gauss");
  ELSE
    PRINT("You picked Newton");
  END;
END;
```

Menu programu w pętli, bardzo częsty wzorzec:

```
EXPORT MENU_GLOWNE()
BEGIN
  LOCAL w;
  REPEAT
    CHOOSE(w, "Menu", {"Pole koła","Tabliczka","Losuj","Koniec"});
    CASE
      IF w==1 THEN MSGBOX(POLE_KOLA()); END;
      IF w==2 THEN MSGBOX("tu tabliczka"); END;
      IF w==3 THEN MSGBOX(RANDINT(1,100)); END;
    END;
  UNTIL w==0 OR w==4;
END;
```

(`POLE_KOLA` to program z sekcji 7.1. Musi być zapisany i eksportowany.)

## 7.7. EDITLIST i EDITMAT (*UG s. 652*)

```
EDITLIST(L1);                    // otwiera edytor listy, program czeka na (OK)
EDITMAT(M1);                     // edytor macierzy
EDITMAT(M1, "Tytuł");
EDITMAT(M1, {"Tytuł", {"w1","w2"}, {"k1","k2"}});   // nazwy wierszy i kolumn
EDITMAT(M1, "Podgląd", 1);       // tylko do odczytu
```

Oba edytory pozwalają wygodnie wpisać wiele danych naraz, np. wyniki pomiarów. `EDITMAT` zwraca macierz, więc działa też z macierzą lokalną: `m := EDITMAT(m);`.

## 7.8. KILL (*UG s. 627, ES #3*)

```
KILL;
```

Natychmiast kończy **cały** program, także gdy wywołano go z innej funkcji. W debugerze przerywa pracę krok po kroku. Stosuj go ostrożnie: program przerwany przez `KILL` nie przywróci zmienionych ustawień (np. `HAngle`).

---

## Sprawdź się

1. Napisz `BMI()`, który jednym oknem `INPUT` pobiera masę i wzrost, a wynik z opisem („niedowaga”, „norma”, „nadwaga”, „otyłość”) pokazuje w `MSGBOX`. Obsłuż Cancel.
2. Napisz przelicznik temperatur: `CHOOSE` z opcjami °C→°F, °F→°C, °C→K, potem `INPUT` wartości.
3. Napisz formularz z trzema checkboxami, na przykład „masło”, „ser”, „szynka”, który zwraca cenę kanapki (bazowo 3 zł, dodatki po 1,50 zł).

[Rozwiązania →](25-cwiczenia.md#rozdział-7)
