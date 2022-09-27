#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

void Init_table(uint32_t poly) {
    char filename[30];
    sprintf(filename, "0x%x_table.txt", poly);
    ofstream outfile(filename, ios::out);
    uint32_t crc_t[256];
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t n = i << 24;
        uint32_t crc_reg = 0;
        for (int j = 0; j < 8; ++j) {
            if ((n ^ crc_reg) & 0x80000000) {
                crc_reg = (crc_reg << 1) ^ poly;
            }
            else {
                crc_reg <<= 1;
            }
            n <<= 1;
        }
        crc_t[i] = crc_reg;
    }
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 8; ++j) {
            outfile << "0x" << hex << crc_t[i * 8 + j] << ", ";
        }
        outfile << endl;
    }
    outfile.close();
}


int main() {
    // Init_table(0x04C11DB7);
    // Init_table(0xEDB88320);
    // Init_table(0xDB710641);
    // Init_table(0x82608EDB);
    // Init_table(0x741B8CD7);
    // Init_table(0xEB31D82E);
	Init_table(0x06D23852);
	// Init_table(0x22222222);
    
    return 0;
}
