/*
 * author: nilputs@nilput.com, 2018
 * see LICENSE file
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include "assembler.h" 


void assertfailed(){ }
#define aassert(cond, msg)\
do {\
    if (!(cond)){\
        fprintf(stderr, "assertion failed: ");\
        fprintf(stderr, msg);\
        assertfailed();\
        exit(1);\
    }\
} while(0)
void err(struct token_s *token, char *msg){
    if (token)
        fprintf(stderr, "near line: %d, ", token->lineno);
    fprintf(stderr, msg);
    fprintf(stderr, "\n");
    exit(1);
}
void warn(char *msg){
    fprintf(stderr, msg);
    fprintf(stderr, "\n");
}

const char *reg_reg     = "add sub xor and shr shl cmp or ldx";
const char *reg_imm     = "add sub xor and shr shl cmp or ldx";
const char *regmem_reg  = "stx";
const char *mem_reg     = "stx";
const char *reg_regmem  = "ldx";
const char *no_operands = "nop hlt in out ret";
const char *reg         = "neg not push pop jcc";
const char *imm         = "call jcc";
const char *jmps        = "jmp jl jle jg jge ja jae jb jbe jz|je jnz|jne"; //order important (enum JMP_COND in data.h)


//update according to whats above, multiple copies of a mnemonic are okay
const char *all_instrs  = "add sub xor and shr shl cmp or ldx add sub xor and shr shl cmp or ldx stx ldx nop hlt in out ret neg not push pop call jmp jl jle jg jge ja jae jb jbe jz je jnz jne";

const char *register_names = "A B C S IP FLAGS"; //order important (enum REG_ENC in data.h)

//keep up to date(data.h)
#define ADDRMMASK 0x7
enum ADDR_MODE{
    NA_M = 0x0,         /* not applicable */
    IMMED_M = 0x1,      /* immediate op16 */
    REG_M = 0x2,        /* RENC */
    MEMA_M = 0x3,       /* memory absolute op16 */
    MEMI_M = 0x4,       /* memory indirect through RENC */ /* second meaning: for jmps relative to instruction */
};
//instruction data, keep up to date
static struct _instrdef{
    uint8_t major_op;
    const char *mnem;
} instrdefs[] = {
//  opcode      mnemonic
//INSTRBEGIN
    {0x10,      "add",      },
    {0x20,      "sub",      },
    {0x30,      "xor",      },
    {0x40,      "and",      },
    {0x50,      "shr",      },
    {0x60,      "shl",      },
    {0x80,      "neg",      },
    {0x90,      "not",      },
    {0xA0,      "cmp",      },
    {0xB0,      "nop",      },
    {0xC0,      "stx",      },
    {0xD0,      "ldx",      },
    {0xE0,      "push",     },
    {0xF0,      "pop",      },
    {0xF8,      "in",       },
    {0xE8,      "out",      },
    {0xD8,      "hlt",      },
    {0xB8,      "jcc",      },
    {0xA8,      "or",       },
    {0x98,      "call",     },
    {0x88,      "ret",      },
//INSTREND
    {0xFF,      "UNDEF",    },
};
int instrdefs_len = sizeof(instrdefs) / sizeof(struct _instrdef);


#define in_range(chr,beg,end) ((chr) >= (beg) && (chr) <= (end))
#define inp(idx) (inps->input[(idx)])
#define __isspace(ch) ((ch) == ' ' || (ch) == '\t')
#define __isdigit(ch) ((ch) >= '0' && (ch) <= '9')
#define __isnl(ch) ((ch) == '\n')
#define __isalphanum(ch) ( ((ch) >= 'a' && (ch) <= 'z') || \
                           ((ch) >= 'A' && (ch) <= 'Z') || \
                           ((ch) >= '0' && (ch) <= '9') || \
                           ((ch) == '_') )

