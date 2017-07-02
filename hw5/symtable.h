#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

// Import the relevant STL classes
#include <bitset>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

#include "astree.h"
#include "lyutils.h"
#include "auxlib.h"

//extern size_t next_block;

struct symbol {
  attr_bitset attributes;
  symbol_table* fields;
  location lloc;
  size_t block_nr;
  vector<symbol*> parameters;
  string *struct_name;
  const string* param_name;
  const string* type_name;
  const string attributes_string(const string *struct_name);
};


int build_sym(FILE* out, astree* root);

#endif
