/*
 * SDCCerr - Standard error handler
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(__SDCCERR_H)

#define __SDCCERR_H

#include <stdio.h>
#include <stdarg.h>

/* ERROR Message Definition */

#define E_DUPLICATE                     0 /* Duplicate variable   */
#define E_SYNTAX_ERROR                  1 /* Syntax Error         */
#define E_CONST_EXPECTED                2 /* constant expected    */
#define E_OUT_OF_MEM                    3 /* malloc failed        */
#define E_FILE_OPEN_ERR                 4 /* File open failed     */
#define E_INVALID_OCLASS                5 /* output class invalid */
#define E_CANNOT_ALLOC                  6 /* cannot allocate space*/
#define E_OLD_STYLE                     7 /* old style C ! allowed*/
#define E_STACK_OUT                     8 /* v r out of stack     */
#define E_INTERNAL_ERROR                9 /* unable to alloc tvar */
#define E_LVALUE_REQUIRED              10 /* lvalue required      */
#define E_TMPFILE_FAILED               11 /* tmpfile creation failed */
#define E_FUNCTION_EXPECTED            12 /* function expected    */
#define E_USING_ERROR                  13 /* using in error       */
#define E_SFR_INIT                     14 /* init error for sbit  */
#define W_INIT_IGNORED                 15 /* initialiser ignored  */
#define E_AUTO_ASSUMED                 16 /* sclass auto assumed  */
#define E_AUTO_ABSA                    17 /* abs addr for auto var*/
#define W_INIT_WRONG                   18 /* initializer type !=  */
#define E_FUNC_REDEF                   19 /* func name redefined  */
#define E_ID_UNDEF                     20 /* identifer undefined  */
#define W_STACK_OVERFLOW               21 /* stack overflow       */
#define E_NEED_ARRAY_PTR               22 /* array or pointer reqd*/
#define E_IDX_NOT_INT                  23 /* index not an integer */
#define W_IDX_OUT_OF_BOUNDS            24 /* array index out of bounds */
#define E_STRUCT_UNION                 25 /* struct,union expected*/
#define E_NOT_MEMBER                   26 /* !struct/union member */
#define E_PTR_REQD                     27 /* pointer required     */
#define E_UNARY_OP                     28 /* unary operator bad op*/
#define E_CONV_ERR                     29 /* conversion error     */
#define E_INT_REQD                     30 /* bit field must be int*/
#define E_BITFLD_SIZE                  31 /* bit field size > 16  */
#define W_TRUNCATION                   32 /* high order trucation */
#define E_CODE_WRITE                   33 /* trying 2 write to code */
#define E_LVALUE_CONST                 34 /* lvalue is a const   */
#define E_ILLEGAL_ADDR                 35 /* address of bit      */
#define E_CAST_ILLEGAL                 36 /* cast illegal        */
#define E_MULT_INTEGRAL                37 /* mult opernd must b integral */
#define E_ARG_ERROR                    38 /* argument count error*/
#define E_ARG_COUNT                    39 /* func expecting more */
#define E_FUNC_EXPECTED                40 /* func name expected  */
#define E_PLUS_INVALID                 41 /* plus invalid        */
#define E_PTR_PLUS_PTR                 42 /* pointer + pointer   */
#define E_SHIFT_OP_INVALID             43 /* shft op op invalid  */
#define E_COMPARE_OP                   44 /* compare operand     */
#define E_BITWISE_OP                   45 /* bit op invalid op   */
#define E_ANDOR_OP                     46 /* && || op invalid    */
#define E_TYPE_MISMATCH                47 /* type mismatch       */
#define E_AGGR_ASSIGN                  48 /* aggr assign         */
#define E_ARRAY_DIRECT                 49 /* array indexing in   */
#define E_BIT_ARRAY                    50 /* bit array not allowed  */
#define E_DUPLICATE_TYPEDEF            51 /* typedef name duplicate */
#define E_ARG_TYPE                     52 /* arg type mismatch   */
#define E_RET_VALUE                    53 /* return value mismatch */
#define E_FUNC_AGGR                    54 /* function returing aggr */
#define E_FUNC_DEF                     55 /* ANSI Style def neede */
#define E_DUPLICATE_LABEL              56 /* duplicate label name */
#define E_LABEL_UNDEF                  57 /* undefined label used */
#define E_FUNC_VOID                    58 /* void func ret value  */
#define W_VOID_FUNC                    59 /* func must return value */
#define W_RETURN_MISMATCH              60 /* return value mismatch */
#define E_CASE_CONTEXT                 61 /* case stmnt without switch */
#define E_CASE_CONSTANT                62 /* case expression ! const */
#define E_BREAK_CONTEXT                63 /* break statement invalid */
#define E_SWITCH_AGGR                  64 /* non integral for switch */
#define E_FUNC_BODY                    65 /* func has body already */
#define E_UNKNOWN_SIZE                 66 /* variable has unknown size */
#define E_AUTO_AGGR_INIT               67 /* auto aggregates no init */
#define E_INIT_COUNT                   68 /* too many initializers */
#define E_INIT_STRUCT                  69 /* struct init wrong   */
#define E_INIT_NON_ADDR                70 /* non address xpr for init */
#define E_INT_DEFINED                  71 /* interrupt already over */
#define E_INT_ARGS                     72 /* interrupt rtn cannot have args */
#define E_INCLUDE_MISSING              73 /* compiler include missing */
#define E_NO_MAIN                      74 /* main function undefined */
#define E_EXTERN_INIT                  75 /* extern variable initialised */
#define E_PRE_PROC_FAILED              76 /* preprocessor failed */
#define E_DUP_FAILED                   77 /* file DUP failed     */
#define E_INCOMPAT_TYPES               78 /* incompatible types casting */
#define W_LOOP_ELIMINATE               79 /* loop eliminated     */
#define W_NO_SIDE_EFFECTS              80 /* expression has no side effects */
#define W_CONST_TOO_LARGE              81 /* constant out of range */
#define W_BAD_COMPARE                  82 /* bad comparison      */
#define E_TERMINATING                  83 /* compiler terminating */
#define W_LOCAL_NOINIT                 84 /* local reference before assignment */
#define W_NO_REFERENCE                 85 /* no reference to local variable */
#define E_OP_UNKNOWN_SIZE              86 /* unknown size for operand */
#define W_LONG_UNSUPPORTED             87 /* 'long' not supported yet */
#define W_LITERAL_GENERIC              88 /* literal being cast to generic pointer */
#define E_SFR_ADDR_RANGE               89 /* sfr address out of range */
#define E_BITVAR_STORAGE               90 /* storage given for 'bit' variable */
#define E_EXTERN_MISMATCH              91 /* extern declaration mismatches */
#define E_NONRENT_ARGS                 92 /* fptr non reentrant has args */
#define W_DOUBLE_UNSUPPORTED           93 /* 'double' not supported yet */
#define W_COMP_RANGE                   94 /* comparison is always %s due to limited range of data type */
#define W_FUNC_NO_RETURN               95 /* no return statement found */
#define W_PRE_PROC_WARNING             96 /* preprocessor generated warning */
#define E_STRUCT_AS_ARG                97 /* structure passed as argument */
#define E_PREV_DEF_CONFLICT            98 /* previous definition conflicts with current */
#define E_CODE_NO_INIT                 99 /* vars in code space must have initializer */
#define E_OPS_INTEGRAL                100 /* operans must be integral for certian assignments */
#define E_TOO_MANY_PARMS              101 /* too many parameters */
#define E_TOO_FEW_PARMS               102 /* to few parameters   */
#define E_FUNC_NO_CODE                103 /* fatalError          */
#define E_TYPE_MISMATCH_PARM          104 /* type mismatch for parameter */
#define E_INVALID_FLOAT_CONST         105 /* invalid floating point literal string */
#define E_INVALID_OP                  106 /* invalid operand for some operation */
#define E_SWITCH_NON_INTEGER          107 /* switch value not integer */
#define E_CASE_NON_INTEGER            108 /* case value not integer */
#define W_FUNC_TOO_LARGE              109 /* function too large  */
#define W_CONTROL_FLOW                110 /* control flow changed due to optimization */
#define W_PTR_TYPE_INVALID            111 /* invalid type specifier for pointer */
#define W_IMPLICIT_FUNC               112 /* function declared implicitly */
#define W_CONTINUE                    113 /* more than one line  */
#define I_EXTENDED_STACK_SPILS        114 /* too many spils occured */
#define W_UNKNOWN_PRAGMA              115 /* #pragma directive unsupported */
#define W_SHIFT_CHANGED               116 /* shift changed to zero */
#define W_UNKNOWN_OPTION              117 /* don't know the option */
#define W_UNSUPP_OPTION               118 /* processor reset has been redifned */
#define W_UNKNOWN_FEXT                119 /* unknown file extension */
#define W_TOO_MANY_SRC                120 /* can only compile one .c file at a time */
#define I_CYCLOMATIC                  121 /* information message */
#define E_DIVIDE_BY_ZERO              122 /* / 0 */
#define E_FUNC_BIT                    123 /* function cannot return bit */
#define E_CAST_ZERO                   124 /* casting to from size zero */
#define W_CONST_RANGE                 125 /* constant too large  */
#define W_CODE_UNREACH                126 /* unreachable code    */
#define W_NONPTR2_GENPTR              127 /* non pointer cast to generic pointer */
#define W_POSSBUG                     128 /* possible code generation error */
#define E_INCOMPAT_PTYPES             129 /* incampatible pointer assignment */
#define W_UNKNOWN_MODEL               130 /* Unknown memory model */
#define E_UNKNOWN_TARGET              131 /* target not defined  */
#define W_INDIR_BANKED                132 /* Indirect call to a banked fun */
#define W_UNSUPPORTED_MODEL           133 /* Unsupported model, ignored */
#define W_BANKED_WITH_NONBANKED       134 /* banked and nonbanked attributes mixed */
#define W_BANKED_WITH_STATIC          135 /* banked and static mixed */
#define W_INT_TO_GEN_PTR_CAST         136 /* Converting integer type to generic pointer. */
#define W_ESC_SEQ_OOR_FOR_CHAR        137 /* Escape sequence of of range for char */
#define E_INVALID_HEX                 138 /* \x used with no following hex digits */
#define W_FUNCPTR_IN_USING_ISR        139 /* Call via function pointer in ISR with using attribute. */
#define E_NO_SUCH_BANK                140 /* 'using' attribute specifies non-existant register bank. */
#define E_TWO_OR_MORE_DATA_TYPES      141
#define E_LONG_OR_SHORT_INVALID       142 /* long or short invalid for .. */
#define E_SIGNED_OR_UNSIGNED_INVALID  143 /* signed or unsigned invalid for .. */
#define E_LONG_AND_SHORT_INVALID      144 /* long and short invalid for .. */
#define E_SIGNED_AND_UNSIGNED_INVALID 145 /* signed and unsigned invalid for .. */
#define E_TWO_OR_MORE_STORAGE_CLASSES 146
#define W_EXCESS_INITIALIZERS         147 /* too much initializers for array */
#define E_ARGUMENT_MISSING            148 /* Option requires an argument. */
#define W_STRAY_BACKSLASH             149
#define W_NEWLINE_IN_STRING           150
#define W_USING_GENERIC_POINTER       151
#define W_EXCESS_SHORT_OPTIONS        152
#define E_VOID_VALUE_USED             153
#define W_INTEGRAL2PTR_NOCAST         154
#define W_PTR2INTEGRAL_NOCAST         155
#define W_SYMBOL_NAME_TOO_LONG        156
#define W_CAST_STRUCT_PTR             157 /* pointer to different structure types */
#define W_LIT_OVERFLOW                158 /* overflow in implicit constant conversion */
#define E_PARAM_NAME_OMITTED          159
#define W_NO_FILE_ARG_IN_C1           160
#define E_NEED_OPT_O_IN_C1            161
#define W_ILLEGAL_OPT_COMBINATION     162
#define E_DUPLICATE_MEMBER            163
#define E_STACK_VIOLATION             164 /* internal stack violation */
#define W_INT_OVL                     165 /* integer overflow in expression */
#define W_USELESS_DECL                166 /* useless declaration */
#define E_INT_BAD_INTNO               167 /* invalid interrupt number */
#define W_BITFLD_NAMED                168 /* declarator used with 0 length bit-field */
#define E_FUNC_ATTR                   169 /* function attribute without function */
#define W_SAVE_RESTORE                170 /* unmatched #pragma SAVE and #pragma RESTORE */
#define E_INVALID_CRITICAL            171 /* operation invalid in critical sequence */
#define E_NOT_ALLOWED                 172 /* %s not allowed here */
#define E_BAD_TAG                     173 /* '%s' is not a %s tag */
#define E_ENUM_NON_INTEGER            174 /* enumeration constant not an integer */
#define W_DEPRECATED_PRAGMA           175 /* deprecated pragma */
#define E_SIZEOF_INCOMPLETE_TYPE      176 /* sizeof applied to an incomplete type */
#define E_PREVIOUS_DEF                177 /* previously defined here */
#define W_SIZEOF_VOID                 178 /* size of void is zero */
#define W_POSSBUG2                    179 /* possible bug, new format */
#define W_COMPLEMENT                  180 /* ~bit can give unexpected results */
#define E_SHADOWREGS_NO_ISR           181 /* shadowregs keyword following non-ISR function */
#define W_SFR_ABSRANGE                182 /* sfr at address out of range */
#define E_BANKED_WITH_CALLEESAVES     183 /* banked and callee-saves mixed */
#define W_INVALID_INT_CONST           184 /* invalid integer literal string */
#define W_CMP_SU_CHAR                 185 /* comparison of 'signed char' with 'unsigned char' requires promotion to int */
#define W_INVALID_FLEXARRAY           186 /* invalid use of structure with flexible array member */
#define W_C89_NO_FLEXARRAY            187 /* ISO C90 does not support flexible array members */
#define E_FLEXARRAY_NOTATEND          188 /* flexible array member not at end of struct */
#define E_FLEXARRAY_INEMPTYSTRCT      189 /* flexible array in otherwise empty struct */
#define W_EMPTY_SOURCE_FILE           190 /* ISO C forbids an empty source file */
#define W_BAD_PRAGMA_ARGUMENTS        191 /* #pragma %s: bad argument(s); pragma ignored */
#define E_BAD_RESTRICT                192 /* Only pointers may be qualified with 'restrict' */
#define E_BAD_INLINE                  193 /* Only functions may be qualified with 'inline' */
#define E_BAD_INT_ARGUMENT            194 /* Bad integer option argument */
#define E_NEGATIVE_ARRAY_SIZE         195 /* Size of array '%s' is negative */
#define W_TARGET_LOST_QUALIFIER       196 /* Pointer target lost qualifier */
#define W_DEPRECATED_KEYWORD          197 /* keyword '%s' is deprecated, use '%s' instead */
#define E_STORAGE_CLASS_FOR_PARAMETER 198 /* storage class specified for parameter '%s' */
#define E_OFFSETOF_TYPE               199 /* offsetof can only be applied to structs/unions */
#define E_INCOMPLETE_FIELD            200 /* struct field has incomplete type */
#define W_DEPRECATED_OPTION           201 /* deprecated compiler option '%s' */
#define E_BAD_DESIGNATOR              202 /* Bad designated initializer */
#define W_DUPLICATE_INIT              203 /* duplicate initializer */

