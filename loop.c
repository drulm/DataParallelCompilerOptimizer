
/*---------------------------------------------------------------------*/
/*	CODE MOTION SUBROUTINES--
		All of this code relates to code motion.
		it finds back edges, loops, exits, invariant-
		code, and does tests to do code motion.

*/
/*---------------------------------------------------------------------*/
#include "op_common.h"
#define MAX_LOOPS 100			/* no more than 100 loops */
#define MAX_NODES_LOOP 100		/* no more than 100 nodes in loops */


/*---------------------------------------------------------------------*/
/*	THESE ARE STRUCTS AVAIL ONLY TO CODE MOTION 
*/
/*---------------------------------------------------------------------*/
struct inv {				/* saves info about invariant code */
	int             i_loop;		/* loop code is in */
	struct quad     *i_quad;};	/* line that is invariant */

struct hd {				/* loop head info */
	int		h_loop;		/* loop the head exists in */
	int		h_block;};	/* block of head */


/*---------------------------------------------------------------------*/
/*	GLOBAL DATA AREA FOR FINDING LOOP INVAR CODE INFORMATION
*/
/*---------------------------------------------------------------------*/
BITS loop[MAX_LOOPS+1];			/* saves bits for blocks in loops */
int looptops[MAX_LOOPS+1];		/* saves loop top blocks */
int loopback[MAX_LOOPS+1];		/* saves loop back edge blocks */
struct hd heads[MAX_LOOPS+1];		/* heads for all loops */
BITS exits[MAX_LOOPS+1];		/* exit bit blocks for all loops */
int lpcnt;				/* number of loops */
int stack[MAX_NODES_LOOP+1];		/* stack used to find loops */
int top;				/* top of stack */
int headcnt;				/* number of heads */
struct inv invar[Q_STACK_SIZE+1];	/* invariant code array */
int itop;				/* number of invariant lines */


/*---------------------------------------------------------------------*/
/*	code_motion()-
		Performs code motion on MACRO scale, does tests
		and then calls routines to move the code 
*/
/*---------------------------------------------------------------------*/
code_motion()
{
int h,i,j,lp,defno,inbl;	/* loop and temp vars */
int test1,test2,test3;		/* flags for the 3 TESTS FOR MOTION */
BITS dom_exit,uses;		/* dominators of exits, and uses copy */
struct basic_b *p;		/* block ptr */
struct quad *q;			/* temp quad ptr */

headcnt = 0;			/* we have NO HEADS! */
for (i=1; i<=itop; i++)
  {
  lp = invar[i].i_loop;		/* get loop number of invariant line */
  q = invar[i].i_quad;		/* get quad ptr of invariant line */
  quad_bl = q->line;
  read_everything();
  get_definition();		/* read this line! */
  get_expression();
				/* DO TEST #2 --no other definitions */
  test2 = TRUE;
  for (j=1; j<d_count; j++)		/* scan all defs */
	{
	if (strcmp(darg,dt[j].d_sym)==0 && quad_bl != dt[j].d_line &&
	  check_bit(loop[lp],dt[j].d_block+1)==1)
		test2 = FALSE;
	if (dt[j].d_line  == quad_bl) defno = j; 
	}
 
				/* DO TEST #1 --exits dominated */
  test1 = FALSE;
  inbl = dt[defno].d_block;
  and_bits(b_list[inbl]->b_dom2,exits[lp],dom_exit);
  if (comp_bits(dom_exit,exits[lp])==0)  test1 = TRUE;

				/* DO TEST #3 --all uses are of this def */
  test3 = TRUE;
  for (j=1; j<l_count; j++)		/* scan all uses */
	{
	if (strcmp(darg,lt[j].l_sym)==0 && 
	  check_bit(loop[lp],lt[j].l_block+1)==1)
		{
		reset_bits(dom_exit);
		copy_bits(lt[j].l_ptr->q_ud1,uses);
		set_bit(uses,defno,0);
		if (comp_bits(uses,dom_exit) != 0) test3 = FALSE;
		}
	if (strcmp(darg,lt[j].l_sym)==0 &&
	  check_bit(loop[lp],lt[j].l_block+1)==1)
		{
		reset_bits(dom_exit);
		copy_bits(lt[j].l_ptr->q_ud2,uses);
		set_bit(uses,defno,0);
		if (comp_bits(uses,dom_exit) != 0) test3 = FALSE;
		}
	}
  if (test1 && test2 && test3)			/* if all tests pass */
	{					/* MOVE THE CODE! */
	printf("CODE MOT: line %d is moved to header before %d from %d\n"
		,quad_bl,looptops[lp],inbl);
 	h = create_preheader(lp);		/* create preheader */
	move_preheader(q,h,inbl);		/* move code into header */
	}
  }
link_up_heads();			/* link of the preheaders */
}


