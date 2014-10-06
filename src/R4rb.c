/**********************************************************************

  R4rb.c

**********************************************************************/
#include <stdio.h>
#include <string.h>

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <Rversion.h>


/* From Parse.h -- must find better solution: */
#define PARSE_NULL              0
#define PARSE_OK                1
#define PARSE_INCOMPLETE        2
#define PARSE_ERROR             3
#define PARSE_EOF               4


#define Need_Integer(x) (x) = rb_Integer(x)
#define Need_Float(x) (x) = rb_Float(x)
#define Need_Float2(x,y) {\
    Need_Float(x);\
    Need_Float(y);\
}
#define Need_Float3(x,y,z) {\
    Need_Float(x);\
    Need_Float(y);\
    Need_Float(z);\
}

#if (R_VERSION < 132352) /* before 2.5 to check!*/
SEXP R_ParseVector(SEXP, int, int *);
#define RR_ParseVector(x,y,z) R_ParseVector(x, y, z)
#else
SEXP R_ParseVector(SEXP, int, int *,SEXP);
#define RR_ParseVector(x,y,z) R_ParseVector(x, y, z, R_NilValue)
#endif

/************* INIT *********************/

extern Rboolean R_Interactive;
extern int Rf_initEmbeddedR(int argc, char *argv[]);
static int R_initialized=0;

mrb_value R2rb_init(mrb_state *mrb, mrb_value obj) //, mrb_value args)
{
  char* argv[4];
    argv[0]="REmbed";
    argv[1]="--save";
    argv[2]="--slave";
    argv[3]="--quiet";

  //printf("argc=%d\n",argc);
  Rf_initEmbeddedR(4,argv);
  R_Interactive = FALSE;
  R_initialized=1;
  return mrb_true_value();
}

/***************** EVAL **********************/

mrb_value R2rb_eval(mrb_state *mrb,mrb_value obj) //, mrb_value cmd, mrb_value print)
{
  char *cmdString;
  mrb_int len;
  int errorOccurred,status, print;

  SEXP text, expr, ans=R_NilValue /* -Wall */;

  mrb_get_args(mrb,"si",&cmdString,&len,&print);
  //printf("%s\n",cmdString);

  //printf("Avant parsing\n");

  //printf("nbCmds : %d\n",nbCmds);

  text = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(text, 0, mkChar(cmdString));
  expr = PROTECT(RR_ParseVector(text, -1, &status));

  if (status != PARSE_OK) {
    printf("Parsing error (status=%d) in:\n",status);
    printf("%s\n",cmdString);
    UNPROTECT(2);
    return mrb_false_value();
  }
  
  /* Note that expr becomes an EXPRSXP and hence we need the loop
     below (a straight eval(expr, R_GlobalEnv) won't work) */
  {
      ans = R_tryEval(VECTOR_ELT(expr, 0),NULL, &errorOccurred);
      if(errorOccurred) {
        //fprintf(stderr, "Caught another error calling sqrt()\n");
        fflush(stderr);
        UNPROTECT(2);
        return mrb_false_value();
      }

      if (print != 0) {
        Rf_PrintValue(ans);
      }
  }

  UNPROTECT(2);
  return mrb_true_value();
}

/***************** PARSE **********************/

mrb_value R2rb_parse(mrb_state *mrb, mrb_value obj) //, mrb_value cmd,mrb_value print)
{
  char *cmdString;
  int status,print;
  mrb_int len;

  SEXP text, expr /* -Wall */;
  
  
  mrb_get_args(mrb,"si",&cmdString,&len,&print);

  //printf("Avant parsing\n");

  //printf("nbCmds : %d\n",nbCmds);

  text = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(text, 0, mkChar(cmdString));
  expr = PROTECT(RR_ParseVector(text, -1, &status));

  if (status != PARSE_OK) {
    if (print != 0) {
      printf("Parsing error (status=%d) in:\n",status); 
      printf("%s\n",cmdString);
    }
    //UNPROTECT(2);
    //return Qfalse;
  }
  UNPROTECT(2);
  //return Qtrue;
  return mrb_fixnum_value(status);
}