#define MAX_ERROR_WARNING             256 /* size of disable warnings array */

/** sdcc style assertion */
#ifdef assert
#undef assert
#endif
#ifdef NDEBUG
# define assert(expr) (void)0
#else
# define assert(expr) ((expr) ? (void)0 : fatal (1, E_INTERNAL_ERROR, __FILE__, __LINE__, #expr))
#endif

/** Describes the maximum error level that will be logged.  Any level
 *  includes all of the levels listed after it.
 *
 *
 */
enum _ERROR_LOG_LEVEL {
  /** Everything.  Currently the same as PEDANTIC. */
  ERROR_LEVEL_ALL,
  /** All warnings, including those considered 'reasonable to use,
      on occasion, in clean programs' (man 3 gcc). */
  ERROR_LEVEL_PEDANTIC,
  /** 'informational' warnings */
  ERROR_LEVEL_INFO,
  /** Most warnings. */
  ERROR_LEVEL_WARNING,
  /** Errors only. */
  ERROR_LEVEL_ERROR
};

typedef enum _ERROR_LOG_LEVEL ERROR_LOG_LEVEL;

struct SDCCERRG {
  ERROR_LOG_LEVEL logLevel;
  FILE *out;
  int style;                        /* 1=MSVC */
  int werror;                       /* treat the warnings as errors */
  char disabled[MAX_ERROR_WARNING]; /* 1=warning disabled*/
};

