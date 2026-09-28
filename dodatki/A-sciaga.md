# Dodatek A. Ściąga — komendy HP PPL w jednym miejscu

[← Spis treści](../README.md)

Kolumna „Menu” pokazuje, gdzie znaleźć komendę w edytorze programów. Ścieżki pochodzą z tutorialu Shore'a (fw 13441) i z instrukcji. W innych wersjach firmware numeracja pozycji w menu może się nieco różnić.

## Struktura (Tmplt)

| Komenda | Składnia | Menu | Rozdział |
|---|---|---|---|
| BEGIN END | `BEGIN polecenia; END;` | Tmplt › 1 Block › 1 | [2](../rozdzialy/02-skladnia-i-zmienne.md) |
| RETURN | `RETURN wyr;` | Tmplt › 1 Block › 2 | [4](../rozdzialy/04-wyjscie.md) |
| KILL | `KILL;` | Tmplt › 1 Block › 3 | [7](../rozdzialy/07-wejscie.md) |
| IF THEN | `IF t THEN p; END;` | Tmplt › 2 Branch › 1 | [5](../rozdzialy/05-warunki.md) |
| IF THEN ELSE | `IF t THEN p1; ELSE p2; END;` | Tmplt › 2 Branch › 2 | [5](../rozdzialy/05-warunki.md) |
| CASE | `CASE IF t1 THEN p1; END; … DEFAULT p; END;` | Tmplt › 2 Branch › 3 | [5](../rozdzialy/05-warunki.md) |
| IFERR | `IFERR p1; THEN p2; [ELSE p3;] END;` | Tmplt › 2 Branch › 4/5 | [5](../rozdzialy/05-warunki.md) |
| FOR | `FOR v FROM a TO b DO p; END;` | Tmplt › 3 Loop › 1 | [6](../rozdzialy/06-petle.md) |
| FOR STEP | `FOR v FROM a TO b STEP k DO p; END;` | Tmplt › 3 Loop › 2 | [6](../rozdzialy/06-petle.md) |
| FOR DOWN | `FOR v FROM a DOWNTO b [STEP k] DO p; END;` | Tmplt › 3 Loop › 3/4 | [6](../rozdzialy/06-petle.md) |
| WHILE | `WHILE t DO p; END;` | Tmplt › 3 Loop › 5 | [6](../rozdzialy/06-petle.md) |
| REPEAT | `REPEAT p; UNTIL t;` | Tmplt › 3 Loop › 6 | [6](../rozdzialy/06-petle.md) |
| BREAK | `BREAK;` / `BREAK(n);` | Tmplt › 3 Loop | [6](../rozdzialy/06-petle.md) |
| CONTINUE | `CONTINUE;` | Tmplt › 3 Loop | [6](../rozdzialy/06-petle.md) |
| LOCAL | `LOCAL a, b:=1;` | Tmplt › 4 Variable › 1 | [2](../rozdzialy/02-skladnia-i-zmienne.md) |
| EXPORT (zmienna) | `EXPORT v [:= w];` | Tmplt › 4 Variable › 2 | [2](../rozdzialy/02-skladnia-i-zmienne.md) |
| EXPORT (funkcja) | `EXPORT F(p) BEGIN … END;` | Tmplt › 5 Function | [8](../rozdzialy/08-funkcje.md) |
| VIEW | `VIEW "tekst", F() BEGIN … END;` | Tmplt › 5 Function | [19](../rozdzialy/19-wlasne-aplikacje.md) |
| KEY | `KEY K_nazwa() BEGIN … END;` | Tmplt › 5 Function | [20](../rozdzialy/20-klawiatura-uzytkownika.md) |

## Teksty (Cmds › 1 Strings)

