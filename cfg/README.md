# UNICENS XML Description

This file describes how to write a valid UNICENS XML configuration file.

**1.) XML Basics**

Be aware that the rules of XML enforce that the document is well formed.
This means it is zero tolerant, a single typo will invalidate the whole document and any application using it must not continue.
So in any case it is useful  to use an IDE to check the document.
This can be the UNICENS System Designer, which covers also deeper rule set related to UNICENS.
But any other editor will also work, examples are Eclipse, NetBeans, Visual Studio.

**In short the XML provides those terminolgies: *(partly from Wikipedia)***

Tag
A _tag_ is a construct that begins with `<` and ends with `>`. 
Tags come in three flavors:

-   _start-tag_, such as  `<section>`;
-   _end-tag_, such as  `</section>`;
-   _empty-element tag_, such as  `<line-break />`.

Tags can be stored inside other tags. This will form a tree architecture.
For example:
```xml
<Parent>
	<Child/>
	<Child>
		<GrandChild/>
	</Child>
</Parent>
```
Then the tree would like that:
```
Parent
│
└───Child
│
└───Child
    │
    └─── GrandChild
```

Element
An _element_ is a construct that begins with a start-tag and ends with a matching end-tag. 
An example is `<greeting>Hello, world!</greeting>`
Note, that the UNICENS schema is not using elements currently.

Attribute
An _attribute_ is a construct consisting of a name–value pair that exists within a start-tag or empty-element tag.
An example is `<img src="madonna.jpg" alt="Madonna" />`, where the names of the attributes are "src" and "alt",
and their values are "madonna.jpg" and "Madonna" respectively.

XML declaration
XML documents may begin with an  _XML declaration_  that describes some information about themselves. 
An example is  `<?xml version="1.0" encoding="UTF-8"?>`

Schema
In addition to being well-formed, an XML document may reference to an external rule set (Document Type Definition (DTD)).
There are certain rules applicable, such as the name of tags, elements and attributes.
Also the values of the attributes can be specified as regular expression.
Such a file is shipped along with UNICENS, its called *unicens.xsd* is shipped.
The linkage between the document and its DTD is done in the first tag (Root Tag):
 `<Unicens xsi:noNamespaceSchemaLocation="unicens.xsd">`

Comments
_Comments_ may appear anywhere in a document. Comments begin with `<!--` and end with `-->`
An example of a valid comment: `<!--this is a comment -->`

**2.) Start with an empty document**

Add the XML declaration and the root tag *Unicens*, along with the reference to the used Schema.
Make sure that the file unicens.xsd is in the same folder as your current document.

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
</Unicens>
```
**2.1) Bandwidth calculation**

The attribute AsyncBandwidth in the previous example is mandatory.
It specifies how fast the Ethernet channel on INICnet shall be.
The value specified is in Bytes within 48kHz. So in the example above, this would mean 

> 80 Bytes \* 48000 1/s = 3,84 MByte/s 
> 3,84 MByte/s \* 8 Bit/Byte = 30,72 MBit/s

Note the total network bandwidth limit for both INICnet speed grades:

| Speedgrade | Bytes Within 48kHz  |
|------------|---------------------|
| INICnet50  | 116 Byte            |
| INICnet150 | 372 Byte            |

This means for the example above, that the remaining bandwidth for audio / video streaming on an INICnet50 network is 36 Byte (116 Byte - 80 Byte).

Setting the AsyncBandwidth to 0 is allowed. Then no Ethernet communication is possible and all bandwidth is available for audio and video streaming.

Beside of using the Ethernet channel, there are dedicated streaming channels to transport audio and video.
They use also use the bytes within 48kHz as their bandwidth configuration.

This table gives an idea, what amount of Bytes within 48kHz is used for which use case and what data rate it consumes on the network.

| Use Case                  | Data Type | Bytes Within 48kHz | Bandwidth in MBit/s |
|---------------------------|-----------|--------------------|---------------------|
| Mono 16 Bit               | Synchron  | 2                  | 0,768               |
| Mono 24 Bit               | Synchron  | 3                  | 1,152               |
| Stereo 16 Bit             | Synchron  | 4                  | 1,536               |
| Stereo 24 Bit             | Synchron  | 6                  | 2,304               |
| 5.1 channels 16 Bit       | Synchron  | 12                 | 4,608               |
| 5.1 channels 24 Bit       | Synchron  | 18                 | 6,912               |
| 7.1 channels 16 Bit       | Synchron  | 16                 | 6,144               |
| 7.1 channels 24 Bit       | Synchron  | 24                 | 9,216               |
| Low Quality H264 Video    | Isochron  | 8                  | 3,072               |
| Superb Quality H264 Video | Isochron  | 80                 | 30,72               |

**3.) Defining the nodes**

A node is a device wich participates on the INICnet.
There are three categories which a node can belong to:

 1. The root node. It runs the central UNICENS stack. There can only be one device with this role.
 2. A slim node without any CPU on it, such as an Microphone or Booster. It is completely remote controlled. Any GPIO or I2C peripheral can be remotely triggered from the root node via the network.
 3. A smart node with an CPU or micro-controller. It is also completely remote controlled and have GPIO and I2C remote access like the slim node. But there are two additional ways to communicate (peer to peer): via the INICnet control channel and the INICnet Ethernet channel.

Slim Nodes and Smart Nodes may be equipped multiple times in the network. 
The theoretical limit of nodes per network is 64. Due to power and timing reasons, the actual value can be lower than that.

Each device needs to have a Node Address which only exists once in that network (not global unique like a MAC address).
This node address specified by the system integrator and need to be flashed into the Flash or OTP memory of the INIC chip.

This Node Address is a 16 Bit address, which is usually written in hexadecimal representation.
Due to historical reasons the range of the Node Address is limited to those sections:

| Start |  End  |
|-------|-------|
| 0x10  | 0xFF  |
| 0x140 | 0x2FF |
| 0x500 | 0xEFF |

This table can be used as an example starting point for a system integrator:

| Address Range  | Device Type              | Instance Numbers |
|----------------|--------------------------|------------------|
| 0x200          | UNICENS Master Node      | 1                |
| 0x201          | Smart Antenna            | 1                |
| 0x202          | Digital Signal Processor | 1                |
| 0x205 .. 0x209 | Cluster                  | 1 .. 5           |
| 0x210 .. 0x23F | Microphone               | 1 .. 64          |
| 0x240 .. 0x26F | AUX IO Board Instance    | 1 .. 64          |
| 0x270 .. 0x29F | Slim Amplifier Instance  | 1 .. 64          |
| 0x2B0 .. 0x2DF | Entertainment System     | 1 .. 64          |
| 0x2E0 .. 0x2EF | Camera Instance          | 1 .. 16          |

Specifying an node is done with the \<Node> tag. All node tags are childs from the UNICENS tag.
It's important to know, that all devices need to be specified in the document.
If a device with an unknown Node address tries to enter the network it's getting ignored.

For example, specifying two devices in the Network: UNICENS Master Node and a smart Entertainment System:

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200"/>
	<Node Address="0x2B0"/>
</Unicens>
```
With this example the mentioned devices are allowed to join the network.
As there is no additional information given, the devices can communicate via the Control
and Ethernet channel but there is no dedicated channels for Audio or Video transmission.

