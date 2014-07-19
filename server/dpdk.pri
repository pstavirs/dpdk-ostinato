#
# Qt qmake integration with DPDK 
#
# The following DPDK environment variables need to be defined during qmake 
# - RTE_SDK (mandatory)
# - RTE_TARGET (optional - default is i686-default-linuxapp-gcc)
# NOTE: If the variables are changed after running qmake, you need to run 
# qmake again
# 

RTE_SDK = $$(RTE_SDK)
isEmpty(RTE_SDK) {
    error("RTE_SDK environment variable is not set")
}

RTE_TARGET = $$(RTE_TARGET)
isEmpty(RTE_TARGET) {
    RTE_TARGET="i686-default-linuxapp-gcc"
}

INCLUDEPATH += "$${RTE_SDK}/$${RTE_TARGET}/include"

# CPPFLAGS reqd by DPDK
# TODO: derive these from RTE_SDK/RTE_TARGET
QMAKE_CXXFLAGS += -m32 \
                  -pthread \
                  -march=native
QMAKE_CXXFLAGS += -D__STDC_LIMIT_MACROS 
QMAKE_CXXFLAGS += -DRTE_MACHINE_CPUFLAG_SSE \
                  -DRTE_MACHINE_CPUFLAG_SSE2 \
                  -DRTE_MACHINE_CPUFLAG_SSE3 \
                  -DRTE_MACHINE_CPUFLAG_SSSE3 \
                  -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3
QMAKE_CXXFLAGS += -include $${RTE_SDK}/$${RTE_TARGET}/include/rte_config.h

# lib location
LIBS += -L"$${RTE_SDK}/$${RTE_TARGET}/lib"

# PMD libs to link
LIBS += -lrte_pmd_e1000 \
        -lrte_pmd_ixgbe \
        -lrte_pmd_virtio_uio \
        -lrte_pmd_vmxnet3_uio

# SDK libs to link
LIBS += -lrte_malloc \
        -lethdev \
        -lrte_eal \
        -lrte_mempool \
        -lrte_ring
