#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>



// Vector utils

#define VECT_MAX_GROW 100
#define VECT_MIN_SIZE 4

typedef struct {
	size_t _el_sz, count, size;
	void *data;
} Vector;

Vector vect_init(size_t item_size) {
	Vector out = {0};
	
	out._el_sz = item_size;
	out.size = VECT_MIN_SIZE;
	out.count = 0;
	out.data = malloc(out.size * out._el_sz);

	return out;
}

void _vect_grow(Vector *v) {
	if (v->size / 2 > VECT_MAX_GROW) {
		v->size += VECT_MAX_GROW;
	} else {
		v->size += v->size / 2;
	}
	v->data = realloc(v->data, v->size * v->_el_sz);
}

void _vect_shrink(Vector *v) {
	if (v->size / 2 > VECT_MIN_SIZE) {
		v->size = v->size / 2;
		v->data = realloc(v->data, v->size * v->_el_sz);
	}
}

bool vect_remove(Vector *v, size_t index) {
	if (index >= v->count) {
		return false;
	}

	char *remove = v->data + (index * v->_el_sz);
	char *override = v->data + (index + 1) * v->_el_sz;

	for(size_t i = 0; i < (v->count - index - 1) * v->_el_sz; i++) {
		remove[i] = override[i];
	}

	v->count -= 1;

	if (v->count < v->size / 4) {
		_vect_shrink(v);
	}

	return true;
}

void vect_pop(Vector *v) {
	vect_remove(v, v->count - 1);
}

bool vect_insert(Vector *v, size_t index, void *el) {
	if (index > v->count) {
		return false;
	}

	char *new_spot = v->data + (v->count + 1) * v->_el_sz;
	char *old_spot = v->data + v->count * v->_el_sz;

	for (size_t i = 0; i < (v->count - index) * v->_el_sz; i++) {
		*new_spot = *old_spot;
		new_spot--;
		old_spot--;
	}

	for (size_t i = 0; i < v->_el_sz; i++) {
		old_spot[i] = ((char *)el)[i];
	}

	v->count += 1;

	if (v->count == v->size - 1) {
		_vect_grow(v);
	}

	return true;
}

void vect_push(Vector *v, void *el) {
	vect_insert(v, v->count, el);
}

void vect_push_string(Vector *v, char *str) {
	if (v->_el_sz != sizeof(char)) {
		return;
	}

	for (size_t i = 0; str[i] != 0; i++) {
		vect_insert(v, v->count, str + i);
	}
}

void vect_push_free_string(Vector *v, char *str) {
	vect_push_string(v, str);
	free(str);
}

void *vect_get(Vector *v, size_t index) {
	if (index >= v->count) {
		return NULL;
	}

	return v->data + (v->_el_sz * index);
}

Vector vect_clone(Vector *v) {
	Vector out = {0};
	
	out._el_sz = v->_el_sz;
	out.count = 0;
	out.size = v->count + 1;

	out.data = malloc((out.count + 1) * out._el_sz);

	char *former = v->data;
	char *latter = out.data;

	for(size_t i = 0; i < out.count * out._el_sz; i++) {
		latter[i] = former[i];
	}

	return out;
}

Vector vect_from_string(char *s) {
	Vector out = vect_init(1);

	size_t i = 0;
	while(s[i] != 0) {
		vect_push(&out, s + i++);
	}

	return out;
}

// Returns the vector data as a null-terminated string
// do NOT free this pointer unless you discard the vector.
// Not safe to use this string at the same time as you are
// adding or removing from the vector. Consider cloning the
// vector if you must have both, or want an independant copy
// of the string.
char *vect_as_string(Vector *v) {
	((char*)v->data)[v->count] = 0;
	return v->data;
}

void vect_end(Vector *v) {
	v->_el_sz = 0;
	v->count = 0;
	v->size = 0;
	free(v->data);
	v->data = NULL;
}



// Artifacts (vect of strings)

typedef Vector Artifact;

/* Splits the string via the given character, and
 * stores the split strings in an artifact
 */
Artifact art_from_str(const char *str, char split) {
	Artifact out = vect_init(sizeof(char *));

	char *cur = malloc(1);
	cur[0] = 0;
	int cur_len = 0;

	for (int i = 0; str[i] != 0; i++) {
		if (str[i] == split) {
			cur[cur_len] = 0;
			vect_push(&out, &cur);
			cur = malloc(1);
			cur_len = 0;
		} else {
			cur_len += 1;
			cur = realloc(cur, cur_len + 1);
			cur[cur_len - 1] = str[i];
		}
	}

	if (cur_len > 0) {
		cur[cur_len] = 0;
		vect_push(&out, &cur);
	} else {
		free(cur);
	}

	return out;
}

// Joins the string together with the provided character in between
// must free the returned data after use.

char *art_to_str(Artifact *art, char join) {
	char *out = malloc(1);
	int out_len = 0;

	for (size_t i = 0; i < art->count; i++) {
		char ** cpy = vect_get(art, i);
		
		for(int j = 0; (*cpy)[j] != 0; j++) {
			out[out_len] = (*cpy)[j];
			out_len += 1;
			out = realloc(out, out_len + 1);
		}
		
		if (i < art->count - 1) {
			out[out_len] = join;
			out_len += 1;
			out = realloc(out, out_len + 1);
		}
	}

	out[out_len] = 0;

	return out;
}

// Pops a string off the end of the artifact,
// freeing the data associated
void art_pop_str(Artifact *art) {
	if (art->count == 0)
		return;
	
	char ** to_free = vect_get(art, art->count - 1);
	free(*to_free);
	vect_pop(art);
}

// Copies a string onto the artifact,
// you must free the original string
// manually if it was malloc-ed
void art_add_str(Artifact *art, char *str) {
	Vector copy = vect_from_string(str);
	char * copy_ptr = vect_as_string(&copy);
	vect_push(art, &copy_ptr);
}

// a = a + b
void art_add_art(Artifact *a, Artifact *b) {
	for(size_t i = 0; i < b->count; i++) {
		art_add_str(a, *(char **)vect_get(b, i));
	}
}

// Weather the string str is contained in the artifact
bool art_contains(Artifact *a, char *str) {
	for (size_t i = 0; i < a->count; i++) {
		char **cur = vect_get(a, i);
		if (strcmp(str, *cur) == 0)
			return true;
	}
	return false;
}

// Frees all strings in the artifact,
// then calls vect_end
void art_end(Artifact *art) {
	char **to_free = art->data;
	for(size_t i = 0; i < art->count; i++) {
		free(to_free[i]);
	}
	vect_end(art);
}


// Compile Data - CompData holds final program as it is assembled
typedef struct {
	Vector header, data, text;
} CompData;

CompData cdat_init() {
	CompData out = {0};

	out.header = vect_from_string("");
	out.data = vect_from_string("");
	out.text = vect_from_string("");

	return out;
}

void cdat_add(CompData *a, CompData *b) {
	vect_push_string(&a->header, vect_as_string(&b->header));
	vect_push_string(&a->data, vect_as_string(&b->data));
	vect_push_string(&a->text, vect_as_string(&b->text));
}

void cdat_write_to_file(CompData *cdat, FILE *fout) {
	fprintf(fout, "bits 64\n");
	fprintf(fout, "\n%s\n", vect_as_string(&(cdat->header)));
	fprintf(fout, "section .data\n%s\n", vect_as_string(&(cdat->data)));
	fprintf(fout, "section .text\n%s\n", vect_as_string(&(cdat->text)));
	fflush(fout);
}

void cdat_end(CompData *cdat) {
	vect_end(&(cdat->header));
	vect_end(&(cdat->data));
	vect_end(&(cdat->text));
}



// Gen utils

char *int_to_str(int i) {
	Vector v = vect_init(sizeof(char));
	
	char to_push = '0';
	bool minus = false;

	// check negative
	if(i < 0) {
		i = -i;
		minus = true;
	}

	// zero case
	if(i == 0) {
		vect_push(&v, &to_push);
	}

	// get all digits (in reverse order)
	while(i > 0) {
		to_push = '0' + i % 10;
		i = i / 10;
		vect_push(&v, &to_push);
	}

	// handle negative
	if(minus) {
		to_push = '-';
		vect_push(&v, &to_push);
	}

	// reverse string
	for(size_t idx = 0; idx < v.count / 2; idx++) {
		to_push = *(char*)vect_get(&v, idx);
		((char*)v.data)[idx] = ((char*)v.data)[v.count - (idx + 1)];
		((char*)v.data)[v.count - (idx + 1)] = to_push;
	}

	//push 0
	to_push = 0;
	vect_push(&v, &to_push);

	return v.data;
}



// Types

typedef struct Module {
	char *name;
	bool exported;
	Vector types, vars, funcs, submods;
	struct Module *parent;
} Module;

typedef struct {
	char *name;         // Name of the type
	int size;           // Size (bytes) of the type
	Vector members;     // Member variables (Stored as variables)
	Module *module;     // Module (for methods and member-type resolution) to tie the type to.
} Type;

typedef struct {
	char *name;
	Type *type;
	Vector ptr_chain;
	int location; // negative one for on stack, negative two for literal, zero for in data section, positive for in register
	int offset;   // offset for member variables (if this is a literal, it represents the actual value)
	Module *mod;  // Only used in the case of a data section variable;
} Variable;

#define LOC_LITL -2
#define LOC_STCK -1
#define LOC_DATA 0

#define PTYPE_NONE -2
#define PTYPE_PTR -1
#define PTYPE_REF 0
#define PTYPE_ARR 1

typedef struct {
	char *name;
	Vector inputs, outputs;
	Module *module;
} Function;

typedef struct Scope {
	char *name;
	Module *current;
	Vector stack_vars, reg_vars;
	struct Scope *parent;
	int next_const;
	int next_bool;
} Scope;


// Copies the name, does not copy the module.
// Types should be freed at the end of the second pass,
// as they are shared among all variable structs
Type typ_init(char *name, Module *module) {
	Type out = {0};

	Vector name_cpy = vect_from_string(name);
	out.name = vect_as_string(&name_cpy);
	out.members = vect_init(sizeof(Variable));
	out.module = module;
	out.size = 0;

	return out;
}

void var_end(Variable *v);

// Deep end, will free all memory associated with the
// struct, including name and sub-member variables
void typ_end(Type *t) {
	free(t->name);
	t->module = NULL;

	for (size_t i = 0; i < t->members.count; i++) {
		Variable *to_end = vect_get(&(t->members), i);
		var_end(to_end);
	}

	vect_end(&(t->members));
}

Type TYP_INBUILT[] = {
	{"uint8", 1, {0}, NULL},
	{"uint16", 2, {0}, NULL},
	{"uint32", 4, {0}, NULL},
	{"uint64", 8, {0}, NULL},
	{"uint", 8, {0}, NULL},      // Platform max uint
	{"int8", 1, {0}, NULL},
	{"int16", 2, {0}, NULL},
	{"int32", 4, {0}, NULL},
	{"int64", 8, {0}, NULL},
	{"int", 8, {0}, NULL},       // Platform max int
	{"float32", 4, {0}, NULL},
	{"float64", 8, {0}, NULL},
	{"float", 8, {0}, NULL},     // Platform max float
	{"bool", 1, {0}, NULL},
	{"void", 8, {0}, NULL},      // Untyped pointer
};

Type *typ_get_inbuilt(char *name) {
	for (size_t i = 0; i < sizeof(TYP_INBUILT)/sizeof(Type); i++) {
		if (strcmp(TYP_INBUILT[i].name, name) == 0) {
			return &(TYP_INBUILT[i]);
		}
	}
	return NULL;
}

bool is_inbuilt(char *name) {
	for (size_t i = 0; i < sizeof(TYP_INBUILT)/sizeof(Type); i++) {
		if (strcmp(TYP_INBUILT[i].name, name) == 0) {
			return true;
		}
	}
	return false;
}


// SCOPE FUNCTIONS

Scope scope_init(char *name, Module *mod) {
	Scope out = {0};

	Vector cpy = vect_from_string(name);
	out.name = vect_as_string(&cpy);

	out.stack_vars = vect_init(sizeof(Variable));
	out.reg_vars = vect_init(sizeof(Variable));
	out.current = mod;

	out.next_const = 0;
	out.next_bool = 0;

	return out;
}

void scope_end(Scope *s) {
	free(s->name);
	
	for(size_t i = 0; i < s->stack_vars.count; i++) {
		Variable *v = vect_get(&s->stack_vars, i);
		var_end(v);
	}
	vect_end(&s->stack_vars);

	for(size_t i = 0; i < s->reg_vars.count; i++) {
		Variable *v = vect_get(&s->reg_vars, i);
		var_end(v);
	}
	vect_end(&s->reg_vars);
}

// Label generation
void _scope_name_rec(Scope *s, Vector *v) {
	// Base case
	if (s == NULL)
		return;

	_scope_name_rec(s->parent, v);
	
	// Add # before name if not directly from module
	if (s->parent != NULL)
		vect_push_string(v, "#");
	vect_push_string(v, s->name);
}

char *mod_label_prefix(Module *m);
Vector _scope_base_label(Scope *s) {
	Vector out = vect_from_string("");
	vect_push_free_string(&out, mod_label_prefix(s->current));

	_scope_name_rec(s, &out);

	return out;
}

char *scope_label_start(Scope *s) {
	Vector out = _scope_base_label(s);
	vect_push_string(&out, "#start");
	return vect_as_string(&out);
}

char *scope_label_rep(Scope *s) {
	Vector out = _scope_base_label(s);
	vect_push_string(&out, "#rep");
	return vect_as_string(&out);
}

char *scope_label_end(Scope *s) {
	Vector out = _scope_base_label(s);
	vect_push_string(&out, "#end");
	return vect_as_string(&out);
}

char *scope_gen_const_label(Scope *s) {
	Vector out = _scope_base_label(s);
	vect_push_string(&out, "#const");
	vect_push_free_string(&out, int_to_str(s->next_const));
	s->next_const++;
	return vect_as_string(&out);
}

char *scope_gen_bool_label(Scope *s) {
	Vector out = _scope_base_label(s);
	vect_push_string(&out, "#bool");
	vect_push_free_string(&out, int_to_str(s->next_bool));
	return vect_as_string(&out);
}

void scope_adv_bool_label(Scope *s) {
	s->next_bool++;
}

// Variables

// Initializes the variable, copying name, not deep copying type as it is a pointer.
Variable var_init(char *name, Type *type) {
	Variable out = {0};
	
	Vector name_cpy = vect_from_string(name);
	out.name = vect_as_string(&name_cpy);
	out.type = type;
	out.ptr_chain = vect_init(sizeof(int));
	out.location = 0;
	out.offset = 0;
	out.mod = NULL;

	return out;
}

Variable var_copy(Variable *to_copy) {
	Variable out = var_init(to_copy->name, to_copy->type);

	out.location = to_copy->location;
	out.offset = to_copy->offset;
	out.mod = to_copy->mod;

	for (size_t i = 0; i < to_copy->ptr_chain.count; i++) {
		int *ptr_orig = vect_get(&(to_copy->ptr_chain), i);
		vect_push(&(out.ptr_chain), ptr_orig);
	}

	return out;
}

// Simple cleanup for variables while the second pass is ongoing.
void var_end(Variable *v) {
	free(v->name);
	vect_end(&(v->ptr_chain));
}

// Variable operations

// Valid prefixes for fasm, take size of data - 1 = index of prefix.
char *PREFIXES[] = {
	"byte ",
	"word ",
	"",
	"dword ",
	"",
	"",
	"",
	"qword "
};

/// Remember to free!
char *_gen_address(char *prefix, char *base, char *offset, int mult, int add, bool rel) {
	Vector out = vect_init(sizeof(char));

	vect_push_string(&out, prefix);
	if (rel)
		vect_push_string(&out, "[rel ");
	else
		vect_push_string(&out, "[");
	vect_push_string(&out, base);
	
	if (offset != NULL && mult > 0) {
		vect_push_string(&out, " + ");
		vect_push_string(&out, offset);
		if(mult > 1) {
			vect_push_string(&out, "*");
			char *mstr = int_to_str(mult);
			vect_push_string(&out, mstr);
			free(mstr);
		}
	}

	if(add != 0) {
		if (add > 0) {
			vect_push_string(&out, " + ");
		} else {
			vect_push_string(&out, " - ");
			add = -add;
		}
		char *astr = int_to_str(add);
		vect_push_string(&out, astr);
		free(astr);
	}

	
	vect_push_string(&out, "]");

	return vect_as_string(&out);
}

// Type coercion engine
// TODO: all
Variable _op_coerce(Variable *base, Variable *to_coerce) {
	Variable out = {0};
	return out;
}

// TODO: Operations on variables
// remember for location: negative = on stack, zero = in DS, positive = in register
// Only values allowed in registers are in-built types and primitives (pointers/references)
// Other values must be in data section or on stack
// De-referencing = changing from a pointer to a reference
// referencing = changing from a variable to a pointer (can only be done for non-literals on stack or in DS)
// indexing returns a reference to the memory
// with an array => pointer + 8 bytes (length of array at start) + index * size of data
// with a pointer => pointer + index * size of data

// Get the last value from ptr_chain
int _var_ptr_type(Variable *v) {
	if(v->ptr_chain.count < 1) {
		// Return an invalid value if ptr_chain has no values.
		return PTYPE_NONE;
	}
	return ((int*)v->ptr_chain.data)[v->ptr_chain.count - 1];
}

// Get the first non-reference value from ptr_chain
int _var_first_nonref(Variable *v) {
	int *chk;
	for(size_t i = v->ptr_chain.count; i > 0; i--) {
		chk = vect_get(&v->ptr_chain, i - 1);
		if(*chk != PTYPE_REF) {
			return *chk;
		}
	}
	return -2;
}

// Valid registers to use in operations:
// rax (1), rdx (4), rsi (5), rdi (6).  Other registers assumed to be used by
// variables
// 1 - rax; 2 - rbx; 3 - rcx; 4 -  rdx; 5 - rsi; 6 - rdi; 7 - rsp; 8 - rbp; 9-16: r8-r15
char *_op_get_register(int reg, int size) {
	Vector out = vect_init(sizeof(char));
	char add = 'r';

	switch(size) {
		case 1:
		case 2:
			if (reg < 9)
				break;
		case 4:
			if (reg < 9)
				add = 'e';
		case 8:
			vect_push(&out, &add);
			break;
		default:
			printf("ERROR: invalid register size %d (this is a compiler issue)\n", size);
			vect_push(&out, &add);
			break;
	}

	switch(reg) {
		case 1:
		case 2:
		case 3:
		case 4:
			add = 'a' + (reg - 1);
			break;
		case 5:
		case 7:
			add = 's';
			break;
		case 6:
			add = 'd';
			break;
		case 8:
			add = 'b';
			break;
		default:

			if(reg > 8) {
				char *tmp = int_to_str(reg - 1);
				vect_push_string(&out, tmp);
				free(tmp);
			}
			add = 0;
			break;
	}

	if (add > 0)
		vect_push(&out, &add);

	add = 'x';
	switch (reg) {
		// Cases rsi, rdi, rsp, rbp need extra character
		case 5:
		case 6:
			add = 'i';
		case 7:
		case 8:
			if (add == 'x') {
				add = 'p';
			}
			if (size == 1) {
				vect_push(&out, &add);
			}
		case 1:
		case 2:
		case 3:
		case 4:
			// Add l if lower 8 bits, just push x otherwise
			if(size == 1) {
				add = 'l';
			}
			vect_push(&out, &add);
			break;
		default:
			// r8 - r15
			if (size == 1) {
				add = 'b';
			} else if (size == 2) {
				add = 'w';
			} else if (size == 4) {
				add = 'd';
			}

			if (size < 8) {
				vect_push(&out, &add);
			}
			break;
	}

	return vect_as_string(&out);
}

int _var_size(Variable *var) {
	if (var->location == LOC_LITL) {
		return -1;
	}
	size_t count = var->ptr_chain.count;
	int *ptype = vect_get(&var->ptr_chain, count - 1);

	while(count > 0 && *ptype == PTYPE_REF) {
	ptype = vect_get(&var->ptr_chain, --count);
	}

	if(count > 0) {
		return 8;
	}

	return var->type->size;
}

// Pure size in the sense that references count as pointers,
// so returns 8 if reference, pointer, or array.
int _var_pure_size(Variable *var) {
	if (var->location == LOC_LITL) {
		return -1;
	}

	if ( var->ptr_chain.count > 0 ) {
		return 8;
	}

	return var->type->size;
}

char *mod_label_prefix(Module *m);

// Get the full label in the data section
// for address generation
char *_var_get_datalabel(Variable *var) {
	Vector v = vect_from_string("");
	vect_push_free_string(&v, mod_label_prefix(var->mod));
	vect_push_string(&v, var->name);
	return vect_as_string(&v);
}

// Gets the location of a variable. Can not get the location
// properly if the variable is a reference.
char *_op_get_location(Variable *var) {
	char *out = NULL;
	
	if (var->location == LOC_LITL) {
		out = int_to_str(var->offset);
	} else if(var->location == LOC_STCK) {
		// Invert because stack grows down (and stack index starts at 1)
		out = _gen_address("", "rbp", "", 0, var->offset, false);
	} else if (var->location == LOC_DATA) {
		// Stored in data sec
		char *name = _var_get_datalabel(var);
		out = _gen_address("", name, "", 0, var->offset, true);
		free(name);
	} else {
		// Stored in register.  Our job here is not to assume
		// what it will be used for (in the case it is a reference)
		// so we use pure size
		out = _op_get_register(var->location, _var_pure_size(var));
	}

	return out;
}

// Can only be used on variables contained in a register.
void var_chg_register(CompData *out, Variable *swap, int new_reg) {
	if(swap->location == new_reg || swap->location == LOC_DATA || swap->location == LOC_STCK)
		return;

	if (swap->location == LOC_LITL) {
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_register(new_reg, 8));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, int_to_str(swap->offset));
		vect_push_string(&out->text, " ; Store literal\n\n");
		swap->offset = 0;
	} else {
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_register(new_reg, _var_pure_size(swap)));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, _op_get_register(swap->location, _var_pure_size(swap)));
		vect_push_string(&out->text, " ; Register swap\n\n");
	}
	
	swap->location = new_reg;
}

// Dereference a pointer variable into a new variable "store".
// Store is copied from "from" variable, so it should be empty (will not be freed).
// In other words, it takes a pointer variable and turns it into a reference,
// which can be used in other var operations like member variables, struct moving,
// or value setting.
void var_op_dereference(CompData *out, Variable *store, Variable *from) {
	*store = var_copy(from);
	if(from->ptr_chain.count < 1) {
		printf("WARNING: var_op_dereference called on variable which is not a pointer!\n");
		return;
	}

	// If we are not in a register, or we are a multiref not already in rsi
	if (from->location < 1 || (_var_ptr_type(from) == PTYPE_REF && from->location != 5 && from->ptr_chain.count > 1)) {
		// Generate initial move (from -> rsi)
		if (_var_ptr_type(from) > 1)
			// If in-place array, generate reference by lea
			vect_push_string(&out->text, "\tlea rsi, ");
		else
			// If pointer, generate reference by mov
			vect_push_string(&out->text, "\tmov rsi, ");
		
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, "; Move for dereference\n");
		// Location -> rsi
		store->location = 5;
	}
	
	if (from->location == LOC_LITL)
		store->offset = 0;

	// Keep de-referencing until we reach the pointer (or ptr_chain bottoms out).
	int *current = vect_get(&store->ptr_chain, store->ptr_chain.count - 1);
	while(*current == PTYPE_REF && store->ptr_chain.count > 1) {
		vect_push_string(&out->text, "\tmov rsi, [rsi] ; Dereference\n");
		vect_pop(&store->ptr_chain);
	}

	// pointer type -> ref
	current = vect_get(&store->ptr_chain, store->ptr_chain.count - 1);
	*current = PTYPE_REF;
}

