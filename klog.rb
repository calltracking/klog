# scan kamailio logs and report to statsd total number of invites and errors
require 'set'
require 'time'
require 'datadog/statsd'

verbose = (ARGV[1] == '-v')

KLogStats = Datadog::Statsd.new('127.0.0.1', 8125)

PROC_NAME='kamailio'
# sample invite line:
# Mar 12 03:29:38 ip-10-55-10-200 /usr/local/sbin/kamailio[31033]: INFO: {1 1 INVITE SJirZxdwez84q8Yz_12923954577010993509} <script>: >>> enter request_route
MATCHER = /\/usr\/local\/sbin\/kamailio\[\d+\]: (INFO|ERROR|NOTICE): {(.*)}/
LOG_MATCH = /\d+\s\d+\s(\w+)\s(.*)/
invites = Set.new
acks    = Set.new
errors  = Set.new
byes    = Set.new
flush_invites = Set.new
flush_acks    = Set.new
flush_errors  = Set.new
flush_byes    = Set.new
current_time = Time.now
current_year = current_time.year
offset_time = File.read("/var/log/klog.time").to_i rescue 0
puts "offset_time: #{offset_time.inspect}" if verbose
progress = 0
ts = nil
IO.foreach("/var/log/messages").each do |line|
  if line.scrub =~ MATCHER
    if $1 && $2
      level = $1
      log = $2
      time_parts = line.split(/\s/)
      ts = Time.parse("#{time_parts[0]} #{time_parts[1]} #{current_year} #{time_parts[2]}").to_i
      # ignore lines if they are before our last log time this allows us to run in a cron once per minute to collect and in batches
      if ts < offset_time
        if verbose
          if (progress % 1000) == 0
            printf("INVITES: ..., ACKS: ..., BYES: ..., ERRORS: ...,   SEEKING: %d lines\t\t\r", progress)
          end
        end
        progress+=1
        next
      else
        if (progress % 1000) == 0
          if verbose
            printf("INVITES: #{invites.size}, ACKS: #{acks.size}, BYES: #{byes.size}, ERRORS: #{errors.size}, processed: %d lines\t\t\r", progress)
          end
          invites.each {|id|
            next if flush_invites.include?(id)
            flush_invites << id
            KLogStats.increment("Kamailio.INVITE")
          }
          acks.each {|id|
            next if flush_acks.include?(id)
            flush_acks << id
            KLogStats.increment("Kamailio.ACK")
          }
          byes.each {|id|
            next if flush_byes.include?(id)
            flush_byes << id
            KLogStats.increment("Kamailio.BYE")
          }
          errors.each {|id|
            next if flush_errors.include?(id)
            flush_errors << id
            KLogStats.increment("Kamailio.ERROR")
          }

        end
        progress+=1
      end

      if log.scrub =~ LOG_MATCH
        status = $1
        call_id = $2
        if level == 'ERROR'
          errors << call_id
        else
          case status
          when 'INVITE' then invites << call_id
          when 'ACK' then acks << call_id
          when 'BYE' then byes << call_id
          end
        end
      end
    end
  end
end

invites.each {|id|
  next if flush_invites.include?(id)
  flush_invites << id
  KLogStats.increment("Kamailio.INVITE")
}
acks.each {|id|
  next if flush_acks.include?(id)
  flush_acks << id
  KLogStats.increment("Kamailio.ACK")
}
byes.each {|id|
  next if flush_byes.include?(id)
  flush_byes << id
  KLogStats.increment("Kamailio.BYE")
}
errors.each {|id|
  next if flush_errors.include?(id)
  flush_errors << id
  KLogStats.increment("Kamailio.ERROR")
}
puts "INVITES: #{invites.size}, ACKS: #{acks.size}, BYES: #{byes.size}, ERRORS: #{errors.size}"
File.open("/var/log/klog.time", "wb") {|f|
  f << ts
}
