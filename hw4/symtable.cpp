#include <bitset>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <stack>
#include <string.h>

#include "astree.h"
#include "lyutils.h"
#include "auxlib.h"
#include "symtable.h"

FILE *sym_file;
extern symbol_table* types;
symbol_table* types = new symbol_table;
int typecheck(FILE* outfile, astree *node);
size_t next_block = 1;
int current_indent = 0;

/* function and prototype stuff*/
int return_attr = ATTR_void;
const string *return_struct;
int returned = 0;

/*
 *symbol_stack is a vector that holds the symbols or "variables"
 *declared at compiling from the program
*/
vector<symbol_table*> symbol_stack;
vector<int> block_stack;
symbol_table *struct_table;

/*
 * global_table is just a unordered_map that is used to 
 * initialize the symbol_stack
*/
symbol_table *global_table;

symbol* new_symbol (astree* node) {
    symbol* sym = new symbol();
    sym->attributes = node->attributes;
    sym->fields = nullptr;
    sym->lloc.filenr = node->lloc.filenr;
    sym->lloc.linenr = node->lloc.linenr;
    sym->lloc.offset = node->lloc.offset;
    sym->block_nr = node->block_nr;
    //symbol->parameters = nullptr;
    return sym;
}

/* this is a helper function for the handle_vardecl function.
this function takes in two astree trees as input*/
void inherit_type (astree* parent, astree* child) {
  for (size_t i = 0; i < ATTR_function; i++) {
    if (child->attributes[i] == 1) {
      parent->attributes.set (i);
    }
  }
}

/*this is a helper function for the TOK_CALL token.
This function is different from the function above 
because it takes different parameters.*/
void inherit_type(astree *node, symbol *symbol) {
    for (int i = 0; i < ATTR_function; i++) {
        if (symbol->attributes.test(i)) {
            node->attributes.set(i);
            if (i == ATTR_struct) {
                node->type_name = symbol->type_name;
            }
        }
    }
}

/*this is the helper function for the handle_struct() function.
This function asssigns all the attributes from the node to the symbol*/
void inherit_type(symbol *symbol, astree *node) {
  
  for (int i = 0; i < ATTR_function; i++) {
    if (node->attributes.test(i)) {
      symbol->attributes.set(i);

      if (i == ATTR_struct) {
        symbol->type_name = node->type_name;
      }
    }
  }
}


/*check all calid types. maybe is a struct with different
types.*/
int check_valid_type(astree *node) {
    int type_defined = 0;
    for (int i = 0; i < ATTR_array; i++) {
        if (node->attributes.test(i)) {
            if (type_defined == 1) {
                return 0;
            }      
            type_defined = 1;
        }
    }
    return type_defined;
}

/*check to see for at least one valid type.*/
int check_valid(astree *node) {
    if (!node->attributes.test(ATTR_int)) return 0;
    if (node->attributes.test(ATTR_void)) return 0;
    if (node->attributes.test(ATTR_null)) return 0;
    if (node->attributes.test(ATTR_string)) return 0;
    if (node->attributes.test(ATTR_struct)) return 0;
    return 1;
}

/*this is the helper function for the handle_vardecl()
function. This function is different from the one below*/
int check_compatible(astree *node, astree *other) {
    if (!check_valid_type(node) ||
            !check_valid_type(other)) {
        return 0;
    }
    
    if (!check_valid(node)) {
        if (other->attributes.test(ATTR_null)) {
            return 1;
        }
    }

    if (!check_valid(other)) {
        if (node->attributes.test(ATTR_null)) {
            return 1;
        }
    }
  
    for (int i = 0; i < ATTR_function; i++) {
        if (node->attributes.test(i) !=
            other->attributes.test(i)) {
            return 0;
        }
    }
    
    if (node->attributes.test(ATTR_array) !=
        other->attributes.test(ATTR_array)) {
        return 0;
    }
    return 1;
}

/*this is the helper function for the TOK_CALL node symbol. 
This function gets called everytime the program tries to call
a function that was already declared*/
int check_compatible(attr_bitset first, attr_bitset second) {
    if (!first.test(ATTR_int)) {
        if (second.test(ATTR_null)) {
            return 1;
        }
    }

    if (!second.test(ATTR_int)) {
        if (first.test(ATTR_null)) {
            return 1;
        }
    }

    for (int i = 0; i < ATTR_function; i++) {
        if (first.test(i) != second.test(i)) {
            printf("%i\n", i);
            return 0;
        }
    }
    
    if (first.test(ATTR_array) != second.test(ATTR_array)) {
        return 0;
    }

    return 1;
}


