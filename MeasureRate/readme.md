测速软件



## Install libpcap

Download `install.sh` and use `bash ./install.sh` to automatically install all dependencies.



## Compile speed.cpp

`g++ -o speed speed.cpp -lpcap`



## Useage

`sudo ./speed dev [filter operation] [interval=1000ms]`





## nstream.cpp

`sudo ./speed dev [filter operation] [interval=1000ms]`

能测任意多个流，只要修改常数就行了，及系统cpu可行。