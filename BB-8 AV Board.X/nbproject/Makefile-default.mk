#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=FSIO.c SD-SPI.c XGS_PIC_PWM_SOUND.c XGS_PIC_SYSTEM_V010.c main.c XGS_PIC_UART_DRV_V010.c exception.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/FSIO.o ${OBJECTDIR}/SD-SPI.o ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o ${OBJECTDIR}/main.o ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o ${OBJECTDIR}/exception.o
POSSIBLE_DEPFILES=${OBJECTDIR}/FSIO.o.d ${OBJECTDIR}/SD-SPI.o.d ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d ${OBJECTDIR}/exception.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/FSIO.o ${OBJECTDIR}/SD-SPI.o ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o ${OBJECTDIR}/main.o ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o ${OBJECTDIR}/exception.o

# Source Files
SOURCEFILES=FSIO.c SD-SPI.c XGS_PIC_PWM_SOUND.c XGS_PIC_SYSTEM_V010.c main.c XGS_PIC_UART_DRV_V010.c exception.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
	${MAKE} ${MAKE_OPTIONS} -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24HJ256GP206
MP_LINKER_FILE_OPTION=,-Tp24HJ256GP206.gld
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/FSIO.o: FSIO.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/FSIO.o.d 
	@${RM} ${OBJECTDIR}/FSIO.o.ok ${OBJECTDIR}/FSIO.o.err 
	@${RM} ${OBJECTDIR}/FSIO.o 
	@${FIXDEPS} "${OBJECTDIR}/FSIO.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/FSIO.o.d" -o ${OBJECTDIR}/FSIO.o FSIO.c    
	
${OBJECTDIR}/SD-SPI.o: SD-SPI.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/SD-SPI.o.d 
	@${RM} ${OBJECTDIR}/SD-SPI.o.ok ${OBJECTDIR}/SD-SPI.o.err 
	@${RM} ${OBJECTDIR}/SD-SPI.o 
	@${FIXDEPS} "${OBJECTDIR}/SD-SPI.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/SD-SPI.o.d" -o ${OBJECTDIR}/SD-SPI.o SD-SPI.c    
	
${OBJECTDIR}/XGS_PIC_PWM_SOUND.o: XGS_PIC_PWM_SOUND.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d 
	@${RM} ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.ok ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.err 
	@${RM} ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o 
	@${FIXDEPS} "${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d" -o ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o XGS_PIC_PWM_SOUND.c    
	
${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o: XGS_PIC_SYSTEM_V010.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d 
	@${RM} ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.ok ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.err 
	@${RM} ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o 
	@${FIXDEPS} "${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d" -o ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o XGS_PIC_SYSTEM_V010.c    
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o.ok ${OBJECTDIR}/main.o.err 
	@${RM} ${OBJECTDIR}/main.o 
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c    
	
${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o: XGS_PIC_UART_DRV_V010.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d 
	@${RM} ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.ok ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.err 
	@${RM} ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o 
	@${FIXDEPS} "${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d" -o ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o XGS_PIC_UART_DRV_V010.c    
	
${OBJECTDIR}/exception.o: exception.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/exception.o.d 
	@${RM} ${OBJECTDIR}/exception.o.ok ${OBJECTDIR}/exception.o.err 
	@${RM} ${OBJECTDIR}/exception.o 
	@${FIXDEPS} "${OBJECTDIR}/exception.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/exception.o.d" -o ${OBJECTDIR}/exception.o exception.c    
	
else
${OBJECTDIR}/FSIO.o: FSIO.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/FSIO.o.d 
	@${RM} ${OBJECTDIR}/FSIO.o.ok ${OBJECTDIR}/FSIO.o.err 
	@${RM} ${OBJECTDIR}/FSIO.o 
	@${FIXDEPS} "${OBJECTDIR}/FSIO.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/FSIO.o.d" -o ${OBJECTDIR}/FSIO.o FSIO.c    
	
${OBJECTDIR}/SD-SPI.o: SD-SPI.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/SD-SPI.o.d 
	@${RM} ${OBJECTDIR}/SD-SPI.o.ok ${OBJECTDIR}/SD-SPI.o.err 
	@${RM} ${OBJECTDIR}/SD-SPI.o 
	@${FIXDEPS} "${OBJECTDIR}/SD-SPI.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/SD-SPI.o.d" -o ${OBJECTDIR}/SD-SPI.o SD-SPI.c    
	
${OBJECTDIR}/XGS_PIC_PWM_SOUND.o: XGS_PIC_PWM_SOUND.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d 
	@${RM} ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.ok ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.err 
	@${RM} ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o 
	@${FIXDEPS} "${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/XGS_PIC_PWM_SOUND.o.d" -o ${OBJECTDIR}/XGS_PIC_PWM_SOUND.o XGS_PIC_PWM_SOUND.c    
	
${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o: XGS_PIC_SYSTEM_V010.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d 
	@${RM} ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.ok ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.err 
	@${RM} ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o 
	@${FIXDEPS} "${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o.d" -o ${OBJECTDIR}/XGS_PIC_SYSTEM_V010.o XGS_PIC_SYSTEM_V010.c    
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o.ok ${OBJECTDIR}/main.o.err 
	@${RM} ${OBJECTDIR}/main.o 
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c    
	
${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o: XGS_PIC_UART_DRV_V010.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d 
	@${RM} ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.ok ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.err 
	@${RM} ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o 
	@${FIXDEPS} "${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o.d" -o ${OBJECTDIR}/XGS_PIC_UART_DRV_V010.o XGS_PIC_UART_DRV_V010.c    
	
${OBJECTDIR}/exception.o: exception.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/exception.o.d 
	@${RM} ${OBJECTDIR}/exception.o.ok ${OBJECTDIR}/exception.o.err 
	@${RM} ${OBJECTDIR}/exception.o 
	@${FIXDEPS} "${OBJECTDIR}/exception.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c ${MP_CC} $(MP_EXTRA_CC_PRE)  -g -omf=elf -x c -c -mcpu=$(MP_PROCESSOR_OPTION) -Wall -I"../../BB-8 Audio" -I"." -MMD -MF "${OBJECTDIR}/exception.o.d" -o ${OBJECTDIR}/exception.o exception.c    
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -omf=elf -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -o dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}         -Wl,--defsym=__MPLAB_BUILD=1,-L"C:/Program Files (x86)/Microchip/MPLAB C30/lib",-L".",-Map="${DISTDIR}/BB-8_AV_Board.X.${IMAGE_TYPE}.map",--report-mem$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_PK3=1
else
dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -omf=elf -mcpu=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}         -Wl,--defsym=__MPLAB_BUILD=1,-L"C:/Program Files (x86)/Microchip/MPLAB C30/lib",-L".",-Map="${DISTDIR}/BB-8_AV_Board.X.${IMAGE_TYPE}.map",--report-mem$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION)
	${MP_CC_DIR}\\pic30-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/BB-8_AV_Board.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -omf=elf
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
