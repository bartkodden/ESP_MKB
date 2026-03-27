set(SOURCES bq27441.c)
idf_component_register(SRCS ${SOURCES}
	INCLUDE_DIRS "."
	REQUIRES driver espressif__arduino-esp32)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -Wno-type-limits -fdiagnostics-color=always)
