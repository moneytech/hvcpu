#!/usr/bin/env python3
'''
Author: nilput (check license file)
License: MIT
'''


import sys, re, os, os.path
import argparse

def error(*args,**kwargs):
    print(*args, **kwargs, file=sys.stderr)
    sys.exit(1)

lineno = 0
filename = ''
instr_defs = None
quick_defs = None
stdin_linux = '/dev/fd/0'
debug = 0
INSTRUCTION_LENGTH = 32//8

'''
kinds of instructions
   (dest, source)

    R, R       
    M, R       
    [R], R       
    R, M       
    R, I  
    nothing    
    R
'''

I_COUNT = 1000
def genint():
    global I_COUNT
    I_COUNT += 1
    return I_COUNT



def fixregx(regex_str):
    #'_' is to be replaced by a white space ignoring pattern
    return regex_str.replace('_',r'[ \t]*')


#Operand type
OT_IMMEDIATE_DEFERRED   = genint() 
OT_IMMEDIATE_INTEGER    = genint() 
OT_REGISTER             = genint() 
OT_MEM_REGISTER         = genint() 


#Token type
T_WHITESPACE    = genint() 
T_MNEM   = genint() 
T_REGISTER      = genint()
T_IDENTIFER     = genint() 
T_IMMED_HEX     = genint() 
T_IMMED_DEC     = genint() 
T_IMMED_OCT     = genint() 
T_IMMED_BIN     = genint() 
T_COMMENT       = genint() 
T_NEWL          = genint()  #newline

T_DATA_BYTE     = genint() 
T_DATA_STR      = genint() 
T_BRACK_OPEN    = genint() 
T_BRACK_CLOS    = genint() 
T_COLON         = genint() 
T_COMMA         = genint() 
T_SHSIGN        = genint() 

T_END           = genint() 



#instruction categories
reg_reg     = 'add sub xor and shr shl cmp or ldx'               .split(' ')
reg_imm     = 'add sub xor and shr shl cmp or ldx'               .split(' ')
regmem_reg  = 'stx'                                         .split(' ')
reg_regmem  = 'ldx'                                         .split(' ')
no_operands = 'nop hlt in out ret'                           .split(' ')
reg         = 'neg not push pop'                             .split(' ')
imm         = 'call'                                         .split(' ')
jmps        = 'jmp jl jle jg jge ja jae jb jbe jz je jnz jne'.split(' ')
all_instrs  = reg_reg + reg_imm + no_operands + reg + jmps + imm + regmem_reg + reg_regmem
all_instrs_regex  = r'(' + ('|'.join(all_instrs)) + r')[^a-zA-Z]'


#           token symbol        regex                   group     skip to group's span end
token_defs = (\
                (T_WHITESPACE   ,   r'[ \t]+'                   , 0,    0), \
                (T_MNEM         ,   all_instrs_regex            , 1,    1), \
                (T_REGISTER     ,   r'A|B|C|S|FLAGS|IP'         , 0,    0), \
                (T_DATA_BYTE    ,   r'db'                       , 0,    0), \
                (T_DATA_STR     ,   r'"([^"\n]*)"'              , 1,    0), \
                (T_IDENTIFER    ,   r'[a-zA-Z][a-zA-Z0-9_]*'    , 0,    0), \
                (T_BRACK_OPEN   ,   r'\['                       , 0,    0), \
                (T_BRACK_CLOS   ,   r'\]'                       , 0,    0), \
                (T_COLON        ,   r':'                        , 0,    0), \
                (T_COMMA        ,   r','                        , 0,    0), \
                (T_SHSIGN       ,   r'#'                        , 0,    0), \
                (T_IMMED_HEX    ,   r'0x[abcdefABCDEF0-9]+'     , 0,    0), \
                (T_IMMED_BIN    ,   r'0b[01]+'                  , 0,    0), \
                (T_IMMED_OCT    ,   r'0[0-7]*'                  , 0,    0), \
                (T_IMMED_DEC    ,   r'[1-9][0-9]*'              , 0,    0), \
                (T_COMMENT      ,   r';[^\n]*'                  , 0,    0), \
                (T_NEWL         ,   r'\n'                       , 0,    0), \
             )