/*helper function for the find_identifier() function. This function
will look through the symbol_stack and check for the given node*/
symbol *find_symbol_in_table(symbol_table *table, astree *node) {
    if (table == nullptr) {
        return nullptr;
    }
    
    auto find = table->find(node->lexinfo);

    if (find != table->end()) {
        return find->second;
    } else {
        return nullptr;
    }

}

symbol *find_identifier(astree *node) {
    for (auto it = symbol_stack.rbegin(); 
        it != symbol_stack.rend(); ++it) {
        symbol *symbol = find_symbol_in_table(*it, node);

        if (symbol != nullptr) {
            return symbol;
        }
    }
    return nullptr;
}

void insert_ident(const string *lexinfo, symbol *symbol) {
    if (symbol_stack.back() == nullptr) {
        symbol_stack.back() = new symbol_table;
    }

    symbol_stack.back()->insert(symbol_entry(lexinfo, symbol));
}

void print_symbol( FILE* outfile, const string *lexinfo, location lloc,
                const string attributes_str, int block_num = -1) {
  
    if (block_stack.back() == 0 && next_block > 1) {
        fprintf(outfile, "\n");
    }
    
    for (int i = 0; i < current_indent; i++) {
        fprintf(outfile, "   ");
    }
    
    fprintf(outfile, "%s (%zd.%zd.%zd) ",
        lexinfo->c_str(),lloc.filenr, lloc.linenr, lloc.offset);

    if (block_num > -1) {
        fprintf(outfile, "{%i} ", block_num);
    }

    fprintf(outfile, "%s\n", attributes_str.c_str());
}

int handle_vardecl(FILE* outfile, astree *node) {
    int errors = 0;
    astree *left_child = nullptr;
    astree *right_child = nullptr;
    if (node->children.size() == 2) {
        left_child = node->children[0];
        right_child = node->children[1];
    }
  
    astree *declarer = left_child;
    // cout << "declarer: " << endl; 
    // cout << declarer->lexinfo->c_str() << endl;
    astree *basetype;
    astree *declid;
  

    if (declarer->symbol == TOK_ARRAY) {
        declarer->attributes.set(ATTR_array);
        basetype = left_child->children[0];
        declid = left_child->children[1];
    } else {
        basetype = left_child;
        declid = left_child->children[0];
    }

    // cout << "basetype: " << endl; 
    // cout << basetype->lexinfo->c_str() << endl;

    // cout << "declid: " << endl; 
    // cout << declid->lexinfo->c_str() << endl;
  
    node->block_nr = block_stack.back();
    declarer->block_nr = block_stack.back();
    // cout << "declarer->block_nr" << endl;
    // cout << declarer->block_nr << endl;
    basetype->block_nr = block_stack.back();
    declid->block_nr = block_stack.back();

    switch (basetype->symbol) {
        case TOK_VOID: { 
            errllocprintf (node->lloc, "error: \'%s\' cannot" 
                " have type void'\n",
                declid->lexinfo->c_str());
            errors++;
            break;
        }
    
        case TOK_INT: {
            //cout << "basetype int" << endl;  
            basetype->attributes.set(ATTR_int);
            break;
        }
        
        case TOK_STRING: {
            //cout << "basetype string" << endl; 
            basetype->attributes.set(ATTR_string);
            break;
        }
        
        case TOK_TYPEID: {
            auto find_struct = struct_table->find(basetype->lexinfo);
            if (find_struct == struct_table->end()) {
                errllocprintf (basetype->lloc, "error: struct \'%s\' "
                    " was not declared in this scope\n", 
                    basetype->lexinfo->c_str());
            errors++;
            } else if (find_struct->second == nullptr) {
                errllocprintf (basetype->lloc, "error: incomplete " 
                    "data type \'%s\'\n",
                    basetype->lexinfo->c_str());
                errors++;
            }
            
            basetype->attributes.set(ATTR_struct);
            basetype->type_name = basetype->lexinfo;
            declarer->attributes.set(ATTR_struct);
            declarer->type_name = left_child->lexinfo;
            break;
        }

            default: {
                errprintf ("Error, invalid token \"%s\"\n", 
                parser::get_tname(basetype->symbol));
            } 
        
    }
  
    //cout << left_child->lexinfo->c_str() << endl;
    errors += typecheck(outfile, right_child);
    inherit_type(declarer, basetype);

    /*this part of the code checks if the types are compatible*/

    int compatible = check_compatible(declarer, right_child);
  
    if (!compatible) {
        errllocprintf2(node->lloc, "error: type %sis not " 
                "compatible with type %s\n", 
                declarer->type_string().c_str(),
                right_child->type_string().c_str());
        errors++;
    }

    /*this part of the code will check if the variable that
    we are trying to declare exists*/
  
    symbol *symbol_to_insert = find_identifier(declid);
    if (symbol_to_insert != nullptr) {
        errllocprintf (declid->lloc, "error: variable \'%s\'" 
                " already defined \n",
                declid->lexinfo->c_str());
        errors++;
    }else{
        symbol_to_insert = new_symbol(declarer);
        symbol_to_insert->attributes.set(ATTR_variable);
        symbol_to_insert->attributes.set(ATTR_lval);
        insert_ident(declid->lexinfo, symbol_to_insert);
        print_symbol(outfile, declid->lexinfo,
                symbol_to_insert->lloc,
                declid->attributes_string(),
                symbol_to_insert->block_nr);
    }
  
    return errors;
}

