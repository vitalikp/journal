# journal
fork journal from [systemd 214](https://github.com/vitalikp/systemd)

# Install
 - download [journal](https://github.com/vitalikp/journal/archive/master.tar.gz);
 - unpack archive and go into directory;
 - create **build** directory;
 - run commands:
```
	# cd build
	# cmake ..
	# make
	# sudo make install
```
Use `cmake -D(OPTION) ..` instead of `cmake ..` for add config option.
##### Optional Features:
 - Use **TESTS_ENABLE=ON** option to enable build tests(default=OFF);
 - Use **XZ_ENABLE=OFF** option to disable xz support(default=ON);
 - Use **LZ4_ENABLE=OFF** option to enable lz4 support(default=OFF);

# License
Original forked code is LGPLv2.1+.<br/>
New code is MIT License. See LICENSE file.
