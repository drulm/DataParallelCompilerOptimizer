
#define Q_STACK_SIZE  10000
#define STACK  0          	/* input is from the quadruple stack */
#define INPUT  1          	/* input is from the source input file */
			  	/* size of the loop stack */
#define L_STACK_SIZE   32 
#define MAX_DEF_TABLE 1000	/* entries is definitions table */
#define MAX_TARGET_TABLE 1000	/* target table size */
#define MAX_BASIC_BLOCKS 1000	/* max number of basic blocks */
#define B_F 16                          /* # word size bit fields */
#define WORD_SIZE 32		/* word size is 32 right? */

				/* following for iterative algorithm */
#define UNION 0			/* confluence op */
#define INTER 1
#define FORWARD 0		/* direction */
#define BACK 1
#define REACH 0			/* set of gens and kills / trans_fn */
#define AVAIL 1
#define LIVE 2 
#define COPY 3 

#define XX 0
#define YY 1
#define ZZ 2
#define XXB 3

typedef unsigned BITS[B_F];	/* type for looong bit fields */

unsigned magic[WORD_SIZE+1];	/* magic power of 2 table! */

struct quad {				/* holds quads together of b_blocks */
	unsigned int 	line;		/* line # */
	int		q_invar;
	BITS		q_ud1;
	BITS		q_ud2;
	BITS		q_du;
	BITS		q_exp;
	BITS		q_copy;
	BITS		y_next;
	BITS		z_next;
	BITS		x_buse;
	struct quad 	*q_n;		/* next quad */
	struct quad 	*q_p;};		/* previous quad */

struct cntl {				/* holds control link info */
	int	 	c_bb;		/* basic block jumping to/from? */
	struct cntl 	*c_n;		/* next link */
	struct cntl	*c_p;};		/* previous link */

struct basic_b {			/* the basic block */
	int 		b_num;		/* the block number */
	int		count;		/* the number of basic blocks */
	struct quad	*b_start;	/* the start quad */
	struct cntl	*b_pred;	/* the pred link blocks */
	struct cntl	*b_succ;	/* the succ link blocks */
	BITS		b_dom;
	BITS		b_dom2;
	BITS		b_l_gen;	/* FAKE! live var */
	BITS		b_l_kill;
	BITS		b_l_in;
	BITS		b_l_out;
	BITS		b_e_gen;	/* reaching exp */
	BITS		b_e_kill;
	BITS		b_e_in;
	BITS		b_e_out;
	BITS		b_c_gen;	/* reaching exp */
	BITS		b_c_kill;
	BITS		b_c_in;
	BITS		b_c_out;
	BITS		b_in;		/* reaching def */
	BITS		b_out;
	BITS		b_gen;
	BITS		b_kill};

struct target_table {			/* the target table */
	char		t_sym[100];	/* the target string */
	unsigned int	t_line;		/* the the line # */
	unsigned int	t_block;};	/* the block # */

struct def_table {			/* definition table */
	char		d_sym[100];	/* the def string */
	struct quad	*d_ptr;		/* was it + or - or what? */
	unsigned int	d_line;		/* the line # */
	unsigned int	d_block;};	/* the block # */

struct copy_table {			/* definition table */
	char		c_sym1[100];	/* the def string */
	char		c_sym2[100];	/* the def string */
	struct quad	*c_ptr;		/* was it + or - or what? */
	unsigned int	c_com;	/* was it + or - or what? */
	unsigned int	c_line;		/* the line # */
	unsigned int	c_block;};	/* the block # */

struct exp_table {			/* expression table */
	char		e_sym1[100];	/* the op1 string */
	char		e_sym2[100];	/* the op2 string */
	unsigned int	e_optype;	/* was it + or - or what? */
	struct quad	*e_ptr;		/* was it + or - or what? */
	unsigned int	e_line;		/* the line # */
	int 		e_dead;
	unsigned int	e_block;};	/* the block # */

struct live_table {			/* expression table */
	char		l_sym[100];	/* the op1 string */
	unsigned int	l_line;
	struct quad	*l_ptr;		/* was it + or - or what? */
	unsigned int	l_block;};

struct sym_table {			/* expression table */
	char		s_sym[100];
	struct quad	*s_ptr;		/* was it + or - or what? */
	int		s_mode1;
	int		s_mode2;
	int		s_s1;
	int		s_s2;
	int		s_s3;
	int		s_s4;};

