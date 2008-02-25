package gosaTriggered;
use Exporter;
@ISA = qw(Exporter);
my @events = (
    "get_events", 
    "get_login_usr_for_client",
    "get_client_for_login_usr",
    "gen_smb_hash",
    "trigger_reload_ldap_config",
    "ping",
    "network_completition",
    "set_activated_for_installation",
    "new_key_for_client",
    "detect_hardware",
    "get_login_usr",
    "get_login_client",
    "trigger_action_localboot",
    "trigger_action_faireboot",
    "trigger_action_reboot",
    "trigger_action_halt",
    "trigger_action_update", 
    "trigger_action_reinstall",
    "trigger_action_memcheck", 
    "trigger_action_sysinfo",
    "trigger_action_instant_update",
    "trigger_action_rescan",
    "trigger_action_wake",
    );
@EXPORT = @events;

use strict;
use warnings;
use GOSA::GosaSupportDaemon;
use Data::Dumper;
use Crypt::SmbHash;
use Net::ARP;
use Net::Ping;
use Socket;
use utf8;

BEGIN {}

END {}

### Start ######################################################################

#&main::read_configfile($main::cfg_file, %cfg_defaults);

sub get_events {
    return \@events;
}


sub get_login_usr_for_client {
    my ($msg, $msg_hash, $session_id) = @_ ;
    my $header = @{$msg_hash->{'header'}}[0];
    my $source = @{$msg_hash->{'source'}}[0];
    my $target = @{$msg_hash->{'target'}}[0];
    my $client = @{$msg_hash->{'client'}}[0];

    $header =~ s/^gosa_//;

    my $sql_statement = "SELECT * FROM known_clients WHERE hostname='$client' OR macaddress='$client'";
    my $res = $main::known_clients_db->select_dbentry($sql_statement);

    my $out_msg = "<xml><header>$header</header><source>$target</source><target>$source</target>";
    $out_msg .= &db_res2xml($res);
    $out_msg .= "</xml>";

    my @out_msg_l = ( $out_msg );
    return @out_msg_l;
}


sub get_client_for_login_usr {
    my ($msg, $msg_hash, $session_id) = @_ ;
    my $header = @{$msg_hash->{'header'}}[0];
    my $source = @{$msg_hash->{'source'}}[0];
    my $target = @{$msg_hash->{'target'}}[0];
    my $usr = @{$msg_hash->{'usr'}}[0];

    my $sql_statement = "SELECT * FROM known_clients WHERE login LIKE '%$usr%'";
    my $res = $main::known_clients_db->select_dbentry($sql_statement);

    my $out_msg = "<xml><header>$header</header><source>$target</source><target>$source</target>";
    $out_msg .= &db_res2xml($res);
    $out_msg .= "</xml>";
    my @out_msg_l = ( $out_msg );
    return @out_msg_l;

}


sub ping {
     my ($msg, $msg_hash, $session_id) = @_ ;
     my $source = @{$msg_hash->{source}}[0];
     my $target = @{$msg_hash->{target}}[0];
     my $out_hash =  &create_xml_hash("ping", $source, $target);
     &add_content2xml_hash($out_hash, "session_id", $session_id);
     my $out_msg = &create_xml_string($out_hash);
    
     my @out_msg_l = ( $out_msg );
     return @out_msg_l;
}


sub gen_smb_hash {
     my ($msg, $msg_hash, $session_id) = @_ ;
     my $source = @{$msg_hash->{source}}[0];
     my $target = @{$msg_hash->{target}}[0];
     my $password = @{$msg_hash->{password}}[0];

     my %data= ('hash' => join(q[:], ntlmgen $password));
     my $out_msg = &build_msg("gen_smb_hash", $target, 'GOSA', \%data );
}


sub network_completition {
     my ($msg, $msg_hash, $session_id) = @_ ;
     my $source = @{$msg_hash->{source}}[0];
     my $target = @{$msg_hash->{target}}[0];
     my $name = @{$msg_hash->{hostname}}[0];

     # Can we resolv the name?
     my %data;
     if (inet_aton($name)){
	     my $address = inet_ntoa(inet_aton($name));
	     my $p = Net::Ping->new('tcp');
	     my $mac= "";
	     if ($p->ping($address, 1)){
	       $mac = Net::ARP::arp_lookup("", $address);
	     }

	     %data= ('ip' => $address, 'mac' => $mac);
     } else {
	     %data= ('ip' => '', 'mac' => '');
     }

     my $out_msg = &build_msg("network_completition", $target, 'GOSA', \%data );
    
     return ( $out_msg );
}


sub detect_hardware {
    my ($msg, $msg_hash) = @_ ;
    # just forward msg to client, but dont forget to split off 'gosa_' in header
    my $source = @{$msg_hash->{source}}[0];
    my $target = @{$msg_hash->{target}}[0];

    my $out_hash = &create_xml_hash("detect_hardware", $source, $target);
    my $out_msg = &create_xml_string($out_hash);

    my @out_msg_l = ( $out_msg );
    return @out_msg_l;

}


sub trigger_reload_ldap_config {
    my ($msg, $msg_hash) = @_ ;
    my $target = @{$msg_hash->{target}}[0];

    my $out_hash = &create_xml_hash("reload_ldap_config", $main::server_address, $main::server_address, $target);
    my $out_msg = &create_xml_string($out_hash);
    my @out_msg_l;
    push(@out_msg_l, $out_msg);
    return @out_msg_l;
}


sub set_activated_for_installation {
    my ($msg, $msg_hash) = @_;

    my $header = @{$msg_hash->{header}}[0];
    my $source = @{$msg_hash->{source}}[0];
    my $target = @{$msg_hash->{target}}[0];

    my $out_hash = &create_xml_hash("set_activated_for_installation", $source, $target);
    my $out_msg = &create_xml_string($out_hash);

    my @out_msg_l = ( $out_msg );
    return @out_msg_l;
}


sub trigger_action_faireboot {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_faireboot<\/header>/<header>trigger_action_faireboot<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_localboot {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_localboot<\/header>/<header>trigger_action_localboot<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_halt {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_halt<\/header>/<header>trigger_action_halt<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_reboot {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_reboot<\/header>/<header>trigger_action_reboot<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_memcheck {
    my ($msg, $msg_hash) = @_ ;
    $msg =~ s/<header>gosa_trigger_action_memcheck<\/header>/<header>trigger_action_memcheck<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_reinstall {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_reinstall<\/header>/<header>trigger_action_reinstall<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_update {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_update<\/header>/<header>trigger_action_update<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_instant_update {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_instant_update<\/header>/<header>trigger_action_instant_update<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_sysinfo {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_sysinfo<\/header>/<header>trigger_action_sysinfo<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub new_key_for_client {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_new_key_for_client<\/header>/<header>new_key<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_rescan {
    my ($msg, $msg_hash) = @_;
    $msg =~ s/<header>gosa_trigger_action_rescan<\/header>/<header>trigger_action_rescan<\/header>/;
    my @out_msg_l = ($msg);  
    return @out_msg_l;
}


sub trigger_action_wake {
    my ($msg, $msg_hash) = @_;
    my %data = ( 'macAddress'  => \@{$msg_hash->{macAddress}} );
    my $out_msg = &build_msg("trigger_wake", "GOSA", "KNOWN_SERVER", \%data);
    my @out_msg_l = ($out_msg);  
    return @out_msg_l;
}

1;