// Index into an array by "index" elements.  Store the reference to the value in "store".
// The "store" variable should be freed or empty before calling this function.
void var_op_index(CompData *out, Variable *store, Variable *from, Variable *index) {
	// Generate a reference to the data we will output.
	// we will do pointer math on store based on ptype of from.
	var_op_dereference(out, store, from);

	
	// First, we'll calculate where the index is coming from
	char *idx_by = NULL;
	if (index->location == LOC_LITL) {
		idx_by = int_to_str(index->offset);
	} else if(_var_ptr_type(index) == PTYPE_REF) {
		vect_push_string(&out->text, "\tmov rdx, ");
		vect_push_string(&out->text, _op_get_location(index));
		vect_push_string(&out->text, " ; !!! DEREF IN INDEX !!!\n");
		
		int *cur;
		for(size_t i = index->ptr_chain.count - 1; i > 0; i--) {
			cur = vect_get(&index->ptr_chain, i - 1);
			if (*cur == PTYPE_REF) {
				vect_push_string(&out->text, "\tmov rdx, [rdx] ; deref\n");
			} else 
				break;
		}

		idx_by = _gen_address(PREFIXES[_var_size(index) - 1], "rdx", "", 0, 0, false);

	} else {
		if (index->location == LOC_STCK || index->location == LOC_DATA) {
			Vector tmp = vect_from_string(PREFIXES[index->type->size - 1]);
			vect_push_free_string(&tmp, _op_get_location(index));
			idx_by = vect_as_string(&tmp);
		} else {
			idx_by = _op_get_register(index->location, _var_size(index));
		}
	}


	// What type of move we will use on the index
	// to get it into rax
	switch(_var_size(index)) {
	case 8:
		// Standard move
		vect_push_string(&out->text, "\tmov rax, ");
		break;
	case 4:
		// mov into 4 byte register zeros out upper four bytes of the corrosponding 8 byte register
		vect_push_string(&out->text, "\tmov eax, ");
		break;
	case 2:
	case 1:
		// Zero extension
		vect_push_string(&out->text, "\tmovzx rax, ");
		break;
	default:
		vect_push_string(&out->text, "\tmov rax, ");
	}

	// Get index into rax
	vect_push_free_string(&out->text, idx_by);
	vect_push_string(&out->text, " ; Pre-index\n");
	
	if(_var_size(from) > 1) {
		// To multiply by the var size, we load the var size into rdx, then
		// multiply by it.
		vect_push_string(&out->text, "\tmov rdx, ");
		vect_push_free_string(&out->text, int_to_str(_var_size(from)));
		vect_push_string(&out->text, " ; Size of element held by ptr (pre-index)\n");
		
		vect_push_string(&out->text, "\tmul rdx ; Index multiplication by data size\n");
	}

	vect_push_string(&out->text, "\tlea rsi, ");
	if (_var_first_nonref(from) == PTYPE_PTR) {
		vect_push_free_string(&out->text, _gen_address("", "rsi", "rax", 1, 0, false));
	} else if (_var_first_nonref(from) == PTYPE_ARR) {
		// Additional offset due to arrays containing a length at the start
		vect_push_free_string(&out->text, _gen_address("", "rsi", "rax", 1, 8, false));
	} else {
		vect_push_string(&out->text, "rsi ; COMPILER ERROR!");
	}
	vect_push_string(&out->text, " ; Index complete.\n\n");
}

// Pure set simply copies data from one source to another, disregarding
// pointer arithmatic.  Should not be used to set the value of data reference variables
// point to, but can be used to directly set the location that reference
// variables point to for things like function parameter passing.
// Should only be used when you can garantee that both types are equal and
// you are not setting by reference
void var_op_pure_set(CompData *out, Variable *store, Variable *from) {
	if (store->location == LOC_LITL) {
		printf("ERROR: Can't set a literal value by pure set.\n");
		return;
	} else if (_var_pure_size(from) != _var_pure_size(store) && from->location != LOC_LITL) {
		printf("ERROR: Can't set one variable to the other as their pure sizes are different!\n");
		return;
	}

	if (from->location == LOC_LITL) {
		vect_push_string(&out->text, "\tmov ");

		if (store->location < 1) {
			// Must be done in case of a very large value
			vect_push_string(&out->text, "rsi, ");
			vect_push_free_string(&out->text, _op_get_location(from));
			vect_push_string(&out->text, "\n");
			// Store to data
			vect_push_string(&out->text, "\tmov ");
			vect_push_string(&out->text, PREFIXES[_var_pure_size(store) - 1]);
			vect_push_free_string(&out->text, _op_get_location(store));
			vect_push_string(&out->text, ", ");
			vect_push_free_string(&out->text, _op_get_register(5, _var_pure_size(store)));
			vect_push_string(&out->text, "; literal move\n\n");
		} else {
			vect_push_free_string(&out->text, _op_get_location(store));
			vect_push_string(&out->text, ", ");
			vect_push_free_string(&out->text, _op_get_location(from));
			vect_push_string(&out->text, "; literal move\n\n");
		}
		
	} else if (!is_inbuilt(from->type->name) && from->ptr_chain.count == 0) {
		// Pure struct move
		vect_push_string(&out->text, "\tlea rsi, ");
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, "\n");

		vect_push_string(&out->text, "\tlea rdi, ");
		vect_push_free_string(&out->text, _op_get_location(store));
		vect_push_string(&out->text, "\n");

		vect_push_string(&out->text, "\tmov rcx, ");
		vect_push_free_string(&out->text, int_to_str(_var_pure_size(from)));
		vect_push_string(&out->text, "\n");
		
		vect_push_string(&out->text, "\trep movsb ; Move struct complete\n\n");
	} else if (from->location < 1 && store->location < 1) {
		// Both in memory, use rsi as temp storage for move
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_register(5, _var_pure_size(from)));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, "\n");

		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_location(store));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, _op_get_register(5, _var_pure_size(from)));
		vect_push_string(&out->text, " ; Memory swap complete\n\n");

	} else {
		// Register to register
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_location(store));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, "; Register move\n\n");
	}
}


// Specific setting rules for pointers
void _var_op_set_ptr(CompData *out, Variable *store, Variable *from) {
	// Pointer coercion should always work
	char *mov_from;
	char *mov_to;

	// First deref from var, then deref store variable, then move.
	if(_var_ptr_type(store) != PTYPE_REF) {
		mov_to = _op_get_location(store);
	} else {
		// Need to deref
		vect_push_string(&out->text, "\tmov rdi, ");
		vect_push_free_string(&out->text, _op_get_location(store));
		vect_push_string(&out->text, " ; Move for ptr set dest deref\n");

		int *cur;
		for (size_t i = store->ptr_chain.count - 1; i > 0; i--) {
			cur = vect_get(&store->ptr_chain, i - 1);
			if (*cur == PTYPE_REF) {
				vect_push_string(&out->text, "\tmov rdi, [rdi]\n");
			} else {
				break;
			}
		}
		mov_to = _gen_address("", "rdi", "", 0, 0, false);
	}

	if (_var_ptr_type(from) != PTYPE_REF) {
		if (from->location > 0 || (_var_ptr_type(store) != PTYPE_REF && store->location > 0)) {
			mov_from = _op_get_location(from);
		} else {
			vect_push_string(&out->text, "\tmov rsi, ");
			vect_push_free_string(&out->text, _op_get_location(from));
			vect_push_string(&out->text, " ; Move for ptr set\n");
			mov_from = _op_get_register(5, 8);
		}
	} else {
		// Need to deref
		vect_push_string(&out->text, "\tmov rsi, ");
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, " ; Move for ptr set source deref\n");
		
		int *cur;
		for (size_t i = from->ptr_chain.count - 1; i > 0; i--) {
			cur = vect_get(&store->ptr_chain, i - 1);
			if (*cur == PTYPE_REF) {
				vect_push_string(&out->text, "\tmov rsi, [rsi]\n");
			} else {
				break;
			}
		}

		switch (_var_size(from)) {
		case 1:
			vect_push_string(&out->text, "\tmovzx rsi, byte [rsi]\n");
			break;
		case 2:
			vect_push_string(&out->text, "\tmovzx rsi, word [rsi]\n");
			break;
		case 4:
			vect_push_string(&out->text, "\tmov esi, dword [rsi]\n");
			break;
		case 8:
			vect_push_string(&out->text, "\tmov rsi, [rsi]\n");
			break;
		}

		mov_from = _op_get_register(5, 8);
	}

	vect_push_string(&out->text, "\tmov ");
	vect_push_free_string(&out->text, mov_to);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, mov_from);
	vect_push_string(&out->text, " ; Ptr set final\n\n");
	
	return;
}

char *_var_get_store(CompData *out, Variable *store) {
	if (_var_ptr_type(store) == PTYPE_REF){
		vect_push_string(&out->text, "\tmov rdi, ");
		vect_push_free_string(&out->text, _op_get_location(store));
		vect_push_string(&out->text, " ; pre-deref for inbuilt mov (store)\n");

		for(size_t i = store->ptr_chain.count - 1; i > 0; i--){
			int *cur = vect_get(&store->ptr_chain, i);
			if (cur == PTYPE_REF) {
				vect_push_string(&out->text, "\tmov rdi, [rdi] ; deref for mov\n");
			} else
				break; // Should not happen
		}

		return _gen_address(PREFIXES[_var_size(store) - 1], "rdi", "", 0, 0, false);
	} else if (store->location == 0) {
		char *name = _var_get_datalabel(store);
		return _gen_address(PREFIXES[_var_size(store) - 1], name, "", 0, 0, true);
		free(name);
	} else if (store->location == LOC_LITL) {
		vect_push_string(&out->text, "\tmov rdi, ");
		vect_push_free_string(&out->text, int_to_str(store->offset));
		vect_push_string(&out->text, "; litl set\n");
		if (store->type != NULL)
			return _op_get_register(6, _var_size(store));
		return _op_get_register(6, 8); 
	} else if (store->location < 0) {
		return _gen_address(PREFIXES[_var_size(store) - 1], "rbp", "", 0, store->offset, false);
	} else {
		return _op_get_location(store);
	}
}

char *_var_get_from(CompData *out, Variable *store, Variable *from) {
	char *mov_from = NULL;

	if (_var_ptr_type(from) == PTYPE_REF) {
		vect_push_string(&out->text, "\tmov rsi, ");
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, " ; pre-deref for inbuilt mov (from)\n");

		for(size_t i = from->ptr_chain.count - 1; i > 0; i--){
			int *cur = vect_get(&from->ptr_chain, i);
			if (cur == PTYPE_REF) {
				vect_push_string(&out->text, "\tmov rsi, [rsi] ; deref for mov\n");
			} else
				break; // Should not happen
		}
		// Final deref (store actual value in rsi)
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_register(5, _var_size(from)));
		vect_push_string(&out->text, ", ");
		vect_push_string(&out->text, PREFIXES[_var_size(from) - 1]);
		vect_push_string(&out->text, "[rsi] ; pre-deref for inbuilt mov (from)\n");

		mov_from = _op_get_register(5, _var_size(from));
		
	} else if (from->location > 0) {
		mov_from = _op_get_register(from->location, _var_size(from));
	} else if (from->location == LOC_LITL) {
		if (store->location == LOC_LITL) {
			vect_push_string(&out->text, "\tmov rsi, ");
			vect_push_free_string(&out->text, int_to_str(from->offset));
			vect_push_string(&out->text, "; litl set\n");
			if (store->type != NULL)
				return _op_get_register(5, store->type->size);
			return _op_get_register(5, 8); 
		} else if (store->location < 1 || _var_ptr_type(store) == PTYPE_REF) {
			vect_push_string(&out->text, "\tmov ");
			vect_push_free_string(&out->text, _op_get_register(5, _var_size(store)));
			vect_push_string(&out->text, ", ");
			vect_push_free_string(&out->text, int_to_str(from->offset));
			vect_push_string(&out->text, "; litl set for inbuilt mov (from)\n");
			mov_from = _op_get_register(5, _var_size(store));
		} else {
			mov_from = int_to_str(from->offset);
		}
	} else if (store->location < 1 || _var_ptr_type(store) == PTYPE_REF) {
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_register(5, _var_size(from)));
		vect_push_string(&out->text, ", ");
		vect_push_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, " ; pre-load for mov (from)\n");

		mov_from = _op_get_register(5, _var_size(from));
	} else if (from->location == 0) {
		// from in data sec
		char *name = _var_get_datalabel(from);
		mov_from = _gen_address(PREFIXES[_var_size(from) - 1], name, "", 0, 0, true);
		free(name);
	} else {
		// from on stack
		mov_from = _gen_address(PREFIXES[_var_size(from) - 1], "rbp", "", 0, from->offset, false);
	}

	// Match sign of data if required.
	if (from->location != LOC_LITL && _var_size(from) < _var_size(store)) {
		// Store larger than from (extend sign)
		if(from->type->name[0] == 'i' && store->type->name[0] == 'i') {
			if (_var_size(from) < 4)
				vect_push_string(&out->text, "\tmovsx rsi, ");
			else
				vect_push_string(&out->text, "\tmovsxd rsi, ");
		} else {
			if(_var_size(from) < 4)
				vect_push_string(&out->text, "\tmovzx rsi, ");
			else
				vect_push_string(&out->text, "\tmovzxd rsi, ");
		}
		vect_push_free_string(&out->text, mov_from);
		vect_push_string(&out->text, " ; Sign extension for mov\n");
		mov_from = _op_get_register(5, _var_size(store));
	} else if (from->location != LOC_LITL && _var_size(from) > _var_size(store)) {
		// Store smaller than from (recompute mov_from)
		if (_var_ptr_type(from) == PTYPE_REF) {
		} else if (from->location > 0) {
			free(mov_from);
			mov_from = _op_get_register(from->location, _var_size(store));
		}
	}

	return mov_from;
}

// Common func to move one variable to another in the case of two
// inbuilts
void _var_op_set_inbuilt(CompData *out, Variable *store, Variable *from) {
	char *mov_from;
	char *mov_to;
	
	// Cases for source/dest:
	// register
	// stack
	// data
	// reference
	
	// Cases for types
	// uint - range from 1 to 8 bytes - zx expansion
	// int - range from 1 to 8 bytes - sx expansion
	// bool - always 1 byte
	// float - not impl
	// void - should only be used to represent ptrs, so we should not see it here.
	
	// In case of references

	mov_to = _var_get_store(out, store);
	mov_from = _var_get_from(out, store, from);

	vect_push_string(&out->text, "\tmov ");
	vect_push_free_string(&out->text, mov_to);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, mov_from);
	vect_push_string(&out->text, " ; Finish mov_inbuilt\n\n");
}

// Tries it's best to coerce the data from "from" into a format
// which will be accepted by "store", following refences in the process.
// This is similar to a normal move operation.
void var_op_set(CompData *out, Variable *store, Variable *from) {
	if(_var_first_nonref(store) == PTYPE_PTR || _var_first_nonref(store) == PTYPE_ARR) {
		_var_op_set_ptr(out, store, from);
		return;
	}

	if (store->location == LOC_LITL) {
		if (from->location == LOC_LITL)
			store->offset = from->offset;
		else
			printf("ERROR: Unable to set a non-literal equal to a literal value\n\n");
		return;
	}

	if (from->location == LOC_LITL) {
		if (is_inbuilt(store->type->name) == true)
			_var_op_set_inbuilt(out, store, from);
		else
			printf("ERROR: Unable to set a struct equal to a literal value\n\n");
		return;
	}

	if(is_inbuilt(store->type->name) != is_inbuilt(from->type->name)) {
		printf("ERROR: Unable to coerce types when one is inbuilt and the other is not.\n\n");
		return;
	}
	
	if(is_inbuilt(from->type->name)) {
		// Inbuilt types like int, uint, void, etc.
		_var_op_set_inbuilt(out, store, from);
	} else {
		// Two structs, we should only copy as much data as we can from one to another,
		// and so will defer to the storage variable for how much to transfer
		
		// since we will be using movsb, we first should mov the from struct into
		// rsi.
		if (from->location < 1) {
			vect_push_string(&out->text, "\tlea rsi, ");
		} else {
			vect_push_string(&out->text, "\tmov rsi, ");
		}
		vect_push_free_string(&out->text, _op_get_location(from));
		vect_push_string(&out->text, " ; Initial mov to rsi\n");
		
		// Handle the case where the from struct is a reference
		size_t i = from->ptr_chain.count;
		if (from->location > 0)
			i--;

		for (; i > 0; i--) {
			int *cur = vect_get(&from->ptr_chain, i - 1);
			if (*cur == PTYPE_REF)
				vect_push_string(&out->text, "\tlea rsi, [rsi] ; Deref\n");
			else
				break;
		}

		// load the location of the storeage var into rdi
		if (store->location < 1) {
			vect_push_string(&out->text, "\tlea rdi, ");
		} else {
			vect_push_string(&out->text, "\tmov rdi, ");
		}
		vect_push_free_string(&out->text, _op_get_location(store));
		vect_push_string(&out->text, " ; Initial mov to rdi\n");

		i = store->ptr_chain.count;
		if (store->location > 0)
			i--;

		for (; i > 0; i--) {
			int *cur = vect_get(&store->ptr_chain, i - 1);
			if (*cur == PTYPE_REF)
				vect_push_string(&out->text, "\tlea rsi, [rsi] ; Deref\n");
			else
				break;
		}

		// We can move up to the minimum number of bytes btwn the two structs
		vect_push_string(&out->text, "\tmov rcx, ");
		if (_var_size(from) < _var_size(store)) {
			vect_push_free_string(&out->text, int_to_str(_var_size(from)));
		} else {
			vect_push_free_string(&out->text, int_to_str(_var_size(store)));
		}
		vect_push_string(&out->text, "\n\trep movsb ; Complete struct move\n\n");
	}
}

// Take a variable on the stack or in data section, and load a reference
// to it's location in memory
void var_op_reference(CompData *out, Variable *store, Variable *from) {
	if (from->location == LOC_LITL) {
		printf("ERROR: Unable to do dereference with literal value\n\n");
		return;
	}
	
	*store = var_copy(from);
	if (store->ptr_chain.count > 0) {
		for(size_t i = 0; i < store->ptr_chain.count; i++) {
			int *cur = vect_get(&store->ptr_chain, i);
			if (*cur == PTYPE_REF) {
				*cur = PTYPE_PTR;
				break;
			}
		}
	} else {
		int ref = PTYPE_REF;
		vect_push(&store->ptr_chain, &ref);
	}
	store->location = 5;
	store->offset = 0;

	if(from->ptr_chain.count > 0 && _var_ptr_type(from) == PTYPE_REF) {
		// The value is a reference to data, so we should just copy it
		var_op_pure_set(out, store, from);
		return;
	}

	if(from->location > 0) {
		// Can't generate reference if variable is in register
		printf("FATAL: (Compiler error) attempt to generate reference to a variable in a register.\n");
		return;
	}

	vect_push_string(&out->text, "\tlea ");
	vect_push_free_string(&out->text, _op_get_register(5, _var_pure_size(store)));
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, _op_get_location(from));
	vect_push_string(&out->text, " ; Generate reference\n");
	vect_push_string(&out->text, "\n");
}

Variable var_op_member(CompData *data, Variable *from, char *member) {
	Variable out = {0};
	out.name = NULL; // This is how you can check weather we succeeded
	
	if (from->location == LOC_LITL) {
		printf("ERROR: Unable to take member from literal value");
		return out;
	}
	
	if (from->ptr_chain.count > 1 || (from->ptr_chain.count == 1 && _var_ptr_type(from) != PTYPE_REF)) {
		printf("ERROR: Can't generate member for pointer!\n");
		return out;
	}

	for (size_t i = 0; i < from->type->members.count; i++) {
		Variable *mem = vect_get(&from->type->members, i);
		if (strcmp(mem->name, member) == 0) {
			out = var_copy(mem);
			break;
		}
	}
	
	if (out.name == NULL)
		return out;

	// Copy ptr_chain so when using the variable we follow all references
	for(size_t i = 0; i < from->ptr_chain.count; i++) {
		int *cur = vect_get(&from->ptr_chain, i);
		vect_push(&out.ptr_chain, cur);
	}
	
	// Copy location
	out.location = from->location;
	
	// If the variable is in the data section, we should copy
	// the name as well so references are properly handled
	if (out.location != 5 && _var_ptr_type(&out) == PTYPE_REF) {
		out.location = 5;
		var_op_pure_set(data, &out, from);
	} else if(out.location == LOC_DATA) {
		free(out.name);
		Vector name = vect_from_string(from->name);
		out.name = vect_as_string(&name);
	}

	if (out.location == 5 && out.offset > 0) {
		vect_push_string(&data->text, "\tadd rsi, ");
		vect_push_free_string(&data->text, int_to_str(out.offset));
		vect_push_string(&data->text, "; Member generation in reference\n");
		out.offset = 0;
	} else {
		// If from is already offset, we should base our new offset on the old one.
		out.offset += from->offset;
	}


	return out;
}

// Adds "base" with "add" and sets "base" to the result
void var_op_add(CompData *out, Variable *base, Variable *add) {

	if(base->location == LOC_LITL) {
		if (add->location == LOC_LITL)
			base->offset += add->offset;
		return;
	}

	char *add_store;
	char *add_from;

	add_store = _var_get_store(out, base);
	add_from = _var_get_from(out, base, add);

	vect_push_string(&out->text, "\tadd ");
	vect_push_free_string(&out->text, add_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, add_from);
	vect_push_string(&out->text, "; complete add\n\n");
}

// Subtracts "sub" from "base" and sets "base" to the result
void var_op_sub(CompData *out, Variable *base, Variable *sub) {
	if(base->location == LOC_LITL) {
		if (sub->location == LOC_LITL)
			base->offset -= sub->offset;
		return;
	}

	char *sub_store;
	char *sub_from;

	sub_store = _var_get_store(out, base);
	sub_from = _var_get_from(out, base, sub);

	vect_push_string(&out->text, "\tsub ");
	vect_push_free_string(&out->text, sub_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, sub_from);
	vect_push_string(&out->text, "; complete sub\n\n");
}

// Ands "base" with "and" and sets "base" to the result
void var_op_and(CompData *out, Variable *base, Variable *and) {

	if(base->location == LOC_LITL) {
		if (and->location == LOC_LITL)
			base->offset &= and->offset;
		return;
	}

	char *and_store;
	char *and_from;

	and_store = _var_get_store(out, base);
	and_from = _var_get_from(out, base, and);

	vect_push_string(&out->text, "\tand ");
	vect_push_free_string(&out->text, and_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, and_from);
	vect_push_string(&out->text, "; complete and\n\n");
}

// Ors "base" with "or" and sets "base" to the result
void var_op_or(CompData *out, Variable *base, Variable *or) {

	if(base->location == LOC_LITL) {
		if (or->location == LOC_LITL)
			base->offset |= or->offset;
		return;
	}

	char *or_store;
	char *or_from;

	or_store = _var_get_store(out, base);
	or_from = _var_get_from(out, base, or);

	vect_push_string(&out->text, "\tor ");
	vect_push_free_string(&out->text, or_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, or_from);
	vect_push_string(&out->text, "; complete or\n\n");
}

