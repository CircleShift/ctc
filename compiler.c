#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


// Vector utils

#define VECT_MAX_GROW 100
#define VECT_MIN_SIZE 4

typedef struct {
	int _el_sz, count, size;
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

	for(int i = 0; i < (v->count - 1) * v->_el_sz; i++) {
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

	for (int i = 0; i > (index * v->_el_sz); i--) {
		new_spot[i] = old_spot[i];
	}

	for (int i = 0; i < v->_el_sz; i++) {
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
	out.size = v->size;

	out.data = malloc(out.size * out._el_sz);

	char *former = v->data;
	char *latter = out.data;

	for(int i = 0; i < out.count * out._el_sz; i++) {
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

char *vect_as_string(Vector *v) {
	((char*)v->data)[v->count] = 0;
	return v->data;
}


void vect_end(Vector *v) {
	free(v->data);
}


// Artifacts (vect of strings)




void help() {
	printf("\n");
	printf("Usage:\n");
	printf("\tctc - The TNSL compiler (written in c)\n\n");
	printf("\tctc [file in]              - compile the given file, writing output assembly in out.asm\n");
	printf("\tctc [file in] [file out]   - same as before, but write the output assembly to the given filename\n");
	printf("\t    -h                     - print this output message\n");
	printf("\n");
}

int main(int argc, char ** argv) {
	if (argc < 2 || strcmp(argv[1], "-h") == 0) {
		help();
		return 1;
	}

	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	return 0;
}

