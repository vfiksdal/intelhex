# intelhex
Simple C implementation of a intelhex-parser. The parsed hex file will populate a simple struct with memory segments and addresses, which can easily be iterated or processed by a bootloader client application. Please see ihinfo.c for a very simple example.

# Example
You can easily compile the simple example using gcc:
```
gcc ihex.c ihinfo.c -o ihinfo
```
And run it to parse a hex file of your own:
```
./ihinfo myfile.hex
```

