// dc4wcomp.h
// set compiler string, if possible
#ifndef  _dc4wcomp_h
#define  _dc4wcomp_h
/* Helper macro to convert to a string */
#define MY_STGIZE(X) DO_STGIZE(X)
#define DO_STGIZE(X) #X

#ifdef __GNUC__
#define MY_COMP_STG "GNU C++ version " MY_STGIZE(__GNUC__) "." MY_STGIZE(__GNUC_MINOR__)
#endif // __GNUC__
/* KAI C++ */
#if defined(__KCC)
#define MY_COMP_STG "Kai C++ version " MY_STGIZE(__KCC_VERSION)
#endif // __KCC
// Metrowerks 
#if defined(__MWERKS__)
#define MY_COMP_STG "Metrowerks CodeWarrior C++ ver. " MY_STGIZE(__MWERKS__)
#endif // __MWERKS__
// Microsoft compilers.
#ifdef _MSC_VER
#define MY_COMP_STG "Microsoft Visual C++ ver. " MY_STGIZE(_MSC_VER)
#endif // _MSC_VER
#ifdef __BORLANDC__
#define MY_COMP_STG "Borland C++ ver. " MY_STGIZE(__BORLANDC__)
#endif // __BORLANDC__
// Native SGI compilers
#if defined ( sgi ) && !defined( __GNUC__ )
#define MY_COMP_STG "SGI MipsPro compiler ver. " MY_STGIZE(_COMPILER_VERSION)
#endif // Native SGI compilers
#if defined (__sun)
#if  !defined( __GNUC__ )
#define MY_COMP_STG "Sun compiler ver. " MY_STGIZE(__SUNPRO_CC)
#  endif
#endif // sun
// Intel C++ Compiler
#if defined(__ICC) || defined (__ECC)
#define MY_COMP_STG "Intel C++ ver. " MY_STGIZE(__ICC)
#endif // __ICC
#ifdef __APPLE__
#ifndef MY_COMP_STG
#define MY_COMP_STG "Compiled in Apple."
#endif
#endif
#ifndef MY_COMP_STG
#define MY_COMP_STG "Compiler NOT cased - unknown."
#endif

#endif // _dc4wcomp_h
// eof - dc4wcom.h