cmake_minimum_required (VERSION 2.4)
add_executable (codegen codegen.cpp)

# 
# LDForge uses alternative operators. GCC and Clang use these by default but MSVC does not.
# So we'll have to tell MSVC to use these alternative operators
# 
if (MSVC)
	set_target_properties (codegen PROPERTIES COMPILE_FLAGS "/Za")
endif()
