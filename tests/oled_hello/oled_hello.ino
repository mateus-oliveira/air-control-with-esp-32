/*
 * Teste 2 de 3 — OLED SSD1306
 *
 * Desenha texto e um contador na tela, provando que o display está
 * inicializando e recebendo dados.
 *
 * Placa: WEMOS D1 R32 (ESP32)  |  Serial Monitor: 115200 baud
 * Bibliotecas: "Adafruit SSD1306" e "Adafruit GFX Library"
 *
 * Se a tela ficar apagada mas o scanner I2C tiver achado o endereço,
 * tente trocar ENDERECO_OLED para 0x3D.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PINO_SDA = 21;
const int PINO_SCL = 22;

const int LARGURA = 128;
const int ALTURA = 64;              // módulos de 0.96" são 128x64; os de 0.91" são 128x32
const uint8_t ENDERECO_OLED = 0x3C;

Adafruit_SSD1306 display(LARGURA, ALTURA, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(PINO_SDA, PINO_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED)) {
    Serial.println(F("Falha ao iniciar o OLED. Rode o i2c_scanner e confira o endereco."));
    while (true) delay(1000);
  }

  Serial.println(F("OLED iniciado com sucesso."));

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Controle Elgin"));
  display.drawLine(0, 10, LARGURA - 1, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(F("OLED OK"));

  display.setTextSize(1);
  display.setCursor(0, 48);
  display.println(F("Proximo: LED IR"));

  display.display();
  delay(3000);
}

void loop() {
  static uint32_t contador = 0;

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Display funcionando"));

  display.setTextSize(3);
  display.setCursor(0, 24);
  display.println(contador);

  display.display();

  Serial.printf("contador = %lu\n", contador);
  contador++;
  delay(1000);
}
