#include <Arduino.h>

const int dacPin = 25;  // DAC1 en GPIO25

const int waveformLength = 100;  // muestras por latido

// 5 formas de onda PQRST simuladas (0 normal, 1 a 4 patologías)
const uint8_t ecgNormal[waveformLength] = {
  128,130,132,134,136,138,140,145,150,155,
  150,145,140,138,136,134,132,130,128,126,
  124,122,160,200,220,200,160,140,135,130,
  128,125,124,122,120,118,117,116,115,114,
  115,116,118,120,125,130,135,138,140,138,
  135,130,128,126,124,122,120,118,117,116,
  115,114,113,112,111,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110
};

const uint8_t ecgPat1[waveformLength] = {
  128,128,128,128,128,128,128,128,128,128,  // Onda P ausente (línea base)
  128,128,128,128,128,128,128,128,128,128,
  124,120,160,190,210,190,160,140,135,130,  // QRS más ancho y lento
  128,125,124,123,122,121,120,119,118,117,
  118,119,120,121,122,123,124,125,126,127,
  126,125,124,123,122,121,120,119,118,117,
  116,115,114,113,112,111,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110
};

const uint8_t ecgPat2[waveformLength] = {
  // Onda P más ancha (20 muestras)
  120,121,122,123,124,125,126,127,128,129,
  130,131,132,133,134,135,136,137,138,139,
  
  // Descenso P (20 muestras)
  138,136,134,132,130,128,126,124,122,120,
  118,116,114,112,110,108,106,104,102,100,
  
  // Complejo QRS (20 muestras)
  98,95,140,180,210,220,210,190,170,150,
  130,110,90,70,65,60,55,50,45,40,
  
  // Descenso S y onda T (20 muestras)
  38,36,34,32,30,28,26,24,22,20,
  25,30,35,40,45,50,55,60,65,70,
  
  // Reposo más largo (20 muestras)
  60,60,60,60,60,60,60,60,60,60,
  60,60,60,60,60,60,60,60,60,60
};

const uint8_t ecgPat3[waveformLength] = {
  128,131,133,135,137,140,143,146,149,154,
  149,144,139,137,135,133,130,128,126,124,
  122,119,162,202,225,202,162,142,137,133,
  130,126,125,122,120,118,117,116,115,114,
  115,116,118,120,124,129,134,137,139,137,
  134,129,127,125,123,121,119,118,117,116,
  115,114,113,112,111,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110,
  110,110,110,110,110,110,110,110,110,110
};

const uint8_t ecgPat4[waveformLength] = {
  128,133,135,137,139,142,145,148,152,157,
  152,147,142,140,138,136,133,131,129,127,
  125,122,168,208,235,208,168,148,143,138,
  135,131,130,127,125,123,122,121,120,119,
  120,121,123,125,130,135,140,143,145,143,
  140,135,133,131,129,127,125,123,122,121,
  120,119,118,117,116,115,115,115,115,115,
  115,115,115,115,115,115,115,115,115,115,
  115,115,115,115,115,115,115,115,115,115,
  115,115,115,115,115,115,115,115,115,115
};

const uint8_t* patologias[] = {ecgNormal, ecgPat1, ecgPat2, ecgPat3, ecgPat4};
const int NUM_PATOLOGIAS = sizeof(patologias) / sizeof(patologias[0]);

int currentPathology = 0;
int indexWav = 0;
unsigned long lastUpdate = 0;

int bpm = 60;
unsigned long intervaloLatido = 1000;
unsigned long tiempoMuestreo = 5;
bool enReposo = false;
unsigned long inicioReposo = 0;
unsigned long duracionReposo = 500;

void calcularDuracionReposo() {
  intervaloLatido = 60000 / bpm;
  duracionReposo = intervaloLatido - (waveformLength * tiempoMuestreo);
  if (duracionReposo < 0) duracionReposo = 0;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(">> Simulador ECG DAC listo");
  Serial.println(">> Comando: BPM <valor> (30-180)");
  Serial.println(">> Comando: PAT <0-4> (selecciona patología)");
  calcularDuracionReposo();
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.startsWith("BPM ")) {
      int newBpm = cmd.substring(4).toInt();
      if (newBpm >= 30 && newBpm <= 180) {
        bpm = newBpm;
        calcularDuracionReposo();
        Serial.print(">> BPM actualizado a: ");
        Serial.println(bpm);
      } else {
        Serial.println(">> BPM fuera de rango (30-180)");
      }
    } else if (cmd.startsWith("PAT ")) {
      int newPat = cmd.substring(4).toInt();
      if (newPat >= 0 && newPat < NUM_PATOLOGIAS) {
        currentPathology = newPat;
        Serial.print(">> Patología seleccionada: ");
        Serial.println(currentPathology);
      } else {
        Serial.println(">> Patología inválida");
      }
    } else {
      Serial.println(">> Comando desconocido");
    }
  }

  unsigned long now = millis();

  if (!enReposo) {
    if (now - lastUpdate >= tiempoMuestreo) {
      dacWrite(dacPin, patologias[currentPathology][indexWav]);
      lastUpdate = now;
      indexWav++;
      if (indexWav >= waveformLength) {
        enReposo = true;
        inicioReposo = now;
        indexWav = 0;
        dacWrite(dacPin, 110);  // línea base en reposo
        Serial.println(">> Latido generado");
      }
    }
  } else {
    if (now - inicioReposo >= duracionReposo) {
      enReposo = false;
    }
  }
}
