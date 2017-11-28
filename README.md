# Chorder input device. 
# Credit to [Greg Priest-Dorman](chorder.cs.vassar.edu/) for the original code

## Features:

* 7-key chord support
* Simple button debouncing
* Only activates chords upon the release of a button - so you don't have to press them all at the same time
* Prefixs can change chords, and three thumb buttons allow for many more chords
* Completely modifiable keyset using a simple config file
* Runs on a [Feather M0 Bluefruit LE](https://www.adafruit.com/products/2995) 
* Uses BLE HID input, so is compatible with almost all mobile devices
* Allows you to configure your own button types by changing a debounce delay

## And the real reason I made this:

Meterpreter-based exploitation!

Yes, you read that right, this code also supports Meterpreter exploitation of an attached computer. The default keybind to do so is \[FC- I---\] while under function mode (which can be accessed via \[--N ---P\] while under the default mode). 

## Use:

The format for showing chords is \[FCN IMRP\] where:
* F is _F_ar Thumb
* C is _C_enter Thumb (home position)
* N is _N_ear Thumb
and IMRP are _I_ndex, _M_iddle, _R_ing, and _P_inky fingers respectively.

See [this](https://learn.adafruit.com/adafruit-feather-m0-bluefruit-le/pinouts?view=all#setup) guide in order to configure your Arduino IDE 

In order to properly use the Meterpreter hack, you have to enter the IP and port that your meterpreter listening shell is on. See the FeatherChorder.ino file in order to do this.

I reccomend using Cherry MX Blue switches from SparkFun, along with the breakout board and an LED. Keycaps are usually pretty easy to find, but my favorites are [these](http://www.maxkeyboard.com/row-4-size-1x1-cherry-mx-keycap-r4-1x1.html) in a translucent color.

Training materials for use of the chording keyboard can be found at Greg's [chording keyboard site](chorder.cs.vassar.edu/)
