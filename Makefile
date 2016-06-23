#
# $Id: Makefile,v 3.31 2016/05/09 by RY$
#
### Command line options for make ###
#
### FOR LINUX ##
#ENV = GCC
#ENV = ICC
#ENV = ICC_MKL_OMP
#
### FOR WINDOWS ###
#ENV = CYGWIN
#ENV = MINGW64
#ENV = MINGW
#
### FOR MAC ###
#ENV = GCC_MAC
#ENV = CLANG
#
### HDF5 SUPPORT ###
#HDF5 = ON

## default options
# Use OCTA environment variables
#GOURMET_HOME_PATH = $(PF_FILES)
#ENGINE_HOME_PATH  = $(PF_ENGINE)
#ARCH              = $(PF_ENGINEARCH)
# OR
# Define environment variables explicitly here
GOURMET_HOME_PATH  = /usr/local/OCTA8/GOURMET
ENGINE_HOME_PATH   = /usr/local/OCTA8/ENGINES
ARCH               = linux_64
#
AUX= ./Tools
CC     = gcc
CXX    = g++
CCOPT  = -O
LINKS  = -lm -lplatform -lstdc++
GOURMET_LIB_PATH = $(GOURMET_HOME_PATH)/lib/$(ARCH)
GOURMET_INCLUDE_PATH = $(GOURMET_HOME_PATH)/include
TARGET_DIR=$(ENGINE_HOME_PATH)/bin/$(ARCH)
OSTYPE = $(shell uname)

ifeq ($(ENV),)
  ifneq (,$(findstring CYGWIN,$(OSTYPE)))
    ifeq ($(ARCH),win32)
      CC     = i686-w64-mingw32-gcc
      CXX    = i686-w64-mingw32-g++
      CCOPT  = -O3 -fno-inline
      LINKS  = -lm -lplatform -static
    else
      ifeq ($(ARCH),win64)
        CC     = x86_64-w64-mingw32-gcc
        CXX    = x86_64-w64-mingw32-g++
        CCOPT  = -O3 -fno-inline
        LINKS  = -lm -lplatform -static
      else
        ARCH   = cygwin
        CCOPT  = -O3 -fno-inline
        LINKS  = -lm -lplatform -static
      endif
    endif
  endif
  ifeq ($(OSTYPE), Linux)
    ARCH   = linux_64
    CCOPT  = -O3 
    LINKS  = -lm -lplatform -lstdc++ -static
  endif
endif

## options for GCC/CYGWIN/WINDOWS
ifeq ($(ENV), CYGWIN)
#ifneq (,$(findstring CYGWIN,$(OSTYPE)))
      ARCH   = cygwin
      CC     = gcc 
      CXX    = g++ 
      CCOPT  = -O3 -fno-inline
      LINKS  = -lm -lplatform 
endif

## options for MINGW32/CYGWIN/WINDOWS
ifeq ($(ENV), MINGW)
      ARCH   = win32
      CC     = i686-w64-mingw32-gcc
      CXX    = i686-w64-mingw32-g++
      CCOPT  = -O3 -fno-inline
      LINKS  = -static -lm -lplatform 
endif

## options for MINGW64/CYGWIN/WINDOWS
ifeq ($(ENV), MINGW64)
      ARCH   = win64
      CC     = x86_64-w64-mingw32-gcc
      CXX    = x86_64-w64-mingw32-g++
      CCOPT  = -O3 -fno-inline
      LINKS  = -static -lm -lplatform 
endif

## options for CLANG/MAC
ifeq ($(ENV), CLANG)
     ARCH    = macosx
     CC	     = clang
     CXX     = clang++
     LINKS   = -L/usr/local/lib -lm -lplatform -stdlib=libc++
     CCOPT   = -I/usr/local/include -g -fcolor-diagnostics -stdlib=libc++ 
endif

## options for GCC/MAC
ifeq ($(ENV), GCC_MAC)
     ARCH    = macosx
     CC	     = gcc-5
     CXX     = g++-5
     CCOPT  = -I/usr/local/include -O3 -fno-inline
     LINKS  = -L/usr/local/lib -lm -lplatform_gcc-5 
endif

## options for GCC/LINUX
ifeq ($(ENV), GCC)
      ARCH   = linux_64
      CC     = gcc
      CXX    = g++
      CCOPT  = -O3 
      LINKS  = -lm -lplatform -lstdc++ -static
	ifeq ($(HDF5), ON)
		LINKS  += -L/opt/hdf5.1.8/lib
		CCOPT  += -I/opt/hdf5.1.8/include
	endif
