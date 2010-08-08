#ifndef __RAPRINS_NAME_H_
#define __RAPRINS_NAME_H_

/*
 * Copyright (C) 2009, t-inoue@jaist.ac.jp
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * 0                16               31
 * +----------------+----------------+
 * |   identifier   |     flag       |
 * +----------------+----------------+
 * |   Ques Number  |   Ans Number   |
 * +----------------+----------------+
 * |    NS Number   |   Add Number   |
 * +----------------+----------------+
 * :                                 :
 * :    Question Resource Record     :
 * :                                 :
 * +----------------+----------------+
 * :                                 :
 * :     Answer Resource Record      :
 * :                                 :
 * +----------------+----------------+
 * :                                 :
 * :   NameServer Resource Record    :
 * :                                 :
 * +----------------+----------------+
 * :                                 :
 * :    Additonal Resource Record    :
 * :                                 :
 * +----------------+----------------+
 *
 *
 * identifier: You set Question == Reply.
 *
 * flags:
 * 0    1                5    6    7    8           11               15
 * +----+----------------+----+----+----+-----------+----------------+
 * | QR | Operation Code | AA | TC | RA |   Zero    |    Recode      |
 * +----+----------------+----+----+----+-----------+----------------+
 *
 * QR : Question or Reply 
 *  0 : Question
 *  1 : Reply
 *
 * Operation Code
 * 0000 : standard
 * 0001 : reverse request
 * 0011 : server status request
 *
 * AA : Authoritative Answer
 *  0 : no
 *  1 : yes
 *
 * TC : Truncated
 * (UDP segments over 512bytes, Only the first 512bytes of the reply)
 *  0 : no
 *  1 : yes
 *
 * RA : recursion available
 *  0 : no
 *  1 : yes
 *
 * Zero : There is a 3bits filling by Zero.
 *
 * Recode : return code 
 * 0000 : no error
 * 0011 : name error
 *
 *
 * Question Resource Record type=A
 * 0                16               31
 * +----------------+----------------+
 * |           Query Name            |
 * +----------------+----------------+
 * |   Query Type   | Protocol Class |
 * +----------------+----------------+
 * 
 * Query Type : 
 *  1 // A
 *  5 // CNAME
 * 12 // PTR
 * 13 // HINFO
 * 14 // MINFO
 * 15 // MX
 * 28 // AAAA (llmnr 0x1c -> 28 AAAA)
 * 55 // ANY (ALL TYPE REQUEST)
 *
 * Protocol Class :
 * 1 // internet
 *
 * Answer Resource Record
 * NameServer Resource Record
 * Additonal Resource Record
 * 0                16               31
 * +----------------+----------------+
 * :           Query Name            :
 * +----------------+----------------+
 * |   Query TYPE   | Protocol Class |
 * +----------------+----------------+
 * |          Time To Live           |
 * +----------------+----------------+
 * |  rdata_length  |                :
 * +----------------+                +
 * :           Resource Data         :
 * +----------------+----------------+
 *
 * Time To Live : client cashe time [sec]
 *
 * Resource Date length:
 * Query type    1 : 4byte
 *              12 : XX byte
 *              15 : 4byte
 *              28 : 16byte
 *
 */



#import <Cocoa/Cocoa.h>
#include "common.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>
#include <vector>
#include <string>

// uint16_t buf , u_char *pointer
#define N_READ16(buf, cp) {   \
    memcpy(&buf, cp, 2);      \
    for(size_t i=0; i<2; i++) \
    cp++;                     \
}

// uint32_t buf , u_char *pointer
#define N_READ32(buf, cp) {   \
    memcpy(&buf, cp, 4);      \
    for(size_t i=0; i<4; i++) \
    cp++;                     \
}

// uint16_t buf , u_char *pointer
#define N_WRITE16(buf, cp) {  \
    memcpy(cp, &buf, 2);      \
    for(size_t i=0; i<2; i++) \
    cp++;                     \
                              \
}

// uint32_t buf , u_char *pointer
#define N_WRITE32(buf, cp) {  \
    memcpy(cp, &buf, 4);      \
    for(size_t i=0; i<4; i++) \
    cp++;                     \
}

#define N_WRITE128(buf, cp) {  \
    memcpy(cp, &buf, 16);      \
    for(size_t i=0; i<16; i++) \
    cp++;                      \
}

