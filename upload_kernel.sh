#!/bin/bash
##################################################
# Author: Facundo Montero <facumo.fm@gmail.com>  #
##################################################
#                                                #
# This script is used for continous integration, #
# it requires 3 (three) environment variables.   #
#                                                #
# FTP_HOST for the FTP hostname or IP.           #
# FTP_USER for the FTP username.                 #
# FTP_PASS for the FTP password.                 #
#                                                #
##################################################
#                                                #
# If you aren't using Semaphore CI, you can pass #
# the missing values by exporting them in the    #
# environment.                                   #
#                                                #
# SEMAPHORE_BUILD_NUMBER for build id.           #
# BRANCH_NAME for branch name/target.            #
#                                                #
##################################################
if [ $? -eq 0 ]
then
 echo "Uploading.."
 DATE=$(date +"%d%m%Y")
 export $(cat Makefile | head -3 | sed 's/ //g')
 KERNEL_NAME="Linux_v""$VERSION"".""$PATCHLEVEL"".""$SUBLEVEL"
 DEVICE="$1"
 FINAL="$KERNEL_NAME"-"$DEVICE"-"$DATE"
 FINAL_ZIP="$FINAL"".zip"
 DONE_ZIP="$KERNEL_NAME"-"$DEVICE"-"$DATE"-"$BRANCH_NAME"-"$SEMAPHORE_BUILD_NUMBER"".zip"
 FILE="Anykernel2/$DEVICE/$FINAL_ZIP"
 echo "
  verbose
  open $FTP_HOST
  quote USER $FTP_USER
  quote PASS $FTP_PASS
  binary
  put $FILE /$DONE_ZIP
  put arch/arm/boot/zImage /zImage_$DEVICE
  bye
 " | /usr/bin/ftp -n
 echo "Build and upload completed."
 exit 0
else
 echo "Failed."
 exit 1
fi
