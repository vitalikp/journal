#
# Copyright © 2018 - Vitaliy Perevertun
#
# This file is part of journal.
#
# This file is licensed under the MIT license.
# See the file LICENSE.
#

prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@libdir@
includedir=@includedir@

Name: journal-@VER@
Description: Journal Library
Version: @VER@
Libs: -L${libdir}
Cflags: -I${includedir}
