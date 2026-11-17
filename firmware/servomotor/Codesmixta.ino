
#include <Servo.h>

Servo aguja;


const int pinPotenciometro = A0;
const int pinServo = 9;


const int zonaMuerta = 8;
const unsigned long intervaloLectura = 20;

int anguloActual = 0;
int ultimaLecturaPot = 0;
int ultimaLecturaAceptada = 0;

unsigned long tiempoAnterior = 0;

char buffer[20];
byte indice = 0;

void setup()
{
    Serial.begin(9600);

    aguja.attach(pinServo);

    int lecturaInicial = analogRead(pinPotenciometro);

    anguloActual = map(lecturaInicial, 0, 1023, 0, 180);

    ultimaLecturaPot = lecturaInicial;
    ultimaLecturaAceptada = lecturaInicial;

    aguja.write(anguloActual);

    Serial.print("S:");
    Serial.print(anguloActual);
    Serial.println(",POT");
}

void loop()
{
    recibirDatosUnity();
    leerPotenciometro();
}


void recibirDatosUnity()
{
    while (Serial.available() > 0)
    {
        char caracter = Serial.read();

        if (caracter == '\n')
        {
            buffer[indice] = '\0';

            if (buffer[0] == 'A' && buffer[1] == ':')
            {
                int nuevoAngulo = atoi(buffer + 2);

                nuevoAngulo = constrain(nuevoAngulo, 0, 180);

                anguloActual = nuevoAngulo;

                aguja.write(anguloActual);

                ultimaLecturaAceptada = analogRead(pinPotenciometro);

                Serial.print("S:");
                Serial.print(anguloActual);
                Serial.println(",DIG");
            }

            indice = 0;
        }
        else if (caracter != '\r')
        {
            if (indice < sizeof(buffer) - 1)
            {
                buffer[indice++] = caracter;
            }
            else
            {
                indice = 0;
            }
        }
    }
}


void leerPotenciometro()
{
    if (millis() - tiempoAnterior < intervaloLectura)
        return;

    tiempoAnterior = millis();

    int lectura = analogRead(pinPotenciometro);

    if (abs(lectura - ultimaLecturaPot) >= zonaMuerta)
    {
        ultimaLecturaPot = lectura;

        if (abs(lectura - ultimaLecturaAceptada) >= zonaMuerta)
        {
            int nuevoAngulo = map(lectura, 0, 1023, 0, 180);

            anguloActual = nuevoAngulo;

            aguja.write(anguloActual);

            ultimaLecturaAceptada = lectura;

            Serial.print("S:");
            Serial.print(anguloActual);
            Serial.println(",POT");
        }
    }
}
