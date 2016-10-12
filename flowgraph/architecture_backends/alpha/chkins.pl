#!/usr/bin/perl
# simple perl script to make sure the instruction table and 
# opcode enum correspond.  If they don't the resultant binary will
# definately be very very broken.
# - Peter Morrow.

my @c;
my $i = 0;

foreach my $line (`cat diabloalpha_opcodes.c`) {
	if ( $line =~ m/\"(\w+)\"/ ) {
		$c[$i++] = $1;
	}
}

my $t;
$i = 0;

foreach my $line (`cat diabloalpha_opcodes.h`) {
  if ($line =~ m/ALPHA_(\w+),/ ) {
		print "Checking $c[$i] vs $1\n";
		if($c[$i++] !~ $1) {
			print "Mistake with $c[$i - 1] in diabloalpha_opcodes.c and $1 in diabloalpha_opcodes.h\n";
			exit 0;
		}
	}
}

$i = $i +1;
print "Everything appears to be OK, total # instruction = $i\n";

