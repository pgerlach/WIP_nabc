
// le lecteur rfid est connecté en i2c, géré par la bibliothèque Wire
#include <Wire.h>
// on inclut la bibliothèque pour le lecteur RFID
#include "CR14_rfid.h"

// On utilise la bibliothèque standard Servo
#include <Servo.h>

// cette variable nous permet de parler avec le lecteur RFID
Rfid rfid;

// Cette variable permettra de controller notre servo moteur
Servo servo;

// TODO, ici remplacer par les bonnes valeurs
const char* tagPourOuvrir = "01234567890ABCDE";
const char* tagPourFermer = "ABCDEF0123456789";

void setup() {
  Serial.begin(9600);

  // on règle le servo-moteur et on se remet en position ouvert (0)
  servo.attach(3, 400, 2300);
  servo.write(0);

  // le lecteur RFID est connecté en i2c : c'est la bibliothèque Wire qui le gère.
  Wire.begin();
  // on initialise le lecteur RFID (en pratique, il allume son champ électro-magnétique)
  rfid.init();
}


void loop() {
  // on ne fait des actions que si on lit un tag RFID
  if (rfid.read()) {
    // est-ce le tag pour ouvrir ou pour fermer ?
    if (!strcmp(rfid.currentTag, tagPourOuvrir)) {
      Serial.println("ouvrir");
      servo.write(0);
    } else if (!strcmp(rfid.currentTag, tagPourFermer)) {
      Serial.println("fermer");
      servo.write(180);
    }
  }
  delay(500);
}

