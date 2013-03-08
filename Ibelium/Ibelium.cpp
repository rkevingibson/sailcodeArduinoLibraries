#include "Arduino.h"
#include "Ibelium.h"
#include <string.h>

Ibelium::Ibelium() {
	//Empty Constructor
}

int Ibelium::init() {
	//Initializes and tests the module. 
	Serial.begin(115200);
	delay(2000);
	pinMode(LED, OUTPUT);
	pinMode(ON_MODULE_PIN, OUTPUT);
	switchModule(); //Switches the module on

	//Send SMS mode to Text
	sendATCmd("AT+CPMS=\"SM\",\"SM\",\"SM\""); //Select message storage
	return sendATCmd("AT+CMGF=1");
}

int Ibelium::sendATCmd(char* cmd) {
	//Send an AT Command without returning the response
	//Returns 1 if successful, 0 otherwise
	char* resp;
	int s;
	s = sendATQuery(cmd, resp);
	free(resp);
	return s;
}

int Ibelium::sendATQuery(char* cmd, char* response) {
	//Send an AT command, and get the response from the board.
	//Returns 1 if successful, 0 otherwise
	free(response); //Avoid creating a memory leak.
	_error=0;
	_watchdog=0;

	int size=10;
	char* buff = (char*) calloc(size, sizeof(char)); //Allocate a 10 character buffer for the response
	int x=0;
	Serial.print(cmd);
	Serial.print("\x0D\x0A"); 
	Serial.flush();
	delay(2000);
	//By now, the command should be sent completely.

	do {
		_watchdog=0;
		while(Serial.available()==0 && _watchdog<1000) _watchdog++;

		if(_watchdog>=1000) //No response received after a certain length of time
		{
			Serial.println("Watchdog timer gone");
			_error=2;
			break;
		}


		buff[x]=Serial.read(); //Read the next character
		x++;

		if(x>=size) { //Reallocate memory if message is getting longer.
			size *= 2;
			buff = (char*) realloc(buff, size);
		}

		if(size>256){ //Reponse too long
			_error = 1;
			//Serial.println(size);
			break;
		}

	} while(x<2 || !(buff[x-1]=='K' && buff[x-2]=='O')  );//Keep Waiting for a response until the last two characters are 'OK'

	buff[x]=0x0;
	response = buff;

	if(_error==0)
		return 1;
	else 
		return 0;	
}

int Ibelium::sendSMS(char* number, char* message, char* response) {
	//Sends an AT Command to send an SMS message
	//AT+CMGS="number"<CR> message <ESC>

	//Cleaner attempt
	char* cmd = appendStrings("AT+CMGS=\"", number);
	cmd = appendStrings(cmd, "\"\r");
	cmd = appendStrings(cmd, message);
	cmd = appendStrings(cmd, "\x1a");

	return sendATQuery(cmd, response);
}

int Ibelium::readSMS(sms* msg) {
	//Reads the most recent SMS and deletes it
	/*Message Format: (reference for string parsing)
		+CMGL:<index>,<stat>,<number>,<unknown>,<timestamp><CR><LF>
		<data>

		<index> = the index in memory of the message
		<stat> = status of the message, either REC UNREAD, REC READ, STO UNSENT, STO UNREAD. We only want to read ones that are REC, not STO.
		<number> = Where the message came from
		<unknown> = seems to be empty
		<timestamp> = "yy/mm/dd, hh:mm:ss-20"
	*/

	free(msg->number); //Make sure response is a NULL pointer.
	free(msg->message);
	free(msg->dateTime);
	int index=0;
	char* resp;
	int i = sendATQuery("AT+CMGL=\"ALL\"", resp);
	if(i!=0) { //Command successfully sent/received
		//First, find the message index to allow for deletion afterwards
		int j=0;
		int k=0;
		while(resp[j]!=':') j++;
		index = atoi(&(resp[j+1]));//This number should indicate the index of the message in memory
		msg->index = index;
		//Second, copy the originating number into the structure.
		while(resp[j]!=',') j++;
		while(resp[j]!=',') j++;
		j+=2;
		msg->number = substring(resp, j, j+11); //Should copy the number to the struct.

		//Thirdly, copy the datestring from the message.
		while(resp[j]!=',') j++;
		while(resp[j]!=',') j++;
		j+=2;
		k=j;
		while(resp[k+1]!='"') k++;
		msg->dateTime = substring(resp, j, k);

		//Lastly, copy the message into the message structure
		while(resp[j]!='\x0A') j++;
		j++; //j indexes the beginning of the message;
		k = j;//k should index the end of the message;
		while(resp[k+1]!='O'&& resp[k+2]!='K') k++; 
		msg->message = substring(resp, j, k);
	}	
	return i; 
}

int Ibelium::checkForSMS() {
	/*Returns an integer indicating the number of SMS messages in memory, or -1 if unsuccessful
	*/
	char* resp;
	int i;
	i= sendATQuery("CPMS=\"SM\",\"SM\",\"SM\"", resp);
	if(i!=1) {
		return -1;
	}
	//Now need to parse the returned string to get the number of messages.
	i=1;
	while(resp[i-1]!=':') i++;//Now i indexes the first digit of the amount of messages in memory.
	i=atoi(&(resp[i]));//Now i is the number of messages in memory, or 0 if an error occurs.
	return i;
} 

void Ibelium::switchModule() {
    digitalWrite(ON_MODULE_PIN,HIGH);
    delay(2000);
    digitalWrite(ON_MODULE_PIN,LOW);
}

char* Ibelium::appendStrings(char* a, char* b) {
	int lenA, lenB;
	char* newString;
	lenA=0;
	lenB=0;
	//Find the length of the two strings;
	while(a[lenA]!='\0') lenA++;
	while(b[lenB]!='\0') lenB++;

	newString = (char*) malloc(sizeof(char)*(lenA+lenB+1));

	for(int i=0; i<lenA; i++) {
		newString[i] = a[i];
	}
	for(int i=0; i<lenB; i++) {
		newString[lenA+i] = b[i];
	}
	newString[lenA+lenB]='\0';
	free(a);
	free(b);

	return newString;
}

char* Ibelium::substring(char* str, int a, int b) {
	int l = b-a +1;//Length of the string, plus null-terminal;
	char* sub = (char*) malloc(sizeof(char)*l);

	for(int i=0;i<l-1;i++) {
		sub[i] = str[a+i];
	}
	sub[l-1] = '\0';
	return sub;
}