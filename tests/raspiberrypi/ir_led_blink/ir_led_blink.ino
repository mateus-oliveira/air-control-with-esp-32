/*
 * Teste 1 de 2 — BitDogLab (Raspberry Pi Pico W): o LED do GP17
 *
 * Alterna dois modos, 3 segundos cada, para sempre:
 *
 *   1) PISCA LENTO (1 Hz, nível contínuo) — prova a fiação.
 *      LED amarelo de teste: pisca à vista.
 *      LED IR: aponte a CÂMERA DO CELULAR para a ponta; o sensor enxerga
 *      infravermelho e mostra um lampejo branco-arroxeado. A câmera frontal
 *      costuma funcionar melhor (filtro IR mais fraco).
 *
 *   2) PORTADORA 38 kHz — prova que o PWM está configurado certo, que é o
 *      que o controle realmente usa.
 *      LED amarelo: fica ACESO, porém visivelmente mais FRACO que no modo 1
 *      (está piscando 38 mil vezes por segundo, com metade do tempo apagado).
 *      Se ele apagar de vez ou não mudar de brilho nenhum, o PWM não subiu.
 *
 * Placa: Raspberry Pi Pico W  |  Serial Monitor: 115200 baud
 * Ligação: GP17 -> resistor 220R -> anodo (perna longa)
 *          catodo (perna curta) -> GND
 *
 * Nada acendeu? Na ordem: 1) LED invertido (troque as pernas),
 * 2) resistor/jumper mal encaixado, 3) LED queimado na dessoldagem.
 */

const int PINO_LED_IR = 17;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PINO_LED_IR, OUTPUT);
  digitalWrite(PINO_LED_IR, LOW);

  analogWriteFreq(38000);
  analogWriteRange(255);

  Serial.println();
  Serial.println("=== Teste do LED no GP17 ===");
}

void loop() {
  Serial.println("Modo 1: pisca lento (nivel continuo)");
  for (int i = 0; i < 3; i++) {
    digitalWrite(PINO_LED_IR, HIGH);
    delay(500);
    digitalWrite(PINO_LED_IR, LOW);
    delay(500);
  }

  Serial.println("Modo 2: portadora de 38 kHz (amarelo fica aceso, mais fraco)");
  analogWrite(PINO_LED_IR, 128);  // 50% de duty
  delay(3000);
  analogWrite(PINO_LED_IR, 0);
  delay(500);
}
