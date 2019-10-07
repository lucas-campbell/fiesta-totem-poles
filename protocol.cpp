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

using namespace std;

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
string makeFilePilot(FilePilot pilot_packet)
{
    string pack = "1 ";
    string num = to_string(pilot_packet.num_packets);
    int zeros = MAX_PACKNUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    num = to_string(pilot_packet.file_ID);
    zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    pack += pilot_packet.hash;
    pack += " ";
    pack += pilot_packet.fname;
    cout << pack << endl;
    return pack;
}

FilePilot unpackFilePilot(string packet)
{
    FilePilot pilot;
    // NEEDSWORK add error check for correct packet type
    stringstream packet_s(packet);
    string intermediate;
    // get past packet type number
    getline(packet_s, intermediate, ' ');
    for (int i = 0; i < MAX_PACKNUM; index++, i++)
        curr_string += packet[index];
    pilot.num_packets = stoi(curr_string);
    index++; // skip past next space
        

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
string makeDirPilot(DirPilot pilot_packet)
{
    string pack = "0 ";
    string num = to_string(pilot_packet.num_files);
    int zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    pack += pilot_packet.hash;
    cout << pack << endl;
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
string makeFilePacket(FilePacket packet)
{
    string pack = "2 ";
    string num = to_string(packet.packet_num);
    int zeros = MAX_PACKNUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    num = to_string(packet.file_ID);
    zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    pack += " ";
    pack += packet.data;
    cout << pack << endl;
    return pack;
}
