#ifndef CAPTEUR_MQ135_H
#define CAPTEUR_MQ135_H

#include <Arduino.h>

enum Gaz {
  CO2,
  ALCOOL
};

class CapteurMQ135 {
  private:
    int _pin;
    float _rl;
    float _cleanAirRatio;
    float _ro;
    void _declencherBuzzer(int buzzerPin, bool &alarmeDeclenchee);

  public:
    CapteurMQ135(int pin, float rl, float cleanAirRatio);

    float readRS();
    float calibrateRo(int nbEchantillons, void (*callbackEtape)(int, int) = NULL);
    
    void setRo(float ro);
    float getRo();

    float getPPM(Gaz typeGaz);
    float ppmToGramPerLitre(float ppm);
    
    // Les deux fonctions d'état demandées
    String getEtatAlcool(float ppm, int buzzerPin, bool &alarmeDeclenchee);
    String getEtatCO2(float ppm, int buzzerPin, bool &alarmeDeclenchee);
};

#endif