**4.)  Connections**

In order to get the major benefits from INICnet, certain connections needs to be defined.
Those connections are reserved bandwidth on the network, which are guarantied for that particular use case only (Quality of Service or QoS).

**4.1) Defining Synchronous Streams for Audio use cases**

For transportation of uncompressed audio data the synchronous data channel of INICnet is optimal.
It always streams with a constant data rate which is by design synchronous on every device.
This means there is no offset, no drift and no Jitter problems to deal with.
To define a synchronous connection the \<SyncConnection> tag is used.
It is a child of the \<Node> tag described in Chapter 3. Every node may have more than one synchronous connection.

A **non** working example (because of missing parameters) would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<SyncConnection>
		</SyncConnection>
	</Node>
	<Node Address="0x2B0">
		<SyncConnection>
		</SyncConnection>
	</Node>
</Unicens>
```

**4.2) Defining Isochronous Streams for compressed Audio and Video use cases**

In contrast to the synchronous data channel, the isochronous channel is able to transfer a variable data rate.
The user specifies a worst case (or burst) date rate, which then is allocated on the network.
When the used compression algorithm (Example:H264 and/or MP3) produce less or no bandwidth, the channel utilization becomes also less.
The specialty about this is stream is that a single data chunk is 188 Byte, when unencrypted or 192 Byte when encrypted.
Refer [MPEG transport stream](https://en.wikipedia.org/wiki/MPEG_transport_stream) for more information.
To define an isochronous connection the \<AVPConnection> tag is used (Audio Video Packetized).
It is a child of the \<Node> tag described in Chapter 3. Every node may have more than one isochronous connection.

A **non** working example (because of missing parameters) would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<AVPConnection>
		</AVPConnection>
	</Node>
	<Node Address="0x2B0">
		<AVPConnection>
		</AVPConnection>
	</Node>
</Unicens>
```

**5.) Working with Sockets**

A socket represents a way into the INIC or out of the INIC. It references the used INIC port and specify how the data shall be formatted on that port.
This are the possible socket types:

| XML Tag          | Mandatory Attributes                  | Usage                      |
|------------------|---------------------------------------|----------------------------|
| \<USBSocket>     | EndpointAddress, FramesPerTransaction | Universal Serial Bus (USB) |
| \<MediaLBSocket> | ChannelAddress, Bandwidth             | Media Local Bus (MLB)      |
| \<StreamSocket>  | StreamPinID, Bandwidth                | I2S/TDM/PDM port           |
| \<NetworkSocket> | Bandwidth, Route                      | INICnet channel            |

Always two Sockets resist inside a connection (Applies to \<SyncConnection> and \<AVPConnection>).
The order of the socket tag inside the connection tag is crucial.
The first socket is the input socket, where the second is the output socket, both of with the view of the INIC.
In any case, the \<NetworkSocket> tag must be either in the first or the second entry.
And the input and output sockets must be from a different kind.

