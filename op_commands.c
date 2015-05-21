/* EM_COMMANDS.C */

#include "op_common.h"

/* ===========================================================================

   function name: op_commands
   input      	 : ---
   output	    : ---
   return value : op_status	- SUCCESS, if no errors
				                  - FAIL, otherwise
 vars  : input_fp	- pointer to input file

   description	 : This routine reads the integer representation of a command
		            from the input file and calls an appropriate routine to
                  read the arguments and create a quadruple record

=========================================================================== */


op_commands()
{
   t_count = 0;				/* set to start */
   quad_bl = 0;
   l_count = 1;

   done = FALSE;
   op_status = SUCCESS;
   source_bl = INPUT;
   save_bl = TRUE;
   while (done == FALSE && op_status == SUCCESS)
      {
      read_everything();		/* read in ALL stuff */
      quad_bl++;

/*      printf("%10x",optype);
      printf("%s\n",buffer_bl);
*/      }
   return(op_status);
}

/* ===========================================================================
	read_everything()
		Read in all the stuff--I modified this so
		the info would be easier to get to later rather
		that building AROUND something, I like to make 8 million
		routines do little things.  That way something always
		works.
=========================================================================== */
read_everything()
{
     if ((command=op_fscanf()) == EOF)
	     done = TRUE;
      else
	{mode = command % SIZE_MODE;
			mode1 = mode / SIZE_MODE2;
			mode2 = mode % SIZE_MODE2;
	      optype = (command / SIZE_MODE) * SIZE_MODE;
         switch(optype)
	   {case T_RETURN     :
            case T_SCAREG     :
	    case T_REGSCA     :
            case T_POP_SCOPE  :
	    case T_STOP       :
            case T_IDENT      :
	    case T_CLRARRAY   :
            case T_INCLUDE    :
	    case T_INITSTACK  :
	    case T_ENDREAD    :
            case T_ENDPRINT   :
				  op_scano_0();
                                  op_stack_buf();
                                   break;
            case T_SUBROUTINE : if(in_sub <= 0) done = TRUE;
            case T_EXTERN     :
	    case T_ENTRY      :
	    case T_ENDMAIN    :
            case T_LABEL      :
	    case T_SETSCOPE   :
	    case T_MAIN       :
 	    case T_ENDNEXT    :
  	    case T_ENDGET     :
 	    case T_BEGWHILE   :
 	    case T_LOOP       :
            case T_BEGIF      :
 	    case T_ENDIF      :
            case T_DUMP       :
	    case T_NL         :
	    case T_CLOSE      :
	    case T_FSTOUT     :
            case T_PRINT_DEL  :
	    case T_STACK      :
	    case T_STACK_SCOPE:
         			  op_scano_1(); 
                                  op_stack_buf();
                                  break;
            case T_DEFLOG     :
            case T_CALL       :
            case T_FLOAT      :
            case T_OUTPT      :
	    case T_IOALLOC    :
	    case T_INPUT      :
	    case T_NOT	      :
            case T_MINUS      :
            case T_PLUS	      :
	    case T_FIX        :
            case T_SHORT      :
            case T_MINDEX     :
            case T_MINVAL     :
            case T_MAXDEX     :
            case T_MAXVAL     :
            case T_RESFST     :
	    case T_POP        :
	    case T_ENDSUB     :
	    case T_RELEASE    :
	    case T_FSTCD      :
	    case T_NXTCD      :
	    case T_PREVCD     :
	    case T_TRNCD      :
	    case T_TRNACD     :
            case T_COUNT      :
	    case T_SUM        :
	    case T_SHAPE      :
            case T_ANY	      :
            case T_ELSENANY   :
 	    case T_ENDWHILE   :
            case T_UNTIL      :
 	    case T_ENDLOOP    :
 	    case T_IF	      :
			          op_scano_2(); 
                                  op_stack_buf();
                                  break;
            case T_ADD	      :
	    case T_SUB	      :
	    case T_MULT       :
	    case T_DIV	      :
            case T_LT 	      :
	    case T_LE	      :
	    case T_EQ	      :
	    case T_NE	      :
	    case T_GT	      :
	    case T_GE	      :
            case T_OR	      :
	    case T_AND	      :
	    case T_XOR	      :
 	    case T_ENDFOR     :
 	    case T_NEXT	      :
  	    case T_GET	      :
 	    case T_WHILE      :
 	    case T_BEGSTACK   :
            case T_ALLOCATE   :
            case T_SIBDEX     :
            case T_MVID       :
	    case T_BEGREAD    :
	    case T_READNL     :
	    case T_EXPAND     :
            case T_BEGPRINT   :
            case T_PRINT      :
			          op_scano_3(); 
                                  op_stack_buf();
                                  break;
	    case T_FSTCD8     :
	    case T_NXTCD8     :
	    case T_PREVCD8    :
	    case T_TRNCD8     :
	    case T_TRNACD8    :
	    case T_DEFVAR     :
  	    case T_FOR        :
            case T_STACKWHILE :
            case T_ENDSTACKWHILE:
 	    case T_RECURSE    :
            case T_MVPAID     :
	    case T_READ       :
	    case T_COLAPSE    :
             			  op_scano_4(); 
                                  op_stack_buf();
                                  break;
            case T_MVCB       :
	    case T_SCOT       :
	    case T_SCOTL      :
	    case T_SCOTP      :
	    case T_SCOT8      :
	    case T_SCIN       :
	    case T_SCINL      :
				  op_scano_5(); 
                                  op_stack_buf();
                                  break;
            case T_OPEN       :
	    case T_SCIN8      :
				  op_scano_6(); 
                                  op_stack_buf();
                                  break;
	    case T_SCINP      :
				  op_scano_9(); 
                                  op_stack_buf();
                                  break;
	    case T_BOS        :   op_bos();
                                  break;
            case T_ASSEMBLE   :
            case T_MSG        :
			          op_scano_msg(); break;
            case T_TEMP       :
            case T_DECLARE    :  if(mode1 == T_CSTR)
			           op_scano_msg();
				 else
				   op_scano_5();
                                 op_stack_buf();
				 break;
           case T_ELSE	      :  if(mode2 == T_PARALLEL)
                                    op_scano_1();
				 else
				    op_scano_2();
                                 op_stack_buf();
				 break;
            case T_MVSC       :  if(mode2 == T_SCALAR || mode2 == T_REG_SCALAR)
                                    op_scano_2();
				 else
				    op_scano_3();
                                 op_stack_buf();
				 break;
            case T_MVPA       :  if(mode2 == T_PARALLEL)
                                    op_scano_2();
				 else
				  op_scano_4();
                                 op_stack_buf();
				 break;
            case T_ENDRECURSE :
            default	      : printf("\nSYSTEM ERROR - line no %d: Invalid",linenum);
   		                printf(" intermediate code : %x  ", command);
	                        printf("optyp=%x \n",optype);
                                printf("  command_bl=%s\n",command_bl);
	 		        op_status = FAIL;
			        done = TRUE;
				break;
	    }
         }
}


op_scano_msg()
{
  int i;
  char arg[LINE_LENGTH];       /* buffer of assembly language code */
  char *ptr,*arg1;
  char chan[LINE_LENGTH];      /* channel spec */

if(source_bl == STACK)
  {sscanf(quads_bl[quad_bl],"%15s %x %s",code_bl,&command_bl,chan);
	ptr = quads_bl[quad_bl]+31;}
else
  {getc(input_fp);
	i=0;
	while ((chan[i++] = getc(input_fp)) != ' ');
	chan[i-1] = NULL;

	i=0;
	while ((arg[i++] = getc(input_fp)) != '\n');
	arg[i-1] = NULL;
	sprintf(buffer_bl,"%15s %4x %10s %s","T_MSG",T_MSG,chan,arg);
	ptr=arg;
	op_stack_buf();
  }
  }

op_bos()
/* beginning of statement quadruple */
{
     int arg1,arg2;

     op_scano_2(); /* do not stack */

}


