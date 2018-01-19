#include "MrSlim.h"
#include <HardwareSerial.h>
#include <Arduino.h>

MrSlim::MrSlim(LogCallback log, HardwareSerial* hardSerial) {
  
	_Log = log;
	_HardSerial = hardSerial;
}


void MrSlim::LogBuffer(const byte* buffer, int size, const char* level)
{
    char hexbuff [(size*3)+1];
	const char* hexMap = {"0123456789ABCDEF"};
	
	byte i=0;
	for (i=0;i<size;i++){
		
		
		//do the high byte
		hexbuff[(i*3)] = hexMap[buffer[i] >> 4];
		
		//do the low byte
		hexbuff[(i*3)+1] = hexMap[buffer[i] & 0x0F];
		
				
		hexbuff[(i*3)+2] = ':';
		
		
	}
	
	hexbuff[(size*3)] = 0;
	
	_Log(hexbuff,level);
	
}

byte MrSlim::calculate2sComplement(byte *buffer, byte bufferLength) {
  byte i = 0;
  byte checksum = 0x00;

  // Checksum excludes first byte
  for(i = 0; i < (bufferLength); i++)
  {
    checksum += buffer[i];
  }

  // 2's complement
  checksum = (byte)~checksum;
  checksum += 1;

  return(checksum);
}

bool MrSlim::verify2sComplement(byte *buffer, byte bufferLength) {
	byte calculatedChecksum;

	// Checksum excludes first byte, and last byte is received checksum
	calculatedChecksum = calculate2sComplement(buffer + 1, bufferLength - 2);

	return(calculatedChecksum == buffer[bufferLength - 1]);
}

void MrSlim::connect(){
	_Log("connecting...", "debug");
	_HardSerial->write(CONNECT,sizeof(CONNECT));
	
	byte reply[22];
	if (_HardSerial->readBytes(reply, 7)==7){
		
		_Log("received reply", "debug");
		LogBuffer(reply,22,"trace");
		//check the reply
		connected = verify2sComplement(reply,7);
		
		if (connected){
			_Log("connected", "debug");
		}else{
			_Log("checksum failed", "debug");
		}
		
	}else{
		_Log("failed to connect", "debug");
		brateIndex=(brateIndex+1)% sizeof(brates);
		start();
		delay(2000);
	}
}

void MrSlim::start() {
	_Log("starting serial", "debug");
	_HardSerial->begin(brates[brateIndex], SERIAL_8E1);
}

bool MrSlim::updateSettings() {
	
	if (!doWrites || ((w_lastCheck+w_checkInterval)> millis())) return false;
	w_lastCheck = millis();
	
	doWrites = false;
	_Log("updating settings", "debug");
	byte command[22];
	byte reply[22];
	memcpy(command, HEADER, 22);
	

	if (w_power!=0xff){
		 command[6] += Power;
		 command[8] = w_power;
		 w_power=0xff;
	}
	if (w_mode!=0xff){
		command[6] += Mode;
		command[9] = w_mode;
		w_mode=0xff;
	}
	if (w_setPoint!=0xff){
		command[6] += SetPoint;
		command[10] = w_setPoint;
		w_setPoint=0xff;
	}
	if (w_fanSpeed!=0xff){
		command[6] += FanSpeed;
		command[11] = w_fanSpeed;
		w_fanSpeed=0xff;
	}
	if (w_vane!=0xff){
		command[6] += Vane;
		command[12] = w_vane;
		w_vane=0xff;
	}
	if (w_wideVane!=0xff){
		command[6] += WideVane;
		command[15] = w_wideVane;
		w_wideVane=0xff;
	}
	
	// Calculate the checksum
	command[21] = calculate2sComplement(command + 1, 22 - 2);
	
	_Log("writing settings", "debug");
	LogBuffer(command,22,"trace");
	_HardSerial->write(command,sizeof(command));
	
	if (_HardSerial->readBytes(reply,22)==22){
		if (verify2sComplement(reply,22)){
			_Log("received reply OK", "debug");
			LogBuffer(reply,22,"trace");
			return true;
		}
		_Log("received reply checksum failed", "debug");
		return false;
		
	}
	_Log("failed to receive reply", "debug");
	return false;
}

bool MrSlim::readSettings() {
	byte reply[22];
	_HardSerial->write(BLOCK2,sizeof(BLOCK2));
	_Log("reading settings block2","debug");
	if (_HardSerial->readBytes(reply,22)==22){
		if(verify2sComplement(reply,22)){
			LogBuffer(reply,22,"trace");
			
			//
			if (power != reply[8]) _Log(String("power changed ")+String(reply[8],HEX),"debug");
			if (mode != reply[9]) _Log(String("mode changed ")+String(reply[9],HEX),"debug");
			if (setPoint != reply[10]) _Log(String("set point changed ")+String(reply[10],HEX),"debug");
			if (fanSpeed != reply[11]) _Log(String("fanSpeed changed ")+String(reply[11],HEX),"debug");
			if (vane != reply[12]) _Log(String("vane changed ")+String(reply[12],HEX),"debug");
			if (wideVane != reply[15]) _Log(String("wideVane changed ")+String(reply[15],HEX),"debug");
		
			hasChanged = hasChanged ||
			power != reply[8]   //3
			|| mode != reply[9] //4
			|| setPoint != reply[10] //5
			|| fanSpeed != reply[11] //6
			|| vane != reply[12] //7
			|| wideVane != reply[15]; //10
			
			power = reply[8];
			mode = reply[9];
			setPoint = reply[10];
			fanSpeed = reply[11];
			vane = reply[12];
			wideVane = reply[15];
			return true;
		}
		_Log("received reply checksum failed","debug");
		return false;
		
	}
	_Log("failed to receive reply", "debug");
	return false;
}

