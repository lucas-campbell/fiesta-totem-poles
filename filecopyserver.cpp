// --------------------------------------------------------------
//
//                        filecopyserver.cpp
//
//        Authors: Lucas Campbell and Dylan Hoffmann
//
//        - adapted from pingserver.cpp, written by Noah Mendelsohn
//   

//
//
//        COMMAND LINE
//
//          fileserver <networknastiness> <filenastiness> <targetdir>
//
//
//        OPERATION
//
//              filecopy server will wait until receiving a directory
//              pilot packet, set up the file environment, and then
//              begin receiving file specific packets. As each packet
//              will be numbered it will ignore any duplicates it
//              receives and fill in the file data as packets arrive.
//             There will likely be additional logic to request packets,
//             and to handle multiple files at once, but as we haven't
//             encountered any issues yet we don't have any concrete
//             plans to address them.
//
//
//     
// --------------------------------------------------------------

#include "c150nastydgmsocket.h"
#include "c150debug.h"
#include "sha1.h"
#include "protocol.h"
#include <fstream>
#include <cstdlib> 


using namespace C150NETWORK;  // for all the comp150 utilities 

void setUpDebugLogging(const char *logname, int argc, char *argv[]);

const int NETWORK_NASTINESS_ARG = 1;
const int FILE_NASTINESS_ARG = 2;
const int TARGET_ARG = 3;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                           main program
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 
int
main(int argc, char *argv[])
{
    GRADEME(argc, argv);
    //
    // Variable declarations
    //
    ssize_t readlen;             // amount of data read from socket
    char incoming_msg[512];   // received message data
    int network_nastiness;       // how aggressively do we drop packets, etc?
    int file_nastiness;
    DIR* TRG;
    (void) file_nastiness;

    //
    // Check command line and parse arguments
    //
    if (argc != 4)  {
        fprintf(stderr,"Correct syntxt is: %s <network nastiness>"
                        "<file nastiness> <target directory>\n", argv[0]);
        exit(1);
    }
    if (strspn(argv[NETWORK_NASTINESS_ARG], "0123456789") != 
        strlen(argv[NETWORK_NASTINESS_ARG])) {
        fprintf(stderr,"Nastiness %s is not numeric\n",
                argv[NETWORK_NASTINESS_ARG]);     
        fprintf(stderr,"Correct syntxt is: %s <nastiness_number>\n", argv[0]);
        exit(4);
    }
    if (strspn(argv[FILE_NASTINESS_ARG], "0123456789") !=
        strlen(argv[FILE_NASTINESS_ARG])) {
        fprintf(stderr,"Nastiness %s is not numeric\n",
                argv[FILE_NASTINESS_ARG]);     
        fprintf(stderr,"Correct syntxt is: %s <nastiness_number>\n", argv[0]);
        exit(4);
    }
    // convert command line string to integer
    network_nastiness = atoi(argv[NETWORK_NASTINESS_ARG]);   
    file_nastiness = atoi(argv[FILE_NASTINESS_ARG]);   
       
    //
    //  Set up debug message logging
    //
    setUpDebugLogging("filecopyserverdebug.txt",argc, argv);

    //
    // Open the target directory
    //
    TRG = opendir(argv[TARGET_ARG]);
    if (TRG == NULL) {
        fprintf(stderr,"Error opening source directory %s\n", argv[TARGET_ARG]);
        exit(8);
    }

    //
    // We set a debug output indent in the server only, not the client.
    // That way, if we run both programs and merge the logs this way:
    //
    //    cat pingserverdebug.txt pingserverclient.txt | sort
    //
    // it will be easy to tell the server and client entries apart.
    //
    // Note that the above trick works because at the start of each
    // log entry is a timestamp that sort will indeed arrange in 
    // timestamp order, thus merging the logs by time across 
    // server and client.
    //
    c150debug->setIndent("    ");              // if we merge client and server
    // logs, server stuff will be indented



    map<string, string> filehash;
    fillChecksumTable(filehash, TRG, argv[TARGET_ARG]);


    //
    // Create socket, loop receiving and responding
    //
    try {
        //   c150debug->printf(C150APPLICATION,"Creating C150DgmSocket");
        //   C150DgmSocket *sock = new C150DgmSocket();

        cerr << "Creating C150NastyDgmSocket(nastiness=" <<
            network_nastiness <<endl;
        c150debug->printf(C150APPLICATION,"Creating "
                          "C150NastyDgmSocket(nastiness=%d)",
                          network_nastiness);
        C150DgmSocket *sock = new C150NastyDgmSocket(network_nastiness);
        cerr << "ready to accept messages\n";
        c150debug->printf(C150APPLICATION,"Ready to accept messages");

        //
        // infinite loop processing messages
        //
        while(1) {

            //
            // Read a packet
            // -1 in size below is to leave room for null
            //
            readlen = sock -> read(incoming_msg, sizeof(incoming_msg)-1);
            if (readlen == 0) {
                cerr << "read zero length meassage\n";
                c150debug->printf(C150APPLICATION,"Read zero length message,"
                                  " trying again");
                continue;
            }

            //
            // Clean up the message in case it contained junk
            //
            incoming_msg[readlen] = '\0'; // make sure null terminated
            string incoming(incoming_msg); // Convert to C++ string
            // ...it's slightly easier to work with, and cleanString expects it
            c150debug->printf(C150APPLICATION,"Successfully read %d bytes."
                              " Message=\"%s\"", readlen, incoming.c_str());

            char pack_type = incoming[0];
            switch (pack_type) {
            case 'D':
                handleDir(incoming);
                break;

            case 'P':
                handleFilePilot(incoming);
                break;
            case 'F':
                handleData(incoming);
                break;
            }
            DirPilot dir_pilot = unpackDirPilot(incoming);

            // checksum of target directory
            string target_dir_hash = getDirHash(filehash, true);

            printf("target_dir_hash: ");
            printHash((const unsigned char *)target_dir_hash.c_str());
            printf("\n");

            printf("unpacked dir_pilot.hash: ");
            printHash((const unsigned char *)dir_pilot.hash.c_str());
            printf("\n");


            bool hashes_match = (target_dir_hash == dir_pilot.hash);

            //
            //  create the message to return. At this point, just return what
            //  we received.
            // 
            string response;
            if (hashes_match)
                response = "DirHashOK";
            else
                response = "DirHashError";

            //
            // write the return message
            //
            c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                              response.c_str());
            sock -> write(response.c_str(), response.length()+1);
        }
    }

    catch (C150NetworkException& e) {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG,"Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0] << ": caught C150NetworkException: "
             << e.formattedExplanation() << endl;
    }

    // This only executes if there was an error caught above
    return 4;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