static int match_alpha(struct input_s *inps)
{
    int idx = inps->idx;
    for (; idx < inps->len; ){
        if (in_range(inp(idx), 'a', 'z') || in_range(inp(idx), 'A', 'Z'))
            idx++;
        else 
            break;
    }
    if (idx == inps->idx)
        return 0;
    inps->t.tok_end = idx;
    return 1;
}
static int is_any(int ch, const char *chars){
    while (*chars){
        if (*chars == ch)
            return 1;
        chars++;
    }
    return 0;
}
//offset is just used to skip 0x/0b/0 part
static int match_numeric(struct input_s *inps, int base, int offset)
{
    const char *any;
    switch(base){
        case 2:
            any = "01"; break;
        case 8:
            any = "01234567"; break;
        case 10:
            any = "0123456789"; break;
        case 16:
            any = "0123456789abcdefABCDEF"; break;
        default:
            err(&inps->t, "invalid base");
    }
    int idx = inps->idx + offset;
    int neg  = 0;
    if (inp(idx) == '-'){
        idx++;
        neg = 1;
    }
    for (; idx < inps->len;){
        if (is_any(inp(idx), any))
            idx++;
        else
            break;
    }
    if (idx == inps->idx || (neg && (idx == inps->idx + 1)))
        return 0;
    inps->t.tok_end = idx;
    return 1;
}
static int match_space(struct input_s *inps)
{
    int idx;
    for (idx = inps->idx; idx < inps->len; idx++){
        if (!__isspace(inp(idx)))
            break;
    }
    inps->t.tok_end = idx;
    return idx != inps->idx; //matched something
}
static int match_alphanumeric(struct input_s *inps)
{
    int idx;
    for (idx =  inps->idx; idx < inps->len; idx++){
        if (!__isalphanum(inp(idx)))
            break;
    }
    inps->t.tok_end = idx;
    return idx != inps->idx; //matched something
}
static void incline(struct input_s *inps){
    inps->t.lastline = inps->t.tok_end;
    inps->t.lineno++;
}
static int match_newline(struct input_s *inps)
{
    int idx = inps->idx;
    if (inp(idx) == '\n' ){
        inps->t.tok_end = idx+1;
        incline(inps);
        return 1;
    }
    else if((inp(idx) == '\r') && (inp(idx+1) == '\n')){
        inps->t.tok_end = idx+2;
        incline(inps);
        return 1;
    }
    else
        return 0;
}
static int match_comment(struct input_s *inps)
{
    int idx = inps->idx;
    if(inp(idx) != ';'){
        return 0;
    }
    for (; idx < inps->len; ){
        idx++;
        if (inp(idx) == '\n'){
            incline(inps);
            idx++;
            break;
        }
    }
    inps->t.tok_end = idx;
    return 1;
}
static int match_string_literal(struct input_s *inps){
    int idx = inps->idx;
    aassert(inp(idx) == '"', "match_string_literal called at wrong symbol (not \")");
    for(; idx < inps->len; ){
        idx++;
        if (inp(idx) == '\n')
            return T_NONE; //invalid 
        if (inp(idx) == '"'){
            idx++;
            break;
        }
    }
    inps->t.tok_end = idx;
    inps->t.tok_type = T_DATA_STR;
    return T_DATA_STR;
}

//third argument can be NULL
//an input of abcdef does NOT match abcd, because e is alphanumeric, same with abcd2f
//however abcd, abcd; do match abcd, this is done to avoid having to sort keywords by length
//some of them are prefixes for others, for example jg and jge
static int keywordmatch(const char *input, const char *seplist, int *matchlen)
{
    int inpidx = 0;
    int slidx = 0;
    int which = 1; //1 based index to determine which element was matched
    int len = 0;
    while (seplist[slidx]){
        if ((seplist[slidx] == ' ' || seplist[slidx] == '|') && !__isalphanum(input[inpidx])){
            break; //implies a match
        }
        else if (seplist[slidx] != input[inpidx]){
            while (seplist[slidx] && seplist[slidx] != ' ' && seplist[slidx] != '|')
                slidx++;
            if (!seplist[slidx])
                return 0; //no match
            else if (seplist[slidx] == ' ')
                which++;
            slidx++;
            inpidx = 0;
            len = 0;
            continue;
        }
        slidx++;
        inpidx++;
        len++;
    }
    if(matchlen)
        *matchlen = len;
    return which;
}

//warning, use rhs for the known string
//returns the length of the known string (intended to be the shorter one)
static int str_match(char *s, const char *q)
{
    int l = strlen(q);
    return (strncmp(s, q, l) == 0) ? l : 0;
}


static int lex(struct input_s *inps){
    int idx = inps->idx;
    int n;
    int matchlen;
    if (idx >= inps->len){
        inps->t.tok_type = T_END;
        return T_END;
    }
    inps->t.tok_modifier = 0;
    inps->t.tok_beg = idx;
    if (match_alpha(inps)){
        match_alphanumeric(inps); 
        if ((n = keywordmatch(&inp(inps->idx), all_instrs, &matchlen)) != 0 &&
                                    (matchlen == (inps->t.tok_end - inps->t.tok_beg))){
            aassert(matchlen, "matchlen = 0");
            inps->t.tok_type = T_MNEM;
            inps->t.tok_end = idx+matchlen;
            return T_MNEM;
        }
        else if ((n = keywordmatch(&inp(inps->idx), register_names, &matchlen)) &&
                    (matchlen == (inps->t.tok_end - inps->t.tok_beg))){
            inps->t.tok_type = T_REGISTER;
            inps->t.tok_end = idx+matchlen;
            return T_REGISTER;
        }
        else if (str_match(&inp(idx), "db")){
            inps->t.tok_end = inps->idx + 2;
            inps->t.tok_type = T_DATA_BYTE;
            return T_DATA_BYTE;
        }
        inps->t.tok_type = T_IDENTIFER;
        return T_IDENTIFER;
    }
    //make sure 0 is treated as decimal as that simplifies things
    if (inp(idx) == '0' && (__isdigit(inp(idx+1)) || (inp(idx+1) == 'x') || (inp(idx+1) == 'b')) ){
        if (str_match(&inp(idx), "0x")){
            if (!match_numeric(inps, 16, 2))
                err(&inps->t, "expected a hexdecimal number after 0x");
            inps->t.tok_type = T_IMMED_HEX;
            return T_IMMED_HEX;
        }
        if (str_match(&inp(idx), "0b")){
            if (!match_numeric(inps, 2, 2))
                err(&inps->t, "expected a binary number after 0b");
            inps->t.tok_type = T_IMMED_BIN;
            return T_IMMED_BIN;
        }
        else{
            match_numeric(inps, 8, 1);
            inps->t.tok_type = T_IMMED_OCT;
            return T_IMMED_OCT;
        }
    }
    if (match_numeric(inps, 10, 0)){
        inps->t.tok_type = T_IMMED_DEC;
        return T_IMMED_DEC;
    }
    if (match_space(inps)){
        inps->t.tok_type = T_WHITESPACE;
        return T_WHITESPACE;
    }
    if (match_newline(inps)){
        inps->t.tok_type = T_NEWL;
        return T_NEWL;
    }
    if (match_comment(inps)){
        inps->t.tok_type = T_COMMENT;
        return T_COMMENT;
    }
    switch (inp(inps->idx)){
        case ',':
            inps->t.tok_end = inps->idx+1;
            inps->t.tok_type = T_COMMA;
            return T_COMMA;
            break; //for warnings
        case ':':
            inps->t.tok_end = inps->idx+1;
            inps->t.tok_type = T_COLON;
            return T_COLON;
            break; //for warnings
        case '[':
            inps->t.tok_end = inps->idx+1;
            inps->t.tok_type = T_BRACK_OPEN;
            return T_BRACK_OPEN;
            break; //for warnings
        case ']':
            inps->t.tok_end = inps->idx+1;
            inps->t.tok_type = T_BRACK_CLOS;
            return T_BRACK_CLOS;
            break; //for warnings
        case '"':
            n = match_string_literal(inps);
            if (n == T_NONE){
                err(&inps->t, "unterminated string literal");
            }
            return n;
            break;
    }
    inps->t.tok_type = T_UNKNOWN;
    inps->t.tok_end = idx+1;
    return T_UNKNOWN;
}