struct var_table {			/* expression table */
	char		v_sym[100];
	int		v_mem;
	int		v_reg};		/* was it + or - or what? */

char darg[100];				/* def string (current) */
char larg[100];				/* def string (current) */
char earg1[100];			/* exp 1 string (current) */
char earg2[100];			/* exp 2 string (current) */
struct def_table dt[MAX_DEF_TABLE+1];	/* the def table */
struct exp_table et[MAX_DEF_TABLE+1];	/* the exp table */
struct live_table lt[MAX_DEF_TABLE+1];	/* the exp table */
struct sym_table st[MAX_DEF_TABLE+1];	/* the exp table */
struct copy_table ct[MAX_DEF_TABLE+1];	/* the exp table */
struct var_table vt[MAX_DEF_TABLE+1];	/* the exp table */
unsigned int d_count;			/* current def count */
unsigned int e_count;			/* current def count */
unsigned int l_count;			/* current def count */
unsigned int s_count;			/* current def count */
unsigned int c_count;			/* current def count */
unsigned int v_count;			/* current def count */
unsigned int expops;			/* number of operators */
unsigned int exp_varno;
char exp_var[100];					/* target string */
char move_var[100];					/* target string */
int m1,m2;
int cur_block;
int dominate;

char targ[100];					/* target string */
struct target_table tt[MAX_TARGET_TABLE];	/* targ table */
unsigned int t_count;				/* current target count */

int change;
int change_exp;
BITS hit_blocks;
int num_ops;					/* number of ops in inst */

unsigned int b_count;                           /* count of basic blocks */
struct basic_b *b_first;                        /* 'link' to first block */
struct basic_b *b_curr;                         /* 'link' to current block */
struct basic_b *b_list[MAX_BASIC_BLOCKS];       /* array of basic blocks */

struct quad *q_curr;                            /* currect quadruple */

int cond_br;                                    /* conditional branch # */
char goto_sym[100];                             /* the goto_symbol found */

/*--------------------------------------------------------------------------*/
int done;                   /* flag for loop termination */
unsigned int command; /* integer value for command read from input file */
unsigned int optype;                /* type of operation */
unsigned int mode,mode1,mode2; /* mode of oeration--(mode + optype = command)*/
int op_status;              /* program error op_status -- SUCCESS or FAIL*/

int code_type; /* STARAN, ASPROL, EMULATE, or CM */
int loops_bl[L_STACK_SIZE], tols_bl;
int quads_bl[Q_STACK_SIZE], toqs_bl, quad_bl;
int buffer_bl[100];
int source_bl, save_bl;
int command_bl;
int em_debug_1,em_debug_2,em_debug_3,em_debug_4,em_debug_5,em_debug;

/* to allow subroutine analysis */
#include <stdio.h>
#define SUB_DEPTH 20
FILE *sub_stack[SUB_DEPTH];
int in_sub;
char sub_file[20];
char sub_file_stack[SUB_DEPTH][20];
char code_bl[16];

/* from p2_common.h */
#define T_EOF		0

/* mode declarations */
/* mode is encoded  mode1,mode2 each has 16 values (4 bits) */
/* mode2 must be right most */
#define SIZE_MODE     256
#define SIZE_MODE2     16

#define T_NONE		0	/* no mode for quadruple */

/*  mode 1 */
#define T_INT		1       /* T_INT is decimal */
#define T_REAL		2
#define T_LOGICAL	3       /* T_INDEX converted to T_LOGICAL in pass1 */
#define T_LONGINT	4
#define T_OCT           5       /* T_OCT is octal integer */
#define T_CHAR          6        
#define T_CARD          7       /* cardinal - unsigned integers */
#define T_BIN           8
#define T_HEX           9
#define T_IFNANY       10       /* mode of T_ELSENANY if in T_IF */
#define T_CSTR         11       /* char string */

/* mode 2 */
#define T_SCALAR	1  
#define T_PARALLEL	2
#define T_COMMON_LEFT	3	/* not really token types, but are needed */
#define T_COMMON_RIGHT	4	/* to generate the op code portion of an  */
#define T_CNST          5       /* CONSTANT by definition is scalar */
#define T_MASKEDMOVE    6       /* pa[xx,...] = ...                  */
#define T_REG_SCALAR    7       /* array scalar                  */
#define T_REG_LEFT      8
#define T_REG_RIGHT     9
#define T_SCALAR_LEFT  10
#define T_SCALAR_RIGHT 11


