#include <OneWire.h> //Biblioteca para sensores temperatura
#include <EEPROM.h>  //Biblioteca para EPROM
#include <LiquidCrystal.h> //Biblioteca para display
#include <SPI.h>
//#include <DHT.h>

#include <Ethernet.h>

  #define VERSAO 8.0
  
  //Porta da esquerda visao lateral esquerda -- Fermentacao
  #define ONE_WIRE_FERM1_INTERNO 39   //  -- PRIMEIRA PORTA da direita para a esquerda.  
  #define ONE_WIRE_FERM1_EXTERNO 46  // OK -- SEGUNDA PORTA da direita para a esquerda.  

  #define ONE_WIRE_FERM2_INTERNO 38 // OK
  #define ONE_WIRE_FERM2_EXTERNO 47   //  

/*Definicao dos 8 Reles 220v*/
  #define RELE1 29
  #define RELE2 31
  #define RELE3 33
  #define RELE4 35

//  #define DHTPIN 5
//  #define DHTTYPE DHT11
 
//   DHT dht(DHTPIN, DHTTYPE);
 
    byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
     
    char server[] = "nanobrejaria.com.br";  

    IPAddress ip(192, 168, 0, 177);

    EthernetClient client;

 //#define MAX_CMD_LENGTH 20
  char drukdata[20]; 
  int cmdIndex=0;
  char incomingByte;

class Elizabeth
{
public:
  void HabilitaHardware();
  void DesabilitaHardware();
  int RetornaHabilitadoHardware();
};
  void Elizabeth::HabilitaHardware ()
  {
	  EEPROM.write(10, 1);
  }

  void Elizabeth::DesabilitaHardware ()
  {
	  EEPROM.write(10, 0);
  }
  int Elizabeth::RetornaHabilitadoHardware()
  {
	  return EEPROM.read(10);
  }


class Sensor
{
  float temperatura;
  int OK = 0;
  int NOK = 1;

public:
  float ObtemTemperatura (int);  
  int ValidaLeitura(float temp);
};

float Sensor::ObtemTemperatura (int porta) 
  {
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius, fahrenheit;
  
    OneWire  ds(porta);  
  
    ds.search(addr);
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
  
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
    }
  
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
  
    return celsius;
  }
  int Sensor::ValidaLeitura(float temp)
  {
    if ((temp > -120) && (String(temp) != "-0.06")){return OK;}
    else return NOK;    
  }


class Fermentador
{
   int LIGADO = 0;
   int DESLIGADO = 1;
   
   int OK = 0;
   int NOK = 1;
   
   float temperatura_interna, temperatura_externa, temperatura_alvo;
   
   unsigned long tempo_desligado_interno, tempo_ligado_interno, tempo_desligado_externo, tempo_ligado_externo, tempo_pause_contingencia, 
            //     tempo_maximo_interno_ligado=30000, tempo_maximo_interno_desligado=60000, 
              //   tempo_maximo_externo_ligado=60000, tempo_maximo_externo_desligado=300000;
 tempo_maximo_interno_ligado=30000, tempo_maximo_interno_desligado=180000, 
              //tempo_maximo_externo_ligado=60000, tempo_maximo_externo_desligado=3000000;
 tempo_maximo_externo_ligado=5000, tempo_maximo_externo_desligado=1200000; // Alterei o tempo ligado e desligado para tentar uma maior eficiencia

   int identificador, bomba_interna, bomba_externa, sensor_interno, sensor_externo, temperatura_interna_erro, temperatura_externa_erro,
   acionamento_bomba_interna, acionamento_bomba_externa;

    boolean MODO_PROTEGIDO=true;

public:
  Fermentador(int, int, int, int, int);
 
  void EnviaDadosServidor();

  void RecebeTemperaturaAlvo();
  
  void GravaTemperaturaAlvo (); 
  void ZeraTemperaturaAlvo ();
  int RetornaTemperaturaAlvo();

  void CalculaTemperaturaInterna();
  void CalculaTemperaturaExterna();
  
  float RetornaTemperaturaInterna();
  float RetornaTemperaturaExterna();
  
  int RetornaEstadoInterno();
  int RetornaEstadoExterno();
  unsigned long RetornaTempoExternoLigado();
  unsigned long RetornaTempoExternoDesligado();
  unsigned long RetornaTempoInternoLigado();
  unsigned long RetornaTempoInternoDesligado();  
  void AtualizaTempoExternoLigado();
  void AtualizaTempoInternoLigado();
  void AtualizaTempoExternoDesligado();
  void AtualizaTempoInternoDesligado();

  void Inicializa();

