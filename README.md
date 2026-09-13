# Controle remoto caseiro — Elgin Inverter 9000 BTUs

Controle infravermelho substituto para um ar-condicionado Elgin Inverter cujo controle
original quebrou. Feito com uma **Wemos D1 R32 (ESP32)**, o **LED infravermelho resgatado
do controle original** e um display **OLED SSD1306**.

O sketch final é [`control/control.ino`](control/control.ino): dois botões físicos para a
temperatura, e o teclado do Serial Monitor para tudo.

| Comando | Ação |
|---|---|
| Botão `IO25` **ou** tecla `A` | aumenta 1 °C |
| Botão `IO17` **ou** tecla `D` | diminui 1 °C |
| **Os dois botões juntos** **ou** tecla `P` | liga / desliga |

**Protocolo: `ELECTRA_AC`** — o Elgin Inverter 9000 responde ao protocolo da Electra
(a Elgin reetiqueta essa plataforma). Descoberto por tentativa, já que não havia receptor
IR para capturar os códigos do controle original.

---

## Hardware

| Item | Observação |
|---|---|
| Wemos D1 R32 (ESP32, formato UNO) | placa principal, 3,3 V |
| LED IR 940 nm | dessoldado do controle original |
| Resistor 220 Ω | limita a corrente do LED |
| OLED 0.96" SSD1306 I2C | mostra o estado atual (opcional) |
| 2 botões táteis (push-button) | aumentar e diminuir temperatura |
| Protoboard e jumpers | |

## Montagem

```
   D1 R32 (ESP32)                         Protoboard
┌────────────────────┐
│  SCL (GPIO22) ─────┼───────────────────────► OLED  SCL
│  SDA (GPIO21) ─────┼───────────────────────► OLED  SDA
│  3V3          ─────┼───────────────────────► trilha (+) ──► OLED VCC
│  GND          ─────┼───────────────────────► trilha (−) ──► OLED GND
│                    │                              ▲
│  IO26 (GPIO26)─────┼──[ 220Ω ]──►|────────────────┤
│                    │            LED IR            │
│                    │                              │
│  IO25 (GPIO25)─────┼────o  o──────────────────────┤   botao AUMENTAR
│                    │                              │
│  IO17 (GPIO17)─────┼────o  o──────────────────────┘   botao DIMINUIR
└────────────────────┘                       trilha (−) = GND
```

| Componente | Perna | Pino da D1 R32 |
|---|---|---|
| OLED | VCC | `3V3` |
| OLED | GND | `GND` |
| OLED | SDA | **SDA** (GPIO21) |
| OLED | SCL | **SCL** (GPIO22) |
| Resistor 220 Ω | — | `IO26` → anodo do LED |
| LED IR | catodo | `GND` |
| Botão aumentar | um lado | `IO25` |
| Botão aumentar | outro lado | `GND` |
| Botão diminuir | um lado | `IO17` |
| Botão diminuir | outro lado | `GND` |

**Os botões não precisam de resistor.** O firmware liga o *pull-up interno* do ESP32
(`INPUT_PULLUP`): em repouso o pino fica em 3,3 V, e apertar o botão o puxa para o GND.
São só dois fios por botão.

> Botão tátil de 4 pernas: os pinos são ligados **aos pares**. Se ao espetar na protoboard
> ele parecer sempre pressionado, gire-o 90°.

**Cuidados**

- O OLED vai no **3V3**, não no 5V — o barramento I2C do ESP32 é 3,3 V.
- Nunca ligue o LED IR direto no pino, sempre com resistor.
- Evite `IO0`, `IO2`, `IO12` e `IO15` (pinos de *strapping*, atrapalham o boot) e
  `IO34/35/36/39` (somente entrada).
- Polaridade do LED: perna **longa = anodo (+)**. Se as pernas foram cortadas, olhe contra
  a luz — o eletrodo **maior, em forma de taça, é o catodo (−)**.

**Corrente x alcance** — o LED cai ~1,3 V, então `I = (3,3 − 1,3) / R`:

| Resistor | Corrente | Comentário |
|---|---|---|
| 220 Ω | ~9 mA | comece aqui — seguro, alcance de ~20–50 cm |
| 150 Ω | ~13 mA | bom equilíbrio |
| 100 Ω | ~20 mA | limite recomendado do GPIO (absoluto: 40 mA) |

Para alcance de vários metros, depois: `GPIO26` → 1 kΩ → base de um **BC337/2N2222**,
emissor no GND, LED entre `5V` e o coletor com 47 Ω. O firmware não muda.

---

## Ambiente