A **non** working example (because of missing parameters) of routing isochronous data from the USB port to the INICnet channel  would be:
```xml
...
	<Node Address="0x200">
		<AVPConnection>
			<!-- Input (INIC view) is USB: -->
			<USBSocket/>

			<!-- Output (INIC view) is isochronous channel on INICnet: -->
			<NetworkSocket/>
		</AVPConnection>
	</Node>
...
```
A **non** working example (because of missing parameters) of routing synchronous data from the INICnet channel to an streaming port (I2S):
```xml
...
	<Node Address="0x200">
		<SyncConnection>
			<!-- Input (INIC view) is synchronous channel on INICnet: -->
			<NetworkSocket/>

			<!-- Output (INIC view) is Streaming Port (I2S): -->
			<StreamSocket/>
		</SyncConnection>
	</Node>
...
```

**5.1) Defining an USB Socket**

Following two Attributes are mandatory to define a valid USB Socket:

 - EndpointAddress=".."
	 - This is the USB endpoint address. This is an byte value usually written in hexadecimal notation. If the data is transmitted out of the INIC, received by the main CPU (EHC), the highest bit is set, this leads to values starting with 0x80. Data which is received by the INIC, sent by the EHC no additional bits are set, so the starting address is 0x00.
	 - Endpoint 0x00 is reserved for administrative purposes.
	 - For streaming data, the INIC supports currently in maximum 5 endpoints. So the range 0x01 .. 0x05 (EHC TX) and 0x81 .. 0x85 (EHC RX) may be used.
	 - Control data is on endpoints for OS81118/9 are 0x0F & 0x8F; for OS81210 are 0x07 & 0x87 (must not be used inside SyncConnection or AVPConnection)
 	 - asynchronous Ethernet is on endpoints for OS81118/9 are 0x0E & 0x8E; for OS81210 are 0x06 & 0x86 (must not be used inside SyncConnection or AVPConnection)
- FramesPerTransaction=".."
	- A micro frame on USB is chunk of 512 Byte. This is the maximum transmission unit (MTU) for the used bulk transfer mode on USB. In order to get a very efficient streaming behavior the INIC always fills a micro frame completely, so there is no additional signaling needed. Depending on the use case, waiting for 512 Byte of data (for example audio data) to arrive and then start the transmission after that can cause a measurable latency, which for example for active noise cancellation use cases my be unwanted.
	- To achieve low latency, the integrator can choose to not fill the the micro frame entirely with data. This means, that the transmission is started earlier, with less then 512 Bytes of valid streaming data. The software driver (for EHC TX) or the INIC hardware for (EHC RX) automatically appends invalid stuffing data to fill the micro frame and keep the signaling overhead as less as possible. The invalid stuffing data is never transported on the INICnet, so no bandwidth on the network is wasted.
	-  The FramesPerTransaction value describes how many samples (PCM, PDM or TS-Packets) are put into one micro frame. For instance, if the there is an stereo 16 bit PCM audio stream (2x2Byte = 4 Byte) to be transported, the maximum possible value for FramesPerTransaction would be 128 (128 x 4 Byte = 512 Byte). Setting an value of 64 in that particular case would leave the micro frame half filled, improving the latency and downgrade the efficiency of USB.
	- For synchronous sockets the **minimum** value for FramesPerTransaction is **7**! This is due to the different timing of USB and INICnet.
	- For isochronous sockets there are only two possibilities: FramesPerTransaction set to 2, in that case two transport stream packets (2 x 188/192 Byte) will stored in one micro frame. The second option is to set the FramesPerTransaction value to 0xFF. In that case, the micro frame is always completely filled. But as 512 Byte of micro frame is not dividable by 188/192 Byte, the fractional rest of the streaming data is put into the next micro frame. This means, that on the received side the integrator can not rely any longer on the fact, that the first Byte which is received, is the first Byte of the transport stream packet. In that case you need to search for 0x47 inside the payload of the stream, which marks the start of a transport stream packet.
	- In contrast to other sockets, the USB socket bandwidth must not be specified. It automatically adjusts its speed to the corresponding network socket.

**5.2) Defining a MLB Socket**

The Media Local Bus is a dedicated bus for interfacing the INIC and Companions. 
It is adopted by many vendors like Atmel SAM V71, NXP i.MX6, Rensas RCAR H3/M3.
It can provide very low latency (lower than USB). 
Of cause the operating system needs to have a scheduler which is fast enough to deal with the increasing amount of interrupts.
Following two Attributes are mandatory to define a valid MLB Socket:
 - ChannelAddress=".."
	 - Integer value between 10 .. 64
	 - Value must be even!
	 - Channel address 0 is unused
	 - Channel addresses 2 & 4 are reserved for control channel
	 - Channel addresses 6 & 8 are reserved for asynchronous Ethernet channel
 - Bandwidth=".."
	 - The amount of Bytes transferred within 48kHz. See (2.1) Bandwidth calculation.
	 - The MLB port can be configured with different speed rates (FS). Depending on the chosen speed, the bandwidth must be in a certain range:

| MLB Speed (Fs) | MLB type |  Bandwidth Range (Byte)  |
|----------------|----------|--------------------------|
|  256           | 3 pin    |  1 .. 28                 |
|  512           | 3 pin    |  1 .. 60                 |
|  1024          | 3 pin    |  1 .. 124                |
|  2048          | 6 pin    |  1 .. 228                |
|  3072          | 6 pin    |  1 .. 344                |
|  4096          | 6 pin    |  1 .. 372                |

