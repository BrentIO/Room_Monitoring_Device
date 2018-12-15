/************************************************
*   Room Monitoring Device
*   Copyright 2018, P5 Software, LLC
*************************************************/

About
--------

The Room Monitoring Device (RMD) is designed to use low-cost hardware for monitoring the environment within a single room.  The hardware uses an ESP8266 NodeMCU v3
to control 4 hardware devices: a standard LED, passive infrared sensor, ambient light sensor, and a temperature/humidity/pressure sensor.

The RMD sends data to an MQTT server for listening applications.  It can also receive data from an MQTT request.


License
---------
Licensed under the Open Software License version 3.0

1) Grant of Copyright License. Licensor grants You a worldwide, royalty-free, non-exclusive, sublicensable license, for the duration of the copyright, to do the following:

a) to reproduce the Original Work in copies, either alone or as part of a collective work;

b) to translate, adapt, alter, transform, modify, or arrange the Original Work, thereby creating derivative works ("Derivative Works") based upon the Original Work;

c) to distribute or communicate copies of the Original Work and Derivative Works to the public, with the proviso that copies of Original Work or Derivative Works that You distribute or communicate shall be licensed under this Open Software License;

d) to perform the Original Work publicly; and

e) to display the Original Work publicly.

2) Grant of Patent License. Licensor grants You a worldwide, royalty-free, non-exclusive, sublicensable license, under patent claims owned or controlled by the Licensor that are embodied in the Original Work as furnished by the Licensor, for the duration of the patents, to make, use, sell, offer for sale, have made, and import the Original Work and Derivative Works.

3) Grant of Source Code License. The term "Source Code" means the preferred form of the Original Work for making modifications to it and all available documentation describing how to modify the Original Work. Licensor agrees to provide a machine-readable copy of the Source Code of the Original Work along with each copy of the Original Work that Licensor distributes. Licensor reserves the right to satisfy this obligation by placing a machine-readable copy of the Source Code in an information repository reasonably calculated to permit inexpensive and convenient access by You for as long as Licensor continues to distribute the Original Work.

4) Exclusions From License Grant. Neither the names of Licensor, nor the names of any contributors to the Original Work, nor any of their trademarks or service marks, may be used to endorse or promote products derived from this Original Work without express prior permission of the Licensor. Except as expressly stated herein, nothing in this License grants any license to Licensor's trademarks, copyrights, patents, trade secrets or any other intellectual property. No patent license is granted to make, use, sell, offer for sale, have made, or import embodiments of any patent claims other than the licensed claims defined in Section 2. No license is granted to the trademarks of Licensor even if such marks are included in the Original Work. Nothing in this License shall be interpreted to prohibit Licensor from licensing under terms different from this License any Original Work that Licensor otherwise would have a right to license.

5) External Deployment. The term "External Deployment" means the use, distribution, or communication of the Original Work or Derivative Works in any way such that the Original Work or Derivative Works may be used by anyone other than You, whether those works are distributed or communicated to those persons or made available as an application intended for use over a network. As an express condition for the grants of license hereunder, You must treat any External Deployment by You of the Original Work or a Derivative Work as a distribution under section 1(c).

6) Attribution Rights. You must retain, in the Source Code of any Derivative Works that You create, all copyright, patent, or trademark notices from the Source Code of the Original Work, as well as any notices of licensing and any descriptive text identified therein as an "Attribution Notice." You must cause the Source Code for any Derivative Works that You create to carry a prominent Attribution Notice reasonably calculated to inform recipients that You have modified the Original Work.

7) Warranty of Provenance and Disclaimer of Warranty. Licensor warrants that the copyright in and to the Original Work and the patent rights granted herein by Licensor are owned by the Licensor or are sublicensed to You under the terms of this License with the permission of the contributor(s) of those copyrights and patent rights. Except as expressly stated in the immediately preceding sentence, the Original Work is provided under this License on an "AS IS" BASIS and WITHOUT WARRANTY, either express or implied, including, without limitation, the warranties of non-infringement, merchantability or fitness for a particular purpose. THE ENTIRE RISK AS TO THE QUALITY OF THE ORIGINAL WORK IS WITH YOU. This DISCLAIMER OF WARRANTY constitutes an essential part of this License. No license to the Original Work is granted by this License except under this disclaimer.

8) Limitation of Liability. Under no circumstances and under no legal theory, whether in tort (including negligence), contract, or otherwise, shall the Licensor be liable to anyone for any indirect, special, incidental, or consequential damages of any character arising as a result of this License or the use of the Original Work including, without limitation, damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses. This limitation of liability shall not apply to the extent applicable law prohibits such limitation.

9) Acceptance and Termination. If, at any time, You expressly assented to this License, that assent indicates your clear and irrevocable acceptance of this License and all of its terms and conditions. If You distribute or communicate copies of the Original Work or a Derivative Work, You must make a reasonable effort under the circumstances to obtain the express assent of recipients to the terms of this License. This License conditions your rights to undertake the activities listed in Section 1, including your right to create Derivative Works based upon the Original Work, and doing so without honoring these terms and conditions is prohibited by copyright law and international treaty. Nothing in this License is intended to affect copyright exceptions and limitations (including "fair use" or "fair dealing"). This License shall terminate immediately and You may no longer exercise any of the rights granted to You by this License upon your failure to honor the conditions in Section 1(c).

