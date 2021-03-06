# UNICENS XML Description

This file describes how to write a valid UNICENS XML configuration file.

**1.) XML Basics**

Note that the rules of XML enforce that the document is well formed.
This means it is zero tolerant, a single typo will invalidate the whole document and any application using it must not continue.
Therefore, it is useful in any case to use an IDE to check the document.
This can be the UNICENS<sup>TM</sup> System Designer, which covers a deep rule set related to UNICENS, INIC and drivers.
But any other IDE will work too, however, with less functionality (Atmel Studio, MPLAB-X, Eclipse, NetBeans, Visual Studio).

**The XML provides the following terminology: *(partly from Wikipedia)***

**Tag**

A _tag_ is a construct that begins with `<` and ends with `>`.

Tags come in three flavors:

-   _start-tag_, such as  `<section>`
-   _end-tag_, such as  `</section>`
-   _empty-element tag_, such as  `<line-break />`

Tags can be stored inside other tags. This will form a tree architecture.

Example:
```xml
<Parent>
	<Child/>
	<Child>
		<GrandChild/>
	</Child>
</Parent>
```
The tree view looks as follows:
```
Parent
¦
+---Child
¦
+---Child
    ¦
    +--- GrandChild
```

**Element**

An _element_ is a construct that begins with a start-tag and ends with a matching end-tag.

An example is `<greeting>Hello, world!</greeting>`

> NOTE: Currently the UNICENS schema is not using elements.

**Attribute**

An _attribute_ is a construct consisting of a nameâ€“value pair that exists within a start-tag or empty-element tag.

An example is `<img src="madonna.jpg" alt="Madonna" />`, where the names of the attributes are "src" and "alt",
and their values are "madonna.jpg" and "Madonna" respectively.

**XML declaration**

XML documents may begin with an  _XML declaration_  which specifies the used version and encoding.

An example is  `<?xml version="1.0" encoding="UTF-8"?>`

**Schema**

In addition to being well-formed, an XML document may reference to an external rule set.
There are certain rules applicable, such as the name of tags, elements and attributes.
Also, the values of the attributes can be specified as regular expressions.
Such a file comes along with the UNICENS configuration examples, it is called *unicens.xsd*.
The linkage between the document and its Schema is done in the first tag (Root Tag):
 `<Unicens xsi:noNamespaceSchemaLocation="unicens.xsd">`

**Comments**

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
**2.1) Bandwidth Calculation**

The attribute _AsyncBandwidth_ in the example above is mandatory.
It specifies the bandwidth of the asynchronous channel on INICnet<sup>TM</sup> technology 50utp network which can be used for e.g. Ethernet communication.
The value specified is in Bytes within 48 kHz. So, in the example above, this would mean

> 80 Byte \* 48000 1/s = 3.84 MByte/s

> 3.84 MByte/s \* 8 Bit/Byte = 30.72 MBit/s

Note the total network bandwidth limits for both network speed grades:

| Speed grade | Bytes Within 48 kHz    |
|-------------|------------------------|
| 50 Mbit/s   | 116 bytes              |
| 150 Mbit/s  | 372 bytes              |

This means for the example above, that the remaining bandwidth for audio/ video streaming on an INICnet technology 50utp network is 36 bytes (116 bytes - 80 bytes).

Setting the _AsyncBandwidth_ to 0 is allowed. In this case no Ethernet communication is possible, and all bandwidth is available for audio and video streaming.
Setting  the _AsyncBandwidth_ to the maximum possible value (here 116 bytes or 372 bytes) is allowed. In this case the asynchronous channel (also called Packet Channel of the INICnet) is running with the full speed, but no streaming is possible.

Besides of using the asynchronous channel, there are dedicated streaming channels to transport audio and video data.
They also use the same metric "bytes within 48 kHz" as their bandwidth configuration.

The table below shows different use-cases for audio data transmission and the corresponding required data rate.

| Use Case                  | Data Type    | Bytes Within 48 kHz | Bandwidth in Mbit/s |
|---------------------------|--------------|---------------------|---------------------|
| Mono 16 bit               | Sync         | 2                   | 0.768               |
| Mono 24 bit               | Sync         | 3                   | 1.152               |
| Stereo 16 bit             | Sync         | 4                   | 1.536               |
| Stereo 24 bit             | Sync         | 6                   | 2.304               |
| 5.1 channels 16 bit       | Sync         | 12                  | 4.608               |
| 5.1 channels 24 bit       | Sync         | 18                  | 6.912               |
| 7.1 channels 16 bit       | Sync         | 16                  | 6.144               |
| 7.1 channels 24 bit       | Sync         | 24                  | 9.216               |
| Low Quality H264 Video    | AVPacketized | 8                   | 3.072               |
| Superb Quality H264 Video | AVPacketized | 80                  | 30.72               |

**3.) Defining the Nodes**

A node is a device which participates on the network.
There are three categories a node can belong to:

 1. **Root Node** - It runs the central UNICENS stack. There can only be one device in the network with this role.
 2. **Slim Node** - Without any CPU on it, such as a microphone or booster. It is completely remote controlled. Any GPIO or I2C peripheral can be remotely triggered from the Root Node via the network.
 3. **Smart Node** - With a CPU or micro-controller. It is also completely remote controlled and has GPIO and I2C remote access like the Slim Node. But there are two additional ways to communicate (peer to peer): via the control channel and/or via the asynchronous  channel of the INICnet.

Slim Nodes and Smart Nodes may be instanced multiple times in the network.
The theoretical limit of nodes per network is 64. Due to power- and timing reasons, the actual value can be lower than that.

Each device needs to have a _Node Address_ which only exists once in that network (not globally unique like a MAC address).
This node address is specified by the system integrator and needs to be flashed into the Flash- or OTP memory of the INIC chip.

A Node Address is a 16-bit address, which is usually written in a hexadecimal representation.
Due to historical reasons the range of Node Addresses is limited to the following sections:

| Start |  End  |
|-------|-------|
| 0x10  | 0xFF  |
| 0x140 | 0x2FF |
| 0x500 | 0xEFF |

This table can be used as an example starting point for a system integrator:

