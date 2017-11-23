/*
 * Standardowa dyrektywa zapobiegająca przez załączeniem tego samego pliku
 * nagłówkoego przez pliki źródłowe kilkukrotnie.
 */

#ifndef MBED_AM2301_H
#define MBED_AM2301_H

/*
 * Załączenie pliku nagłówkowego mbed.h zawierdajacego definicje obiektów
 * wykorzystywanych przez poniższą klasę. Wszystkie sterowniki przeznaczone do
 * współpracy ze śrdowoiskiem mbed powinny załączać ten plik nagłówkowy
 */

#include "mbed.h"

/*
 * Maska bitowa, wykorzysytwana do wyzerowania bitów poza ośmioma najmłodszymi
 */

#define ONE_BYTE_MASK         0xFF

/*
 * Liczba bitów w bajcie
 */

#define N_BITS             8

/*
 * Liczba bajtów transmitowanych przez sensor
 */

#define N_BYTES            5

/*
 * Maska bitowa służąca do wyzerowania najstarszego bitu w bajcie
 */

#define WITHOUT_SIGN 0x7F

/*
 * Maska bitowa służąca do wyzerowania wszystkich bitów w bajcie poza
 * najstarszym
 */

#define CHECK_SIGN 0x80

/*
 * Przyjęta wartość czasu (w mikrosekundach) po upływie której bez zmiany stanu
 * na linii danych następuje uznanie tranmisji za nieudaną i wyjście z kodem
 * błędu
 */

#define TIMEOUT        300

/*
 * Kody wyjścia zwracane przez metody tej klasy
 */

enum exitCodes {
    AM2301_SUCCESS = 0,
    AM2301_RESPONSE_TIMEOUT = -1,
    AM2301_RESPONSE_ERROR = -2,
    AM2301_READ_BIT_TIMEOUT = -3,
    AM2301_READ_BIT_ERROR = -4,
    AM2301_CHECKSUM_ERROR = -5,
    AM2301_POLLING_INTERVAL_ERROR = -6
} ;

class AM2301 {

    /*
     * Częśc publiczna klasy
     */

public:

    /*
     * Kontruktor inicjalizujący
     */

    AM2301(PinName pin);

    /*
     * Destruktor domyślny
     */

    ~AM2301();

    /*
     * Metoda obliczająca wartość zmierzonej temperatury i wilgotności
     * względnej na podstawie otrzymanych danych z sensora
     */

    int calculate(void);

    /*
     * Metoda (akcesor typu get) zwracająca wartość pola prywatnego "_temperature"
     */

    int get_temperature(void);

    /*
     * Metoda (akcesor typu get) zwracająca wartość pola prywatnego "_humidity"
     */

    int get_humidity(void);

    /*
     * Częśc prywatna klasy
     */

private:

    /*
     * Pole przechowujące wartość zmierzonej temperatury pomnożoną przez 10
     */

    int _temperature;

    /*
     * Pole przechowujące wartość zmierzonej wilgotności względnej pomnożoną
     * przez 10
     */

    int _humidity;

    /*
     * Pole przechowujące czas ostatniego odpytania sensora
     */

    time_t _last_poll;

    /* Wskaźnik na obiekt typu DigitalInOut reprezentujący pin mikrokontrolera
     * służacy do komunikacji z sensorem
     */

    DigitalInOut *_data;

    /*
     * Obiekt klasy Timer wykorzysytwany przez metody tej klasy do pomiaru
     * czasów trwania stanu wysokiego lub niskiego na linii danych
     */

    Timer timer;

    /*
     * Pięcioelementowa tablica służąca do przechowywania danych odczytanych z
     * sensora
     */

    int buf[5];

    /*
     * Metoda inicjująca transmisję danych z sensora
     */

    void init();

    /*
     * Metoda służąca do odczytu danych transmitowanych przez sensor
     */

    int read();

    /*
     * Metoda kończąca komunikację z sensorem. Pozostawia linię danych w stanie
     * bezczynności (stan wysoki).
     */

    void close();
};

#endif
