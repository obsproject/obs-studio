IF EXIST filename.txt (
	curl -kLO https://obsproject.com/downloads/dependencies2015.zip -f --retry 5 -z dependencies2015.zip
 ) ELSE ( 
	curl -kLO https://obsproject.com/downloads/dependencies2015.zip -f --retry 5 -C -
 )
IF EXIST filename.txt (
	curl -kLO https://obsproject.com/downloads/vlc.zip -f --retry 5 -z vlc.zip
 ) ELSE ( 
	curl -kLO https://obsproject.com/downloads/vlc.zip -f --retry 5 -C -
 )
