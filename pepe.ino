#include <Adafruit_NeoPixel.h>
#include <Servo.h>

//Eventos del sistema
//-----------------------------------------------------
    #define EVENTO_CONTINUAR                    10
    #define EVENTO_VOLVER_A_REPOSAR             15
    #define EVENTO_ATRAER_CARPINCHOS            20
    #define EVENTO_AJUSTAR_CHICHARRA            25
    #define EVENTO_COMENZAR_LLAMADO             30
    #define EVENTO_PESAR_CARPINCHOS             40
    #define EVENTO_PRENDER_HIPNOTIZADOR         50
    #define EVENTO_PULSAR_BOTON                 60
    #define EVENTO_CERRAR_COMPUERTA             70

//-----------------------------------------------------

//Estados del sistema
//-----------------------------------------------------
    #define ESTADO_REPOSANDO                    100
    #define ESTADO_CREANDO_EJERCITO_CARPINCHOS  200
    #define ESTADO_INVADIENDO_NORDELTA          300
    #define ESTADO_FINALIZADO                   400
    #define ESTADO_HIPNOTIZANDO_CARPINCHOS      500
    #define ESTADO_NO_RECONOCIDO                999
//-----------------------------------------------------

//Constantes del Sensor e interruptor
//-----------------------------------------------------
    #define UMBRAL_TIMEOUT                      200
    #define UMBRAL_PESO_CARPINCHOS              200
    #define UMBRAL_PESO_SIN_CARPINCHOS          0
    #define PULSADOR_ACTIVADO                   1
    #define INTERRUPTOR_ACTIVADO                0
    #define HABILITAR_INICIO                    0
    #define INICIO_HABILITADO                   1
//-----------------------------------------------------

//Constantes del Piezo
//-----------------------------------------------------
    #define PIEZO_INICIO_RANGO_ENTRADA          0
    #define PIEZO_INICIO_RANGO_SALIDA           0
    #define PIEZO_FINAL_RANGO_ENTRADA           1023
    #define PIEZO_FINAL_RANGO_SALIDA            127
//-----------------------------------------------------

//Constantes del NeoPixel
//-----------------------------------------------------
    #define MIN_PIXEL                           0
    #define MAX_PIXEL                           12
    #define PIXEL_SIN_COLOR                     0
    #define PIXEL_COLOR_ROJO                    255
    #define PIXEL_COLOR_VERDE                   255
    #define PIXEL_COLOR_AZUL                    255
//-----------------------------------------------------

//constantes del servomotor
//-----------------------------------------------------
    #define SERVO_ESTADO_INICIAL                0
    #define SERVO_ANGULO_APERTURA               180
//-----------------------------------------------------

//constantes del potenciometro
//-----------------------------------------------------
    #define POTENCIOMETRO_ESTADO_INICIAL        0
    #define POTENCIOMETRO_HABILITADO            1
    #define POTENCIOMETRO_DESHABILITADO         1
//-----------------------------------------------------

//Constante para prender o apagar logs
//-----------------------------------------------------
    #define LOGS_ACTIVADOS                      0
//-----------------------------------------------------

//Pines de Arduino en los que conectamos los sensores y actuadores
//-----------------------------------------------------
    #define PIN_POTENCIOMETRO                   A1
    #define PIN_PIEZO                           3
    #define PIN_FORSE_SENSOR                    A0
    #define PIN_ANILLO_LED                      4
    #define PIN_PULSADOR                        7
    #define PIN_SERVO                           8
    #define PIN_INTERRUPTOR_DESLIZANTE          12
//-----------------------------------------------------

// Variables
//-----------------------------------------------------
    Servo Servo1;
    int estado;
    int flag_inicio = HABILITAR_INICIO;
//-----------------------------------------------------

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(MAX_PIXEL, PIN_ANILLO_LED, NEO_GRB + NEO_KHZ800);

