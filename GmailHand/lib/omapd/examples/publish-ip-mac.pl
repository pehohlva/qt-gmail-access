#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper qw(Dumper);
use Fcntl qw(:flock);
use SOAP::Lite 
    # For debugging
    #+trace=>'all';
    ;

sub request_publish;
sub request_delete;
sub canonicalized_mac;

my $leaseFile = "/tmp/ifmap-leases";
my $lockFile  = "/tmp/ifmap-lease-lock";

my $leases;

my $type = shift;
my $ip   = shift;
my $mac  = shift;
my $ifmapServer = "https://127.0.0.1:8081";

# Fork so we can return control
# to the dhcp server ASAP.
my $pid = fork();

# If we're the child them
# attempt to publish this
# information to the MAP
# server. 
if($pid == 0) {
    # Wait for the lease lock
    open(LOCKFH, "> $lockFile");
    flock(LOCKFH, LOCK_EX);

    # Thaw the leases out
    if(-f $leaseFile) {
	$leases = do $leaseFile;
    }

    # Canonicalize the MAC address.
    if(defined($mac)) {
	$mac = canonicalized_mac($mac);
    }

    # If $leases->{$ip} eq $mac, then we're
    # already published, so do nothing
    # rather than beat on the MAP server.
    if($type eq "commit") {
	if(defined($leases->{$ip}) and defined($mac)) {
	    if($leases->{$ip} eq $mac) {
		print("IF-MAP Link already up-to-date.\n");
		exit(0);
	    } 
	}
    }

    my $soap = SOAP::Lite
	->readable(1)
	->proxy('https://127.0.0.1:8081');
        # Use the below with the WSDL interface.
        # and remove the proxy line.
	#->service('file:ifmap.wsdl')
	#->endpoint("https://127.0.0.1:8081");

    my $serializer = $soap->serializer();

    $serializer->register_ns('http://www.trustedcomputinggroup.org/2006/IFMAP/1', 'ifmap');

    $serializer->register_ns('http://www.trustedcomputinggroup.org/2006/IFMAP-METADATA/1', 'meta');

    my $newSession = SOAP::Data->name("ifmap:new-session");

    my $soapReply = $soap->call($newSession);
    
    my $sessionId   = $soapReply->valueof('//session-id');
    my $publisherId = $soapReply->valueof('//publisher-id');

    print("Session ID:   $sessionId\n");
    print("Publisher ID: $publisherId\n");

    my $sessionHeader = 
	SOAP::Header
	->name("ifmap:session-id")
	->value($sessionId);
    
    if($type eq "commit") {
	# Check to see if this IP is already published.
	if(defined($leases->{$ip}) and defined($mac)) {
	    # We have a stale lease in our
	    # cache, so send a delete request
	    if($leases->{$ip} ne $mac) {
		request_delete($sessionId, $ip, $leases->{$ip});
		
		delete $leases->{$ip};
	    }
	} 

	# Now publish the lease.
	request_publish($sessionId, $ip, $mac);
	
	# And update our cache.
	$leases->{$ip} = $mac;
    } elsif($type eq "release" or $type eq "expiry") {
	# If we weren't provided a MAC address
	# then pull it out of the leases cache
	# if it exists there.
	if(!defined($mac)) {
	    # This should probably only ever happen
	    # with the "expiry" event.
	    if(defined($leases->{$ip})) {
		$mac = canonicalized_mac($leases->{$ip});
	    } else {
		# Store something in $mac
		# since request_delete 
		# expects it.
		$mac = "00:00:00:00:00:00";
	    }	   
	}

	# Delete the session.
	request_delete($sessionId, $ip, $mac);	
	
	# Delete the cache entry.
	if(exists($leases->{$ip})) {
	    delete $leases->{$ip};
	}
    }

    # Save the updated lease cache.
    open(FH, "> $leaseFile");
    print FH Dumper($leases);
    close(FH);

    # Closing a locked file handle
    # unlocks it.
    close(LOCKFH);
}

sub request_publish {
    my ($sessionId, $ip, $mac) = @_;

my $publishCommand = <<END;
curl --insecure -X POST -H 'Content-type: text/xml' -d '<?xml version="1.0"?>
<env:Envelope 
  xmlns:env="http://www.w3.org/2003/05/soap-envelope" 
  xmlns:ifmap="http://www.trustedcomputinggroup.org/2006/IFMAP/1" 
  xmlns:meta="http://www.trustedcomputinggroup.org/2006/IFMAP-METADATA/1">
  <env:Header>
    <ifmap:session-id>$sessionId</ifmap:session-id>
  </env:Header>
  <env:Body>
    <ifmap:publish>
      <update>
        <link>
          <identifier>
            <mac-address value="$mac"/>
          </identifier>
          <identifier>
            <ip-address value="$ip" type="IPv4"/>
          </identifier>
        </link>
        <metadata>
          <meta:ip-mac cardinality="singleValue"/>
        </metadata>
      </update>
    </ifmap:publish>
  </env:Body>
</env:Envelope>' $ifmapServer > /dev/null 2> /dev/null
END
`$publishCommand`;
}

sub request_delete {
    my ($sessionId, $ip, $mac) = @_;

my $deleteCommand = <<END;
curl --insecure -X POST -H 'Content-type: text/xml' -d '<?xml version="1.0"?>
<env:Envelope 
  xmlns:env="http://www.w3.org/2003/05/soap-envelope" 
  xmlns:ifmap="http://www.trustedcomputinggroup.org/2006/IFMAP/1" 
  xmlns:meta="http://www.trustedcomputinggroup.org/2006/IFMAP-METADATA/1">
  <env:Header>
    <ifmap:session-id>$sessionId</ifmap:session-id>
  </env:Header>
  <env:Body>
    <ifmap:publish>
      <delete filter="meta:ip-mac">
        <link>
          <identifier>
            <mac-address value="$mac"/>
          </identifier>
          <identifier>
            <ip-address value="$ip" type="IPv4"/>
          </identifier>
        </link>
      </delete>
    </ifmap:publish>
  </env:Body>
</env:Envelope>' $ifmapServer > /dev/null 2> /dev/null
END
`$deleteCommand`;	
}

# Is there a way to do this automatically
# with ISC DHCP configuration?
sub canonicalized_mac {
    my ($mac) = @_;

    # Make sure the MAC address
    # is in canonical format.
    if(defined($mac)) {
	my @octets = split(/[:-_\.]/, $mac);
	
	if(scalar(@octets) != 6) {
	    print("MAC '$mac' has incorrect length: '", scalar(@octets),"'\n");

	    $mac = "00:00:00:00:00:00";

	    return $mac;
	}

	for(my $index = 0; $index < scalar(@octets); $index++) {
	    my $octetLength = length($octets[$index]);
	    
	    # Make sure we're lowercase
	    # as per the IF-MAP spec.
	    $octets[$index] =~ tr/[A-F]/[a-f]/;

	    # This takes care of verifying
	    # that we're the correct length
	    # and hexadecimal.
	    if($octets[$index] !~ /^[a-f0-9]{1,2}$/) {
		print("MAC '$mac' has incorrect case or length: '$octets[$index]'\n");
		
		$mac = "00:00:00:00:00:00";

		# We're done.
		return $mac;
	    }

	    # And finally, if we're 1 digit
	    # then prepend a 0.
	    if($octetLength == 1) {
		$octets[$index] = "0".$octets[$index];
	    }
	}

	$mac = join(':', @octets);
    }
    
    return $mac;
}