/*---type declaration keywords---*/
/*---arithmetic operators---*/
#define T_ADD  		0x100
#define T_SUB  		0x200
#define T_MULT		0x300
#define T_DIV  		0x400

/*---relational operators---*/
#define T_LT   		0x500
#define T_LE		0x600
#define T_EQ  		0x700
#define T_NE		0x800
#define T_GT   		0x900
#define T_GE		0xa00

/*---logical and bitwise operators---*/
#define T_OR		0xb00
#define T_AND		0xc00
#define T_NOT		0xd00
#define T_XOR		0xe00

/*---unary plus and minus operators---*/
#define T_MINUS	 	0xf00
#define T_PLUS 		0x1000

/*---arithmetic functions---*/
#define T_COUNT         0x7f00
#define T_SUM           0x9d00

/*---assignment operator---*/
#define T_ASSIGN	0x1100

/* TOKEN TYPE STATUS VALUES - must be unique types  */
#define T_TRUE          0x1200
#define T_FALSE         0x1300

/* DEFINE OPERATORS */
#define T_DEFLOG        0x1500
#define T_DEFVAR        0x1600
#define T_DEFINE	0x1700
#define T_SHAPE         0xa100 

/* procedure type */
#define T_MAIN		0x1800
#define T_SUBROUTINE	0x1900

/* include and assembly types */
#define T_INCLUDE	0x1a00
#define T_ASSEMBLE	0x1b00

/* Beginning of statement */
#define T_BOS           0x1c00

/* "Find" quadruples */
#define T_RESFST        0x1d00

/* Error quadruples */
#define T_MSG           0x1e00

/*---token types for control statement keywords---*/
#define T_BEGIF         0x2000
#define T_ANY		0x2100
#define T_DURING	0x2200
#define T_END		0x2300
#define T_ENDANY	0x2400
#define T_ENDFOR	0x2500
#define T_ENDGET	0x2600
#define T_ENDIF		0x2700
#define T_ENDIFNANY	0x2800
#define T_ENDLOOP	0x2900
#define T_ENDNEXT	0x2a00
#define T_ENDWHILE	0x2b00
#define T_ELSE		0x2c00
#define T_ELSENANY	0x2d00
#define T_FOR		0x2e00
#define T_GET		0x2f00
#define T_IF		0x3000
#define T_NANY	        0x3100
#define T_LOOP		0x3200
#define T_NEXT		0x3300
#define T_THEN		0x3400
#define T_THENANY	0x3500
#define T_UNTIL		0x3600
#define T_WHILE		0x3700
#define T_BEGWHILE      0x3900
#define T_ALLOCATE	0x3a00
#define T_ASSOCIATE	0x3b00
#define T_FIRST		0x3d00
#define T_IN		0x3e00
#define T_RELEASE       0x3f00 
#define T_ENDALLOCATE   0x4000
#define T_WITH		0x4300
#define T_CALL          0x4500
#define T_RETURN        0x4600

/* Subscript intermediate code */
#define T_MVSC          0x4700
#define T_MVPA          0x4800
#define T_MVPAID        0x4900
#define T_MVID          0x4a00
#define T_MVCB          0x4b00 /* added 880221 jlp */

/* added for ANDCONS   */
#define T_ANDFOR        0x4d00
#define T_ENDANDFOR     0x4e00
#define T_ANDIF         0x4f00
#define T_ATHEN         0x5000
#define T_ENDANDIF      0x5100
#define T_ANDCONS       0x5200
#define T_ENDANDCONS    0x5300

/* I/O KEYWORDS  */
   /* Print statement quadruples */
#define T_FSTOUT        0x6a00     /* NOTE: value out of sequence */
#define T_OUTPT         0x5400
#define T_COLAPSE       0x5500
#define T_BEGPRINT      0x5600
#define T_PRINT         0x5700
#define T_NL            0x5800
#define T_ENDPRINT      0x5900
#define T_PRINT_DEL     0x8a00    /*out of sequence value */
/* Read statement quadruples */
#define T_BEGREAD       0x5a00
#define T_READNL        0x5b00
#define T_READ          0x5c00
#define T_IOALLOC       0x5d00
#define T_EXPAND        0x5e00
#define T_INPUT         0x5f00
#define T_ENDREAD       0x6000
#define T_REREAD        0x9e00 
#define T_COREAD        0x9f00
#define T_CONTIG        0xa000 