#define DNS_MAX_LEN_SIZE 64
#define UDP_PACKET_SIZE 512

#define QR         0x8000 // if your packet is answer, you must check this flag.
#define OC_Reverse 0x0800 // i dont know.
#define OC_Status  0x1800 // i dont know.
#define AA         0x0400 // if you can assert this flag ,you must check with QR!
#define TC         0x0200 // when your packet data will be over 512, 
#define RD         0x0100 // you want to request recursing to nameserver.you check this flag.
#define RA         0x0080 // you want to request recursing to nameserver.you check this flag.
#define RE_Error   0x0003 // if you return to client error packet.you must check this flag.
//0x8000
//0x4000
//0x2000
//0x1000
//0x0800
//0x0400
//0x0200
//0x0100
//0x0080
//0x0040
//0x0020
//0x0010
//0x0008
//0x0004
//0x0002
//0x0001

#define n_qus_name  n_rr_qus.rr_name
#define n_qus_size  n_rr_qus.rr_size
#define n_qus_type  n_rr_qus.rr_type
#define n_qus_class n_rr_qus.rr_class

#define n_ans_name  n_rr_ans.rr_name
#define n_ans_size  n_rr_ans.rr_size
#define n_ans_type  n_rr_ans.rr_type
#define n_ans_class n_rr_ans.rr_class
#define n_ans_ttl   n_rr_ans.rr_ttl
#define n_ans_rlen  n_rr_ans.rr_rlen
#define n_ans_rdata n_rr_ans.rr_rdata
#define n_ans_raddr n_rr_ans.rr_sin_addr
#define n_ans_raddr6 n_rr_ans.rr_sin6_addr

#define n_name_name  n_rr_name.rr_name
#define n_name_size  n_rr_name.rr_size
#define n_name_type  n_rr_name.rr_type
#define n_name_class n_rr_name.rr_class
#define n_name_ttl   n_rr_name.rr_ttl
#define n_name_rlen  n_rr_name.rr_rlen
#define n_name_rdata n_rr_name.rr_rdata
#define n_name_raddr n_rr_name.rr_sin_addr
#define n_name_raddr6 n_rr_name.rr_sin6_addr

#define n_add_name  n_rr_add.rr_name
#define n_add_size  n_rr_add.rr_size
#define n_add_type  n_rr_add.rr_type
#define n_add_class n_rr_add.rr_class
#define n_add_ttl   n_rr_add.rr_ttl
#define n_add_rlen  n_rr_add.rr_rlen
#define n_add_rdata n_rr_add.rr_rdata
#define n_add_raddr n_rr_add.rr_sin_addr
#define n_add_raddr6 n_rr_add.rr_sin6_addr


// local structure
struct _rr_pkt {
    char rr_name[DNS_MAX_LEN_SIZE];
    size_t rr_size;
    uint16_t rr_type;
    uint16_t rr_class;
    uint32_t rr_ttl;
    uint16_t rr_rlen;
    union {
        char rr_rdata[DNS_MAX_LEN_SIZE];
        union {
            struct in_addr rr_sin_addr;
            struct in6_addr rr_sin6_addr;
        };
    };
};


@interface NamePacket : NSObject {
/*
@protected
@public
@private
*/
    // class member (for class information)
    u_char* count_pointer;
    u_char n_payload_buf[UDP_PACKET_SIZE];
    size_t compress_size;
    size_t n_size_payload;

    // class member (for packet header)
    uint16_t n_id;    // identifier
    uint16_t n_flags; // flags

    // resource recode numver
    uint16_t n_qus;   // question number
    uint16_t n_ans;   // answer number 
    uint16_t n_name;  // nameserver number
    uint16_t n_add;   // additional number

    struct _rr_pkt n_rr_qus; // resource recode question
    struct _rr_pkt n_rr_ans; // resource recode answer
    struct _rr_pkt n_rr_name; // resource recode nameserver
    struct _rr_pkt n_rr_add; // resource recode addional

}

- (id)init;
- (void)dealloc;


- (void)n_set_id:(int)i;
- (void)n_reset_flags;
- (void)n_set_flags:(uint16_t)i;
- (void)n_build_payload;
- (u_char*)n_payload;
- (size_t)n_payload_size;
//- (bool)n_create_rr_questionA:(std::string)s;
- (bool)n_create_rr_questionA:(NSString*)s;
//- (bool)n_create_rr_questionAAAA:(std::string)s;
- (bool)n_create_rr_questionAAAA:(NSString*)s;
//- (bool)n_create_rr_answer:(std::string)s;
- (bool)n_create_rr_answer:(NSString*)s;
//- (bool)n_create_rr_nameserver:(std::string)s;
//- (bool)n_create_rr_additional:(std::string)s;

