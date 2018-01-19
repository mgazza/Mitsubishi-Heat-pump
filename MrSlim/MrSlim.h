#ifndef MrSlim_h
#define MrSlim_h
#include <Arduino.h>
#include <WString.h>
#include <ESPAsyncWebServer.h>

typedef void (*LogCallback) (String message, const char* level);

const byte Power=0x01;
const byte Mode=0x02;
const byte SetPoint=0x04;
const byte FanSpeed=0x08;
const byte Vane=0x10;
const byte WideVane=0x80;

//enum Control {Power=0x01, Mode=0x02, SetPoint=0x04, FanSpeed=0x08, Vane=0x10, WideVane=0x10};

const byte CONNECT[] = {0xfc, 0x5a, 0x01, 0x30, 0x02, 0xca, 0x01, 0xa8};
const byte HEADER[] = {0xFC, 0x41, 0x01, 0x30, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const byte BLOCK2[] = {0xFC, 0x42, 0x01, 0x30, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7B};
const byte BLOCK3[] = {0xFC, 0x42, 0x01, 0x30, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A};
const byte BLOCK4[] = {0xFC, 0x42, 0x01, 0x30, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

class MrSlim
{  
	private:		
		//const byte CONTROL_PACKET_1[5] = {0x01,    0x02,  0x04,  0x08, 0x10};
									   //{"POWER","MODE","TEMP","FAN","VANE"};
		//const byte CONTROL_PACKET_2[1] = {0x01};
									   //{"WIDEVANE"};
		const byte POWER[2]            = {0x00, 0x01};
		const String POWER_MAP[2]      = {"OFF", "ON"};
		const byte MODE[5]             = {0x01,   0x02,  0x03, 0x07, 0x08};
		const String MODE_MAP[5]       = {"HEAT", "DRY", "COOL", "FAN", "AUTO"};
		const byte TEMP[16]            = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
		const int TEMP_MAP[16]         = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16};
		const byte FAN[6]              = {0x00,  0x01,   0x02, 0x03, 0x05, 0x06};
		const String FAN_MAP[6]        = {"AUTO", "QUIET", "1", "2", "3", "4"};
		const byte VANE[7]             = {0x00,  0x01, 0x02, 0x03, 0x04, 0x05, 0x07};
		const String VANE_MAP[7]       = {"AUTO", "1", "2", "3", "4", "5", "SWING"};
		const byte WIDEVANE[7]         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x0c};
		const String WIDEVANE_MAP[7]   = {"<<", "<",  "|",  ">",  ">>", "<>", "SWING"};
		const byte ROOM_TEMP[32]       = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
										  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
		const int ROOM_TEMP_MAP[32]    = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
										  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
        HardwareSerial * _HardSerial;
		int brateIndex =0;
		int  brates[2] = {9600,2400};
		bool doWrites;
		bool hasChanged;
		byte power = 0;
		byte mode = 0;
		byte setPoint = 0;
		byte fanSpeed = 0;
		byte vane = 0;
		byte wideVane = 0;
		word faultCode = 0;
		byte roomTemperature = 0;
		bool connected;
		const int r_checkInterval = 5000;
		const int w_checkInterval = 500;
		long r_lastCheck=0;
		long w_lastCheck=0;
		
		byte w_power = 0xff;
		byte w_mode = 0xff;
		byte w_setPoint = 0xff;
		byte w_fanSpeed = 0xff;
		byte w_vane = 0xff;
		byte w_wideVane = 0xff;
		
		void LogBuffer(const byte* buffer, int size, const char* level);

		bool updateSettings();
		void connect();
		bool readSettings();
		bool readRoomTemp();
		bool readFaultCode();
		byte calculate2sComplement(byte *buffer, byte bufferLength);
		bool verify2sComplement(byte *buffer, byte bufferLength);
		unsigned int convertValueToSetpoint(byte rawValue);
		byte convertSetpointToValue(unsigned int temperatureValue);
		unsigned int convertValueToRoomTemperature(byte rawValue);

		String mapByteToString(const String valuesMap[], const byte byteMap[], int len, byte value);
		int mapByteToInt(const int valuesMap[], const byte byteMap[], int len, byte value);
		byte mapIntToByte(const int valuesMap[], const byte byteMap[], int len, int value);
		byte mapStringToByte(const String valuesMap[], const byte byteMap[], int len, String value);
		LogCallback _Log;
		void doReads();
public:
	MrSlim(LogCallback log, HardwareSerial* hardSerial);
	void start();
	void stop();
	void loop();
	bool getHasChanged();
	int getRoomTemperature();
	String getPower();
	String getMode();
	int getSetPoint();
	String getFanSpeed() ;
	String getVane();
	String getWideVane();

	void setPower(String value);
	void setMode(String value);
	void setSetPoint(int value);
	void setFanSpeed(String value) ;
	void setVane(String value);
	void setWideVane(String value);

	word getFaultCode();
	bool isConnected();
};

#endif