**5.3) Defining a Stream Socket**

The Streaming Port addressed with a Stream Socket, is most known as I2S port.
But it can also support multi channel TDM and PDM data types.
Following two Attributes are mandatory to define a valid Stream Socket:
- StreamPinID=".."
	- The hardware pin of the INIC, transporting the data part of the stream. FSY and CLK may be shared between multiple data pins.
	- Following enumeration represents the allowed values. Choose one out of it: SRXA0, SRXA1, SRXB0, SRXB1
 - Bandwidth=".."
	 - The amount of Bytes transferred within 48kHz. See (2.1) Bandwidth calculation.
	 - The Streaming port can be configured with different speed rates (FS) and different data formats. Depending on the chosen speed and format, the bandwidth must be in a certain range:

| Streaming Port Speed (Fs) | 16Bit Bandwidth Range | 24Bit Bandwidth Range  | Sequential Bandwidth Range |
|---------------------------|-----------------------|------------------------|----------------------------|
|  64                       |  1 .. 4               |  1 .. 6                |  1 .. 8                    |
|  128                      |  1 .. 8               |  1 .. 12               |  1 .. 16                   |
|  256                      |  1 .. 16              |  1 .. 24               |  1 .. 32                   |
|  512                      |  1 .. 32              |  1 .. 48               |  1 .. 64                   |                    |

**5.4) Defining a Network Socket**

The Network Socket describes the resources allocated on the INICnet streaming channels.
As mentioned earlier, in every Connection (SyncConnection, AVPConnection), there must be exactly one Network Socket defined, either as input or as output.
Following two Attributes are mandatory to define a valid Network Socket:
 - Bandwidth=".."
	 - The amount of Bytes transferred within 48kHz. See (2.1) Bandwidth calculation.
	 - This value should match to the opposite socket bandwidth (except for USB).
	 - For isochronous streaming on MLB the socket bandwidth of the Network Socket and the MediaLB Socket may be different.
 - Route=".."
	 - The value is any sort of user defined name, there is no syntax to be followed (other than to be XML complaint).
	 - This route name acts as a reference. 
	 - If two connections share the same route name, they will be automatically connected.
	 - The integrator must make sure that all source network sockets are connected to at least one sink network socket.

An example, routing a microphone to a head unit and an additional slave device would be:

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<SyncConnection>
			<NetworkSocket Bandwidth="4" Route="Route_Microphone" />
			<USBSocket EndpointAddress="0x81" FramesPerTransaction="128" />
		</SyncConnection>
	</Node>

	<Node Address="0x2B0">
		<SyncConnection>
			<NetworkSocket Bandwidth="4" Route="Route_Microphone" />
			<MediaLBSocket ChannelAddress="0x10" Bandwidth="4"/>
		</SyncConnection>
	</Node>

	<Node Address="0x210">
		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="4" />
			<NetworkSocket Route="Route_Microphone" Bandwidth="4" />
		</SyncConnection>
	</Node>
</Unicens>
```

In the example above, the node 0x210 is acting as audio source, because the \<NetworkSocket> is the second entry in the \<SyncConnection>.
It defines the route name "Route_Microphone", as mentioned earlier, any name would be valid here.
You will find exactly the same name (case and space sensitive!) in node 0x200 and 0x2B0.
This time \<SyncConnection> is the first entry within \<SyncConnection>, meaning those connections are the sinks.
So both devices are getting the audio data from the microphone in parallel and with the same latency and phase.
The node 0x200 is streaming to USB endpoint address 0x81 and the node 0x2B0 to MLB channel address 0x10.

**6.) Working with Combiner and Splitter**

For audio streaming use cases on synchronous channel it can be very helpful to group or separate channels. 

**6.1) Defining a Combiner**

The Combiner is for synchronous connections only and has the ability to join multiple audio streams from the network to one big time division multiplex (TDM) data stream. This newly generated stream can be routed out to USB, MediaLB or Streaming port. For instance to combine 3 stereo microphones on the network into a single six channel TDM stream on USB.
Doing so has multiple benefits:
	- It safes resources (USB endpoints, MLB channels or Streaming Pins).
	- It keeps multiple channels in the correct phase relation. Especially important for beam forming and noise canceling applications.
	- It helps to improve the efficiency of the sink port in low latency use cases. Especially the USB bus becomes inefficient, if it must transport very small data chunks (see FramesPerTransaction attribute in 5.1).

A Combiner resists in  inside a \<SyncConnection> as first entry, meaning it is an input to the INIC. It has one mandatory attribute:
 - BytesPerFrame=".."
	 - The combined amount of Bytes within 48kHz for all streams together.
	 - For example, combining three stereo microphones would need a value of 6.
	 - This value is then the same as the corresponding Bandwidth attribute of the opposite \<MediaLBSocket> or \<StreamSocket> tag.
	 - This value needs to be considered also in the calculation for the FramesPerTransaction attribute, if the opposite socket is an \<USBSocket>

The childs of the Combiner are then multiple \<NetworkSocket> tags with additional mandatory attributes:
 - Offset=".."
	 - This attribute declares the start position inside of the combined TDM stream for that particular \<NetworkSocket>.
	 - A value of 0 means that it shall be routed to begin of the TDM stream.
	 - This value must be smaller than the attribute BytesPerFrame of the \<SyncConnection> tag
 - Bandwidth 
	 - This attribute was already marked as mandatory in (5.4). But in the Combiner context it is also used to define indirect the end position inside of the combined TDM stream: 
*End = Bandwidth + Offset*

Each \<NetworkSocket> forms with those two information a block inside the TDM stream. The integrator must ensure that the blocks are not overlapping it each other.

An example, routing three mono microphones to a head unit using a Combiner:

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<SyncConnection>
			<Combiner BytesPerFrame="6">
				<NetworkSocket Route="Mic_Channel1" Offset="0" Bandwidth="2" />
				<NetworkSocket Route="Mic_Channel2" Offset="2" Bandwidth="2" />
				<NetworkSocket Route="Mic_Channel3" Offset="4" Bandwidth="2" />
			</Combiner>
			<StreamSocket StreamPinID="SRXA1" Bandwidth="6" />
		</SyncConnection>
	</Node>

	<Node Address="0x210">
		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="2" />
			<NetworkSocket Route="Mic_Channel1" Bandwidth="2" />
		</SyncConnection>
	</Node>

	<Node Address="0x211">
		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="2" />
			<NetworkSocket Route="Mic_Channel2" Bandwidth="2" />
		</SyncConnection>
	</Node>

	<Node Address="0x212">
		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="2" />
			<NetworkSocket Route="Mic_Channel3" Bandwidth="2" />
		</SyncConnection>
	</Node>
</Unicens>
```
**6.2) Defining a Splitter**