enum SKIP_ENUM{
    SKIP_WHITESPACE = 1,
    SKIP_NL = 2,
    SKIP_COMMENT = 4,
};

static int lexs(struct input_s *inps, int skipopt){
    int c = -1;
    while(c != T_END && c != T_UNKNOWN){
        c = lex(inps);
        if ((skipopt & SKIP_WHITESPACE) && c == T_WHITESPACE){
            inps->idx = inps->t.tok_end;
            continue;
        }
        if ((skipopt & SKIP_NL) && c == T_NEWL){
            inps->idx = inps->t.tok_end;
            continue;
        }
        if ((skipopt & SKIP_COMMENT) && c == T_COMMENT){
            inps->idx = inps->t.tok_end;
            continue;
        }
        break;
    }
    return c;
}

static char *extract_token(struct token_s *tok, struct input_s *inps)
{
    int len = tok->tok_end - tok->tok_beg;
    char *buff = malloc(len+1);
    if (!buff)
        return NULL;
    memcpy(buff, &inp(tok->tok_beg), len);
    buff[len] = '\0';
    return buff;
}
//dbg
static void represent_dlist(struct decl_list_s *dlist, struct symboltable_s *symtab)
{
    for (int i=0; i<dlist->len; i++){
        struct decl_s *decl = &(dlist->decls[i]);
        switch(decl->dtype){
            case DECL_INSTRUCTION:
                aassert(decl->u.i.mnem_idx < instrdefs_len, "out of range instruction idx");
                printf("(instr)    %s ", instrdefs[decl->u.i.mnem_idx].mnem);
                for (int j=0; j<decl->u.i.n_operands; j++){
                    if (j > 0){
                        printf(", ");
                    }
                }
                printf("\n");
                break;
            case DECL_DATA:
                printf("(data) %d bytes", decl->u.d.gbuff.len);
                printf("\n");
                break;
            case DECL_LABEL:
                printf("(label) name: '%s', type: %d, address: 0x%x\n", symtab->symbols[decl->u.l.sym_idx].name,
                                                                        symtab->symbols[decl->u.l.sym_idx].type,
                                                                        symtab->symbols[decl->u.l.sym_idx].address);

                break;
            default:
                printf("\n\n\tinvalid decl!\n\n");
        }
    }
}
static void represent_token(struct token_s *tok, struct input_s *inps)
{
    char *token_str = extract_token(tok, inps);
    if (!token_str){
        warn("error: extract_token failed\n");
        return;
    }
    printf("current token: %s \"%s\"\n", token_names[tok->tok_type], token_str);
    free(token_str);
}

static int findn_instr(const char *mnem, long len){
    for(int i=0; i<instrdefs_len; i++){
        if (strncmp(mnem, instrdefs[i].mnem, len) == 0)
            return i;
    }
    return -1;
}
static int find_instr(const char *mnem){
    return findn_instr(mnem, strlen(mnem));
}


