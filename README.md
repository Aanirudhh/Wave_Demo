# Wave Demo

Demonstarting plasma/ wave flow and even how waves interact with each other on superposition , 

## Preview

Five switchable plasma modes, all animated in real time:

| Key | Mode |
|-----|------|
| `1` | Classic sine waves |
| `2` | Ripple rings from a moving center |
| `3` | Interference — two moving wave sources |
| `4` | Tunnel / zoom |
| `5` | Twisted grid |

`ESC` — quit

## Build

Requires MinGW (GCC for Windows).

```bash
g++ plasma.cpp -o plasma.exe -mwindows
```

> `-mwindows` suppresses the console window.

## How It Works

Uses a classical wave equation i.e -

```bash
sin(kx + wt)
```

Here the value of x and y are the coordinates and t is the value captured through a per-delta counter 
by a CPU instruction `QueryPerformanceCounter` .

We calculate the value of each pixel through superposition of different waves and get its final interferance value i.e `v`

For better underdstanding , this is the way :

```bash
wave1 alone:   ||||||||||||    (vertical stripes)
wave2 alone:   ════════════    (horizontal stripes)
wave3 alone:   ////////////    (diagonal stripes)
wave4 alone:   ))))))))))))    (concentric rings)

all four added: the plasma mess you see
```

this `v` value if clamped to a [1 to 255] and gets its pixel colour and this is thrown on the monitor using `StretchDIBits`

this is done for all the pixels in the 2-nested for loop  and the pixel's colour gets assigned for each passing time `t`
simulating the animation 

The way we superposition it is the reason behind every different plasma modes .


## Modification

You can change it to whatever colours you want by changing the values of `r` `g` `b` values in build_palette()  
by still keeping the  `r` `g` `b` values a f(t) still .

## Requirements

- Windows
- MinGW / g++