| Address Range   | Device Type              | Instance Numbers  |
|-----------------|--------------------------|-------------------|
| 0x200           | UNICENS Root Node      | 1                 |
| 0x201           | Smart Antenna            | 1                 |
| 0x202           | Digital Signal Processor | 1                 |
| 0x205 ... 0x209 | Cluster                  | 1 ... 5           |
| 0x210 ... 0x23F | Microphone               | 1 ... 64          |
| 0x240 ... 0x26F | AUX IO Board Instance    | 1 ... 64          |
| 0x270 ... 0x29F | Slim Amplifier Instance  | 1 ... 64          |
| 0x2B0 ... 0x2DF | Entertainment System     | 1 ... 64          |
| 0x2E0 ... 0x2EF | Camera Instance          | 1 ... 16          |

Specifying a node is done with the `<Node>` tag. All Node tags are children of the `<Unicens>` tag.
It's important to know that all devices need to be specified in the document.
If a device with an unknown Node address tries to enter the network, it will be ignored.

Here is an example, specifying two devices in the network: UNICENS Root Node and a smart entertainment system:

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200"/>
	<Node Address="0x2B0"/>
</Unicens>
```
With this example, the mentioned devices are allowed to join the network.
As there is no additional information given, the devices can communicate via the Control
and/or the asynchronous channel of INICnet, but at this stage there are no dedicated channels for audio or video transmission. 

**4.)  Connections**

In order to get the major benefits from INICnet technology, audio and video connections can be defined by the system integrator.
These connections use reserved network bandwidth, which is guaranteed for that particular use-case (Quality of Service or QoS)

**4.1) Defining Synchronous Streams for Audio Use Cases**

For transportation of uncompressed audio data, the synchronous data channel of the network is optimal.
It always streams with a constant data rate, which is by design synchronous on every device.
This means, there are no offset, no drift and no jitter problems to deal with.
To define a synchronous connection, the `<SyncConnection>` tag is used.
It is a child of the `<Node>` tag described in Chapter 3. Every node may have more than one synchronous connection.

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

**4.2) Defining Isochronous Streams for Compressed Audio and Video Use Cases**

In contrast to the synchronous data channel, the isochronous channel is capable to transfer data at a variable rate.
The user specifies a worst case (or burst) data rate, which then is allocated on the network.
When the used compression algorithm (e.g. H264 and/or MP3) produces less or no bandwidth, the channel utilization becomes also less.

The isochronous channel uses MPEG transport stream as its data format.
Therefore, a single data chunk is always a multiple of 188 bytes if the data is unencrypted.
For encrypted data, each data chunk is always a multiple of 192 bytes.
Refer to [MPEG transport stream](https://en.wikipedia.org/wiki/MPEG_transport_stream) for more information.

To define an isochronous connection, the `<AVPConnection>` tag is used (AVP: Audio Video Packetized).
It is a child of the `<Node>` tag described in Chapter 3.
Every node may have more than one isochronous connection.
There is only one mandatory attribute for the `<AVPConnection>` called _IsocPacketSize_.

 - IsocPacketSize=".."
	 - Enumeration of three integers, choose one:
		 - *188* for unencrypted transport streams
		 - *196* for encrypted transport streams

A **non** working example (because of missing tags) would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<AVPConnection IsocPacketSize="188">
		</AVPConnection>
	</Node>
	<Node Address="0x2B0">
		<AVPConnection IsocPacketSize="188">
		</AVPConnection>
	</Node>
</Unicens>
```

**5.) Working with Sockets**

A socket represents a data path going either into the INIC or out of the INIC. It references the used INIC port and specifies how the data shall be formatted on that port.
The following table lists the possible socket types:

| XML Tag           | Mandatory Attributes                  | Usage                      |
|-------------------|---------------------------------------|----------------------------|
| `<USBSocket>`     | EndpointAddress, FramesPerTransaction | Universal Serial Bus (USB) |
| `<MediaLBSocket>` | ChannelAddress, Bandwidth             | Media Local Bus (MLB)      |
| `<StreamSocket>`  | StreamPinID, Bandwidth                | I2S/TDM/PDM Port           |
| `<NetworkSocket>` | Bandwidth, Route                      | Network channel            |

Always two sockets exist inside a connection (applies to `<SyncConnection>` and `<AVPConnection>`).
The order of the socket tag inside the connection tag is crucial.
The first socket is the input socket, whereas the second one is the output socket. The directions are always set from the INIC's point of view.
In any case, the `<NetworkSocket>` tag must be either the first or the second entry.
Additionally, the input and output sockets which form a connection must be of different type.

A **non** working example (because of missing parameters) of routing isochronous data from the USB Port to the network channel would be:
```xml
...
	<Node Address="0x200">
		<AVPConnection>
			<!-- Input (INIC view) is USB: -->
			<USBSocket/>

			<!-- Output (INIC view) is isochronous channel on Network: -->
			<NetworkSocket/>
		</AVPConnection>
	</Node>
...
```
A **non** working example (because of missing parameters) of routing synchronous data from the network channel to a Streaming Port (I2S):
```xml
...
	<Node Address="0x200">
		<SyncConnection>
			<!-- Input (INIC view) is synchronous channel on Network: -->
			<NetworkSocket/>

			<!-- Output (INIC view) is Streaming Port (I2S): -->
			<StreamSocket/>
		</SyncConnection>
	</Node>
...
```

**5.1) Defining a USB Socket**

The following two attributes are mandatory to define a valid USB socket:

 - EndpointAddress=".."
	 - This is the USB endpoint address. It's a byte value usually written in a hexadecimal notation. If the data is transmitted out of the INIC, received by the main CPU (EHC), the highest bit is set, which leads to values starting with 0x80. For data which is received by the INIC, sent by the EHC, no additional bits are set, so the starting address is 0x00.
	 - Endpoint 0x00 is reserved for administrative purposes on USB.
	 - For streaming data, currently a maximum of 5 endpoints is supported. Hence, USB endpoint addresses in the rage of 0x01 ... 0x05 (EHC TX) and 0x81 ... 0x85 (EHC RX) may be used.
	 - Control data is transmitted on endpoints for OS81118/9: 0x0F & 0x8F; for OS81210: 0x07 & 0x87 (must not be used inside SyncConnection or AVPConnection)
 	 - Asynchronous data, e.g. Ethernet packets, is transmitted on endpoints for OS81118/9: 0x0E & 0x8E; for OS81210: 0x06 & 0x86 (must not be used inside SyncConnection or AVPConnection)
