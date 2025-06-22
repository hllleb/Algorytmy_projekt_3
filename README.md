# Chess AI – Minimax & Alpha-Beta Pruning

Gra w szachy z SI opartą na algorytmie min-max z cięciami alfa-beta, napisana w C++ z graficznym interfejsem użytkownika (SFML).

---

## Spis treści

* [Opis](#opis)
* [Wymagania](#wymagania)
* [Instalacja](#instalacja)
* [Sposób użycia](#sposób-użycia)
* [Zasady gry](#zasady-gry)
* [Opis algorytmu AI](#opis-algorytmu-ai)
* [Zrzuty ekranu](#zrzuty-ekranu)
* [Autor](#autor)

---

## Opis

Projekt realizuje grę w szachy z możliwością gry przeciwko komputerowi. SI wykorzystuje algorytm Minimax z cięciami alfa-beta oraz heurystyczną oceną pozycji. Gra posiada graficzny interfejs (biblioteka SFML), obsługę standardowych reguł szachów oraz zapis ruchów do pliku.

---

## Wymagania

* **C++ (C++11 lub nowszy)**
* **SFML 2.5 lub nowszy**
* System: Windows / Linux / macOS (zalecany Windows – pliki DLL SFML)
* (Opcjonalnie) CMake

---

## Instalacja

1. **Klonowanie repozytorium:**

   ```sh
   git clone https://github.com/hlllebbb/Algorytmy_projekt_3.git
   cd chess-ai-minimax
   ```

2. **Budowanie projektu (przykład dla CMake):**

   ```sh
   mkdir build
   cd build
   cmake ..
   make
   ```

   Lub użyj swojego ulubionego IDE (np. CLion, Visual Studio).

3. **Pliki DLL SFML:**

   * Skopiuj pliki DLL SFML (`*.dll`) do katalogu z plikiem wykonywalnym (`.exe`).
   * Upewnij się, że folder `resources/` (np. z grafikami) znajduje się obok pliku `.exe`.

---

## Sposób użycia

1. Uruchom plik wykonywalny (np. `projekt3.exe`).
2. Gra rozpoczyna się od białych.
3. Ruchy wykonuj myszką – kliknij figurę, a potem pole docelowe.
4. Program zapisuje log ruchów w pliku `chess_log.txt`.
5. Aby zrestartować grę, użyj odpowiedniej opcji w GUI lub uruchom program ponownie.

---

## Zasady gry

* Gra obsługuje wszystkie standardowe reguły szachów:

  * Ruchy wszystkich figur
  * Roszada
  * Promocja pionka
  * Bicie w przelocie (en passant)
  * Wykrywanie szacha, mata, pata
* AI gra przeciwko graczowi (człowiek vs komputer).

---

## Opis algorytmu AI

Sztuczna inteligencja korzysta z:

* Algorytmu Minimax z cięciami alfa-beta.
* Heurystyki pozycyjnej uwzględniającej:

  * Materiał (wartości figur)
  * Mobilność króla i figur
  * Bezpieczeństwo króla
  * Możliwość promocji pionka
  * Ograniczenie mobilności króla przeciwnika w końcówce
  * Kary za powtarzanie pozycji
  * Premie/ kary za groźby matujące
* Głębokość drzewa min-max można ustawić w kodzie (zmienna `maxDepth` w pliku `ChessGame.h`).

---

## Zrzuty ekranu

![Przykładowa rozgrywka](images/screen1.png)

---

## Autor

* **Hleb Lizhbanau**
  e-mail: [hleblizhbanau@gmail.com](mailto:hleblizhbanau@gmail.com)

---
