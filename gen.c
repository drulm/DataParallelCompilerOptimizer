
/*-------------------------------------------------------------------------*/
/*			CODE GEN ROUTINES 

			Implements GetReg
*/
/*-------------------------------------------------------------------------*/
#include "op_common.h"
#define MAXREG 1

BITS rd[MAXREG+1];			/* register descriptors */
BITS zero;				/* bits all 0 */
int L,yloc,xloc,zloc,yp,zp;		/* locations of x,y,z y' z' L */


/*-------------------------------------------------------------------------*/
/*	code_gen()
		This is the heart of it, it is called after the last
		of the optimizations and this implements the codegen
		function described on page 537.

		Yes, it is nasty code, but I had to do it in <10 hours
		in order to study.
*/
/*-------------------------------------------------------------------------*/
code_gen()
{
int i,j,dflag;			/* loop */
struct quad *q;			/* check each line */
struct basic_b *p;		/* check all blocks */

printf("\n\n	CODE GEN\n-----------------------\n\n");
reset_bits(zero);
for (i=0; i<b_count; i++)		/* for all blocks */
	{
	printf("\n######################### BLOCK %d ####################\n",i);
	clear_rd();			/* clear register descriptors */
	p = b_list[i];			/* get the block */
	q = p->b_start;			/* get the 1st quad */
	dflag = 0;			/* havent found a branch */
	while (q != NULL)		/* until out of quads */
		{
		quad_bl = q->line;	/* get all info in the quad */
		read_everything();
		get_definition();
		get_expression();
		if (!dflag && branch())		/* store live regs of branch */
			{
			dflag = 1;
			store_live_regs(p);
			}
		xloc = find_var(darg);		/* get locs of variables */
		yloc = find_var(earg1);
		zloc = find_var(earg2);

						/* X = Y op Z generation */

		if (definition() && !branch() && !target() && expops==2)
			{
	                printf("* (%10d)%s\n",quad_bl,quads_bl[quad_bl]);
			L = getreg(q);
			if (vt[yloc].v_reg != -1) yp=vt[yloc].v_reg;
				else yp = -yloc;

					/* copy y' into L */
			if (L != yp)
			  { print_leader();
			  if (mode2 == T_PARALLEL) printf("%8s ","MVPA_");
			  	else printf("%8s ","MVSC_");
			  if (yp<0) printf("%s , ",vt[yloc].v_sym);
			  	  else printf("R%1d , ",yp);
			  if (L<0) printf("%s\n",vt[xloc].v_sym);
			  	  else printf("R%1d\n",L); }

					/* generate (op) z' L */

			if (vt[zloc].v_reg != -1) zp=vt[zloc].v_reg;
				else zp = -zloc;
			print_leader();	printf("%8s ",code_bl);
			if (zp<0) printf("%s , ",vt[zloc].v_sym);
				else printf("R%1d , ",zp);
			if (L<0) printf("%s\n",vt[xloc].v_sym);
			  	else printf("R%1d\n",L);

					/* set new reg and address descs. */

			if (L<0) {vt[xloc].v_mem= xloc; vt[xloc].v_reg = -1; }
			   else 
			   {
			   vt[xloc].v_mem = 0; vt[xloc].v_reg = L;
			   for (j=0; j<=MAXREG; j++) set_bit(rd[j],xloc,0);
			   reset_bits(rd[L]);
			   set_bit(rd[L],xloc,1);
			   }
			if (comp_bits(zero,q->y_next)==0)
			   for (j=0; j<=MAXREG; j++) set_bit(rd[j],yloc,0);
			if (comp_bits(zero,q->z_next)==0)
			   for (j=0; j<=MAXREG; j++) set_bit(rd[j],zloc,0);
			}

					/* case for a MV or a COPY */

		else if (definition() && !branch() && !target() && expops==1)
			{
	                printf("* (%10d)%s\n",quad_bl,quads_bl[quad_bl]);
			if (vt[yloc].v_reg != -1) yp=vt[yloc].v_reg;
				else yp = -yloc;

					/* if copy into y' is a register */
					/* just update reg. desc. */
			if (yp>=0)
			    { 
			    vt[xloc].v_mem = 0;
			    vt[xloc].v_reg = yp;
			    for (j=0; j<=MAXREG; j++) set_bit(rd[j],xloc,0);
			    set_bit(rd[yp],xloc,1);
			    }
			else		/* else call GETREG and actually */
					/* do the MV y' into L */
			    {
			    L = getreg(q);
			    print_leader();
			    if (mode2 == T_PARALLEL) printf("%8s ","MVPA_");
				else printf("%8s ","MVSC_");
			    if (yp<0) printf("%s , ",vt[yloc].v_sym);
			  	  else printf("R%1d , ",yp);
			    if (L<0) printf("%s\n",vt[xloc].v_sym);
			  	  else printf("R%1d\n",L);

			    if (L>=0)	/* adjust descriptors for MV */
			      {
			      vt[xloc].v_mem = 0;
			      vt[xloc].v_reg = L;
			      for (j=0; j<=MAXREG; j++) set_bit(rd[j],xloc,0);
			      set_bit(rd[L],xloc,1);
			      } 
			    }		/* adjust reg. decs for yloc */

			if (comp_bits(q->y_next,zero)==0)
			    {
			    vt[yloc].v_mem = yloc;
			    vt[yloc].v_reg = -1;
			    set_bit(rd[yp],yloc,0);
			    }
			}
		else printf("  (%10d)%s\n",quad_bl,quads_bl[quad_bl]);
		q = q->q_n;
		}
	if (!dflag) store_live_regs(p);	/* store live values in regs */
	}
}