// /*****************************************

// Interface to get values of RObj from Ruby
// The basic idea : no copy of the R Vector
// just methods to extract value !!!

// ******************************************/

// used internally !!! -> eval only one string line
SEXP util_eval1string(mrb_state *mrb,mrb_value cmd)
{
  char *cmdString;
  int  errorOccurred,status;
    
  SEXP text, expr, ans=R_NilValue /* -Wall */;

  text = PROTECT(allocVector(STRSXP, 1)); 
  cmdString=(char*)mrb_string_value_ptr(mrb,cmd);
//printf("cmd: %s\n",cmdString);
  SET_STRING_ELT(text, 0, mkChar(cmdString));
  expr = PROTECT(RR_ParseVector(text, -1, &status));
  if (status != PARSE_OK) {
    printf("Parsing error in: %s\n",cmdString);
    UNPROTECT(2);
    return R_NilValue;
  }
  /* Note that expr becomes an EXPRSXP and hence we need the loop
     below (a straight eval(expr, R_GlobalEnv) won't work) */
  ans = R_tryEval(VECTOR_ELT(expr, 0),R_GlobalEnv,&errorOccurred);
  //ans = eval(VECTOR_ELT(expr, 0),R_GlobalEnv);
  if(errorOccurred) {
    //fflush(stderr);
    printf("Exec error in: %s\n",cmdString);
    UNPROTECT(2);
    return R_NilValue;
  }
  UNPROTECT(2);
  return ans;
}

int util_isVector(SEXP ans)
{
  return (!isNewList(ans) & isVector(ans));
}

int util_isVariable(mrb_state *mrb, mrb_value self)
{
  mrb_value tmp;
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb,"type"));
  return strcmp(mrb_string_value_ptr(mrb,tmp),"var")==0;
}

SEXP util_getVar(mrb_state *mrb, mrb_value self)
{
  SEXP ans;
  char *name;
  mrb_value tmp;

  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb,"name"));
  name=(char*)mrb_string_value_ptr(mrb,tmp);
  if(util_isVariable(mrb,self)) {
    ans = findVar(install(name),R_GlobalEnv); //currently in  R_GlobalEnv!!!
  } else {
    //printf("getVar:%s\n",name);
    ans=util_eval1string(mrb,mrb_iv_get(mrb,self,mrb_intern_lit(mrb,"name")));
    if(ans==R_NilValue) return ans;
  }
  if(!util_isVector(ans)) return R_NilValue;
  return ans;
}

//with argument!! necessarily an expression and not a variable
SEXP util_getExpr_with_arg(mrb_state *mrb, mrb_value self)
{
  SEXP ans;
  mrb_value tmp;

  //printf("getVar:%s\n",name);
  tmp=mrb_str_dup(mrb,mrb_iv_get(mrb,self,mrb_intern_lit(mrb,"arg")));
  ans=util_eval1string(mrb,mrb_str_cat_cstr(mrb,mrb_str_dup(mrb,mrb_iv_get(mrb,self,mrb_intern_lit(mrb,"name"))),mrb_string_value_ptr(mrb,tmp)));
  if(ans==R_NilValue) return ans;
  if(!util_isVector(ans)) return R_NilValue;
  return ans;
}


mrb_value util_SEXP2mrb_value(mrb_state *mrb,SEXP ans)
{
  mrb_value res;
  int n,i;
  //Rcomplex cpl; 
  
  n=length(ans);
  res = mrb_ary_new_capa(mrb,n);
  switch(TYPEOF(ans)) {
  case REALSXP:
    for(i=0;i<n;i++) {
      mrb_ary_push(mrb,res,mrb_float_value(mrb,REAL(ans)[i]));
    }
    break;
  case INTSXP:
    for(i=0;i<n;i++) {
      mrb_ary_push(mrb,res,mrb_fixnum_value(INTEGER(ans)[i]));
    }
    break;
  case LGLSXP:
    for(i=0;i<n;i++) {
      mrb_ary_push(mrb,res,(INTEGER(ans)[i] ? mrb_true_value() : mrb_false_value()));
    }
    break;
  case STRSXP:
    for(i=0;i<n;i++) {
      mrb_ary_push(mrb,res,mrb_str_new_cstr(mrb,CHAR(STRING_ELT(ans,i))));
    }
    break;
  // case CPLXSXP:
  //   rb_require("complex");
  //   for(i=0;i<n;i++) {
  //     cpl=COMPLEX(ans)[i];
  //     res2 = rb_eval_string("Complex.new(0,0)");
  //     rb_iv_set(res2,"@real",rb_float_new(cpl.r));
  //     rb_iv_set(res2,"@image",rb_float_new(cpl.i));
  //     rb_ary_store(res,i,res2);
  //   }
  //   break;
  }

  return res;
}


