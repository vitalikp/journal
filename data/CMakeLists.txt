# journal data cmake file

# etc
install(FILES journald.conf DESTINATION /etc)

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