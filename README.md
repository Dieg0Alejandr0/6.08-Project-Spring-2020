# Description

The following is a handheld motion-controlled iteration of ballbreaker, requiring the use of an IMU to control the motion of the cursor, capable of multiplayer gameplay.
The user of a Arduino device starts a game session, in which another player can try to sabotage the game session by logging into a specific site to place extra blocks
in the original player's session live. See the Game.ino file for the majority of my contributions to this project. Thanks to my teammates Isaak Hernandez, Peter Amenowolde,
and Peter Lantigua, this project would not have been possible without you.


## Libraries/Packages Required (Game.ino file)
C/C++: 
[Arduino TFT_eSPI](https://www.arduino.cc/reference/en/libraries/tft_espi/),
mpu6050_esp32
