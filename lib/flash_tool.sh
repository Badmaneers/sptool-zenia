 #!/bin/sh
 appname=`basename $0 | sed s/.sh$//g`

 dirname=`dirname $0`
 tmp="${dirname#?}"

 if [ "${dirname%$tmp}" != "/" ]; then
     dirname=$PWD/$dirname
 fi
 LD_LIBRARY_PATH=$dirname:$dirname/lib
 export LD_LIBRARY_PATH

 # Kernel 5.4+ CDC ACM rejects TIOCGSERIAL/TIOCSSERIAL.
 # The precompiled MTK library treats this as fatal.  The shim
 # intercepts those ioctls on /dev/ttyACM* and returns success.
 if [ -f "$dirname/lib/libpatch_brom.so" ]; then
     LD_PRELOAD="$dirname/lib/libpatch_brom.so"
     export LD_PRELOAD
 fi
 
 chmod +x $appname
 $dirname/$appname "$@"
