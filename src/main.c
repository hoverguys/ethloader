#include <debug.h>
#include <errno.h>
#include <gccore.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sidestep.h"

#define DATA_PORT 8856
#define SD_PORT 8890
#define SD_MULTICAST_ADDR "234.1.9.14" // September 14, 2001 :P

#define TIMEOUT 2 // 2 seconds between probes

#define PROBE "BBA\1"
#define PROBELEN 4

static void* xfb = NULL;
static GXRModeObj* rmode;

void initialise();
bool setup_tcp_data();
bool setup_udp_sd();
void handlePayload();
void handleDiscovery();

// Network
s32 ssock,  // Server socket (data)
	csock,  // Client socket (data)
	sdsock; // Server socket (service discovery)
int ret;
u32 clientlen;
struct sockaddr_in client;
struct sockaddr_in server;
struct sockaddr_in broadcast;

char probetxt[PROBELEN + 16] = {'\0'};
int probelen = 0;

int main() {
	// Data
	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};

	// Initialize
	initialise();

	printf("\nethloader by zeroZshadow\n");

	// Check if BBA is present
	// Code from swiss-gc: https://github.com/emukidid/swiss-gc/blob/master/cube/swiss/source/exi.c
	u32 devid = 0;
	EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_2, &devid);
	if (devid != 0x04020200) {
		printf("\nBBA not found! Please make sure it's properly connected to the SP1 slot.\n");
		VIDEO_WaitVSync();
		return 1;
	}

	printf("Configuring network, please wait ...\n");

	// Configure the network interface
	ret = if_config(localip, netmask, gateway, TRUE, 30);
	if (ret < 0) {
		printf("network configuration failed!\n");
		VIDEO_WaitVSync();
		return 1;
	}
	printf("Network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);
	printf("Now listening on port %d\n", DATA_PORT);

	// Fill probe string
	strncat(probetxt, PROBE, PROBELEN);
	strncat(probetxt, localip, strlen(localip));
	probelen = PROBELEN + strlen(localip);

	// Load over network
	clientlen = sizeof(client);

	// Setup TCP socket (data channel)
	bool dataok = setup_tcp_data();
	if (!dataok) {
		printf("\nFailed to setup data channel.\n");
		// Data channel is required, fail if had issues opening it
		VIDEO_WaitVSync();
		return 1;
	}

	// Setup UDP socket (service discovery)
	bool sdok = setup_udp_sd();
	if (!sdok) {
		printf("\nFailed to setup service discovery, it will be disabled.\n");
	}

	fd_set sockets; // Just one, really..
	struct timeval timeout;
	while (1) {
		// Setup sockets
		FD_ZERO(&sockets);
		FD_SET(ssock, &sockets);

		// Reset timeout
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;

		// Check if the data channel has stuff to deal with
		ret = net_select(ssock + 1, &sockets, NULL, NULL, &timeout);
		if (ret < 0) {
			printf("Error checking sockets %ld!\n", csock);
			VIDEO_WaitVSync();
			break;
		}
		if (ret == 0) {
			// Timeout'd, send a discovery probe (if discovery is ok) and try again
			if (sdok) {
				handleDiscovery();
			}
			continue;
		}

		handlePayload();
	}

	return 0;
}

bool setup_tcp_data() {
	ssock = net_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (ssock == INVALID_SOCKET) {
		printf("DATA: Cannot create a socket!\n");
		VIDEO_WaitVSync();
		return 1;
	}

	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_port = htons(DATA_PORT);
	server.sin_addr.s_addr = INADDR_ANY;
	s32 ret = net_bind(ssock, (struct sockaddr*)&server, sizeof(server));

	if (ret) {
		printf("DATA: Error binding socket: %s (%ld)\n", strerror(ret), ret);
		return false;
	}

	return true;
}

bool setup_udp_sd() {
	sdsock = net_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sdsock == INVALID_SOCKET) {
		printf("DISCOVERY: Cannot create a socket!\n");
		return false;
	}

	broadcast.sin_family = AF_INET;
	broadcast.sin_port = htons(SD_PORT);
	broadcast.sin_addr.s_addr = inet_addr(SD_MULTICAST_ADDR);
	s32 ret = net_bind(sdsock, (struct sockaddr*)&broadcast, sizeof(broadcast));
	if (ret) {
		printf("DISCOVERY: Error binding socket: %s (%ld)\n", strerror(ret), ret);
		return false;
	}

	return true;
}

void handlePayload() {
	// Loader
	u8* payload;

	csock = net_accept(ssock, (struct sockaddr*)&client, &clientlen);

	if (csock < 0) {
		printf("DATA: Error connecting socket %ld!\n", csock);
		VIDEO_WaitVSync();
		return;
	}

	// Zero data length
	uint datalength, buffersize;
	memset(&datalength, 0, sizeof(datalength));
	memset(&buffersize, 0, sizeof(buffersize));

	// Read datalength
	s32 ret = net_recv(csock, &datalength, sizeof(datalength), 0);
	if (ret) {
		printf("DATA: Error receiving data length: %s (%ld)\n", strerror(ret), ret);
		return;
	}
	printf("Incoming file of %u bytes\n", datalength);

	// Read buffersize
	ret = net_recv(csock, &buffersize, sizeof(buffersize), 0);
	if (ret) {
		printf("DATA: Error receiving buffer size: %s (%ld)\n", strerror(ret), ret);
		return;
	}
	printf("Using buffer size of %u bytes\n", buffersize);

	// Create buffer to store payload in
	payload = memalign(32, datalength);

	printf("Downloading");
	VIDEO_WaitVSync();

	// Recieve payload
	u32 readtotal = 0;
	u32 ack = 0xdeadbeef;
	while (readtotal < datalength) {
		// Read a part of the file
		ret = net_read(csock, payload + readtotal, buffersize);
		if (ret > 0) {
			printf("DATA: Error receiving blob: %s (%ld)\n", strerror(ret), ret);
			return;
		}
		if (ret < 0) {
			printf("\nEOF\n");
			VIDEO_WaitVSync();
			break;
		}

		readtotal += ret;

		// Notify progress
		printf("\33[2K\r");
		printf("%d%%", (int)((readtotal / (float)datalength) * 100.0f));

		// Tell the server we got the packet
		net_write(csock, &ack, sizeof(ack));
	}
	printf("\nDownload complete\n");

	// Cleanup
	net_close(csock);
	DCFlushRange(payload, datalength);
	VIDEO_WaitVSync();

	printf("Starting DOL\n");

	// Run
	s32 res = DOLtoARAM(payload, 0, NULL);

	printf("Error %ld\n", res);
	VIDEO_WaitVSync();
}

void handleDiscovery() {
	s32 ret = net_sendto(sdsock, probetxt, probelen, 0, (struct sockaddr*)&broadcast, sizeof(broadcast));
	if (ret < 0) {
		printf("Error sending probe: %s (%ld)\n", strerror(-ret), -ret);
	}
}

void initialise() {
	// Audio
	AUDIO_Init(NULL);
	DSP_Init();
	AUDIO_StopDMA();
	AUDIO_RegisterDMACallback(NULL);

	// Input
	PAD_Init();

	// Video
	VIDEO_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE) {
		VIDEO_WaitVSync();
	}

	// Console
	CON_InitEx(rmode, 20, 30, rmode->fbWidth - 40, rmode->xfbHeight - 60);
}