package Gdigi;

use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(gdigi_init gdigi_fini
             gdigi_get_parameter gdigi_set_parameter
             gdigi_set_debug gdigi_clear_debug);


use Socket;
use File::Temp qw(tempfile);

$GDIGI_API_GET_PARAMETER = 1;
$GDIGI_API_SET_PARAMETER = 2;

my $server_name = "";
my $client_path = "";
my $Debug;

sub gdigi_set_debug () {
    $Debug = 1;
}

sub gdigi_clear_debug () {
    $Debug = 0;
}

sub debug {
    if ($Debug) {
        my $format = shift @_;
        printf STDERR $format, @_;
    }
}
        
        
sub gdigi_receive ($) {
    my $len = shift;
    my $msg;
    my $from;
    my $rin;

    $rin = '';
    vec($rin, fileno(SOCK), 1) = 1;

    ($nfound, $timeleft) = select($rin, undef, undef, 1);
    if (not $nfound) {
        print STDERR "Timeout on read.\n";
        return undef;
    }

    $from = recv(SOCK, $msg, $len, 0);

    debug("Received msg from $from.\n");

    return $msg;
}
    
sub gdigi_send ($) {
    my $msg = shift;
    my $n = send(SOCK, $msg, 0, $server_name);

    return $n;
}

    
sub gdigi_init () {
    
    # Setup the rendezvous point.
    $path = "/var/tmp/gdigi_sock";
    $server_name = sockaddr_un($path);

    # Get a socket.
    socket(SOCK, PF_UNIX, SOCK_DGRAM, 0) or die("socket: $!\n");

    # Bind a temporary filename to the socket for replies.
    ($client_fh, $client_path) = tempfile("gdigi_clientXXXX",
                                          DIR=> "/var/tmp",
                                          UNLINK => 1);
    $client_addr = sockaddr_un($client_path);
    unlink($client_path);
    bind(SOCK, $client_addr) or die("bind: $!\n");

}

sub pack_words {
    my @words;
    my $num_words;
  
    my $msg = pack("L"x @_, @_);

    return $msg;
}

$GDIGI_OP_GET_PARAMETER = 1;
    
sub gdigi_get_parameter ($$) {
    my $id = shift;
    my $pos = shift;
    my $op = $GDIGI_OP_GET_PARAMETER;

    my $msg = pack_words($op, $id, $pos);

    gdigi_send($msg);

    my $value = gdigi_receive(4);

    return unpack("L", $value);
}

    
sub gdigi_set_parameter ($$$) {
    my $id = shift;
    my $pos = shift;
    my $value = shift;
    my $op = $GDIGI_API_SET_PARAMETER;

    my $msg = pack_words($op, $id, $pos, $value);

    gdigi_send($msg);

}
sub gdigi_fini () {
    unlink $client_path;
    close(SOCK);
}


1;
