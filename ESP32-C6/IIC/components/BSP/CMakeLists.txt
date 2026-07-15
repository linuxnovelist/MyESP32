# components/BSP/CMakeLists.txt
idf_component_register(
    SRC_DIRS "LED"
    SRC_DIRS "KEY"
    SRC_DIRS "OLED_SPI(M242)"
    SRC_DIRS "IIC"
    INCLUDE_DIRS "LED"
    INCLUDE_DIRS "KEY"
    INCLUDE_DIRS "OLED_SPI(M242)"
    INCLUDE_DIRS "IIC"
    REQUIRES driver
)