# ZMK Sensor Driver Attribute Cycler

This behaviour cycles through a list of values and sends the selected value to a sensor driver attribute. I made it to cycle through DPI values on the Ploopy mouse, but it may be able to do more than that.

## Features

- Can persist values to storage and reload them at startup.
- Can delay loading at start, in case drivers are not ready.
- Can cycles forwards or backwards.

## Configuration

See and example in my [Ploopy mouse keymap](https://github.com/george-norton/zmk-keyboard-ploopy/blob/5f04c84d964a73106ec5563be07f08e38472c015/boards/arm/ploopy_mouse/ploopy_mouse.keymap#L52).

The `attr` value is driver specific, sensor drivers provide a `set_attr` function ([like this](https://github.com/george-norton/zmk-driver-pmw3360/blob/0dfa2f0411b795613a35f222f993e45a1e4cc32a/drivers/input/input_pmw3360.c#L365)), this allows runtime modification of certain attributes. Only things exposed through this API are modifiable by this behavior.

Persisting to flash requires that you have configured your board to work with the settings API. You will need a storage flash partition and a bunch of config options enabled. If your board supports ZMK studio, you probably have all this setup already.

## TODO

- Configurable channels?
- Cycle val2 as well?