SEXP util_mrb_value2SEXP(mrb_state *mrb, mrb_value arr)
{
  SEXP ans;
  mrb_value res,tmp,elt;
  
  int i,n=0;

  if(!mrb_obj_is_kind_of(mrb,arr,mrb->array_class)) {
    n=1;
    res = mrb_ary_new_capa(mrb,1);
    mrb_ary_push(mrb,res,arr);
    arr=res;
  } else {
    n=RARRAY_LEN(arr);
  }  

  elt=mrb_ary_entry(arr,0);
  
  if(mrb_type(elt)==MRB_TT_FLOAT) {
    PROTECT(ans=allocVector(REALSXP,n));
    for(i=0;i<n;i++) {
      REAL(ans)[i]=mrb_float(mrb_ary_entry(arr,i));
    }
  } else if(mrb_type(elt)==MRB_TT_FIXNUM) {
    PROTECT(ans=allocVector(INTSXP,n));
    for(i=0;i<n;i++) {
      INTEGER(ans)[i]=mrb_int(mrb,mrb_ary_entry(arr,i));
    }
  } else if(mrb_type(elt)==MRB_TT_TRUE || mrb_type(elt)==MRB_TT_FALSE) {
    PROTECT(ans=allocVector(LGLSXP,n));
    for(i=0;i<n;i++) {
      LOGICAL(ans)[i]=(mrb_type(mrb_ary_entry(arr,i))==MRB_TT_FALSE ? FALSE : TRUE);
    }
  } else if(mrb_type(elt)==MRB_TT_STRING) {
    PROTECT(ans=allocVector(STRSXP,n));
    for(i=0;i<n;i++) {
      tmp=mrb_ary_entry(arr,i);
      SET_STRING_ELT(ans,i,mkChar(mrb_string_value_ptr(mrb,tmp)));
    }
  } else ans=R_NilValue;

  if(n>0) UNPROTECT(1);
  return ans; 
}



mrb_value RVect_initialize(mrb_state *mrb, mrb_value self) //, mrb_value name)
{
  mrb_value name;

  mrb_get_args(mrb,"S",&name);
  mrb_iv_set(mrb,self,mrb_intern_lit(mrb, "name"),name);
  mrb_iv_set(mrb,self,mrb_intern_lit(mrb, "type"),mrb_str_new_lit(mrb,"var"));
  mrb_iv_set(mrb,self,mrb_intern_lit(mrb, "arg"),mrb_str_new_lit(mrb,""));
  return self;
}

mrb_value RVect_isValid(mrb_state *mrb, mrb_value self)
{
  SEXP ans;
  char *name;

#ifdef cqls
  mrb_value tmp;
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"));
  name = (char*)mrb_string_value_ptr(mrb,tmp);
  ans = findVar(install(name),R_GlobalEnv); //currently in  R_GlobalEnv!!!
#else
  ans = util_getVar(mrb,self);
#endif
  if(!util_isVector(ans)) {
#ifndef cqls
    mrb_value tmp;
    tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"));
    name = (char*)mrb_string_value_ptr(mrb,tmp);
#endif
    mrb_warn(mrb,"%s is not a R vector !!!",name); //TODO name not defined
    return mrb_false_value();
  }
  return mrb_true_value();
}