- FramesPerTransaction="..."
	- A micro frame on USB is a chunk of 512 bytes. This is the maximum transmission unit (MTU) for the used bulk transfer mode on USB. In order to get a very efficient streaming behavior, the INIC always fills a micro frame completely, so there is no additional signaling needed. Depending on the use case, waiting for 512 bytes of data (for example audio data) to arrive and then start the transmission after that can cause a measurable latency. For some use-cases this latency is unwanted (e.g. noise cancellation).
	- To achieve low latency, the integrator can choose to not fill the micro frame entirely with data. This means that the transmission is started earlier, with less than 512 bytes of valid streaming data. The software driver (for EHC TX) or the INIC hardware (for EHC RX) automatically appends invalid stuffing data to fill the USB micro frame and keep the signaling overhead as small as possible. The invalid stuffing data is never transported on the network, so no bandwidth on the network is wasted.
	-  The `FramesPerTransaction` value describes how many samples (PCM, PDM or TS-Packets) are put into one USB micro frame.
	- For instance, if there is a 16-bit Stereo PCM audio stream (2 \* 2 bytes = 4 bytes) to be transported, the maximum possible value for FramesPerTransaction would be 128 (128 \* 4 bytes = 512 bytes). Setting a value of 64 in that particular case would leave the micro frame half filled, improving the latency and downgrading the efficiency of USB.
	- For synchronous sockets, the **minimum** value for FramesPerTransaction is **7**. This is due to the different timing of USB and network.
	- For isochronous sockets, there are only two possibilities: FramesPerTransaction set to 2 - in this case two transport stream packets (2 \* 188/192 bytes) will be stored in one USB micro frame. The second option is to set the FramesPerTransaction value to 0xFF. In this case, the USB micro frame is always completely filled. Since 512 bytes of the micro frame are not dividable by 188/192 bytes, the fractional rest of the streaming data is put into the next micro frame. This means, that on the receiver side the integrator cannot rely any longer on the fact that the first received byte of a micro frame will be the first byte of the transport stream packet. In that case integrator needs to search for 0x47 inside the payload of the stream, which marks the start of a transport stream packet.
	- In contrast to other sockets, the USB socket bandwidth must not be specified. It automatically adjusts its speed to the corresponding network socket.

**5.2) Defining a MediaLB<sup>&reg;</sup> Socket**

The Media Local Bus (MLB) is a dedicated bus for interfacing the INIC and I/O Companion chips.
It is adopted by many vendor products, such as Atmel SAM V71, NXP i.MX6, Renesas RCAR H3/M3.
MediaLB can provide very low latency use cases (lower than USB).
Of course, the operating system needs to have a scheduler which is fast enough to deal with the increasing number of interrupts.

The following two attributes are mandatory to define a valid MediaLB socket `<MediaLBSocket>`:
 - ChannelAddress=".."
	 - Integer value between 10 ... 64.
	 - Value must be even.
	 - Channel address 0 is unused.
	 - Channel addresses 2 & 4 are reserved for the control channel.
	 - Channel addresses 6 & 8 are reserved for the asynchronous channel.
 - Bandwidth="..."
	 - The number of bytes transferred within 48 kHz (See 2.1).
	 - The MediaLB Port can be configured with different speed rates (See 9.2). Depending on the speed chosen for the port, the bandwidth of a socket must be within a certain range:

| MediaLB Speed (Fs) | MediaLB Type | Bandwidth Range (Byte) |
|--------------------|--------------|------------------------|
|  256               | 3 pin        |  1 ... 28              |
|  512               | 3 pin        |  1 ... 60              |
|  1024              | 3 pin        |  1 ... 124             |
|  2048              | 6 pin        |  1 ... 228             |
|  3072              | 6 pin        |  1 ... 344             |
|  4096              | 6 pin        |  1 ... 372             |

**5.3) Defining a Stream Socket**

The Streaming Port addressed with a Stream socket is most known as I2S Port.
But it can also support multi-channel TDM and PDM data types.
Following two Attributes are mandatory to define a valid Stream socket `<StreamSocket>`:
- StreamPinID=".."
	- One of the hardware pins of the INIC's Streaming port, transporting the data part of the stream. 
	  Note: The clock signals of INIC's Streaming ports, FSY and CLK, are shared among all Streaming data pins.
	- Following enumeration represents the allowed values. Choose one out of it: *SRXA0*, *SRXA1*, *SRXB0*, *SRXB1*
 - Bandwidth=".."
	 - The number of bytes transferred within 48 kHz (See 2.1).
	 - The Streaming Port can be configured with different speed rates and different data formats (See 9.3). Depending on the chosen speed and format for the Streaming Port, the bandwidth of a Stream socket must be within a certain range:

| Streaming Port Speed (Fs) | 16-bit Bandwidth Range | 24-bit Bandwidth Range  | Sequential Bandwidth Range  |
|---------------------------|------------------------|-------------------------|-----------------------------|
|  64                       |  1 ... 4               |  1 ... 6                |  1 ... 8                    |
|  128                      |  1 ... 8               |  1 ... 12               |  1 ... 16                   |
|  256                      |  1 ... 16              |  1 ... 24               |  1 ... 32                   |
|  512                      |  1 ... 32              |  1 ... 48               |  1 ... 64                   |

**5.4) Defining a Network Socket**

The Network socket describes the resources allocated on the networkÂ´s streaming channel.
As mentioned earlier, in every connection (SyncConnection, AVPConnection), there must be exactly one Network socket defined, either as input or as output.
Following two attributes are mandatory to define a valid Network socket `<NetworkSocket>`:
 - Bandwidth=".."
	 - The number of bytes transferred within 48 kHz (See 2.1).
	 - This value should match to the opposite socket bandwidth (except for USB).
	 - For isochronous streaming on MediaLB, the socket bandwidth of the Network socket and the MediaLB socket may be different.
 - Route=".."
	 - The value may be any kind of user defined name; there is no syntax to be followed (other than to be XML complaint).
	 - This route name acts as a reference.
	 - If two connections in the network share the same route name, they will be automatically connected.
	 - The integrator must make sure that each source Network socket is connected to at least one sink network socket.

Here is an example, routing audio from a microphone to a head unit and to an additional node:

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

In the example above, the node 0x210 is acting as an audio source, because the `<NetworkSocket>` is the second entry in the `<SyncConnection>`.
It defines the route name "Route_Microphone". As mentioned earlier, any name would be valid here.
You will find exactly the same name (case- and space sensitive) in node 0x200 and 0x2B0.
This time `<NetworkSocket>` is the first entry within `<SyncConnection>`, meaning those connections each constitute an audio sink.

Both devices (node 0x200 and node 0x2B0) are receiving the audio data from the microphone in parallel and with the same latency and phase.
The node 0x200 is (internally) streaming to its USB endpoint address 0x81 and the node 0x2B0 to its MediaLB channel address 0x10.

