# include <stdio.h>
# include "cc.h"
# include "semutil.h"
# include "sym.h"
# include "sem.h"
# include <string.h>
# include <stdlib.h>

#define MAXLINES 80
#define MAXARGS 50
#define MAXLEVELS 50
#define MAXLOOPS 50
#define MAXLABELS 50

extern int  formalnum;                 /* number of formal arguments */
extern char formaltypes[];             /* types of formal arguments  */
extern char formalnames[][MAXLINES];   /* names of formal arguments  */
extern int  localnum;                  /* number of local variables  */
extern char localtypes[];              /* types of local variables   */
extern char localnames[][MAXLINES];    /* names of local variables   */
extern int  localwidths[]; 
extern int  level;

extern int lineno;

// for proecssing args in function calls
static int  argplaces[MAXARGS];
static char argtypes[MAXARGS];
static int  argwidths[MAXARGS];

// holds name of current function so that its return type can be looked up 
static char retfunc[MAXLINES];

// used for branches, labels, and backpatching
int brnchnum;
int lbl;

// for processing labeldcls and goto stmts
static int labelnum;                             /* number of labels in the current block  */
static int labelnums[MAXLABELS];                 /* label number associated with the label */
static int gotonum;                              /* number of goto's in the current block  */ 
static char gotolabels[MAXLABELS+1][MAXLINES];   /* label name associated with the goto    */
int gotobrnums[MAXLABELS+1];                     /* branch number of the goto to be backpatched */

// this variable will be used to print the correct bgstmt line number
// when we are inside a conditional stmt or loop  
int condstmt;
int lblstmt;
int startedloopscope;

// following is for loop scope and break/continue backpatching
int loopnum;
struct sem_rec *loopscopes[MAXLOOPS+1] = {0};

//function prototypes
int get_type(int x);
int type_cast(int x, char t);

/*
 * backpatch - backpatch list of quadruples starting at p with k
 */
void backpatch(struct sem_rec *p, int k)
{
  struct sem_rec *t;
  t = p;
  
  if (t->back.s_link == 0) // only one label to backpatch
    printf("B%d=L%d\n", t->s_place, k);
  else { // more than one label to backpatch
    for (; t->back.s_link; t = t->back.s_link) {
      printf("B%d=L%d\n", t->s_place, k);
    }
    printf("B%d=L%d\n", t->s_place, k);
  }
}

/*
 * bgnstmt - encountered the beginning of a statement
 */
void bgnstmt()
{
  printf("bgnstmt %d\n", lineno);
  /*
  if (condstmt && lblstmt) {
    condstmt = 0;
    lblstmt = 0;
  }
  else
    printf("bgnstmt %d\n", lineno);
  */
}

/*
 * call - procedure invocation
 */
struct sem_rec *call(char *f, struct sem_rec *args)
{
  struct id_entry *ie;
  struct sem_rec *t;
  int ret_type, argnum = 0, temp;

  // lookup function to get its return type and width(num of params)
  ie = lookup(slookup(f),0);
  // if function is not found in the sym table, assume it is a global library function call
  if (ie == NULL) {
    ie = install(f,2);
    ie->i_type = T_INT;
    ie->i_scope = GLOBAL;
  }
  ret_type = get_type(ie->i_type);

  // process list or args
  if (args != NULL) {
    t = args;
    while(t) {
      printf("arg%c t%d\n", get_type(t->s_mode), t->s_place);
      argplaces[argnum++] = t->s_place;
      t = t->back.s_link;
    }
  }
  else
    argnum = 0;

  printf("t%d := global %s\n", nexttemp(), f);
  temp = currtemp();
  printf("t%d := f%c t%d %d", nexttemp(), ret_type, temp, argnum); 
  
  int i;
  for(i = 0; i < argnum; i++)
    printf(" t%d", argplaces[i]);
  puts("");

  return (node(currtemp(), ie->i_type, 0, 0));
}

/*
 * ccand - logical and
 */
struct sem_rec *ccand(struct sem_rec *e1, int m, struct sem_rec *e2)
{
  backpatch(e1->back.s_link, m);
  return (node(0, 0, e2->back.s_link, merge(e1->s_false, e2->s_false)));
}