The Splitter is for synchronous connections only and has the ability to cut multi channel audio TDM streams into one or multiple audio streams with smaller bandwidth and stream them to the network.
Doing so has one benefits:
	- It helps arrange the audio channels in different orders.
	- For instance, the integrator can choose to have only mono channels on the network using a Splitter. On the sink side he can then combine various mono channels (also from different source devices) for that particular use case using a Combiner (see 6.1).

A Splitter resists in  inside a \<SyncConnection> as second entry, meaning it is an output of the INIC. It has one mandatory attribute:
 - BytesPerFrame=".."
	 - The amount of Bytes within 48kHz of the source stream.
	 - This value is then the same as the corresponding Bandwidth attribute of the opposite \<MediaLBSocket> or \<StreamSocket> tag.
	 - This value needs to be considered also in the calculation for the FramesPerTransaction attribute, if the opposite socket is an \<USBSocket>

The childs of the Splitter are then multiple \<NetworkSocket> tags with additional mandatory attributes:
 - Offset=".."
	 - This attribute declares the cutting start position inside of the source TDM stream for that particular \<NetworkSocket>.
	 - A value of 0 means that it shall be cutted at the begin of the TDM stream.
	 - This value must be smaller than the attribute BytesPerFrame of the \<SyncConnection> tag
 - Bandwidth 
	 - This attribute was already marked as mandatory in (5.4). But in the Splitter context it is also used to define indirect the cutting end position inside of the combined TDM stream: 
*End = Bandwidth + Offset*

Each \<NetworkSocket> forms with those two information a block inside the TDM stream. The integrator must ensure that the blocks are not overlapping it each other.

Here is An example, cutting a 5.1 multi channel stream from a head into three stereo streams using a Splitter. The sinks are three stereo slim amplifier.

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
        <SyncConnection>
            <USBSocket EndpointAddress="0x1" FramesPerTransaction="42"/>
            <Splitter BytesPerFrame="12">
                <NetworkSocket Route="Route_HeadUnit_Front" Offset="0" Bandwidth="4"/>
                <NetworkSocket Route="Route_HeadUnit_Rear" Offset="4" Bandwidth="4"/>
                <NetworkSocket Route="Route_HeadUnit_Effect" Offset="8" Bandwidth="4"/>
            </Splitter>
        </SyncConnection>
    </Node>

    <Node Address="0x270">
        <SyncConnection>
            <NetworkSocket Route="Route_HeadUnit_Front" Bandwidth="4"/>
            <StreamSocket StreamPinID="SRXA0" Bandwidth="4"/>
        </SyncConnection>
    </Node>

    <Node Address="0x271">
        <SyncConnection>
            <NetworkSocket Route="Route_HeadUnit_Rear" Bandwidth="4"/>
            <StreamSocket StreamPinID="SRXA0" Bandwidth="4"/>
        </SyncConnection>
    </Node>

    <Node Address="0x272">
        <SyncConnection>
            <NetworkSocket Route="Route_HeadUnit_Effect" Bandwidth="4"/>
            <StreamSocket StreamPinID="SRXA0" Bandwidth="4"/>
        </SyncConnection>
    </Node>    
