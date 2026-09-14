/*
 * Controle remoto — Elgin Inverter 9000 BTUs  |  versão BitDogLab (RP2040)
 *
 * Mesmo controle do sketch esp32/esp32.ino (Wemos D1 R32), portado para a
 * placa BitDogLab da Embarcatech, que tem Raspberry Pi Pico W, dois botões e
 * o display OLED já embutidos. Com a bateria da placa, vira um controle de
 * verdade — sem PC.
 *
 * TECLAS (Serial Monitor a 115200):
 *   P   liga / desliga
 *   A   aumenta a temperatura
 *   D   diminui a temperatura
 *   V   alterna o visor do ar
 *   O   liga / desliga a oscilacao das aletas
 *   SL  velocidade baixa do ventilador
 *   SM  velocidade media do ventilador
 *   SF  velocidade alta do ventilador
 *
 * BOTÕES DA PLACA (os dois que já vêm soldados):
 *   Botao B (direita) sozinho   aumenta a temperatura
 *   Botao A (esquerda) sozinho  diminui a temperatura
 *   os dois ao mesmo tempo      liga / desliga
 *
 * BOTÕES EXTERNOS (dois, na protoboard):
 *   GP16                     velocidade do vento, em ciclo: low > med > fast > low
 *   GP19                     liga / desliga a oscilacao das aletas
 *
 * Placa: Raspberry Pi Pico W (core "Raspberry Pi Pico/RP2040" do Philhower)
 * Bibliotecas: Adafruit SSD1306, Adafruit GFX
 *
 * NÃO usa a IRremoteESP8266: aquela biblioteca só compila em ESP8266/ESP32.
 * O protocolo ELECTRA_AC está reimplementado aqui embaixo, em ~60 linhas.
 *
 * Ligação: GP17 -> resistor 220R -> anodo do LED (IR ou o amarelo de teste);
 *          catodo -> GND
 *          Botao velocidade: GP16 -> GND
 *          Botao oscilacao:  GP19 -> GND
 *          O OLED e os botões A/B já estão ligados na própria placa.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pinos da BitDogLab. Só o LED é fio solto; o resto já vem soldado na placa.
//
// GP17 não é um pino qualquer: os dois canais de um slice de PWM do RP2040
// compartilham a mesma frequência, então a portadora de 38 kHz prende o slice
// inteiro. GP17 está no slice 0, que não é usado por nada da placa. Já GP28
// (slice 6) travaria o LED RGB e ainda disputa o pino com o microfone, e GP20
// (slice 2) travaria o buzzer. GP16, GP18 e GP19 servem igualmente bem.
const int PINO_LED_IR = 17;
const int PINO_SDA = 14;   // OLED da placa: barramento I2C1
const int PINO_SCL = 15;
const int PINO_BTN_MAIS = 6;    // Botão B da placa, o da DIREITA
const int PINO_BTN_MENOS = 5;   // Botão A da placa, o da ESQUERDA
const int PINO_BTN_VEL = 16;    // botão externo: velocidade do vento
const int PINO_BTN_OSC = 19;    // botão externo: oscilação das aletas
const uint8_t ENDERECO_OLED = 0x3C;
const int TEMP_MIN = 16;
const int TEMP_MAX = 32;

Adafruit_SSD1306 display(128, 64, &Wire1, -1);
bool temDisplay = false;

bool ligado = false;
bool visor = true;
bool oscilando = false;
int temperatura = 24;

// A Electra codifica a velocidade nesses três bits; guardar o valor já no
// formato do protocolo evita uma tabela de conversão.
const uint8_t VEL_BAIXA = 0b011;
const uint8_t VEL_MEDIA = 0b010;
const uint8_t VEL_ALTA  = 0b001;
uint8_t velocidade = VEL_BAIXA;

// O comando de velocidade tem duas letras ("sl", "sm", "sf"). Ao receber o 's'
// guardamos que a proxima letra completa o comando; ela costuma chegar na
// volta seguinte do loop.
bool aguardandoVelocidade = false;

// Os botões da BitDogLab já estão ligados entre o GPIO e o GND: com o pull-up
// interno, em repouso o pino lê HIGH e pressionado lê LOW. Igual ao ESP32.
bool maisAntes = false;
bool menosAntes = false;
bool comboJaDisparou = false;
bool velAntes = false;
bool oscAntes = false;
uint32_t ultimaLeitura = 0;

/* ------------------------------------------------------------------------
 * Camada de rádio: portadora de 38 kHz
 *
 * O ESP32 tinha a IRremoteESP8266 para isso. Aqui a portadora sai do PWM
 * por hardware do RP2040: "marca" é o PWM a 38 kHz com 50% de duty, "espaço"
 * é o pino em nível baixo. O tempo é medido com micros(), que no RP2040 vem
 * de um timer de hardware de 1 us — preciso o bastante para IR.
 * --------------------------------------------------------------------- */