  void HabilitaInterno();
  void DesabilitaInterno();
  void HabilitaExterno();
  void DesabilitaExterno();
  
  void HabilitaInternoSeguro();
  void DesabilitaInternoSeguro();
  void HabilitaExternoSeguro();
  void DesabilitaExternoSeguro();


  
  void Resfria();
  
  void Protege();
  void DesProtege();
  boolean RetornaModoProtegido();
  
};

  Fermentador::Fermentador(int id, int bomba1, int bomba2, int sensor1, int sensor2)
  {
      identificador = id;
      bomba_interna = bomba1;
      bomba_externa = bomba2;
      sensor_interno = sensor1;
      sensor_externo = sensor2;      
  }  
    
  void Fermentador::EnviaDadosServidor()
  {

    String vTemperaturaInterna="?TEMP_INTERNA=", vTemperaturaExterna="&TEMP_EXTERNA=", vVolume="&VOL=0", vFermentador="&FERMENTADOR=", 
           vTxtEnvio, vTemperaturaAlvo="&TEMP_ALVO=", vAcionamentoInterno="&QTD_INTERNO=", vAcionamentoExterno="&QTD_EXTERNO=";
    
    vTxtEnvio = String("GET /add_fermentacao.php")+ vTemperaturaInterna + RetornaTemperaturaInterna()+ vTemperaturaExterna + RetornaTemperaturaExterna() + vVolume + vFermentador + identificador + vTemperaturaAlvo + RetornaTemperaturaAlvo() + vAcionamentoInterno+acionamento_bomba_interna + vAcionamentoExterno+acionamento_bomba_externa +String(" HTTP/1.1");

//    vTxtEnvio = String("GET /add_fermentacao.php")+ vTemperaturaInterna + random(0, 15)+ vTemperaturaExterna + random(5, 20) + vVolume + vTemperaturaAlvo + RetornaTemperaturaAlvo(identificador) + vFermentador + identificador+String(" HTTP/1.1");

    if (Ethernet.begin(mac) == 0) 
    {
      Serial.println("Failed to configure Ethernet using DHCP");
      // try to congifure using IP address instead of DHCP:
      Ethernet.begin(mac, ip);
    }
    // give the Ethernet shield a second to initialize:
    delay(2000);
    Serial.println("Obtendo Endereço IP...");

    if (client.connect(server, 80)) 
    {
      Serial.println("Conectado !");
      // Make a HTTP request:
      //client.println("GET /add_fermentacao.php?TEMP_INTERNA=20&TEMP_EXTERNA=30&VOL=40&FERMENTADOR=2 HTTP/1.1");
      Serial.println(vTxtEnvio);
      client.println(vTxtEnvio);
      client.println("Host: nanobrejaria.com.br");
      client.println("Connection: close");
      client.println();
      acionamento_bomba_externa = 0;
      acionamento_bomba_interna = 0;
    } 
    else 
    {
      // if you didn't get a connection to the server:
      Serial.println("connection failed");
    }    
    Serial.println("Disconectando...");
    client.stop();
    Serial.println("Desconectado.");  
  }

  void Fermentador::RecebeTemperaturaAlvo()
  {
  
    String vTxtEnvio, vStrValorAlvo;
    char vTempAlvoOld;
    int vIntTempAlvo;
 
    
    vTxtEnvio = String("GET /consulta_alvo.php?FERMENTADOR=") + identificador + String(" HTTP/1.1");
    if (Ethernet.begin(mac) == 0) 
    {
      Serial.println("Failed to configure Ethernet using DHCP");
      Ethernet.begin(mac, ip);
    }
    delay(2000);
    Serial.println("Obtendo Endereço IP...");

    if (client.connect(server, 80)) 
    {
      Serial.println("Conectado !");
      Serial.println(vTxtEnvio);
      client.println(vTxtEnvio);
      client.println("Host: nanobrejaria.com.br");
      client.println("Connection: close");
      client.println();     
    
      while (client.connected())
       {
        char vTempAlvo = client.read();  
        vStrValorAlvo = vStrValorAlvo + (char)vTempAlvo;          
       };
      if (!client.connected())
      {
          vStrValorAlvo.trim();
          vStrValorAlvo.remove(0,vStrValorAlvo.length()-2);
          if (vStrValorAlvo != "" || vStrValorAlvo != 0 ) // Objetivo desta condicao eh impedir que o valor seja setado para 0 quando nao existir agenda.
          {
            vIntTempAlvo = vStrValorAlvo.toInt();
            Serial.println(vIntTempAlvo);        
            Serial.println();
            Serial.println("disconnecting.");
            client.stop();
             
            if (vIntTempAlvo > 0 and EEPROM.read(identificador) != vIntTempAlvo )
            {
              Serial.print("Novos Parametros Recebidos temperatura Alterada para:"); 
              Serial.println(vIntTempAlvo);
              EEPROM.write(identificador, vIntTempAlvo);        
            }
            else { 
                Serial.print("Novos Parametros Recebidos nada a Alterar :");
                Serial.println(vIntTempAlvo);
            }
          }
          else { 
                Serial.print("Parametros vazios nada alterado");
                Serial.println(vIntTempAlvo);
          }
    }     
    else 
    {
      // if you didn't get a connection to the server:
      Serial.println("connection failed");
    }    
    }
  }
  
  void Fermentador::GravaTemperaturaAlvo ()
  {
    String x="";
    char inChar;      
          do {
          while (Serial.available()) {
           inChar = Serial.read();
           x += (char)inChar;
           if (inChar == '\n') { 
               Serial.println("DADOS RECEBIDOS - DIGITE * PARA CONFIRMAR OS DADOS");               
               break;                         
           }
         }
      }while(inChar!='*');      
      EEPROM.write(identificador, x.toInt());
      Serial.println("DADOS GRAVADOS COM SUCESSO");      
  }
  
  void Fermentador::ZeraTemperaturaAlvo ()
  {
	  EEPROM.write(identificador, 100);
  }
  int Fermentador::RetornaTemperaturaAlvo()
  {
	  return EEPROM.read(identificador);
  }
    
  void Fermentador::CalculaTemperaturaInterna()
  {
    float temp;    
    Sensor SensorTempInterno;
    temp = SensorTempInterno.ObtemTemperatura(sensor_interno);
    if (SensorTempInterno.ValidaLeitura(temp)==OK){ temperatura_interna = temp; temperatura_interna_erro=0;}    
    else { 
        if (temperatura_interna_erro <= 110) temperatura_interna_erro = temperatura_interna_erro + 1; 
      }
   // return temperatura_interna;    
   }
  
  float Fermentador::RetornaTemperaturaInterna()
  {
    return temperatura_interna;
  }
  
  void Fermentador::CalculaTemperaturaExterna()
  {
    float temp;    
    Sensor SensorTempExterno;
    temp = SensorTempExterno.ObtemTemperatura(sensor_externo); 
    
    if (SensorTempExterno.ValidaLeitura(temp)==OK)
    { 
      temperatura_externa = temp; 
      temperatura_externa_erro=0; 
    }
    else 
    { 
      if ( temperatura_externa_erro <= 110) temperatura_externa_erro = temperatura_externa_erro + 1;     }    
   }
  
  float Fermentador::RetornaTemperaturaExterna()
  {
    return temperatura_externa;
  }
  
  int Fermentador::RetornaEstadoInterno()
  {
  	if (digitalRead(bomba_interna) == LIGADO){ return LIGADO; }
  	else { return DESLIGADO; }
  }   
  
  int Fermentador::RetornaEstadoExterno()
  {
  	if (digitalRead(bomba_externa) == LIGADO){ return LIGADO;}
  	else{ return DESLIGADO;}
  } 

  void Fermentador::AtualizaTempoExternoLigado()
  {
  	tempo_ligado_externo=millis();
  } 
  void Fermentador::AtualizaTempoInternoLigado()
  {
  	tempo_ligado_interno=millis();
  } 

  void Fermentador::AtualizaTempoExternoDesligado()
  {
  	tempo_desligado_externo=millis();
  } 
  void Fermentador::AtualizaTempoInternoDesligado()
  {
  	tempo_desligado_interno=millis();
  } 

  
  unsigned long Fermentador::RetornaTempoExternoLigado()
  {
  	return tempo_ligado_externo;
  } 
  unsigned long Fermentador::RetornaTempoExternoDesligado()
  {
  	return tempo_desligado_externo;
  } 
  unsigned long Fermentador::RetornaTempoInternoLigado()
  {
  	return tempo_ligado_interno;
  } 
  unsigned long Fermentador::RetornaTempoInternoDesligado()
  {
  	return tempo_desligado_interno;
  } 
  
  void Fermentador::Inicializa()
  {
    float temp;
    int vloop;
      do{
        CalculaTemperaturaExterna();
        temp = RetornaTemperaturaExterna();
        vloop = vloop +1;
        if (vloop == 100) { Serial.println("ERRO NA INICIALIZACAO DO SENSOR EXTERNO"); break; }
    }while(temp != -0.06);
    Serial.println("TEMPERATURA OBTIDA PARA O SENSOR EXTERNO : " + String(temp));
    
    temp = 0;
    vloop = 0;
      do{
        CalculaTemperaturaInterna();
        temp = RetornaTemperaturaInterna();
        vloop = vloop +1;
        if (vloop == 100) { Serial.println("ERRO NA INICIALIZACAO DO SENSOR INTERNO"); break; }
    }while(temp != -0.06);
    Serial.println("TEMPERATURA OBTIDA PARA O SENSOR INTERNO : " + String(temp));   
  }

  void Fermentador::HabilitaInterno()
  {
      digitalWrite(bomba_interna,LIGADO);          

  }

  void Fermentador::HabilitaInternoSeguro()
  {
  	if (RetornaEstadoInterno() == DESLIGADO)
  	{
          if (((millis() - RetornaTempoInternoDesligado()) > tempo_maximo_interno_desligado) || (millis()-RetornaTempoInternoLigado() < tempo_maximo_interno_ligado) )
          {
  	    HabilitaInterno();
            AtualizaTempoInternoLigado();
            acionamento_bomba_interna = acionamento_bomba_interna + 1;
	  }
	}
	else 
	{	
	  if (((millis()-RetornaTempoInternoLigado()) > tempo_maximo_interno_ligado) && (RetornaModoProtegido()==true))
	  {
	    DesabilitaInternoSeguro();		
	  }
	}	
  }

  void Fermentador::DesabilitaInterno()
  {
     digitalWrite(bomba_interna,DESLIGADO);
  }
  
  void Fermentador::DesabilitaInternoSeguro()
  {
  	  if (RetornaEstadoInterno() == LIGADO)
  	  {
         	DesabilitaInterno();
                AtualizaTempoInternoDesligado();
       }
  }

  void Fermentador::HabilitaExterno()
  {
      digitalWrite(bomba_externa,LIGADO);   
  }

  void Fermentador::HabilitaExternoSeguro()
  {
  	if (RetornaEstadoExterno() == DESLIGADO)
  	{ 
          if (((millis()-RetornaTempoExternoDesligado())>tempo_maximo_externo_desligado) || ((millis()-RetornaTempoExternoLigado()))<tempo_maximo_externo_ligado) // 5 Minutos de Descanco
          {
              HabilitaExterno();
              AtualizaTempoExternoLigado();
              acionamento_bomba_externa = acionamento_bomba_externa + 1 ;
	    }
	}
	else 
	{	
		if (((millis()-RetornaTempoExternoLigado()) > tempo_maximo_externo_ligado) && (RetornaModoProtegido()==true))
		{
			DesabilitaExternoSeguro();				
		}
	}	
  }
  
  void Fermentador::DesabilitaExterno()
  {
      digitalWrite(bomba_externa, DESLIGADO);
  }
  
  void Fermentador::DesabilitaExternoSeguro()
  {
  		if (RetornaEstadoExterno() == LIGADO)
  		{
	          DesabilitaExterno();
                  AtualizaTempoExternoDesligado();
          	}
  }


  void Fermentador::Resfria()
  {
    temperatura_alvo = RetornaTemperaturaAlvo();
    
    if (temperatura_alvo != 100 )
    {
      if (temperatura_externa_erro > 100 && temperatura_interna_erro > 100)
      {
        if (millis()-tempo_pause_contingencia >= 1800000) //Aguarda 30 Minutos para o proximo acionamento
        {
           HabilitaInternoSeguro(); 
           delay(20000);
           DesabilitaInternoSeguro();
           HabilitaExternoSeguro();
           delay(20000);
           DesabilitaExternoSeguro();
           
           tempo_pause_contingencia = millis();
        }
      }
      else
      {
          if (temperatura_externa > temperatura_alvo)
          {
            HabilitaExternoSeguro();
            //Serial.println("Recirculando Externo...");
          }
          else
          {
            if (temperatura_externa < temperatura_alvo)
            {  
              DesabilitaExternoSeguro();
              //Serial.println("Parando recirculacao externa...");
            }
          }   
          
          if (temperatura_interna > temperatura_alvo)
          {
            HabilitaInternoSeguro();
            //Serial.print("Recirculadno Interno... Temp Alvo: ");
            //Serial.print(temperatura_alvo);
            //Serial.print("Temperatura Atual: ");
            //Serial.println(temperatura_interna);
          }
          else
          {
            if (temperatura_interna < temperatura_alvo) 
            {  
              DesabilitaInternoSeguro();
              //Serial.print("Parando recirculacao interno... Temp Alvo");
              //Serial.print(temperatura_alvo);
              //Serial.print("Temperatura Atual: ");
              //Serial.println(temperatura_interna);

            }
          }      
      }
    }
  }

 void Fermentador::DesProtege()
  {
      MODO_PROTEGIDO = false;     
      Serial.println("Modo Protegido Desligado"); 
  }
  
 void Fermentador::Protege()
  {
      MODO_PROTEGIDO = true;      
      Serial.println("Modo protegido Ligado");
  }

