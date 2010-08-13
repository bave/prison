
#ifndef __DSLIBINFOMIG_TYPES_H_
#define	__DSLIBINFOMIG_TYPES_H_

#ifndef kDSStdMachDSLookupPortName
#define kDSStdMachDSLookupPortName	"com.apple.system.DirectoryService.libinfo_v1"
#endif

#ifndef MAX_MIG_INLINE_DATA
#define MAX_MIG_INLINE_DATA 16384
#endif

typedef char* inline_data_t;
typedef char* proc_name_t;

struct sLibinfoRequest
{
    mach_port_t         fReplyPort;
    int32_t             fProcedure;
    char                *fBuffer;
    int32_t             fBufferLen;
    mach_vm_address_t   fCallbackAddr;
    audit_token_t       fToken;
};

#endif