// internal function
- (void) _n_format;
- (size_t) _n_compress:(const char*)src :(size_t)src_size :(char*)dst :(size_t)dst_size;

@end //@interface NamePacket

@implementation NamePacket

- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        [self _n_format];
    }
    return self;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    [super dealloc];
    return;
}

- (void) _n_format {
    // public initialize
    n_id = 0;
    n_flags = 0;
    n_qus = 0;
    n_ans = 0;
    n_name = 0;
    n_add = 0;
    memset(&n_rr_qus, 0, sizeof(n_rr_qus));
    memset(&n_rr_ans, 0, sizeof(n_rr_ans));
    memset(&n_rr_name, 0, sizeof(n_rr_name));
    memset(&n_rr_add, 0, sizeof(n_rr_add));

    // private initialize
    n_size_payload = 0;
    memset(n_payload_buf, 0, sizeof(n_payload_buf));
    count_pointer = n_payload_buf;
    return;
}

- (void)n_set_id:(int)i {
    n_id = i;  
    return;
}

- (void)n_set_flags:(uint16_t)i {
    n_flags |= i; 
    //n_flags = (n_flags | i);
    //printf("%d\n", n_flags);
    return;
}

- (void)n_reset_flags {
    n_flags = 0; 
    return;
}

- (bool)n_create_rr_questionA:(NSString*)s {
    // DNS HEADER PART

    // DNS Question Record
    n_qus++;
    n_qus_size  = [self _n_compress:[s UTF8String] :(size_t)[s length] :n_qus_name :sizeof(n_qus_name)];
    n_qus_type  = 1;
    n_qus_class = 1;
    return true;
}

/*
- (bool)n_create_rr_questionA:(std::string)s {
    // DNS HEADER PART

    // DNS Question Record
    n_qus++;
    //memcpy(npkt.n_rr_qus.rr_name, buf, compress_size);
    //memcpy(npkt.n_qus_name, buf, compress_size);
    //npkt.n_qus_size  = compress_size;
    //n_qus_size  = n_compress(s.data(), (size_t)s.size(), n_qus_name, sizeof(n_qus_name));
    n_qus_size  = [self _n_compress:s.data() :(size_t)s.size() :n_qus_name :sizeof(n_qus_name)];
    n_qus_type  = 1;
    n_qus_class = 1;
    return true;
}
*/

- (bool)n_create_rr_questionAAAA:(NSString*)s {
    // DNS HEADER PART

    // DNS Question Record
    n_qus++;
    n_qus_size  = [self _n_compress:[s UTF8String] :(size_t)[s length] :n_qus_name :sizeof(n_qus_name)];
    n_qus_type  = 28;
    n_qus_class = 1;
    return true;
}

/*
- (bool)n_create_rr_questionAAAA:(std::string)s {
    // DNS HEADER PART

    // DNS Question Record
    n_qus++;
    //memcpy(npkt.n_rr_qus.rr_name, buf, compress_size);
    //memcpy(npkt.n_qus_name, buf, compress_size);
    //npkt.n_qus_size  = compress_size;
    //n_qus_size  = n_compress(s.data(), (size_t)s.size(), n_qus_name, sizeof(n_qus_name));
    n_qus_size  = [self _n_compress:s.data() :(size_t)s.size() :n_qus_name :sizeof(n_qus_name)];
    n_qus_type  = 28;
    n_qus_class = 1;
    return true;
}
*/

