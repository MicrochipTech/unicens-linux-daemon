<p align="center">
<img src="https://c1.staticflickr.com/5/4197/33948221753_6a3b102240_n.jpg">
<h1 align="center">UNICENS</h1>
<p align="center">
Unified Centralized Network Stack
</p>
</p>

<br>

## Introduction
UNICENS is a source code library written in C. It is a platform independent 
library that can be integrated into a bare-metal/mainloop-architecture as well as 
into any operating system.
<br><br>
UNICENS allows to startup and configure a MOST&reg; Network from one central node.
This means that only one node within the network must run UNICENS. It is able
to configure the shared ethernet bandwidth and establishes the dedicated audio and 
video channels without the necessity of an application interaction.
The result is, that applications residing on the network nodes can communicate
in a transparent way without having to care about the details *how* network related 
tasks are solved.
<br><br>
A UNICENS driven network allows the following ways of application communication:
* IP communication via MOST Ethernet Channel
* Remote communication via RemoteI2C or RemoteGPIO

<br>
The availibility of remote communication allows to design network nodes 
without needing a dedicated ECU that hosts an application. E.g., UNICENS hosted 
on the central node may modify codec specific values of a microphone node without ECU.
<br><br>
<p align="center">
<img src="https://c1.staticflickr.com/5/4247/34717065806_08347db3b4_b.jpg">
</p>

Please find further information on [www.microchip.com](http://www.microchip.com/design-centers/automotive/most/unicens).

## Documentation
Please download the related version and find the documentation in `/doc/ucs.html` or `/doc/ucs.chm`.
You can find the latest documentation [here](https://rawgit.com/MicrochipTech/unicens/master/doc/html/index.html).

## Related Projects
* [MOST Linux Driver](https://github.com/microchip-ais/linux)
* [UNICENS Binding for Automotive Grade Linux (AGL - Framework)](https://github.com/iotbzh/unicens2-binding)

## UNICENS on GitHub
This GitHub repository is used to publish UNICENS. Please be aware that UNICENS 
is not actively developed on GitHub.  

## Contribution, Maintenance and Support
Please do not send merge requests on GitHub. For bug reports, feature and support 
requests please contact [support-ais-de@microchip.com](mailto:support-ais-de@microchip.com).

## License
UNICENS source code is released under the [BSD License](https://github.com/MicrochipTech/unicens/blob/master/LICENSE).