**6.) Working with Combiner and Splitter**

For audio streaming use cases on synchronous channels, it can be very helpful to group or separate channels.

**6.1) Defining a Combiner**

The Combiner works for synchronous connections only and has the ability to join multiple audio streams from the network to one time-division multiplex (TDM) data stream. This newly generated stream can be routed out to a USB, MediaLB or Streaming Port. For instance, to combine three stereo microphones from the network into a single six-channel TDM stream on USB.

Doing so has multiple benefits:

 - It safes resources (USB endpoints, MediaLB channels or Streaming Port pins).
 - It keeps multiple channels in the correct phase relation. Especially important for beam forming and noise canceling applications.
 - It helps to improve the efficiency of the sink port in low latency use cases. Especially the USB bus becomes inefficient if it must transport very small data chunks (See 5.1, FramesPerTransaction attribute).

A Combiner resides inside a `<SyncConnection>` as first entry, meaning it is an input to the INIC. It has one mandatory attribute:
 - BytesPerFrame="..."
	 - The combined number of bytes within 48 kHz for all streams together.
	 - For example, combining three stereo microphones would need a value of 6.
	 - This value is the same for the corresponding Bandwidth attribute of the opposite `<MediaLBSocket>` or `<StreamSocket>` tag.
	 - This value needs to be considered also in the calculation for the FramesPerTransaction attribute if the opposite socket is a `<USBSocket>`

The children of the Combiner are then multiple `<NetworkSocket>` tags with additional mandatory attributes:
 - Offset=".."
	 - This attribute declares the start position inside of the combined TDM stream for that particular `<NetworkSocket>`.
	 - A value of 0 means that it shall be routed to the start of the TDM stream.
	 - This value must be smaller than the attribute BytesPerFrame of the `<SyncConnection>` tag.
 - Bandwidth
	 - This attribute was already marked as mandatory in (See 5.4). But in the Combiner context it is also used to indirectly define the end position inside of the combined TDM stream:
*End = Bandwidth + Offset*

Having this information, each `<NetworkSocket>` forms a block inside the TDM stream. The integrator must ensure that the blocks are not overlapping each other.

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

The Splitter works for synchronous connections only and is capable to cut multi-channel audio TDM streams, coming from the application,  into one or multiple audio streams with smaller bandwidth and stream them to the network.

Doing so has one benefit:

 - It helps to arrange the audio channels in different orders.
	- For instance, the integrator can choose to have only mono channels on the network using a Splitter. On the sink side the integrator can then combine various mono channels (also from different source devices). For that particular use (see section 6.1).

A Splitter resides inside a `<SyncConnection>` as second entry, meaning it is an output of the INIC. It has one mandatory attribute:
 - BytesPerFrame="..."
	 - The number of bytes within 48 kHz of the source stream.
	 - This value is the same as the corresponding Bandwidth attribute of the opposite `<MediaLBSocket>` or `<StreamSocket>` tag.
	 - This value needs to be considered also in the calculation for the FramesPerTransaction attribute if the opposite socket is a `<USBSocket>`.

The children of the Splitter are then multiple `<NetworkSocket>` tags with additional mandatory attributes:
 - Offset=".."
	 - This attribute declares the cutting start position inside of the source TDM stream for that particular `<NetworkSocket>`.
	 - A value of 0 means that it shall be cut at the beginning of the TDM stream.
	 - This value must be smaller than the attribute BytesPerFrame of the `<SyncConnection>` tag.
 - Bandwidth
	 - This attribute was already marked as mandatory (See 5.4). But in the Splitter context it is also used to indirectly define the cutting end position inside of the combined TDM stream:
*End = Bandwidth + Offset*

Having this information, each `<NetworkSocket>` forms a block inside the TDM stream. The integrator must ensure that the blocks are not overlapping each other.

Here is an example, cutting a 5.1 multi-channel stream from a head unit into three stereo streams using a Splitter. The sinks are three stereo amplifiers.

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

In certain cases, it may be helpful to route the audio from the source back to itself (looping back). This may be the case when the radio tuner shall not accidentally activate the head unitÂ´s wake word (like "Alexa" or "Hey Siri").
To achieve this, simply add two `<SyncConnection>` to the same Node. Ensure that the Route name of the `<NetworkSocket>` is the same for both.
The target peripheral interface (USB, MediaLB, Streaming Port) may be different for both connections.

An example, routing a SyncConnection back to the same device (0x200):

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
Unfortunately, it is not possible to use a loopback with connections in which a Splitter or a Combiner is used.

**8.) Switching Connections**

It may be very useful to activate/deactivate certain connections (applies to `<SyncConnection>` and `<AVPConnection>`).
Reasons to do so are:
- Save network bandwidth - If all source connections together consume more bandwidth than the network supports, switching off unused streams can free up the needed space.
- Having multiple sources - Connections can only be established, if there is exactly one source available. However, if a source is switched off, another source may be activated instead.
- Safely shut down audio connections to avoid plopping sound.

In order to prepare a connection to be switched, the integrator needs to add two optional attributes to the `<NetworkSocket>` tag (valid for source and sink):
 - RouteId=".."
	 - The RouteId  value must be an unsigned 16-bit value (0x0 ... 0xFFFF). The RouteId value must be unique over the complete XML file. It acts as a handle, which can be used later in the C-code to address this connection.
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
 - 0x1001: The  Head Unit will be routed on the auxiliary boardÂ´s headphone jacket, by default activated.
 - 0x1002: The  microphone will be routed on the auxiliary boardÂ´s headphone jacket, by default deactivated.
 - 0x1003: The auxiliary boards "Line In"-jacket will be routed to its own headphone jacket (Loopback (See 7)), by default deactivated.

Once the XML is prepared, the source code of the UNICENS daemon can be modified.
A solution might be the C-code snippet below (can be put anywhere in the project):

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

**9.) Working With Ports**

So far only sockets were used for configuration of the data routing in the above examples. However, the socket attributes configure specific parameters for only one specific socket/connection. Parameters which are common for all sockets/connections at a specific port of the INIC can be set in so-called port tags in the XML configuration file or saved persistently into the INIC Configuration String (Flash/OTP) memory. Those parameters are mandatory. Not configuring them in the XML or in the Configuration String will lead to a lot of run time errors and may leave the entire setup unusable.
Port tags are defined as a child of a `<Node>` tag.

The following are possible port types:

