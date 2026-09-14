# Controle remoto caseiro — Elgin Inverter 9000 BTUs

Controle infravermelho substituto para um ar-condicionado Elgin Inverter cujo controle
original quebrou. Feito com uma **Wemos D1 R32 (ESP32)**, o **LED infravermelho resgatado
do controle original** e um display **OLED SSD1306**.

O sketch final é [`esp32/esp32.ino`](esp32/esp32.ino): dois botões físicos para a
temperatura, e o teclado do Serial Monitor para tudo.

| Comando | Ação |
|---|---|
| Botão `IO25` **ou** tecla `A` | aumenta 1 °C |
| Botão `IO17` **ou** tecla `D` | diminui 1 °C |
| **Os dois botões juntos** **ou** tecla `P` | liga / desliga |
| Tecla `V` | liga / desliga o visor do aparelho |
| Tecla `O` | liga / desliga a oscilação das aletas |
| Teclas `SL` / `SM` / `SF` | ventilador baixo / médio / alto |

Os botões físicos cobrem só temperatura e liga/desliga — visor, oscilação e
velocidade são exclusivos do teclado.

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
   Wemos D1 R32 (ESP32)                      Protoboard
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

### Fase 2 — O controle (`esp32/esp32.ino`)

Grave o sketch e abra o Serial Monitor a **115200 baud**. Liga/desliga e temperatura
podem ser feitos pelos **botões físicos** ou pelas teclas `P`, `A` e `D` — os dois caminhos
são equivalentes. Visor, oscilação e velocidade só existem no teclado.

**Liga/desliga é apertar os dois botões ao mesmo tempo.** São só dois botões: um sozinho
mexe na temperatura, os dois juntos ligam ou desligam.

Cada comando faz o aparelho **apitar** — esse apito é a confirmação de que chegou.

```
=== Controle Elgin (ELECTRA_AC) ===
P = liga/desliga | A = aumentar | D = diminuir
V = visor | O = oscilar
SL / SM / SF = velocidade baixa / media / alta
Botoes: um de cada vez = temperatura | os dois = liga/desliga
OLED: ok
Estado inicial: DESLIGADO, 24 C, vel low

LIGADO  24 C  visor on  vel low  oscilar off
LIGADO  25 C  visor on  vel low  oscilar off
LIGADO  25 C  visor on  vel fast  oscilar on
DESLIGADO
```

**Velocidade é o único comando de duas letras:** `S` sozinho não faz nada, ele só avisa
que a próxima letra (`L`, `M` ou `F`) escolhe a velocidade. Qualquer outra letra depois do
`S` descarta o comando com um aviso.

O OLED mostra `LIGADO`/`DESLIGADO` e a temperatura em fonte grande — visor, oscilação e
velocidade aparecem só no Serial. Se o display não for detectado, o sketch avisa no boot e
segue funcionando normalmente só pelo Serial.

Deixe a caixa de envio do Serial Monitor em **"Sem final de linha"**. Nas outras opções a
IDE manda um `\n` junto — e esse `\n` seria lido como a segunda letra de um `S`, cancelando
o comando de velocidade. Fora isso, o sketch ignora qualquer caractere que não reconheça.

Os botões são lidos a cada 30 ms, o que já elimina o repique dos contatos. A temperatura
muda **ao soltar** o botão, não ao apertar: como é impossível apertar os dois exatamente no
mesmo instante, agir no aperto faria o primeiro botão mudar a temperatura antes de o segundo
chegar e formar o combo. Segurar apertado manda um comando só, não uma rajada.

Se um botão disparar sozinho, é mau contato na protoboard ou os dois fios caíram no mesmo
par interno do botão (veja a nota sobre as 4 pernas, acima).

O modo é fixo em **COOL** — para mudar, ajuste `ac.next.mode` dentro de `enviar()`.
Estado inicial: desligado, 24 °C, ventilador **baixo**, visor **ligado**, oscilação
**desligada**.

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
está desligado a 24 °C, com ventilador baixo, visor ligado e oscilação desligada.

---

