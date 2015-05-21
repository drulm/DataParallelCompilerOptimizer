/*---------------------------------------------------------------------------*/
/*		FILE--op.c

  (1)	This section of code builds the basic blocks from the quads
	list which is read in, and as the blocks are being built,
	goto-target symbols, and define symbols are stored in symbol 
	tables for faster acces & ease of use.  There is an array
	that currently holds the basic blocks (this does not denote
	order, but is good for getting to basic block number...)

  (2)	The next partscans through all the basic blocks and links
	up the blocks by saving lists of forward and backward links
	(all doubly linked).  While doing this, the gens and kills
	are propagated for the basic block.

  (3)	Afterwards there is a dump of the basic block info which is
	only for debugging purposes...
*/
/*---------------------------------------------------------------------------*/
#include "op_common.h"

/*---------------------------------------------------------------------------*/
/*	main()
		This routine starts everything up (by calling some code
		to read the quads into a stack) and seqments the code
		into basic blocks.
*/
/*---------------------------------------------------------------------------*/
main()
{
int i;					/* Loop through blocks */
int made,last;				/* made true if block made */
					/* last for last block */

fill_magic();				/* fill powers of 2 in array */
input_fp = fdopen(0,"r");		/* change input descriptor (no DUP!) */
op_commands();				/* sacn in all relevant quads */

made = FALSE;				/* init basic flags */
last = FALSE;
b_first =  balloc();			/* create 1st basic block */
b_curr = b_first;			/* set it to first */
b_count = 0;				/* init count of blocks */

q_curr = NULL;				/* no quad yet */

done = FALSE;				/* not done */
op_status = SUCCESS;			/* not failed yet */
save_bl = TRUE;
source_bl = STACK;			/* source is NOW stack */
quad_bl = 0;				/* start quads at 0 */
t_count = 0;				/* target symbol table starts at 0 */
d_count = 1;				/* defin symbol table starts at 1 */
e_count = 1;				/* defin symbol table starts at 1 */
l_count = 1;				/* defin symbol table starts at 1 */
s_count = 1;				/* defin symbol table starts at 1 */
v_count = 1;				/* defin symbol table starts at 1 */
c_count = 1;				/* defin symbol table starts at 1 */
exp_varno = 1;

while (done == FALSE && op_status == SUCCESS)
	{
	last = FALSE;
	read_everything(); 		/* read a line */
	if (target() && !made)		/* if target, make a basic block */
		{
		b_curr = make_basic_block();
		}
	made = FALSE;
	insert_code();
	save_target();
	save_definition();		/* save targets defs and code */
	save_expression();		/* save the expressions */
	save_live();
	save_vars();
	save_copy();
	if (branch()) 			/* if branch make a block */
		{	
		b_curr = make_basic_block();
		made = TRUE; 
		last = TRUE;
		}			/* next quad */
	quad_bl++;
	if (quad_bl >= toqs_bl) done=TRUE;
	}
if (! last)
	b_curr = make_basic_block();

link_up_blocks();			/* link blocks, make gens/kills */
gen_kill(REACH);
gen_kill(AVAIL);
gen_kill(LIVE);
gen_kill(COPY);
du_chain(XXB);

data_flow(UNION,FORWARD,REACH);		/* Do global data flow */
data_flow(INTER,FORWARD,AVAIL);
data_flow(UNION,BACK,LIVE);
data_flow(INTER,FORWARD,COPY);

ud_chain(REACH);			/* compute at point chains */
ud_chain(AVAIL);
ud_chain(COPY);
du_chain(XX);				/* compute next uses for Z=X+Y */
du_chain(YY);
du_chain(ZZ);


/* printf("\n------------------PRE CODE OPTIMIZATION STATUS-----------------\n\n");
basic_block_dump();
*/

change_exp = 1;
while (change_exp)			/* do while elim expressions */
	{
printf("\n\n------------DOING SUB-EXP ELIM & COPY PROP OPERATIONS--------\n\n");
	change_exp = 0;
	common_subexp();		/* remove common subexps */
	re_hash();			/* re-link */
	copy_prop();			/* copy_propgation */
	re_hash();			/* re-link */
	if (change_exp)
		{
/* printf("\n\n-------------AFTER 1 PASS OF SUB ELIM AND COPY PROP----------\n\n");
basic_block_dump();
*/ 		}
	}
printf("\n\n---------------DOING LOOP SCANNING OPERATIONS----------------\n\n");
clean_up_vars();			/* get rid of unused temps */
find_dom();				/* find dominators */
find_loops();				/* find loops */
mark_invariant();			/* mark invariant code */
find_exits();				/* find loop exits */
code_motion();				/* move code to pre-header */
re_hash();				/* re-link stuff */


code_gen();
printf("\n\n\n----------------POST CODE OPTIMIZATION STATUS----------------\n\n");
basic_block_dump();			/* dump blocks post OPT*/
}