- (bool)n_create_rr_answer:(NSString*)s {

    n_ans++;
    //npkt.n_ans_name[0] = 0xC0;
    //npkt.n_ans_name[1] = 0x0C;
    //n_ans_size = 2;

    n_ans_size = n_qus_size;
    memcpy(n_ans_name, n_qus_name, n_qus_size);

    //n_ans_type = 1;
    n_ans_type = n_qus_type;
    n_ans_class= 1;
    n_ans_ttl = 10000;

    if ( n_ans_type == 1) {
        // input IPv4 address
        //cout << "rr_answer ipv4" << endl;
        n_ans_rlen = 4;
        //inet_pton(AF_INET, s.data(), &n_ans_raddr);
        //printf("test:%s\n", [s UTF8String])
        inet_pton(AF_INET, [s UTF8String], &n_ans_raddr);
    } else if ( n_ans_type == 28) {
        // input IPv6 address
        n_ans_rlen = 16;
        //inet_pton(AF_INET6, s.data(), &n_ans_raddr6);
        inet_pton(AF_INET6, [s UTF8String], &n_ans_raddr6);
    } else {
        // input compress binet name
        //n_ans_rlen = s.size();
        n_ans_rlen = [s length];
        //printf("test:[s UTF8String]\n");
        memcpy(n_ans_rdata , [s UTF8String] , n_ans_rlen);
    }
    return true;
}

/*
- (bool)n_create_rr_answer:(std::string)s {

    n_ans++;
    //npkt.n_ans_name[0] = 0xC0;
    //npkt.n_ans_name[1] = 0x0C;
    //n_ans_size = 2;

    n_ans_size = n_qus_size;
    memcpy(n_ans_name, n_qus_name, n_qus_size);

    //n_ans_type = 1;
    n_ans_type = n_qus_type;
    n_ans_class= 1;
    n_ans_ttl = 10000;

    if ( n_ans_type == 1) {
        // input IPv4 address
        //cout << "rr_answer ipv4" << endl;
        n_ans_rlen = 4;
        inet_pton(AF_INET, s.data(), &n_ans_raddr);
    } else if ( n_ans_type == 28) {
        // input IPv6 address
        n_ans_rlen = 16;
        inet_pton(AF_INET6, s.data(), &n_ans_raddr6);
    } else {
        // input compress binet name
        n_ans_rlen = s.size();
        memcpy(n_ans_rdata , s.data() , n_ans_rlen);
    }
    return true;
}
*/

/*
// non implementation

bool name_pkt::n_create_rr_nameserver(string &s) {
//n_name++;
return false;
};

bool name_pkt::n_create_rr_additional(string &s) {
//n_add++;
return false;
};
*/

