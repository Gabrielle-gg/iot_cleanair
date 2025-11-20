# README – Projeto *Clean Air IoT*

## 1. Visão Geral do Projeto

O **Clean Air IoT** é um sistema de monitoramento da qualidade do ar baseado em sensores conectados à internet utilizando MQTT e Wi-Fi (TCP/IP).  
O projeto permite medir temperatura, umidade e qualidade do ar, enviando os dados em tempo real para um broker MQTT acessível pela internet.

O projeto Clean Air, foi desenvolvido como parte da disciplina Objetos Inteligentes Conectados da Faculdade de Computação e Informática da Universidade Presbiteriana Mackenzie.
Autores: Aldezon Henrique Salvador Santos, Caio Fernandes, Gabrielle Gonçalves Guimarães.

🔗 **Simulação no Wokwi:**  
https://wokwi.com/projects/446015436066367489  

🎥 **Vídeo demonstrando a execução do projeto:**  
https://www.youtube.com/watch?v=8GZTYE6zMVc  

---

## 2. Funcionamento e Uso (Reprodução do Projeto)

O funcionamento do sistema segue estas etapas:

1. O ESP32 inicializa e conecta ao Wi-Fi configurado.  
2. O dispositivo conecta-se ao **broker MQTT**.  
3. Os sensores coletam dados periodicamente.  
4. As informações são publicadas nos tópicos MQTT (ex.: `cleanair/temperatura`).  
5. Qualquer cliente MQTT pode visualizar os dados.  
6. Alertas podem ser enviados caso valores estejam fora dos níveis seguros.

### Passo a passo para reprodução

1. Abra o projeto no Wokwi pelo link.  
2. Configure SSID e senha do Wi-Fi (para uso real).  
3. Ajuste o broker MQTT de sua preferência.  
4. Execute o código no Wokwi.  
5. Assine os tópicos MQTT para acompanhar os dados.  

---

## 3. Software Desenvolvido e Documentação

O software foi organizado para ser fácil de manter e entender. Ele inclui:

- Configuração da conexão Wi-Fi  
- Configuração da conexão MQTT  
- Loop principal de coleta e envio de dados  
- Lógica de reconexão automática MQTT  
- Funções de leitura dos sensores  

### Estrutura dos módulos

| Módulo | Função |
|--------|--------|
| `setupWifi()` | Conecta o ESP32 ao Wi-Fi |
| `setupMQTT()` | Conecta ao broker MQTT |
| `readSensors()` | Realiza leituras dos sensores |
| `loop()` | Publica dados e mantém comunicação ativa |
| `mqttReconnect()` | Reestabelece a conexão caso caia |

---

## 4. Hardware Utilizado

O projeto Clean Air IoT utiliza:

### Placa de desenvolvimento
- **ESP32 DevKit V1**

### Sensores
- **DHT22** – temperatura e umidade  
- **MQ-135** – gases e poluentes

### Componentes auxiliares
- Jumpers  
- Protoboard  
- Cabo USB  
- Fonte 5V  

---

## 5. Comunicação, Interfaces e Protocolos

### Wi-Fi (TCP/IP)
Usado para conectar o ESP32 à internet.

### MQTT
Protocolo ideal para IoT pelo baixo consumo e simplicidade.

### Tópicos Utilizados

```
cleanair/temperatura
cleanair/umidade
cleanair/co2
cleanair/alertas
```

### Brokers compatíveis

- mqtt.eclipseprojects.io  
- test.mosquitto.org  
- broker.hivemq.com  

### Fluxo MQTT

1. ESP32 → Conecta ao Wi-Fi  
2. ESP32 → Conecta ao broker MQTT  
3. ESP32 → Publica nos tópicos  
4. Clientes MQTT → Recebem dados em tempo real  

---

## 6. Comunicação via Internet

- Conexão via rede **Wi-Fi (TCP/IP)**  
- Envio de dados usando **MQTT**  
- Comunicação com um **broker online**  
- Possibilidade de monitoramento remoto de qualquer lugar do mundo  

---


## Conclusão

O desenvolvimento do projeto Clean Air permitiu aplicar conceitos fundamentais de Internet das Coisas (IoT) em uma solução voltada para o monitoramento da qualidade do ar em ambientes internos. A proposta foi desenvolvida com foco na prevenção de doenças respiratórias, alinhando-se ao ODS 3 da ONU, que trata da promoção da saúde e bem-estar. 

