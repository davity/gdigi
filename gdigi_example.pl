#!/usr/bin/perl

use Gdigi qw(
        gdigi_init
        gdigi_get_parameter
        gdigi_set_parameter
);

my $op = shift;
my $pos = shift;
my $id = shift;
my $val = shift;

gdigi_init();
    
if ($op eq "set") {
    if (gdigi_set_parameter($pos, $id, $val) < 0) {
        print "Failed to set parameter id $id pos $pos val $val\n";
        exit(1);
    }

    print "Successfully set $id pos $pos value $val\n";

} elsif ($op eq "get") {
    if (gdigi_get_parameter($pos, $id, $val) < 0) {
        print "Failed to get parameter id $id pos $pos\n";
        exit(1);
    }

    print "Got value $val\n";
} else {
    Usage();
    exit(1);
}

exit(0);

sub Usage ()
{
    print "gdigi_example.pl get <pos> <id>
gdigi_example.pl set <pos> <id> <value>\n";
    
    return;
}
