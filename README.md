# Introduction
An emulator of the 1978 arcade game, Space Invaders. 👾

<table>
<tr>
  <td><img src="https://media.giphy.com/media/MBafbEVK5DlYHTFFFQ/giphy.gif" width=256px></td>
  <td><img src="https://media.giphy.com/media/PlrFY2xjuT9QkXwhWs/giphy.gif" width=256px></td>
  <td><img src="https://media.giphy.com/media/RNF5pzoz0WrWkVlPaR/giphy.gif" width=256px></td>
</tr>
</table>

# Dependencies
The emulator requires the SDL2 library to be installed on your machine. 

# Build
To launch the emulator, execute the below command from the `src` directory:

`make && ./invaders`

The emulator requires the game ROM `invaders.h`, `invaders.g`, `invaders.f`, `invaders.e` to be present in the `res/roms` directory.

# Controls
<table>
<tr><th>Key</th></th><th>Space Invaders</th></tr>
<tr><td><kbd>C</kbd></td><td>Insert coin</td></tr>
<tr><td><kbd>1</kbd></td><td>Start one player game</td></tr>
<tr><td><kbd>2</kbd></td><td>Start two player game</td></tr>
<tr><td><kbd>&larr;</kbd></td><td>Move ship left</td></tr>
<tr><td><kbd> &rarr;</kbd></td><td>Move ship right</td></tr>
<tr><td><kbd>Space</kbd></td><td>Shoot</td></tr>
</table>

# References
- [Intel 8080 Assembly Language Programming Manual](http://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf)
- [Space Invaders - Computer Archeology](http://computerarcheology.com/Arcade/SpaceInvaders/)
- [Space Invaders Arcade Game Info](http://www.brentradio.com/SpaceInvaders.htm)
