set(requires freertos)

set(src_dirs "ports/freertos"
             "src/qf"
             "include")

set(include_dirs "include"
                 "ports/freertos")

set(priv_include_dirs "src")

idf_component_register(SRC_DIRS ${src_dirs}
                    INCLUDE_DIRS ${include_dirs}
                    PRIV_INCLUDE_DIRS ${priv_include_dirs}
                    REQUIRES ${requires})

target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DQPC_ESP32_PORT")

if(CONFIG_QPC_QSPY_ENABLE)
    list(APPEND src_dirs "src/qs")
    target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DQ_SPY")
endif()
