### Cellular basestation using FreeRTOS
#### chip: echobase_sam4e16c

Research project to determine feasibility and accuracy of battery powered ultrawideband based asset tag. Proof of concept delivered approximately 20cm accuracy in an indoor environment. Approximately 25-30 cm accuracy outdoors depending on anchor denisty and obstacles.

```console
#JLink commands
#Device <devicename> ie: atsam3x8e
#loadfile <filetoload.hex>

# The following commands will load a new binary file to the micro using J-Link
# Run JLinkExe from command line
# Set the device name
# Reset command
# Halt command
# Load Binary command
# Reset command
# Go command 
# Quit
# Press reset pin on micro

device = ATSAM3X8E
r
h
loadbin <filename.bin>, 0x00080000
r
g
qc
# Hit reset on board to run the new code
```
