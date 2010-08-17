#ifndef __PRISON_DS_H_
#define __PRISON_DS_H_

#import <servers/bootstrap.h>
#include <string.h>
#include <stdint.h>
#include "./ds/DSlibinfoMIG.h"
#include "./ds/DSlibinfoMIG_types.h"

#define PortName  "com.apple.system.DirectoryService.libinfo_v1"
#define MaxMigInLineData 16384

bool flushcache(void);

bool flushcache(void)
{
    char                    reply[MaxMigInLineData];
    int32_t                 procno          = 0;
    vm_offset_t             ooreply         = 0;
    mach_msg_type_number_t  replyCnt        = 0;
    mach_msg_type_number_t  ooreplyCnt      = 0;
    security_token_t        userToken;
    mach_port_t             serverPort;

    memset(reply, 0, sizeof(reply));

    int ret1;
    ret1 = bootstrap_look_up(bootstrap_port, PortName, &serverPort);
    if (ret1 == 0) {
        int ret2;
        ret2 = libinfoDSmig_GetProcedureNumber(serverPort, (char*)"_flushcache", &procno, &userToken);
        if (ret2 == 0) {    

            libinfoDSmig_Query(
                serverPort,
                procno,
                (char*)"",
                0,
                reply,
                &replyCnt,
                &ooreply,
                &ooreplyCnt,
                &userToken
            );
            return true;
        }
    }
    return false;
}

#endif