#instruction_data, these are tied to the c implementation !!
#the way jmps are done is that they have one 'jcc' opcode prefix, and the first field in the instruction
#is used for the condition, the last two fields are used for the destination
JMPS = {
   'jmp': 0x1,
   'jl':  0x2,
   'jle': 0x3,
   'jg':  0x4,
   'jge': 0x5,
   'ja':  0x6,
   'jae': 0x7,
   'jb':  0x8,
   'jbe': 0x9,
   'jz':  0xA,
   'je':  0xA,
   'jnz': 0xB,
   'jne': 0xB,
}
ADDR_MODE = {
   'NA_M' : 0x0,  
   'IMMED_M' : 0x1,  
   'REG_M'   : 0x2,    
   'MEMA_M'  : 0x3,   
   'MEMI_M'  : 0x4,   
}

RENC = {
   'A'     : 1,
   'B'     : 2,
   'C'     : 3,
   'S'     : 4,
   'IP'    : 5,
   'FLAGS' : 6,
}
#end of instruction_data that's tied to the c implementation


class coord:
    def __init__(self, filename, lineno, colno):
        self.filename = filename
        self.lineno = lineno
        self.colno = colno
    def __repr__(self):
        return '%s:%d' % (self.filename, self.lineno)
    def verbose_repr(self):
        return self.__repr__() + (':%d' % (self.colno))

class token:
    def __init__(self, tid, val, xyz):
        self.tid = tid
        self.val = val
        self.xyz = xyz
    def tidval(self):
        return self.tid, self.val
    def is_eof(self):
        return self.tid == T_END
    def is_invalid(self):
        return self.tid is None
    def __bool__(self):
        return (not self.is_invalid()) and (not self.is_eof())
    def __repr__(self):
        if not self:
            return '(token %s, %s)' % (rep_I(self.tid) if self.tid is not None else 'INVALID', self.val)
        return '(token %s, "%s")' % (rep_I(self.tid), self.val)


class token_source:
    DEFAULT_STACK_MAX = 5
    DEFAULT_SKIP = (T_WHITESPACE, T_NEWL, T_COMMENT)
    DEFAULT_SKIP_ARG = -1
    def __init__(self, input_file):
        self.content = input_file.read()
        self.idx = 0
        self.lineno = 1
        self.undo_stack = [(self.idx, self.lineno)]


    def next_token(self, skip=DEFAULT_SKIP_ARG):
        '''
        returns a token() object whenever it finds a match
        when it consumes the entire input it returns a token where token.is_eof() returns true
        when it doesn't recognize the input it returns a token where token.is_invalid() returns true
        in the last two cases if (token) will be false
        
        skip can be a list of token types to be skipped, use None to not skip anything, 
        by default whitespaces are skipped
        '''
        if skip == token_source.DEFAULT_SKIP_ARG:
            skip = token_source.DEFAULT_SKIP
        elif skip is None:
            skip = ()
        self.skip(skip)
        return self._next_token()
    def _next_token(self):
        '''
        internal
        '''
        for tid, regex, grp, skip_to in token_defs:
            reobj = re.compile(regex)
            m = reobj.match(self.content, self.idx)
            if m:
                if (m.span(skip_to)[1] - self.idx) <= 0:
                    raise RuntimeError(("infinite loop detected in next_token()," +\
                                         "regex: '{}' idx: '{}'").format(regex, self.idx))
                self.undo_stack.append((self.idx, self.lineno,))
                if len(self.undo_stack) > token_source.DEFAULT_STACK_MAX:
                    self.undo_stack = self.undo_stack[-token_source.DEFAULT_STACK_MAX:]
                if tid == T_NEWL:
                    self.lineno += 1
                self.idx = m.span(skip_to)[1]
                val = m.group(grp)
                t = token(tid, val, coord(filename, self.lineno, self.colno))
                if debug:
                    print(t)
                    self.lt = t
                return t

        self.undo_stack.append((self.idx, self.lineno,))
        if self.idx >= len(self.content):
            return token(T_END, None, coord(filename, self.lineno, self.colno))
        return token(None, None, coord(filename, self.lineno, self.colno))

    def skip(self, what = DEFAULT_SKIP_ARG):
        if what == token_source.DEFAULT_SKIP_ARG:
            what = token_source.DEFAULT_SKIP
        tok = self._next_token()
        while tok.tid in what:
            tok = self._next_token()
        self.unget()

    def unget(self):
        if debug:
            try:
                print('ungetting', self.lt)
            except: 
                pass

        if not self.undo_stack:
            raise RuntimeError("unget(): attempted to undo more than the default allowed size, idx: {}".format(self.idx))
        old_idx,old_lineno  = self.undo_stack.pop()
        self.idx = old_idx
        self.lineno = old_lineno


    @property
    def colno(self):
        prv = self.content.rfind('\n',0, self.idx)
        if prv == -1:
            return 0
        return self.idx - prv


