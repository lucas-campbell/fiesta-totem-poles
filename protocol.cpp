/*
 * protocol.cpp: Implements our network protocol functions
 * Written by: Dylan Hoffmann and Lucas Campbell
 */

#include <string>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include "protocol.h"
#include "sha1.h"

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
    //printHash((const unsigned char *)(pilot_packet.hash.c_str()));
    pack += " ";
    pack += pilot_packet.fname;
    return pack;
}

FilePilot unpackFilePilot(string packet)
{
    //cout << "unpacking from packet:\n" << packet << endl;
    // NEEDSWORK add error check for correct packet type

    // Get number of packets
    int num_packets = stoi(packet.substr(2, MAX_PACKNUM));

    // Get File ID
    int file_ID = stoi(packet.substr(10, MAX_FILENUM));
    
    // Get hash value
    string hash = packet.substr(18, SHA1_LEN-1);

    // Get file name
    string fname = packet.substr(39);

    return FilePilot(num_packets, file_ID, hash, fname);      

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
    //cout << pack;
    string num = to_string(pilot_packet.num_files);
    int zeros = MAX_FILENUM - num.length();
    pack += string(zeros, '0').append(num);
    //cout << string(zeros, '0').append(num) << " ";
    pack += " ";
    pack += pilot_packet.hash;
    //printHash((const unsigned char *)(pilot_packet.hash.c_str()));
    //cout << endl;
    return pack;
}

DirPilot unpackDirPilot(string packet)
{
    // Get number of files
    int num_files = stoi(packet.substr(2, MAX_FILENUM));

    // Get hash value for the directory
    string hash = packet.substr(10);

    return DirPilot(num_files, hash);
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
    //cout << pack << endl;
    return pack;
}

FilePacket unpackFilePacket(std::string packet)
{
    // Packet number
    int packet_num = stoi(packet.substr(2, MAX_PACKNUM));

    // File ID
    int file_ID = stoi(packet.substr(10, MAX_FILENUM));

    // Actual data
    string data  = packet.substr(18);

    return FilePacket(packet_num, file_ID, data);
}
