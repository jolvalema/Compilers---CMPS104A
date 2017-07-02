#ifndef __EMIT_H
#define __EMIT_H
#include <string>
#include <vector>
#include <iostream>

using namespace std;

#include "astree.h"
#include "lyutils.h"
#include "auxlib.h"

extern vector<astree*> string_queue;

int emit(FILE *outfile, astree *root);

#endif
