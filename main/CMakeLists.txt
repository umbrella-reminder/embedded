# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
# /main/CMakeLists.txt  
idf_component_register(SRCS 
                                    "main.c"    #add sorce file ex) "Src/LED_Moudle.c"
                                            
                       INCLUDE_DIRS 
                                    "."
                                    "Inc")