/*---------------------------------------------------------------------------*/
/*	du_chain()-
		Computes all uses for each definition or
		"live" information-
*/
/*---------------------------------------------------------------------------*/
du_chain(oper)
int oper;
{					/* t holds the target block */
int i,j,t;				/* i is to loop then basic blocks */
struct basic_b *p;			/* points to current block */
struct quad *q,*old;			/* points to current quad */
BITS temp,genbb,killbb,gensi,killsi;	/* gens and kill alg bit fields */
BITS chain;

for (i=0; i<b_count; i++)		/* loop through the blocks  */
	{
	p = b_list[i];			/* get the block pointer */
	q = p->b_start;			/* get the quad start */
	old = q;
	while (q != NULL)		/* scan to end, go reverse */
		{old = q;
		q = q->q_n;}
	q = old;
	quad_bl = q->line;		/* get the 1st line number */
	read_everything();		/* read everything */
	if (oper!=XXB) copy_bits(p->b_l_out,genbb);
		else reset_bits(genbb);
	reset_bits(killbb);
	while (q != NULL)		/* while still quads to read */
		{
		quad_bl = q->line;		/* get quad line # */
		read_everything();		/* read everything */
		live_use_def(gensi,killsi,i);
		or_bits(gensi,genbb,genbb);
		reset_bits(chain);
		if (definition())		/* get uses for def */
			{get_definition();
			get_expression();
			for (j=1; j<l_count; j++)
			 if (oper==XX && strcmp(darg,lt[j].l_sym)==0)
			   set_bit(chain,j,1);
			 else if (oper==XXB && strcmp(darg,lt[j].l_sym)==0)
			   set_bit(chain,j,1);
			 else if (oper==YY && strcmp(earg1,lt[j].l_sym)==0 &&
			 lt[j].l_line!=q->line)
			   set_bit(chain,j,1);
			 else if (oper==ZZ && expops==2 &&
			 lt[j].l_line!=q->line && strcmp(earg2,lt[j].l_sym)==0)
			   set_bit(chain,j,1);
			and_bits(chain,genbb,chain);}
		if (oper==XX) copy_bits(chain,q->q_du);
		else if (oper==XXB) copy_bits(chain,q->x_buse);
		else if (oper==YY) copy_bits(chain,q->y_next);
		else if (oper==ZZ) copy_bits(chain,q->z_next);
		minus_bits(genbb,killsi,genbb);	/* compute current gen/kill */
		q = q->q_p;
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	ud_chain(oper)-
		if oper = REACH then computes definitions for each use
		if open = AVAIL then finds avail expressions at point
		if open = copy then finds current copy info at a point
*/
/*---------------------------------------------------------------------------*/
ud_chain(oper)
int oper;
{					/* t holds the target block */
int i,j,t;				/* i is to loop then basic blocks */
struct basic_b *p;			/* points to current block */
struct quad *q;				/* points to current quad */
BITS temp,genbb,killbb,gensi,killsi;	/* gens and kill alg bit fields */
BITS chain,chain2;

for (i=0; i<b_count; i++)		/* loop through the blocks  */
	{
	p = b_list[i];			/* get the block pointer */
	q = p->b_start;			/* get the quad start */
	quad_bl = q->line;		/* get the 1st line number */
	read_everything();		/* read everything */
	if (oper == REACH)		/* select operation */
		{line_gen_kill(genbb,killbb);
		copy_bits(p->b_in,genbb);}
	else if (oper == AVAIL)
		{exp_gen_kill(genbb,killbb);
		copy_bits(p->b_e_in,genbb);}
	else if (oper == COPY)
		{copy_gen_kill(genbb,killbb);
		copy_bits(p->b_c_in,genbb);}
	reset_bits(killbb);
	while (q != NULL)		/* while still quads to read */
		{
		quad_bl = q->line;		/* get quad line # */
		read_everything();		/* read everything */
		if (oper == REACH)
			line_gen_kill(gensi,killsi);
		else if (oper == AVAIL)
			exp_gen_kill(gensi,killsi);
		else if (oper == COPY)
			copy_gen_kill(gensi,killsi);
		minus_bits(genbb,killsi,temp);	/* compute current gen/kill */
		or_bits(gensi,temp,genbb);
		minus_bits(killbb,gensi,temp);
		or_bits(killsi,temp,killbb);
		reset_bits(chain);
		reset_bits(chain2);
		if (definition())
			{get_definition();
			get_expression();      /* logic based on oper */
			if (oper == REACH)
			   for (j=1; j<d_count; j++)
				{if (strcmp(earg1,dt[j].d_sym)==0)
					set_bit(chain,j,1);
				if (strcmp(earg2,dt[j].d_sym)==0)
					set_bit(chain2,j,1);}
			if (oper == AVAIL)
			     for (j=1; j<e_count; j++)
				if (eq_exp(j) && et[j].e_line != quad_bl)	
					set_bit(chain,j,1);
			if (oper == COPY)
			     for (j=1; j<c_count; j++)
				if (ct[j].c_line != quad_bl &&
				  (strcmp(ct[j].c_sym2,earg1)==0 ||
				  strcmp(ct[j].c_sym2,earg2)==0))
					set_bit(chain,j,1); 
			and_bits(chain,genbb,chain);
			and_bits(chain2,genbb,chain2);}
		if (oper == REACH)
			{copy_bits(chain,q->q_ud1);      /* into quads */
			copy_bits(chain2,q->q_ud2);}
 		else if (oper == AVAIL)
			copy_bits(chain,q->q_exp);
 		else if (oper == COPY)
			copy_bits(chain,q->q_copy);
		q = q->q_n;
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	gen_kill(oper)-
		Compute gen kill information based on operator
		REACH,AVAIL,LIVE and COPY
*/
/*---------------------------------------------------------------------------*/
gen_kill(oper)
int oper;
{					/* t holds the target block */
int i,t;				/* i is to loop then basic blocks */
struct basic_b *p;			/* points to current block */
struct quad *q,*old;				/* points to current quad */
BITS temp,genbb,killbb,gensi,killsi;	/* gens and kill alg bit fields */

for (i=0; i<b_count; i++)		/* loop through the blocks  */
	{
	cur_block = i;
	p = b_list[i];			/* get the block pointer */
	q = p->b_start;			/* get the quad start */
	if (oper == LIVE)
		{old = q;		/* if live start backwards */
		while (q != NULL)
			{old = q;
			q = q->q_n;}
		q = old;}
	quad_bl = q->line;		/* get the 1st line number */
	read_everything();		/* read everything */
	if (oper == REACH)
		line_gen_kill(genbb,killbb);
	else if (oper == AVAIL)
		exp_gen_kill(genbb,killbb);
	else if (oper == LIVE)
		live_use_def(genbb,killbb);
	else if (oper == COPY)
		copy_gen_kill(genbb,killbb);
	while (q != NULL)		/* while still quads to read */
		{
		quad_bl = q->line;		/* get quad line # */
		read_everything();		/* read everything */
		if (oper == REACH)
			line_gen_kill(gensi,killsi);
		else if (oper == AVAIL)
			exp_gen_kill(gensi,killsi);
		else if (oper == LIVE)
			live_use_def(gensi,killsi);
		else if (oper == COPY)
			copy_gen_kill(gensi,killsi);
		minus_bits(genbb,killsi,temp);	/* compute current gen/kill */
		or_bits(gensi,temp,genbb);
		minus_bits(killbb,gensi,temp);
		or_bits(killsi,temp,killbb);
		if (oper != LIVE)
			q = q->q_n;		/* go forward */
		else
			q = q->q_p;		/* go backwards */
		}
	if (oper == REACH)
		{copy_bits(genbb,p->b_gen);
		copy_bits(killbb,p->b_kill);}
	else if (oper == AVAIL)
		{copy_bits(genbb,p->b_e_gen);
		copy_bits(killbb,p->b_e_kill);}
	else if (oper == LIVE)
		{copy_bits(genbb,p->b_l_gen);
		copy_bits(killbb,p->b_l_kill);}
	else if (oper == COPY)
		{copy_bits(genbb,p->b_c_gen);
		copy_bits(killbb,p->b_c_kill);}
	}
}


/*---------------------------------------------------------------------------*/
/*	link_up_blocks()-
		links up branch and target information
*/
/*---------------------------------------------------------------------------*/
link_up_blocks()
{					/* t holds the target block */
int i,t;				/* i is to loop then basic blocks */
struct basic_b *p;			/* points to current block */
struct quad *q;				/* points to current quad */

for (i=0; i<b_count; i++)		/* loop through the blocks  */
	{
	p = b_list[i];			/* get the block pointer */
	q = p->b_start;			/* get the quad start */
	quad_bl = q->line;		/* get the 1st line number */
	read_everything();		/* read everything */
	while (q != NULL)		/* while still quads to read */
		{
		quad_bl = q->line;		/* get quad line # */
		read_everything();		/* read everything */
		if (branch())			/* if found branch */
			{			/* link the blocks up! */
			fill_branch();
			t = scan_target_table(goto_sym);
			if (t == -1)
				printf("*ERR-could not find target\n");
			else
				insert_goto(i,t);
			if (cond_br && i+1<b_count && (i+1)!=t)
				insert_goto(i,i+1);
  			}
		else if (q->q_n == NULL && i+1<b_count)
			insert_goto(i,i+1);
		q = q->q_n;			/* get next quadword */
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	line_use_def(g,k)
			g - use bit field
			k - def bit field
		This routine gets the use and defs for a current line
		by looking at the definition table,
*/
/*---------------------------------------------------------------------------*/
live_use_def(g,k)
BITS g,k;
{
int i,found;				/* i loops through def table */
reset_bits(g);	/* found it, put 1 bit in def */
reset_bits(k);	/* found it, put 1 bit in def */
if (definition())			/* found = 1 when done with gen */
	{
	get_definition();		/* get definition symbol */
	get_expression();		/* get definition symbol */
	i=1;				/* start defs at 1 */
	found=0;			/* not found yet */
	while (i<l_count && !found)	/* go though table until found */
		{	
		if (strcmp(earg1,lt[i].l_sym)==0 && quad_bl == lt[i].l_line)
			set_bit(g,i,1);
		if (strcmp(earg2,lt[i].l_sym)==0 && quad_bl == lt[i].l_line)
			set_bit(g,i,1);
		i++;
		}
	i=1;
	while (i<l_count)		/* but at current gen */
		{	
		if (strcmp(darg,lt[i].l_sym) == 0 && quad_bl != lt[i].l_line)
			set_bit(k,i,1);
		i++;	
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	copy_gen_kill(g,k)
			g - gen bit field
			k - kill bit field
		This routine gets the gens and kills for a current line
		by looking at the definition table,
*/
/*---------------------------------------------------------------------------*/
copy_gen_kill(g,k)
BITS g,k;
{
int i,found;				/* i loops through def table */
reset_bits(g);	/* found it, put 1 bit in def */
reset_bits(k);	/* found it, put 1 bit in def */
if (definition())			/* found = 1 when done with gen */
	{
	get_definition();		/* get definition symbol */
	get_expression();		/* get definition symbol */
	i=1;				/* start defs at 1 */
	found=0;			/* not found yet */
	while (i<c_count && !found)	/* go though table until found */
		{	
		if (quad_bl == ct[i].c_line)
			{
			reset_bits(g);	/* found it, put 1 bit in def */
			set_bit(g,i,1);
			found=1;
			}
		i++;
		}
	i=1;
	reset_bits(k);			/* find kills, put 1's everywhere */
	while (i<c_count)		/* but at current gen */
		{	
		if (((strcmp(darg,ct[i].c_sym2)==0 || 
			strcmp(darg,ct[i].c_sym1)==0) &&
			cur_block != ct[i].c_block) || 
			(strcmp(darg,ct[i].c_sym1)==0 &&
			quad_bl != ct[i].c_line))
			{
			set_bit(k,i,1);
			}
		i++;	
		}
	}
}




/*---------------------------------------------------------------------------*/
/*	line_gen_kill(g,k)
			g - gen bit field
			k - kill bit field
		This routine gets the gens and kills for a current line
		by looking at the definition table,
*/
/*---------------------------------------------------------------------------*/
line_gen_kill(g,k)
BITS g,k;
{
int i,found;				/* i loops through def table */
reset_bits(g);	/* found it, put 1 bit in def */
reset_bits(k);	/* found it, put 1 bit in def */
if (definition())			/* found = 1 when done with gen */
	{
	get_definition();		/* get definition symbol */
	i=1;				/* start defs at 1 */
	found=0;			/* not found yet */
	while (i<d_count && !found)	/* go though table until found */
		{	
		if (strcmp(darg,dt[i].d_sym) == 0 && quad_bl == dt[i].d_line)
			{
			reset_bits(g);	/* found it, put 1 bit in def */
			set_bit(g,i,1);
			found=1;
			}
		i++;
		}
	i=1;
	reset_bits(k);			/* find kills, put 1's everywhere */
	while (i<d_count)		/* but at current gen */
		{	
		if (strcmp(darg,dt[i].d_sym) == 0 && quad_bl != dt[i].d_line)
			{
			set_bit(k,i,1);
			}
		i++;	
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	exp_gen_kill(g,k)
			g - gen bit field
			k - kill bit field
		This routine gets the gens and kills for reaching exps
		by looking at the exp & definition table,
*/
/*---------------------------------------------------------------------------*/
exp_gen_kill(g,k)
BITS g,k;
{
int i,found;				/* i loops through exp table */

reset_bits(g);	/* found it, put 1 bit in def */
reset_bits(k);	/* found it, put 1 bit in def */
if (definition())			/* found = 1 when done with gen */
	{
	get_expression();		/* get expression symbols */
	get_definition();		/* get expression symbols */
	i=1;				/* start defs at 1 */
	found=0;			/* not found yet */
	while (i<e_count && !found)	/* go though table until found */
		{	
		if (eq_exp(i) && expops == 2 && quad_bl == et[i].e_line)
			{
			reset_bits(g);	/* found it, put 1 bit in def */
			set_bit(g,i,1);
			found=1;
			}
		i++;
		}
	i=1;
	reset_bits(k);			/* find kills, put 1's everywhere */
	while (i<e_count)		/* but at current gen */
		{	
		if (strcmp(darg,et[i].e_sym1) == 0 ||
		   strcmp(darg,et[i].e_sym2)==0)
			{
			set_bit(k,i,1);
			}
		i++;	
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	display_bits(f)
			f - the bit field to show
		Shows number for bits displayed.
*/
/*---------------------------------------------------------------------------*/
display_bits(f)
BITS f;
{int i,j;
for (i=0; i<B_F; i++)				/* decode address into field */
	for (j=1; j<=32; j++)
		if (f[i] & magic[j]) 
			printf("%1d ",i*32+j);	/* show it */
printf("\n");}


/*---------------------------------------------------------------------------*/
/*	display_bits_loop(f)
			f - the bit field to show
		Shows number for bits displayed - 1 for loop info.
*/
/*---------------------------------------------------------------------------*/
display_loop_bits(f)
BITS f;
{int i,j;
for (i=0; i<B_F; i++)				/* decode address into field */
	for (j=1; j<=32; j++)
		if (f[i] & magic[j]) 
			printf("%1d ",i*32+j-1);	/* show it */
printf("\n");}


/*---------------------------------------------------------------------------*/
/*	display_bits(f)
			f - the bit field to show
		Shows to standard error number for bits displayed.
			(for debugging purposes)
*/
/*---------------------------------------------------------------------------*/
display_bits2(f)
BITS f;
{int i,j;
for (i=0; i<B_F; i++)				/* decode address into field */
	for (j=1; j<=32; j++)
		if (f[i] & magic[j]) 
			fprintf(stderr,"%1d ",i*32+j);	/* show it */
fprintf(stderr,"\n");}


/*---------------------------------------------------------------------------*/
/*	copy_bits(f)
			f - the bit field to copy from
			t - the bit field to copy to
		Copies one bit field to another. 
*/
/*---------------------------------------------------------------------------*/
copy_bits(f,t)
BITS f,t;
{int i;
for (i=0; i<B_F; i++)
	t[i] = f[i];}


/*---------------------------------------------------------------------------*/
/*	reset_bits(f)
			f - the bit field to reset
		Resets all bits to zeros.
*/
/*---------------------------------------------------------------------------*/
reset_bits(f)
BITS f;
{int i;
for (i=0; i<B_F; i++)
	f[i] = 0;}

/*---------------------------------------------------------------------------*/
/*	all_bits(f)
			f - the bit field to set to all 1's
		Sets all bits to one.
*/
/*---------------------------------------------------------------------------*/
all_bits(f)
BITS f;
{int i;
for (i=0; i<B_F; i++)
	f[i] = -1;}

/*---------------------------------------------------------------------------*/
/*	comp_bits(f)
			f,g  -  the bit field to compare
		Returns a 0 if they are == and a 1 if not.
*/
/*---------------------------------------------------------------------------*/
int comp_bits(f,g)
BITS f,g;
{int i;
for (i=0; i<B_F; i++)
	if (f[i] != g[i]) return(1);
return(0);}

/*---------------------------------------------------------------------------*/
/*	or_bits(f)
			x - the bit field to or
			y - the bit field to or
			z - the bit field to put result
		Ors big ass bit fields together and jams result in z.
*/
/*---------------------------------------------------------------------------*/
or_bits(x,y,z)
BITS x,y,z;
{int i;
for (i=0; i<B_F; i++)
	z[i] = x[i] | y[i];}

/*---------------------------------------------------------------------------*/
/*	and_bits(x,y,z)
			x - the bit field to and
			y - the bit field to and
			z - the bit field to put result
		Ands big ass bit fields together and jams result in z.
*/
/*---------------------------------------------------------------------------*/
and_bits(x,y,z)
BITS x,y,z;
{int i;
for (i=0; i<B_F; i++)
	z[i] = x[i] & y[i];}


/*---------------------------------------------------------------------------*/
/*	minus_bits(f)
			x - the bit field to minus
			y - the bit field to minus
			z - the bit field to put result
		Logically subtracts big ass bit fields together 
		and jams result in z. 
*/
/*---------------------------------------------------------------------------*/
minus_bits(x,y,z)
BITS x,y,z;
{int i;
for (i=0; i<B_F; i++)
	z[i] = x[i] & ~y[i];}


/*---------------------------------------------------------------------------*/
/*	set_bit(b,index,to)
			b - the bit field to set
			index - what bit to set (or reset) 
			to - a 1 sets bit and a 0 resets
		Sets or resets and indexed bit in BITS bitfield.
*/
/*---------------------------------------------------------------------------*/
set_bit(b,index,to)
BITS b;
unsigned index;
int to;
{
unsigned bit,ind;
ind = (index-1)/WORD_SIZE;		/* compute word to go in */
bit = ((index-1) % WORD_SIZE) + 1;	/* compute bit in word to flip */
if (to == 0)
	b[ind] &= ~(magic[bit]);	/* reset bit */
else
	b[ind] |= magic[bit];		/* set the bit */
}

/*---------------------------------------------------------------------------*/
/*	check_bit(b,index)
			b - the bit field to check
			index - what bit to check (or reset) 
		Sets or resets and indexed bit in BITS bitfield.
*/
/*---------------------------------------------------------------------------*/
check_bit(b,index)
BITS b;
unsigned index;
{
unsigned bit,ind;
ind = (index-1)/WORD_SIZE;		/* compute word to go in */
bit = ((index-1) % WORD_SIZE) + 1;	/* compute bit in word to flip */
if ((b[ind] & (magic[bit])) != 0)
	return(1);
else return(0);
}

/*---------------------------------------------------------------------------*/
/*	print_bits(f)
			v - the bit field to show
		Shows 1's and 0's for only one word.
*/
/*---------------------------------------------------------------------------*/
print_bits(v)
int v;
{int i,mask;
mask = 1; 
mask <<= 31;
for (i=1; i<=32; ++i)
	{putchar(((v & mask) == 0) ? '0' : '1');
	v <<= 1;}
}


/*---------------------------------------------------------------------------*/
/*	fill_magic()
		Fills magic with all powers of 2 up to WORD_SIZE so
		I don't have to screw with it.
*/
/*---------------------------------------------------------------------------*/
fill_magic()
{int i,j;
i = 1;
j = 1;
for (i=1; i<=WORD_SIZE; i++)
	{magic[i] = j;
	j = j * 2;}
}


/*---------------------------------------------------------------------------*/
/*	save_definition()
		Checks for a definition in quad and if exists
		will toss the info into the definition symbol
		table.
*/
/*---------------------------------------------------------------------------*/
save_definition()
{
if (definition()==1)
	{
	get_definition();			/* get string for symbol */
	strcpy(dt[d_count].d_sym,darg);		/* copy info */
	dt[d_count].d_line = quad_bl;
	dt[d_count].d_ptr = q_curr;
	dt[d_count].d_block = b_count;
	d_count++;
	}
}

/*---------------------------------------------------------------------------*/
/*	save_copy()
		Checks for a copy in quad and if exists
		will toss the info into the copy symbol
		table.
*/
/*---------------------------------------------------------------------------*/
save_copy()
{
if (definition() && !branch() && !target())
  {
  get_expression();			/* get string for symbol */
  get_definition();			/* get string for symbol */
  if (expops == 1)
	{
	strcpy(ct[c_count].c_sym1,earg1);		/* copy info */
	strcpy(ct[c_count].c_sym2,darg);		/* copy info */
	ct[c_count].c_com = command_bl;
	ct[c_count].c_ptr = q_curr;
	ct[c_count].c_line = quad_bl;
	ct[c_count].c_block = b_count;
	c_count++;
	}
  }
}


/*---------------------------------------------------------------------------*/
/*	save_var()
		Checks for a T_DECLARE or T_TEMP quad and if exists
		will toss the info into the var symbol
		table.
*/
/*---------------------------------------------------------------------------*/
save_vars()
{
int i,found;
if (definition()==1)
	{
	found = 0;
	for (i=1; i<v_count; i++)
		if (strcmp(vt[i].v_sym,darg)==0) found=1;
	if (found==0 && strcmp(darg,"")!=0 && strncmp(darg," ",1)!=0)
		{
		strcpy(vt[v_count].v_sym,darg);
		vt[v_count].v_reg = -1;
		vt[v_count].v_mem = v_count;
		v_count++;
		}
	found = 0;
	for (i=1; i<v_count; i++)
		if (strcmp(vt[i].v_sym,earg1)==0) found=1;
	if (found==0 && strcmp(earg1,"")!=0 && strncmp(earg1," ",1)!=0)
		{
		strcpy(vt[v_count].v_sym,earg1);
		vt[v_count].v_reg = -1;
		vt[v_count].v_mem = v_count;
		v_count++;
		}
	found = 0;
	if (expops == 2)
	   for (i=1; i<v_count; i++)
		if (strcmp(vt[i].v_sym,earg2)==0) found=1;
	if (found==0 && strcmp(earg2,"")!=0 && strncmp(earg2," ",1)!=0)
		{
		strcpy(vt[v_count].v_sym,earg2);
		vt[v_count].v_reg = -1;
		vt[v_count].v_mem = v_count;
		v_count++;
		}
	}

if (optype==T_DECLARE || optype==T_TEMP)
	{
	sscanf(carg1,"%s",&st[s_count].s_sym);
	sscanf(carg2,"%d",&st[s_count].s_s1);
	sscanf(carg3,"%d",&st[s_count].s_s2);
	sscanf(carg4,"%d",&st[s_count].s_s3);
	sscanf(carg5,"%d",&st[s_count].s_s4);
	st[s_count].s_ptr = q_curr;
	st[s_count].s_mode1 = mode1;
	st[s_count].s_mode2 = mode2;
	s_count++;
	}
}


/*---------------------------------------------------------------------------*/
/*	save_var()
		Checks for a definition quad and if exists
		will toss the use info into the live symbol
		table.
*/
/*---------------------------------------------------------------------------*/
save_live()
{
if (definition())
	{
	get_expression();
	strcpy(lt[l_count].l_sym,earg1);		/* copy info */
	lt[l_count].l_line = quad_bl;
	lt[l_count].l_ptr = q_curr;
	lt[l_count].l_block = b_count;
	l_count++;
	if (expops == 2)
		{
		strcpy(lt[l_count].l_sym,earg2);		/* copy info */
		lt[l_count].l_line = quad_bl;
		lt[l_count].l_ptr = q_curr;
		lt[l_count].l_block = b_count;
		l_count++;
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	save_expression()
		Checks for a expression (z=x+y) in quad and if exists
		will toss the info into the expression symbol
		table.
*/
/*---------------------------------------------------------------------------*/
save_expression()
{
if (definition() && !branch() && !target())
  {
  get_expression();			/* get string for symbol */
  if (expops == 2)
	{
	strcpy(et[e_count].e_sym1,earg1);		/* copy info */
	strcpy(et[e_count].e_sym2,earg2);		/* copy info */
	et[e_count].e_optype = optype;
	et[e_count].e_line = quad_bl;
	et[e_count].e_block = b_count;
	et[e_count].e_ptr = q_curr;
	et[e_count].e_dead = 0;
	e_count++;
	}
   }
}


/*---------------------------------------------------------------------------*/
/*	get_definition()
		Gets any definitions into darg string.
*/
/*---------------------------------------------------------------------------*/
get_definition()
{
switch(optype)				/* translate right opcode for arg */
	{
	case T_ADD		:
	case T_SUB		:
	case T_MULT		:
	case T_DIV		:
	case T_LT		:
	case T_LE		:
	case T_EQ		:
	case T_NE		:
	case T_GT		:
	case T_GE		:
	case T_OR		:
	case T_AND		:
	case T_XOR		:
		strcpy(darg,carg3);
		break;
	case T_MVSC		:
	case T_MVPA		:
		strcpy(darg,carg2);
		break;
	case T_INPUT		:
		strcpy(darg,carg1);
		break;
	
	case T_FOR		:
		strcpy(darg,carg2);
		break;
	}
}


/*---------------------------------------------------------------------------*/
/*	get_expression()
		Gets any expression into earg1, earg2 strings.
*/
/*---------------------------------------------------------------------------*/
get_expression()
{
expops = 0;
switch(optype)				/* translate right opcode for arg */
	{
	case T_ADD		:
	case T_SUB		:
	case T_MULT		:
	case T_DIV		:
	case T_LT		:
	case T_LE		:
	case T_EQ		:
	case T_NE		:
	case T_GT		:
	case T_GE		:
	case T_OR		:
	case T_AND		:
	case T_XOR		:
		strcpy(earg1,carg1);
		strcpy(earg2,carg2);
		expops = 2;
		break;
	case T_MVSC		:
	case T_MVPA		:
		strcpy(earg1,carg1);
		strcpy(earg2,"");
		expops = 1;
		break;
	case T_INPUT		:
		strcpy(earg1,carg2);
		strcpy(earg2,"");
		expops = 1;
		break;
	
	case T_ENDFOR		:
		strcpy(earg1,carg1);
		expops = 1;
		break;
	case T_FOR		:
		strcpy(earg1,carg1);
		expops = 1;
		break;
	case T_IF		:
	case T_UNTIL		:
		strcpy(earg1,carg1);
		expops = 1;
		break;
	}
}


/*---------------------------------------------------------------------------*/
/*	int definition()
		Returns a TRUE if the quad contains a definition.
*/
/*---------------------------------------------------------------------------*/
int definition()
{
switch(optype)				/* check optype for def */
	{				/* HOPE I GOT EM ALL!!!--ack! */
	case T_ADD		:
	case T_SUB		:
	case T_MULT		:
	case T_DIV		:
	case T_LT		:
	case T_LE		:
	case T_EQ		:
	case T_NE		:
	case T_GT		:
	case T_GE		:
	case T_OR		:
	case T_AND		:
	case T_XOR		:
	case T_MVSC		:
	case T_MVPA		:
	case T_INPUT		:

	case T_FOR		:
		return(1);
	case T_ENDFOR		:
	case T_IF		:
	case T_UNTIL		:
		return(2);
	}
return(FALSE);
}


/*---------------------------------------------------------------------------*/
/*	insert_goto(from,to)
			from - block number branching from
			to   - block number branching to
		The code here creates branch nodes with nalloc()
		and puts the branch information if these and
		then links the nodes off of the basic blocks.
*/
/*---------------------------------------------------------------------------*/
insert_goto(from,to)
int from,to;
{
struct cntl *p;			/* control node (saves links) */

b_curr = b_list[from];		/* get from block and link to TO block */
p = nalloc();
p->c_bb = to;
p->c_n = b_curr->b_succ;
p->c_p = NULL;
if (b_curr->b_succ != NULL)
	b_curr->b_succ->c_p = p;
b_curr->b_succ = p;

b_curr = b_list[to];		/* get to block and link to FROM block */
p = nalloc();
p->c_bb = from;
p->c_n = b_curr->b_pred;
p->c_p = NULL;
if (b_curr->b_pred != NULL)
	b_curr->b_pred->c_p = p;
b_curr->b_pred = p;
}


/*---------------------------------------------------------------------------*/
/*	re_hash() - Re-links stuff and re-propagates data-flow info.
		Looks through the global data flow after changes
		are made to reflect the new situation.
*/
/*---------------------------------------------------------------------------*/
re_hash()
{
int i,tmp;
struct basic_b *p;			/* points to current block */
struct quad *q;				/* points to current quad */

d_count=1; e_count=1; l_count=1; s_count=1; c_count=1; v_count=1;
for (i=0; i<b_count; i++)		/* loop through the blocks  */
	{ p = b_list[i];		/* get the block pointer */
	q = p->b_start;			/* get the quad start */
	reset_bits(q->q_ud1);
	reset_bits(q->q_ud2);
	reset_bits(q->q_du);
	reset_bits(q->q_exp);
	reset_bits(q->q_copy);
	reset_bits(q->y_next);
	reset_bits(q->z_next);
	reset_bits(q->x_buse);
	quad_bl = q->line;
	read_everything();
	while (q != NULL)
		{ quad_bl = q->line;		/* get quad line # */
		read_everything();		/* read everything */
		tmp = b_count;
		b_count = i;
		q_curr = q;
		save_definition();		/* save targets defs and code */
		save_expression();		/* save the expressions */
		save_live(); save_vars(); save_copy();
		b_count = tmp;
		q = q->q_n; }
	}
gen_kill(REACH); gen_kill(AVAIL); gen_kill(LIVE); gen_kill(COPY);
du_chain(XXB);
data_flow(UNION,FORWARD,REACH); data_flow(INTER,FORWARD,AVAIL);
data_flow(UNION,BACK,LIVE); data_flow(INTER,FORWARD,COPY);
ud_chain(REACH); ud_chain(AVAIL); ud_chain(COPY); 
du_chain(XX);
du_chain(YY);
du_chain(ZZ);
}


/*---------------------------------------------------------------------------*/
/*	insert_code()
		Insert_code links in a doublely linked lisrt (in order)
		a quad into the current basic block.
*/
/*---------------------------------------------------------------------------*/
insert_code()
{
struct quad *p;			/* new quad node */

p = qalloc();			/* get quad node space */
p->q_n = NULL;			/* link it last so next is ALWAYS null */
p->line = quad_bl;		/* save line number for reference */
if (q_curr == NULL)		/* this is the FIRST node on basic block */
	{
	p->q_p = NULL;
 	b_curr->b_start = p;	
	}
else
	{			/* this is NOT the FIRST node on the block */
	p->q_p = q_curr;
	q_curr->q_n = p;
	}
q_curr = p;			/* update current quadword pointer */
reset_bits(p->q_ud1);
reset_bits(p->q_ud2);
reset_bits(p->q_du);
reset_bits(p->q_exp);
reset_bits(p->q_copy);
reset_bits(p->x_buse);
reset_bits(p->y_next);
reset_bits(p->z_next);
p->q_invar = 0;

b_curr->count++;		/* update count of lines in block */
}


/*---------------------------------------------------------------------------*/
/*	int branch()
		Returns true if we have a quad with a branch
		and false otherwise.
*/
/*---------------------------------------------------------------------------*/
int branch()
{
switch(optype)				/* just check to optype */
	{
/*	case T_ENDMAIN		:
*/	case T_ANY		:
	case T_ELSENANY		:
	case T_ENDWHILE		:
	case T_UNTIL		:
	case T_ENDLOOP		:
	case T_IF		:
	case T_RETURN		:
/*	case T_STOP		:
*/	case T_CALL		:
	case T_ENDFOR		:
	case T_NEXT 		:
	case T_GET		:
	case T_WHILE		:
	case T_FOR		:
	case T_ENDSTACKWHILE	:
	case T_RECURSE	 	:
	case T_ENDSUB		:
	case T_ELSE		:
		return(TRUE);
	}
return(FALSE);
}


/*---------------------------------------------------------------------------*/
/*	fill_branch()
		Fill a branch string (goto_sym) with the branch (to)
		symbol---used to match up targets.
		Cond_br is set to TRUE for a condition branch
		and FALSE for a unconditional branch.
*/
/*---------------------------------------------------------------------------*/
fill_branch()
{
switch(optype)				/* check optype */
	{
	case T_ENDMAIN		:
	case T_ENDSUB		:
		strcpy(goto_sym,carg1);	/* get operand right for optype */
		cond_br = FALSE;
		break;
	case T_ELSENANY		:
	case T_ENDLOOP		:
		strcpy(goto_sym,carg2);
		cond_br = FALSE;
		break;	
	case T_ENDFOR		:
		strcpy(goto_sym,carg3);
		cond_br = FALSE;
		break;	
	case T_ANY		:
	case T_ENDWHILE		:
		strcpy(goto_sym,carg2);
		cond_br = FALSE;
		break;	
	case T_UNTIL		:
	case T_IF		:
		strcpy(goto_sym,carg2);
		cond_br = TRUE;
		break;	
	case T_NEXT 		:
	case T_GET		:
	case T_WHILE		:
	case T_FOR		:
		strcpy(goto_sym,carg3);
		cond_br = TRUE;
		break;	
	case T_ENDSTACKWHILE	:
	case T_RECURSE	 	:
		strcpy(goto_sym,carg4);
		cond_br = TRUE;
		break;	
	case T_RETURN		:
	case T_STOP		:
	case T_CALL		:
		strcpy(goto_sym,"");
		cond_br = FALSE;
		break;	
	case T_ELSE		:		/* 2 cases for T_ELSE */
		if (mode2 == T_PARALLEL)
			{	
			strcpy(goto_sym,carg1);
			cond_br = TRUE;
			}
		else
			{
			strcpy(goto_sym,carg1);
			cond_br = FALSE;
			}
		break;
	}
}


/*---------------------------------------------------------------------------*/
/*	int scan_target_table(s)
			s - a 100 byte string(max) to match
		Look through the target table for a match (should only
		be one--boy)  If found return block of that branch---
		this means the actual target could be in a quad in the
		previous block from where we are jumping.
*/
/*---------------------------------------------------------------------------*/
int scan_target_table(s)
char *s;
{
int i;				/* loop through table */
					
for (i=0; i<t_count; i++)
	if (strcmp(s,tt[i].t_sym) == 0)
		return(tt[i].t_block);
return(-1);
}


/*---------------------------------------------------------------------------*/
/*	save_target()
		Checks to see if we have a target quad and handles
		special yukey case and if true fills the target table
		entry.
*/
/*---------------------------------------------------------------------------*/
save_target()
{							/* check spec cases */
if (target() || optype==T_ENDFOR || optype==T_ENDLOOP || optype==T_ENDWHILE
	|| (optype==T_ELSE && mode2!=T_PARALLEL))
	{
	get_target();					/* get target symbol */
	strcpy(tt[t_count].t_sym,targ);			/* fill in info */
	tt[t_count].t_line = quad_bl;
	tt[t_count].t_block = b_count;
	if (optype==T_ENDFOR || optype==T_ENDLOOP 	/* special cases */
	   || optype==T_ENDWHILE
           || (optype==T_ELSE && mode2!=T_PARALLEL))	/* to next block */
		tt[t_count].t_block++;
	t_count++;					/* go next entry */
	}
}


/*---------------------------------------------------------------------------*/
/*	get_target()
		Get the target symbol and put it in string targ.
*/
/*---------------------------------------------------------------------------*/
get_target()
{
switch(optype)				/* check optype */
	{
	case T_SUBROUTINE	:
  	case T_LABEL		:
	case T_MAIN		:
	case T_ENDNEXT		:
	case T_ENDANY		:
	case T_ENDGET		:
	case T_BEGWHILE		:
	case T_LOOP		:
	case T_ENDIF		:
	case T_ELSENANY		:
	case T_ENDWHILE		:
	case T_ENDLOOP		:	
		strcpy(targ,carg1);
		break;
	case T_ENDFOR		:
		strcpy(targ,carg2);
		break;
	case T_ENDSTACKWHILE	:
	case T_RECURSE		:
	case T_BEGSTACK		:
	case T_STACKWHILE	:
		strcpy(targ,carg3);
		break;
	case T_FOR 		:	
		strcpy(targ,carg4);
		break;
	case T_ELSE		:
		if (mode2 == T_PARALLEL) break;
		strcpy(targ,carg2);
		break;
	}
}


/*---------------------------------------------------------------------------*/
/*	int target()
		Returns true if quad is a target and false otherwise.
*/
/*---------------------------------------------------------------------------*/
int target()
{
switch(optype)				/* check to see if target */
	{
	case T_SUBROUTINE	:
  	case T_LABEL		:
/*	case T_MAIN		:
*/	case T_ENDNEXT		:
	case T_ENDANY		:
	case T_ENDGET		:
	case T_BEGWHILE		:
	case T_LOOP		:
	case T_ENDIF		:
	case T_ELSENANY		:
/*	case T_ENDWHILE		:
	case T_ENDLOOP		:
	case T_ENDFOR		:
*/	case T_BEGSTACK		:
	case T_FOR		:
	case T_STACKWHILE	:
	case T_ENDSTACKWHILE	:
	case T_RECURSE		:
		return(TRUE);
/*	case T_ELSE		:
		if (mode2 == T_PARALLEL) break; 
		return(TRUE);
*/	}
return(FALSE);
}


/*---------------------------------------------------------------------------*/
/*	basic_block_dump()
		Dump all the basic block info out...includes
			- block number
			- number of quads
			- the quad string
			- the exits of the block
			- the entries of the block
			- the gens and kills at the end point of 
				the block
			- the definition symbol tabel
			- the target symbol table
*/
/*---------------------------------------------------------------------------*/
basic_block_dump()
{
int i;
struct basic_b *p;			/* current basic block pointer */
struct quad *q;				/* current quad pointer */
struct cntl *c;				/* current control pointer */

reset_bits(hit_blocks);
for (i=0; i<b_count; i++)		/* loop through all blocks */
	{
	p = b_list[i];
	printf("   ### BASIC BLOCK #%d ###   (%d) lines",i,p->count);
	printf("----------------------------------------\n");
	q = p->b_start;
	while (q != NULL)		/* loop through quads */
		{
		quad_bl = q->line;
		read_everything();
		if (q->q_invar) printf("INV"); else printf("   ");
		printf("(%10d)%s\n",quad_bl,quads_bl[quad_bl]);
		if (comp_bits(hit_blocks,q->q_ud1))
	  	  {printf("\tud-chain left >"); display_bits(q->q_ud1);}
		if (comp_bits(hit_blocks,q->q_ud2))
		  {printf("\tud-chain right>"); display_bits(q->q_ud2);}
		if (comp_bits(hit_blocks,q->q_du))
		  {printf("\tdu-chain      >"); display_bits(q->q_du);}
		if (comp_bits(hit_blocks,q->q_exp))
		  {printf("\texp-chain     >"); display_bits(q->q_exp);}
		if (comp_bits(hit_blocks,q->q_copy))
		  {printf("\tcopy-chain    >"); display_bits(q->q_copy);}
		if (comp_bits(hit_blocks,q->y_next))
		  {printf("\tY_NEXT-chain  >"); display_bits(q->y_next);}
		if (comp_bits(hit_blocks,q->z_next))
		  {printf("\tZ_NEXT-chain  >"); display_bits(q->z_next);}
		if (comp_bits(hit_blocks,q->x_buse))
		  {printf("\tX_BUSE-chain  >"); display_bits(q->x_buse);}
		q = q->q_n;
		}
	c = p->b_pred;			/* loop through predecessors */
	printf("    - ENTER this block from blocks :");
	while (c != NULL)
		{
		printf("(%1d) ",c->c_bb);
		c = c->c_n;
		}
	printf("\n");
	printf("    - EXIT this block to blocks    :");
	c = p->b_succ;			/* loop through succesors */
	while (c != NULL)
		{
		printf("(%1d) ",c->c_bb);
		c = c->c_n;
		}
	printf("\n");
/*    if (comp_bits(hit_blocks,p->b_gen))    {
	printf("REACH GEN ->");		
	display_bits(p->b_gen);}
    if (comp_bits(hit_blocks,p->b_kill))    {
	printf("REACH KILL->");
	display_bits(p->b_kill);}
    if (comp_bits(hit_blocks,p->b_in))    {
	printf("REACH IN  ->");		
	display_bits(p->b_in);}
    if (comp_bits(hit_blocks,p->b_out))    {
	printf("REACH OUT ->");
	display_bits(p->b_out);}
    if (comp_bits(hit_blocks,p->b_e_gen))    {
	printf("AVAIL GEN ->");		
	display_bits(p->b_e_gen);}
    if (comp_bits(hit_blocks,p->b_e_kill))    {
	printf("AVAIL KILL->");
	display_bits(p->b_e_kill);}
    if (comp_bits(hit_blocks,p->b_e_in))    {
	printf("AVAIL IN  ->");		
	display_bits(p->b_e_in);}
    if (comp_bits(hit_blocks,p->b_e_out))    {
	printf("AVAIL OUT ->");
	display_bits(p->b_e_out);}
    if (comp_bits(hit_blocks,p->b_l_gen))    {
	printf("LIVE USE  ->");		
	display_bits(p->b_l_gen);}
    if (comp_bits(hit_blocks,p->b_l_kill))    {
	printf("LIVE DEF  ->");
	display_bits(p->b_l_kill);}
    if (comp_bits(hit_blocks,p->b_l_in))    {
	printf("LIVE IN   ->");		
	display_bits(p->b_l_in);}
    if (comp_bits(hit_blocks,p->b_l_out))    {
	printf("LIVE OUT  ->");
	display_bits(p->b_l_out);}
    if (comp_bits(hit_blocks,p->b_c_gen))    {
	printf("COPY GEN  ->");		
	display_bits(p->b_c_gen);}
    if (comp_bits(hit_blocks,p->b_c_kill))    {
	printf("COPY KILL ->");
	display_bits(p->b_c_kill);}
    if (comp_bits(hit_blocks,p->b_c_in))    {
	printf("COPY IN   ->");		
	display_bits(p->b_c_in);}
    if (comp_bits(hit_blocks,p->b_c_out))    {
	printf("COPY OUT  ->");
	display_bits(p->b_c_out);}
*/	printf("\n");
	}						/* dump tables */
printf("\n    *** DUMPING TARGET TABLE ***\n");
for (i=0; i<t_count; i++)
	printf("line(%5d)  block(%5d)   %s\n",
		tt[i].t_line, tt[i].t_block, tt[i].t_sym);
printf("\n    *** DUMPING DEFINITION TABLE ***\n");
for (i=1; i<d_count; i++)
	printf("def#(%5d)  line(%5d)  block(%5d)   %s\n",
		i,dt[i].d_line, dt[i].d_block, dt[i].d_sym);
printf("\n    *** DUMPING EXPRESSION TABLE ***\n");
for (i=1; i<e_count; i++)
	printf("dead[%1d]  exp#(%5d)  line(%5d)  block(%5d) %s (%5x) %s\n",
		et[i].e_dead,i,et[i].e_line,et[i].e_block,
		et[i].e_sym1,et[i].e_optype,et[i].e_sym2);
printf("\n    *** DUMPING USEAGE TABLE ***\n");
for (i=1; i<l_count; i++)
	printf("var#(%5d)  line(%5d)  block(%5d)  %s\n",
		i, lt[i].l_line, lt[i].l_block, lt[i].l_sym);
printf("\n    *** DUMPING COPY TABLE ***\n");
for (i=1; i<c_count; i++)
	printf("var#(%5d)  line(%5d)  block(%5d)  %4x %s %s\n",
		i, ct[i].c_line, ct[i].c_block, ct[i].c_com,
		ct[i].c_sym1, ct[i].c_sym2);
printf("\n    *** DUMPING VAR TABLE ***\n");
for (i=1; i<s_count; i++)
	printf("var#(%5d) %1d%1d %5d %5d %5d %5d %s\n",
		i, st[i].s_mode1, st[i].s_mode2,
		st[i].s_s1, st[i].s_s2,
		st[i].s_s3, st[i].s_s4, st[i].s_sym);
printf("\n    *** DUMPING DEF USE TABLE ***\n");
for (i=1; i<v_count; i++)
	printf("var#(%5d) %s\n",i,vt[i].v_sym);
}


/*---------------------------------------------------------------------------*/
/*	int make_basic_block()
		Creates new node by calling balloc and enters
		the block in the table.
*/
/*---------------------------------------------------------------------------*/
int make_basic_block()
{
b_list[b_count] = b_curr;
q_curr = NULL;
return(balloc());
}


/*---------------------------------------------------------------------------*/
/*	int balloc()
		Create and allocate space for a basic block node,
		fill in fields to be SAFE.
*/
/*---------------------------------------------------------------------------*/
int balloc()
{
struct basic_b *p;
p = malloc(sizeof(struct basic_b));
p->b_num = b_count++;
p->count = 0;
p->b_start = NULL;
p->b_pred = NULL;
p->b_succ = NULL;		/* set all set bist to 0 */
reset_bits(p->b_dom); reset_bits(p->b_dom2);
reset_bits(p->b_l_gen); reset_bits(p->b_l_kill);
reset_bits(p->b_l_in); reset_bits(p->b_l_out);
reset_bits(p->b_e_gen); reset_bits(p->b_e_kill);
reset_bits(p->b_e_in); reset_bits(p->b_e_out);
reset_bits(p->b_c_gen); reset_bits(p->b_c_kill);
reset_bits(p->b_c_in); reset_bits(p->b_c_out);
reset_bits(p->b_in); reset_bits(p->b_out);
reset_bits(p->b_gen); reset_bits(p->b_kill);
return((int)p);}


/*---------------------------------------------------------------------------*/
/*	int qalloc()
		Create and allocate space for a quad word node,
		fill in fields to be SAFE.
*/
/*---------------------------------------------------------------------------*/
int qalloc()
{
struct quad *p;
p = malloc(sizeof(struct quad));
p->line = -1;
p->q_n = NULL;
p->q_p = NULL;
return((int)p);}


/*---------------------------------------------------------------------------*/
/*	int nalloc()
		Create and allocate space for a control node,
		fill in fields to be SAFE.
*/
/*---------------------------------------------------------------------------*/
int nalloc()
{
struct cntl *p;
p = malloc(sizeof(struct cntl));
p->c_bb = NULL;
p->c_n = NULL;
p->c_p = NULL;
return((int)p);}



