#!/usr/bin/env ruby

# scan kamailio logs and report to statsd total number of invites and errors
require 'set'
require 'time'
require 'fileutils'

pidfile_path="/var/run/klog.pid"
if File.exist?(pidfile_path)
  pid = File.read(pidfile_path).to_i
  is_running = `ps -ef | grep #{pid}`.strip.split("\n")
  if is_running.reject {|l| l.match?('grep') }.any?
    puts "currently running: #{is_running.inspect}"
    exit 1
  end
end
begin
File.open(pidfile_path, 'wb') { |f| f << Process.pid.to_s }

require 'datadog/statsd'
KLogStats = Datadog::Statsd.new('127.0.0.1', 8125)

verbose = (ARGV[0] == '-v')

klog_parser = File.expand_path(File.join(File.dirname(__FILE__),'klog_parser'))
output = `#{klog_parser}#{verbose ? ' -v' : ''}`.split("\n")

# INVITES: 68, ACKS: 95, BYES: 82, ERRORS: 6, CANCELS: 1, LINES: 639783
counts = output.last.split(",")
counts.each {|count|
  key, value = count.strip.split(":").map {|k| k.strip }
  value = value.to_i
  key.gsub!(/S/,'')
  if key == 'LINE'
    if value.to_i > 639783
      puts "trigger logrotate"
      puts `ls -lh /var/log/messages`
      system("/usr/sbin/logrotate -f /etc/logrotate.d/syslog")
      puts `ls -lh /var/log/messages`
    else
      puts "log size: #{value}"
    end
  else
    puts "increment: Kamailio.#{key} by #{value}"
    KLogStats.increment("Kamailio.#{key}", by: value)
    KLogStats.flush
  end
}
ensure
  FileUtils.rm(pidfile_path, force: true)
end
