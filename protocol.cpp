/*
 * protocol.cpp: Implements our network protocol functions
 * Written by: Dylan Hoffmann and Lucas Campbell
 */

#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include "protocol.h"

using namespace std;11

/*
 * Our UDP File Pilot packet is in the following format
 * "P ####### PPPPPPP HHHHHHHHHHHHHHHHHHHH FFFFFFF....."
 * Where:
 * F is the packet type indicator for a file Pilot packet
 * # is the number of packets for the file == (file-size // 420) +1
 * P is the file ID
 * H is the SHA1 hash of the file
 * F... is a variable length field for the file name (up to 480 bytes long)
 */
string makeFilePilot(int num_packets, int file_ID, string hash, string fname)
{
    string pack = "1 ";
    string num = to_string(num_packets);
    int zeros = MAX_PACKNUM - num.length();
    pack += string(zeros, '0').append(num);
    cout << pack << endl;
    pack += " ";
    num = to_string(file_ID);
    zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    pack += hash;
    pack += " ";
    pack += fname;
    return pack;
}


/*
 * Our UDP Directory Pilot packet is in the following format
 * "D ####### HHHHHHHHHHHHHHHHHHHH T..."
 * Where:
 * D is the packet type indicator for a Directory pilot packet
 * # is the number of files in the directory
 * H is the SHA1 hash of the directory
 * T... is a variable length field for the Target path (up to 465 bytes long)
 */
string makeDirPilot(int num_files, string hash, string TARGET)
{
    string pack = "0 ";
    string num = to_string(num_files);
    int zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    pack += hash;
    pack += " ";
    pack += TARGET;
    return pack;
}


/*
 * Our UDP File Data packet is in the following format
 * "F ####### PPPPPPP D....."
 * Where:
 * F is the packet type indicator for a File data packet
 * # is the packets number of this packet
 * P is the file ID
 * D... is a variable length field for the file data (up to 480 bytes long)
 */
string makeFilePacket(int packet_num, int file_ID, string data)
{
    string pack = "2 ";
    string num = to_string(packet_num);
    int zeros = MAX_PACKNUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    num = to_string(file_ID);
    zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    pack += data;
    return pack;
}