</Unicens>
```

**7.) Defining an Audio Loopback**

In certain cases it may be helpful to route the audio from the source back to it self (looping back). This may be the case, where the radio tuner shall not accidental activate the head units wake word (like "Alexa" or "Hey Siri").
To achieve this simply add two \<SyncConnection> where the Route name is the same, and where the \<NetworkSocket> is an output in connection, and the input on the other connection. The target bus (USB, MediaLB, Streaming Port) may be different for both connections.

An example, routing an SyncConnection back to the same device (0x200):

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<!-- Audio sent by the Head Unit -->
		<SyncConnection>
			<USBSocket EndpointAddress="0x1" FramesPerTransaction="128" />
			<NetworkSocket Bandwidth="4" Route="Amp" />
		</SyncConnection>
		<!-- Loopback, to be analyzed by an attached DSP -->
		<SyncConnection>
			<NetworkSocket Bandwidth="4" Route="Amp" />
			<MediaLBSocket ChannelAddress="0x10" Bandwidth="4" />
		</SyncConnection>
		<!-- Microphone, to be analyzed by an attached DSP -->
		<SyncConnection>
			<NetworkSocket Bandwidth="4" Route="Mic" />
			<MediaLBSocket ChannelAddress="0x12" Bandwidth="4" />
		</SyncConnection>
	</Node>

	<Node Address="0x210">
		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="4" />
			<NetworkSocket Route="Mic" Bandwidth="4" />
		</SyncConnection>
	</Node>

	<Node Address="0x270">
		<SyncConnection>
			<NetworkSocket Route="Amp" Bandwidth="4" />
			<StreamSocket StreamPinID="SRXA0" Bandwidth="4" />
		</SyncConnection>
	</Node>
</Unicens>
```
Unfortunately it is not possible to use a loopback with connections where a Splitter or a Combiner is used!

**8.) Switching Connections**

It may be very useful to activate / deactivate certain connections (applies to \<SyncConnection> and \<AVPConnection>).
Reasons to do so are:
 - Save Network bandwidth. If all connections together consume more bandwidth as the network can handle, switching off unused streams can free the needed space.
- Having multiple sources. Connections can only be established, if there is exactly one source available. However, if a source is switched off, another source may be activated instead.
- Safely shut down audio connections to avoid plopping sound.

In order to prepare a connection to be switched, the integrator needs to add two optional attributes to the \<NetworkSocket> tag (valid for source and sink):
 - RouteId=".."
	 - The RouteId  value must be a unsigned 16 bit value (0x0 .. 0xFFFF). The RouteId value must be unique over the complete XML file. It acts as a handle, which can be used later in the C-code to address this connection.
 - IsActive=".."
	 - The IsActive value is a Boolean (either "true" or "false"). If this attribute is not specified, the default behavior is activated. If there are multiple sources for one connection, only one source may be activated by setting "true", all other need to be set to "false".

An example, supporting switching of multiple sources, would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">

	<Node Address="0x200">
		<SyncConnection>
			<USBSocket EndpointAddress="0x1" FramesPerTransaction="128" />
			<NetworkSocket Bandwidth="4" Route="HeadUnit" />
		</SyncConnection>
	</Node>

	<Node Address="0x210">
		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="4" />
			<NetworkSocket Route="Microphone" Bandwidth="4" />
		</SyncConnection>
	</Node>

	<Node Address="0x240">
		<SyncConnection>
			<NetworkSocket Route="HeadUnit" Bandwidth="4" RouteId="0x1001" IsActive="true" />
			<StreamSocket StreamPinID="SRXA1" Bandwidth="4" />
		</SyncConnection>
		
		<SyncConnection>
			<NetworkSocket Route="Microphone" Bandwidth="4" RouteId="0x1002" IsActive="false" />
			<StreamSocket StreamPinID="SRXA1" Bandwidth="4" />
		</SyncConnection>

		<SyncConnection>
			<NetworkSocket Route="LineIn" Bandwidth="4" RouteId="0x1003" IsActive="false" />
			<StreamSocket StreamPinID="SRXA1" Bandwidth="4" />
		</SyncConnection>

		<SyncConnection>
			<StreamSocket StreamPinID="SRXA0" Bandwidth="4" />
			<NetworkSocket Route="LineIn" Bandwidth="4" />
		</SyncConnection>
	</Node>
```

In the example above three sources are available:
 - 0x1001: The  Head Unit will be routed on the auxiliary boards headphone jacket, by default actived.
 - 0x1002: The  microphone will be routed on the auxiliary boards headphone jacket, by default deactivated.
 - 0x1003: The auxiliary boards "Line In"-jacket will be routed to its own headphone jacket (Loopback (see 7)), by default deactivated.

Once the XML is prepared, the source code of the UNICENS daemon shall be modified.
A solution would be this C-code snippet (can be put everywhere in the project):

```C
#include "ucsi_api.h"
extern UCSI_Data_t *unicens;
static uint16_t currentRoute = 0x1001;