/*---------------------------------------------------------------------*/
/*	link_up_heads()-
		This code adjusts all links that should point to
		the pre-header (not the back edge) and fixes all
		the old links that used to point to the header of
		the loop.
*/
/*---------------------------------------------------------------------*/
link_up_heads()
{
int i;
int lphd;				/* loop head */
struct cntl *c;				/* for scanning block links */

for (i=1; i<=headcnt; i++)		/* go through all loop heads */
	{
	lphd = looptops[ heads[i].h_loop ];	/* get loop head */
	c = b_list[lphd]->b_pred;		/* get preds of loop */
	while (c != NULL)			/* link up pre-header */
		{
		if (loopback[heads[i].h_loop] != c->c_bb)
			{
			insert_goto(c->c_bb, heads[i].h_block);
			remove_goto(c->c_bb, lphd);
			}
		c = c->c_n;			/* go next link */
		}				/* 1 link from pre-head->head */
	insert_goto(heads[i].h_block, looptops[ heads[i].h_loop]);
	}
}


/*---------------------------------------------------------------------*/
/*	remove_goto(f,t)
		Removes link from block (f) to block (t) if it exists
		(it better!)...unlinks both control blocks in pred
		and succ node
*/
/*---------------------------------------------------------------------*/
remove_goto(f,t)
int f,t;
{
int found;			/* found the control node */
struct cntl *c;			/* to scan control nodes */
struct basic_b *b;

b = b_list[f];			/* get from block */
c = b->b_succ;			/* get succ of that block */
found = 0;
while (c != NULL && found==0)	/* find the TO cntrl block */
	{
	if (c->c_bb == t) found = 1;
		else c = c->c_n;
	}
if (found)			/* if found delete that sucker */
  {
  if (b->b_succ == c)		/* is it first cntrl node for block? */
	if (c->c_n == NULL)
		b->b_succ = NULL;		/* YES */
	else
		{
		c->c_n->c_p = NULL;
		b->b_succ = c->c_n;
		}
  else
	{
	if (c->c_n != NULL)			/* NO */
		{
		c->c_n->c_p = c->c_p;
		}
	c->c_p->c_n = c->c_n;
	}
  }

b = b_list[t];				/* now do same for to block */
c = b->b_pred;				/* get preds of to */
found = 0;
while (c != NULL && !found)		/* find the from block in cntl */
	{
	if (c->c_bb == f) found = 1;
	else c = c->c_n;
	}
if (found)				/* found it */
  if (b->b_succ == c)
	if (c->c_n == NULL)		/* case of 1st node off block */
		b->b_succ = NULL;
	else
		{
		c->c_n->c_p = NULL;
		b->b_succ = c->c_n;
		}
  else
	{				/* not first node off block */
	if (c->c_n != NULL)
		c->c_n->c_p = c->c_p;
	c->c_p->c_n = c->c_n;
	}
}


/*---------------------------------------------------------------------*/
/*	int create_preheader(lp)-
		Creates the preheader if there doesnt already
		exit one for this loop (lp)
*/
/*---------------------------------------------------------------------*/
int create_preheader(lp)
int lp;
{
int i,found;

found = 0;
i = 1;
while (i<=headcnt && found==0)		/* is there already a preheader lp */
	if (heads[i].h_loop == lp)
		found = 1;
	else 
		i++;
if ( found==0 )				/* NO, better make a new one */
	{
	headcnt++;				/* one more head */
	heads[headcnt].h_loop = lp;		
	heads[headcnt].h_block = b_count;
	b_curr = (int)balloc();			/* create new block node */
	b_list[b_count-1] = (int)b_curr;
	return(headcnt);			/* return the new head # */
	}
else return(i);			/* return existing head number */
}