bool MrSlim::readRoomTemp() {
	byte reply[22];
	_HardSerial->write(BLOCK3,sizeof(BLOCK3));
	_Log("reading settings block3","debug");
	if (_HardSerial->readBytes(reply,22)==22){
		if(verify2sComplement(reply,22)){
			LogBuffer(reply,22,"trace");
			if (roomTemperature != reply[8]) _Log("Temperature changed","debug");
			hasChanged = hasChanged || roomTemperature != reply[8];
			roomTemperature = reply[8];
			return true;
		}
		_Log("received reply checksum failed","debug");
		return false;
	}
	_Log("failed to receive reply", "debug");
	return false;
}

bool MrSlim::readFaultCode() {
	byte reply[22];
	_HardSerial->write(BLOCK4,sizeof(BLOCK4));
	_Log("reading settings block4","debug");
	if (_HardSerial->readBytes(reply,22)==22){
		if(verify2sComplement(reply,22)){
			LogBuffer(reply,22,"trace");
			
			word _faultCode = ((reply[9] << 8) + reply[10]);
			if ( _faultCode != faultCode) _Log("fault code changed","debug");
			
			hasChanged = hasChanged || _faultCode != faultCode;
			faultCode = _faultCode;
			
			return true;
		}
		_Log("received reply checksum failed","debug");
		return false;
	}
	_Log("failed to receive reply", "debug");
	return false;
}

void MrSlim::doReads() {	
    hasChanged = false;
	if ((r_lastCheck+r_checkInterval)> millis()) return;
	
	delay(10);
	connected=readSettings();
	delay(10);
	connected=readRoomTemp();
	delay(10);
	connected=readFaultCode();
		
	r_lastCheck = millis();
}

void MrSlim::loop(){
	
	if (!connected){
		connect();
		return;
	}
	connected=updateSettings();
	doReads();
	
}

bool MrSlim::getHasChanged(){
	return hasChanged;
}

byte MrSlim::mapStringToByte(const String valuesMap[], const byte byteMap[], int len, String value){
	for(int i=0;i<len;i++){
		if (valuesMap[i]==value){
			return byteMap[i];
		}
	}
	return byteMap[0];
}

String MrSlim::mapByteToString(const String valuesMap[], const byte byteMap[], int len, byte value){
	for(int i=0;i<len;i++){
		if (byteMap[i]==value){
			return valuesMap[i];
		}
	}
	return valuesMap[0];
}

int MrSlim::mapByteToInt(const int valuesMap[], const byte byteMap[], int len, byte value){
	for(int i=0;i<len;i++){
		if (byteMap[i]==value){
			return valuesMap[i];
		}
	}
	return valuesMap[0];
}

byte MrSlim::mapIntToByte(const int valuesMap[], const byte byteMap[], int len, int value){
	for(int i=0;i<len;i++){
		if (valuesMap[i]==value){
			return byteMap[i];
		}
	}
	return byteMap[0];
}

int MrSlim::getRoomTemperature(){
	return mapByteToInt(ROOM_TEMP_MAP, ROOM_TEMP, 31, roomTemperature);
}

String MrSlim::getPower(){
	return mapByteToString(POWER_MAP,POWER,2, power);
}

String MrSlim::getMode(){
	return mapByteToString(MODE_MAP,MODE,5, mode);
}

int MrSlim::getSetPoint(){
	return mapByteToInt(TEMP_MAP,TEMP,16, setPoint);
}

String MrSlim::getFanSpeed(){
	return mapByteToString(FAN_MAP,FAN,6, fanSpeed);
}

String MrSlim::getVane(){
	return mapByteToString(VANE_MAP,VANE,7, vane);
}

String MrSlim::getWideVane(){
	return mapByteToString(WIDEVANE_MAP,WIDEVANE,7, wideVane);
}

word MrSlim::getFaultCode(){
	return faultCode;
}

bool MrSlim::isConnected(){
	return connected;
}

void MrSlim::setPower(String value){
	w_power= mapStringToByte(POWER_MAP, POWER, 2, value);
	doWrites = true;
}

void MrSlim::setMode(String value){
	w_mode = mapStringToByte(MODE_MAP, MODE, 5, value);
	doWrites = true;
}

void MrSlim::setSetPoint(int value){
	w_setPoint =mapIntToByte(TEMP_MAP, TEMP,16, value);
	doWrites = true;
}

void MrSlim::setFanSpeed(String value){
	w_fanSpeed = mapStringToByte(FAN_MAP, FAN, 6, value);
	doWrites = true;
}

void MrSlim::setVane(String value){
	w_vane = mapStringToByte(VANE_MAP, VANE, 7, value);
	doWrites = true;
}

void MrSlim::setWideVane(String value){
	w_wideVane = mapStringToByte(WIDEVANE_MAP, WIDEVANE, 7, value);
	doWrites = true;
}