def rep_I(I):
    '''
        returns integer tids as strings
    '''
    for k,v in globals().items():
        if v == I:
            return k
    return str(I)



'''
    syntax
    progarm : statement_list
    statement_list  =   instr_def statement_list 
                    |   label_def statement_list
                    |   data_def  statement_list

    label_def : identifier colon

    immed   : label | hex | dec | bin 
    operand : immed | register | [ register ] 

    op_list : operand op_list
            : nil

    instr   : mnem op_list
'''


class label:
    def __init__(self, label_name):
        self.label_name = label_name
        self.value = None

    def resolve(self, symtable):
        '''
        returns the entry in symbtab or None
        '''
        if self.label_name in symtable:
            return symtable[self.label_name]
        return None
    def __repr__(self):
        return '(label "%s")' % (self.label_name)


class operand:
    def __init__(self, operand_type, value):
        '''
        operand_type is one of the tids prefixed by OT_
        value can be either an integer or a label, they all resolve to integers eventually
        '''
        self.otype = operand_type
        self.value = value
    def __repr__(self):
        if self.otype == OT_IMMEDIATE_INTEGER:
            return ('(type: %s, val: %s)' % (rep_I(self.otype), hex(self.value)))
        elif self.otype in ( OT_IMMEDIATE_DEFERRED , OT_REGISTER ):
            return ('(type: %s, val: %s)' % (rep_I(self.otype), self.value))
        elif self.otype in ( OT_IMMEDIATE_DEFERRED , OT_REGISTER ):
            return ('(type: %s, val: %s)' % (rep_I(self.otype), '[' + self.value + ']'))
        else:
            return ('(type: %s, val: %s)' % (rep_I(self.otype), self.value))

class instruction:
    def __init__(self, mnem_tkn, operands):
        self.mnem_tkn = mnem_tkn
        self.operands = operands
    def encode(self):
        return encode_instruction(self.mnem_tkn.val, self.operands, self.mnem_tkn.xyz)
    def length(self):
        return INSTRUCTION_LENGTH
    def __repr__(self):
        return '(instr %s %s)' % (self.mnem_tkn.val, ', '.join(map(repr,self.operands)))



class data_section:
    def __init__(self, elements):
        #elements is a list of the form [ (elem1_type, elem1_data), (elem2_type, elem2_type) ... ]
        self.elements = elements
    def encode(self):
        return encode_data(self.elements)
    def length(self):
        return INSTRUCTION_LENGTH


