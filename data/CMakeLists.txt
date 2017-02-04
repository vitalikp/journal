# journal data cmake file

# etc
install(FILES journald.conf DESTINATION ${JOURNAL_SYSCONFDIR})

# completions
# bash
if (BASH_COMPL_FOUND)
	install(FILES completions/bash/journalctl DESTINATION ${datadir}/bash-completion/completions)
endif(BASH_COMPL_FOUND)
# zsh
if (ZSH)
	install(FILES completions/zsh/_journalctl DESTINATION ${datadir}/zsh/site-functions)
endif(ZSH)

# systemd
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/data/sysinit.target.wants/)
execute_process(COMMAND ${LN} -fs ../journal-flush.service ${PROJECT_BINARY_DIR}/data/sysinit.target.wants/)
execute_process(COMMAND ${LN} -fs ../journald.service ${PROJECT_BINARY_DIR}/data/sysinit.target.wants/)
configure_file(systemd/journal-flush.service.cmake journal-flush.service)
configure_file(systemd/journald.service.cmake journald.service)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/data/initrd-switch-root.target.wants/)
execute_process(COMMAND ${LN} -fs ../journald.service ${PROJECT_BINARY_DIR}/data/initrd-switch-root.target.wants/)

install(FILES ${PROJECT_BINARY_DIR}/data/journal-flush.service DESTINATION lib/systemd/system)
install(FILES ${PROJECT_BINARY_DIR}/data/journald.service DESTINATION lib/systemd/system)
install(DIRECTORY ${PROJECT_BINARY_DIR}/data/initrd-switch-root.target.wants DESTINATION lib/systemd/system)
install(DIRECTORY ${PROJECT_BINARY_DIR}/data/sysinit.target.wants DESTINATION lib/systemd/system)

# tmpfiles
install(FILES tmpfiles/journal.conf DESTINATION lib/tmpfiles.d)

# log directory
install(DIRECTORY DESTINATION ${logdir}/journal)