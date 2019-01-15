#!/bin/sh

#--------------------------------------
#   isf
#--------------------------------------
export DISPLAY=:0.0
ISF_DEBUG=$1/isf
ISF_HOME=/home/app/.scim
ISF_SCIM_HOME=/root/.scim
ISF_DB=/opt/usr/dbspace
/bin/mkdir -p ${ISF_DEBUG}
/bin/cat ${ISF_HOME}/isf.log > ${ISF_DEBUG}/isf.log
/bin/cat ${ISF_HOME}/config > ${ISF_DEBUG}/config
/bin/cat ${ISF_HOME}/global > ${ISF_DEBUG}/global
/bin/cat ${ISF_SCIM_HOME}/isf.log > ${ISF_DEBUG}/scim.log
/bin/cp ${ISF_DB}/.ime_info.db* ${ISF_DEBUG}/
/bin/sync