boolean Fermentador::RetornaModoProtegido()
{
    return MODO_PROTEGIDO;
}

/************************************************************************************************
            DECLARACAO DE VARIAVEIS GLOBAIS
/***********************************************************************************************/  


  LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

    // Objeto fermentador ( Identificador, MotorBombaInterna, MotorBombaExterna, TemperaturaInterna, TemperaturaExterna)
    Fermentador Cabecao1(1, RELE1, RELE2, ONE_WIRE_FERM1_INTERNO, ONE_WIRE_FERM1_EXTERNO);

    Fermentador Cabecao2(2, RELE3, RELE4, ONE_WIRE_FERM2_INTERNO, ONE_WIRE_FERM2_EXTERNO);
    
    Elizabeth Eliz;
 
    float vTempBrassagem, vTempLavagem, vTempCabecao1Externo, vTempCabecao1Interno, vTempCabecao2Externo, vTempCabecao2Interno, vIntTempBunker, vIntTermAmbiente;
  
    unsigned long vTempoElizControle, vTempoElizUltResp, vTempoElizEsperaEnviaDadosServidor=1800000, vContaTempoEnviaDadosServidor; // 30 minutos (1800000)
    unsigned long vContaTempoRecebeDadosServidor, vTempoElizEsperaReceberDadosServidor=60000; // 43200000 12 horas

      /*Definicoes das variaveis para a string de protocolo*/

    String vMgsLCD, TotalString, Comunicando, VolBrassagem, VolLavagem,
         TempBrassagem, TempLavagem, TempFermentador1, TempFermentador2, 
         TempFermentador3, TempFermentador4, vErroInicializacaoSD, TempBunker, TemIntFerm2, TemIntFerm1, TempAmbiente, Humid;

    String vTemperaturaInterna, vTemperaturaExterna, vVolume;

      // Conteudo das EPROMs
    byte vIntENBFerm1, vIntENBFerm2;
   
    