extern struct SDCCERRG _SDCCERRG;

/** Sets the maximum error level to log.
    See MAX_ERROR_LEVEL.  The default is ERROR_LEVEL_ALL.
*/
void setErrorLogLevel (ERROR_LOG_LEVEL level);

/*
-------------------------------------------------------------------------------
SetErrorOut - Set the error output file

-------------------------------------------------------------------------------
*/

FILE * SetErrorOut (FILE *NewErrorOut);

/*
-------------------------------------------------------------------------------
vwerror - Output a standard eror message with variable number of arguements

-------------------------------------------------------------------------------
*/

int vwerror (int errNum, va_list marker);

/*
-------------------------------------------------------------------------------
werror - Output a standard eror message with variable number of arguements

-------------------------------------------------------------------------------
*/

int werror (int errNum, ... );

/*
-------------------------------------------------------------------------------
werrorfl - Output a standard eror message with variable number of arguements.
           Use a specified filename and line number instead of the default.

-------------------------------------------------------------------------------
*/

int werrorfl (char *newFilename, int newLineno, int errNum, ...);

/*
-------------------------------------------------------------------------------
fatal - Output a standard eror message with variable number of arguements and
        call exit()
-------------------------------------------------------------------------------
*/

void fatal (int exitCode, int errNum, ... );

/*
-------------------------------------------------------------------------------
style - Change the output error style to MSVC
-------------------------------------------------------------------------------
*/

void MSVC_style (int style);

/*
-------------------------------------------------------------------------------
disabled - Disable output of specified warning
-------------------------------------------------------------------------------
*/

void setWarningDisabled (int errNum);

/*
-------------------------------------------------------------------------------
Set the flag to treat warnings as errors
-------------------------------------------------------------------------------
*/
void setWError (int flag);

#endif
