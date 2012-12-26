#!/usr/bin/perl

use Socket;
use File::Temp qw(tempfile);
use Gdigi;

$Usage = "Usage: gdigi.pl [get|set] <id> <pos> <val>
'val' is required for 'set', but ignored for 'get'.\n";

my $op = shift or die($Usage);
my $id = shift or die($Usage);
my $pos = shift or die($Usage);
my $val;

if ($op =~ /get/i) {
    $op = "get";
} elsif ($op =~ /set/i) {
    $op = "set";
    $val = shift;
    if (not defined $val) {
        die($Usage);
    }
} else {
    die($Usage);
}

if (not $id =~ /\d+/) {
    print STDERR "'$id' isn't a number.\n";
    die($Usage);
}

if (not $pos =~ /\d+/) {
    print STDERR "'$pos' isn't a number.\n";
    die($Usage);
}

if ($op eq "set" and not $val =~ /\d+/) {
    print STDERR "'$val' isn't a number.\n";
    die($Usage);
}

gdigi_init();


if ($op eq "get") {

    print "Getting parameter position $pos id $id\n";

    my $value = gdigi_get_parameter($id, $pos);
    if ($value) {
        print STDOUT "id $id pos $pos --> value $value\n";
    } else {
        print STDOUT "No value returned\n";
    }

} elsif ($op eq "set") {

    print "Setting parameter position $pos id $id to value $val\n";
    gdigi_set_parameter($id, $pos, $val);

}

gdigi_fini();

exit(0);