// Xors "base" with "xor" and sets "base" to the result
void var_op_xor(CompData *out, Variable *base, Variable *xor) {

	if(base->location == LOC_LITL) {
		if (xor->location == LOC_LITL)
			base->offset ^= xor->offset;
		return;
	}

	char *xor_store;
	char *xor_from;

	xor_store = _var_get_store(out, base);
	xor_from = _var_get_from(out, base, xor);

	vect_push_string(&out->text, "\txor ");
	vect_push_free_string(&out->text, xor_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, xor_from);
	vect_push_string(&out->text, "; complete xor\n\n");
}

// nors "base" with "nor" and sets "base" to the result
void var_op_nor(CompData *out, Variable *base, Variable *nor) {

	if(base->location == LOC_LITL) {
		if (nor->location == LOC_LITL)
			base->offset |= ~nor->offset;
		return;
	}

	char *nor_store;
	char *nor_from;

	nor_store = _var_get_store(out, base);
	nor_from = _var_get_from(out, base, nor);

	vect_push_string(&out->text, "\tor ");
	vect_push_free_string(&out->text, nor_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, nor_from);
	vect_push_string(&out->text, "\n\tnot ");
	vect_push_free_string(&out->text, nor_store);
	vect_push_string(&out->text, " ; Complete nor\n");
}

// nands "base" with "nand" and sets "base" to the result
void var_op_nand(CompData *out, Variable *base, Variable *nand) {

	if(base->location == LOC_LITL) {
		if (nand->location == LOC_LITL) {
			base->offset &= ~nand->offset;
		}
		return;
	}

	char *nand_store = _var_get_store(out, base);
	char *nand_from = _var_get_from(out, base, nand);

	vect_push_string(&out->text, "\tand ");
	vect_push_free_string(&out->text, nand_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, nand_from);
	vect_push_string(&out->text, "\n\tnot ");
	vect_push_free_string(&out->text, nand_store);
	vect_push_string(&out->text, " ; Complete nand\n");
}

// xands "base" with "xand" and sets "base" to the result
void var_op_xand(CompData *out, Variable *base, Variable *xand) {

	if(base->location == LOC_LITL) {
		if (xand->location == LOC_LITL)
			base->offset ^= ~xand->offset;
		return;
	}

	char *xand_store = _var_get_store(out, base);
	char *xand_from = _var_get_from(out, base, xand);

	vect_push_string(&out->text, "\txor ");
	vect_push_free_string(&out->text, xand_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, xand_from);
	vect_push_string(&out->text, "\n\tnot ");
	vect_push_free_string(&out->text, xand_store);
	vect_push_string(&out->text, " ; Complete xand\n");
}

// bit inversion of base
void var_op_not(CompData *out, Variable *base) {

	if(base->location == LOC_LITL) {
		base->offset = ~base->offset;
		return;
	}

	char *not_store = _var_get_store(out, base);

	vect_push_string(&out->text, "\tnot ");
	vect_push_free_string(&out->text, not_store);
	vect_push_string(&out->text, " ; Complete not\n");
}

// test base with itself
void var_op_test(CompData *out, Variable *base) {

	if(base->location == LOC_LITL) {
		vect_push_string(&out->text, "\tmov rax, ");
		vect_push_free_string(&out->text, int_to_str(base->offset));
		vect_push_string(&out->text, "\n");
		vect_push_string(&out->text, "\ttest rax, rax ; lit test\n\n");
		return;
	}

	char *test_store = _var_get_store(out, base);
	char *test_from = _var_get_from(out, base, base);

	vect_push_string(&out->text, "\ttest ");
	vect_push_free_string(&out->text, test_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, test_from);
	vect_push_string(&out->text, " ; Complete not\n");
}

// bit shift base left by "bsl"
void var_op_bsl(CompData *out, Variable *base, Variable *bsl) {

	if(base->location == LOC_LITL) {
		if (bsl->location == LOC_LITL)
			base->offset = base->offset << bsl->offset;
		return;
	}

	char *bsl_store = _var_get_store(out, base);

	char *bsl_from;
	if (bsl->location == LOC_LITL) {
		bsl_from = int_to_str(bsl->offset % 128);
	} else {
		Variable cx = var_copy(bsl);
		cx.offset = 0;
		cx.mod = NULL;
		cx.location = 3;
		var_op_pure_set(out, &cx, bsl);
		var_end(&cx);
		bsl_from = _op_get_register(3, 1);
	}
	
	// Signed and unsigned shift are the same
	vect_push_string(&out->text, "\tshl ");
	vect_push_free_string(&out->text, bsl_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, bsl_from);
	vect_push_string(&out->text, " ; Complete shift left\n");
}

// bit shift base right by "bsr"
void var_op_bsr(CompData *out, Variable *base, Variable *bsr) {

	if(base->location == LOC_LITL) {
		if (bsr->location == LOC_LITL)
			base->offset = base->offset >> bsr->offset;
		return;
	}

	char *bsr_store = _var_get_store(out, base);
	char *bsr_from;
	if (bsr->location == LOC_LITL) {
		bsr_from = int_to_str(bsr->offset % 256);
	} else {
		Variable cx = var_copy(bsr);
		cx.offset = 0;
		cx.mod = NULL;
		cx.location = 3;
		var_op_pure_set(out, &cx, bsr);
		var_end(&cx);
		bsr_from = _op_get_register(3, 1);
	}
	
	if (is_inbuilt(base->type->name) && base->type->name[0] == 'i') {
		// integer shift
		vect_push_string(&out->text, "\tsar ");
	} else {
		// unsigned shift
		vect_push_string(&out->text, "\tshr ");
	}
	vect_push_free_string(&out->text, bsr_store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, bsr_from);
	vect_push_string(&out->text, " ; Complete not\n");
}

Variable var_op_cmpbase(CompData *out, Variable *base, Variable *cmp, char *cc) {

	char *store = _var_get_store(out, base);

	int tmp_loc = base->location;
	if (cmp->location == LOC_LITL) {
		base->location = LOC_LITL;
	}
	
	char *from = _var_get_from(out, base, cmp);
	
	if (cmp->location == LOC_LITL) {
		base->location = tmp_loc;
	}

	// cmp
	vect_push_string(&out->text, "\tcmp ");
	vect_push_free_string(&out->text, store);
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, from);
	vect_push_string(&out->text, "\n");
	
	// prime rax and rdx
	vect_push_string(&out->text, "\tmov rax, 0\n");
	vect_push_string(&out->text, "\tmov rdx, 1\n");
	
	// Store bool value in rax and test so jumps make sense
	vect_push_string(&out->text, "\tcmov");
	vect_push_string(&out->text, cc);
	vect_push_string(&out->text, " rax, rdx ; bool gen\n");
	vect_push_string(&out->text, "\ttest rax, rax ; less than test\n\n");

	// Generate variable
	Variable v = var_init("#bool", typ_get_inbuilt("bool"));
	v.location = 1;
	return v;
}

void var_op_le(CompData *out, Variable *base, Variable *cmp) {
	Variable tmp;
	if (base->location == LOC_LITL || (is_inbuilt(base->type->name) && base->type->name[0] == 'i')) {
		// int compare
		tmp = var_op_cmpbase(out, base, cmp, "le");
	} else {
		tmp = var_op_cmpbase(out, base, cmp, "be");
	}
	var_end(base);
	*base = tmp;
}

void var_op_ge(CompData *out, Variable *base, Variable *cmp) {
	Variable tmp;
	if (base->location == LOC_LITL || (is_inbuilt(base->type->name) && base->type->name[0] == 'i')) {
		// int compare
		tmp = var_op_cmpbase(out, base, cmp, "ge");
	} else {
		tmp = var_op_cmpbase(out, base, cmp, "ae");
	}
	var_end(base);
	*base = tmp;
}

void var_op_lt(CompData *out, Variable *base, Variable *cmp) {
	Variable tmp;
	if (base->location == LOC_LITL || (is_inbuilt(base->type->name) && base->type->name[0] == 'i')) {
		// int compare
		tmp = var_op_cmpbase(out, base, cmp, "l");
	} else {
		tmp = var_op_cmpbase(out, base, cmp, "b");
	}
	var_end(base);
	*base = tmp;
}

void var_op_gt(CompData *out, Variable *base, Variable *cmp) {
	Variable tmp;
	if (base->location == LOC_LITL || (is_inbuilt(base->type->name) && base->type->name[0] == 'i')) {
		// int compare
		tmp = var_op_cmpbase(out, base, cmp, "g");
	} else {
		tmp = var_op_cmpbase(out, base, cmp, "a");
	}
	var_end(base);
	*base = tmp;
}

void var_op_eq(CompData *out, Variable *base, Variable *cmp) {
	Variable tmp = var_op_cmpbase(out, base, cmp, "e");
	var_end(base);
	*base = tmp;
}

void var_op_ne(CompData *out, Variable *base, Variable *cmp) {
	Variable tmp = var_op_cmpbase(out, base, cmp, "ne");
	var_end(base);
	*base = tmp;
}

// Boolean and
void var_op_band(CompData *out, Scope *s) {
	// Just parsed the left side, short circuit if zero
	vect_push_string(&out->text, "\tjz ");
	vect_push_free_string(&out->text, scope_gen_bool_label(s));
	vect_push_string(&out->text, "false ; bool and\n\n");
}

// Boolean or
void var_op_bor(CompData *out, Scope *s) {
	// Just parsed the left side, short circuit if zero
	vect_push_string(&out->text, "\tjnz ");
	vect_push_free_string(&out->text, scope_gen_bool_label(s));
	vect_push_string(&out->text, "true ; bool or\n\n");
}

void var_op_bxor(CompData *out, Variable *lhs, Variable *rhs) {
	// Nothing for now
}

// Multiplies "base" by "mul" and sets "base" to the result.
void var_op_mul(CompData *out, Variable *base, Variable *mul) {
	if(base->location == LOC_LITL) {
		if (mul->location == LOC_LITL)
			base->offset *= mul->offset;
		return;
	}

	if (mul->location == 1) {
		var_chg_register(out, mul, 3);
	}

	if(base->type->name[0] == 'i') {
		// Integer multiplication
		if (base->location > 0 && mul->location != LOC_LITL) {
			char *store = _var_get_store(out, base);
			char *from = _var_get_from(out, base, mul);

			vect_push_string(&out->text, "\timul ");
			vect_push_free_string(&out->text, store);
			vect_push_string(&out->text, ", ");
			vect_push_free_string(&out->text, from);
			vect_push_string(&out->text, "; complete mul\n\n");
		} else if (base->location > 0) {
			vect_push_string(&out->text, "\tmov rcx, ");
			vect_push_free_string(&out->text, int_to_str(mul->offset));
			vect_push_string(&out->text, "; literal load\n");

			char *store = _var_get_store(out, base);

			vect_push_string(&out->text, "\timul ");
			vect_push_free_string(&out->text, store);
			vect_push_string(&out->text, ", ");
			vect_push_free_string(&out->text, _op_get_register(3, _var_size(base)));
			vect_push_string(&out->text, "; complete mul\n\n");
		} else {
			char *store = _var_get_store(out, base);

			// Mov to rax for the multiplication, move back after.
			vect_push_string(&out->text, "\tmov ");
			vect_push_free_string(&out->text, _op_get_register(1, _var_size(base)));
			vect_push_string(&out->text, ", ");
			vect_push_string(&out->text, store);
			vect_push_string(&out->text, "; pre-mul mov\n");

			if (mul->location == LOC_LITL) {
				vect_push_string(&out->text, "\tmov rcx, ");
				vect_push_free_string(&out->text, int_to_str(mul->offset));
				vect_push_string(&out->text, "; literal load\n");

				vect_push_string(&out->text, "\timul ");
				vect_push_free_string(&out->text, _op_get_register(3, _var_size(base)));
				vect_push_string(&out->text, "; mul\n");
			} else {
				char *from = _var_get_from(out, base, mul);
				vect_push_string(&out->text, "\timul ");
				vect_push_free_string(&out->text, from);
				vect_push_string(&out->text, "; mul\n");
			}
			
			// move back after mul
			vect_push_string(&out->text, "\tmov ");
			vect_push_free_string(&out->text, store);
			vect_push_string(&out->text, ", ");
			vect_push_free_string(&out->text, _op_get_register(1, _var_size(base)));
			vect_push_string(&out->text, "; post-mul mov\n");
		}
	} else {
		char *store = _var_get_store(out, base);
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _op_get_register(1, _var_size(base)));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, store);
		vect_push_string(&out->text, "; pre-mul mov\n");

		if (mul->location == LOC_LITL) {
			vect_push_string(&out->text, "\tmov rcx, ");
			vect_push_free_string(&out->text, int_to_str(mul->offset));
			vect_push_string(&out->text, "; literal load\n");

			vect_push_string(&out->text, "\tmul ");
			vect_push_free_string(&out->text, _op_get_register(3, _var_size(base)));
			vect_push_string(&out->text, "; mul\n");
		} else {
			char *from = _var_get_from(out, base, mul);
			vect_push_string(&out->text, "\tmul ");
			vect_push_free_string(&out->text, from);
			vect_push_string(&out->text, "; mul\n");
		}
		
		// move back after mul
		vect_push_string(&out->text, "\tmov ");
		vect_push_free_string(&out->text, _var_get_store(out, base));
		vect_push_string(&out->text, ", ");
		vect_push_free_string(&out->text, _op_get_register(1, _var_size(base)));
		vect_push_string(&out->text, "; post-mul mov\n");
	}
}

// Divides "base" by "div" and sets "base" to the result
void var_op_div(CompData *out, Variable *base, Variable *div) {
	if(base->location == LOC_LITL) {
		if (div->location == LOC_LITL)
			base->offset /= div->offset;
		return;
	}

	// zero out rdx before divide
	vect_push_string(&out->text, "\txor rdx, rdx ; Clear rdx for divide\n");

	char *div_by;
	if (base->type->name[0] == 'i') {
		// mov into rax
		switch(_var_size(base)) {
		case 4:
			vect_push_string(&out->text, "\tmovsxd rax, ");
			break;
		case 8:
			vect_push_string(&out->text, "\tmov rax, ");
			break;
		default:
			vect_push_string(&out->text, "\tmovsx rax, ");
			break;
		}
		vect_push_free_string(&out->text, _var_get_store(out, base));
		vect_push_string(&out->text, "; initial mov\n\n");

		// Calculate div_by
		if(_var_size(base) > _var_size(div) && div->location == LOC_LITL) {
			vect_push_string(&out->text, "\tmov rcx, ");
			vect_push_free_string(&out->text, int_to_str(div->offset));
			div_by = _op_get_register(3, _var_size(base));

		} else {
			div_by = _var_get_from(out, base, div);
		}

		// Do div
		vect_push_string(&out->text, "\tidiv ");
		vect_push_free_string(&out->text, div_by);
		vect_push_string(&out->text, "; div\n");

	} else {
		// mov into rax
		switch(_var_size(base)) {
		case 4:
			vect_push_string(&out->text, "\tmov eax, ");
			break;
		case 8:
			vect_push_string(&out->text, "\tmov rax, ");
			break;
		default:
			vect_push_string(&out->text, "\tmovzx rax, ");
		}
		vect_push_free_string(&out->text, _var_get_store(out, base));
		vect_push_string(&out->text, "; initial mov\n\n");
		
		// Calculate div by
		if(_var_size(base) > _var_size(div) && div->location == LOC_LITL) {
			vect_push_string(&out->text, "\tmov rcx, ");
			vect_push_free_string(&out->text, int_to_str(div->offset));
			div_by = _op_get_register(3, _var_size(base));

		} else {
			div_by = _var_get_from(out, base, div);
		}

		// Do div
		vect_push_string(&out->text, "\tdiv ");
		vect_push_free_string(&out->text, div_by);
		vect_push_string(&out->text, "; div\n");
	}

	// Mov back to base
	vect_push_string(&out->text, "\tmov ");
	vect_push_free_string(&out->text, _var_get_store(out, base));
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, _op_get_register(1, _var_size(base)));
	vect_push_string(&out->text, "; final mov for div\n\n");

}

// Divides "base" by "mod" and sets "base" to the remainder
void var_op_mod(CompData *out, Variable *base, Variable *mod) {
	if(base->location == LOC_LITL) {
		if (mod->location == LOC_LITL)
			base->offset %= mod->offset;
		return;
	}

	// zero out rdx before divide
	vect_push_string(&out->text, "\txor rdx, rdx ; Clear rdx for divide\n");
	
	char *div_by;
	if (base->type->name[0] == 'i') {
		// mov into rax
		switch(_var_size(base)) {
		case 4:
			vect_push_string(&out->text, "\tmovsxd rax, ");
			break;
		case 8:
			vect_push_string(&out->text, "\tmov rax, ");
			break;
		default:
			vect_push_string(&out->text, "\tmovsx rax, ");
			break;
		}
		vect_push_free_string(&out->text, _var_get_store(out, base));
		vect_push_string(&out->text, "; initial mov\n\n");

		// Calculate div_by
		if(_var_size(base) > _var_size(mod) && mod->location == LOC_LITL) {
			vect_push_string(&out->text, "\tmov rcx, ");
			vect_push_free_string(&out->text, int_to_str(mod->offset));
			div_by = _op_get_register(3, _var_size(base));

		} else {
			div_by = _var_get_from(out, base, mod);
		}

		// Do div
		vect_push_string(&out->text, "\tidiv ");
		vect_push_free_string(&out->text, div_by);
		vect_push_string(&out->text, "; div\n");

	} else {
		// mov into rax
		switch(_var_size(base)) {
		case 4:
			vect_push_string(&out->text, "\tmov eax, ");
			break;
		case 8:
			vect_push_string(&out->text, "\tmov rax, ");
			break;
		default:
			vect_push_string(&out->text, "\tmovzx rax, ");
		}
		vect_push_free_string(&out->text, _var_get_store(out, base));
		vect_push_string(&out->text, "; initial mov\n\n");

		// Calculate div by
		if(_var_size(base) > _var_size(mod) && mod->location == LOC_LITL) {
			vect_push_string(&out->text, "\tmov rcx, ");
			vect_push_free_string(&out->text, int_to_str(mod->offset));
			div_by = _op_get_register(3, _var_size(base));

		} else {
			div_by = _var_get_from(out, base, mod);
		}

		// Do div
		vect_push_string(&out->text, "\tdiv ");
		vect_push_free_string(&out->text, div_by);
		vect_push_string(&out->text, "; div\n");
	}

	// Mov back to base
	vect_push_string(&out->text, "\tmov ");
	vect_push_free_string(&out->text, _var_get_store(out, base));
	vect_push_string(&out->text, ", ");
	vect_push_free_string(&out->text, _op_get_register(4, _var_size(base)));
	vect_push_string(&out->text, "; final mov for mod\n\n");
}



// Functions

Function func_init(char *name, Module *module) {
	Function out = {0};

	Vector name_cpy = vect_from_string(name);
	out.name = vect_as_string(&name_cpy);
	out.module = module;
	out.inputs = vect_init(sizeof(Variable));
	out.outputs = vect_init(sizeof(Variable));

	return out;
}

void func_end(Function *func) {
	free(func->name);
	func->module = NULL;

	for(size_t i = 0; i < func->inputs.count; i++) {
		Variable *to_end = vect_get(&(func->inputs), i);
		var_end(to_end);
	}
	vect_end(&(func->inputs));

	for(size_t i = 0; i < func->outputs.count; i++) {
		Variable *to_end = vect_get(&(func->outputs), i);
		var_end(to_end);
	}
	vect_end(&(func->outputs));
}



// Modules

Module mod_init(char *name, Module *parent, bool export) {
	Module out = {0};

	Vector name_cpy = vect_from_string(name);
	out.name = vect_as_string(&name_cpy);
	out.parent = parent;
	out.exported = export;

	out.types = vect_init(sizeof(Type));
	out.vars = vect_init(sizeof(Variable));
	out.funcs = vect_init(sizeof(Function));
	out.submods = vect_init(sizeof(Module));

	return out;
}

#define FT_VAR 0
#define FT_FUN 1
#define FT_TYP 2

void *mod_find_rec(Module *mod, Artifact *art, size_t sub, int find_type) {
	// Not at end of art, need to go deeper
	if (sub + 1 < art->count) {
		char **to_check = vect_get(art, sub);

		Vector e_check = vect_from_string("@@"); // In case it is a variable inside an enum
		Vector t_check = vect_from_string("_#"); // In case it is a function inside a method block
		vect_push_string(&e_check, *to_check);
		vect_as_string(&e_check);
		vect_push_string(&t_check, *to_check);
		vect_as_string(&t_check);
		
		void *out = NULL;
		for (size_t i = 0; i < mod->submods.count; i++) {
			Module *m = vect_get(&(mod->submods), i);
			if (strcmp(m->name, *to_check) == 0 || strcmp(m->name, e_check.data) == 0 || strcmp(m->name, t_check.data)) {
				out = mod_find_rec(m, art, sub + 1, find_type);
				break;
			}
		}

		vect_end(&e_check);
		vect_end(&t_check);

		if (out != NULL)
			return out;
	} else if (art->count > 0) {
		Vector *search = NULL;
		char **to_check = vect_get(art, art->count - 1);

		switch(find_type) {
		case FT_VAR:
			search = &(mod->vars);
			break;
		case FT_FUN:
			search = &(mod->funcs);
			break;
		case FT_TYP:
			search = &(mod->types);
			break;
		default:
			printf("FATAL: Compiler error, mod_find_rec called with find_type value %d\n", find_type);
			return NULL;
		}

		for (size_t i = 0; i < search->count;i++) {
			void *e = vect_get(search, i);
			if (find_type == FT_VAR && strcmp(((Variable *)e)->name, *to_check) == 0) {
				return e;
			} else if (find_type == FT_FUN && strcmp(((Function *)e)->name, *to_check) == 0) {
				return e;
			} else if (find_type == FT_TYP && strcmp(((Type *)e)->name, *to_check) == 0) {
				return e;
			}
		}
	}

	if (mod->parent == NULL || sub > 0)
		return NULL;

	return mod_find_rec(mod->parent, art, 0, find_type);
}

Type *mod_find_type(Module *mod, Artifact *art) {
	Type *out = NULL;
	
	if (art->count == 1) {
		char ** name = vect_get(art, 0);
		out = typ_get_inbuilt(*name);
	}

	if (out == NULL)
		out = mod_find_rec(mod, art, 0, FT_TYP);
	
	return out;
}

Function *mod_find_func(Module *mod, Artifact *art) {
	return mod_find_rec(mod, art, 0, FT_FUN);
}

Variable *mod_find_var(Module *mod, Artifact *art) {
	return mod_find_rec(mod, art, 0, FT_VAR);
}

Module *mod_find_sub(Module *mod, char *chk) {
	for(size_t i = 0; i < mod->submods.count; i++) {
		Module *m = vect_get(&mod->submods, i);
		if(strcmp(m->name, chk) == 0)
				return m;
	}
	return NULL;
}

void mod_full_path_rec(Module *m, Vector *v) {
	if(m->parent != NULL)
		mod_full_path_rec(m->parent, v);
	
	char dot = '.';
	if(v->count > 0)
		vect_push(v, &dot);
	vect_push_string(v, m->name);
}

char *mod_full_path(Module *m) {
	Vector out = vect_init(sizeof(char));
	mod_full_path_rec(m, &out);
	return vect_as_string(&out);
}