mrb_value RVect_length(mrb_state *mrb, mrb_value self)
{
  SEXP ans;
  
#ifdef cqls
  char *name;
  mrb_value tmp;
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"));
  if(!RVect_isValid(mrb,self)) return mrb_nil_value();
  name = mrb_string_value_ptr(mrb,tmp);
  ans = findVar(install(name),R_GlobalEnv); //currently in  R_GlobalEnv!!!
#else
  ans = util_getVar(mrb,self);

  if(ans==R_NilValue) {
    //printf("Sortie de length avec nil\n");
    return mrb_nil_value();
  }
#endif
  return mrb_fixnum_value(length(ans));
}

mrb_value RVect_get(mrb_state *mrb, mrb_value self)
{
  SEXP ans;
  mrb_value res;

  //int i;
  //Rcomplex cpl;
  //mrb_value res2; 

  //#define cqls
#ifdef cqls 
  mrb_value tmp;
  if(!RVect_isValid(self)) return mrb_nil_value();
#else  
  ans = util_getVar(mrb,self);

  if(ans==R_NilValue) {
    //printf("Sortie de get avec nil\n");
    return mrb_nil_value();
  }
#endif
#ifdef cqls 
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"));
  name = mrb_string_value_ptr(mrb,tmp);
  ans = findVar(install(name),R_GlobalEnv); 
#endif

  res=util_SEXP2mrb_value(mrb,ans);
  if(length(ans)==1) res=mrb_ary_entry(res,0);
  return res; 
}

mrb_value RVect_get_with_arg(mrb_state *mrb, mrb_value self)
{
  SEXP ans;
  mrb_value res;
  
  //Rcomplex cpl;
  //mrb_value res2; 

  ans = util_getExpr_with_arg(mrb,self);
 
  if(ans==R_NilValue) {
    //printf("Sortie de get avec nil\n");
    return mrb_nil_value();
  }
  res=util_SEXP2mrb_value(mrb,ans);
 
//printf("RVect_get_with_arg: length(ans)=%d\n",length(ans));
 if (length(ans)==1) res=mrb_ary_entry(res,0);

  return res;
}



// faster than self.to_a[index]
mrb_value RVect_aref(mrb_state *mrb, mrb_value self) //, mrb_value index)
{
  SEXP ans;
  mrb_value res;
  //char *name;
  int n;
  //Rcomplex cpl;
  mrb_int i;
#ifdef cqls
  mrb_value tmp;
#endif
  
  mrb_get_args(mrb,"i",&i);
  
#ifdef cqls
  if(!RVect_isValid(mrb,self)) return mrb_nil_value();
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"));
  name = mrb_string_value_ptr(mrb,tmp);
  ans = findVar(install(name),R_GlobalEnv); //currently in  R_GlobalEnv!!!
#else
  ans = util_getVar(mrb,self);
#endif
  n=length(ans);
  //printf("i=%d and n=%d\n",i,n);
  if(i<n) {
    switch(TYPEOF(ans)) {
    case REALSXP:
      res=mrb_float_value(mrb,REAL(ans)[i]);
      break;
    case INTSXP:
      res=mrb_fixnum_value(INTEGER(ans)[i]);
      break;
    case LGLSXP:
      res=(INTEGER(ans)[i] ? mrb_true_value() : mrb_false_value());
      break;
    case STRSXP:
      res=mrb_str_new_cstr(mrb,CHAR(STRING_ELT(ans,i)));
      break;
    // case CPLXSXP:
    //   rb_require("complex");
    //   cpl=COMPLEX(ans)[i];
    //   res = rb_eval_string("Complex.new(0,0)");
    //   rb_iv_set(res,"@real",rb_float_new(cpl.r));
    //   rb_iv_set(res,"@image",rb_float_new(cpl.i));
    //   break;
    }
  } else {
    res = mrb_nil_value();
  }
  return res;
}

