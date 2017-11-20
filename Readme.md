PROJECT TAPIRCHIP 
================================================================

[![TAPIRCHIP Presentation Video](https://macondomomments.files.wordpress.com/2014/09/dsc00289.jpg)](https://www.youtube.com/watch?v=K2XoNLCk-Mk)
[![TAPIRCHIP Release](docs/Logos/Tapir.bmp)](https://www.youtube.com/watch?v=K2XoNLCk-Mk)

DESCRIPTION
================================================================
This is an open source project to implement a wireless camera trap that consists of 3 modules: Camera, Audio and WiFi Radio.
This project is to be implemented in the rainforests of PERU, where the Wireless Trap Cameras will have to transmit images from nodes to a sink node over distances of 800 meters
The 3 modules can work independently or in combination accroding to the user needs.
The Camera module consists of a 5MP CMOS sensor from Omnivision which is controlled by a DSPIC processor of the DSPIC33EP family.
For this modules the chosen processor is the DSPIC33EP256GP504.
The Audio module uses the VS1063a audio encoder from VLSI Solution to generated mp3 audio files from the environment, this device is also controlled by a DSPIC processor.
The Radio module core is the ESP8266 SoC chip, this device has a WiFi radio with its RF components and a processor incorporated in one die. This component reduces greatly the pcb size and the support available online makes it easier to implement transmission protocols.
The project has the following  files:
1. CameraTrapCode_OV5642_dsPIC33EP
In this folder we can find all the code to control the OV5642 sensor, as well as its behaviour as a "camera trap".
2. ImageTransmitProtocol_ESP8266
In this folder we can find the transmission protocol to transmit the images taken by the camera module.
To allow the WiFi signal to be transmitted over the trees in the jungle, the antenas will be located on the top of the trees.
For this purpose the nodes will work in 2 configurations. The first folder named "Nodes" will transmitt the pictures from the trap camera located in the ground to the top of the tree.
The second folder named "Network" will transmit images from 4 nodes to a sink node, located on the top of the trees.
3. Mp3Recorder_VS1063
In this folder we can find all the code to record audio in mp3 format using a VS1063 encoder/decoder with a dsPIC33EP processor.