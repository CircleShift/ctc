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
		vect_push(&out, s + i);
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
	fprintf(fout, "%s\n", vect_as_string(&(cdat->header)));
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
	int location; // negative for on stack, positive or zero for in register
} Variable;

#define PTYPE_PTR 0
#define PTYPE_REF 1
#define PTYPE_ARR 2

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
	out.ptr_chain = vect_init(sizeof(int));

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

	for(size_t i = 0; i < func->outputs.count; i++) {
		Variable *to_end = vect_get(&(func->outputs), i);
		var_end(to_end);
	}
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

}



// Tokenizer
typedef struct {
	char *data;
	int line, col;
	int type;
} Token;

#define TT_DEFWORD 0
#define TT_KEYWORD 1
#define TT_KEYTYPE 2
#define TT_LITERAL 3
#define TT_AUGMENT 4
#define TT_DELIMIT 5
#define TT_SPLITTR 6

char *KEYWORDS = "module,export,asm,if,else,loop,label,goto,continue,break,return,import,as,using,struct,method,interface,implements,operator,len,is";
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
			else if (data[0] == ',')
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

		if (isspace(check)) {
			check = fgetc(fin);
		} else if (check == '#') {
			parse_comment(&check, fin);
		} else if (check == '\"' || check == '\'') {
			add = parse_string_literal(&check, &line, &col, fin);
		} else if (check >= '0' && check <= '9') {
			add = parse_numeric_literal(&check, &line, &col, fin);
		} else if (is_reserved(check)) {
			parse_reserved_tokens(&check, &out, &line, &col, fin);
		} else {
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

void compile (Artifact path_in, Artifact path_out) {
	char *full_path = art_to_str(&path_in, '/');
	FILE *fin = fopen(full_path, "r");
	free(full_path);

	if (fin == NULL)
		return;

	Vector tokens = parse_file(fin);

	fclose(fin);

	full_path = art_to_str(&path_out, '/');
	FILE *fout = fopen(full_path, "w");
	free(full_path);

	if (fout != NULL) {
		// Write all tokens
		for (size_t i = 0; i < tokens.count; i++) {
			Token *t = vect_get(&tokens, i);
			fprintf(fout, "{ line: %d, column: %d, type %d, data: %s }\n", t->line, t->col, t->type, t->data);
			free(t->data);
		}
	}
	vect_end(&tokens);

	fclose(fout);
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

	compile(in, out);
	art_end(&in);
	art_end(&out);

	return 0;
}

