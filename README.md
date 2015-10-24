Repozytorium kodu gry Laeran MUD. Kod jest połączeniem drivera DGD oraz mudliba Phantasmal.

## 1. Informacje ogólne ##

* Na chwilę obecną, kod był testowany jedynie na na systemach GNU/Linux. Jest małe prawdopodobieństwo, że będzie działać na Windowsie również.
* W repozytorium nie ma danych o świecie, takich jak pokoje, moby, przedmioty itd.
* Do działania kod potrzebuje również pliku konfiguracyjnego dla drivera, kluczy ssh oraz plików świata gry (strefy, pokoje, moby, przedmioty).
* Ten kod nie zawiera dokumentacji, można ją pobrać z repozytoriów kodu.
* Podobnie jak driver i mudlib, kod gry jest dostępny na licencji AGPL wersja 3.

## 2. Przydatne linki ##

https://github.com/dworkin/dgd/ - dokumentacja dla drivera DGD i LPC (katalogi doc i lpc-doc)

https://github.com/shentino/kernellib/tree/master/doc - dokumentacja dla KernelLiba

https://github.com/shentino/phantasmal - dokumentacja dla Phantasmal mudliba, plik konfiguracyjny drivera, testowe klucze ssh oraz pliki ze światem gry

## 3. Instalacja w systemie Linux ##

Wszystkie kroki/polecenia wykonujemy w terminalu. Oczywiście najpierw trzeba pobrać kod oraz potrzebne do uruchomienia pliki podane w pkt 1 ;)

* Przejdź do katalogu src/
* Wykonaj komendę:

		make install

* Zaktualizuj plik konfiguracyjny gry

## 4. Uruchamianie ##

W konsoli, przejdź do katalogu bin/ i wykonaj komendę:

	./driver [nazwa pliku .dgd]

Bądź edytuj dowolnym edytorem plik *run.sh*
