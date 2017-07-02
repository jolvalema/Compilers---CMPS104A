// $Id: astree.h,v 1.7 2016-10-06 16:13:39-07 - - $

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
#include <bitset>
#include <unordered_map>
using namespace std;

#include "auxlib.h"

struct location {
   size_t filenr;
   size_t linenr;
   size_t offset;
};

/*
 * This code is left here in astree because the astree
 * struct needs to know what attr_bitset is at compiling.
 *
*/
enum { ATTR_void, ATTR_int, ATTR_null, ATTR_string, 
       ATTR_struct, ATTR_array, ATTR_function,
       ATTR_prototype, ATTR_variable,
       ATTR_field, ATTR_typeid, ATTR_param, ATTR_lval, ATTR_const,
       ATTR_vreg, ATTR_vaddr, ATTR_bitset_size };

struct symbol;
using attr_bitset = bitset<ATTR_bitset_size>;
using symbol_table = unordered_map<const string*, symbol*>;
using symbol_entry = pair<const string*,symbol*>;

struct astree {

   // Fields.
   int symbol;               // token code
   location lloc;            // source location
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node

   /*
    * This is where the block number and the attributes
    * for each token will be set 
   */
   attr_bitset attributes;
   size_t block_nr;
   // this is not used right now.
   const string *type_name;
   // this is not used right now.
   location declared_loc;
   //const string *type_name; //used in inherit_attributes()

   // Functions.
   astree (int symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr);
   astree* adopt_sym (astree* child, int symbol);
   astree* adopt_two (astree* child); // new new new
   void dump_node (FILE*);
   void dump_tree (FILE*, int depth = 0);
   static void dump (FILE* outfile, astree* tree);
   /*this function prints the entire AST tree. Asgn3 and Asgn4*/
   static void print (FILE* outfile, astree* tree, int depth = 0);
   /*this function will get the attributes from each node. It is
   also a helper function for the print function. Asgn4*/
   string attributes_string();
   string type_string();
};

void destroy (astree* tree1, astree* tree2 = nullptr);
void errllocprintf (const location&, const char* format, const char*);
void errllocprintf2 (const location&, const char* format,
                    const char* arg, const char* arg2);

// new new new
astree *adopt_func(astree* identdecl, 
                        astree* paramlist, astree* block);
// new new new
astree *prototype_function(astree* identdecl, astree* paramlist); 

#endif

