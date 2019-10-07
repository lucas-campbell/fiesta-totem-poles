/*
 * File for testing packet making functions
 */

#include "protocol.h"
#include <iostream>
#include <string>
using namespace std;

int main() {
    string dirpack = makeDirPilot(4, "c/p/ath/to/file");
    string filepilotpack =
        makeFilePilot(4, 10, "92a1dfec849ad2ebdb3819b689cf700be2d592bb",
                      "sha1test.cpp");
     string filedatapack =
        makeFilePacket(4, 10, "sfsgdfgsdfgdfjsfalfknkfwefEWAEFAAAWFAWFAWFAWFAWFWAWEFWAEFAFAFWFEFWRERGWERGWERGWEGEGAFWFQWKFBQWFBQWBQWQWBGKQWFBQWFFBJKFBAWFBAJFKBAFLKASJFBASKDJFBAKDFBASKLDBFAJKSBWKJBAKNERNVVNWEVNPEVNINWRUIBRJNBSLNBSDKFVJNDLFVDFKJSLDFNVNVUARIADDCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCASsdslkjsnvvaeesdjksndfgjklnfkdjnjksndbkldnfbsjkblndfkjnbsdbkjsdbsdbgjkdsfkljsdfkjladfdfssdfffffgfgafgadfglakjdfgnlfnagklanpp");
}