void inherit_attributes(astree *node, symbol *symbol) {
    for (int i = 0; i < ATTR_bitset_size; i++) {
        if (symbol->attributes.test(i)) {
            node->attributes.set(i);

            if (i == ATTR_struct) {
                node->type_name = symbol->type_name;
            }
        }
    }
}

/*helper function for the handle_struct(). This function assigns all
the attributes from the node to the symbol.*/
void inherit_attributes(symbol *symbol, astree *node) {
  
  for (int i = 0; i < ATTR_bitset_size; i++) {
    if (node->attributes.test(i)) {
      symbol->attributes.set(i);

      if (i == ATTR_struct) {
        symbol->type_name = node->type_name;
      }
    }
  }
}

/*
 * Code to enter a new block
 * Increments next block counter and pushes 
 * nullptr onto the symboll stack
 * 
*/
void enter_block() {
    block_stack.push_back(next_block);
    symbol_stack.push_back(nullptr);
    next_block++;
}

/*
 * 
 * Code to leave a block, symbol stack is popped
 * 
*/
void leave_block() {
    block_stack.pop_back();
    symbol_stack.pop_back();
}

int check_same(attr_bitset first, attr_bitset second) {
    for (int i = 0; i < ATTR_function; i++) {
        if (first.test(i) != second.test(i)) {
            return 0;
        }
    }
    
    if (first.test(ATTR_array) != second.test(ATTR_array)) {
        return 0;
    }
    
    return 1;
}