/*---------------------------------------------------------------------*/
/*	move_preheader(q,h,oldb)-
		Moves line (q) into pre-header (h) from block (b).
		Handles unlinking this line out of the old block.
*/
/*---------------------------------------------------------------------*/
move_preheader(q,h,oldb)
struct quad *q;
int h;
struct basic_b *oldb;
{
struct quad *p,*old;                 	/* new quad node */
struct basic_b *b;

kill_line(oldb,q);			/* unlink this line! */

b = b_list[ heads[h].h_block ];
p = b->b_start;
old = p;
while (p != NULL)			/* go to end of prehead block */
	{old = p;
	p = p->q_n;}
p = old;
if (p == NULL)             /* this is the FIRST node on basic block */
	{
	q->q_p = NULL;
	b->b_start = q;
	}
else
	{                       /* this is NOT the FIRST node on the block */
	q->q_p = p;
	p->q_n = q;
	}
q->q_n = NULL;
b_curr->count++;                /* update count of lines in block */
}


/*---------------------------------------------------------------------*/
/*	find_exits()-
		Finds all exits for ALL loops and stores this
		info as an array of bitfields.
*/
/*---------------------------------------------------------------------*/
find_exits()
{
int j,i,k,b;			/* loops and temps */
struct basic_b *p;
struct cntl *c;

for (i=1; i<=lpcnt; i++)		/* for all loops do */
  {
  reset_bits(exits[i]);			/* reset exit bit info */
  for (k=0; k<B_F; k++)
    for (j=1; j<=32; j++)		/* loop over loop[] bits */
	{ 
	b = k*32+j;
	if (check_bit(loop[i],b)==1)	/* if block in loop */
		{
		p = b_list[b-1];
		c = p->b_succ;		/* see if succ outside of loop */
		while (c != NULL) 
			{
			if (check_bit(loop[i],(c->c_bb)+1)==0)
				set_bit(exits[i],b,1);
			c = c->c_n;
			}
		}
	}
  printf("  EXITS are for loop %d ->",i); display_loop_bits(exits[i]);
  }
}


/*---------------------------------------------------------------------*/
/*	mark_invariant()-
		Check the code in the blocks to look for code which
		is invariant, that is all reaching definitions
		for the code uses are outside of this block.
*/
/*---------------------------------------------------------------------*/
mark_invariant()
{
int i,j,k,u,b,mark,invar_change;	/* lotsa loops, and 2 flags */
struct quad *q;				/* scan through code */
struct basic_b *p;			/* scan through blocks */
BITS udbits1,udbits2;			/* use-def bits */

itop = 0;
invar_change = TRUE;			/* invariant code was found */
while (invar_change)			/* while invariant code still found */
 {
 invar_change = FALSE;
 for (i=1; i<=lpcnt; i++)		/* for all loops */
   for (b=0; b<b_count; b++)
     if (check_bit(loop[i],b+1)==1)	/* do blocks in loops only!!!!! */
	{
	p = b_list[b];
	q = p->b_start;
	while (q != NULL)		/* scan through quads in loops */
		{
		quad_bl = q->line;
		copy_bits(q->q_ud1,udbits1);
		copy_bits(q->q_ud2,udbits2);
		read_everything();
		get_expression();
		get_definition();		/* check invariance-uga */
		mark = 0;
		if (definition() && expops != 0 && q->q_invar == 0
		  && !target() && !branch())
			{
			mark = 1;
			for (k=0; k<B_F; k++)
			  for (j=1; j<=32; j++)
			    {
			    u = k*32+j;
			    if (check_bit(udbits1,u)==1)
				if (check_bit(loop[i],dt[u].d_block+1)==1
					&& dt[u].d_ptr->q_invar == 0)
			   	   mark = 0;
			    if (expops == 2)
			      if (check_bit(udbits2,u)==1)
				if (check_bit(loop[i],dt[u].d_block+1)==1
					&& dt[u].d_ptr->q_invar == 0)
			   	   mark = 0; 
			    }
			} 
		if (mark) 			/* must be invariant line */
			{
			invar_change = TRUE;
			printf("INVAR: line %d invariant\n",q->line);
			itop++;
			invar[itop].i_loop = i;		/* MARK IT INVARIANT */
			invar[itop].i_quad = q;
			q->q_invar = TRUE;
			}	
		q = q->q_n; 			/* do next line */
		}
	}
 }
}


/*---------------------------------------------------------------------*/
/*	find_loops()-
		Finds all loops in code by looking at link information
		and stores the bits for the blocks in an array of
		bits (the array handles all the loops).
*/
/*---------------------------------------------------------------------*/
find_loops()
{
int i;
struct basic_b *p;
struct cntl *c;

lpcnt = 0;				/* no loops found yet */
for (i=0; i<b_count; i++)		/* loop through blocks */
	{
	p = b_list[i];
	c = p->b_succ;
	while (c != NULL) 
		{			/* look for back edge */
		p = b_list[i];
		if (check_bit(p->b_dom,c->c_bb + 1)==1)
			get_loop(i,c->c_bb);		/* got loop, fill */
		c = c->c_n;
		}
	}
}


