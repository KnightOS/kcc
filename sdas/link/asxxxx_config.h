/* sdas/linksrc/asxxxx_config.h.  Generated from asxxxx_config.h.in by configure.  */
#ifndef __ASXXXX_CONFIG_H
#define __ASXXXX_CONFIG_H

#define TYPE_BYTE char
#define TYPE_WORD short
#define TYPE_DWORD int
#define TYPE_UBYTE unsigned char
#define TYPE_UWORD unsigned short
#define TYPE_UDWORD unsigned int

#if !defined TYPE_WORD && defined _WIN32
# define TYPE_BYTE char
# define TYPE_WORD short
# define TYPE_DWORD int
# define TYPE_UBYTE unsigned char
# define TYPE_UWORD unsigned short
# define TYPE_UDWORD unsigned int
#endif

#endif  /* __ASXXXX_CONFIG_H */