/*
 * 
 * Handle function declarations
 * 
*/
int handle_function(FILE* outfile, astree *node) {
    int errors = 0;
    int needs_matching = 0;
    if (block_stack.back() != 0) {
        errllocprintf (node->lloc, "error: functions must be declared "
                "in global scope\n" ,node->lexinfo->c_str());
    return 1;
  } 

    astree *return_type = node->children[0];
    astree *basetype;
    astree *declid;
  

    if (return_type->symbol == TOK_ARRAY) {
        return_type->attributes.set(ATTR_array);
        basetype = return_type->children[0];
        declid = return_type->children[1];
    }else {
        basetype = return_type;
        declid = return_type->children[0];
    }
  
    node->block_nr = block_stack.back();
    return_type->block_nr = block_stack.back();
    basetype->block_nr = block_stack.back();
    declid->block_nr = block_stack.back();

    astree *paramlist = node->children[1];
    astree *block = nullptr;

    node->block_nr = block_stack.back();

    if (node->symbol == TOK_FUNCTION) {
        block = node->children[2];
    }

    symbol *function_symbol = find_identifier(declid);
  
    // check if the identifier is already taken by a non
    // prototype
    if (function_symbol != nullptr) {
        if (node->symbol == TOK_FUNCTION) {
            if (function_symbol->attributes.test(ATTR_function)) {
                errllocprintf (node->lloc, "error: redeclaration of " 
                       "\'%s\'\n", declid->lexinfo->c_str());
                return errors + 1;
            }else {
                needs_matching = 1;
            }
        }else {
            errllocprintf (node->lloc, "error: redeclaration of " 
                        "\'%s\'\n", declid->lexinfo->c_str());
            return errors + 1;
        }
    }
    switch (basetype->symbol) {
        case TOK_VOID: {
            basetype->block_nr = block_stack.back();
            declid->block_nr = block_stack.back();
            basetype->attributes.set(ATTR_void);
            return_attr = ATTR_void;
            break;
        }
   
        case TOK_STRING: {
            basetype->block_nr = block_stack.back();
            declid->block_nr = block_stack.back();
            basetype->attributes.set(ATTR_string);
            return_attr = ATTR_string;
            break;
        }
    
        case TOK_INT: {
            basetype->block_nr = block_stack.back();
            declid->block_nr = block_stack.back();
            basetype->attributes.set(ATTR_int);
            return_attr = ATTR_int;
            break;
        }

        case TOK_TYPEID: {
            basetype->block_nr = block_stack.back();
            declid->block_nr = block_stack.back();
            basetype->attributes.set(ATTR_struct);
            basetype->type_name = basetype->lexinfo;
            return_attr = ATTR_struct;
            return_struct = declid->lexinfo;
            break;
        }
    }
  
    inherit_type(return_type, basetype);

    // if the function doesn't already have a prototype
    if (function_symbol == nullptr) {
        function_symbol = new_symbol(return_type);
        if (node->symbol == TOK_PROTOTYPE) {
            function_symbol->attributes.set(ATTR_prototype);
        }else {
            function_symbol->attributes.set(ATTR_function);
        }
    

        insert_ident(declid->lexinfo, function_symbol);
        print_symbol(outfile, declid->lexinfo,
              function_symbol->lloc,
              declid->attributes_string(),
              function_symbol->block_nr);
    
        enter_block();
        current_indent++;
        paramlist->block_nr = block_stack.back();

        astree *param_declarer;
        astree *param_basetype;
        astree *param_declid;
        symbol *param_symbol;

        for (size_t i = 0; i < paramlist->children.size(); i++) {
            param_declarer = paramlist->children[i];      
            if (param_declarer->symbol == TOK_ARRAY) {
                param_declarer->attributes.set(ATTR_array);
                param_basetype = param_declarer->children[0];
                param_declid = param_declarer->children[1];
            }else {
                param_basetype = param_declarer;
                param_declid = param_declarer->children[0];
            }

      
            errors += typecheck(outfile, param_declarer);
            errors += typecheck(outfile, param_basetype);
            inherit_type(param_declarer, param_basetype);

            param_declarer ->block_nr = block_stack.back();
            param_declid ->block_nr = block_stack.back();
            param_basetype ->block_nr = block_stack.back();

            param_declarer->attributes.set(ATTR_param);
            param_declarer->attributes.set(ATTR_variable);
            param_declarer->attributes.set(ATTR_lval);
            param_symbol = new_symbol(param_declarer);
            param_symbol->param_name = param_declid->lexinfo;

            if (errors == 0) {
                insert_ident(param_declid->lexinfo, param_symbol);
                function_symbol->parameters.push_back(param_symbol);
                print_symbol(outfile, param_declid->lexinfo,
                    param_symbol->lloc,
                    param_declid->attributes_string(),
                    param_symbol->block_nr);
            }

        }
    }

    if (node->symbol == TOK_FUNCTION) {
        function_symbol->attributes.reset(ATTR_prototype);
        function_symbol->attributes.set(ATTR_function);

        if (needs_matching) {
            if (paramlist->children.size() !=
                function_symbol->parameters.size()) {
                errllocprintf (node->lloc, "error: wrong number of " 
                       "arguments\n", declid->lexinfo->c_str());
                return errors + 1;
            }
            
            print_symbol(outfile, declid->lexinfo, declid->lloc,
                declid->attributes_string(),
                declid->block_nr);

            enter_block();
            current_indent++;
            paramlist->block_nr = block_stack.back();

            astree *param_declarer;
            astree *param_basetype;
            astree *param_declid;
            symbol *param_symbol;

            symbol *param_check;

            for (size_t i = 0; i < paramlist->children.size(); i++) {
                param_declarer = paramlist->children[i];      
                if (param_declarer->symbol == TOK_ARRAY) {
                    param_declarer->attributes.set(ATTR_array);
                    param_basetype = param_declarer->children[0];
                    param_declid = param_declarer->children[1];
                }else {
                    param_basetype = param_declarer;
                    param_declid = param_declarer->children[0];
                }

        
                errors += typecheck(outfile, param_declarer);
                errors += typecheck(outfile, param_basetype);
                inherit_type(param_declarer, param_basetype);

                param_declarer ->block_nr = block_stack.back();
                param_declid ->block_nr = block_stack.back();
                param_basetype ->block_nr = block_stack.back();

                param_declarer->attributes.set(ATTR_param);
                param_declarer->attributes.set(ATTR_variable);
                param_declarer->attributes.set(ATTR_lval);
                param_symbol = new_symbol(param_declarer);

                const string *param_name = param_declid->lexinfo;
                param_check = function_symbol->parameters[i];


                if (param_name != param_check->param_name) {
                    errllocprintf (basetype->lloc, 
                        "error: parameter \'%s\' doesn't match \n", 
                        param_name->c_str());
                    return errors + 1;
                }

                if (!check_same(param_declarer->attributes, 
                        param_check->attributes)) {
                    errllocprintf(param_declarer->children[0]->lloc, 
                        "error: parameter \'%s\' doesn't match \n", 
                            param_name->c_str());
                    return errors + 1;
                }

                if (errors == 0) {
                    insert_ident(param_declid->lexinfo, param_symbol);
                    function_symbol->parameters.push_back(param_symbol);
                    print_symbol(outfile, param_declid->lexinfo, 
                        param_symbol->lloc,
                        param_declid->attributes_string(),
                        param_symbol->block_nr);
                }

            }

        }
        errors += typecheck(outfile, block);
    }

  
    return_attr = ATTR_void;
    leave_block();
    current_indent--;
    return errors;
}


