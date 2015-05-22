
/*---------------------------------------------------------------------------
	GENERALIZED DATA FLOW EQUATION SOLVING PROGRAM
	_____________________________________________________
		Can calculate both available expressions and
		reaching definitions.  Is a single subroutine that
		can be called with different values.

			(1)	a meet operator (union, intersection)
			(2)	a direction (fwd, back)
			(3)	a transformation function (avail def, exp..)
			(4)	set of gens and kills (derive from 3)

---------------------------------------------------------------------------*/
#include "op_common.h"				/* need structs and vars */

int i,dont_do;
						/* i- to loop through blocks */
						/* dont_do- block not to incl*/
struct basic_b *b,*q;				/* b- current block */
						/* q- succ or pred blocks */
struct cntl *g;					/* holds block links */
BITS *in,*out,*gen,*kill,oldout,temp;		/* ins, outs, gens and kills */
						/* all in terms of problem! */


/*---------------------------------------------------------------------------
	data_flow(meet,direc,trans_fn)
		Generalized data flow program.
			meet-	  a meet operator (union, intersection)
			direc-	  a direction (fwd, back)
			trans_fn- set of gens and kills (derive from 3)
				  a transformation function (avail def, exp..)
---------------------------------------------------------------------------*/
data_flow(meet,direc,trans_fn)
int meet,direc,trans_fn;
{
change = TRUE;				/* assume true for 1st loop */
init_fn(trans_fn,direc);		/* depending on direction and Fn */
					/* set up the problem! */

while (change)				/* as long as outs are still changing */
	{
	change = FALSE;			/* assume no change */
	for (i=0; i<b_count; i++)	/* for each block in B do! */
	   if (i != dont_do)		/* dont do certain block */
		{			/*build the IN's */
		build_in(meet,direc,trans_fn);

					/* save oldout */
		copy_bits(*out,oldout);

					/* compute outs from ins, gens, kills */
		compute_out(meet,direc,trans_fn);

					/* if out changed then keep going! */
		if (comp_bits(*out,oldout) != 0) change = TRUE;
		}
	}
}


/*---------------------------------------------------------------------------
	build_in(meet,direc,trans_fn)
		Depending on direc, trans_fn and meet, compute the 
		IN for the block. (could be considered IN for reverse
		direction) 
			meet-	  a meet operator (union, intersection) 
			direc-	  a direction (fwd, back) 
			trans_fn- set of gens and kills (derive from 3) 
				  a transformation function (avail def, exp..)
---------------------------------------------------------------------------*/
build_in(meet,direc,trans_fn)
int meet,direc,trans_fn;
{
b = b_list[i];					/* get block */
find_bit_sets(trans_fn,b,direc);		/* get gen,kill,in,out ptrs */
if (meet == UNION) reset_bits(*in);		/* reset for UNION */
	else all_bits(*in);			/* set all for INTER */
if (direc == FORWARD) g = b->b_pred;		/* get pred or succ from dir */
	else g = b->b_succ;
if (g == NULL) reset_bits(*in);			/* if no blocks then 0's! */
while (g != NULL)				/* until no more blocks */
	{
	q = b_list[g->c_bb];			/* get pred/succ block */
	copy_bits(*in,temp);			/* save in */
	find_bit_sets(trans_fn,q,direc);	/* out for a NEW block */
	confluence(meet);			/* INTER or UNION */
	find_bit_sets(trans_fn,b,direc);	/* back to original block */
	copy_bits(temp,*in);			/* get in from temp */
	g = g->c_n;				/* go to next block */
	} 
}


/*---------------------------------------------------------------------------
	compute_out(meet,direc,trans_fn)
		Computes OUT[B] from the IN information and the
		GENS, KILLS
			meet-	  a meet operator (union, intersection) 
			direc-	  a direction (fwd, back) 
			trans_fn- set of gens and kills (derive from 3)
			a transformation function (avail def, exp..)
---------------------------------------------------------------------------*/
compute_out(meet,direc,trans_fn)
int meet,direc,trans_fn;
{
minus_bits(*in,*kill,temp);	/* out[B] = gen[B] (con) (in[B] - kill[B]) */
or_bits(*gen,temp,*out);
}


