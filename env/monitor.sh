#/bin/bash

#author:ShellAlbert.
#date:July 19,2018.
#this script is used to check the wifi hotspot by periodly.
#if the hotspot is not exist then restart it.
#

#export libraries.
LIB_QT=$PWD/libqt
LIB_OPENCV=$PWD/libopencv
LIB_ALSA=$PWD/libalsa
LIB_OPUS=$PWD/libopus
LIB_RNNOISE=$PWD/librnnoise
LIB_X264=$PWD/libx264
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_QT:$LIB_OPENCV:$LIB_ALSA:$LIB_OPUS:$LIB_RNNOISE:$LIB_X264

#start main app function.
function startMainApp()
{
    #check if os has framebuffer.
    FB=`ls -l /dev/fb* | wc -l`
    if [ $FB -gt 0 ];then
        #./AVLizard.bin  -plugin evdevmouse=/dev/input/event6
       ./AVLizard.bin  --DoNotCmpCamId
       #./AVLizard.bin  
    else
# ./AVLizard.bin  -plugin evdevmouse=/dev/input/event6
       ./AVLizard.bin  --DoNotCmpCamId
      # ./AVLizard.bin  
    fi
    return 0
}

#start function.
function startHotspot()
{
    nmcli connection add type wifi ifname '*' con-name AVLizard autoconnect no ssid "AVLizard(*)"
    nmcli connection modify AVLizard 802-11-wireless.mode ap 802-11-wireless.band bg ipv4.method shared
    nmcli connection modify AVLizard 802-11-wireless-security.key-mgmt wpa-psk 802-11-wireless-security.psk 12345678
    nmcli connection up AVLizard
    return 0
}

#stop function.
function stopHotspot()
{
    nmcli connection delete AVLizard
#    nmcli device disconnect wlan0
    return 0
}

#check wifi hotspot function.
function chkWifiHotspot()
{
    LIVESTR=`nmcli connection show -active | grep AVLizard`
    if [ -z "$LIVESTR" ];then
        echo "<error>:cannot detected AVLizard hotspot,restart it."
        stopHotspot
        sleep 5
        startHotspot
    else
        echo "<okay>:wifi hotspot okay."
    fi
    return 0
}

#check main program pid function.
function chkMainAppPid()
{
    if [ -f "/tmp/AVLizard.pid" ];then
        PID=`cat /tmp/AVLizard.pid`
        kill -0 $PID
        if [ $? -eq 0 ];then
            echo "<okay>:pid detect okay."
        else
            echo "<error>:pid detect error! schedule it."
            startMainApp
        fi
    else
        echo "<error>:pid file not exist! schedule it."
        startMainApp
    fi
    return 0

}

#the main entry.
#make sure we run this with root priority.
if [ `whoami` != "root" ];then
    echo "<error>: latch me with root priority please."
    exit -1
fi

#the main loop.
while true
do
	#make sure we have 2 cameras at least.
	CAM_NUM=`ls -l /dev/video* | wc -l`
	if [ $CAM_NUM -lt 2 ];then
   		 echo "<error>: I need 2 cameras at least."
		 sleep 5
	fi

    #check wifi hotspot.
    chkWifiHotspot

    #check the main app.
    chkMainAppPid

    #check periodly every 5 seconds.
    sleep 5
done

#the end of file.
