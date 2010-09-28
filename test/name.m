#include "../name.h"

void usage(void);

int main(int argc, char *argv[])
{
    id pool = [[NSAutoreleasePool alloc] init];
    //id name=[[NamePacket alloc] init];
    id name=[NamePacket new];

	//std::string rr("aris.jpcert.cc");
	//std::string addr("192.50.109.27");

	if (argc != 3) usage();

	[name n_set_id:12345];
	[name n_set_flags:QR|AA|RA];
	[name n_create_rr_questionA:[NSString stringWithString:@"aris.jaist.ac.jp"]];
	[name n_create_rr_answer:[NSString stringWithString:@"150.65.32.64"]];
	[name n_build_payload];

	int sockfd;
	struct sockaddr_in servaddr;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	sendto(sockfd, [name n_payload], [name n_payload_size], 0, (struct sockaddr *)&servaddr,sizeof(servaddr));
    [pool drain];

	return 0;
}

void usage(void){
	std::cout << "name [dst_IP_addr] [dst_udp_port]" << std::endl;
	exit(-1);
}