/*
 * ccexpr - convert arithmetic expression to logical expression
 */
struct sem_rec *ccexpr(struct sem_rec *e)
{
  // need to compare the value of the arithmetic expr to 0 to determine truth value
  return(rel("!=", e, con("0")));
}

/*
 * ccnot - logical not
 */
struct sem_rec *ccnot(struct sem_rec *e)
{
  return (node(0, 0, e->s_false, e->back.s_link));
}

/*
 * ccor - logical or
 */
struct sem_rec *ccor(struct sem_rec *e1, int m, struct sem_rec *e2)
{
  backpatch(e1->s_false, m);
  return (node(0, 0, merge(e1->back.s_link, e2->back.s_link), e2->s_false));
}

/*
 * con - constant reference in an expression
 */
struct sem_rec *con(char *x)
{
  struct sem_rec *t;
  t = node(nexttemp(), T_INT, 0, 0);

  printf("t%d := %s\n", currtemp(), x);

  return (t);
}

/*
 * dobreak - break statement
 */
void dobreak()
{
  struct sem_rec *t;

  t = node(++brnchnum, 0, 0, 0);
  
  if (loopscopes[loopnum] == 0) {
    loopscopes[loopnum] = node(0, 0, 0, t);
  }
  else {
    if (loopscopes[loopnum]->s_false != 0)
      merge(loopscopes[loopnum]->s_false,t);
    else
      loopscopes[loopnum]->s_false = t;
  }
  printf("br B%d\n", brnchnum);
}

/*
 * docontinue - continue statement
 */
void docontinue()
{
  struct sem_rec *t;

  t = node(++brnchnum, 0, 0, 0);
  
  if (loopscopes[loopnum] == 0)
    loopscopes[loopnum] = node(0, 0, t, 0);
  else {
    if (loopscopes[loopnum]->back.s_link != 0)
      merge(loopscopes[loopnum]->back.s_link,t);
    else
      loopscopes[loopnum]->back.s_link = t;
  }
  printf("br B%d\n", brnchnum);
}

/*
 * dodo - do statement
 */
void dodo(int m1, int m2, struct sem_rec *e, int m3)
{
  backpatch(e->back.s_link, m1);
  backpatch(e->s_false, m3);

  if (loopscopes[loopnum] != 0) { // we have break and/or continue stmts to backpatch
    if (loopscopes[loopnum]->back.s_link != 0)
      backpatch(loopscopes[loopnum]->back.s_link, m2);
    if(loopscopes[loopnum]->s_false != 0)
      backpatch(loopscopes[loopnum]->s_false, m3); 
  }
  condstmt = 0;
  endloopscope(loopnum);
}

/*
 * dofor - for statement
 */
void dofor(int m1, struct sem_rec *e2, int m2, struct sem_rec *n1,
           int m3, struct sem_rec *n2, int m4)
{
  if (e2->s_place != 0) {
    backpatch(e2->back.s_link, m3);
    backpatch(e2->s_false, m4);
    backpatch(n1->back.s_link, m1);
  }
  else {
    backpatch(e2->back.s_link->back.s_link, m3);
    backpatch(n1->back.s_link, m3);
  }
  backpatch(n2->back.s_link, m2);
  

  if (loopscopes[loopnum] != 0) { // we have break and/or continue stmts to backpatch
    if (loopscopes[loopnum]->back.s_link != 0)
      backpatch(loopscopes[loopnum]->back.s_link, m2);
    if(loopscopes[loopnum]->s_false != 0)
      backpatch(loopscopes[loopnum]->s_false, m4); 
  }
  lblstmt = 0;
  endloopscope(loopnum);
}

/*
 * dogoto - goto statement
 */
void dogoto(char *id)
{
  struct id_entry *p;

  p = lookup(slookup(id), 0);
  if (p != NULL) {
    printf("br L%d\n", labelnums[p->i_offset]);
  }
  else { // add to list to be backpatched if label is later declared
    if (gotonum > MAXLABELS) {
      fprintf(stderr, "Maximum goto labels exceeded\n");
      exit(1);
    }
    strcpy(gotolabels[gotonum], id);
    gotobrnums[gotonum] = ++brnchnum;
    printf("br B%d\n", brnchnum);
    
    gotonum++;
  }
}