char *mod_label_prefix(Module *m) {
	Vector out = vect_from_string("");
	
	while (m->parent != NULL) {
		vect_push_string(&out, m->name);
		vect_push_string(&out, ".");
		m = m->parent;
	}

	return vect_as_string(&out);
}

// Recursive end of all modules. To be called at the end
// of the compilation on the root module. Cleans everything
// in the modules except for the tokenizations.
void mod_deep_end(Module *mod) {
	free(mod->name);

	for(size_t i = 0; i < mod->vars.count; i++) {
		Variable *v = vect_get(&(mod->vars), i);
		var_end(v);
	}

	for(size_t i = 0; i < mod->funcs.count; i++) {
		Function *f = vect_get(&(mod->funcs), i);
		func_end(f);
	}

	for(size_t i = 0; i < mod->submods.count; i++) {
		Module *m = vect_get(&(mod->submods), i);
		mod_deep_end(m);
	}

	for(size_t i = 0; i < mod->types.count; i++) {
		Type *t = vect_get(&(mod->types), i);
		typ_end(t);
	}

	vect_end(&(mod->vars));
	vect_end(&(mod->funcs));
	vect_end(&(mod->submods));
	vect_end(&(mod->types));
}



// Tokenizer
typedef struct {
	char *data;
	int line, col;
	int type;
} Token;

bool tok_str_eq(Token *tok, const char *cmp) {
	if (tok == NULL)
		return false;
	return strcmp(tok->data, cmp) == 0;
}

bool tok_eq(Token *a, Token *b) {
	return strcmp(a->data, b->data) == 0 && a->type == b->type;
}

#define TT_DEFWORD 0
#define TT_KEYWORD 1
#define TT_KEYTYPE 2
#define TT_LITERAL 3
#define TT_AUGMENT 4
#define TT_DELIMIT 5
#define TT_SPLITTR 6

char *KEYWORDS = "module,export,asm,if,else,loop,label,goto,continue,break,return,import,as,using,struct,method,interface,enum,implements,operator,len,is";
char *KEYTYPES = "uint8,uint16,uint32,uint64,uint,int8,int16,int32,int64,int,float32,float64,float,comp64,comp,bool,vect,void,type";
char *LITERALS = "false,true";

char *RESERVED = "~`!@#$%^&*()[]{}+-=\"\'\\|:;/?>.<,";

char *OPS = "~`!%&|^*/+-=.<>@";
char *MULTI_OPS = "==,&&,||,^^,!==,!&&,!||,!^^,!<,!>,<<,>>,!&,!|,!^,++,--,>==,<==,+=,-=,*=,/=,%=,!=,&=,|=,^=,~=,`=";

char *DELIMS = "()[]{}";
char *MULTI_DELIMS = ";:#";


bool in_csv(char *csv, char *match) {
	int along = 0;

	for (int i = 0; csv[i] != 0; i++) {
		if (csv[i] == ',') {
			if(along >= 0 && match[along] == 0)
				return true;
			along = 0;
		} else if (along >= 0 && match[along] == csv[i]) {
			along++;
		} else {
			along = -1;
		}
	}
	
	return along >= 0 && match[along] == 0; 
}

bool is_reserved(char c) {
	return strchr(RESERVED, c) != NULL;
}

bool is_delim(char *data) {
	int l = strlen(data);
	
	if (l == 1 && strchr(DELIMS, data[0]) != NULL)
		return true;
	else if (l == 2) {
		if (strchr(MULTI_DELIMS, data[0]) != NULL)
			return (data[1] == data[0] && data[0] != '#') || data[1] == '/';
		else if (strchr(MULTI_DELIMS, data[1]) != NULL)
			return (data[0] == '/');
	}
	return false;
}

int token_type(char*data) {
	int l = strlen(data);

	// Invalid token
	if (l < 1)
		return -1;
	
	if (is_delim(data))
		return TT_DELIMIT;
	else if (is_reserved(data[0])) {
		if (l == 1) {
			if (strchr(OPS, data[0]) != NULL)
				return TT_AUGMENT;
			else if (data[0] == ',' || data[0] == ';' || data[0] == ':')
				return TT_SPLITTR;
		} else if (l == 2 && in_csv(MULTI_OPS, data)) {
			return TT_AUGMENT;
		} else if (l == 3 && in_csv(MULTI_OPS, data)) {
			return TT_AUGMENT;
		}
	} else if (in_csv(KEYTYPES, data)) {
		return TT_KEYTYPE;
	} else if (in_csv(KEYWORDS, data)) {
		return TT_KEYWORD;
	} else if (in_csv(LITERALS, data)){
		return TT_LITERAL;
	}

	return TT_DEFWORD;
}

Token parse_string_literal(int *ch, int *line, int *col, FILE *fin) {
	char first = *ch;
	Vector str = vect_init(sizeof(char));

	vect_push(&str, &first);
	
	Token out = {0};
	out.line = *line;
	out.col = *col;
	out.type = TT_LITERAL;
	
	*ch = fgetc(fin);
	*col += 1;
	char add = 0;
	while (*ch != first && *ch != EOF) {
		add = *ch;
		if (*ch == '\\') {
			vect_push(&str, &add);
			*ch = fgetc(fin);
			*col += 1;
			if (*ch != EOF) {
				if (*ch == '\n') {
					*line += 1;
					*col = 1;
				}
				add = *ch;
				vect_push(&str, &add);
				*ch = fgetc(fin);
				*col += 1;
			}
		} else {
			if (*ch == '\n') {
				*line += 1;
				*col = 1;
			}
			vect_push(&str, &add);
			*ch = fgetc(fin);
			*col += 1;
		}
	}

	vect_push(&str, &first);

	if (*ch == first)
		*ch = fgetc(fin);

	out.data = vect_as_string(&str);

	return out;
}

Token parse_numeric_literal(int *next, int *line, int *col, FILE *fin) {
	Token out = {0};

	out.col = *col;
	out.line = *line;
	out.type = TT_LITERAL;

	Vector num = vect_init(sizeof(char));
	char ch = *next;
	vect_push(&num, &ch);
	
	bool dec = false;

	while (*next != EOF) {
		*next = fgetc(fin);
		*col += 1;

		if (*next == '.' && dec) {
			break;
		} else if (*next == '.') {
			dec = true;
		}

		if (*next < '0' || *next > '9') {
			break;
		} else {
			ch = *next;
			vect_push(&num, &ch);
		}
	}

	out.data = vect_as_string(&num);

	return out;
}

void parse_reserved_tokens(int *next, Vector *out, int *line, int *col, FILE *fin) {
	Token tmp = {0};
	tmp.col = *col;
	tmp.line = *line;

	Vector res = vect_init(sizeof(char));
	char add = *next;
	vect_push(&res, &add);

	*next = fgetc(fin);
	*col += 1;

	while (*next != EOF) {
		add = *next;
		
		if (!is_reserved(add) || add == '"' || add == '\'') {
			break;
		}

		vect_push(&res, &add);
		int after = token_type(vect_as_string(&res));

		if (after == TT_DEFWORD) {
			vect_pop(&res);
			tmp.data = vect_as_string(&res);
			tmp.type = token_type(tmp.data);
			vect_push(out, &tmp);
			
			res = vect_init(sizeof(char));
			vect_push(&res, &add);
			tmp.col = *col;
		}

		*next = fgetc(fin);
		*col += 1;
	}
	
	if (res.count > 0) {
		tmp.data = vect_as_string(&res);
		tmp.type = token_type(tmp.data);
		vect_push(out, &tmp);
	}
}

Token parse_word_token(int *next, int *line, int *col, FILE *fin) {
	Token out = {0}; 
	out.line = *line;
	out.col = *col;

	Vector str = vect_init(sizeof(char));
	char add = *next;

	while (*next != EOF) {
		if (isspace(*next) != 0 || is_reserved(add)) {
			break;
		} else {
			vect_push(&str, &add);
		}

		*next = fgetc(fin);
		add = *next;
		*col += 1;
	}
	
	out.data = vect_as_string(&str);

	out.type = token_type(out.data);
	return out;
}

void parse_nl_token(Vector *out, int *line, int *col) {
	Token add = {0};
	
	add.col = *col;
	add.line = *line;
	add.type = TT_SPLITTR;

	add.data = malloc(sizeof(char) * 2);
	add.data[0] = '\n';
	add.data[1] = 0;

	vect_push(out, &add);

	*col = 1;
	*line += 1;
}

void parse_comment(int *ch, FILE *fin) {
	while(*ch != '\n' && *ch != EOF) {
		*ch = fgetc(fin);
	}
}


Vector parse_file(FILE *fin) {
	Vector out = vect_init(sizeof(Token));

	int line = 1, col = 1;
	int check = fgetc(fin);
	Token add = {0};

	while (check != EOF) {
		add.type = -1;

		if (isspace(check) && check != '\n') {
			check = fgetc(fin);
			col++;
		} else if (check == '#') {
			parse_comment(&check, fin);
		} else if (check == '\"' || check == '\'') {
			add = parse_string_literal(&check, &line, &col, fin);
		} else if (check >= '0' && check <= '9') {
			add = parse_numeric_literal(&check, &line, &col, fin);
		} else if (is_reserved(check)) {
			parse_reserved_tokens(&check, &out, &line, &col, fin);
		} else if(check != '\n') {
			add = parse_word_token(&check, &line, &col, fin);
		}

		if (add.type >= 0)
			vect_push(&out, &add);

		if (check == '\n') {
			parse_nl_token(&out, &line, &col);
			check = fgetc(fin);
		}
	}

	return out;
}



// Compiler funcs

#define BT_FUNCTION 0
#define BT_METHOD 1
#define BT_OPERATOR 2
#define BT_INTERFACE 3
#define BT_MODULE 4
#define BT_CONTROL 5
#define BT_ENUM 6
#define BT_LAMBDA 7

unsigned long tnsl_parse_binary(char *data) {
	unsigned long out = 0;
	
	for(size_t i = 2; data[i] != 0; i++) {
		if (data[i] != '0' && data[i] != '1') {
			return out;
		}
		
		out = out << 1;

		if(data[i] == 1)
			out++;
	}

	return out;
}

unsigned long tnsl_parse_octal(char *data) {
	unsigned long out = 0;
	
	for(size_t i = 2; data[i] != 0; i++) {
		if (data[i] < '0' || data[i] > '7') {
			return out;
		}

		out = out << 3;
		out += (data[i] - '0');
	}

	return out;
}

unsigned long tnsl_parse_decimal(char *data) {
	unsigned long out = 0;

	for(size_t i = 0; data[i] != 0; i++) {
		if (data[i] < '0' || data[i] > '9')
			return out;

		out = out * 10;
		out += (data[i] - '0');
	}

	return out;
}

unsigned long tnsl_parse_hex(char *data) {
	unsigned long out = 0;

	for(size_t i = 2; data[i] != 0; i++) {
		char tmp = data[i];

		if (tmp >= 'a') {
			tmp -= ('a' - 10);
		} else if (tmp >= 'A') {
			tmp -= ('A' - 10);
		} else if (tmp > '0') {
			tmp -= '0';
		}

		if (tmp > 15) {
			return out;
		}

		out = out << 4;
		out += tmp;
	}

	return out;
}

unsigned long tnsl_parse_number (Token *numeric_literal) {
	int l = strlen(numeric_literal->data);

	if (l > 2 && numeric_literal->data[0] == '0' && numeric_literal->data[1] > '9') {
		switch (numeric_literal->data[1]) {
			case 'B':
			case 'b':
				return tnsl_parse_binary(numeric_literal->data);
			case 'O':
			case 'o':
				return tnsl_parse_octal(numeric_literal->data);
			case 'X':
			case 'x':
				return tnsl_parse_hex(numeric_literal->data);
			default:
				printf("ERROR: Unknown prefix for number (0%c) at %d:%d\n\n", numeric_literal->data[1], numeric_literal->line, numeric_literal->col);
				return 0;
		}
	}
	return tnsl_parse_decimal(numeric_literal->data);
}

Token *tnsl_find_last_token(Vector *tokens, size_t pos) {
	if (pos >= tokens->count && tokens->count > 0)
		return vect_get(tokens, tokens->count - 1);
	else if (tokens->count > 0)
		return vect_get(tokens, pos);
	return NULL;
}

int tnsl_next_non_nl(Vector *tokens, size_t pos) {
	Token *t = vect_get(tokens, ++pos);

	while (t != NULL && tok_str_eq(t, "\n")) {
		t = vect_get(tokens, ++pos);
	}

	return pos;
}

// Returns a variable which can be constructed into
// a type.
// name = fully qualified type name
// location = next token after type
// ptr_chain = full pointer chain of the type
Variable tnsl_parse_type(Vector *tokens, size_t cur) {
	Vector ftn = vect_init(sizeof(char));
	Vector ptr = vect_init(sizeof(int));
	
	Variable err = {0};
	err.name = NULL;
	err.location = LOC_LITL;
	err.offset = 0;
	err.type = NULL;

	int add = 0;

	// Pre loop
	for (; cur < tokens->count; cur++) {
		Token *t = vect_get(tokens, cur);
		if (t == NULL) {
			vect_end(&ftn);
			vect_end(&ptr);
			return err;
		} else if (t->type == TT_KEYTYPE || t->type == TT_DEFWORD) {
			break;
		} else if (t->type == TT_AUGMENT) {
			if (tok_str_eq(t, "~")) {
				add = PTYPE_PTR;
				vect_push(&ptr, &add);
			} else {
				vect_end(&ftn);
				vect_end(&ptr);
				return err;
			}
		} else if (t->type == TT_DELIMIT) {
			if (tok_str_eq(t, "{")) {
				cur++;
				t = vect_get(tokens, cur);
				if (t->type == TT_DELIMIT && tok_str_eq(t, "}")) {
					add = PTYPE_ARR;
				} else if (t->type == TT_LITERAL && t->data[0] >= '0' && t->data[0] <= '9') {
					// This functionality is not well implemented yet, but it is supposed to
					// represent a fixed-size array
					add = tnsl_parse_number(t);
					vect_push(&ptr, &add);
					cur++;
					t = vect_get(tokens, cur);
					if (t->type != TT_DELIMIT || !tok_str_eq(t, "}")) {
						vect_end(&ftn);
						vect_end(&ptr);
						return err;
					}
				}
				vect_push(&ptr, &add);
			} else {
				vect_end(&ftn);
				vect_end(&ptr);
				return err;
			}
		} else {
			vect_end(&ftn);
			vect_end(&ptr);
			return err;
		}
	}

	// Get the full type name (ftn)
	// includes the parent module names if
	// dot delineation is used.
	Token *t = vect_get(tokens, cur);
	if(t == NULL) {
		vect_end(&ftn);
		vect_end(&ptr);
		return err;
	} else if (t->type == TT_KEYTYPE) {
		vect_push_string(&ftn, t->data);
		cur++;
	} else if (t->type == TT_DEFWORD) {
		for(; cur < tokens->count; cur++) {
			t = vect_get(tokens, cur);

			if (t != NULL && t->type == TT_DEFWORD) {
				vect_push_string(&ftn, t->data);	
			} else {
				vect_end(&ftn);
				vect_end(&ptr);
				return err;
			}

			cur++;
			t = vect_get(tokens, cur);

			if (t != NULL && t->type == TT_AUGMENT && tok_str_eq(t, ".")) { 
				vect_push_string(&ftn, t->data);
			} else {
				break;
			}
		}
	}

	t = vect_get(tokens, cur);
	if (t != NULL && tok_str_eq(t, "`")) {
		add = PTYPE_REF;
		vect_push(&ptr, &add);
		cur++;
	}

	Variable out = {0};
	
	out.name = vect_as_string(&ftn);
	out.type = NULL;
	out.location = cur;
	out.ptr_chain = ptr;

	return out;
}

bool tnsl_is_def(Vector *tokens, size_t cur) {
	Variable to_free = tnsl_parse_type(tokens, cur);
	
	if (to_free.name == NULL) {
		return false;
	}

	free(to_free.name);
	vect_end(&(to_free.ptr_chain));

	Token *next = vect_get(tokens, to_free.location);
	if (next != NULL && next->type == TT_DEFWORD) {
		return true;
	}
	return false;
}

bool tnsl_is_boolean() {
	return false;
}

int tnsl_find_closing(Vector *tokens, size_t cur) {
	char closing = 0;

	Token *first = vect_get(tokens, cur);
	Token *check;

	if (tok_str_eq(first, "(")) {
		closing = ')';
	} else if (tok_str_eq(first, "[")) {
		closing = ']';
	} else if (tok_str_eq(first, "{")) {
		closing = '}';
	} else if (tok_str_eq(first, "/;") || tok_str_eq(first, ";;")) {
		closing = ';';
	} else {
		return -1;
	}

	cur += 1;
	int paren = 0, brak = 0, squig = 0, block = 0;

	for(; cur < tokens->count; cur++) {
		check = vect_get(tokens, cur);
		if (check->type == TT_DELIMIT) {
			if (check->data[0] == closing && paren == 0 && brak == 0 && squig == 0 && block == 0) {
				return cur;
			}

			switch (check->data[0]) {
			case '(':
				paren += 1;
				break;
			case '[':
				brak += 1;
				break;
			case '{':
				squig += 1;
				break;
			case ')':
				paren -= 1;
				break;
			case ']':
				brak -= 1;
				break;
			case '}':
				squig -= 1;
				break;
			}

			if (tok_str_eq(check, "/;"))
				block += 1;
			else if (tok_str_eq(check, ";/"))
				block -= 1;

			if (paren < 0 || brak < 0 || squig < 0 || block < 0) {
				printf("Unmatched closing delimiter at {line %d, col %d, \"%s\"}\n", check->line, check->col, check->data);
				printf("Looking for closing delimiter for {line %d, col %d, \"%s\"}\n\n", first->line, first->col, first->data);
				return -1;
			}
		}
	}

	printf("Could not find closing for delimiter (line %d, col %d, \"%s\")\n\n", first->line, first->col, first->data);

	return -1;
}

Vector tnsl_find_all_pointers(Vector *tokens, size_t start, size_t end) {
	Vector out = vect_init(sizeof(char*));
	
	for(start++;start < end; start++) {
		Token *cur = vect_get(tokens, start);
		if (tok_str_eq(cur, "~")) {
			start++;
			cur = vect_get(tokens, start);
			if(cur->type == TT_DEFWORD) {
				Vector data = vect_from_string(cur->data);
				char *str = vect_as_string(&data);
				vect_push(&out, &str);
			}
		}
	}

	return out;
}

char tnsl_unquote_char(char *str) {
	if(str == NULL)
		return 0;

	if(*str == '\\') {
		switch(str[1]) {
			case '\\':
				return '\\';
			case '\'':
				return '\'';
			case '"':
				return '"';
			case 'n':
				return '\n';
			case 'r':
				return '\r';
			case '0':
				return 0;
			case 't':
				return '\t';
		}
	}

	return str[0];
}

Vector tnsl_unquote_str(char *literal) {
	int len = strlen(literal);
	Vector str_out = vect_init(sizeof(char));
	if (len < 2)
		return str_out;

	char end = literal[0];
	for(int i = 1; i < len; i++) {
		if(literal[i] == end)
			break;
		else if (literal[i] == '\\') {
			char tmp = tnsl_unquote_char(literal + i);
			vect_push(&str_out, &tmp);
			i++;
		} else
			vect_push(&str_out, literal + i);
	}

	return str_out;
}

int tnsl_block_type(Vector *tokens, size_t cur) {
	
	for (cur++; cur < tokens->count; cur++) {
		Token *t = vect_get(tokens, cur);
		
		if (tok_str_eq(t, "\n")) {
			return BT_LAMBDA;
		} else if (t->type == TT_DEFWORD) {
			return BT_FUNCTION;
		} else if (t->type == TT_KEYWORD) {
			if (tok_str_eq(t, "loop") || tok_str_eq(t, "if") || tok_str_eq(t, "else")) {
				return BT_CONTROL;
			} else if (tok_str_eq(t, "export") || tok_str_eq(t, "module")) {
				return BT_MODULE;
			} else if (tok_str_eq(t, "method")) {
				return BT_METHOD;
			} else if (tok_str_eq(t, "enum")) {
				return BT_ENUM;
			} else if (tok_str_eq(t, "operator")) {
				printf("WARNING: Operator block not implemented (Found at %d:%d)\n\n", t->line, t->col);
				return BT_OPERATOR;
			} else if (tok_str_eq(t, "interface")) {
				printf("WARNING: Interface block not implemented (Found at %d:%d)\n\n", t->line, t->col);
				return BT_INTERFACE;
			} else {
				printf("ERROR: Invalid keyword when parsing block (%s at %d:%d)\n\n", t->data, t->line, t->col);
				return -1;
			}
		} else if (t->type == TT_DELIMIT) {
			int next = tnsl_find_closing(tokens, cur);
			if (next < 0)
				return -1;
			cur = next;
		}
	}

	return -1;
}


// Phase 1 - Module building
bool p1_error = false;

void p1_parse_params(Vector *var_list, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);

	if (end < 0) {
		printf("ERROR: Could not find closing when parsing parameter list \"%s\" (%d:%d)\n", t->data, t->line, t->col);
		p1_error = true;
		return;
	}
	
	Variable current_type = {0};
	current_type.name = NULL;
	current_type.type = NULL;

	for(*pos = tnsl_next_non_nl(tokens, *pos); *pos < (size_t) end; *pos = tnsl_next_non_nl(tokens, *pos)) {
		size_t next = tnsl_next_non_nl(tokens, *pos);
		t = vect_get(tokens, tnsl_next_non_nl(tokens, *pos));

		if(!tok_str_eq(t, ",") && next < (size_t) end) {
			if(current_type.name != NULL) {
				free(current_type.name);
				vect_end(&(current_type.ptr_chain));
			}
			current_type = tnsl_parse_type(tokens, *pos);
			
			if (current_type.location > 0)
				*pos = tnsl_next_non_nl(tokens, current_type.location - 1);
		}

		t = vect_get(tokens, *pos);
		
		if (current_type.name == NULL) {
			printf("ERROR: Expected a valid type.\n");
			printf("       \"%s\" line %d column %d\n\n", t->data, t->line, t->col);
			p1_error = true;
			break;
		} else if (t->type != TT_DEFWORD) {
			printf("ERROR: Unexpected token in member/parameter list (was looking for a user defined name)\n");
			printf("       \"%s\" line %d column %d\n\n", t->data, t->line, t->col);
			p1_error = true;
			break;
		}

		// The name of the variable is "<User defined name> <Dot deliniated type string>"
		// this is specifically so that types may be defined out of order, and will be
		// cleaned up when p1_size_structs is called.
		Variable member = var_copy(&current_type);
		member.location = -1;
		Vector name_type = vect_from_string(t->data);
		vect_push_string(&name_type, " ");
		vect_push_string(&name_type, member.name);
		free(member.name);
		member.name = vect_as_string(&name_type);

		// Add the member to the struct (the member's type will be resolved later)
		vect_push(var_list, &member);

		*pos = tnsl_next_non_nl(tokens, *pos);
		t = vect_get(tokens, *pos);

		if (*pos >= (size_t)end) {
			break;
		} else if (tok_str_eq(t, ",") != true) {
			printf("ERROR: Unexpected token in member list (was looking for a comma to separate members)\n");
			printf("       \"%s\" line %d column %d\n\n", t->data, t->line, t->col);
			p1_error = true;
			break;
		}
	}
	
	if (current_type.name != NULL) {
		free(current_type.name);
		vect_end(&(current_type.ptr_chain));
	}

	*pos = end;
}