// Estructuras de datos
//-----------------------------------------------------
    struct structPotenciometro{
        int pin;
        int estado;
        long valorActual;
        long valorPrevio;
    };
    structPotenciometro potenciometro;

    struct strucPulsador{
        int pin;
    };
    strucPulsador pulsador;

    struct strucInterruptorDeslizante{
        int pin;
        int estadoActual;
        int estadoPrevio;
    };
    strucInterruptorDeslizante interruptorDeslizante;

    struct structForceSensor{
        int pin;
        int pesoActual;
        int pesoAnterior;
    };
    structForceSensor forceSensor;

    struct structPiezo{
        int pin;
    };
    structPiezo piezo;

    struct structAnilloLed{
        int pin;
    };
    structAnilloLed anilloLed;

    struct structServoMotor{
        int pin;
    };
    structServoMotor servoMotor;

    struct structEvento{
        int tipo;
        long valor;
    };
    structEvento evento;
//-----------------------------------------------------

//Setup inicial
//-----------------------------------------------------
    void setup(){ 
        Serial.begin(9600);

        potenciometro.pin       = PIN_POTENCIOMETRO;
        potenciometro.estado    = POTENCIOMETRO_ESTADO_INICIAL;
        potenciometro.valorPrevio = leerPotenciometro();
        potenciometro.valorActual = POTENCIOMETRO_ESTADO_INICIAL;

        pulsador.pin                = PIN_PULSADOR;
        forceSensor.pin             = PIN_FORSE_SENSOR;
        piezo.pin                   = PIN_PIEZO;
        anilloLed.pin               = PIN_ANILLO_LED;
        servoMotor.pin              = PIN_SERVO;
        interruptorDeslizante.pin   = PIN_INTERRUPTOR_DESLIZANTE;

        pinMode(piezo.pin,OUTPUT);         //Piezo
        pinMode(forceSensor.pin,INPUT);    //Forse sensor
        pinMode(anilloLed.pin,OUTPUT);     //Anillo led
        pinMode(pulsador.pin,INPUT);       //Pulsador
        pinMode(interruptorDeslizante.pin,INPUT);

        evento.tipo = EVENTO_CONTINUAR;

        Servo1.attach(servoMotor.pin);
        Servo1.write(SERVO_ESTADO_INICIAL);
        delay(100);
        pixels.begin();

        estado = ESTADO_REPOSANDO;
    }
//-----------------------------------------------------


//Lectura de sensores
//-----------------------------------------------------
    long leerPotenciometro(){
        return analogRead(potenciometro.pin);
    }

    long leerForceSensor(){
        return analogRead(forceSensor.pin);
    }

    int leerPulsador(){
        return digitalRead(pulsador.pin);
    }

    int leerInterruptorDeslizante(){
        return digitalRead(interruptorDeslizante.pin);
    }
//-----------------------------------------------------


//Verificacion de Sensores
//-----------------------------------------------------
    bool verificarInterruptorDeslizante(){
        interruptorDeslizante.estadoActual = leerInterruptorDeslizante();

        int valorActual = interruptorDeslizante.estadoActual;
        int valorPrevio = interruptorDeslizante.estadoPrevio;

        if(valorActual == INTERRUPTOR_ACTIVADO && flag_inicio == HABILITAR_INICIO){
            evento.tipo = EVENTO_ATRAER_CARPINCHOS;
            interruptorDeslizante.estadoPrevio = valorActual;
            flag_inicio = INICIO_HABILITADO;
            return true;
        } else if(valorActual == INTERRUPTOR_ACTIVADO && flag_inicio == INICIO_HABILITADO){
                    evento.tipo = EVENTO_ATRAER_CARPINCHOS;
                    interruptorDeslizante.estadoPrevio = valorActual;
                    return false;
        } else{
            evento.tipo = EVENTO_VOLVER_A_REPOSAR;
            interruptorDeslizante.estadoPrevio = valorActual;
            flag_inicio = HABILITAR_INICIO;
            return true;
        }
    }
    

    bool verificarPosicionDelPotenciometro(){
        potenciometro.valorActual = leerPotenciometro();

        int valorActual = potenciometro.valorActual;
        int valorPrevio = potenciometro.valorPrevio;

        if (valorActual != valorPrevio){
            evento.tipo = EVENTO_AJUSTAR_CHICHARRA;
            evento.valor = valorActual;
            potenciometro.valorPrevio = valorActual;
            potenciometro.estado = valorActual;
            return true;
        }

        evento.valor = valorActual;
        return false;
    }

    bool verificarPesoDelForceSensor(){
        forceSensor.pesoActual = leerForceSensor();

        int pesoActual = forceSensor.pesoActual;
        int pesoPrevio = forceSensor.pesoAnterior;

        if(pesoActual != pesoPrevio && pesoActual > UMBRAL_PESO_CARPINCHOS){
            evento.tipo = EVENTO_PRENDER_HIPNOTIZADOR;
            evento.valor = pesoActual;
            forceSensor.pesoAnterior = pesoActual;
            return true;
        } 

        if(pesoActual ==  UMBRAL_PESO_SIN_CARPINCHOS && pesoPrevio != UMBRAL_PESO_SIN_CARPINCHOS){
            evento.tipo = EVENTO_CERRAR_COMPUERTA;
            evento.valor = pesoActual;
            forceSensor.pesoAnterior = pesoActual;
            flag_inicio = HABILITAR_INICIO;
            return true;
        } 
        return false;
    }

    bool verificarPulsador(){
        if(leerPulsador() == PULSADOR_ACTIVADO){
            evento.tipo = EVENTO_PULSAR_BOTON;
            return true;
        } 

        return false;
    }