/*this is the function that deals with function calls.*/
int handle_call(FILE* outfile, astree *node){
    int error = 0;
    node->block_nr = block_stack.back();
    symbol *function_symbol = find_identifier(node->children[0]);

    error += typecheck(outfile, node->children[0]);
      
    if (function_symbol == nullptr) {
        errllocprintf (node->lloc, "error: \'%s\' was not " 
            "declared in this scope\n", 
        node->children[0]->lexinfo->c_str());
        return error + 1;
    }
      
    if (!function_symbol->attributes.test(ATTR_function) && 
        !function_symbol->attributes.test(ATTR_prototype)) {
            errllocprintf (node->lloc, "error: \'%s\' is not " 
                "a function\n", 
            node->children[0]->lexinfo->c_str());
                return error + 1;
    }

    astree *paramlist = node;

    if (paramlist->children.size() - 1 != 
        function_symbol->parameters.size()) {
            errllocprintf (node->lloc, "error: wrong number of " 
                "arguments\n", "");
            return error + 1;
    }

    astree *param;
    for (size_t i = 1; i < paramlist->children.size(); i++) {
        param = paramlist->children[i];
        error += typecheck(outfile, param);
        if (!check_compatible(param->attributes, 
            function_symbol->parameters[i - 1]->attributes)) {
                errllocprintf (node->lloc, "error: arguments don't " 
                    "match\n", "");
            return error + 1;
        }
    }
    inherit_type(node, function_symbol);
        node->attributes.set(ATTR_vreg);
    return error;
}