// Definidas mais abaixo, junto da parte de interface; enviar() precisa delas.
void atualizarTela();
const char* nomeVelocidade();
const char* nomeVelocidadeCurto();

static inline void esperar(uint32_t us) {
  uint32_t inicio = micros();
  while (micros() - inicio < us) {}
}

static void marcar(uint32_t us) {
  analogWrite(PINO_LED_IR, 128);  // 128/255 ~ 50% de duty
  esperar(us);
  analogWrite(PINO_LED_IR, 0);
}

/* ------------------------------------------------------------------------
 * Protocolo ELECTRA_AC
 *
 * Valores conferidos contra o ir_Electra.cpp da IRremoteESP8266 2.9.0 — a
 * mesma biblioteca que roda no sketch do ESP32, então o quadro transmitido é
 * bit a bit idêntico ao que já funciona no aparelho.
 *
 * O quadro tem 13 bytes. Ar Inverter não tem "código por botão": cada comando
 * transmite o estado inteiro do aparelho, com checksum no último byte.
 * --------------------------------------------------------------------- */

const uint16_t HDR_MARCA   = 9166;
const uint16_t HDR_ESPACO  = 4470;
const uint16_t BIT_MARCA   = 646;
const uint16_t UM_ESPACO   = 1647;
const uint16_t ZERO_ESPACO = 547;
const uint32_t INTERVALO   = 100;  // ms de silêncio depois do quadro

const uint8_t MODO_COOL = 0b001;
const uint8_t SWING_ON  = 0b000;
const uint8_t SWING_OFF = 0b111;
const uint8_t VISOR_ON  = 0x15;
const uint8_t VISOR_OFF = 0x08;

/*
 * O quadro inteiro dura ~195 ms, então NÃO dá para desligar as interrupções
 * durante a transmissão — o USB cairia. Não precisa: a espera é feita contra
 * o relógio absoluto (micros()), e não por contagem de ciclos. Se uma
 * interrupção roubar alguns microssegundos no meio de um pulso, a espera
 * seguinte simplesmente termina no mesmo instante que terminaria antes. O
 * erro não acumula, e receptores IR toleram bem mais folga do que isso.
 */
void enviarQuadro(const uint8_t quadro[13]) {
  marcar(HDR_MARCA);
  esperar(HDR_ESPACO);

  // Bytes na ordem do vetor, bits do menos significativo para o mais.
  for (int i = 0; i < 13; i++) {
    uint8_t b = quadro[i];
    for (int bit = 0; bit < 8; bit++, b >>= 1) {
      marcar(BIT_MARCA);
      esperar((b & 1) ? UM_ESPACO : ZERO_ESPACO);
    }
  }

  marcar(BIT_MARCA);  // rodapé

  delay(INTERVALO);
}

// Monta os 13 bytes a partir do estado atual e transmite.
void enviar() {
  uint8_t quadro[13] = {0};

  quadro[0] = 0xC3;  // marca fixa de início de quadro

  // Byte 1: bits 0-2 oscilação vertical, bits 3-7 temperatura (com offset 8).
  uint8_t swingv = oscilando ? SWING_ON : SWING_OFF;
  quadro[1] = swingv | ((uint8_t)(temperatura - 8) << 3);

  quadro[2] = SWING_OFF << 5;    // oscilação horizontal: sempre desligada
  quadro[4] = velocidade << 5;   // bits 5-7
  quadro[6] = MODO_COOL << 5;    // bits 5-7
  quadro[9] = ligado ? (1 << 5) : 0;
  quadro[11] = visor ? VISOR_ON : VISOR_OFF;

  // Byte 12: checksum = soma dos 12 primeiros bytes, truncada em 8 bits.
  uint8_t soma = 0;
  for (int i = 0; i < 12; i++) soma += quadro[i];
  quadro[12] = soma;

  enviarQuadro(quadro);
  atualizarTela();

  if (ligado) {
    Serial.printf("LIGADO  %d C  visor %s  vel %s  oscilar %s\n", temperatura,
                  visor ? "on" : "off", nomeVelocidade(),
                  oscilando ? "on" : "off");
  } else {
    Serial.println("DESLIGADO");
  }
}

