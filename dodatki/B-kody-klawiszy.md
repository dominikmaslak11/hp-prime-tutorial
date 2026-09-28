# Dodatek B. Kody klawiszy (GETKEY) i nazwy klawiszy (KEY)

[← Spis treści](../README.md)

## Kody GETKEY / ISKEYDOWN (*UG s. 607, ES #3*)

Kody są numerowane od lewego górnego klawisza (0) do prawego dolnego (50). Brak klawisza w buforze to **-1**.

Układ kodów według rzędów klawiatury (od góry):

| Rząd | Klawisze (kod) |
|---|---|
| 1 | Apps (0), Symb (1), ▲ (2), Help (3), Esc (4) |
| 2 | Home (5), Plot (6), ◀ (7), ▶ (8), View (9), CAS (10) |
| 3 | Num (11), ▼ (12), Menu (13) |
| 4 | Vars (14), Toolbox (15), Template (16), x,t,θ,n (17), a b/c (18), Backspace (19) |
| 5 | xʸ (20), SIN (21), COS (22), TAN (23), LN (24), LOG (25) |
| 6 | x² (26), +/- (27), ( ) (28), , (29), Enter (30) |
| 7 | EEX (31), 7 (32), 8 (33), 9 (34), ÷ (35) |
| 8 | ALPHA (36), 4 (37), 5 (38), 6 (39), × (40) |
| 9 | Shift (41), 1 (42), 2 (43), 3 (44), − (45) |
| 10 | On (46), 0 (47), . (48), spacja (49), + (50) |

Klawisze kierunkowe ▲ ◀ ▶ ▼ tworzą okrągły pad na środku górnej części klawiatury. Dlatego ich kody są „wplecione” między kody klawiszy stojących obok.

Tabela alfabetyczna:

| Klawisz | Kod | | Klawisz | Kod |
|---|---|---|---|---|
| 0 | 47 | | Esc | 4 |
| 1 | 42 | | Help | 3 |
| 2 | 43 | | Home | 5 |
| 3 | 44 | | LN | 24 |
| 4 | 37 | | LOG | 25 |
| 5 | 38 | | Menu | 13 |
| 6 | 39 | | Num | 11 |
| 7 | 32 | | On | 46 |
| 8 | 33 | | Plot | 6 |
| 9 | 34 | | Shift | 41 |
| . | 48 | | SIN | 21 |
| spacja | 49 | | Symb | 1 |
| + | 50 | | TAN | 23 |
| − | 45 | | Template | 16 |
| × | 40 | | Toolbox | 15 |
| ÷ | 35 | | Vars | 14 |
| ( ) | 28 | | View | 9 |
| , | 29 | | x,t,θ,n | 17 |
| +/- | 27 | | x² | 26 |
| a b/c | 18 | | xʸ | 20 |
| ALPHA | 36 | | ▲ | 2 |
| Apps | 0 | | ▼ | 12 |
| Backspace | 19 | | ◀ | 7 |
| CAS | 10 | | ▶ | 8 |
| COS | 22 | | EEX | 31 |
| Enter | 30 | | | |

Klawisze cyfr jako lista (indeks = cyfra+1): `{47,42,43,44,37,38,39,32,33,34}`.

## Nazwy klawiszy (KEY)

Nazwy są potrzebne do przeprogramowania klawiszy ([rozdział 20](../rozdzialy/20-klawiatura-uzytkownika.md)). **Wielkość liter ma znaczenie.**

