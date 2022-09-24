#pragma once
typedef struct
{
    int srcIP;
    int dstIP;
    short srcPort;
    short dstPort;
    char protocol;
}FIVE_TUPLE;