/*
 * doif - one-arm if statement
 */
void doif(struct sem_rec *e, int m1, int m2)
{
  backpatch(e->back.s_link, m1);
  backpatch(e->s_false, m2);
    
  condstmt = 0;
}

/*
 * doifelse - if then else statement
 */
void doifelse(struct sem_rec *e, int m1, struct sem_rec *n,
                         int m2, int m3)
{
  backpatch(e->back.s_link, m1);
  backpatch(e->s_false, m2);
  backpatch(n->back.s_link, m3);

  condstmt = 0;
}

/*
 * doret - return statement
 */
void doret(struct sem_rec *e)
{
  struct id_entry *ie;
  int ret_type, e_type, temp;

  // find enclosing function to determine return type
  ie = lookup(slookup(retfunc),0);
  ret_type = get_type(ie->i_type);
  
  if (e != NULL) {
    temp = e->s_place;
    e_type = get_type(e->s_mode);
    if (e_type != ret_type) {
      printf("t%d := cv%c t%d\n", nexttemp(), ret_type, temp);
      temp = currtemp();
    }
    printf("ret%c t%d\n", ret_type, temp);
  }
  else
    printf("ret%c\n", ret_type);
}

/*
 * dowhile - while statement
 */
void dowhile(int m1, struct sem_rec *e, int m2, struct sem_rec *n,
             int m3)
{
  backpatch(e->back.s_link, m2);
  backpatch(e->s_false, m3);
  backpatch(n->back.s_link, m1);

  if (loopscopes[loopnum] != 0) { // we have break and/or continue stmts to backpatch
    if (loopscopes[loopnum]->back.s_link != 0)
      backpatch(loopscopes[loopnum]->back.s_link, m1);
    if(loopscopes[loopnum]->s_false != 0) 
      backpatch(loopscopes[loopnum]->s_false, m3);
  }
  condstmt = 0;
  endloopscope(loopnum);
}

/*
 * endloopscope - end the scope for a loop
 */
void endloopscope(int m)
{
  loopscopes[loopnum--] = 0;
}

/*
 * exprs - form a list of expressions
 */
struct sem_rec *exprs(struct sem_rec *l, struct sem_rec *e)
{
  merge(l,e);
  return (l);
}

/*
 * fhead - beginning of function body
 */
void fhead(struct id_entry *p)
{ 
  int t, i =  0;
  for(; i < formalnum; i++) {
    if (formaltypes[i] == 'i')
      t = T_INT;
    else // == 'f'
      t = T_DOUBLE;
    printf("formal %s %d %d\n", formalnames[i], t, tsize(t&~T_ARRAY));
  }

  for(i = 0; i < localnum; i++) {
    if (localtypes[i] == 'i')
      t = T_INT;
    else // == 'f'
      t = T_DOUBLE;
    
    if (localwidths[i] > 1)
      t |= T_ARRAY;

    printf("localloc %s %d %d\n", localnames[i], t, localwidths[i] * tsize(t&~T_ARRAY));
  }
  
  // set the width of the function to formalnum so that we know the number of parameters
  // when the function is called
  p->i_width = formalnum;

  // reset nums so that next function delcaration can be processed
  formalnum = 0;
  localnum = 0;
}

/*
 * fname - function declaration
 */
struct id_entry *fname(int t, char *id)
{
  struct id_entry *p;
  char msg[80];

  if ((p = lookup(id,level)) == NULL)
    p= install(id,-1);
  else if (p->i_defined) {
    sprintf(msg, "procedure previously declared");
    yyerror(msg);
    return (p);
  }
  else if (p->i_type != t) {
    yyerror("procedure type does not match");
  }

  p->i_defined = 1;
  p->i_type = t;
  p->i_scope = GLOBAL;

  printf("func %s %d\n", id, t);

  // copy function name to global var so that we can lookup its return type later
  strcpy(retfunc,id);

  enterblock();

  return (p);
}