# Versão BitDogLab (Raspberry Pi Pico W)

Sketch: [`raspiberrypi/raspiberrypi.ino`](raspiberrypi/raspiberrypi.ino).

<img width="559" height="552" alt="image" src="https://github.com/user-attachments/assets/e46cf839-0ea7-41ed-9ad9-23990e8af9ad" />


Mesmo controle, mesmos comandos, mesmo protocolo — rodando na **BitDogLab** da
Embarcatech. A diferença que importa: a BitDogLab já traz os **dois botões**, o
**OLED** e uma **bateria**, então o controle deixa de depender da protoboard e do PC.
Só o LED IR é fio solto.

Os dois sketches convivem no repositório e são equivalentes: `control/` é a Wemos D1 R32,
`raspiberrypi/` é a BitDogLab.

## O que muda no hardware

| Peça | Wemos D1 R32 (ESP32) | BitDogLab (Pico W) |
|---|---|---|
| Botão aumentar | `IO25` (na protoboard) | **Botão B**, o da direita = `GP6` (já na placa) |
| Botão diminuir | `IO17` (na protoboard) | **Botão A**, o da esquerda = `GP5` (já na placa) |
| OLED SDA / SCL | `GPIO21` / `GPIO22` (I2C0) | `GP14` / `GP15` (**I2C1**) |
| LED IR | `GPIO26` | `GP17` |
| Alimentação | USB do PC | bateria da placa |
| Botão velocidade | não existia | `GP16` (na protoboard) |
| Botão oscilação | não existia | `GP19` (na protoboard) |

As duas últimas linhas são **novas**, não têm equivalente na Wemos: os dois botões que lá
faziam temperatura e liga/desliga foram reaproveitados aqui para outra coisa, já que a
BitDogLab traz os seus próprios. Com quatro botões, o controle deixa de depender do teclado
para tudo menos o visor.

Os botões continuam sem resistor: na BitDogLab eles já estão ligados entre o GPIO e o GND,
exatamente o arranjo que o `INPUT_PULLUP` do firmware espera.

**O OLED da BitDogLab está no I2C1, não no I2C0.** Por isso o sketch usa `Wire1` e chama
`setSDA`/`setSCL` **antes** do `begin()` — no core do Pico, remapear os pinos depois do
`begin()` não faz efeito, e o display simplesmente não aparece.

## Ligação do LED e dos botões externos

O OLED e os botões A/B já vêm soldados; só isto vai na protoboard:

```
   BitDogLab (Pico W)                      Protoboard
┌────────────────────┐
│  GP17         ─────┼──[ 220Ω ]──►|───────────────┐
│                    │            LED IR           │
│                    │                             │
│  GP16         ─────┼────o  o─────────────────────┤   botao VELOCIDADE
│                    │                             │
│  GP19         ─────┼────o  o─────────────────────┤   botao OSCILACAO
│                    │                             │
│  GND          ─────┼─────────────────────────────┘
└────────────────────┘
```

Os botões externos seguem a mesma regra dos da Wemos: **dois fios cada, sem resistor**. O
firmware liga o pull-up interno (`INPUT_PULLUP`), então em repouso o pino fica em 3,3 V e
apertar puxa para o GND. Vale também a mesma nota sobre botão tátil de 4 pernas — se parecer
sempre pressionado, gire 90°.

Vale a mesma conta de corrente da versão ESP32 (o Pico também é 3,3 V), e o mesmo aviso:
**nunca ligue o LED direto no pino**, sempre com resistor.

### Por que GP17

Não é um pino qualquer: o RP2040 tem **8 slices de PWM**, cada um com dois canais, e os
dois canais de um slice **compartilham a mesma frequência**. Como a portadora do IR fixa o
slice em 38 kHz, todo periférico que caia no mesmo slice fica refém dessa frequência.

| Pino | Slice | Divide o slice com |
|---|---|---|
| `GP28` | 6 | LED RGB azul (`GP12`), LED RGB vermelho (`GP13`) |
| `GP20` | 2 | **Buzzer A** (`GP21`), Botão A (`GP5`) |
| **`GP16` / `GP17`** | 0 | só `GP0` e `GP1` — nada da placa usa como PWM |
| **`GP18` / `GP19`** | 1 | só `GP2` e `GP3` — idem |

