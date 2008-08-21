           Bento4 
           ------
           
Bento4/AP4 is a C++ class library designed to read and write ISO-MP4 files. 
This format is defined in ISO/IEC 14496-12, 14496-14 and 14496-15. 
The format is a derivative of the Apple Quicktime file format. Because of that, 
Bento4 can be used to read and write a number of Quicktime files as well, 
even though some Quicktime specific features are not supported. In addition, 
Bento4 supports a number of extensions as defined in various other specifications. 
This includes some support for ISMA Encrytion and Decryption as defined in the 
ISMA E&A specification (http://www.isma.tv), OMA 2.0 and 2.1 DCF/PDCF Encryption and 
Decryption as defined in the OMA specifications 
(http://www.openmobilealliance.org) and iTunes compatible metadata. 
The SDK includes a number of command line tools, built using the class library, 
that serve as general purpose tools as well as examples of how to use the API.

The SDK is designed to be cross-platform. The code is very portable; it can be
compiled with any sufficiently modern C++ compiler. The code does not rely on
any external library; all the code necessary to compile the SDK and its tools is
included in the standard distribution. The standard distribution contains
makefiles for unix-like operating systems, including Linux, project files for
Microsoft Visual Studio, and an XCode project for MacOS X. There is also support
for building the library with the SCons build system.

You can find out more about the license in LICENSE.txt
You can also read some of the API documentation produced from the source
code comments using Doxygen. The doxygen output is available as a 
windows CHM file in Bento4.chm, and a set of HTML pages zipped together
in Bento4-HTML.zip (to start, open the file named index.html with an 
HTML browser)