10) Termination for Patent Action. This License shall terminate automatically and You may no longer exercise any of the rights granted to You by this License as of the date You commence an action, including a cross-claim or counterclaim, against Licensor or any licensee alleging that the Original Work infringes a patent. This termination provision shall not apply for an action alleging patent infringement by combinations of the Original Work with other software or hardware.

11) Jurisdiction, Venue and Governing Law. Any action or suit relating to this License may be brought only in the courts of a jurisdiction wherein the Licensor resides or in which Licensor conducts its primary business, and under the laws of that jurisdiction excluding its conflict-of-law provisions. The application of the United Nations Convention on Contracts for the International Sale of Goods is expressly excluded. Any use of the Original Work outside the scope of this License or after its termination shall be subject to the requirements and penalties of copyright or patent law in the appropriate jurisdiction. This section shall survive the termination of this License.

12) Attorneys' Fees. In any action to enforce the terms of this License or seeking damages relating thereto, the prevailing party shall be entitled to recover its costs and expenses, including, without limitation, reasonable attorneys' fees and costs incurred in connection with such action, including any appeal of such action. This section shall survive the termination of this License.

13) Miscellaneous. If any provision of this License is held to be unenforceable, such provision shall be reformed only to the extent necessary to make it enforceable.

14) Definition of "You" in This License. "You" throughout this License, whether in upper or lower case, means an individual or a legal entity exercising rights under, and complying with all of the terms of, this License. For legal entities, "You" includes any entity that controls, is controlled by, or is under common control with you. For purposes of this definition, "control" means (i) the power, direct or indirect, to cause the direction or management of such entity, whether by contract or otherwise, or (ii) ownership of fifty percent (50%) or more of the outstanding shares, or (iii) beneficial ownership of such entity.

15) Right to Use. You may use the Original Work in all ways not otherwise restricted or conditioned by this License or by law, and Licensor promises not to interfere with or be responsible for such uses by You.

16) Modification of This License. This License is Copyright © 2005 Lawrence Rosen. Permission is granted to copy, distribute, or communicate this License without modification. Nothing in this License permits You to modify this License as applied to the Original Work or to Derivative Works. However, You may modify the text of this License and copy, distribute or communicate your modified version (the "Modified License") and apply it to other original works of authorship subject to the following conditions: (i) You may not indicate in any way that your Modified License is the "Open Software License" or "OSL" and you may not use those names in the name of your Modified License; (ii) You must replace the notice specified in the first paragraph above with the notice "Licensed under <insert your license name here>" or with a notice of your own that is not confusingly similar to the notice in this License; and (iii) You may not claim that your original works are open source software unless your Modified License has been approved by Open Source Initiative (OSI) and You comply with its license review and certification process.


