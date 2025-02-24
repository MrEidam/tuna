/* Includes */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <sys/stat.h>

/* Defines */

#define TUNA_VERSION "0.2.1"
#define TUNA_TAB_STOP 4
#define TUNA_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f) 

enum editorKey{
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
};

enum editorHighlight{
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
	HL_KEYWORD3,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH,
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/* Data */

struct editorSyntax{
    char *filetype;
    char **filematch;
    char **keywords;
    char *singleline_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int flags;
};

typedef struct erow{
    int idx;
    int size;
    int rsize;
    char *chars;
    char *render;
    unsigned char *hl;
    int hl_open_comment;
}erow;

struct editorConfig{
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    int dirty;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;
    struct termios orig_termios;
};

struct editorConfig E;

/* Filetypes */

char *C_HL_extensions[] = {".c", ".h", ".cpp", ".hpp", NULL};
char *C_HL_keywords[] = {
    "switch", "free", "if", "while", "for", "break", "continue", "return", "else", "sizeof", "default", "do", "extern",
    "typedef", "static", "enum", "case", "template", "operator", "goto", "system",

	"class$", "struct$", "union$",

    "#define|", "#include|", "#if|", "#ifdef|", "#ifndef|", "#else|", "#endif|", "#undef|", "#pragma|", "#error|", "#line|",

    "int|", "long|", "double|", "const|", "float|", "char|", "unsigned|", "signed|", "auto|", "short|",
    "void|", "time_t|", "size_t|", "ssize_t|", "NULL|", "erow", NULL
};

char *PY_HL_extensions[] = {".py", ".by", NULL};
char *PY_HL_keywords[] = {
    "False|", "None|", "True|", "and|", "as|", "assert|", "async|", "await|", "break|", 
    "class|", "continue|", "def|", "del|", "elif|", "else|", "except|", "finally|", 
    "for|", "from|", "global|", "if|", "import|", "in|", "is|", "lambda|", "nonlocal|", 
    "not|", "or|", "pass|", "raise|", "return|", "try|", "while|", "with|", "yield|",

    "abs", "all", "any", "ascii", "bin", "bool", "bytearray", "bytes", "callable", "chr", "classmethod",
    "compile", "complex", "delattr", "dict", "dir", "divmod", "enumerate", "eval", "exec", "filter",
    "float", "format", "frozenset", "getattr", "globals", "hasattr", "hash", "help", "hex", "id", "input",
    "int", "isinstance", "issubclass", "iter", "len", "list", "locals", "map", "max", "memoryview", "min",
    "next", "object", "oct", "open", "ord", "pow", "print", "property", "range", "repr", "reversed", "round",
    "set", "setattr", "slice", "sorted", "staticmethod", "str", "sum", "super", "tuple", "type", "vars", "zip", NULL
};

char *ASM_HL_extensions[] = {".asm",NULL};
char *ASM_HL_keywords[] = {
    // Registers
    
    "al$", "ah$", "bl$", "bh$", "cl$", "ch$", "dl$", "dh$",
    "sil$", "dil$", "bpl$", "spl$", "r8b$", "r9b$", "r10b$", "r11b$", "r12b$", "r13b$", "r14b$", "r15b$",
    "ax$", "bx$", "cx$", "dx$", "si$", "di$", "bp$", "sp$", "r8w$", "r9w$", "r10w$", "r11w$", "r12w$", "r13w$", "r14w$", "r15w$",
    "eax$", "ebx$", "ecx$", "edx$", "esi$", "edi$", "ebp$", "esp$", "r8d$", "r9d$", "r10d$", "r11d$", "r12d$", "r13d$", "r14d$", "r15d$",
    "rax$", "rbx$", "rcx$", "rdx$", "rsi$", "rdi$", "rbp$", "rsp$", "r8$", "r9$", "r10$", "r11$", "r12$", "r13$", "r14$", "r15$",
    "mm0$", "mm1$", "mm2$", "mm3$", "mm4$", "mm5$", "mm6$", "mm7$",
    "xmm0$", "xmm1$", "xmm2$", "xmm3$", "xmm4$", "xmm5$", "xmm6$", "xmm7$", "xmm8$", "xmm9$", "xmm10$", "xmm11$", "xmm12$", "xmm13$", "xmm14$", "xmm15$",
    "ymm0$", "ymm1$", "ymm2$", "ymm3$", "ymm4$", "ymm5$", "ymm6$", "ymm7$", "ymm8$", "ymm9$", "ymm10$", "ymm11$", "ymm12$", "ymm13$", "ymm14$", "ymm15$",
    "zmm0$", "zmm1$", "zmm2$", "zmm3$", "zmm4$", "zmm5$", "zmm6$", "zmm7$", "zmm8$", "zmm9$", "zmm10$", "zmm11$", "zmm12$", "zmm13$", "zmm14$", "zmm15$",
    "k0$", "k1$", "k2$", "k3$", "k4$", "k5$", "k6$", "k7$",
    "st0$", "st1$", "st2$", "st3$", "st4$", "st5$", "st6$", "st7$",
    "cr0$", "cr2$", "cr3$", "cr4$", "cr8$",
    "dr0$", "dr1$", "dr2$", "dr3$", "dr6$", "dr7$",
    "cs$", "ds$", "es$", "fs$", "gs$", "ss$",
    "rip$", "eflags$",

    // Instructions

    "mov|", "add|", "sub|", "mul|", "imul|", "div|", "idiv|", "inc|", "dec|",
    "and|", "or|", "xor|", "not|", "neg|", "shl|", "shr|", "sal|", "sar|", "rol|", "ror|",
    "cmp|", "test|", "jmp|", "je|", "jne|", "jg|", "jge|", "jl|", "jle|", "ja|", "jae|", "jb|", "jbe|",
    "call|", "ret|", "push|", "pop|", "lea|",
    "nop|", "hlt|", "int|", "syscall|", "sysret|",
    "cmovz|", "cmovnz|", "cmovg|", "cmovge|", "cmovl|", "cmovle|",
    "sete|", "setne|", "setg|", "setge|", "setl|", "setle|",
    "movzx|", "movsx|", "movaps|", "movups|", "movdqu|", "movdqa|",
    "addps|", "subps|", "mulps|", "divps|", "sqrtps|",
    "addpd|", "subpd|", "mulpd|", "divpd|", "sqrtpd|",
    "pand|", "por|", "pxor|", "pandn|", "pslld|", "psrld|", "psrad|",
    "prefetchnta|", "prefetcht0|", "prefetcht1|", "prefetcht2|",
    "cvtsi2sd|", "cvtsi2ss|", "cvtss2sd|", "cvtsd2ss|",
    "cvtdq2ps|", "cvtps2dq|", "cvttps2dq|",
    "movq|", "movhpd|", "movlpd|",
    "rdtsc|", "rdmsr|", "wrmsr|",
    "lfence|", "sfence|", "mfence|",
    "clflush|", "clflushopt|", "xchg|", "xadd|", "lock|",
    "bt|", "bts|", "btr|", "btc|",
    "test|", "popcnt|", "bsf|", "bsr|",
    "paddb|", "paddw|", "paddd|", "paddq|", "pmuludq|", "psubb|", "psubw|", "psubd|", "psubq|",
    "pause|", "crc32|", "rdrand|", "rdseed|",

    // Keywords

    "entry", "global", "extern", "section", "segment", "org", "align", "db", "dw", "dd", "dq", "rb", "rw", "rd", "rq",
    "resb", "resw", "resd", "resq",
    "equ", "times", "struc", "endstruc", "macro", "endm", "repeat", "endrepeat", "while", "endw",
    "if", "else", "elif", "endif", "define", "undef", "include", "incbin",
    "import", "export", "stdcall", "fastcall", "cdecl",
    "end", "use16", "use32", "use64",
    "label", "default", "dup",
    "flat", "public", "virtual", "common",
    "int3", "sysenter", "sysexit", "iretq", "iret",
    "bits", "org", "cpu", "model",
    "proc", "endp", "data", "code",
    "macro", "local", "ends", "group",
    "format", "call", "jmp", "ret", "start", NULL
};

struct editorSyntax HLDB[] = {
    {
        "C",
        C_HL_extensions,
	C_HL_keywords,
	"//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },{
        "Python",
        PY_HL_extensions,
        PY_HL_keywords,
        "#", "'''", "'''",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },{
        "Assembly",
        ASM_HL_extensions,
        ASM_HL_keywords,
        ";", ";", ";",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/* Prototypes */

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorSavePrompt();
char *editorPromptInput();

/* Terminal */

void die(const char *s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
        die("tcsetattr");
    }
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '['){
            if(seq[1] >= '0' && seq[1] <= '9'){
                if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if(seq[2] == '~'){
                    switch(seq[1]){
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }else{
                switch(seq[1]){
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }else if(seq[0] == 'O'){
            switch(seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    }else{
        return c;
    }
}

int getCursorPosition(int *rows, int *cols){
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if(buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    int max_lines = E.numrows;
    int max_digits = max_lines > 0 ? (int)log10(max_lines) + 1 : 1;
    *cols -= (max_digits + 1);

    return 0;
}

int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        editorReadKey();
        return getCursorPosition(rows, cols);
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/* Syntax Highlighing */

int is_separator(int c){
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];{}", c) != NULL;
}

void editorUpdateSyntax(erow *row){
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if(E.syntax == NULL) return;

    char **keywords = E.syntax->keywords;

    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;

    int prev_sep = 1;
    int in_string = 0;
    int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

    int i = 0;
    while(i < row->rsize){
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

		if(scs_len && !in_string && !in_comment){
		    if(!strncmp(&row->render[i], scs, scs_len)){
				memset(&row->hl[i], HL_COMMENT, row->rsize - i);
				break;
		    }
		}

		if(mcs_len && mce_len && !in_string){
		    if(in_comment){
				row->hl[i] = HL_MLCOMMENT;
				if(!strncmp(&row->render[i], mce, mce_len)){
				    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
				    i += mce_len;
				    in_comment = 0;
				    prev_sep = 1;
				    continue;
				}else{
				    i++;
				    continue;
				}
		    }else if(!strncmp(&row->render[i], mcs, mcs_len)){
				memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
				i += mcs_len;
				in_comment = 1;
				continue;
		    }
		}

		if(E.syntax->flags & HL_HIGHLIGHT_STRINGS){
		    if(in_string){
		        row->hl[i] = HL_STRING;
			if(c == '\\' && i + 1 < row->rsize){
			    row->hl[i + 1] = HL_STRING;
			    i += 2;
			    continue;
			}
			if(c == in_string) in_string = 0;
			i++;
			prev_sep = 1;
			continue;
	    }else{
			if(c == '"' || c == '\''){
			    in_string = c;
			    row->hl[i] = HL_STRING;
			    i++;
			    continue;
			}
	    }
	}

	if(E.syntax->flags & HL_HIGHLIGHT_NUMBERS){
 		if((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || (c == '.' && prev_hl == HL_NUMBER)){
			row->hl[i] = HL_NUMBER;
            i++;
            prev_sep = 0;
            continue;
        }
	}

	if(prev_sep){
	    int j;
	    for(j = 0; keywords[j]; j++){
			int klen = strlen(keywords[j]);
			int kw2 = keywords[j][klen - 1] == '|';
			if(kw2) klen--;
			int kw3 = keywords[j][klen - 1] == '$';
			if(kw3) klen--;

			if(!strncmp(&row->render[i], keywords[j], klen) && is_separator(row->render[i + klen])){
			    memset(&row->hl[i], kw3 ? HL_KEYWORD3 : (kw2 ? HL_KEYWORD2 : HL_KEYWORD1) , klen);
			    i += klen;
		 		break;
			}
	    }
	    
		if(keywords[j] != NULL){
			prev_sep = 0;
			continue;
	    }
	}

        prev_sep = is_separator(c);
        i++;
    }
    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if(changed && row->idx + 1 < E.numrows)
	editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl){
    switch(hl){
		case HL_COMMENT:
		case HL_MLCOMMENT: return 91;	// 36 - Cyan
		case HL_KEYWORD1: return 93;	// 33 - Yellow
		case HL_KEYWORD2: return 94;	// 32 - Green
		case HL_KEYWORD3: return 32;
        case HL_STRING: return 31;		// 35 - Magenta
        case HL_NUMBER: return 96;		// 31 - Red
        case HL_MATCH: return 34;		// 34 - Blue
        default: return 37;				// 37 - White
    }
}

/*/
	COLORS LIST
	- 30 - Black
	- 31 - Red
	- 32 - Green
	- 33 - Yellow ( shitty )
	- 34 - Blue
	- 35 - Magenta
	- 36 - Cyan
	- 37 - White

	BOLD COLOR LIST
	- 90 - 97 - Same color (better sometimes) but bold
/*/


void editorSelectSyntaxHighlight(){
    E.syntax = NULL;
    if(E.filename == NULL) return;

    char *ext = strrchr(E.filename, '.');

    for(unsigned int j = 0; j < HLDB_ENTRIES; j++){
        struct editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while(s->filematch[i]){
            int is_ext = (s->filematch[i][0] == '.');
            if((is_ext && ext && !strcmp(ext, s->filematch[i])) || (!is_ext && strstr(E.filename, s->filematch[i]))){
                E.syntax = s;

                int filerow;
                for(filerow = 0; filerow < E.numrows; filerow++){
                    editorUpdateSyntax(&E.row[filerow]);
                }

                return;
            }
        i++;
        }
    }
}

/* Row Operations */

int editorRowCxToRx(erow *row, int cx){
    int rx = 0;
    int j;
    for(j = 0; j < cx; j++){
        if(row->chars[j] == '\t')
            rx += (TUNA_TAB_STOP - 1) - (rx % TUNA_TAB_STOP);
        rx++;
    }
    return rx;
}
int editorRowRxToCx(erow *row, int rx){
    int cur_rx = 0;
    int cx;
    for(cx = 0; cx < row->size; cx++){
        if(row->chars[cx] == '\t')
            cur_rx += (TUNA_TAB_STOP - 1) - (cur_rx % TUNA_TAB_STOP);
        cur_rx++;

        if(cur_rx > rx) return cx;
    }
    return cx;
}

void editorUpdateRow(erow *row){
    int tabs = 0;
    int j;
    for(j = 0; j < row->size; j++)
        if(row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs*(TUNA_TAB_STOP - 1) + 1);

    int idx = 0;
    for(j = 0; j<row->size; j++){
        if(row->chars[j] == '\t'){
            row->render[idx++] = ' ';
            while(idx % TUNA_TAB_STOP != 0) row->render[idx++] = ' ';
        }else{
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;

    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len){
    if(at < 0 || at > E.numrows) return;

    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    for(int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

    E.row[at].idx = at;

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].hl = NULL;
    E.row[at].hl_open_comment = 0;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
    E.dirty++;
}

void editorFreeRow(erow *row){
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void editorDelRow(int at){
    if(at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    for(int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
    E.numrows--;
    E.dirty++;
}

void editorRowInsertChat(erow *row, int at, int c){
    if(at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len){
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow *row, int at){
    if(at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

/* Editor Operations */

void editorInsertChar(int c){
    if(E.cy == E.numrows){
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChat(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewLine(){
    if(E.cx == 0){
        editorInsertRow(E.cy, "", 0);
    }else{
        erow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar(){
    if(E.cy == E.numrows) return;
    if(E.cx == 0 && E.cy == 0) return;

    erow *row = &E.row[E.cy];
    if(E.cx > 0){
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    }else{
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

/* Delete */

void editorClear(){
	while(E.numrows > 0){
		editorDelRow(E.numrows - 1);
	}
}

/* File i/o */

char *editorRowsToString(int *buflen){
    int totlen = 0;
    int j;
    for(j = 0; j <E.numrows; j++){
        totlen += E.row[j].size + 1;
    }
    *buflen = totlen;

    char *buf = malloc(totlen);
    char *p = buf;
    for(j = 0; j < E.numrows; j++){
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

char *expandTilde(const char *path){
	if(path[0] == '~'){
		const char *home = getenv("HOME");
		if(!home){
			struct passwd *pw = getpwuid(getuid());
			if(pw){
				home = pw->pw_dir;
			}
		}

		if(home){
			size_t len = strlen(home) + strlen(path);
			char *expanded = malloc(len);
			if(!expanded) return NULL;
			snprintf(expanded, len, "%s%s", home, path + 1);
			return expanded;
		}else{
			fprintf(stderr, "Could not determine home directory.");
			return NULL;
		}
	}
	return strdup(path);
}

void editorOpen(const char *filename){
	if(E.dirty){
		editorSavePrompt();
		if(E.dirty) return;
	}

	char *full_path;
	struct stat file_stat;

	char *expanded_path = expandTilde(filename);
	if(!expanded_path){
		editorSetStatusMessage("Failed to expand path");
		return;
	}
	
	if(E.filename && filename[0] == '~'){
		char *base_dir = strdup(E.filename);
		base_dir = dirname(base_dir); // Current file dir		

		size_t len = strlen(base_dir) + strlen(expanded_path) + 2;
		full_path = malloc(len);
		if(!full_path){
			editorSetStatusMessage("Memory allocation failed");
			free(base_dir);
			free(expanded_path);
			return;
		}

		/*snprintf*/editorSetStatusMessage(full_path, len, "%s/%s", base_dir, expanded_path);
		free(base_dir);
	}else{
		full_path = strdup(expanded_path);
	}

	free(expanded_path);

	// Does file exist?
	if(stat(full_path, &file_stat) == -1){
		FILE *fp = fopen(full_path, "w");
		if(!fp){
			free(full_path);
			editorSetStatusMessage("Failed to create file");
			return;
		}
		fclose(fp);
	}

	// Open the BOI
	FILE *fp = fopen(full_path, "r");
	if(!fp){
		free(full_path);
		editorSetStatusMessage("Failed to open file");
		return;
	}

	editorClear();

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while((linelen = getline(&line, &linecap, fp)) != -1){
        while(linelen>0 && (line[linelen - 1] == '\n' ||
							line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);

	free(E.filename);
	E.filename = full_path;

    E.dirty = 0;
	editorSelectSyntaxHighlight();
}

void editorSave(){
    if(E.filename == NULL){
        E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
        if(E.filename == NULL){
            editorSetStatusMessage("Save aborted");
            return;
        }
        editorSelectSyntaxHighlight();
    }

    int len;
    char *buf = editorRowsToString(&len);

    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if(fd != -1){
        if(ftruncate(fd, len) != -1){
            if(write(fd, buf, len) == len){
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }
    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

void editorChangeTitle(){
	char *title = editorPrompt("Enter new title: %s (ESC to cancel)", NULL);
	if(title){
		printf("\033]0;%s\007", title);
		fflush(stdout);
		free(title);
	}
}

/* Open Files */

void editorOpenFileMenu(){
	char *filename = editorPrompt("Open file: %s (ESC to cancel)", NULL);

	if(filename){
		editorOpen(filename);
		free(filename);
	}
}

char *editorPromptInput(){
	size_t bufsize = 128;
	char *buf = malloc(bufsize);
	size_t buflen = 0;

	while(1){
		editorSetStatusMessage("%s", buf);
		int c = editorReadKey();

		if(c == '\r'){
			if(buflen != 0){
				buf[buflen] = '\0';
				return buf;
			}
		}else if(!iscntrl(c) && c < 128){
			if(buflen == bufsize - 1){
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
		}
	}
}

void editorSavePrompt(){
	editorSetStatusMessage("You have unsaved changes. Save them? (y/n)");
	int c = editorReadKey();
	if(c == 'y' || c == 'Y'){
		editorSave();
	}
}

/* Find */

void editorFindCallback(char *query, int key){
    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    if(saved_hl){
        memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if(key == '\r' || key == '\x1b'){
        last_match = -1;
        direction = 1;
        return;
    }else if(key == ARROW_RIGHT || key == ARROW_DOWN){
        direction = 1;
    }else if(key == ARROW_LEFT || key == ARROW_UP){
        direction = -1;
    }else{
        last_match = -1;
        direction = 1;
    }

    if(last_match == -1) direction = 1;
    int current = last_match;
    int i;
    for(i = 0; i < E.numrows; i++){
        current += direction;
        if(current == -1) current = E.numrows -1;
        else if(current == E.numrows) current = 0;

        erow *row = &E.row[current];
        char *match = strstr(row->render, query);
        if(match){
            last_match = current;
            E.cy = current;
            E.cx = editorRowRxToCx(row, match - row->render);
            E.rowoff = E.numrows;

            saved_hl_line = current;
            saved_hl = malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void editorFind(){
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;

    char *query = editorPrompt("Search: %s (ESC/Arrows/Enter)", editorFindCallback);

    if(query){
        free(query);
    }else{
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_coloff;
    }
}

/* Terminal */

void editorTerminal(){
	char *command = editorPrompt("Tuna~$: %s", NULL);

	int comm = read(0, command, sizeof(command));
	command[comm - 1] = 0;

	system(command);
	free(command); 
}

/* Append Buffer */

struct abuf{
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len){
    char *new = realloc(ab->b, ab->len + len);

    if(new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab){
    free(ab->b);
}

/* Output */

void editorScroll(){
    E.rx = 0;
    if(E.cy < E.numrows){
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }
    
    int max_lines = E.numrows;
    int max_digits = max_lines > 0 ? (int)log10(max_lines) + 1 : 1;
    int line_number_width = max_digits + 1;

    if(E.rx + line_number_width < E.coloff){
	E.rx = E.coloff - line_number_width + 1;
    }

    if(E.cy < E.rowoff){
        E.rowoff = E.cy;
    }
    if(E.cy >= E.rowoff + E.screenrows){
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if(E.rx < E.coloff){
        E.coloff = E.rx;
    }
    if(E.rx >= E.coloff + E.screencols){
        E.coloff = E.rx - E.screencols + 1;
    }
}

void editorDrawRows(struct abuf *ab){
    int y;
    int max_lines = E.numrows;
    int max_digits = max_lines > 0 ? (int)log10(max_lines) + 0 : 0;
    max_digits += 1;
    
    for(y = 0; y < E.screenrows; y++){
        int filerow = y + E.rowoff;
        
        if(filerow >= E.numrows){
            if(E.numrows == 0 && y == E.screenrows/3){
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Tuna editor -- version %s", TUNA_VERSION);
                if(welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen)/2;
                if(padding){
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while(padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            }else{
                abAppend(ab, "~", 1);
            }
        }else{
	    int line_number = filerow + 1;
	    char line_number_str[16];
	    snprintf(line_number_str, sizeof(line_number_str), "%*d ", max_digits, line_number);
	    abAppend(ab, line_number_str, strlen(line_number_str));

            int len = E.row[filerow].rsize - E.coloff;
            if(len < 0) len = 0;
            if(len > E.screencols) len = E.screencols;
            char *c = &E.row[filerow].render[E.coloff];
            unsigned char *hl = &E.row[filerow].hl[E.coloff];
            int current_color = -1;
            int j;
            for(j = 0; j < len; j++){
		if(iscntrl(c[j])){
		    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
		    abAppend(ab, "\x1b[7m", 4);
		    abAppend(ab, &sym, 1);
		    abAppend(ab, "\x1b[m", 3);
		    if(current_color != -1){
			char buf[16];
			int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
			abAppend(ab, buf, clen);
		    }
		}else if(hl[j] == HL_NORMAL){
                    if(current_color != -1){
                        abAppend(ab, "\x1b[39m", 5);
                        current_color = -1;
                    }
                    abAppend(ab, &c[j], 1);
                }else{
                    int color = editorSyntaxToColor(hl[j]);
                    if(color != current_color){
                        current_color = color;
	                char buf[16];
 	                int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
  	                abAppend(ab, buf, clen);
                    }
                    abAppend(ab, &c[j], 1);
                }
            }
            abAppend(ab, "\x1b[39m", 5);
        }
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuf *ab){
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", E.filename ? E.filename : "[No Name]", E.numrows, E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", E.syntax ? E.syntax->filetype : "no known filetype", E.cy+1, E.numrows);
    if(len > E.screencols) len = E.screencols;
    abAppend(ab, status, len);
    while(len < E.screencols){
        if(E.screencols - len == rlen){
            abAppend(ab, rstatus, rlen);
            break;
        }else{
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab){
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if(msglen>E.screencols) msglen = E.screencols;
    if(msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen(){
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    int max_lines = E.numrows;
    int max_digits = max_lines > 0 ? (int)log10(max_lines) + 1 : 1;
    int line_number_width = max_digits + 1;

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + line_number_width + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

/* Input */

char *editorPrompt(char * prompt, void (*callback)(char *, int)){
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while(1){
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if(c  == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE){
            if(buflen != 0) buf[--buflen] = '\0';
        }else if(c == '\x1b'){
            editorSetStatusMessage("");
            if(callback) callback(buf, c);
            free(buf);
            return NULL;
        }else if(c == '\r'){
            if(buflen != 0){
                editorSetStatusMessage("");
                if(callback) callback(buf, c);
                return buf;
            }
        }else if(!iscntrl(c) && c < 128){
            if(buflen == bufsize - 1){
                bufsize  *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        if(callback) callback(buf, c);
    }
}

void editorMoveCursor(int key){
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    int line_number_width = (int)log10(E.numrows) + 2;
    int max_columns = E.screencols - line_number_width;

    switch(key){
        case ARROW_LEFT:
            if(E.cx > 0){
                E.cx--;
            }else if(E.cy > 0){
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;
            }else if(row && E.cx == row->size){
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if(E.cy > 0){
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if(E.cy < E.numrows){
                E.cy++;
            }
            break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if(E.cx > rowlen){
        E.cx = rowlen;
    }

    if(E.cx + line_number_width > max_columns){
        E.cx = max_columns - line_number_width;
    }
}

void editorMoveSelection(int key){
    switch(key){
	case ARROW_LEFT:
	    if(E.cx > 0) E.cx--;
	    break;
	case ARROW_RIGHT:
	    if(E.cx < E.row[E.cy].size) E.cx++;
	    break;
	case ARROW_UP:
	    if(E.cy > 0) E.cy--;
	    break;
	case ARROW_DOWN:
	    if(E.cy < E.numrows - 1) E.cy++;
	    break;
    }
    // sel.end_row = E.cy;
    // sel.end_col = E.cx;
}

//#define CTRL_SPACE 32

void editorProcessKeypress(){
    static int quit_times = TUNA_QUIT_TIMES;
    int c = editorReadKey();

    switch(c){       
        case '\r':
            editorInsertNewLine();
            break;

        case CTRL_KEY('q'):
            if(E.dirty && quit_times > 0){
                editorSetStatusMessage("WARNING!!! File has unsaved changes. Press ctrl-Q %d more times to quit.", quit_times);
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'): // Saving
            editorSave();
            break;

		case CTRL_KEY('o'): // Opening
			if(E.dirty){
				editorSetStatusMessage("Unsaved Changes! Save (CTRL-S) before opening another file.");
				return;
			}
			editorOpenFileMenu();
			break;

		case CTRL_KEY('t'):	// Title change
			editorChangeTitle();
			break;

		case CTRL_KEY('u'): // Terminal
			editorTerminal();
			break;

        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            if(E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;

        case CTRL_KEY('f'):
            editorFind();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if(c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if(c == PAGE_UP){
                    E.cy = E.rowoff;
                }else if(c == PAGE_DOWN){
                    E.cy = E.rowoff + E.screenrows - 1;
                    if(E.cy > E.numrows) E.cy = E.numrows;
                }

                int times = E.screenrows;
                while(times--) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
			editorMoveCursor(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            editorInsertChar(c);
            break;
    }
    quit_times = TUNA_QUIT_TIMES;
}

/* Init bruv */

void initEditor(){
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    E.syntax = NULL;

    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
    E.screenrows -= 2;
}

int main(int argc, char *argv[]){
    enableRawMode();
    initEditor();
    if(argc >= 2){
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl + (S)ave | (Q)uit | (F)ind | (O)pen | (T)itle | (U)Terminal");

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    return 0;
}
