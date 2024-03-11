# EBNF language description

    
    decimal digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
    decimal literal = decimal digit, {decimal digit}, [".", {decimal digit}];

    octal digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7";
    octal literal = "0o", octal digit, {octal digit}, [".", {octal digit}];

    hex digit = decimal digit | "A" | "B" | "C" | "D" | "E" | "F" | "a" | "b" | "c" | "d" | "e" | "f";
    hex literal = "0x", hex digit, {hex digit}, [".", {hex digit}];

    binary digit = "0" | "1";
    binary literal = "0b", {binary digit}, [".", {binary digit}];

    numeric literal = binary literal | hex literal | octal literal | decimal literal;
    
    character = ? any unicode character except "\" ?;
    escape sequence = "\", ("u", hex digit, {hex digit} | ? any ascii character except "u" ?);

    character literal = "'" (character | escape sequence) "'";
    
    singlet literal = character literal | numeric literal

    string literal = "\"", {character | escape sequence}, "\""

    literal = singlet literal | string literal



