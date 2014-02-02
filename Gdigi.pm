package Gdigi;

require Exporter;
@ISA = qw(Exporter);

use Net::DBus;
use Net::DBus::Object;
use Net::DBus::Dumper;

use Try::Tiny;

# This module implements the client side of the gdigi DBus client API.

@EXPORT = qw(gdigi_init gdigi_set_parameter gdigi_get_parameter);
my $gdigi_object;

# Connects to the Gdigi service on the session bus.
sub gdigi_init()
{
    my $gdigi_bus;

    try {
        $gdigi_bus = Net::DBus->session;
    } catch {
        warn("Failed to get session bus.\n");
        return -1;
    };

    my $gdigi_service;
    try {
        $gdigi_service = $gdigi_bus->get_service("gdigi.server");
    } catch {
        warn "Failed to get service gdigi.server.\n";
        return -1;
    };
        
    try {
        $gdigi_object = $gdigi_service->get_object("/gdigi/parameter/Object", "gdigi.parameter.io");
    } catch {
        warn "Failed to get gdigi object.\n";
        return -1;
    };

    return 0;
}

# Get a parameter.
sub gdigi_set_parameter($$$)
{
    my $pos = shift;
    my $id = shift;
    my $val = shift;
    my $ret = 0;

    try {
        $gdigi_object->set($pos, $id, $val);
    } catch {
        warn "Failed to set id $id pos $id value $val\n";
        $ret = -1;
    };

    return $ret;
}

# Set a parameter.
sub gdigi_get_parameter($$$)
{
    my $pos = shift;
    my $id = shift;
    my $val = \shift;

    my $ret = 0;
    try {
        $$val = $gdigi_object->get($pos, $id);
    } catch {
        warn "gdigi get of pos $pos id $id failed.\n";
        $ret = -1;
    };


    return $ret;
}

return 1;
