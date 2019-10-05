/*
 * endtoend.cpp: Implements a function for directory checksum comparison
 * Written by: Dylan Hoffmann and Lucas Campbell
 */

#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <openssl/sha.h>
#include "sha1.h"

using namespace std;


void computeChecksum(string filename, unsigned char (&hash)[21])
{
    ifstream *t;
    stringstream *buffer;
    //unsigned char hash[21];
    t = new ifstream(filename);
    buffer = new stringstream;
    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(), (buffer->str()).length(),
         hash);
    hash[20] = '\0';
}


// int main(int argc, char *argv[])
// {
//   int i, j;
//   ifstream *t;
//   stringstream *buffer;

//   unsigned char obuf[20];

//   if (argc < 2) 	{
// 		fprintf (stderr, "usage: %s file [file...]\n", argv[0]);
// 		exit (1);
//   }

//   for (j = 1; j < argc; ++j) {
// 		printf ("SHA1 (\"%s\") = ", argv[j]);
// 		t = new ifstream(argv[j]);
// 		buffer = new stringstream;
// 		*buffer << t->rdbuf();
// 		SHA1((const unsigned char *)buffer->str().c_str(), 
// 		     (buffer->str()).length(), obuf);
// 		for (i = 0; i < 20; i++)
// 		{
// 			printf ("%02x", (unsigned int) obuf[i]);
// 		}
// 		printf ("\n");
// 		delete t;
// 		delete buffer;
//   }
//   return 0;
// }
