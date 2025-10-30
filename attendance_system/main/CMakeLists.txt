idf_component_register(
    SRCS "attendance_system.c"
    INCLUDE_DIRS "."
    REQUIRES i2c GPIO_CONF HTTP WIFI
)