| XML Tag         | Mandatory Attributes                                                         | Usage                      |
|-----------------|------------------------------------------------------------------------------|----------------------------|
| `<USBPort>`     | DeviceInterfaces, PhysicalLayer, StreamingIfEpOutCount, StreamingIfEpInCount | Universal Serial Bus (USB) |
| `<MediaLBPort>` | ClockConfig                                                                  | Media Local Bus (MLB)      |
| `<StreamPort>`  | DataAlignment, ClockConfig                                                   | I2S/TDM/PDM Port           |

**9.1) Defining a USB Port**

Following four Attributes are mandatory to define a valid USB Port `<USBPort>`:

 - DeviceInterfaces=".."
	 - This value is a bit mask.
	 - Each bit has the meaning: 0 = Not active; 1 = Activate

| Bit # | Description                                       |
|-------|---------------------------------------------------|
| 0     | EnableControlIf (control interface activated)     |
| 1     | EnablePacketIf (packet interface activated)       |
| 2     | EnableIpcPacketIf (IPC packet interface activated)|
| 3     | EnableStreamingIf (streaming interface activated) |

 - PhysicalLayer=".."
	 - For this attribute only two values are valid.
		 - "Standard" configures the USB Port to be used as external standard USB device
		 - "HSIC" configures the USB Port to be used for internal PCB connections only. This reduces cost, when the INIC is on the same PCB as the CPU, because the analog front end is less complex.
 - StreamingIfEpOutCount=".."
	 - The number of streaming channels going out of the INIC (RX for CPU), starting with 0x81. The maximum number is 5.
 - StreamingIfEpInCount=".."
	- The number of streaming channels going into the INIC (TX for CPU), starting with 0x01. The maximum number is 5.

**9.2) Defining a MediaLB Port**

The `<MediaLBPort>` tag has only one attribute:

 - ClockConfig="..."
	 - The value is a multiple of the network frame rate Fs (48 kHz); this means that the MediaLB port is always frequency locked to the networkâ€™s system clock.
	 - This is an enumeration.
	 - Refer to section 5.2 to see the meaning.
	 - These are the allowed values (case and space sensitive):
		 - *256Fs*
		 - *512Fs*
		 - *1024Fs*
		 - *2048Fs*
		 - *3072Fs*
		 - *4096Fs*
		 - *6114Fs*
		 - *8192Fs*

**9.3) Defining a Streaming Port**

Following two attributes are mandatory to define a valid Streaming Port `<StreamPort>`:
 - DataAlignment=".."
	 - Defines the alignment of the data bytes within the Streaming Port frame.
	 - This is an enumeration.
	 - Refer to section 5.3 to see the meaning.
	 - These are the allowed values (case- and space sensitive):
		 - *Left16Bit*
		 - *Right16Bit*
		 - *TDM16Bit*
		 - *Left24Bit*
		 - *Right24it*
		 - *TDM24Bit*
		 - *Seq*
 - ClockConfig=".."
	 - This value is a multiple of the network frame rate Fs (48kHz); this means the
Streaming Port is always frequency locked to the networkâ€™s system clock.
 	 - This is an enumeration.
	 - Refer to section 5.3 to see the meaning.
	 - These are the allowed values (case and space sensitive):
		 - *64Fs*
		 - *128Fs*
		 - *256Fs*
		 - *512Fs*

**10.) Working with Scripts**

The INIC on a Slim- or a Smart Node can control its local peripherals including audio codecs, camera sensors, INIC I/O companions, port expander, LEDs and Buttons as a proxy for the Root Node. Therefore, it provides an I2C master interface and GPIO pins, which are controllable via network.
UNICENS provides the capability to execute a list of jobs when a device connects to the network the first time.
Those scripts can be assigned for each node in the XML file. Therefore, the `<Node>` tag has an optional attribute called _Script_:
 - Script=".."
	 - This value is any sort of user-defined name, there is no syntax to be followed (other than to be XML complaint).
	 - This script name acts as a reference.

The content of the script is then embedded in a tag called `<Script>`, which is a child of `<Unicens>` and has the same hierarchy level as `<Node>`.

The `<Script>` tag has only one mandatory attribute called _Name_:
 - Name=".."
	 - This is the counter part of the Script attribute of the `<Node>` tag
	 - If the names of the attributes are identical, they are linked, and the script will be executed, once the device is found at network startup.

A **non** working example (because of missing parameters) would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x240" Script="Script_AUX_IO"/>
	<Script Name="Script_AUX_IO">
		<!-- Job 1 goes here -->
		<!-- Job 2 goes here -->
	</Script>