/*
    syntax
    progarm : statement_list
    statement_list  :   instr_def statement_list 
                    |   label_def statement_list
                    |   data_def  statement_list
                    | none

    instr_def : mnem op_list
    label_def : identifier colon
    data_def : 'db' op_list

    immed   : label | hex | dec | bin 
    operand : immed | register | [ register ] 

    op_list : operand COMMA op_list
            : none


    predictive grammar
    %(descend if)%
    %(nt)% if next token is
    %(la)% if lookahead is
    progarm : statement_list %(always)%
    statement_list  :   instr_def statement_list  %(nt mnem)%
                    |   label_def statement_list  %(nt identifier)%
                    |   data_def  statement_list  %(nt 'db')%
                    |   none

    instr_def : mnem op_list %(nt mnem)%
    label_def : identifier colon %(nt identifier)%
    data_def : 'db' op_list %(nt 'db')%

    immed   : label | hex | dec | bin %(nt is one of the respective tokens)%
    operand : register %(nt is register)%
            | [ register ]  %(nt is bracket)%
            | immed  %(nt is one of (label | hex | dec | bin))%
            

    op_list : operand op_list %(nt is one of (label | hex | dec | bin | brack_open | register ))%
            : none

*/
static int pnext(struct parser_s *prs){
    if (prs->token.tok_type == T_END){
        return T_END;
    }
    //use lookahead
    memcpy(&prs->token, &prs->lookahead, sizeof(struct token_s));
    //keep the lookahead as it is (T_END), because no further tokens are there
    if (prs->token.tok_type == T_END){
        return T_END;
    }
    prs->lexer->idx = prs->lexer->t.tok_end;
    lexs(prs->lexer, SKIP_WHITESPACE | SKIP_NL | SKIP_COMMENT);
    memcpy(&prs->lookahead, &(prs->lexer->t), sizeof(struct token_s));
    return prs->token.tok_type;
}
static int pmatch(struct parser_s *prs, int tok){
    return prs->token.tok_type == tok;
}
static int pla_match(struct parser_s *prs, int tok){
    return prs->lookahead.tok_type == tok;
}

#define SYMTAB_INIT_SIZE 8
#define DECLLIST_INIT_SIZE 8
static void symboltable_init(struct symboltable_s *symtab)
{
    symtab->symbols = malloc(sizeof(struct sym_s) * SYMTAB_INIT_SIZE);
    if (!symtab->symbols){
        err(NULL, "no memory for symboltable");
    }
    symtab->len = 0;
    symtab->size = SYMTAB_INIT_SIZE;
}
static void symboltable_destroy(struct symboltable_s *symtab)
{
    for(int i=0; i<symtab->len; i++){
        free(symtab->symbols[i].name);
        symtab->symbols[i].name = NULL;
    }
    free(symtab->symbols);
    symtab->symbols = NULL;
}
static void decl_list_init(struct decl_list_s *dlist)
{
    dlist->size = DECLLIST_INIT_SIZE;
    dlist->decls = malloc(sizeof(struct decl_s) * dlist->size);
    if (!dlist->decls){
        err(NULL, "couldnt allocate memory for decl_list_init");
    }
    dlist->len = 0;
}
static int decl_list_push(struct decl_list_s *dlist, struct decl_s *decl)
{

    struct decl_s *new_mem = NULL;
    if (dlist->len == dlist->size){
        new_mem = realloc(dlist->decls, sizeof(struct decl_s) * dlist->size*2);
        if (!new_mem)
            err(NULL, "no memory when attempting to grow decllist");
        dlist->decls = new_mem;
        dlist->size *= 2;
    }
    memcpy(dlist->decls + dlist->len, decl, sizeof(struct decl_s));
    return dlist->len++;
}
static void decl_list_destroy(struct decl_list_s *dlist){
    free(dlist->decls);
    dlist->decls = NULL;
}
static int symboltable_find(struct symboltable_s *symtab, const char *id)
{
    for(int i=0; i<symtab->len; i++){
        if (strcmp(symtab->symbols[i].name, id) == 0){
            return i;
        }
    }
    return -1;
}
enum SYMTYPE{
    SYMLABEL,
};
//returns the new idx
//OWNS the id pointer (frees it with malloc)
static int symboltable_push(struct symboltable_s *symtab, char *id, int type, int address)
{
    struct sym_s *new_mem = NULL;
    if (symtab->len == symtab->size){
        new_mem = realloc(symtab->symbols, sizeof(struct sym_s) * symtab->size*2);
        if (!new_mem)
            err(NULL, "no memory when attempting to grow symboltable");
        symtab->symbols = new_mem;
        symtab->size *= 2;
    }
    if (symboltable_find(symtab, id) != -1){
        return -1; //duplicate
    }
    symtab->symbols[symtab->len].name = id;
    symtab->symbols[symtab->len].type = type;
    symtab->symbols[symtab->len].address = address;
    return symtab->len++;
}


int gbuff_init(struct gbuffer_s *gbuf, long size)
{
    gbuf->b = malloc(size);
    if (!gbuf->b){
        err(NULL, "gbuf_init(): no memory");
    }
    gbuf->size = size;
    gbuf->len = 0;
    return 1;
}
void gbuff_free(struct gbuffer_s *gbuf)
{
    free(gbuf->b);
    gbuf->b = NULL;
}
int gbuff_grow(struct gbuffer_s *gbuf, long atleast)
{
    if (gbuf->size >= atleast)
        return 1;
    long new_size = gbuf->size;
    if (new_size <= 0)
        err(NULL, "invalid size");
    while(new_size < atleast)
        new_size *= 2;
    char *newmem = realloc(gbuf->b, new_size);
    if (!newmem)
        err(NULL, "gbuff_grow(): no mem");
    gbuf->size = new_size;
    gbuf->b = newmem;
    return 1;
}

int gbuff_memcpy(struct gbuffer_s *gbuf, void *src, long size)
{
    if(!gbuff_grow(gbuf, size + gbuf->len))
        return 0;
    memcpy(gbuf->b + gbuf->len, src, size);
    gbuf->len += size;
    return 1;
}