O `GP28` seria a escolha ruim por dois motivos: é a entrada do **microfone** (ADC2, ligada à
saída de um amplificador, que o LED passaria a disputar) e trava o slice do LED RGB em
38 kHz. O `GP20` tem o mesmo defeito em relação ao **buzzer** — justamente o que se usaria
para dar um bip de confirmação num controle sem PC.

`GP16` a `GP19` caem nos slices 0 e 1, que não encostam em nada da BitDogLab, e são
intercambiáveis entre si. Para trocar, basta mudar `PINO_LED_IR` no topo do sketch.

> Confira no esquemático da sua revisão de placa quais pinos o conector IDC expõe
> fisicamente — o mapa de slices acima vale para o RP2040, mas o que está disponível no
> conector depende da placa.

## Ambiente

1. **Arduino IDE** → *Preferências* → *URLs adicionais de gerenciadores de placas*:
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

   O campo é uma linha só, mas aceita **vários URLs separados por vírgula** — a do ESP32
   continua valendo, é só acrescentar a nova depois de uma vírgula. Se preferir, o ícone
   pequeno na ponta direita do campo abre um diálogo com **uma URL por linha**.
2. *Ferramentas → Placa → Gerenciador de Placas* → procure por `pico` e instale
   **Raspberry Pi Pico/RP2040/RP2350**, de *Earle F. Philhower, III*. São ~500 MB, demora.

   > **Cuidado: há dois cores para o Pico, e só um serve.** Procurando por `pico` também
   > aparece o **Arduino Mbed OS RP2040 Boards**, da própria Arduino — esse vem no índice
   > padrão, então instala sem precisar de URL nenhuma, e é a armadilha. O Mbed **não tem**
   > `Serial.printf`, `analogWriteFreq`, `analogWriteRange` nem `Wire1`, e sem
   > `analogWriteFreq` não existe portadora de 38 kHz. Não é questão de ajustar o código:
   > o sketch depende dessas quatro coisas.
   >
   > Se o erro vier com `'class arduino::UART' has no member named 'printf'` ou
   > `'Wire1' was not declared in this scope`, é o core Mbed que está selecionado.
   > O nome `arduino::UART` é a assinatura dele.

3. *Ferramentas → Placa* → o submenu certo chama-se **Raspberry Pi Pico/RP2040/RP2350**
   (o errado chama-se *Arduino Mbed OS RP2040 Boards*). Dentro dele escolha
   **Raspberry Pi Pico W** — a BitDogLab usa o Pico **W**.

   Os dois cores convivem sem problema; o que decide é qual submenu você escolhe. Se quiser
   tirar a tentação do caminho, desinstale o Mbed pelo mesmo Gerenciador de Placas.
4. *Ferramentas → Gerenciar Bibliotecas* → instale **Adafruit SSD1306** e
   **Adafruit GFX Library**. As mesmas do ESP32 — se já instalou antes, nada a fazer.
   **Não** instale a `IRremoteESP8266` para esta placa; veja a seção seguinte.
5. Serial Monitor em **115200 baud**.

### Gravando: o modo de gravação (BOOTSEL)

Para gravar, o RP2040 precisa estar em **modo de gravação** — nele o USB deixa de ser porta
serial e vira um pendrive chamado **RPI-RP2**, e a IDE escreve o `.uf2` direto nesse volume.
A IDE tenta entrar nesse modo sozinha, e quando consegue é só clicar em *Carregar*.

Quando ela **não** consegue, o erro é sempre este:

```
Resetting /dev/cu.usbmodem21301
Scanning for RP2040 devices
No drive to deploy.
Failed uploading: uploading error: exit status 1
```

Traduzindo: ela mandou a placa reiniciar em modo de gravação, a placa não foi, e não havia
volume para escrever.

**O jeito que funciona — energizar com o BOOTSEL apertado:**

