#pragma once

inline void XORCrypt(char* data, int len, char key) {
    for(int i=0; i<len; i++){
        data[i]^=key;
    }
}
