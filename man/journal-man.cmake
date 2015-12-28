# journal man cmake file

# xsltproc flags
set(XSLTPROC_OPT --nonet --xinclude)
set(XSLTPROC_OPT ${XSLTPROC_OPT} --stringparam man.output.quietly 1)
set(XSLTPROC_OPT ${XSLTPROC_OPT} --stringparam funcsynopsis.style ansi)
set(XSLTPROC_OPT ${XSLTPROC_OPT} --stringparam journal.version ${VERSION})
set(XSLTPROC_OPT ${XSLTPROC_OPT} --path '${PROJECT_BINARY_DIR}/man:${CMAKE_SOURCE_DIR}/man')


macro(add_man var section name)
    install(FILES ${PROJECT_BINARY_DIR}/${name}.${section} DESTINATION share/man/man${section})
    set(aliases "")
    if (${ARGC} GREATER 3)
    	foreach(argval ${ARGN})
    		list(APPEND aliases "${argval}.${section}")
    		install(FILES ${PROJECT_BINARY_DIR}/${argval}.${section} DESTINATION share/man/man${section})
    	endforeach()
    endif()
	add_custom_command(
		OUTPUT "${name}.${section}" ${aliases}
		COMMAND ${XSLTPROC}
		ARGS -o ${name}.${section} ${XSLTPROC_OPT} ${CMAKE_SOURCE_DIR}/man/custom-man.xsl ${CMAKE_SOURCE_DIR}/man/${name}.xml
		COMMENT "  XSLT\t${name}.${section}" VERBATIM)
	list(APPEND ${var} "${name}.${section}")
	list(APPEND ${var} ${aliases})
endmacro(add_man)
