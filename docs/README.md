FastoTV &mdash; is a crossplatform tv player. <br />

How to use:
for starting player needed input file which you can specify via -i option (-i file://C:\msys64\home\Sasha\work\playlist.txt)
format of this file is next:

{"user":"alex", 
"urls" : [
{"id": 0 , "url": "rtmp://", "name": "tv1"},
{"id": 1 , "url": "rtmp://", "name": "tv2"},
{"id": 2 , "url": "http://", "name": "tv3"}
]
}

where user field for authorization, array of urls it is channels which will be played in players and name field how this channel will be displaying 

other options:
in mostly we using the same options like ffplay, but some was removed

tested OS:
Windows
Linux 
Linux ARM
MacOSX
FreeBSD