void p1_parse_type_list(Vector *var_list, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);

	if (end < 0) {
		printf("ERROR: Could not find closing when parsing parameter list \"%s\" (%d:%d)\n", t->data, t->line, t->col);
		p1_error = true;
		return;
	}

	for(*pos += 1; *pos < (size_t) end;) {
		Variable to_add = tnsl_parse_type(tokens, *pos);

		if(to_add.location < 0) {
			t = vect_get(tokens, *pos);
			printf("ERROR: Could not parse type when in type list ~(%d:%d)\n", t->line, t->col);
			p1_error = true;
			break;
		}

		*pos = to_add.location;
		to_add.location = -1;
		vect_push(var_list, &to_add);
	}

	*pos = end;
}

void p1_parse_struct(Module *add, Vector *tokens, size_t *pos) {
	Token *s = vect_get(tokens, *pos);
	Token *t = vect_get(tokens, *pos + 1);
	if (tok_str_eq(s, "struct") == false) {
		printf("COMPILER ERROR: p1_parse_struct was called on a non-struct token. Aborting.\n\n");
		p1_error = true;
		return;
	} else if (t == NULL || t->type != TT_DEFWORD) {
		printf("ERROR: Expected a user defined name after 'struct' keyword %d:%d\n\n", s->line, s->col);
		p1_error = true;
		return;
	}

	Type to_add = typ_init(t->data, add);

	*pos += 2;
	int closing = tnsl_find_closing(tokens, *pos);
	t = vect_get(tokens, *pos);

	if(closing < 0 || tok_str_eq(t, "{") != true) {
		printf("ERROR: Expected a member list (Types and member names enclosed with '{}') when defining struct.\n");
		printf("       Place one after token \"%s\" line %d column %d\n\n", t->data, t->line, t->col);
		p1_error = true;
		typ_end(&to_add);
		return;
	}
	
	p1_parse_params(&(to_add.members), tokens, pos);
	vect_push(&(add->types), &to_add);
}

void p1_parse_def(Module *root, Vector *tokens, size_t *pos) {
	Variable type = tnsl_parse_type(tokens, *pos);
	*pos = type.location;
	type.mod = root;

	Token *t = vect_get(tokens, *pos);
	if (t == NULL || t->type != TT_DEFWORD) {
		printf("ERROR: p1_parse_def was called but did not find a defword after the type. (%d:%d)\n\n", t->line, t->col);
		*pos -= 1;
		return;
	}

	for(;*pos < tokens->count; *pos += 1) {
		t = vect_get(tokens, *pos);
		if (t == NULL || t->type != TT_DEFWORD) {
			break;
		}

		Variable to_add = var_copy(&type);
		free(to_add.name);
		to_add.location = *pos;

		Vector name = vect_from_string(t->data);
		vect_push_string(&name, " ");
		vect_push_string(&name, type.name);
		to_add.name = vect_as_string(&name);

		vect_push(&root->vars, &to_add);

		*pos += 1;
		t = vect_get(tokens, *pos);
		
		if (tok_str_eq(t, "=")) {
			for(*pos += 1; *pos < tokens->count; *pos += 1) {
				t = vect_get(tokens, *pos);
				if (tok_str_eq(t, ",")) {
					break;
				} else if(t->type == TT_SPLITTR) {
					*pos -= 1;
					break;
				} else if (t->type == TT_DELIMIT) {
					*pos = tnsl_find_closing(tokens, *pos);
				}
			}
		} else if (tok_str_eq(t, ",")) {
			continue;
		} else {
			break;
		}
	}

	*pos -= 1;

	var_end(&type);
}

