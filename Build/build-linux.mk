# tools
QMAKE                         := /opt/QtSDK/Desktop/Qt/473/gcc/bin/qmake
MAKE                          := make
RM                            := rm
MKDIR                         := mkdir
CDDIR                         := cd
CP                            := cp
MV                            := mv
CHMOD                         := chmod
PWD                           := pwd
QtFlashTool.CCFLAGS           := -r -spec linux-g++-64 CONFIG+=$(BUILD_TYPE)

# path
QtFlashTool.SRC.Path          := $(PWD)

# Dependent files
QtFlashTool.Dependency.Files := \
 $(wildcard lib/*.xml) \
 $(wildcard lib/*.xsd) \
 $(wildcard lib/*.ini) \
 $(wildcard lib/*.so)  \
 $(wildcard lib/*.bin) \
 $(wildcard lib/*.sh) \
 $(wildcard lib/*.qhc) \
 $(wildcard lib/*.qch) \
 $(wildcard lib/Release.json) \
 $(wildcard lib/99-ttyacms.rules) \

QtFlashTool.Dependency.Dirs := \
 $(wildcard lib/qtlinux/*)

QtFlashTool.Output.Files := *.xml *.xsd *.ini *.so *.bin *.sh *.qhc *.qch *.txt \
	flash_tool 99-ttyacms.rules Release.json \
	bin lib plugins \
	codecs imageformats sqldrivers

#Step1: set up environment
.PHONY: set-up
set-up:
	$(eval QtFlashTool.SRC.Path := $(shell $(PWD)))
	$(MKDIR) -p $(OUTPUT_PATH)
	
#Step2: qmake *.pro + make	
all: set-up 
	$(CDDIR) $(OUTPUT_PATH);$(PWD);$(QMAKE) $(QtFlashTool.SRC.Path) $(QtFlashTool.CCFLAGS);$(MAKE);
	$(CP) $(QtFlashTool.Dependency.Files) $(OUTPUT_PATH)
	$(CP) -R $(QtFlashTool.Dependency.Dirs) $(OUTPUT_PATH)
	$(eval QtPlugins := $(OUTPUT_PATH)/codecs  $(OUTPUT_PATH)/imageformats  $(OUTPUT_PATH)/sqldrivers)
	$(RM) -rf $(QtPlugins)
	$(CP) -R $(OUTPUT_PATH)/plugins/* $(OUTPUT_PATH)
	@echo $(BUILD_TYPE) version build pass
	
.PHONY: clean
clean: 
	$(CDDIR) $(OUTPUT_PATH);$(PWD);$(MAKE) clean
	$(CDDIR) $(OUTPUT_PATH);$(PWD);$(RM) -rf $(QtFlashTool.Output.Files)


