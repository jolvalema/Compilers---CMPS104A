// $Id: astree.cpp,v 1.8 2016-09-21 17:13:03-07 - - $

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "string_set.h"
#include "lyutils.h"

astree::astree (int symbol_, const location& lloc_, const char* info) {
   symbol = symbol_;
   lloc = lloc_;
   lexinfo = string_set::intern (info);
   block_nr = 0;
   global = 0;
   // vector defaults to empty -- no children
}

astree::~astree() {
   while (not children.empty()) {
      astree* child = children.back();
      children.pop_back();
      delete child;
   }
   if (yydebug) {
      fprintf (stderr, "Deleting astree (");
      astree::dump (stderr, this);
      fprintf (stderr, ")\n");
   }
}

astree* astree::adopt (astree* child1, astree* child2) {
   if (child1 != nullptr) children.push_back (child1);
   if (child2 != nullptr) children.push_back (child2);
   return this;
}

astree* astree::adopt_sym (astree* child, int symbol_) {
   symbol = symbol_;
   return adopt (child);
}

astree* astree::adopt_two (astree* child) {
   
   if (child != nullptr) {
      vector<astree*>::iterator it;
      it = children.begin();
      children.insert(it, child);
   }

   return this;
}

string astree::type_string() {
  string type_string = "";
  if (attributes.test (ATTR_void)) type_string += "void ";
  if (attributes.test (ATTR_int)) type_string += "int ";
  if (attributes.test (ATTR_null)) type_string += "null ";
  if (attributes.test (ATTR_string)) type_string += "string ";
  if (attributes.test (ATTR_struct)) {
    type_string += "struct ";
    /*type_string += "\"";
    type_string += *type_name;
    type_string += "\" ";*/
  }
  if (attributes.test (ATTR_array)) type_string += "array";
  return type_string;

}


void astree::dump_node (FILE* outfile) {
   fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
            this, parser::get_tname (symbol),
            lloc.filenr, lloc.linenr, lloc.offset,
            lexinfo->c_str());
   for (size_t child = 0; child < children.size(); ++child) {
      fprintf (outfile, " %p", children.at(child));
   }
}

void astree::dump_tree (FILE* outfile, int depth) {
   fprintf (outfile, "%*s", depth * 3, "");
   dump_node (outfile);
   fprintf (outfile, "\n");
   for (astree* child: children) child->dump_tree (outfile, depth + 1);
   fflush (NULL);
}

void astree::dump (FILE* outfile, astree* tree) {
   if (tree == nullptr) fprintf (outfile, "nullptr");
                   else tree->dump_node (outfile);
}

void destroy (astree* tree1, astree* tree2) {
   if (tree1 != nullptr) delete tree1;
   if (tree2 != nullptr) delete tree2;
}

void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
   static char buffer[0x1000];
   assert (sizeof buffer > strlen (format) + strlen (arg));
   snprintf (buffer, sizeof buffer, format, arg);
   errprintf ("%s:%zd.%zd: %s", 
              lexer::filename (lloc.filenr), lloc.linenr, lloc.offset,
              buffer);
}

void errllocprintf2 (const location& lloc, const char* format,
                    const char* arg, const char* arg2) {
  static char buffer[0x1000];
  assert (sizeof buffer > strlen (format) + strlen (arg));
  snprintf (buffer, sizeof buffer, format, arg, arg2);


  errprintf ("%s:%zd.%zd: %s", 
             lexer::filename (lloc.filenr)->c_str(), 
             lloc.linenr, lloc.offset,
             buffer);
}

/**********************************************
 this are new functions for Asgn3.


**********************************************/

// this is the function that prints out the ast tree
// it has to be modified
void astree::print (FILE* outfile, astree* tree, int depth) {
   for(int i = 0; i < depth; i++) {
     fprintf(outfile, "|  ");
   }

   char* token_name = strdup(parser::get_tname(tree->symbol));
   char* short_name = token_name;

   if (strstr(token_name, "TOK_") == token_name){
      short_name = token_name + 4;
   } 

   /*fprintf (outfile, "%s %s (%zd.%zd.%zd)\n", 
            short_name,
            tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset);
   free((char*) token_name);*/

   fprintf (outfile, "%s %s (%zd.%zd.%zd) {%lu} %s\n", 
            short_name,
            tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset,
            tree->block_nr, tree->attributes_string().c_str());
   free((char*) token_name);
   
   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
   
}

// this function adopts the func definitions from our
// grammar.
astree *adopt_func(astree* identdecl, 
                        astree* paramlist, astree* block) {

   astree* funct = new astree(TOK_FUNCTION, identdecl->lloc, "");

   if(!string(";").compare(*block->lexinfo)) {
      funct = prototype_function(identdecl, paramlist);
      return funct;
   }

   funct->adopt(identdecl, paramlist);
   funct->adopt(block);
   return funct;

}

// this functions is a helper function for the adopt_func in 
// order to save function prototypes.
astree *prototype_function(astree* identdecl, astree* paramlist) {
   astree* prototype = new astree(TOK_PROTOTYPE, identdecl->lloc, "");
   prototype->adopt(identdecl, paramlist); 
   return prototype;
}

/**********************************************
 this are new functions for Asgn4


**********************************************/

string astree::attributes_string() {
  string attrs = "";
 
  if (attributes.test (ATTR_field)) attrs += "field ";
  if (attributes.test (ATTR_void)) attrs += "void ";
  if (attributes.test (ATTR_int)) attrs += "int ";
  if (attributes.test (ATTR_null)) attrs += "null ";
  if (attributes.test (ATTR_string)) attrs += "string ";
  if (attributes.test (ATTR_struct)) {
    attrs += "struct ";
    /*attrs += "\"";
    attrs += *type_name;
    attrs += "\" ";*/
  }
  if (attributes.test (ATTR_array)) attrs += "array ";
  if (attributes.test (ATTR_function)) attrs += "function ";
  if (attributes.test (ATTR_prototype)) attrs += "prototype ";
  if (attributes.test (ATTR_variable)) attrs += "variable ";
  if (attributes.test (ATTR_typeid)) attrs += "typeid ";
  if (attributes.test (ATTR_param)) attrs += "param ";
  if (attributes.test (ATTR_lval)) attrs += "lval ";
  if (attributes.test (ATTR_const)) attrs += "const ";
  if (attributes.test (ATTR_vreg)) attrs += "vreg ";
  if (attributes.test (ATTR_vaddr)) attrs += "vaddr ";

  if (symbol == TOK_IDENT) {
    attrs += "(";
    attrs += to_string(declared_loc.filenr);
    attrs += ".";
    attrs += to_string(declared_loc.linenr);
    attrs += ".";
    attrs += to_string(declared_loc.offset);
    attrs += ")";
  }

  return attrs;

}