/*-------------------------------------------------------------------------*/
/*	store_live_regs(p)
		This function takes a block descriptor and stores all
		values in register descriptors which are live on exit
		of the block
*/
/*-------------------------------------------------------------------------*/
store_live_regs(p)
struct basic_b *p;
{
int i,j,k,x,y,a,b,live;			/* loops believe it or not */
BITS lv;
copy_bits(p->b_l_out,lv);		/* save live out bits */
for (i=0; i<=MAXREG; i++)		/* for all register desc. */
	if (comp_bits(zero,rd[i])!=0)
	  for (k=0; k<B_F; k++)
	    for (j=1; j<=32; j++)	/* check for values in rd's */
	  	{
		live = 0;
		a = k*32+j;
		if (check_bit(rd[i],a))
	  	  for (x=0; x<B_F; x++)
	    	    for (y=1; y<=32; y++)	/* check for liveness */
	  		{
			b = x*32+y;
			if (check_bit(lv,b) &&
				strcmp(vt[a].v_sym,lt[b].l_sym)==0) live=1;
			}
		if (live)			/* generate the copies */
			{
			print_leader();
			if (mode2 == T_PARALLEL) printf("%8s ","MVPA_");
				else printf("%8s ","MVSC_");
			printf("R%1d , %s\n",i,vt[a].v_sym);
			vt[a].v_mem = a;
			vt[a].v_reg = i;
			}
		}
}


/*-------------------------------------------------------------------------*/
/*	int find_var(s)
		This one takes a string and searches the use-def variable
		table for a match and returns a 0 if not found or the 
		index of found.
*/
/*-------------------------------------------------------------------------*/
int find_var(s)
char *s;
{
int i,found;
found = 0;
for (i=1; i<v_count; i++)
	if (strcmp(s,vt[i].v_sym)==0) found=i;
return(found);
}


/*-------------------------------------------------------------------------*/
/*	clear_rd()
		Clears register descriptors and address descriptors */
/*-------------------------------------------------------------------------*/
clear_rd()
{
int i;
for (i=0; i<=MAXREG; i++)
	reset_bits(rd[i]);
for (i=0; i<=v_count; i++)
	{vt[i].v_mem = i;
	vt[i].v_reg = -1;}
}


/*-------------------------------------------------------------------------*/
/*	int getreg(q)
		Returns a location to store the final results
		of the x = y op z or MV operation.  It does this
		by looking at various criteria.  q is the quadword
		pointer to consider...
*/
/*-------------------------------------------------------------------------*/
int getreg(q)
struct quad *q;
{
BITS one,usedef;
int R,k,i,j,loc,found,live,a;		/* loops and flags */

reset_bits(one);			/* TEST #1 */
set_bit(one,yloc,1);			/* find regiser holding ONE value */
found = -1;
for (i=0; i<=MAXREG && found== -1 ; i++)
	if (comp_bits(one,rd[i])==0) found = i;
live=0;
copy_bits(q->y_next,usedef);
for (k=0; k<B_F; k++)                   /* scan through avail bits */
	for (j=1; j<=32; j++)
	  	{
		a = k*32+j;
		if (check_bit(usedef,a)) live=1;
		}
if (!live && found != -1) return(found); 

					/* TEST #2 */
found = -1;				/* find empty register if exists */
for (i=0; i<=MAXREG && found== -1 ; i++)
	if (comp_bits(zero,rd[i])==0) found = i;
if (found != -1) 
	{
	return(found);
	vt[yloc].v_reg = -1;
	}
					/* TEST #3 */
					/* see if X has next use in block */
live=0;
copy_bits(q->x_buse,usedef);
for (k=0; k<B_F; k++)                   /* scan through avail bits */
	for (j=1; j<=32; j++)
	  	{
		a = k*32+j;
		if (check_bit(usedef,a)) live=1;
		}
if (live)				/* if X live, store occupied reg. */
	{
	R=find_occu_reg();
	for (k=0; k<B_F; k++)                   /* scan through avail bits */
		for (j=1; j<=32; j++)
			{	
			a = k*32+j;
			if (check_bit(rd[R],a) && vt[a].v_mem==0)
				{	
				print_leader();
				if (mode2 == T_PARALLEL) 
					printf("%8s ","MVPA_");
					else printf("%8s ","MVSC_");
				printf("R%1d , %s\n",R,vt[a].v_sym);
				vt[a].v_mem=a;
				}
			}
	return(R);				/* return new avail register */
	}
return(-xloc);				/* - xloc means the xloc variable */ 
}					/* or NO REGISTER FOUND, USE MEMORY! */


/*-------------------------------------------------------------------------*/
/*	print_leader()
		Prints a nice leader for output formatting
*/
/*-------------------------------------------------------------------------*/
print_leader()
{printf("              ");}


/*-------------------------------------------------------------------------*/
/*	find_occu_reg()
		Find an occupied register and needs to find a better
		one, for now it just returns 0....

		Gimme a break, gotta study remember?
*/
/*-------------------------------------------------------------------------*/
find_occu_reg()
{
return(0);
}