</Unicens>
```

In the example above, the node with the address 0x240 references to the Script named "Script_AUX_IO".
In the next line the Script is getting declared having exactly the same name (case- and space sensitive).
The jobs will then be declared as children of the `<Script>` tag. UNICENS executes the jobs in the same order as they appear in the XML file.

These are the possible jobs:

| XML Tag             | Mandatory Attributes | Optional Attributes               | Usage                      |
|---------------------|----------------------|-----------------------------------|----------------------------|
| `<I2CPortCreate>`   | Speed                |                                   | Creating Remote I2C Port   |
| `<I2CPortWrite>`    | Address, Payload     | Mode, BlockCount, Length, Timeout | Writing to I2C             |
| `<I2CPortRead>`     | Length, Address      | Timeout                           | Reading from I2C           |
| `<GPIOPortCreate>`  | DebounceTime         |                                   | Creating Remote GPIO Port  |
| `<GPIOPortPinMode>` | PinConfiguration     |                                   | Configuring GPIO Port      |
| `<GPIOPinState>`    | Data, Mask           |                                   | Toggling GPIOs             |

**10.1) Defining an I2C Port Create Job**

In order to enable the usage of remote I2C in a remote node, the I2C port needs to be created first with the `<I2CPortCreate>` tag. It has only one mandatory attribute called _Speed_:

 - Speed=".."
	 - This is an enumeration.
	 - These are the allowed values (case- and space sensitive):

| Value      | Mnemonic                           |
|------------|------------------------------------|
| *SlowMode* | Port SCL clock operates at 100 kHz |
| *FastMode* | Port SCL clock operates at 400 kHz |

Example: Create I2C Port with 400 kHz:
```xml
<I2CPortCreate Speed="FastMode"/>
```

**10.2) Defining an I2C Write Job**

In order to use this job, make sure that the I2C Port has been already created by the `<I2CPortCreate>` tag.
The tag to create an I2C write job is named `<I2CPortWrite>`.
With this job, a single- or multiple I2C write commands can be sent across the network to a Remote Node.
If only the two mandatory attributes are given, a single message is sent:

- Address=".."
	- The I2C slave address.
	- The lowest bit (read/write) is not part of this address (so shift right by one bit).
	- If addressing in hexadecimal notation is intended, add leading a `0x` before the value. Otherwise, it will be interpreted in decimal notation.
 - Payload=".."
	 - Bytes will be sent to the remote I2C slave in the same order as written in this string.
	 - Hexadecimal array of bytes
	 - The notation is without leading 0x.
	 - Each byte is represented by two nibbles [0...F].
	 - Each byte (except the last one) is separated by a trailing space.

Example: Write five bytes to an I2C slave with address 0x20:
```xml
<I2CPortWrite Payload="A1 B2 C3 D4 E5" Address="0x20"/>
```

In order to boost up the overall sending speed of I2C, multiple I2C write commands can be grouped into a single job. Therefore, the following three optional attributes are used:

 - Mode=".."
	 - This is an enumeration.
	 - These are the allowed values (case- and space sensitive):

| Value               | Mnemonic                                                                                                    |
|---------------------|-------------------------------------------------------------------------------------------------------------|
| *DefaultMode*       | No optimization used (default). After transaction a STOP condition is issued, and the bus is released.      |                                                                           |
| *RepeatedStartMode* | After transaction the STOP condition will be suppressed and further read or write sequences can be issued.  |
| *BurstMode*         | Enables writing of multiple blocks with the same size.                                                      |                                             |

 - BlockCount="..."
	 - Specifies the number of blocks to be written to the I2C address.
	 - If parameter Mode is not set to BurstMode, the value of BlockCount must be set to 0 (default).
	 -  Otherwise, the valid range for this parameter goes from 1 to 30.

 - Length=".."
	 - Number of bytes to be written within one block.
	 - If parameter Mode is set to BurstMode, the valid range of this parameter goes from 1 to 30.
	 - For all other modes, the length is automatically taken from the byte array length, given with the Payload attribute.

Example: Write five blocks, each with three bytes, to an I2C slave with address 0x10:
```xml
<I2CPortWrite Mode="BurstMode" BlockCount="5" Length="3" Address="0x10"
              Payload="10 50 50 11 00 00 12 00 00 13 00 00 14 00 00" />
