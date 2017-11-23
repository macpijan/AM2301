#include "AM2301.h"

AM2301::AM2301(PinName pin) {
    _data = new DigitalInOut(pin);
    _last_poll = 0;
}

AM2301::~AM2301() {
}


void AM2301::init(void)
{
    /* Zainicjowanie rozpoczęcia transmisji. Konfiguracja pinu miktrokontrolera
     * jako wyjście. Wystawienie stanu wysokiego na linii danych/.
     */

    _data->output();
    _data->write(1);
}

int AM2301::read() {

    /*
     * Zmienna "n" będzie służyła do przechowywania odczytanego stanu licznika
     * w celu porównania zmierzonej wartości czasu trwania z wartościami
     * uważanymi za prawidłowe
     */

    int n;

    /* Stan wysoki na linii danych oznacza stan bezczynności.
     * Rozpoczęcie komunikcji następuje poprzez wystawienie stanu niskiego
     * na przynajmniej 800 us. Inicjallizujemy transjisję poprzez sytawienie
     * stanu niskiego trwającego 1 ms.
     */

    _data->output();
    _data->write(0);
    wait_ms(1);

    /* Wystawienie stanu wysokiego na linii danych.
     * Rekonfiguracja pinu mikrokontrolera jako wejście i oczekiwanie na
     * odpowiedź sensora.
     * Uruchamiamy i restartujemy również licznik, aby mierzyć czas do
     * uzyskania odpowiedzi z sensora.
     */

    timer.reset();
    _data->write(1);
    timer.start();
    _data->input();

    /* Jeżeli sensor nie odpowiada (nie pojawia się stan niski na linii danych)
     * przez czas 300 us, unajemy transmisję za zakończoną niepowodzeniem i
     * zwracamy kod błędu AM2301_RESPONSE_TIMEOUT (-1).
     */

    do {
        n = timer.read_us();
        if (n > TIMEOUT) {
            timer.stop();
            return AM2301_RESPONSE_TIMEOUT;
        }
    } while (_data->read() == 1);

    /* Licznik został już wcześniej wystartowany. Restartujemy licznik, aby
     * mierzyć długość impulsu o stanie niskim wystawionego przez sensor w
     * odpowiedzi na inicjalizację transmisji przez mikrkontroler.
     */

     timer.reset();

    /* Sprawdzenie czy otrzymana odpowiedź została otrymana w przewidzianym
     * przedziale czasowym. Zgodnie z notą katalogową powinno to być 20-200 us.
     * Podczas badań eksperymentalnych okazało się, że w przypadku posiadanego
     * sensora odpowiedź przychodzi po czasie nieco niższym niż 20 us, wobec
     * czego warunek został złagodzony.
     */

    if ((n < 10) || (n > 200)) {
        timer.stop();
        return AM2301_RESPONSE_ERROR;
    }

    /* Pomiar czasu trwania odpowiedzi sensora (stanu niskiego na linii
     * danych). Jeżeli trwa on dłużej niż założona wartość TIMEOUT, transmisję
     * uznajemy za nieudaną i wychodzimy zwracając kod błędu
     * AM2301_RESPONSE_TIMEOUT (-1).
     */

    do {
        n = timer.read_us();
        if (n > TIMEOUT) {
            timer.stop();
            return AM2301_RESPONSE_TIMEOUT;
        }
    } while (_data->read() == 0);

    /* Sprawdzenie poprawnosci odpowiedzi.
     * Sensor powinien wystawic stan niski na linii danych trwajacy 75-85 us
     * W praktyce zaobserwowany czas trwania najczęściej zawierał się w
     * przedziale 90-100 us, wobec czego warunek został skorygowany.
     * Jeżeli stan niski wystawiony przez sensor w odpowiedzi na inicjalizację
     * transmisji trwał zbyt krotko badz zbyt długo, transmisję uznaje się za
     * niepoprawną i wychodzimy z kodem błędu AM2301_REPONSE_ERROR (-2).
     */

    timer.reset();
    if ((n < 70) || (n > 110)) {
        timer.stop();
        return AM2301_RESPONSE_ERROR;
    }

    /* Po poprawnej odpowiedzi sensora powinien nastąpić stan bezczynności na
     * linii danych (stan wysoki) trwający 75-85 us. Również w tym przypadku,
     * badania eksperymentalne wykazały, że wymagana czasowe należy nieco
     * skorygować, gdyż sensor nie zawsze zachowuje parametry czasowe podane w
     * specyfikacji.
     */

    /* Jeżeli sensor nie odpowiada, wychodzimy z kodem błędu
     * AM2301_REPONSE_TIMEOUT (-1).
     */

    do {
        n = timer.read_us();
        if (n > TIMEOUT) {
            timer.stop();
            return AM2301_RESPONSE_TIMEOUT;
        }
    } while (_data->read() == 1);

    /* Jeżeli odpowiedź nie spełnia założonych parametrów czaswych, uznawana
     * jest za błędną i następuje zwrócenie kodu błędu AM2301_RESPONSE_ERROR (-2).
     */

    timer.reset();
    if ((n < 70) || (n > 100)) {
        timer.stop();
        return AM2301_RESPONSE_ERROR;
    }

    /*
     * Odczyt 40 bitów (5 bajtów) w następującej kolejności:
     * 1. wilgotność - bardziej znaczący bajt
     * 2. wilgotność - mniej znaczący bajt
     * 3. temperatura - bardziej znaczący bajt
     * 4. temepratura - mniej znaczący bajt
     * 5. suma kontrolna
     * Transmisja każdego kolejnego bitu poprzedzona jest niskim stanem
     * linii danych trwającym przynajmniej 50 us. Następnie na linii danych
     * pojawia się stan wysoki.
     * Długość trwania stanu wysokiego determinuje wartość trasmitowanego bitu
     * danych:
     * 0 - czas trwania 26-28 us
     * 1 - czas trwania 70 us
     * Najstarszy bit transmitowany jest jako pierwszy
     */

    /*
     * Wypełnienie tablicy służącej do przechywania transmitowanych danych
     * wartościami zerowymi
     */

    memset(buf, 0, sizeof(buf));

    /* N_BYTES (5) oznacza liczbę bajtów do odczytu. Tyle razy zostanie
     * wykonana pętla nadrzędna, której zadaniem jest odczyt jednego bajtu.
     */

    for (int i = 0; i < N_BYTES; i++) {

        /*
         * N_BITS oznacza liczbę bitów w jednym bajcie. Tyle razy zostanie
         * wykonana pętla wewenętrzna, której zadaniem jest odczyt pojedycznego
         * bitu
         */

        for (int b = 0; b < N_BITS; b++) {

            /*
             * Jeżeli stan magistrali nie ulega zmianie przez czas dłuższy niż
             * zdefiniowany w stałej TIMEOUT, następuje zakończenie transmjisji
             * z kodem błędu AM2301_READ_BIT_TIMEOUT (-3).
             */

            do {
                n = timer.read_us();
                if (n > TIMEOUT) {
                    timer.stop();
                    return AM2301_READ_BIT_TIMEOUT;
                }
            } while (_data->read() == 0);
            timer.reset();

            /* Zgodnie ze specyfikacją, czas trwania stanu niskiego
             * rozdzielającego transmisję dwóch sąsiednich bitów, powinien
             * wynosić 48-55 us. W trakcie badań eksperymentalnych czas ten
             * nierzadko dochodził do 60 us, wobec czego warunki zostały
             * skorygowane. Jeżeli czas ten nie zawiera się w ustalonych
             * granicach, transmisja zostaje uznana za błędną i następuje
             * wyjście z kodem błędu AM2301_RED_BIT_ERROR (-4).
             */

            if ((n < 40) || (n > 70)) {
                return AM2301_READ_BIT_ERROR;
            }

            /*
             * Pomiar czasu trwania stanu wysokiego linii danych. W zależności
             * od wartości tego czasu, zdekodowany zostanie bit "0" lub "1".
             * Jeżeli stan na linii danych nie zmienia się przez okres
             * zdefiniowany w stałej TIMEOUT, następuje wyjście z kodem błędu
             * AM2301_READ_BIT_TIMEOUT
             */

            do {
                n = timer.read_us();
                if (n > TIMEOUT) {
                    timer.stop();
                    return AM2301_READ_BIT_TIMEOUT;
                }
            } while (_data->read() == 1);
            timer.reset();

            /*
             * Dekodowanie otrzymanego bitu na podstawie czasu trwania sygnału
             * wysokiego na linii danych. Jeżeli zmierzony czas trwania stanu
             * wysokiego nie odpowiada ani bitowu "0" ani "1", wychodzimy z
             * kodem błędu AM2301_READ_BIT_ERROR.
             */

            if ((n > 15) && (n < 35)) {
                buf[i] <<= 1;
            } else if ((n > 65) && (n < 80)) {
                buf[i] = ((buf[i] << 1) | 1);
            } else
                return AM2301_READ_BIT_ERROR;
        }
    }

    /*
     * Zatrzyanie licznika, gdyż nie będzie już potrzebny
     */

    timer.stop();

    /*
     * Rekonfiguracja pinu mikrokontrolera jako wyjście i wystawienie stanu
     * wysokiego na linii danych (stan bezczynności magistrali).
     */

    _data->output();
    _data->write(1);

    /* Obliczenie sumy kontrolnej poprzez zsumowanie pierwszych pięciu bajtów
     * danych
     */

    int checksum = (buf[0] + buf[1] + buf[2] + buf[3]);

    /* Ograniczenie otrzymanej sumy do jednego bajtu poprzez nałożenie maski
     * bitowej ONE_BYTE_MASK (0xFF). Jeżeli obliczona suma kontrolna mieściła
     * się na więcej niż 8 bitach, wynikiem takiego działania będzie
     * pozostawienie tylko ośmiu mniej znaczących bitów.
     */

    /* Jeżeli tak obliczona suma kontrolna równa jest piątemu z odczytanych
     * bajtów, transmisję uznaje się za przeprowadzoną poprawnie i następuje
     * wyjście z kodem AM2301_SUCCESS (0). W przeciwnym wypadku transmisję uznaję
     * się za przeprowadzoną niepoprawnie i następuje wyjście z kodem błędu
     * AM2301_CHECKSUM_ERROR (-5).
     */

    if ((checksum & ONE_BYTE_MASK) == buf[4]) {
        return AM2301_SUCCESS;
    } else {
        return AM2301_CHECKSUM_ERROR;
    }
}

