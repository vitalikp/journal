# journal data cmake file

# etc
install(FILES journald.conf DESTINATION /etc)

# completions
# bash
if (BASH_COMPL_FOUND)
	install(FILES completions/bash/journalctl DESTINATION ${datadir}/bash-completion/completions)
endif(BASH_COMPL_FOUND)
# zsh
if (ZSH)
	install(FILES completions/zsh/_journalctl DESTINATION ${datadir}/zsh/site-functions)
endif(ZSH)

# sysctl
if (${COREDUMP_ENABLE})
configure_file(sysctl/50-coredump.conf.cmake 50-coredump.conf @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/data/50-coredump.conf DESTINATION lib/sysctl.d)
endif()

# systemd
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/data/sockets.target.wants/)
execute_process(COMMAND ${LN} -fs ../journald.socket ${PROJECT_BINARY_DIR}/data/sockets.target.wants/)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/data/sysinit.target.wants/)
execute_process(COMMAND ${LN} -fs ../journal-flush.service ${PROJECT_BINARY_DIR}/data/sysinit.target.wants/)
execute_process(COMMAND ${LN} -fs ../journald.service ${PROJECT_BINARY_DIR}/data/sysinit.target.wants/)
configure_file(systemd/journal-flush.service.cmake journal-flush.service)
configure_file(systemd/journald.service.cmake journald.service)

install(FILES systemd/syslog.socket DESTINATION lib/systemd/system)
install(FILES systemd/journald.socket DESTINATION lib/systemd/system)
install(DIRECTORY ${PROJECT_BINARY_DIR}/data/sockets.target.wants DESTINATION lib/systemd/system)
install(FILES ${PROJECT_BINARY_DIR}/data/journal-flush.service DESTINATION lib/systemd/system)
install(FILES ${PROJECT_BINARY_DIR}/data/journald.service DESTINATION lib/systemd/system)
install(DIRECTORY ${PROJECT_BINARY_DIR}/data/sysinit.target.wants DESTINATION lib/systemd/system)