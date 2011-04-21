#ifndef __HEADER_HPP__
#define __HEADER_HPP__

#include <libcage/cage.hpp>

const char* trans_m_type(int m_type);
const char* trans_f_type(int f_type);

struct _short_header
{
    uint16_t f_type;
    uint16_t m_type;
    uint32_t descriptor;
};

struct _long_header
{
    uint16_t f_type;
    uint16_t m_type;
    uint32_t descriptor;
    char peer_addr[CAGE_ID_LEN];
    char own_addr[CAGE_ID_LEN];
};

#define F_RDP_CONNECT_T2B 1  
#define F_RDP_CONNECT_B2T 2  
#define F_RDP_LISTEN_T2B  4  
#define F_RDP_LISTEN_B2T  8  

#define M_RDP_DATA    1  
#define M_RDP_ACCEPT  2  
#define M_RDP_CONNECT 4  
#define M_RDP_CLOSED  8  
#define M_RDP_TIMEOUT 16 
#define M_RDP_ERROR   32 

const char* trans_f_type(int f_type)
{
    if (f_type == 1) {
        return "CONNECT_TopToBottom";
    }
    else if (f_type == 2) {
        return "CONNECT_BottomToTop";
    }
    else if (f_type == 4) {
        return "LISTEN_TopToBottom";
    }
    else if (f_type == 8) {
        return "LISTEN_BottomToTop";
    }
    else {
        return "undefined";
    }
}

const char* trans_m_type(int m_type)
{

    if (m_type == 1) {
        return "RDP_DATA";
    }
    else if (m_type == 2) {
        return "RDP_ACCEPT";
    }
    else if (m_type == 4) {
        return "RDP_CONNECT";
    }
    else if (m_type == 8) {
        return "RDP_CLOSE";
    } 
    else if (m_type == 16) {
        return "RDP_TIMEOUT";
    }
    else if (m_type == 32) {
        return "RDP_ERROR";
    }
    else {
        return "undefined";
    }
}

#endif
