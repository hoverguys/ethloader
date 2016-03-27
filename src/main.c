#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>

#include "sidestep.h"

static void *xfb = NULL;
static GXRModeObj *rmode;

void initialise();

// Network
s32 sock, csock;
int ret;
u32	clientlen;
struct sockaddr_in client;
struct sockaddr_in server;

int main() {
	// Data
	char localip[16] = { 0 };
	char gateway[16] = { 0 };
	char netmask[16] = { 0 };

	// Loader
	u8* payload;

	// Initialize
	initialise();

	printf("\nethloader by zeroZshadow\n");
	printf("Configuring network, please wait ...\n");

	// Configure the network interface
	ret = if_config(localip, netmask, gateway, TRUE, 30);
	if (ret < 0) {
		printf("network configuration failed!\n");
		return 1;
	}
	printf("network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);

	// Load over network
	clientlen = sizeof(client);

	// Setup TCP socket
	sock = net_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == INVALID_SOCKET) {
		printf("Cannot create a socket!\n");
		return 1;
	}

	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_port = htons(8856);
	server.sin_addr.s_addr = INADDR_ANY;
	s32 ret = net_bind(sock, (struct sockaddr *) &server, sizeof(server));

	if (ret) {
		printf("Error %d binding socket!\n", ret);
		return 1;
	}

	if ((ret = net_listen(sock, 5))) {
		printf("Error %d listening!\n", ret);
		return 1;
	}

	while (1) {
		csock = net_accept(sock, (struct sockaddr *) &client, &clientlen);

		if (csock < 0) {
			printf("Error connecting socket %d!\n", csock);
			continue;
		}

		// Zero data length
		uint datalength, buffersize;
		memset(&datalength, 0, sizeof(datalength));
		memset(&buffersize, 0, sizeof(buffersize));

		// Read datalength
		ret = net_recv(csock, &datalength, sizeof(datalength), 0);
		printf("Incoming file of %u bytes\n", datalength);

		// Read buffersize
		ret = net_recv(csock, &buffersize, sizeof(buffersize), 0);
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
			if (ret < 0) {
				printf("\nEOF\n");
				VIDEO_WaitVSync();
				break;
			}

			readtotal += ret;

			// Notify progress
			printf("\33[2K\r");
			printf("%d%%", (int)((readtotal /(float)datalength) * 100.0f));

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

		printf("Error %d\n", res);
	}

	return 0;
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
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	// Console
	CON_InitEx(rmode, 20, 30, rmode->fbWidth - 40, rmode->xfbHeight - 60);
}