def encode_instruction(mnem, oprs, xyz):
    ins = bytearray(4)
    def invalid_error():
        msg = ( 'invalid instruction format: %s at %s' + \
                ' operands: %s') % (mnem, xyz, ', '.join(map(repr,oprs)))
        error(msg)
        
    if len(oprs) == 0:
        if (mnem in no_operands): 
            op_code = quick_defs[mnem]['opcode_int'] + 0x0
            ins[0] = op_code.to_bytes(1, 'little')[0]
        else:
            invalid_error()
    elif len(oprs) == 1:
        if (mnem in reg) and (oprs[0].otype == OT_REGISTER):
            r1 = RENC[oprs[0].value]
            addrmode = ADDR_MODE['REG_M']
            op_code = quick_defs[mnem]['opcode_int'] + addrmode
            ins[0] = (op_code).to_bytes(1, 'little')[0]
            ins[1] = (0).to_bytes(1, 'little')[0]
            ins[2] = r1.to_bytes(1, 'little')[0]
        elif (mnem in imm) and (oprs[0].otype == OT_IMMEDIATE_INTEGER):
            val = oprs[0].value
            addrmode = ADDR_MODE['IMMED_M']
            op_code = quick_defs[mnem]['opcode_int'] + addrmode
            ins[0] = (op_code).to_bytes(1, 'little')[0]
            ins[1] = (0).to_bytes(1, 'little')[0]
            ins[2] = val.to_bytes(2, 'little')[0]
            ins[3] = val.to_bytes(2, 'little')[1]

        elif (mnem in jmps) and (oprs[0].otype == OT_IMMEDIATE_INTEGER):
            #TODO relative jmps
            cond = JMPS[mnem]
            target = oprs[0].value
            addrmode = ADDR_MODE['IMMED_M']
            op_code = quick_defs[mnem]['opcode_int'] + addrmode
            ins[0] = (op_code).to_bytes(1, 'little')[0]
            ins[1] = (cond).to_bytes(1, 'little')[0]
            ins[2] = (target).to_bytes(2, 'little')[0]
            ins[3] = (target).to_bytes(2, 'little')[1]
        elif (mnem in complicated):
            error('not impl (%s)' % mnem)
        else:
            invalid_error()
    elif len(oprs) == 2:
        if (mnem in reg_reg) and (oprs[0].otype == OT_REGISTER) and (oprs[1].otype == OT_REGISTER):
            r1 = RENC[oprs[0].value]
            r2 = RENC[oprs[1].value]
            addrmode = ADDR_MODE['REG_M']
            #opcode as integer
            op_code = quick_defs[mnem]['opcode_int'] + addrmode
            ins[0] = (op_code).to_bytes(1, 'little')[0]
            ins[1] = (r1).to_bytes(1, 'little')[0]
            ins[2] = (r2).to_bytes(1, 'little')[0]
        elif (mnem in reg_imm) and (oprs[0].otype == OT_REGISTER) and (oprs[1].otype == OT_IMMEDIATE_INTEGER):
            r1 = RENC[oprs[0].value]
            val = oprs[1].value
            addrmode = ADDR_MODE['IMMED_M']
            op_code = quick_defs[mnem]['opcode_int'] + addrmode
            ins[0] = (op_code).to_bytes(1, 'little')[0]
            ins[1] = r1.to_bytes(1, 'little')[0]
            ins[2] = val.to_bytes(2, 'little')[0]
            ins[3] = val.to_bytes(2, 'little')[1]
        elif ((mnem in regmem_reg) and (oprs[0].otype == OT_MEM_REGISTER) and (oprs[1].otype == OT_REGISTER)) or\
            ((mnem in reg_regmem) and (oprs[0].otype == OT_REGISTER) and (oprs[1].otype == OT_MEM_REGISTER)):
            r1 = RENC[oprs[0].value]
            r2 = RENC[oprs[1].value]
            addrmode = ADDR_MODE['MEMI_M']
            op_code = quick_defs[mnem]['opcode_int'] + addrmode
            ins[0] = (op_code).to_bytes(1, 'little')[0]
            ins[1] = (r1).to_bytes(1, 'little')[0]
            ins[2] = (r2).to_bytes(1, 'little')[0]
        else:
            invalid_error()

    else:
        invalid_error()

    return ins

def assemble_program(instructions):
    bytecode = bytes()
    for instr in instructions:
        encoded = instr.encode()
        bytecode += encoded
    return bytecode

class prog:
    def __init__(self):
        self.symtable = {}
        self.instrs = []
        self.curr_address = 0

    def add_instr(self, instr):
        '''
        also used to add data sections
        they all have the same interface with .length() and .encode()
        '''
        if instr is None:
            raise ValueError('instr can\'t be None')
        if debug:
            print(instr)
        self.instrs += [ instr ]
        self.curr_address += instr.length()
    def assemble(self):
        self.resolve_all()
        return assemble_program(self.instrs)

    def resolve_all(self):
        for instr in self.instrs:
            if not isinstance(instr, instruction):
                continue
            for op in instr.operands:
                if op.otype == OT_IMMEDIATE_DEFERRED:
                    #op.value in this case is a label object
                    v = op.value.resolve(self.symtable)
                    if v is None:
                        error("couldn't resolve the symbol '{}'".format(op.value.label_name))
                    op.otype = OT_IMMEDIATE_INTEGER
                    op.value = v


    @property
    def instr_count(self):
        return len(self.instrs)