| Komenda | Działanie |
|---|---|
| `ASC(s)` | lista kodów znaków `{65,66}` |
| `LOWER(s)`, `UPPER(s)` | małe / wielkie litery |
| `CHAR(k)` | znak lub znaki o podanych kodach |
| `DIM(s)` | długość tekstu (dla macierzy i ciągów wymiary) |
| `STRING(wyr, …)` | wyrażenie jako tekst z formatowaniem |
| `INSTRING(s1,s2)` | pozycja s2 w s1 (0 = brak) |
| `LEFT(s,n)`, `RIGHT(s,n)`, `MID(s,p[,n])` | fragmenty tekstu |
| `ROTATE(s,n)` | przesunięcie cykliczne |
| `STRINGFROMID(n)` | napis systemowy |
| `REPLACE(s,p,s2)` | nadpisanie fragmentu |
| `EXPR(s)` | oblicza tekst jako wyrażenie |

## Grafika (Cmds › 2 Drawing)

| Komenda | Działanie |
|---|---|
| `C→PX`, `PX→C` | konwersja współrzędnych kartezjańskich i pikselowych |
| `DRAWMENU(s1,…,s6)` | pasek menu na dole ekranu |
| `FREEZE` | zatrzymuje ekran |
| `RGB(r,g,b[,a])` | kod koloru |
| `ARC_P([G],x,y,r[,a1,a2],c)` | okrąg, łuk, elipsa (`r` jako `{rx,ry}`), wypełnienie (`c` jako `{krawędź,wnętrze}`) |
| `BLIT_P([cel,dx1,dy1,dx2,dy2],źr[,sx1,sy1,sx2,sy2,c,alfa])` | kopiowanie i skalowanie obrazu |
| `DIMGROB_P(G,w,h[,c])` | tworzenie bufora |
| `FILLPOLY_P([G],{(x,y),…},c[,alfa])` | wielokąt wypełniony |
| `GETPIX_P([G],x,y)` | kolor piksela |
| `GROBW_P(G)`, `GROBH_P(G)` | wymiary GROB-a |
| `INVERT_P([G,x1,y1,x2,y2])` | negatyw obszaru |
| `LINE_P([G],x1,y1,x2,y2[,c])` | linia (forma zaawansowana: wiele linii i 3D) |
| `PIXON_P([G],x,y[,c])`, `PIXOFF_P([G],x,y)` | piksel |
| `RECT_P([G,x1,y1,x2,y2,ck,cw])` | prostokąt; `RECT()` czyści ekran |
| `SUBGROB_P(źr[,x1,y1,x2,y2],cel)` | wycinek obrazu |
| `TEXTOUT_P(t[,G],x,y[,f,c1,w,c2])` | tekst; zwraca x końca tekstu |
| `TRIANGLE_P([G],x1,y1,x2,y2,x3,y3,c[,c2,c3][,alfa])` | trójkąt, gradient, 3D |

Każda komenda z `_P` ma wersję kartezjańską bez `_P`.

## Macierze (Cmds › 3 Matrix)

`ADDCOL`, `ADDROW`, `DELCOL`, `DELROW`, `EDITMAT`, `REDIM`, `REPLACE`, `SCALE`, `SCALEADD`, `SUB`, `SWAPCOL`, `SWAPROW`. Zob. [rozdz. 11](../rozdzialy/11-macierze.md).

## Aplikacje (Cmds › 4 App Functions)

| Komenda | Działanie |
|---|---|
| `STARTAPP("nazwa")` | uruchamia aplikację |
| `STARTVIEW(n[,rysuj])` | otwiera widok (tabela w [dodatku C](C-tabele.md)) |
| `VIEW` | pozycja menu View |
| `CHECK(n)`, `UNCHECK(n)`, `ISCHECK(n)` | zaznaczanie równań |

## Liczby całkowite (Cmds › 5 Integer)

`BITAND`, `BITNOT`, `BITOR`, `BITSL`, `BITSR`, `BITXOR`, `B→R`, `GETBASE`, `GETBITS`, `R→B`, `SETBITS`, `SETBASE`. Zob. [rozdz. 12](../rozdzialy/12-liczby-calkowite.md).

## Wejście i wyjście (Cmds › 6 I/O)

