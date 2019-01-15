# Make GStreamer registry

export GST_REGISTRY=/usr/share/.gstreamer-1.0/registry.bin

/usr/bin/gst-inspect-1.0

chmod 666 /usr/share/.gstreamer-1.0/registry.bin

chown app:app /usr/share/.gstreamer-1.0 -R

chsmack -a "_" /usr/share/.gstreamer-1.0/registry.bin

chsmack -a "system::homedir" /usr/share/.gstreamer-1.0/