```
With the example above, the following I2C write commands will be issued to slave address 0x10:

 -  0x10, 0x50, 0x50, STOP
 -  0x11, 0x00, 0x00, STOP
 -  0x12, 0x00, 0x00, STOP
 -  0x13, 0x00, 0x00, STOP
 -  0x14, 0x00, 0x00, STOP

Another optional attribute is the Timeout:

 - Timeout="..."
	 - Time in milliseconds
	 - If not set, 1000 ms is used as default.
	 - Reduce this value if an I2C device is optional and the system shall not wait for it to appear.

**10.3) Defining an I2C Read Job**

In order to use this job, make sure that the I2C Port has been already created by the `<I2CPortCreate>` tag.
The tag to create an I2C read job is named `<I2CPortRead>`.
With this job, a single I2C read command can be triggered.

These are the two mandatory attributes for `<I2CPortRead>`:

 - Length="..."
	 - The number of bytes to be read.
- Address="..."
	- The I2C slave address
	- The lowest bit (read/write) is not part of this address (so shift right by one bit).
	- If addressing in hexadecimal notation is intended, add a leading `0x` before the value. Otherwise, it will be interpreted in decimal notation.

This is an optional attribute:
 - Timeout="..."
	 - Time in milliseconds
	 - If not set, 1000 ms is used as default.
	 - Reduce this value if an I2C device is optional and the system shall not wait for it to appear.

 Example: Read eight bytes from an I2C slave with address 0x10:
```xml
<I2CPortRead Length="8" Address="0x10"/>
```

To get the result and use the received data to trigger further action, the code of UNICENS daemon needs to be adjusted. Inspect the callback function "UCSI_CB_OnI2CRead" for this purpose:

```C
void UCSI_CB_OnI2CRead(void *pTag, bool success, uint16_t inicNetNodeAddress, uint8_t i2cSlaveAddr, const uint8_t *pBuffer, uint32_t bufLen)
{ }
```

**10.4) Defining a GPIO Create Job**

In order to enable the usage of remote GPIO, the port needs to be created first with the `<GPIOPortCreate>` tag.

It only has one mandatory attribute called _DebounceTime_:

 - DebounceTime=".."
	 - Specifies the timeout for the GPIO debounce timer (in ms)
	 - Each pin is debounced with its own timer that starts to count on every pin event.
	 - This suppresses unintended events from push buttons, as they usually flicker for a short time on press and release.

 Example: Create a GPIO Port with the debouncing filter set to 20 ms:

```xml
<GPIOPortCreate DebounceTime="20"/>
```

**10.5) Defining a GPIO Pin Mode Job**

In order to use this job, make sure that the GPIO Port has already been created by the `<GPIOPortCreate>` tag.
The tag to be used is named `<GPIOPortPinMode>`.
It only has one mandatory attribute called _PinConfiguration_:

 - PinConfiguration=".."
 	 - Hexadecimal array of bytes
	 - The notation is without leading 0x.
	 - Each byte is represented by two nibbles [0...F].
	 - Each byte (except the last one) is separated by a trailing space.
	 - For every GPIO pin to be defined (just skip unused pins) add two bytes to the array:
		 - First byte: The GPIO pin number (from 0 for GPIO 0 to 8 for GPIO 8)
		 - Second byte: The pin configuration byte, which must be chosen from the table below:

| Valid Values | Mnemonic                               |
|--------------|----------------------------------------|
| 0x00         | Unavailable                            |
| 0x01         | Unused                                 |
| 0x10         | Input                                  |
| 0x11         | InputStickyHighLevel                   |
| 0x12         | InputStickyLowLevel                    |
| 0x13         | InputTriggerRisingEdge                 |
| 0x14         | InputTriggerFallingEdge                |
| 0x15         | InputTriggerRisingFallingEdge          |
| 0x16         | InputTriggerHighLevel                  |
| 0x17         | InputTriggerLowLevel                   |
| 0x30         | InputDebounced                         |
| 0x33         | InputDebouncedTriggerRisingEdge        |
| 0x34         | InputDebouncedTriggerFallingEdge       |
| 0x35         | InputDebouncedTriggerRisingFallingEdge |
| 0x36         | InputDebouncedTriggerHighLevel         |
| 0x37         | InputDebouncedTriggerLowLevel          |
| 0x40         | OutputDefaultLow                       |
| 0x41         | OutputDefaultHigh                      |
| 0x50         | OutputOpenDrain                        |
| 0x53         | OutputOpenDrainTriggerRisingEdge       |
| 0x54         | OutputOpenDrainTriggerFallingEdge      |
| 0x56         | OutputOpenDrainTriggerHighLevel        |
| 0x57         | OutputOpenDrainTriggerLowLevel         |

 Example: Configure GPIO Pin Modes:

```xml
<GPIOPortPinMode PinConfiguration="03 35 07 41 08 40"/>
```

In the example above, there are three GPIO pins configured:

 - GPIO Pin 3 is set to `InputDebouncedTriggerRisingFallingEdge`: It is a debounced input and will report network events on rising and falling edges.
 - GPIO Pin 7 is set to `OutputDefaultHigh`: It is an output. The initial state of the output is high level.
 - GPIO Pin 8 is set to `OutputDefaultLow`: It is an output. The initial state of the output is low level.
 - GPIO Pins 0, 1, 2, 4, 5, 6 will remain in an unused state.

To get the result and use the received events from the input pins, the code of UNICENS daemon needs to be adjusted. Inspect the callback function `UCSI_CB_OnGpioStateChange` for this purpose:

```C
void UCSI_CB_OnGpioStateChange(void *pTag, uint16_t inicNetNodeAddress, uint8_t gpioPinId, bool isHighState)
{ }
```

**10.6) Defining a GPIO Pin State job**

In order to use this job, make sure that the GPIO Port has already been created by `<GPIOPortCreate>`.
The tag to be used is named `<GPIOPinState>`.
The purpose of this function is to set the state of GPIOs configured as output pins. For all other configurations, this function has no influence.

The `<GPIOPinState>` tag has two mandatory attributes:

 - Mask =".."
 	 - This value is a bit mask (unsigned 16 bit).
 	 - Each bit addresses one GPIO pin (starting with bit 0 for GPIO 0 and ending with bit 15 for GPIO 15).
 	 - This attribute selects which GPIO output pin(s) shall be adjusted.
 	 - Each bit has the meaning: 0 = Untouched; 1 = Change the state of the Pin according to the Value attribute (see next).

 - Data=".."
 	 - This value is a bit mask (unsigned 16 bit).
 	 - Each bit addresses one GPIO pin (starting with bit 0 for GPIO 0 and ending with bit 15 for GPIO 15).
 	 - This attribute selects the new state of the GPIO output pin(s). Make sure this is also reflected by the corresponding Mask attribute, mentioned earlier.
 	 - Each bit has the meaning: 0 = Set to Low state (if Mask matches); 1 = Set to High state (if Mask matches).

```xml
<GPIOPinState Mask="0x100" Data="0x100"/>
```

In the example above, the GPIO 8 (0x100 == 1 << 8) is set to High state.

**11.) Working with Drivers**

The UNICENS daemon may use the UNICENS XML description to configure the MOST Linux Driver on the fly.
The helper tool `xml2struct` can generate an offline default configuration out of the UNICENS XML description, which is then used for configuring the driver in a static way. Hence, the driver always comes up with an already working configuration, once the INIC is detected.

Having one configuration for all will automatically keep both UNICENS and the driver domain linked together. Otherwise, if the driver configuration does not align with the INIC configuration (for example FramesPerTransaction (See 5.1)), since the driver was configured manually, awful audio artifacts can occur.

A driver configuration can be assigned for each connection in the XML file. Therefor the `<SyncConnection>` and the `<AVPConnection>` tag have an optional attribute called _Driver_:
 - Driver=".."
	 - The value is any sort of user defined name, there is no syntax to be followed (other than to be XML complaint).
	 - This driver name acts as a reference.

The content of the driver is then embedded in a tag called `<Driver>`, which is a child of `<Unicens>` and has the same hierarchy level as `<Node>`.

The `<Driver>` tag has only one mandatory attribute called _Name_:
 - Name=".."
	 - This is the counter part of the Driver attribute of the `<SyncConnection>` or `<AVPConnection>` tag.
	 - If the names of the attributes are identical, then they are linked, and the driver settings will be applied.

A **non** working example (because of missing parameters) would be:
```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<SyncConnection Driver="Driver_HeadUnitTx"/>
	</Node>
	<Driver Name="Driver_HeadUnitTx"/>