void AM2301::close(void)
{
    timer.stop();
    _data->output();
    _data->write(1);
}

int AM2301::calculate(void)
{

    /*
     * Czas pomiędzy kolejnymi odpytaniami sensora nie powinien być krótszy niż
     * 2 sekundy. Zmienna "_last_poll" przechowuje czas ostatniego odpytania.
     * Przy pierwszym wywołaniu jest ona równa 0 (inicjalizacja następuje w
     * konstruktorze, podczas tworzenia obiektu klasy AM2301) i warunek ten nie
     * jest sprawdzany. Jeżeli czas pomiędzy odpytaniami jest krótszy niż 2
     * sekundy, zwracany jest kod błędu AM2301_POLLING_INTERVAL_ERROR (-6).
     */

    if ((_last_poll != 0)) {
        if ((time(NULL) - _last_poll < 2)) {
            return AM2301_POLLING_INTERVAL_ERROR;
        }
    }

    /*
     * Odwołanie się do metody init() z tej klasy w celu rozpoczęcia transmisji
     * danych pomiarowych przez sensor
     */

    this->init();

    /*
     * Wywołanie metody read() z tej klasy, której zadaniem jest
     * przeprowadzenie poprawnego odczytu danych transmitowanych przez sensor
     */

    int ret = this->read();

    /*
     * Wywołanie metody close() z tej klasy w celu upewnieniu się o zakończeniu
     * transmisji i pozostawieniu linii danych w stanie bezczynności
     */

    this->close();

    /*
     * Przechowanie czasu odpytania sensora. "time(NULL) zwraca aktualny czas
     */

    _last_poll = time(NULL);

    /*
     * Jeżeli kod wyjście z metody "read()" wynosi "AM2301_SUCCESS" (0), oznacza
     * to że tranmisja została zakończona poprawnie. Możemy zatem przejść do
     * obliczenia otrzymanych wartości wilgotności względnej i temperatury
     */

    if (ret == AM2301_SUCCESS) {

        /*
         * Obliczenie wartości wilgotności względnej poprzez zsumowanie
         * młodszego bajtu ze starszym bajtem przesuniętym o 8 miejsc w lewo
         */

        _humidity = static_cast<int16_t>((buf[0] << N_BITS)  + buf[1]);

        /*
         * Obliczenie wartości temperatury
         * W pierwszym etapie na starszy bajt nakładana jest maska WITHOUT_SIGN
         * (0x7F), która powoduje wyzerowanie najstarszego bitu, a pozostałe
         * pozostawia bez zmian. Następnie obliczana jest wartość bezwzględna
         * temperatury poprzez zsumowanie młodzego bajtu tmperatury z uprzednio
         * zmodyfikowanym starszym bajtem temperatury przeuniętym o 8 miejsc w
         * lewo.
         */

        _temperature = static_cast<int16_t>(((buf[2] & WITHOUT_SIGN) << N_BITS)  + buf[3]);

        /*
         * Najbardziej znaczący bit w starszym bajcie temperatury oznacza jej
         * znak. Jeżeli jest on dodatni, oznacza to że zmierzono temperaturę
         * ujemną, wobec czego należy obliczoną wyżej wartość bezwględną
         * temperatury pomnożyć przez -1.
         */

        if ((buf[0] & CHECK_SIGN) == CHECK_SIGN) {
            _temperature *= -1;
        }
    }

    /*
     * Przekazanie do programu głównego kodu wyjścia z metody read()
     */

    return ret;
}

/*
 * Funkcja pełniąca rolę akcesora "get" do pola "_temperature". Zwraca ona
 * wartość tego pola, czyli wartość zmierzonej temperatury pomnożeonej przez
 * 10.
 */

int AM2301::get_temperature() {
    return _temperature;
}

/*
 * Funkcja pełniąca rolę akcesora "get" do pola "_humidity". Zwraca ona
 * wartość tego pola, czyli wartość zmierzonej wilgotności względnej
 * pomnożeonej przez  10.
 */

int AM2301::get_humidity() {
    return _humidity;
}
