//#include <arduino.h>
#define		TWO_MONITOR
#define		IP_ADDRESS_MON_1	192,168,1,3
#define		MON_PORT	1515
#define		IP_ADDRESS_MON_2	192,168,1,13
#define		str(a) #a
#define		xstr(a) str(a)
#define		IP_ADDRESS_STRING(a,b,c,d) xstr(a)"." xstr(b)"." xstr(c)"." xstr(d)
#define		XIP_ADDRESS_STRING(abcd) IP_ADDRESS_STRING(abcd)

#include <Wire.h>
#include "BH1750.h"
#include <UIPEthernet.h>

#define		MON1				1
#define		MON2				0
#define		SIGNAL_POWER_OFF	0
#define		SIGNAL_POWER_ON		1
#define		gerkon	5
#define		BUF_SIZE 128
#define		MIN_LEN_CMD	8
#define		LEN_HEAD	sizeof(header)
#define		LEN_CRC_HEAD		2 //0x25 + 0x00
#define		LEN_DATA			1
#define		LEN_CRC				1
#define		LEN_BRIGHT_ANSWER	1
#define		SHIFT_LEN_DATA		3
#define		SHIFT_ACK			4
#define		SHIFT_CMD			5
#define		MAX_LUX_LEVEL_MON1	2500 
#define		MIN_LUX_LEVEL_MON1	200
#define		MAX_LUX_LEVEL_MON2	2500 
#define		MIN_LUX_LEVEL_MON2	200
#define		MAX_BRIGHT_LEVEL_MON1	80
#define		MIN_BRIGHT_LEVEL_MON1	20
#define		MAX_BRIGHT_LEVEL_MON2	80
#define		MIN_BRIGHT_LEVEL_MON2	20
#define		DIFF			5

void checkAnswer(uint8_t num_mon);
uint8_t find_cmd(uint8_t *b, uint8_t number_mon);
void printHex(uint8_t * cmd, uint8_t size);
uint8_t checkCRC(uint8_t *b, uint8_t crc_size);
void writeCRC(uint8_t * cmd_arr, uint8_t size);

//#line 39 "C:\\Users\\Admin\\YandexDisk\\AVR\\Arduino\\TcpClient\\TcpClient.ino"
EthernetClient client;
BH1750 lightMeter;

unsigned long next;
const uint8_t get_lamp_value[] = { 0xAA, 0x58, 0x00, 0x00, 0x58 };
uint8_t set_lamp_value[] = { 0xAA, 0x58, 0x00, 0x01, 0x00, 0x00 };
//answer lamp_control {0xAA, 0xFF, 0x00, 0x03(dataltngth), 0x41(A), 0x58, lamp_value, crc};
const uint8_t power_on[] = { 0xAA, 0x11, 0x00, 0x01, 0x01, 0x13 };
const uint8_t power_off[] = { 0xAA, 0x11, 0x00, 0x01, 0x00, 0x12 };
const uint8_t header[] = { 0xAA, 0xFF, 0x00 };
const uint8_t status[] = { 0xAA, 0x00, 0x00, 0x00, 0x00 };
const uint8_t get_bright[] = { 0xAA, 0x25, 0x00, 0x00, 0x25 };
uint8_t set_bright[] = { 0xAA, 0x25, 0x00, 0x01, 0x00, 0x00 };
uint8_t answer_bright = 0;
uint8_t buf[BUF_SIZE] = { 0 };
uint8_t c;
volatile uint8_t cnt_c = 0;
uint8_t mon1_cur_lamp_value = 0;
uint8_t new_lamp_value = 0;
uint16_t diff = 0;
uint8_t mon2_cur_lamp_value = 0;
uint8_t ans = 0;
uint8_t *res_mem = 0;
float lux = -2;
uint8_t mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

