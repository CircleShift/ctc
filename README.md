# ctc

C based TNSL compiler


Technically, this program is a TNSL to ASM transpiler, which works in two passes.

### pass 1

The first pass loads struct, module, and function declarations into memory, and checks for
circular types.  It also catalogs global variable definitions to later put into the data section.

all first pass functions are prefixed with `p1_`

### pass 2

The second pass generates the actual assembly code by recursive descent.

all second pass functions are prefixed with `p2_`


### helper functions and utilities

There are a few common data structures for use within the program to make programming easier:

- Vectors: dynamic arrays (common functions prefixed with `vect_`)
- Artifacts: representations of delineated strings such as file paths or fully qualified type names (common functions start with `art_`)
- Types: representations of internal and user defined types (common functions prefixed with `type_`)
- Variables: representation of actual variable data within the program.  Can be a literal, register, stack, or data based value.  Operations can be performed with Variable structs to generate assembly (common functions start with `var_`)
- Modules: representation of tnsl modules which contain Types, global Variables, Functions, and sub Modules (common funcs start with `mod_`)

