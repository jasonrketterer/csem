//
// Created by Jason Ketterer on 12/16/21.
//

#ifndef CSEM_SEM_H
#define CSEM_SEM_H
void backpatch(struct sem_rec *p, int k);
void bgnstmt();
struct sem_rec *call(char *f, struct sem_rec *args);
struct sem_rec *ccand(struct sem_rec *e1, int m, struct sem_rec *e2);
struct sem_rec *ccexpr(struct sem_rec *e);
struct sem_rec *ccnot(struct sem_rec *e);
struct sem_rec *ccor(struct sem_rec *e1, int m, struct sem_rec *e2);
struct sem_rec *con(char *x);
void dobreak();
void docontinue();
void dodo(int m1, int m2, struct sem_rec *e, int m3);
void dofor(int m1, struct sem_rec *e2, int m2, struct sem_rec *n1,
           int m3, struct sem_rec *n2, int m4);
void dogoto(char *id);
void doif(struct sem_rec *e, int m1, int m2);
void doifelse(struct sem_rec *e, int m1, struct sem_rec *n,
              int m2, int m3);
void doret(struct sem_rec *e);
void dowhile(int m1, struct sem_rec *e, int m2, struct sem_rec *n,
             int m3);
void endloopscope(int m);
struct sem_rec *exprs(struct sem_rec *l, struct sem_rec *e);
void fhead(struct id_entry *p);
struct id_entry *fname(int t, char *id);
void ftail();
struct sem_rec *id(char *x);
struct sem_rec *sindex(struct sem_rec *x, struct sem_rec *i);
void labeldcl(char *id);
int m();
struct sem_rec *n();
struct sem_rec *op1(char *op, struct sem_rec *y);
struct sem_rec *op2(char *op, struct sem_rec *x, struct sem_rec *y);
struct sem_rec *opb(char *op, struct sem_rec *x, struct sem_rec *y);
struct sem_rec *rel(char *op, struct sem_rec *x, struct sem_rec *y);
struct sem_rec *set(char *op, struct sem_rec *x, struct sem_rec *y);
void startloopscope();
struct sem_rec *string(char *s);
#endif //CSEM_SEM_H
