package GOSA::DBsqlite;


use strict;
use warnings;
use DBI;
use Data::Dumper;



sub new {
    my $object = shift;
    my $db_name = shift;

    my $obj_ref = {};
    bless($obj_ref,$object);
    my $dbh = DBI->connect("dbi:SQLite:dbname=$db_name");
    $obj_ref->{dbh} = $dbh;

    return($obj_ref);
}


sub create_table {
    my $object = shift;
    my $table_name = shift;
    my $col_names_ref = shift;
#    unshift(@{$col_names_ref}, "id INTEGER PRIMARY KEY AUTOINCREMENT");
    my $col_names_string = join(', ', @{$col_names_ref});
    my $sql_statement = "CREATE TABLE IF NOT EXISTS $table_name ( $col_names_string )"; 
    $object->{dbh}->do($sql_statement);
    return 0;
}
 


sub add_dbentry {

    my $obj = shift;
    my $arg = shift;

    # if dbh not specified, return errorflag 1
    my $table = $arg->{table};
    if (not defined $table) { 
        return 1; 
    }

    # specify primary key in table
    my $primkey = $arg->{primkey};

    # check wether value to primary key is specified
    if ( ( defined $primkey ) && ( not $arg->{ $primkey } ) ) {
        return 2;
    }

    # check wether primkey is unique in table, otherwise return errorflag 3
    if ( defined $primkey ) {
        my $res = @{ $obj->{dbh}->selectall_arrayref( "SELECT * FROM $table WHERE $primkey='$arg->{$primkey}'") };
        if ($res != 0) { 
            return 3;
        }
    }
  
    # fetch column names of table
    my $col_names = $obj->get_table_columns($table);
    
    # assign values to column name variables
    my @add_list;
    foreach my $col_name (@{$col_names}) {
        # use function parameter for column values
        if (exists $arg->{$col_name}) {
            push(@add_list, $arg->{$col_name});
        }
        # use default values for column values
#        } else {
#            my $default_val = "none";
#            if ($col_name eq "timestamp") {
#                $default_val = "19700101000000";
#            }             
#            push(@add_list, $default_val);
#        }
    }    

    my $sql_statement = " INSERT INTO $table VALUES ('".join("', '", @add_list)."')";
    print " INSERT INTO $table VALUES ('".join("', '", @add_list)."')\n";
    $obj->{dbh}->do($sql_statement);
    return 0;

}


# error-flags
# 1 no table ($table) defined
# 2 no restriction parameter ($restric_pram) defined
# 3 no restriction value ($restric_val) defined
# 4 column name not known in table
# 5 no column names to change specified
sub update_dbentry {
    my $obj = shift;
    my $arg = shift;

    # check completeness of function parameter
    # extract table statement from arg hash
    my $table = $arg->{table};
    if (not defined $table) {
        return 1;
    } else {
        delete $arg->{table};
    }
    # extract where parameter from arg hash
    my $restric_pram = $arg->{where};
    if (not defined $restric_pram) {
        return 2;
    } else {  
        delete $arg->{'where'};
    }
    # extrac where value from arg hash
    my $restric_val = $arg->{$restric_pram};
    if (not defined $restric_val) {
        return 3;
    } else {
        delete $arg->{$restric_pram};
    }

    # check wether table has all specified columns
    my $columns = {};
    my @res = @{$obj->{dbh}->selectall_arrayref("pragma table_info('$table')")};
    foreach my $column (@res) {
        $columns->{@$column[1]} = "";
    }
    my @pram_list = keys %$arg;
    foreach my $pram (@pram_list) {
        if (not exists $columns->{$pram}) {
            return 4;
        }
    }
    

    # select all changes
    my @change_list;
    my $sql_part;

    while (my($pram, $val) = each(%{$arg})) {
        push(@change_list, "$pram='$val'");
    }

    if (not @change_list) {
        return 5;
    }

    $obj->{dbh}->do("UPDATE $table SET ".join(', ',@change_list)." WHERE $restric_pram='$restric_val'");
    return 0;
}  


sub del_dbentry {
    my $obj = shift;
    my $arg = shift;

    # check completeness of function parameter
    # extract table statement from arg hash
    my $table = $arg->{table};
    if (not defined $table) {
        return 1;
    } else {
        delete $arg->{table};
    }
    # extract where parameter from arg hash
    my $restric_pram = $arg->{where};
    if (not defined $restric_pram) {
        return 2;
    } else {  
        delete $arg->{'where'};
    }
    # extrac where value from arg hash
    my $restric_val = $arg->{$restric_pram};
    if (not defined $restric_val) {
        return 3;
    } else {
        delete $arg->{$restric_pram};
    }
   
    # check wether entry exists
    my $res = @{$obj->{dbh}->selectall_arrayref( "SELECT * FROM $table WHERE $restric_pram='$restric_val'")};
    if ($res == 0) { 
        return 4;
    }

    $obj->{dbh}->do("DELETE FROM $table WHERE $restric_pram='$restric_val'");
 
    return 0;
}


sub get_table_columns {
    my $obj = shift;
    my $table = shift;

    my @columns;
    my @res = @{$obj->{dbh}->selectall_arrayref("pragma table_info('$table')")};
    foreach my $column (@res) {
        push(@columns, @$column[1]);
    }

    return \@columns; 
}

sub select_dbentry {
    my $obj = shift;
    my $arg = shift;

    # check completeness of function parameter
    # extract table statement from arg hash
    my $table = $arg->{table};
    if (not defined $table) {
        return 1;
    } else {
        delete $arg->{table};
    }

    # collect select statements
    my @select_list;
    my $sql_statement;
    while (my ($pram, $val) = each %{$arg}) {
        if ( $pram eq 'timestamp' ) {
            push(@select_list, "$pram < '$val'");
        } else {
            push(@select_list, "$pram = '$val'");
        }

    }
    if (@select_list == 0) {
        $sql_statement = "SELECT ROWID, * FROM '$table'";
    } else {
        $sql_statement = "SELECT ROWID, * FROM '$table' WHERE ".join(' AND ', @select_list);
    }

    # query db
    my $query_answer = $obj->{dbh}->selectall_arrayref($sql_statement);

    # fetch column list of db and create a hash with column_name->column_value of the select query
    my $column_list = &get_table_columns($obj, $table);    
    my $list_len = @{ $column_list } ;
    my $answer = {};
    my $hit_counter = 0;

    
    foreach my $hit ( @{ $query_answer }) {
        $hit_counter++;
        $answer->{ $hit_counter }->{ 'ROWID' } = shift @{ $hit };
        for ( my $i = 0; $i < $list_len; $i++) {
            $answer->{ $hit_counter }->{ @{ $column_list }[$i] } = @{ $hit }[$i];
        }
    }
    return $answer;  
}


sub show_table {
    my $obj = shift;
    my $table_name = shift;
    my @res = @{$obj->{dbh}->selectall_arrayref( "SELECT ROWID, * FROM $table_name")};
    my @answer;
    foreach my $hit (@res) {
        push(@answer, "hit: ".join(', ', @{$hit}));
    }
    return join("\n", @answer);
}


sub exec_statement {
    my $obj = shift;
    my $sql_statement = shift;
    my @res = @{$obj->{dbh}->selectall_arrayref($sql_statement)};
    return \@res;
}

1;