/****************************************************************************
SETUP INICIAL
*****************************************************************************/

void setup() {
 
  lcd.begin(20, 4);
  
  lcd.setCursor(0, 1);
  
  Serial.println("Definindo portas de entradas...");
  Serial.println("Definindo velocidade de comunicacao...");
  Serial.begin(9600); 
  Serial.println("Definindo interrupcoes...");
  Serial.println("Inicialiando interrupcoes dos sensores...");

  Serial.println("Definindo portas dos reles...");
  pinMode(RELE1,OUTPUT);
  pinMode(RELE2,OUTPUT);
  pinMode(RELE3,OUTPUT);
  pinMode(RELE4,OUTPUT);

  digitalWrite(RELE1,1);
  digitalWrite(RELE2,1);
  digitalWrite(RELE3,1);
  digitalWrite(RELE4,1);


  Serial.println("Inicalizando os sensores de temperatura...");
  
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Calibrando Brassagem");
  Serial.println("Calibrando sensor de Brassagem...");
  //Brassagem.Inicializa();
  
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Calibrando Lavagem");
  Serial.println("Calibrando sensor de Lavagem...");
  //Lavagem.Inicializa();
    
  lcd.clear();  
  lcd.setCursor(0,1);
  lcd.print("Calibrando Ferm1");
  Serial.println("Calibrando sensor do Fermentador 1...");
  //Cabecao1.Inicializa();
      
  lcd.clear();  
  lcd.setCursor(0,1);
  lcd.print("Calibrando Ferm2");
  Serial.println("Calibrando sensor do Fermentador 2...");
  //Cabecao2.Inicializa();
  
  lcd.clear();

  lcd.setCursor(0,1);
  lcd.print("Inicializacao OK.");
  Serial.println("Inicializacao OK.");
  lcd.setCursor(0,2);
  lcd.print("BOA BRASSAGEM ");
  Serial.println("PROECESSO OK ;) BOA BRASSAGEM");

  Man();

  delay(2000);

  lcd.clear();
  
}

