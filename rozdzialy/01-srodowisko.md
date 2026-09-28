# Rozdział 1. Środowisko — kalkulator oczami programisty

[← Spis treści](../README.md) · [Następny: Składnia i zmienne →](02-skladnia-i-zmienne.md)

W tym rozdziale:
- poznasz te miejsca w kalkulatorze, które są ważne dla programisty,
- nauczysz się tworzyć, zapisywać, sprawdzać, uruchamiać i usuwać programy,
- napiszesz i uruchomisz pierwszy program na cztery różne sposoby.

---

## 1.1. Co to jest HP PPL

HP Prime ma dwa języki:
- **HP PPL** (*HP Prime Programming Language*) — główny język kalkulatora, podobny do Pascala: bloki `BEGIN … END`, pętle `FOR … DO … END`, średniki na końcu poleceń. Ten kurs dotyczy PPL.
- **Python** (od firmware 2020+, w aplikacji *Python*) — osobne środowisko, nie jest opisane w żadnym z dwóch źródeł.

Program PPL to tekst: ciąg poleceń wykonywanych automatycznie (*UG s. 552*). Polecenia oddziela się średnikiem `;`. Argumenty komend podaje się w nawiasach, rozdzielone przecinkami:

```
PIXON(xposition, yposition);
```

Część argumentów jest opcjonalna. Jeśli ich nie podasz, komenda użyje wartości domyślnych. W opisach składni (w instrukcji i w tym kursie) argumenty opcjonalne są w nawiasach kwadratowych:

```
PIXON([G,] xposition, yposition [,color]);
```

Kilka komend przyjmuje argument bez nawiasów, np. `RETURN wyrażenie;`.

## 1.2. Najważniejsze miejsca kalkulatora

| Miejsce | Jak otworzyć | Do czego przyda się programiście |
|---|---|---|
| **Home** | [Home] | Uruchamianie programów, testowanie wyrażeń, podgląd zmiennych. |
| **CAS** | [CAS] | Obliczenia symboliczne; tu też można uruchamiać programy. |
| **Katalog programów** | [Shift] [1] (Program) | Tworzenie, edycja, uruchamianie, debugowanie, wysyłanie programów. |
| **Toolbox** | [Toolbox] (klawisz ze skrzynką narzędzi) | Menu (Math), (CAS), (App), (User), (Catlg) — wszystkie funkcje kalkulatora; w (User) są Twoje programy. |
| **Vars** | [Vars] | Zmienne: (Home), (App), (CAS), (User). |
| **Template** | [Template] | Szablony matematyczne: pochodna, całka, suma, macierz itd. |
| **Terminal** | przytrzymaj [On] i naciśnij [÷] | Tekst wypisany komendą `PRINT`. |
| **Pomoc** | [Help] | Pomoc do podświetlonej komendy — działa też w edytorze programów. |
| **Ustawienia Home** | [Shift] [Home] (Settings) | Tryb kątów, format liczb, tryb wprowadzania, system liczbowy. |
| **Biblioteka aplikacji** | [Apps] | Aplikacje HP i Twoje własne aplikacje. |

### Tryby wprowadzania

W ustawieniach Home (pole *Entry*) wybierasz jeden z trzech trybów. Od trybu zależy sposób uruchamiania programów z argumentami:

