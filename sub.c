i sub
/*---------------------------------------------------------------------------*/
/*	COMMON SUB-EXPRESSION ELIMINATION
	and COPY PROPAGATION
*/
/*---------------------------------------------------------------------------*/
#include "op_common.h"

/*---------------------------------------------------------------------------*/
/*	common_subexp()-
		Do common sub expression elimination by looking for
		matching avail exps. at a given point where they
		are being used. Then check conditions.
*/
/*---------------------------------------------------------------------------*/
common_subexp()
{
int i,k,j,a;			/* loop vars */
struct quad *q;			/* line ptr */
BITS avl;			/* available exps */

for (i=1; i<e_count; i++)
	{
	for (k=0; k<B_F; k++)			/* scan through avail bits */
	  for (j=1; j<=32; j++)
  		{ 
		q = et[i].e_ptr;
		quad_bl = q->line;
		read_everything();		/* read everything */
		strcpy(move_var,carg3);
		get_definition();
		get_expression();
		copy_bits(q->q_exp,avl);	/* save info on line */
		m1 = mode1;
		m2 = mode2;
		a = k*32+j;
		if (check_bit(avl,a))
			if (eq_exp(a) && et[a].e_line != quad_bl && 
				!et[a].e_dead)
				{
				printf("AVAIL: exp(%d) avail at exp (%d)\n",a,i);
				if (scan_back(a,q->q_p)==1)  /* in block */
					{
					new_exp_var();	     /* change code */
					change_avail(a);
					change_common(i);}
				else
					{		     /* search preds */
					reset_bits(hit_blocks);
					search_exp(et[i].e_block,a,i);}
				}
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	clean_up_vars()-
		This routine checks for all uses and definitions
		of subexpression elimination induced variables.
		It gets rid of any that arened needed in the
		T_DEFINE section early on in the code.
*/
/*---------------------------------------------------------------------------*/
clean_up_vars()
{
int i,j,found;				/* 2 loops and a flag */
struct basic_b *p;			/* basic block ptr */
struct quad *q,*oldq;			/* line ptrs */

for (i=1; i<s_count; i++)		/* loop through symbols */
	{
	found = 0;			/* look for no uses and defs */
	for (j=1; j<l_count; j++)
		if (strcmp(st[i].s_sym,lt[j].l_sym)==0)
			found = 1;
	for (j=1; j<d_count; j++)
		if (strcmp(st[i].s_sym,dt[j].d_sym)==0)
			found = 1;
	if ( ! found && strncmp(st[i].s_sym,"EXP&",3)==0 )
		{
		p = b_list[0];
		q = p->b_start;
		while (q != NULL)	/* got one, search for T_DECLARE */
			{
			quad_bl = q->line;
			read_everything();
			oldq = q;
			if ((optype == T_DECLARE || optype == T_ENTRY) &
			   (strcmp(carg1,st[i].s_sym)==0))
				{
				printf("VARCLEANUP: killing %s\n",carg1);
				kill_line(0,q);
				}
			q = oldq->q_n;
			}
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	copy_prop()-
		Does copy propagation at the overall level (checks the
		conditions and then calls the routines to change the
		code.  

		This routine and common sub-elim are done repetitively
		until no changes occured.
*/
/*---------------------------------------------------------------------------*/
copy_prop()
{
int i,k,j,u,b,all,notzero;		/* loops and flags */
struct quad *q,*uq;			/* line ptrs */
BITS du,c_in;				/* temp bit vars */
struct basic_b *p;			/* block ptr */

for (i=1; i<c_count; i++)		/* loop through all copies */
	{
	all = 1;
	notzero = 0;			/* loop through all copies at pnt */
	for (k=0; k<B_F; k++)
	  for (j=1; j<=32; j++)
  		{ 
		q = ct[i].c_ptr;
		quad_bl = q->line;
		copy_bits(q->q_du,du);
		u = k*32+j;
		if (check_bit(du,u))	/* is it used here? */
			{
			notzero = 1;			/* did at least one */
			uq = lt[u].l_ptr;
			copy_bits(uq->q_copy,c_in);
			if  (! check_bit(c_in,i))
				all = 0;		/* cant change it */
			}
		}
	if (all && notzero)			/* if all uses ok and >=1 */
		{
		quad_bl = ct[i].c_line;
		read_everything();		/* read everything */
		get_definition();
		get_expression();
		strcpy(move_var,earg1);
		strcpy(exp_var,ct[i].c_sym2);
		printf("COPYPROP: line %d\n",ct[i].c_line);
		delete_line(i);			/* KILL THE orig COPY! */
		for (k=0; k<B_F; k++)		/* go change all uses */
	  	  for (j=1; j<=32; j++)
  		    { 
		    q = ct[i].c_ptr;
		    quad_bl = q->line;
		    copy_bits(q->q_du,du);
		    u = k*32+j;
		    if (check_bit(du,u))
			{
			uq = lt[u].l_ptr;
			copy_bits(uq->q_copy,c_in);
			if  (check_bit(c_in,i))
				{
				printf("     ->to line %d\n",uq->line);
				replace_copy(uq);	/* FIX THE CODE!! */
				}
			}
		    }		
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	replace_copy(q)-
		Change this usage with the operand of the copy so
		we can eliminate a copy.

		(q) is the line that we want to change.
*/
/*---------------------------------------------------------------------------*/
replace_copy(q)	
struct quad *q;
{				/* REPRINT INTO THE quads[] AREA */
quad_bl = q->line;
read_everything();		/* read everything */
get_definition();
get_expression();
printf("   BEFORE:(%10d)%s\n",quad_bl,quads_bl[quad_bl]);
if (expops == 1)
   {
   if (num_ops == 2) sprintf(quads_bl[quad_bl],"%15s %1x %s %s"
	,code_bl,command_bl,move_var,carg2);
   else if (num_ops == 3) sprintf(quads_bl[quad_bl],"%15s %1x %s %s %s"
	,code_bl,command_bl,move_var,carg2,carg3);
   else if (num_ops == 4) sprintf(quads_bl[quad_bl],"%15s %1x %s %s %s %s"
	,code_bl,command_bl,move_var,carg2,carg3,carg4);
   }  
else if (strcmp(exp_var,earg1)==0)
   sprintf(quads_bl[quad_bl],"%15s %1x %s %s %s"
	,code_bl,command_bl,move_var,carg2,carg3);
else
   sprintf(quads_bl[quad_bl],"%15s %1x %s %s %s"
	,code_bl,command_bl,carg1,move_var,carg3);
printf("   AFTER :(%10d)%s\n",quad_bl,quads_bl[quad_bl]);
}


/*---------------------------------------------------------------------------*/
/*	delete_line(i)-
		Actually a special case for deleting a copy statement
		inside of copy propagation.  I is the copy statement
		to delete.
*/
/*---------------------------------------------------------------------------*/
delete_line(i)
int i;
{int b;
struct quad *q;
b = ct[i].c_block;		/* get info from copy table and kill it */
q = ct[i].c_ptr;
kill_line(b,q);}


/*---------------------------------------------------------------------------*/
/*	kill_line(b,q)-
		Erases a line from the quads area (and unlinks in from
		the basic blocks)

		b - is the block this quad is in
		q - is the pointer to the quad to erase
*/
/*---------------------------------------------------------------------------*/
kill_line(b,q)
int b;
struct quad *q;
{
struct basic_b *p;
struct quad *bq,*t;

p = b_list[b];				/* get ptr to basic block */
bq = p->b_start;			/* get start quad for this block */
if (q != bq)
	{
	q->q_p->q_n = q->q_n;		/* general case unlink */
	t = q->q_n;
	if (t != NULL)
		q->q_n->q_p = q->q_p;
	}
else
	{				/* unlink if first line of block */
	t = q->q_n;
	if (t == NULL)
   		sprintf(quads_bl[q->line],"");
	else
		{
		p->b_start = q->q_n;
		q->q_n->q_p = q->q_p;
		}	
	}
}


/*---------------------------------------------------------------------------*/
/*	new_exp_var()-
		This routine creates a new subexpression elimination
		variable EXP&n where (n) is an integer.  This
		variable is given a T_DECLARE and a T_ENTRY at
		the start of the code.
*/
/*---------------------------------------------------------------------------*/
new_exp_var()
{
struct quad *w;					/* line of symbol */
int i,flag;

sprintf(exp_var,"EXP&%1d$",exp_varno++);	/* new var in sym table */
w = st[1].s_ptr;
for (i=1; i<s_count; i++)			/* go add it into code */
	{
	if (strchr(st[i].s_sym,'$')!=NULL && strchr(move_var,'$')!=NULL ||
	   strchr(st[i].s_sym,'$')==NULL && strchr(move_var,'$')==NULL)
		flag = 1;
	else
		flag = 0;
	if (((strncmp(st[i].s_sym,move_var,strlen(st[i].s_sym))==0 &&
	  strlen(st[i].s_sym) != strlen(move_var)) ||
	  strcmp(st[i].s_sym,move_var)==0) & flag)
		{
		sprintf(buffer_bl,"%15s %4x %s" ,"entry_",0x6f00 ,exp_var);
		add_line(w->q_n);
		sprintf(buffer_bl,"%15s %2x%1d%1d %s %d %d %d %d"
			,"decl_",0x61,st[i].s_mode1,st[i].s_mode2 ,exp_var
			,st[i].s_s1, st[i].s_s2, st[i].s_s3, st[i].s_s4);
		add_line(w->q_n);
		}
	}
}


/*---------------------------------------------------------------------------*/
/*	add_line(w)-
		Add a new quad or a new line to the code.  Take buffer_bl
		and set pointer in quad[] array to the buffer.

		(w) is where to add the line into (the line ptr, make
		the new line afterwards 
*/
/*---------------------------------------------------------------------------*/
add_line(w)
struct quad *w;			/* where to add new line after */
{
struct quad *p,*t;

source_bl = INPUT;		/* get in from input */
op_stack_buf();			/* add into quads */
source_bl = STACK;		/* source back to stack */
p = qalloc();			/* create line link node */
reset_bits(p->q_ud1);
reset_bits(p->q_ud2);		/* clear all the bits */
reset_bits(p->q_du);
reset_bits(p->q_exp);
reset_bits(p->q_copy);
p->q_invar = 0;
p->q_n = w->q_n;		/* link into block */
p->q_p = w;
p->line = toqs_bl-1;
t = w->q_n;
w->q_n = p;
t->q_p = p;
}


/*---------------------------------------------------------------------------*/
/*	change_avail(a)-
		Change avail expressions copy compute expression
		to temp variable and then copy this variable into
		whereever it was going anyway.

		(a) a is the expression number from the avail
		expression table to change.
*/
/*---------------------------------------------------------------------------*/
change_avail(a)				/* a is the exp to change */
unsigned int a;
{
struct quad *w;

printf("ALTER AVAIL EXP: u:= y+z AT(%d)  ",a);
printf(" on line (%d)\n",et[a].e_line);

quad_bl = et[a].e_line;			/* get info */
read_everything();
get_definition();			/* re-write the expression */
get_expression();
sprintf(quads_bl[quad_bl],"%15s %1x %s %s %s"
	,code_bl,command_bl,carg1,carg2,exp_var);
w = et[a].e_ptr;
if (m2==T_PARALLEL)					/* make new copy */
	sprintf(buffer_bl,"%15s %1x%1d%1d %s %s"
		,"mvpa_",0x48,m1,m2
		,exp_var,carg3);
else
	sprintf(buffer_bl,"%15s %1x%1d%1d %s %s"
		,"mvsc_",0x47,m1,m2
		,exp_var,carg3);
add_line(w);
}


/*---------------------------------------------------------------------------*/
/*	change_common(exp_num)-
		Change the the USE of the sub-expression results
		for the new temp variable.
*/
/*---------------------------------------------------------------------------*/
change_common(exp_num)
unsigned int exp_num;
{
int ch_line;

printf("ALTER COMMON EXP: x:=y;  line (%d)  ",et[exp_num].e_line);
printf("  VAR=%s\n",exp_var);
						/* rewrite the line */
ch_line = et[exp_num].e_line;
if (m2==T_PARALLEL)
	sprintf(quads_bl[ch_line],"%15s %1x%1d%1d %s %s"
		,"mvpa_",0x48,m1,m2
		,exp_var,move_var);
else
	sprintf(quads_bl[ch_line],"%15s %1x%1d%1d %s %s"
		,"mvsc_",0x47,m1,m2
		,exp_var,move_var);
change_exp = 1;
et[exp_num].e_dead = 1;				/* this expression is dead */
}


/*---------------------------------------------------------------------------*/
/*	search_exp(b,a,expnum)-
		Searches backwards from a block recursively for an available
		sub expression (or first instances of).

		b - is the block to start from
		a - is the number of the expression
		expnum - is the expression number of the use of a
*/
/*---------------------------------------------------------------------------*/
search_exp(b,a,expnum)
unsigned int b,a,expnum;
{
int ret;				/* return var */
struct basic_b *p,*m;			/* need for scanning blocks */
struct cntl *c;				/* go through block links */
struct quad *q,*old;			/* for scanning lines */

if ( !check_bit(hit_blocks,b+1) )
  {
  set_bit(hit_blocks,b+1,1);
  p = b_list[b];			/* get the block pointer */
  c = p->b_pred;
  while (c != NULL)
     {
     if ( !check_bit(hit_blocks,c->c_bb+1) )
	{
	m = b_list[c->c_bb];
	q = m->b_start;			/* get the quad start */
	old = q;
	while (q != NULL)
		{old = q;
		q = q->q_n;}
	q = old;
	ret = scan_back(a,q);
	if (ret == 1)			/* FOUND IT, go change the code */
		{
  		set_bit(hit_blocks,c->c_bb+1,1);
		new_exp_var();
		change_avail(a);
		change_common(expnum);
		}
	else if (ret == 2)		/* found but cant use, mark as hit */
  		set_bit(hit_blocks,c->c_bb+1,0);
	else				/* keep going! */
		search_exp(c->c_bb,a,expnum);
	}
     c = c->c_n;			/* go to next link in loop */
     }
  }
}


/*---------------------------------------------------------------------------*/
/*	int scan_back(a,q)-
		Scans backwards in a block starting at quad pointer (q)
		looking for expression (a). if founnd (of an avail
		expression) returns 1, if found on an unavail expression
		then 2 and not found returns 0.
*/
/*---------------------------------------------------------------------------*/
int scan_back(a,q)
int a;
struct quad *q;
{
while (q != NULL)		/* loop back until top quad */
	{
	quad_bl = q->line;
	read_everything();
	get_definition();
	get_expression();
	if (q->line == et[a].e_line)	/* is it the expression? */
		return(1);		/* yes!  = 1 */
	else if (eq_exp(a))		/* no but is the same, wait avail */
		return(2);
	q = q->q_p;			/* nope, keep going */
	}
return(0);
}


/*---------------------------------------------------------------------------*/
/*	eq_exp(j)-
		check to see of expression number j is the same
		as what is in the currently read expression
			earg1 earg2 and optype
		if so return 1 else 0
*/
/*---------------------------------------------------------------------------*/
eq_exp(j)
unsigned int j;
{
if (((strcmp(earg1,et[j].e_sym1)==0 && 
   strcmp(earg2,et[j].e_sym2)==0) ||
   (strcmp(earg1,et[j].e_sym2)==0 && 
   strcmp(earg2,et[j].e_sym1)==0)) && 
   strcmp(earg1,darg)!=0 &&
   strcmp(earg2,darg)!=0 &&
   et[j].e_optype == optype &&
   ! norev_exp(optype))				/* check for - / cases */
	return(1);
else if (strcmp(earg1,et[j].e_sym1)==0 && 
   strcmp(earg2,et[j].e_sym2)==0 &&
   strcmp(earg1,darg)!=0 &&
   strcmp(earg2,darg)!=0 &&
   et[j].e_optype == optype &&
   norev_exp(optype))
	return(1);
else
	return(0);
}


/*---------------------------------------------------------------------------*/
/*	int norev_exp(op)-
		Given an optype, this function returns true if
		the optype is a nonreversible expression...such AS...
		subtraction, div, < <= > >=
*/
/*---------------------------------------------------------------------------*/
int norev_exp(op)
unsigned int op;
{
switch(op)
	{case	T_SUB	:
	case	T_DIV	:
	case	T_LT	: 
	case	T_LE	:
	case	T_GT	:
	case	T_GE	:
		return(TRUE);
	}
return(FALSE);
}


/*	END OF THIS SHTUFF	*/