| Klawisz | Sam | + Shift | + ALPHA | + Shift + ALPHA |
|---|---|---|---|---|
| Apps | `K_Apps` | `KS_Apps` | `KA_Apps` | `KSA_Apps` |
| Symb | `K_Symb` | `KS_Symb` | `KA_Symb` | `KSA_Symb` |
| ▲ | `K_Up` | `KS_Up` | `KA_Up` | `KSA_Up` |
| Help | `K_Help` | — | `KA_Help` | `KSA_Help` |
| Esc | `K_Esc` | `KS_Esc` | `KA_Esc` | `KSA_Esc` |
| Home | `K_Home` | `KS_Home` | `KA_Home` | `KSA_Home` |
| Plot | `K_Plot` | `KS_Plot` | `KA_Plot` | `KSA_Plot` |
| ◀ | `K_Left` | `KS_Left` | `KA_Left` | `KSA_Left` |
| ▶ | `K_Right` | `KS_Right` | `KA_Right` | `KSA_Right` |
| View | `K_View` | `KS_View` | `KA_View` | `KSA_View` |
| CAS | `K_Cas` | `KS_Cas` | `KA_Cas` | `KSA_Cas` |
| Num | `K_Num` | `KS_Num` | `KA_Num` | `KSA_Num` |
| ▼ | `K_Down` | `KS_Down` | `KA_Down` | `KSA_Down` |
| Menu | `K_Menu` | `KS_Menu` | `KA_Menu` | `KSA_Menu` |
| Vars | `K_Vars_` | `KS_Vars_` | `KA_Vars_` | `KSA_Vars_` |
| Toolbox | `K_Math` | `KS_Math` | `KA_Math` | `KSA_Math` |
| Template | `K_Templ` | `KS_Templ` | `KA_Templ` | `KSA_Templ` |
| x,t,θ,n | `K_Xttn` | `KS_Xttn` | `KA_Xttn` | `KSA_Xttn` |
| a b/c | `K_Abc` | `KS_Abc` | `KA_Abc` | `KSA_Abc` |
| Backspace | `K_Bksp` | `KS_Bksp` | `KA_Bksp` | `KSA_Bksp` |
| xʸ | `K_Power` | `KS_Power` | `KA_Power` | `KSA_Power` |
| SIN | `K_Sin` | `KS_Sin` | `KA_Sin` | `KSA_Sin` |
| COS | `K_Cos` | `KS_Cos` | `KA_Cos` | `KSA_Cos` |
| TAN | `K_Tan` | `KS_Tan` | `KA_Tan` | `KSA_Tan` |
| LN | `K_Ln` | `KS_Ln` | `KA_Ln` | `KSA_Ln` |
| LOG | `K_Log` | `KS_Log` | `KA_Log` | `KSA_Log` |
| x² | `K_Sq` | `KS_Sq` | `KA_Sq` | `KSA_Sq` |
| +/- | `K_Neg` | `KS_Neg` | `KA_Neg` | `KSA_Neg` |
| ( ) | `K_Paren` | `KS_Paren` | `KA_Paren` | `KSA_Paren` |
| , | `K_Comma` | `KS_Comma` | `KA_Comma` | `KSA_Comma` |
| Enter | `K_Enter`¹ | `KS_Enter` | `KA_Enter` | `KSA_Enter` |
| EEX | `K_Eex` | `KS_Eex` | `KA_Eex` | `KSA_Eex` |
| 7 8 9 | `K_7` `K_8` `K_9` | `KS_7`… | `KA_7`… | `KSA_7`… |
| ÷ | `K_Div` | `KS_Div` | `KA_Div` | `KSA_Div` |
| ALPHA | `K_Alpha` | `KS_Alpha` | `KA_Alpha` | `KSA_Alpha` |
| 4 5 6 | `K_4` `K_5` `K_6` | `KS_4`… | `KA_4`… | `KSA_4`… |
| × | `K_Mul` | `KS_Mul` | `KA_Mul` | `KSA_Mul` |
| 1 2 3 | `K_1` `K_2` `K_3` | `KS_1`… | `KA_1`… | `KSA_1`… |
| − | `K_Minus` | `KS_Minus` | `KA_Minus` | `KSA_Minus` |
| On | `K_On` | — | `KA_On` | `KSA_On` |
| 0 | `K_0` | `KS_0` | `KA_0` | `KSA_0` |
| . | `K_Dot` | `KS_Dot` | `KA_Dot` | `KSA_Dot` |
| spacja | `K_Space` | `KS_Space` | `KA_Space` | `KSA_Space` |
| + | `K_Plus` | `KS_Plus` | `KA_Plus` | `KSA_Plus` |

¹ W instrukcji wydrukowano `K_Ente`, prawdopodobnie przez literówkę. Użyj *Create user key*, a szablon wpisze poprawną nazwę.
