/*
 * Teste 2 de 2 — BitDogLab (Raspberry Pi Pico W): o display OLED da placa
 *
 * Desenha um texto e um contador. Se aparecer, o display e o barramento
 * estão certos e o sketch do controle vai conseguir usá-los.
 *
 * O OLED da BitDogLab NÃO está nos pinos I2C padrão do Pico: ele fica no
 * barramento I2C1, em GP14 (SDA) e GP15 (SCL). Por isso o código usa "Wire1"
 * e chama setSDA/setSCL ANTES de Wire1.begin() — no core do Pico, remapear
 * os pinos depois do begin() não tem efeito.
 *
 * Placa: Raspberry Pi Pico W  |  Serial Monitor: 115200 baud
 * Bibliotecas: Adafruit SSD1306, Adafruit GFX
 *
 * "OLED nao encontrado"? O endereço pode ser 0x3D em vez de 0x3C —
 * troque a constante abaixo e grave de novo.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PINO_SDA = 14;
const int PINO_SCL = 15;
const uint8_t ENDERECO_OLED = 0x3C;

Adafruit_SSD1306 display(128, 64, &Wire1, -1);
int contador = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire1.setSDA(PINO_SDA);
  Wire1.setSCL(PINO_SCL);
  Wire1.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED)) {
    Serial.println("OLED nao encontrado no 0x3C — tente 0x3D");
    while (true) delay(1000);
  }

  Serial.println("OLED: ok");
  display.clearDisplay();
  display.display();
}

void loop() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("BitDogLab OK");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(3);
  display.setCursor(18, 28);
  display.print(contador);

  display.display();

  Serial.printf("contador = %d\n", contador);
  contador++;
  delay(1000);
}
