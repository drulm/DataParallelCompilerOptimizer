#include "op_common.h"

char *index();   /* declare index to be a pointer type */

/* read left preamble and equivalent code */
op_fscanf()
{
int ret;

  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x",code_bl,&command_bl);
  else
    ret=fscanf(input_fp,"%15s %x",code_bl,&command_bl);
  if (ret==EOF) command_bl=EOF;
  return (command_bl);
}

/* read zero arguments */
op_scano_0()
{
num_ops = 0;
  if(source_bl == STACK) 
    /*quad_bl++*/; 
  else 
    sprintf(buffer_bl,"%15s %x",code_bl,command_bl);
}

/* read one argument */
op_scano_1()
{
num_ops = 1;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s",code_bl,&command_bl,carg1);
  else
    { fscanf(input_fp,"%s",carg1);
      sprintf(buffer_bl,"%15s %x %s",code_bl,command_bl,carg1);
    };
}

op_scano_2()
{
num_ops = 2;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s %s",code_bl,&command_bl,carg1,carg2);
  else
    { fscanf(input_fp,"%s %s",carg1,carg2);
      sprintf(buffer_bl,"%15s %x %s %s",code_bl,command_bl,carg1,carg2);
    };
}

op_scano_3()
{
num_ops = 3;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s %s %s",
           code_bl,&command_bl,carg1,carg2,carg3);
  else
    { fscanf(input_fp,"%s %s %s",carg1,carg2,carg3);
      sprintf(buffer_bl,"%15s %x %s %s %s",
              code_bl,command_bl,carg1,carg2,carg3);
    };
}

op_scano_4()
{
num_ops = 4;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s %s %s %s",
	   code_bl,&command_bl,carg1,carg2,carg3,carg4);
  else
    { fscanf(input_fp,"%s %s %s %s",carg1,carg2,carg3,carg4);
      sprintf(buffer_bl,"%15s %x %s %s %s %s",
              code_bl,command_bl,carg1,carg2,carg3,carg4);
    };
}

op_scano_5()
{
num_ops = 5;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s %s %s %s %s",
	   code_bl,&command_bl,carg1,carg2,carg3,carg4,carg5);
  else
    {fscanf(input_fp,"%s %s  %s %s %s",carg1,carg2,carg3,carg4,carg5);
     sprintf(buffer_bl,"%15s %x %s %s %s %s %s",code_bl,command_bl,
             carg1,carg2,carg3,carg4,carg5);
    };
}

op_scano_6()
{
num_ops = 6;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s %s %s %s %s %s",
	   code_bl,&command_bl,carg1,carg2,carg3,carg4,carg5,carg6);
  else
    {fscanf(input_fp,"%s %s  %s %s %s %s",carg1,carg2,carg3,carg4,carg5,carg6);
     sprintf(buffer_bl,"%15s %x %s %s %s %s %s %s",code_bl,command_bl,
             carg1,carg2,carg3,carg4,carg5,carg6);
    };
}

op_scano_9()
{
num_ops = 9;
  if(source_bl == STACK)
    sscanf(quads_bl[quad_bl],"%15s %x %s %s %s %s %s %s %s %s %s",
	   code_bl,&command_bl,carg1,carg2,carg3,carg4,carg5,carg6,carg7,carg8,carg9);
  else
    {fscanf(input_fp,"%s %s  %s %s %s %s %s %s %s",carg1,carg2,carg3,carg4,carg5,carg6,carg7,carg8,carg9);
     sprintf(buffer_bl,"%15s %x %s %s %s %s %s %s %s %s %s",code_bl,command_bl,carg1,carg2
	     ,carg3,carg4,carg5,carg6,carg7,carg8,carg9);
    };
}


/* If save flag is on (one or larger), this routine moves buffer_bl 
   to string memory and puts pointer on the quadruple stack.
   If save flag is zero or less, stack is cleared and memory is
   garbage collected - not sure what this does for optimizer   */
op_stack_buf()
{
  char *pointer;
  int i;

  if(save_bl && source_bl==INPUT)
    {if((pointer = (char *)malloc(strlen(buffer_bl)+1)) == 0)
       /* error return with dump */
       {printf("ERROR - 0 return from malloc in op_stack_buf\n");
        printf("sbrk(0)=%d\n",sbrk(0));
        printf("Quadruple stack =\n");
          for (i=toqs_bl-1;i>toqs_bl-20;i--) 
	     printf("                  %s\n",quads_bl[i]);
         if(*sub_file != 0) printf("in subroutine %s\n",sub_file);
         for(i=in_sub; i>=0; i--) 
             printf("sub_stack[%d]=%s\n",i,&sub_file_stack[i][0]);
	 exit(1);}
      /* memory allocated, copy string and add to stack */
      strcpy(pointer,buffer_bl);
      op_push_quads(pointer);
    }
  else if(save_bl <= 0) op_garbage(); 
}

/* put quadruple pointer on stack */
op_push_quads(pointer)
int pointer;
{  int i;

   if(toqs_bl<Q_STACK_SIZE) 
     quads_bl[toqs_bl++]=pointer; 
   else /* error with dump */
     {printf("ERROR - Quadruple stack overflow");
      printf("\nQuadruple stack =\n");
      for (i=toqs_bl-1;i>toqs_bl-20;i--) 
         printf("                  %s\n",quads_bl[i]);
      if(*sub_file != 0) printf("in subroutine %s\n",sub_file);
      for(i=in_sub; i>=0; i--) 
         printf("sub_stack[%d]=%s\n",i,&sub_file_stack[i][0]);
      exit(1); };
}

/* remove top quadruple from stack */
op_pop_quads()
{  int i;

   toqs_bl--;
}


/* clears entire stack, may want to modify for optimization */
op_garbage()
{
  if(toqs_bl>=0)
    {while(toqs_bl-- >= 0) 
       if(quads_bl[toqs_bl] != NULL) 
          free(quads_bl[toqs_bl]);
     toqs_bl=0; /* restore toqs_bl to 0 */
     quad_bl=0;
     }
}


