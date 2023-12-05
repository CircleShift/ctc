#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>



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
// do NOT free this pointer. Not safe to use this string
// at the same time as you are adding or removing from the
// vector. Instead consider cloning the vector if you must
// have both, or want an independant copy of the string.
char *vect_as_string(Vector *v) {
	((char*)v->data)[v->count * v->_el_sz] = 0;
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

	out.header = vect_init(sizeof(char));
	out.data = vect_init(sizeof(char));
	out.text = vect_init(sizeof(char));

	return out;
}

void cdat_write_to_file(CompData *cdat, FILE *fout) {
	fprintf(fout, "format ELF64\n%s\n", vect_as_string(&(cdat->header)));
	fprintf(fout, "%s\n", vect_as_string(&(cdat->data)));
	fprintf(fout, "%s\n", vect_as_string(&(cdat->text)));
	fflush(fout);
}

void cdat_end(CompData *cdat) {
	vect_end(&(cdat->header));
	vect_end(&(cdat->data));
	vect_end(&(cdat->text));
}



// Types

typedef struct Module {
	char *name;
	bool exported;
	Vector types, vars, funcs, submods;
	struct Module *parent;
} Module;

typedef struct {
	char *name;       // Name of the type
	int size;         // Size (bytes) of the type
	Vector members;   // Member variables (Stored as variables)
	Module *module;     // Module (for methods and member-type resolution) to tie the type to.
} Type;

typedef struct {
	char *name;
	Type *type;
	Vector ptr_chain;
	int location; // negative for on stack, zero for in data section, positive for in register
} Variable;

#define PTYPE_PTR -1
#define PTYPE_REF 0
#define PTYPE_ARR 1

typedef struct {
	char *name;
	Vector inputs, outputs;
	Module *module;
} Function;



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
	{"uint", 8, {0}, NULL},
	{"int8", 1, {0}, NULL},
	{"int16", 2, {0}, NULL},
	{"int32", 4, {0}, NULL},
	{"int64", 8, {0}, NULL},
	{"int", 8, {0}, NULL},
	{"float32", 4, {0}, NULL},
	{"float64", 8, {0}, NULL},
	{"float", 8, {0}, NULL},
	{"bool", 1, {0}, NULL},
	{"void", 8, {0}, NULL},
};

Type *typ_get_inbuilt(char *name) {
	for (int i = 0; i < sizeof(TYP_INBUILT)/sizeof(Type); i++) {
		if (strcmp(TYP_INBUILT[i].name, name) == 0) {
			return &(TYP_INBUILT[i]);
		}
	}
	return NULL;
}



// Variables

// Initializes the variable, copying name, not copying type.
Variable var_init(char *name, Type *type) {
	Variable out = {0};
	
	Vector name_cpy = vect_from_string(name);
	out.name = vect_as_string(&name_cpy);
	out.type = type;
	out.ptr_chain = vect_init(sizeof(int));
	out.location = 0;

	return out;
}

Variable var_copy(Variable *to_copy) {
	Variable out = var_init(to_copy->name, to_copy->type);

	out.location = to_copy->location;

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

// Type coercion engine
// TODO: all
Variable _op_coerce(Variable *base, Variable *to_coerce) {
	Variable out = {0};
	return out;
}

// TODO: Operations on variables

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
		vect_push_string(&e_check, *to_check);
		vect_as_string(&e_check);
		
		void *out = NULL;
		for (size_t i = 0; i < mod->submods.count; i++) {
			Module *m = vect_get(&(mod->submods), i);
			if (strcmp(m->name, *to_check) == 0 || strcmp(m->name, e_check.data) == 0) {
				out = mod_find_rec(m, art, sub + 1, find_type);
				break;
			}
		}

		vect_end(&e_check);

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
		if(strcmp(m->name, chk))
				return m;
	}
	return NULL;
}

void mod_full_path_rec(Module *m, Vector *v) {
	if(m->parent != NULL)
		mod_full_path_rec(m->parent, v);
	
	char dot = '.';
	if(v->count > 0 || (v->count == 0 && m->name[0] == '@'))
		vect_push(v, &dot);
	vect_push_string(v, m->name);
}