def def_label(symbol_table, label_name, address):
    symbol_table[label_name] = address

def ensure(tsrc, tid):
    '''
    tries to get a token that matches the tid and returns it
    if it finds one that doesn't match it it doesn't take it, and instead returns None
    '''
    tkn = tsrc.next_token()
    if tkn.tid != tid:
        tsrc.unget()
        return None
    return tkn

INTEGER_LITERALS = (\
                        T_IMMED_HEX, T_IMMED_OCT, T_IMMED_BIN,\
                        T_IMMED_DEC, \
                   )
def parse_integer_literal(tkn):
    if tkn.tid == T_IMMED_HEX:
        return int(tkn.val, 16)
    elif tkn.tid == T_IMMED_OCT:
        return int(tkn.val, 8)
    elif tkn.tid == T_IMMED_BIN:
        return int(tkn.val, 2)
    elif tkn.tid == T_IMMED_DEC:
        return int(tkn.val)
    else:
        raise ValueError('incompaitable token type: %s' % rep_I(tkn.tid))


def recognize_operand(tsrc):
    #skip only white space, prevent it from taking new lines!
    op_tkn = tsrc.next_token(skip = (T_WHITESPACE,) )
    if op_tkn.tid == T_COMMA:
        #not our responsibility, recognize_instr should get rid of commas
        error('unexpected comma at %s' % (op_tkn.xyz))
    elif op_tkn.tid in INTEGER_LITERALS:
        val = parse_integer_literal(op_tkn)
        return operand(OT_IMMEDIATE_INTEGER, val)
    elif op_tkn.tid == T_IDENTIFER:
        return operand(OT_IMMEDIATE_DEFERRED, label(op_tkn.val))
    elif op_tkn.tid == T_REGISTER:
        return operand(OT_REGISTER, op_tkn.val)
    elif op_tkn.tid == T_BRACK_OPEN:
        register = ensure(tsrc, T_REGISTER)
        if register is None:
            error("expected a register after '[' in %s" % (op_tkn.xyz))
        if ensure(tsrc, T_BRACK_CLOS) is None:
            error("expected ']' after %s" % (filename, register.xyz))
        return operand(OT_MEM_REGISTER, register.val)
    tsrc.unget()
    return None

def recognize_comma_operand(tsrc):
    if ensure(tsrc, T_COMMA) is not None:
        op = recognize_operand(tsrc)
        if op is None:
            error("expected an operand after ',' in %s:%d" % (filename, tsrc.lineno))
        return op
    return None

def encode_data(elements):
    data = bytes()
    for int_byte in elements:
        data += int_byte.to_bytes(1, 'little')
    return data
def prepare_str(data_tkn):
    chars = []
    try:
        bites = data_tkn.val.encode('ascii')
    except:
        error('couldn\'t encode the string literal "%s" as ascii, %s' % (data_tkn.val, data_tkn.xyz))
    for b in data_tkn.val:
        chars.append(ord(b))
    return chars

def recognize_data_elements(tsrc):
    elements = []

    while True:
        data_tkn = tsrc.next_token()
        if data_tkn.tid in INTEGER_LITERALS:
            val = parse_integer_literal(data_tkn)
            elements.append(val)
            parsed = True
        elif data_tkn.tid == T_DATA_STR:
            chars = prepare_str(data_tkn) 
            elements.extend(chars)
            parsed = True
        else:
            tsrc.unget()
            break
        if ensure(tsrc, T_COMMA) is None:
            break
    return elements

def recognize_data(tsrc):
    db_tkn = ensure(tsrc, T_DATA_BYTE)
    if db_tkn is not None:
        elements = recognize_data_elements(tsrc)
        if len(elements):
            d = data_section(elements)
            return d
        else:
            error('expected data after %s' % db_tkn.xyz)
    return None

def recognize_instr(tsrc):
    m_tkn = ensure(tsrc, T_MNEM)
    if m_tkn is None:
        return None
    operands = []
    op = recognize_operand(tsrc)
    while op is not None:
        operands.append(op)
        op = recognize_comma_operand(tsrc)
    return instruction(m_tkn, operands)



    
