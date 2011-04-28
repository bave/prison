#include <iostream>
#include <iomanip>
#include <vector>
#include "../base64.hpp"


int main()
{
    std::vector<uint8_t> v1, v2;

    /*
    v1.push_back(0xde);
    v1.push_back(0xad);
    v1.push_back(0xbe);
    v1.push_back(0xef);
    */

    uint8_t a[] = {0xde, 0xad, 0xbe, 0xef};
    v1.assign(a,a+sizeof(a));

    std::string s;

    base64::encode(v1, s);
    std::cout << s << std::endl; // 3q2+7w==

    base64::decode(s, v2);

    std::cout << v2.size() << std::endl;

    std::vector<uint8_t>::const_iterator it;
    for (it = v2.begin(); it != v2.end(); ++it) {
        std::cout << std::hex << static_cast<int>(*it);
    } // deadbeef
    std::cout << std::endl;

    uint8_t* box = (uint8_t*)malloc(v2.size());
    memset(box, 0, v2.size());

    for (it = v2.begin(); it != v2.end(); ++it) {
        *box = *it;
        std::cout << std::hex << static_cast<int>(*box) << std::endl;
        box++;
    }
    return 0;
}