int handle_struct(FILE* outfile, astree *node) {
    int errors = 0;
    if (block_stack.back() != 0) {
        errllocprintf (node->lloc, "error: structs must" 
            "be declared in global scope\n" ,
            node->lexinfo->c_str());
        return 1;
    }

    astree *type_id = node->children[0];
    type_id->attributes.set(ATTR_struct);
    type_id->type_name = type_id->lexinfo;

    auto find = struct_table->find(type_id->lexinfo);
    symbol *struct_symbol;

    if (find != struct_table->end()) {
        struct_symbol = find->second;
        if (struct_symbol != nullptr) {
            if (struct_symbol->fields != nullptr) {
                errllocprintf (node->lloc, "error: \'%s\' already " 
                       "declared\n" ,
                       type_id->lexinfo->c_str());
                return errors + 1;
            } 
        }
    }

    struct_symbol = new_symbol(node);
    struct_symbol->attributes = type_id->attributes;
  
    struct_table->insert(symbol_entry(type_id->lexinfo, struct_symbol));
    struct_symbol->fields = new symbol_table;
    struct_symbol->type_name = type_id->type_name;

    print_symbol(outfile, type_id->lexinfo, type_id->lloc,
        type_id->attributes_string(), type_id->block_nr);
          
    current_indent++;
    astree *declarer;
    astree *basetype;
    astree *declid;
    symbol *symbol_to_insert;
    
    for (size_t i = 1; i < node->children.size(); i++) {
        declarer = node->children[i];
        if (declarer->symbol == TOK_ARRAY) {
            declarer->attributes.set(ATTR_array);
            basetype = declarer->children[0];
            symbol_to_insert = new_symbol(basetype);
            symbol_to_insert->attributes.set(ATTR_array);
            declid = declarer->children[1];
        }else {
            basetype = declarer;
            symbol_to_insert = new_symbol(basetype);
            declid = declarer->children[0];
        }
        declid->attributes.set(ATTR_field);
    
        if (errors > 0) {
            return errors;
        }

        switch (basetype->symbol) {
            case TOK_VOID: {
                errllocprintf (node->lloc, "error: \'%s\' cannot" 
                    " have type void'\n", declid->lexinfo->c_str());
                basetype->attributes.set(ATTR_void);
                return errors + 1;
                break;
            }
            case TOK_INT: {
                basetype->attributes.set(ATTR_int);
                inherit_type(symbol_to_insert, basetype);
                inherit_attributes(symbol_to_insert, declid);
                struct_symbol->fields->insert(
                    symbol_entry(declid->lexinfo, symbol_to_insert));
                print_symbol(outfile, declid->lexinfo,
                symbol_to_insert->lloc,
                declid->attributes_string());
                break;
            }
            case TOK_STRING: {
                basetype->attributes.set(ATTR_string);
                inherit_type(symbol_to_insert, basetype);
                inherit_attributes(symbol_to_insert, declid);
                struct_symbol->fields->insert(
                    symbol_entry(declid->lexinfo, symbol_to_insert));
                print_symbol(outfile, declid->lexinfo,
                symbol_to_insert->lloc,
                declid->attributes_string());
                break;
            }
            case TOK_TYPEID: {
                basetype->attributes.set(ATTR_struct);
                basetype->type_name = basetype->lexinfo;
                symbol *find_struct = 
                    find_symbol_in_table(struct_table, basetype);

                if (find_struct == nullptr) {
                    struct_table->insert(symbol_entry(
                        basetype->lexinfo, nullptr));
                } 
                inherit_type(symbol_to_insert, basetype);
                inherit_attributes(symbol_to_insert, declid);
                struct_symbol->fields->insert(symbol_entry(
                    declid->lexinfo, symbol_to_insert));
                print_symbol(outfile, declid->lexinfo,
                symbol_to_insert->lloc,
                declid->attributes_string());
        
                break;
            }
        }

    }
  
    //fprintf(sym_out, "\n");
    current_indent--;
    return errors;
}

