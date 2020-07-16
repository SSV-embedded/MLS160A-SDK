# Secure Device Update

Many typical control solutions in automation often lack possibilities for software updates of individual components such as sensors. In many cases, patches do not even exist, although some controllers have long known vulnerabilities.

Secure Device Updates (SDU) solve these problems and also offer the possibility to distribute new functions. If a component, machine or system is to be supplied with a software or configuration update via an IoT connection, the IT security must be taken into account.

With the current state of the art, this requires a public key infrastructure (PKI) for digital signatures with private and public keys, certificates, blacklists, etc., in order to at least guarantee the authenticity and integrity of the update. All the necessary components are contained in SDU.

![mls160a_riot_sdk_schema_en](/images/mls160a_riot_sdk_schema_en.png)

The following example describes how a new firmware for the soft sensor MLS/160A is created, signed and then uploaded via an IP network.

> **PLEASE NOTE:<br/>
> Our example describes the creation of a new firmware and the usage of SDU on a Windows 10 system.**

## Overview
The soft sensor MLS/160A is connected via an RS485 connection with the gateway RMG/941. The sensor data is sent in a special data streaming mode to the gateway where it can be used e.g. for machine learning-based condition monitoring like in this **[example](https://github.com/SSV-embedded/RMG-941-and-AWS)**.

The MLS/160A runs with the embedded operating system RIOT OS and offers an A/B boot concept, allowing the remote update of the sensor’s firmware. Therefore a special SDU device driver is pre-installed on the MLS/160A.

The developer creates a new RIOT firmware file on his developer workstation and signs it with his private key with a special SDU signing tool. The signed file is then stored in the RMG/941 where a special SDU app checks with a public key and a blacklist if the new firmware file is genuine. Once everything is verified the new firmware is uploaded to the MLS/160A.

This solution enables the update of a whole fleet of sensors or other IoT devices over an IP network like the internet.

## Hardware Components

### RMG/941
The Remote Maintenance Gateway RMG/941 from **[SSV Software Systems](https://www.ssv-embedded.de)** runs with Embedded Linux and offers a lot of features like VPN, Python 3, Jupyter Notebook and Node-RED. Even the TensorFlow Lite runtime can be installed.

The RMG/941 has a pre-installed SDU app which carries out the firmware update of the MLS/160A.

![rmg941](/images/rmg941.png)

For more information visit the **[RMG/941 product description](https://www.ssv-embedded.de/produkte/rmg941)**.

### MLS/160A
The sensor MLS/160A has the Bosch BMI160 at its heart with accelerometer and gyroscope. It is able to stream measurement values with a maximum frequency of 1.6 kHz. The MLS/160A is connected with the RMG/941 via RS485 in half duplex mode.

The MLS/160A has an A/B boot concept with two slots of which only one is active. Every time the sensor is getting updated, the inactive slot is overwritten with the new firmware file. After the update the slot with the newest firmware becomes the active one and runs at power up. The firmware of the MLS/160A is build with RIOT OS.

![mls160a](/images/mls160a.png)

For more information visit the **[MLS/160A product description](https://www.ssv-embedded.de/produkte/mls160a)**.

### Developer Workstation

For using SDU, a **64-bit Windows 10 computer** with the following tools is needed:

* Docker Desktop
* MLS/160A SDK
* SDU Signing Tool
* X.509 Certificates
* Web browser
* Internet connection

**Working with older systems than Windows 10 is not recommended**, because they do not support the current Docker Desktop version and the Docker Toolbox (for older Windows versions) does not work appropriate with the MLS/160A SDK.

Of course it is also possible to develop the RIOT firmware on a Linux system as the RIOT OS is made for Linux, which also means, that Docker is not necessary. But since **the SDU signing tool is currently only available for Windows**, the signing of the firmware file must be done on a Windows system.

### ST-Link/V2 (for Factory Reset)
The ST-Link/V2 is an in-circuit/in-system debugger and programmer for STM32 microcontrollers like the one inside of the MLS/160A. Usually - and strongly advised - before a firmware update is being deployed on the sensors in the field, the firmware is tested locally on a "golden master" to find any possible errors or malfunctions. If the new firmware is buggy and the golden master does not work anymore, you can reset the MLS/160A manually with the help of the ST-Link/V2. 

The ST-Link/V2 debugger/programmer can be obtained from **[DigiKey](https://www.digikey.com/product-detail/en/ST-LINK%2FV2/497-10484-ND/2214535)**.

> **PLEASE NOTE:<br/>
> To connect the ST-Link/V2 to the MLS/160A a special adapter cable with a JTAG controller is required. This adapter cable can be obtained directly from SSV. Simply contact the sales team via sales@ssv-embedded.de.**

![ST-LINK_V2](/images/ST-LINK_V2.png)

## Hardware Setup

The hardware setup for this example is very simple. First connect the RMG/941 with the MLS/160A and the power supply. Then connect the RMG/941 to the **same network (LAN)** as the developer workstation.

The ex-factory IP address of the RMG/941 is `192.168.0.126`. So the workstation must have an IP address in the same range like `192.168.0.xxx`.

The figure below shows a demo setup in which a small water pump is used as a vibration source. The sensor data of the MLS/160A is sent to the RMG/941 where a machine learning algorithm classifies the current condition of the pump in real time.

The Raspberry Pi in the picture has a CODESYS runtime installed which also can be updated with SDU.

![hardware_setup](/images/hardware_setup.jpg)


## Software Components

### Docker Desktop

As mentioned before, the RIOT OS as well as the MLS/160A SDK are actual made for Linux. For working with the MLS/160A SDK on  a Windows system it is necessary to install Docker. Docker works with so-called containers, which are like lightweight virtual machines that enable to execute system specific applications platform independant.

#### Installation and Configuration

Here you can **[download Docker Desktop](https://www.docker.com/products/docker-desktop).**

![docker_1](/images/docker_1.png)

Make sure to download the Windows version (*.exe), then install Docker Desktop on your computer and start it. The start process may take a while. 

### MLS/160A SDK
The MLS/160A SDK is needed to create and compile a new firmware for the MLS/160A. The firmware is based on the RIOT OS, which is a small operating system for microcontrollers, especially aimed at IoT low power applications. For more information visit the **[RIOT website](https://www.riot-os.org/)**.

#### Installation and Configuration
Before installing the MLS/160A SDK a special Docker container for compiling RIOT OS applications has to be installed. Make sure that Docker is running, open a Windows command prompt (CMD) and enter:

```docker pull riot/riotbuild:latest```

This process may take a while as the files are downloaded from the Internet.

![docker_pull_riot_complete](/images/docker_pull_riot_complete.PNG)

Now the MLS/160A SDK can be installed.

**Installation via Git**

If you have **Git** installed on your developer workstation, the easiest way to install the MLS/160A SDK is to clone the repository using Git.

Therefore create a directory where you want to save the MLS/160A SDK, e.g. **/MLS**. Now open the context menu in that directory with a rightclick and click on **Git Bash Here** to open a Git bash window. Enter the following commands to install the SDK:

```
git config --global core.autocrlf false
git clone https://github.com/SSV-embedded/MLS160A-SDK
cd MLS160A-SDK/
git submodule init
git submodule update
```

After the installation you will see a folder structure like shown in the following picture:

![folder_1](/images/folder_1.PNG)

**Installation without Git**

Without Git you have to download the **[MLS/160A SDK repository](https://github.com/SSV-embedded/MLS160A-SDK)** directly from GitHub as a ZIP file. Therefore click on the green button **[Code]** and then click on **Download ZIP**.

![mls160a-sdk_github_download](/images/mls160a-sdk_github_download.PNG)

After the download just extract the ZIP file in the MLS directory.

> **PLEASE NOTE:<br/>
> The RIOT directory will be empty since it is just a link to the RIOT-OS fork with SSV-specific patches.**

So go back to GitHub and click on the RIOT folder. You will be redirected to the SSV-specific RIOT OS. Download RIOT by clicking on **[Code]** and on **Download ZIP** and then extract the ZIP file in the empty RIOT directory.

The content of the RIOT directory should now look like shown in the following figure:

![directory_riot](/images/directory_riot.PNG)


### SDU Signing Tool
The SDU Signing Tool was especially developed to sign important files like firmware updates before they are transmitted over the internet to a target device.

In our example here the SDU Signing Tool is used to sign the firmware files for the MLS/160A. The signed files are then sent to the RMG/941 where they are checked by the SDU app. If the files were corrupted or modified in any way, the SDU app rejects the files. If everything is fine, the files are accepted and can be used to update the MLS/160A. 


#### Installation and Configuration

You can **[download the SDU Signing Tool](https://github.com/SSV-embedded/SDU)** directly from Github. Click on the green button **[Code]** and then click on **Download ZIP**.

![github_sdu](/images/github_sdu.png)

Extract the ZIP file after the download and execute the file **SDU Signing Tool Setup 2.0.2.exe** to start the installation.

![sdu_exe_1](/images/sdu_exe_1.PNG)

Probably a blue dialog box will appear during the installation, saying that Windows blocks the execution of an unknown app (build.bat).

![windows_warnung](/images/windows_warnung.png)

In this case, just click on **More information** (or **Weitere informationen**) and then choose the option to execute the app anyway. After the installation the SDU Signing Tool opens automatically.

> **IMPORTANT!<br/>
> The ZIP file of the SDU Signing Tool also contains a certificate file called ssvdemo-user.pfx. This certificate is needed to sign files with the SDU Signing Tool.**

### Web browser

The web browser is needed to access the user interface (SSV/WebUI) of the RMG/941.

We recommend to use the latest version of **Mozilla Firefox** or **Google Chrome**. Other browsers like Microsoft Edge might not work properly.


### SDU App

The SDU app runs on the RMG/941 and is used to browse for new updates on your computer and to download them into the gateway. The update files must have been signed with the **SDU Signing Tool** (see above). If the files were corrupted or modified in any way, the SDU app rejects the files. If everything is fine, the files are accepted and can be used to update the target device. 

The SDU app offers different so-called SDU agents for different target devices.


#### Installation and Configuration

If the SDU app is not pre-installed on your RMG/941, click in the menu on **System > Apps management**. There you can see which apps are already installed and which apps are available.

You can install the SDU app directly on the RMG/941 or **[download the SDU app](https://www.ssv-embedded.de/downloads/igw941/)** on your computer and then install it manually.

### OpenOCD (for Factory Reset)

OpenOCD is used when you need to flash the MLS/160A by hand with the help of the command line and the ST-Link/V2 debugger.

Usually this is only necessary for your "golden master", since before an update is released for the field sensors, the new firmware should **always** be tested with the golden master. So if the new firmware is buggy and the MLS/160A does not work anymore, then you can always flash it by hand. If you skip this important test, you might cause the failure of all field sensors!

![openocd_1](/images/openocd_1.png)

#### Installation and Configuration

Here you can **[download OpenOCD](https://gnutoolchains.com/arm-eabi/openocd/).** Please make sure to download the latest version of OpenOCD from the top of the list.

> **PLEASE NOTE: <br/>
> Since the OpenOCD files are packed with **7-Zip**, it might be necessary to install 7-Zip in order to extract the files. Here you can [download 7-Zip](https://7-zip.org/download.html).**

You can store the OpenOCD files anywhere on the computer, but to be able  to execute OpenOCD from any location within Windows, we recommend to **add the OpenOCD location to the global path of Windows**.

Therefore open the Windows **control panel**, select **system** and then open the **advanced system settings**.

In the dialog box click on the button **[environment variables...]**.

![path](/images/path.PNG)

In the section **System Variables**, doubleclick on **Path**.

![path2](/images/path2.PNG)

Finally add the path where you installed OpenOCD.

![path3](/images/path3.PNG)


# Step 1: Building a New Firmware for MLS/160A

After the hard- and software setup is done, we can start building a new firmware for the MLS/160A.

First open the directory **/your MLS-directory/MLS160A-SDK/app**. You will see the following files inside the app-directory:

![folder_fw-mls160a_1](/images/folder_fw-mls160a_1.PNG)

The following table gives a short description of the files:

**File name** | **File purpose**
------------ | -------------
**build.bat** | Builds the new firmware files when executed - **Do not edit!**
**hmac.sh** | Is called automatically by build.bat and generates the hash files from the new generated firmware files - **Do not edit!**
**Makefile** | Makefile - **Do not edit!**
**make.sh** | Is called automatically by build.bat - **Do not edit!**
**main.c** | Main program file with the main c routines. **Edit this file to implement your own configurations.**
**dgram.c dgram.h** | Routines to build datagrams containing the sensor values that are sent to the RMG/941 - **Do not edit!**
**crc8.c crc8.h** | Cyclic redundancy check routine. Used to attach a CRC8 checksum to the sensor value datagrams - **Do not edit!**
**rs485.c rs485.h** | Implementation for the RS485 serial connection with the RMG/941 - **Do not edit!**.

Now start docker and **doubleclick on build.bat** to execute it. A command line prompt will open automatically where you can see the building process.

![build1](/images/build1.PNG)

When everything is finished, just press any key to close the command line.

The **build.bat** always creates 4 files, since the MLS/160A offers the A/B boot concept:

The files with **slot0** in their names are for slot A and the files with **slot1** are for slot B. The firmware files have the ending **.bin** and their hashes the ending **.hash**.

> **IMPORTANT! <br/>
> You will always need all 4 files to update the MLS/160A. Only the files with the same number in their name are important. If there are additional .bin and .hash files you can ignore them.**

In the directory **/app** a new directory called **/bin** has also been created, which is only needed for troubleshooting.

![folder_fw-mls160a_2](/images/folder_fw-mls160a_2.PNG)


# Step 2: Signing New Firmware with SDU Signing Tool

After the new firmware has been built, it has to be signed and packed with the SDU Signing Tool before it can be transferred to the RMG/941. 

Following settings have to be done in the SDU Signing Tool:

1. In the dropdown menu **Product** you have to select the target device, in this case it is the **SSV MLS/160A Sensor**. 

2. On the right side a version as well as a comment can be added, but are not mandatory.

3. In the field **Source files** new files can be added by clicking on **[Add File]**. Add the 4 files that have been generated in step 1 (.bin and .hash files). You can select several files at once.

4. Choose the **certificate** to sign the files with by clicking on **[Browse]**. On the right side you have to enter the certificate's password. If you use the **ssvdemo-user.pfx** file, the password is **ene**.

5. Now choose the **desired output** directory by clicking on **[Browse]**.

6. Finally click on **[Sign]** to start the process.

![sdu_st1](/images/sdu_st1.PNG)

If everything is right, a **message appears on the top** of the SDU Signing Tool.

A new **.sdu** file with **Product name** and **Version** as name was created in the desired output folder.

This .sdu file can be securely transmitted over the internet to the RMG/941.


# Step 3: Downloading New Firmware on RMG/941

In this step the **.sdu file** generated in step 2 is downloaded to the RMG/941.

1. Log into the RMG/941 to open the **SSV/WebUI**.

2. Select in the menu **Apps > SDU** to open the SDU app.

3. In the line **Update adapter** choose the **SSV MLS/160A Sensor** from the dropdown menu.

4. In the line **Upload firmware file** click on **[Browse]** (or "Durchsuchen..."), select the **.sdu file** created in step 2 and click on **[Upload]**.

If the file is okay, a message is displayed at the bottom of the page and the file is listed in the field **Available firmware files**.

![upload_success](/images/upload_success.png)


# Step 4: Uploading New Firmware to MLS/160A

The last step is to update the firmware of our MLS/160A.

Therefore just select the desired update file in the field **Available firmware files** and then click on **[Update]**.

Confirm the question if you are sure to update the firmware by clicking on **Yes**.

![question_sdu](/images/question_sdu.png)

If the update was successful a message on a green background appears.

![sdu_success](/images/sdu_success.png)
  
  
**Congratulations! You just updated the firmware of the MLS/160A :)**
  
  
# MLS/160A Webplot

To see the MLS/160A in action we offer a little **webplot demo** for the RMG/941. The demo is downloaded together with the MLS/160A SDK and is found in the directory called **/webplot-demo**.

If you want to run the demo, the demo files have to be transferred to the RMG/941. You can use an **FTP client** like FileZilla for this.

1. First connect to the RMG/941 with the FTP client, navigate to the directory where you want to save the new files on the RMG/941 (we suggest **/media/data/**) and select the **/webplot-demo** directory on your computer. Rightclick on the directory and then click on **Upload**.

![filezilla_2](/images/filezilla_2.PNG)

2. Before the demo can be run, the **file mls160a_webplot.py must be set as executable**. Therefore you have to log into the RMG/941 via PuTTY or Shellinabox (web console). In this example we use Shellinabox. Open the SSV/WebUI and click in the menu on **Services > General**. Click the checkbox next to **Shellinabox service** and click on **[Apply]**.

![general](/images/general.png)

3. A green arrow on the right side indicates that the service is running. To open Shellinabox just click on the link **web console** on the right side or enter `your ip address:4200` in the browser's address bar.

4. Log into the console with the username `root` and the password `root`. 

5. Enter the command `mc` to open the **Midnight Commander** tool.

6. Navigate to the directory where the demo files are stored, in this example **/media/data/webplot-demo**. To set the file **mls160a_webplot.py** as an executable use this command: `chmod +x mls160a_webplot.py`.

![shellinabox_2](/images/shellinabox_2.PNG)

7. Exit the Midnight Commander, change to the directory **/tmp** in Shellinabox and run the following command to start the demo webplot:

    `nohup /media/data/webplot-demo/mls160a_webplot.py &`

    This starts the mls160a_webplot.py in the background.

8. To see the webplot demo enter the IP address of the RMG/941 with the **port number 5000** in the address bar of the web browser, e.g.

    `192.168.0.126:5000`.

![webplot](/images/webplot.png)

# Factory reset

If the MLS/160A needs to be reset, OpenOCD is used for flashing. Therefore it is necessary to open the case and to connect the MLS/160A with the **SSV adapter cable** to the ST-Link/V2 and the ST-Link/V2 to the computer.

Then open a command prompt on the computer and change to the directory **/bin/ssv-mls160a/**.

Now execute the following command:

`openocd.exe -f board/st_nucleo_f103rb.cfg -c "init" -c "program mls160a-slot0-extended.bin verify reset exit 0x08000000"`

The MLS/160A should now work properly again!

![mls160a_open](/images/mls160a_open.JPG)

