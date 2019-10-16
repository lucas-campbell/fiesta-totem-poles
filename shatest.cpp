#include <iostream>
#include <string>
#include "utils.h"

using namespace std;

int main(int argc, char *argv[]) {
    string files[] = {"sha1test.cpp", "utils.h", "Makefile"};
    for(int i = 0; i < 3; i++) {
        cout << files[i] << ": ";
        unsigned char hash[SHA1_LEN];
        size_t t;
        getFileChecksum(string("."), files[i], t, hash);
        for (int j = 0; j < 20; j++)
		{
			printf ("%02x", (unsigned int) hash[j]);
		}
        cout << endl;
    }
}