//-----------------------------------------------------


//Funciones del sistema
//-----------------------------------------------------
    void reproducirSonidoPiezo(){
        evento.valor = map(evento.valor, PIEZO_INICIO_RANGO_ENTRADA, PIEZO_FINAL_RANGO_ENTRADA, PIEZO_INICIO_RANGO_SALIDA, PIEZO_FINAL_RANGO_SALIDA);
        tone(piezo.pin, evento.valor);
    }

    void prenderLeds(){
        pixels.setPixelColor(random(MIN_PIXEL, MAX_PIXEL), pixels.Color(PIXEL_COLOR_ROJO, PIXEL_SIN_COLOR, PIXEL_SIN_COLOR));
        pixels.show();
        pixels.setPixelColor(random(MIN_PIXEL, MAX_PIXEL), pixels.Color(PIXEL_SIN_COLOR, PIXEL_COLOR_VERDE, PIXEL_SIN_COLOR));
        pixels.show();
        pixels.setPixelColor(random(MIN_PIXEL, MAX_PIXEL), pixels.Color(PIXEL_SIN_COLOR, PIXEL_SIN_COLOR, PIXEL_COLOR_AZUL));
        pixels.show();
    }

    void detenerSonidoPiezo(){
        noTone(piezo.pin);
    }

    void abrirCompuerta(){
        Servo1.write(SERVO_ANGULO_APERTURA);
    }

    void cerrarCompuerta(){
        Servo1.write(SERVO_ESTADO_INICIAL);
    }

    void apagarHiptonizador(){
        pixels.clear();
        pixels.show();
    }

    void generarLogs(int estadoActual, int eventoActual){
        if(LOGS_ACTIVADOS){
            Serial.println("---------------------------------------------");
            Serial.print("ESTADO: "); Serial.println(estadoActual);
            Serial.print("EVENTO: "); Serial.println(eventoActual);
            Serial.println("---------------------------------------------");
        }
    }
//-----------------------------------------------------

//Genera eventos
//-----------------------------------------------------
    void genera_evento(){
        if(verificarInterruptorDeslizante() || verificarPosicionDelPotenciometro() || verificarPesoDelForceSensor() || verificarPulsador()){
            return;
        } else {
            evento.tipo = EVENTO_CONTINUAR;
            }
    }
//-----------------------------------------------------

