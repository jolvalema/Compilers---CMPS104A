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
#include "astree.h"
#include "lyutils.h"
#include "symtable.h"


using namespace std;
constexpr size_t LINESIZE = 1024;
const string CPP = "/usr/bin/cpp";

FILE * tok_file;
//size_t next_block = 1;

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

int main (int argc, char** argv) {
   yy_flex_debug = 0;
   yydebug = 0;
   exec::execname = basename (argv[0]);
   int opt;
   string command = "";
   int exit_status = EXIT_SUCCESS;
   bool D_flag = false;
   
   while ((opt = getopt (argc, argv, "@:D:ly")) != -1) {
      switch (opt) {
         case 'l':
            //cout << "Option l was selected" << endl;
            yy_flex_debug = 1;
            break;
         case 'y':
            //cout << "Option y was selected" << endl;
            yydebug = 1;
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
      string outtok = file_base_name + ".tok";
      string outstr = file_base_name + ".str";
      string outast = file_base_name + ".ast";
      string outsym = file_base_name + ".sym";

      yyin = popen (command.c_str(), "r");

      if (yyin == NULL) {
         fprintf(stderr, "No such file\n");
         exit_status = EXIT_FAILURE;
      }else {

         tok_file = fopen(outtok.c_str(), "w");

         /*for(;;){
            int token = yyparse();
            if(token == YYEOF) break;
         }*/
         int parsing = 0;

         parsing = yyparse();

         if(parsing){
            errprintf ("%:parse failed (%d)\n", parsing);
         }

         fclose(tok_file);
         
         int pipe_close = pclose(yyin);
         eprint_status (command.c_str(), pipe_close);
         if (pipe_close != 0 ) {
            fprintf(stderr, "Error closing the pipe\n");
            exit_status = EXIT_FAILURE;
         }

         FILE* str_file = fopen(outstr.c_str(), "w");
         FILE* ast_file = fopen(outast.c_str(), "w");
         FILE* sym_file = fopen(outsym.c_str(), "w");
         astree::print(ast_file, parser::root, 0);
         fclose(ast_file);
         string_set::dump(str_file);
         fclose(str_file);
         build_sym(sym_file, parser::root);
         fclose(sym_file);

      }
   }

   DEBUGF('x', "hello!\n");
   yylex_destroy();
   destroy(parser::root);
   return exit_status;
}

