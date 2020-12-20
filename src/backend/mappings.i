static const ASM_MAPPING _asxxxx_z80_mapping[] = {
    /* We want to prepend the _ */
    { "area", ".area _%s" },
    { "areacode", ".area _%s" },
    { "areadata", ".area _%s" },
    { "areahome", ".area _%s" },
    { "*ixx", "(ix + %d)" },
    { "*iyx", "(iy + %d)" },
    { "*hl", "(hl)" },
    { "jphl", "jp (hl)" },
    { "di", "di" },
    { "ei", "ei" },
    { "globalfunctionlabeldef", "%s:" },
    { "ldahli",
      "ld a, (hl)\n"
      "inc\thl" },
    { "ldahlsp",
      "ld hl, #%d\n"
      "add\thl, sp" },
    { "ldaspsp",
      "ld iy,#%d\n"
      "add\tiy,sp\n"
      "ld\tsp,iy" },
    { "mems", "(%s)" },
    { "enter",
      "push\tix\n"
      "ld\tix,#0\n"
      "add\tix,sp" },
    { "enters",
      "call\t___sdcc_enter_ix\n" },
    { "pusha",
      "push af\n"
      "push\tbc\n"
      "push\tde\n"
      "push\thl\n"
      "push\tiy"
    },
    { "popa",
      "pop iy\n"
      "pop\thl\n"
      "pop\tde\n"
      "pop\tbc\n"
      "pop\taf"
    },
    { "adjustsp", "lda sp,-(sp + %d)" },
    { "profileenter",
      "ld a,#3\n"
      "rst\t0x08"
    },
    { "profileexit",
      "ld a,#4\n"
      "rst\t0x08"
    },
    { NULL, NULL }
};

const ASM_MAPPINGS _asxxxx_z80 = {
    &asm_asxxxx_mapping,
    _asxxxx_z80_mapping
};

