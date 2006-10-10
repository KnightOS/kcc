#ifndef MAIN_INCLUDE
#define MAIN_INCLUDE

typedef struct {
  unsigned int isLibrarySource:1;
  int disable_df;
  int stackLocation;
  int stackSize;
} pic14_options_t;

extern pic14_options_t pic14_options;

bool x_parseOptions (char **argv, int *pargc);
void x_setDefaultOptions (void);
void x_finaliseOptions (void);

#endif