//                     setUpDebugLogging
//
//        For COMP 150-IDS, a set of standards utilities
//        are provided for logging timestamped debug messages.
//        You can use them to write your own messages, but 
//        more importantly, the communication libraries provided
//        to you will write into the same logs.
//
//        As shown below, you can use the enableLogging
//        method to choose which classes of messages will show up:
//        You may want to turn on a lot for some debugging, then
//        turn off some when it gets too noisy and your core code is
//        working. You can also make up and use your own flags
//        to create different classes of debug output within your
//        application code
//
//        NEEDSWORK: should be factored and shared w/pingclient
//        NEEDSWORK: document arguments
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 
void setUpDebugLogging(const char *logname, int argc, char *argv[]) {

    //   
    //           Choose where debug output should go
    //
    // The default is that debug output goes to cerr.
    //
    // Uncomment the following three lines to direct
    // debug output to a file. Comment them to 
    // default to the console
    //  
    // Note: the new DebugStream and ofstream MUST live after we return
    // from setUpDebugLogging, so we have to allocate
    // them dynamically.
    //
    //
    // Explanation: 
    // 
    //     The first line is ordinary C++ to open a file
    //     as an output stream.
    //
    //     The second line wraps that will all the services
    //     of a comp 150-IDS debug stream, and names that filestreamp.
    //
    //     The third line replaces the global variable c150debug
    //     and sets it to point to the new debugstream. Since c150debug
    //     is what all the c150 debug routines use to find the debug stream,
    //     you've now effectively overridden the default.
    //
    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);


    //
    //  Put the program name and a timestamp on each line of the debug log.
    //
    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp(); 

    //
    // Ask to receive all classes of debug message
    //
    // See c150debug.h for other classes you can enable. To get more than
    // one class, you can or (|) the flags together and pass the combined
    // mask to c150debug -> enableLogging 
    //
    // By the way, the default is to disable all output except for
    // messages written with the C150ALWAYSLOG flag. Those are typically
    // used only for things like fatal errors. So, the default is
    // for the system to run quietly without producing debug output.
    //
    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC | 
                             C150NETWORKDELIVERY); 
}

/*
 * handleDir
 * TODO: FUNCTION CONTRACT
 */
void handleDir(string incoming)
{
    DirPilot dir_pilot = unpackDirPilot(incoming);
    
    // checksum of target directory
    string target_dir_hash = getDirHash(filehash, true);
    
    printf("target_dir_hash: ");
    printHash((const unsigned char *)target_dir_hash.c_str());
    printf("\n");
    
    printf("unpacked dir_pilot.hash: ");
    printHash((const unsigned char *)dir_pilot.hash.c_str());
    printf("\n");
    
    
    bool hashes_match = (target_dir_hash == dir_pilot.hash);
    
            //
            //  create the message to return. At this point, just return what
            //  we received.
            // 
    string response;
    if (hashes_match)
        response = "DirHashOK";
    else
        response = "DirHashError";
    
    //
    // write the return message
    //
    c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                      response.c_str());
    sock -> write(response.c_str(), response.length()+1);
}

/*
 * handleFilePilot
 * TODO: FUNCTION CONTRACT
 */
void handleFilePilot(string incoming)
{
    DirPilot dir_pilot = unpackDirPilot(incoming);

    // checksum of target directory
    string target_dir_hash = getDirHash(filehash, true);
    
    printf("target_dir_hash: ");
    printHash((const unsigned char *)target_dir_hash.c_str());
    printf("\n");
    
    printf("unpacked dir_pilot.hash: ");
    printHash((const unsigned char *)dir_pilot.hash.c_str());
    printf("\n");
    
    
    bool hashes_match = (target_dir_hash == dir_pilot.hash);
    
    //
    //  create the message to return. At this point, just return what
    //  we received.
    // 
    string response;
    if (hashes_match)
        response = "DirHashOK";
    else
        response = "DirHashError";
    
    //
    // write the return message
    //
    c150debug->printf(C150APPLICATION,"Responding with message=\"%s\"",
                      response.c_str());
    sock -> write(response.c_str(), response.length()+1);
}
