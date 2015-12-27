#  This file is part of systemd.
#
#  systemd is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation; either version 2.1 of the License, or
#  (at your option) any later version.

[Unit]
Description=Journal Service
Documentation=man:journald(8) man:journald.conf(5)
DefaultDependencies=no
Before=sysinit.target

[Service]
ExecStart=@sbindir@/journald
StandardOutput=null
CapabilityBoundingSet=CAP_SYS_ADMIN CAP_DAC_OVERRIDE CAP_SYS_PTRACE CAP_SYSLOG CAP_AUDIT_CONTROL CAP_CHOWN CAP_DAC_READ_SEARCH CAP_FOWNER CAP_SETUID CAP_SETGID

# Increase the default a bit in order to allow many simultaneous
# services being run since we keep one fd open per service.
LimitNOFILE=16384