static void pinit(struct parser_s *prs, struct input_s *lexer)
{
    int t;
    aassert(lexer, "pinit(): invalid lexer at initialization");
    prs->lexer = lexer;
    t = lexs(prs->lexer,  SKIP_WHITESPACE | SKIP_NL | SKIP_COMMENT);
    memcpy(&prs->token, &lexer->t, sizeof(struct token_s));
    if (t == T_END)
        goto second;
    lexer->idx = lexer->t.tok_end;
    t = lexs(prs->lexer,  SKIP_WHITESPACE | SKIP_NL | SKIP_COMMENT);
    second:
    memcpy(&prs->lookahead, &lexer->t, sizeof(struct token_s));
    symboltable_init(&prs->symtab);
    decl_list_init(&prs->dlist);
}
static void pdestroy(struct parser_s *prs){
    decl_list_destroy(&prs->dlist);
    symboltable_destroy(&prs->symtab);

}
#ifdef DBG
#define ddprint(msg) \
    do {\
        fprintf(stderr, msg); \
    } while(0)
#else
#define ddprint 0
#endif

static void *xmalloc(size_t s)
{
    void *m  = malloc(s);
    if (!m){
        fprintf(stderr, "xmalloc(): no mem\n");
        exit(1);
    }
    return m;
}
static void print_coords(struct token_s *tok, struct input_s *inps)
{
    char *tokstr = extract_token(tok, inps);
    printf("at line %d, token: '%s'\n", tok->lineno, tokstr);
}


#define CHECK_LIMIT() \
        if (n > INSTR_MAX_OPERANDS){\
            print_coords(&prs->token, prs->lexer);\
            err(NULL, "operands > INSTR_MAX_OPERANDS");\
        }

//this includes literal immediates and symbols which resolve to their addresses
static int is_immediate(struct token_s *tok){
    return tok->tok_type == T_IMMED_BIN  ||
           tok->tok_type == T_IMMED_HEX  ||
           tok->tok_type == T_IMMED_OCT  ||
           tok->tok_type == T_IMMED_DEC  ||
           tok->tok_type == T_IDENTIFER;
}
//return 1 on success
static int get_immediate(struct token_s *tok, struct parser_s *prs, long *lval){
    int base = 0;
    char *s = extract_token(tok, prs->lexer);
    char *v = s;
    char *endptr = NULL;
    int symidx;
    if (!s){
        err(NULL, "get_immediate(): failed to extract token");
    }
    switch(tok->tok_type){
        case T_IMMED_BIN: 
            v += 2; //skip 0b
            base = 2; break; 
        case T_IMMED_OCT: 
            v += 1; //skip 0
            base = 8; break;
        case T_IMMED_HEX: 
            v += 2; //skip 0x
            base = 16; break;
        case T_IMMED_DEC:
            base = 10; break;
        case T_IDENTIFER:
            goto get_sym_address; break;
        default:
            goto invalid;
    }

    errno = 0;
    //TODO: check that the value fits
    *lval = strtol(s, &endptr, base);
    if (!errno && (*endptr == 0)){
        free(s);
        return 1;
    }
    free(s);
    return 0;
get_sym_address:
    symidx = symboltable_find(&prs->symtab, s);
    free(s);
    if (symidx < 0)
        return 0;
    if (prs->symtab.symbols[symidx].address == -1)
        return 0;
    *lval = prs->symtab.symbols[symidx].address;
    return 1;
invalid:
    free(s);
    return 0;
}

int instr_op_list(struct decl_s *dest_decl, struct parser_s *prs)
{
    int t = prs->token.tok_type;
    int n = 0;
    while (1){
        if (is_immediate(&prs->token) && t != T_IDENTIFER){
            CHECK_LIMIT();
            memcpy(&dest_decl->u.i.operands[n], &prs->token, sizeof(struct token_s));
        }
        else if (t == T_IDENTIFER && !pla_match(prs, T_COLON)){
            memcpy(&dest_decl->u.i.operands[n], &prs->token, sizeof(struct token_s));
            dest_decl->u.i.operands[n].tok_associated_idx = -1;
        }
        else if (t == T_BRACK_OPEN){
            CHECK_LIMIT();
            t = pnext(prs); //skip bracket
            if (t == T_REGISTER ||
                (is_immediate(&prs->token) && t != T_IDENTIFER))
            {
                memcpy(&dest_decl->u.i.operands[n], &prs->token, sizeof(struct token_s));
                dest_decl->u.i.operands[n].tok_modifier |= TINFO_INDIRECT;
                t = pnext(prs); //skip whats inside it
                if (t != T_BRACK_CLOS){
                    print_coords(&prs->token, prs->lexer);\
                    err(&prs->token, "expected ]");\
                }
            }
            else{
                print_coords(&prs->token, prs->lexer);\
                err(&prs->token, "invalid indirect value");\
            }
        }
        else if (t == T_REGISTER){
            CHECK_LIMIT();
            memcpy(&dest_decl->u.i.operands[n], &prs->token, sizeof(struct token_s));
        }
        else{
            if (n > 0){ //this must match, so its an error
                print_coords(&prs->token, prs->lexer);\
                err(&prs->token, "unexpected token");\
            }
            //otherwise there are no operands
            aassert(n == 0, "expected n to be 0");
            break;
        }
        //we have just added an operand
        n++;
        pnext(prs);
        if (pmatch(prs, T_COMMA))
            pnext(prs); //skip comma
        else{
            break;
        }
        t = prs->token.tok_type;
    }
    
    dest_decl->u.i.n_operands = n;
    return n;
}

