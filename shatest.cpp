#include <iostream>
#include <string>
#include "sha1.h"

using namespace std;

int main(int argc, char *argv[]) {
    string files[] = {"sha1test.cpp", "sha1.cpp", "Makefile"};
    for(int i = 0; i < 3; i++) {
        cout << files[i] << ": ";
        unsigned char hash[SHA1_LEN];
        computeChecksum(files[i], hash);
        for (int j = 0; j < 20; j++)
		{
			printf ("%02x", (unsigned int) hash[j]);
		}
        cout << endl;
    }
}