void loop() {

/*******************************************************************************
**************** RECEBENDO PARAMETROS DOS SENDORES *****************************
*******************************************************************************/

Cabecao1.CalculaTemperaturaExterna();
Cabecao1.CalculaTemperaturaInterna();
Cabecao2.CalculaTemperaturaExterna();
Cabecao2.CalculaTemperaturaInterna();
    

//float h = dht.readHumidity();
//float t = dht.readTemperature();

//vIntTermAmbiente = t;

/******************************************************************************
*************** VERIFICANDO SE O SISTEMA ESTA EM MODO HARDWARE ****************
******************************************************************************/

//MODO DEFAULT 1
 if  (vMgsLCD == "MODO HARDWARE")
 {   
   
  Cabecao1.Resfria();
  
  Cabecao2.Resfria();
  
 }

/****************************************************
 Construindo o protocolo de envio das informacoes 
*****************************************************/
Comunicando = String("COMUNICANDO:");
VolBrassagem = String("VBRAS:");
VolLavagem = String(";VLAVA:");
TempBrassagem = String(";T_BRAS:");
TempLavagem = String(";T_LAVA:");
TempFermentador1 = String(";FERM1:");
TempFermentador2 = String(";FERM2:");
TempFermentador3 = String(";FERM3:");
TempFermentador4 = String(";FERM4:");
TemIntFerm2 = String(";F2INT:");
TemIntFerm1 = String(";F1INT:");
TempBunker = String(";BUNK:");
TempAmbiente = String(";AMBIE:");
Humid = String(";HUMI:");

if (incomingByte=Serial.available()>0){
  
  char byteIn = Serial.read();
  drukdata[cmdIndex] = byteIn;

  if(byteIn=='\n'){
    drukdata[cmdIndex] = '\0';
    Serial.println(drukdata);
    cmdIndex = 0;
    if (strcmp(drukdata, "TESTE" ) == 0) {
      Serial.println("TESTE OK ");
    }    
  }else{
    if(cmdIndex++ >= 20){
      cmdIndex = 0;
    }
  }  
}

// Os reles somente serao habilitados caso a Elizabeth estiver no controle.
// isso ira impedir que existam 2 controladores ao mesmo tempo.
/*Parametros para habilitar ou desabilitar os RELES*/

//if ( Eliz.RetornaHabilitadoHardware() == 1 ) {
                //HABILTA              
  if (strcmp(drukdata, "0" ) == 0){ Cabecao1.HabilitaInterno(); Serial.println("HABILITADO INTERNO FERM_1"); Cabecao1.DesProtege(); drukdata[0] = '-';  }     // ATIVA RECIRCULACAO DA CAMISA FERMENTADOR 1
  if (strcmp(drukdata, "1" ) == 0){ Cabecao1.HabilitaExterno(); Serial.println("HABILITADO EXTERNO FERM_1"); Cabecao1.DesProtege(); drukdata[0] = '-'; }      // ATIVA RECIRCULACAO DA CAMISA FERMENTADOR 2
  if (strcmp(drukdata, "2" ) == 0){ Cabecao2.HabilitaInterno(); Serial.println("HABILITADO INTERNO FERM_2"); Cabecao2.DesProtege(); drukdata[0] = '-'; }      // ATIVA RECIRCULACAO INTERNA FERMENTADOR 2
  if (strcmp(drukdata, "3" ) == 0){ Cabecao2.HabilitaExterno(); Serial.println("HABILITADO EXTERNO FERM_2"); Cabecao2.DesProtege(); drukdata[0] = '-'; }      // ATIVA RECIRCULACAO INTERNA FERMENTADOR 1
  
                //DESABILITA
  if (strcmp(drukdata, "a" ) == 0){ Cabecao1.DesabilitaInterno(); Serial.println("DESABILITADO INTERNO FERM_1"); Cabecao1.Protege(); drukdata[0] = '-'; }
  if (strcmp(drukdata, "b" ) == 0){ Cabecao1.DesabilitaExterno(); Serial.println("DESABILITADO EXTERNO FERM_1"); Cabecao1.Protege(); drukdata[0] = '-'; }
  if (strcmp(drukdata, "c" ) == 0){ Cabecao2.DesabilitaInterno(); Serial.println("DESABILITADO INTERNO FERM_2"); Cabecao2.Protege(); drukdata[0] = '-'; }
  if (strcmp(drukdata, "d" ) == 0){ Cabecao2.DesabilitaExterno(); Serial.println("DESABILITADO EXTERNO FERM_2"); Cabecao2.Protege(); drukdata[0] = '-'; }
//}

TelaLCD(vMgsLCD, Cabecao1.RetornaTemperaturaExterna(), Cabecao2.RetornaTemperaturaExterna(), Cabecao1.RetornaTemperaturaInterna(), Cabecao2.RetornaTemperaturaInterna());


//Envia todos os dados calculados a saida serial
if (strcmp(drukdata, "i" ) == 0)
{
    TotalString = Comunicando + VolBrassagem + /*Volume_fluxo2 +*/ VolLavagem + /*Volume_fluxo1 +*/ TempBrassagem + vTempBrassagem + TempLavagem + vTempLavagem + TempFermentador1 + Cabecao1.RetornaTemperaturaExterna() + TempFermentador2 + Cabecao2.RetornaTemperaturaExterna() + TemIntFerm2 + Cabecao2.RetornaTemperaturaInterna() + TemIntFerm1 + Cabecao1.RetornaTemperaturaInterna() + TempBunker + vIntTempBunker + TempAmbiente + vIntTermAmbiente + Humid + "<CR>";
    Serial.print( TotalString+ "\n" ); 
    if (Eliz.RetornaHabilitadoHardware() == 0 )
    {
      vTempoElizUltResp = millis();
    }    
    drukdata[0] = '-';
}
        
//Zera todas as variaveis de calculos
if (strcmp(drukdata, "Z" ) == 0)
{
    Serial.println("Zerando variaveis de protocolo...");
    vTempBrassagem = 0;
    vTempLavagem = 0;
    vTempCabecao1Externo = 0;
    vTempCabecao1Interno = 0;
    vTempCabecao2Externo = 0;
    vTempCabecao2Interno = 0;
    vIntTermAmbiente = 0;
    Serial.println("Zerando dados da EPROM de temperatura padrao...");
    Cabecao1.ZeraTemperaturaAlvo();
    Cabecao2.ZeraTemperaturaAlvo();
    Eliz.DesabilitaHardware();
    Serial.println("Processo concluido.");    
    
    if (Eliz.RetornaHabilitadoHardware() == 0 )
    {
      vTempoElizUltResp = millis();
    }        

     drukdata[0] = '-';
}

if (strcmp(drukdata, "F" ) == 0)
{
    Serial.println("**************************************************************"); 
    Serial.println("BLAIR EM MODO AUTONOMO ELIZABETH SOMENTE ESTCUTA >"); 
    Serial.println("**************************************************************"); 
    Eliz.HabilitaHardware();
     drukdata[0] = '-';
}

if (strcmp(drukdata, "S" ) == 0)
{
    Serial.println("**************************************************************"); 
    Serial.println("ENVIANDO E RECEBENDO DADOS DO SERVIDOR MODO MANUAL  "); 
    Serial.println("**************************************************************"); 
  
      Cabecao1.EnviaDadosServidor();
      Cabecao2.EnviaDadosServidor();
      vContaTempoEnviaDadosServidor = millis();
      Cabecao1.RecebeTemperaturaAlvo();
      Cabecao2.RecebeTemperaturaAlvo();
 drukdata[0] = '-';
}

/*************************************************************************
****   INICIO DA CONFIGURACAO DA INTERFACE DIRETO POR HARDWARE
*************************************************************************/
  if (strcmp(drukdata, "L" ) == 0) //Lista os parametros para fermentacao
  {
    Serial.print("CABECAO 1 - TEMPERATURA: ");
    Serial.println(Cabecao1.RetornaTemperaturaAlvo());    
    Serial.print("CABECAO 2 - TEMPERATURA: ");
    Serial.println(Cabecao2.RetornaTemperaturaAlvo());
    
    if (Eliz.RetornaHabilitadoHardware() == 0 )
    {
      vTempoElizUltResp = millis();
    }        
    
     drukdata[0] = '-';
  }  

  if (strcmp(drukdata, "s" ) == 0) //Salva os parametros para fermentacao do cabecao 1
  {
    Serial.println("**************************************************************"); 
    Serial.println("FERMENTADOR 1 -> DIGITE VALOR DE 0-99 <ENTER> APOS * e <ENTER>"); 
    Serial.println("**************************************************************"); 
    Cabecao1.GravaTemperaturaAlvo();
     drukdata[0] = '-';
  }

  if (strcmp(drukdata, "t" ) == 0) //Salva os parametros para fermentacao do cabecao 2
  {
    Serial.println("**************************************************************"); 
    Serial.println("FERMENTADOR 2 -> DIGITE VALOR DE 0-99 <ENTER> APOS * e <ENTER>"); 
    Serial.println("**************************************************************"); 
    Cabecao2.GravaTemperaturaAlvo();
     drukdata[0] = '-';
  }

  if (strcmp(drukdata, "Man" ) == 0) //Exibe o manual do sistema.
  {
    Man();
    
    if (Eliz.RetornaHabilitadoHardware() == 0 )
    {
      vTempoElizUltResp = millis();
    }
            
     drukdata[0] = '-';
  }

// Verifica se a Elizabeth esta conectada ou nao.
  if ( (vTempoElizUltResp - millis()) < 3000 )
  {
    if (vMgsLCD == "MODO HARDWARE") {
    Serial.println("ELIZABETH N CONTROLE"); }
    vMgsLCD = "ELIZABETH N CONTROLE";
  }
  else
  {
    vTempoElizControle = millis() - vTempoElizUltResp;
    if (vTempoElizControle >= 18000) // Trez minutos o Hardware assume
    {
      if ((Cabecao1.RetornaTemperaturaAlvo() != 100 ) || (Cabecao2.RetornaTemperaturaAlvo() != 100 ))  
      {
        if (vMgsLCD == "ELIZABETH N CONTROLE"){ Serial.println("MODO HARDWARE"); }
        vMgsLCD = "MODO HARDWARE";
      }
    }
  }

/* Envia dados para o servidor */

  if ((millis() - vContaTempoEnviaDadosServidor) >= vTempoElizEsperaEnviaDadosServidor )
  {
      Cabecao1.EnviaDadosServidor();
      Cabecao2.EnviaDadosServidor();
      vContaTempoEnviaDadosServidor = millis();
      Cabecao1.RecebeTemperaturaAlvo();
      Cabecao2.RecebeTemperaturaAlvo();
  }
 
}