#define T_DECLARE	0x6100
#define T_FIX		0x6200
#define T_FLOAT		0x6300
#define T_DEF_CONST	0x6400
#define	T_IDENT		0x6500
#define T_CLRARRAY	0x6600
#define T_INITSTACK	0x6700
#define T_LABEL		0x6800
#define	T_SETSCOPE	0x6900
/* #define T_FSTOUT:    0X6a00   DEFINED ABOVE  */
#define T_TEMP          0X6b00   /* added 880707 jlp */

#define T_STOP		0x6c00
#define T_SHORT		0x6d00
#define T_LONG		0x6e00
#define T_ENTRY		0x6f00
#define T_EXTERN	0x7000
#define T_STACK_SCOPE	0x7100
#define T_POP_SCOPE	0x7200
#define T_ENDSETSCOPE   0x7300
#define T_ENDMAIN	0x7400
#define T_ENDSUB	0x7500

#define T_MINDEX	0x7600
#define T_MINVAL	0x7700
#define T_MAXDEX	0x7800
#define T_MAXVAL	0x7900
#define T_ADDRESS	0x7a00 /* indirect address token type */
#define T_OPEN          0x7b00   /* IO quads */
#define T_CLOSE         0x7c00
#define T_SCIN          0x7d00
#define T_SCOT          0x7e00
/*#define T_COUNT         0x7f00*/

#define T_BEGSTACK      0x8000 /* STACK FOR AND WHILE KEYWORDS */
#define T_STACK         0x8100 
#define T_RECURSE       0x8200 
#define T_ENDRECURSE    0x8300 
#define T_POP           0x8400 
#define T_STACKFOR      0x8500 
#define T_ENDSTACKFOR   0x8600 
#define T_DUMP          0x8700
#define T_STACKWHILE    0x8800 
#define T_ENDSTACKWHILE 0x8900 
/* fine T_PRINT_DEL     0x8a00      see print quadruples */
#define T_SCINL         0x8b00
#define T_SCINP         0x8c00
#define T_SCOTL         0x8d00
#define T_SCOTP         0x8e00


#define T_FSTCD         0x9000    /* STRUCTURE CODE FUNCTION KEYWORDS */
#define T_NXTCD         0x9100
#define T_PREVCD        0x9200
#define T_TRNCD         0x9300
#define T_TRNACD        0x9400
#define T_SCIN8         0x9500
#define T_SCOT8         0x9600
#define T_FSTCD8        0x9700    /* STRUCTURE CODE FUNCTION KEYWORDS */
#define T_NXTCD8        0x9800
#define T_PREVCD8       0x9900
#define T_TRNCD8        0x9a00
#define T_TRNACD8       0x9b00
#define T_SIBDEX        0x9c00
/*#define T_SUM         0x9d00 */
/*#define T_REREAD        0x9e00 */
/* #define T_COREAD        0x9f00 */
/*#define T_CONTIG        0xa000 */
/* #define T_SHAPE      0xa100 */
#define T_REGSCA        0xa200
#define T_SCAREG        0xa300

#define T_TO            T_LT
#define T_FROM          T_GT


/* DEFINITIONS FOR FUNCTION RETURN VALUES */

#define TRUE		1
#define FALSE		0
#define FAIL		-1
#define SUCCESS		-2


/* DEFINITIONS FOR SCANNER AND PARSER CONSTANTS */
#define TOKEN_SIZE	50		/* maximum size of a token */
#define MAX_ID_SIZE	6		/* maximum size of an identifier */
#define LINE_LENGTH	81		/* maximum size of input buffer for
					   assembly code */


/* EXTERNAL VARIABLE DECLARATIONS */

int     op_debug,		/* flag for debugging */
        token_size,		/* size of current token */
        token_type;		/* type of current token */

char	token_buf[TOKEN_SIZE];	/* buffer for single token */

FILE 	*source_fp,		/* source file descriptor */
         *input_fp,             /* input file descriptor */
	*object_fp;		/* object file descriptor */


/* FUNCTION DECLARATIONS */
FILE	*fopen();		/* system routine to open a file */

int linenum;  /* set by bos for error messages, etc */

char carg1[100],carg2[100],carg3[100],carg4[100],carg5[100],carg6[100],
     carg7[100],carg8[100],carg9[100];