mrb_value RVect_set(mrb_state *mrb, mrb_value self) //,mrb_value arr)
{
  SEXP ans;
  char *name;
  mrb_value tmp;
  mrb_value arr;


  mrb_get_args(mrb,"A",&arr);

  ans=util_mrb_value2SEXP(mrb,arr);
  
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"));
  name = (char*)mrb_string_value_ptr(mrb,tmp);
  if(util_isVariable(mrb,self)) {
    defineVar(install(name),ans,R_GlobalEnv); //currently in R_GlobalEnv!!!
  } else {
    defineVar(install(".rubyExport"),ans,R_GlobalEnv);
    util_eval1string(mrb,mrb_str_cat_cstr(mrb,mrb_str_dup(mrb,mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"))),"<-.rubyExport"));
  }

  return self; 
}

mrb_value RVect_assign(mrb_state *mrb, mrb_value obj) //, mrb_value name,mrb_value arr)
{
  SEXP ans;
  char *name;
  mrb_int len;

  mrb_value arr;

  mrb_get_args(mrb,"sA",&name,&len,&arr);

  ans=util_mrb_value2SEXP(mrb,arr);

  defineVar(install(name),ans,R_GlobalEnv);

  return mrb_nil_value(); 
}

mrb_value RVect_set_with_arg(mrb_state *mrb, mrb_value self) //,mrb_value arr)
{
  mrb_value arr,tmp;

  mrb_get_args(mrb,"A",&arr);
  defineVar(install(".rubyExport"),util_mrb_value2SEXP(mrb,arr),R_GlobalEnv);
  tmp=mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "arg")); 
  util_eval1string(mrb,mrb_str_cat_cstr(mrb,mrb_str_cat_cstr(mrb,mrb_str_dup(mrb,mrb_iv_get(mrb,self,mrb_intern_lit(mrb, "name"))),mrb_string_value_ptr(mrb,tmp)),"<-.rubyExport"));
  return self;
}



void
mrb_mruby_R4rb_gem_init(mrb_state* mrb)
{

  struct RClass *mR2rb;
  mR2rb = mrb_define_module(mrb,"R2rb");

  mrb_define_module_function(mrb, mR2rb, "initR", R2rb_init, MRB_ARGS_NONE());

  mrb_define_module_function(mrb, mR2rb, "evalLine", R2rb_eval, MRB_ARGS_REQ(2));

  mrb_define_module_function(mrb, mR2rb, "parseLine", R2rb_parse, MRB_ARGS_REQ(2));

  struct RClass *cRVect;

  cRVect = mrb_define_class_under(mrb, mR2rb,"RVector",mrb->object_class);

  mrb_define_module_function(mrb, cRVect, "assign", RVect_assign, MRB_ARGS_REQ(2));

  mrb_define_method(mrb, cRVect,"initialize",RVect_initialize,MRB_ARGS_REQ(1));

  mrb_define_method(mrb, cRVect,"get",RVect_get, MRB_ARGS_NONE());
  mrb_define_alias(mrb, cRVect,"to_a","get");
  mrb_define_alias(mrb, cRVect,"value","get");

  mrb_define_method(mrb, cRVect,"set",RVect_set,MRB_ARGS_REQ(1));
  mrb_define_alias(mrb, cRVect,"<","set");
  mrb_define_alias(mrb, cRVect,"value=","set");

  // //method "arg=" defined in eval.rb!! @arg initialized in method "initialize"
  mrb_define_method(mrb, cRVect,"get_with_arg",RVect_get_with_arg,MRB_ARGS_NONE());
  mrb_define_alias(mrb, cRVect,"value_with_arg","get_with_arg");
  mrb_define_method(mrb, cRVect,"set_with_arg",RVect_set_with_arg,MRB_ARGS_REQ(1));
  mrb_define_alias(mrb, cRVect,"value_with_arg=","set_with_arg");

  mrb_define_method(mrb, cRVect,"valid?",RVect_isValid,MRB_ARGS_NONE());
  mrb_define_method(mrb, cRVect,"length",RVect_length,MRB_ARGS_NONE());
  mrb_define_method(mrb, cRVect,"[]",RVect_aref,MRB_ARGS_REQ(1));
  // //[]= iter !!!
  // mrb_define_attr(mrb, cRVect,"name",1,1);
  // mrb_define_attr(mrb, cRVect,"type",1,1);
}

void
mrb_mruby_R4rb_gem_final(mrb_state* mrb) {
}