char *mod_full_path(Module *m) {
	Vector out = vect_init(sizeof(char));
	mod_full_path_rec(m, &out);
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

char *RESERVED = "~`!@#$%^&*()[]{}+-=\"\'\\|:;/?>.<,";

char *OPS = "~`!%&|^*/+-=.<>";
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
		
		if (!is_reserved(add)) {
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

Variable tnsl_parse_type(Vector *tokens, size_t cur) {
	Vector ftn = vect_init(sizeof(char));
	Vector ptr = vect_init(sizeof(int));
	
	Variable err = {0};
	err.name = NULL;
	err.location = -1;
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

	// ftn loop
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

	return -1;
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

	for(*pos = tnsl_next_non_nl(tokens, *pos); *pos < end; *pos = tnsl_next_non_nl(tokens, *pos)) {
		size_t next = tnsl_next_non_nl(tokens, *pos);
		t = vect_get(tokens, tnsl_next_non_nl(tokens, *pos));

		if(!tok_str_eq(t, ",") && next < end) {
			if(current_type.name != NULL) {
				free(current_type.name);
				vect_end(&(current_type.ptr_chain));
			}
			current_type = tnsl_parse_type(tokens, *pos);
			*pos = tnsl_next_non_nl(tokens, current_type.location - 1);
		}

		t = vect_get(tokens, *pos);
		
		if (current_type.name == NULL) {
			printf("ERROR: Expected a type before the first member/parameter\n");
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

		if (*pos >= end) {
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

	Token *t = vect_get(tokens, *pos);
	if (t == NULL || t->type != TT_DEFWORD) {
		printf("ERROR: p1_parse_def was called but did not find a defword after the type. (%d:%d)\n\n", t->line, t->col);
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
	
	for (;*pos < end; *pos = tnsl_next_non_nl(tokens, *pos)) {
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
		} else if (tok_str_eq(t, ",") || *pos >= end) {
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

	for (*pos += 1; *pos < end; *pos += 1) {
		Token *t = vect_get(tokens, *pos);
		if(t->type == TT_DEFWORD) {
			free(out.name);
			Vector copy = vect_from_string(t->data);
			out.name = vect_as_string(&copy);
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

	Module out = mod_init(name, root, export);

	p1_file_loop(path, &out, tokens, *pos, end);

	*pos = end;
	vect_push(&(root->submods), &out);
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
		}
	}
}

void p1_size_type (Module *root, Type *t) {
	if (t->size == -1)
		return;
	
	int sum = 0;
	t->size = -1;

	Vector tmp = vect_from_string("@");
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
	}

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
	}

	for (size_t i = 0; i < root->vars.count; i++) {
		Variable *v = vect_get(&root->vars, i);
		char *n_end = strchr(v->name, ' ');

		if (n_end == NULL) {
			printf("COMPILER ERROR: Not properly formatted variable name \"%s\"\n\n", v->name);
			p1_error = true;
			continue;
		}

		Artifact rtn = art_from_str(n_end + 1, '.');
		*n_end = 0;
		Vector name = vect_from_string(v->name);
		free(v->name);
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

bool p2_error = false;

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
}

CompData phase_2(Artifact *path, Module *root) {
	return p2_compile_file(path, root);
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

	CompData out = phase_2(path_in, &root);
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

// Entrypoint

void help() {
	printf("\n");
	printf("Usage:\n");
	printf("\tctc - The TNSL compiler (written in c)\n\n");
	printf("\tctc [file in]              - compile the given file, writing output assembly in out.asm\n");
	printf("\tctc [file in] [file out]   - same as before, but write the output assembly to the given filename\n");
	printf("\t    -h                     - print this output message\n");
	printf("\t    -v                     - print version information\n");
	printf("\n");
}

int main(int argc, char ** argv) {
	if (argc < 2 || strcmp(argv[1], "-h") == 0) {
		help();
		return 1;
	} else if (strcmp(argv[1], "-v") == 0) {
		printf("C based TNSL Compiler (CTC) - version 0.1\n");
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

