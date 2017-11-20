PROJECT TAPIRCHIP 
================================================================

[![TAPIRCHIP Presentation Video](https://macondomomments.files.wordpress.com/2014/09/dsc00289.jpg)](https://www.youtube.com/watch?v=K2XoNLCk-Mk)
[![TAPIRCHIP Release](docs/Logos/Tapir.bmp)](https://www.youtube.com/watch?v=K2XoNLCk-Mk)

INTRODUCTION
===============================================================
The Amazon rainforest is considered to be one of the last natural reserves for animals, plants and trees; untouched by humans. It is also considered to be a frontline in the fight against global warning. 
Illegal mining and uncontrolled deforestation have become a menace to this environment. For this reason, protection and conservation policies must be created by the government. 
In this context biologists are doing their best effort to put into value the amazon rainforest; to show that preserving the amazon can be sustainable economic activity.
The method employed by the biologists consists of doing an animal inventory; done mainly through walking and observation. 
This method is not reliable because animals rarely show their self’s in the presence of humans, besides most of the animals have nightlife activity. 
This makes it difficult to gather information of all the current species of the amazon.
Trap cameras have become a popular method to study wildlife. These devices are attached to a tree and use a motion sensor that is
triggered by the animal's movement to take a picture. These devices have the great advantage that they can work day and night for many
days depending on the energy consumption. They can be installed in remote areas deep in the jungle were humans barely reach.
The information gathered by the traps cameras can help biologist to discover new species, validate the presence of known
species and also study animal bahaviour.

If a transmission system is implemented with the trap camera, the biologists could reduce their walking to the jungle and also monitor the device status. 
In commercial trap cameras the transmission is available through mobile services, but in the jungle the lack of a communication system makes it difficult to use this device. 
Therefore our purpose is to develop a trap camera that meets the biologist’s requirements and that has a communication system to transfer data to a central base.

SYSTEM SPECS
===============================================================
This camera trap have the following characteristics:
-	CMOS sensor with a resolution of 5 Megapixeles.
-	2.4 GHz Wi-Fi Radio to have a better throughput.
-	Temperature, humidity and MP3 recorder, to have information about the environment.
-	Li-on Battery for 3 months operation.

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