1. **Arduino IDE** → *Preferências* → *URLs adicionais de gerenciadores de placas*:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. *Ferramentas → Placa → Gerenciador de Placas* → instale **esp32** (Espressif Systems).
3. Selecione a placa **WEMOS D1 R32** (ou *ESP32 Dev Module*).
4. *Ferramentas → Gerenciar Bibliotecas* → instale:
   - **IRremoteESP8266** (funciona em ESP32, apesar do nome)
   - **Adafruit SSD1306**
   - **Adafruit GFX Library**
5. Serial Monitor em **115200 baud**.

---

## Passo a passo

### Fase 1 — Testes de sanidade (`tests/`)

Validar cada peça isoladamente antes de misturar tudo.

| Sketch | O que prova | Resultado esperado |
|---|---|---|
| `i2c_scanner` | o OLED está no barramento | imprime `0x3C` |
| `oled_hello` | o display desenha | texto + contador na tela |
| `ir_led_blink` | o LED IR sobreviveu | lampejo visível **pela câmera do celular** |

O LED IR é invisível a olho nu. Aponte a câmera do celular para a ponta dele: o sensor
enxerga infravermelho e mostra um lampejo branco-arroxeado. A câmera **frontal** costuma
funcionar melhor (filtro IR mais fraco).

### Fase 2 — O controle (`control/control.ino`)

Grave o sketch e abra o Serial Monitor a **115200 baud**. Tudo pode ser feito pelos
**botões físicos** ou pelas teclas `P`, `A` e `D` — os dois caminhos são equivalentes.

**Liga/desliga é apertar os dois botões ao mesmo tempo.** São só dois botões: um sozinho
mexe na temperatura, os dois juntos ligam ou desligam.

Cada comando faz o aparelho **apitar** — esse apito é a confirmação de que chegou.

```
=== Controle Elgin (ELECTRA_AC) ===
P = liga/desliga | A = aumentar | D = diminuir
Botoes: um de cada vez = temperatura | os dois = liga/desliga
OLED: ok
Estado inicial: DESLIGADO, 24 C

LIGADO  24 C
LIGADO  25 C
DESLIGADO
```

O OLED mostra `LIGADO`/`DESLIGADO` e a temperatura em fonte grande. Se o display não for
detectado, o sketch avisa no boot e segue funcionando normalmente só pelo Serial.

Deixe a caixa de envio do Serial Monitor em **"Sem final de linha"**. Nas outras opções a
IDE manda um `\n` junto, mas o sketch ignora qualquer caractere que não seja P, A ou D.

Os botões são lidos a cada 30 ms, o que já elimina o repique dos contatos. A temperatura
muda **ao soltar** o botão, não ao apertar: como é impossível apertar os dois exatamente no
mesmo instante, agir no aperto faria o primeiro botão mudar a temperatura antes de o segundo
chegar e formar o combo. Segurar apertado manda um comando só, não uma rajada.

Se um botão disparar sozinho, é mau contato na protoboard ou os dois fios caíram no mesmo
par interno do botão (veja a nota sobre as 4 pernas, acima).

Fixos no código: modo **COOL** e ventilador **automático** — para mudar, ajuste
`ac.next.mode` e `ac.next.fanspeed` dentro de `enviar()`.

---

## Como o protocolo foi descoberto

Ar-condicionado Inverter não usa "um código por botão": a cada clique o controle transmite o
**estado inteiro** (liga/desliga + modo + temperatura + ventilador + swing + checksum).
A `IRremoteESP8266` monta esses quadros prontos — faltava saber **qual protocolo** a Elgin usa.

Sem receptor IR não dava para capturar do controle original, então foi pelo caminho inverso:
enviar o mesmo comando ("ligar, COOL, 24 °C") em ~19 protocolos candidatos, um a cada 4
segundos, até o aparelho apitar. Deu **`ELECTRA_AC`**.

Se um dia precisar refazer isso para outro aparelho, o caminho mais direto é um receptor
**TSOP1838 / VS1838B** (poucos reais) com o exemplo `IRrecvDumpV3` da biblioteca, capturando
os botões de um controle que funcione — identifica o protocolo com certeza absoluta.

---

## Limitação conhecida

Como não há receptor, o firmware **não sabe** o estado real do aparelho — ele mantém o estado
que acredita ter enviado, exatamente como o controle original. Se alguém desligar o ar por
outro meio, os dois ficam dessincronizados; basta apertar `P` duas vezes para realinhar.

O estado também não é salvo: ao reiniciar a placa, o controle volta a achar que o aparelho
está desligado a 24 °C.