/*---------------------------------------------------------------------------
	confluence(meet)
		Takes the OR or AND of the bits in question
			meet-	  a meet operator (union, intersection) 
---------------------------------------------------------------------------*/
confluence(meet)
int meet;
{
switch (meet) {
	case UNION:
		or_bits(temp,*out,temp);
		break;
	case INTER:
		and_bits(temp,*out,temp);
		break;
	default:
		printf("\n\n**ERROR1\n\n");
	}
}


/*---------------------------------------------------------------------------
	init_fn(trans_fn,direc)
		Initializes IN's and OUT's based on trans_fn and direc.
		All this is done BEFORE the iterative algorithm loop.
			direc-	  a direction (fwd, back) 
			trans_fn- set of gens and kills (derive from 3)
---------------------------------------------------------------------------*/
init_fn(trans_fn,direc)
int trans_fn;
int direc;
{
switch (trans_fn) {
	case REACH:
		dont_do = -1;
		for (i=0; i<b_count; i++)
			{
			b = b_list[i];
			find_bit_sets(trans_fn,b,direc);
			copy_bits(*gen,*out);
			}
		break;
	case COPY:
		dont_do = 0;
		b = b_list[0];
		find_bit_sets(trans_fn,b,direc);
		reset_bits(*in);
		copy_bits(*gen,*out);
		for (i=1; i<b_count; i++)
			{
			b = b_list[i];
			find_bit_sets(trans_fn,b,direc);
			all_bits(temp);
			minus_bits(temp,*kill,*out);
			}
		break;
	case AVAIL:
		dont_do = 0;
		b = b_list[0];
		find_bit_sets(trans_fn,b,direc);
		reset_bits(*in);
		copy_bits(*gen,*out);
		for (i=1; i<b_count; i++)
			{
			b = b_list[i];
			find_bit_sets(trans_fn,b,direc);
			all_bits(temp);
			minus_bits(temp,*kill,*out);
			}
		break;
	case LIVE:
		dont_do = -1;
		for (i=0; i<b_count; i++)
			{
			b = b_list[i];
			find_bit_sets(trans_fn,b,direc);
			reset_bits(*out);
			}
		break;
	default:
		printf("\n\n**ERROR2\n\n");
	}
}


/*---------------------------------------------------------------------------
	find_bit_sets(trans_fn,b,direc)
		This is the function that makes the "generality"
		of this algorithm exist.  Given a block, this function
		sets up the pointers for gen,kill,in,out based
		on the problem we are trying to solve.  If direc
		is BACKWARDS then we can just swap the pointers for
		in and out since that is how the algorithm works
		in the reverse direction.
			direc-	  a direction (fwd, back) 
			trans_fn- set of gens and kills (derive from 3)
---------------------------------------------------------------------------*/
find_bit_sets(trans_fn,b,direc)
int trans_fn;
struct basic_b *b;
int direc;
{
BITS *t;				/* used to swap in and out */

switch (trans_fn) {
	case REACH:			/* the 3 trans functions so far */
		gen = b->b_gen;
		kill = b->b_kill;
		in = b->b_in;
		out = b->b_out;
		break;
	case COPY:
		gen = b->b_c_gen;
		kill = b->b_c_kill;
		in = b->b_c_in;
		out = b->b_c_out;
		break;
	case AVAIL:
		gen = b->b_e_gen;
		kill = b->b_e_kill;
		in = b->b_e_in;
		out = b->b_e_out;
		break;
	case LIVE:
		gen = b->b_l_gen;
		kill = b->b_l_kill;
		in = b->b_l_in;
		out = b->b_l_out;
		break;
	default:
		printf("\n\n**ERROR3\n\n");
	}
if (direc == BACK)			/* sap IN and OUT for REVERSE! */
 	{t = in;  in = out;  out = t;}
}