/*---------------------------------------------------------------------*/
/*	get_loop(n,d)-
		Fill the loop bits by backscanning through the
		nodes.  (n) is the loop header, (d) is the block
		that contains a back edge.
*/
/*---------------------------------------------------------------------*/
get_loop(n,d)
int n,d;
{
int m;
struct basic_b *p;
struct cntl *c;

lpcnt++;
printf("LOOP: loop found %d to %d\n",n,d);
top = 0;
looptops[lpcnt] = d;			/* save loop heads */
loopback[lpcnt] = n;			/* save back edge node */
reset_bits(loop[lpcnt]);		/* reset loop info */
set_bit(loop[lpcnt],d+1,1);		/* set head node as IN loop */
insert(n);				/* start inserting at back node */
while (top != 0)			/* when stack is empty */
	{
	top--;				/* pop stack */
	m = stack[top];			/* get top of stack */
	p = b_list[m];			/* scan through block pred links */
	c = p->b_pred;
	while (c != NULL) 
		{
		insert(c->c_bb);	/* insert back nodes into loop */
		c = c->c_n;
		}
	}
printf("   LOOP: CONTAINS!->"); display_loop_bits(loop[lpcnt]);
}

/*---------------------------------------------------------------------*/
/*	insert(m)-
		inserts block m into loop and pushes the m node
		onto the stack to its predecesors will be looked
		at
*/
/*---------------------------------------------------------------------*/
insert(m)
int m;
{
if (check_bit(loop[lpcnt],m+1,1)==0)	/* if not already in loop */
	{
	set_bit(loop[lpcnt],m+1,1);	/* add to loop info */
	stack[top] = m;			/* push m onto stack */
	top++;
	}
}


/*---------------------------------------------------------------------*/
/*	find_dom()-
		For each basic_blockfind all nodes that dominate it
		and all nodes that its dominates.
*/
/*---------------------------------------------------------------------*/
find_dom()
{
int i,j,passed;
BITS d,covered;
struct basic_b *p,*in,*out;

for (i=0; i<b_count; i++)		/* for all blocks */
   {
   reset_bits(d);
   for (j=0; j<b_count; j++)		/* compare to all blocks */
	{ 
	dominate = TRUE;
	passed = FALSE;
	reset_bits(covered);
	search_dom(j,i,passed,covered);		/* j dom i? */
	if (dominate) set_bit(d,j+1,1);		/* YES, set bit */
	}
   p = b_list[i];
   copy_bits(d,p->b_dom);
   }
for (i=0; i<b_count; i++)		/* re-write info for DOMINATES */
   {
   out = b_list[i];
   reset_bits(d);
   for (j=0; j<b_count; j++)
	{
	in = b_list[j];
	copy_bits(in->b_dom,covered);
	if (check_bit(covered,i+1)==1)	/* if i dom j then set the bit */
		set_bit(d,j+1,1);
	}
   copy_bits(d,out->b_dom2);
   printf("DOM:  block %d dominates->",i); display_loop_bits(d);
   }	
}


/*---------------------------------------------------------------------*/
/*	search_dom(m,n,passed,covered)
		See if block m dominates block n and passed = 1 if
		the dominator node was passed en-route to initial
		node.  Covered it a bit field telling what blocks
		have already been passed.
*/
/*---------------------------------------------------------------------*/
search_dom(m,n,passed,covered)			/* m dom n */
int m,n; 
int passed;
BITS covered;
{
struct cntl *c;
struct basic_b *p;
BITS cover_copy;

if (n == m) passed = TRUE;			/* we got to dom node */
if (n == 0 && !passed) dominate = FALSE;	/* got to root, no dom */
if (n != 0 && check_bit(covered,n+1)==0)	/* still looking */
	{
	copy_bits(covered,cover_copy);		/* make copy of covered */
	set_bit(cover_copy,n+1,1);		/* cover current node */
	p = b_list[n];
	c = p->b_pred;				/* scan through preds */
	while (c != NULL) 
		{
		search_dom(m,c->c_bb,passed,cover_copy);
		c = c->c_n;
		}
	}
}

/* 	DONE WITH LOOP CODE 	*/