/*
 * ftail - end of function body
 */
void ftail()
{
  printf("fend\n");

  // check for goto's that were unable to be backpatched
  int i;
  for (i = 0; i < gotonum; i++) {
    if (gotolabels[i][0] != 0) {
      fprintf(stderr, "label %s referenced in goto, but never declared\n", gotolabels[i]);
      gotolabels[i][0] = 0;
    }
  }
  gotonum = 0;
  labelnum = 0;
  leaveblock();
}

/*
 * id - variable reference
 */
struct sem_rec *id(char *x)
{
  struct id_entry *p;
  struct sem_rec *t;
  
  p = lookup(x,0);
  if (p == NULL) {
    fprintf(stderr, " undeclared identifer.  Line %d\n", lineno);

    // given csem executable adds the undeclared id to the sym table and sets the scope to LOCAL and offset to 0
    p = install(x,level);

    p->i_scope = LOCAL;
    p->i_type = T_INT;
    p->i_offset = 0;
  }
 
  switch (p->i_scope) {
    case LOCAL :
      printf("t%d := local %s %d\n", nexttemp(), p->i_name, p->i_offset);
      break;
    case PARAM :
      printf("t%d := param %s %d\n", nexttemp(), p->i_name, p->i_offset);
      break;
    case GLOBAL :
      printf("t%d := global %s\n", nexttemp(), p->i_name);
      break;
    default:
      break;
  }
  t = node(currtemp(), p->i_type|T_ADDR, 0, 0);
  return (t);
}

/*
 * sindex - subscript
 */
struct sem_rec *sindex(struct sem_rec *x, struct sem_rec *i)
{
  struct sem_rec *t;
  int x_type, i_type, mode;
  int index = i->s_place;

  x_type = get_type(x->s_mode);
  i_type = get_type(i->s_mode);

  if (i_type != 'i') {
    printf("t%d := cvi t%d\n", nexttemp(), i->s_place);
    index = currtemp();
  }

  printf("t%d := t%d []%c t%d\n", nexttemp(), x->s_place, x_type, index);
  mode = (x_type == 'i') ? T_INT : T_DOUBLE;
  t = node(currtemp(), mode, 0, 0);
 
  return (t);
}

/*
 * labeldcl - process a label declaration
 */
void labeldcl(char *id)
{
  struct id_entry *p;

  p = lookup(slookup(id), 0);
  if (p == NULL) {
    ++lbl; ++labelnum;
    if (labelnum > MAXLABELS) {
      fprintf(stderr, "Maximum number of labels exceeded.\n");
      exit(1);
    }
    p = install(id, level);
    p->i_type = T_LBL;
    p->i_offset = labelnum;
    
    labelnums[labelnum] = lbl;
    
    printf("label L%d\n", lbl);

    // check goto backpatch list
    int i;
    for (i = 0; i < gotonum; i++) {
      if (strcmp(gotolabels[i], id) == 0) {
	printf("B%d=L%d\n", gotobrnums[i], lbl);
	gotolabels[i][0] = 0;
      }
    }
  }
  else
    fprintf(stderr, " identifier %s previously declared.  Line %d", id, lineno);
}

/*
 * m - generate label and return next temporary number
 */
int m()
{
  printf("label L%d\n", ++lbl); 
  lblstmt = 1;
  return (lbl);
}

/*
 * n - generate goto and return backpatch pointer
 */
struct sem_rec *n()
{
  struct sem_rec *t;
  
  printf("br B%d\n", ++brnchnum);
  t = node(brnchnum, T_LBL, 0, 0);

  return (node(0, 0, t, 0));
}

/*
 * op1 - unary operators
 */
struct sem_rec *op1(char *op, struct sem_rec *y)
{
  struct sem_rec *t;
  
  int y_type = get_type(y->s_mode);
  int mode = y->s_mode;
  int temp = y->s_place;