void setup() {
	Serial.begin(9600);
	Wire.begin();
	while (1) {
		Serial.println("BH1750 Init");
		lightMeter.begin();
		delay(500);
		lux = lightMeter.readLightLevel();
		if (lux != -2)
		{
			Serial.println("BH1750 Ok");
			break;
		}
		delay(5000);
	}
	Serial.println("");
  
	while (1) {
		Serial.println("Ethernet Init");
		int ret = Ethernet.begin(mac);
		if (ret != 0)
		{
			Serial.println("Ethernet Ok");
			break;
		}
		Serial.println("Not DHCP adress");
		Serial.print("localIP: ");
		Serial.println(Ethernet.localIP());
	}
	Serial.print("localIP: ");
	Serial.println(Ethernet.localIP());
	Serial.print("subnetMask: ");
	Serial.println(Ethernet.subnetMask());
	Serial.print("gatewayIP: ");
	Serial.println(Ethernet.gatewayIP());
	Serial.print("dnsServerIP: ");
	Serial.println(Ethernet.dnsServerIP());
	Serial.println("");
	Serial.println("");
}

void loop() {
	lux = lightMeter.readLightLevel();
	Serial.println("-------------------------------------------");
	Serial.println("Monitor1:");
	Serial.println("MAX_LUX_LEVEL_MON1: " xstr(MAX_LUX_LEVEL_MON1));
	Serial.println("MIN_LUX_LEVEL_MON1: " xstr(MIN_LUX_LEVEL_MON1));
	Serial.print("SensorLight value: ");
	Serial.print(lux);
	Serial.println(" lx;");
	if (lux > MAX_LUX_LEVEL_MON1)
	{
		lux = MAX_LUX_LEVEL_MON1;
		Serial.print("Set MAX_LUX_LEVEL_MON1: ");
		Serial.print(MAX_LUX_LEVEL_MON1);
		Serial.println(" lx;");
	}
	
	if (lux < 0)
	{
		lux = MAX_LUX_LEVEL_MON1;
		Serial.println("ERROR: NOT SENSOR.");
		Serial.print("Set MAX_LUX_LEVEL_MON1: ");
		Serial.print(MAX_LUX_LEVEL_MON1);
		Serial.println(";");
	}
	else {
		if (lux < MIN_LUX_LEVEL_MON1)
		{
			lux = MIN_LUX_LEVEL_MON1;
			Serial.print("Set MIN_LUX_LEVEL_MON1: ");
			Serial.print(MIN_LUX_LEVEL_MON1);
			Serial.println(" lx;");
		}
	}
	new_lamp_value = map((uint16_t)lux, MIN_LUX_LEVEL_MON1, MAX_LUX_LEVEL_MON1, MIN_BRIGHT_LEVEL_MON1, MAX_BRIGHT_LEVEL_MON1);
	Serial.print("New Lamp Value: ");
	Serial.print(new_lamp_value);
	Serial.print(" (");
	Serial.print(new_lamp_value, HEX);
	Serial.println("h);");
	Serial.print("Cur Lamp Value Monitor#1: ");
	Serial.print(mon1_cur_lamp_value);
	Serial.print(" (");
	Serial.print(mon1_cur_lamp_value, HEX);
	Serial.println("h);");
	
	if (new_lamp_value > mon1_cur_lamp_value)
	{
		diff = new_lamp_value - mon1_cur_lamp_value;
	}
	else {
		diff = mon1_cur_lamp_value - new_lamp_value;
	}
	
	if (diff > DIFF) {
		Serial.println("Set New Lamp Value to Monitor#1");
		set_lamp_value[4] = new_lamp_value;
		writeCRC(set_lamp_value, sizeof(set_lamp_value));
		Serial.print("Connect to " XIP_ADDRESS_STRING(IP_ADDRESS_MON_1) "/" xstr(MON_PORT));
		if (client.connect(IPAddress(IP_ADDRESS_MON_1), MON_PORT))
		{
			Serial.println(" -> Connected");
			client.flush();
			printHex(set_lamp_value, sizeof(set_lamp_value));
			client.write(set_lamp_value, sizeof(set_lamp_value));
			next = millis() + 10000;
			while (client.available() == 0) {
				if (millis() > next)
				{
					Serial.println("");
					Serial.println(XIP_ADDRESS_STRING(IP_ADDRESS_MON_1) "/" xstr(MON_PORT)" not available");
					Serial.println("");
					break;
				}
			}
			checkAnswer(MON1);
			Serial.println("Close connect");
			Serial.println("");
			Serial.println("");
			client.stop();
			
		}
		else {
			Serial.println("");
			Serial.println(XIP_ADDRESS_STRING(IP_ADDRESS_MON_1) "/" xstr(MON_PORT)" connect failed");
			Serial.println("");
			delay(10000);
		}		
	}
	else {
		Serial.println("No change bright");
		Serial.println();
	}
	delay(10000);
	
#ifdef	TWO_MONITOR
	lux = lightMeter.readLightLevel();
	Serial.println("-------------------------------------------");
	Serial.println("Monitor2:");
	Serial.println("MAX_LUX_LEVEL_MON2: " xstr(MAX_LUX_LEVEL_MON2));
	Serial.println("MIN_LUX_LEVEL_MON2: " xstr(MIN_LUX_LEVEL_MON2));
	Serial.print("SensorLight value: ");
	Serial.print(lux);
	Serial.println(" lx;");
	if (lux > MAX_LUX_LEVEL_MON2)
	{
		lux = MAX_LUX_LEVEL_MON2;
		Serial.print("Set MAX_LUX_LEVEL_MON2: ");
		Serial.print(MAX_LUX_LEVEL_MON2);
		Serial.println(" lx;");
	}
	
	if (lux < 0)
	{
		lux = MAX_LUX_LEVEL_MON2;
		Serial.println("ERROR: NOT SENSOR.");
		Serial.print("Set MAX_LUX_LEVEL_MON2: ");
		Serial.print(MAX_LUX_LEVEL_MON2);
		Serial.println(";");
	}
	else {
		if (lux < MIN_LUX_LEVEL_MON2)
		{
			lux = MIN_LUX_LEVEL_MON2;
			Serial.print("Set MIN_LUX_LEVEL_MON2: ");
			Serial.print(MIN_LUX_LEVEL_MON2);
			Serial.println(" lx;");
		}
	}
	new_lamp_value = map((uint16_t)lux, MIN_LUX_LEVEL_MON2, MAX_LUX_LEVEL_MON2, MIN_BRIGHT_LEVEL_MON2, MAX_BRIGHT_LEVEL_MON2);
	Serial.print("New Lamp Value: ");
	Serial.print(new_lamp_value);
	Serial.print(" (");
	Serial.print(new_lamp_value, HEX);
	Serial.println("h);");
	Serial.print("Cur Lamp Value Monitor#2: ");
	Serial.print(mon2_cur_lamp_value);
	Serial.print(" (");
	Serial.print(mon2_cur_lamp_value, HEX);
	Serial.println("h);");
	
	if (new_lamp_value > mon2_cur_lamp_value)
	{
		diff = new_lamp_value - mon2_cur_lamp_value;
	}
	else {
		diff = mon2_cur_lamp_value - new_lamp_value;
	}
	
	if (diff > DIFF) {
		Serial.println("Set New Lamp Value to Monitor#2");
		set_lamp_value[4] = new_lamp_value;
		writeCRC(set_lamp_value, sizeof(set_lamp_value));
		Serial.print("Connect to " XIP_ADDRESS_STRING(IP_ADDRESS_MON_2) "/" xstr(MON_PORT));
		if (client.connect(IPAddress(IP_ADDRESS_MON_2), MON_PORT))
		{
			Serial.println(" -> Connected");
			client.flush();
			printHex(set_lamp_value, sizeof(set_lamp_value));
			client.write(set_lamp_value, sizeof(set_lamp_value));
			next = millis() + 10000;
			while (client.available() == 0) {
				if (millis() > next)
				{
					Serial.println("");
					Serial.println(XIP_ADDRESS_STRING(IP_ADDRESS_MON_2) "/" xstr(MON_PORT)" not available");
					Serial.println("");
					break;
				}
			}
			checkAnswer(MON2);
			Serial.println("Close connect");
			Serial.println("");
			Serial.println("");
			client.stop();
			
		}
		else {
			Serial.println("");
			Serial.println(XIP_ADDRESS_STRING(IP_ADDRESS_MON_2) "/" xstr(MON_PORT)" connect failed");
			Serial.println("");
			delay(10000);
		}
	}
	else {
		Serial.println("No change bright");
		Serial.println();
	}
	delay(10000);
#endif
	  
}

