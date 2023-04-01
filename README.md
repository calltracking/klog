# Kamailio Log StatsD

Assuming your Kamailio server logs to syslog e.g. /var/log/messages this will (not efficiently) (but hopefully effectively)
report the following metrics for your datadlog or other statsd like services:

```
Kamailio.ACK
Kamailio.INVITE
Kamailio.BYE
Kamailio.ERROR
```

<img width="1403" alt="sample-dd-metrics" src="https://user-images.githubusercontent.com/1568/224875137-e3cc7631-61af-4f7c-be08-1181e2ee7da0.png">

# Install

copy klog.rb to /usr/local/bin
install ruby (tested on 3.x)
```
gem install dogstatsd-ruby
```

you will need a c++ compatible compiler
```
make
```

```
make install
```

every 1 minutes
```
crontab -e
* * * * * /usr/local/bin/ruby /usr/local/bin/klog.rb
```

It keeps track of the last log entry processed by time stamp by writing to 
```
/var/log/klog.time
```
