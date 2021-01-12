void readSensor(int *concentration, int *temperature){

 
  byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
  byte response[9]; 

  co2Serial.write(cmd, 9);
  
  memset(response, 0, 9);
  while (co2Serial.available() == 0) {
    delay(1000);
  }
 
  co2Serial.readBytes(response, 9);

 
  byte check = getCheckSum(response);
  
  if (response[8] != check) {
    Serial.println("Fehler in der Übertragung!");
    return;
  }
 
  
  *concentration = 256 * (int)response[2] + response[3];

  *temperature = response[4] - 42;

}


byte getCheckSum(byte *packet) {
  byte i;
  byte checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}
