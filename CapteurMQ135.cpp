#include "CapteurMQ135.h"

CapteurMQ135::CapteurMQ135(int pin, float rl, float cleanAirRatio) {
  _pin = pin;
  _rl = rl;
  _cleanAirRatio = cleanAirRatio;
  _ro = 10.0; 
}

float CapteurMQ135::readRS() {
  int raw = analogRead(_pin);   
  float volt = raw * (5.0 / 1023.0);   
  if (volt == 0) volt = 0.01;    
  float rp = 5.0 * _rl;   
  float RS = (rp / volt) - _rl;   
  return RS; 
}

float CapteurMQ135::calibrateRo(int nbEchantillons, void (*callbackEtape)(int, int)) {
  float rsSum = 0;   
  for (int i = 0 ; i < nbEchantillons; i++ ){     
    rsSum += readRS();     
    if (callbackEtape != NULL) {
      callbackEtape(i + 1, nbEchantillons);
    }
    delay(100); 
  }   
  float rsA = rsSum / (float)nbEchantillons;   
  _ro = rsA / _cleanAirRatio;   
  return _ro; 
}

void CapteurMQ135::setRo(float ro) { _ro = ro; }
float CapteurMQ135::getRo() { return _ro; }

float CapteurMQ135::getPPM(Gaz typeGaz) {
  float rs = readRS();   
  float ratio = rs / _ro;   
  
  float a = 0.0;
  float b = 0.0;

  switch(typeGaz) {
    case CO2:
      a = 110.47;  
      b = -2.862;  
      break;
    case ALCOOL:
      a = 77.25;   
      b = -3.18;   
      break;
  }

  float ppm = a * pow(ratio, b);   
  return ppm; 
}

float CapteurMQ135::ppmToGramPerLitre(float ppm) {
  return ppm * 1000.0; 
}

// Fonction privée d'aide pour faire biper le buzzer 3 fois sans bloquer indéfiniment
void CapteurMQ135::_declencherBuzzer(int buzzerPin, bool &alarmeDeclenchee) {
  if (!alarmeDeclenchee) {
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1500); 
      delay(200);         
      noTone(buzzerPin);     
      delay(200);         
    }
    alarmeDeclenchee = true; 
  }
}

// ÉTAT ALCOOL (Anciennement getetativresse)
String CapteurMQ135::getEtatAlcool(float ppm, int buzzerPin, bool &alarmeDeclenchee) {
  if (ppm < 100.0) { 
    return "Sobre"; 
  }   
  else if (ppm < 250.0) { 
    return "Traces detectees"; 
  } 
  
  String libelle = "";
  if (ppm < 450.0)       libelle = "Seuil legal";   
  else if (ppm < 1000.0) libelle = "Ivresse...";   
  else                   libelle = "Intox severe";   

  _declencherBuzzer(buzzerPin, alarmeDeclenchee);
  return libelle; 
}

// ÉTAT CO2 (Nouveaux seuils basés sur la qualité de l'air)
String CapteurMQ135::getEtatCO2(float ppm, int buzzerPin, bool &alarmeDeclenchee) {
  if (ppm < 800.0) {
    return "Air Excellent";
  }
  else if (ppm < 1200.0) {
    return "Air Confine";
  }
  
  String libelle = "";
  if (ppm < 2000.0)      libelle = "Maux de tete";
  else if (ppm < 5000.0) libelle = "Intox Moderee";
  else                   libelle = "Danger/Asphyxie";

  _declencherBuzzer(buzzerPin, alarmeDeclenchee);
  return libelle;
}