| Komenda | Działanie | Menu (ES) |
|---|---|---|
| `CHOOSE(v,"tytuł",o1,o2,…)` | okno wyboru | |
| `EDITLIST(L)` | edytor listy | |
| `EDITMAT(M[,tytuł][,tylko_odczyt])` | edytor macierzy | |
| `GETKEY` | kod klawisza z bufora albo -1 | I/O › 5 |
| `INPUT(v,…)` | formularz | I/O › B |
| `ISKEYDOWN(k)` | czy klawisz jest wciśnięty | |
| `MOUSE[(i)]` | dotyk | |
| `MSGBOX(wyr[,okcancel])` | komunikat | I/O › 8 |
| `PRINT(wyr)`, `PRINT()` | terminal; bez argumentu czyści | I/O › 9 |
| `WAIT(n)` | pauza | I/O › 3 |

## Różne (Cmds › 7 More)

| Komenda | Działanie |
|---|---|
| `%CHANGE(x,y)`, `%TOTAL(x,y)` | procenty |
| `CAS.f(…)` | wywołanie funkcji CAS |
| `EVALLIST(l)` | obliczenie elementów listy |
| `EXECON("wyr",l1[,l2])` | przekształcenie listy |
| `→HMS(x)`, `HMS→(x)` | zapis sześćdziesiątkowy |
| `ITERATE(wyr,zm,start,n)` | iteracja |
| `TICKS` | milisekundy |
| `TEVAL(wyr)` | czas obliczenia |
| `TYPE(ob)` | typ obiektu |

## Najczęstsze funkcje matematyczne i listowe

`ABS`, `IP`, `FP`, `FLOOR`, `CEILING`, `ROUND`, `TRUNCATE`, `MAX`, `MIN`, `MOD`, `irem`, `√`, `^`, `!`, `COMB`, `PERM`, `RANDOM`, `RANDINT`, `RANDSEED`, `SIN`…`ATAN`, `LN`, `LOG`, `EXP`, `ALOG`, `when`, `SIZE`, `MAKELIST`, `SORT`, `REVERSE`, `CONCAT`, `POS`, `ΣLIST`, `ΠLIST`, `ΔLIST`, `DIFFERENCE`, `INTERSECT`, `TRN`, `DET`, `RREF`, `MAKEMAT`, `IDENMAT`.

## Zmienne systemowe

`A`–`Z`, `θ` (liczby), `Z0`–`Z9` (zespolone), `L0`–`L9` (listy), `M0`–`M9` (macierze), `G0`–`G9` (grafika), `Ans`, `HAngle`, `HFormat`, `HDigits`, `HComplex`, `Date`, `Time`, `Entry`, `Base`, `Bits`, `Signed`, `Language`, `TOff`, `HVars`, `DelHVars`, `Notes`, `Programs`, `AFiles`, `AFilesB`, `ANote`, `AProgram`, `AVars`, `DelAVars`, `DelAFiles`.

## Wzorce do skopiowania

```
// program z dialogiem i obsługą Cancel
EXPORT NAZWA()
BEGIN
  LOCAL a, b;
  IF NOT INPUT({a,b}, "Tytuł", {"a=","b="}) THEN RETURN "anulowano"; END;
  RETURN a+b;
END;
```

```
// oczekiwanie na klawisz
REPEAT k := GETKEY; UNTIL k <> -1;
```

```
// pętla animacji z buforem
DIMGROB_P(G1,320,240);
REPEAT
  RECT_P(G1,0,0,319,239,#FFFFFFh,#FFFFFFh);
  // … rysuj w G1 …
  BLIT_P(G0,G1);
  WAIT(0.02);
UNTIL GETKEY == 4;          // Esc
```

```
// zapamiętanie i przywrócenie ustawienia
LOCAL stary := HAngle;
HAngle := 1;
// … obliczenia w stopniach …
HAngle := stary;
```

```
// podprogram prywatny
POMOC();
EXPORT GLOWNA() BEGIN RETURN POMOC(2); END;
POMOC(x) BEGIN RETURN x^2; END;
```