1. **Desplugue o cabo USB.**
2. Segure o **BOOTSEL** — o único botão do módulo do Pico, colado no micro-USB *dele*.
   Não é o Botão A, nem o B, nem o RESET da BitDogLab.
3. **Plugue o USB** com o botão ainda apertado.
4. Conte até três e solte. O volume **RPI-RP2** monta.
5. Clique em *Carregar*. O volume fica montado indefinidamente, não há pressa.

A variante mais divulgada — segurar BOOTSEL, apertar e soltar RESET, soltar BOOTSEL — **é
mais frágil**, e aqui falhou várias vezes seguidas. O RP2040 lê o BOOTSEL no instante exato
em que sai do reset, então soltar o botão cedo demais faz o chip acordar em modo normal. Na
energização essa janela não existe: o botão já está pressionado quando o chip liga.

**Como saber se é técnica ou defeito.** O sintoma de BOOTSEL que não pega é a porta serial
sumir e voltar (o RESET funcionou), sem nenhum disco aparecer. Se quiser descartar cabo ruim
ou placa reiniciando sozinha, observe a porta com a placa **em repouso**, sem tocar nela:

```sh
for i in $(seq 1 30); do
  ls /dev/cu.usbmodem* >/dev/null 2>&1 && echo "[${i}s] porta" || echo "[${i}s] SUMIU"
  sleep 1
done
```

Trinta linhas iguais = placa estável, e o problema é só a janela do BOOTSEL. Se ela oscilar
sozinha, aí sim o assunto é alimentação, cabo ou firmware.

**Como saber que gravou**, sem depender da mensagem da IDE: o volume `RPI-RP2` desmonta
sozinho e a porta serial volta poucos segundos depois. Volume não desmonta por conta
própria — se desmontou, é porque alguém escreveu nele.

**Feche o Serial Monitor antes de gravar.** Ele segura a porta (`lsof /dev/cu.usbmodem*`
mostra o processo `serial-mo`), e o reset automático da IDE depende de abrir e fechar essa
porta a 1200 bps. A IDE 2.x costuma fechar o monitor sozinha, mas com o Pico no macOS ela
escorrega.

**Plano B: arrastar o `.uf2` no Finder.** Em *Sketch → Exportar binário compilado*, a IDE
grava o `.uf2` em `raspiberrypi/build/`. Com o `RPI-RP2` montado, arraste o arquivo para o
volume. É o mesmo que a IDE faz, com a vantagem de não ter pressa — o
`Scanning for RP2040 devices` dela tem uma janela curta e desiste rápido.

## Passo a passo

### Fase 1 — Testes de sanidade (`tests/raspiberrypi/`)

São testes próprios da BitDogLab — os de `tests/` na raiz são da Wemos e usam outros pinos.

| Sketch | O que prova | Resultado esperado |
|---|---|---|
| `ir_led_blink` | o LED e o PWM de 38 kHz | pisca 3 s, depois fica aceso mais fraco 3 s |
| `oled_hello` | o display no I2C1 | texto + contador na tela |

O `ir_led_blink` alterna dois modos de propósito. No **pisca lento** o LED liga em nível
contínuo — é o teste da fiação. Na **portadora de 38 kHz** ele liga e desliga 38 mil vezes
por segundo: o LED amarelo fica **aceso, porém nitidamente mais fraco** que no pisca, porque
passa metade do tempo apagado. Essa queda de brilho é a prova de que o PWM subiu certo.
Se o LED apagar de vez ou o brilho não mudar nada entre os dois modos, o problema está no PWM.

Quando trocar o amarelo pelo LED IR, o teste é o mesmo, só que olhando pela **câmera do
celular** — vale toda a explicação da versão ESP32 acima.

### Fase 2 — O controle (`raspiberrypi/raspiberrypi.ino`)

São **quatro** botões, e o teclado do Serial Monitor continua fazendo tudo
(`P A D V O SL SM SF`).