int data_op_list(struct decl_s *dest_decl, struct parser_s *prs, int bytes)
{
    aassert(bytes < 2, "invalid argument bytes, expected 1 or 2");
    const int byte_max[3] = {0,255, 65535};
    int t;
    int n = 0;
    struct data_decl_s *ddecl = &dest_decl->u.d;
    long lval;
    uint16_t val;
    pnext(prs); //skip dataspecifier
    while (1){
        t = prs->token.tok_type;
        if (is_immediate(&prs->token) && t != T_IDENTIFER){
            if(!get_immediate(&prs->token, prs, &lval) || (lval > byte_max[bytes])){
                fprintf(stderr, "at data ");
                print_coords(&prs->token, prs->lexer);
                err(&prs->token, "invalid immediate");
            }
            /* printf("immd: %ld\n", lval); */
            //TODO, LITTLE ENDIAN ASSUMPTION
            val = lval;
            gbuff_memcpy(&ddecl->gbuff, &val, bytes);
        }
        else if (t == T_DATA_STR){
            int len = (prs->token.tok_end-1) - (prs->token.tok_beg+1); //skip \"
            aassert(len > 0, "empty string");
            gbuff_memcpy(&ddecl->gbuff, prs->lexer->input + prs->token.tok_beg+1, len);
        }
        else{
            if (n == 0){ //this must match, so its an error
                print_coords(&prs->token, prs->lexer);
                err(&prs->token, "expected data after dataspecifier");
            }
            break; //trailing comma is allowed on n > 0
        }
        //we have just added an operand
        n++;
        pnext(prs);
        if (pmatch(prs, T_COMMA))
            pnext(prs); //skip comma
        else{
            break;
        }
    }
    
    dest_decl->u.i.n_operands = n;
    return n;
}


#undef CHECK_LIMIT


int instr_def(struct decl_s *dest_decl, struct parser_s *prs)
{
    aassert(prs->token.tok_type == T_MNEM, "instr_def(): expected an instruction");
    dest_decl->dtype = DECL_INSTRUCTION;
    dest_decl->u.i.t8_enc = 0;
    char *buff = prs->lexer->input;
    int mnem_idx = findn_instr(buff+prs->token.tok_beg, prs->token.tok_end - prs->token.tok_beg);
    int which, matchlen;
    if (mnem_idx == -1){
        if ((which = keywordmatch(buff+prs->token.tok_beg, jmps, &matchlen))){
            mnem_idx = find_instr("jcc");
            dest_decl->u.i.t8_enc = which;
            aassert(mnem_idx != 0, "jcc instr def was not found");
        }
        else{
            print_coords(&prs->token, prs->lexer);
            err(&prs->token, "invalid instruction");
        }
    }
    pnext(prs);
    dest_decl->u.i.mnem_idx = mnem_idx;
    instr_op_list(dest_decl, prs);

    return 1;
}

int data_def(struct decl_s *dest_decl, struct parser_s *prs)
{
    aassert(prs->token.tok_type == T_DATA_BYTE, "data_def(): datatype specifier");
    dest_decl->dtype = DECL_DATA;
    gbuff_init(&(dest_decl->u.d.gbuff), 8);
    data_op_list(dest_decl, prs, 1);
    return 1;
}

int label_def(struct decl_s *dest_decl, struct parser_s *prs)
{
    aassert(prs->token.tok_type == T_IDENTIFER, "label_def(): expected a label");
    dest_decl->dtype = DECL_LABEL;
    char *label = extract_token(&prs->token, prs->lexer);
    if (!label){
        err(NULL, "not enough memory / error while storing label");
    }
    int idx = symboltable_push(&prs->symtab, label, SYMLABEL, -1);
    if (idx < 0 ){
        err(NULL, "couldnt add symbol, (symboltable_push())");
    }
    dest_decl->u.l.sym_idx = idx;
    if (pnext(prs) != T_COLON){
        print_coords(&prs->token, prs->lexer);
        err(NULL, "expected colon after identifier");
    }
    pnext(prs); //skip colon
    return 1;
}

int statement_list(struct parser_s *prs){
    int n = 0;
    struct decl_s ldecl; //temp
    while(1){
        memset(&ldecl, 0, sizeof(struct decl_s));
        ldecl.t = prs->token;
        if (pmatch(prs, T_MNEM)){
            instr_def(&ldecl, prs);
            decl_list_push(&prs->dlist, &ldecl);
        }
        else if (pmatch(prs, T_IDENTIFER)){
            label_def(&ldecl, prs);
            decl_list_push(&prs->dlist, &ldecl);
        }
        else if (pmatch(prs, T_DATA_BYTE)){
            data_def(&ldecl, prs);
            decl_list_push(&prs->dlist, &ldecl);
        }
        else{
            if (prs->token.tok_type != T_END){
                fprintf(stderr, "unexpected token: ");
                print_coords(&prs->token, prs->lexer);
                err(&prs->token, "unknown token");
            }
            break;
        }
        n++;
    }
    return n;
}
int program(struct parser_s *prs, struct input_s *lexer)
{
    pinit(prs, lexer);
    return statement_list(prs);
}

