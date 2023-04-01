#!/usr/bin/env ruby

# scan kamailio logs and report to statsd total number of invites and errors
require 'set'
require 'time'
require 'datadog/statsd'

is_running = `ps -ef | grep klog.rb | grep ruby`.strip.split("\n")
if is_running.size > 1
  parts = is_running.first.split(/\s/).reject{|p| p == '' }
  parts.shift
  pid = parts.first.to_i
  ppid = parts[1].to_i
  if Process.pid != pid && Process.pid != ppid
    puts "checked pid: #{Process.pid} != #{pid}"
    puts "locked and running: #{parts.inspect}\n#{is_running.inspect}"
    return
  end
end

KLogStats = Datadog::Statsd.new('127.0.0.1', 8125)

klog_parser = File.expand_path(File.join(File.dirname(__FILE__),'klog_parser'))
output = `#{klog_parser}`.split("\n")

# INVITES: 68, ACKS: 95, BYES: 82, ERRORS: 6
counts = output.last.split(",")
counts.each {|count|
  key, value = count.strip.split(":").map {|k| k.strip }
  value = value.to_i
  key.gsub!(/S/,'')
  puts "Kamailio.#{key}, #{value}"
  KLogStats.count("Kamailio.#{key}", value)
}
