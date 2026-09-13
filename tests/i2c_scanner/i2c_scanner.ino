/*
 * Teste 1 de 3 — Scanner I2C
 *
 * Varre o barramento I2C e lista os endereços que responderam.
 * O OLED SSD1306 deve aparecer como 0x3C (alguns módulos usam 0x3D).
 *
 * Placa: WEMOS D1 R32 (ESP32)  |  Serial Monitor: 115200 baud
 * Ligação: OLED VCC->3V3, GND->GND, SDA->GPIO21, SCL->GPIO22
 *
 * Se nada aparecer, o problema é fiação: confira VCC/GND antes de tudo,
 * depois se SDA e SCL não estão trocados.
 */

#include <Wire.h>

const int PINO_SDA = 21;
const int PINO_SCL = 22;

void setup() {
  Serial.begin(115200);
  delay(1000);  // dá tempo do Serial Monitor conectar

  Wire.begin(PINO_SDA, PINO_SCL);

  Serial.println();
  Serial.println(F("=== Scanner I2C ==="));
  Serial.printf("SDA = GPIO%d, SCL = GPIO%d\n\n", PINO_SDA, PINO_SCL);
}

void loop() {
  int encontrados = 0;

  Serial.println(F("Varrendo..."));

  for (uint8_t endereco = 1; endereco < 127; endereco++) {
    Wire.beginTransmission(endereco);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  dispositivo em 0x%02X", endereco);
      if (endereco == 0x3C || endereco == 0x3D) {
        Serial.print(F("   <-- provavelmente o OLED SSD1306"));
      }
      Serial.println();
      encontrados++;
    }
  }

  if (encontrados == 0) {
    Serial.println(F("  nenhum dispositivo encontrado."));
    Serial.println(F("  Confira: VCC no 3V3, GND, SDA no GPIO21, SCL no GPIO22."));
  } else {
    Serial.printf("Total: %d dispositivo(s).\n", encontrados);
  }

  Serial.println();
  delay(5000);
}
