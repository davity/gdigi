package Gdigi;

use 5.014002;
use strict;
use warnings;
use Carp;

require Exporter;
use AutoLoader;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Gdigi ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	GDIGI_API_GET_PARAMETER
	GDIGI_API_SET_PARAMETER
	gdigi_clear_debug
	gdigi_fini
	gdigi_get_parameter
	gdigi_init
	gdigi_set_debug
	gdigi_set_parameter
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	GDIGI_API_GET_PARAMETER
	GDIGI_API_SET_PARAMETER
);

our $VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.

    my $constname;
    our $AUTOLOAD;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "&Gdigi::constant not defined" if $constname eq 'constant';
    my ($error, $val) = constant($constname);
    if ($error) { croak $error; }
    {
	no strict 'refs';
	# Fixed between 5.005_53 and 5.005_61
#XXX	if ($] >= 5.00561) {
#XXX	    *$AUTOLOAD = sub () { $val };
#XXX	}
#XXX	else {
	    *$AUTOLOAD = sub { $val };
#XXX	}
    }
    goto &$AUTOLOAD;
}

require XSLoader;
XSLoader::load('Gdigi', $VERSION);

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Gdigi - Perl extension for managing a Digitech guitar effects processor.

=head1 SYNOPSIS

use Gdigi qw(
    gdigi_init
    gdigi_fini
    gdigi_get_parameter
    gdigi_set_parameter
    gdigi_set_debug
    gdigi_clear_debug
);


=head1 DESCRIPTION

First call gdigi_init() to initialize the module.

To retrieve a parameter from the device:

gdigi_get_paramter(id, position, value);

Returns -1 on failure, 0 on success.
'value' is used as an out parameter. I.e., if the function returns success,
the retrieved value will be in 'value'.

This API will block for no more than one second waiting for the device to
respond.

To set a parameter to a value:

gdigi_set_paramter(id, position, value);

Returns -1 on failure, 0 on succcess.

Call gdigi_fini() to cleanup module state.

There is a sample program gdigi.pl included with the gdigi source.

Calling gdigi_set_debug() will cause the module to print debugging information
to stderr.

Calling gdigi_clear_debug() will turn off debugging.

=head2 EXPORT

None by default.

=head2 Exportable constants

  GDIGI_API_GET_PARAMETER
  GDIGI_API_SET_PARAMETER

=head2 Exportable functions

  void gdigi_clear_debug()
  void gdigi_fini(void)
  gint gdigi_get_parameter(guint id, guint position, guint *value)
  gint gdigi_init(void)
  void gdigi_set_debug()
  gint gdigi_set_parameter(guint id, guint position, guint value)


=head1 SEE ALSO

There is a sample program gdigi.pl distributed with the gdigi source.

=head1 AUTHOR

Tim LaBerge, E<lt>tlaberge@visi.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Tim LaBerge

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
