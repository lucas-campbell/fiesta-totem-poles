//
//                            makedatafile
//
//           Author: _noah Mendelsohn
//
//     A simple program to fill a file with numbers
//

#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

using namespace std;

int
main(int argc, char *argv[]) {

  int lines_to_write;
  int line_number;
  int number_number;
  int output_number =0;
  char *filename;
  int nums_per_line=10;

  if (argc != 3) {
    fprintf(stderr,"Correct syntax is %s <filename> <lines_to_write>\n", argv[0]);
    exit (4);
  }

  filename = argv[1];
  lines_to_write = atoi(argv[2]);

  if (lines_to_write <= 0) {
    fprintf(stderr,"Correct syntax is %s <filename> <lines_to_write>\n", argv[0]);    
    exit (4);
  }

  printf("Writing %d lines to file %s\n", lines_to_write, filename);

  ofstream of(filename);

  output_number = 0;

  for (line_number = 0; line_number < lines_to_write; line_number++) {
    for (number_number = 0; number_number< nums_per_line; number_number++) {
      of << setw(6) << (output_number++) << ' '; 
    }
    of << endl;
  }

  of.close();

  printf("Wrote %d lines to file %s\n", lines_to_write, filename);

  
}