/************************ FINAL DO PROGRAMA PRINCIPAL ****************************/
// F1E, F2E, F1I, F2I
/*void TelaLCD(String Mensagem, float temp1, float temp2, float temp3, float temp4)
{
  char outstr[20];
  lcd.setCursor(0,0);
  lcd.print("                    "); //Zera a mensagem
  lcd.setCursor(0,0);
  lcd.print(Mensagem);
  
  lcd.setCursor(0,1);
  dtostrf(temp1, 2, 2, outstr);
  lcd.print("F1E:");
  lcd.print(outstr);
  
  lcd.setCursor(11,1);
  dtostrf(temp2, 2, 2, outstr);
  lcd.print("F2E:");
  lcd.print(outstr);
  
  lcd.setCursor(0,3);
  dtostrf(temp3, 2, 2, outstr);
  lcd.print("F1I:");
  lcd.print(outstr);

  lcd.setCursor(11,3);
  dtostrf(temp4, 2, 2, outstr);
  lcd.print("F2I:");
  lcd.print(outstr);

}
*/
void TelaLCD(String Mensagem, float temp1, float temp2, float temp3, float temp4)
{
  char outstr[20];
  lcd.setCursor(0,0);
  lcd.print("                    "); //Zera a mensagem
  lcd.setCursor(0,0);
  lcd.print(Mensagem);

  lcd.setCursor(0,1);  
  lcd.print("FERM 1");
  
  lcd.setCursor(0,2);
  dtostrf(temp1, 2, 2, outstr);
  lcd.print("EXT:");
  lcd.print(outstr);
  
  lcd.setCursor(0,3);
  dtostrf(temp3, 2, 2, outstr);
  lcd.print("INT:");
  lcd.print(outstr);

  lcd.setCursor(12,1);  
  lcd.print("FERM 2");
  
  lcd.setCursor(11,2);
  dtostrf(temp2, 2, 2, outstr);
  lcd.print("EXT:");
  lcd.print(outstr);
  
  lcd.setCursor(11,3);
  dtostrf(temp4, 2, 2, outstr);
  lcd.print("INT:");
  lcd.print(outstr);

}


void Man(){

  Serial.println("---------------------------------------------------------");
  Serial.print  ("----  VERSAO.....:");
  Serial.print( VERSAO);
  Serial.println("    --------------------------------");
  Serial.println("---------------------------------------------------------");
  Serial.println("-     i - Retorna as informacoes do protocolo            -");
  Serial.println("-     0 a 3 - Habilita os reles                          -");  
  Serial.println("-     a a d - Desabilita os reles                        -");  
  Serial.println("-     Z - Zera todos os dados e variaveis do sist        -");
  Serial.println("-     L - Lista os valores de temperatura hardware       -");
  Serial.println("-     s - Salva a temperatura para modo hardware Ferm1   -");
  Serial.println("-     t - Salva a temperatura para modo hardware Ferm2   -");
  Serial.println("-     F - Forca modo HardWare independente se Eliz Ligada-");
  Serial.println("-     S - Forca o envio e recebimento de dados Server    -");
  Serial.println("-     Man - Lista as opcoes do Menu                      -");
  Serial.println("---------------------------------------------------------");

}