void SwitchRoute(uint16_t newRoute)
{
    /* Deactivate current route */
    UCSI_SetRouteActive(unicens, currentRoute, false);

    /* Activate route given as parameter */
    currentRoute = newRoute;
    UCSI_SetRouteActive(unicens, currentRoute, true);
}
```

Error cases can be handled by inspecting the callback "UCSI_CB_OnRouteResult".

**9.) Working with Ports**

So far only sockets where used. They also configured the ports of the INIC. But the attributes used there, configured only the specific parameters for that connection. There are more parameters, which are shared for all connections using a port. Those parameters can be stored in port tags in the XML file or saved persistent into the INIC Configuration String (Flash / OTP) memory. Those parameters are mandatory, not configuring them in the XML nor configuring them in the Configuration String will lead to a lot of run time errors and may leave the entire setup unusable.
Port tags are defined as a child of a \<Node> tag.
This are the possible port types:

| XML Tag        | Mandatory Attributes                                                         | Usage                      |
|----------------|------------------------------------------------------------------------------|----------------------------|
| \<USBPort>     | DeviceInterfaces, PhysicalLayer, StreamingIfEpOutCount, StreamingIfEpInCount | Universal Serial Bus (USB) |
| \<MediaLBPort> | ClockConfig                                                                  | Media Local Bus (MLB)      |
| \<StreamPort>  | DataAlignment, ClockConfig                                                   | I2S/TDM/PDM port           |

**9.1) Defining an USB port**

Following four Attributes are mandatory to define a valid USB port:

 - DeviceInterfaces=".."
	 - This value is a bit mask. 
	 - Each bit has the meaning: 0=Deactive; 1=Activate

| Bit # | Description                                      |
|-------|--------------------------------------------------|
| 0     | EnableControlIf (control interface activate)     |
| 1     | EnablePacketIf (packet interface activate)       |
| 2     | EnableIpcPacketIf (IPC packet interface activate)|
| 3     | EnableStreamingIf (streaming interface activate) |

 - PhysicalLayer=".."
	 - For this attribute only two values are valid.
		 - "Standard" will configure the USB port to be used external as norm USB device
		 - "HSIC" will configure the USB port to be used for PCB connections only. This reduces cost, when the INIC is on the same PCB as the CPU, because the analog front end is less complex.
 - StreamingIfEpOutCount=".."
	 - The amount of streaming channels going out of the INIC (RX for CPU), starting with 0x81. The maximum number is 5.
 - StreamingIfEpInCount=".."
	- The amount of streaming channels going into the INIC (TX for CPU), starting with 0x01. The maximum number is 5.

**9.2) Defining a MediaLB port**

This tag has only one attribute:

 - ClockConfig=".."
	 - The value is a multiple of the network frame rate Fs  (48kHz); this means the
MediaLB port can only be frequency locked to the network’s system clock.
	 - This is an enumeration.
	 - Refer 5.2 to see the meaning.
	 - Those values are allowed (Case Sensitive):
		 - 256Fs
		 - 512Fs
		 - 1024Fs
		 - 2048Fs
		 - 3072Fs
		 - 4096Fs
		 - 6114Fs
		 - 8192Fs

**9.3) Defining a Streaming port**

Following two Attributes are mandatory to define a valid streaming port:
 - DataAlignment=".."
	 - Defines the alignment of the data bytes within the Streaming Port frame.
	 - This is an enumeration.
	 - Refer 5.3 to see the meaning.
	 - Those values are allowed (Case Sensitive):
		 - Left16Bit
		 - Right16Bit
		 - TDM16Bit
		 - Left24Bit
		 - Right24it
		 - TDM24Bit
		 - Seq
 - ClockConfig=".."
	 - The value is a multiple of the network frame rate Fs (48kHz); this means the
streaming port can only be frequency locked to the network’s system clock.
 	 - This is an enumeration.
	 - Refer 5.3 to see the meaning.
	 - Those values are allowed (Case Sensitive):
		 - 64Fs
		 - 128Fs
		 - 256Fs
		 - 512Fs

**10.) Working with Scripts**

The INIC on Slim and Smart nodes can remote control peripheral like audio codecs, camera sensors, INIC companions, port expander, LEDs and Buttons. Therefor it provides an I2C master interface and GPIO pins, which are controllable via network.
UNICENS provides the capability to execute a list of jobs, when a device first time enters the network.  This list is called script.
Those scripts can be assigned for each node in the XML file. Therefor the \<Node> tag has an optional attribute called Script:
 - Script=".."
	 - The value is any sort of user defined name, there is no syntax to be followed (other than to be XML complaint).
	 - This script name acts as a reference. 

The content of the script is then embedded in a tag called \<Script>, which is a child of \<Unicens>, the same hierarchy level as \<Node>
The \<Script> tag has only mandatory attribute called "Name":
 - Name=".."
	 - This is the counter part of the Script attribute of the <\Node> tag
	 - If the name of the names are identical, there are linked and the script will be executed once the device is found at network startup.

A **non** working example (because of missing parameters) would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x240" Script="Script_AUX_IO"/>
	<Script Name="Script_AUX_IO">
		<!-- Job 1 goes here ->
		<!-- Job 2 goes here ->
	</Script>
</Unicens>
```

In the example above, the node with the address 0x240 references to the script named "Script_AUX_IO". 
In the next line the script is getting declared, with exact the same name (Case sensitive!). 
The jobs will then declared as childs of the \<Script> tag. UNICENS will execute the jobs in the same order as they appear in the XML file.

These are the possible jobs:

| XML Tag            | Mandatory Attributes                                | Optional Attributes               | Usage                      |
|--------------------|-----------------------------------------------------|-----------------------------------|----------------------------|
| \<I2CPortCreate>   | Speed                                               |                                   | Creating Remote I2C port   |
| \<I2CPortWrite>    | Address, Payload                                    | Mode, BlockCount, Length, Timeout | Writing to I2C             |
| \<I2CPortRead>     | Length, Address                                     | Timeout                           | Reading from I2C           |
| \<GPIOPortCreate>  | DebounceTime                                        |                                   | Creating Remote GPIO port  |
| \<GPIOPortPinMode> | PinConfiguration                                    |                                   | Configuring GPIO port      |
| \<GPIOPinState>    | Data, Mask                                          |                                   | Toggling GPIOs             |
| \<MsgSend>         | FBlockId, FunctionId, OpTypeRequest, PayloadRequest | OpTypeResponse, PayloadResponse   | Generic INIC command API   |

