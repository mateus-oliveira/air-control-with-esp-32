/*
 * Controle remoto — Elgin Inverter 9000 BTUs
 *
 * Protocolo ELECTRA_AC (descoberto com o sketch 02_scanner_protocolo).
 *
 * TECLAS (Serial Monitor a 115200):
 *   P   liga / desliga
 *   A   aumenta a temperatura
 *   D   diminui a temperatura
 *   V   liga / desliga o visor do ar
 *   O   liga / desliga a oscilacao das aletas
 *   SL  velocidade baixa do ventilador
 *   SM  velocidade media do ventilador
 *   SF  velocidade alta do ventilador
 *
 * BOTÕES FÍSICOS (dois):
 *   botao de cima sozinho    aumenta a temperatura
 *   botao de baixo sozinho   diminui a temperatura
 *   os dois ao mesmo tempo   liga / desliga
 *
 * Placa: WEMOS D1 R32 (ESP32)
 * Bibliotecas: IRremoteESP8266, Adafruit SSD1306, Adafruit GFX
 *
 * Ligação: GPIO26 -> resistor 220R -> anodo do LED IR; catodo -> GND
 *          OLED: VCC 3V3, GND, SDA GPIO21, SCL GPIO22
 *          Botao aumentar: GPIO25 -> GND
 *          Botao diminuir: GPIO17 -> GND
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
const int PINO_BTN_MAIS = 25;
const int PINO_BTN_MENOS = 17;
const uint8_t ENDERECO_OLED = 0x3C;
const int TEMP_MIN = 16;
const int TEMP_MAX = 32;

IRac ac(PINO_LED_IR);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool temDisplay = false;

bool ligado = false;
bool visor = true;
bool oscilando = false;
int temperatura = 24;
stdAc::fanspeed_t velocidade = stdAc::fanspeed_t::kLow;

// O comando de velocidade tem duas letras ("sl", "sm", "sf"). Ao receber o 's'
// guardamos que a proxima letra completa o comando; ela costuma chegar na
// volta seguinte do loop.
bool aguardandoVelocidade = false;

// Botões ligados entre o GPIO e o GND, usando o pull-up interno: em repouso
// o pino lê HIGH, pressionado lê LOW. Não precisa de resistor externo.
bool maisAntes = false;
bool menosAntes = false;
bool comboJaDisparou = false;
uint32_t ultimaLeitura = 0;

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

const char* nomeVelocidade() {
  switch (velocidade) {
    case stdAc::fanspeed_t::kLow:    return "low";
    case stdAc::fanspeed_t::kMedium: return "medium";
    case stdAc::fanspeed_t::kHigh:   return "fast";
    default:                         return "?";
  }
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
  ac.next.fanspeed = velocidade;
  ac.next.swingv = oscilando ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff;
  ac.next.swingh = stdAc::swingh_t::kOff;
  ac.next.light = visor;
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
    Serial.printf("LIGADO  %d C  visor %s  vel %s  oscilar %s\n", temperatura,
                  visor ? "on" : "off", nomeVelocidade(),
                  oscilando ? "on" : "off");
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

void mudarVelocidade(stdAc::fanspeed_t nova) {
  velocidade = nova;
  enviar();
}

/*
 * Lê os dois botões. Um sozinho muda a temperatura; os dois juntos ligam ou
 * desligam o aparelho.
 *
 * A temperatura age ao SOLTAR o botão, não ao apertar: como é impossível
 * apertar os dois exatamente no mesmo instante, agir no aperto faria o
 * primeiro botão mudar a temperatura antes de o segundo chegar.
 *
 * Ler só a cada 30 ms já elimina o repique mecânico dos contatos.
 */
void lerBotoes() {
  if (millis() - ultimaLeitura < 30) return;
  ultimaLeitura = millis();

  bool mais = (digitalRead(PINO_BTN_MAIS) == LOW);
  bool menos = (digitalRead(PINO_BTN_MENOS) == LOW);

  // Os dois apertados: liga/desliga, uma vez só por combo.
  if (mais && menos && !comboJaDisparou) {
    comboJaDisparou = true;
    ligado = !ligado;
    enviar();
  }

  // Soltou: só vale se este aperto não fez parte de um combo.
  if (maisAntes && !mais && !comboJaDisparou)   mudarTemperatura(+1);
  if (menosAntes && !menos && !comboJaDisparou) mudarTemperatura(-1);

  // Libera o próximo combo só depois que os dois estiverem soltos. Precisa
  // vir depois dos testes acima, senão o segundo botão a ser solto escaparia
  // e mudaria a temperatura.
  if (!mais && !menos) comboJaDisparou = false;

  maisAntes = mais;
  menosAntes = menos;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PINO_BTN_MAIS, INPUT_PULLUP);
  pinMode(PINO_BTN_MENOS, INPUT_PULLUP);

  Wire.begin(PINO_SDA, PINO_SCL);
  temDisplay = display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED);

  Serial.println();
  Serial.println("=== Controle Elgin (ELECTRA_AC) ===");
  Serial.println("P = liga/desliga | A = aumentar | D = diminuir");
  Serial.println("V = visor | O = oscilar");
  Serial.println("SL / SM / SF = velocidade baixa / media / alta");
  Serial.println("Botoes: um de cada vez = temperatura | os dois = liga/desliga");
  Serial.printf("OLED: %s\n", temDisplay ? "ok" : "nao encontrado (segue sem ele)");
  Serial.printf("Estado inicial: DESLIGADO, %d C, vel %s\n\n", temperatura,
                nomeVelocidade());

  atualizarTela();
}

void loop() {
  lerBotoes();

  if (!Serial.available()) return;

  char c = Serial.read();

  // Segunda letra de "s?": se nao for uma velocidade conhecida, o comando é
  // descartado e a letra não cai no switch abaixo.
  if (aguardandoVelocidade) {
    aguardandoVelocidade = false;

    switch (c) {
      case 'l': case 'L': mudarVelocidade(stdAc::fanspeed_t::kLow);    return;
      case 'm': case 'M': mudarVelocidade(stdAc::fanspeed_t::kMedium); return;
      case 'f': case 'F': mudarVelocidade(stdAc::fanspeed_t::kHigh);   return;
      default:
        Serial.println("Velocidade invalida: use SL, SM ou SF");
        return;
    }
  }

  switch (c) {
    case 's':
    case 'S':
      aguardandoVelocidade = true;
      break;

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

    case 'v':
    case 'V':
      visor = !visor;
      enviar();
      break;

    case 'o':
    case 'O':
      oscilando = !oscilando;
      enviar();
      break;
  }
}