void p1_parse_enum(Module *root, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);

	if (end < 0) {
		printf("ERROR: Could not find closing for enum \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p1_error = true;
		return;
	}

	*pos += 2;
	t = vect_get(tokens, *pos);

	if (t == NULL || t->type != TT_DEFWORD) {
		t = tnsl_find_last_token(tokens, *pos);
		printf("ERROR: Expected user defined name for enum \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		*pos = end;
		return;
	}

	Vector name = vect_from_string("@@");
	vect_push_string(&name, t->data);
	Module out = mod_init(vect_as_string(&name), root, root->exported);
	vect_end(&name);
	
	*pos += 1;
	t = vect_get(tokens, *pos);

	if (t == NULL || !tok_str_eq(t, "[")) {
		printf("ERROR: Expected a type after enum name (enclose the type in []) (%d:%d)\n\n", t->line, t->col);
		p1_error = true;
		vect_push(&root->submods, &out);
		*pos = end;
		return;
	}

	*pos += 1;
	Variable e_type = tnsl_parse_type(tokens, *pos);
	*pos = tnsl_next_non_nl(tokens, e_type.location); // Past the closing bracket
	
	for (;*pos < (size_t)end; *pos = tnsl_next_non_nl(tokens, *pos)) {
		t = vect_get(tokens, *pos);
		if (t == NULL || t->type != TT_DEFWORD) {
			printf("ERROR: Unexpected token in enum block (expected user defined name) (%d:%d)\n\n", t->line, t->col);
			p1_error = true;
			break;
		}

		Variable to_add = var_copy(&e_type);
		free(to_add.name);
		to_add.location = *pos;

		Vector name = vect_from_string(t->data);
		vect_push_string(&name, " ");
		vect_push_string(&name, e_type.name);
		to_add.name = vect_as_string(&name);

		vect_push(&out.vars, &to_add);

		*pos = tnsl_next_non_nl(tokens, *pos);
		t = vect_get(tokens, *pos);
		
		if (tok_str_eq(t, "=")) {
			for(*pos = tnsl_next_non_nl(tokens, *pos); *pos < tokens->count; *pos = tnsl_next_non_nl(tokens, *pos)) {
				t = vect_get(tokens, *pos);
				if (tok_str_eq(t, ",")) {
					break;
				} else if(t->type == TT_SPLITTR) {
					*pos -= 1;
					break;
				} else if (t->type == TT_DELIMIT) {
					*pos = tnsl_find_closing(tokens, *pos);
				}
			}
		} else if (tok_str_eq(t, ",") || *pos >= (size_t)end) {
			continue;
		} else {
			printf("ERROR: Expected an assignment (= <value>) or a comma after user defined word in enum block \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
			p1_error = true;
			break;
		}

	}

	vect_push(&root->submods, &out);
	var_end(&e_type);
	*pos = end;
}

void p1_parse_function(Module *root, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);

	if (end < 0)
		return;
	
	Function out = func_init("", root);

	for (*pos += 1; *pos < (size_t)end; *pos += 1) {
		Token *t = vect_get(tokens, *pos);
		if(t->type == TT_DEFWORD) {
			free(out.name);
			Vector copy = vect_from_string(t->data);
			out.name = vect_as_string(&copy);

			for(size_t i = 0; i < root->funcs.count; i++) {
				Function *chk = vect_get(&root->funcs, i);
				if(strcmp(chk->name, out.name) == 0) {
					printf("ERROR: Redefinition of function with name '%s' at (%d:%d)\n", out.name, t->line, t->col);
					func_end(&out);
					*pos = end;
					return;
				}
			}
		} else if (tok_str_eq(t, "(")) {
			p1_parse_params(&(out.inputs), tokens, pos);
		} else if (tok_str_eq(t, "[")) {
			p1_parse_type_list(&(out.outputs), tokens, pos);
		} else {
			break;
		}
	}

	vect_push(&(root->funcs), &out);
	*pos = end;
}

void p1_parse_method(Module *root, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);

	if (end < 0) {
		printf("ERROR: Could not find closing for block \"%s\" at (%d:%d)\n\n", t->data, t->line, t->col);
		p1_error = true;
		return;
	}

	*pos += 2;
	t = vect_get(tokens, *pos);

	if (t == NULL || t->type != TT_DEFWORD) {
		printf("ERROR: Expected user defined type while parsing method block. \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p1_error = true;
		*pos = end;
		return;
	}

	Vector mod_name = vect_from_string("_#");
	vect_push_string(&mod_name, t->data);
	Module out = mod_init(vect_as_string(&mod_name), root, root->exported);
	vect_end(&mod_name);

	for (*pos += 1; *pos < end; *pos += 1) {
		t = vect_get(tokens, *pos);
		if (tok_str_eq(t, "/;") || tok_str_eq(t, ";;")) {
			if (tnsl_block_type(tokens, *pos) == BT_FUNCTION) {
				p1_parse_function(&out, tokens, pos);
			}
			t = vect_get(tokens, *pos);
			if(tok_str_eq(t, ";;"))
				*pos -= 1;
		}
	}

	vect_push(&(root->submods), &out);

	*pos = end;
}

void p1_file_loop(Artifact *path, Module *root, Vector *tokens, size_t start, size_t end);

void p1_parse_module(Artifact *path, Module *root, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);
	
	if (end < 0) {
		t = tnsl_find_last_token(tokens, *pos);
		printf("ERROR: Unable to find closing for module block \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		return;
	}
	
	*pos += 1;
	t = vect_get(tokens, *pos);
	bool export = false;
	if (tok_str_eq(t, "export")) {
		export = true;
		*pos += 1;
	}

	*pos += 1;
	t = vect_get(tokens, *pos);
	if (t == NULL || t->type != TT_DEFWORD) {
		t = tnsl_find_last_token(tokens, *pos - 1);
		if (t != NULL) {
			printf("ERROR: Expected user defined module name after token \"%s\" (%d:%d) %d\n\n", t->data, t->line, t->col, t->type);
		}
		p1_error = true;
		*pos = end;
		return;
	}
	char *name = t->data;

	Module *out = NULL;
	for(size_t i = 0; i < root->submods.count; i++) {
		Module *chk = vect_get(&root->submods, i);
		if (strcmp(chk->name, name) == 0) {
			out = chk;
			break;
		}
	}

	if (out == NULL) {
		Module tmp = mod_init(name, root, export);
		p1_file_loop(path, &tmp, tokens, *pos, end);
		vect_push(&(root->submods), &tmp);
	} else {
		p1_file_loop(path, out, tokens, *pos, end);
		vect_push(&(root->submods), out);
	}

	*pos = end;
}


void p1_parse_file(Artifact *path, Module *root) {
	char *full_path = art_to_str(path, '/');
	FILE *fin = fopen(full_path, "r");

	if (fin == NULL) {
		printf("Unable to open file %s for reading.\n\n", full_path);
		free(full_path);
		return;
	}

	free(full_path);
	
	Vector tokens = parse_file(fin);
	fclose(fin);

	p1_file_loop(path, root, &tokens, 0, tokens.count);

	for (size_t i = 0; i < tokens.count; i++) {
		Token *t = vect_get(&tokens, i);
		free(t->data);
	}
	vect_end(&tokens);
}

void p1_file_loop(Artifact *path, Module *root, Vector *tokens, size_t start, size_t end) {
	for(;start < end; start++) {
		Token *t = vect_get(tokens, start);
		if (t->type == TT_SPLITTR && tok_str_eq(t, ":")) {
			t = vect_get(tokens, ++start);
			
			if (t == NULL || !tok_str_eq(t, "import")) {
				t = tnsl_find_last_token(tokens, start);
				printf("ERROR: Comptime declarations are not implemented other than 'import' \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p1_error = true;
				continue;
			}

			t = vect_get(tokens, ++start);
			if(t != NULL && t->type == TT_LITERAL) {
				// Process new path to follow using old path.
				char *cpy = art_to_str(path, '/');
				Artifact new_path = art_from_str(cpy, '/');
				free(cpy);

				// Pop off file name
				art_pop_str(&new_path);
				
				// Copy token data (something like "path/to/file.tnsl" including the quotes)
				// This path is relative to the current file
				Vector v = vect_from_string(t->data);
				// Pop off last quote, replace with \0
				vect_pop(&v);
				vect_as_string(&v);
				// Remove starting quote by indexing into data
				Artifact addon = art_from_str((char *)v.data + 1, '/');
				// No more need for copy string
				vect_end(&v);
				// Add relative path to current folder
				art_add_art(&new_path, &addon);
				// no more need for path relative to file
				art_end(&addon);

				p1_parse_file(&new_path, root);
				// Cleanup last remaining artifact
				art_end(&new_path);
			}
		} else if (t->type == TT_DELIMIT && (tok_str_eq(t, "/;") || tok_str_eq(t, ";;"))) {
			size_t block_start = start;
			switch(tnsl_block_type(tokens, start)) {
			case BT_FUNCTION:
				p1_parse_function(root, tokens, &start);
				break;
			case BT_MODULE:
				p1_parse_module(path, root, tokens, &start);
				break;
			case BT_METHOD:
				p1_parse_method(root, tokens, &start);
				break;
			case BT_ENUM:
				p1_parse_enum(root, tokens, &start);
				break;
			case BT_CONTROL:
			case BT_INTERFACE:
			case BT_OPERATOR:
				printf("ERROR: Block type not implemented \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p1_error = true;
				break;
			default:
				printf("ERROR: Unknown block type \"%s\" at file root (%d:%d)\n\n", t->data, t->line, t->col);
				p1_error = true;
				break;
			}
			
			t = vect_get(tokens, start);

			if (start == block_start) {
				int chk = tnsl_find_closing(tokens, start);
				if (chk < 0)
					printf("ERROR: Could not find closing for \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				else
					start = chk;
			} else if (tok_str_eq(t, ";;")) {
				start--;
			}
		} else if (t->type == TT_KEYWORD && tok_str_eq(t, "struct")) {
			p1_parse_struct(root, tokens, &start);
		} else if (tnsl_is_def(tokens, start)) {
			p1_parse_def(root, tokens, &start);
		}
	}
}

void p1_size_type (Module *root, Type *t) {
	if (t->size == -1)
		return;
	
	int sum = 0;
	t->size = -1;

	Vector tmp = vect_from_string("_#");
	vect_push_string(&tmp, t->name);
	t->module = mod_find_sub(root, vect_as_string(&tmp));
	vect_end(&tmp);

	for(size_t i = 0; i < t->members.count; i++) {
		Variable *var = vect_get(&t->members, i);
		char *n_end = strchr(var->name, ' ');
		
		if (n_end == NULL) {
			printf("COMPILER ERROR: Did not properly assure type %s had all members with both name and RTN\n\n", t->name);
			p1_error = true;
			sum = -2;
			break;
		}

		*n_end = 0;
		Vector name = vect_from_string(var->name);
		Artifact rta = art_from_str(n_end + 1, '.');
		free(var->name);
		var->name = vect_as_string(&name);

		var->offset = sum;

		Type *mt = mod_find_type(root, &rta);
		art_end(&rta);
		
		if(mt == NULL) {
			// Could not find type
			char *rtn = art_to_str(&rta, '.');
			printf("ERROR: Could not find type %s when parsing type %s.\n\n", rtn, t->name);
			p1_error = true;
			free(rtn);
			break;
		} else if (var->ptr_chain.count > 0) {
			// Pointer to type, don't need to size
			sum += 8;
			var->type = mt;
			continue;
		} else if (mt->size == -1) {
			// Cycle in type definition
			printf("ERROR: Cyclical type definition %s -> %s\n\n", mt->name, t->name);
			p1_error = true;
			sum = -1;
			break;
		} else if (mt->size == 0 && mt->module != NULL) {
			// Need to size this type as well
			p1_size_type(mt->module, mt);
		}
		
		sum += mt->size;
		var->type = mt;
	}
	
	t->size = sum;
}

void p1_resolve_func_types(Module *root, Function *func) {

	bool method = root->name != NULL && strlen(root->name) > 1 && root->name[0] == '_' && root->name[1] == '#';

	int reg = 1;
	if (method)
		reg = 2;
	int stack_accum = 0;
	for (size_t i = 0; i < func->outputs.count; i++) {
		Variable *var = vect_get(&func->outputs, i);
		Artifact rtn = art_from_str(var->name, '.');
		Type *t = mod_find_type(root, &rtn);

		if(t == NULL) {
			char *rt = art_to_str(&rtn, '.');
			printf("ERROR: Could not find type %s for function %s\n\n", rt, func->name);
			free(rt);
			art_end(&rtn);
			break;
		}

		art_end(&rtn);

		Vector name = vect_from_string(t->name);
		free(var->name);
		var->name = vect_as_string(&name);
		var->type = t;

		// Check where the output should be stored
		if (
				(
					// is struct
					!is_inbuilt(var->type->name)
					&& // and not by reference
					var->ptr_chain.count == 0
				)
				|| // or we are an array with predefined size (stack array)
				_var_ptr_type(var) > 1
				|| // or we ran out of registers
				reg > 6
			) {
			// The output should be stored on the stack
			var->location = LOC_STCK;
			stack_accum += _var_pure_size(var);
			var->offset = stack_accum;
		} else {
			// The output should be stored in a register
			var->location = reg;
			var->offset = 0;
			reg++;
		}
	}

	reg = 1;
	if (method)
		reg = 2;
	// Stack accum not reset because the stack would get clobbered
	for (size_t i = 0; i < func->inputs.count; i++) {
		Variable *var = vect_get(&func->inputs, i);
		char *n_end = strchr(var->name, ' ');

		if (n_end == NULL) {
			printf("COMPILER ERROR: Did not properly assure function %s had all parameters with both name and RTN\n\n", func->name);
			p1_error = true;
			break;
		}

		Artifact rtn = art_from_str(n_end + 1, '.');
		Type *t = mod_find_type(root, &rtn);

		if(t == NULL) {
			char *rt = art_to_str(&rtn, '.');
			printf("ERROR: Could not find type %s for function %s\n\n", rt, func->name);
			free(rt);
			art_end(&rtn);
			break;
		}

		art_end(&rtn);

		*n_end = 0;
		Vector name = vect_from_string(var->name);
		free(var->name);
		var->name = vect_as_string(&name);
		var->type = t;
		
		// Check where the input should be read from
		if (
				(
					// is struct
					!is_inbuilt(var->type->name)
					&& // and not by reference
					var->ptr_chain.count == 0
				)
				|| // or we are an array with predefined size (stack array)
				_var_ptr_type(var) > 1
				|| // or we ran out of registers
				reg > 6
			) {
			// The output should be stored on the stack
			var->location = LOC_STCK;
			stack_accum += _var_pure_size(var);
			var->offset = stack_accum;
		} else {
			// The output should be stored in a register
			var->location = reg;
			var->offset = 0;
			reg++;
		}
	}

	// Stack variables will be pushed on in reverse order for a call
	// so we need to invert all the variable offsets
	// we are adding 8 here because we assume a return pointer right where
	// rbp is located
	for(size_t i = 0; i < func->outputs.count; i++) {
		Variable *var = vect_get(&func->outputs, i);
		if(var->location == LOC_STCK)
			var->offset = 8 + stack_accum - var->offset;
	}

	for(size_t i = 0; i < func->inputs.count; i++) {
		Variable *var = vect_get(&func->inputs, i);
		if(var->location == LOC_STCK)
			var->offset = 8 + stack_accum - var->offset;
	}
}

void p1_resolve_types(Module *root) {
	for(size_t i = 0; i < root->types.count; i++) {
		p1_size_type(root, vect_get(&root->types, i));
	}

	for(size_t i = 0; i < root->submods.count; i++) {
		p1_resolve_types(vect_get(&root->submods, i));
	}

	for(size_t i = 0; i < root->funcs.count; i++) {
		p1_resolve_func_types(root, vect_get(&root->funcs, i));
		// when created, the function was on the stack. Make sure it is not anymore
		Function *f = vect_get(&root->funcs, i);
		f->module = root;
	}

	for (size_t i = 0; i < root->vars.count; i++) {
		Variable *v = vect_get(&root->vars, i);
		// when created, the module was on the stack.  Make sure it is not anymore.
		v->mod = root;
		char *n_end = strchr(v->name, ' ');

		if (n_end == NULL) {
			printf("COMPILER ERROR: Not properly formatted variable name \"%s\"\n\n", v->name);
			p1_error = true;
			continue;
		}

		Artifact rtn = art_from_str(n_end + 1, '.');
		*n_end = 0;
		Vector name = vect_from_string("");
		vect_push_free_string(&name, v->name);
		v->name = vect_as_string(&name);

		Type *t = mod_find_type(root, &rtn);
		
		if (t == NULL) {
			char *rts = art_to_str(&rtn, '.');
			printf("ERROR: Could not find type \"%s\" for variable \"%s\"\n\n", rts, v->name);
			free(rts);
			art_end(&rtn);
			p1_error = true;
			continue;
		}

		art_end(&rtn);
		v->type = t;
	}
}

void phase_1(Artifact *path, Module *root) {
	p1_parse_file(path, root);
	p1_resolve_types(root);
}

// Phase 2


// Sub scopes
Scope scope_subscope(Scope *s, char *name) {
	Vector n = vect_from_string(name);
	vect_push_string(&n, "#");
	vect_push_free_string(&n, int_to_str(s->next_const));
	s->next_const++;

	Scope out = scope_init(vect_as_string(&n), s->current);
	out.parent = s;

	vect_end(&n);

	return out;
}

bool scope_name_eq(Scope *s, char *name) {
	char *pound = strchr(s->name, '#');
	*pound = 0;
	bool test = strcmp(s->name, name) == 0;
	*pound = '#';
	return test;
}

// Scope variable creation and management

int _scope_next_stack_loc(Scope *s, int size) {
	int sum = -56 - size;
	
	if (s->parent != NULL)
		sum = _scope_next_stack_loc(s->parent, size);

	for (size_t i = 0; i < s->stack_vars.count; i++) {
		Variable *v = vect_get(&s->stack_vars, i);
		if (v->offset - size < sum) {
			sum = v->offset - size;
		}
	}

	return sum;
}

// Tmp reg masks
#define RMSK_B 0b001
#define RMSK_8 0b010
#define RMSK_9 0b100

// Main reg masks
#define RMSK_10 0b000001000
#define RMSK_11 0b000010000
#define RMSK_12 0b000100000
#define RMSK_13 0b001000000
#define RMSK_14 0b010000000
#define RMSK_15 0b100000000

// Generate a bitmask representing available registers
int _scope_avail_reg(Scope *s) {
	int mask = 0b111111111;
	if (s->parent != NULL) {
		mask = _scope_avail_reg(s->parent);
	}

	for (size_t i = 0; i < s->reg_vars.count; i++) {
		Variable *v = vect_get(&s->reg_vars, i);
		int vmask = 0;
		
		if (v->location == 2) {
			vmask = RMSK_B;
		} else if (v->location > 8) {
			vmask = 1;
			vmask = vmask << (v->location - 8);
		}

		mask &= ~vmask;
	}

	return mask;
}

// Creates a new tmp variable from an existing variable
// ALL TEMP VARIABLES SHOULD BE FREED BEFORE CREATING MORE
// PERSISTANT VARIABLES TO PREVENT STACK CLUTTER!!!!!!
Variable scope_mk_tmp(Scope *s, CompData *data, Variable *v) {
	Variable out = var_copy(v);

	int p_typ = _var_ptr_type(v);

	free(out.name);
	Vector nm = vect_from_string("#tmp");
	out.name = vect_as_string(&nm);

	if ((is_inbuilt(v->type->name) && p_typ < 1) || p_typ == PTYPE_PTR || p_typ == PTYPE_PTR) {
		int regs = _scope_avail_reg(s);
		if (regs & 0b111) {
			
			if (regs & RMSK_B) {
				out.location = 2;
			} else if (regs & RMSK_8) {
				out.location = 9;
			} else if (regs & RMSK_9) {
				out.location = 10;
			}
			out.offset = 0;

			var_op_pure_set(data, &out, v);

			vect_push(&s->reg_vars, &out);
			return var_copy(&out);
		}
	}

	int loc = _scope_next_stack_loc(s, _var_pure_size(v));
	
	out.location = LOC_STCK;
	out.offset = loc;

	var_op_pure_set(data, &out, v);

	vect_push(&s->stack_vars, &out);
	return var_copy(&out);
}

// Create a new variable on the stack
// MAKE SURE ALL TMP STACK IS FREED BEFORE THIS IS DONE
Variable scope_mk_stack(Scope *s, CompData *data, Variable *v) {
	Variable out = var_copy(v);
	int loc = _scope_next_stack_loc(s, _var_pure_size(v));
	
	out.location = LOC_STCK;
	out.offset = loc;

	vect_push_string(&data->text, "\tlea rsp, [rbp - ");
	vect_push_free_string(&data->text, int_to_str(-loc));
	vect_push_string(&data->text, "]; Stack variable\n");

	vect_push(&s->stack_vars, &out);
	return var_copy(&out);
}

// Checks if a variable is a tmp variable in the scope
bool scope_is_tmp(Variable *v) {
	return strcmp(v->name, "#tmp") == 0;
}


void _scope_free_tmp_reg(Scope *s, Variable *v) {
	for (size_t i = 0; i < s->reg_vars.count; i++) {
		Variable *to_free = vect_get(&s->reg_vars, i);
		if (to_free->location == v->location) {
			var_end(to_free);
			vect_remove(&s->reg_vars, i);
			i--;
		}
	}
}

void _scope_free_stack_var(Scope *s, CompData *data, Variable *v) {
	int new_top = -56;
	int target_top = v->offset;
	for (size_t i = 0; i < s->stack_vars.count; i++) {
		Variable *to_free = vect_get(&s->stack_vars, i);
		if(to_free->offset == target_top) {
			var_end(to_free);
			vect_remove(&s->stack_vars, i);
			i--;
		} else if (new_top > to_free->offset) {
			new_top = to_free->offset;
		}
	}

	if (new_top > target_top) {
		// Restore rsp
		vect_push_string(&data->text, "\tlea rsp, [rbp - ");
		vect_push_free_string(&data->text, int_to_str(-new_top));
		vect_push_string(&data->text, "]; Tmp variable removed\n");
	}
}

// Free a tmp variable in the scope
void scope_free_tmp(Scope *s, CompData *data, Variable *v) {
	if (v->location > 0) {
		_scope_free_tmp_reg(s, v);
	} else {
		_scope_free_stack_var(s, data, v);
	}

	var_end(v);
}

void scope_free_to(Scope *s, CompData *data, Variable *v) {
	bool freed = false;

	for (size_t i = s->stack_vars.count; i > 0; i--) {
		Variable *cur = vect_get(&s->stack_vars, i - 1);
		if (cur->offset < v->offset || v->offset == 0) {
			var_end(cur);
			vect_pop(&s->stack_vars);
			freed = true;
		} else {
			break;
		}
	}

	if (freed || v->offset == 0) {
		int next_loc = _scope_next_stack_loc(s, 0);
		vect_push_string(&data->text, "\tlea rsp, [rbp - ");
		vect_push_free_string(&data->text, int_to_str(-next_loc));
		vect_push_string(&data->text, "]; Scope free to\n");
	}
}

Variable scope_get_stack_pin(Scope *s) {
	Variable out = {0};
	out.offset = 0;

	if (s->stack_vars.count > 0) {
		Variable *last = vect_get(&s->stack_vars, s->stack_vars.count - 1);
		out = var_copy(last);
	}

	return out;
}

void scope_free_all_tmp(Scope* s, CompData *data) {
	for (size_t i = 0; i < s->reg_vars.count; i++) {
		Variable *to_free = vect_get(&s->reg_vars, i);
		if (to_free->location < RMSK_10) {
			var_end(to_free);
			vect_remove(&s->reg_vars, i);
			i--;
		}
	}
	
	int new_top = 0;
	for (size_t i = 0; i < s->stack_vars.count; i++) {
		Variable *to_free = vect_get(&s->stack_vars, i);
		if(strcmp(to_free->name, "#tmp") == 0) {
			var_end(to_free);
			vect_remove(&s->stack_vars, i);
			i--;

			if (new_top == 0) {
				new_top = -56;
			}
		} else if (new_top > to_free->offset) {
			new_top = to_free->offset;
		}
	}

	if (new_top > 0) {
		// Restore rsp
		vect_push_string(&data->text, "\tlea rsp, [rbp - ");
		vect_push_free_string(&data->text, int_to_str(-new_top));
		vect_push_string(&data->text, "]; All tmp free\n");
	}
}

// Create a tmp variable specifically on the stack
Variable scope_mk_stmp(Scope *s, CompData *data, Variable *v)
{
	Variable out = var_copy(v);

	Vector name = vect_from_string("#tmp");
	out.name = vect_as_string(&name);
	
	int loc = _scope_next_stack_loc(s, _var_pure_size(v));
	out.location = LOC_STCK;
	out.offset = loc;
	
	vect_push_string(&data->text, "\tlea rsp, [rbp - ");
	vect_push_free_string(&data->text, int_to_str(-loc));
	vect_push_string(&data->text, "]; Stack variable\n");

	vect_push(&s->stack_vars, &out);
	return var_copy(&out);
}

// Generate a new variable in the scope
Variable scope_mk_var(Scope *s, CompData *data, Variable *v) {
	Variable out = var_copy(v);
	int p_typ = _var_ptr_type(v);

	if ((is_inbuilt(v->type->name) && p_typ < 1) || p_typ == PTYPE_PTR || p_typ == PTYPE_PTR) {
		int regs = _scope_avail_reg(s);
		if (regs > 0b111) {
			
			if (regs & RMSK_10) {
				out.location = 11;
			} else if (regs & RMSK_11) {
				out.location = 12;
			} else if (regs & RMSK_12) {
				out.location = 13;
			} else if (regs & RMSK_13) {
				out.location = 14;
			} else if (regs & RMSK_14) {
				out.location = 15;
			} else if (regs & RMSK_15) {
				out.location = 16;
			}

			out.offset = 0;

			vect_push(&s->reg_vars, &out);
			return var_copy(&out);
		}
	}

	int loc = _scope_next_stack_loc(s, _var_pure_size(v));
	
	out.location = LOC_STCK;
	out.offset = loc;

	vect_push_string(&data->text, "\tlea rsp, [rbp - ");
	vect_push_free_string(&data->text, int_to_str(-loc));
	vect_push_string(&data->text, "]; Stack variable\n");

	vect_push(&s->stack_vars, &out);
	return var_copy(&out);
}

Variable _scope_get_var(Scope *s, char *name) {
	for (size_t i = 0; i < s->reg_vars.count; i++) {
		Variable *v = vect_get(&s->reg_vars, i);
		if (strcmp(v->name, name) == 0)
			return var_copy(v);
	}

	for (size_t i = 0; i < s->stack_vars.count; i++) {
		Variable *v = vect_get(&s->stack_vars, i);
		if (strcmp(v->name, name) == 0)
			return var_copy(v);
	}

	if (s->parent == NULL) {
		Variable out = {0};
		out.name = NULL;
		return out;
	}

	return _scope_get_var(s->parent, name);
}


Variable scope_get_var(Scope *s, Artifact *name) {
	Variable out = {0};
	out.name = NULL;

	if (name->count == 1) {
		char *full = art_to_str(name, '.');
		out = _scope_get_var(s, full);
		free(full);
	}

	if (out.name != NULL)
		return out;

	Variable *mod_search = mod_find_var(s->current, name);
	
	if (mod_search == NULL)
		return out;

	out = var_copy(mod_search);
	out.location = 0;
	out.offset = 0;
	return out;
}

// TODO: Scope ops like sub-scoping, variable management
// conditional handling, data-section parts for function
// literals, etc.

bool p2_error = false;

/* Op order
 * first is parens (not handled here)
 * 
 * 0: `
 * dereference
 *
 * 1: .
 * get member or method
 *
 * 2: ~
 * Get reference
 *
 * 3: ++ --
 * Increment/decrement
 *
 * 4: len
 * length of array or type
 *
 * 5: * / %
 * Multiplication/division
 * 
 * 6: + -
 * Addition/subtraction
 *
 * 7: ! & | ^ << >> !& !| !^
 * Bitwise operations (and boolean not)
 *
 * 8: == !== < > !< !> <== >==
 * Boolean compare
 *
 * 9: && || ^^ !&& !|| !^^
 * Boolean logic
 *
 * 10: = *= /= %= += -= etc.
 * Assignment operators
 */

// TODO: Test
// returns the integer prescident of the operator (lower means first)
int op_order(Token *t) {
	if (t == NULL || t->type != TT_AUGMENT) {
		printf("COMPILER ERROR: op_order called on null or non-augment token ");
		if (t == NULL)
			printf("NULL\n\n");
		else
		 	printf(" \"%s\" (%d:%d)", t->data, t->line, t->col);
		return -1;
	}

	int l = strlen(t->data);
	
	if(l == 1) {
		switch(t->data[0]) {
		case '`':
			return 0;
		case '.':
			return 1;
		case '~':
			return 2;
		case '*':
		case '/':
		case '%':
			return 5;
		case '+':
		case '-':
			return 6;
		case '!':
		case '&':
		case '|':
		case '^':
			return 7;
		case '<':
		case '>':
			return 8;
		case '=':
			return 10;
		}
	} else if (l == 2) {

		if(t->data[0] == t->data[1]) {
			if (t->data[1] == '+' || t->data[1] == '-')
				return 3;
			if (t->data[0] == '<' || t->data[0] == '>')
				return 7;
			if (t->data[0] == '=')
				return 8;
			return 9;
		}

		if (t->data[1] == '<' || t->data[1] == '>')
			return 8;

		if (t->data[1] == '=')
			return 10;

		if (t->data[0] == '!')
			return 7;
	} else if (l == 3) {
		if(tok_str_eq(t, "len"))
			return 4;
		if(t->data[1] == '=')
			return 8;
		return 9;
	}
	
	return -1;
}


Variable _eval(Scope *s, CompData *data, Vector *tokens, size_t start, size_t end);

Variable _eval_call(Scope *s, CompData *data, Vector *tokens, Function *f, Variable *self, size_t start) {
	
	Variable out = {0};
	out.name = NULL;

	Variable pin = scope_get_stack_pin(s);

	// First, load all stack-based outputs onto the stack
	// and set the output
	for (size_t i = 0; i < f->outputs.count; i++) {
		Variable *cur = vect_get(&f->outputs, i);
		if (cur->location == LOC_STCK) {
			Variable store = scope_mk_stmp(s, data, cur);
			if (i == 0) {
				out = store;
			} else {
				var_end(&store);
			}
		} else if (i == 0) {
			out = var_copy(cur);
		}
	}

	// Second, evaluate all stack-based parameters
	int max = tnsl_find_closing(tokens, start);
	size_t pstart = start + 1;
	size_t pend = start + 1;

	for(size_t i = 0; i < f->inputs.count; i++) {
		Variable *cur = vect_get(&f->inputs, i);

		// preprocess, find end of current param
		while (pend < max) {
			Token *psep = vect_get(tokens, pend);
			if (tok_str_eq(psep, "}") || tok_str_eq(psep, ",")) {
				break;
			} else if (psep->type == TT_DELIMIT) {
				pend = tnsl_find_closing(tokens, pend);
			}
			pend++;
		}

		if (cur->location == LOC_STCK) {
			// create tmp var
			Variable set = scope_mk_stmp(s, data, cur);
			Variable from = _eval(s, data, tokens, pstart, pend);
			// eval and set
			if (_var_ptr_type(&set) == PTYPE_REF)
				var_op_pure_set(data, &set, &from);
			else
				var_op_set(data, &set, &from);
			scope_free_to(s, data, &set);
			var_end(&from);
			var_end(&set);
		}

		if (pend < max) {
			pend = pend + 1;
			pstart = pend;
		}
	}

	// Third, evaluate all register based parameters
	Vector inputs = vect_init(sizeof(Variable));
	Variable inpin = scope_get_stack_pin(s);

	// handle self for methods
	if (self != NULL) {
		Variable from = {0};
		if (_var_ptr_type(self) == PTYPE_NONE) {
			var_op_reference(data, &from, self);
		} else
			from = var_copy(self);

		Variable set = scope_mk_stmp(s, data, &from);
		var_op_pure_set(data, &set, &from);
		var_end(&from);
		vect_push(&inputs, &set);
	}

	pstart = start + 1;
	pend = start + 1;

	for(size_t i = 0; i < f->inputs.count; i++) {
		Variable *cur = vect_get(&f->inputs, i);

		// preprocess, find end of current param
		while (pend < max) {
			Token *psep = vect_get(tokens, pend);
			if (tok_str_eq(psep, "}") || tok_str_eq(psep, ",")) {
				break;
			} else if (psep->type == TT_DELIMIT) {
				pend = tnsl_find_closing(tokens, pend);
			}
			pend++;
		}

		if (cur->location > 0) {
			// create tmp var
			Variable set = scope_mk_stmp(s, data, cur);
			// eval and set
			Variable from = _eval(s, data, tokens, pstart, pend);
			if (_var_ptr_type(&set) == PTYPE_REF)
				var_op_pure_set(data, &set, &from);
			else
				var_op_set(data, &set, &from);
			// cleanup
			scope_free_to(s, data, &set);
			var_end(&from);
			vect_push(&inputs, &set);
		}

		if (pend < max) {
			pend = pend + 1;
			pstart = pend;
		}
	}

	// Fifth, move all register based parameters into their correct registers
	for(size_t i = 0; i < inputs.count; i++) {
		Variable *cur = vect_get(&inputs, i);

		// create tmp var
		Variable set = var_copy(cur);
		set.location = i + 1;

		// eval and set
		var_op_pure_set(data, &set, cur);

		// cleanup
		var_end(cur);
	}
	vect_end(&inputs);

	// set stack to where it needs to be for call
	scope_free_to(s, data, &inpin);
	if (inpin.name != NULL) {
		var_end(&inpin);
	}

	// Sixth, make call
	vect_push_string(&data->text, "\tcall ");
	vect_push_free_string(&data->text, mod_label_prefix(f->module));
	vect_push_string(&data->text, f->name);
	vect_push_string(&data->text, "; Function call\n\n");

	// Seventh, return output
	scope_free_to(s, data, &pin);
	if (out.name != NULL) {
		var_end(&pin);
	}
	return out;
}

Variable _eval_dot(Scope *s, CompData *data, Vector *tokens, size_t start, size_t end) {
	Token *t = vect_get(tokens, start);
	Artifact name = art_from_str(t->data, '.');

	if (start == end - 1) {
		Variable v = scope_get_var(s, &name);
		art_end(&name);
		return v;
	}
	
	Variable v = scope_get_var(s, &name);

	// Match with first possible variable
	start = tnsl_next_non_nl(tokens, start);
	for (; start < end; start = tnsl_next_non_nl(tokens, start)) {
		// Try match variable

		if (v.name != NULL) {
			art_end(&name);
			name = art_from_str("", '.');
			break;
		}

		// Try expand artifact, or call
		t = vect_get(tokens, start);
		if (tok_str_eq(t, ".")) {
			start++;
			t = vect_get(tokens, start);
			if (t->type != TT_DEFWORD) {
				printf("ERROR: Expected defword after '.' operator but found \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				return v;
			}
			art_add_str(&name, t->data);
			v = scope_get_var(s, &name);
		} else if (tok_str_eq(t, "(")) {
			// Call
			break;
		} else {
			printf("ERROR: Failed to find variable or call in dot chain \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
			return v;
		}
	}
	
	for (; start < end; start = tnsl_next_non_nl(tokens, start)) {
		t = vect_get(tokens, start);

		// After found var, or in call.  We need to check.
		if (tok_str_eq(t, "(")) {
			if (v.name == NULL) {
				Function *f = mod_find_func(s->current, &name);
				if (f == NULL) {
					printf("ERROR: Could not find function \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
					art_end(&name);
					return v;
				}
				v = _eval_call(s, data, tokens, f, NULL, start);
			} else {
				Function *m = mod_find_func(v.type->module, &name);
				if (m == NULL) {
					printf("ERROR: Could not find function \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
					art_end(&name);
					return v;
				}
				Variable tmp = _eval_call(s, data, tokens, m, &v, start);
				var_end(&v);
				v = tmp;
			}
			art_end(&name);
			start = tnsl_find_closing(tokens, start);
			name = art_from_str("", '.');
		} else if (tok_str_eq(t, "`")) {
			Variable store;
			var_op_dereference(data, &store, &v);
			var_end(&v);
			v = store;
		} else if (tok_str_eq(t, ".")) {
			Token *next = vect_get(tokens, start + 1);
			if (next == NULL || next->type != TT_DEFWORD) {
				printf("ERROR: Expected defword after dot in dotchain: \"%s\" (%d:%d)\n", t->data, t->line, t->col);
				art_end(&name);
				return v;
			}
			char *m_name = next->data;
			next = vect_get(tokens, start + 2);
			if (next == NULL || !tok_str_eq(next, "(")) {
				Variable m = var_op_member(data, &v, m_name);
				if (m.name == NULL) {
					printf("ERROR: Unable to find member variable: \"%s\" of variable \"%s\" (%d:%d)\n", next->data, v.name, next->line, next->col);
					art_end(&name);
					return v;
				}
				var_end(&v);
				v = m;
			} else if (tok_str_eq(next, "(")) {
				art_add_str(&name, m_name);
			}
			start++;
		}
	}

	art_end(&name);

	// TODO: Dot eval post processing (calls to methods, dereference, and members of structs)

	return v;
}

Variable _eval_literal(Scope *s, CompData *data, Vector *tokens, size_t literal) {
	Variable out = var_init("#literal", NULL);
	Token *t = vect_get(tokens, literal);
	
	if (t->data[0] == '"') {
		// handle str
		Vector str_dat = tnsl_unquote_str(t->data);
		char *label = scope_gen_const_label(s);
		
		vect_push_string(&data->data, label);
		vect_push_string(&data->data, ":\n\tdq ");
		vect_push_free_string(&data->data, int_to_str(str_dat.count));
		
		if (str_dat.count > 0)
			vect_push_string(&data->data, "\n\tdb ");

		for (size_t i = 0; i < str_dat.count; i++) {
			char *ch = vect_get(&str_dat, i);
			vect_push_free_string(&data->data, int_to_str(*ch));
			if (i < str_dat.count - 1) {
				vect_push_string(&data->data, ", ");
			}
		}
		vect_push_string(&data->data, "\n\n");
		vect_end(&str_dat);
		var_end(&out);

		out = var_init(label, typ_get_inbuilt("uint8"));
		out.mod = s->current;
		out.location = LOC_DATA;
		free(label);
		int arr_t = str_dat.count;
		vect_push(&out.ptr_chain, &arr_t);
	} else if (tok_str_eq(t, "false") || tok_str_eq(t, "true")) {
		out.type = typ_get_inbuilt("bool");
		if (tok_str_eq(t, "true")) {
			vect_push_string(&data->text, "\tmov rax, 1\n");
			vect_push_string(&data->text, "\ttest rax, rax ; literal bool\n\n");
			out.offset = 1;
		} else {
			vect_push_string(&data->text, "\tmov rax, 0\n");
			vect_push_string(&data->text, "\ttest rax, rax ; literal bool\n\n");
			out.offset = 0;
		}
	} else {
		out.location = LOC_LITL;
		if (t->data[0] == '\'')
			out.offset = tnsl_unquote_char(t->data + 1);
		else
			out.offset = tnsl_parse_number(t);
	}
	return out;
}

// Main implementation, recursive
Variable _eval(Scope *s, CompData *data, Vector *tokens, size_t start, size_t end) {
	Token *t = vect_get(tokens, start);
	if (start == end - 1 && t->type == TT_LITERAL) {
		return _eval_literal(s, data, tokens, start);
	} 

	int op = -1;
	int op_pos = 0;
	int delim = -1;

	for(size_t i = start; i < end; i++) {
		t = vect_get(tokens, i);
		if (t->type == TT_DELIMIT) {
			if(delim < 0) {
				delim = i;
			}
			int dcl = tnsl_find_closing(tokens, i);
			if (dcl < 0) {
				printf("ERROR: could not find closing for delimiter \"%s\" (%d:%d)\n", t->data, t->line, t->col);
				p2_error = true;
				i = end;
			} else {
				i = dcl;
			}
		} else if (t->type == TT_AUGMENT && op_order(t) > op) {
			op = op_order(t);
			op_pos = i;
		}
	}

	// Found first delim and last lowest priority op
	Variable out;
	out.name = NULL;
	out.location = LOC_LITL;

	if (op < 2){
		// handle dot chain
		Token *d = vect_get(tokens, delim);
		if (delim == -1 || (d->data[0] == '(' && delim > start)) {
			// Handle dot chains and calls
			out = _eval_dot(s, data, tokens, start, end);
			if (out.location == 5) {
				Variable tmp = scope_mk_tmp(s, data, &out);
				var_end(&out);
				out = tmp;
			}
			return out;
		}
		
		// Handle delim
		int dcl = tnsl_find_closing(tokens, delim);
		switch (d->data[0]){
			case '(':
				if (dcl < end - 1) {
					// Extra tokens after paren are not yet supported
					d = vect_get(tokens, dcl);
					printf("Unexpected token after parenthesis \"%s\" (%d:%d)\n\n", d->data, d->line, d->col);
					return out;
				} else {
					// Eval paren as expression
					return _eval(s, data, tokens, start + 1, end - 1);
				}
			case '[':
				printf("Explicit type casts are not yet supported, sorry (%d:%d)\n\n", d->line, d->col);
				return out;
			case '{':
				if (delim > start) {
					// Index
					Variable index = _eval(s, data, tokens, delim + 1, dcl);
					Variable to_index = _eval(s, data, tokens, start, delim);
					Variable store;
					var_op_index(data, &store, &to_index, &index);
					var_end(&index);
					var_end(&to_index);
					to_index = scope_mk_tmp(s, data, &store);
					var_end(&store);
					return to_index;
				} else if (dcl < end - 1) {
					// Invalid
					p2_error = true;
					printf("Unexpected tokens after composite value (%d:%d)\n\n", d->line, d->col);
					return out;
				}
				printf("Composite values in blocks are not yet supported, sorry (%d;%d)\n\n", d->line, d->col);
				p2_error = true;
				return out;
			case '/':
				printf("Anonymous blocks as values are not yet supported, sorry (%d:%d)\n\n", d->line, d->col);
			default:
				p2_error = true;
				return out;
		}
	}
	
	Token *op_token = vect_get(tokens, op_pos);

	// Based on op_token, split the two halves and recurse.
	
	// if boolean, eval left to right, not right to left
	if (op == 9) {
		printf("ERROR: TO IMPL BOOL\n");
		p2_error = true;
		return out;
	}
	
	if (op_pos == start) {
		Variable rhs = _eval(s, data, tokens, op_pos + 1, end);
		if (op_token->data[0] == '~') {
			Variable store;
			var_op_reference(data, &store, &rhs);
			var_end(&rhs);
			
			int *ptype = vect_get(&store.ptr_chain, store.ptr_chain.count - 1);
			*ptype = PTYPE_PTR;
			
			rhs = scope_mk_tmp(s, data, &store);
			var_end(&store);
			
			
			return rhs;
		} else if (op_token->data[0] == '!') {
			out = scope_mk_tmp(s, data, &rhs);
			var_end(&rhs);
			
			var_op_not(data, &out);
			return out;
		} else {
			printf("ERROR: Unexpected prefix token\n");
			return rhs;
		}
	}
	
	Variable rhs = _eval(s, data, tokens, op_pos + 1, end);

	if (!scope_is_tmp(&rhs) && rhs.location > 1 && rhs.location < 9) {
		Variable tmp = scope_mk_tmp(s, data, &rhs);
		var_end(&rhs);
		rhs = tmp;
	}

	out = _eval(s, data, tokens, start, op_pos);
	
	if (op != 10 && !scope_is_tmp(&out) && out.location != LOC_LITL) {
		Variable tmp = scope_mk_tmp(s, data, &out);
		var_end(&out);
		out = tmp;
	}
	
	if (out.location == LOC_LITL) {
		if (rhs.location != LOC_LITL) {
			Variable tmp = rhs;
			out = rhs;
			rhs = tmp;
		}
	}
	
	
	if (strlen(op_token->data) == 1) {
		switch(op_token->data[0]) {
		case '+':
			var_op_add(data, &out, &rhs);
			break;
		case '-':
			var_op_sub(data, &out, &rhs);
			break;
		case '*':
			var_op_mul(data, &out, &rhs);
			break;
		case '/':
			var_op_div(data, &out, &rhs);
			break;
		case '%':
			var_op_mod(data, &out, &rhs);
			break;
		case '|':
			var_op_or(data, &out, &rhs);
			break;
		case '&':
			var_op_and(data, &out, &rhs);
			break;
		case '^':
			var_op_xor(data, &out, &rhs);
			break;
		case '=':
			var_op_set(data, &out, &rhs);
			break;
		case '<':
			var_op_lt(data, &out, &rhs);
			break;
		case '>':
			var_op_gt(data, &out, &rhs);
			break;
		}
	} else if (strlen(op_token->data) == 2){
		switch(op_token->data[0]) {
		case '!':
			if (op_token->data[1] == '&') {
				var_op_nand(data, &out, &rhs);
			} else if (op_token->data[1] == '|') {
				var_op_nor(data, &out, &rhs);
			} else if (op_token->data[1] == '^') {
				var_op_xand(data, &out, &rhs);
			} else if (op_token->data[1] == '<') {
				var_op_ge(data, &out, &rhs);
			} else if (op_token->data[1] == '>') {
				var_op_le(data, &out, &rhs);
			}
			break;
		case '=':
			var_op_eq(data, &out, &rhs);
			break;
		case '<':
			var_op_bsl(data, &out, &rhs);
			break;
		case '>':
			var_op_bsr(data, &out, &rhs);
			break;
		}
	} else if (strlen(op_token->data) == 3){
		switch(op_token->data[0]) {
		case '!':
			if (op_token->data[1] == '=') {
				var_op_ne(data, &out, &rhs);
			}
			break;
		case '<':
			var_op_le(data, &out, &rhs);
			break;
		case '>':
			var_op_ge(data, &out, &rhs);
			break;
		}
	}

	if (scope_is_tmp(&rhs)) {
		scope_free_tmp(s, data, &rhs);
	} else {
		var_end(&rhs);
	}

	return out;
}

// TODO: Operator evaluation, variable members, literals, function calls
Variable eval(Scope *s, CompData *out, Vector *tokens, size_t *pos, bool keep, Variable *out_type) {
	Variable store;

	size_t start = *pos;
	size_t end = start;
	for (; end < tokens->count; end++) {
		Token *chk = vect_get(tokens, end);
		if(chk->type == TT_SPLITTR) {
			break;
		} else if (chk->type == TT_DELIMIT) {
			int i = tnsl_find_closing(tokens, end);
			if(i > 0) {
				end = i;
				continue;
			}
			break;
		}
	}

	store = _eval(s, out, tokens, start, end);
	scope_free_all_tmp(s, out);
	
	if (keep) {
		Variable tmp = var_copy(out_type);
		var_op_set(out, &tmp, &store);
		var_end(&store);
		store = tmp;
	}
	
	*pos = end;
	return store;
}

void eval_strict_zero(CompData *out, Vector *tokens, Variable *v) {
	int size = _var_pure_size(v);
	bool array = false;
	if (_var_ptr_type(v) < 2 && _var_ptr_type(v) > PTYPE_NONE) {
		size = 8;
	} else if (_var_ptr_type(v) > 1) {
		size = v->type->size * _var_ptr_type(v); 
	}

	vect_push_free_string(&out->data, _var_get_datalabel(v));
	vect_push_string(&out->data, ":\n");

	if (array) {
		vect_push_string(&out->data, "\tdq ");
		vect_push_free_string(&out->data, int_to_str(_var_ptr_type(v)));
		vect_push_string(&out->data, "\n");
	}

	vect_push_string(&out->data, "\tdb 0");
	for (int i = 1; i < size; i++) {
		vect_push_string(&out->data, ", 0");
	}
	vect_push_string(&out->data, "\n\n");
}

void eval_strict_literal(Vector *out, Vector *tokens, Variable *v, size_t start) {
	Token *cur = vect_get(tokens, start);
	
	if (cur->type != TT_LITERAL) {
		printf("ERROR: Expected literal value, got \"%s\" (%d:%d)\n\n", cur->data, cur->line, cur->col);
		p2_error = true;
		return;
	} else if (cur->data[0] == '"') {
		printf("ERROR: Unexpected string literal (wanted numeric or character value), got \"%s\" (%d:%d)\n\n", cur->data, cur->line, cur->col);
		p2_error = true;
		return;
	}
	
	char *numstr;
	if (cur->data[0] == '\'') {
		numstr = int_to_str(tnsl_unquote_char(cur->data));
	} else {
		numstr = int_to_str(tnsl_parse_number(cur));
	}

	char *str;
	
	switch(_var_size(v)) {
		case 1:
			str = "\tdb ";
		case 2:
			str = "\tdw ";
		case 4:
			str = "\tdd ";
		case 8:
			str = "\tdq ";
	}

	vect_push_string(out, str);
	vect_push_free_string(out, numstr);
	vect_push_string(out, "; Numeric literal\n");
}

void eval_strict_composite(Module *mod, Vector *out, Vector *tokens, Variable *v, size_t start, char *datalab, int *ntharr);
void eval_strict_ptr(Module *mod, Vector *out, Vector *tokens, Variable *v, size_t start, char *datalab, int *ntharr);

void eval_strict_arr(Module *mod, Vector *out, Vector *tokens, Variable *v, size_t start, char *datalab, int *ntharr) {
	Vector store = vect_from_string(datalab);
	vect_push_string(&store, "#ptr");
	vect_push_free_string(&store, int_to_str(*ntharr));
	vect_push_string(&store, ":\n");
	*ntharr += 1;
	
	// TODO array eval loop
	
	Variable strip = var_copy(v);
	vect_pop(&strip.ptr_chain);
	
	Token *cur = vect_get(tokens, start);
	if (cur->data[0] == '\"') {
		Vector data = tnsl_unquote_str(cur->data);
		vect_push_string(&store, "\tdq ");
		vect_push_free_string(&store, int_to_str(data.count));
		vect_push_string(&store, "\n");

		// char array
		switch(_var_pure_size(&strip)) {
		default:
		case 1:
			vect_push_string(&store, "\tdb ");
			break;
		case 2:
			vect_push_string(&store, "\tdw ");
			break;
		case 4:
			vect_push_string(&store, "\tdd ");
			break;
		case 8:
			vect_push_string(&store, "\tdq ");
			break;
		}

		for(size_t i = 0; i < data.count; i++) {
			char *num = vect_get(&data, i);
			vect_push_free_string(&store, int_to_str(*num));
			if (i + 1 < data.count) {
				vect_push_string(&store, ", ");
			}
		}

		if (data.count == 0) {
			vect_push_string(&store, "0");
		}

		vect_push_string(&store, "\n");
	} else if (cur->data[0] == '{') {
		// Traditional array
		int end = tnsl_find_closing(tokens, start);
		int count = 0;
		bool first = true;

		for (size_t i = start; i < end; i++) {
			cur = vect_get(tokens, i);
			if(tok_str_eq(cur, ",")) {
				first = true;
			} else {
				if (first) {
					count++;
					first = false;
				}

				if (cur->type == TT_DELIMIT) {
					i = tnsl_find_closing(tokens, i);
				}
			}
		}
		
		vect_push_string(&store, "\tdq ");
		vect_push_free_string(&store, int_to_str(count));
		vect_push_string(&store, "\n");

		first = true;
		for (start++; start < end; start++) {
			cur = vect_get(tokens, start);
			if(tok_str_eq(cur, ",")) {
				first = true;
			} else {
				if (first) {
					first = false;
					if (_var_ptr_type(&strip) > 1) {
						eval_strict_arr(mod, &store, tokens, &strip, start + 2, datalab, ntharr);
					} else if (_var_ptr_type(v) > PTYPE_NONE) {
						eval_strict_ptr(mod, &store, tokens, &strip, start + 2, datalab, ntharr);
					} else if (!is_inbuilt(v->type->name)) {
						eval_strict_composite(mod, &store, tokens, &strip, start + 2, datalab, ntharr);
					} else {
						eval_strict_literal(&store, tokens, &strip, start + 2);
					}
				}

				if (cur->type == TT_DELIMIT) {
					start = tnsl_find_closing(tokens, start);
				}
			}
		}
	} else {
		printf("ERROR: Expected array but found \"%s\" (%d:%d)\n\n", cur->data, cur->line, cur->col);
	}
	var_end(&strip);
	
	// Overwrite out
	if (_var_ptr_type(v) < 2) {
		vect_push_string(&store, vect_as_string(out));
		vect_end(out);
		*out = store;
	} else {
		vect_push_string(out, vect_as_string(&store));
		vect_end(&store);
	}
}

void eval_strict_ptr(Module *mod, Vector *out, Vector *tokens, Variable *v, size_t start, char *datalab, int *ntharr) {
	Token *cur = vect_get(tokens, start);

	if (tok_str_eq(cur, "~")) {
		// define qword pointer to existing label
		Artifact v_art = art_from_str("", '.');
		for (start++; ;start++) {
			cur = vect_get(tokens, start);
			if (cur == NULL) {
				cur = tnsl_find_last_token(tokens, start);
				break;
			} else if (cur->type == TT_DEFWORD) {
				art_add_str(&v_art, cur->data);
			} else if (tok_str_eq(cur, "\n") || tok_str_eq(cur, ",")) {
				break;
			} else if (!tok_str_eq(cur, ".")) {
				printf("ERROR: Unexpected token in pointer declaration (file level) \"%s\", (%d:%d)\n\n", cur->data, cur->line, cur->col);
				p2_error = true;
				break;
			}
		}

		// Find var and get data label
		Variable *ptr = mod_find_var(mod, &v_art);
		if (ptr == NULL) {
			char *v_name = art_to_str(&v_art, '.');
			printf("ERROR: Could not find variable \"%s\" for pointer value (%d:%d)\n\n", v_name, cur->line, cur->col);
			free(v_name);
		}
		vect_push_string(out, "\tdq ");
		vect_push_free_string(out, _var_get_datalabel(ptr));
		vect_push_string(out, "\n");

	} else if (tok_str_eq(cur, "{") || cur->data[0] == '\"') {
		// Harder, define array
		vect_push_string(out, "\tdq ");
		vect_push_string(out, datalab);
		vect_push_string(out, "#ptr");
		vect_push_free_string(out, int_to_str(*ntharr));
		
		if (_var_ptr_type(v) < 1) {
			vect_push_string(out, " + 8 ; ref to pointer\n");
		} else {
			vect_push_string(out, " ; ref to pointer\n");
		}

		eval_strict_arr(mod, out, tokens, v, start, datalab, ntharr);
	} else if (cur->type == TT_LITERAL){
		// Literal value interpreted as memory location
		eval_strict_literal(out, tokens, v, start);
	} else {
		// Not impl
		printf("ERROR: Unexpected token when parsing a pointer value at \"%s\" (%d:%d)\n\n", cur->data, cur->line, cur->col);
		p2_error = true;
	}
}

void eval_strict_composite(Module *mod, Vector *out, Vector *tokens, Variable *v, size_t start, char *datalab, int *ntharr) {
	Token *cur = vect_get(tokens, start);

	if (!tok_str_eq(cur, "{")) {
		printf("ERROR: Expected composite value (enclosed with '{}'), got \"%s\" (%d:%d)\n\n", cur->data, cur->line, cur->col);
		p2_error = true;
		return;
	}

	int last = tnsl_find_closing(tokens, start);
	if (last < 0) {
		printf("ERROR: Could not find closing '}' for composite value (%d:%d)\n\n", cur->line, cur->col);
		p2_error = true;
		return;
	}
	
	size_t subvar = 0;
	bool first = true;
	for (start++; start < last; start++) {
		Token *cur = vect_get(tokens, start);
		if(tok_str_eq(cur, ",")) {
			first = true;
		} else {
			if (first) {
				Variable *sub = vect_get(&v->type->members, subvar);
				first = false;
				if (_var_ptr_type(sub) > 1) {
					eval_strict_arr(mod, out, tokens, sub, start, datalab, ntharr);
				} else if (_var_ptr_type(sub) > PTYPE_NONE) {
					eval_strict_ptr(mod, out, tokens, sub, start, datalab, ntharr);
				} else if (!is_inbuilt(sub->type->name)) {
					eval_strict_composite(mod, out, tokens, sub, start, datalab, ntharr);
				} else {
					eval_strict_literal(out, tokens, sub, start);
				}
				subvar++;
			}

			if (cur->type == TT_DELIMIT) {
				start = tnsl_find_closing(tokens, start);
			}
		}
	}
}

void eval_strict(CompData *out, Vector *tokens, Variable *v, size_t start) {
	char *datalab = _var_get_datalabel(v);
	Vector store = vect_from_string(datalab);
	vect_push_string(&store, ":\n");
	int ntharr = 0;

	Token *cur = vect_get(tokens, start + 1);

	if (_var_ptr_type(v) > 1) {
		eval_strict_arr(v->mod, &store, tokens, v, start + 2, datalab, &ntharr);
	} else if (_var_ptr_type(v) > PTYPE_NONE) {
		eval_strict_ptr(v->mod, &store, tokens, v, start + 2, datalab, &ntharr);
	} else if (!is_inbuilt(v->type->name)) {
		eval_strict_composite(v->mod, &store, tokens, v, start + 2, datalab, &ntharr);
	} else {
		eval_strict_literal(&store, tokens, v, start + 2);
	}

	vect_push_string(&out->data, vect_as_string(&store));
	free(datalab);
	vect_end(&store);
}

// Compile a top-level definition
void p2_compile_fdef(Module *root, CompData *out, Vector *tokens, size_t *pos) {
	Variable type = tnsl_parse_type(tokens, *pos);
	*pos = type.location;
	var_end(&type);

	size_t start = *pos;
	Variable *cur;
	while (*pos < tokens->count && !tok_str_eq(vect_get(tokens, *pos), "\n")) {
		Token *t = vect_get(tokens, *pos);

		if (start == *pos) {
			if (t->type != TT_DEFWORD) {
				printf("ERROR: Expected variable name (file), got \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				return;
			}
			
			Artifact v_art = art_from_str(t->data, '.');
			cur = mod_find_var(root, &v_art);
			art_end(&v_art);
			
			if (cur == NULL) {
				printf("ERROR: Definition should have been cataloged but was not \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				return;
			} else if (cur->location == 0) {
				printf("ERROR: Redefinition of variable \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				return;
			}
		} else if (t->type == TT_DELIMIT) {
			*pos = tnsl_find_closing(tokens, *pos);
		} else if (tok_str_eq(t, ",")) {
			if (*pos - start > 1) {
				eval_strict(out, tokens, cur, start);
			} else if(*pos - start > 0) {
				eval_strict_zero(out, tokens, cur);
			}
			start = *pos + 1;
		}

		*pos += 1;
	}

	if (*pos - start > 1) {
		eval_strict(out, tokens, cur, start);
	} else if(*pos - start > 0) {
		eval_strict_zero(out, tokens, cur);
	}
}

// Compiles a variable definition inside a function block
void p2_compile_def(Scope *s, CompData *out, Vector *tokens, size_t *pos, Vector *p_list) {

	Variable type = tnsl_parse_type(tokens, *pos);
	*pos = type.location;

	Artifact t_art = art_from_str(type.name, '.');
	type.type = mod_find_type(s->current, &t_art);
	art_end(&t_art);

	size_t start = *pos;
	while (*pos < tokens->count) {
		Token *t = vect_get(tokens, *pos);
		
		if (start == *pos) {
			if (t->type != TT_DEFWORD) {
				printf("ERROR: Expected variable name, got \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				return;
			}

			// Define new scope var
			free(type.name);
			Vector nm = vect_from_string(t->data);
			type.name = vect_as_string(&nm);
			Variable tmp;
			if (_var_ptr_type(&type) != PTYPE_REF && art_contains(p_list, type.name)) {
				tmp = scope_mk_stack(s, out, &type);
			} else {
				tmp = scope_mk_var(s, out, &type);
			}
			var_end(&tmp);
		} else if (t->type == TT_DELIMIT) {
			int chk = tnsl_find_closing(tokens, *pos);
		} else if (tok_str_eq(t, ",")) {
			// Split def
			if(*pos - start > 1) {
				Variable store = _eval(s, out, tokens, start, *pos);
				var_end(&store);
			}
			start = *pos + 1;
		} else if (tok_str_eq(t, ";") || tok_str_eq(t, "\n")) {
			break;
		}
		*pos += 1;
	}

	var_end(&type);
	if (*pos - start > 1) {
		Variable store = _eval(s, out, tokens, start, *pos);
		var_end(&store);
	}
}

// TODO (depends on top-level defs working)
void p2_compile_enum(Module *root, CompData *out, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);
	
	if(end < 0) {
		printf("ERROR: Could not find closing for enum \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p2_error = true;
		return;
	}

}

void _p2_func_scope_end(CompData *out, Scope *fs) {
	// No multi returns atm
	
	vect_push_string(&out->text, "\tlea rsp, [rbp - 56]\n");
	vect_push_string(&out->text, "\tpop r15\n");
	vect_push_string(&out->text, "\tpop r14\n");
	vect_push_string(&out->text, "\tpop r13\n");
	vect_push_string(&out->text, "\tpop r12\n");
	vect_push_string(&out->text, "\tpop r11\n");
	vect_push_string(&out->text, "\tpop r10\n");
	vect_push_string(&out->text, "\tpop rbp\n"); // restore stack frame
	vect_push_string(&out->text, "\tret ; Scope end\n");

}

void p2_compile_control(Scope *s, Function *f, CompData *out, Vector *tokens, size_t *pos, Vector *p_list);

void p2_wrap_control(Scope *s, Function *f, CompData *out, Vector *tokens, size_t *pos, Vector *p_list) {
	Scope sub = scope_subscope(s, "wrap");

	int end;
	Token *cur;
	bool first = true;
	for (;*pos < tokens->count;) {
		end = tnsl_find_closing(tokens, *pos);
		cur = vect_get(tokens, *pos);
		if (end < 0 || (!tok_str_eq(cur, "/;") && !tok_str_eq(cur, ";;"))) {
			break;
		}

		cur = vect_get(tokens, *pos + 1);
		
		if (tok_str_eq(cur, "if") && first) {
			p2_compile_control(&sub, f, out, tokens, pos, p_list);
			first = false;
		} else if (tok_str_eq(cur, "else") && !first) {
			p2_compile_control(&sub, f, out, tokens, pos, p_list);
		} else {
			if (tok_str_eq(cur, "else") && first) {
				printf("ERROR: Expected if block before else block (%d:%d)\n\n", cur->line, cur->col);
			}
			break;
		}
		
		*pos = end;
	}
	
	vect_push_free_string(&out->text, scope_label_end(&sub));
	vect_push_string(&out->text, ":\n\n");
	scope_end(&sub);
}

// TODO loop blocks, if blocks, else blocks
void p2_compile_control(Scope *s, Function *f, CompData *out, Vector *tokens, size_t *pos, Vector *p_list) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);
	
	if(end < 0) {
		printf("ERROR: Could not find closing for control block \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p2_error = true;
		return;
	}

	*pos += 1;
	t = vect_get(tokens, *pos);

	Scope sub;

	// Generate wrap for if if none exists
	if (tok_str_eq(t, "if") || tok_str_eq(t, "else")) {
		if (s->parent == NULL || !scope_name_eq(s, "wrap")) {
			*pos -= 1;
			p2_wrap_control(s, f, out, tokens, pos, p_list);
			return;
		}
	}

	// Generate subscope
	if (tok_str_eq(t, "if") || tok_str_eq(t, "else")) {
		t = vect_get(tokens, *pos + 1);
		if (tok_str_eq(t, "if")) {
			*pos += 1;
			sub = scope_subscope(s, "elif");
		} else {
			t = vect_get(tokens, *pos);
			sub = scope_subscope(s, t->data);
		}
	} else if (t->type != TT_KEYWORD || !tok_str_eq(t, "loop")) {
		printf("ERROR: Expected control block type ('loop' or 'if'), found \"%s\" (%d:%d)\n", t->data, t->line, t->col);
		p2_error = true;
		*pos = end;
		return;
	} else {
		sub = scope_subscope(s, t->data);
	}
	
	// Find pre and post control statements
	int build = -1;
	int rep = -1;
	for (*pos += 1; *pos < end; *pos += 1) {
		t = vect_get(tokens, *pos);
		
		if (tok_str_eq(t, "(")) {
			build = *pos;
			*pos = tnsl_find_closing(tokens, *pos);
		} else if (tok_str_eq(t, "[")) {
			rep = *pos;
			*pos = tnsl_find_closing(tokens, *pos);
		} else {
			break;
		}
	}

	if (build > -1) {
		// Generate pre-control statements
		size_t start = tnsl_next_non_nl(tokens, build);
		int b_end = tnsl_find_closing(tokens, build);
		build = start;

		for (;start <= build && build <= b_end; build = tnsl_next_non_nl(tokens, build)) {
			if (build == start && tnsl_is_def(tokens, start)) {
				p2_compile_def(&sub, out, tokens, &start, p_list);
				build = start;
			}

			t = vect_get(tokens, build);
			if (tok_str_eq(t, ";") || tok_str_eq(t, ")")) {
				if (build != start) {
					// Eval, check ending
					Variable v = _eval(&sub, out, tokens, start, build);
					if (strcmp(v.type->name, "bool") == 0 && tok_str_eq(t, ")")) {
						build = start - 1;
						start = b_end;
						vect_push_string(&out->text, "\tjz ");
						vect_push_free_string(&out->text, scope_label_end(&sub));
						vect_push_string(&out->text, "; Conditional start\n");
					} else {
						start = build + 1;
					}
					var_end(&v);
				} else {
					start = build + 1;
				}
			} else if (t->type == TT_DELIMIT) {
				build = tnsl_find_closing(tokens, build);
			}
		}

		if (build > b_end) {
			build = -1;
		}
	}

	vect_push_free_string(&out->text, scope_label_start(&sub));
	vect_push_string(&out->text, ": ; Start label\n");

	// Main loop statements
	for(; *pos <= end; *pos = tnsl_next_non_nl(tokens, *pos)) {
		t = vect_get(tokens, *pos);
		if (tok_str_eq(t, "/;") || tok_str_eq(t, ";;")) {
			size_t b_open = *pos;
			
			if(tnsl_block_type(tokens, *pos) == BT_CONTROL) {
				p2_compile_control(&sub, f, out, tokens, pos, p_list);
			} else {
				printf("ERROR: Only control blocks (if, else, loop, switch) are valid inside functions (%d:%d)\n\n", t->line, t->col);
				p2_error = true;
				*pos = end - 1;
			}

			if (*pos == b_open) {
				*pos = tnsl_find_closing(tokens, b_open);
			} else if (tok_str_eq(t, ";;")) {
				*pos -= 1;
			}

		} else if (t->type == TT_KEYWORD) {
			if(tok_str_eq(t, "return")) {
				t = vect_get(tokens, *pos + 1);
				if (f->outputs.count > 0) {
					if (*pos + 1 < end && !tok_str_eq(t, "\n")) {
						*pos += 1;
						Variable *out_type = vect_get(&f->outputs, 0);
						Variable e = eval(&sub, out, tokens, pos, true, out_type);
						var_end(&e);
					} else {
						t = vect_get(tokens, *pos);
						printf("ERROR: Attempt to return from a function without a value, but the function requires one (%d:%d)", t->line, t->col);
						p2_error = true;
					}
				}
				_p2_func_scope_end(out, s);
				*pos = end;
			} else if (tok_str_eq(t, "asm")) {
				t = vect_get(tokens, ++(*pos));
				if(t->type != TT_LITERAL || t->data[0] != '"') {
					printf("ERROR: Expected string literal after asm keyword (%d:%d)\n", t->line, t->col);
					p2_error = true;
				} else {
					Vector asm_str = tnsl_unquote_str(t->data);
					vect_push_string(&out->text, "\t");
					vect_push_string(&out->text, vect_as_string(&asm_str));
					vect_push_string(&out->text, "; User insert asm\n");
					vect_end(&asm_str);
				}
			} else if (tok_str_eq(t, "continue") || tok_str_eq(t, "break")) {
				printf("ERROR: This keyword will be implemented in a future commit \"%s\" (%d:%d)\n", t->data, t->line, t->col);
				p2_error = true;
			} else {
				printf("ERROR: Keyword not implemented inside control blocks \"%s\" (%d:%d)\n", t->data, t->line, t->col);
				p2_error = true;
			}
		} else if (tnsl_is_def(tokens, *pos)) {
			p2_compile_def(&sub, out, tokens, pos, p_list);
		} else if (*pos == end) {
			break;
		} else {
			// TODO: figure out eval parameter needs (maybe needs start and end size_t?)
			// and how eval will play into top level defs (if at all)
			Variable e = eval(&sub, out, tokens, pos, false, NULL);
			var_end(&e);
		}
	}

	vect_push_free_string(&out->text, scope_label_rep(&sub));
	vect_push_string(&out->text, ": ; Rep label\n");

	if (rep > -1) {
		// Generate post-control statements
		size_t start = tnsl_next_non_nl(tokens, rep);
		int r_end = tnsl_find_closing(tokens, rep);
		rep = start;

		for (;start <= rep && rep <= r_end; rep = tnsl_next_non_nl(tokens, rep)) {
			if (rep == start && tnsl_is_def(tokens, start)) {
				p2_compile_def(&sub, out, tokens, &start, p_list);
				rep = start;
			}

			t = vect_get(tokens, rep);
			if (tok_str_eq(t, ";") || tok_str_eq(t, "]")) {
				if (rep != start) {
					// Eval, check ending
					Variable v = _eval(&sub, out, tokens, start, rep);
					if (strcmp(v.type->name, "bool") == 0 && tok_str_eq(t, "]") && scope_name_eq(&sub, "loop")) {
						rep = start - 1;
						start = r_end;
						vect_push_string(&out->text, "\tjnz ");
						vect_push_free_string(&out->text, scope_label_start(&sub));
						vect_push_string(&out->text, "; Conditional rep\n");
					} else {
						start = rep + 1;
					}
					var_end(&v);
				} else {
					start = rep + 1;
				}
			} else if (t->type == TT_DELIMIT) {
				rep = tnsl_find_closing(tokens, rep);
			}
		}

		if (rep > r_end) {
			rep = -1;
		}
	}

	Variable free_to = {0};
	
	// Post control
	if (scope_name_eq(&sub, "loop")) {
		if (build >= 0 && rep < 0) {
			// find end of build, re-eval and cond jmp
			int b_end = build;
			t = vect_get(tokens, b_end);
			while(!tok_str_eq(t, ")")) {
				if (t->type == TT_DELIMIT) {
					b_end = tnsl_find_closing(tokens, b_end);
					b_end++;
				}
				t = vect_get(tokens, b_end);
			}
			Variable v = _eval(&sub, out, tokens, build, b_end);
			vect_push_string(&out->text, "\tjnz ");
			vect_push_free_string(&out->text, scope_label_start(&sub));
			vect_push_string(&out->text, "; Conditional rep\n");
			var_end(&v);

		} else if (build < 0 && rep < 0) {
			vect_push_string(&out->text, "\tjmp ");
			vect_push_free_string(&out->text, scope_label_start(&sub));
			vect_push_string(&out->text, "\n");
		}
	} else {
		// Jmp to outer wrap at end of if
		scope_free_to(&sub, out, &free_to);
		vect_push_string(&out->text, "\tjmp ");
		vect_push_free_string(&out->text, scope_label_end(s));
		vect_push_string(&out->text, "\n");
	}
	
	// Cleanup scope
	vect_push_free_string(&out->text, scope_label_end(&sub));
	vect_push_string(&out->text, ": ; End label\n");
	scope_free_to(&sub, out, &free_to);
	scope_end(&sub);
	vect_push_string(&out->text, "\n\n");
}

// Handles the 'self' variable in the case where the function is in a method block.
void _p2_handle_method_scope(Module *root, CompData *out, Scope *fs, Function *f) {
	
	// load type for method
	Artifact t_art = art_from_str((root->name + 2), '.');
	Type *t = mod_find_type(root, &t_art);
	art_end(&t_art);
	
	// Create self var
	Variable self = var_init("self", t);
	self.location = 1;
	int pt = PTYPE_REF;
	vect_push(&self.ptr_chain, &pt);
	
	// Add to scope
	Variable set = scope_mk_var(fs, out, &self);
	var_op_pure_set(out, &set, &self);
	
	// clean up vars
	var_end(&self);
	var_end(&set);
}

void _p2_func_scope_init(Module *root, CompData *out, Scope *fs, Function *f, Vector *p_list) {
	// TODO: decide what happens when a function scope is created
	
	// export function if module is exported.
	Vector tmp = _scope_base_label(fs);
	if (root->exported) {
		vect_push_string(&out->header, "global ");
		vect_push_string(&out->header, vect_as_string(&tmp));
		vect_push_string(&out->header, "\n");
	}

	// put the label
	vect_push_free_string(&out->text, vect_as_string(&tmp));
	vect_push_string(&out->text, ":\n");

	// Update stack pointers
	vect_push_string(&out->text, "\tpush rbp\n");
	vect_push_string(&out->text, "\tlea rbp, [rsp + 8]\n");
	
	// Push registers to save callee variables (subject to ABI change)
	vect_push_string(&out->text, "\tpush r10\n");
	vect_push_string(&out->text, "\tpush r11\n");
	vect_push_string(&out->text, "\tpush r12\n");
	vect_push_string(&out->text, "\tpush r13\n");
	vect_push_string(&out->text, "\tpush r14\n");
	vect_push_string(&out->text, "\tpush r15 ; scope init\n\n");

	// Load function parameters into expected registers (we assume the stack frame was set up proprely by caller)
	for (size_t i = 0; i < f->inputs.count; i++) {
		Variable *input =  vect_get(&f->inputs, i);
		Variable set;
		if (_var_ptr_type(input) != PTYPE_REF && art_contains(p_list, input->name)) {
			set = scope_mk_stack(fs, out, input);
		} else {
			set = scope_mk_var(fs, out, input);
		}
		var_op_pure_set(out, &set, input);
		var_end(&set);
	}
}


void p2_compile_function(Module *root, CompData *out, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *start = vect_get(tokens, *pos);
	
	// Pre-checks for end of function and function name so scope can be initialized
	if(end < 0) {
		printf("ERROR: Could not find closing for function \"%s\" (%d:%d)\n\n", start->data, start->line, start->col);
		p2_error = true;
		return;
	}

	Vector p_list = tnsl_find_all_pointers(tokens, *pos, end);

	Token *t = vect_get(tokens, *pos);
	while (t != NULL && *pos < (size_t)end && t->type != TT_DEFWORD) {
		t = vect_get(tokens, ++(*pos));
		if(tok_str_eq(t, "\n"))
			break;
		else if (t->type == TT_DELIMIT && !tok_str_eq(t, ";/") && !tok_str_eq(t, ";;")) {
			*pos = tnsl_find_closing(tokens, *pos);
		}
	}

	if(t == NULL || t->type != TT_DEFWORD) {
		printf("ERROR: Could not find user defined name for function \"%s\" (%d:%d)\n\n", start->data, start->line, start->col);
		p2_error = true;
		return;
	}

	// fart
	Artifact f_art = art_from_str(t->data, ' ');
	Function *f = mod_find_func(root, &f_art);
	art_end(&f_art);

	// Scope init
	Scope fs = scope_init(t->data, root);
	_p2_func_scope_init(root, out, &fs, f, &p_list);
	if(root->name != NULL && strlen(root->name) > 1 && root->name[0] == '_' && root->name[1] == '#') {
		_p2_handle_method_scope(root, out, &fs, f);
	}

	while(*pos < (size_t)end) {
		t = vect_get(tokens, *pos);
		if(tok_str_eq(t, "\n"))
			break;
		else if (t->type == TT_DELIMIT)
			*pos = tnsl_find_closing(tokens, *pos);
		*pos += 1;
	}

	*pos = tnsl_next_non_nl(tokens, *pos);
	for (; *pos < (size_t)end; *pos = tnsl_next_non_nl(tokens, *pos)) {
		t = vect_get(tokens, *pos);
		if (tok_str_eq(t, "/;") || tok_str_eq(t, ";;")) {
			size_t b_open = *pos;
			
			if(tnsl_block_type(tokens, *pos) == BT_CONTROL) {
				p2_compile_control(&fs, f, out, tokens, pos, &p_list);
			} else {
				printf("ERROR: Only control blocks (if, else, loop, switch) are valid inside functions (%d:%d)\n\n", t->line, t->col);
				p2_error = true;
			}

			if (*pos == b_open) {
				*pos = tnsl_find_closing(tokens, b_open);
			} else if (tok_str_eq(t, ";;")) {
				*pos -= 1;
			}

		} else if (t->type == TT_KEYWORD) {
			if(tok_str_eq(t, "return")) {
				t = vect_get(tokens, *pos + 1);
				if (f->outputs.count > 0) {
					if (*pos + 1 < end && !tok_str_eq(t, "\n")) {
						*pos += 1;
						Variable *out_type = vect_get(&f->outputs, 0);
						Variable e = eval(&fs, out, tokens, pos, true, out_type);
						var_end(&e);
					} else {
						t = vect_get(tokens, *pos);
						printf("ERROR: Attempt to return from a function without a value, but the function requires one (%d:%d)", t->line, t->col);
						p2_error = true;
					}
				}
				_p2_func_scope_end(out, &fs);
				*pos = end;
				scope_end(&fs);
				return;
			} else if (tok_str_eq(t, "asm")) {
				t = vect_get(tokens, ++(*pos));
				if(t->type != TT_LITERAL || t->data[0] != '"') {
					printf("ERROR: Expected string literal after asm keyword (%d:%d)\n", t->line, t->col);
					p2_error = true;
				} else {
					Vector asm_str = tnsl_unquote_str(t->data);
					vect_push_string(&out->text, "\t");
					vect_push_string(&out->text, vect_as_string(&asm_str));
					vect_push_string(&out->text, "; User insert asm\n");
					vect_end(&asm_str);
				}
			} else {
				printf("ERROR: Keyword not implemented inside functions \"%s\" (%d:%d)\n", t->data, t->line, t->col);
				p2_error = true;
			}
		} else if (tnsl_is_def(tokens, *pos)) {
			p2_compile_def(&fs, out, tokens, pos, &p_list);
		} else {
			// TODO: figure out eval parameter needs (maybe needs start and end size_t?)
			// and how eval will play into top level defs (if at all)
			Variable e = eval(&fs, out, tokens, pos, false, NULL);
			var_end(&e);
		}
	}
	
	if (f->outputs.count > 0) {
		printf("ERROR: Expected return value for function (%d:%d)\n\n", t->line, t->col);
		p2_error = true;
	}

	_p2_func_scope_end(out, &fs);
	scope_end(&fs);
	art_end(&p_list);
	*pos = end;
}

void p2_compile_method(Module *root, CompData *out, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = vect_get(tokens, *pos);
	
	if(end < 0) {
		printf("ERROR: Could not find closing for method \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p2_error = true;
		return;
	}

	*pos += 2;
	t = vect_get(tokens, *pos);

	if (t == NULL || t->type != TT_DEFWORD) {
		printf("ERROR: Expected user defined type after 'method' keyword (%d:%d)\n\n", t->line, t->col);
		*pos = end;
		p2_error = true;
		return;
	}

	Vector sub_name = vect_from_string("_#");
	vect_push_string(&sub_name, t->data);

	// TODO: method loop
	Module *mmod = mod_find_sub(root, vect_as_string(&sub_name));
	vect_end(&sub_name);

	for(;*pos < end;*pos = tnsl_next_non_nl(tokens, *pos)) {
		t = vect_get(tokens, *pos);
		if(tok_str_eq(t, "/;") != true && tok_str_eq(t, ";;") != true) {
			continue;
		}

		p2_compile_function(mmod, out, tokens, pos);
		
		t = vect_get(tokens, *pos);
		if(tok_str_eq(t, ";;")) {
			*pos -= 1;
		}
	}
	
	*pos = end;
}

void p2_file_loop(
		Artifact *path, Module *root, CompData *out,
		Vector *tokens, size_t start, size_t end);

void p2_compile_module(Artifact *path, Module *root, CompData *out, Vector *tokens, size_t *pos) {
	int end = tnsl_find_closing(tokens, *pos);
	Token *t = tnsl_find_last_token(tokens, *pos);

	if (end < 0) {
		printf("ERROR: Could not find closing delimiter for module (%d:%d)\n\n", t->line, t->col);
		p2_error = true;
		return;
	}

	*pos += 1;
	t = vect_get(tokens, *pos);

	if(tok_str_eq(t, "export")){
		*pos += 1;
	}

	*pos += 1;
	t = vect_get(tokens, *pos);

	if (t == NULL || t->type != TT_DEFWORD) {
		t = tnsl_find_last_token(tokens, *pos);
		printf("ERROR: Expected module name after \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p2_error = true;
		*pos = end;
		return;
	}

	Module *mod_root = mod_find_sub(root, t->data);

	if(mod_root == NULL) {
		printf("COMPILER ERROR: Could not find sub module for \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
		p2_error = true;
		*pos = end;
		return;
	}

	p2_file_loop(path, mod_root, out, tokens, *pos, end);

	*pos = end;
}

CompData p2_compile_file(Artifact *path, Module *root) {
	CompData out = cdat_init();

	char *full_path = art_to_str(path, '/');
	FILE *fin = fopen(full_path, "r");

	if (fin == NULL) {
		printf("Unable to open file %s for reading.\n\n", full_path);
		p2_error = true;
		free(full_path);
		return out;
	}

	free(full_path);
	
	Vector tokens = parse_file(fin);
	fclose(fin);

	p2_file_loop(path, root, &out, &tokens, 0, tokens.count);

	for (size_t i = 0; i < tokens.count; i++) {
		Token *t = vect_get(&tokens, i);
		free(t->data);
	}

	vect_end(&tokens);

	return out;
}

void p2_file_loop(
		Artifact *path, Module *root, CompData *out,
		Vector *tokens, size_t start, size_t end) {
	
	for(;start < end; start++) {
		Token *t = vect_get(tokens, start);
		if (t->type == TT_SPLITTR && tok_str_eq(t, ":")) {
			t = vect_get(tokens, ++start);
			
			if (t == NULL || !tok_str_eq(t, "import")) {
				t = tnsl_find_last_token(tokens, start);
				printf("ERROR: Comptime declarations are not implemented other than 'import' \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				continue;
			}

			t = vect_get(tokens, ++start);
			if(t != NULL && t->type == TT_LITERAL) {
				// Process new path to follow using old path.
				char *cpy = art_to_str(path, '/');
				Artifact new_path = art_from_str(cpy, '/');
				free(cpy);

				// Pop off file name
				art_pop_str(&new_path);
				
				// Copy token data (something like "path/to/file.tnsl" including the quotes)
				// This path is relative to the current file
				Vector v = vect_from_string(t->data);
				// Pop off last quote, replace with \0
				vect_pop(&v);
				vect_as_string(&v);
				// Remove starting quote by indexing into data
				Artifact addon = art_from_str((char *)v.data + 1, '/');
				// No more need for copy string
				vect_end(&v);
				// Add relative path to current folder
				art_add_art(&new_path, &addon);
				// no more need for path relative to file
				art_end(&addon);

				CompData dat = p2_compile_file(&new_path, root);
				cdat_add(out, &dat);
				cdat_end(&dat);
				// Cleanup last remaining artifact
				art_end(&new_path);
			}
		} else if (t->type == TT_DELIMIT && (tok_str_eq(t, "/;") || tok_str_eq(t, ";;"))) {
			size_t block_start = start;
			switch(tnsl_block_type(tokens, start)) {
			case BT_FUNCTION:
				p2_compile_function(root, out, tokens, &start);
				break;
			case BT_MODULE:
				p2_compile_module(path, root, out, tokens, &start);
				break;
			case BT_METHOD:
				p2_compile_method(root, out, tokens, &start);
				break;
			case BT_ENUM:
				p2_compile_enum(root, out, tokens, &start);
				break;
			case BT_CONTROL:
				printf("ERROR: Control blocks (if, loop, switch) can only be placed within a function block. (%d:%d)\n\n", t->line, t->col);
				p2_error = true;
				break;
			case BT_INTERFACE:
			case BT_OPERATOR:
				printf("ERROR: Block type not implemented \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				break;
			default:
				printf("ERROR: Unknown block type \"%s\" at file root (%d:%d)\n\n", t->data, t->line, t->col);
				p2_error = true;
				break;
			}
			
			t = vect_get(tokens, start);

			if (start == block_start) {
				int chk = tnsl_find_closing(tokens, start);
				if (chk < 0)
					printf("ERROR: Could not find closing for \"%s\" (%d:%d)\n\n", t->data, t->line, t->col);
				else
					start = chk;
			} else if (tok_str_eq(t, ";;")) {
				start--;
			}
		} else if (tok_str_eq(t, "asm")) {
			// TODO: top level asm should go where?
			start++;
			t = vect_get(tokens, start);
			if(t != NULL && t->type == TT_LITERAL) {
				Vector asm_str = tnsl_unquote_str(t->data);
				if (asm_str.count > 0) {
					vect_push_string(&out->header, vect_as_string(&asm_str));
					vect_push_string(&out->header, "\n");
				}
				vect_end(&asm_str);
			}
		} else if (tok_str_eq(t, "struct")){
			start += 2;
			start = tnsl_find_closing(tokens, start);
		} else if (tnsl_is_def(tokens, start)) {
			p2_compile_fdef(root, out, tokens, &start);
		}
	}
}


void compile(Artifact *path_in, Artifact *path_out) {

	// Root module used for artifact resolution
	Module root = mod_init("", NULL, true);

	phase_1(path_in, &root);
	
	if (p1_error) {
		printf("Parser encountered errors, stopping.\n\n");
		mod_deep_end(&root);
		return;
	}

	CompData out = p2_compile_file(path_in, &root);
	mod_deep_end(&root);

	if (p2_error) {
		printf("Compiler encountered errors, stopping.\n\n");
		cdat_end(&out);
		return;
	}

	char *full_path = art_to_str(path_out, '/');
	FILE *fout = fopen(full_path, "w");
	
	if (fout == NULL) {
		printf("Unable to open output file %s for writing.\n\n", full_path);
		free(full_path);
		cdat_end(&out);
		return;
	}
	
	free(full_path);

	cdat_write_to_file(&out, fout);

	fclose(fout);
	cdat_end(&out);
}

char *tok_type_strs[] = {
	"DEFWORD",
	"KEYWORD",
	"KEYTYPE",
	"LITERAL",
	"AUGMENT",
	"DELIMIT",
	"SPLITTR"
};

void write_token(FILE *out, Token *t) {
	fprintf(out, "{line: %d, column: %d, type %s, data: \"%s\"}\n", t->line, t->col, tok_type_strs[t->type], t->data);
}

void tokenize(Artifact *path_in, Artifact *path_out) {
	char *full_path = art_to_str(path_in, '/');
	FILE *fin = fopen(full_path, "r");

	if (fin == NULL) {
		printf("Unable to open file %s for reading.\n\n", full_path);
		free(full_path);
		return;
	}

	free(full_path);
	full_path = art_to_str(path_out, '/');
	FILE *fout = fopen(full_path, "w");
	
	if (fout == NULL) {
		printf("Unable to open file %s for writing.\n\n", full_path);
		free(full_path);
		fclose(fin);
		return;
	}

	free(full_path);
	
	Vector tokens = parse_file(fin);
	fclose(fin);

	for(size_t i = 0; i < tokens.count; i++) {
		Token *t = vect_get(&tokens, i);
		write_token(fout, t);
		free(t->data);
	}

	fflush(fout);
	fclose(fout);

	vect_end(&tokens);
}

// Entrypoint

void help() {
	printf("\n");
	printf("Usage:\n");
	printf("\tctc - The TNSL compiler (written in c)\n\n");
	printf("\tctc [file in]                  - compile the given file, writing output assembly in out.asm\n");
	printf("\tctc [file in] [file out]       - same as before, but write the output assembly to the given filename\n");
	printf("\t    -h                         - print this output message\n");
	printf("\t    -v                         - print version information\n");
	printf("\t    -t [file in]               - output tokenization of file instead of assembly in out.asm\n");
	printf("\t    -t [file in] [file out]    - output tokenization of file instead of assembly in output file\n");
	printf("\n");
}

int main(int argc, char ** argv) {
	if (argc < 2 || strcmp(argv[1], "-h") == 0) {
		help();
		return 1;
	} else if (strcmp(argv[1], "-v") == 0) {
		printf("C based TNSL Compiler (CTC) - version 0.1\n");
		return 0;
	} else if (strcmp(argv[1], "-t") == 0) {
		Artifact in = art_from_str(argv[2], '/');
		Artifact out;
		if (argc == 3) {
			out = art_from_str("out.asm", '/');
		} else {
			out = art_from_str(argv[3], '/');
		}
		tokenize(&in, &out);
		art_end(&in);
		art_end(&out);
		return 0;
	}

	Artifact in = art_from_str(argv[1], '/');
	Artifact out;
	if (argc == 2) {
		out = art_from_str("out.asm", '/');
	} else {
		out = art_from_str(argv[2], '/');
	}

	compile(&in, &out);
	art_end(&in);
	art_end(&out);

	return 0;
}