**10.1) Defining a I2C port create job**
In order to enable the usage of remote I2C, the ports need to be created first with the \<I2CPortCreate> tag. It only has one mandatory attribute called Speed:

 - Speed=".."
	 - This is an enumeration.
	 - Those values are allowed (Case Sensitive):

| Value    | Meaning                           |
|----------|-----------------------------------|
| SlowMode | Port SCL clock operates at 100kHz |
| FastMode | Port SCL clock operates at 400kHz |

Example, how to create I2C port with 400kHz:
```xml
<I2CPortCreate Speed="FastMode"/>
```

**10.2) Defining a I2C write job**

In order to use this job, make sure that the I2C port has already been created by the  \<I2CPortWrite> tag.
With this job a single or multiple I2C write commands can be sent.
When only the two mandatory attributes are given, then a single message is sent:

- Address=".."
	- The I2C slave address.
	- The lowest Bit (Read/Write) is not part of this address (so shift right by one Bit).
	- If addressing in hexadecimal notation is intended, add leading 0x before the value. Otherwise it will be interpreted in decimal notation. 
 - Payload=".."
	 - Hexadecimal array of Bytes.
	 - Bytes will be sent to the remote I2C slave, in the same order as written in this string.
	 - The notation is without leading 0x.
	 - Each Byte is represented by two digits [0..F].
	 - Each Byte (except the last one) is separated by a trailing space.


Example, how to write 5 Bytes to I2C slave with address 0x20:
```xml
<I2CPortWrite Payload="A1 B2 C3 D4 E5" Address="0x20"/>
```

In order to boost up the overall sending speed of I2C, multiple I2C write commands can be grouped into a single job. Therefor the following three optional attributes are used:

 - Mode=".."
	 - This is an enumeration.
	 - Those values are allowed (Case Sensitive):

| Value             | Meaning                                                                                                   |
|-------------------|-----------------------------------------------------------------------------------------------------------|
| DefaultMode       | No optimization used (Default). After transaction a STOP condition is issued and the bus is released      |                                                                           |
| BurstMode         | After transaction the STOP condition will be suppressed and further read or write sequences can be issued |
| RepeatedStartMode | Enables writing multiple blocks of bytes of the same size                                                 |

 - BlockCount=".."
	 - Specifies the number of blocks to be written to the I2C address.
	 - If parameter Mode is not set to BurstMode, the value of BlockCount has to be set to 0 (default).
	 -  Otherwise the valid range for this parameter is from 1 to 30.

 - Length
	 - Number of bytes to be written to the I2C address. 
	 - If parameter Mode is set to BurstMode, the valid range of this parameter goes from 1 to 30
	 - For all other modes, the length is automatically taken from the Byte array length given with the Payload attribute.

Example, how to write 5 blocks with each 3 Bytes to I2C slave with address 0x10:
```xml
<I2CPortWrite Mode="BurstMode" BlockCount="5" Length="3" Address="0x18"
              Payload="10 50 50 11 00 00 12 00 00 13 00 00 14 00 00" />
```
With the example above, the following I2C write commands will be issued to slave address 0x18:

 -  0x10, 0x50, 0x50, STOP
 -  0x11, 0x00, 0x00, STOP
 -  0x12, 0x00, 0x00, STOP
 -  0x13, 0x00, 0x00, STOP
 -  0x14, 0x00, 0x00, STOP

Another attribute is the Timeout:

 - Timeout=".."
	 - Time in milliseconds.
	 - If not set, 1000 ms is used as default.
	 - Reduce this value, if an I2C device is optional and the system does not want to wait for it to appear.

**10.3) Defining a I2C read job**

In order to use this job, make sure that the I2C port has already been created by the  \<I2CPortRead> tag.
With this job a single I2C read commands can be triggered.
This are the two mandatory attributes:

 - Length=".."
	 - The amount of Bytes to read.
- Address=".."
	- The I2C slave address.
	- The lowest Bit (Read/Write) is not part of this address (so shift right by one Bit).
	- If addressing in hexadecimal notation is intended, add leading 0x before the value. Otherwise it will be interpreted in decimal notation. 

This is an optional attribute:
 - Timeout=".."
	 - Time in milliseconds.
	 - If not set, 1000 ms is used as default.
	 - Reduce this value, if an I2C device is optional and the system does not want to wait for it to appear.

 Example, how to read 8 Bytes to I2C slave with address 0x10:
```xml
<I2CPortRead Length="8" Address="0x10"/>
```

To get the result and use the received data to trigger further action the code of UNICENS daemon needs to be adjusted. Inspect the callback function "UCSI_CB_OnI2CRead" for this purpose:

```C
void UCSI_CB_OnI2CRead(void *pTag, bool success, uint16_t inicNetNodeAddress, uint8_t i2cSlaveAddr, const uint8_t *pBuffer, uint32_t bufLen)
{ }
```
