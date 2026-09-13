/*
 * Controle remoto — Elgin Inverter 9000 BTUs
 *
 * Protocolo ELECTRA_AC (descoberto com o sketch 02_scanner_protocolo).
 *
 * TECLAS (Serial Monitor a 115200):
 *   P   liga / desliga
 *   A   aumenta a temperatura
 *   D   diminui a temperatura
 *
 * Placa: WEMOS D1 R32 (ESP32)
 * Bibliotecas: IRremoteESP8266, Adafruit SSD1306, Adafruit GFX
 *
 * Ligação: GPIO26 -> resistor 220R -> anodo do LED IR; catodo -> GND
 *          OLED: VCC 3V3, GND, SDA GPIO21, SCL GPIO22
 */

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRac.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const uint16_t PINO_LED_IR = 26;
const int PINO_SDA = 21;
const int PINO_SCL = 22;
const uint8_t ENDERECO_OLED = 0x3C;
const int TEMP_MIN = 16;
const int TEMP_MAX = 32;

IRac ac(PINO_LED_IR);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool temDisplay = false;

bool ligado = false;
int temperatura = 24;

// Se o OLED não for encontrado, tudo aqui vira no-op e o controle segue
// funcionando normalmente pelo Serial.
void atualizarTela() {
  if (!temDisplay) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(ligado ? "LIGADO" : "DESLIGADO");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  if (ligado) {
    display.setTextSize(4);
    display.setCursor(18, 24);
    display.print(temperatura);

    display.setTextSize(2);
    display.setCursor(82, 26);
    display.print("C");
  } else {
    display.setTextSize(2);
    display.setCursor(16, 30);
    display.print("-- C");
  }

  display.display();
}

// Ar Inverter não tem "código por botão": cada comando transmite o estado
// inteiro do aparelho, com checksum. A IRac monta esse quadro para nós.
void enviar() {
  ac.next.protocol = decode_type_t::ELECTRA_AC;
  ac.next.model = 1;
  ac.next.power = ligado;
  ac.next.mode = stdAc::opmode_t::kCool;
  ac.next.celsius = true;
  ac.next.degrees = temperatura;
  ac.next.fanspeed = stdAc::fanspeed_t::kAuto;
  ac.next.swingv = stdAc::swingv_t::kOff;
  ac.next.swingh = stdAc::swingh_t::kOff;
  ac.next.light = true;
  ac.next.beep = true;
  ac.next.econo = false;
  ac.next.filter = false;
  ac.next.turbo = false;
  ac.next.quiet = false;
  ac.next.clean = false;
  ac.next.sleep = -1;
  ac.next.clock = -1;

  ac.sendAc();
  atualizarTela();

  if (ligado) {
    Serial.printf("LIGADO  %d C\n", temperatura);
  } else {
    Serial.println("DESLIGADO");
  }
}

void mudarTemperatura(int passo) {
  int nova = constrain(temperatura + passo, TEMP_MIN, TEMP_MAX);

  if (nova == temperatura) {
    Serial.printf("Limite: %d C\n", temperatura);
    return;
  }

  temperatura = nova;
  enviar();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(PINO_SDA, PINO_SCL);
  temDisplay = display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED);

  Serial.println();
  Serial.println("=== Controle Elgin (ELECTRA_AC) ===");
  Serial.println("P = liga/desliga | A = aumentar | D = diminuir");
  Serial.printf("OLED: %s\n", temDisplay ? "ok" : "nao encontrado (segue sem ele)");
  Serial.printf("Estado inicial: DESLIGADO, %d C\n\n", temperatura);

  atualizarTela();
}

void loop() {
  if (!Serial.available()) return;

  char c = Serial.read();

  switch (c) {
    case 'p':
    case 'P':
      ligado = !ligado;
      enviar();
      break;

    case 'a':
    case 'A':
      mudarTemperatura(+1);
      break;

    case 'd':
    case 'D':
      mudarTemperatura(-1);
      break;
  }
}
