#ifndef __PRISON_COMMON_H_
#define __PRISON_COMMON_H_

#define FW_PROGRAM_NAME     "prison"  // cant change
#define FW_ANY_ADDRESS      "0.0.0.0"  // cant change
#define FW_BIAS_RULE_NUMBER 50         // start ipfw rule number
#define FW_UPPER_LIMIT      50         // it can be make upper limit of ipfw filters
#define FW_RESOLVER_PORT    53         // resolve port number, default 53 , cant change
#define FW_NAME_DIVERT      10000
#define FW_ME2L_DIVERT      10001
#define FW_EXT2ME_DIVERT    10002

#define SIZE_IP4_HDR 20 // struct ip.ip_hl * 4
#define SIZE_IP6_HDR 40 // fixed length
#define SIZE_TCP_HDR 20 // struct tcphder.th_off * 4 
#define SIZE_UDP_HDR  8 // fixed length
#define SIZE_RECV_BUFFER  65535

#define PPFLAG_ZERO       0x00
#define PPFLAG_ME2L_FIN   0x01
#define PPFLAG_ME2L_ACK   0x02
#define PPFLAG_EXT2ME_FIN 0x04
#define PPFLAG_EXT2ME_ACK 0x08
//#define PPFLAG_hoge     0x10
//#define PPFLAG_hage     0x20
//#define PPFLAG_huge     0x40
#define PPFLAG_ERROR      0x80

#define INITIALLY_TIMEOUT (-60)
#define FW_IDLE_TIMEOUT   (-10)
#define PP_IDLE_TIMEOUT   (-4)
#define KVT_CAGE_TIMEOUT  (10)
#define KVT_CAGE_TTL 120

// reduce declaration
#define SA    struct sockaddr
#define SAIN  struct sockaddr_in
#define SAIN6 struct sockaddr_in6
#define SAST  struct sockaddr_storage

#endif
