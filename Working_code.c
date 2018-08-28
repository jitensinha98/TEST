#include <SoftwareSerial.h> 
#include <LiquidCrystal.h>
#include<dht.h>

SoftwareSerial espSerial(12, 11);   //Pin 2 and 3 act as RX and TX. Connect them to TX and RX of ESP8266      

#define DEBUG true
String mySSID = "TP-LINK_9F28";       // WiFi SSID
String myPWD = "jiten12345"; // WiFi Password
String myAPI = "Y3EZWTEQP5QB7BX1";   // API Key
String myHOST = "api.thingspeak.com";
String myPORT = "80";

String myFIELD1 = "field1"; 
String myFIELD2 = "field2"; 
String myFIELD3 = "field3"; 
String myFIELD5 = "field5";
String myFIELD6 = "field6";

boolean pump_status=false;

int moisture_value_cutoff = 25;

LiquidCrystal lcd (9, 2, 4, 5, 6, 7);

dht DHT;

#define sensor_delay  10
#define pump_relay 8
#define DHT11_PIN 10
#define moisture_pin A0
#define moisture_power_pin 13

int Moisture_Dry_cutoff=1023;
int Moisture_Wet_cutoff=400;

int Temperature;
int Humidity;
int Moisture_Raw;
int Moisture_mapped;
int Moisture_sum=0;
int Moisture_average=0;

void setup()
{
  lcd.begin(16,2);
  lcd.setCursor(2,0);
  lcd.print("INITIALIZING");
  Serial.begin(9600);
  pinMode(pump_relay,OUTPUT);
  digitalWrite(pump_relay,HIGH);
  pinMode(moisture_pin,INPUT);
  pinMode(moisture_power_pin,OUTPUT);
  digitalWrite(moisture_power_pin, LOW);
  espSerial.begin(115200);
  pinMode(DHT11_PIN,INPUT);
  espData("AT+RST", 1000, DEBUG);                      //Reset the ESP8266 module
  espData("AT+CWMODE=1", 1000, DEBUG);                 //Set the ESP mode as station mode
  espData("AT+CWJAP=\""+ mySSID +"\",\""+ myPWD +"\"", 1000, DEBUG);   //Connect to WiFi network
  lcd.begin(16,2);
  lcd.setCursor(1,0);
  lcd.print("SYSTEM ONLINE");
  delay(500);
  
}

  void loop()
  {
    int chk = DHT.read11(DHT11_PIN);
    
    get_Temperature();
    get_Humidity();
    get_Moisture();

    if(Temperature != 0 && Humidity != 0 )
    {
      lcd.clear();
    
      lcd_show_Moisture();
      lcd_show_Humidity();
      lcd_show_Temperature();
  
      check_pump_status();

      if(pump_status==true)
          {
           lcd.clear();
           pump_on();
           delay(6000);
           pump_off();
           delay(4000);
          }
        else
          {
           pump_off();
          }
      lcd.begin(16,2);
      lcd_show_Moisture();
      lcd_show_Humidity();
      lcd_show_Temperature();
      
      send_data_to_server();
        
     }
       else
    { }
  }

  String espData(String command, const int timeout, boolean debug)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";
  espSerial.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (espSerial.available())
    {
      char c = espSerial.read();
      response += c;
    }
  }
  if (debug)
  {
    //Serial.print(response);
  }
  return response;
}
void get_Temperature()
{
  
  Temperature=DHT.temperature;
  
}

void get_Humidity()
{
  
  Humidity=DHT.humidity;  
   
}

void calculate_Moisture()
{
  digitalWrite(moisture_power_pin,HIGH);
  delay(sensor_delay);
  Moisture_Raw=analogRead(moisture_pin);
  if (Moisture_Raw >= Moisture_Dry_cutoff)
  {
      Moisture_Raw = Moisture_Dry_cutoff;
  }
  if(Moisture_Raw <= Moisture_Wet_cutoff)
  {
      Moisture_Raw = Moisture_Wet_cutoff;
  }
  Moisture_mapped=map(Moisture_Raw,Moisture_Dry_cutoff,Moisture_Wet_cutoff,0,100);
  digitalWrite(moisture_power_pin,LOW);
}
void get_Moisture()
{
  Moisture_sum=0;
  Moisture_average=0;
  for(int i=1;i<=10;i++)
  {
    calculate_Moisture();
    delay(200);
    Moisture_sum=Moisture_sum+Moisture_mapped;
  }
  Moisture_average=Moisture_sum/10;
}
void lcd_show_Temperature()
{
  lcd.setCursor(0,2);
  lcd.print("T = "+ String(Temperature)+(char)223+"C"); 
}
void lcd_show_Humidity()
{
  lcd.setCursor(9,0);
  lcd.print("H = "+ String(Humidity)+"%"); 
}

void lcd_show_Moisture()
{
  lcd.setCursor(0,0);
  lcd.print("M = "+ String(Moisture_average)+"%"); 
}
void pump_on()
{
  digitalWrite(pump_relay,LOW);
}
void pump_off()
{
  digitalWrite(pump_relay,HIGH);
}

void check_pump_status()
{
  if (Moisture_average <= moisture_value_cutoff)
  {
    pump_status=true;
  }
  else
  {
    pump_status=false;
  }
}
void send_data_to_server()
{
      String sendData = "GET /update?api_key="+ myAPI +"&"+ myFIELD1 +"="+String(Moisture_average) +"&"+ myFIELD2 +"="+String(Humidity) +"&"+ myFIELD3 +"="+String(Temperature) +"&"+ myFIELD5 +"="+String(pump_status);
      espData("AT+CIPMUX=1", 1000, DEBUG);       //Allow multiple connections
      espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);
      espData("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG);  
      espSerial.find(">"); 
      espSerial.println(sendData);
      espData("AT+CIPCLOSE=0",1000,DEBUG);
      delay(20000);
}