//Máquina de estados
//-----------------------------------------------------
    void maquina_estados(){

        genera_evento();

        switch(estado)
        {
            case ESTADO_REPOSANDO:
            {
                switch(evento.tipo)
                {
                    case EVENTO_CONTINUAR:
                    {
                        generarLogs(ESTADO_REPOSANDO, EVENTO_CONTINUAR);

                        estado = ESTADO_REPOSANDO;
                    }
                    break;

                    case EVENTO_ATRAER_CARPINCHOS:
                    {
                        generarLogs(ESTADO_REPOSANDO, EVENTO_ATRAER_CARPINCHOS);

                        estado = ESTADO_CREANDO_EJERCITO_CARPINCHOS;
                    }
                    break;
                    
                    default:{
                        generarLogs(ESTADO_NO_RECONOCIDO, EVENTO_ATRAER_CARPINCHOS);}
                    break;
                }
            }
            break;

            case ESTADO_CREANDO_EJERCITO_CARPINCHOS: 
            {
                switch(evento.tipo)
                {
                    case EVENTO_VOLVER_A_REPOSAR:
                    {
                        generarLogs(ESTADO_CREANDO_EJERCITO_CARPINCHOS, EVENTO_VOLVER_A_REPOSAR);

                        detenerSonidoPiezo();
                        cerrarCompuerta();
                        apagarHiptonizador();

                        estado = ESTADO_REPOSANDO;
                    }
                    break;

                    case EVENTO_AJUSTAR_CHICHARRA:
                    {
                        generarLogs(ESTADO_CREANDO_EJERCITO_CARPINCHOS, EVENTO_AJUSTAR_CHICHARRA);

                        reproducirSonidoPiezo();

                        estado = ESTADO_CREANDO_EJERCITO_CARPINCHOS;

                    }
                    break;

                    case EVENTO_PESAR_CARPINCHOS:
                    {
                        generarLogs(ESTADO_CREANDO_EJERCITO_CARPINCHOS, EVENTO_PESAR_CARPINCHOS);

                        estado = ESTADO_CREANDO_EJERCITO_CARPINCHOS;
                    }
                    break;
                    
                    case EVENTO_PRENDER_HIPNOTIZADOR:
                    {
                        generarLogs(ESTADO_CREANDO_EJERCITO_CARPINCHOS, EVENTO_PRENDER_HIPNOTIZADOR);

                        prenderLeds();
                        detenerSonidoPiezo();

                        estado = ESTADO_HIPNOTIZANDO_CARPINCHOS;
                    }
                    break;
                }
            }
            break;

            case ESTADO_HIPNOTIZANDO_CARPINCHOS:
            {
                switch(evento.tipo)
                {
                    case EVENTO_VOLVER_A_REPOSAR:
                    {
                        generarLogs(ESTADO_HIPNOTIZANDO_CARPINCHOS, EVENTO_VOLVER_A_REPOSAR);

                        detenerSonidoPiezo();
                        cerrarCompuerta();
                        apagarHiptonizador();

                        estado = ESTADO_REPOSANDO;
                    }
                    break;

                    case EVENTO_PULSAR_BOTON:
                    {
                        generarLogs(ESTADO_HIPNOTIZANDO_CARPINCHOS, EVENTO_PULSAR_BOTON);

                        abrirCompuerta();
                        apagarHiptonizador();

                        estado = ESTADO_INVADIENDO_NORDELTA;
                    } 
                    break;

                    case EVENTO_CONTINUAR:
                    {
                        generarLogs(ESTADO_HIPNOTIZANDO_CARPINCHOS, EVENTO_CONTINUAR);

                        prenderLeds();

                        estado = ESTADO_HIPNOTIZANDO_CARPINCHOS;
                    } 
                    break;
                 }
            }
            break;

            case ESTADO_INVADIENDO_NORDELTA:
            {
                switch(evento.tipo)
                {
                    case EVENTO_VOLVER_A_REPOSAR:
                    {
                        generarLogs(ESTADO_INVADIENDO_NORDELTA, EVENTO_VOLVER_A_REPOSAR);

                        detenerSonidoPiezo();
                        cerrarCompuerta();
                        apagarHiptonizador();

                        estado = ESTADO_REPOSANDO;
                    }
                    break;

                    case EVENTO_CERRAR_COMPUERTA:
                    {
                        generarLogs(ESTADO_INVADIENDO_NORDELTA, EVENTO_CERRAR_COMPUERTA);

                        cerrarCompuerta();

                        estado = ESTADO_REPOSANDO;
                    }
                    break;
                }
            }
            break;
        }
    }
//-----------------------------------------------------

//Funcion loop
//-----------------------------------------------------
    void loop(){
        maquina_estados();
    }
//-----------------------------------------------------

