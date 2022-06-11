
Build and install
-----------------
To build avarice: in this directory
~~~~
> cmake -Bbuild .
> cd build
> cmake --build .
~~~~

~~~~
> cat /etc/udev/rules.d/99-jtag.rules
SUBSYSTEMS=="usb", ATTRS{idVendor}=="03eb", ATTRS{idProduct}=="2103", GROUP="users", MODE="0666", SYMLINK+="avrjtagmkii"
~~~~