| Botão | Onde | Ação |
|---|---|---|
| **B** — o da **direita** | na placa | aumenta 1 °C |
| **A** — o da **esquerda** | na placa | diminui 1 °C |
| **A + B juntos** | na placa | liga / desliga |
| `GP16` | protoboard | velocidade em ciclo: `low → med → fast → low` |
| `GP19` | protoboard | liga / desliga a oscilação |

Os dois da placa mantêm a lógica da Wemos — agir ao soltar, 30 ms de anti-repique, o combo
que só dispara uma vez — explicada na seção dela.

Os dois externos são diferentes de propósito: **agem no aperto, não ao soltar**. Aquela
lógica de agir ao soltar existe só por causa do combo, e como estes não formam combo com
ninguém, esperar o dedo sair só atrasaria a resposta. É o flanco que dispara, então segurar
o botão manda um comando só, não uma rajada.

O **visor é o único comando que sobrou exclusivo do teclado** (tecla `V`), e a razão está na
última seção: nesse protocolo ele é um *toggle*, não um estado.

O OLED agora mostra tudo que tem botão:

```
LIGADO
────────────────────
    24 C
────────────────────
VEL MED      OSC ON
```

Sem esse rodapé os dois botões novos não dariam retorno nenhum com a placa fora do PC — que
é exatamente a situação para a qual eles existem.

Gravado o sketch, desplugue o USB e ligue a bateria: daí em diante é um controle remoto de
verdade, e só o visor pede o PC.

---

## Por que o código IR é diferente nas duas placas

A `IRremoteESP8266` **não compila no RP2040** — apesar de rodar em ESP32 apesar do nome, ela
depende dos periféricos da Espressif. Não existe equivalente dela para Pico com suporte a
protocolos de ar-condicionado: as bibliotecas de IR do RP2040 tratam de controles simples
("um código por botão"), e Inverter não é assim.

Então o `ELECTRA_AC` foi **reimplementado à mão** dentro do sketch, em cerca de 60 linhas,
divididas em duas camadas:

- **A portadora de 38 kHz** sai do PWM por hardware do RP2040 (`analogWriteFreq(38000)`).
  "Marca" é o PWM ligado a 50% de duty, "espaço" é o pino em nível baixo.
- **O quadro de 13 bytes** é montado campo a campo (temperatura com offset 8, modo,
  ventilador, oscilação, liga/desliga, visor) e fechado com o checksum, que é só a soma
  dos 12 bytes anteriores truncada em 8 bits.

Os valores não foram deduzidos nem adivinhados: saíram do `ir_Electra.cpp` da própria
IRremoteESP8266 2.9.0, a mesma versão que roda no ESP32. Os dois caminhos foram comparados
byte a byte em **todas as 408 combinações** de liga/desliga × 17 temperaturas × 3
velocidades × oscilação × visor, e o quadro transmitido é idêntico. Ou seja: o aparelho não
tem como notar a diferença entre as duas placas.

**Interrupções ficam ligadas durante a transmissão.** O quadro inteiro dura ~195 ms e
desligá-las por todo esse tempo derrubaria o USB. Não é preciso: as esperas são medidas
contra o relógio absoluto (`micros()`, que no RP2040 vem de um timer de hardware), e não por
contagem de ciclos. Se uma interrupção roubar alguns microssegundos no meio de um pulso, a
espera seguinte termina no mesmo instante que terminaria de qualquer forma — o erro não
acumula, e receptores IR toleram folga muito maior que isso.

### Uma pegadinha do protocolo: o visor é *toggle*

No `ELECTRA_AC` o campo do visor não é um estado ("ligado"/"desligado"), é um **toggle**:
o aparelho inverte o visor toda vez que recebe um quadro com esse campo em `0x15`. Como o
firmware manda o estado inteiro a cada comando, com `visor` começando em `true`, **qualquer**
comando (mudar temperatura, ligar, trocar velocidade) também alterna o visor do aparelho.

Isso vem da versão ESP32 e foi mantido igual de propósito, para as duas placas se comportarem
do mesmo jeito. Se incomodar, o conserto é mandar `0x15` só no comando `V` e `0x08` em todos
os outros — ou seja, tratar `visor` como um pulso, não como um estado.