void read_file(FILE *f, char **ptr, long *bfsize, long *read)
{
    long last_read = 0, total_read = 0, size = 8;
    char *buff = malloc(size+1);
    if (!buff)
        err(NULL, "no mem");
    last_read = fread(buff+total_read, 1, size-total_read, f);
    while(last_read > 0){
        total_read += last_read;
        size *= 2;
        buff = realloc(buff, size+1);
        if (!buff)
            err(NULL, "realloc failed");
        last_read = fread(buff+total_read, 1, size-total_read, f);
    }
    *read = total_read;
    *bfsize = size;
    *ptr = buff;
    buff[total_read] = '\0';
}

void help_exit(char *msg){
    if (msg){
        fprintf(stderr, msg);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, ""
    "vcpu Assembler v0.1\n"
    "options:"
    "   -i [input file]\n"
    "   -o [output binary]\n");
    exit(1);
}


union encinstr{
    struct{
        uint32_t t32;
    };
    struct{
        uint16_t t16a;
        uint16_t t16b;
    };
    struct{
        uint8_t t8a;
        uint8_t t8b;
        uint8_t t8c;
        uint8_t t8d;
    };
};

uint8_t rname_encoding(struct token_s *tok, struct input_s *inps){
    //using the order of register_names string
    char *extracted = extract_token(tok, inps);
    int enc = keywordmatch(extracted, register_names, NULL);
    free(extracted);
    return enc;
}

uint32_t encode_instr(struct decl_s *decl, struct parser_s *prs)
{
    //TODO, make it portable, now only works in little endian machines
    aassert(decl->dtype == DECL_INSTRUCTION, "invalid decl type");
    struct instr_decl_s *instr = &decl->u.i;
    int midx = instr->mnem_idx;
    union encinstr encoded;
    long lval;
    encoded.t32 = 0;
    aassert(midx >= 0, "instruction was not initialized");
    struct _instrdef *instrdef = &instrdefs[midx];
    if (instr->n_operands == 0){
        if (keywordmatch(instrdef->mnem, no_operands, NULL)){
            encoded.t8a = instrdef->major_op + 0x0;
        }
        else{
            fprintf(stderr, "instruction %s cannot be encoded with no operands ", instrdef->mnem);
            err(NULL, "");
        }
    }
    else if (instr->n_operands == 1){
        //see instr_def, it does some of the work for us
        //the second byte .t8b will be the jmp extension (jg,jl etc..)
        //in other instructions it is 0, this is only relevant in instruction with 1 operand
        encoded.t8b = instr->t8_enc;

        if (instr->operands[0].tok_type == T_REGISTER && keywordmatch(instrdef->mnem, reg, NULL)){
            encoded.t8a = instrdef->major_op + REG_M;
            encoded.t8c = rname_encoding(&instr->operands[0], prs->lexer);
            if (!encoded.t8c){
                fprintf(stderr, "at instruction %s ", instrdef->mnem);
                err(&decl->t, "invalid register name");
            }
        }
        else if (is_immediate(&instr->operands[0]) && keywordmatch(instrdef->mnem, imm, NULL)){
            encoded.t8a = instrdef->major_op + IMMED_M;
            if(!get_immediate(&instr->operands[0], prs, &lval)){
                fprintf(stderr, "at instruction %s ", instrdef->mnem);
                err(&decl->t, "invalid immediate");
            }
            //check if it fits
            encoded.t16b = lval;
        }
        else{
            fprintf(stderr, "instruction %s cannot be encoded with these operands", instrdef->mnem);
            err(&decl->t, "");
        }
    }
    else if (instr->n_operands == 2){
        if(instr->t8_enc != 0){
            fprintf(stderr, "instruction %s cannot be encoded with these operands (invalid extension)",
                                                                                        instrdef->mnem);
            err(&decl->t, "");
        }
        if (instr->operands[0].tok_type == T_REGISTER &&
                 instr->operands[1].tok_type == T_REGISTER &&
                 (instr->operands[1].tok_modifier & TINFO_INDIRECT) &&
                 keywordmatch(instrdef->mnem, reg_regmem, NULL)) //{ldx reg, [reg]}
        {
            encoded.t8a = instrdef->major_op + MEMI_M;
            encoded.t8b = rname_encoding(&instr->operands[0], prs->lexer);
            encoded.t8c = rname_encoding(&instr->operands[1], prs->lexer);
            if (!encoded.t8b || !encoded.t8c){
                fprintf(stderr, "at instruction %s ", instrdef->mnem);
                err(&decl->t, "invalid register name");
            }
        }
        else if (instr->operands[0].tok_type == T_REGISTER &&
                 (instr->operands[0].tok_modifier & TINFO_INDIRECT) &&
                 instr->operands[1].tok_type == T_REGISTER &&
                 keywordmatch(instrdef->mnem, regmem_reg, NULL)) //{stx [reg], reg}, flipped operands
        {
            encoded.t8a = instrdef->major_op + MEMI_M;
            encoded.t8b = rname_encoding(&instr->operands[1], prs->lexer);
            encoded.t8c = rname_encoding(&instr->operands[0], prs->lexer);
            if (!encoded.t8b || !encoded.t8c){
                fprintf(stderr, "at instruction %s ", instrdef->mnem);
                err(&decl->t, "invalid register name");
            }
        }
        else if (instr->operands[0].tok_type == T_REGISTER &&
                instr->operands[1].tok_type == T_REGISTER &&
                keywordmatch(instrdef->mnem, reg_reg, NULL)) //other instructions like add,and,or ...
        {
            encoded.t8a = instrdef->major_op + REG_M;
            encoded.t8b = rname_encoding(&instr->operands[0], prs->lexer);
            encoded.t8c = rname_encoding(&instr->operands[1], prs->lexer);
            if (!encoded.t8b || !encoded.t8c){
                fprintf(stderr, "at instruction %s ", instrdef->mnem);
                err(&decl->t, "invalid register name");
            }
        }
        else if (instr->operands[0].tok_type == T_REGISTER &&
                 is_immediate(&instr->operands[1]) &&
                 keywordmatch(instrdef->mnem, reg_imm, NULL)) //add,and, {ldx immed}
        {
            encoded.t8a = instrdef->major_op + IMMED_M;
            encoded.t8b = rname_encoding(&instr->operands[0], prs->lexer);
            if(!get_immediate(&instr->operands[1], prs, &lval)){
                fprintf(stderr, "at instruction %s ", instrdef->mnem);
                err(&decl->t, "invalid immediate");
            }
            //check
            encoded.t16b = lval;
        }
        else{
            fprintf(stderr, "instruction %s cannot be encoded with these operands", instrdef->mnem);
            err(&decl->t, "");
        }
    }
    else 
        err(&decl->t, "invalid number of operands");
    return encoded.t32;
}



