# Kamailio Log StatsD

Assuming your Kamailio server logs to syslog e.g. /var/log/messages this will (not efficiently) (but hopefully effectively)
report the following metrics for your datadlog or other statsd like services:

```
Kamailio.ACK
Kamailio.INVITE
Kamailio.BYE
Kamailio.ERROR
```


# Install

copy klog.rb to /usr/local/bin
install ruby (tested on 3.x)

every 5 minutes
```
crontab -e
*/5 * * * * /usr/local/bin/ruby /usr/local/bin/klog.rb
```

It keeps track of the last log entry processed by time stamp by writing to 
```
/var/log/klog.time
```