| Tryb | Jak uruchomić program `TEST` z 4 argumentami |
|---|---|
| **Textbook** (podręcznikowy, domyślny) | `TEST(1,2,3,4)` [Enter] |
| **Algebraic** | `TEST(1,2,3,4)` [Enter] |
| **RPN** | Wpisz argumenty na stos (każdy zatwierdź [Enter]), potem `TEST(4)`, gdzie 4 to liczba argumentów (*ES #1*). |

Stos w trybie RPN przed wywołaniem `TEST(4)`:
```
4: argument_1
3: argument_2
2: argument_3
1: argument_4
```

## 1.3. Katalog programów (Program Catalog)

Otwierasz go przez **[Shift] [1]** (*UG s. 553*). Widać w nim listę programów. Na górze listy jest zawsze pozycja o nazwie aktywnej aplikacji, np. *Function*. To **program aplikacji**, omówiony w [rozdziale 19](19-wlasne-aplikacje.md).

Przyciski katalogu (*UG s. 554*):

| Przycisk | Działanie |
|---|---|
| (Edit) | Otwiera zaznaczony program w edytorze. |
| (New) | Pyta o nazwę i tworzy nowy program. |
| (More) | Dodatkowe opcje: **Save** (kopia pod nową nazwą), **Rename**, **Sort** (alfabetycznie/chronologicznie), **Delete**, **Clear** (usuwa wszystkie programy). |
| (Send) | Wysyła program do innego HP Prime (kablem USB). |
| (Debug) | Uruchamia program w debugerze. |
| (Run) | Uruchamia program. |
| [Shift][▲] / [Shift][▼] | Skok na początek/koniec listy. |
| [Del] | Usuwa zaznaczony program. |
| [Shift][Esc] (Clear) | Usuwa wszystkie programy. |

## 1.4. Tworzenie programu krok po kroku

1. Naciśnij **[Shift] [1]**, a potem **(New)**.
2. Wpisz nazwę. Litery wpisujesz klawiszem **[ALPHA]** i klawiszem z daną literą:
   - [ALPHA] raz: jedna wielka litera,
   - [ALPHA] [ALPHA]: blokada wielkich liter (*alpha-lock*),
   - [ALPHA] [Shift] [ALPHA]: blokada małych liter,
   - kolejne [ALPHA] wyłącza blokadę.
3. Naciśnij [Enter] (lub (OK)). Kalkulator utworzy szablon:

```
EXPORT MYPROGRAM()
BEGIN

END;
```

### Zasady nazywania programów (*UG s. 555, ES #1*)

- dozwolone są tylko litery (także greckie), cyfry i podkreślnik `_`,
- nazwa musi zaczynać się od litery,
- nie może to być nazwa zarezerwowana (np. `M1`, `L2`, `Xmin`, `SIN`).

| Nazwa | Poprawna? | Dlaczego |
|---|---|---|
| `TEST123` | tak | |
| `TEST_123` | tak | |
| `Spin2` | tak | |
| `123TEST` | nie | zaczyna się od cyfry |
| `HOT STUFF` | nie | zawiera spację |
| `2Cool!` | nie | cyfra na początku i znak `!` |
| `M1` | nie | nazwa zarezerwowana dla macierzy |

## 1.5. Edytor programów

Przyciski i klawisze edytora (*UG s. 556–558*):

| Przycisk / klawisz | Działanie |
|---|---|
| **(Check)** | Sprawdza składnię i pokazuje miejsce pierwszego błędu. Jeśli błędów nie ma: *No errors in the program*. |
| **(Tmplt)** | Szablony struktur: **Block** (BEGIN END, RETURN, KILL), **Branch** (IF…, CASE, IFERR), **Loop** (FOR…, WHILE, REPEAT, BREAK, CONTINUE), **Variable** (LOCAL, EXPORT), **Function** (EXPORT, VIEW, KEY). |
| **(Cmds)** | Komendy: **Strings**, **Drawing**, **Matrix**, **App Functions**, **Integer**, **I/O**, **More**. |
| **(Page ◂ ▸)** | Przewijanie długiego programu o ekran w górę lub w dół. |
| [Vars] | Wstawia nazwy zmiennych. |
| [Shift][Vars] (Chars) | Paleta znaków specjalnych: `≠ ≤ ≥ π θ ▶ →` itd. |
| [Shift][◂] / [Shift][▸] | Początek / koniec linii. |
| [Shift][▲] / [Shift][▼] | Początek / koniec programu. |
| [Enter] | Nowa linia. |
| [Del] / [Shift][Del] | Usuwa znak na lewo / na prawo od kursora. |
| [Shift][Esc] (Clear) | Usuwa całą treść programu. |
| [Shift][Menu] | Dodatkowe opcje: **Create user key** (szablon przeprogramowania klawisza, zob. [rozdział 20](20-klawiatura-uzytkownika.md)) oraz **Insert pragma** (wstawia `#pragma mode(...)`, zob. [rozdział 22](22-debugowanie.md)). |

Przydatne skróty klawiszowe:
- średnik `;` to **[ALPHA] [+]**,
- przypisanie `:=` to dwa znaki, dwukropek i znak równości. Oba znajdziesz na klawiaturze (opisy w kolorach Shift/ALPHA) albo w palecie [Shift][Vars] (Chars). Zamiast tego możesz użyć Sto▶ ([Shift][EEX]),
- `//` (komentarz): klawisz **[÷]** dwa razy,
- porównania `< ≤ == ≠ ≥ >` oraz `AND`, `OR`, `NOT` są w menu **[Shift] [6]**.

> **Wskazówka.** Dopóki nie znasz komend na pamięć, wstawiaj je z menu (Tmplt), (Cmds) albo z katalogu [Toolbox] (Catlg). Unikniesz literówek, a szablon podpowie składnię.

## 1.6. Pierwszy program: SQIN (*ES #1*)

Program oblicza funkcję f(x) = 1/x².

```
EXPORT SQIN(X)
BEGIN
  RETURN 1/X^2;
END;
```

Co oznaczają poszczególne linie:
- `EXPORT SQIN(X)` to nagłówek: nazwa funkcji i lista parametrów. `EXPORT` sprawia, że funkcja jest widoczna na zewnątrz programu (w Home, w menu User i w innych programach).
- `BEGIN … END;` to blok, czyli ciało funkcji.
- `RETURN 1/X^2;` zwraca wynik do Home i kończy funkcję.

### Sprawdzanie

Naciśnij **(Check)**. Pamiętaj, że Check sprawdza tylko **składnię**. Nie sprawdza, czy argumenty komend mają sens (*ES #1*): błąd typu „dzielenie przez zero” wyjdzie dopiero podczas działania programu.

### Cztery sposoby uruchomienia

1. **W Home:** wpisz `SQIN(5)` i [Enter]. Wynik: `0.04`.
2. **Z menu User:** [Toolbox] (User), rozwiń `SQIN >`, wybierz `SQIN`, uzupełnij argument i naciśnij [Enter].
3. **Z katalogu programów:** zaznacz program i naciśnij (Run). Jeśli program ma parametry, kalkulator wyświetli formularz do ich wpisania (*UG s. 562*).
4. **W trybie RPN:** `5` [Enter], potem `SQIN(1)`.

Wyniki kontrolne: `SQIN(5)` = `0.04`, `SQIN(36)` = `7.71604938272E-4`.

> **Uruchamianie z katalogu a funkcja START.** Po naciśnięciu (Run) system najpierw szuka w pliku funkcji `START()` bez parametrów (*UG s. 561*). Jeśli plik ma kilka funkcji z `EXPORT`, zobaczysz listę do wyboru.

## 1.7. Drugi program: MOPMT — rata kredytu (*ES #1*)

```
EXPORT MOPMT(L,R,M)
BEGIN
  LOCAL K:=R/1200;              // miesięczna stopa procentowa
  K:=L*K/(1-(1+K)^-M);          // wzór na ratę annuitetową
  RETURN "Payment ="+K;         // łączenie tekstu z liczbą
END;
```

- `L` to kwota kredytu, `R` oprocentowanie roczne w %, `M` liczba miesięcy.
- Parametry `L`, `R`, `M` są automatycznie zmiennymi **lokalnymi**, podobnie jak `K` zadeklarowane przez `LOCAL`. Po zakończeniu programu żadna globalna zmienna nie zmienia wartości, mimo że `L`, `R`, `M` i `K` to też nazwy zmiennych systemowych.
- `"tekst"+liczba` tworzy nowy tekst.

Przykłady:
- `MOPMT(4000, 9.5, 30)` → `"Payment =150.317437565"`
- `MOPMT(370000, 3.5, 360)` → `"Payment =1661.46534383"`

## 1.8. Program z wieloma funkcjami (*UG s. 562*)

Jeden plik programu może zawierać kilka funkcji z `EXPORT`. W menu (User) plik pojawi się wtedy jako „folder” z kilkoma pozycjami. To dobry sposób na zebranie własnych narzędzi w jednym miejscu.

Program `MYFOLDER` (automatycznie utworzony szablon trzeba usunąć):

```
EXPORT FUNCTION1(X)
BEGIN
  RETURN X+1;
END;

EXPORT FUNCTION2(X)
BEGIN
  RETURN X-1;
END;
```

W [Toolbox] (User) pojawi się `MYFOLDER >` z pozycjami `FUNCTION1` i `FUNCTION2`.

## 1.9. Edycja, kopiowanie, usuwanie, udostępnianie (*UG s. 565–567*)

- **Edycja:** w katalogu zaznacz program i naciśnij (Edit) albo [Enter].
- **Kopiowanie fragmentu:** w edytorze naciśnij [Shift][View] (Copy). Pojawią się przyciski (Begin), (End), (All), (Cut), (Copy). Zaznacz fragment, skopiuj go, przejdź do innego programu i wklej przez [Shift][Menu] (Paste).
- **Kopia całego programu:** (More) › Save.
- **Usuwanie programu:** zaznacz i [Del]. **Wszystkie programy:** [Shift][Esc] (Clear).
- **Wyczyszczenie treści** (nazwa zostaje): w edytorze [Shift][Esc].
- **Wysyłanie:** (Send) w katalogu, kablem do drugiego HP Prime.
- **Na komputerze:** darmowy program *HP Connectivity Kit* (Windows/macOS) pozwala pisać programy na PC, kopiować je na kalkulator i robić kopie zapasowe. Emulator *HP Prime Virtual Calculator* wygodnie nadaje się do testów. Kod z tego kursu można wkleić bezpośrednio do edytora Connectivity Kit.

## 1.10. Zapisywanie wartości w zmiennych — dwa sposoby (*ES #1*)

```
4 ▶ A        // Sto▶: [Shift][EEX]; najpierw wartość, potem zmienna
A := 4       // najpierw zmienna, potem wartość
```

Obie formy działają w Home i w programach. W kursie używam `:=`, bo tak jest przyjęte w językach programowania. Instrukcja HP często używa `▶` (np. `MAKELIST(0,X,1,10,1) ▶ L2;`).

---

## Sprawdź się

1. Napisz program `CUBE(X)`, który zwraca X³. Uruchom go na trzy sposoby.
2. Napisz program `CIRC(R)`, który zwraca listę `{obwód, pole}` koła o promieniu R.
3. Utwórz plik `GEOM` z trzema funkcjami: `SQAREA(a)`, `RECTAREA(a,b)`, `TRIAREA(a,h)`. Sprawdź, jak wygląda w menu (User).

[Rozwiązania →](25-cwiczenia.md#rozdział-1)
