/*-------------------------------------------------------------------------

	pcode.c - post code generation
	Written By -  Scott Dattalo scott@dattalo.com

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2, or (at your option) any
	later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
-------------------------------------------------------------------------*/

#include <stdio.h>

#include "common.h"   // Include everything in the SDCC src directory
#include "newalloc.h"


#include "pcode.h"
#include "pcodeflow.h"
#include "ralloc.h"
#include "device.h"

pCode *findFunction(char *fname);

static void FixRegisterBanking(pBlock *pb,int cur_bank);

#if defined(__BORLANDC__) || defined(_MSC_VER)
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif

/****************************************************************/
/****************************************************************/

peepCommand peepCommands[] = {
	
	{NOTBITSKIP, "_NOTBITSKIP_"},
	{BITSKIP, "_BITSKIP_"},
	{INVERTBITSKIP, "_INVERTBITSKIP_"},
	
	{-1, NULL}
};



// Eventually this will go into device dependent files:
pCodeOpReg pc_status    = {{PO_STATUS,  "STATUS"}, -1, NULL,0,NULL};
pCodeOpReg pc_indf      = {{PO_INDF,    "INDF"}, -1, NULL,0,NULL};
pCodeOpReg pc_fsr       = {{PO_FSR,     "FSR"}, -1, NULL,0,NULL};
pCodeOpReg pc_intcon    = {{PO_INTCON,  "INTCON"}, -1, NULL,0,NULL};
pCodeOpReg pc_pcl       = {{PO_PCL,     "PCL"}, -1, NULL,0,NULL};
pCodeOpReg pc_pclath    = {{PO_PCLATH,  "PCLATH"}, -1, NULL,0,NULL};

pCodeOpReg pc_wsave     = {{PO_GPR_REGISTER,  "WSAVE"}, -1, NULL,0,NULL};
pCodeOpReg pc_ssave     = {{PO_GPR_REGISTER,  "SSAVE"}, -1, NULL,0,NULL};
pCodeOpReg pc_psave     = {{PO_GPR_REGISTER,  "PSAVE"}, -1, NULL,0,NULL};

static int mnemonics_initialized = 0;

static hTab *pic14MnemonicsHash = NULL;
static hTab *pic14pCodePeepCommandsHash = NULL;


pFile *the_pFile = NULL;
static pBlock *pb_dead_pcodes = NULL;

/* Hardcoded flags to change the behavior of the PIC port */
static int functionInlining = 1;      /* inline functions if nonzero */
int debug_verbose = 0;                /* Set true to inundate .asm file */

// static int GpCodeSequenceNumber = 1;
int GpcFlowSeq = 1;

/* statistics (code size estimation) */
static unsigned int pcode_insns = 0;
static unsigned int pcode_doubles = 0;


unsigned maxIdx; /* This keeps track of the maximum register index for call tree register reuse */
unsigned peakIdx; /* This keeps track of the peak register index for call tree register reuse */

extern void RemoveUnusedRegisters(void);
extern void RegsUnMapLiveRanges(void);
extern void BuildFlowTree(pBlock *pb);
extern void pCodeRegOptimizeRegUsage(int level);
extern int picIsInitialized(void);
extern const char *pCodeOpType(pCodeOp *pcop);

/****************************************************************/
/*                      Forward declarations                    */
/****************************************************************/

void unlinkpCode(pCode *pc);
#if 0
static void genericAnalyze(pCode *pc);
static void AnalyzeGOTO(pCode *pc);
static void AnalyzeSKIP(pCode *pc);
static void AnalyzeRETURN(pCode *pc);
#endif

static void genericDestruct(pCode *pc);
static void genericPrint(FILE *of,pCode *pc);

static void pCodePrintLabel(FILE *of, pCode *pc);
static void pCodePrintFunction(FILE *of, pCode *pc);
static void pCodeOpPrint(FILE *of, pCodeOp *pcop);
static char *get_op_from_instruction( pCodeInstruction *pcc);
char *get_op( pCodeOp *pcop,char *buff,size_t buf_size);
int pCodePeepMatchLine(pCodePeep *peepBlock, pCode *pcs, pCode *pcd);
int pCodePeepMatchRule(pCode *pc);
void pBlockStats(FILE *of, pBlock *pb);
pBlock *newpBlock(void);
pCodeOp *popCopyGPR2Bit(pCodeOp *pc, int bitval);
void pCodeRegMapLiveRanges(pBlock *pb);

pBranch * pBranchAppend(pBranch *h, pBranch *n);


/****************************************************************/
/*                    PIC Instructions                          */
/****************************************************************/

pCodeInstruction pciADDWF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_ADDWF,
		"ADDWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_C | PCC_DC | PCC_Z) // outCond
};

pCodeInstruction pciADDFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_ADDFW,
		"ADDWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_W | PCC_C | PCC_DC | PCC_Z) // outCond
};

pCodeInstruction pciADDLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_ADDLW,
		"ADDLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		(PCC_W | PCC_LITERAL),   // inCond
		(PCC_W | PCC_Z | PCC_C | PCC_DC) // outCond
};

pCodeInstruction pciANDLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_ANDLW,
		"ANDLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		(PCC_W | PCC_LITERAL),   // inCond
		(PCC_W | PCC_Z) // outCond
};

pCodeInstruction pciANDWF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_ANDWF,
		"ANDWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_Z) // outCond
};

pCodeInstruction pciANDFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_ANDFW,
		"ANDWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_W | PCC_Z) // outCond
};

pCodeInstruction pciBCF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_BCF,
		"BCF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,1,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_BSF,
		(PCC_REGISTER | PCC_EXAMINE_PCOP),	// inCond
		(PCC_REGISTER | PCC_EXAMINE_PCOP)	// outCond
};

pCodeInstruction pciBSF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_BSF,
		"BSF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,1,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_BCF,
		(PCC_REGISTER | PCC_EXAMINE_PCOP),	// inCond
		(PCC_REGISTER | PCC_EXAMINE_PCOP)	// outCond
};

pCodeInstruction pciBTFSC = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeSKIP,
		genericDestruct,
		genericPrint},
		POC_BTFSC,
		"BTFSC",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,1,  // dest, bit instruction
		1,1,  // branch, skip
		0,    // literal operand
		POC_BTFSS,
		(PCC_REGISTER | PCC_EXAMINE_PCOP),	// inCond
		PCC_NONE // outCond
};

pCodeInstruction pciBTFSS = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeSKIP,
		genericDestruct,
		genericPrint},
		POC_BTFSS,
		"BTFSS",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,1,  // dest, bit instruction
		1,1,  // branch, skip
		0,    // literal operand
		POC_BTFSC,
		(PCC_REGISTER | PCC_EXAMINE_PCOP),   // inCond
		PCC_NONE // outCond
};

pCodeInstruction pciCALL = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_CALL,
		"CALL",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		1,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_NONE | PCC_W), // inCond, reads argument from WREG
		(PCC_NONE | PCC_W | PCC_C | PCC_DC | PCC_Z)  // outCond, flags are destroyed by called function
};

pCodeInstruction pciCOMF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_COMF,
		"COMF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,  // inCond
		PCC_REGISTER | PCC_Z  // outCond
};

pCodeInstruction pciCOMFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_COMFW,
		"COMF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,  // inCond
		PCC_W | PCC_Z  // outCond
};

pCodeInstruction pciCLRF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_CLRF,
		"CLRF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE, // inCond
		PCC_REGISTER | PCC_Z // outCond
};

pCodeInstruction pciCLRW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_CLRW,
		"CLRW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		0,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE, // inCond
		PCC_W | PCC_Z  // outCond
};

pCodeInstruction pciCLRWDT = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_CLRWDT,
		"CLRWDT",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		0,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE, // inCond
		PCC_NONE  // outCond
};

pCodeInstruction pciDECF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_DECF,
		"DECF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_REGISTER | PCC_Z   // outCond
};

pCodeInstruction pciDECFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_DECFW,
		"DECF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_W | PCC_Z   // outCond
};

pCodeInstruction pciDECFSZ = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeSKIP,
		genericDestruct,
		genericPrint},
		POC_DECFSZ,
		"DECFSZ",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		1,1,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_REGISTER    // outCond
};

pCodeInstruction pciDECFSZW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeSKIP,
		genericDestruct,
		genericPrint},
		POC_DECFSZW,
		"DECFSZ",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		1,1,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_W           // outCond
};

pCodeInstruction pciGOTO = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeGOTO,
		genericDestruct,
		genericPrint},
		POC_GOTO,
		"GOTO",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		1,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE,   // inCond
		PCC_NONE    // outCond
};

pCodeInstruction pciINCF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_INCF,
		"INCF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_REGISTER | PCC_Z   // outCond
};

pCodeInstruction pciINCFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_INCFW,
		"INCF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_W | PCC_Z   // outCond
};

pCodeInstruction pciINCFSZ = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeSKIP,
		genericDestruct,
		genericPrint},
		POC_INCFSZ,
		"INCFSZ",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		1,1,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_REGISTER    // outCond
};

pCodeInstruction pciINCFSZW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeSKIP,
		genericDestruct,
		genericPrint},
		POC_INCFSZW,
		"INCFSZ",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		1,1,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_W           // outCond
};

pCodeInstruction pciIORWF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_IORWF,
		"IORWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_Z) // outCond
};

pCodeInstruction pciIORFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_IORFW,
		"IORWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_W | PCC_Z) // outCond
};

pCodeInstruction pciIORLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_IORLW,
		"IORLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		(PCC_W | PCC_LITERAL),   // inCond
		(PCC_W | PCC_Z) // outCond
};

pCodeInstruction pciMOVF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_MOVF,
		"MOVF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		PCC_Z // outCond
};

pCodeInstruction pciMOVFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_MOVFW,
		"MOVF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_REGISTER,   // inCond
		(PCC_W | PCC_Z) // outCond
};

pCodeInstruction pciMOVWF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_MOVWF,
		"MOVWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_W,   // inCond
		PCC_REGISTER // outCond
};

pCodeInstruction pciMOVLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		genericDestruct,
		genericPrint},
		POC_MOVLW,
		"MOVLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		(PCC_NONE | PCC_LITERAL),   // inCond
		PCC_W // outCond
};

pCodeInstruction pciNOP = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		genericDestruct,
		genericPrint},
		POC_NOP,
		"NOP",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		0,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE,   // inCond
		PCC_NONE // outCond
};

pCodeInstruction pciRETFIE = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeRETURN,
		genericDestruct,
		genericPrint},
		POC_RETFIE,
		"RETFIE",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		0,    // num ops
		0,0,  // dest, bit instruction
		1,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE,   // inCond
		(PCC_NONE | PCC_C | PCC_DC | PCC_Z) // outCond (not true... affects the GIE bit too), STATUS bit are retored
};

pCodeInstruction pciRETLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeRETURN,
		genericDestruct,
		genericPrint},
		POC_RETLW,
		"RETLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		1,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		PCC_LITERAL,   // inCond
		(PCC_W| PCC_C | PCC_DC | PCC_Z) // outCond, STATUS bits are irrelevant after RETLW
};

pCodeInstruction pciRETURN = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   AnalyzeRETURN,
		genericDestruct,
		genericPrint},
		POC_RETURN,
		"RETURN",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		0,    // num ops
		0,0,  // dest, bit instruction
		1,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE | PCC_W,   // inCond, return value is possibly present in W
		(PCC_NONE | PCC_C | PCC_DC | PCC_Z) // outCond, STATUS bits are irrelevant after RETURN
};

pCodeInstruction pciRLF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_RLF,
		"RLF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_C | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_C ) // outCond
};

pCodeInstruction pciRLFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_RLFW,
		"RLF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_C | PCC_REGISTER),   // inCond
		(PCC_W | PCC_C) // outCond
};

pCodeInstruction pciRRF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_RRF,
		"RRF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_C | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_C) // outCond
};

pCodeInstruction pciRRFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_RRFW,
		"RRF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_C | PCC_REGISTER),   // inCond
		(PCC_W | PCC_C) // outCond
};

pCodeInstruction pciSUBWF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_SUBWF,
		"SUBWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_C | PCC_DC | PCC_Z) // outCond
};

pCodeInstruction pciSUBFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_SUBFW,
		"SUBWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_W | PCC_C | PCC_DC | PCC_Z) // outCond
};

pCodeInstruction pciSUBLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_SUBLW,
		"SUBLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		(PCC_W | PCC_LITERAL),   // inCond
		(PCC_W | PCC_Z | PCC_C | PCC_DC) // outCond
};

pCodeInstruction pciSWAPF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_SWAPF,
		"SWAPF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_REGISTER),   // inCond
		(PCC_REGISTER) // outCond
};

pCodeInstruction pciSWAPFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_SWAPFW,
		"SWAPF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_REGISTER),   // inCond
		(PCC_W) // outCond
};

pCodeInstruction pciTRIS = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_TRIS,
		"TRIS",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE,   // inCond /* FIXME: what's TRIS doing? */
		PCC_REGISTER // outCond	/* FIXME: what's TIS doing */
};

pCodeInstruction pciXORWF = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_XORWF,
		"XORWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		1,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_REGISTER | PCC_Z) // outCond
};

pCodeInstruction pciXORFW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_XORFW,
		"XORWF",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		2,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		(PCC_W | PCC_REGISTER),   // inCond
		(PCC_W | PCC_Z) // outCond
};

pCodeInstruction pciXORLW = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_XORLW,
		"XORLW",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		1,    // literal operand
		POC_NOP,
		(PCC_W | PCC_LITERAL),   // inCond
		(PCC_W | PCC_Z) // outCond
};


pCodeInstruction pciBANKSEL = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_BANKSEL,
		"BANKSEL",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE, // inCond
		PCC_NONE  // outCond
};

pCodeInstruction pciPAGESEL = {
	{PC_OPCODE, NULL, NULL, 0, 0, NULL, 
		//   genericAnalyze,
		genericDestruct,
		genericPrint},
		POC_PAGESEL,
		"PAGESEL",
		NULL, // from branch
		NULL, // to branch
		NULL, // label
		NULL, // operand
		NULL, // flow block
		NULL, // C source 
		1,    // num ops
		0,0,  // dest, bit instruction
		0,0,  // branch, skip
		0,    // literal operand
		POC_NOP,
		PCC_NONE, // inCond
		PCC_NONE  // outCond
};

pCodeInstruction *pic14Mnemonics[MAX_PIC14MNEMONICS];


/*-----------------------------------------------------------------*/
/* return a unique ID number to assist pCodes debuging             */
/*-----------------------------------------------------------------*/
unsigned PCodeID(void) {
	static unsigned int pcodeId = 1; /* unique ID number to be assigned to all pCodes */
	/*
	static unsigned int stop;
	if (pcodeId == 1448)
		stop++; // Place break point here
	*/
	return pcodeId++;
}

#ifdef HAVE_VSNPRINTF
// Alas, vsnprintf is not ANSI standard, and does not exist
// on Solaris (and probably other non-Gnu flavored Unixes).

/*-----------------------------------------------------------------*/
/* SAFE_snprintf - like snprintf except the string pointer is      */
/*                 after the string has been printed to. This is   */
/*                 useful for printing to string as though if it   */
/*                 were a stream.                                  */
/*-----------------------------------------------------------------*/
void SAFE_snprintf(char **str, size_t *size, const  char  *format, ...)
{
	va_list val;
	int len;
	
	if(!str || !*str)
		return;
	
	va_start(val, format);
	
	vsnprintf(*str, *size, format, val);
	
	va_end (val);
	
	len = strlen(*str);
	if((size_t)len > *size) {
		fprintf(stderr,"WARNING, it looks like %s has overflowed\n",__FUNCTION__);
		fprintf(stderr,"len = %d is > str size %d\n",len,(int)*size);
	}
	
	*str += len;
	*size -= len;
	
}

#else  //  HAVE_VSNPRINTF

// This version is *not* safe, despite the name.

void SAFE_snprintf(char **str, size_t *size, const  char  *format, ...)
{
	va_list val;
	int len;
	static char buffer[1024]; /* grossly conservative, but still not inherently safe */
	
	if(!str || !*str)
		return;
	
	va_start(val, format);
	
	vsprintf(buffer, format, val);
	va_end (val);
	
	len = strlen(buffer);
	if(len > *size) {
		fprintf(stderr,"WARNING, it looks like %s has overflowed\n",__FUNCTION__);
		fprintf(stderr,"len = %d is > str size %d\n",len,*size);
	}
	
	strcpy(*str, buffer);
	*str += len;
	*size -= len;
	
}

#endif    //  HAVE_VSNPRINTF


extern  void initStack(int base_address, int size);
extern regs *allocProcessorRegister(int rIdx, char * name, short po_type, int alias);
extern regs *allocInternalRegister(int rIdx, char * name, short po_type, int alias);
extern void init_pic(char *);

void  pCodeInitRegisters(void)
{
	static int initialized=0;
	int shareBankAddress,stkSize;
	
	if(initialized)
		return;
	initialized = 1;
	
	init_pic(port->processor);
	shareBankAddress = 0x7f; /* FIXME - some PIC ICs like 16C7X which do not have a shared bank need a different approach. */
	if ((unsigned)shareBankAddress > getMaxRam()) /* If total RAM is less than 0x7f as with 16f84 then reduce shareBankAddress to fit */
		shareBankAddress = (int)getMaxRam();
	stkSize = 15; /* Set pseudo stack size to 15, on multi memory bank ICs this leaves room for WSAVE (used for interrupts) to fit into the shared portion of the memory bank */
	initStack(shareBankAddress, stkSize); /* Putting the pseudo stack in shared memory so all modules use the same register when passing fn parameters */
	
	pc_status.r = allocProcessorRegister(IDX_STATUS,"STATUS", PO_STATUS, 0x180);
	pc_pcl.r = allocProcessorRegister(IDX_PCL,"PCL", PO_PCL, 0x80);
	pc_pclath.r = allocProcessorRegister(IDX_PCLATH,"PCLATH", PO_PCLATH, 0x180);
	pc_fsr.r = allocProcessorRegister(IDX_FSR,"FSR", PO_FSR, 0x180);
	pc_indf.r = allocProcessorRegister(IDX_INDF,"INDF", PO_INDF, 0x180);
	pc_intcon.r = allocProcessorRegister(IDX_INTCON,"INTCON", PO_INTCON, 0x180);
	
	pc_status.rIdx = IDX_STATUS;
	pc_fsr.rIdx = IDX_FSR;
	pc_indf.rIdx = IDX_INDF;
	pc_intcon.rIdx = IDX_INTCON;
	pc_pcl.rIdx = IDX_PCL;
	pc_pclath.rIdx = IDX_PCLATH;
	
	pc_wsave.r = allocInternalRegister(IDX_WSAVE,pc_wsave.pcop.name,pc_wsave.pcop.type, 0x180); /* Interrupt storage for working register - must be same address in all banks ie section SHAREBANK. */
	pc_ssave.r = allocInternalRegister(IDX_SSAVE,pc_ssave.pcop.name,pc_ssave.pcop.type, 0); /* Interrupt storage for status register. */
	pc_psave.r = allocInternalRegister(IDX_PSAVE,pc_psave.pcop.name,pc_psave.pcop.type, 0); /* Interrupt storage for pclath register. */
	
	pc_wsave.rIdx = pc_wsave.r->rIdx;
	pc_ssave.rIdx = pc_ssave.r->rIdx;
	pc_psave.rIdx = pc_psave.r->rIdx;
	
	pc_wsave.r->isFixed = 1; /* Some PIC ICs do not have a sharebank - this register needs to be reserved across all banks. */
	pc_wsave.r->address = shareBankAddress-stkSize;
	pc_ssave.r->isFixed = 1; /* This register must be in the first bank. */
	pc_ssave.r->address = shareBankAddress-stkSize-1;
	pc_psave.r->isFixed = 1; /* This register must be in the first bank. */
	pc_psave.r->address = shareBankAddress-stkSize-2;
	
	/* probably should put this in a separate initialization routine */
	pb_dead_pcodes = newpBlock();
	
}

/*-----------------------------------------------------------------*/
/*  mnem2key - convert a pic mnemonic into a hash key              */
/*   (BTW - this spreads the mnemonics quite well)                 */
/*                                                                 */
/*-----------------------------------------------------------------*/

int mnem2key(unsigned char const *mnem)
{
	int key = 0;
	
	if(!mnem)
		return 0;
	
	while(*mnem) {
		
		key += toupper(*mnem++) +1;
		
	}
	
	return (key & 0x1f);
	
}

