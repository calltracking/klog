all: klog_parser

klog_parser:  klog_parser.cc
	g++ -g -O2 -Wall -std=c++17 -o klog_parser klog_parser.cc

install: klog_parser
	cp klog_parser /usr/local/bin/
	cp klog.rb /usr/local/bin/
	chmod a+x /usr/local/bin/klog.rb
	ln -sf /usr/local/bin/klog.rb /usr/local/bin/klog

dtest: dtest.cc
	g++ -g -O2 -Wall -std=c++17 -o dtest dtest.cc

clean:
	rm -f klog_parser
