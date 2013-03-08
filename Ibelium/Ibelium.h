/*  Ibelium 3G module library.  Allows for sending messages through text
messages, uploading log files to an ftp server, and receiving text messages.

AT Commands: all prefixed by AT+

CMGL="ALL" - lists all messages currently stored (incoming and outgoing)

CPMS="SM","SM","SM" to select SMS storage

CMGR=0 - reads first message in the SMS storage

CMGF=1 - sets text mode

CMGS="+1number"<CR>
message goes here <ctrl-z>
^sends a text message

*/
#ifndef Ibelium_h
#define Ibelium_h

#include "Arduino.h"


class Ibelium
{
public:
	typedef struct {
		int index; //index of the sms in memory
		char* number; //sending number of the message
		char* message; //the "payload"
		char* dateTime; //the date/time string.
	} sms;
	
	Ibelium();
	int init();
	int sendSMS(char* number, char* message, char* response);
	int readSMS(sms* msg);
	int checkForSMS();

	
	
	//char* checkError(); Not implemented yet
private:	
	void switchModule();
	char* appendStrings(char* a, char* b);
	int sendATCmd(char* cmd);
	int sendATQuery(char* cmd, char* response);
	char* substring(char* str, int a, int b);

	static const int ON_MODULE_PIN =2;
	static const int LED = 13;
	int _error; //An error code
	int _watchdog; //A watchdog counter, for responses
};


/*
Error code mapping:
1 - Invalid or null response

2 - Response overflow

*/
#endif