void pic14initMnemonics(void)
{
	int i = 0;
	int key;
	//  char *str;
	pCodeInstruction *pci;
	
	if(mnemonics_initialized)
		return;
	
	//FIXME - probably should NULL out the array before making the assignments
	//since we check the array contents below this initialization.
	
	pic14Mnemonics[POC_ADDLW] = &pciADDLW;
	pic14Mnemonics[POC_ADDWF] = &pciADDWF;
	pic14Mnemonics[POC_ADDFW] = &pciADDFW;
	pic14Mnemonics[POC_ANDLW] = &pciANDLW;
	pic14Mnemonics[POC_ANDWF] = &pciANDWF;
	pic14Mnemonics[POC_ANDFW] = &pciANDFW;
	pic14Mnemonics[POC_BCF] = &pciBCF;
	pic14Mnemonics[POC_BSF] = &pciBSF;
	pic14Mnemonics[POC_BTFSC] = &pciBTFSC;
	pic14Mnemonics[POC_BTFSS] = &pciBTFSS;
	pic14Mnemonics[POC_CALL] = &pciCALL;
	pic14Mnemonics[POC_COMF] = &pciCOMF;
	pic14Mnemonics[POC_COMFW] = &pciCOMFW;
	pic14Mnemonics[POC_CLRF] = &pciCLRF;
	pic14Mnemonics[POC_CLRW] = &pciCLRW;
	pic14Mnemonics[POC_CLRWDT] = &pciCLRWDT;
	pic14Mnemonics[POC_DECF] = &pciDECF;
	pic14Mnemonics[POC_DECFW] = &pciDECFW;
	pic14Mnemonics[POC_DECFSZ] = &pciDECFSZ;
	pic14Mnemonics[POC_DECFSZW] = &pciDECFSZW;
	pic14Mnemonics[POC_GOTO] = &pciGOTO;
	pic14Mnemonics[POC_INCF] = &pciINCF;
	pic14Mnemonics[POC_INCFW] = &pciINCFW;
	pic14Mnemonics[POC_INCFSZ] = &pciINCFSZ;
	pic14Mnemonics[POC_INCFSZW] = &pciINCFSZW;
	pic14Mnemonics[POC_IORLW] = &pciIORLW;
	pic14Mnemonics[POC_IORWF] = &pciIORWF;
	pic14Mnemonics[POC_IORFW] = &pciIORFW;
	pic14Mnemonics[POC_MOVF] = &pciMOVF;
	pic14Mnemonics[POC_MOVFW] = &pciMOVFW;
	pic14Mnemonics[POC_MOVLW] = &pciMOVLW;
	pic14Mnemonics[POC_MOVWF] = &pciMOVWF;
	pic14Mnemonics[POC_NOP] = &pciNOP;
	pic14Mnemonics[POC_RETFIE] = &pciRETFIE;
	pic14Mnemonics[POC_RETLW] = &pciRETLW;
	pic14Mnemonics[POC_RETURN] = &pciRETURN;
	pic14Mnemonics[POC_RLF] = &pciRLF;
	pic14Mnemonics[POC_RLFW] = &pciRLFW;
	pic14Mnemonics[POC_RRF] = &pciRRF;
	pic14Mnemonics[POC_RRFW] = &pciRRFW;
	pic14Mnemonics[POC_SUBLW] = &pciSUBLW;
	pic14Mnemonics[POC_SUBWF] = &pciSUBWF;
	pic14Mnemonics[POC_SUBFW] = &pciSUBFW;
	pic14Mnemonics[POC_SWAPF] = &pciSWAPF;
	pic14Mnemonics[POC_SWAPFW] = &pciSWAPFW;
	pic14Mnemonics[POC_TRIS] = &pciTRIS;
	pic14Mnemonics[POC_XORLW] = &pciXORLW;
	pic14Mnemonics[POC_XORWF] = &pciXORWF;
	pic14Mnemonics[POC_XORFW] = &pciXORFW;
	pic14Mnemonics[POC_BANKSEL] = &pciBANKSEL;
	pic14Mnemonics[POC_PAGESEL] = &pciPAGESEL;
	
	for(i=0; i<MAX_PIC14MNEMONICS; i++)
		if(pic14Mnemonics[i])
			hTabAddItem(&pic14MnemonicsHash, mnem2key(pic14Mnemonics[i]->mnemonic), pic14Mnemonics[i]);
		pci = hTabFirstItem(pic14MnemonicsHash, &key);
		
		while(pci) {
			DFPRINTF((stderr, "element %d key %d, mnem %s\n",i++,key,pci->mnemonic));
			pci = hTabNextItem(pic14MnemonicsHash, &key);
		}
		
		mnemonics_initialized = 1;
}

int getpCodePeepCommand(char *cmd);

int getpCode(char *mnem,unsigned dest)
{
	
	pCodeInstruction *pci;
	int key = mnem2key(mnem);
	
	if(!mnemonics_initialized)
		pic14initMnemonics();
	
	pci = hTabFirstItemWK(pic14MnemonicsHash, key);
	
	while(pci) {
		
		if(STRCASECMP(pci->mnemonic, mnem) == 0) {
			if((pci->num_ops <= 1) || (pci->isModReg == dest) || (pci->isBitInst))
				return(pci->op);
		}
		
		pci = hTabNextItemWK (pic14MnemonicsHash);
		
	}
	
	return -1;
}

/*-----------------------------------------------------------------*
* pic14initpCodePeepCommands
*
*-----------------------------------------------------------------*/
void pic14initpCodePeepCommands(void)
{
	
	int key, i;
	peepCommand *pcmd;
	
	i = 0;
	do {
		hTabAddItem(&pic14pCodePeepCommandsHash, 
			mnem2key(peepCommands[i].cmd), &peepCommands[i]);
		i++;
	} while (peepCommands[i].cmd);
	
	pcmd = hTabFirstItem(pic14pCodePeepCommandsHash, &key);
	
	while(pcmd) {
		//fprintf(stderr, "peep command %s  key %d\n",pcmd->cmd,pcmd->id);
		pcmd = hTabNextItem(pic14pCodePeepCommandsHash, &key);
	}
	
}

/*-----------------------------------------------------------------
*
*
*-----------------------------------------------------------------*/

int getpCodePeepCommand(char *cmd)
{
	
	peepCommand *pcmd;
	int key = mnem2key(cmd);
	
	
	pcmd = hTabFirstItemWK(pic14pCodePeepCommandsHash, key);
	
	while(pcmd) {
		// fprintf(stderr," comparing %s to %s\n",pcmd->cmd,cmd);
		if(STRCASECMP(pcmd->cmd, cmd) == 0) {
			return pcmd->id;
		}
		
		pcmd = hTabNextItemWK (pic14pCodePeepCommandsHash);
		
	}
	
	return -1;
}

char getpBlock_dbName(pBlock *pb)
{
	if(!pb)
		return 0;
	
	if(pb->cmemmap)
		return pb->cmemmap->dbName;
	
	return pb->dbName;
}
void pBlockConvert2ISR(pBlock *pb)
{
	if(!pb)
		return;
	
	if(pb->cmemmap)
		pb->cmemmap = NULL;
	
	pb->dbName = 'I';
}

/*-----------------------------------------------------------------*/
/* movepBlock2Head - given the dbname of a pBlock, move all        */
/*                   instances to the front of the doubly linked   */
/*                   list of pBlocks                               */
/*-----------------------------------------------------------------*/

void movepBlock2Head(char dbName)
{
	pBlock *pb;
	
	if (!the_pFile)
		return;
	
	pb = the_pFile->pbHead;
	
	while(pb) {
		
		if(getpBlock_dbName(pb) == dbName) {
			pBlock *pbn = pb->next;
			pb->next = the_pFile->pbHead;
			the_pFile->pbHead->prev = pb;
			the_pFile->pbHead = pb;
			
			if(pb->prev)
				pb->prev->next = pbn;
			
			// If the pBlock that we just moved was the last
			// one in the link of all of the pBlocks, then we
			// need to point the tail to the block just before
			// the one we moved.
			// Note: if pb->next is NULL, then pb must have 
			// been the last pBlock in the chain.
			
			if(pbn)
				pbn->prev = pb->prev;
			else
				the_pFile->pbTail = pb->prev;
			
			pb = pbn;
			
		} else
			pb = pb->next;
		
	}
	
}

void copypCode(FILE *of, char dbName)
{
	pBlock *pb;
	
	if(!of || !the_pFile)
		return;

	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		if(getpBlock_dbName(pb) == dbName) {
			pBlockStats(of,pb);
			printpBlock(of,pb);
			fprintf (of, "\n");
		}
	}
	
}

void resetpCodeStatistics (void)
{
  pcode_insns = pcode_doubles = 0;
}

void dumppCodeStatistics (FILE *of)
{
	/* dump statistics */
	fprintf (of, "\n");
	fprintf (of, ";\tcode size estimation:\n");
	fprintf (of, ";\t%5u+%5u = %5u instructions (%5u byte)\n", pcode_insns, pcode_doubles, pcode_insns + pcode_doubles, 2*(pcode_insns + 2*pcode_doubles));
	fprintf (of, "\n");
}

void pcode_test(void)
{
	
	DFPRINTF((stderr,"pcode is alive!\n"));
	
	//initMnemonics();
	
	if(the_pFile) {
		
		pBlock *pb;
		FILE *pFile;
		char buffer[100];
		
		/* create the file name */
		strcpy(buffer,dstFileName);
		strcat(buffer,".p");
		
		if( !(pFile = fopen(buffer, "w" ))) {
			werror(E_FILE_OPEN_ERR,buffer);
			exit(1);
		}
		
		fprintf(pFile,"pcode dump\n\n");
		
		for(pb = the_pFile->pbHead; pb; pb = pb->next) {
			fprintf(pFile,"\n\tNew pBlock\n\n");
			if(pb->cmemmap)
				fprintf(pFile,"%s",pb->cmemmap->sname);
			else
				fprintf(pFile,"internal pblock");
			
			fprintf(pFile,", dbName =%c\n",getpBlock_dbName(pb));
			printpBlock(pFile,pb);
		}
	}
}
/*-----------------------------------------------------------------*/
/* int RegCond(pCodeOp *pcop) - if pcop points to the STATUS reg-  */
/*      ister, RegCond will return the bit being referenced.       */
/*                                                                 */
/* fixme - why not just OR in the pcop bit field                   */
/*-----------------------------------------------------------------*/

static int RegCond(pCodeOp *pcop)
{
	
	if(!pcop)
		return 0;
	
	if (pcop->type == PO_GPR_BIT) {
		char *name = pcop->name;
		if (!name) 
			name = PCOR(pcop)->r->name;
		if (strcmp(name, pc_status.pcop.name) == 0)
		{
			switch(PCORB(pcop)->bit) {
			case PIC_C_BIT:
				return PCC_C;
			case PIC_DC_BIT:
				return PCC_DC;
			case PIC_Z_BIT:
				return PCC_Z;
			}
		}
	}
	
	return 0;
}

/*-----------------------------------------------------------------*/
/* newpCode - create and return a newly initialized pCode          */
/*                                                                 */
/*  fixme - rename this                                            */
/*                                                                 */
/* The purpose of this routine is to create a new Instruction      */
/* pCode. This is called by gen.c while the assembly code is being */
/* generated.                                                      */
/*                                                                 */
/* Inouts:                                                         */
/*  PIC_OPCODE op - the assembly instruction we wish to create.    */
/*                  (note that the op is analogous to but not the  */
/*                  same thing as the opcode of the instruction.)  */
/*  pCdoeOp *pcop - pointer to the operand of the instruction.     */
/*                                                                 */
/* Outputs:                                                        */
/*  a pointer to the new malloc'd pCode is returned.               */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*-----------------------------------------------------------------*/
pCode *newpCode (PIC_OPCODE op, pCodeOp *pcop)
{
	pCodeInstruction *pci ;
	
	if(!mnemonics_initialized)
		pic14initMnemonics();
	
	pci = Safe_calloc(1, sizeof(pCodeInstruction));
	
	if((op>=0) && (op < MAX_PIC14MNEMONICS) && pic14Mnemonics[op]) {
		memcpy(pci, pic14Mnemonics[op], sizeof(pCodeInstruction));
		pci->pc.id = PCodeID();
		pci->pcop = pcop;
		
		if(pci->inCond & PCC_EXAMINE_PCOP)
			pci->inCond  |= RegCond(pcop);
		
		if(pci->outCond & PCC_EXAMINE_PCOP)
			pci->outCond  |= RegCond(pcop);
		
		pci->pc.prev = pci->pc.next = NULL;
		return (pCode *)pci;
	}
	
	fprintf(stderr, "pCode mnemonic error %s,%d\n",__FUNCTION__,__LINE__);
	exit(1);
	
	return NULL;
}       

/*-----------------------------------------------------------------*/
/* newpCodeWild - create a "wild" as in wild card pCode            */
/*                                                                 */
/* Wild pcodes are used during the peep hole optimizer to serve    */
/* as place holders for any instruction. When a snippet of code is */
/* compared to a peep hole rule, the wild card opcode will match   */
/* any instruction. However, the optional operand and label are    */
/* additional qualifiers that must also be matched before the      */
/* line (of assembly code) is declared matched. Note that the      */
/* operand may be wild too.                                        */
/*                                                                 */
/*   Note, a wild instruction is specified just like a wild var:   */
/*      %4     ; A wild instruction,                               */
/*  See the peeph.def file for additional examples                 */
/*                                                                 */
/*-----------------------------------------------------------------*/

pCode *newpCodeWild(int pCodeID, pCodeOp *optional_operand, pCodeOp *optional_label)
{
	
	pCodeWild *pcw;
	
	pcw = Safe_calloc(1,sizeof(pCodeWild));
	
	pcw->pci.pc.type = PC_WILD;
	pcw->pci.pc.prev = pcw->pci.pc.next = NULL;
	pcw->id = PCodeID();
	pcw->pci.from = pcw->pci.to = pcw->pci.label = NULL;
	pcw->pci.pc.pb = NULL;
	
	//  pcw->pci.pc.analyze = genericAnalyze;
	pcw->pci.pc.destruct = genericDestruct;
	pcw->pci.pc.print = genericPrint;
	
	pcw->id = pCodeID;              // this is the 'n' in %n
	pcw->operand = optional_operand;
	pcw->label   = optional_label;
	
	pcw->mustBeBitSkipInst = 0;
	pcw->mustNotBeBitSkipInst = 0;
	pcw->invertBitSkipInst = 0;
	
	return ( (pCode *)pcw);
	
}

/*-----------------------------------------------------------------*/
/* newPcodeInlineP - create a new pCode from a char string           */
/*-----------------------------------------------------------------*/


pCode *newpCodeInlineP(char *cP)
{
	
	pCodeComment *pcc ;
	
	pcc = Safe_calloc(1,sizeof(pCodeComment));
	
	pcc->pc.type = PC_INLINE;
	pcc->pc.prev = pcc->pc.next = NULL;
	pcc->pc.id = PCodeID();
	//pcc->pc.from = pcc->pc.to = pcc->pc.label = NULL;
	pcc->pc.pb = NULL;
	
	//  pcc->pc.analyze = genericAnalyze;
	pcc->pc.destruct = genericDestruct;
	pcc->pc.print = genericPrint;
	
	if(cP)
		pcc->comment = Safe_strdup(cP);
	else
		pcc->comment = NULL;
	
	return ( (pCode *)pcc);
	
}

/*-----------------------------------------------------------------*/
/* newPcodeCharP - create a new pCode from a char string           */
/*-----------------------------------------------------------------*/

pCode *newpCodeCharP(char *cP)
{
	
	pCodeComment *pcc ;
	
	pcc = Safe_calloc(1,sizeof(pCodeComment));
	
	pcc->pc.type = PC_COMMENT;
	pcc->pc.prev = pcc->pc.next = NULL;
	pcc->pc.id = PCodeID();
	//pcc->pc.from = pcc->pc.to = pcc->pc.label = NULL;
	pcc->pc.pb = NULL;
	
	//  pcc->pc.analyze = genericAnalyze;
	pcc->pc.destruct = genericDestruct;
	pcc->pc.print = genericPrint;
	
	if(cP)
		pcc->comment = Safe_strdup(cP);
	else
		pcc->comment = NULL;
	
	return ( (pCode *)pcc);
	
}

/*-----------------------------------------------------------------*/
/* newpCodeFunction -                                              */
/*-----------------------------------------------------------------*/


pCode *newpCodeFunction(char *mod,char *f,int isPublic)
{
	pCodeFunction *pcf;
	
	pcf = Safe_calloc(1,sizeof(pCodeFunction));
	//_ALLOC(pcf,sizeof(pCodeFunction));
	
	pcf->pc.type = PC_FUNCTION;
	pcf->pc.prev = pcf->pc.next = NULL;
	pcf->pc.id = PCodeID();
	//pcf->pc.from = pcf->pc.to = pcf->pc.label = NULL;
	pcf->pc.pb = NULL;
	
	//  pcf->pc.analyze = genericAnalyze;
	pcf->pc.destruct = genericDestruct;
	pcf->pc.print = pCodePrintFunction;
	
	pcf->ncalled = 0;
	
	if(mod) {
		//_ALLOC_ATOMIC(pcf->modname,strlen(mod)+1);
		pcf->modname = Safe_calloc(1,strlen(mod)+1);
		strcpy(pcf->modname,mod);
	} else
		pcf->modname = NULL;
	
	if(f) {
		//_ALLOC_ATOMIC(pcf->fname,strlen(f)+1);
		pcf->fname = Safe_calloc(1,strlen(f)+1);
		strcpy(pcf->fname,f);
	} else
		pcf->fname = NULL;
	
	pcf->isPublic = (unsigned)isPublic;
	
	return ( (pCode *)pcf);
	
}

/*-----------------------------------------------------------------*/
/* newpCodeFlow                                                    */
/*-----------------------------------------------------------------*/
void destructpCodeFlow(pCode *pc)
{
	if(!pc || !isPCFL(pc))
		return;
	
		/*
		if(PCFL(pc)->from)
		if(PCFL(pc)->to)
	*/
	unlinkpCode(pc);
	
	deleteSet(&PCFL(pc)->registers);
	deleteSet(&PCFL(pc)->from);
	deleteSet(&PCFL(pc)->to);
	free(pc);
	
}