  switch (op[0]) {
    case '-':
      printf("t%d := -%c t%d\n", nexttemp(), y_type, temp);
      break;
    case '~': // bitwise negation operates on ints only
      if (y_type == 'f') {
	temp = type_cast(temp, 'i');
	mode = T_INT;
      }
      printf("t%d := ~i t%d\n", nexttemp(), temp);
      break;
    case '@':
      printf("t%d := @%c t%d\n", nexttemp(), y_type, temp);
      mode = mode & ~T_ADDR;
      break;
    default:
      break;
  }
  t = node(currtemp(), mode, 0, 0);
  return (t);
}

/*
 * op2 - arithmetic operators
 */
struct sem_rec *op2(char *op, struct sem_rec *x, struct sem_rec *y)
{
  struct sem_rec *t;
  int op1, op2, mode;
  int op1_type, op2_type, op_type;

  op1 = x->s_place;
  op2 = y->s_place;

  op1_type = get_type(x->s_mode);
  op2_type = op_type = get_type(y->s_mode);
 
  // perform type checking
  if ((op1_type == 'i') && (op2_type == 'f')) {
    op1 = type_cast(op1, 'f');
    op_type = 'f';
  }
  else if ((op1_type == 'f') && (op2_type == 'i')) {
    op2 = type_cast(op2, 'f');
    op_type = 'f';
  }

  if (op[0] == '%' && op_type == 'f')
      fprintf(stderr, " cannot %%%% floating-point values.  Line %d\n", lineno);

  switch (op[0]) {
    case '+': 
      printf("t%d := t%d +%c t%d\n", nexttemp(), op1, op_type, op2);
      break;
    case '-': 
      printf("t%d := t%d -%c t%d\n", nexttemp(), op1, op_type, op2);
      break;
    case '*':
      printf("t%d := t%d *%c t%d\n", nexttemp(), op1, op_type, op2);
      break;
    case '/':
      printf("t%d := t%d /%c t%d\n", nexttemp(), op1, op_type, op2);
      break;
    case '%':
      printf("t%d := t%d %%%c t%d\n", nexttemp(), op1, op_type, op2);
      break;
    default:
      break;
  }
  
  mode = (op_type == 'i') ? T_INT : T_DOUBLE;
  t = node(currtemp(), mode, 0, 0);

  return (t);
}

/*
 * opb - bitwise operators
 */
struct sem_rec *opb(char *op, struct sem_rec *x, struct sem_rec *y)
{
  struct sem_rec *t;
  int op1, op2;
  int op1_type, op2_type;

  op1 = x->s_place;
  op2 = y->s_place;

  op1_type = get_type(x->s_mode);
  op2_type = get_type(y->s_mode);

  // type checking - all bitwise ops operate on ints only
  if (op2_type != 'i')
    op2 = type_cast(op2, 'i');
    
  if (op1_type != 'i')
    op1 = type_cast(op1, 'i');

  switch(op[0]) {
    case '|':  
      printf("t%d := t%d |i t%d\n", nexttemp(), op1, op2);
      break;
    case '^': 
      printf("t%d := t%d ^i t%d\n", nexttemp(), op1, op2);
      break;
    case '&': 
      printf("t%d := t%d &i t%d\n", nexttemp(), op1, op2);
      break;
    case '<': 
      printf("t%d := t%d <<i t%d\n", nexttemp(), op1, op2);
      break;
    case '>': 
      printf("t%d := t%d >>i t%d\n", nexttemp(), op1, op2);
      break;
    default:
      break;
  }
  
  t = node(currtemp(), T_INT, 0, 0);
  return (t);
}

/*
 * rel - relational operators
 */
struct sem_rec *rel(char *op, struct sem_rec *x, struct sem_rec *y)
{
  struct sem_rec *t, *b_true, *b_false;
  int e1, e2;
  int e1_type, e2_type, ctype;


  e1 = x->s_place;
  e2 = y->s_place;

  e1_type = get_type(x->s_mode);
  e2_type = ctype = get_type(y->s_mode);
 
  // perform type checking
  if ((e1_type == 'i') && (e2_type == 'f')) {
    e1 = type_cast(e1, 'f');
    ctype = 'f';
  }
  else if ((e1_type == 'f') && (e2_type == 'i')) {
    e2 = type_cast(e2, 'f');
    ctype = 'f';
  }

