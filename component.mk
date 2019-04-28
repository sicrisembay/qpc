#
# Component Makefile
#

COMPONENT_DEPENDS := freertos

COMPONENT_ADD_INCLUDEDIRS += include
COMPONENT_ADD_INCLUDEDIRS += ports/freertos
COMPONENT_PRIV_INCLUDEDIRS += src

COMPONENT_SRCDIRS += ports/freertos
COMPONENT_SRCDIRS += src/qf
COMPONENT_SRCDIRS += src/qs
COMPONENT_SRCDIRS += include

CFLAGS += -DQ_SPY