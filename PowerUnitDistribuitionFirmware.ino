#include <WiFiManager.h>   //Biblioteca para configurar o Wifi
#include <otadrive_esp.h>  //Biblioteca para atualizar o firmware via OTA
#include <mcp_can.h>       // Biblioteca para configuração do módulo CAN
#include <SPI.h>           // Biblioteca para utilizar a comunicação SPI

// Definição dos pinos dos sensores de corrente
const int LambdaPin = 34;
const int FanPin = 35;
const int FuelPumpPin = 33;
const int MainRelayPin = 32;

// Definição dos pinos do módulo can MCP2515
const int CAN_INT = 4;
const int CAN_CS = 5;

// Criação do objeto CAN0 para gerenciar a comunicação com o barramento CAN
MCP_CAN CAN0(CAN_CS);

// Variáveis para calibração dos sensores
const int sensitivity = 66;      // Sensibilidade (verificar no Datasheet do sensor de corrente)
const int offsetVoltage = 2153;  // Ajuste baseado no sensor (Valor que está sendo lido realmente)
const int numSamples = 50;       // Número de amostras para média

WiFiManager wm;
unsigned long lastCheck = 0;
const unsigned long checkInterval = 10000;  // Verifica Wi-Fi a cada 10 segundos

// Função da biblioteca OTADrive para atualizar o firmware via OTA
void sync_task() {
  if (!OTADRIVE.timeTick(60))
    return;
  if (WiFi.status() != WL_CONNECTED)
    return;
  OTADRIVE.updateFirmware();
}

void setup() {
  // O primeiro parâmetro é o ID da conta do OTADrive e o segundo é o controle de versão
  OTADRIVE.setInfo("79bb2f38-8263-4ce8-91bf-a453da7baf7c", "v@1.0.0");

  Serial.begin(115200);

  // Iniciando o AcessPoint para configurar a Rede Wifi
  if (!wm.autoConnect("EquipePoliRacing", "EPRFP16!")) {
    Serial.println("Falha na conexão ou tempo limite atingido.");
  } else {
    Serial.println("Conectado com sucesso!");
  }

  // Inicializando o MCP2515
  // Atente-se quanto a velocidade de comunicação definida e ao clock (o clock tem que verificar no cristal oscilador do módulo)
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 inicializado com sucesso (ESP32)");
  } else {
    Serial.println("Erro ao inicializar MCP2515 (ESP32)");
    while (1)
      ;
  }

  CAN0.setMode(MCP_NORMAL);  // Define o modo normal
  pinMode(CAN_INT, INPUT);   // Configura o pino INT como entrada

  Serial.println("Setup finalizado e pronto para comunicação via CAN.");
}

void loop() {
  sync_task();  //Chamando a função para verificar se há um novo firmware

  // Leitura dos sensores de corrente
  readCurrent(LambdaPin, "Lambda");
  readCurrent(FanPin, "Fan");
  readCurrent(FuelPumpPin, "Fuel Pump");
  readCurrent(MainRelayPin, "Main Relay");

  delay(2000);
}

// Faz a média das leituras do sensor p/ maior precisão
int averageAnalogRead(int pin) {
  long total = 0;
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(pin);
  }
  return total / numSamples;
}

// Lê a corrente de um sensor e exibe no Serial Monitor
void readCurrent(int pin, const char* label) {
  int adcValue = averageAnalogRead(pin); // Lendo a entrada analógica
  double adcVoltage = (adcValue / 1024.0) * 5000;  // Conversão para mV
  double currentValue = (adcVoltage - offsetVoltage) / sensitivity; // Valor da corrente real

  Serial.printf("%s - ADC: %d\t Tensão (mV): %.2f\t Corrente (A): %.2f\n",
                label, adcValue, adcVoltage, currentValue);
}