/*******************************************************************************
 * Copyright (c) 2012, 2022 IBM Corp., Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

// 定义需要使用到的MQTT连接的宏，如broker地址和客户端ID等
#define ADDRESS     "tcp://broker.emqx.io:1883"
#define CLIENTID    "PahoClientSub"
#define TOPIC       "nanomq/test"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

// 定义在主线程中的逻辑Flag
int disc_finished = 0;
int subscribed = 0;
int finished = 0;

//预先声明 API 回调函数
void onConnect(void* context, MQTTAsync_successData* response);
void onConnectFailure(void* context, MQTTAsync_failureData* response);
void onSubscribe(void* context, MQTTAsync_successData* response);
void onSubscribeFailure(void* context, MQTTAsync_failureData* response);

// 以下2个是 Async 使用的回调方法
// 异步连接成功的回调函数，在连接成功的地方进行Subscribe操作。
void conn_established(void *context, char *cause)
{
	printf("client reconnected!\n");
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
}

// 异步连接收到 Disconnect包时的回调，由于大部分断连的情况并不会收到 Disconnect包，所以此方法很少会被触发
void disconnect_lost(void* context, MQTTProperties* properties,
		enum MQTTReasonCodes reasonCode)
{
	printf("client disconnected!\n");
}

// 以下为客户端全局回调函数，分别是连接断开和消息到达
void conn_lost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.maxRetryInterval = 16;
	conn_opts.minRetryInterval = 2;
	conn_opts.automaticReconnect = 1;
	
	//conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	MQTTAsync_setConnected(client, client, conn_established);
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}
}

// 收到消息时的全局回调方法，此处就简单的打印消息
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

// 成功发送消息的确认回调
void msgdeliverd(void *context, MQTTAsync_token token)
{
	printf("msg sent");
}

// 以下是 API 层面的回调方法
void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed, rc %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response->code);
}

void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
}


int main(int argc, char* argv[])
{
    // 创建异步连接客户端需要使用的属性结构体
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	int rc;
	int ch;
    // 创建异步连接客户端，不使用 Paho SDK 内置的持久化来处理缓存消息
	if ((rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL))
			!= MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}
    // 设置异步连接回调，注意此处设置的回调函数为连接层面的全局回调函数
    // connlost 为连接断开触发，有且只有连接成功后断开才会触发，在断开连接的情况下进行重连失败不触发。
    // msgarrvd 收到消息时触发的回调函数
    // msgdeliverd 是消息成功发送的回调函数，一般设置为NULL
	if ((rc = MQTTAsync_setCallbacks(client, client, conn_lost, msgarrvd, msgdeliverd)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callbacks, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}
    //设置连接参数
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	// 此处设置 API调用失败会触发的回调，接下来进行connect操作所以设置为 onConnectFailure 方法
	conn_opts.onFailure = onConnectFailure;
	// 此处设置 客户端连接API调用成功会触发的回调，由于例程使用异步连接的 API，设置了会导致2个回调都被触发，所以建议不使用此回调
	//conn_opts.onSuccess = onConnect;
	// 初次连接失败不会触发自动重连
	conn_opts.automaticReconnect = 1;
	//开启自动重连，并且设置 2-16s 的随机退避时间
	conn_opts.maxRetryInterval = 10;
	conn_opts.minRetryInterval = 1;
	conn_opts.context = client;
	// 设置异步回调函数，此与之前的 API 回调不同，每次连接/断开都会触发
	MQTTAsync_setConnected(client, client, conn_established);
	MQTTAsync_setDisconnected(client, client, disconnect_lost);
    // 启动客户端连接，之前设置的 API 回调只会在这一次操作生效
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	while (!subscribed && !finished)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	if (finished)
		goto exit;

	do 
	{
		ch = getchar();
	} while (ch!='Q' && ch != 'q');
    // 收到命令行输入 Q 字母时断开连接，重新设置 API 回调函数。
	disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;

	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}
 	while (!disc_finished)
 	{
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
 	}

destroy_exit:
	MQTTAsync_destroy(&client);
exit:
	printf("rc 0");
 	return rc;
}