</Unicens>
```

There can be child tags declared inside the Driver tag. They hold the actual information to setup the driver. Currently three kind of device drivers can be instanced and configured:

| XML Tag   | Mandatory Attributes                                                     | Usage                           |
|-----------|--------------------------------------------------------------------------|---------------------------------|
| `<Cdev>`  | Name, BufferSize, BufferCount                                            | Character Device                |
| `<V4l2>`  | Name, BufferSize, BufferCount                                            | Video For Linux 2 video capture |
| `<Alsa>`  | Name, BufferSize, BufferCount, AudioChannelCount, AudioChannelResolution | ALSA sound                      |

**11.1) Defining a CDEV Driver Instance**

In order to activate the character device driver, the `<Cdev>` tag is used. Once the driver is up and running, it will generate a virtual file in the */dev* folder in the target file system. This virtual file can be opened, read and written as any other file by a tool, such as *cat* or *dd* or in any programming language including C, C++, Java, Python.  However, seeking inside the character device is not supported, as the data is getting streamed and therefore has no random access possibility.
Whether the file supports reading or writing, depends on the role of the current connection in the INIC:
If a connection builds a sink (from network point of view), the associated CDEV supports reading. If the connection builds a source, the CDEV supports writing.
There is no protocol header involved. The first byte written to the CDEV is the first byte on the network. The same applies for the receiving part.

The `<Cdev>` tag  has three mandatory attributes:

 - Name=".."
	  - The given name will appear up in the /dev folder in the target file system. UNICENS or the `xml2struct` tool will add the prefix "inic-" in front of the given name to make sure that all INIC-related character devices are grouped in the directory.
	  - Name="hello" will end up in the character device `/dev/inic-hello`.

 - BufferSize=".."
	 - Integer value, specifying the number of bytes for a single buffer element
	 - This buffer will be allocated inside the driver and will temporary hold the streamed data.
	 - Making this value bigger, makes the application more robust, in cases where the operating systemÂ´s scheduler is slowly doing task switching, due to heavy system load.
	 - Reducing the buffer size may help to reduce the audio/video latency. But if the value is too small, the risk of data loss (Buffer-Underrun) increases, which may result in audio- and video artifacts.
	 - If the latency does not matter, using 8 Kbyte is usually safe.
	 - If the latency is crucial, it is generally hard to specify a "good" buffer size, as this depends a lot on the used CPU, operating system, kernel configuration and the typical system load. The integrator needs to find a good compromise between low latency and high robustness.

 - BufferCount=".."
	 - Integer Value, specifying the number of buffers (each with the size defined in attribute _BufferSize_).
	 - Having at least two buffers is recommended (double buffering).
	 - If the latency does not matter, using eight buffers is usually safe.
	 - If the latency is crucial, the integrator needs to tweak this value.

Example: Create a CDEV driver instance:
```xml
<Cdev Name="Sync-Tx" BufferSize="4096" BufferCount="4"/>
```

**11.2) Defining a Video for Linux Driver Instance**

In order to activate the Video for Linux 2 (V4L2) driver, the `<V4l2>` tag is used. It is limited to video reception (sink) use cases only.
It provides a video capture device, as known for webcams or TV tuners.
Doing so will add support for standard video players like VLC. The isochronous video stream on the network will be shown along with all other available capture devices. The playback of the content can then start without any further configuration of the player.
The V4L2 devices will also appear in the */dev* folder of the target file system. But all V4L2 devices will start with the prefix *video*, followed by an incrementing instance number.

The mandatory attributes are identical to the ones used for the CDEV driver (see section 11.1).

The only difference is the Name attribute. It is not reflected in the file name. Instead it can be accessed via the V4L2 ioctl API.

Example: Create a V4L2 driver instance
```xml
<V4l2 Name="RearView" BufferSize="7520" BufferCount="4"/>
```

As shown in the example above, a buffer size of 7520 bytes (40 * 188 bytes) is memory efficient and usually robust (if IsocPacketSize is set to 188 bytes).

**11.3) Defining an ALSA Driver Instance**

In order to activate the Advanced Linux Sound Architecture (ALSA) driver, the `<Alsa>` tag is used. It will either form an audio capture device or an audio playback device, depending if it is configured as a source or a sink.
Any standard Linux audio enabled tool (such as Audacity, mplayer, aplay, arecord, speaker-test) can instantly access the synchronous audio data channel on the network without any further configuration.
The ALSA subsystem also handles the sample rate conversion (if necessary), such as up-sampling from PCM 16 bit 44.1 kHz from the Bluetooth receiver to PCM 24 bit 48 kHz on INICnet technology.

If PulseAudio is available on the target, the ALSA driver instance will also be usable from that audio subsystem.

However, if the audio latency is crucial and the audio data shall not be influenced by the operation system, using CDEV (see section 11.1) instead of ALSA should be considered.

Three mandatory attributes are identical to the ones used for the CDEV driver (see section 11.1).
The only difference is the name attribute. It is not reflected in the file name. Instead, it can be accessed via the ALSA ioctl API and is visible in audio applications.
But there are two additional mandatory attributes:

 - AudioChannelCount=".."
	 - Integer number, describing the number of channels
	 - Must be at least *1*, meaning it is a mono channel
	 - Other well-known numbers (not limited to) are:
		 - *2* is a stereo channel.
		 - *6* is a 5.1 surround sound channel.
		 - *8* is a 7.1 surround sound channel.
		 - *9* is a 7.2 surround sound channel.
 - AudioChannelResolution=".."
	 - These are the allowed values (case- and space sensitive): *8bit*, *16bit*, *24bit*, *32bit*

Example: Setup an ALSA driver

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="unicens.xsd">
	<Node Address="0x200">
		<SyncConnection Driver="Driver_HeadUnitTx">
			<USBSocket EndpointAddress="0x1" FramesPerTransaction="128" />
			<NetworkSocket Bandwidth="4" Route="Route_HeadUnit" />
		</SyncConnection>
	</Node>
		<Driver Name="Driver_HeadUnitTx">
		<!-- Uncomment the following line to use character device (located in /dev/inic*) -->
		<!-- Cdev Name="HeadUnitTx" BufferSize="4096" BufferCount="4"/ -->

		<!-- Comment the following line to disable ALSA (use arecord and aplay or Audacity) -->
		<Alsa Name="HeadUnitTx" BufferSize="4096" BufferCount="4" AudioChannelCount="2" AudioChannelResolution="16bit" />
	</Driver>
</Unicens>
```

As shown in the example above, the API could theoretically provide multiple driver configurations inside the `<Driver>` tag. However, the driver is currently not ready to handle multiple device instances for a single connection. Therefore, make sure that only one configuration is enabled.

**12.) Adding Documentation**

Besides the possibility to place comment tags (`<!-- -->`) anywhere in the XML document, there are several optional attributes available for furhter documentation.
All of those attributes have no influence neither on the UNICENS daemon nor on the xml2struct tool.
But the UNICENS System Designer tool can apply additional checks based on those attributes. The tool also generates better graphical representations of the configuration, when the documentation attributes are filled in.

These are the optional documentation attributes:

| Tag            | Attribute                | Mnemonic                                                                     |
|----------------|--------------------------|------------------------------------------------------------------------------|
| Unicens        | Network                  | Enum: *INICnet-50*, *INICnet-150*, *MOST150*, used for bandwidth calculation |
| Unicens        | Name                     | Name of the XML document                                                     |
| Node           | Name                     | Name of the node, visible in tools graph                                     |
| Node           | Role                     | Enum: *Root*, *Slim*, *Smart*                                                |
| Node           | NetworkController        | Enum: *OS81118*, *OS81119*, *OS81210*, *OS81212*, *OS81214*, *OS81216*       |
| Node           | NetworkControllerVersion | The firmware version string (like *V2.4.0-76_RELEASE*)                       |

In addition to the attributes mentioned above, **all** tags have an optional attribute called _Description_.

 - Description=".."
	 - Any short string explaining the intention about that specific tag

Example: Commenting attributes

```xml
<?xml version="1.0"?>
<Unicens AsyncBandwidth="80"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xsi:noNamespaceSchemaLocation="unicens.xsd"
	Network="INICnet-150" Description="Sample Configuration">

	<Node Address="0x200" Name="HeadUnit"
		  Role="Root"
		  NetworkController="OS81118"
		  NetworkControllerVersion="V2.4.0-76_RELEASE"
		  Description="Vendor specific"/>
</Unicens>
```
