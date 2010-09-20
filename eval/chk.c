#include <stdio.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

void showhex(unsigned long x);

int
main()
{   
    unsigned long i = SIOCGIFADDR;

    printf("SIOCGIFADDR_dec = %lu\n", i);
    printf("SIOCGIFADDR_hex = 0x");
    showhex(i);
    printf("\n");
    
    return 0;
}

void showhex(unsigned long x)
{   
    int  r; 
    r = x % 16;
    x = x / 16;
    
    if (x > 0) showhex(x);

    if (r >= 0 && r <= 9) printf("%d", r);
    else if (r > 9) putchar('A' + r - 10);
}