endif

## options for ICC/LINUX
ifeq ($(ENV), ICC)
      ARCH   = linux_64
      CC     = icc 
      CXX    = icpc 
      CCOPT  = -O3 -xSSSE3 -axAVX,SSE4.2,SSE4.1,SSSE3,SSE3,SSE2 -w0
#      LINKS  = -lm -lplatform -lcxaguard -lstdc++
      LINKS  = -lm -lplatform -lstdc++ -static-intel
	ifeq ($(HDF5), ON)
		LINKS  += -L/opt/hdf5.1.8/lib
		CCOPT  += -I/opt/hdf5.1.8/include
	endif
endif

## options for GCC+MKL+OMP/LINUX
ifeq ($(ENV), ICC_MKL_OMP)
      ARCH   = linux_64
      CC     = icc 
      CXX    = icpc 
      CCOPT  = -O3 -xSSSE3 -axAVX,SSE4.2,SSE4.1,SSSE3,SSE3,SSE2 -ip -qopenmp -parallel -w0
#      LINKS  = -lplatform -lcxaguard -lstdc++ -lmkl_intel_lp64 -lmkl_intel_thread  -lmkl_core -lm
      LINKS  = -lplatform -lstdc++ -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -lm -static-intel
	ifeq ($(HDF5), ON)
		LINKS  += -L/opt/hdf5.1.8/lib
		CCOPT  += -I/opt/hdf5.1.8/include
	endif
endif

OBJS  	= mt19937ar.o\
	operate_electrolyte.o\
	fluct.o\
	alloc.o\
	solute_rhs.o\
	fftsg.o\
	fftsg3d.o\
	avs_output.o\
	avs_output_p.o\
	output.o\
	resume.o\
	make_phi.o\
	fluid_solver.o\
	particle_solver.o\
	md_force.o\
	profile.o\
	interaction.o\
	operate_omega.o\
	fft_wrapper.o\
	f_particle.o\
	init_fluid.o\
	init_particle.o\
	input.o\
	rigid_body.o\
	operate_surface.o\
	matrix_diagonal.o\
	periodic_spline.o\
	sp_3d_ns.o

## options for HDF5 support
ifeq ($(HDF5), ON)
      LINKS  += -lhdf5 -lhdf5_hl
      CCOPT  += -DWITH_EXTOUT
      OBJS   += output_writer.o
endif

CFLAGS 	= $(CCOPT) -I$(GOURMET_INCLUDE_PATH)
LINKS  += -L$(GOURMET_LIB_PATH) 

XYZ_OBJS= alloc.o\
	rigid_body.o\
	$(AUX)/udf2xyz.o

TARGET 	= kapsel
XYZ	= udf2xyz

ENGINE = $(TARGET)
CONVERTER =  $(XYZ)
ifeq ($(ARCH), cygwin)
  ENGINE = $(TARGET).exe
  CONVERTER = $(XYZ).exe
endif
ifeq ($(ARCH), win32)
  ENGINE = $(TARGET).exe
  CONVERTER = $(XYZ).exe
endif
ifeq ($(ARCH), win64)
  ENGINE = $(TARGET).exe
  CONVERTER = $(XYZ).exe
endif

## Implicit rules

.SUFFIXES: .c .cxx .o .out

## Build rules

all: $(TARGET) $(XYZ)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(CFLAGS) $(LINKS)

$(XYZ): $(XYZ_OBJS)
	$(CXX) $(XYZ_OBJS) -o $(XYZ) $(CFLAGS) $(LINKS)

## Compile

.cxx.o: 
	$(CXX) -c $< $(CFLAGS) -o $@

.c.o: 
	$(CC) -c $< $(CFLAGS) -o $@

## Clean

clean:
	rm -f $(OBJS) $(AUX)/$(XYZ_OBJS) $(TARGET) $(XYZ)
	rm -f *~ *.bak

## Install

install:
	if [ x$(ENGINE_HOME_PATH) = x ]; then \
		echo " Environment variable PF_ENIGNE must be defined to install engine."; \
		exit 1;\
	fi
	if [ ! -d $(TARGET_DIR) ]; then \
		mkdir -p $(TARGET_DIR) ; \
	fi;\
	if [ -f $(ENGINE) ]; then \
		cp -f $(ENGINE) $(TARGET_DIR)/ ;\
	fi
	if [ -f $(CONVERTER) ]; then \
		cp -f $(CONVERTER) $(TARGET_DIR)/ ;\
	fi

depend:
	makedepend -- $(CFLAGS) -- *.cxx *.c *.h