int assemble(struct parser_s *prs, FILE *out)
{

    struct gbuffer_s gbuf;
    gbuff_init(&gbuf, 256);
    //first pass, find addresses
    int address = 0;
    for(int i=0; i<prs->dlist.len; i++){
        struct decl_s *decl = &prs->dlist.decls[i];
        if (decl->dtype == DECL_INSTRUCTION){
            address += 4; //32bit
        }
        else if (decl->dtype == DECL_DATA){
            address += decl->u.d.gbuff.len;
        }
        else if (decl->dtype == DECL_LABEL){
            prs->symtab.symbols[decl->u.l.sym_idx].address = address;
        }
    }
    gbuff_grow(&gbuf, address);
    address = 0;
    for(int i=0; i<prs->dlist.len; i++){
        struct decl_s *decl = &prs->dlist.decls[i];
        uint32_t instr;
        if (decl->dtype == DECL_INSTRUCTION){
            address += 4; //32bit
            instr = encode_instr(decl, prs);
            gbuff_memcpy(&gbuf, &instr, sizeof(uint32_t));
        }
        else if (decl->dtype == DECL_DATA){
            address += decl->u.d.gbuff.len;
            gbuff_memcpy(&gbuf, decl->u.d.gbuff.b, decl->u.d.gbuff.len);
            gbuff_free(&decl->u.d.gbuff);
        }
    }
    long written  = fwrite(gbuf.b, 1, gbuf.len, out);
    printf("written %ld/%ld bytes\n", written, gbuf.len);
    gbuff_free(&gbuf);
    return 1;
}

int main(int argc, const char **argv){
    char *filecontent;
    long fbfsize;
    long flen;
    FILE *in;
    FILE *out;
    char *fin_path = NULL, *fout_path = NULL;
    char lex_info = 0;
    char verbose = 0;

    int c;
    while ((c = getopt (argc,  (char * const *)argv, "i:o:vlh")) != -1)
        switch (c)
        {
        case 'i':
            fin_path = optarg;
            break;
        case 'o':
            fout_path = optarg;
            break;
        case 'l':
            lex_info = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            help_exit(NULL);
            return 1;
        case '?':
        default:
            fprintf(stderr, "Option -%c requires an argument.\n", c);
            help_exit(NULL);
            break;
        }

    if (fin_path){
        in = fopen(fin_path, "r");
        if (!in){
            fprintf(stderr, "couldnt open input file %s", fin_path);
            perror("fopen: ");
            err(NULL, "");
        }
    }
    else{
        help_exit("didn't provide an input file");
    }

    if (fout_path){
        out = fopen(fout_path, "wb");
        if (!out){
            fprintf(stderr, "couldnt open output file %s", fout_path);
            perror("fopen: ");
            err(NULL, "");
        }
    }
    else{
        help_exit("didn't provide an output file");
    }

    read_file(in, &filecontent, &fbfsize, &flen);
    if (!filecontent){
        err(NULL, "couldn't read file content\n");
    }
    struct input_s inps = {
        .input = filecontent,
        .len = flen,
        .idx = 0,
        .t = {
            .tok_beg = 0,
            .lineno = 1,
            .lastline = 0,
        }
    };

    if (lex_info){
        while (lexs(&inps, SKIP_WHITESPACE) != T_END){
            represent_token(&inps.t,&inps);
            inps.idx = inps.t.tok_end;
        }
        /* int t = find_instr("hlt"); */
        /* printf("hlt idx: %d, op: %x\n", t, (unsigned) instrdefs[t].major_op); */
        /* t = find_instr("cmp"); */
        /* printf("cmp idx: %d, op: %x\n", t, (unsigned) instrdefs[t].major_op); */
        return 0;
    }
    struct parser_s prs;
    int p = program(&prs, &inps);
    if (verbose){
        represent_dlist(&prs.dlist, &prs.symtab);
        return 0;
    }
    p = assemble(&prs, out);
    if(!p){
        fprintf(stderr, "an error has occured during assembly\n");
    }
    pdestroy(&prs);
    free(inps.input);
    inps.input = NULL;
}
