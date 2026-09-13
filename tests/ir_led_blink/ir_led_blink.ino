/*
 * Teste 3 de 3 — LED infravermelho
 *
 * Pisca o LED IR devagar (ligado 1s / desligado 1s), em nível contínuo.
 * NÃO é o sinal de 38 kHz do controle remoto — é só um teste de vida do LED.
 *
 * COMO VERIFICAR: aponte a CÂMERA DO CELULAR para a ponta do LED.
 * O sensor da câmera enxerga infravermelho e você verá um lampejo
 * branco-arroxeado piscando. A olho nu não se vê nada — é normal.
 *
 * A câmera frontal costuma funcionar melhor (filtro IR mais fraco).
 * Se a traseira não mostrar nada, tente a frontal antes de suspeitar do LED.
 *
 * Placa: WEMOS D1 R32 (ESP32)  |  Serial Monitor: 115200 baud
 * Ligação: GPIO26 -> resistor 220R -> anodo (perna longa)
 *          catodo (perna curta) -> GND
 *
 * Nada acendeu? Na ordem: 1) LED invertido (troque as pernas),
 * 2) resistor/jumper mal encaixado, 3) LED queimado na dessoldagem.
 */

const int PINO_LED_IR = 26;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PINO_LED_IR, OUTPUT);
  digitalWrite(PINO_LED_IR, LOW);

  Serial.println();
  Serial.println(F("=== Teste do LED IR ==="));
  Serial.printf("Pino: GPIO%d\n", PINO_LED_IR);
  Serial.println(F("Aponte a camera do celular para o LED."));
  Serial.println(F("Voce deve ver um lampejo branco/roxo a cada segundo.\n"));
}

void loop() {
  digitalWrite(PINO_LED_IR, HIGH);
  Serial.println(F("LED ON  (procure o brilho na camera)"));
  delay(1000);

  digitalWrite(PINO_LED_IR, LOW);
  Serial.println(F("LED OFF"));
  delay(1000);
}
