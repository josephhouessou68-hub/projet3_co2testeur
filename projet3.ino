#include <string.h>          
#include <LiquidCrystal.h>  
#include <Wire.h>           // Indispensable pour la liaison I2C entre les Arduinos
#include "CapteurMQ135.h"

// Configuration des broches pour l'afficheur LCD 16x2
const int rs = 11, en = 10, d4 = 5, d5 = 4, d6 = 3, d7 = 2;  
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  

// Définition des broches et des constantes du système
#define mqpin A0             // Entrée analogique du capteur MQ-135
#define rl 20.0              // Résistance de charge (en Kohms)
#define cleanair 3.6         // Ratio de calibration en air pur
#define buzzer 8             // Broche de la sirène
#define buttom 9             // Bouton poussoir pour le reset
#define NB_ECHANTILLONS 50   // Nombre de mesures pour lisser le calibrage

bool alarmeDeclenchee = false; // Petit flag pour éviter de relancer le buzzer en boucle

// Instanciation de notre objet capteur personnalisé
CapteurMQ135 capteur(mqpin, rl, cleanair);
String commandeRecue = "";   // Tampon pour lire le port série sans bloquer le programme

// Fonction de callback pour afficher l'avancement du calibrage directement sur le LCD
void callbackCalibrationLCD(int etape, int total) {
  lcd.setCursor(0, 1);
  lcd.print("Etape : ");
  lcd.print(etape);
  lcd.print("/");
  lcd.print(total);
}

void reset_system();     
void ecouter_ordinateur();

void setup() {    
  pinMode(LED_BUILTIN, OUTPUT);    
  pinMode(buzzer, OUTPUT);                 
  pinMode(buttom, INPUT_PULLUP);          
  Serial.begin(9600); 
  
  Wire.begin(); // On rejoint le bus I2C en tant que Maître
  lcd.begin(16, 2);  
  
  // Petit temps de chauffe pour stabiliser les mesures du capteur
  Serial.println("Chauffage du capteur en cours (20s)...");    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Chauffage capteur");
  lcd.setCursor(0, 1);
  lcd.print("Patientez svp...");
  delay(2000);  
  
  // Calibrage initial de Ro
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrage Ro...");
  
  float roCalcule = capteur.calibrateRo(NB_ECHANTILLONS, callbackCalibrationLCD);
  capteur.setRo(roCalcule);
  
  Serial.print("Ro initialisé à : ");    
  Serial.println(capteur.getRo());  

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ro calcule :");
  lcd.setCursor(0, 1);
  lcd.print(capteur.getRo());
  delay(2000); 
}  

void loop() {  
  pinMode(buttom, INPUT_PULLUP);
  ecouter_ordinateur();  // On écoute si l'ordinateur nous envoie des ordres
  reset_system();        // On vérifie si l'utilisateur veut réinitialiser

  // Petit battement de cœur visuel sur la LED intégrée pour montrer que tout tourne
  digitalWrite(LED_BUILTIN, HIGH);      
  delay(250);                           
  digitalWrite(LED_BUILTIN, LOW);       
  delay(250);      
  
  // Récupération des calculs d'alcool depuis la classe
  float ppm = capteur.getPPM(ALCOOL);   
  float gram = capteur.ppmToGramPerLitre(ppm); 
  String ivresse = capteur.getEtatAlcool(ppm, buzzer, alarmeDeclenchee);     
  
  // Envoi des données sur le port série (pour un script externe ou le moniteur)
  Serial.print("DATA:");    
  Serial.print(ivresse);    
  Serial.print(";");    
  Serial.print(ppm);    
  Serial.print(";");    
  Serial.println(gram);     
  
  // --- Envoi des données vers le second Arduino (Esclave - Adresse 8) via I2C ---
  Wire.beginTransmission(8); 
  Wire.write("PPM:");        
  Wire.write(String(ppm).c_str()); 
  Wire.endTransmission();    
  ------------------------------------------------------------------------------

  // Actualisation propre de l'écran LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Etat: ");
  lcd.print(ivresse);
  
  lcd.setCursor(0, 1);
  lcd.print("PPM: ");
  lcd.print(ppm, 1);  
  lcd.print(" g/L:");
  lcd.print(gram, 1);
  
  delay(1000);  
}  

// Gestion du bouton physique pour relancer un calibrage à la volée
void reset_system() {    
  if (digitalRead(buttom) == HIGH) {     
    Serial.println("Reset demandé par l'utilisateur...");     
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Remise a zero...");
    delay(1000);
    
    noTone(buzzer);     
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nouveau calibrage");
    float roCalcule = capteur.calibrateRo(NB_ECHANTILLONS, callbackCalibrationLCD);
    capteur.setRo(roCalcule);
    
    delay(300);   
  } 
}

// Lecture non bloquante du port série pour recevoir des commandes distantes
void ecouter_ordinateur() {
  while (Serial.available() > 0) {
    char c = Serial.read(); 
    
    if (c == '\n') { // Fin de ligne détectée, on traite la commande
      commandeRecue.trim(); 

      if (commandeRecue == "STOP_BUZZER") {
        noTone(buzzer);
        Serial.println("RPT:Buzzer coupé manuellement");
      }
      else if (commandeRecue == "SONNER") {
        alarmeDeclenchee = false; // Réarme l'alarme
        Serial.println("RPT:Ordre de sonnerie reçu");
      }
      else if (commandeRecue == "RECALIBRER") {
        Serial.println("RPT:Calibrage distant en cours...");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Calibrage distant");
        float roCalcule = capteur.calibrateRo(NB_ECHANTILLONS, callbackCalibrationLCD);
        capteur.setRo(roCalcule);
      }
      else if (commandeRecue.length() > 0) {
        Serial.print("RPT:Commande inconnue (");
        Serial.print(commandeRecue);
        Serial.println(")");
      }
      
      commandeRecue = ""; // On nettoie le tampon pour la prochaine commande
    } 
    else if (c != '\r') {
      commandeRecue += c; // On assemble les caractères reçus
    }
  }
}