/* --------------------------------------------------------------------- */

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
    display.setCursor(18, 16);
    display.print(temperatura);

    display.setTextSize(2);
    display.setCursor(82, 18);
    display.print("C");
  } else {
    display.setTextSize(2);
    display.setCursor(16, 22);
    display.print("-- C");
  }

  // Rodapé com velocidade e oscilação. Sem ele, os dois botões externos não
  // dariam retorno nenhum com a placa desligada do PC — e é justamente aí que
  // o controle precisa se bastar.
  display.drawLine(0, 50, 127, 50, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("VEL ");
  display.print(nomeVelocidadeCurto());
  display.setCursor(66, 55);
  display.print("OSC ");
  display.print(oscilando ? "ON" : "OFF");

  display.display();
}

const char* nomeVelocidade() {
  switch (velocidade) {
    case VEL_BAIXA: return "low";
    case VEL_MEDIA: return "medium";
    case VEL_ALTA:  return "fast";
    default:        return "?";
  }
}

// Versão curta, para o rodapé do OLED: "medium" não caberia ao lado do OSC.
const char* nomeVelocidadeCurto() {
  switch (velocidade) {
    case VEL_BAIXA: return "LOW";
    case VEL_MEDIA: return "MED";
    case VEL_ALTA:  return "FAST";
    default:        return "?";
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

void mudarVelocidade(uint8_t nova) {
  velocidade = nova;
  enviar();
}

// Avança a velocidade em ciclo: low -> medium -> fast -> low.
void proximaVelocidade() {
  switch (velocidade) {
    case VEL_BAIXA: velocidade = VEL_MEDIA; break;
    case VEL_MEDIA: velocidade = VEL_ALTA;  break;
    default:        velocidade = VEL_BAIXA; break;
  }
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

  // Os dois botões externos são avulsos: não formam combo com ninguém, então
  // agem no APERTO, não ao soltar. A lógica de agir ao soltar existe só por
  // causa do combo dos botões da placa; aqui ela só atrasaria a resposta.
  // Como é o flanco que dispara, segurar o botão manda um comando só.
  bool btnVel = (digitalRead(PINO_BTN_VEL) == LOW);
  bool btnOsc = (digitalRead(PINO_BTN_OSC) == LOW);

  if (btnVel && !velAntes) proximaVelocidade();

  if (btnOsc && !oscAntes) {
    oscilando = !oscilando;
    enviar();
  }

  velAntes = btnVel;
  oscAntes = btnOsc;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PINO_BTN_MAIS, INPUT_PULLUP);
  pinMode(PINO_BTN_MENOS, INPUT_PULLUP);
  pinMode(PINO_BTN_VEL, INPUT_PULLUP);
  pinMode(PINO_BTN_OSC, INPUT_PULLUP);

  analogWriteFreq(38000);   // portadora do IR
  analogWriteRange(255);
  analogWrite(PINO_LED_IR, 0);

  // O OLED da BitDogLab está no I2C1, e não no I2C0 padrão — por isso Wire1.
  Wire1.setSDA(PINO_SDA);
  Wire1.setSCL(PINO_SCL);
  Wire1.begin();
  temDisplay = display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED);

  Serial.println();
  Serial.println("=== Controle Elgin (ELECTRA_AC) — BitDogLab ===");
  Serial.println("P = liga/desliga | A = aumentar | D = diminuir");
  Serial.println("V = visor | O = oscilar");
  Serial.println("SL / SM / SF = velocidade baixa / media / alta");
  Serial.println("Botoes da placa: direita = +1 C | esquerda = -1 C | os dois = liga/desliga");
  Serial.println("Botoes externos: GP16 = velocidade | GP19 = oscilacao");
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
      case 'l': case 'L': mudarVelocidade(VEL_BAIXA); return;
      case 'm': case 'M': mudarVelocidade(VEL_MEDIA); return;
      case 'f': case 'F': mudarVelocidade(VEL_ALTA);  return;
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