void checkAnswer(uint8_t num_mon) {
	cnt_c = 0;
	while (client.available() > 0) {
		c = client.read();
		buf[cnt_c++] = c;
		cnt_c &= 0x7F;
	}
	
	uint8_t cnt_c_const = cnt_c;
	while (cnt_c >= MIN_LEN_CMD) {
		Serial.print("receive ch> ");
		Serial.println(cnt_c);
		res_mem = (uint8_t *) memmem(buf, cnt_c_const, header, sizeof(header));  //finds the start of the first occurrence of the substring
		if(res_mem != NULL)
		{
			uint8_t s = find_cmd(res_mem, num_mon);
			if (s)
			{
				cnt_c = cnt_c - s;
			}
			else
			{
				cnt_c = cnt_c - MIN_LEN_CMD;
				for (uint8_t i = 0; i < MIN_LEN_CMD; i++) {
					res_mem[i] = 0;
				}
				Serial.println("Erase brake command");
				Serial.println();
			}
		}
		else
		{
			Serial.println("No find command");
			Serial.println();
			break;
		}
	}
}

uint8_t find_cmd(uint8_t *b, uint8_t number_mon) {
	uint8_t data_len = b[SHIFT_LEN_DATA];
	uint8_t crc_len = LEN_CRC_HEAD + LEN_DATA + data_len;  //ff 00 + 03 + 0a cmd + xx
	uint8_t cmd_len = LEN_HEAD + LEN_DATA + data_len + LEN_CRC; //aa+ff+00+03+0a+cmd+xx+crc
	if (cmd_len > cnt_c)
	{
		Serial.println("ERROR: Lenght cmd brake");
		return 0;
	}
	if (b[SHIFT_ACK] == 0x41) //ACK
		{
			Serial.println("answer: ASK");
			if (checkCRC(b, crc_len))
			{
				Serial.println("CRC: Ok");
				Serial.print("find command> ");
				Serial.print(b[SHIFT_CMD], HEX);
				Serial.print("; data lenght> ");
				Serial.print(data_len, HEX);
				Serial.print("; data cmd> ");
				Serial.println(cmd_len, HEX);
				if (b[SHIFT_CMD] == 0x58)
				{
					uint8_t val = b[6];
					Serial.print("Answer: bright is> ");
					Serial.print(val);
					Serial.print(" (");
					Serial.print(val, HEX);
					Serial.println(")h;");
					if (val == new_lamp_value)
					{
						if (number_mon)
						{
							mon1_cur_lamp_value = new_lamp_value;
						} 
						else
						{
							mon2_cur_lamp_value = new_lamp_value;
						}
					}
				}
				for (uint8_t i = 0; i < cmd_len; i++) {
					Serial.print(b[i], HEX);
					Serial.print(' ');
					buf[i] = 0;
				}
				Serial.println();
				//Serial.println();
				return cmd_len;
			}
			else
			{
				Serial.println("CRC: ERROR");
				return 0;
			}
		}
	else
	{
		Serial.println("ERROR: NASK");
		return 0;
	}
}

void printHex(uint8_t * cmd, uint8_t size) {
	Serial.print("Send cmd> ");
	for (uint8_t i = 0; i < size; i++) {
		Serial.print(cmd[i], HEX);
		Serial.print(' ');
	}
	Serial.println();
	Serial.println();
}

uint8_t checkCRC(uint8_t *b, uint8_t crc_size) {
	uint8_t crc;
	uint16_t summ = 0;
	b += 1;  //start to 0xFF
	//for (uint8_t i = 0; i < size; i++){
	//Serial.print(buf[i], HEX);
	//Serial.print(' ');
	//}
	//Serial.println();
	for(uint8_t i = 0 ; i < crc_size ; i++) {
		summ = summ + b[i];
	}
	crc = (uint8_t)(0x00FF & summ);
	if (crc == b[crc_size]) {
		return 1;
	}
	return 0;
}

void writeCRC(uint8_t * cmd_arr, uint8_t size) {
	uint8_t s = size - LEN_CRC;
	uint16_t summ = 0;
	for (uint8_t i = 1; i < s; i++) {
		summ = summ + cmd_arr[i];
	}
	cmd_arr[s] = (uint8_t)(0x00FF & summ);
}