- (void)n_build_payload {

    uint16_t n_id_nw_s    = htons(n_id);
    uint16_t n_flags_nw_s = htons(n_flags);
    uint16_t n_qus_nw_s   = htons(n_qus);
    uint16_t n_ans_nw_s   = htons(n_ans);
    uint16_t n_name_nw_s  = htons(n_name);
    uint16_t n_add_nw_s   = htons(n_add);

    N_WRITE16(n_id_nw_s, count_pointer);
    N_WRITE16(n_flags_nw_s, count_pointer);
    N_WRITE16(n_qus_nw_s, count_pointer);
    N_WRITE16(n_ans_nw_s, count_pointer);
    N_WRITE16(n_name_nw_s, count_pointer);
    N_WRITE16(n_add_nw_s, count_pointer);

    if (n_qus > 0) {
        memcpy(count_pointer, n_qus_name, (int)n_qus_size);
        for(size_t i=0; i<n_qus_size; i++) count_pointer++;

        uint16_t n_qus_type_nw_s = htons(n_qus_type);
        N_WRITE16(n_qus_type_nw_s, count_pointer);

        uint16_t n_qus_class_nw_s = htons(n_qus_class);
        N_WRITE16(n_qus_class_nw_s, count_pointer);
    }

    if (n_ans > 0) {
        memcpy(count_pointer, n_ans_name, (int)n_ans_size);
        for(size_t i=0; i<n_qus_size; i++) count_pointer++;

        uint16_t n_ans_type_nw_s = htons(n_ans_type);
        N_WRITE16(n_ans_type_nw_s, count_pointer);

        uint16_t n_ans_class_nw_s = htons(n_ans_class);
        N_WRITE16(n_ans_class_nw_s, count_pointer);

        uint32_t n_ans_ttl_nw_s = htonl(n_ans_ttl);
        N_WRITE32(n_ans_ttl_nw_s, count_pointer);

        uint16_t n_ans_rlen_nw_s = htons(n_ans_rlen);
        N_WRITE16(n_ans_rlen_nw_s, count_pointer);

        if (n_ans_type == 1) {
            N_WRITE32(n_ans_raddr.s_addr, count_pointer);
        } else if (n_ans_type == 28) { 
            // IPv6 name type response
            N_WRITE128(n_ans_raddr6.__u6_addr, count_pointer);
        } else {
            memcpy(count_pointer, n_ans_rdata, (int)n_ans_rlen);
            for(size_t i=0; i<n_qus_size; i++) count_pointer++;
        }
    }

    if (n_name > 0) {
        memcpy(count_pointer, n_name_name, (int)n_name_size);
        for(size_t i=0; i<n_qus_size; i++) count_pointer++;

        uint16_t n_name_type_nw_s = htons(n_name_type);
        N_WRITE16(n_name_type_nw_s, count_pointer);

        uint16_t n_name_class_nw_s = htons(n_name_class);
        N_WRITE16(n_name_class_nw_s, count_pointer);

        uint32_t n_name_ttl_nw_s = htonl(n_name_ttl);
        N_WRITE32(n_name_ttl_nw_s, count_pointer);

        uint16_t n_name_rlen_nw_s = htons(n_name_rlen);
        N_WRITE16(n_name_rlen_nw_s, count_pointer);

        if (n_name_type == 1) {
            N_WRITE32(n_name_raddr.s_addr, count_pointer);
        } else if (n_name_type == 28) { 
            // IPv6 name type response
            N_WRITE128(n_ans_raddr6.__u6_addr, count_pointer);
        } else {
            memcpy(count_pointer, n_name_rdata, (int)n_name_rlen);
            for(size_t i=0; i<n_qus_size; i++) count_pointer++;
        }
    }

    if (n_add > 0) {
        memcpy(count_pointer, n_add_name, (int)n_add_size);
        for(size_t i=0; i<n_qus_size; i++) count_pointer++;

        uint16_t n_add_type_nw_s = htons(n_add_type);
        N_WRITE16(n_add_type_nw_s, count_pointer);

        uint16_t n_add_class_nw_s = htons(n_add_class);
        N_WRITE16(n_add_class_nw_s, count_pointer);

        uint32_t n_add_ttl_nw_s = htonl(n_add_ttl);
        N_WRITE32(n_add_ttl_nw_s, count_pointer);

        uint16_t n_add_rlen_nw_s = htons(n_add_rlen);
        N_WRITE16(n_add_rlen_nw_s, count_pointer);

        if (n_add_type == 1) {
            N_WRITE32(n_add_raddr.s_addr, count_pointer);
        } else if (n_add_type == 28) {
            // IPv6 name type response
            N_WRITE128(n_ans_raddr6.__u6_addr, count_pointer);
        } else {
            memcpy(count_pointer, n_add_rdata, (int)n_add_rlen);
            for (size_t i=0; i<n_qus_size; i++) count_pointer++;
        }
    }

    n_size_payload = (size_t)(count_pointer - n_payload_buf);
    //printf("base  : %p\n", n_payload_buf);
    //printf("shift : %p\n", count_pointer);
    //printf("byte  : %d\n", n_size_payload);

    return;
};

- (u_char*)n_payload {
    return n_payload_buf;
}

- (size_t)n_payload_size {
    return n_size_payload;
}

// internal function method
// c++ language coding
- (size_t) _n_compress:(const char*)src :(size_t)src_size :(char*)dst :(size_t)dst_size {

    if (src_size == 0) return 0;

    std::string::size_type i;
    std::string::size_type j;
    std::vector<std::string> v;
    std::string s(src, src_size);

    while (s[s.size()-1] == '\0') {
        s = s.substr(0,s.size()-1);
    }

    v.clear();

    i = 0;
    j = s.find(".");

    while(j != std::string::npos){
        if (i == 0) v.push_back(s.substr(i,j-i));
        if (i != 0) v.push_back(s.substr(i+1,j-i-1));
        i = j++;
        j = s.find(".", j);
        if (j == std::string::npos){
            v.push_back(s.substr(i+1, s.size()));
            break;
        }
    }

    std::string compress_s;
    for (unsigned int i=0; i< v.size(); i++) {
        compress_s += (char)v[i].size();
        compress_s += v[i];
    }
    compress_s += (char)0;

    /*
      // check compress name
      for (unsigned int i=0; i< compress_s.size(); i++) {
      char a = compress_s.at(i);
      printf("%3x",a);
      printf("  ",a);
      printf("%1c ",a);
      printf("\n");
      }
     */
    if (dst_size < compress_s.size()) return 0;
    memcpy(dst, compress_s.data(), compress_s.size());
    return (size_t)compress_s.size();
}

@end //@implementation NamePacket

#endif
