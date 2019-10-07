/*
 * protocol.h: Interface for UDP file copy protocol
 * Written By Dylan Hoffmann & Lucas Campbell
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

const int MAX_FILENUM = 7; // number of digits allowed for number of files
const int MAX_PACKNUM = 7; // number of digits allowed for number of packets


/*
 * makeFilePilot
 * Constructs the pilot packet for files
 * Args:
 * * int num_packets: # packets for this file = file_size / PACKET_DATA_SIZE
 * * string fname: name of the file
 * * int file_ID: numerical id of this file (incrememntal)
 * * string hash: SHA1 hash of file contents
 * Return: string - packet with metadata packed into sting
 * Assumptions: TODO
 */  
struct FilePilot {
    int num_packets;
    int file_ID;
    std::string hash;
    std::string fname;
    FilePilot(int p, int i, std::string h, std::string f) :
        num_packets(p), file_ID(i), hash(h), fname(f) {}
};
std::string makeFilePilot(FilePilot pilot_packet);

/*
 * makeDirPilot
 * Constructs the pilot packet for directories
 * Args:
 * * int num_files: # files in this directory
 * * string hash: SHA1 hash of the directory
 * * string target: name of copy target directory
 * Return: string - packet with metadata packed into sting
 * Assumptions: TODO
 */  
struct DirPilot {
    int num_files;
    std::string hash;
    std::string TARGET;
    DirPilot(int n, std::string h, std::string T) :
        num_files(n), hash(h), TARGET(T) {}
};
std::string makeDirPilot(DirPilot pilot_packet);

/*
 * makeFilePacket
 * Constructs the data packet for files
 * Args:
 * * int packet_num: which packet in order this packet is TODO: fix this <--
 * * int file_ID: numerical id of this file (incrememntal)
 * * string data: file data payload -- 420 bytes except final packet
 * Return: string - packet with metadata packed into sting
 * Assumptions: TODO
 * Additional info: the data payload is always 420 bytes except for the
 * final packet for a file, which may be shorter
 */  
struct FilePacket {
    int packet_num;
    int file_ID;
    std::string data;
    FilePacket(int p, int f, std::string d) :
        packet_num(p), file_ID(f), data(d) {}
};
std::string makeFilePacket(FilePacket packet);

#endif