def recognize(input_tsrc):
    input_tsrc.skip()
    tkn = input_tsrc.next_token()
    program = prog()
    while tkn:
        if tkn.tid == T_END:
            return None
        elif tkn.tid == T_IDENTIFER:
            next_tkn = input_tsrc.next_token()
            if next_tkn.tid == T_COLON:
                def_label(program.symtable, tkn.val, program.curr_address)
            else:
                error('expected \':\' at %s, got %s' %(tkn.xyz,  next_tkn))
            tkn = input_tsrc.next_token()
        elif tkn.tid == T_MNEM:
            input_tsrc.unget()
            instr = recognize_instr(input_tsrc)
            program.add_instr(instr)
            tkn = input_tsrc.next_token()
        elif tkn.tid == T_DATA_BYTE:
            input_tsrc.unget()
            data = recognize_data(input_tsrc)
            program.add_instr(data)
            tkn = input_tsrc.next_token()
        else:
            error('unrecognized construct at %s, token type: %s' %(tkn.xyz, rep_I(tkn.tid)))

    if tkn.is_invalid():
        error("unrecognized token %s" %(tkn))
    if debug:
        print('instrs: {}'.format(program.instrs))
        print('symtable: {}'.format(program.symtable))

    return program




def assemble(out, input_tsrc):
    program = recognize(input_tsrc)
    if not program.instr_count:
        print('warning, empty source file', file=sys.stderr)
        return
    program_bytecode = program.assemble()
    out.write( program_bytecode )


def get_defs(content):
    '''
    this thing steals defs from the c instructions_defs.h file
    '''
    #//  opcode     func       mnemonic    immed    reg     mema    memi   na
    #    {0x10,     addi,      "add",      1,        1,      0,      0,    0,},
    beg_mark = '//INSTRBEGIN'
    end_mark = '//INSTREND'
    bidx = content.find(beg_mark)
    eidx = content.find(end_mark)
    if bidx == -1 or eidx == -1:
        error('couldnt parse instructions_defs.h')
        sys.exit(1)
    defs = content[bidx : eidx]
    regstr = r'_{_(0x[a-fA-F0-9]+)_,_(\w+)_,_"([a-zA-Z0-9]+)"_,_(\d)_,_(\d)_,_(\d)_,_(\d)_,_(\d)_,?_}'
    reobj = re.compile(fixregx(regstr))
    instr_defs = []
    for m in reobj.finditer(defs):
        instr_defs.append( {
                        'opcode':      m.group(1),
                        'opcode_int':  int(m.group(1), 16),
                        'func':        m.group(2),
                        'mnemonic':    m.group(3),
                        'immed':       int(m.group(4)),
                        'reg':         int(m.group(5)),
                        'mema':        int(m.group(6)),
                        'memi':        int(m.group(7)),
                        'na':          int(m.group(8)),
                       } )
    return instr_defs
    
def initialize():
    '''
    loads defs from a c file, used to determine the properties of instructions, sets a global variable instr_defs
    '''
    global instr_defs
    global quick_defs

    cpuc = os.path.join(os.path.dirname(__file__), 'instructions_defs.h')
    content = open(cpuc).read()
    instr_defs = get_defs(content)
    if not instr_defs:
        error('no instructions found')

    quick_defs = {x['mnemonic'] : x for x in instr_defs}
    jmp_instructions = {k : quick_defs['jcc'] for k,v in JMPS.items()}
    quick_defs = {**quick_defs, **jmp_instructions}

def parse_args():
    '''
    parses command line args, returns a dictionary of them
    '''
    parser = argparse.ArgumentParser(description='vcpu assembler')
    parser.add_argument('--in', default=stdin_linux,
                        help='input file')
    parser.add_argument('--out', default='p.out',
                        help='out file')
    parser.add_argument('-v', action='store_true')

    args = vars(parser.parse_args())
    if args['v']:
        global debug
        debug = True
    return args


def main():
    global filename 

    initialize()
    args = parse_args()

    filename = args.get('in','') #filename is used for FILE:LINENO error info

    with open(args['in'], 'r') as fi:
        with open(args['out'], 'wb') as fo:
            tsrc = token_source(fi)
            try:
                assemble(fo, tsrc)
            except:
                print('last xyz:\nline: %d, col:%d' % (tsrc.lineno, tsrc.colno), file=sys.stderr)
                raise


if __name__ == '__main__':
    main()
