#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h> //segun la libreria se diseño para el esp8266, pero lo admite el esp32

const char *ssid = "";
const char *password = "";

const char *mqtt_server = ""; //http://broker.mqtt-dashboard.com/index.html
const int mqtt_port = 1883;                 //TCP_URL
const char *mqtt_user = "esp32_client";
const char *mqtt_pass = "121212";

TaskHandle_t Task1;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[100];

int card = 117541900;
int door = 21;
int led = 32;
//*****************************
//*** DECLARACION FUNCIONES ***
//*****************************
void setup_wifi();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

//*****************************
//***   TAREA OTRO NUCLEO   ***
//*****************************

void codeForTask1(void *parameter)
{

  for (;;)
  {

    if (digitalRead(door))
    {
      Serial.println(" ->  Puerta abierta ");
      digitalWrite(led, HIGH);
    }
    else
    {
      Serial.println(" -> Puerta Cerrada");
      digitalWrite(led, LOW);
    }

    delay(500);

    vTaskDelay(10);
  }
}

void setup()
{
  pinMode(led, OUTPUT);
  pinMode(door, INPUT_PULLUP);
  pinMode(BUILTIN_LED, OUTPUT);

  Serial.begin(9600);
  randomSeed(micros());

  xTaskCreatePinnedToCore(
      codeForTask1, /* Task function. */
      "Task_1",     /* name of task. */
      1000,         /* Stack size of task */
      NULL,         /* parameter of the task */
      1,            /* priority of the task */
      &Task1,       /* Task handle to keep track of created task */
      0);           /* Core */

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  long now = millis();
  if (now - lastMsg > 3000)
  {
    lastMsg = now;
    card++;

    String to_send = "5e992a6806505c5262396ff0," + String(card) + "," + String(37);
    to_send.toCharArray(msg, 100);
    Serial.print("Publicamos mensaje -> ");
    Serial.println(msg);
    client.publish("porvenir", msg);
  }
}

//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi()
{
  delay(10);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++)
  {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

  if (incoming == "on")
  {
    digitalWrite(BUILTIN_LED, HIGH);
  }
  else
  {
    digitalWrite(BUILTIN_LED, LOW);
  }
}

void reconnect()
{

  while (!client.connected())
  {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "esp32_";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("Conectado!");
      digitalWrite(BUILTIN_LED, HIGH);
      // Nos suscribimos
      client.subscribe("led");
    }
    else
    {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");

      delay(5000);
    }
  }
}