  if (strcmp(op, "==") == 0) 
    printf("t%d := t%d ==%c t%d\n", nexttemp(), e1, ctype, e2);
  else if (strcmp(op, "!=") == 0)
    printf("t%d := t%d !=%c t%d\n", nexttemp(), e1, ctype, e2);
  else if (strcmp(op, "<=") == 0)
    printf("t%d := t%d <=%c t%d\n", nexttemp(), e1, ctype, e2);
  else if (strcmp(op, ">=") == 0)
    printf("t%d := t%d >=%c t%d\n", nexttemp(), e1, ctype, e2);
  else if (strcmp(op, "<") == 0)
    printf("t%d := t%d <%c t%d\n", nexttemp(), e1, ctype, e2);
  else // ">"
    printf("t%d := t%d >%c t%d\n", nexttemp(), e1, ctype, e2);

  b_true = node(++brnchnum, T_LBL, 0, 0);
  printf("bt t%d B%d\n", currtemp(), brnchnum);

  b_false = node(++brnchnum, T_LBL, 0, 0);
  printf("br B%d\n", brnchnum);

  condstmt = 1;

  return (node(currtemp(), 0, b_true, b_false));
}

/*
 * set - assignment operators
 */
struct sem_rec *set(char *op, struct sem_rec *x, struct sem_rec *y)
{
  struct sem_rec *t, *t1, *t2;
  int lhs, result, mode;
  int lhs_type, rhs_type;

  t1 = x;
  t2 = y;

  // we need lhs's value if it's being used in a calcuation
  if (strcmp(op,"") != 0) // not simple assignment
    t1 = op1("@", x);

  lhs = x->s_place;
  lhs_type = get_type(x->s_mode);

  switch (op[0]) {
    // note: bitwise operators work on ints only 
    case '|': // x |= y
      t = opb(op, t1, t2);
      break;
    case '^': // x ^= y
      t = opb(op, t1, t2);
      break;
    case '&': // x &= y
      t = opb(op, t1, t2);
      break;
    case '<': // x <<= y
      t = opb(op, t1, t2);
      break;
    case '>': // x >>= y
      t = opb(op, t1, t2);
      break;
    case '%': // x %= y
      t = op2(op, t1, t2);
      break;
    case '+': // x += y
      t = op2(op, t1, t2);
      break;
    case '-': // x -= y
      t = op2(op, t1, t2);
      break;
    case '*': // x *= y
      t = op2(op, t1, t2);
      break;
    case '/': // x /= y
      t = op2(op, t1, t2);
      break;
    default : // '='
      t = y;
      break;
  }
  
  result = t->s_place;
  rhs_type = get_type(t->s_mode);

  // type check assignment
  if (lhs_type != rhs_type)
    result = type_cast(result, lhs_type);
  
  printf("t%d := t%d =%c t%d\n", nexttemp(), lhs, lhs_type, result);

  mode = (lhs_type == 'i') ? (T_INT|T_ADDR) : (T_DOUBLE|T_ADDR);
  t = node(currtemp(), mode, 0, 0);
   
  return (t);
}

/*
 * startloopscope - start the scope for a loop
 */
void startloopscope()
{
  /* you may assume the maximum number of loops in a loop nest is 50 */
  loopnum++;
  if (loopnum > MAXLOOPS) {
    fprintf(stderr, "maximum number of nested loops exceeded\n");
    exit(1);
  }
  startedloopscope = 1;
}

/*
 * string - generate code for a string
 */
struct sem_rec *string(char *s)
{
  struct sem_rec *t;
  t = node(nexttemp(), T_STR, 0, 0);
  printf("t%d := %s\n", currtemp(), s);
  return (t);
}

/*
 * Returns 'i' if x is T_INT or T_STR; 'f' if x is T_DOUBLE
 */
int get_type(int x) {
  int t = x&~T_ARRAY&~T_ADDR&~T_PROC; 
  return (t == T_INT || t == T_STR) ? 'i' : 'f'; 
}

/*
 * Casts x to type t and returns temporary of type cast
 */
int type_cast(int x, char t) {
  printf("t%d := cv%c t%d\n", nexttemp(), t, x);
  return currtemp();
}
