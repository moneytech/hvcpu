#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#define INSTR_MAX_OPERANDS 2

enum ASSEMBLER_ENUM{
T_NONE,
T_UNKNOWN,
T_WHITESPACE,
T_MNEM,
T_REGISTER,
T_IDENTIFER,
T_IMMED_HEX,
T_IMMED_DEC,
T_IMMED_OCT,
T_IMMED_BIN,
T_COMMENT,
T_NEWL,
T_DATA_BYTE,
T_DATA_STR,
T_BRACK_OPEN,
T_BRACK_CLOS,
T_COLON,
T_COMMA,
T_SHSIGN,
T_END,
};
//keep up to date by copying changes, order important
const char *token_names[] = {
"T_NONE",
"T_UNKNOWN",
"T_WHITESPACE",
"T_MNEM",
"T_REGISTER",
"T_IDENTIFER",
"T_IMMED_HEX",
"T_IMMED_DEC",
"T_IMMED_OCT",
"T_IMMED_BIN",
"T_COMMENT",
"T_NEWL",
"T_DATA_BYTE",
"T_DATA_STR",
"T_BRACK_OPEN",
"T_BRACK_CLOS",
"T_COLON",
"T_COMMA",
"T_SHSIGN",
"T_END",
};

enum TOK_INFO{
    TINFO_INDIRECT = 1,

};
struct token_s{
    int lineno;
    int lastline; //when did the line begin, can be used to find column no.
    int tok_type;
    int tok_beg;
    int tok_end; //execlusive
    int tok_modifier; //used to store details such as ... whatever
    int tok_associated_idx; //used to store an index into the symbol table with some tokens
};
struct input_s{
    char *input;
    int idx;
    int len;
    //token info
    struct token_s t;
};



struct sym_s{
    char *name;
    int type;
    int address;
};
enum DECLTYPES{
    DECL_INSTRUCTION,
    DECL_DATA,
    DECL_LABEL,
};

//genericbuffer
struct gbuffer_s{
    char *b;
    long len;
    long size;
};


#define DECL_HEAD \
    int dtype



struct instr_decl_s{
    DECL_HEAD;
    int mnem_idx;
    struct token_s operands[INSTR_MAX_OPERANDS];
    int n_operands;
    int t8_enc; //only relevant for jmp instructions
};
struct data_decl_s{
    DECL_HEAD;
    struct gbuffer_s gbuff;
};
struct label_decl_s{
    DECL_HEAD;
    int sym_idx;
};
union decl_s{
    DECL_HEAD;
    struct instr_decl_s i;
    struct data_decl_s d;
    struct label_decl_s l;
};
struct decl_list_s{
    union decl_s *decls;
    int len;
    int size;
};
struct symboltable_s{
    struct sym_s *symbols;
    int len;
    int size;
};


struct parser_s {
    struct input_s *lexer;
    struct token_s token;
    struct token_s lookahead;

    struct symboltable_s symtab;
    struct decl_list_s dlist;
};

#endif



