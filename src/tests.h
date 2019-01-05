#ifndef __TEST_H_INIT_
#define STRINGIFY(X) STRINGIFY2(X)    
#define STRINGIFY2(X) #X

// Macros for concatenating tokens
#define CAT(X,Y) CAT2(X,Y)
#define CAT2(X,Y) X##Y
#define CAT_2 CAT
#define INCLUDE_FILE(HEAD) STRINGIFY( CAT_2(tests/test_,HEAD).c )

#endif

#ifdef TEST
  #ifndef TESTNAME
    #error "A test was requested but no testname defined"
  #endif
  #include INCLUDE_FILE(TESTNAME)
  #undef TESTNAME
#endif
