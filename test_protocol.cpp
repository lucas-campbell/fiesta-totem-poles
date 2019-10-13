/*
 * File for testing packet making functions
 */

#include "protocol.h"
#include "utils.h"
#include <iostream>
#include <string>
using namespace std;

int main() {
    
    unsigned char hash[SHA1_LEN];
    computeChecksum("README.md", hash); //compute and fill hash
    printHash(reinterpret_cast<unsigned char *>(hash)); printf("\n");
    // create second checksum for testing purposes
    unsigned char hash2[SHA1_LEN];
    computeChecksum("sha1test.cpp", hash2);
    printHash(reinterpret_cast<unsigned char *>(hash2)); printf("\n");

    string dirpack = makeDirPilot(DirPilot(4, string(reinterpret_cast<const char *>(hash))));
    DirPilot dir_pilot = unpackDirPilot(dirpack);
    printf("DirPilot:\n%07d ", dir_pilot.num_files);
    printHash((const unsigned char *)dir_pilot.hash.c_str());
    printf("\n");

    string filepilotpack =
        makeFilePilot(FilePilot(1234567, 13, string(reinterpret_cast<const char *>(hash2)), "sha1test.cpp"));
    FilePilot file_pilot = unpackFilePilot(filepilotpack);
    printf("File Pilot:\n%07d %07d ", file_pilot.num_packets, file_pilot.file_ID);
    printHash((const unsigned char *)file_pilot.hash.c_str());
    printf(" %s\n", file_pilot.fname.c_str());

    string filedatapack =
        makeFilePacket(FilePacket(4, 10, "sfsgdfgsdfgdfjsfalfknkfwefEWAEFAAAWFAWFAWFAWFAWFWAWEFWAEFAFAFWFEFWRERGWERGWERGWEGEGAFWFQWKFBQWFBQWBQWQWBGKQWFBQWFFBJKFBAWFBAJFKBAFLKASJFBASKDJFBAKDFBASKLDBFAJKSBWKJBAKNERNVVNWEVNPEVNINWRUIBRJNBSLNBSDKFVJNDLFVDFKJSLDFNVNVUARIADDCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCASsdslkjsnvvaeesdjksndfgjklnfkdjnjksndbkldnfbsjkblndfkjnbsdbkjsdbsdbgjkdsfkljsdfkjladfdfssdfffffgfgafgadfglakjdfgnlfnagklanpp"));
    FilePacket file_packet = unpackFilePacket(filedatapack);
    printf("File Packet:\n %07d %07d %s\n", file_packet.packet_num,
             file_packet.file_ID, file_packet.data.c_str());
}