Hardware Bill of Materials
----------------------------
1 x ESP8266 NodeMCU v3 (https://www.aliexpress.com/item/Wireless-module-NodeMcu-v3-Lua-WIFI-Internet-of-Things-development-board-ESP8266-with-pcb-Antenna-and/32656775273.html)
1 x GY-302 BH1750FVI Light Intensity Module (https://www.aliexpress.com/item/GY-302-BH1750-Chip-Light-Intensity-Light-Module/32319393979.html)
1 x GY-BME280 Temperature/Humidity/Barometric Pressure Module (https://www.aliexpress.com/item/BME280-Digital-Sensor-Temperature-Humidity-Barometric-Pressure-Sensor-Module-GY-BME280-I2C-SPI-1-8-5V/32831163980.html)
1 x AM312 Passve Infrared Sensor (https://www.aliexpress.com/item/AM312-DC-2-7-to-12V-Mini-IR-Pyroelectric-Infrared-PIR-Motion-Human-Sensor-Automatic-Detector/32860464854.html)
1 x 3mm Round LED (https://www.aliexpress.com/item/100PCS-UltraBright-White-Ultra-Bright-3mm-Round-LED-Diode-3mm-White-Led/32710028410.html)


Device Provisioning
---------------------

After flashing the sketch to the Arduino, RMD starts as an access point and web server.  The access point name will be RMD-[MAC].  The hardware address will have no spaces,
dashes, or colons.

Connect to the RMD-[MAC] access point then point a browser to http://192.168.1.1.  The provisioning page will display for the intermediate SSID, WPA key, and a URL for the
RMD to connec to and read a bootstrap file.  The bootstrap file defines the device's settings.

When specifying the bootstrap URL, you must use a fully-qualified domain name or IP address.

The bootstrap file is a JSON document which defines the settings that the device should use.  It may or may not be the same SSID and key that were defined in the fields
on the device provisining screen.

An example bootstrap JSON:

    {
        "SSID": "MyWiFi",
        "wpaKey": "SuperStrongWifiPassW0rd!",
        "mqtt": {
            "server": "myMQTTServer.local",
            "port": 1883
        }
    }

Upon clicking the "Provision" button, the device will return "OK" on the web browser.  The best indication a user will have of a successful provisioning will be the
disappearance of the RMD-[MAC] WiFi network.

If provisioning is successful, the device will reboot and begin to transmit MQTT messages with the measurements from the peripherals.

If provisining is unsuccessful, the device will fall back to the device provisining screen and prompt the user for information again.  The infrastructure access point and
web server are restarted.


MQTT Topics
--------------

There is one inbound MQTT topic and two outbound MQTT topics.  The inbound topic is /manage/ and the two outbound topics are /client/ for non-sensor information and 
/sensor/ for sensor data.

An example MQTT topic name for a device with a hardware address of AA:BB:CC:DD:EE:FF would be /manage/AABBCCDDEEFF/status.


 == Inbound ==
 The inbound MQTT topic is /manage/[MAC]/[Command], where [MAC] is the hardware address without spaces, dashes, or colons; [Command] is a command that the device will
 respond to, if the command is known.

 >> Commands <<
 The device understands certain commands when posted to an MQTT topic.  The only topic which the device subscribes to is /manage/[MAC]/#.  Responses to commands are sent
 to the /client/[MAC]/[Command] topic.


    /status
        Requests the status of the device, similar to a heartbeat.  If the device is responsive, it will send "OK" to the outbound /client/[MAC]/status topic.  No inbound
        payload is required to render a response, and any payload will be ignored.

    /uptime
        Requests the number of milliseconds (mills) that the device has been active.  If the device is responsive, it will a long integer value to the outbound 
        /client/[MAC]/uptime topic.  No inbound payload is required to render a response, and any payload will be ignored. 

    /provisionReset
        Requests the RMD to reset to sketch defaults and remove all settings, similar to a factory reset.  If the device is responsive, it will send a message on the
        topic /client/[MAC]/status with a payload of "provisionReset" and on the /client/[MAC]/restart topic with a payload of "OK".  An inbound payload of "request", case
        insensitive, is required to take the action.  Any other payload will be ignored and the action will not be taken.
    
    /restart
        Requests the RMD to perform a soft reset.  If the device is responsive, it will send a message on the topic /client/[MAC]/restart topic with a payload of "OK".  An
        inbound payload of "request", case insensitive, is required to take the action.  Any other payload will be ignored and the action will not be taken.
        
    /onboardLED
        Sets the on-board LED to be on, off, or to blink modes.  It can also be used to get the status of the onboard LED mode.  If the device is responsive, it will send
        a message on the topic /client/[MAC]/onboardLED with the current mode.  Four inbound payloads are supported, case insensitive: "on", "off", "blink", and an empty
        payload.  An empty payload is equivalent to simply requesting the current mode.
        
 == Outbound ==

 >>Sensor<<
 All sensor output is sent to the /sensor/[MAC]/[Peripheral] topic.  Sensor data is read once every 5 seconds, except motion, which is read with each loop because the sensor
 includes its own timer.  Note that the on-board LED is not considered a sensor and will not post data to the sensor topic.

 Each peripheral will output its own sensor data on the topic with the payload inclusive of its most current reading.  There is a minimum delta between the last reported
 observation and the current observation that must be met in order for the peripheral to output data.  By default, all peripherals will report on a restart.

 The peripherals supported are:
    /temperature
        The observed temperature in celcius with two digits of precision (XX.YY).

    /humidity
        The observed humidity as a percentage with two digits of precision (XX.YY).

    /pressure
        The observed atmospheric pressure in inches of mercury with two digits of precision (XX.YY).

    /luminance
        The observed light intensity in lux as whole integer (XX).

    /motion
        Motion activity where 0 = no motion reported and 1 = motion detected.  Motion will continue to be reported as 1 as long as motion is present plus
        approximately 2 seconds.

 >>Client<<
 Information about the device itself is output to the /client/[MAC]/[Command] topic.  This topic is the response to all /manage requests.

 The topics that are output are:
    /status
        The status of the device and related messages about device health.  Payloads supported: "started", which will trigger on each device restart indicating the RMD
        is connected successfully to MQTT; "OK", for a solicited heartbeat update; "provisionReset", a confirmation that the provisioning has been reset; 
        "Unable to Find Sensor BME280 Disabling Peripherals", indicating a hardware failure where the atmospheric sensor is not responsive.

    /restart
        Payloads supported: "OK", A notification that the device soft restart has been received and will be attempted.

    /uptime
        The number of milliseconds the device has been active.  Payloads supported: varies, as a data type long.

    /onboardLED
        Returns the status of the on-board LED setting.  Payloads supported: "on", indicating the LED is currently illuminated; "off", indicating the LED is not illuminated;
        "blink", indicating the LED is blinking on and off at a repeated interval; "disabled", indicating a hardware failure.