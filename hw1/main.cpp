// $Id: main.cpp,v 1.2 2016-08-18 15:13:48-07 - - $

#include <string>
#include <iostream>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auxlib.h"
#include "string_set.h"


using namespace std;
constexpr size_t LINESIZE = 1024;
const string CPP = "/usr/bin/cpp";

std::string checkFileExtension(char* file_name) {
   string filename = string(file_name);
   size_t indexDot = filename.rfind('.', filename.length());

   if (indexDot == string::npos || filename.substr(indexDot, 
                           filename.length()-indexDot) != ".oc") {
      fprintf(stderr, 
         "%s is an invalid input file.Please provide a .oc file\n", 
         file_name);
      return "";
   }else{
      return filename.substr(0, indexDot);
   }
}

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}

// Run cpp against the lines of the file.
void cpplines (FILE* pipe, const char* filename) {
   int linenr = 1;
   char inputname[LINESIZE];
   strcpy (inputname, filename);
   for (;;) {
      char buffer[LINESIZE];
      char* fgets_rc = fgets (buffer, LINESIZE, pipe);
      if (fgets_rc == NULL) break;
      chomp (buffer, '\n');
      //printf ("%s:line %d: [%s]\n", filename, linenr, buffer);
      // http://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
      int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
                              &linenr, inputname);
      if (sscanf_rc == 2) {
         /*printf ("DIRECTIVE: line %d file \"%s\"\n", 
                                          linenr, inputname);*/
         continue;
      }
      char* savepos = NULL;
      char* bufptr = buffer;
      for (int tokenct = 1;; ++tokenct) {
         char* token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = NULL;
         if (token == NULL) break;
         string_set::intern(token);
         //printf ("token %d.%d: [%s]\n", linenr, tokenct, token);
         //const string* str = string_set::intern(token);
         /*printf ("intern (\"%s\") returned %p->\"%s\"\n", 
                                       token, str, str->c_str());*/
      }
      ++linenr;
   }
}


int main (int argc, char** argv) {
   exec::execname = basename (argv[0]);
   int opt;
   string command = "";
   int exit_status = EXIT_SUCCESS;
   bool D_flag = false;
   
   while ((opt = getopt (argc, argv, "@:D:ly")) != -1) {
      switch (opt) {
         case 'l':
            //cout << "Option l was selected" << endl;
            break;
         case 'y':
            //cout << "Option y was selected" << endl;
            break;
         case '@':
            set_debugflags(optarg);
            break;
         case 'D':
            D_flag = true;
            command = CPP + " -D" + optarg + " ";
            break;
      }
   }

   //Check to see if there are arguments after the flag options
   if (optind >= argc) {
      fprintf(stderr, "Expected argument after options\n");
      exit_status = EXIT_FAILURE;
      return exit_status;
   }
   
   for (int i = optind; i < argc; i ++) {
      char* filename = argv[i];
      if(!D_flag){
         command = CPP + " ";
      }
      command.append(filename);
      
      string file_base_name = checkFileExtension(filename);
      if (file_base_name == ""){
         exit_status = EXIT_FAILURE;
         return exit_status;
      }
      string str_out_file_name = file_base_name + ".str";

      FILE* pipe = popen (command.c_str(), "r");

      if (pipe == NULL) {
         fprintf(stderr, "No such file\n");
         exit_status = EXIT_FAILURE;
      }else {
         //const string* str = string_set::intern (argv[i]);
         /*printf ("intern (\"%s\") returned %p->\"%s\"\n", 
                                       argv[i], str, str->c_str());*/
         
         cpplines (pipe, filename);
         // close the pipe
         int pipe_close = pclose(pipe);
         eprint_status (command.c_str(), pipe_close);
         if (pipe_close != 0 ) {
            //fprintf(stderr, "Error closing the pipe\n");
            exit_status = EXIT_FAILURE;
         }

         FILE* str_out_file = fopen(str_out_file_name.c_str(), "w");
         string_set::dump(str_out_file);
         fclose(str_out_file);

      }
   }

   //DEBUGF('x', "hello!\n");
   return exit_status;
}

