#!/usr/bin/perl
use strict;
use Fcntl;
use Term::ReadKey;

my $LOOP_WAIT_TIME   = 0.100;

my $PIANOBAR_INPUT  = '/tmp/piano_pipe_in';
my $PIANOBAR_OUTPUT = '/tmp/piano_output';
my $DAEMON_CONTROL_CHAR = '%';

my $daemon_control_mode = 0;
my $daemon_control_buffer = '';

ReadMode 'cbreak';

print "before...\n";
open(OUT_FILE, "tail -f $PIANOBAR_OUTPUT |") or die "Error: Could not open output fifo ('$PIANOBAR_OUTPUT')\n";
print "after...\n";

my $done = 0;
MAIN_LOOP: 
while(!$done){
    while(defined (my $key = ReadKey(-1))) {
	if($key eq $DAEMON_CONTROL_CHAR){
	    $daemon_control_mode = 1;
	}

	if($daemon_control_mode){
	    if($key ne "\n"){
		$daemon_control_buffer .= $key;
		print $key; # echo the key in this mode
	    }else{
		if($daemon_control_buffer =~ m/^%q(uit)?$/i){
		    $done = 1;
		    next MAIN_LOOP;
		}
		send_command($daemon_control_buffer);
		$daemon_control_buffer = '';
		$daemon_control_mode = 0;
	    }
	}else{ 	## Normal mode - send each char individually
	    send_command($key);
	}
    }    
    while(defined (my $key = ReadKey(-1, \*OUT_FILE))){
	print $key;
    }
    sleep($LOOP_WAIT_TIME);
}


close(OUT_FILE);
ReadMode 'normal';

sub send_command {
    my($cmd) = @_;
    #print "Sending... '$cmd'\n";
    system("echo \"$cmd\" > $PIANOBAR_INPUT");
}
