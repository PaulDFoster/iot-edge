/*******************************************************************************
 * Copyright (c) 2012, 2013 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"

#define ADDRESS     "tcp://127.0.0.1"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "lora/+/up"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

struct macdeviceid {
	// mac address 23 bytes
	char macAddress[24];
	// device id up to 128
	char deviceId[128];
	// SAS key 32 bytes??? primary access key below is 45bytes :(
	char sasKey[45];
};

void doFileCreate()
{
	struct macdeviceid keynEncode;
	struct macdeviceid *storedVal = &keynEncode;

	//Note string termination char at end of macAddress and sasKey
	// Should be using strcpy_s for safety
	strcpy(keynEncode.macAddress, "00-80-00-00-00-00-C9-75");
	strcpy(keynEncode.deviceId, "mdotpaulfo                                                                                                                     ");
	strcpy(keynEncode.sasKey, "2RXYs/Ax1BE310/xrbNPGbhHin+tyRZE8HQ0CE4gzCE=");

	FILE *fp;
	// should be using fopen_s for safety
	fp = fopen("macdeviceid.dat", "w+b");
	fwrite(storedVal, sizeof(*storedVal), 1, fp);
	fclose(fp);
}

void doFileLoad()
{
	struct macdeviceid keynDecode;
	struct macdeviceid *readVal = &keynDecode;

	FILE *fp;
	// should be using fopen_s for safety
	fp = fopen("macdeviceid.dat", "rb");
	fread(readVal, sizeof(*readVal), 1, fp);
	fclose(fp);

	printf("\"");
	printf(keynDecode.deviceId);
	printf("\"\n");
	printf("\"");
	printf(keynDecode.macAddress);
	printf("\"\n");
	printf("\"");
	printf(keynDecode.sasKey);
	printf("\"\n");
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

	doFileLoad();
	printf("\n\n");

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    printf("Verison 1.3\n\n");

	doFileCreate();

	printf("File created\n\n");

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
