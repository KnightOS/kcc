/** @file z80/z80.h
    Common definitions between the z80 and gbz80 parts.
*/
#include "../common.h"
#include "ralloc.h"
#include "gen.h"
#include "peep.h"
#include "support.h"

typedef enum {
  SUB_Z80,
  SUB_EZ80_Z80,
} Z80_SUB_PORT;

typedef struct {
  Z80_SUB_PORT sub;
  int calleeSavesBC;
  int port_mode;
  int port_back;
  int reserveIY;
  int noOmitFramePtr;
  int legacyBanking;
  int nmosZ80;
} Z80_OPTS;

extern Z80_OPTS z80_opts;

#define IS_RAB false
#define IS_TLCS90 false
#define IS_Z80N false
#define IS_Z180 false
#define IS_R2K false
#define IS_R2KA false
#define IS_R3KA false
#define IS_GB false
#define IS_Z80 (z80_opts.sub == SUB_Z80)
#define IS_EZ80_Z80 (z80_opts.sub == SUB_EZ80_Z80)

#define IY_RESERVED (z80_opts.reserveIY)

#define OPTRALLOC_HL 1
#define OPTRALLOC_IY !(IY_RESERVED)

enum { ACCUSE_A = 1, ACCUSE_SCRATCH, ACCUSE_IY };
