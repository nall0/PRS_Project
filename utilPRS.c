#include "utilPRS.h"

void handleError(int var, char *functionName) {
	if (var < 0) {
		perror("bind");
		printf("error in function %s\nExit\n",functionName);
		exit(-1);
	}
}

int initSocket(int* pValid, struct sockaddr_in* pAdresse, int port) {
	int desc; //file descripteur for the socket
	desc = socket(AF_INET, SOCK_DGRAM,0); //SOCK_DGRAM indicate that we use UDP
	handleError(desc,"socket");

	//Handling some options on socket
	int sockOption = setsockopt(desc, SOL_SOCKET, SO_REUSEADDR, pValid, sizeof(int));
	handleError(sockOption, "setsockopt");

	//Associating struc adresse with the port
	pAdresse->sin_family= AF_INET;
	pAdresse->sin_port= htons(port);
	pAdresse->sin_addr.s_addr= htonl(INADDR_ANY);

	//associating  the socket with the adresse
	int bindRes = bind(desc, (struct sockaddr*) pAdresse, sizeof(*pAdresse));
	handleError(bindRes, "bind");

	return desc;
}

void handshake(int desc, struct sockaddr* pAddr, socklen_t *pSizeAddr, int privatePort) {
	char SYN_ACK[12];

	//reception de SYN
	memset(SYN_ACK,0,strlen(SYN_ACK));
	recvfrom(desc,SYN_ACK,sizeof(SYN_ACK),0, pAddr,pSizeAddr);
	if (strcmp(SYN_ACK , "SYN") != 0 ) {
		printf("il faut envyer SYN au début d'une connexion\n recu : %s\n", SYN_ACK);
		exit(1);
	}

	//envoi de SYN_ACK0000
	memset(SYN_ACK,0,strlen(SYN_ACK));
	sprintf(SYN_ACK, "SYN-ACK");
	char charPrivatePort[6];
	sprintf(charPrivatePort, "%d", privatePort); //convertion du port privé en string
	int i=7;	
	for(i=7; i<11; i++) {
		SYN_ACK[i] = charPrivatePort[i-7];
	}
	sendto(desc,SYN_ACK,sizeof(SYN_ACK),0, pAddr, *pSizeAddr);

	//reception de ACK
	memset(SYN_ACK,0,strlen(SYN_ACK));
	recvfrom(desc,SYN_ACK,sizeof(SYN_ACK),0, pAddr,pSizeAddr);
	if (strcmp(SYN_ACK , "ACK") != 0 ) {
		printf("il faut envyer ACK au début d'une connexion\n");
		exit(1);
	}
}

void ajoutDebut(char msg[], char read[], int seqNum) {
	char charSeqNum[7];
	sprintf(charSeqNum, "%d", seqNum); //convertion du numéro de sequence en string
	strcpy(msg,"000000");
	int i=0, j=6;

	//ajout du numero de sequence au début de msg
	for(i=0; i<(int)strlen(charSeqNum); i++) {
		msg[i+(6-(int)strlen(charSeqNum))] = charSeqNum[i];
	}
	//on ajoute ensuite le contenu lu dans le fichier
	for(j=6; j<SEGSIZE; j++) {
		msg[j] = read[j-6];
	}
}

void sendFile(char *fileName, int descUtil,struct sockaddr* pUtil, socklen_t sizeUtilAddr) {	
	char read[SEGSIZE-6]; //contenu lu dans le fichier
	char msg[SEGSIZE]; //message envoyé, avec le numéro de sequence au debut
	int seqNum = 0;
	int sndto;
	int readSize;
	FILE *f1;
	f1 = fopen(fileName,"rb");

	while(feof(f1) == FALSE) { 
		readSize = fread(read, sizeof(char), SEGSIZE-1, f1);
		ajoutDebut(msg, read, ++seqNum);
		printf("envoi du segment %d : \n%s\n\n",seqNum, msg);
		sndto = sendto(descUtil,msg,readSize+6,0,pUtil, sizeUtilAddr);
		handleError(sndto, "sendto");
		memset(read,0,SEGSIZE-6);
		memset(msg,0,SEGSIZE);
	}

	fclose(f1);
	sendto(descUtil,"FIN",sizeof("FIN"),0,pUtil, sizeUtilAddr);
}