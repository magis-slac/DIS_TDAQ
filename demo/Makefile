################################################################################
# Makefile for acquireImages.cpp
################################################################################
PROJECT_ROOT=./

################################################################################
# Key paths and settings
################################################################################
CFLAGS += -std=c++11 -g
ifeq ($(wildcard ${OPT_INC}),)
CXX = g++ ${CFLAGS}
ODIR  = .obj/build
SDIR  = .
MKDIR = mkdir -p
PLATFORM = $(shell uname)
ifeq ($(PLATFORM),Darwin)
OS = mac
endif
endif

# OUTPUTNAME = acquireImages prepareCamera
# OUTPUTNAME = $(patsubst %,%,$(_OUTPUTNAME))

OUTDIR = .

################################################################################
# Dependencies
################################################################################
# Library dependencies
SPINNAKER_LIB = -L/opt/spinnaker/lib -lSpinnaker
LGPIO_LIB = -lpigpio

################################################################################
# Master inc/lib/obj/dep settings
################################################################################
_OBJ = acquireImages.o prepareCamera.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
INC = -I./include
INC += -I/opt/spinnaker/include
LIB += -Wl,-Bdynamic ${LGPIO_LIB} ${SPINNAKER_LIB}
 
################################################################################
# Rules/recipes
################################################################################
# Final binary

all: acquireImages prepareCamera

acquireImages: % : ${ODIR}/%.o
	${CXX} -o $@ $< ${LIB}
	

prepareCamera: % : ${ODIR}/%.o
	${CXX} -o $@ $< ${LIB}


# Intermediate object files
${OBJ}: ${ODIR}/%.o : ${SDIR}/%.cpp
	@${MKDIR} ${ODIR}
	${CXX} ${CFLAGS} ${INC} -Wall -D LINUX -c $< -o $@

# Clean up intermediate objects
clean_obj:
	rm -f ${OBJ}
	@echo "intermediate objects cleaned up!"

# Clean up everything.
clean: clean_obj
	rm -f ${OUTDIR}/acquireImages
	rm -f ${OUTDIR}/prepareCamera
	@echo "all cleaned up!"