pCode *newpCodeFlow(void )
{
	pCodeFlow *pcflow;
	
	//_ALLOC(pcflow,sizeof(pCodeFlow));
	pcflow = Safe_calloc(1,sizeof(pCodeFlow));
	
	pcflow->pc.type = PC_FLOW;
	pcflow->pc.prev = pcflow->pc.next = NULL;
	pcflow->pc.pb = NULL;
	
	//  pcflow->pc.analyze = genericAnalyze;
	pcflow->pc.destruct = destructpCodeFlow;
	pcflow->pc.print = genericPrint;
	
	pcflow->pc.seq = GpcFlowSeq++;
	
	pcflow->from = pcflow->to = NULL;
	
	pcflow->inCond = PCC_NONE;
	pcflow->outCond = PCC_NONE;
	
	pcflow->firstBank = 'U'; /* Undetermined */
	pcflow->lastBank = 'U'; /* Undetermined */
	
	pcflow->FromConflicts = 0;
	pcflow->ToConflicts = 0;
	
	pcflow->end = NULL;
	
	pcflow->registers = newSet();
	
	return ( (pCode *)pcflow);
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
pCodeFlowLink *newpCodeFlowLink(pCodeFlow *pcflow)
{
	pCodeFlowLink *pcflowLink;
	
	pcflowLink = Safe_calloc(1,sizeof(pCodeFlowLink));
	
	pcflowLink->pcflow = pcflow;
	pcflowLink->bank_conflict = 0;
	
	return pcflowLink;
}

/*-----------------------------------------------------------------*/
/* newpCodeCSource - create a new pCode Source Symbol              */
/*-----------------------------------------------------------------*/

pCode *newpCodeCSource(int ln, char *f, const char *l)
{
	
	pCodeCSource *pccs;
	
	pccs = Safe_calloc(1,sizeof(pCodeCSource));
	
	pccs->pc.type = PC_CSOURCE;
	pccs->pc.prev = pccs->pc.next = NULL;
	pccs->pc.id = PCodeID();
	pccs->pc.pb = NULL;
	
	pccs->pc.destruct = genericDestruct;
	pccs->pc.print = genericPrint;
	
	pccs->line_number = ln;
	if(l)
		pccs->line = Safe_strdup(l);
	else
		pccs->line = NULL;
	
	if(f)
		pccs->file_name = Safe_strdup(f);
	else
		pccs->file_name = NULL;
	
	return ( (pCode *)pccs);
	
}

/*******************************************************************/
/* pic16_newpCodeAsmDir - create a new pCode Assembler Directive   */
/*                        added by VR 6-Jun-2003                   */
/*******************************************************************/

pCode *newpCodeAsmDir(char *asdir, char *argfmt, ...)
{
  pCodeAsmDir *pcad;
  va_list ap;
  char buffer[512];
  char *lbp=buffer;

  pcad = Safe_calloc(1, sizeof(pCodeAsmDir));
  pcad->pci.pc.type = PC_ASMDIR;
  pcad->pci.pc.prev = pcad->pci.pc.next = NULL;
  pcad->pci.pc.pb = NULL;
  pcad->pci.pc.destruct = genericDestruct;
  pcad->pci.pc.print = genericPrint;

  if(asdir && *asdir) {

    while(isspace((unsigned char)*asdir))asdir++;	// strip any white space from the beginning

    pcad->directive = Safe_strdup( asdir );
  }

  va_start(ap, argfmt);

  memset(buffer, 0, sizeof(buffer));
  if(argfmt && *argfmt)
    vsprintf(buffer, argfmt, ap);

  va_end(ap);

  while(isspace((unsigned char)*lbp))lbp++;

  if(lbp && *lbp)
    pcad->arg = Safe_strdup( lbp );

  return ((pCode *)pcad);
}

/*-----------------------------------------------------------------*/
/* pCodeLabelDestruct - free memory used by a label.               */
/*-----------------------------------------------------------------*/
static void pCodeLabelDestruct(pCode *pc)
{
	
	if(!pc)
		return;
	
	if((pc->type == PC_LABEL) && PCL(pc)->label)
		free(PCL(pc)->label);
	
	free(pc);
	
}

pCode *newpCodeLabel(char *name, int key)
{
	
	char *s = buffer;
	pCodeLabel *pcl;
	
	pcl = Safe_calloc(1,sizeof(pCodeLabel) );
	
	pcl->pc.type = PC_LABEL;
	pcl->pc.prev = pcl->pc.next = NULL;
	pcl->pc.id = PCodeID();
	//pcl->pc.from = pcl->pc.to = pcl->pc.label = NULL;
	pcl->pc.pb = NULL;
	
	//  pcl->pc.analyze = genericAnalyze;
	pcl->pc.destruct = pCodeLabelDestruct;
	pcl->pc.print = pCodePrintLabel;
	
	pcl->key = key;
	
	pcl->label = NULL;
	if(key>0) {
		sprintf(s,"_%05d_DS_",key);
	} else
		s = name;
	
	if(s)
		pcl->label = Safe_strdup(s);
	
	//fprintf(stderr,"newpCodeLabel: key=%d, name=%s\n",key, ((s)?s:""));
	return ( (pCode *)pcl);
	
}


/*-----------------------------------------------------------------*/
/* newpBlock - create and return a pointer to a new pBlock         */
/*-----------------------------------------------------------------*/
pBlock *newpBlock(void)
{
	
	pBlock *PpB;
	
	PpB = Safe_calloc(1,sizeof(pBlock) );
	PpB->next = PpB->prev = NULL;
	
	PpB->function_entries = PpB->function_exits = PpB->function_calls = NULL;
	PpB->tregisters = NULL;
	PpB->visited = 0;
	PpB->FlowTree = NULL;
	
	return PpB;
	
}

/*-----------------------------------------------------------------*/
/* newpCodeChain - create a new chain of pCodes                    */
/*-----------------------------------------------------------------*
*
*  This function will create a new pBlock and the pointer to the
*  pCode that is passed in will be the first pCode in the block.
*-----------------------------------------------------------------*/


pBlock *newpCodeChain(memmap *cm,char c, pCode *pc)
{
	
	pBlock *pB  = newpBlock();
	
	pB->pcHead  = pB->pcTail = pc;
	pB->cmemmap = cm;
	pB->dbName  = c;
	
	return pB;
}

/*-----------------------------------------------------------------*/
/* newpCodeOpLabel - Create a new label given the key              */
/*  Note, a negative key means that the label is part of wild card */
/*  (and hence a wild card label) used in the pCodePeep            */
/*   optimizations).                                               */
/*-----------------------------------------------------------------*/

pCodeOp *newpCodeOpLabel(char *name, int key)
{
	char *s=NULL;
	static int label_key=-1;
	
	pCodeOp *pcop;
	
	pcop = Safe_calloc(1,sizeof(pCodeOpLabel) );
	pcop->type = PO_LABEL;
	
	pcop->name = NULL;
	
	if(key>0)
		sprintf(s=buffer,"_%05d_DS_",key);
	else 
		s = name, key = label_key--;
	
	PCOLAB(pcop)->offset = 0;
	if(s)
		pcop->name = Safe_strdup(s);
	
	((pCodeOpLabel *)pcop)->key = key;
	
	//fprintf(stderr,"newpCodeOpLabel: key=%d, name=%s\n",key,((s)?s:""));
	return pcop;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
pCodeOp *newpCodeOpLit(int lit)
{
	char *s = buffer;
	pCodeOp *pcop;
	
	
	pcop = Safe_calloc(1,sizeof(pCodeOpLit) );
	pcop->type = PO_LITERAL;
	
	pcop->name = NULL;
	if(lit>=0) {
		sprintf(s,"0x%02x", (unsigned char)lit);
		if(s)
			pcop->name = Safe_strdup(s);
	}
	
	((pCodeOpLit *)pcop)->lit = (unsigned char)lit;
	
	return pcop;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
pCodeOp *newpCodeOpImmd(char *name, int offset, int index, int code_space, int is_func)
{
	pCodeOp *pcop;
	
	pcop = Safe_calloc(1,sizeof(pCodeOpImmd) );
	pcop->type = PO_IMMEDIATE;
	if(name) {
		regs *r = NULL;
		pcop->name = Safe_strdup(name);
		
		if(!is_func) 
			r = dirregWithName(name);
		
		PCOI(pcop)->r = r;
		if(r) {
			//fprintf(stderr, " newpCodeOpImmd reg %s exists\n",name);
			PCOI(pcop)->rIdx = r->rIdx;
		} else {
			//fprintf(stderr, " newpCodeOpImmd reg %s doesn't exist\n",name);
			PCOI(pcop)->rIdx = -1;
		}
		//fprintf(stderr,"%s %s %d\n",__FUNCTION__,name,offset);
	} else {
		pcop->name = NULL;
	}
	
	PCOI(pcop)->index = index;
	PCOI(pcop)->offset = offset;
	PCOI(pcop)->_const = code_space;
	PCOI(pcop)->_function = is_func;
	
	return pcop;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
pCodeOp *newpCodeOpWild(int id, pCodeWildBlock *pcwb, pCodeOp *subtype)
{
	char *s = buffer;
	pCodeOp *pcop;
	
	
	if(!pcwb || !subtype) {
		fprintf(stderr, "Wild opcode declaration error: %s-%d\n",__FILE__,__LINE__);
		exit(1);
	}
	
	pcop = Safe_calloc(1,sizeof(pCodeOpWild));
	pcop->type = PO_WILD;
	sprintf(s,"%%%d",id);
	pcop->name = Safe_strdup(s);
	
	PCOW(pcop)->id = id;
	PCOW(pcop)->pcwb = pcwb;
	PCOW(pcop)->subtype = subtype;
	PCOW(pcop)->matched = NULL;
	
	return pcop;
}
/*-----------------------------------------------------------------*/
/* Find a symbol with matching name                                */
/*-----------------------------------------------------------------*/
static symbol *symFindWithName(memmap * map, const char *name)
{
	symbol *sym;
	
	for (sym = setFirstItem(map->syms); sym; sym = setNextItem (map->syms)) {
		if (sym->rname && (strcmp(sym->rname,name)==0))
			return sym;
	}
	return 0;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
pCodeOp *newpCodeOpBit(char *name, int ibit, int inBitSpace)
{
	pCodeOp *pcop;
	struct regs *r = 0;
	
	pcop = Safe_calloc(1,sizeof(pCodeOpRegBit) );
	pcop->type = PO_GPR_BIT;
	
	PCORB(pcop)->bit = ibit;
	PCORB(pcop)->inBitSpace = inBitSpace;
	
	if (name) r = regFindWithName(name);
	if (!r) {
		// Register has not been allocated - check for symbol information
		symbol *sym;
		sym = symFindWithName(bit, name);
		if (!sym) sym = symFindWithName(sfrbit, name);
		if (!sym) sym = symFindWithName(sfr, name);
		if (sym) {
			r = allocNewDirReg(sym->etype,name);
		}
	}
	if (r) {
		pcop->name = NULL;
		PCOR(pcop)->r = r;
		PCOR(pcop)->rIdx = r->rIdx;
	} else {
		pcop->name = Safe_strdup(name);   
		PCOR(pcop)->r = NULL;
		PCOR(pcop)->rIdx = 0;
	}
	return pcop;
}

/*-----------------------------------------------------------------*
* pCodeOp *newpCodeOpReg(int rIdx) - allocate a new register
*
* If rIdx >=0 then a specific register from the set of registers
* will be selected. If rIdx <0, then a new register will be searched
* for.
*-----------------------------------------------------------------*/

pCodeOp *newpCodeOpReg(int rIdx)
{
	pCodeOp *pcop;
	
	pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
	
	pcop->name = NULL;
	
	if(rIdx >= 0) {
		PCOR(pcop)->rIdx = rIdx;
		PCOR(pcop)->r = pic14_regWithIdx(rIdx);
	} else {
		PCOR(pcop)->r = pic14_findFreeReg(REG_GPR);
		
		if(PCOR(pcop)->r)
			PCOR(pcop)->rIdx = PCOR(pcop)->r->rIdx;
	}
	
	if(PCOR(pcop)->r)
		pcop->type = PCOR(pcop)->r->pc_type;
	
	return pcop;
}

pCodeOp *newpCodeOpRegFromStr(char *name)
{
	pCodeOp *pcop;
	
	pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
	PCOR(pcop)->r = allocRegByName(name, 1);
	PCOR(pcop)->rIdx = PCOR(pcop)->r->rIdx;
	pcop->type = PCOR(pcop)->r->pc_type;
	pcop->name = PCOR(pcop)->r->name;
	
	return pcop;
}

pCodeOp *newpCodeOpStr(char *name)
{
	pCodeOp *pcop;
	
	pcop = Safe_calloc(1,sizeof(pCodeOpStr));
	pcop->type = PO_STR;
	pcop->name = Safe_strdup(name);   
	
	PCOS(pcop)->isPublic = 0;
	
	return pcop;
}


/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

pCodeOp *newpCodeOp(char *name, PIC_OPTYPE type)
{
	pCodeOp *pcop;
	
	switch(type) {
	case PO_BIT:
	case PO_GPR_BIT:
		pcop = newpCodeOpBit(name, -1,0);
		break;
		
	case PO_LITERAL:
		pcop = newpCodeOpLit(-1);
		break;
		
	case PO_LABEL:
		pcop = newpCodeOpLabel(NULL,-1);
		break;
		
	case PO_GPR_TEMP:
		pcop = newpCodeOpReg(-1);
		break;
		
	case PO_GPR_POINTER:
	case PO_GPR_REGISTER:
		if(name)
			pcop = newpCodeOpRegFromStr(name);
		else
			pcop = newpCodeOpReg(-1);
		break;
		
	case PO_STR:
		pcop = newpCodeOpStr(name);
		break;
		
	default:
		pcop = Safe_calloc(1,sizeof(pCodeOp) );
		pcop->type = type;
		if(name)
			pcop->name = Safe_strdup(name);   
		else
			pcop->name = NULL;
	}
	
	return pcop;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void pCodeConstString(char *name, char *value)
{
	pBlock *pb;
	unsigned i;
	
	//  fprintf(stderr, " %s  %s  %s\n",__FUNCTION__,name,value);
	
	if(!name || !value)
		return;
	
	pb = newpCodeChain(NULL, 'P',newpCodeCharP("; Starting pCode block"));
	
	addpBlock(pb);
	
	sprintf(buffer,"; %s = %s",name,value);
	for (i=strlen(buffer); i--; ) {
		unsigned char c = buffer[i];
		if (c=='\r' || c=='\n') {
			memmove(buffer+i+1,buffer+i,strlen(buffer)-i+1);
			buffer[i] = '\\';
			if (c=='\r') buffer[i+1] = 'r';
			else if (c=='\n') buffer[i+1] = 'n';
		}
	}
	
	addpCode2pBlock(pb,newpCodeCharP(buffer));
	addpCode2pBlock(pb,newpCodeLabel(name,-1));
	
	do {
		addpCode2pBlock(pb,newpCode(POC_RETLW,newpCodeOpLit(*value)));
	}while (*value++);
	
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void pCodeReadCodeTable(void)
{
	pBlock *pb;
	
	fprintf(stderr, " %s\n",__FUNCTION__);
	
	pb = newpCodeChain(NULL, 'P',newpCodeCharP("; Starting pCode block"));
	
	addpBlock(pb);
	
	addpCode2pBlock(pb,newpCodeCharP("; ReadCodeTable - built in function"));
	addpCode2pBlock(pb,newpCodeCharP("; Inputs: temp1,temp2 = code pointer"));
	addpCode2pBlock(pb,newpCodeCharP("; Outpus: W (from RETLW at temp2:temp1)"));
	addpCode2pBlock(pb,newpCodeLabel("ReadCodeTable:",-1));
	
	addpCode2pBlock(pb,newpCode(POC_MOVFW,newpCodeOpRegFromStr("temp2")));
	addpCode2pBlock(pb,newpCode(POC_MOVWF,newpCodeOpRegFromStr("PCLATH")));
	addpCode2pBlock(pb,newpCode(POC_MOVFW,newpCodeOpRegFromStr("temp1")));
	addpCode2pBlock(pb,newpCode(POC_MOVWF,newpCodeOpRegFromStr("PCL")));
	
	
}

/*-----------------------------------------------------------------*/
/* addpCode2pBlock - place the pCode into the pBlock linked list   */
/*-----------------------------------------------------------------*/
void addpCode2pBlock(pBlock *pb, pCode *pc)
{
	
	if(!pc)
		return;
	
	if(!pb->pcHead) {
	/* If this is the first pcode to be added to a block that
	* was initialized with a NULL pcode, then go ahead and
		* make this pcode the head and tail */
		pb->pcHead  = pb->pcTail = pc;
	} else {
		//    if(pb->pcTail)
		pb->pcTail->next = pc;
		
		pc->prev = pb->pcTail;
		pc->pb = pb;
		
		pb->pcTail = pc;
	}
}

/*-----------------------------------------------------------------*/
/* addpBlock - place a pBlock into the pFile                       */
/*-----------------------------------------------------------------*/
void addpBlock(pBlock *pb)
{
	// fprintf(stderr," Adding pBlock: dbName =%c\n",getpBlock_dbName(pb));
	
	if(!the_pFile) {
		/* First time called, we'll pass through here. */
		//_ALLOC(the_pFile,sizeof(pFile));
		the_pFile = Safe_calloc(1,sizeof(pFile));
		the_pFile->pbHead = the_pFile->pbTail = pb;
		the_pFile->functions = NULL;
		return;
	}
	
	the_pFile->pbTail->next = pb;
	pb->prev = the_pFile->pbTail;
	pb->next = NULL;
	the_pFile->pbTail = pb;
}

/*-----------------------------------------------------------------*/
/* removepBlock - remove a pBlock from the pFile                   */
/*-----------------------------------------------------------------*/
void removepBlock(pBlock *pb)
{
	pBlock *pbs;
	
	if(!the_pFile)
		return;
	
	
	//fprintf(stderr," Removing pBlock: dbName =%c\n",getpBlock_dbName(pb));
	
	for(pbs = the_pFile->pbHead; pbs; pbs = pbs->next) {
		if(pbs == pb) {
			
			if(pbs == the_pFile->pbHead)
				the_pFile->pbHead = pbs->next;
			
			if (pbs == the_pFile->pbTail) 
				the_pFile->pbTail = pbs->prev;
			
			if(pbs->next)
				pbs->next->prev = pbs->prev;
			
			if(pbs->prev)
				pbs->prev->next = pbs->next;
			
			return;
			
		}
	}
	
	fprintf(stderr, "Warning: call to %s:%s didn't find pBlock\n",__FILE__,__FUNCTION__);
	
}

/*-----------------------------------------------------------------*/
/* printpCode - write the contents of a pCode to a file            */
/*-----------------------------------------------------------------*/
void printpCode(FILE *of, pCode *pc)
{
	
	if(!pc || !of)
		return;
	
	if(pc->print) {
		pc->print(of,pc);
		return;
	}
	
	fprintf(of,"warning - unable to print pCode\n");
}

/*-----------------------------------------------------------------*/
/* printpBlock - write the contents of a pBlock to a file          */
/*-----------------------------------------------------------------*/
void printpBlock(FILE *of, pBlock *pb)
{
	pCode *pc;
	
	if(!pb)
		return;
	
	if(!of)
		of = stderr;
	
	for(pc = pb->pcHead; pc; pc = pc->next) {
		printpCode(of,pc);

		if (isPCI(pc))
		{
			if (isPCI(pc) && (PCI(pc)->op == POC_PAGESEL || PCI(pc)->op == POC_BANKSEL)) {
				pcode_doubles++;
			} else {
				pcode_insns++;
			}
		}
	} // for

}

/*-----------------------------------------------------------------*/
/*                                                                 */
/*       pCode processing                                          */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*-----------------------------------------------------------------*/

void unlinkpCode(pCode *pc)
{
	
	
	if(pc) {
#ifdef PCODE_DEBUG
		fprintf(stderr,"Unlinking: ");
		printpCode(stderr, pc);
#endif
		if(pc->prev) 
			pc->prev->next = pc->next;
		if(pc->next)
			pc->next->prev = pc->prev;

#if 0
		/* RN: I believe this should be right here, but this did not
		 *     cure the bug I was hunting... */
		/* must keep labels -- attach to following instruction */
		if (isPCI(pc) && PCI(pc)->label && pc->next)
		{
		  pCodeInstruction *pcnext = PCI(findNextInstruction (pc->next));
		  if (pcnext)
		  {
		    pBranchAppend (pcnext->label, PCI(pc)->label);
		  }
		}
#endif
		pc->prev = pc->next = NULL;
	}
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

static void genericDestruct(pCode *pc)
{
	
	unlinkpCode(pc);
	
	if(isPCI(pc)) {
	/* For instructions, tell the register (if there's one used)
		* that it's no longer needed */
		regs *reg = getRegFromInstruction(pc);
		if(reg)
			deleteSetItem (&(reg->reglives.usedpCodes),pc);
	}
	
	/* Instead of deleting the memory used by this pCode, mark
	* the object as bad so that if there's a pointer to this pCode
	* dangling around somewhere then (hopefully) when the type is
	* checked we'll catch it.
	*/
	
	pc->type = PC_BAD;
	
	addpCode2pBlock(pb_dead_pcodes, pc);
	
	//free(pc);
	
}


/*-----------------------------------------------------------------*/
/*  Copies the pCodeInstruction flow pointer from source pCode     */
/*-----------------------------------------------------------------*/
static void CopyFlow(pCodeInstruction *pcd, pCode *pcs) {
	pCode *p;
	pCodeFlow *pcflow = 0;
	for (p=pcs; p; p=p->prev) {
		if (isPCI(p)) {
			pcflow = PCI(p)->pcflow;
			break;
		}
		if (isPCF(p)) {
			pcflow = (pCodeFlow*)p;
			break;
		}
	}
	PCI(pcd)->pcflow = pcflow;
}

/*-----------------------------------------------------------------*/
/*  pCodeInsertAfter - splice in the pCode chain starting with pc2 */
/*                     into the pCode chain containing pc1         */
/*-----------------------------------------------------------------*/
void pCodeInsertAfter(pCode *pc1, pCode *pc2)
{
	
	if(!pc1 || !pc2)
		return;
	
	pc2->next = pc1->next;
	if(pc1->next)
		pc1->next->prev = pc2;
	
	pc2->pb = pc1->pb;
	pc2->prev = pc1;
	pc1->next = pc2;
	
	/* If this is an instrution type propogate the flow */
	if (isPCI(pc2))
		CopyFlow(PCI(pc2),pc1);
}

/*------------------------------------------------------------------*/
/*  pCodeInsertBefore - splice in the pCode chain starting with pc2 */
/*                      into the pCode chain containing pc1         */
/*------------------------------------------------------------------*/
void pCodeInsertBefore(pCode *pc1, pCode *pc2)
{
	
	if(!pc1 || !pc2)
		return;
	
	pc2->prev = pc1->prev;
	if(pc1->prev)
		pc1->prev->next = pc2;
	
	pc2->pb = pc1->pb;
	pc2->next = pc1;
	pc1->prev = pc2;
	
	/* If this is an instrution type propogate the flow */
	if (isPCI(pc2))
		CopyFlow(PCI(pc2),pc1);
}

/*-----------------------------------------------------------------*/
/* pCodeOpCopy - copy a pcode operator                             */
/*-----------------------------------------------------------------*/
pCodeOp *pCodeOpCopy(pCodeOp *pcop)
{
	pCodeOp *pcopnew=NULL;
	
	if(!pcop)
		return NULL;
	
	switch(pcop->type) { 
	case PO_NONE:
	case PO_STR:
		pcopnew = Safe_calloc (1, sizeof (pCodeOp));
		memcpy (pcopnew, pcop, sizeof (pCodeOp));
		break;
		
	case PO_W:
	case PO_STATUS:
	case PO_FSR:
	case PO_INDF:
	case PO_INTCON:
	case PO_GPR_REGISTER:
	case PO_GPR_TEMP:
	case PO_GPR_POINTER:
	case PO_SFR_REGISTER:
	case PO_PCL:
	case PO_PCLATH:
	case PO_DIR:
		//DFPRINTF((stderr,"pCodeOpCopy GPR register\n"));
		pcopnew = Safe_calloc(1,sizeof(pCodeOpReg) );
		memcpy (pcopnew, pcop, sizeof (pCodeOpReg));
		DFPRINTF((stderr," register index %d\n", PCOR(pcop)->r->rIdx));
		break;

	case PO_LITERAL:
		//DFPRINTF((stderr,"pCodeOpCopy lit\n"));
		pcopnew = Safe_calloc(1,sizeof(pCodeOpLit) );
		memcpy (pcopnew, pcop, sizeof (pCodeOpLit));
		break;
		
	case PO_IMMEDIATE:
		pcopnew = Safe_calloc(1,sizeof(pCodeOpImmd) );
		memcpy (pcopnew, pcop, sizeof (pCodeOpImmd));
		break;
		
	case PO_GPR_BIT:
	case PO_CRY:
	case PO_BIT:
		//DFPRINTF((stderr,"pCodeOpCopy bit\n"));
		pcopnew = Safe_calloc(1,sizeof(pCodeOpRegBit) );
		memcpy (pcopnew, pcop, sizeof (pCodeOpRegBit));
		break;

	case PO_LABEL:
		//DFPRINTF((stderr,"pCodeOpCopy label\n"));
		pcopnew = Safe_calloc(1,sizeof(pCodeOpLabel) );
		memcpy (pcopnew, pcop, sizeof(pCodeOpLabel));
		break;
		
	case PO_WILD:
		/* Here we expand the wild card into the appropriate type: */
		/* By recursively calling pCodeOpCopy */
		//DFPRINTF((stderr,"pCodeOpCopy wild\n"));
		if(PCOW(pcop)->matched)
			pcopnew = pCodeOpCopy(PCOW(pcop)->matched);
		else {
			// Probably a label
			pcopnew = pCodeOpCopy(PCOW(pcop)->subtype);
			pcopnew->name = Safe_strdup(PCOW(pcop)->pcwb->vars[PCOW(pcop)->id]);
			//DFPRINTF((stderr,"copied a wild op named %s\n",pcopnew->name));
		}
		
		return pcopnew;
		break;

	default:
		assert ( !"unhandled pCodeOp type copied" );
		break;
	} // switch
	
	if(pcop->name)
		pcopnew->name = Safe_strdup(pcop->name);
	else
		pcopnew->name = NULL;
	
	return pcopnew;
}

/*-----------------------------------------------------------------*/
/* popCopyReg - copy a pcode operator                              */
/*-----------------------------------------------------------------*/
pCodeOp *popCopyReg(pCodeOpReg *pc)
{
	pCodeOpReg *pcor;
	
	pcor = Safe_calloc(1,sizeof(pCodeOpReg) );
	pcor->pcop.type = pc->pcop.type;
	if(pc->pcop.name) {
		if(!(pcor->pcop.name = Safe_strdup(pc->pcop.name)))
			fprintf(stderr,"oops %s %d",__FILE__,__LINE__);
	} else
		pcor->pcop.name = NULL;
	
	if (pcor->pcop.type == PO_IMMEDIATE){
		PCOL(pcor)->lit = PCOL(pc)->lit;
	} else {
		pcor->r = pc->r;
		pcor->rIdx = pc->rIdx;
		if (pcor->r)
			pcor->r->wasUsed=1;
	}	
	//DEBUGpic14_emitcode ("; ***","%s  , copying %s, rIdx=%d",__FUNCTION__,pc->pcop.name,pc->rIdx);
	
	return PCOP(pcor);
}

/*-----------------------------------------------------------------*/
/* pCodeInstructionCopy - copy a pCodeInstructionCopy              */
/*-----------------------------------------------------------------*/
pCode *pCodeInstructionCopy(pCodeInstruction *pci,int invert)
{
	pCodeInstruction *new_pci;
	
	if(invert)
		new_pci = PCI(newpCode(pci->inverted_op,pci->pcop));
	else
		new_pci = PCI(newpCode(pci->op,pci->pcop));
	
	new_pci->pc.pb = pci->pc.pb;
	new_pci->from = pci->from;
	new_pci->to   = pci->to;
	new_pci->label = pci->label;
	new_pci->pcflow = pci->pcflow;
	
	return PCODE(new_pci);
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void pCodeDeleteChain(pCode *f,pCode *t)
{
	pCode *pc;
	
	while(f && f!=t) {
		DFPRINTF((stderr,"delete pCode:\n"));
		pc = f->next;
		//f->print(stderr,f);
		//f->delete(f);  this dumps core...
		f = pc;
	}
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void pBlockRegs(FILE *of, pBlock *pb)
{
	
	regs  *r;
	
	r = setFirstItem(pb->tregisters);
	while (r) {
		r = setNextItem(pb->tregisters);
	}
}


/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
char *get_op(pCodeOp *pcop,char *buffer, size_t size)
{
	regs *r;
	static char b[50];
	char *s;
	int use_buffer = 1;    // copy the string to the passed buffer pointer
	
	if(!buffer) {
		buffer = b;
		size = sizeof(b);
		use_buffer = 0;     // Don't bother copying the string to the buffer.
	} 
	
	if(pcop) {
		switch(pcop->type) {
		case PO_INDF:
		case PO_FSR:
			if(use_buffer) {
				SAFE_snprintf(&buffer,&size,"%s",PCOR(pcop)->r->name);
				return buffer;
			}
			//return PCOR(pcop)->r->name;
			return pcop->name;
			break;
		case PO_GPR_TEMP:
			if (PCOR(pcop)->r->type == REG_STK)
				r = typeRegWithIdx(PCOR(pcop)->r->rIdx,REG_STK,1);
			else
				r = pic14_regWithIdx(PCOR(pcop)->r->rIdx);
			
			if(use_buffer) {
				SAFE_snprintf(&buffer,&size,"%s",r->name);
				return buffer;
			}
			
			return r->name;
			
			
		case PO_IMMEDIATE:
			s = buffer;
			if(PCOI(pcop)->_const) {
				
				if( PCOI(pcop)->offset >= 0 && PCOI(pcop)->offset<4) {
					switch(PCOI(pcop)->offset) {
					case 0:
						SAFE_snprintf(&s,&size,"low (%s+%d)",pcop->name, PCOI(pcop)->index);
						break;
					case 1:
						SAFE_snprintf(&s,&size,"high (%s+%d)",pcop->name, PCOI(pcop)->index);
						break;
					default:
						fprintf (stderr, "PO_IMMEDIATE/_const/offset=%d\n", PCOI(pcop)->offset);
						assert ( !"offset too large" );
						SAFE_snprintf(&s,&size,"(((%s+%d) >> %d)&0xff)",
							pcop->name,
							PCOI(pcop)->index,
							8 * PCOI(pcop)->offset );
					}
				} else
					SAFE_snprintf(&s,&size,"LOW (%s+%d)",pcop->name,PCOI(pcop)->index);
			} else {
				if( !PCOI(pcop)->offset) { // && PCOI(pcc->pcop)->offset<4) 
					SAFE_snprintf(&s,&size,"(%s + %d)",
						pcop->name,
						PCOI(pcop)->index);
				} else {
					switch(PCOI(pcop)->offset) {
					case 0:
						SAFE_snprintf(&s,&size,"(%s + %d)",pcop->name, PCOI(pcop)->index);
						break;
					case 1:
						SAFE_snprintf(&s,&size,"high (%s + %d)",pcop->name, PCOI(pcop)->index);
						break;
					default:
						fprintf (stderr, "PO_IMMEDIATE/mutable/offset=%d\n", PCOI(pcop)->offset);
						assert ( !"offset too large" );
						SAFE_snprintf(&s,&size,"((%s + %d) >> %d)&0xff",pcop->name, PCOI(pcop)->index, 8*PCOI(pcop)->offset);
						break;
					}
				}
			}
			
			return buffer;
			
		case PO_DIR:
			s = buffer;
			//size = sizeof(buffer);
			if( PCOR(pcop)->instance) {
				SAFE_snprintf(&s,&size,"(%s + %d)",
					pcop->name,
					PCOR(pcop)->instance );
				//fprintf(stderr,"PO_DIR %s\n",buffer);
			} else
				SAFE_snprintf(&s,&size,"%s",pcop->name);
			return buffer;
			
		case PO_LABEL:
			s = buffer;
			if  (pcop->name) {
				if(PCOLAB(pcop)->offset == 1)
					SAFE_snprintf(&s,&size,"HIGH(%s)",pcop->name);
				else
					SAFE_snprintf(&s,&size,"%s",pcop->name);
			}
			return buffer;

		case PO_GPR_BIT:
			if(PCOR(pcop)->r) {
				if(use_buffer) {
					SAFE_snprintf(&buffer,&size,"%s",PCOR(pcop)->r->name);
					return buffer;
				}
				return PCOR(pcop)->r->name;
			}
			
			/* fall through to the default case */
		default:
			if(pcop->name) {
				if(use_buffer) {
					SAFE_snprintf(&buffer,&size,"%s",pcop->name);
					return buffer;
				}
				return pcop->name;
			}
		}
	}

	printf("PIC port internal warning: (%s:%d(%s)) %s not found\n",
	  __FILE__, __LINE__, __FUNCTION__,
	  pCodeOpType(pcop));

	return "NO operand";

}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static char *get_op_from_instruction( pCodeInstruction *pcc)
{
	
	if(pcc)
		return get_op(pcc->pcop,NULL,0);
	
	return ("ERROR Null: get_op_from_instruction");
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static void pCodeOpPrint(FILE *of, pCodeOp *pcop)
{
	fprintf(of,"pcodeopprint- not implemented\n");
}

/*-----------------------------------------------------------------*/
/* pCode2str - convert a pCode instruction to string               */
/*-----------------------------------------------------------------*/
char *pCode2str(char *str, size_t size, pCode *pc)
{
  char *s = str;

  switch(pc->type) {

  case PC_OPCODE:

    SAFE_snprintf(&s,&size, "\t%s\t", PCI(pc)->mnemonic);

    if( (PCI(pc)->num_ops >= 1) && (PCI(pc)->pcop)) {

      if(PCI(pc)->isBitInst) {
        if(PCI(pc)->pcop->type == PO_GPR_BIT) {
          char *name = PCI(pc)->pcop->name;
          if (!name)
            name = PCOR(PCI(pc)->pcop)->r->name;
          if( (((pCodeOpRegBit *)(PCI(pc)->pcop))->inBitSpace) )
            SAFE_snprintf(&s,&size,"(%s >> 3), (%s & 7)", name, name);
          else
            SAFE_snprintf(&s,&size,"%s,%d", name, 
            (((pCodeOpRegBit *)(PCI(pc)->pcop))->bit)&7);
        } else if(PCI(pc)->pcop->type == PO_GPR_BIT) {
          SAFE_snprintf(&s,&size,"%s,%d", get_op_from_instruction(PCI(pc)),PCORB(PCI(pc)->pcop)->bit);
      } else
          SAFE_snprintf(&s,&size,"%s,0 ; ?bug", get_op_from_instruction(PCI(pc)));
        //PCI(pc)->pcop->t.bit );
      } else {
        if(PCI(pc)->pcop->type == PO_GPR_BIT) {
          if( PCI(pc)->num_ops == 2)
            SAFE_snprintf(&s,&size,"(%s >> 3),%c",get_op_from_instruction(PCI(pc)),((PCI(pc)->isModReg) ? 'F':'W'));
          else
            SAFE_snprintf(&s,&size,"(1 << (%s & 7))",get_op_from_instruction(PCI(pc)));
        } else {
          SAFE_snprintf(&s,&size,"%s",get_op_from_instruction(PCI(pc)));
          if( PCI(pc)->num_ops == 2)
            SAFE_snprintf(&s,&size,",%c", ( (PCI(pc)->isModReg) ? 'F':'W'));
        }
      }
    }
    break;

  case PC_COMMENT:
    /* assuming that comment ends with a \n */
    SAFE_snprintf(&s,&size,";%s", ((pCodeComment *)pc)->comment);
    break;

  case PC_INLINE:
    /* assuming that inline code ends with a \n */
    SAFE_snprintf(&s,&size,"%s", ((pCodeComment *)pc)->comment);
    break;

  case PC_LABEL:
    SAFE_snprintf(&s,&size,";label=%s, key=%d\n",PCL(pc)->label,PCL(pc)->key);
    break;
  case PC_FUNCTION:
    SAFE_snprintf(&s,&size,";modname=%s,function=%s: id=%d\n",PCF(pc)->modname,PCF(pc)->fname);
    break;
  case PC_WILD:
    SAFE_snprintf(&s,&size,";\tWild opcode: id=%d\n",PCW(pc)->id);
    break;
  case PC_FLOW:
    SAFE_snprintf(&s,&size,";\t--FLOW change\n");
    break;
  case PC_CSOURCE:
//    SAFE_snprintf(&s,&size,";#CSRC\t%s %d\n; %s\n", PCCS(pc)->file_name, PCCS(pc)->line_number, PCCS(pc)->line);
    SAFE_snprintf(&s,&size,"%s\t.line\t%d; \"%s\"\t%s\n",(options.debug?"":";"),PCCS(pc)->line_number, PCCS(pc)->file_name, PCCS(pc)->line);
    break;
  case PC_ASMDIR:
    if(PCAD(pc)->directive) {
      SAFE_snprintf(&s,&size,"\t%s%s%s\n", PCAD(pc)->directive, PCAD(pc)->arg?"\t":"", PCAD(pc)->arg?PCAD(pc)->arg:"");
    } else if(PCAD(pc)->arg) {
      /* special case to handle inline labels without a tab */
      SAFE_snprintf(&s,&size,"%s\n", PCAD(pc)->arg);
    }
    break;

  case PC_BAD:
    SAFE_snprintf(&s,&size,";A bad pCode is being used\n");
  }

  return str;
}

/*-----------------------------------------------------------------*/
/* genericPrint - the contents of a pCode to a file                */
/*-----------------------------------------------------------------*/
static void genericPrint(FILE *of, pCode *pc)
{
  if(!pc || !of)
    return;

  switch(pc->type) {
  case PC_COMMENT:
    fprintf(of,";%s\n", ((pCodeComment *)pc)->comment);
    break;

  case PC_INLINE:
    fprintf(of,"%s\n", ((pCodeComment *)pc)->comment);
    break;

  case PC_OPCODE:
    // If the opcode has a label, print that first
    {
      char str[256];
      pCodeInstruction *pci = PCI(pc);
      pBranch *pbl = pci->label;
      while(pbl && pbl->pc) {
        if(pbl->pc->type == PC_LABEL)
          pCodePrintLabel(of, pbl->pc);
        pbl = pbl->next;
      }

      if(pci->cline)
        genericPrint(of,PCODE(pci->cline));


      pCode2str(str, 256, pc);

      fprintf(of,"%s",str);

      /* Debug */
      if(debug_verbose) {
        pCodeOpReg *pcor = PCOR(pci->pcop);
        fprintf(of, "\t;id=%u,key=%03x,inCond:%x,outCond:%x",pc->id,pc->seq, pci->inCond, pci->outCond);
        if(pci->pcflow)
          fprintf(of,",flow seq=%03x",pci->pcflow->pc.seq);
        if (pcor && pcor->pcop.type==PO_GPR_TEMP && !pcor->r->isFixed)
          fprintf(of,",rIdx=r0x%X",pcor->rIdx);
      }
    }
#if 0
    {
      pBranch *dpb = pc->to;   // debug
      while(dpb) {
        switch ( dpb->pc->type) {
        case PC_OPCODE:
          fprintf(of, "\t;%s", PCI(dpb->pc)->mnemonic);
          break;
        case PC_LABEL:
          fprintf(of, "\t;label %d", PCL(dpb->pc)->key);
          break;
        case PC_FUNCTION:
          fprintf(of, "\t;function %s", ( (PCF(dpb->pc)->fname) ? (PCF(dpb->pc)->fname) : "[END]"));
          break;
        case PC_FLOW:
          fprintf(of, "\t;flow");
          break;
        case PC_COMMENT:
        case PC_WILD:
          break;
        }
        dpb = dpb->next;
      }
    }
#endif
    fprintf(of,"\n");
    break;

  case PC_WILD:
    fprintf(of,";\tWild opcode: id=%d\n",PCW(pc)->id);
    if(PCW(pc)->pci.label)
      pCodePrintLabel(of, PCW(pc)->pci.label->pc);
    
    if(PCW(pc)->operand) {
      fprintf(of,";\toperand  ");
      pCodeOpPrint(of,PCW(pc)->operand );
    }
    break;

  case PC_FLOW:
    if(debug_verbose) {
      fprintf(of,";<>Start of new flow, seq=0x%x",pc->seq);
      if(PCFL(pc)->ancestor)
        fprintf(of," ancestor = 0x%x", PCODE(PCFL(pc)->ancestor)->seq);
      fprintf(of,"\n");
      fprintf(of,";  from: ");
      {
        pCodeFlowLink *link;
        for (link = setFirstItem(PCFL(pc)->from); link; link = setNextItem (PCFL(pc)->from))
	{
	  fprintf(of,"%03x ",link->pcflow->pc.seq);
	}
      }
      fprintf(of,"; to: ");
      {
        pCodeFlowLink *link;
        for (link = setFirstItem(PCFL(pc)->to); link; link = setNextItem (PCFL(pc)->to))
	{
	  fprintf(of,"%03x ",link->pcflow->pc.seq);
	}
      }
      fprintf(of,"\n");
    }
    break;

  case PC_CSOURCE:
//    fprintf(of,";#CSRC\t%s %d\n;  %s\n", PCCS(pc)->file_name, PCCS(pc)->line_number, PCCS(pc)->line);
    fprintf(of,"%s\t.line\t%d; \"%s\"\t%s\n", (options.debug?"":";"), PCCS(pc)->line_number, PCCS(pc)->file_name, PCCS(pc)->line);
    break;

  case PC_ASMDIR:
    {
      pBranch *pbl = PCAD(pc)->pci.label;
      while(pbl && pbl->pc) {
        if(pbl->pc->type == PC_LABEL)
          pCodePrintLabel(of, pbl->pc);
        pbl = pbl->next;
      }
    }
    if(PCAD(pc)->directive) {
      fprintf(of, "\t%s%s%s\n", PCAD(pc)->directive, PCAD(pc)->arg?"\t":"", PCAD(pc)->arg?PCAD(pc)->arg:"");
    } else
    if(PCAD(pc)->arg) {
      /* special case to handle inline labels without tab */
      fprintf(of, "%s\n", PCAD(pc)->arg);
    }
    break;

  case PC_LABEL:
  default:
    fprintf(of,"unknown pCode type %d\n",pc->type);
  }
}

/*-----------------------------------------------------------------*/
/* pCodePrintFunction - prints function begin/end                  */
/*-----------------------------------------------------------------*/

static void pCodePrintFunction(FILE *of, pCode *pc)
{
	
	if(!pc || !of)
		return;
	
	if( ((pCodeFunction *)pc)->modname) 
		fprintf(of,"F_%s",((pCodeFunction *)pc)->modname);
	
	if(PCF(pc)->fname) {
		pBranch *exits = PCF(pc)->to;
		int i=0;
		fprintf(of,"%s\t;Function start\n",PCF(pc)->fname);
		while(exits) {
			i++;
			exits = exits->next;
		}
		//if(i) i--;
		fprintf(of,"; %d exit point%c\n",i, ((i==1) ? ' ':'s'));
		
	}else {
		if((PCF(pc)->from && 
			PCF(pc)->from->pc->type == PC_FUNCTION &&
			PCF(PCF(pc)->from->pc)->fname) )
			fprintf(of,"; exit point of %s\n",PCF(PCF(pc)->from->pc)->fname);
		else
			fprintf(of,"; exit point [can't find entry point]\n");
	}
}
/*-----------------------------------------------------------------*/
/* pCodePrintLabel - prints label                                  */
/*-----------------------------------------------------------------*/

static void pCodePrintLabel(FILE *of, pCode *pc)
{
	
	if(!pc || !of)
		return;
	
	if(PCL(pc)->label) 
		fprintf(of,"%s\n",PCL(pc)->label);
	else if (PCL(pc)->key >=0) 
		fprintf(of,"_%05d_DS_:\n",PCL(pc)->key);
	else
		fprintf(of,";wild card label: id=%d\n",-PCL(pc)->key);
	
}

/*-----------------------------------------------------------------*/
/* unlinkpCodeFromBranch - Search for a label in a pBranch and     */
/*                         remove it if it is found.               */
/*-----------------------------------------------------------------*/
static void unlinkpCodeFromBranch(pCode *pcl , pCode *pc)
{
  pBranch *b, *bprev;

  bprev = NULL;

  if(pcl->type == PC_OPCODE || pcl->type == PC_INLINE || pcl->type == PC_ASMDIR)
    b = PCI(pcl)->label;
  else {
    fprintf(stderr, "LINE %d. can't unlink from non opcode\n",__LINE__);
    exit(1);
  }
  
  //fprintf (stderr, "%s \n",__FUNCTION__);
  //pcl->print(stderr,pcl);
  //pc->print(stderr,pc);
  while(b) {
    if(b->pc == pc) {
      //fprintf (stderr, "found label\n");
      
      /* Found a label */
      if(bprev) {
        bprev->next = b->next;  /* Not first pCode in chain */
        free(b);
      } else {
        pc->destruct(pc);
        PCI(pcl)->label = b->next;   /* First pCode in chain */
        free(b);
      }
      return;  /* A label can't occur more than once */
    }
    bprev = b;
    b = b->next;
  }
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
pBranch * pBranchAppend(pBranch *h, pBranch *n)
{
	pBranch *b;
	
	if(!h)
		return n;
	
	if(h == n)
		return n;
	
	b = h;
	while(b->next)
		b = b->next;
	
	b->next = n;
	
	return h;
	
}  
/*-----------------------------------------------------------------*/
/* pBranchLink - given two pcodes, this function will link them    */
/*               together through their pBranches                  */
/*-----------------------------------------------------------------*/
static void pBranchLink(pCodeFunction *f, pCodeFunction *t)
{
	pBranch *b;
	
	// Declare a new branch object for the 'from' pCode.
	
	//_ALLOC(b,sizeof(pBranch));
	b = Safe_calloc(1,sizeof(pBranch));
	b->pc = PCODE(t);             // The link to the 'to' pCode.
	b->next = NULL;
	
	f->to = pBranchAppend(f->to,b);
	
	// Now do the same for the 'to' pCode.
	
	//_ALLOC(b,sizeof(pBranch));
	b = Safe_calloc(1,sizeof(pBranch));
	b->pc = PCODE(f);
	b->next = NULL;
	
	t->from = pBranchAppend(t->from,b);
	
}

#if 0
/*-----------------------------------------------------------------*/
/* pBranchFind - find the pBranch in a pBranch chain that contains */
/*               a pCode                                           */
/*-----------------------------------------------------------------*/
static pBranch *pBranchFind(pBranch *pb,pCode *pc)
{
	while(pb) {
		
		if(pb->pc == pc)
			return pb;
		
		pb = pb->next;
	}
	
	return NULL;
}

/*-----------------------------------------------------------------*/
/* pCodeUnlink - Unlink the given pCode from its pCode chain.      */
/*-----------------------------------------------------------------*/
static void pCodeUnlink(pCode *pc)
{
	pBranch *pb1,*pb2;
	pCode *pc1;
	
	if(!pc->prev || !pc->next) {
		fprintf(stderr,"unlinking bad pCode in %s:%d\n",__FILE__,__LINE__);
		exit(1);
	}
	
	/* first remove the pCode from the chain */
	pc->prev->next = pc->next;
	pc->next->prev = pc->prev;
	
	/* Now for the hard part... */
	
	/* Remove the branches */
	
	pb1 = pc->from;
	while(pb1) {
	pc1 = pb1->pc;    /* Get the pCode that branches to the
	* one we're unlinking */
	
	/* search for the link back to this pCode (the one we're
	* unlinking) */
	if(pb2 = pBranchFind(pc1->to,pc)) {
		pb2->pc = pc->to->pc;  // make the replacement
		
							   /* if the pCode we're unlinking contains multiple 'to'
							   * branches (e.g. this a skip instruction) then we need
		* to copy these extra branches to the chain. */
		if(pc->to->next)
			pBranchAppend(pb2, pc->to->next);
	}
	
	pb1 = pb1->next;
	}
	
	
}
#endif
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
#if 0
static void genericAnalyze(pCode *pc)
{
	switch(pc->type) {
	case PC_WILD:
	case PC_COMMENT:
		return;
	case PC_LABEL:
	case PC_FUNCTION:
	case PC_OPCODE:
		{
			// Go through the pCodes that are in pCode chain and link
			// them together through the pBranches. Note, the pCodes
			// are linked together as a contiguous stream like the 
			// assembly source code lines. The linking here mimics this
			// except that comments are not linked in.
			// 
			pCode *npc = pc->next;
			while(npc) {
				if(npc->type == PC_OPCODE || npc->type == PC_LABEL) {
					pBranchLink(pc,npc);
					return;
				} else
					npc = npc->next;
			}
			/* reached the end of the pcode chain without finding
			* an instruction we could link to. */
		}
		break;
	case PC_FLOW:
		fprintf(stderr,"analyze PC_FLOW\n");
		
		return;
	case PC_BAD:
		fprintf(stderr,";A bad pCode is being used\n");
		
	}
}
#endif

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int compareLabel(pCode *pc, pCodeOpLabel *pcop_label)
{
  pBranch *pbr;
  
  if(pc->type == PC_LABEL) {
    if( ((pCodeLabel *)pc)->key ==  pcop_label->key)
      return TRUE;
  }
  if(pc->type == PC_OPCODE || pc->type == PC_ASMDIR) {
    pbr = PCI(pc)->label;
    while(pbr) {
      if(pbr->pc->type == PC_LABEL) {
        if( ((pCodeLabel *)(pbr->pc))->key ==  pcop_label->key)
          return TRUE;
      }
      pbr = pbr->next;
    }
  }
  
  return FALSE;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int checkLabel(pCode *pc)
{
	pBranch *pbr;
	
	if(pc && isPCI(pc)) {
		pbr = PCI(pc)->label;
		while(pbr) {
			if(isPCL(pbr->pc) && (PCL(pbr->pc)->key >= 0))
				return TRUE;
			
			pbr = pbr->next;
		}
	}
	
	return FALSE;
}

/*-----------------------------------------------------------------*/
/* findLabelinpBlock - Search the pCode for a particular label     */
/*-----------------------------------------------------------------*/
pCode * findLabelinpBlock(pBlock *pb,pCodeOpLabel *pcop_label)
{
	pCode  *pc;
	
	if(!pb)
		return NULL;
	
	for(pc = pb->pcHead; pc; pc = pc->next) 
		if(compareLabel(pc,pcop_label))
			return pc;
		
		return NULL;
}

/*-----------------------------------------------------------------*/
/* findLabel - Search the pCode for a particular label             */
/*-----------------------------------------------------------------*/
pCode * findLabel(pCodeOpLabel *pcop_label)
{
	pBlock *pb;
	pCode  *pc;
	
	if(!the_pFile)
		return NULL;
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		if( (pc = findLabelinpBlock(pb,pcop_label)) != NULL)
			return pc;
	}
	
	fprintf(stderr,"Couldn't find label %s\n", pcop_label->pcop.name);
	return NULL;
}

/*-----------------------------------------------------------------*/
/* findNextpCode - given a pCode, find the next of type 'pct'      */
/*                 in the linked list                              */
/*-----------------------------------------------------------------*/
pCode * findNextpCode(pCode *pc, PC_TYPE pct)
{
	
	while(pc) {
		if(pc->type == pct)
			return pc;
		
		pc = pc->next;
	}
	
	return NULL;
}

/*-----------------------------------------------------------------*/
/* findPrevpCode - given a pCode, find the previous of type 'pct'  */
/*                 in the linked list                              */
/*-----------------------------------------------------------------*/
pCode * findPrevpCode(pCode *pc, PC_TYPE pct)
{
	
	while(pc) {
		if(pc->type == pct) {
			/*
			static unsigned int stop;
			if (pc->id == 524)
				stop++; // Place break point here
			*/
			return pc;
		}
		
		pc = pc->prev;
	}
	
	return NULL;
}

/*-----------------------------------------------------------------*/
/* findNextInstruction - given a pCode, find the next instruction  */
/*                       in the linked list                        */
/*-----------------------------------------------------------------*/
pCode * findNextInstruction(pCode *pci)
{
  pCode *pc = pci;

  while(pc) {
  if((pc->type == PC_OPCODE)
    || (pc->type == PC_WILD)
    || (pc->type == PC_ASMDIR))
      return pc;

#ifdef PCODE_DEBUG
    fprintf(stderr,"findNextInstruction:  ");
    printpCode(stderr, pc);
#endif
    pc = pc->next;
  }

  //fprintf(stderr,"Couldn't find instruction\n");
  return NULL;
}

/*-----------------------------------------------------------------*/
/* findNextInstruction - given a pCode, find the next instruction  */
/*                       in the linked list                        */
/*-----------------------------------------------------------------*/
pCode * findPrevInstruction(pCode *pci)
{
  pCode *pc = pci;

  while(pc) {

    if((pc->type == PC_OPCODE)
      || (pc->type == PC_WILD)
      || (pc->type == PC_ASMDIR))
      return pc;
      

#ifdef PCODE_DEBUG
    fprintf(stderr,"pic16_findPrevInstruction:  ");
    printpCode(stderr, pc);
#endif
    pc = pc->prev;
  }

  //fprintf(stderr,"Couldn't find instruction\n");
  return NULL;
}

/*-----------------------------------------------------------------*/
/* findFunctionEnd - given a pCode find the end of the function    */
/*                   that contains it                              */
/*-----------------------------------------------------------------*/
pCode * findFunctionEnd(pCode *pc)
{
	while(pc) {
		if(pc->type == PC_FUNCTION &&  !(PCF(pc)->fname))
			return pc;
		
		pc = pc->next;
	}
	
	fprintf(stderr,"Couldn't find function end\n");
	return NULL;
}

#if 0
/*-----------------------------------------------------------------*/
/* AnalyzeLabel - if the pCode is a label, then merge it with the  */
/*                instruction with which it is associated.         */
/*-----------------------------------------------------------------*/
static void AnalyzeLabel(pCode *pc)
{
	
	pCodeUnlink(pc);
	
}
#endif

#if 0
static void AnalyzeGOTO(pCode *pc)
{
	
	pBranchLink(pc,findLabel( (pCodeOpLabel *) (PCI(pc)->pcop) ));
	
}

static void AnalyzeSKIP(pCode *pc)
{
	
	pBranchLink(pc,findNextInstruction(pc->next));
	pBranchLink(pc,findNextInstruction(pc->next->next));
	
}

static void AnalyzeRETURN(pCode *pc)
{
	
	//  branch_link(pc,findFunctionEnd(pc->next));
	
}

#endif

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
regs * getRegFromInstruction(pCode *pc)
{
	regs *r;
	if(!pc                   || 
		!isPCI(pc)            ||
		!PCI(pc)->pcop        ||
		PCI(pc)->num_ops == 0 )
		return NULL;
	
	switch(PCI(pc)->pcop->type) {
	case PO_STATUS:
	case PO_FSR:
	case PO_INDF:
	case PO_INTCON:
	case PO_BIT:
	case PO_GPR_TEMP:
	case PO_SFR_REGISTER:
	case PO_PCL:
	case PO_PCLATH:
		return PCOR(PCI(pc)->pcop)->r;
	
	case PO_GPR_REGISTER:
	case PO_GPR_BIT:
	case PO_DIR:
		r = PCOR(PCI(pc)->pcop)->r;
		if (r)
			return r;
		return dirregWithName(PCI(pc)->pcop->name);
		
	case PO_LITERAL:
		break;
		
	case PO_IMMEDIATE:
		r = PCOI(PCI(pc)->pcop)->r;
		if (r)
			return r;
		return dirregWithName(PCI(pc)->pcop->name);
		
	default:
		break;
	}
	
	return NULL;
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

void AnalyzepBlock(pBlock *pb)
{
	pCode *pc;
	
	if(!pb)
		return;
	
		/* Find all of the registers used in this pBlock 
		* by looking at each instruction and examining it's
		* operands
	*/
	for(pc = pb->pcHead; pc; pc = pc->next) {
		
		/* Is this an instruction with operands? */
		if(pc->type == PC_OPCODE && PCI(pc)->pcop) {
			
			if((PCI(pc)->pcop->type == PO_GPR_TEMP) 
				|| ((PCI(pc)->pcop->type == PO_GPR_BIT) && PCOR(PCI(pc)->pcop)->r && (PCOR(PCI(pc)->pcop)->r->pc_type == PO_GPR_TEMP))) {
				
				/* Loop through all of the registers declared so far in
				this block and see if we find this one there */
				
				regs *r = setFirstItem(pb->tregisters);
				
				while(r) {
					if((r->rIdx == PCOR(PCI(pc)->pcop)->r->rIdx) && (r->type == PCOR(PCI(pc)->pcop)->r->type)) {
						PCOR(PCI(pc)->pcop)->r = r;
						break;
					}
					r = setNextItem(pb->tregisters);
				}
				
				if(!r) {
					/* register wasn't found */
					//r = Safe_calloc(1, sizeof(regs));
					//memcpy(r,PCOR(PCI(pc)->pcop)->r, sizeof(regs));
					//addSet(&pb->tregisters, r);
					addSet(&pb->tregisters, PCOR(PCI(pc)->pcop)->r);
					//PCOR(PCI(pc)->pcop)->r = r;
					//fprintf(stderr,"added register to pblock: reg %d\n",r->rIdx);
					}/* else 
					 fprintf(stderr,"found register in pblock: reg %d\n",r->rIdx);
				*/
			}
			if(PCI(pc)->pcop->type == PO_GPR_REGISTER) {
				if(PCOR(PCI(pc)->pcop)->r) {
					pic14_allocWithIdx (PCOR(PCI(pc)->pcop)->r->rIdx);
					DFPRINTF((stderr,"found register in pblock: reg 0x%x\n",PCOR(PCI(pc)->pcop)->r->rIdx));
				} else {
					if(PCI(pc)->pcop->name)
						fprintf(stderr,"ERROR: %s is a NULL register\n",PCI(pc)->pcop->name );
					else
						fprintf(stderr,"ERROR: NULL register\n");
				}
			}
		}
		
		
	}
}

/*-----------------------------------------------------------------*/
/* */
/*-----------------------------------------------------------------*/
void InsertpFlow(pCode *pc, pCode **pflow)
{
	if(*pflow)
		PCFL(*pflow)->end = pc;
	
	if(!pc || !pc->next)
		return;
	
	*pflow = newpCodeFlow();
	pCodeInsertAfter(pc, *pflow);
}

/*-----------------------------------------------------------------*/
/* BuildFlow(pBlock *pb) - examine the code in a pBlock and build  */
/*                         the flow blocks.                        */
/*
* BuildFlow inserts pCodeFlow objects into the pCode chain at each
* point the instruction flow changes. 
*/
/*-----------------------------------------------------------------*/
void BuildFlow(pBlock *pb)
{
	pCode *pc;
	pCode *last_pci=NULL;
	pCode *pflow=NULL;
	int seq = 0;
	
	if(!pb)
		return;
	
	//fprintf (stderr,"build flow start seq %d  ",GpcFlowSeq);
	/* Insert a pCodeFlow object at the beginning of a pBlock */
	
	InsertpFlow(pb->pcHead, &pflow);
	
	//pflow = newpCodeFlow();    /* Create a new Flow object */
	//pflow->next = pb->pcHead;  /* Make the current head the next object */
	//pb->pcHead->prev = pflow;  /* let the current head point back to the flow object */
	//pb->pcHead = pflow;        /* Make the Flow object the head */
	//pflow->pb = pb;
	
	for( pc = findNextInstruction(pb->pcHead);
	pc != NULL;
	pc=findNextInstruction(pc)) { 
		
		pc->seq = seq++;
		PCI(pc)->pcflow = PCFL(pflow);
		
		//fprintf(stderr," build: ");
		//pflow->print(stderr,pflow);
		
		if (checkLabel(pc)) { 
			
		/* This instruction marks the beginning of a
			* new flow segment */
			
			pc->seq = 0;
			seq = 1;
			
			/* If the previous pCode is not a flow object, then 
			* insert a new flow object. (This check prevents 
			* two consecutive flow objects from being insert in
			* the case where a skip instruction preceeds an
			* instruction containing a label.) */

			last_pci = findPrevInstruction (pc->prev);
			
			if(last_pci && (PCI(last_pci)->pcflow == PCFL(pflow)))
				InsertpFlow(last_pci, &pflow);
			
			PCI(pc)->pcflow = PCFL(pflow);
			
		}

		if(isPCI_SKIP(pc)) {
			
		/* The two instructions immediately following this one 
			* mark the beginning of a new flow segment */
			
			while(pc && isPCI_SKIP(pc)) {
				
				PCI(pc)->pcflow = PCFL(pflow);
				pc->seq = seq-1;
				seq = 1;
				
				InsertpFlow(pc, &pflow);
				pc=findNextInstruction(pc->next);
			}
			
			seq = 0;
			
			if(!pc)
				break;
			
			PCI(pc)->pcflow = PCFL(pflow);
			pc->seq = 0;
			InsertpFlow(pc, &pflow);
			
		} else if ( isPCI_BRANCH(pc) && !checkLabel(findNextInstruction(pc->next)))  {
			
			InsertpFlow(pc, &pflow);
			seq = 0;
			
		}
		
		last_pci = pc;
		pc = pc->next;
	}
	
	//fprintf (stderr,",end seq %d",GpcFlowSeq);
	if(pflow)
		PCFL(pflow)->end = pb->pcTail;
}

/*-------------------------------------------------------------------*/
/* unBuildFlow(pBlock *pb) - examine the code in a pBlock and build  */
/*                           the flow blocks.                        */
/*
* unBuildFlow removes pCodeFlow objects from a pCode chain
*/
/*-----------------------------------------------------------------*/
void unBuildFlow(pBlock *pb)
{
	pCode *pc,*pcnext;
	
	if(!pb)
		return;
	
	pc = pb->pcHead;
	
	while(pc) {
		pcnext = pc->next;
		
		if(isPCI(pc)) {
			
			pc->seq = 0;
			if(PCI(pc)->pcflow) {
				//free(PCI(pc)->pcflow);
				PCI(pc)->pcflow = NULL;
			}
			
		} else if(isPCFL(pc) )
			pc->destruct(pc);
		
		pc = pcnext;
	}
	
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void dumpCond(int cond)
{
	
	static char *pcc_str[] = {
		//"PCC_NONE",
		"PCC_REGISTER",
			"PCC_C",
			"PCC_Z",
			"PCC_DC",
			"PCC_W",
			"PCC_EXAMINE_PCOP",
			"PCC_REG_BANK0",
			"PCC_REG_BANK1",
			"PCC_REG_BANK2",
			"PCC_REG_BANK3"
	};
	
	int ncond = sizeof(pcc_str) / sizeof(char *);
	int i,j;
	
	fprintf(stderr, "0x%04X\n",cond);
	
	for(i=0,j=1; i<ncond; i++, j<<=1)
		if(cond & j)
			fprintf(stderr, "  %s\n",pcc_str[i]);
		
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void FlowStats(pCodeFlow *pcflow)
{
	
	pCode *pc;
	
	if(!isPCFL(pcflow))
		return;
	
	fprintf(stderr, " FlowStats - flow block (seq=%d)\n", pcflow->pc.seq);
	
	pc = findNextpCode(PCODE(pcflow), PC_OPCODE); 
	
	if(!pc) {
		fprintf(stderr, " FlowStats - empty flow (seq=%d)\n", pcflow->pc.seq);
		return;
	}
	
	
	fprintf(stderr, "  FlowStats inCond: ");
	dumpCond(pcflow->inCond);
	fprintf(stderr, "  FlowStats outCond: ");
	dumpCond(pcflow->outCond);
	
}

/*-----------------------------------------------------------------*
* int isBankInstruction(pCode *pc) - examine the pCode *pc to determine
*    if it affects the banking bits. 
* 
* return: -1 == Banking bits are unaffected by this pCode.
*
* return: > 0 == Banking bits are affected.
*
*  If the banking bits are affected, then the returned value describes
* which bits are affected and how they're affected. The lower half
* of the integer maps to the bits that are affected, the upper half
* to whether they're set or cleared.
*
*-----------------------------------------------------------------*/
/*
#define SET_BANK_BIT (1 << 16)
#define CLR_BANK_BIT 0

static int isBankInstruction(pCode *pc)
{
	regs *reg;
	int bank = -1;
	
	if(!isPCI(pc))
		return -1;
	
	if( ( (reg = getRegFromInstruction(pc)) != NULL) && isSTATUS_REG(reg)) {
		
		// Check to see if the register banks are changing
		if(PCI(pc)->isModReg) {
			
			pCodeOp *pcop = PCI(pc)->pcop;
			switch(PCI(pc)->op) {
				
			case POC_BSF:
				if(PCORB(pcop)->bit == PIC_RP0_BIT) {
					//fprintf(stderr, "  isBankInstruction - Set RP0\n");
					return  SET_BANK_BIT | PIC_RP0_BIT;
				}
				
				if(PCORB(pcop)->bit == PIC_RP1_BIT) {
					//fprintf(stderr, "  isBankInstruction - Set RP1\n");
					return  CLR_BANK_BIT | PIC_RP0_BIT;
				}
				break;
				
			case POC_BCF:
				if(PCORB(pcop)->bit == PIC_RP0_BIT) {
					//fprintf(stderr, "  isBankInstruction - Clr RP0\n");
					return  CLR_BANK_BIT | PIC_RP1_BIT;
				}
				if(PCORB(pcop)->bit == PIC_RP1_BIT) {
					//fprintf(stderr, "  isBankInstruction - Clr RP1\n");
					return  CLR_BANK_BIT | PIC_RP1_BIT;
				}
				break;
			default:
				//fprintf(stderr, "  isBankInstruction - Status register is getting Modified by:\n");
				//genericPrint(stderr, pc);
				;
			}
		}
		
				}
				
	return bank;
}
*/

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*
static void FillFlow(pCodeFlow *pcflow)
{
	pCode *pc;
	int cur_bank;
	
	if(!isPCFL(pcflow))
		return;
	
	//  fprintf(stderr, " FillFlow - flow block (seq=%d)\n", pcflow->pc.seq);
	
	pc = findNextpCode(PCODE(pcflow), PC_OPCODE); 
	
	if(!pc) {
		//fprintf(stderr, " FillFlow - empty flow (seq=%d)\n", pcflow->pc.seq);
		return;
	}
	
	cur_bank = -1;
	
	do {
		isBankInstruction(pc);
		pc = pc->next;
	} while (pc && (pc != pcflow->end) && !isPCFL(pc));
	/ *
		if(!pc ) {
			fprintf(stderr, "  FillFlow - Bad end of flow\n");
		} else {
			fprintf(stderr, "  FillFlow - Ending flow with\n  ");
			pc->print(stderr,pc);
		}
		
		fprintf(stderr, "  FillFlow inCond: ");
		dumpCond(pcflow->inCond);
		fprintf(stderr, "  FillFlow outCond: ");
		dumpCond(pcflow->outCond);
		* /
}
*/

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void LinkFlow_pCode(pCodeInstruction *from, pCodeInstruction *to)
{
	pCodeFlowLink *fromLink, *toLink;
	
	if(!from || !to || !to->pcflow || !from->pcflow)
		return;
	
	fromLink = newpCodeFlowLink(from->pcflow);
	toLink   = newpCodeFlowLink(to->pcflow);
	
	addSetIfnotP(&(from->pcflow->to), toLink);   //to->pcflow);
	addSetIfnotP(&(to->pcflow->from), fromLink); //from->pcflow);
	
}

/*-----------------------------------------------------------------*
* void LinkFlow(pBlock *pb)
*
* In BuildFlow, the PIC code has been partitioned into contiguous
* non-branching segments. In LinkFlow, we determine the execution
* order of these segments. For example, if one of the segments ends
* with a skip, then we know that there are two possible flow segments
* to which control may be passed.
*-----------------------------------------------------------------*/
void LinkFlow(pBlock *pb)
{
	pCode *pc=NULL;
	pCode *pcflow;
	pCode *pct;
	
	//fprintf(stderr,"linkflow \n");
	
	for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
	pcflow != NULL;
	pcflow = findNextpCode(pcflow->next, PC_FLOW) ) {
		
		if(!isPCFL(pcflow))
			fprintf(stderr, "LinkFlow - pcflow is not a flow object ");
		
		//fprintf(stderr," link: ");
		//pcflow->print(stderr,pcflow);
		
		//FillFlow(PCFL(pcflow));
		
		/* find last instruction in flow */
		pc = findPrevInstruction (PCFL(pcflow)->end);
		if (!pc) continue;
		
		//fprintf(stderr, "LinkFlow - flow block (seq=%d) ", pcflow->seq);
		if(isPCI_SKIP(pc)) {
			//fprintf(stderr, "ends with skip\n");
			//pc->print(stderr,pc);
			pct=findNextInstruction(pc->next);
			LinkFlow_pCode(PCI(pc),PCI(pct));
			pct=findNextInstruction(pct->next);
			LinkFlow_pCode(PCI(pc),PCI(pct));
			continue;
		}
		
		if(isPCI_BRANCH(pc)) {
			pCodeOpLabel *pcol = PCOLAB(PCI(pc)->pcop);
			
			//fprintf(stderr, "ends with branch\n  ");
			//pc->print(stderr,pc);

			if(!(pcol && isPCOLAB(pcol))) {
				if((PCI(pc)->op != POC_RETLW) && (PCI(pc)->op != POC_RETURN) && (PCI(pc)->op != POC_CALL) && (PCI(pc)->op != POC_RETFIE) ) {
					pc->print(stderr,pc);
					fprintf(stderr, "ERROR: %s, branch instruction doesn't have label\n",__FUNCTION__);
				}
				continue;
			}
			
			if( (pct = findLabelinpBlock(pb,pcol)) != NULL)
				LinkFlow_pCode(PCI(pc),PCI(pct));
			else
				fprintf(stderr, "ERROR: %s, couldn't find label. key=%d,lab=%s\n",
				__FUNCTION__,pcol->key,((PCOP(pcol)->name)?PCOP(pcol)->name:"-"));
			//fprintf(stderr,"newpCodeOpLabel: key=%d, name=%s\n",key,((s)?s:""));
			
			/* link CALLs to next instruction */
			if (PCI(pc)->op != POC_CALL) continue;
		}
		
		if(isPCI(pc)) {
			//fprintf(stderr, "ends with non-branching instruction:\n");
			//pc->print(stderr,pc);
			
			LinkFlow_pCode(PCI(pc),PCI(findNextInstruction(pc->next)));
			
			continue;
		}
		
		if(pc) {
			fprintf(stderr, "ends with unknown\n");
			pc->print(stderr,pc);
			continue;
		}
		
		fprintf(stderr, "ends with nothing: ERROR\n");
		
	}
}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int isPCinFlow(pCode *pc, pCode *pcflow)
{
	
	if(!pc || !pcflow)
		return 0;
	
	if(!isPCI(pc) || !PCI(pc)->pcflow || !isPCFL(pcflow) )
		return 0;
	
	if( PCI(pc)->pcflow->pc.seq == pcflow->seq)
		return 1;
	
	return 0;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*
static void BanksUsedFlow2(pCode *pcflow)
{
	pCode *pc=NULL;
	
	int bank = -1;
	bool RegUsed = 0;
	
	regs *reg;
	
	if(!isPCFL(pcflow)) {
		fprintf(stderr, "BanksUsed - pcflow is not a flow object ");
		return;
	}
	
	pc = findNextInstruction(pcflow->next);
	
	PCFL(pcflow)->lastBank = -1;
	
	while(isPCinFlow(pc,pcflow)) {
		
		int bank_selected = isBankInstruction(pc);
		
		//if(PCI(pc)->pcflow) 
		//fprintf(stderr,"BanksUsedFlow2, looking at seq %d\n",PCI(pc)->pcflow->pc.seq);
		
		if(bank_selected > 0) {
			//fprintf(stderr,"BanksUsed - mucking with bank %d\n",bank_selected);
			
			// This instruction is modifying banking bits before accessing registers
			if(!RegUsed)
				PCFL(pcflow)->firstBank = -1;
			
			if(PCFL(pcflow)->lastBank == -1)
				PCFL(pcflow)->lastBank = 0;
			
			bank = (1 << (bank_selected & (PIC_RP0_BIT | PIC_RP1_BIT)));
			if(bank_selected & SET_BANK_BIT)
				PCFL(pcflow)->lastBank |= bank;
			
			
		} else { 
			reg = getRegFromInstruction(pc);
			
			if(reg && !isREGinBank(reg, bank)) {
				int allbanks = REGallBanks(reg);
				if(bank == -1)
					PCFL(pcflow)->firstBank = allbanks;
				
				PCFL(pcflow)->lastBank = allbanks;
				
				bank = allbanks;
			}
			RegUsed = 1;
								}
								
		pc = findNextInstruction(pc->next);
	}
	
	//  fprintf(stderr,"BanksUsedFlow2 flow seq=%3d, first bank = 0x%03x, Last bank 0x%03x\n",
	//    pcflow->seq,PCFL(pcflow)->firstBank,PCFL(pcflow)->lastBank);
}
*/
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*
static void BanksUsedFlow(pBlock *pb)
{
	pCode *pcflow;
	
	
	//pb->pcHead->print(stderr, pb->pcHead);
	
	pcflow = findNextpCode(pb->pcHead, PC_FLOW);
	//pcflow->print(stderr,pcflow);
	
	for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
	pcflow != NULL;
	pcflow = findNextpCode(pcflow->next, PC_FLOW) ) {
		
		BanksUsedFlow2(pcflow);
	}
	
}
*/

void pCodeReplace (pCode *old, pCode *new)
{
	pCodeInsertAfter (old, new);

	/* special handling for pCodeInstructions */
	if (isPCI(new) && isPCI(old))
	{
		assert (!PCI(new)->from && !PCI(new)->to && !PCI(new)->label && /*!PCI(new)->pcflow && */!PCI(new)->cline);
		PCI(new)->from = PCI(old)->from;
		PCI(new)->to = PCI(old)->to;
		PCI(new)->label = PCI(old)->label;
		PCI(new)->pcflow = PCI(old)->pcflow;
		PCI(new)->cline = PCI(old)->cline;
	} // if

	old->destruct (old);
}

/*-----------------------------------------------------------------*/
/* Inserts a new pCodeInstruction before an existing one           */
/*-----------------------------------------------------------------*/
static void insertPCodeInstruction(pCodeInstruction *pci, pCodeInstruction *new_pci)
{
	pCode *pcprev;

	pcprev = findPrevInstruction(pci->pc.prev);
	
	pCodeInsertAfter(pci->pc.prev, &new_pci->pc);
	
	/* Move the label, if there is one */
	
	if(pci->label) {
		new_pci->label = pci->label;
		pci->label = NULL;
	}
	
	/* Move the C code comment, if there is one */
	
	if(pci->cline) {
		new_pci->cline = pci->cline;
		pci->cline = NULL;
	}
	
	/* The new instruction has the same pcflow block */
	new_pci->pcflow = pci->pcflow;

	/* Arrrrg: is pci's previous instruction is a skip, we need to
	 * change that into a jump (over pci and the new instruction) ... */
	if (pcprev && isPCI_SKIP(pcprev))
	{
		symbol *lbl = newiTempLabel (NULL);
		pCode *label = newpCodeLabel (NULL, lbl->key);
		pCode *jump = newpCode(POC_GOTO, newpCodeOpLabel(NULL, lbl->key));

		pCodeInsertAfter (pcprev, jump);

		pCodeReplace (pcprev, pCodeInstructionCopy (PCI(pcprev), 1));
		pcprev = NULL;
		pCodeInsertAfter((pCode*)pci, label);
	}
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static void insertBankSwitch(pCodeInstruction *pci, int Set_Clear, int RP_BankBit)
{
	pCode *new_pc;
	
	new_pc = newpCode((Set_Clear?POC_BSF:POC_BCF),popCopyGPR2Bit(PCOP(&pc_status),RP_BankBit));
	
	insertPCodeInstruction(pci, PCI(new_pc));
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static void insertBankSel(pCodeInstruction  *pci, const char *name)
{
	pCode *new_pc;
	
	pCodeOp *pcop = popCopyReg(PCOR(pci->pcop));
	pcop->type = PO_GPR_REGISTER; // Sometimes the type is set to legacy 8051 - so override it
	if (pcop->name == 0)
		pcop->name = strdup(name);
	new_pc = newpCode(POC_BANKSEL, pcop);
	
	insertPCodeInstruction(pci, PCI(new_pc));
}

/*-----------------------------------------------------------------*/
/* If the register is a fixed known addess then we can assign the  */
/* bank selection bits. Otherwise the linker is going to assign    */
/* the register location and thus has to set bank selection bits   */
/* through the banksel directive.                                  */
/* One critical assumption here is that within this C module all   */ 
/* the locally allocated registers are in the same udata sector.   */
/* Therefore banksel is only called for external registers or the  */
/* first time a local register is encountered.                     */
/*-----------------------------------------------------------------*/
static int LastRegIdx = -1; /* If the previous register is the same one again then no need to change bank. */
static int BankSelect(pCodeInstruction *pci, int cur_bank, regs *reg)
{
	int bank;
	int a = reg->alias>>7;
	if ((a&3) == 3) {
		return cur_bank; // This register is available in all banks
	} else if ((a&1)&&((cur_bank==0)||(cur_bank==1))) {
		return cur_bank; // This register is available in banks 0 & 1
	} else if (a&2) {
		if (reg->address&0x80) {
			if ((cur_bank==1)||(cur_bank==3)) {
				return cur_bank; // This register is available in banks 1 & 3
			}
		} else {
			if ((cur_bank==0)||(cur_bank==1)) {
				return cur_bank; // This register is available in banks 0 & 2
			}
		}
	}
	
#if 1
	if (LastRegIdx == reg->rIdx) // If this is the same register as last time then it is in same bank
		return cur_bank;
	LastRegIdx = reg->rIdx;
#endif
	
	if (reg->isFixed) {
		bank = REG_BANK(reg);
	} else if (reg->isExtern) {
		bank = 'E'; // Unfixed extern registers are allocated by the linker therefore its bank is unknown
	} else {
		bank = 'L'; // Unfixed local registers are allocated by the linker therefore its bank is unknown
	}
	if ((cur_bank == 'L')&&(bank == 'L')) { // If current bank and new bank are both allocated locally by the linker, then assume it is in same bank.
		return 'L'; // Local registers are presumed to be in same linker assigned bank
	} else if ((bank == 'L')&&(cur_bank != 'L')) { // Reg is now local and linker to assign bank
		insertBankSel(pci, reg->name); // Let linker choose the bank selection
	} else if (bank == 'E') { // Reg is now extern and linker to assign bank
		insertBankSel(pci, reg->name); // Let linker choose the bank selection
	} else if ((cur_bank == -1)||(cur_bank == 'L')||(cur_bank == 'E')) { // Current bank unknown and new register bank is known then can set bank bits
		insertBankSwitch(pci, bank&1, PIC_RP0_BIT);
		if (getMaxRam()&0x100)
			insertBankSwitch(pci, bank&2, PIC_RP1_BIT);
	} else { // Current bank and new register banks known - can set bank bits
		switch((cur_bank^bank) & 3) {
		case 0:
			break;
		case 1:
			insertBankSwitch(pci, bank&1, PIC_RP0_BIT);
			break;
		case 2:
			insertBankSwitch(pci, bank&2, PIC_RP1_BIT);
			break;
		case 3:
			insertBankSwitch(pci, bank&1, PIC_RP0_BIT);
			if (getMaxRam()&0x100)
				insertBankSwitch(pci, bank&2, PIC_RP1_BIT);
			break;
		}
	}
	
	return bank;
}

/*-----------------------------------------------------------------*/
/* Check for bank selection pcodes instructions and modify         */
/* cur_bank to match.                                              */
/*-----------------------------------------------------------------*/
static int IsBankChange(pCode *pc, regs *reg, int *cur_bank) {
	
	if (isSTATUS_REG(reg)) {
		
		if (PCI(pc)->op == POC_BCF) {
			int old_bank = *cur_bank;
			if (PCORB(PCI(pc)->pcop)->bit == PIC_RP0_BIT) {
				/* If current bank is unknown or linker assigned then set to 0 else just change the bit */
				if (*cur_bank & ~(0x3))
					*cur_bank = 0;
				else
					*cur_bank = *cur_bank&0x2;
				LastRegIdx = reg->rIdx;
			} else if (PCORB(PCI(pc)->pcop)->bit == PIC_RP1_BIT) {
				/* If current bank is unknown or linker assigned then set to 0 else just change the bit */
				if (*cur_bank & ~(0x3))
					*cur_bank = 0;
				else
					*cur_bank = *cur_bank&0x1;
				LastRegIdx = reg->rIdx;
			}
			return old_bank != *cur_bank;
		}
		
		if (PCI(pc)->op == POC_BSF) {
			int old_bank = *cur_bank;
			if (PCORB(PCI(pc)->pcop)->bit == PIC_RP0_BIT) {
				/* If current bank is unknown or linker assigned then set to bit else just change the bit */
				if (*cur_bank & ~(0x3))
					*cur_bank = 0x1;
				else
					*cur_bank = (*cur_bank&0x2) | 0x1;
				LastRegIdx = reg->rIdx;
			} else if (PCORB(PCI(pc)->pcop)->bit == PIC_RP1_BIT) {
				/* If current bank is unknown or linker assigned then set to bit else just change the bit */
				if (*cur_bank & ~(0x3))
					*cur_bank = 0x2;
				else
					*cur_bank = (*cur_bank&0x1) | 0x2;
				LastRegIdx = reg->rIdx;
			}
			return old_bank != *cur_bank;
		}
		
	} else if (PCI(pc)->op == POC_BANKSEL) {
		int old_bank = *cur_bank;
		regs *r = PCOR(PCI(pc)->pcop)->r;
		*cur_bank = (!r || r->isExtern) ? 'E' : 'L';
		LastRegIdx = reg->rIdx;
		return old_bank != *cur_bank;
	}
	
	return 0;
}

/*-----------------------------------------------------------------*/
/* Set bank selection if necessary                                 */
/*-----------------------------------------------------------------*/
static int DoBankSelect(pCode *pc, int cur_bank) {
	pCode *pcprev;
	regs *reg;
	
	if(!isPCI(pc))
		return cur_bank;
	
	if (isCALL(pc)) {
		pCode *pcf = findFunction(get_op_from_instruction(PCI(pc)));
		LastRegIdx = -1; /* do not know which register is touched in the called function... */
		if (pcf && isPCF(pcf)) {
			pCode *pcfr;
			int rbank = 'U'; // Undetermined
			FixRegisterBanking(pcf->pb,cur_bank); // Ensure this block has had its banks selection done
			// Check all the returns to work out what bank is selected
			for (pcfr=pcf->pb->pcHead; pcfr; pcfr=pcfr->next) {
				if (isPCI(pcfr)) {
					if ((PCI(pcfr)->op==POC_RETURN) || (PCI(pcfr)->op==POC_RETLW)) {
						if (rbank == 'U')
							rbank = PCI(pcfr)->pcflow->lastBank;
						else
							if (rbank != PCI(pcfr)->pcflow->lastBank)
								return -1; // Unknown bank - multiple returns with different banks
					}
				}
			}
			if (rbank == 'U')
				return -1; // Unknown bank
			return rbank;
		} else if (isPCOS(PCI(pc)->pcop) && PCOS(PCI(pc)->pcop)->isPublic) {
			/* Extern functions may use registers in different bank - must call banksel */
			return -1; /* Unknown bank */
		}
		/* play safe... */
		return -1;
	}
	
	if ((isPCI(pc)) && (PCI(pc)->op == POC_BANKSEL)) {
		return -1; /* New bank unknown - linkers choice. */
	}
	
	reg = getRegFromInstruction(pc);
	if (reg) {
		if (IsBankChange(pc,reg,&cur_bank))
			return cur_bank;
		if (!isPCI_LIT(pc)) {
			
			/* Examine the instruction before this one to make sure it is
			* not a skip type instruction */
			pcprev = findPrevpCode(pc->prev, PC_OPCODE);
			
			/* This approach does not honor the presence of labels at this instruction... */
			//if(!pcprev || (pcprev && !isPCI_SKIP(pcprev))) {
				cur_bank = BankSelect(PCI(pc),cur_bank,reg);
			//} else {
			//	cur_bank = BankSelect(PCI(pcprev),cur_bank,reg);
			//}
			if (!PCI(pc)->pcflow)
				fprintf(stderr,"PCI ID=%d missing flow pointer ???\n",pc->id);
			else
				PCI(pc)->pcflow->lastBank = cur_bank; /* Maintain pCodeFlow lastBank state */
		}
	}
	return cur_bank;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*
static void FixRegisterBankingInFlow(pCodeFlow *pcfl, int cur_bank)
{
	pCode *pc=NULL;
	pCode *pcprev=NULL;
	
	if(!pcfl)
		return;
	
	pc = findNextInstruction(pcfl->pc.next);
	
	while(isPCinFlow(pc,PCODE(pcfl))) {
		
		cur_bank = DoBankSelect(pc,cur_bank);
		pcprev = pc;
		pc = findNextInstruction(pc->next);
		
	}
	
	if(pcprev && cur_bank) {
		// Set bank state to unknown at the end of each flow block
		cur_bank = -1;
	}
	
}
*/
/*-----------------------------------------------------------------*/
/*int compareBankFlow - compare the banking requirements between   */
/*  flow objects. */
/*-----------------------------------------------------------------*/
/*
int compareBankFlow(pCodeFlow *pcflow, pCodeFlowLink *pcflowLink, int toORfrom)
{
	
	if(!pcflow || !pcflowLink || !pcflowLink->pcflow)
		return 0;
	
	if(!isPCFL(pcflow) || !isPCFL(pcflowLink->pcflow))
		return 0;
	
	if(pcflow->firstBank == -1)
		return 0;
	
	
	if(pcflowLink->pcflow->firstBank == -1) {
		pCodeFlowLink *pctl = setFirstItem( toORfrom ? 
			pcflowLink->pcflow->to : 
		pcflowLink->pcflow->from);
		return compareBankFlow(pcflow, pctl, toORfrom);
	}
	
	if(toORfrom) {
		if(pcflow->lastBank == pcflowLink->pcflow->firstBank)
			return 0;
		
		pcflowLink->bank_conflict++;
		pcflowLink->pcflow->FromConflicts++;
		pcflow->ToConflicts++;
	} else {
		
		if(pcflow->firstBank == pcflowLink->pcflow->lastBank)
			return 0;
		
		pcflowLink->bank_conflict++;
		pcflowLink->pcflow->ToConflicts++;
		pcflow->FromConflicts++;
		
	}
	/ *
		fprintf(stderr,"compare flow found conflict: seq %d from conflicts %d, to conflicts %d\n",
		pcflowLink->pcflow->pc.seq,
		pcflowLink->pcflow->FromConflicts,
		pcflowLink->pcflow->ToConflicts);
	* /
		return 1;
	
}
*/
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*
void FixBankFlow(pBlock *pb)
{
	pCode *pc=NULL;
	pCode *pcflow;
	pCodeFlowLink *pcfl;
	
	pCode *pcflow_max_To=NULL;
	pCode *pcflow_max_From=NULL;
	int max_ToConflicts=0;
	int max_FromConflicts=0;
	
	/fprintf(stderr,"Fix Bank flow \n");
	pcflow = findNextpCode(pb->pcHead, PC_FLOW);
	
	
	/ *
		First loop through all of the flow objects in this pcode block
		and fix the ones that have banking conflicts between the 
		entry and exit.
		* /
		
	//fprintf(stderr, "FixBankFlow - Phase 1\n");
	
	for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
	pcflow != NULL;
	pcflow = findNextpCode(pcflow->next, PC_FLOW) ) {
		
		if(!isPCFL(pcflow)) {
			fprintf(stderr, "FixBankFlow - pcflow is not a flow object ");
			continue;
		}
		
		if(PCFL(pcflow)->firstBank != PCFL(pcflow)->lastBank  &&
			PCFL(pcflow)->firstBank >= 0 &&
			PCFL(pcflow)->lastBank >= 0 ) {
			
			int cur_bank = (PCFL(pcflow)->firstBank < PCFL(pcflow)->lastBank) ?
				PCFL(pcflow)->firstBank : PCFL(pcflow)->lastBank;
			
			FixRegisterBankingInFlow(PCFL(pcflow),cur_bank);
			BanksUsedFlow2(pcflow);
			
		}
	}
	
	//fprintf(stderr, "FixBankFlow - Phase 2\n");
	
	for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
	pcflow != NULL;
	pcflow = findNextpCode(pcflow->next, PC_FLOW) ) {
		
		int nFlows;
		int nConflicts;
		
		if(!isPCFL(pcflow)) {
			fprintf(stderr, "FixBankFlow - pcflow is not a flow object ");
			continue;
		}
		
		PCFL(pcflow)->FromConflicts = 0;
		PCFL(pcflow)->ToConflicts = 0;
		
		nFlows = 0;
		nConflicts = 0;
		
		//fprintf(stderr, " FixBankFlow flow seq %d\n",pcflow->seq);
		pcfl = setFirstItem(PCFL(pcflow)->from);
		while (pcfl) {
			
			pc = PCODE(pcfl->pcflow);
			
			if(!isPCFL(pc)) {
				fprintf(stderr,"oops dumpflow - to is not a pcflow\n");
				pc->print(stderr,pc);
			}
			
			nConflicts += compareBankFlow(PCFL(pcflow), pcfl, 0);
			nFlows++;
			
			pcfl=setNextItem(PCFL(pcflow)->from);
		}
		
		if((nFlows >= 2) && nConflicts && (PCFL(pcflow)->firstBank>0)) {
			//fprintf(stderr, " From conflicts flow seq %d, nflows %d ,nconflicts %d\n",pcflow->seq,nFlows, nConflicts);
			
			FixRegisterBankingInFlow(PCFL(pcflow),-1);
			BanksUsedFlow2(pcflow);
			
			continue;  / * Don't need to check the flow from here - it's already been fixed * /
				
		}
		
		nFlows = 0;
		nConflicts = 0;
		
		pcfl = setFirstItem(PCFL(pcflow)->to);
		while (pcfl) {
			
			pc = PCODE(pcfl->pcflow);
			if(!isPCFL(pc)) {
				fprintf(stderr,"oops dumpflow - to is not a pcflow\n");
				pc->print(stderr,pc);
			}
			
			nConflicts += compareBankFlow(PCFL(pcflow), pcfl, 1);
			nFlows++;
			
			pcfl=setNextItem(PCFL(pcflow)->to);
		}
		
		if((nFlows >= 2) && nConflicts &&(nConflicts != nFlows) && (PCFL(pcflow)->lastBank>0)) {
			//fprintf(stderr, " To conflicts flow seq %d, nflows %d ,nconflicts %d\n",pcflow->seq,nFlows, nConflicts);
			
			FixRegisterBankingInFlow(PCFL(pcflow),-1);
			BanksUsedFlow2(pcflow);
		}
	}
	
	/ *
		Loop through the flow objects again and find the ones with the 
		maximum conflicts
		* /
		
		for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
		pcflow != NULL;
		pcflow = findNextpCode(pcflow->next, PC_FLOW) ) {
			
			if(PCFL(pcflow)->ToConflicts > max_ToConflicts)
				pcflow_max_To = pcflow;
			
			if(PCFL(pcflow)->FromConflicts > max_FromConflicts)
				pcflow_max_From = pcflow;
		}
		/ *
			if(pcflow_max_To)
				fprintf(stderr,"compare flow Max To conflicts: seq %d conflicts %d\n",
				PCFL(pcflow_max_To)->pc.seq,
				PCFL(pcflow_max_To)->ToConflicts);
			
			if(pcflow_max_From)
				fprintf(stderr,"compare flow Max From conflicts: seq %d conflicts %d\n",
				PCFL(pcflow_max_From)->pc.seq,
				PCFL(pcflow_max_From)->FromConflicts);
			* /
}
*/

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void DumpFlow(pBlock *pb)
{
	pCode *pc=NULL;
	pCode *pcflow;
	pCodeFlowLink *pcfl;
	
	
	fprintf(stderr,"Dump flow \n");
	pb->pcHead->print(stderr, pb->pcHead);
	
	pcflow = findNextpCode(pb->pcHead, PC_FLOW);
	pcflow->print(stderr,pcflow);
	
	for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
	pcflow != NULL;
	pcflow = findNextpCode(pcflow->next, PC_FLOW) ) {
		
		if(!isPCFL(pcflow)) {
			fprintf(stderr, "DumpFlow - pcflow is not a flow object ");
			continue;
		}
		fprintf(stderr,"dumping: ");
		pcflow->print(stderr,pcflow);
		FlowStats(PCFL(pcflow));
		
		for(pcfl = setFirstItem(PCFL(pcflow)->to); pcfl; pcfl=setNextItem(PCFL(pcflow)->to)) {
			
			pc = PCODE(pcfl->pcflow);
			
			fprintf(stderr, "    from seq %d:\n",pc->seq);
			if(!isPCFL(pc)) {
				fprintf(stderr,"oops dumpflow - from is not a pcflow\n");
				pc->print(stderr,pc);
			}
			
		}
		
		for(pcfl = setFirstItem(PCFL(pcflow)->to); pcfl; pcfl=setNextItem(PCFL(pcflow)->to)) {
			
			pc = PCODE(pcfl->pcflow);
			
			fprintf(stderr, "    to seq %d:\n",pc->seq);
			if(!isPCFL(pc)) {
				fprintf(stderr,"oops dumpflow - to is not a pcflow\n");
				pc->print(stderr,pc);
			}
			
		}
		
	}
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int OptimizepBlock(pBlock *pb)
{
	pCode *pc, *pcprev;
	int matches =0;
	
	if(!pb || options.nopeep)
		return 0;
	
	DFPRINTF((stderr," Optimizing pBlock: %c\n",getpBlock_dbName(pb)));
	/*
	for(pc = pb->pcHead; pc; pc = pc->next)
	matches += pCodePeepMatchRule(pc);
	*/
	
	pc = findNextInstruction(pb->pcHead);
	if(!pc)
		return 0;
	
	pcprev = pc->prev;
	do {
		
		
		if(pCodePeepMatchRule(pc)) {
			
			matches++;
			
			if(pcprev)
				pc = findNextInstruction(pcprev->next);
			else 
				pc = findNextInstruction(pb->pcHead);
		} else
			pc = findNextInstruction(pc->next);
	} while(pc);
	
	if(matches)
		DFPRINTF((stderr," Optimizing pBlock: %c - matches=%d\n",getpBlock_dbName(pb),matches));
	return matches;
	
}

/*-----------------------------------------------------------------*/
/* pBlockRemoveUnusedLabels - remove the pCode labels from the     */
/*-----------------------------------------------------------------*/
pCode * findInstructionUsingLabel(pCodeLabel *pcl, pCode *pcs)
{
  pCode *pc;

  for(pc = pcs; pc; pc = pc->next) {

    if(((pc->type == PC_OPCODE) || (pc->type == PC_INLINE) || (pc->type == PC_ASMDIR)) &&
      (PCI(pc)->pcop) && 
      (PCI(pc)->pcop->type == PO_LABEL) &&
      (PCOLAB(PCI(pc)->pcop)->key == pcl->key))
      return pc;
  }

  return NULL;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void exchangeLabels(pCodeLabel *pcl, pCode *pc)
{
	
	char *s=NULL;
	
	if(isPCI(pc) && 
		(PCI(pc)->pcop) && 
		(PCI(pc)->pcop->type == PO_LABEL)) {
		
		pCodeOpLabel *pcol = PCOLAB(PCI(pc)->pcop);
		
		//fprintf(stderr,"changing label key from %d to %d\n",pcol->key, pcl->key);
		if(pcol->pcop.name)
			free(pcol->pcop.name);
		
			/* If the key is negative, then we (probably) have a label to
		* a function and the name is already defined */
		
		if(pcl->key>0)
			sprintf(s=buffer,"_%05d_DS_",pcl->key);
		else 
			s = pcl->label;
		
		//sprintf(buffer,"_%05d_DS_",pcl->key);
		if(!s) {
			fprintf(stderr, "ERROR %s:%d function label is null\n",__FUNCTION__,__LINE__);
		}
		pcol->pcop.name = Safe_strdup(s);
		pcol->key = pcl->key;
		//pc->print(stderr,pc);
		
	}
	
	
}

/*-----------------------------------------------------------------*/
/* pBlockRemoveUnusedLabels - remove the pCode labels from the     */
/*                            pCode chain if they're not used.     */
/*-----------------------------------------------------------------*/
void pBlockRemoveUnusedLabels(pBlock *pb)
{
	pCode *pc; pCodeLabel *pcl;
	
	if(!pb)
		return;
	
	for(pc = pb->pcHead; (pc=findNextInstruction(pc->next)) != NULL; ) {
		
		pBranch *pbr = PCI(pc)->label;
		if(pbr && pbr->next) {
			pCode *pcd = pb->pcHead;
			
			//fprintf(stderr, "multiple labels\n");
			//pc->print(stderr,pc);
			
			pbr = pbr->next;
			while(pbr) {
				
				while ( (pcd = findInstructionUsingLabel(PCL(PCI(pc)->label->pc), pcd)) != NULL) {
					//fprintf(stderr,"Used by:\n");
					//pcd->print(stderr,pcd);
					
					exchangeLabels(PCL(pbr->pc),pcd);
					
					pcd = pcd->next;
				}
				pbr = pbr->next;
			}
		}
	}
	
	for(pc = pb->pcHead; pc; pc = pc->next) {
		
		if(isPCL(pc)) // Label pcode
			pcl = PCL(pc);
		else if (isPCI(pc) && PCI(pc)->label) // pcode instruction with a label
			pcl = PCL(PCI(pc)->label->pc);
		else continue;
		
		//fprintf(stderr," found  A LABEL !!! key = %d, %s\n", pcl->key,pcl->label);
		
		/* This pCode is a label, so search the pBlock to see if anyone
		* refers to it */
		
		if( (pcl->key>0) && (!findInstructionUsingLabel(pcl, pb->pcHead))) {
			//if( !findInstructionUsingLabel(pcl, pb->pcHead)) {
			/* Couldn't find an instruction that refers to this label
			* So, unlink the pCode label from it's pCode chain
			* and destroy the label */
			//fprintf(stderr," removed  A LABEL !!! key = %d, %s\n", pcl->key,pcl->label);
			
			DFPRINTF((stderr," !!! REMOVED A LABEL !!! key = %d, %s\n", pcl->key,pcl->label));
			if(pc->type == PC_LABEL) {
				unlinkpCode(pc);
				pCodeLabelDestruct(pc);
			} else {
				unlinkpCodeFromBranch(pc, PCODE(pcl));
				/*if(pc->label->next == NULL && pc->label->pc == NULL) {
				free(pc->label);
			}*/
			}
			
		}
	}
	
}


/*-----------------------------------------------------------------*/
/* pBlockMergeLabels - remove the pCode labels from the pCode      */
/*                     chain and put them into pBranches that are  */
/*                     associated with the appropriate pCode       */
/*                     instructions.                               */
/*-----------------------------------------------------------------*/
void pBlockMergeLabels(pBlock *pb)
{
	pBranch *pbr;
	pCode *pc, *pcnext=NULL;
	
	if(!pb)
		return;
	
	/* First, Try to remove any unused labels */
	//pBlockRemoveUnusedLabels(pb);
	
	/* Now loop through the pBlock and merge the labels with the opcodes */
	
	pc = pb->pcHead;
	//  for(pc = pb->pcHead; pc; pc = pc->next) {
	
	while(pc) {
		pCode *pcn = pc->next;
		
		if(pc->type == PC_LABEL) {
			
			//fprintf(stderr," checking merging label %s\n",PCL(pc)->label);
			//fprintf(stderr,"Checking label key = %d\n",PCL(pc)->key);
			if((pcnext = findNextInstruction(pc) )) {
				
				// Unlink the pCode label from it's pCode chain
				unlinkpCode(pc);
				
				//fprintf(stderr,"Merged label key = %d\n",PCL(pc)->key);
				// And link it into the instruction's pBranch labels. (Note, since
				// it's possible to have multiple labels associated with one instruction
				// we must provide a means to accomodate the additional labels. Thus
				// the labels are placed into the singly-linked list "label" as 
				// opposed to being a single member of the pCodeInstruction.)
				
				//_ALLOC(pbr,sizeof(pBranch));
				pbr = Safe_calloc(1,sizeof(pBranch));
				pbr->pc = pc;
				pbr->next = NULL;
				
				PCI(pcnext)->label = pBranchAppend(PCI(pcnext)->label,pbr);
				
			} else {
				fprintf(stderr, "WARNING: couldn't associate label %s with an instruction\n",PCL(pc)->label);
			}
		} else if(pc->type == PC_CSOURCE) {
			
			/* merge the source line symbolic info into the next instruction */
			if((pcnext = findNextInstruction(pc) )) {
				
				// Unlink the pCode label from it's pCode chain
				unlinkpCode(pc);
				PCI(pcnext)->cline = PCCS(pc);
				//fprintf(stderr, "merging CSRC\n");
				//genericPrint(stderr,pcnext);
			}
			
		}
		pc = pcn;
	}
	pBlockRemoveUnusedLabels(pb);
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int OptimizepCode(char dbName)
{
#define MAX_PASSES 4
	
	int matches = 0;
	int passes = 0;
	pBlock *pb;
	
	if(!the_pFile)
		return 0;
	
	DFPRINTF((stderr," Optimizing pCode\n"));
	
	do {
		matches = 0;
		for(pb = the_pFile->pbHead; pb; pb = pb->next) {
			if('*' == dbName || getpBlock_dbName(pb) == dbName)
				matches += OptimizepBlock(pb);
		}
	}
	while(matches && ++passes < MAX_PASSES);
	
	return matches;
}

/*-----------------------------------------------------------------*/
/* popCopyGPR2Bit - copy a pcode operator                          */
/*-----------------------------------------------------------------*/

pCodeOp *popCopyGPR2Bit(pCodeOp *pc, int bitval)
{
	pCodeOp *pcop;
	
	pcop = newpCodeOpBit(pc->name, bitval, 0);
	
	if( !( (pcop->type == PO_LABEL) ||
		(pcop->type == PO_LITERAL) ||
		(pcop->type == PO_STR) ))
		PCOR(pcop)->r = PCOR(pc)->r;  /* This is dangerous... */
	
	return pcop;
}


/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static void FixRegisterBanking(pBlock *pb,int cur_bank)
{
	pCode *pc;
	int firstBank = 'U';
	
	if(!pb)
		return;
	
	for (pc=pb->pcHead; pc; pc=pc->next) {
		if (isPCFL(pc)) {
			firstBank = PCFL(pc)->firstBank;
			break;
		}
	}
	if (firstBank != 'U') {
		/* This block has already been done */
		if (firstBank != cur_bank) {
			/* This block has started with a different bank - must adjust it */ 
			if ((firstBank != -1)&&(firstBank != 'E')) { /* The first bank start off unknown or extern then need not worry as banksel will be called */
				while (pc) {
					if (isPCI(pc)) {
						regs *reg = getRegFromInstruction(pc);
						if (reg) {
							DoBankSelect(pc,cur_bank);
						}
					}
					pc = pc->next;
				}
			}
		}
		return;
	}
	
	/* loop through all of the pCodes within this pblock setting the bank selection, ignoring any branching */
	LastRegIdx = -1;
	cur_bank = -1;
	for (pc=pb->pcHead; pc; pc=pc->next) {
		if (isPCFL(pc)) {
			PCFL(pc)->firstBank = cur_bank;
			continue;
		}
		cur_bank = DoBankSelect(pc,cur_bank);
	}
	
	/* Trace through branches and set the bank selection as required. */
	LastRegIdx = -1;
	cur_bank = -1;
	for (pc=pb->pcHead; pc; pc=pc->next) {
		if (isPCFL(pc)) {
			PCFL(pc)->firstBank = cur_bank;
			continue;
		}
		if (isPCI(pc)) {
			if (PCI(pc)->op == POC_GOTO) {
				int lastRegIdx = LastRegIdx;
				pCode *pcb = pc;
				/* Trace through branch */
				pCode *pcl = findLabel(PCOLAB(PCI(pcb)->pcop));
				while (pcl) {
					if (isPCI(pcl)) {
						regs *reg = getRegFromInstruction(pcl);
						if (reg) {
							int bankUnknown = -1;
							if (IsBankChange(pcl,reg,&bankUnknown)) /* Look for any bank change */
								break;
							if (cur_bank != DoBankSelect(pcl,cur_bank)) /* Set bank selection if necessary */
								break;
						}
					}
					pcl = pcl->next;
				}
				LastRegIdx = lastRegIdx;
			} else {
				/* Keep track out current bank */
				regs *reg = getRegFromInstruction(pc);
				if (reg)
					IsBankChange(pc,reg,&cur_bank);
			}
		}
	}
}


/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void pBlockDestruct(pBlock *pb)
{
	
	if(!pb)
		return;
	
	
	free(pb);
	
}

/*-----------------------------------------------------------------*/
/* void mergepBlocks(char dbName) - Search for all pBlocks with the*/
/*                                  name dbName and combine them   */
/*                                  into one block                 */
/*-----------------------------------------------------------------*/
void mergepBlocks(char dbName)
{
	
	pBlock *pb, *pbmerged = NULL,*pbn;
	
	pb = the_pFile->pbHead;
	
	//fprintf(stderr," merging blocks named %c\n",dbName);
	while(pb) {
		
		pbn = pb->next;
		//fprintf(stderr,"looking at %c\n",getpBlock_dbName(pb));
		if( getpBlock_dbName(pb) == dbName) {
			
			//fprintf(stderr," merged block %c\n",dbName);
			
			if(!pbmerged) {
				pbmerged = pb;
			} else {
				addpCode2pBlock(pbmerged, pb->pcHead);
				/* addpCode2pBlock doesn't handle the tail: */
				pbmerged->pcTail = pb->pcTail;
				
				pb->prev->next = pbn;
				if(pbn) 
					pbn->prev = pb->prev;
				
				
				pBlockDestruct(pb);
			}
			//printpBlock(stderr, pbmerged);
		} 
		pb = pbn;
	}
	
}

/*-----------------------------------------------------------------*/
/* AnalyzeFlow - Examine the flow of the code and optimize         */
/*                                                                 */
/* level 0 == minimal optimization                                 */
/*   optimize registers that are used only by two instructions     */
/* level 1 == maximal optimization                                 */
/*   optimize by looking at pairs of instructions that use the     */
/*   register.                                                     */
/*-----------------------------------------------------------------*/

void AnalyzeFlow(int level)
{
	static int times_called=0;
	
	pBlock *pb;
	
	if(!the_pFile)
		return;
	
	
		/* if this is not the first time this function has been called,
	then clean up old flow information */
	if(times_called++) {
		for(pb = the_pFile->pbHead; pb; pb = pb->next)
			unBuildFlow(pb);
		
		RegsUnMapLiveRanges();
		
	}
	
	GpcFlowSeq = 1;
	
	/* Phase 2 - Flow Analysis - Register Banking
	*
	* In this phase, the individual flow blocks are examined
	* and register banking is fixed.
	*/
	
	//for(pb = the_pFile->pbHead; pb; pb = pb->next)
	//FixRegisterBanking(pb);
	
	/* Phase 2 - Flow Analysis
	*
	* In this phase, the pCode is partition into pCodeFlow 
	* blocks. The flow blocks mark the points where a continuous
	* stream of instructions changes flow (e.g. because of
	* a call or goto or whatever).
	*/
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
		BuildFlow(pb);
	
	
		/* Phase 2 - Flow Analysis - linking flow blocks
		*
		* In this phase, the individual flow blocks are examined
		* to determine their order of excution.
	*/
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
		LinkFlow(pb);
	
		/* Phase 3 - Flow Analysis - Flow Tree
		*
		* In this phase, the individual flow blocks are examined
		* to determine their order of excution.
	*/
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
		BuildFlowTree(pb);
	
	
		/* Phase x - Flow Analysis - Used Banks
		*
		* In this phase, the individual flow blocks are examined
		* to determine the Register Banks they use
	*/
	
	//  for(pb = the_pFile->pbHead; pb; pb = pb->next)
	//    FixBankFlow(pb);
	
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
		pCodeRegMapLiveRanges(pb);
	
	RemoveUnusedRegisters();
	
	//  for(pb = the_pFile->pbHead; pb; pb = pb->next)
	pCodeRegOptimizeRegUsage(level);
	
	OptimizepCode('*');
	
	/*	
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
	DumpFlow(pb);
	*/
	/* debug stuff */
	/*
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
    pCode *pcflow;
    for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
    (pcflow = findNextpCode(pcflow, PC_FLOW)) != NULL;
    pcflow = pcflow->next) {
    
	  FillFlow(PCFL(pcflow));
	  }
	  }
	*/
	/*
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
    pCode *pcflow;
    for( pcflow = findNextpCode(pb->pcHead, PC_FLOW); 
    (pcflow = findNextpCode(pcflow, PC_FLOW)) != NULL;
    pcflow = pcflow->next) {
	
	  FlowStats(PCFL(pcflow));
	  }
	  }
	*/
}

/*-----------------------------------------------------------------*/
/* AnalyzeBanking - Called after the memory addresses have been    */
/*                  assigned to the registers.                     */
/*                                                                 */
/*-----------------------------------------------------------------*/

void AnalyzeBanking(void)
{
	pBlock  *pb;

	if(!picIsInitialized()) {
		setDefMaxRam(); // Max RAM has not been included, so use default setting
	}
	
	if (!the_pFile) return;
	
	/* Phase x - Flow Analysis - Used Banks
	*
	* In this phase, the individual flow blocks are examined
	* to determine the Register Banks they use
	*/
	
	AnalyzeFlow(0);
	AnalyzeFlow(1);
	
	//  for(pb = the_pFile->pbHead; pb; pb = pb->next)
	//    BanksUsedFlow(pb);
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
		FixRegisterBanking(pb,-1); // cur_bank is unknown
	
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
DEFSETFUNC (resetrIdx)
{
	regs *r = (regs *)item;
	if (!r->isFixed) {
		r->rIdx = 0;
	}
	
	return 0;
}

/*-----------------------------------------------------------------*/
/* InitRegReuse - Initialises variables for code analyzer          */
/*-----------------------------------------------------------------*/

void InitReuseReg(void)
{
	/* Find end of statically allocated variables for start idx */
	unsigned maxIdx = 0x20; /* Start from begining of GPR. Note may not be 0x20 on some PICs */
	regs *r;
	for (r = setFirstItem(dynDirectRegs); r; r = setNextItem(dynDirectRegs)) {
		if (r->type != REG_SFR) {
			maxIdx += r->size; /* Increment for all statically allocated variables */
		}
	}
	peakIdx = maxIdx;
	applyToSet(dynAllocRegs,resetrIdx); /* Reset all rIdx to zero. */
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static unsigned register_reassign(pBlock *pb, unsigned idx)
{
	pCode *pc;
	
	/* check recursion */
	pc = setFirstItem(pb->function_entries);
	if(!pc)
		return idx;
	
	pb->visited = 1;
	
	DFPRINTF((stderr," reassigning registers for function \"%s\"\n",PCF(pc)->fname));
	
	if (pb->tregisters) {
		regs *r;
		for (r = setFirstItem(pb->tregisters); r; r = setNextItem(pb->tregisters)) {
			if (r->type == REG_GPR) {
				if (!r->isFixed) {
					if (r->rIdx < (int)idx) {
						char s[20];
						r->rIdx = idx++;
						if (peakIdx < idx) peakIdx = idx;
						sprintf(s,"r0x%02X", r->rIdx);
						DFPRINTF((stderr," reassigning register \"%s\" to \"%s\"\n",r->name,s));
						free(r->name);
						r->name = Safe_strdup(s);
					}
				}
			}
		}
	}
	for(pc = setFirstItem(pb->function_calls); pc; pc = setNextItem(pb->function_calls)) {
		
		if(pc->type == PC_OPCODE && PCI(pc)->op == POC_CALL) {
			char *dest = get_op_from_instruction(PCI(pc));
			
			pCode *pcn = findFunction(dest);
			if(pcn) {
				register_reassign(pcn->pb,idx);
			}
		}
		
	}
	
	return idx;
}

/*------------------------------------------------------------------*/
/* ReuseReg were call tree permits                                  */
/*                                                                  */
/*  Re-allocate the GPR for optimum reuse for a given pblock        */ 
/*  eg  if a function m() calls function f1() and f2(), where f1    */
/*  allocates a local variable vf1 and f2 allocates a local         */
/*  variable vf2. Then providing f1 and f2 do not call each other   */
/*  they may share the same general purpose registers for vf1 and   */
/*  vf2.                                                            */
/*  This is done by first setting the the regs rIdx to start after  */
/*  all the global variables, then walking through the call tree    */
/*  renaming the registers to match their new idx and incrementng   */
/*  it as it goes. If a function has already been called it will    */
/*  only rename the registers if it has already used up those       */
/*  registers ie rIdx of the function's registers is lower than the */
/*  current rIdx. That way the register will not be reused while    */
/*  still being used by an eariler function call.                   */
/*                                                                  */
/*  Note for this to work the functions need to be declared static. */
/*                                                                  */
/*------------------------------------------------------------------*/
void ReuseReg(void)
{
	pBlock  *pb;
	InitReuseReg();
	if (!the_pFile) return;
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		/* Non static functions can be called from other modules so their registers must reassign */
		if (pb->function_entries&&(PCF(setFirstItem(pb->function_entries))->isPublic||!pb->visited))
			register_reassign(pb,peakIdx);
	}
}

/*-----------------------------------------------------------------*/
/* buildCallTree - look at the flow and extract all of the calls   */
/*                                                                 */
/*-----------------------------------------------------------------*/

void buildCallTree(void    )
{
	pBranch *pbr;
	pBlock  *pb;
	pCode   *pc;
	
	if(!the_pFile)
		return;
	
		/* Now build the call tree.
		First we examine all of the pCodes for functions.
		Keep in mind that the function boundaries coincide
		with pBlock boundaries. 
		
		  The algorithm goes something like this:
		  We have two nested loops. The outer loop iterates
		  through all of the pBlocks/functions. The inner
		  loop iterates through all of the pCodes for
		  a given pBlock. When we begin iterating through
		  a pBlock, the variable pc_fstart, pCode of the start
		  of a function, is cleared. We then search for pCodes
		  of type PC_FUNCTION. When one is encountered, we
		  initialize pc_fstart to this and at the same time
		  associate a new pBranch object that signifies a 
		  branch entry. If a return is found, then this signifies
		  a function exit point. We'll link the pCodes of these
		  returns to the matching pc_fstart.
		  
			When we're done, a doubly linked list of pBranches
			will exist. The head of this list is stored in
			`the_pFile', which is the meta structure for all
			of the pCode. Look at the printCallTree function
			on how the pBranches are linked together.
			
	*/
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		pCode *pc_fstart=NULL;
		for(pc = pb->pcHead; pc; pc = pc->next) {
			if(isPCF(pc)) {
				pCodeFunction *pcf = PCF(pc);
				if (pcf->fname) {
					
					if(STRCASECMP(pcf->fname, "_main") == 0) {
						//fprintf(stderr," found main \n");
						pb->cmemmap = NULL;  /* FIXME do we need to free ? */
						pb->dbName = 'M';
					}
					
					pbr = Safe_calloc(1,sizeof(pBranch));
					pbr->pc = pc_fstart = pc;
					pbr->next = NULL;
					
					the_pFile->functions = pBranchAppend(the_pFile->functions,pbr);
					
					// Here's a better way of doing the same:
					addSet(&pb->function_entries, pc);
					
				} else {
					// Found an exit point in a function, e.g. return
					// (Note, there may be more than one return per function)
					if(pc_fstart)
						pBranchLink(PCF(pc_fstart), pcf);
					
					addSet(&pb->function_exits, pc);
				}
			} else if(isCALL(pc)) {
				addSet(&pb->function_calls,pc);
			}
		}
	}
}

/*-----------------------------------------------------------------*/
/* AnalyzepCode - parse the pCode that has been generated and form */
/*                all of the logical connections.                  */
/*                                                                 */
/* Essentially what's done here is that the pCode flow is          */
/* determined.                                                     */
/*-----------------------------------------------------------------*/

void AnalyzepCode(char dbName)
{
	pBlock *pb;
	int i,changes;
	
	if(!the_pFile)
		return;
	
	mergepBlocks('D');
	
	
	/* Phase 1 - Register allocation and peep hole optimization
	*
	* The first part of the analysis is to determine the registers
	* that are used in the pCode. Once that is done, the peep rules
	* are applied to the code. We continue to loop until no more
	* peep rule optimizations are found (or until we exceed the
	* MAX_PASSES threshold). 
	*
	* When done, the required registers will be determined.
	*
	*/
	i = 0;
	do {
		
		DFPRINTF((stderr," Analyzing pCode: PASS #%d\n",i+1));
		
		/* First, merge the labels with the instructions */
		for(pb = the_pFile->pbHead; pb; pb = pb->next) {
			if('*' == dbName || getpBlock_dbName(pb) == dbName) {
				
				DFPRINTF((stderr," analyze and merging block %c\n",dbName));
				pBlockMergeLabels(pb);
				AnalyzepBlock(pb);
			} else {
				DFPRINTF((stderr," skipping block analysis dbName=%c blockname=%c\n",dbName,getpBlock_dbName(pb)));
			}
		}
		
		changes = OptimizepCode(dbName);
		
	} while(changes && (i++ < MAX_PASSES));
	
	buildCallTree();
}

/*-----------------------------------------------------------------*/
/* ispCodeFunction - returns true if *pc is the pCode of a         */
/*                   function                                      */
/*-----------------------------------------------------------------*/
bool ispCodeFunction(pCode *pc)
{
	
	if(pc && pc->type == PC_FUNCTION && PCF(pc)->fname)
		return 1;
	
	return 0;
}

/*-----------------------------------------------------------------*/
/* findFunction - Search for a function by name (given the name)   */
/*                in the set of all functions that are in a pBlock */
/* (note - I expect this to change because I'm planning to limit   */
/*  pBlock's to just one function declaration                      */
/*-----------------------------------------------------------------*/
pCode *findFunction(char *fname)
{
	pBlock *pb;
	pCode *pc;
	if(!fname)
		return NULL;
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		
		pc = setFirstItem(pb->function_entries);
		while(pc) {
			
			if((pc->type == PC_FUNCTION) &&
				(PCF(pc)->fname) && 
				(strcmp(fname, PCF(pc)->fname)==0))
				return pc;
			
			pc = setNextItem(pb->function_entries);
			
		}
		
	}
	return NULL;
}

void MarkUsedRegisters(set *regset)
{
	
	regs *r1,*r2;
	
	for(r1=setFirstItem(regset); r1; r1=setNextItem(regset)) {
		r2 = pic14_regWithIdx(r1->rIdx);
		if (r2) {
			r2->isFree = 0;
			r2->wasUsed = 1;
		}
	}
}

void pBlockStats(FILE *of, pBlock *pb)
{
	
	pCode *pc;
	regs  *r;
	
	fprintf(of,";***\n;  pBlock Stats: dbName = %c\n;***\n",getpBlock_dbName(pb));
	
	// for now just print the first element of each set
	pc = setFirstItem(pb->function_entries);
	if(pc) {
		fprintf(of,";entry:  ");
		pc->print(of,pc);
	}
	pc = setFirstItem(pb->function_exits);
	if(pc) {
		fprintf(of,";has an exit\n");
		//pc->print(of,pc);
	}
	
	pc = setFirstItem(pb->function_calls);
	if(pc) {
		fprintf(of,";functions called:\n");
		
		while(pc) {
			if(pc->type == PC_OPCODE && PCI(pc)->op == POC_CALL) {
				fprintf(of,";   %s\n",get_op_from_instruction(PCI(pc)));
			}
			pc = setNextItem(pb->function_calls);
		}
	}
	
	r = setFirstItem(pb->tregisters);
	if(r) {
		int n = elementsInSet(pb->tregisters);
		
		fprintf(of,";%d compiler assigned register%c:\n",n, ( (n!=1) ? 's' : ' '));
		
		while (r) {
			fprintf(of,";   %s\n",r->name);
			r = setNextItem(pb->tregisters);
		}
	}
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
#if 0
static void sequencepCode(void)
{
	pBlock *pb;
	pCode *pc;
	
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		
		pb->seq = GpCodeSequenceNumber+1;
		
		for( pc = pb->pcHead; pc; pc = pc->next)
			pc->seq = ++GpCodeSequenceNumber;
	}
	
}
#endif

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*
set *register_usage(pBlock *pb)
{
	pCode *pc,*pcn;
	set *registers=NULL;
	set *registersInCallPath = NULL;
	
	/ * check recursion * /
		
		pc = setFirstItem(pb->function_entries);
	
	if(!pc)
		return registers;
	
	pb->visited = 1;
	
	if(pc->type != PC_FUNCTION)
		fprintf(stderr,"%s, first pc is not a function???\n",__FUNCTION__);
	
	pc = setFirstItem(pb->function_calls);
	for( ; pc; pc = setNextItem(pb->function_calls)) {
		
		if(pc->type == PC_OPCODE && PCI(pc)->op == POC_CALL) {
			char *dest = get_op_from_instruction(PCI(pc));
			
			pcn = findFunction(dest);
			if(pcn) 
				registersInCallPath = register_usage(pcn->pb);
		} else
			fprintf(stderr,"BUG? pCode isn't a POC_CALL %d\n",__LINE__);
		
	}
	
#ifdef PCODE_DEBUG
	pBlockStats(stderr,pb);  // debug
#endif
	
	// Mark the registers in this block as used.
	
	MarkUsedRegisters(pb->tregisters);
	if(registersInCallPath) {
		/ * registers were used in the functions this pBlock has called * /
		/ * so now, we need to see if these collide with the ones we are * /
		/ * using here * /
		
			regs *r1,*r2, *newreg;
		
		DFPRINTF((stderr,"comparing registers\n"));
		
		r1 = setFirstItem(registersInCallPath);
		while(r1) {
			if (r1->type != REG_STK) {
				r2 = setFirstItem(pb->tregisters);
				
				while(r2 && (r2->type != REG_STK)) {
					
					if(r2->rIdx == r1->rIdx) {
						newreg = pic14_findFreeReg(REG_GPR);
						
						
						if(!newreg) {
							DFPRINTF((stderr,"Bummer, no more registers.\n"));
							exit(1);
						}
						
						DFPRINTF((stderr,"Cool found register collision nIdx=%d moving to %d\n",
							r1->rIdx, newreg->rIdx));
						r2->rIdx = newreg->rIdx;
						if(newreg->name)
							r2->name = Safe_strdup(newreg->name);
						else
							r2->name = NULL;
						newreg->isFree = 0;
						newreg->wasUsed = 1;
					}
					r2 = setNextItem(pb->tregisters);
				}
			}
			
			r1 = setNextItem(registersInCallPath);
		}
		
		/ * Collisions have been resolved. Now free the registers in the call path * /
		r1 = setFirstItem(registersInCallPath);
		while(r1) {
			newreg = pic14_regWithIdx(r1->rIdx);
			if (newreg) newreg->isFree = 1;
			r1 = setNextItem(registersInCallPath);
		}
		
	}// else
	//	MarkUsedRegisters(pb->registers);
	
	registers = unionSets(pb->tregisters, registersInCallPath, THROW_NONE);
#ifdef PCODE_DEBUG
	if(registers) 
		DFPRINTF((stderr,"returning regs\n"));
	else
		DFPRINTF((stderr,"not returning regs\n"));
	
	DFPRINTF((stderr,"pBlock after register optim.\n"));
	pBlockStats(stderr,pb);  // debug
#endif
	
	return registers;
}
*/

/*-----------------------------------------------------------------*/
/* printCallTree - writes the call tree to a file                  */
/*                                                                 */
/*-----------------------------------------------------------------*/
void pct2(FILE *of,pBlock *pb,int indent)
{
	pCode *pc,*pcn;
	int i;
	//  set *registersInCallPath = NULL;
	
	if(!of)
		return;
	
	if(indent > 10)
		return; //recursion ?
	
	pc = setFirstItem(pb->function_entries);
	
	if(!pc)
		return;
	
	pb->visited = 0;
	
	for(i=0;i<indent;i++)   // Indentation
		fputc(' ',of);
	
	if(pc->type == PC_FUNCTION)
		fprintf(of,"%s\n",PCF(pc)->fname);
	else
		return;  // ???
	
	
	pc = setFirstItem(pb->function_calls);
	for( ; pc; pc = setNextItem(pb->function_calls)) {
		
		if(pc->type == PC_OPCODE && PCI(pc)->op == POC_CALL) {
			char *dest = get_op_from_instruction(PCI(pc));
			
			pcn = findFunction(dest);
			if(pcn) 
				pct2(of,pcn->pb,indent+1);
		} else
			fprintf(of,"BUG? pCode isn't a POC_CALL %d\n",__LINE__);
		
	}
	
	
}


/*-----------------------------------------------------------------*/
/* printCallTree - writes the call tree to a file                  */
/*                                                                 */
/*-----------------------------------------------------------------*/

void printCallTree(FILE *of)
{
	pBranch *pbr;
	pBlock  *pb;
	pCode   *pc;
	
	if(!the_pFile)
		return;
	
	if(!of)
		of = stderr;
	
	fprintf(of, "\npBlock statistics\n");
	for(pb = the_pFile->pbHead; pb;  pb = pb->next )
		pBlockStats(of,pb);
	
	
	
	fprintf(of,"Call Tree\n");
	pbr = the_pFile->functions;
	while(pbr) {
		if(pbr->pc) {
			pc = pbr->pc;
			if(!ispCodeFunction(pc))
				fprintf(of,"bug in call tree");
			
			
			fprintf(of,"Function: %s\n", PCF(pc)->fname);
			
			while(pc->next && !ispCodeFunction(pc->next)) {
				pc = pc->next;
				if(pc->type == PC_OPCODE && PCI(pc)->op == POC_CALL)
					fprintf(of,"\t%s\n",get_op_from_instruction(PCI(pc)));
			}
		}
		
		pbr = pbr->next;
	}
	
	
	fprintf(of,"\n**************\n\na better call tree\n");
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		if(pb->visited)
			pct2(of,pb,0);
	}
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		fprintf(of,"block dbname: %c\n", getpBlock_dbName(pb));
	}
}



/*-----------------------------------------------------------------*/
/*                                                                 */
/*-----------------------------------------------------------------*/

void InlineFunction(pBlock *pb)
{
	pCode *pc;
	pCode *pc_call;
	
	if(!pb)
		return;
	
	pc = setFirstItem(pb->function_calls);
	
	for( ; pc; pc = setNextItem(pb->function_calls)) {
		
		if(isCALL(pc)) {
			pCode *pcn = findFunction(get_op_from_instruction(PCI(pc)));
			pCode *pcp = pc->prev;
			pCode *pct;
			pCode *pce;
			
			pBranch *pbr;
			
			if(pcn && isPCF(pcn) && (PCF(pcn)->ncalled == 1) && !PCF(pcn)->isPublic && (pcp && (isPCI_BITSKIP(pcp)||!isPCI_SKIP(pcp)))) { /* Bit skips can be inverted other skips can not */
				
				InlineFunction(pcn->pb);
				
				/*
				At this point, *pc points to a CALL mnemonic, and
				*pcn points to the function that is being called.
				
				  To in-line this call, we need to remove the CALL
				  and RETURN(s), and link the function pCode in with
				  the CALLee pCode.
				  
				*/
				
				pc_call = pc;
				
				/* Check if previous instruction was a bit skip */
				if (isPCI_BITSKIP(pcp)) {
					pCodeLabel *pcl;
					/* Invert skip instruction and add a goto */
					PCI(pcp)->op = (PCI(pcp)->op == POC_BTFSS) ? POC_BTFSC : POC_BTFSS;
					
					if(isPCL(pc_call->next)) { // Label pcode
						pcl = PCL(pc_call->next);
					} else if (isPCI(pc_call->next) && PCI(pc_call->next)->label) { // pcode instruction with a label
						pcl = PCL(PCI(pc_call->next)->label->pc);
					} else {
						pcl = PCL(newpCodeLabel(NULL, newiTempLabel(NULL)->key+100));
						PCI(pc_call->next)->label->pc = (struct pCode*)pcl;
					}
					pCodeInsertAfter(pcp, newpCode(POC_GOTO, newpCodeOp(pcl->label,PO_STR)));
				}
				
				/* remove callee pBlock from the pBlock linked list */
				removepBlock(pcn->pb);
				
				pce = pcn;
				while(pce) {
					pce->pb = pb;
					pce = pce->next;
				}
				
				/* Remove the Function pCode */
				pct = findNextInstruction(pcn->next);
				
				/* Link the function with the callee */
				if (pcp) pcp->next = pcn->next;
				pcn->next->prev = pcp;
				
				/* Convert the function name into a label */
				
				pbr = Safe_calloc(1,sizeof(pBranch));
				pbr->pc = newpCodeLabel(PCF(pcn)->fname, -1);
				pbr->next = NULL;
				PCI(pct)->label = pBranchAppend(PCI(pct)->label,pbr);
				PCI(pct)->label = pBranchAppend(PCI(pct)->label,PCI(pc_call)->label);
				
				/* turn all of the return's except the last into goto's */
				/* check case for 2 instruction pBlocks */
				pce = findNextInstruction(pcn->next);
				while(pce) {
					pCode *pce_next = findNextInstruction(pce->next);
					
					if(pce_next == NULL) {
						/* found the last return */
						pCode *pc_call_next =  findNextInstruction(pc_call->next);
						
						//fprintf(stderr,"found last return\n");
						//pce->print(stderr,pce);
						pce->prev->next = pc_call->next;
						pc_call->next->prev = pce->prev;
						PCI(pc_call_next)->label = pBranchAppend(PCI(pc_call_next)->label,
							PCI(pce)->label);
					}
					
					pce = pce_next;
				}
				
			}
		} else
			fprintf(stderr,"BUG? pCode isn't a POC_CALL %d\n",__LINE__);
		
	}
	
}

/*-----------------------------------------------------------------*/
/*                                                                 */
/*-----------------------------------------------------------------*/

void InlinepCode(void)
{
	
	pBlock  *pb;
	pCode   *pc;
	
	if(!the_pFile)
		return;
	
	if(!functionInlining)
		return;
	
		/* Loop through all of the function definitions and count the
	* number of times each one is called */
	//fprintf(stderr,"inlining %d\n",__LINE__);
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next) {
		
		pc = setFirstItem(pb->function_calls);
		
		for( ; pc; pc = setNextItem(pb->function_calls)) {
			
			if(isCALL(pc)) {
				pCode *pcn = findFunction(get_op_from_instruction(PCI(pc)));
				if(pcn && isPCF(pcn)) {
					PCF(pcn)->ncalled++;
				}
			} else
				fprintf(stderr,"BUG? pCode isn't a POC_CALL %d\n",__LINE__);
			
		}
	}
	
	//fprintf(stderr,"inlining %d\n",__LINE__);
	
	/* Now, Loop through the function definitions again, but this
	* time inline those functions that have only been called once. */
	
	InlineFunction(the_pFile->pbHead);
	//fprintf(stderr,"inlining %d\n",__LINE__);
	
	for(pb = the_pFile->pbHead; pb; pb = pb->next)
		unBuildFlow(pb);
	
}
