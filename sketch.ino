/*
 * Sistema de Monitoramento de Qualidade do Ar - IoT
 * Projeto: Contribuição ao ODS3 (Saúde e Bem-estar)
 * Autores: Aldezon Henrique, Caio Fernandes, Gabrielle Gonçalves
 * 
 * Descrição: Monitora gases nocivos (GLP, CO, fumaça, propano, etc) usando
 * sensor MQ-2 e ESP32, com alertas via buzzer e transmissão MQTT
 * para visualização em tempo real.
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

// Display LCD 16x2 com endereço I2C 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==================== CONFIGURAÇÃO DE HARDWARE ====================
#define MQ2_PIN 32          // Entrada analógica do sensor MQ-2 (GPIO 32)
#define BUZZER_PIN 25       // Saída digital para buzzer piezoelétrico
#define LED_STATUS 2        // LED interno do ESP32

// ==================== CONFIGURAÇÃO DE REDE ====================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ==================== CONFIGURAÇÃO MQTT ====================
const char* mqtt_broker = "broker.hivemq.com";  // Broker público HiveMQ
const int mqtt_port = 1883;
const char* topic_qualidade = "cleanair/ods3/qualidade";
const char* topic_alerta = "cleanair/ods3/alerta";
const char* topic_status = "cleanair/ods3/status";

// ==================== PARÂMETROS DE QUALIDADE DO AR ====================
// 100% = excelente (sem gás), 0% = crítico (muito gás)
const int LIMITE_CRITICO = 30;      // Abaixo de 30% = crítico (muito gás)
const int LIMITE_ATENCAO = 50;      // Abaixo de 50% = atenção
const int LIMITE_BOM = 70;          // Acima de 70% = ar bom

// ==================== CONFIGURAÇÕES DO SISTEMA ====================
const unsigned long INTERVALO_LEITURA = 2000;     // 2 segundos entre leituras
const unsigned long INTERVALO_RECONEXAO = 5000;   // 5 segundos para reconexão
const int NUM_AMOSTRAS = 5;                       // Amostras para média móvel

// ==================== VARIÁVEIS GLOBAIS ====================
WiFiClient espClient;
PubSubClient mqtt(espClient);

float amostras[NUM_AMOSTRAS];
int indiceAmostra = 0;
bool primeiraLeitura = true;
unsigned long ultimaLeitura = 0;
unsigned long ultimaReconexao = 0;
int contadorLeituras = 0;
String estadoAtual = "INICIALIZANDO";

// ==================== PROTÓTIPOS DE FUNÇÕES ====================
void conectarWiFi();
void conectarMQTT();
void verificarConexoes();
float calcularMediaMovel();
String classificarQualidadeAr(float valor);
void processarLeituraSensor();
void acionarAlertas(float qualidade, String classificacao);
void publicarDadosMQTT(float qualidade, String classificacao);
void exibirDiagnostico(float qualidade, String classificacao, int valorADC);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configuração dos pinos
  pinMode(MQ2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  
  // Inicializa o display LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CleanAir - ODS3");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  
  // Inicializa array de amostras
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    amostras[i] = 0;
  }
  
  // Cabeçalho do sistema
  Serial.println("\n");
  Serial.println("=========================================================");
  Serial.println("  SISTEMA DE MONITORAMENTO DE QUALIDADE DO AR - IoT");
  Serial.println("=========================================================");
  Serial.println("Projeto: Contribuição ao ODS3 da ONU");
  Serial.println("Objetivo: Saúde e Bem-estar para Todos");
  Serial.println("Tecnologia: ESP32 + MQ-2 + MQTT + Wokwi");
  Serial.println("---------------------------------------------------------");
  Serial.println("Autores: Aldezon Henrique Salvador Santos");
  Serial.println("         Caio Fernandes");
  Serial.println("         Gabrielle Gonçalves Guimarães");
  Serial.println("=========================================================\n");
  
  // Teste inicial dos componentes
  Serial.println("[INIT] Testando componentes...");
  digitalWrite(LED_STATUS, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_STATUS, LOW);
  delay(200);
  digitalWrite(LED_STATUS, HIGH);
  delay(200);
  digitalWrite(LED_STATUS, LOW);
  Serial.println("[OK] Componentes funcionando corretamente\n");
  
  // Conecta à rede WiFi
  conectarWiFi();
  
  // Configura cliente MQTT
  mqtt.setServer(mqtt_broker, mqtt_port);
  
  Serial.println("\n[SISTEMA] Pronto para operação!");
  Serial.println("=========================================================\n");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Pronto!");
  lcd.setCursor(0, 1);
  lcd.print("Monitorando...");
  delay(1000);
  
  estadoAtual = "OPERACIONAL";
}

// ==================== CONEXÃO WiFi ====================
void conectarWiFi() {
  Serial.print("[WiFi] Conectando à rede: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi conectado com sucesso!");
    Serial.print("[INFO] Endereço IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[INFO] RSSI (sinal): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n[ERRO] Falha na conexão WiFi!");
    Serial.println("[AVISO] Sistema operará em modo offline");
  }
}

// ==================== CONEXÃO MQTT ====================
void conectarMQTT() {
  if (millis() - ultimaReconexao < INTERVALO_RECONEXAO) {
    return;
  }
  
  ultimaReconexao = millis();
  
  if (!mqtt.connected()) {
    Serial.print("[MQTT] Conectando ao broker ");
    Serial.print(mqtt_broker);
    Serial.print("...");
    
    // Gera ID único baseado no MAC
    String clientId = "ESP32_ODS3_" + String(random(0xffff), HEX);
    
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" Conectado!");
      
      // Publica status online
      mqtt.publish(topic_status, "Sistema Online - Monitoramento Ativo", true);
      
      Serial.println("[OK] Broker MQTT conectado");
      Serial.print("[INFO] Client ID: ");
      Serial.println(clientId);
      
      // Pisca LED para indicar conexão
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_STATUS, HIGH);
        delay(100);
        digitalWrite(LED_STATUS, LOW);
        delay(100);
      }
    } else {
      Serial.print(" Falha! Código: ");
      Serial.println(mqtt.state());
    }
  }
}

// ==================== VERIFICAÇÃO DE CONEXÕES ====================
void verificarConexoes() {
  // Verifica WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[AVISO] WiFi desconectado! Tentando reconectar...");
    conectarWiFi();
  }
  
  // Verifica MQTT
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  
  mqtt.loop();
}

// ==================== MÉDIA MÓVEL ====================
float calcularMediaMovel() {
  float soma = 0;
  int numAmostrasValidas = primeiraLeitura ? indiceAmostra + 1 : NUM_AMOSTRAS;
  
  for (int i = 0; i < numAmostrasValidas; i++) {
    soma += amostras[i];
  }
  
  return soma / numAmostrasValidas;
}

// ==================== CLASSIFICAÇÃO DA QUALIDADE ====================
String classificarQualidadeAr(float valor) {
  // 100% = excelente (sem gás), 0% = crítico (muito gás)
  if (valor >= LIMITE_BOM) {
    return "EXCELENTE";
  } else if (valor >= LIMITE_ATENCAO) {
    return "BOA";
  } else if (valor >= LIMITE_CRITICO) {
    return "MODERADA";
  } else if (valor >= 15) {
    return "RUIM";
  } else {
    return "CRITICA";
  }
}

// ==================== PROCESSAMENTO DO SENSOR ====================
void processarLeituraSensor() {
  // Lê o sensor MQ-2
  int valorADC = analogRead(MQ2_PIN);
  
  // Converte para porcentagem primeiro
  float porcentagemADC = map(valorADC, 0, 4095, 0, 100);
  
  // INVERTE: Slider direita (ADC baixo) = muito gás = qualidade baixa
  float qualidadeAtual = 100 - porcentagemADC;
  
  // Adiciona à média móvel
  amostras[indiceAmostra] = qualidadeAtual;
  indiceAmostra++;
  
  if (indiceAmostra >= NUM_AMOSTRAS) {
    indiceAmostra = 0;
    primeiraLeitura = false;
  }
  
  // Calcula média móvel (reduz ruído)
  float qualidadeMedia = calcularMediaMovel();
  String classificacao = classificarQualidadeAr(qualidadeMedia);
  
  contadorLeituras++;
  
  // Exibe diagnóstico
  exibirDiagnostico(qualidadeMedia, classificacao, valorADC);
  
  // Aciona alertas
  acionarAlertas(qualidadeMedia, classificacao);
  
  // Publica dados via MQTT
  publicarDadosMQTT(qualidadeMedia, classificacao);
}

// ==================== SISTEMA DE ALERTAS ====================
void acionarAlertas(float qualidade, String classificacao) {
  // Atualiza o display LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Qualidade: ");
  lcd.print(qualidade, 0);
  lcd.print("%");
  
  lcd.setCursor(0, 1);
  
  // Abaixo de 30% = crítico (muito gás)
  if (qualidade < LIMITE_CRITICO) {
    // ALERTA CRÍTICO (muito gás detectado!)
    lcd.print("CRITICO!");
    if (estadoAtual != "CRITICO") {
      Serial.println("\n*** ALERTA CRÍTICO ATIVADO ***");
      Serial.println("Alto nível de gases detectado!");
      Serial.println("Recomendação: Ventile o ambiente IMEDIATAMENTE\n");
      estadoAtual = "CRITICO";
    }
    digitalWrite(BUZZER_PIN, HIGH);  // Liga buzzer
    digitalWrite(LED_STATUS, HIGH);
    
  } else if (qualidade < LIMITE_ATENCAO) {
    // ATENÇÃO (gás moderado)
    lcd.print("ATENCAO!");
    if (estadoAtual != "ATENCAO") {
      Serial.println("\n** ATENÇÃO: Gases detectados **");
      Serial.println("Considere melhorar a ventilação\n");
      estadoAtual = "ATENCAO";
    }
    digitalWrite(BUZZER_PIN, LOW);   // Desliga buzzer
    // LED pisca em modo atenção
    digitalWrite(LED_STATUS, (millis() / 500) % 2);
    
  } else {
    // NORMAL (ar limpo)
    lcd.print(classificacao);
    if (estadoAtual != "NORMAL") {
      Serial.println("\n✓ Qualidade do ar excelente\n");
      estadoAtual = "NORMAL";
    }
    digitalWrite(BUZZER_PIN, LOW);   // Desliga buzzer
    digitalWrite(LED_STATUS, LOW);
  }
}

// ==================== PUBLICAÇÃO MQTT ====================
void publicarDadosMQTT(float qualidade, String classificacao) {
  if (mqtt.connected()) {
    // Cria payload JSON
    String payload = "{";
    payload += "\"qualidade\":" + String(qualidade, 1) + ",";
    payload += "\"classificacao\":\"" + classificacao + "\",";
    payload += "\"timestamp\":" + String(millis()) + ",";
    payload += "\"leitura\":" + String(contadorLeituras);
    payload += "}";
    
    // Publica nos tópicos
    mqtt.publish(topic_qualidade, payload.c_str());
    mqtt.publish(topic_alerta, classificacao.c_str());
  }
}

// ==================== DIAGNÓSTICO NO SERIAL ====================
void exibirDiagnostico(float qualidade, String classificacao, int valorADC) {
  Serial.println("─────────────────────────────────────────────────────");
  Serial.print("LEITURA #");
  Serial.print(contadorLeituras);
  Serial.print(" | Timestamp: ");
  Serial.println(millis());
  Serial.println("─────────────────────────────────────────────────────");
  Serial.print("Sensor ADC: ");
  Serial.print(valorADC);
  Serial.print("/4095 | Qualidade: ");
  Serial.print(qualidade, 1);
  Serial.println("%");
  Serial.print("Classificação: ");
  Serial.print(classificacao);
  Serial.print(" | Estado: ");
  Serial.println(estadoAtual);
  Serial.print("MQTT: ");
  Serial.print(mqtt.connected() ? "Conectado" : "Desconectado");
  Serial.print(" | WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado");
  Serial.println("─────────────────────────────────────────────────────\n");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Mantém conexões ativas
  verificarConexoes();
  
  // Processa leitura do sensor periodicamente
  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = millis();
    processarLeituraSensor();
  }
}