int typecheck(FILE* outfile, astree *node) {
    int errors = 0;

    /*left_child and right_child are used in the '=' case.*/
    astree *left_child = nullptr;
    astree *right_child = nullptr;
    if(node->children.size() == 2){
        //cout << "left + right" << endl;
        left_child = node->children[0];
        right_child = node->children[1];
    }else if (node->children.size() == 1) {
        //cout << "only left" << endl;
        left_child = node->children[0];
    }

    switch (node->symbol) {

        case TOK_STRUCT: {
            errors += handle_struct(outfile, node);
            break;
        }

        case TOK_CALL: {
            errors += handle_call(outfile, node);
            break;
        }

        case TOK_PROTOTYPE:
        case TOK_FUNCTION: {
            //cout << "prototype and function" << endl;
            errors += handle_function(outfile, node);
            break;
        }

        case TOK_RETURNVOID: {
            node->block_nr = block_stack.back();
            if (return_attr != ATTR_void) {
                errllocprintf (node->lloc, "error: invalid return type" 
                       " \'%s\'\n", node->lexinfo->c_str());
                errors++;
            }
            break;
        }

        case TOK_RETURN: {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            if (!left_child->attributes.test(return_attr)) {
                errllocprintf (node->lloc, 
                        "error: invalid return type" 
                       " \'%s\'\n", node->lexinfo->c_str());
                errors++;
            }else if (return_attr == ATTR_typeid &&
                 left_child->type_name != return_struct) {
                errllocprintf (node->lloc, 
                        "error: invalid return type" 
                       " \'%s\'\n", node->lexinfo->c_str());
            } else {
                returned = 1;
            }
            break;
        }

        case TOK_WHILE:
        case TOK_IF:
        case TOK_IFELSE: {
            node->block_nr = block_stack.back();
            for (size_t i = 0; i < node->children.size(); i++) {
                errors += typecheck(outfile, node->children[i]);
            }
            break;
        }

        case TOK_BLOCK: {
            enter_block();
            current_indent++;
            node->block_nr = block_stack.back();
            for (size_t i = 0; i < node->children.size(); i++) {
                errors += typecheck(outfile, node->children[i]);
            }
            leave_block();
            current_indent--;
            break;
        }

        case TOK_VARDECL:{
            //cout << "tok_vardecl" << endl; 
            errors += handle_vardecl(outfile, node);
            break;
        }

        case TOK_IDENT: {
            //cout << "tok_ident" << endl;
            node->block_nr = block_stack.back();
            symbol *ident_symbol = find_identifier(node);
      
            if (ident_symbol == nullptr) {
                errllocprintf (node->lloc, "error: \'%s\' was not " 
                    "declared in this scope\n", 
                    node->lexinfo->c_str());
                return errors + 1;
            }
            inherit_attributes(node,ident_symbol);
            node->declared_loc = ident_symbol->lloc;
            break;
        }

        case TOK_NEWARRAY: {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            errors += typecheck(outfile, right_child);

            if (check_valid(right_child)) {
                inherit_type(node, left_child);
                node->attributes.set(ATTR_vreg);
                node->attributes.set(ATTR_array);
            } else {
                errllocprintf (left_child->lloc, "error: size " 
                    "is not type int\n", 
                    left_child->lexinfo->c_str());
                return errors + 1;
            }     
            break;
        }

        case TOK_NEW: {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            inherit_type(node, left_child);
            node->attributes.set(ATTR_vreg);
            break;
        }

        case '.': {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            errors += typecheck(outfile, right_child);
            if(errors){
                break;
            }
            //string str = "hello";
            if (!left_child->attributes.test(ATTR_struct)) {
                errllocprintf (left_child->lloc, "error: " 
                    "\'%s\' is not a struct\n",
                    left_child->lexinfo->c_str());
                return errors + 1;
            }
            auto find_struct = 
                struct_table->find(left_child->type_name);
            if (find_struct == struct_table->end()) {
                errllocprintf (left_child->lloc, 
                    "error: struct \'%s\' " 
                        "was not declared in this scope\n", 
                        left_child->lexinfo->c_str());
            return errors + 1;
            } 
            if (find_struct->second == nullptr) {
                errllocprintf (left_child->lloc, "error: incomplete " 
                    "data type \'%s\'\n", 
                    left_child->lexinfo->c_str());
            return errors + 1;
            } 
      
            symbol_table *fields = find_struct->second->fields;
            auto find_field = fields->find(right_child->lexinfo);
            if (find_field == fields->end()) {
                errllocprintf (right_child->lloc, 
                    "error: \'%s\' was not defined in this struct\n",
                     left_child->lexinfo->c_str());
                return errors + 1;
            }
            inherit_attributes(node, find_field->second);
            node->attributes.set(ATTR_vaddr);
            node->attributes.reset(ATTR_field);
            node->declared_loc = find_field->second->lloc;
            node->attributes.set(ATTR_lval);
      
            break;
        }

        case TOK_INT:{
            //cout << "tok_int" << endl;
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_int);
            break;
        }

        case TOK_STRING: {
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_string);
            break;
        }

        case TOK_TYPEID: {
            node->block_nr = block_stack.back();
            auto find_struct = struct_table->find(node->lexinfo);
            if (find_struct == struct_table->end()) {
                errllocprintf (node->lloc, 
                    "error: struct \'%s\' was not "
                    "declared in this scope\n", 
                    node->lexinfo->c_str());
                errors++;
            } else if (find_struct->second == nullptr) {
                 errllocprintf (node->lloc, "error: incomplete " 
                    "data type \'%s\'\n", node->lexinfo->c_str());
                errors++;
            }
            node->attributes.set(ATTR_struct); // this causes seg error
            node->type_name = node->lexinfo; //
            break;
        }

        case TOK_FIELD: {
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_field);
            break;
        }

        case TOK_NULL: {
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_null);
            node->attributes.set(ATTR_const);
            break;
        }

        case TOK_INTCON: {
            //cout << "tok_intcon" << endl;
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_const);
            break;
        }

        case TOK_STRINGCON: {
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_string);
            node->attributes.set(ATTR_const);
            break;
        }

        case TOK_CHARCON: {
            //DEBUGF('s', "case charcon\n");
            node->block_nr = block_stack.back();
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_const);
            break;
        }

        case TOK_INDEX: {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            errors += typecheck(outfile, right_child);

            if (!right_child->attributes.test(ATTR_int)) {
                errllocprintf (right_child->lloc, 
                    "error: invalid type for "
                    "array subscript: %s\n",
                    right_child->attributes_string().c_str());
                errors++;
            }

            if (left_child->attributes.test(ATTR_array)) {
                inherit_type(node, left_child);
                node->attributes.reset(ATTR_array);
                node->attributes.set(ATTR_vaddr);
                node->attributes.set(ATTR_lval);
            } else if (left_child->attributes.test(ATTR_string)){
                node->attributes.set(ATTR_int);
                node->attributes.set(ATTR_vaddr);
                node->attributes.set(ATTR_lval);
            }else {
                errllocprintf (right_child->lloc, 
                    "error: type %sis not" 
                    " subscriptable\n", 
                    left_child->type_string().c_str());
                errors++;
            }
            break;

        }

        case '=': {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            //cout << left_child->lexinfo->c_str() << endl;
            errors += typecheck(outfile, right_child);
            //cout << right_child->lexinfo->c_str() << endl;
            if(errors){
                break;
            }

            if (!left_child->attributes.test(ATTR_lval)){
                errllocprintf (left_child->lloc, "error: type %s " 
                    "is not assignable\n",
                       left_child->type_string().c_str());
            }

            if (check_compatible(left_child, right_child)) {
                inherit_type(node, left_child);
                node->attributes.set(ATTR_vreg);
            } else {
                errllocprintf2(node->lloc, "error: type %scannot be " 
                    "assigned to type %s\n", 
                right_child->type_string().c_str(),
                left_child->type_string().c_str());
                errors += 1;
            }
            break;

        }
        
        case TOK_ARRAY:{
            break;
        }

        case TOK_EQ: 
        case TOK_NE: 
        case TOK_LT: 
        case TOK_LE: 
        case TOK_GT: 
        case TOK_GE: {
      
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            errors += typecheck(outfile, right_child);
            if (check_compatible(left_child, right_child)) {
                node->attributes.set(ATTR_int);
                node->attributes.set(ATTR_vreg);
            } else {
                errllocprintf (node->lloc, "error: \'%s\' has invalid " 
                    "type comparison\n" , node->lexinfo->c_str());
                errors++;
            }
            break;
        } 

        case TOK_POS:
        case TOK_NEG:
        case '!': {
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            if (check_valid(left_child)) {
                node->attributes.set(ATTR_int);
                node->attributes.set(ATTR_vreg);
            } else {
                errllocprintf (node->lloc, "error: \'%s\' has invalid " 
                    "type comparison\n" ,
                    node->lexinfo->c_str());
                errors++;
            }
            break;
        }

        case '*':
        case '-':
        case '+': {
            //cout << "plus" << endl;
            node->block_nr = block_stack.back();
            errors += typecheck(outfile, left_child);
            errors += typecheck(outfile, right_child);
            if (check_valid(left_child) && check_valid(right_child)) {
                node->attributes.set(ATTR_int);
                node->attributes.set(ATTR_vreg);
            }else {
                errllocprintf (node->lloc, "error: \'%s\' has invalid " 
                    "type comparison\n" ,
                node->lexinfo->c_str());
                errors++;
            }
            break;
        }
 
        case TOK_ROOT: {
            //DEBUGF('s', "case root\n");
            for (astree* child: node->children) {
                //DEBUGF('c', "children\n");
                typecheck(outfile, child);
                //cout << "hello" << endl;
            }
            node->block_nr = 0;
            break;
        }

        default: {
            errprintf ("Error, invalid token \"%s\"\n", 
                parser::get_tname(node->symbol)); 
        }
        
    }

    return errors;
}

int build_sym(FILE* out, astree* root){
    symbol_stack.push_back(global_table);
    block_stack.push_back(0);
    struct_table = new symbol_table;

    return typecheck(out, root);

}
